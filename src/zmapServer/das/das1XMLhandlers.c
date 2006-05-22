/*  File: das1handlers.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: May 19 22:27 2006 (rds)
 * Created: Thu Sep  1 14:44:07 2005 (rds)
 * CVS info:   $Id: das1XMLhandlers.c,v 1.8 2006-05-22 09:30:12 rds Exp $
 *-------------------------------------------------------------------
 */

#include <dasServer_P.h>

static void setCurrentSegment(DasServer server, ZMapXMLElement segElement);
static ZMapFeatureSet makeZmapFeatureFromElement(ZMapXMLElement element,
                                                 GList *list);

gboolean dsnStart(void *userData,
                  ZMapXMLElement element,
                  ZMapXMLParser parser)
{
  return TRUE;
}
gboolean dsnEnd(void *userData,
                ZMapXMLElement element,
                ZMapXMLParser parser)
{
  DasServer server = (DasServer)userData;
  dasOneDSN dsn    = NULL;
  ZMapXMLElement sub_element = NULL;
  ZMapXMLAttribute attribute = NULL;
  GQuark id = 0, version = 0, name = 0, map = 0, desc = 0;
  gboolean handled = TRUE; 
  
  if((sub_element = zMapXMLElementGetChildByName(element, "source")) != NULL)
    {
      if((attribute = zMapXMLElementGetAttributeByName(sub_element, "id")) != NULL)
        id = zMapXMLAttributeGetValue(attribute);
      if((attribute = zMapXMLElementGetAttributeByName(sub_element, "version")) != NULL)
        version = zMapXMLAttributeGetValue(attribute);
      name = g_quark_from_string(sub_element->contents->str);
    }
  
  if((sub_element = zMapXMLElementGetChildByName(element, "mapmaster")) != NULL)
    map = g_quark_from_string(sub_element->contents->str);
  
  if((sub_element = zMapXMLElementGetChildByName(sub_element, "description")) != NULL)
    desc = g_quark_from_string(sub_element->contents->str);
  
  if((dsn = dasOneDSN_create1(id, version, name)) != NULL)
    {
      dasOneDSN_setAttributes1(dsn, map, desc);
      server->dsn_list = g_list_append(server->dsn_list, dsn);
      handled = TRUE;
    }
  else
    printf("Warning this needs to be written\n");
  
  return handled;
}


gboolean segStart(void *userData,
                  ZMapXMLElement element,
                  ZMapXMLParser parser)
{
  gboolean handled = FALSE;
  dasOneSegment seg = NULL;
  int start = 0, stop = 0, size = 0;
  char *ref = NULL, *sub = NULL;
  GQuark id = 0, type = 0, orientation = 0;
  ZMapXMLAttribute attr;

  orientation = g_quark_from_string("+"); /* optional and assumed positive */
  if((attr = zMapXMLElementGetAttributeByName(element, "id")))
    id = zMapXMLAttributeGetValue(attr);
  if((attr = zMapXMLElementGetAttributeByName(element, "class")))
    type = zMapXMLAttributeGetValue(attr);
  if((attr = zMapXMLElementGetAttributeByName(element, "orientation")))
    orientation = zMapXMLAttributeGetValue(attr);
  if((attr = zMapXMLElementGetAttributeByName(element, "start")))
    start = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
  if((attr = zMapXMLElementGetAttributeByName(element, "stop")))
    stop = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
  if((attr = zMapXMLElementGetAttributeByName(element, "size")))
    size = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
  
  if((attr = zMapXMLElementGetAttributeByName(element, "reference")))
    ref = (char *)g_quark_to_string( zMapXMLAttributeGetValue(attr) );
  if((attr = zMapXMLElementGetAttributeByName(element, "subparts")))
    sub = (char *)g_quark_to_string( zMapXMLAttributeGetValue(attr) );
  
  /* For Old skool */
  if((!start || !stop) && size)
    {
      start = 1;
      stop  = size;
    }
  
  if((seg = dasOneSegment_create1(id, start, stop, type, orientation)) != NULL)
    {
      dasOneSegment_refProperties(seg, ref, sub, TRUE);
    }

  handled = TRUE;

  return handled;
}

