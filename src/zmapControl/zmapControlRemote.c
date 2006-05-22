/*  File: zmapControlRemote.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
 *-------------------------------------------------------------------
 * ZMap is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Implements code to respond to commands sent to a
 *              ZMap via the xremote program. The xremote program
 *              is distributed as part of acedb but is not dependent
 *              on it (although it currently makes use of acedb utils.
 *              code in the w1 directory).
 *              
 * Exported functions: See zmapControl_P.h
 * HISTORY:
 * Last edited: May 19 22:35 2006 (rds)
 * Created: Wed Nov  3 17:38:36 2004 (edgrif)
 * CVS info:   $Id: zmapControlRemote.c,v 1.24 2006-05-22 09:27:14 rds Exp $
 *-------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapXML.h>
#include <zmapControl_P.h>

/* For switch statements */
typedef enum {
  ZMAP_CONTROL_ACTION_ZOOM_IN = 1,
  ZMAP_CONTROL_ACTION_ZOOM_OUT,
  ZMAP_CONTROL_ACTION_ZOOM_TO,
  ZMAP_CONTROL_ACTION_CREATE_FEATURE,
  ZMAP_CONTROL_ACTION_ALTER_FEATURE,
  ZMAP_CONTROL_ACTION_DELETE_FEATURE,
  ZMAP_CONTROL_ACTION_FIND_FEATURE,  
  ZMAP_CONTROL_ACTION_HIGHLIGHT_FEATURE,
  ZMAP_CONTROL_ACTION_UNHIGHLIGHT_FEATURE,
  ZMAP_CONTROL_ACTION_REGISTER_CLIENT,

  ZMAP_CONTROL_ACTION_ADD_DATASOURCE,

  ZMAP_CONTROL_ACTION_UNKNOWN
} zmapControlAction;

typedef struct {
  unsigned long xid;
  GQuark request;
  GQuark response;
} controlClientObjStruct, *controlClientObj;

typedef struct
{
  ZMapWindowFToIQuery query;
  ZMapFeature feature;
} controlFeatureQueryStruct, *controlFeatureQuery;

typedef struct {
  zmapControlAction action;
  ZMapWindow window;

  controlClientObj  client;
  ZMapWindowFToIQuery set;   /* The current featureset */
  /* queries.... */
  GList *featureQueries_first;
  GList *featureQueries_last;

  GList *styles;                /* These should be ZMapFeatureTypeStyle */
  GList *locations;             /* This is just a list of ZMapSpan Structs */

} xmlObjectsDataStruct, *xmlObjectsData;

typedef struct
{
  ZMap zmap;
  int  code;
  gboolean persist;
} responseCodeZMapStruct, *responseCodeZMap;

/* ZMAPXREMOTE_CALLBACK and destroy internals for zmapControlRemoteInstaller */
static char *controlexecuteCommand(char *command_text, ZMap zmap, int *statusCode);
static void destroyNotifyData(gpointer destroy_data);


static gboolean createClient(ZMap zmap, controlClientObj data);
static ZMapXMLParser setupControlRemoteXMLParser(void *data);

static void drawNewFeature(gpointer data, gpointer userdata);
static void alterFeature(gpointer data, gpointer userdata);
static void deleteFeature(gpointer data, gpointer userdata);
static gboolean fillInFeature(controlFeatureQuery fq, ZMapWindow win);
/* xml handlers */
static gboolean zmapStrtHndlr(gpointer userdata, 
                              ZMapXMLElement zmap_element,
                              ZMapXMLParser parser);
static gboolean featuresetStrtHndlr(gpointer userdata, 
                                    ZMapXMLElement set_element,
                                    ZMapXMLParser parser);
static gboolean featureStrtHndlr(gpointer userdata, 
                                 ZMapXMLElement feature_element,
                                 ZMapXMLParser parser);
static gboolean featureEndHndlr(void *userData, 
                                ZMapXMLElement sub_element, 
                                ZMapXMLParser parser);
