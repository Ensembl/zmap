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
 * Last edited: Sep 16 14:44 2005 (rds)
 * Created: Thu Sep  1 14:44:07 2005 (rds)
 * CVS info:   $Id: das1XMLhandlers.c,v 1.6 2005-09-20 17:16:10 rds Exp $
 *-------------------------------------------------------------------
 */

#include <dasServer_P.h>

static void remFeaturesFromFactory(gpointer listData, gpointer myData);
static void setCurrentSegment(DasServer server, zmapXMLElement segElement);
static ZMapFeatureSet makeZmapFeatureFromElement(zmapXMLElement element,
                                                 GList *list);
static void rmInternalDasFeature(gpointer data);

gpointer dsnStart(void *userData,
                  zmapXMLElement element,
                  gpointer storage,
                  zmapXMLFactoryDetail detail)
{
  detail->result = FALSE;
  return NULL;
}
gpointer dsnEnd(void *userData,
                zmapXMLElement element,
                gpointer storage,
                zmapXMLFactoryDetail detail)
{
  DasServer server = (DasServer)userData;
  dasOneDSN dsn    = NULL;
  zmapXMLElement srcEle, mapEle, descEle; 
  zmapXMLAttribute idAttr, versionAttr;
  GQuark id = 0, version = 0, name = 0, map = 0, desc = 0;
  gboolean handled = FALSE; 
  
  if((srcEle = zMapXMLElement_getChildByName(element, "source")) != NULL)
    {
      if((idAttr = zMapXMLElement_getAttributeByName(srcEle, "id")) != NULL)
        id = zMapXMLAttribute_getValue(idAttr);
      if((versionAttr = zMapXMLElement_getAttributeByName(srcEle, "version")) != NULL)
        version = zMapXMLAttribute_getValue(versionAttr);
      name = g_quark_from_string(srcEle->contents->str);
    }
  
  if((mapEle = zMapXMLElement_getChildByName(element, "mapmaster")) != NULL)
    map = g_quark_from_string(mapEle->contents->str);
  
  if((descEle = zMapXMLElement_getChildByName(element, "description")) != NULL)
    desc = g_quark_from_string(descEle->contents->str);
  
  if((dsn = dasOneDSN_create1(id, version, name)) != NULL)
    {
      dasOneDSN_setAttributes1(dsn, map, desc);
      server->dsn_list = g_list_append(server->dsn_list, dsn);
      handled = TRUE;
    }
  else
    printf("Warning this needs to be written\n");
  
  
  detail->result = handled;

  return NULL;
}


gpointer segStart(void *userData,
                  zmapXMLElement element,
                  gpointer storage,
                  zmapXMLFactoryDetail detail)
{
  gboolean handled = FALSE;
  dasOneSegment seg = NULL;
  int start = 0, stop = 0, size = 0;
  char *ref = NULL, *sub = NULL;
  GQuark id = 0, type = 0, orientation = 0;
  zmapXMLAttribute attr;

  orientation = g_quark_from_string("+"); /* optional and assumed positive */
  if((attr = zMapXMLElement_getAttributeByName(element, "id")))
    id = zMapXMLAttribute_getValue(attr);
  if((attr = zMapXMLElement_getAttributeByName(element, "class")))
    type = zMapXMLAttribute_getValue(attr);
  if((attr = zMapXMLElement_getAttributeByName(element, "orientation")))
    orientation = zMapXMLAttribute_getValue(attr);
  if((attr = zMapXMLElement_getAttributeByName(element, "start")))
    start = strtol((char *)g_quark_to_string(zMapXMLAttribute_getValue(attr)), (char **)NULL, 10);
  if((attr = zMapXMLElement_getAttributeByName(element, "stop")))
    stop = strtol((char *)g_quark_to_string(zMapXMLAttribute_getValue(attr)), (char **)NULL, 10);
  if((attr = zMapXMLElement_getAttributeByName(element, "size")))
    size = strtol((char *)g_quark_to_string(zMapXMLAttribute_getValue(attr)), (char **)NULL, 10);
  
  if((attr = zMapXMLElement_getAttributeByName(element, "reference")))
    ref = (char *)g_quark_to_string( zMapXMLAttribute_getValue(attr) );
  if((attr = zMapXMLElement_getAttributeByName(element, "subparts")))
    sub = (char *)g_quark_to_string( zMapXMLAttribute_getValue(attr) );
  
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
  

  detail->result = handled;
  return seg;
}
gpointer segEnd(void *userData,
                zmapXMLElement element,
                gpointer storage,
                zmapXMLFactoryDetail detail)
{
  DasServer server = (DasServer)userData;
  gboolean handled = FALSE;

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
          datalist = zMapXMLFactory_dataFetchOutput(server->factory, g_quark_from_string("feature"));

          if((feat = g_datalist_id_get_data(&(datalist), seq)) != NULL)
            {
              setCurrentSegment(server, element);              
            }
          else
            {
              zmapXMLFactoryItem item = NULL;
              item = zMapXMLFactory_decodeNameQuark(server->factory, g_quark_from_string("feature"));
              zMapXMLFactoryItem_dataListClear(item);
            }
        }
      handled = TRUE;
    default:
      break;
    }

  detail->result = handled;
  return (gpointer)NULL;
}