gboolean segEnd(void *userData,
                ZMapXMLElement element,
                ZMapXMLParser parser)
{
  DasServer server = (DasServer)userData;
  gboolean handled = FALSE;
#ifdef RDS_DONT_INCLUDE
  switch(server->type)
    {
    case ZMAP_DASONE_ENTRY_POINTS:
      {
        GList *list = (GList *)storage;
        if(list)
          {
            dasOneSegment seg = NULL;
            seg = (dasOneSegment)list->data;
            dasOneSegment_description(seg, element->contents->str );
          }
        handled = TRUE;
      }
      break;
    case ZMAP_DASONE_INTERNALFEATURES:
      if(server->req_context && !server->current_segment)
        { 
          GQuark seq  = server->req_context->sequence_name;
          GData *datalist    = NULL;
          dasOneFeature feat = NULL;
          printf("FIX  ME  NOW\n");

          if((feat = g_datalist_id_get_data(&(datalist), seq)) != NULL)
            setCurrentSegment(server, element);              
          else
            printf("F I X  M E  N O W\n");

        }
      handled = TRUE;
    default:
      break;
    }
#endif
  return handled;
}

gboolean featStart(void *userData,
                   ZMapXMLElement element,
                   ZMapXMLParser parser)
{
  DasServer server = (DasServer)userData;
  gboolean handled = FALSE;
#ifdef RDS_DONT_INCLUDE
  dasOneFeature feat    = NULL;
  ZMapXMLAttribute attr = NULL;
  GQuark id = 0, label  = 0;

  switch(server->type)
    {
    case ZMAP_DASONE_INTERNALFEATURES:
      if((attr = zMapXMLElementGetAttributeByName(element, "id")))
        id = zMapXMLAttributeGetValue(attr);
      if((attr = zMapXMLElementGetAttributeByName(element, "label")))
        label = zMapXMLAttributeGetValue(attr);

      feat = dasOneFeature_create1(id, label);
      //detail->key    = id;
      //detail->notify = rmInternalDasFeature;
      handled = TRUE;
    case ZMAP_DASONE_FEATURES:
      /* Don't think we actually need to do anything here... */
      break;
    case ZMAP_DAS_UNKNOWN:
    case ZMAP_DASONE_UNKNOWN:
    case ZMAP_DASONE_DSN:
    case ZMAP_DASONE_ENTRY_POINTS:
    case ZMAP_DASONE_DNA:
    case ZMAP_DASONE_SEQUENCE:
    case ZMAP_DASONE_TYPES:
    case ZMAP_DASONE_LINK:
    case ZMAP_DASONE_STYLESHEET:
    case ZMAP_DASTWO_UNKNOWN:
    default:
      break;
    }
#endif
  return handled;
}

