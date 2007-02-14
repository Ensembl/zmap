/*  File: zmapControlRemoteXML.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Feb 14 17:30 2007 (rds)
 * Created: Thu Feb  1 00:12:49 2007 (rds)
 * CVS info:   $Id: zmapControlRemoteXML.c,v 1.5 2007-02-14 17:31:48 rds Exp $
 *-------------------------------------------------------------------
 */

#include <zmapControlRemote_P.h>
#include <ZMap/zmapGLibUtils.h>

static gboolean setupStyles(ZMapFeatureSet set, ZMapFeature feature, 
                            GList *styles,      GQuark style_id);
/* xml event callbacks ... */
/* starts */
static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement zmap_element, 
                                  ZMapXMLParser parser);
static gboolean xml_featureset_start_cb(gpointer user_data, ZMapXMLElement set_element,
                                        ZMapXMLParser parser);
static gboolean xml_feature_start_cb(gpointer user_data, ZMapXMLElement feature_element,
                                     ZMapXMLParser parser);
static gboolean xml_client_start_cb(gpointer user_data, ZMapXMLElement client_element,
                                    ZMapXMLParser parser);
/* ends */
static gboolean xml_feature_end_cb(gpointer user_data, ZMapXMLElement sub_element, 
                                   ZMapXMLParser parser);
static gboolean xml_subfeature_end_cb(gpointer user_data, ZMapXMLElement sub_element, 
                                      ZMapXMLParser parser);
static gboolean xml_segment_end_cb(gpointer user_data, ZMapXMLElement sub_element, 
                                   ZMapXMLParser parser);
static gboolean xml_location_end_cb(gpointer user_data, ZMapXMLElement zmap_element,
                                    ZMapXMLParser parser);
static gboolean xml_style_end_cb(gpointer user_data, ZMapXMLElement zmap_element,
                                 ZMapXMLParser parser);
static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement zmap_element,
                                ZMapXMLParser parser);

/* 
 *
 * <zmap action="create_feature">
 *   <featureset>
 *     <feature name="RP6-206I17.6-001" start="3114" end="99885" strand="-" style="Coding">
 *       <subfeature ontology="exon"   start="3114"  end="3505" />
 *       <subfeature ontology="intron" start="3505"  end="9437" />
 *       <subfeature ontology="exon"   start="9437"  end="9545" />
 *       <subfeature ontology="intron" start="9545"  end="18173" />
 *       <subfeature ontology="exon"   start="18173" end="19671" />
 *       <subfeature ontology="intron" start="19671" end="99676" />
 *       <subfeature ontology="exon"   start="99676" end="99885" />
 *       <subfeature ontology="cds"    start="1"     end="2210" />
 *     </feature>
 *   </featureset>
 * </zmap>
 *
 * <zmap action="shutdown" />
 *
 * <zmap action="new">
 *   <segment sequence="U.2969591-3331416" start="1" end="" />
 * </zmap>
 *
 *  <zmap action="delete_feature">
 *    <featureset>
 *      <feature name="U.2969591-3331416" start="87607" end="87608" strand="+" style="polyA_site" score="0.000"></feature>
 *    </featureset>
 *  </zmap>
 *
 */

ZMapXMLParser zmapControlRemoteXMLInitialise(void *data)
{
  ZMapXMLParser parser = NULL;
  gboolean cmd_debug   = TRUE;
  ZMapXMLObjTagFunctionsStruct starts[] = {
    { "zmap",       xml_zmap_start_cb       },
    { "featureset", xml_featureset_start_cb },
    { "feature",    xml_feature_start_cb    },
    { "client",     xml_client_start_cb     },
    { NULL, NULL }
  };
  ZMapXMLObjTagFunctionsStruct ends[] = {
    { "zmap",       xml_zmap_end_cb       },
    { "feature",    xml_feature_end_cb    },
    { "segment",    xml_segment_end_cb    },
    { "subfeature", xml_subfeature_end_cb },
    { "location",   xml_location_end_cb   },
    { "style",      xml_style_end_cb      },
    { NULL, NULL }
  };

  zMapAssert(data);

  parser = zMapXMLParserCreate(data, FALSE, cmd_debug);

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &starts[0], &ends[0]);

  return parser;
}


/* INTERNALS */

