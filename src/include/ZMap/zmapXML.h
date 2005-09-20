/*  File: zmapXML.h
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
 * Last edited: Sep 15 12:12 2005 (rds)
 * Created: Tue Aug  2 16:27:08 2005 (rds)
 * CVS info:   $Id: zmapXML.h,v 1.5 2005-09-20 17:10:47 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_XML_H
#define ZMAP_XML_H

#include <stdio.h>
#include <expat.h>
#include <glib.h>

/* TYPEDEFS */
typedef struct _zmapXMLElementStruct   *zmapXMLElement;
typedef struct _zmapXMLElementStruct
{
  GQuark   name;
  GString *contents; 
  GList   *attributes;
#ifdef USING_PRE_ORDERED_TREE_FOR_FREE
  int left;
  int right;
#endif /* USING_PRE_ORDERED_TREE_FOR_FREE */
  zmapXMLElement parent;
  GList   *children;
} zmapXMLElementStruct;

typedef struct _zmapXMLAttributeStruct *zmapXMLAttribute;
typedef struct _zmapXMLDocumentStruct  *zmapXMLDocument;
typedef struct _zmapXMLParserStruct    *zmapXMLParser;

typedef struct _zmapXMLFactoryStruct         *zmapXMLFactory;
typedef struct _zmapXMLFactoryItemStruct     *zmapXMLFactoryItem;
typedef struct _zmapXMLFactoryLiteItemStruct *zmapXMLFactoryLiteItem;

typedef struct _zmapXMLFactoryDetailStruct
{
  gboolean result;
  GQuark key;
  GDestroyNotify notify;
} zmapXMLFactoryDetailStruct, *zmapXMLFactoryDetail;


typedef enum {
  ZMAP_XML_FACTORY_NONE = 0,
  ZMAP_XML_FACTORY_LIST,
  ZMAP_XML_FACTORY_DATALIST
} zmapXMLFactoryStorageType;

typedef gpointer (*zmapXMLFactoryFunc)(gpointer userData,
                                       zmapXMLElement element,
                                       gpointer storage,
                                       zmapXMLFactoryDetail detail
#ifdef OLDVERSION
                                       gboolean *handled_out,
                                       GQuark *key_out
#endif
);

typedef gboolean 
(*zmapXML_StartMarkupObjectHandler)(void *userData, zmapXMLElement element, zmapXMLParser parser);
typedef gboolean 
(*zmapXML_EndMarkupObjectHandler)(void *userData, zmapXMLElement element, zmapXMLParser parser);


/* ATTRIBUTES */
GQuark zMapXMLAttribute_getValue(zmapXMLAttribute attr);



/* DOCUMENTS */
zmapXMLDocument zMapXMLDocument_create(const XML_Char *version,
                                       const XML_Char *encoding,
                                       int standalone);
void zMapXMLDocument_setRoot(zmapXMLDocument doc,
                              zmapXMLElement root);
char *zMapXMLDocument_version(zmapXMLDocument doc);
char *zMapXMLDocument_encoding(zmapXMLDocument doc);
gboolean zMapXMLDocument_isStandalone(zmapXMLDocument doc);
void zMapXMLDocument_reset(zmapXMLDocument doc);
void zMapXMLDocument_destroy(zmapXMLDocument doc);



/* ELEMENTS */
zmapXMLElement zMapXMLElement_nextSibling(zmapXMLElement ele);
zmapXMLElement zMapXMLElement_previousSibling(zmapXMLElement ele);

zmapXMLElement zMapXMLElement_getChildByPath(zmapXMLElement parent,
                                             char *path);

zmapXMLElement zMapXMLElement_getChildByName(zmapXMLElement parent,
                                             char *name);
zmapXMLElement zMapXMLElement_getChildByName1(zmapXMLElement parent,
                                              GQuark name);
GList *zMapXMLElement_getChildrenByName(zmapXMLElement parent,
                                        GQuark name,
                                        int expect);
zmapXMLAttribute zMapXMLElement_getAttributeByName(zmapXMLElement ele,
                                                   char *name);
zmapXMLAttribute zMapXMLElement_getAttributeByName1(zmapXMLElement ele,
                                                    GQuark name);


/* PARSER */
zmapXMLParser zMapXMLParser_create(void *userData, gboolean validating, gboolean debug);

void zMapXMLParser_setMarkupObjectHandler(zmapXMLParser parser, 
                                          zmapXML_StartMarkupObjectHandler start,
                                          zmapXML_EndMarkupObjectHandler end);

gboolean zMapXMLParser_parseFile(zmapXMLParser parser,
                                  FILE *file);
gboolean zMapXMLParser_parseBuffer(zmapXMLParser parser, 
                                    void *data, 
                                    int size);
void zMapXMLParser_useFactory(zmapXMLParser parser,
                              zmapXMLFactory factory);

char *zMapXMLParser_lastErrorMsg(zmapXMLParser parser);

zmapXMLElement zMapXMLParser_getRoot(zmapXMLParser parser);

gboolean zMapXMLParser_reset(zmapXMLParser parser);
void zMapXMLParser_destroy(zmapXMLParser parser);



/* FACTORY Methods */
zmapXMLFactory zMapXMLFactory_create(gboolean isLite);
void zMapXMLFactory_setUserData(zmapXMLFactory factory, 
                                gpointer data);


/* lite methods */
zmapXMLFactoryLiteItem zMapXMLFactoryLiteItem_create(int tag_type);
void zMapXMLFactoryLite_addItem(zmapXMLFactory factory, 
                                zmapXMLFactoryLiteItem item,
                                char *tag);

int zMapXMLFactoryLite_decodeElement(zmapXMLFactory factory,
                                     zmapXMLElement element,
                                     GList **listout);
int zMapXMLFactoryLite_decodeNameQuark(zmapXMLFactory factory,
                                       GQuark name,
                                       GList **listout);

void zMapXMLFactoryLite_addOutput(zmapXMLFactory factory, 
                                  zmapXMLElement element, 
                                  void *listItem);


/* main methods */
void zMapXMLFactory_addItem(zmapXMLFactory factory, 
                            zmapXMLFactoryItem item,
                            char *tag);


void zMapXMLFactory_createItemInsert(zmapXMLFactory factory,
                                     char *tag,
                                     zmapXMLFactoryFunc start,
                                     zmapXMLFactoryFunc end,
                                     zmapXMLFactoryStorageType type
                                     );
zmapXMLFactoryItem zMapXMLFactory_decodeElement(zmapXMLFactory factory,
                                                zmapXMLElement element);

zmapXMLFactoryItem zMapXMLFactory_decodeNameQuark(zmapXMLFactory factory,
                                                  GQuark name);

GList *zMapXMLFactory_listFetchOutput(zmapXMLFactory factory,
                                      GQuark name);

GData *zMapXMLFactory_dataFetchOutput(zmapXMLFactory factory,
                                      GQuark name);

void zMapXMLFactory_dataAddOutput(zmapXMLFactoryItem item, 
                                  gpointer data,
                                  GQuark key,
                                  GDestroyNotify notify);
void zMapXMLFactory_listAddOutput(zmapXMLFactoryItem item, 
                                  gpointer data);


void zMapXMLFactoryFreeListComplete(zmapXMLFactory factory,
                                    GQuark name,
                                    GFunc func,
                                    gpointer userData);

void zMapXMLFactoryItem_dataListClear(zmapXMLFactoryItem item);

#endif /* ZMAP_XML_H */