static gboolean clientStrtHndlr(gpointer userdata, 
                                ZMapXMLElement client_element,
                                ZMapXMLParser parser);
static gboolean locationEndHndlr(gpointer userdata, 
                             ZMapXMLElement zmap_element,
                             ZMapXMLParser parser);
static gboolean styleEndHndlr(gpointer userdata, 
                             ZMapXMLElement zmap_element,
                             ZMapXMLParser parser);
static gboolean zmapEndHndlr(gpointer userdata, 
                             ZMapXMLElement zmap_element,
                             ZMapXMLParser parser);



void zmapControlRemoteInstaller(GtkWidget *widget, gpointer zmap_data)
{
  ZMap zmap = (ZMap)zmap_data;
  zMapXRemoteObj xremote;

  if((xremote = zMapXRemoteNew()) != NULL)
    {
      Window id;
      zMapXRemoteNotifyData notifyData;

      id = (Window)zMapGetXID(zmap);

      notifyData           = g_new0(zMapXRemoteNotifyDataStruct, 1);
      notifyData->xremote  = xremote;
      notifyData->callback = ZMAPXREMOTE_CALLBACK(controlexecuteCommand);
      notifyData->data     = zmap_data; 

      /* Moving this (add_events) BEFORE the call to InitServer stops
       * some Xlib BadWindow errors (turn on debugging in zmapXRemote 2 c
       * them).  This doesn't feel right, but I couldn't bear the
       * thought of having to store a handler id for an expose_event
       * in the zmap struct just so I could use it over realize in
       * much the same way as the zmapWindow code does.  This
       * definitely points to some problem with GTK2.  The Widget
       * reports it's realized GTK_WIDGET_REALIZED(widget) has a
       * window id, but then the XChangeProperty() fails in the
       * zmapXRemote code.  Hmmm.  If this continues to be a problem
       * then I'll change it to use expose.  Only appeared on Linux. */
      /* Makes sure we actually get the events!!!! Use add_events as set_events needs to be done BEFORE realize */
      gtk_widget_add_events(widget, GDK_PROPERTY_CHANGE_MASK) ;
      
      zMapXRemoteInitServer(xremote, id, PACKAGE_NAME, ZMAP_DEFAULT_REQUEST_ATOM_NAME, ZMAP_DEFAULT_RESPONSE_ATOM_NAME);
      zMapConfigDirWriteWindowIdFile(id, zmap->zmap_id);

      g_signal_connect_data(G_OBJECT(widget), "property_notify_event",
                            G_CALLBACK(zMapXRemotePropertyNotifyEvent), (gpointer)notifyData,
                            (GClosureNotify) destroyNotifyData, G_CONNECT_AFTER
                            );
      
      zmap->propertyNotifyData = notifyData;
      
    }

  return;
}

/* ========================= */
/* ONLY INTERNALS BELOW HERE */
/* ========================= */

static void destroyNotifyData(gpointer destroy_data)
{
  ZMap zmap;
  zMapXRemoteNotifyData destroy_me = (zMapXRemoteNotifyData)destroy_data;

  zmap = destroy_me->data;
  zmap->propertyNotifyData = NULL; /* Set this to null, as we're emptying the mem */
  g_free(destroy_me);           /* Is this all we need to destroy?? */

  return ;
}

static ZMapXMLParser setupControlRemoteXMLParser(void *data)
{
  ZMapXMLParser parser = NULL;
  gboolean cmd_debug   = TRUE;
  ZMapXMLObjTagFunctionsStruct starts[] = {
    { "zmap",       zmapStrtHndlr,      },
    { "featureset", featuresetStrtHndlr },
    { "feature",    featureStrtHndlr    },
    { "client",     clientStrtHndlr     },
    { NULL, NULL }
  };
  ZMapXMLObjTagFunctionsStruct ends[] = {
    { "zmap",       zmapEndHndlr     },
    { "subfeature", featureEndHndlr  },
    { "location",   locationEndHndlr },
    { "style",      styleEndHndlr    },
    { NULL, NULL }
  };

  zMapAssert(data);

  parser = zMapXMLParserCreate(data, FALSE, cmd_debug);
  /*  zMapXMLParser_setMarkupObjectHandler(parser, start, end); */
  zMapXMLParserSetMarkupObjectTagHandlers(parser, &starts[0], &ends[0]);

  return parser;
}