static gboolean setupStyles(ZMapFeatureSet set, ZMapFeature feature, 
                            GList *styles,      GQuark style_id)
{
  ZMapFeatureTypeStyle style, set_style;
  gboolean got_style = TRUE;

  if(!(style = zMapFeatureGetStyle((ZMapFeatureAny)feature)))
    {
      if((style = zMapFindStyle(styles, style_id)))
        feature->style = style;
      else
        got_style = FALSE;
    }
  
  /* inherit styles. */
  if(!(set_style = zMapFeatureGetStyle((ZMapFeatureAny)set)))
    {
      if(got_style)
        set->style = style;
    }
  else if(!got_style)
    {
      feature->style = set_style;
      got_style = TRUE;
    }

  return got_style;
}

/* Handlers */

static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement zmap_element,
                                  ZMapXMLParser parser)
{
  ZMapXMLAttribute attr = NULL;
  XMLData   obj  = (XMLData)user_data;

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
      else if(action == g_quark_from_string("register_client") ||
              action == g_quark_from_string("create_client"))
        obj->action = ZMAP_CONTROL_ACTION_REGISTER_CLIENT;
      else if(action == g_quark_from_string("new_view"))
        obj->action = ZMAP_CONTROL_ACTION_NEW_VIEW;
    }
  else
    zMapXMLParserRaiseParsingError(parser, "action is a required attribute for zmap.");

  return FALSE;
}

static gboolean xml_featureset_start_cb(gpointer user_data, ZMapXMLElement set_element,
                                        ZMapXMLParser parser)
{
  ZMapXMLAttribute attr = NULL;
  ZMapFeatureAlignment align;
  ZMapFeatureBlock     block;
  ZMapFeatureSet feature_set;
  XMLData xml_data = (XMLData)user_data;
  GQuark align_id, block_id, set_id;
  char *align_name, *block_seq, *set_name;

  /* Isn't this fun... */
  if((attr = zMapXMLElementGetAttributeByName(set_element, "align")))
    {
      align_id   = zMapXMLAttributeGetValue(attr);
      align_name = (char *)g_quark_to_string(align_id);

      /* look for align in context. try master and non-master ids */
      if(!(align = zMapFeatureContextGetAlignmentByID(xml_data->context, zMapFeatureAlignmentCreateID(align_name, TRUE))) &&
         !(align = zMapFeatureContextGetAlignmentByID(xml_data->context, zMapFeatureAlignmentCreateID(align_name, FALSE))))
        {
          /* not there...create... */
          align = zMapFeatureAlignmentCreate(align_name, FALSE);
          zMapFeatureContextAddAlignment(xml_data->context, align, FALSE);
        }
      xml_data->align = align;
    }
  else
    {
      xml_data->align = xml_data->context->master_align;
    }

  if((attr = zMapXMLElementGetAttributeByName(set_element, "block")))
    {
      block_id  = zMapXMLAttributeGetValue(attr);
      block_seq = (char *)g_quark_to_string(block_id);
      /* This is blatantly wrong! */
      block_id  = zMapFeatureBlockCreateID(1, 1, ZMAPSTRAND_FORWARD,
                                           1, 1, ZMAPSTRAND_FORWARD);

      if(!(block = zMapFeatureAlignmentGetBlockByID(xml_data->align, block_id)))
        {
          /* As is this! */
          block = zMapFeatureBlockCreate(block_seq,
                                         1, 1, ZMAPSTRAND_FORWARD,
                                         1, 1, ZMAPSTRAND_FORWARD);
          zMapFeatureAlignmentAddBlock(xml_data->align, block);
        }
      xml_data->block = block;
    }
  else
    {
      /* Get the first one! */
      xml_data->block = zMap_g_datalist_first(&(xml_data->align->blocks));
    }

  if((attr = zMapXMLElementGetAttributeByName(set_element, "set")))
    {
      set_id   = zMapXMLAttributeGetValue(attr);
      set_name = (char *)g_quark_to_string(set_id);
      set_id   = zMapFeatureSetCreateID(set_name);
      if(!(feature_set = zMapFeatureBlockGetSetByID(xml_data->block, set_id)))
        {
          feature_set = zMapFeatureSetCreate(set_name, NULL);
          zMapFeatureBlockAddFeatureSet(xml_data->block, feature_set);
        }
      xml_data->feature_set = feature_set;
    }
  else
    {
      /* the feature handler will rely on the style to find the feature set... */
      xml_data->feature_set = NULL;
    }

  return FALSE;
}

