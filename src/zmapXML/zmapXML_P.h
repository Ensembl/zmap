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
 * Last edited: Mar  1 19:02 2006 (rds)
 * Created: Fri Aug  5 12:50:44 2005 (rds)
 * CVS info:   $Id: zmapXML_P.h,v 1.7 2006-03-02 14:24:14 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_XML_P_H
#define ZMAP_XML_P_H

#include <ZMap/zmapXML.h>

#define ZMAP_XML_BASE_ATTR "xml:base"
#define ZMAP_XML_ERROR_CONTEXT_SIZE 10

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
  gboolean dirty;
  GQuark name;
  GQuark value;
} zmapXMLAttributeStruct;

typedef struct _zmapXMLParserStruct
{
  XML_Parser expat ; 
  gboolean debug, validating, useXMLBase;

  GQueue *elementStack ;        /* Stack of zmapXMLElementStructs */
#if GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION < 4
#define ZMAP_XML_PARSE_USE_DEPTH
  int depth;                    /* Annoyingly g_queue_get_length is a 2.4 addition,
                                 * so it turns out we need to know how deep we are
                                 * using this int. (Grrr.)
                                 */
#endif
  zmapXMLDocument document;

  char *last_errmsg, *aborted_msg ;
  void *user_data ;       /* Caller stores any data they need here. */

  GArray *elements, *attributes;
  int max_size;

  ZMapXMLMarkupObjectHandler startMOHandler;
  ZMapXMLMarkupObjectHandler   endMOHandler;

  /* Hopefully these will replace the two above! */
  GList *startTagHandlers, *endTagHandlers;

  GList *xmlBaseHandlers, *xmlBaseStack;
  GQuark xmlbase;
} zmapXMLParserStruct ;


/* ATTRIBUTES */
zmapXMLAttribute zmapXMLAttribute_create(const XML_Char *name,
                                         const XML_Char *value);
void zmapXMLAttributeMarkDirty(zmapXMLAttribute attr);
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
void zmapXMLElementMarkDirty(zmapXMLElement ele);
void zmapXMLElement_free(zmapXMLElement ele);
void zmapXMLElement_freeAttrList(zmapXMLElement ele);

/* PARSER */

#endif /* !ZMAP_XML_P_H */