static void alterFeature(gpointer data, gpointer userdata)
{
  
}

static void deleteFeature(gpointer data, gpointer userdata)
{ 
  FooCanvasItem *ftitm = NULL;
  ZMapFeature  feature = NULL;
  gboolean        good = TRUE, error_recorded = FALSE;
  ZMapWindowFToIQuery query = NULL;
  controlFeatureQuery  fq = (controlFeatureQuery)data;
  responseCodeZMap common = (responseCodeZMap)userdata;
  ZMap      zmap = NULL;
  ZMapWindow win = NULL;

  if(!common->persist)
    return ;
  zmap = common->zmap;
  win  = zMapViewGetWindow(zmap->focus_viewwindow);

  if((feature = fq->feature) == NULL)
    good = FALSE;
  if((query = fq->query) == NULL)
    good = FALSE;

  if(!good)
    {
      common->code = 412;
      zmapControlInfoSet(zmap, common->code,
                         "[CHANGE THIS] XML Data Error. Not enough information");
      error_recorded = TRUE;
    }

  if(good)
    {
      gboolean found = FALSE;

      query->query_type = ZMAP_FTOI_QUERY_FEATURE_ITEM;

      if((found = zMapWindowFToIFetchByQuery(win, query)) &&
         query->return_type == ZMAP_FTOI_RETURN_ITEM)
         ftitm = query->ans.item_answer;
      else if(!found)
        good = found;
      else
        good = FALSE;
    }

  if(good)
    good = zMapWindowFeatureRemove(win, ftitm);
  else if(!error_recorded)
    {
      common->code = 404;
      zmapControlInfoSet(zmap, common->code,
                         "[CHANGE THIS] Failed to find feature '%s'",
                         (char *)g_quark_to_string(query->originalId));
      error_recorded = TRUE;
    }
  
  if(!good && !error_recorded)
    {
      common->code = 404;
      zmapControlInfoSet(zmap, common->code,
                         "[CHANGE THIS] Failed to remove feature '%s'",
                         (char *)g_quark_to_string(query->originalId));
      error_recorded = TRUE;
    }

  common->persist = !error_recorded;

  return ; 
}
static void drawNewFeature(gpointer data, gpointer userdata)
{
  FooCanvasItem   *itm = NULL;
  FooCanvasGroup  *grp = NULL;
  ZMapFeature  feature = NULL;
  gboolean        good = TRUE, error_recorded = FALSE;
  ZMapWindowFToIQuery query = NULL;
  controlFeatureQuery  fq = (controlFeatureQuery)data;
  responseCodeZMap common = (responseCodeZMap)userdata;
  ZMap      zmap = NULL;
  ZMapWindow win = NULL;

  if(!common->persist)
    return ;

  zmap = common->zmap;
  win  = zMapViewGetWindow(zmap->focus_viewwindow);

  if((feature = fq->feature) == NULL)
    good = FALSE;
  if((query = fq->query) == NULL)
    good = FALSE;

  /* All the [CHANGE THIS] refers to the fact that we need to be
   * appending to the error message as we can have multiple
   * drawNewFeature calls.  This is after all a g_list_foreach func.
   * I'm only assuming here that we'll only get one call, but it's
   * written as a g_list_foreach to make it work, although debug (xremote)
   * unfriendly(!) with multiple features. It seems like a flaw in 
   * my whole info system as what we really want is a list of messages.
   * <xml>
   *   <error>
   *     <message>Something went wrong at point A</message>
   *     <message>Something went wrong at point C</message>
   *     <message>Something went wrong at point C</message>
   *     <message>Something went wrong at point D</message>
   *     <message>Something went wrong at point A</message>
   *   </error>
   * </xml>
   * NOT
   * <xml>
   *   <error>
   *     <message>Something went wrong at point ASomething went ...</message>
   *   </error>
   * </xml>
   * It makes the whole parsing thing _much_ more difficult.
   */

  if(!good)
    {
      common->code = 412;
      zmapControlInfoSet(zmap, common->code,
                         "[CHANGE THIS] XML Data Error. Not enough information");
      error_recorded = TRUE;
    }

  if(good)
    {
      gboolean found = FALSE;

      query->query_type = ZMAP_FTOI_QUERY_SET_ITEM;

      if((found = zMapWindowFToIFetchByQuery(win, query)) &&
         query->return_type == ZMAP_FTOI_RETURN_ITEM)
         grp = (FooCanvasGroup *)query->ans.item_answer;
      else if(!found)
        good = found;
      else
        good = FALSE;
    }

  if(good)
    {
      if((itm = zMapWindowFeatureAdd(win, grp, feature)) == NULL)
        good = FALSE;
    }
  else if(!error_recorded)
    {
      common->code = 412;
      zmapControlInfoSet(zmap, common->code,
                         "[CHANGE THIS] Failed to find column for feature '%s'",
                         (char *)g_quark_to_string(query->originalId));
      error_recorded = TRUE;
    }

  if(!good && !error_recorded)
    {
      common->code = 412;
      zmapControlInfoSet(zmap, common->code,
                         "[CHANGE THIS] Failed to draw feature '%s'",
                         (char *)g_quark_to_string(query->originalId));
      error_recorded = TRUE;
    }

  common->persist = !error_recorded;

  return ;
}

