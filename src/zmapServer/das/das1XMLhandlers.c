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
 * Last edited: Sep  8 18:37 2005 (rds)
 * Created: Thu Sep  1 14:44:07 2005 (rds)
 * CVS info:   $Id: das1XMLhandlers.c,v 1.2 2005-09-08 17:45:35 rds Exp $
 *-------------------------------------------------------------------
 */

#include <dasServer_P.h>

static void remFeaturesFromFactory(gpointer listData, gpointer myData);
static void setCurrentSegment(DasServer server, zmapXMLElement segElement);




/* The start and end handlers */
gboolean dsnStart(void *userData, 
                  zmapXMLElement element, 
                  zmapXMLParser parser)
{
  DasServer server = (DasServer)userData;
  return FALSE;
}
gboolean dsnEnd(void *userData, 
                zmapXMLElement element, 
                zmapXMLParser parser)
{
  gboolean handled = FALSE;
  DasServer server = (DasServer)userData;

  dasXMLTagType type = (dasXMLTagType)zMapXMLFactoryDecodeElement(server->hashtable, 
                                                                  element, NULL);

  switch(type){
  case TAG_DASONE_DSN:
    {
      dasOneDSN dsn = NULL;
      zmapXMLElement srcEle, mapEle, descEle; 
      zmapXMLAttribute idAttr, versionAttr;
      GQuark id = 0, version = 0, name = 0, map = 0, desc = 0;
      
      if((srcEle = zMapXMLElement_getChildByName(element, "source")) != NULL)
        {
          if((idAttr = zMapXMLElement_getAttributeByName(srcEle, "id")) != NULL)
            id = zMapXMLAttribute_getValue(idAttr);
          if((versionAttr = zMapXMLElement_getAttributeByName(srcEle, "version")) != NULL)
            version = zMapXMLAttribute_getValue(versionAttr);
          if(srcEle->contents)
            name = g_quark_from_string(srcEle->contents->str);
        }

      if((mapEle = zMapXMLElement_getChildByName(element, "mapmaster")) != NULL)
        if(mapEle->contents)
          map = g_quark_from_string(mapEle->contents->str);

      if((descEle = zMapXMLElement_getChildByName(element, "description")) != NULL)
        if(descEle->contents)
          desc = g_quark_from_string(descEle->contents->str);

      if((dsn = dasOneDSN_create1(id, version, name)) != NULL)
        {
          dasOneDSN_setAttributes1(dsn, map, desc);
          server->dsn_list = g_list_append(server->dsn_list, dsn);
          handled = TRUE;
        }
      else
        printf("Warning this needs to be written\n");
    }
    break;
  default:
    break;
  }

  return handled;
}

gboolean epStart(void *userData, 
                 zmapXMLElement element, 
                 zmapXMLParser parser)
{
  DasServer server   = (DasServer)userData;
  dasXMLTagType type = (dasXMLTagType)zMapXMLFactoryDecodeElement(server->hashtable, 
                                                                  element, NULL);
  switch(type)
    {
    case TAG_DASONE_SEGMENT:
      {
        dasOneSegment seg = NULL;
        int start = 0, stop = 0, size = 0;
        char *ref = NULL, *sub = NULL;
        GQuark id = 0, type = 0, orientation = 0;
        zmapXMLAttribute attr;
        orientation = g_quark_from_string("+"); /* optional and assumed positive */
        if(attr = zMapXMLElement_getAttributeByName(element, "id"))
          id = zMapXMLAttribute_getValue(attr);
        if(attr = zMapXMLElement_getAttributeByName(element, "class"))
          type = zMapXMLAttribute_getValue(attr);
        if(attr = zMapXMLElement_getAttributeByName(element, "orientation"))
          orientation = zMapXMLAttribute_getValue(attr);
        if(attr = zMapXMLElement_getAttributeByName(element, "start"))
          start = strtol((char *)g_quark_to_string(zMapXMLAttribute_getValue(attr)), (char **)NULL, 10);
        if(attr = zMapXMLElement_getAttributeByName(element, "stop"))
          stop = strtol((char *)g_quark_to_string(zMapXMLAttribute_getValue(attr)), (char **)NULL, 10);
        if(attr = zMapXMLElement_getAttributeByName(element, "size"))
          size = strtol((char *)g_quark_to_string(zMapXMLAttribute_getValue(attr)), (char **)NULL, 10);

        if(attr = zMapXMLElement_getAttributeByName(element, "reference"))
          ref = (char *)g_quark_to_string( zMapXMLAttribute_getValue(attr) );
        if(attr = zMapXMLElement_getAttributeByName(element, "subparts"))
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
            zMapXMLFactoryListAddItem(server->hashtable, element, seg);
          }
      }
      break;
    default:
      break;
    }

  return FALSE;

}
gboolean epEnd(void *userData, 
               zmapXMLElement element, 
               zmapXMLParser parser)
{
  DasServer server = (DasServer)userData;
  gboolean handled = FALSE;
  GList *list      = NULL;
  dasXMLTagType type = (dasXMLTagType)zMapXMLFactoryDecodeElement(server->hashtable, 
                                                                  element, &list);
  switch(type)
    {
    case TAG_DASONE_SEGMENT:
      if(list)
        {
          dasOneSegment seg = NULL;
          seg = (dasOneSegment)list->data;
          dasOneSegment_description(seg, element->contents->str );
        }
      handled = TRUE;
      break;
    case TAG_DASONE_ENTRY_POINTS:
      handled = TRUE;
      break;
    default:
      
      handled = TRUE;
      break;
    }
  return handled;
}

