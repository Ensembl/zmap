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
 * Last edited: Jun 27 14:47 2005 (rds)
 * Created: Wed Nov  3 17:38:36 2004 (edgrif)
 * CVS info:   $Id: zmapControlRemote.c,v 1.10 2005-06-27 13:54:41 rds Exp $
 *-------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapUtils.h>
#include <zmapControl_P.h>


/* OK THERE IS A GENERAL PROBLEM HERE....WE NEED TO BE ABLE TO SEND COMMANDS TO A SPECIFIC WINDOW
   WITHIN THE ZMAP.......TRICKY....WE NEED THE WINDOW ID REALLY.... ok so we've got that

   THINK ABOUT THIS........we will need the current window and all that stuff.....
*/


/* These seem ok here.  At least they're all in one place then */
static char *controlexecuteCommand(char *command_text, ZMap zmap, int *statusCode);

static char *findFeature(ZMap zmap, char *command_text) ;
static gboolean createClient(ZMap zmap, char *command_text) ;

static void destroyNotifyData(gpointer destroy_data);

void zmapControlRemoteInstaller(GtkWidget *widget, gpointer zmap_data)
{
  ZMap zmap = (ZMap)zmap_data;
  zMapXRemoteObj xremote;

  if((xremote = zMapXRemoteNew()) != NULL)
    {
      Window id;
      zMapXRemoteNotifyData notifyData;

      id = (Window)GDK_DRAWABLE_XID(GTK_WIDGET(widget)->window);

      notifyData           = g_new0(zMapXRemoteNotifyDataStruct, 1);
      notifyData->xremote  = xremote;
      notifyData->callback = ZMAPXREMOTE_CALLBACK(controlexecuteCommand);
      notifyData->data     = zmap_data; 

      /* Moving this (add_events) BEFORE the call to InitServer stops
         some Xlib BadWindow errors (turn on debugging in zmapXRemote 2 c
         them).  This doesn't feel right, but I couldn't bear the
         thought of having to store a handler id for an expose_event
         in the zmap struct just so I could use it over realize in
         much the same way as the zmapWindow code does.  This
         definitely points to some problem with GTK2.  The Widget
         reports it's realized GTK_WIDGET_REALIZED(widget) has a
         window id, but then the XChangeProperty() fails in the
         zmapXRemote code.  Hmmm.  If this continues to be a problem
         then I'll change it to use expose.  Only appeared on Linux. */
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
/*  */
static void destroyNotifyData(gpointer destroy_data)
{
  ZMap zmap;
  zMapXRemoteNotifyData destroy_me = (zMapXRemoteNotifyData)destroy_data;

  zmap = destroy_me->data;
  zmap->propertyNotifyData = NULL; /* Set this to null, as we're emptying the mem */
  g_free(destroy_me);           /* Is this all we need to destroy?? */

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

  g_clear_error(&(zmap->info));

  //command prop = value ; prop = value ; prop = value
  /* We assume command is first word in text with no preceding blanks. */
  if (g_str_has_prefix(command_text, "zoom_in"))
    {
      if(zmapControlWindowDoTheZoomR(zmap, 2.0) == TRUE)
        code = ZMAPXREMOTE_OK;
      else
        code = ZMAPXREMOTE_PRECOND;
    }
  else if (g_str_has_prefix(command_text, "zoom_out"))
    {
      if(zmapControlWindowDoTheZoomR(zmap, 0.5) == TRUE)
        code = ZMAPXREMOTE_OK;
      else
        code = ZMAPXREMOTE_PRECOND;
    }
  else if (g_str_has_prefix(command_text, "feature_find"))
    {
      findFeature(zmap, command_text);
      code = ZMAPXREMOTE_OK;
    }
  else if (g_str_has_prefix(command_text, "register_client"))
    {
      if(createClient(zmap, command_text) == TRUE)
        code = ZMAPXREMOTE_OK;
      else
        code = ZMAPXREMOTE_BADREQUEST;
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
        zmap->info || (zmap->info = 
                       g_error_new(g_quark_from_string(__FILE__),
                                   code,
                                   "<!-- request was %s -->%s",
                                   command_text,
                                   "unknown command"
                                   ));
        break;
      }
    case ZMAPXREMOTE_BADREQUEST:
      {
        zmap->info || (zmap->info = 
                       g_error_new(g_quark_from_string(__FILE__),
                                   code,
                                   "<!-- request was %s -->%s",
                                   command_text,
                                   "bad request"
                                   ));
        break;
      }
    case ZMAPXREMOTE_FORBIDDEN:
      {
        zmap->info || (zmap->info = 
                       g_error_new(g_quark_from_string(__FILE__),
                                   code,
                                   "<!-- request was %s -->%s",
                                   command_text,
                                   "forbidden request"
                                   ));
        break;
      }
    default:
      {
        /* If the info isn't set then someone forgot to set it */
        if(!zmap->info)
          {
            code = ZMAPXREMOTE_INTERNAL;
            zmap->info || (zmap->info = 
              g_error_new(g_quark_from_string(__FILE__),
                          code,
                          "<!-- request was %s -->%s",
                          command_text,
                          "CODE error on the part of the zmap programmers."
                          ));
          }
        break;
      }
    }

  xml_reply = g_strdup(zmap->info->message);
  *statusCode = code;
  
  return xml_reply;
}


/* The feature_find command has the following format: 
 * 
 * "feature_find method = <method_id> ; type = <exon | homol | etc.> ; feature = <feature_name> ;
 *  strand = < C|F|+|-|. > ; q_start = 40 ; q_end = 387 ; t_start = 2613255 ; t_end = 2613598"
 * 
 *  "feature_find method = readpairs ; type = homol ; sequence = 20SNP45079-1505c12.p1c ;
 *   strand = C ; q_start = 40 ; q_end = 387 ; t_start = 2613255 ; t_end = 2613598"
 * 
 * um, actually not sure how generalistic we want this....should we cases for homols or exons or....
 * 
 *  */
static char *findFeature(ZMap zmap, char *command_text)
{
  char *result = NULL ;
  gboolean parse_error ;
  char *next ;
  char *keyword, *value ;
  gchar *style = NULL ;
  ZMapFeatureType feature_type = ZMAPFEATURE_INVALID ;
  char *feature_name = NULL ;
  ZMapStrand strand = ZMAPSTRAND_NONE ;
  int start = 0, end = 0, query_start = 0, query_end = 0 ;

  next = strtok(command_text, " ") ;			    /* Skip feature_find. */

  parse_error = FALSE ;
  while (!parse_error && (next = strtok(NULL, " ")))
    {
      keyword = g_strdup(next) ;			    /* Get keyword. */

      next = strtok(NULL, " ") ;			    /* skip "=" */

      next = strtok(NULL, " ") ;			    /* Get value. */
      value = g_strdup(next) ;

      next = strtok(NULL, " ") ;			    /* skip ";" */


      if (g_ascii_strcasecmp(keyword, "method") == 0)
	style = g_strdup(value) ;
      else if (g_ascii_strcasecmp(keyword, "type") == 0)
	{
	  /* we need a convertor from string -> SO compliant feature type here....needed in
	   * zmapGFF.c as well... */

	  if (g_ascii_strcasecmp(value, "homol") == 0)
	    feature_type = ZMAPFEATURE_HOMOL ;
	}
      else if (g_ascii_strcasecmp(keyword, "feature") == 0)
	feature_name = g_strdup(value) ;
      else if (g_ascii_strcasecmp(keyword, "strand") == 0)
	{
	  if (g_ascii_strcasecmp(value, "F") == 0
	      || g_ascii_strcasecmp(value, "+") == 0)
	    strand = ZMAPSTRAND_FORWARD ;
	  else if (g_ascii_strcasecmp(value, "C") == 0
		   || g_ascii_strcasecmp(value, "-") == 0)
	    strand = ZMAPSTRAND_REVERSE ;
	}
      else if (g_ascii_strcasecmp(keyword, "q_start") == 0)
	query_start = atoi(value) ;
      else if (g_ascii_strcasecmp(keyword, "q_end") == 0)
	query_end = atoi(value) ;
      else if (g_ascii_strcasecmp(keyword, "t_start") == 0)
	start = atoi(value) ;
      else if (g_ascii_strcasecmp(keyword, "t_end") == 0)
	end = atoi(value) ;
      else
	{
	  parse_error = TRUE ;
	  result = "";          /* XREMOTE_S_500_COMMAND_UNPARSABLE ; */
	}

      g_free(keyword) ;
      g_free(value) ;
    }


  if (!result)
    {
      if (style && *style && feature_type != ZMAPFEATURE_INVALID && feature_name && *feature_name
	  && strand
	  && start != 0 && end != 0
	  && (feature_type == ZMAPFEATURE_HOMOL && query_start != 0 && query_end != 0))
	{
	  ZMapWindow window ;
	  FooCanvasItem *item ;

	  window = zMapViewGetWindow(zmap->focus_viewwindow) ;

	  if ((item = zMapWindowFindFeatureItemByName(window, style, feature_type, feature_name,
						      strand, start, end, query_start, query_end))
	      && (zMapWindowScrollToItem(zMapViewGetWindow(zmap->focus_viewwindow), item)))
	    result = "";      /*  XREMOTE_S_200_COMMAND_EXECUTED ;  */

	  else
	    result = "";      /* XREMOTE_S_500_COMMAND_UNPARSABLE ; */
	}
      else
	{
	  result = "";        /* XREMOTE_S_500_COMMAND_UNPARSABLE ; */
	}
    }

  /* Clean up. */
  if (style)
    g_free(style) ;
  if (feature_name)
    g_free(feature_name) ;


  return result ;
}

static gboolean createClient(ZMap zmap, char *command_text)
{
  gboolean parse_error, result ;
  char *next ;
  char *keyword, *value ;
  gchar *type = NULL ;
  char *remote, *request, *response;
  zMapXRemoteObj client;
  remote = request = response = NULL;

  next = strtok(command_text, " ") ;			    /* Skip register_client. */

  result = parse_error = FALSE ;
  while (!parse_error && (next = strtok(NULL, " ")))
    {
      keyword = g_strdup(next) ;			    /* Get keyword. */

      next = strtok(NULL, " ") ;			    /* skip "=" */

      next = strtok(NULL, " ") ;			    /* Get value. */
      value = g_strdup(next) ;

      next = strtok(NULL, " ") ;			    /* skip ";" */


      if (g_ascii_strcasecmp(keyword, "id") == 0)
        {
          remote = g_strdup(value) ;
        }
      else if (g_ascii_strcasecmp(keyword, "request") == 0)
	{
          request = g_strdup(value) ;
	}
      else if (g_ascii_strcasecmp(keyword, "response") == 0)
        {
          response = g_strdup(value) ;
        }
      else
	{
	  parse_error = TRUE ;
	}

      g_free(keyword) ;
      g_free(value) ;
    }

  if (!parse_error && remote && request && response)
    {
      if((client = zMapXRemoteNew()) != NULL)
        {
          zMapXRemoteInitClient(client, strtoul(remote, (char **)NULL, 16));
          zMapXRemoteSetRequestAtomName(client, request);
          zMapXRemoteSetResponseAtomName(client, response);
          result = TRUE;
        }
    }
  /* Clean up. */
  if (remote)
    g_free(remote) ;
  if (request)
    g_free(request) ;
  if (response)
    g_free(response) ;
  



  return result ;
}