/* Handle commands sent from xremote. */
/* Return is string in the style of ZMAP_XREMOTE_REPLY_FORMAT (see ZMap/zmapXRemote.h) */
/* Building the reply string is a bit arcane in that the xremote reply strings are really format
 * strings...perhaps not ideal...., but best in the cicrumstance I guess */
static char *controlexecuteCommand(char *command_text, ZMap zmap, int *statusCode)
{
  char *xml_reply = NULL ;
  int code        = ZMAPXREMOTE_INTERNAL;
  xmlObjectsDataStruct objdata = {0, NULL};
  responseCodeZMapStruct listExecData = {};
  ZMapXMLParser parser = NULL;
  gboolean parse_ok    = FALSE;

  g_clear_error(&(zmap->info)); /* Clear the info */
  
  /* Create and setup the parser */
  parser = setupControlRemoteXMLParser(&objdata);
  objdata.window = zMapViewGetWindow(zmap->focus_viewwindow);
  /* Do the parsing and check all ok */
  if((parse_ok = zMapXMLParserParseBuffer(parser, 
                                          command_text, 
                                          strlen(command_text))) == TRUE)
    {
      /* Check which action  */
      switch(objdata.action){
        /* PRECOND falls through to default below, so info gets set there. */
      case ZMAP_CONTROL_ACTION_ZOOM_IN:
        if(zmapControlWindowDoTheZoom(zmap, 2.0) == TRUE)
          code = ZMAPXREMOTE_OK;
        else
          code = ZMAPXREMOTE_PRECOND;
        break;
      case ZMAP_CONTROL_ACTION_ZOOM_OUT:
        if(zmapControlWindowDoTheZoom(zmap, 0.5) == TRUE)
          code = ZMAPXREMOTE_OK;
        else
          code = ZMAPXREMOTE_PRECOND;
        break;
      case ZMAP_CONTROL_ACTION_ZOOM_TO:
        code = ZMAPXREMOTE_OK;
        break;
      case ZMAP_CONTROL_ACTION_FIND_FEATURE:
        
        code = ZMAPXREMOTE_OK;
        break;
      case ZMAP_CONTROL_ACTION_CREATE_FEATURE:
        /* need to check the feature doesn't exist already... */
        /* need to find the column.
         * To start with I'll assume just the one column
         * BUT it needs to do it for each query really!
         */
        listExecData.code = ZMAPXREMOTE_OK;
        listExecData.zmap = zmap;
        listExecData.persist = TRUE;
        g_list_foreach(objdata.featureQueries_first, drawNewFeature, &listExecData);
        zmapControlInfoSet(zmap, listExecData.code, "%s",
                           "Looks like I drew some features.");
        break;
      case ZMAP_CONTROL_ACTION_ALTER_FEATURE:
        listExecData.code = ZMAPXREMOTE_OK;
        listExecData.zmap = zmap;
        listExecData.persist = TRUE;
        g_list_foreach(objdata.featureQueries_first, alterFeature, &listExecData);
        zmapControlInfoSet(zmap, listExecData.code, "%s",
                           "Looks like I modified some features.");
        break;
      case ZMAP_CONTROL_ACTION_DELETE_FEATURE:
        listExecData.code = ZMAPXREMOTE_OK;
        listExecData.zmap = zmap;
        listExecData.persist = TRUE;
        g_list_foreach(objdata.featureQueries_first, deleteFeature, &listExecData);
        zmapControlInfoSet(zmap, listExecData.code, "%s",
                           "Looks like I deleted some features.");
        break;
      case ZMAP_CONTROL_ACTION_HIGHLIGHT_FEATURE:
        code = ZMAPXREMOTE_OK;
        break;
      case ZMAP_CONTROL_ACTION_UNHIGHLIGHT_FEATURE:
        code = ZMAPXREMOTE_OK;
        break;
      case ZMAP_CONTROL_ACTION_REGISTER_CLIENT:
        if(objdata.client != NULL)
          {
            if(createClient(zmap, objdata.client) == TRUE)
              code = ZMAPXREMOTE_OK;
            else
              code = ZMAPXREMOTE_BADREQUEST;
          }
        else
          code = ZMAPXREMOTE_BADREQUEST;
        break;
      default:
        code = ZMAPXREMOTE_INTERNAL;
        break;
      }
    }
  else if(!parse_ok)
    {
      code = ZMAPXREMOTE_BADREQUEST;
      zmapControlInfoSet(zmap, code,
                         "%s",
                         zMapXMLParserLastErrorMsg(parser)
                         );
    }
  /* Now check what the status is */
  /* zmap->info is tested for truth first in case a function called
     above set the info, we don't want to kill this information as it
     might be more useful than these defaults! */
  switch (code)
    {
    case ZMAPXREMOTE_INTERNAL:
      {
        code = ZMAPXREMOTE_UNKNOWNCMD;    
        zmapControlInfoSet(zmap, code,
                           "<!-- request was %s -->%s",
                           command_text,
                           "unknown command");
        break;
      }
    case ZMAPXREMOTE_BADREQUEST:
      {
        zmapControlInfoSet(zmap, code,
                           "<!-- request was %s -->%s",
                           command_text,
                           "bad request");
        break;
      }
    case ZMAPXREMOTE_FORBIDDEN:
      {
        zmapControlInfoSet(zmap, code,
                           "<!-- request was %s -->%s",
                           command_text,
                           "forbidden request");
        break;
      }
    default:
      {
        /* If the info isn't set then someone forgot to set it */
        zmapControlInfoSet(zmap, ZMAPXREMOTE_INTERNAL,
                          "<!-- request was %s -->%s",
                          command_text,
                          "CODE error on the part of the zmap programmers.");
        break;
      }
    }

  xml_reply   = g_strdup(zmap->info->message);
  *statusCode = zmap->info->code; /* use the code from the info (GError) */

  zMapXMLParserDestroy(parser);
  
  return xml_reply;
}