gboolean typesStart(void *userData, 
                    zmapXMLElement element, 
                    zmapXMLParser parser)
{
  DasServer server = (DasServer)userData;

  dasXMLTagType type = (dasXMLTagType)zMapXMLFactoryDecodeElement(server->hashtable, 
                                                                  element, NULL);
  switch(type)
    {
    case TAG_DASONE_SEGMENT:
      
      break;
    default:
      break;
    }

  return FALSE;
}
gboolean typesEnd(void *userData, 
                  zmapXMLElement element, 
                  zmapXMLParser parser)
{
  gboolean handled = FALSE;
  DasServer server = (DasServer)userData;

  dasXMLTagType type = (dasXMLTagType)zMapXMLFactoryDecodeElement(server->hashtable, 
                                                                  element, NULL);

  switch(type){
  case TAG_DASONE_TYPES:
    
    break;
  default:
    break;
  }

  return handled;
}

gboolean internalDasStart(void *userData, 
                          zmapXMLElement element, 
                          zmapXMLParser parser)
{
  gboolean handled = FALSE;
  DasServer server = (DasServer)userData;

  dasXMLTagType type = (dasXMLTagType)zMapXMLFactoryDecodeElement(server->hashtable, 
                                                                  element, NULL);
  switch(type){
  case TAG_DASONE_FEATURES:
    /* Create the internal feature */
    {
      dasOneFeature feat = NULL;
      zmapXMLAttribute attr = NULL;
      GQuark id = 0, label = 0;
      if(attr = zMapXMLElement_getAttributeByName(element, "id"))
        id = zMapXMLAttribute_getValue(attr);
      if(attr = zMapXMLElement_getAttributeByName(element, "label"))
        label = zMapXMLAttribute_getValue(attr);
      feat = dasOneFeature_create1(id, label);
      if(feat)
        zMapXMLFactoryListAddItem(server->hashtable, element, feat);
    }
    break;
  default:
    break;
  }
  return handled;
}
gboolean internalDasEnd(void *userData, 
                        zmapXMLElement element, 
                        zmapXMLParser parser)
{
  gboolean handled = FALSE;
  DasServer server = (DasServer)userData;
  GList *list      = NULL;
  dasXMLTagType type = (dasXMLTagType)zMapXMLFactoryDecodeElement(server->hashtable, 
                                                                  element, &list);
  switch(type){
  case TAG_DASONE_FEATURES:
    {
      /* now we need to munge through the feature element */
      dasOneType type     = NULL;
      dasOneMethod method = NULL;
      dasOneTarget target = NULL;
      zmapXMLElement ele  = NULL; /* temp 2 hold subelements */
      if(ele = zMapXMLElement_getChildByName(element, "type"))
        {
          zmapXMLAttribute attr = NULL;
          GQuark id = 0, cat = 0, ref = 0, sub = 0, sup = 0;
          if(attr = zMapXMLElement_getAttributeByName(ele, "id"))
            id = zMapXMLAttribute_getValue(attr);
          if(attr = zMapXMLElement_getAttributeByName(ele, "category"))
            cat = zMapXMLAttribute_getValue(attr);
          if(attr = zMapXMLElement_getAttributeByName(ele, "reference"))
            ref = zMapXMLAttribute_getValue(attr);
          if(attr = zMapXMLElement_getAttributeByName(ele, "subparts"))
            sub = zMapXMLAttribute_getValue(attr);
          if(attr = zMapXMLElement_getAttributeByName(ele, "superparts"))
            sup = zMapXMLAttribute_getValue(attr);
          type = dasOneType_create1(id, g_quark_from_string(ele->contents->str), cat);
          dasOneType_refProperties(type, 
                                   (char *)g_quark_to_string(ref),
                                   (char *)g_quark_to_string(sub),
                                   (char *)g_quark_to_string(sup),
                                   TRUE);
        }
      if(ele = zMapXMLElement_getChildByName(element, "method"))
        method = dasOneMethod_create(ele->contents->str);
      if(ele = zMapXMLElement_getChildByName(element, "target"))
        {
          zmapXMLAttribute attr = NULL;
          GQuark id = 0, start = 0, stop = 0;
          if(attr = zMapXMLElement_getAttributeByName(ele, "id"))
            id = zMapXMLAttribute_getValue(attr);
          if(attr = zMapXMLElement_getAttributeByName(ele, "start"))
            start = strtol((char *)g_quark_to_string(zMapXMLAttribute_getValue(attr)), (char **)NULL, 10);
          if(attr = zMapXMLElement_getAttributeByName(ele, "stop"))
            stop = strtol((char *)g_quark_to_string(zMapXMLAttribute_getValue(attr)), (char **)NULL, 10);
          target = dasOneTarget_create1(id, start, stop);
        }

      if(list)
        {
          dasOneFeature feature = (dasOneFeature)list->data;
          char *orientation = 0;
          int start = 0, stop = 0, phase = 0;
          double score = 0.0;
          if(ele = zMapXMLElement_getChildByName(element, "start"))
            start = strtol(ele->contents->str, (char **)NULL, 10);
          if(ele = zMapXMLElement_getChildByName(element, "end"))
            stop = strtol(ele->contents->str, (char **)NULL, 10);
          if(ele = zMapXMLElement_getChildByName(element, "orientation"))
            orientation = ele->contents->str;
          if(ele = zMapXMLElement_getChildByName(element, "score"))
            score = strtod(ele->contents->str, (char **)NULL);
          if(ele = zMapXMLElement_getChildByName(element, "phase"))
            phase = strtol(ele->contents->str, (char **)NULL, 10);

          dasOneFeature_setProperties(feature, start, stop, score, orientation, phase);
          if(type)
            dasOneFeature_setType(feature, type);
          if(target)
            dasOneFeature_setTarget(feature, target);
        }
      handled = TRUE;
    }
    break;
  case TAG_DASONE_SEGMENT:
    {
      if(server->req_context && !server->current_segment)
        { 
          GQuark seq  = server->req_context->sequence_name;
          GList *list = NULL;
          gboolean clear = TRUE;
          zMapXMLFactoryDecodeNameQuark(server->hashtable, 
                                        g_quark_from_string("feature"), 
                                        &list);
          while(list)
            {
              dasOneFeature feat = (dasOneFeature)list->data;
              if(dasOneFeature_id1(feat) == seq)
                {
                  clear = FALSE;
                  printf("Wow we found %s.\n", g_quark_to_string(seq));
                  setCurrentSegment(server, element);
                }
              list = list->next;
            }
          if(clear)
            zMapXMLFactoryFreeListComplete(server->hashtable, 
                                           g_quark_from_string("feature"),
                                           remFeaturesFromFactory,
                                           NULL);
        }
    }
    break;
  default:
    break;
  }
  return handled;
}

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

  if(attr = zMapXMLElement_getAttributeByName(segElement, "id"))
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
