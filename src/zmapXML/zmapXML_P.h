/*  File: zmapXML_P.h
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
 * Last edited: Sep 14 13:23 2005 (rds)
 * Created: Fri Aug  5 12:50:44 2005 (rds)
 * CVS info:   $Id: zmapXML_P.h,v 1.2 2005-09-20 17:18:11 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_XML_P_H
#define ZMAP_XML_P_H

#include <ZMap/zmapXML.h>

/*
  typedef enum {SAXPARSE_NOWHERE, 
                SAXPARSE_STARTDOC,
                SAXPARSE_START, 
                SAXPARSE_INSIDE, 
                SAXPARSE_END, 
                SAXPARSE_ENDDOC} SaxParseState ;
*/
/* For an XML tag holds the element name and attributes, i.e.   <element_name attributes>
 * Used to track which tag we are processing. */


typedef struct _zmapXMLDocumentStruct
{
  GQuark version;
  GQuark encoding;
  int standalone;
  zmapXMLElement root;
} zmapXMLDocumentStruct;

typedef struct _zmapXMLAttributeStruct
{
  GQuark name;
  GQuark value;
} zmapXMLAttributeStruct;

typedef struct _zmapXMLParserStruct
{
  XML_Parser expat ; 
  gboolean debug, validating;

  GQueue *elementStack ;        /* Stack of zmapXMLElementStructs */
#if GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION < 4
#define ZMAP_XML_PARSE_USE_DEPTH
  int depth;                    /* Annoyingly g_queue_get_length is a 2.4 addition,
                                 * so it turns out we need to know how deep we are
                                 * using this int. (Grrr.)
                                 */
#endif
  zmapXMLDocument document;

  char *last_errmsg ;
  void *user_data ;       /* Caller stores any data they need here. */

  zmapXML_StartMarkupObjectHandler startMOHandler;
  zmapXML_EndMarkupObjectHandler   endMOHandler;

} zmapXMLParserStruct ;


typedef struct _zmapXMLFactoryStruct
{
  GHashTable *hash;
  gboolean isLite;
  gpointer userData;            /* pointer to the user's data */
}zmapXMLFactoryStruct;

typedef struct _zmapXMLFactoryLiteItemStruct
{
  int    tag_type;
  GList *list;
}zmapXMLFactoryLiteItemStruct;

typedef struct _zmapXMLFactoryItemStruct
{
  zmapXMLFactory parent;       /* pointer to the hash that holds us */

  zmapXMLFactoryFunc begin;
  zmapXMLFactoryFunc end;

  zmapXMLFactoryStorageType datatype; /* What's in the below */
  union{
    GList *list;
    GData *datalist;
  } storage;

}zmapXMLFactoryItemStruct;


/* ATTRIBUTES */
zmapXMLAttribute zmapXMLAttribute_create(const XML_Char *name,
                                         const XML_Char *value);
void zmapXMLAttribute_free(zmapXMLAttribute attr);

/* ELEMENTS */
zmapXMLElement zmapXMLElement_create(const XML_Char *name);
gboolean zmapXMLElement_addAttribute(zmapXMLElement ele, zmapXMLAttribute attr);
gboolean zmapXMLElement_addAttrPair(zmapXMLElement ele,
                                    const XML_Char *name,
                                    const XML_Char *value);
void zmapXMLElement_addContent(zmapXMLElement ele, 
                               const XML_Char *content,
                               int len);
void zmapXMLElement_addChild(zmapXMLElement parent, zmapXMLElement child);
gboolean zmapXMLElement_signalParentChildFree(zmapXMLElement child);
void zmapXMLElement_free(zmapXMLElement ele);
void zmapXMLElement_freeAttrList(zmapXMLElement ele);

/* PARSER */


/* FACTORY */

gboolean zmapXML_FactoryEndHandler(void *userData,
                                  zmapXMLElement element,
                                  zmapXMLParser parser);
gboolean zmapXML_FactoryStartHandler(void *userData,
                                    zmapXMLElement element,
                                    zmapXMLParser parser);


#endif /* !ZMAP_XML_P_H */







#ifdef NEWFACTORYINTERFACE
/* Lite Items */

void zmapXMLFactoryDataAddItem(zmapXMLFactoryItem item, 
                               gpointer output,
                               GQuark key);
void zmapXMLFactoryListAddItem(zmapXMLFactoryItem item, 
                               gpointer output);

#endif