static gboolean createClient(ZMap zmap, controlClientObj data)
{
  gboolean result;
  zMapXRemoteObj client;
  char *format_response = "<client created=\"%d\" exists=\"%d\" />";

  if(!zmap->client){
    if((client = zMapXRemoteNew()) != NULL)
      {
        zMapXRemoteInitClient(client, data->xid);
        zMapXRemoteSetRequestAtomName(client, (char *)g_quark_to_string(data->request));
        zMapXRemoteSetResponseAtomName(client, (char *)g_quark_to_string(data->response));
        result = TRUE;
        zmapControlInfoSet(zmap, ZMAPXREMOTE_OK,
                           format_response, 1 , 1);
        zmap->client = client;
      }
    else
      {
        zmapControlInfoSet(zmap, ZMAPXREMOTE_OK,
                           format_response, 0, 0);
        result = FALSE;
      }
  }
  else{
    zmapControlInfoSet(zmap, ZMAPXREMOTE_OK,
                       format_response, 0, 1);
    result = FALSE;        
  }

  return result ;
}



static gboolean zmapStrtHndlr(gpointer userdata, 
                              ZMapXMLElement zmap_element,
                              ZMapXMLParser parser)
{
  ZMapXMLAttribute attr = NULL;
  xmlObjectsData   obj  = (xmlObjectsData)userdata;

  if((attr = zMapXMLElementGetAttributeByName(zmap_element, "action")) != NULL)
    {
      GQuark action = zMapXMLAttributeGetValue(attr);
      if(action == g_quark_from_string("zoom_in"))
        obj->action = ZMAP_CONTROL_ACTION_ZOOM_IN;
      else if(action == g_quark_from_string("zoom_out"))
        obj->action = ZMAP_CONTROL_ACTION_ZOOM_OUT;
      else if(action == g_quark_from_string("zoom_to"))
        obj->action = ZMAP_CONTROL_ACTION_ZOOM_TO;  
      else if(action == g_quark_from_string("find_feature"))
        obj->action = ZMAP_CONTROL_ACTION_FIND_FEATURE;
      else if(action == g_quark_from_string("create_feature"))
        obj->action = ZMAP_CONTROL_ACTION_CREATE_FEATURE;
      /********************************************************
       * delete and create will have to do
       * else if(action == g_quark_from_string("alter_feature"))
       * obj->action = ZMAP_CONTROL_ACTION_ALTER_FEATURE;
       *******************************************************/
      else if(action == g_quark_from_string("delete_feature"))
        obj->action = ZMAP_CONTROL_ACTION_DELETE_FEATURE;
      else if(action == g_quark_from_string("highlight_feature"))
        obj->action = ZMAP_CONTROL_ACTION_HIGHLIGHT_FEATURE;
      else if(action == g_quark_from_string("unhighlight_feature"))
        obj->action = ZMAP_CONTROL_ACTION_UNHIGHLIGHT_FEATURE;
      else if(action == g_quark_from_string("create_client"))
        obj->action = ZMAP_CONTROL_ACTION_REGISTER_CLIENT;
    }

  return FALSE;
}