gpointer featStart(void *userData,
                   zmapXMLElement element,
                   gpointer storage,
                   zmapXMLFactoryDetail detail)
{
  DasServer server = (DasServer)userData;
  gboolean handled = FALSE;
  dasOneFeature feat    = NULL;
  zmapXMLAttribute attr = NULL;
  GQuark id = 0, label  = 0;

  switch(server->type)
    {
    case ZMAP_DASONE_INTERNALFEATURES:
      if((attr = zMapXMLElement_getAttributeByName(element, "id")))
        id = zMapXMLAttribute_getValue(attr);
      if((attr = zMapXMLElement_getAttributeByName(element, "label")))
        label = zMapXMLAttribute_getValue(attr);

      feat = dasOneFeature_create1(id, label);
      detail->key    = id;
      detail->notify = rmInternalDasFeature;
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

  detail->result = handled;

  return feat;
}

gpointer featEnd(void *userData,
                 zmapXMLElement element,
                 gpointer storage,
                 zmapXMLFactoryDetail detail)
{
  DasServer server = (DasServer)userData;
  gpointer output  = NULL;
  gboolean handled = FALSE;
  /* now we need to munge through the feature element */
  dasOneType type     = NULL;
  dasOneMethod method = NULL;
  dasOneTarget target = NULL;
  zmapXMLElement ele  = NULL; /* temp 2 hold subelements */
  char *orientation   = "", *score = "", *phase = "", *methodStr = "";
  int start = 0, stop = 0;
  GQuark typeID = 0;
  GList *groups = NULL, *rawGroups = NULL;

  if((ele = zMapXMLElement_getChildByName(element, "start")))
    start = strtol(ele->contents->str, (char **)NULL, 10);
  if((ele = zMapXMLElement_getChildByName(element, "end")))
    stop = strtol(ele->contents->str, (char **)NULL, 10);
  if((ele = zMapXMLElement_getChildByName(element, "orientation")))
    orientation = ele->contents->str;
  if((ele = zMapXMLElement_getChildByName(element, "score")))
    score = ele->contents->str;
  if((ele = zMapXMLElement_getChildByName(element, "phase")))
    phase = ele->contents->str;


  if((ele = zMapXMLElement_getChildByName(element, "type")))
    {
      zmapXMLAttribute attr = NULL;
      GQuark cat = 0, ref = 0, sub = 0, sup = 0;
      if((attr = zMapXMLElement_getAttributeByName(ele, "id")))
        typeID = zMapXMLAttribute_getValue(attr);
      if((attr = zMapXMLElement_getAttributeByName(ele, "category")))
        cat = zMapXMLAttribute_getValue(attr);
      if((attr = zMapXMLElement_getAttributeByName(ele, "reference")))
        ref = zMapXMLAttribute_getValue(attr);
      if((attr = zMapXMLElement_getAttributeByName(ele, "subparts")))
        sub = zMapXMLAttribute_getValue(attr);
      if((attr = zMapXMLElement_getAttributeByName(ele, "superparts")))
        sup = zMapXMLAttribute_getValue(attr);
      type = dasOneType_create1(typeID, g_quark_from_string(ele->contents->str), cat);
      dasOneType_refProperties(type, 
                               (char *)g_quark_to_string(ref),
                               (char *)g_quark_to_string(sub),
                               (char *)g_quark_to_string(sup),
                               TRUE);
    }
  if((ele = zMapXMLElement_getChildByName(element, "method")))
    {
      methodStr = ele->contents->str;
      method = dasOneMethod_create(methodStr);
    }
  if((ele = zMapXMLElement_getChildByName(element, "target")))
    {
      zmapXMLAttribute attr = NULL;
      GQuark id = 0;
      int start = 0, stop = 0;
      if((attr = zMapXMLElement_getAttributeByName(ele, "id")))
        id = zMapXMLAttribute_getValue(attr);
      if((attr = zMapXMLElement_getAttributeByName(ele, "start")))
        start = strtol((char *)g_quark_to_string(zMapXMLAttribute_getValue(attr)), (char **)NULL, 10);
      if((attr = zMapXMLElement_getAttributeByName(ele, "stop")))
        stop = strtol((char *)g_quark_to_string(zMapXMLAttribute_getValue(attr)), (char **)NULL, 10);
      target = dasOneTarget_create1(id, start, stop);
    }

  if((rawGroups = zMapXMLElement_getChildrenByName(element, g_quark_from_string("group"), -1)))
    {
      while(rawGroups)
        {
          GQuark id = 0, type = 0, label = 0;
          zmapXMLAttribute attr = NULL;
          dasOneGroup group = NULL;

          ele = (zmapXMLElement)rawGroups->data;
          if((attr = zMapXMLElement_getAttributeByName(ele, "id")))
            id     = zMapXMLAttribute_getValue(attr);
          if((attr = zMapXMLElement_getAttributeByName(ele, "type")))
            type   = zMapXMLAttribute_getValue(attr);
          if((attr = zMapXMLElement_getAttributeByName(ele, "label")))
            label  = zMapXMLAttribute_getValue(attr);

          group     = dasOneGroup_create1(id, type, label);
          groups    = g_list_prepend(groups, group);
          rawGroups = rawGroups->next;
        }
    }

  if(storage != NULL)
    {
      GQuark id = 0, label = 0;
      zmapXMLAttribute attr = NULL;
      dasOneFeature feature = NULL;
      GData *featuredata    = (GData *)storage;

      if((attr = zMapXMLElement_getAttributeByName(element, "id")))
        id = zMapXMLAttribute_getValue(attr);
      if((attr = zMapXMLElement_getAttributeByName(element, "label")))
        label = zMapXMLAttribute_getValue(attr);

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

  detail->result = handled;
  return output;
}




/* if this is set as the root elements end handler the document of
   elements will get cleaned up */
gpointer cleanUpDoc(void *userData,
                    zmapXMLElement element,
                    gpointer storage,
                    zmapXMLFactoryDetail detail)
{
  detail->result = TRUE;
  return (gpointer)NULL;
}


/* ================================================================= */
/* ================================================================= */
/*                         INTERNALS                                 */
/* ================================================================= */
/* ================================================================= */

static void remFeaturesFromFactory(gpointer listData, gpointer myData)
{
  dasOneFeature_free((dasOneFeature)listData);
  listData = NULL;
  return ;
}


static void setCurrentSegment(DasServer server, zmapXMLElement segElement)
{
  zmapXMLAttribute attr = NULL;
  dasOneDSN dsn = NULL;
  GList *list   = NULL;
  GQuark id = 0;
  checkDSNExists(server, &dsn);

  if((attr = zMapXMLElement_getAttributeByName(segElement, "id")))
    {
      id = zMapXMLAttribute_getValue(attr);
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

static ZMapFeatureSet makeZmapFeatureFromElement(zmapXMLElement element,
                                           GList *list)
{
  ZMapFeatureSet feature_set = NULL;
  zmapXMLElement ele  = NULL; /* temp 2 hold subelements */
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

  if((ele = zMapXMLElement_getChildByName(element, "type")))
    {
      zmapXMLAttribute attr = NULL;
      if((attr = zMapXMLElement_getAttributeByName(ele, "id")))
        type_id = zMapXMLAttribute_getValue(attr);
      if((attr = zMapXMLElement_getAttributeByName(ele, "category")))
        type_cat = zMapXMLAttribute_getValue(attr);
      if((attr = zMapXMLElement_getAttributeByName(ele, "reference")))
        type_ref = zMapXMLAttribute_getValue(attr);
      if((attr = zMapXMLElement_getAttributeByName(ele, "subparts")))
        type_sub = zMapXMLAttribute_getValue(attr);
      if((attr = zMapXMLElement_getAttributeByName(ele, "superparts")))
        type_sup = zMapXMLAttribute_getValue(attr);
    }
  if((ele = zMapXMLElement_getChildByName(element, "method")))
    method = ele->contents->str;
  if((ele = zMapXMLElement_getChildByName(element, "target")))
    {
      zmapXMLAttribute attr = NULL;
      if((attr = zMapXMLElement_getAttributeByName(ele, "id")))
        target_id = zMapXMLAttribute_getValue(attr);
      if((attr = zMapXMLElement_getAttributeByName(ele, "start")))
        target_start = strtol((char *)g_quark_to_string(zMapXMLAttribute_getValue(attr)), (char **)NULL, 10);
      if((attr = zMapXMLElement_getAttributeByName(ele, "stop")))
        target_stop = strtol((char *)g_quark_to_string(zMapXMLAttribute_getValue(attr)), (char **)NULL, 10);
    }
  /* Feature elements */
  if((ele = zMapXMLElement_getChildByName(element, "start")))
    f_start = strtol(ele->contents->str, (char **)NULL, 10);
  if((ele = zMapXMLElement_getChildByName(element, "end")))
    f_stop = strtol(ele->contents->str, (char **)NULL, 10);
  if((ele = zMapXMLElement_getChildByName(element, "orientation")))
    f_orientation = ele->contents->str;
  if((ele = zMapXMLElement_getChildByName(element, "score")))
    f_score = strtod(ele->contents->str, (char **)NULL);
  if((ele = zMapXMLElement_getChildByName(element, "phase")))
    f_phase = strtol(ele->contents->str, (char **)NULL, 10);
  

  return feature_set;
}

static void rmInternalDasFeature(gpointer data)
{
  dasOneFeature feature = (dasOneFeature)data;

  dasOneFeature_free(feature);

  return ;
}
