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
 * Last edited: May 19 22:12 2006 (rds)
 * Created: Fri Aug  5 12:50:44 2005 (rds)
 * CVS info:   $Id: zmapXML_P.h,v 1.9 2006-05-22 09:28:03 rds Exp $
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
  ZMapXMLElement root;
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
  ZMapXMLDocument document;

  char *last_errmsg, *aborted_msg ;
  void *user_data ;       /* Caller stores any data they need here. */

  GArray *elements, *attributes;
  int max_size_e, max_size_a;   /* Maximum number of elements and attributes */
  int array_point_e;

  ZMapXMLMarkupObjectHandler startMOHandler;
  ZMapXMLMarkupObjectHandler   endMOHandler;

  /* Hopefully these will replace the two above! */
  GList *startTagHandlers, *endTagHandlers;

  GList *xmlBaseHandlers, *xmlBaseStack;
  GQuark xmlbase;
} zmapXMLParserStruct ;


/* ATTRIBUTES */
ZMapXMLAttribute zmapXMLAttributeCreate(const XML_Char *name,
                                         const XML_Char *value);
void zmapXMLAttributeMarkDirty(ZMapXMLAttribute attr);
void zmapXMLAttributeFree(ZMapXMLAttribute attr);

/* ELEMENTS */
ZMapXMLElement zmapXMLElementCreate(const XML_Char *name);
gboolean zmapXMLElementAddAttribute(ZMapXMLElement ele, ZMapXMLAttribute attr);
gboolean zmapXMLElementAddAttrPair(ZMapXMLElement ele,
                                    const XML_Char *name,
                                    const XML_Char *value);
void zmapXMLElementAddContent(ZMapXMLElement ele, 
                               const XML_Char *content,
                               int len);
void zmapXMLElementAddChild(ZMapXMLElement parent, ZMapXMLElement child);
gboolean zmapXMLElementSignalParentChildFree(ZMapXMLElement child);
void zmapXMLElementMarkDirty(ZMapXMLElement ele);
void zmapXMLElementMarkAttributesDirty(ZMapXMLElement ele);
void zmapXMLElementFree(ZMapXMLElement ele);
void zmapXMLElementFreeAttrList(ZMapXMLElement ele);

/* PARSER */

#endif /* !ZMAP_XML_P_H */