static gboolean featuresetStrtHndlr(gpointer userdata, 
                                    ZMapXMLElement set_element,
                                    ZMapXMLParser parser)
{
  ZMapWindowFToIQuery query = NULL;
  ZMapXMLAttribute    attr  = NULL;
  xmlObjectsData input = (xmlObjectsData)userdata;

  if(input && !(query = input->set))
    query = input->set = zMapWindowFToINewQuery();

  /* Isn't this fun... */
  if((attr = zMapXMLElementGetAttributeByName(set_element, "align")))
    {
      query->alignId = zMapXMLAttributeGetValue(attr);
    }
  if((attr = zMapXMLElementGetAttributeByName(set_element, "block")))
    {
      query->blockId = zMapXMLAttributeGetValue(attr);
    }
  if((attr = zMapXMLElementGetAttributeByName(set_element, "set")))
    {
      query->columnId = zMapXMLAttributeGetValue(attr);
    }

  return FALSE;
}

static gboolean featureStrtHndlr(gpointer userdata, 
                                 ZMapXMLElement feature_element,
                                 ZMapXMLParser parser)
{
  xmlObjectsData    data = (xmlObjectsData)userdata;
  ZMapXMLAttribute attr  = NULL;
  controlFeatureQuery fq = g_new0(controlFeatureQueryStruct, 1);
  ZMapWindowFToIQuery new_query = NULL, query;

  zMapXMLParserCheckIfTrueErrorReturn(data->set == NULL,
                                      parser, 
                                      "feature tag not contained within featureset tag");
  
  query     = data->set;        /* Get current one */  
  /* Make a new one for this feature. */
  new_query = zMapWindowFToINewQuery(); 
  /* copy stuff accross that gets set in featuresetStrtHndlr */
  /* so it inherits data from parent */
  new_query->alignId = query->alignId;
  new_query->blockId = query->blockId;
  new_query->styleId = query->styleId;

  if((attr = zMapXMLElementGetAttributeByName(feature_element, "name")))
    {
      new_query->originalId = zMapXMLAttributeGetValue(attr);
    }
  if((attr = zMapXMLElementGetAttributeByName(feature_element, "style")))
    {
      char *style_name = NULL;
      style_name = (char *)g_quark_to_string(zMapXMLAttributeGetValue(attr));
      if(new_query->styleId == 0)
        new_query->styleId = zMapStyleCreateID(style_name);
    }
  if((attr = zMapXMLElementGetAttributeByName(feature_element, "start")))
    {
      new_query->start = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), 
                                (char **)NULL, 10);
    }
  if((attr = zMapXMLElementGetAttributeByName(feature_element, "end")))
    {
      new_query->end = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), 
                              (char **)NULL, 10);
    }
  if((attr = zMapXMLElementGetAttributeByName(feature_element, "strand")))
    {
      if((zMapFeatureFormatStrand((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)),
                                  &(new_query->strand))))
        query = query;
    }
  if((attr = zMapXMLElementGetAttributeByName(feature_element, "suid")))
    {
      /* Nothing done here yet. */
      new_query->suId = zMapXMLAttributeGetValue(attr);
    }

  fq->query   = new_query;

  /* Put this in a function like it was! And clean it up. */
  if(fillInFeature(fq, data->window))
    {
      if(data->featureQueries_first == NULL)
        {
          data->featureQueries_first = g_list_append(data->featureQueries_first, fq);
          data->featureQueries_last  = data->featureQueries_first;
        }
      else
        data->featureQueries_last = (g_list_append(data->featureQueries_last, fq))->next;
    }

  return FALSE;
}

