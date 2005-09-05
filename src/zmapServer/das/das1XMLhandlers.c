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
 * Last edited: Sep  5 18:22 2005 (rds)
 * Created: Thu Sep  1 14:44:07 2005 (rds)
 * CVS info:   $Id: das1XMLhandlers.c,v 1.1 2005-09-05 17:27:51 rds Exp $
 *-------------------------------------------------------------------
 */

#include <dasServer_P.h>

/* THIS FILE NEEDS RENAMING! */

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
      
      if((srcEle = zMapXMLElement_getChildByName(element, g_quark_from_string("source"))) != NULL)
        {
          if((idAttr = zMapXMLElement_getAttributeByName(srcEle, "id")) != NULL)
            id = zMapXMLAttribute_getValue(idAttr);
          if((versionAttr = zMapXMLElement_getAttributeByName(srcEle, "version")) != NULL)
            version = zMapXMLAttribute_getValue(versionAttr);
          if(srcEle->contents)
            name = g_quark_from_string(srcEle->contents->str);
        }

      if((mapEle = zMapXMLElement_getChildByName(element, g_quark_from_string("mapmaster"))) != NULL)
        if(mapEle->contents)
          map = g_quark_from_string(mapEle->contents->str);

      if((descEle = zMapXMLElement_getChildByName(element, g_quark_from_string("description"))) != NULL)
        if(descEle->contents)
          desc = g_quark_from_string(descEle->contents->str);

      if((dsn = dasOneDSN_create1(id, version, name)) != NULL)
        {
          dasOneDSN_setAttributes1(dsn, map, desc);
          server->dsn_list = g_list_append(server->dsn_list, dsn);
          handled = TRUE;
        }
      else
        printf("Warning this needs to best written\n");
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
        int start, stop, size;
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

        /* For Old skool */
        if((!start || !stop) && size)
          {
            start = 1;
            stop  = size;
          }

        if((seg = dasOneSegment_create1(id, start, stop, type, orientation)) != NULL)
          zMapXMLFactory_listAppend(server->hashtable, element, seg);
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
  dasXMLTagType type = (dasXMLTagType)zMapXMLFactoryDecodeElement(server->hashtable, 
                                                                  element, NULL);
  switch(type)
    {
    case TAG_DASONE_SEGMENT:
      handled = TRUE;
      break;
    default:
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
