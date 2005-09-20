/*  File: zmapXMLFactory.c
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
 * Last edited: Sep 14 14:35 2005 (rds)
 * Created: Tue Aug 16 14:40:59 2005 (rds)
 * CVS info:   $Id: zmapXMLFactory.c,v 1.3 2005-09-20 17:18:11 rds Exp $
 *-------------------------------------------------------------------
 */

#include <zmapXML_P.h>

static
gboolean zmapXMLFactoryIntHandler(void *userData,
                                  zmapXMLElement element,
                                  zmapXMLParser parser,
                                  gboolean isStart);

zmapXMLFactoryLiteItem zMapXMLFactoryLiteItem_create(int tag_type)
{
  zmapXMLFactoryLiteItem item = NULL;
  item = g_new0(zmapXMLFactoryLiteItemStruct, 1);

  item->tag_type = tag_type;
  item->list     = NULL;

  return item;
}

void zMapXMLFactoryLite_addItem(zmapXMLFactory factory, 
                                zmapXMLFactoryLiteItem item,
                                char *tag)
{
  /* Assert factory, item and tag */
  if(tag && *tag)
    g_hash_table_insert(factory->hash,
                        GINT_TO_POINTER(g_quark_from_string(g_ascii_strdown(tag, -1))),
                        item);
  return ;
}



/* This might mean we can make elements private */
int zMapXMLFactoryLite_decodeElement(zmapXMLFactory factory,
                                     zmapXMLElement element,
                                     GList **listout)
{
  return zMapXMLFactoryLite_decodeNameQuark(factory, element->name, listout);
}

int zMapXMLFactoryLite_decodeNameQuark(zmapXMLFactory factory,
                                       GQuark name,
                                       GList **listout)
{
  zmapXMLFactoryLiteItem item = NULL;
  int type = -1;

  item = g_hash_table_lookup(factory->hash, 
                              GINT_TO_POINTER(name));
  if(item)
    {
      type = item->tag_type;
      if(listout)
        *listout = item->list;
    }

  return type;
}

void zMapXMLFactoryLite_addOutput(zmapXMLFactory factory, 
                                  zmapXMLElement element, 
                                  void *listItem)
{
  zmapXMLFactoryLiteItem item = NULL;

  if(!listItem)
    return ;

  item = g_hash_table_lookup(factory->hash, 
                              GINT_TO_POINTER(element->name));
  if(item)
    item->list = g_list_prepend(item->list, listItem);

  return ;
}



void zMapXMLFactoryFreeListComplete(zmapXMLFactory factory,
                                    GQuark name,
                                    GFunc func,
                                    gpointer userData)
{
  zmapXMLFactoryLiteItem item = NULL;
  
  item = g_hash_table_lookup(factory->hash, 
                              GINT_TO_POINTER(name));

  if(func)
    g_list_foreach(item->list, func, userData);
  g_list_free(item->list);
  item->list = NULL;

  return ;
}


zmapXMLFactory zMapXMLFactory_create(gboolean useLite)
{
  zmapXMLFactory factory = NULL;

  factory         = g_new0(zmapXMLFactoryStruct, 1);
  factory->hash   = g_hash_table_new(NULL, NULL);
  factory->isLite = useLite;

  return factory;
}

void zMapXMLFactory_setUserData(zmapXMLFactory factory, gpointer data)
{
  factory->userData = data;
}

zmapXMLFactoryItem zMapXMLFactory_decodeElement(zmapXMLFactory factory,
                                                zmapXMLElement element)
{
  return zMapXMLFactory_decodeNameQuark(factory, element->name);
}

zmapXMLFactoryItem zMapXMLFactory_decodeNameQuark(zmapXMLFactory factory,
                                                  GQuark name)
{
  zmapXMLFactoryItem item = NULL;

  item = g_hash_table_lookup(factory->hash, 
                              GINT_TO_POINTER(name));
  return item;
}
GList *zMapXMLFactory_listFetchOutput(zmapXMLFactory factory,
                                      GQuark name)
{
  zmapXMLFactoryItem item = NULL;
  GList *list = NULL;

  item = zMapXMLFactory_decodeNameQuark(factory, name);
  if(item && (item->datatype == ZMAP_XML_FACTORY_LIST))
    list = item->storage.list;

  return list;
}