gboolean featEnd(void *userData,
                 ZMapXMLElement element,
                 ZMapXMLParser parser)
{
  DasServer server = (DasServer)userData;
  gpointer output  = NULL;
  gboolean handled = TRUE;
#ifdef RDS_DONT_INCLUDE
  /* now we need to munge through the feature element */
  dasOneType type     = NULL;
  dasOneMethod method = NULL;
  dasOneTarget target = NULL;
  ZMapXMLElement ele  = NULL; /* temp 2 hold subelements */
  char *orientation   = "", *score = "", *phase = "", *methodStr = "";
  int start = 0, stop = 0;
  GQuark typeID = 0;
  GList *groups = NULL, *rawGroups = NULL;

  if((ele = zMapXMLElementGetChildByName(element, "start")))
    start = strtol(ele->contents->str, (char **)NULL, 10);
  if((ele = zMapXMLElementGetChildByName(element, "end")))
    stop = strtol(ele->contents->str, (char **)NULL, 10);
  if((ele = zMapXMLElementGetChildByName(element, "orientation")))
    orientation = ele->contents->str;
  if((ele = zMapXMLElementGetChildByName(element, "score")))
    score = ele->contents->str;
  if((ele = zMapXMLElementGetChildByName(element, "phase")))
    phase = ele->contents->str;


  if((ele = zMapXMLElementGetChildByName(element, "type")))
    {
      ZMapXMLAttribute attr = NULL;
      GQuark cat = 0, ref = 0, sub = 0, sup = 0;
      if((attr = zMapXMLElementGetAttributeByName(ele, "id")))
        typeID = zMapXMLAttributeGetValue(attr);
      if((attr = zMapXMLElementGetAttributeByName(ele, "category")))
        cat = zMapXMLAttributeGetValue(attr);
      if((attr = zMapXMLElementGetAttributeByName(ele, "reference")))
        ref = zMapXMLAttributeGetValue(attr);
      if((attr = zMapXMLElementGetAttributeByName(ele, "subparts")))
        sub = zMapXMLAttributeGetValue(attr);
      if((attr = zMapXMLElementGetAttributeByName(ele, "superparts")))
        sup = zMapXMLAttributeGetValue(attr);
      type = dasOneType_create1(typeID, g_quark_from_string(ele->contents->str), cat);
      dasOneType_refProperties(type, 
                               (char *)g_quark_to_string(ref),
                               (char *)g_quark_to_string(sub),
                               (char *)g_quark_to_string(sup),
                               TRUE);
    }
  if((ele = zMapXMLElementGetChildByName(element, "method")))
    {
      methodStr = ele->contents->str;
      method = dasOneMethod_create(methodStr);
    }
  if((ele = zMapXMLElementGetChildByName(element, "target")))
    {
      ZMapXMLAttribute attr = NULL;
      GQuark id = 0;
      int start = 0, stop = 0;
      if((attr = zMapXMLElementGetAttributeByName(ele, "id")))
        id = zMapXMLAttributeGetValue(attr);
      if((attr = zMapXMLElementGetAttributeByName(ele, "start")))
        start = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
      if((attr = zMapXMLElementGetAttributeByName(ele, "stop")))
        stop = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
      target = dasOneTarget_create1(id, start, stop);
    }

  if((rawGroups = zMapXMLElementGetChildrenByName(element, g_quark_from_string("group"), -1)))
    {
      while(rawGroups)
        {
          GQuark id = 0, type = 0, label = 0;
          ZMapXMLAttribute attr = NULL;
          dasOneGroup group = NULL;

          ele = (ZMapXMLElement)rawGroups->data;
          if((attr = zMapXMLElementGetAttributeByName(ele, "id")))
            id     = zMapXMLAttributeGetValue(attr);
          if((attr = zMapXMLElementGetAttributeByName(ele, "type")))
            type   = zMapXMLAttributeGetValue(attr);
          if((attr = zMapXMLElementGetAttributeByName(ele, "label")))
            label  = zMapXMLAttributeGetValue(attr);

          group     = dasOneGroup_create1(id, type, label);
          groups    = g_list_prepend(groups, group);
          rawGroups = rawGroups->next;
        }
    }

  if(storage != NULL)
    {
      GQuark id = 0, label = 0;
      ZMapXMLAttribute attr = NULL;
      dasOneFeature feature = NULL;
      GData *featuredata    = (GData *)storage;

      if((attr = zMapXMLElementGetAttributeByName(element, "id")))
        id = zMapXMLAttributeGetValue(attr);
      if((attr = zMapXMLElementGetAttributeByName(element, "label")))
        label = zMapXMLAttributeGetValue(attr);

      switch(server->type)
        {
        case ZMAP_DASONE_INTERNALFEATURES:
          feature = (dasOneFeature)g_datalist_id_get_data(&(featuredata), id);
          if(feature)
            dasOneFeature_setProperties(feature, start, stop, score, orientation, phase);
          if(feature && type)
            dasOneFeature_setType(feature, type);
          if(feature && target)
            dasOneFeature_setTarget(feature, target);
          handled = TRUE;
          break;
        case ZMAP_DASONE_FEATURES:
          {
            ZMapFeatureType typeEnum ;
            ZMapStrand strandEnum ;
            ZMapPhase phaseEnum ;
            ZMapFeatureSet fset;
            char *feature_name_id;
            int query_start = 0, query_end = 0; /* These will come from target start/end */

            zMapFeatureFormatType(FALSE, FALSE, (char *)g_quark_to_string(typeID), &typeEnum);
            zMapFeatureFormatStrand(orientation, &strandEnum);
            zMapFeatureFormatPhase(phase, &phaseEnum);
            
            /*            featureName(&feature_name, &feature_name_id,
                        typeEnum, strandEnum, start, stop,
                        query_start, query_end);
            */
            feature_name_id = zMapFeatureCreateName(typeEnum, (char *)g_quark_to_string(id), 
                                                    strandEnum, start, stop, 
                                                    query_start, query_end) ;

            printf("feature %s start %d stop %d type %d \n", feature_name_id, start, stop, typeEnum);
            /* check if this column/method has stuff... */
            if((fset = (ZMapFeatureSet)g_datalist_id_get_data(&(featuredata), 
                                                          g_quark_from_string(methodStr))))
              {
                printf("everything smells\n");
              }
            else
              {
                
              }
            /* make feature_name, feature_name_id, sequence
                                                  
            result = zMapFeatureAugmentData(feature, feature_name_id, feature_name, sequence,
                                            feature_type, curr_style,
                                            start, end, score, strand, phase,
                                            homol_type, query_start, query_end, gaps) ;
            */

          }
          break;
        case ZMAP_DAS_UNKNOWN:
        case ZMAP_DASONE_UNKNOWN:
        case ZMAP_DASONE_DSN:
        case ZMAP_DASONE_ENTRY_POINTS:
        case ZMAP_DASONE_DNA:
        case ZMAP_DASONE_SEQUENCE:
        case ZMAP_DASONE_TYPES:
        case ZMAP_DASONE_LINK:
        case ZMAP_DASONE_STYLESHEET:
        case ZMAP_DASTWO_UNKNOWN:
        default:
          handled = FALSE;
          break;
        }
      
    }
#endif
  
  return handled;
}




