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
 * Last edited: Feb 10 08:08 2006 (rds)
 * Created: Tue Aug  2 16:27:08 2005 (rds)
 * CVS info:   $Id: zmapXML.h,v 1.6 2006-02-10 08:09:18 rds Exp $
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


typedef gboolean 
(*ZMapXMLMarkupObjectHandler)(void *userData, zmapXMLElement element, zmapXMLParser parser);

typedef struct ZMapXMLObjTagFunctionsStruct_
{
  char *element_name;
  ZMapXMLMarkupObjectHandler handler;
} ZMapXMLObjTagFunctionsStruct, *ZMapXMLObjTagFunctions;


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
                                          ZMapXMLMarkupObjectHandler start,
                                          ZMapXMLMarkupObjectHandler end);
void zMapXMLParser_setMarkupObjectTagHandlers(zmapXMLParser parser,
                                              ZMapXMLObjTagFunctions starts,
                                              ZMapXMLObjTagFunctions end);

gboolean zMapXMLParser_parseFile(zmapXMLParser parser,
                                  FILE *file);
gboolean zMapXMLParser_parseBuffer(zmapXMLParser parser, 
                                    void *data, 
                                    int size);
char *zMapXMLParser_lastErrorMsg(zmapXMLParser parser);

zmapXMLElement zMapXMLParser_getRoot(zmapXMLParser parser);

char *zMapXMLParserGetBase(zmapXMLParser parser);

gboolean zMapXMLParser_reset(zmapXMLParser parser);
void zMapXMLParser_destroy(zmapXMLParser parser);

#endif /* ZMAP_XML_H */