static gboolean fillInFeature(controlFeatureQuery fq, ZMapWindow win)
{
  ZMapWindowFToIQuery query = NULL;
  ZMapFeatureTypeStyle   style = NULL;
  /* These are hardcoded ATM :( */
  ZMapFeatureType         type = ZMAPFEATURE_TRANSCRIPT;
  ZMapPhase              phase = 0;
  gboolean                good = FALSE;
  query = fq->query;

  if((query->styleId != 0) && 
     ((style = zMapFindStyle(zMapWindowFeatureAllStyles(win), 
                             query->styleId)) != NULL))
    {
      char *original_name = NULL;
      original_name = (char *)g_quark_to_string(query->originalId);
      query->type = type;
      fq->feature = zMapFeatureCreateFromStandardData(original_name,
                                                      "",
                                                      "",
                                                      query->type, 
                                                      style,
                                                      query->start, query->end,
                                                      FALSE, 0.0,
                                                      query->strand, phase);
      good = TRUE;
    }

  return good;
}
static gboolean featureEndHndlr(void *userData, 
                                ZMapXMLElement sub_element, 
                                ZMapXMLParser parser)
{
  ZMapXMLAttribute attr = NULL;
  xmlObjectsData   data = (xmlObjectsData)userData;
  ZMapFeature   feature = NULL;
  gboolean          bad = FALSE;

  zMapXMLParserCheckIfTrueErrorReturn(data->featureQueries_last == NULL, 
                                      parser, 
                                      "a feature end without a feature start.");

  if(!bad)
    feature = ((controlFeatureQuery)(data->featureQueries_last->data))->feature;

  if(!bad && (attr = zMapXMLElementGetAttributeByName(sub_element, "ontology")))
    {
      GQuark ontology = zMapXMLAttributeGetValue(attr);
      ZMapSpanStruct span = {0,0};
      ZMapSpan exon_ptr = NULL, intron_ptr = NULL;
      
      if((attr = zMapXMLElementGetAttributeByName(sub_element, "start")))
        {
          span.x1 = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), 
                           (char **)NULL, 10);
        }
      if((attr = zMapXMLElementGetAttributeByName(sub_element, "end")))
        {
          span.x2 = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), 
                           (char **)NULL, 10);
        }

      /* Don't like this lower case stuff :( */
      if(ontology == g_quark_from_string("exon"))
        exon_ptr   = &span;
      if(ontology == g_quark_from_string("intron"))
        intron_ptr = &span;
      if(ontology == g_quark_from_string("cds"))
        zMapFeatureAddTranscriptData(feature, TRUE, span.x1, span.x2, FALSE, 0, FALSE, NULL, NULL);

      if(exon_ptr != NULL || intron_ptr != NULL) /* Do we need this check isn't it done internally? */
        zMapFeatureAddTranscriptExonIntron(feature, exon_ptr, intron_ptr);

    }

  return TRUE;                  /* tell caller to clean us up. */
}