static gboolean xml_feature_start_cb(gpointer user_data, ZMapXMLElement feature_element,
                                     ZMapXMLParser parser)
{
  ZMapXMLAttribute attr = NULL;
  XMLData      xml_data = (XMLData)user_data;
  ZMapFeatureAny feature_any;
  ZMapStrand strand = ZMAPSTRAND_NONE;
  GQuark feature_name_q = 0, style_q = 0, style_id;
  gboolean has_score = FALSE;
  char *feature_name, *style_name;
  int start = 0, end = 0;
  double score = 0.0;

  zMapXMLParserCheckIfTrueErrorReturn(xml_data->block == NULL,
                                      parser, 
                                      "feature tag not contained within featureset tag");
  
  if((attr = zMapXMLElementGetAttributeByName(feature_element, "name")))
    {
      feature_name_q = zMapXMLAttributeGetValue(attr);
    }
  else
    zMapXMLParserRaiseParsingError(parser, "name is a required attribute for feature.");

  if((attr = zMapXMLElementGetAttributeByName(feature_element, "style")))
    {
      style_q = zMapXMLAttributeGetValue(attr);
    }
  else
    zMapXMLParserRaiseParsingError(parser, "style is a required attribute for feature.");

  if((attr = zMapXMLElementGetAttributeByName(feature_element, "start")))
    {
      start = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), 
                                      (char **)NULL, 10);
    }
  else
    zMapXMLParserRaiseParsingError(parser, "start is a required attribute for feature.");

  if((attr = zMapXMLElementGetAttributeByName(feature_element, "end")))
    {
      end = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), 
                                      (char **)NULL, 10);
    }
  else
    zMapXMLParserRaiseParsingError(parser, "end is a required attribute for feature.");

  if((attr = zMapXMLElementGetAttributeByName(feature_element, "strand")))
    {
      zMapFeatureFormatStrand((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)),
                              &(strand));
    }

  if((attr = zMapXMLElementGetAttributeByName(feature_element, "score")))
    {
      score = zMapXMLAttributeValueToDouble(attr);
      has_score = TRUE;
    }

  if((attr = zMapXMLElementGetAttributeByName(feature_element, "suid")))
    {
      /* Nothing done here yet. */
      zMapXMLAttributeGetValue(attr);
    }

  if(!zMapXMLParserLastErrorMsg(parser))
    {
      style_name = (char *)g_quark_to_string(style_q);
      style_id   = zMapStyleCreateID(style_name);
      feature_name = (char *)g_quark_to_string(feature_name_q);

      if(!(xml_data->feature_set = zMapFeatureBlockGetSetByID(xml_data->block, style_id)))
        {
          xml_data->feature_set = zMapFeatureSetCreate(style_name , NULL);
          zMapFeatureBlockAddFeatureSet(xml_data->block, xml_data->feature_set);
        }
      
      if((xml_data->feature = zMapFeatureCreateFromStandardData(feature_name, NULL, "", 
                                                                ZMAPFEATURE_BASIC, NULL,
                                                                start, end, has_score,
                                                                score, strand, ZMAPPHASE_NONE)))
        {
          if(setupStyles(xml_data->feature_set, xml_data->feature, 
                         xml_data->styles, zMapStyleCreateID(style_name)))
            zMapFeatureSetAddFeature(xml_data->feature_set, xml_data->feature);
          else
            {
              char *error;
              error = g_strdup_printf("Valid style is required. Nothing known of '%s'.", style_name);
              zMapFeatureDestroy(xml_data->feature);
              zMapXMLParserRaiseParsingError(parser, error);
              g_free(error);
            }
        }

      if((feature_any = g_new0(ZMapFeatureAnyStruct, 1)))
        {
          feature_any->unique_id   = xml_data->feature->unique_id;
          feature_any->original_id = feature_name_q;
          feature_any->struct_type = ZMAPFEATURE_STRUCT_FEATURE;
          xml_data->feature_list   = g_list_prepend(xml_data->feature_list, feature_any);
        }
    }

  return FALSE;
}


static gboolean xml_client_start_cb(gpointer user_data, ZMapXMLElement client_element,
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
  else
    zMapXMLParserRaiseParsingError(parser, "id is a required attribute for client.");

  if((req_attr  = zMapXMLElementGetAttributeByName(client_element, "request")) != NULL)
    clientObj->request = zMapXMLAttributeGetValue(req_attr);
  if((res_attr  = zMapXMLElementGetAttributeByName(client_element, "response")) != NULL)
    clientObj->response = zMapXMLAttributeGetValue(res_attr);

  ((XMLData)user_data)->client = clientObj;

  return FALSE;
}

