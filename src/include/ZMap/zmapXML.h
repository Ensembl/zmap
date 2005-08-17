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
 * Last edited: Aug 16 18:41 2005 (rds)
 * Created: Tue Aug  2 16:27:08 2005 (rds)
 * CVS info:   $Id: zmapXML.h,v 1.1 2005-08-17 09:10:42 rds Exp $
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

typedef struct _zmapXMLFactoryItemStruct
{
  int    type;
  GList *list;
}zmapXMLFactoryItemStruct, *zmapXMLFactoryItem;


typedef gboolean 
(*zmapXML_StartMarkupObjectHandler)(void *userData, zmapXMLElement element, zmapXMLParser parser);
typedef gboolean 
(*zmapXML_EndMarkupObjectHandler)(void *userData, zmapXMLElement element, zmapXMLParser parser);


/* ATTRIBUTES */
GQuark zMapXMLAttributeValue(zmapXMLAttribute attr);

/* DOCUMENTS */
zmapXMLDocument zMapXMLDocument_create(const XML_Char *version,
                                       const XML_Char *encoding,
                                       int standalone);
void zMapXMLDocument_set_root(zmapXMLDocument doc,
                              zmapXMLElement root);
char *zMapXMLDocument_version(zmapXMLDocument doc);
char *zMapXMLDocument_encoding(zmapXMLDocument doc);
gboolean zMapXMLDocument_is_standalone(zmapXMLDocument doc);
void zMapXMLDocument_reset(zmapXMLDocument doc);
void zMapXMLDocument_destroy(zmapXMLDocument doc);


/* ELEMENTS */
zmapXMLElement zMapXMLElement_nextSibling(zmapXMLElement ele);
zmapXMLElement zMapXMLElement_previousSibling(zmapXMLElement ele);

zmapXMLElement zMapXMLElement_getChildByName(zmapXMLElement parent,
                                             GQuark name);
GList *zMapXMLElement_getChildrenByName(zmapXMLElement parent,
                                        GQuark name,
                                        int expect);

zmapXMLAttribute zMapXMLElement_getAttributeByName(zmapXMLElement ele,
                                                   char *name);

#ifdef RANDASDKASJDFJAJSDASDHJ
GQuark zMapXMLElement_getAttributeQuarkByName(zmapXMLElement element,
                                              GQuark name);
char *zMapXMLElement_getAttributeValueByName(zmapXMLElement element,
                                             GQuark name);
#endif

/* PARSER */
zmapXMLParser zMapXMLParser_create(void *userData, gboolean validating, gboolean debug);

void zMapXMLParser_SetMarkupObjectHandler(zmapXMLParser parser, 
                                          zmapXML_StartMarkupObjectHandler start,
                                          zmapXML_EndMarkupObjectHandler end);

gboolean zMapXMLParser_parse_file(zmapXMLParser parser,
                                  FILE *file);
gboolean zMapXMLParser_parse_buffer(zmapXMLParser parser, 
                                    void *data, 
                                    int size);
zmapXMLElement zMapXMLParser_get_root(zmapXMLParser parser);



int zMapXMLFactoryDecodeElement(GHashTable *userTypesTable, 
                                zmapXMLElement element, 
                                GList **listout);


#endif /* ZMAP_XML_H */