static gboolean clientStrtHndlr(gpointer userdata, 
                                ZMapXMLElement client_element,
                                ZMapXMLParser parser)
{
  controlClientObj clientObj = (controlClientObj)g_new0(controlClientObjStruct, 1);
  ZMapXMLAttribute xid_attr, req_attr, res_attr;
  if((xid_attr = zMapXMLElementGetAttributeByName(client_element, "id")) != NULL)
    {
      char *xid;
      xid = (char *)g_quark_to_string(zMapXMLAttributeGetValue(xid_attr));
      clientObj->xid = strtoul(xid, (char **)NULL, 16);
    }
  if((req_attr  = zMapXMLElementGetAttributeByName(client_element, "request")) != NULL)
    clientObj->request = zMapXMLAttributeGetValue(req_attr);
  if((res_attr  = zMapXMLElementGetAttributeByName(client_element, "response")) != NULL)
    clientObj->response = zMapXMLAttributeGetValue(res_attr);

  ((xmlObjectsData)userdata)->client = clientObj;

  return FALSE;
}

static gboolean locationEndHndlr(gpointer userdata, 
                                 ZMapXMLElement zmap_element,
                                 ZMapXMLParser parser)
{
  /* we only allow one of these objects???? */
  return TRUE;
}
static gboolean styleEndHndlr(gpointer userdata, 
                              ZMapXMLElement element,
                              ZMapXMLParser parser)
{
  ZMapFeatureTypeStyle style = NULL;
  ZMapXMLAttribute      attr = NULL;
  xmlObjectsData        data = (xmlObjectsData)userdata;
  GQuark id = 0;

  if((attr = zMapXMLElementGetAttributeByName(element, "id")))
    id = zMapXMLAttributeGetValue(attr);

  if(id && (style = zMapFindStyle(zMapWindowFeatureAllStyles(data->window), 
                                  id)) != NULL)
    {
#ifdef PARSINGCOLOURS
      if((attr = zMapXMLElementGetAttributeByName(element, "foreground")))
        gdk_color_parse(g_quark_to_string(zMapXMLAttributeGetValue(attr)),
                        &(style->foreground));
      if((attr = zMapXMLElementGetAttributeByName(element, "background")))
        gdk_color_parse(g_quark_to_string(zMapXMLAttributeGetValue(attr)),
                        &(style->background));
      if((attr = zMapXMLElementGetAttributeByName(element, "outline")))
        gdk_color_parse(g_quark_to_string(zMapXMLAttributeGetValue(attr)),
                        &(style->outline));
#endif
      if((attr = zMapXMLElementGetAttributeByName(element, "description")))
        style->description = (char *)g_quark_to_string( zMapXMLAttributeGetValue(attr) );
      
    }
  return TRUE;
}


static gboolean zmapEndHndlr(void *userData, 
                             ZMapXMLElement element, 
                             ZMapXMLParser parser)
{
  return TRUE;
}