static gboolean xml_subfeature_end_cb(gpointer user_data, ZMapXMLElement sub_element, 
                                      ZMapXMLParser parser)
{
  ZMapXMLAttribute attr = NULL;
  XMLData   xml_data = (XMLData)user_data;
  ZMapFeature   feature = NULL;

  zMapXMLParserCheckIfTrueErrorReturn(xml_data->feature == NULL, 
                                      parser, 
                                      "a feature end tag without a created feature.");

  feature = xml_data->feature;

  if((attr = zMapXMLElementGetAttributeByName(sub_element, "ontology")))
    {
      GQuark ontology = zMapXMLAttributeGetValue(attr);
      ZMapSpanStruct span = {0,0};
      ZMapSpan exon_ptr = NULL, intron_ptr = NULL;
      
      feature->type = ZMAPFEATURE_TRANSCRIPT;

      if((attr = zMapXMLElementGetAttributeByName(sub_element, "start")))
        {
          span.x1 = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), 
                           (char **)NULL, 10);
        }
      else
        zMapXMLParserRaiseParsingError(parser, "start is a required attribute for subfeature.");

      if((attr = zMapXMLElementGetAttributeByName(sub_element, "end")))
        {
          span.x2 = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), 
                           (char **)NULL, 10);
        }
      else
        zMapXMLParserRaiseParsingError(parser, "end is a required attribute for subfeature.");

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

static gboolean xml_segment_end_cb(gpointer user_data, ZMapXMLElement segment, 
                                   ZMapXMLParser parser)
{
  XMLData xml_data = (XMLData)user_data;
  ZMapXMLAttribute attr = NULL;

  if((attr = zMapXMLElementGetAttributeByName(segment, "sequence")) != NULL)
    xml_data->new_view.sequence = zMapXMLAttributeGetValue(attr);
  if((attr = zMapXMLElementGetAttributeByName(segment, "start")) != NULL)
    xml_data->new_view.start = strtol(g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
  else
    xml_data->new_view.start = 1;
  if((attr = zMapXMLElementGetAttributeByName(segment, "end")) != NULL)
    xml_data->new_view.end = strtol(g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
  else
    xml_data->new_view.end = 0;

  /* Need to put contents into a source stanza buffer... */
  xml_data->new_view.config = zMapXMLElementStealContent(segment);
  
  return TRUE;
}

static gboolean xml_location_end_cb(gpointer user_data, ZMapXMLElement zmap_element,
                                    ZMapXMLParser parser)
{
  /* we only allow one of these objects???? */
  return TRUE;
}
static gboolean xml_style_end_cb(gpointer user_data, ZMapXMLElement element,
                                 ZMapXMLParser parser)
{
#ifdef RDS_FIX_THIS
  ZMapFeatureTypeStyle style = NULL;
  XMLData               data = (XMLData)user_data;
#endif
  ZMapXMLAttribute      attr = NULL;
  GQuark id = 0;

  if((attr = zMapXMLElementGetAttributeByName(element, "id")))
    id = zMapXMLAttributeGetValue(attr);

#ifdef PARSINGCOLOURS
  if(id && (style = zMapFindStyle(zMapViewGetStyles(zmap->focus_viewwindow), id)) != NULL)
    {
      if((attr = zMapXMLElementGetAttributeByName(element, "foreground")))
        gdk_color_parse(g_quark_to_string(zMapXMLAttributeGetValue(attr)),
                        &(style->foreground));
      if((attr = zMapXMLElementGetAttributeByName(element, "background")))
        gdk_color_parse(g_quark_to_string(zMapXMLAttributeGetValue(attr)),
                        &(style->background));
      if((attr = zMapXMLElementGetAttributeByName(element, "outline")))
        gdk_color_parse(g_quark_to_string(zMapXMLAttributeGetValue(attr)),
                        &(style->outline));
      if((attr = zMapXMLElementGetAttributeByName(element, "description")))
        style->description = (char *)g_quark_to_string( zMapXMLAttributeGetValue(attr) );
      
    }
#endif

  return TRUE;
}


static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement element, 
                                ZMapXMLParser parser)
{
  return TRUE;
}
static gboolean xml_feature_end_cb(gpointer user_data, ZMapXMLElement sub_element, 
                                   ZMapXMLParser parser)
{
  return TRUE;
}