GData *zMapXMLFactory_dataFetchOutput(zmapXMLFactory factory,
                                      GQuark name)
{
  zmapXMLFactoryItem item = NULL;
  GData *data = NULL;

  item = zMapXMLFactory_decodeNameQuark(factory, name);
  if(item && (item->datatype == ZMAP_XML_FACTORY_DATALIST))
    data = item->storage.datalist;

  return data;
}

void zMapXMLFactory_createItemInsert(zmapXMLFactory factory,
                                     char *tag,
                                     zmapXMLFactoryFunc start,
                                     zmapXMLFactoryFunc end,
                                     zmapXMLFactoryStorageType type)
{
  zmapXMLFactoryItem item = NULL;

  item = g_new0(zmapXMLFactoryItemStruct, 1);
  item->begin = start;
  item->end   = end;
  item->datatype = type;

  if(item->datatype == ZMAP_XML_FACTORY_DATALIST)
    g_datalist_init(&(item->storage.datalist));

  if(tag && *tag)
    g_hash_table_insert(factory->hash,
                        GINT_TO_POINTER(g_quark_from_string(g_ascii_strdown(tag, -1))),
                        item);

  return ;
}


/* parser start and end handlers */
gboolean zmapXML_FactoryEndHandler(void *userData,
                                  zmapXMLElement element,
                                  zmapXMLParser parser)
{
  return zmapXMLFactoryIntHandler(userData, element, parser, FALSE);
}
gboolean zmapXML_FactoryStartHandler(void *userData,
                                    zmapXMLElement element,
                                    zmapXMLParser parser)
{
  return zmapXMLFactoryIntHandler(userData, element, parser, TRUE);
}

void zMapXMLFactory_dataAddOutput(zmapXMLFactoryItem item, 
                                  gpointer data,
                                  GQuark key,
                                  GDestroyNotify notify)
{
  if(key && (item->datatype == ZMAP_XML_FACTORY_DATALIST))
    {
      g_datalist_id_set_data_full(&(item->storage.datalist), 
                                  key, data, notify);
    }
  return ;
}

void zMapXMLFactory_listAddOutput(zmapXMLFactoryItem item,
                                  gpointer data)
{
  if(item->datatype == ZMAP_XML_FACTORY_LIST)
    item->storage.list = g_list_prepend(item->storage.list, data);
  return ;
}

void zMapXMLFactoryItem_dataListClear(zmapXMLFactoryItem item)
{
  if(item->datatype == ZMAP_XML_FACTORY_DATALIST)
    g_datalist_clear(&(item->storage.datalist));
  return ;
}






/* INTERNAL */
static
gboolean zmapXMLFactoryIntHandler(void *userData,
                                  zmapXMLElement element,
                                  zmapXMLParser parser,
                                  gboolean isStart)
{
  zmapXMLFactory factory  = (zmapXMLFactory)userData;
  zmapXMLFactoryFunc func = NULL;
  zmapXMLFactoryStorageType storage = 0;
  gpointer output = NULL;
  gboolean result = FALSE;
  zmapXMLFactoryItem item;

  item = zMapXMLFactory_decodeElement(factory,
                                      element);

  if(item != NULL)
    {
      storage = item->datatype;
      func    = (isStart ? item->begin : item->end);

      if(func)
        {
          zmapXMLFactoryDetailStruct detail = {FALSE, 0, NULL};
          switch (storage)
            {
            case ZMAP_XML_FACTORY_LIST:
              if((output = (*func)(factory->userData, element, item->storage.list, &detail)) != NULL)
                zMapXMLFactory_listAddOutput(item, output);
              break;
            case ZMAP_XML_FACTORY_DATALIST:
              if(((output = (*func)(factory->userData, element, item->storage.datalist, &detail)) != NULL) 
                 && detail.key > 0)
                zMapXMLFactory_dataAddOutput(item, output, detail.key, detail.notify);
              break;
            case ZMAP_XML_FACTORY_NONE:
              (*func)(factory->userData, element, NULL, &detail);
              break;
            default:
              break;
            }
          result = detail.result;
        }
    }

  return result;
}
