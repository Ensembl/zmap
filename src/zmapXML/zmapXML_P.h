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
 * Last edited: Sep  5 10:22 2005 (rds)
 * Created: Fri Aug  5 12:50:44 2005 (rds)
 * CVS info:   $Id: zmapXML_P.h,v 1.1 2005-09-05 17:28:22 rds Exp $
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
void zmapXMLElement_addChild(zmapXMLElement parent, zmapXMLElement child);
void zmapXMLElement_removeChild(zmapXMLElement parent, zmapXMLElement child);
void zmapXMLElement_free(zmapXMLElement ele);

/* PARSER */




#endif /* !ZMAP_XML_P_H */




















#ifdef SOME_THOUGHTS_EXAMPLES_OF_WHAT_MIGHT_HAPPEN_HERE

typedef enum {
  ZMAP_OBJ_TYPE_A,
  ZMAP_OBJ_TYPE_B,
  ZMAP_OBJ_TYPE_C,
  ZMAP_OBJ_TYPE_D,
  ZMAP_OBJ_TYPE_E,
  ZMAP_OBJ_LAST
}zmapObjectType;

typedef struct beepBeep
{
  zmapObjectType type;
  char *elementname;
} BeepTable;

typedef struct _zmapFactoryOutputStruct
{
  zmapObjTypeA typeA;
  zmapObjTypeB typeB;
  zmapObjTypeC typeC;
  zmapObjTypeD typeD;
  zmapObjTypeE typeE;
} zmapFactoryOutputStruct, *zmapFactoryOutputStruct;

void factory2create(void *userData, zmapXMLElement element);
void factory2create(void *userData, zmapXMLElement element)
{
  zmapFactoryOutput fo = (zmapFactoryOutput)userData;
  zmapObjectType type;
  /* This probably relies on tree of data....
   */
  type = decodeElement(element);
  switch (type){
  case ZMAP_OBJ_TYPE_A:
    fo->typeA = makeObjectAFrom(element);
    break;
  case ZMAP_OBJ_TYPE_B:
    fo->typeB = makeObjectBFrom(element);
    break;
  case ZMAP_OBJ_TYPE_C:
    fo->typeC = makeObjectCFrom(element);
    break;
  case ZMAP_OBJ_TYPE_D:
    fo->typeD = makeObjectDFrom(element);
    break;
  case ZMAP_OBJ_TYPE_E:
    fo->typeE = makeObjectEFrom(element);
    break;
  default:
    zMapFatal("Something went WRONG");
    break;
  }
}
void factory2complete(void *userData, zmapXMLElement element);
void factory2complete(void *userData, zmapXMLElement element)
{
  zmapFactoryOutput fo = (zmapFactoryOutput)userData;
  zmapObjectType type;
  /* This probably relies on tree of data....
   */
  type = decodeElement(element);
  switch (type){
  case ZMAP_OBJ_TYPE_A:
    completeObjectA(fo->typeA, element);
    draw_or_do_something_else_with_it(fo->typeA);
    fo->typeA = NULL;
    break;
  case ZMAP_OBJ_TYPE_B:
    /* Same here, but for B... */
    break;
  case ZMAP_OBJ_TYPE_C:
    break;
  case ZMAP_OBJ_TYPE_D:
    break;
  case ZMAP_OBJ_TYPE_E:
    break;
  default:
    zMapFatal("Something went WRONG");
    break;
  }

}

zmapObjectType decodeElement(zmapXMLElement element)
{
  zmapObjectType type = ZMAP_OBJ_UNKNOWN;
  GList *interestingElements = NULL;
  int i = 0;
  gboolean escape = FALSE;

  interestingElements = getElementNames();
  /* should step through ourselves, probably */
  while(notEndOfList && !escape)
    {
      if(g_quark_from_string(listItem) == element->name)
        {
          type = (zmapObjectType)i;
          escape = TRUE;
        }
      i++;
    }
  if(type >= ZMAP_OBJ_LAST)
    zMapFatal("Inconsistency.\n");

  return type;
}

zmapObjectType decodeElement(zmapXMLElement element)
{
  static beepBeep[] = {
    {ZMAP_OBJ_TYPE_A, "objectTypeA"},
    {ZMAP_OBJ_TYPE_B, "objectTypeB"},
    {ZMAP_OBJ_TYPE_C, "objectTypeC"},
    {ZMAP_OBJ_TYPE_D, "objectTypeD"},
    {ZMAP_OBJ_TYPE_E, "objectTypeE"},    
    {ZMAP_OBJ_LAST, NULL}
  };
  int i;
  zmapObjectType type;
  for(i = 0; beepBeep[i] != ZMAP_OBJ_LAST; i++)
    {
      if(element->name = g_quark_from_string((beepBeep[i])->elementname))
        {
          type = i;
          i = ZMAP_OBJ_LAST;
        }
    }
  return type;
}
#endif /* SOME_THOUGHTS_EXAMPLES_OF_WHAT_MIGHT_HAPPEN_HERE */