/* if this is set as the root elements end handler the document of
   elements will get cleaned up */
gboolean cleanUpDoc(void *userData,
                    ZMapXMLElement element,
                    ZMapXMLParser parser)
{
  return TRUE;
}


/* ================================================================= */
/* ================================================================= */
/*                         INTERNALS                                 */
/* ================================================================= */
/* ================================================================= */

static void setCurrentSegment(DasServer server, ZMapXMLElement segElement)
{
  ZMapXMLAttribute attr = NULL;
  dasOneDSN dsn = NULL;
  GList *list   = NULL;
  GQuark id = 0;
  checkDSNExists(server, &dsn);

  if((attr = zMapXMLElementGetAttributeByName(segElement, "id")))
    {
      id = zMapXMLAttributeGetValue(attr);
    }
  list = dasOneEntryPoint_getSegmentsList(dasOneDSN_entryPoint(dsn, NULL));
  while(list)
    {
      dasOneSegment seg = (dasOneSegment)list->data;
      if(dasOneSegment_id1(seg) == id)
        server->current_segment = seg;
      list = list->next;
    }
  return ;
}

static ZMapFeatureSet makeZmapFeatureFromElement(ZMapXMLElement element,
                                                 GList *list)
{
  ZMapFeatureSet feature_set = NULL;
  ZMapXMLElement ele  = NULL; /* temp 2 hold subelements */
  /* Type Quarks */
  GQuark type_id = 0, type_cat = 0, type_ref = 0, type_sub = 0, type_sup = 0;
  /* Target Quarks */
  GQuark target_id = 0;
  int target_start = 0, target_stop = 0;
  /* Feature Quarks */
  char *f_orientation = "+";
  int f_start = 0, f_stop = 0, f_phase = 0;
  double f_score = 0.0;
  char *method;

  if(!list)
    printf("beginning\n");

  if((ele = zMapXMLElementGetChildByName(element, "type")))
    {
      ZMapXMLAttribute attr = NULL;
      if((attr = zMapXMLElementGetAttributeByName(ele, "id")))
        type_id = zMapXMLAttributeGetValue(attr);
      if((attr = zMapXMLElementGetAttributeByName(ele, "category")))
        type_cat = zMapXMLAttributeGetValue(attr);
      if((attr = zMapXMLElementGetAttributeByName(ele, "reference")))
        type_ref = zMapXMLAttributeGetValue(attr);
      if((attr = zMapXMLElementGetAttributeByName(ele, "subparts")))
        type_sub = zMapXMLAttributeGetValue(attr);
      if((attr = zMapXMLElementGetAttributeByName(ele, "superparts")))
        type_sup = zMapXMLAttributeGetValue(attr);
    }
  if((ele = zMapXMLElementGetChildByName(element, "method")))
    method = ele->contents->str;
  if((ele = zMapXMLElementGetChildByName(element, "target")))
    {
      ZMapXMLAttribute attr = NULL;
      if((attr = zMapXMLElementGetAttributeByName(ele, "id")))
        target_id = zMapXMLAttributeGetValue(attr);
      if((attr = zMapXMLElementGetAttributeByName(ele, "start")))
        target_start = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
      if((attr = zMapXMLElementGetAttributeByName(ele, "stop")))
        target_stop = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
    }
  /* Feature elements */
  if((ele = zMapXMLElementGetChildByName(element, "start")))
    f_start = strtol(ele->contents->str, (char **)NULL, 10);
  if((ele = zMapXMLElementGetChildByName(element, "end")))
    f_stop = strtol(ele->contents->str, (char **)NULL, 10);
  if((ele = zMapXMLElementGetChildByName(element, "orientation")))
    f_orientation = ele->contents->str;
  if((ele = zMapXMLElementGetChildByName(element, "score")))
    f_score = strtod(ele->contents->str, (char **)NULL);
  if((ele = zMapXMLElementGetChildByName(element, "phase")))
    f_phase = strtol(ele->contents->str, (char **)NULL, 10);
  

  return feature_set;
}




