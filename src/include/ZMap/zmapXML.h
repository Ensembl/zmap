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
 * Last edited: May 17 17:49 2006 (rds)
 * Created: Tue Aug  2 16:27:08 2005 (rds)
 * CVS info:   $Id: zmapXML.h,v 1.9 2006-05-17 16:53:13 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_XML_H
#define ZMAP_XML_H

#include <stdio.h>
#include <expat.h>
#include <glib.h>

/*! @addtogroup zmapXML
 * @{
 * */

/*! 
 * \brief macro to abort parsing if expression is true.
 * \param expression to test
 * \param zmapXMLParser parser object
 * \param char *message which will be returned to user
 * \retval TRUE also returning from calling function
 
 * Without a validating parser it's difficult to ensure everything is
 * as it seems.  When writing start and end handlers for nested
 * elements checking correct level of nesting and that all parent
 * object exist is required.  In order to return control back to the
 * parser user level handlers need to explicitly return as just
 * calling parser STOP is not enough as it only flags to the parser
 * not to call the next handler.  The macro provides simple utility
 * access to test the expression, raise an error (message) and return
 * TRUE from the handler if the expression is true.
 */
#define zMapXMLParserCheckIfTrueErrorReturn(EXPR, PARSER, MESSAGE) \
G_STMT_START{                                                      \
  if ((EXPR))                                                      \
    {                                                              \
      zMapXMLParserRaiseParsingError(PARSER, MESSAGE);             \
      return TRUE;                                                 \
    };                                                             \
}G_STMT_END


/* TYPEDEFS */

/*!
 * \brief A XML element object
 */
typedef struct _zmapXMLElementStruct   *zmapXMLElement;
typedef struct _zmapXMLElementStruct 
{
  gboolean dirty;               /* internal flag */
  GQuark   name;                /* element name */
  GString *contents;            /* contents <first>This is content</first> */
  GList   *attributes;          /* zmapXMLAttribute list */

  zmapXMLElement parent;        /* parent element */
  GList   *children;            /* child elements */
} zmapXMLElementStruct;

/*!
 * \typedef zmapXMLAttribute
 * \brief Opaque XML attribute object
 *
 * \typedef zmapXMLDocument
 * \brief Opaque XML document object
 *
 * \typedef zmapXMLParser
 * \brief Opaque XML parser object
 *
 */
typedef struct _zmapXMLAttributeStruct *zmapXMLAttribute;
typedef struct _zmapXMLDocumentStruct  *zmapXMLDocument;
typedef struct _zmapXMLParserStruct    *zmapXMLParser;

/*!
 * \brief XML object handler.

 * If XML is broken into a series of objects, a collection of
 * properties between like elements then it's sensible to get your
 * handler called when an object is created (start handler) and
 * initialised (end handler).  Everything you want should then be
 * within that object.  Of course there may be internal dependencies
 * within the xml document that can't be resolved until the end of the
 * document, but these links maybe resolved in the last end handler.

 * Viewing XML as a collection of objects you may limit the number of
 * handlers that get called. Internally the handlers are still called,
 * it's just that as a user of this package you don't need to handle
 * everything in your handlers.

 */
typedef gboolean 
(*ZMapXMLMarkupObjectHandler)(void *userData, zmapXMLElement element, zmapXMLParser parser);

/*!
 * \brief Small struct to link element names to their handlers
 * pass a pointer to an array of these to
 * zMapXMLParser_setMarkupObjectTagHandlers
 */
typedef struct ZMapXMLObjTagFunctionsStruct_
{
  char *element_name;
  ZMapXMLMarkupObjectHandler handler;
} ZMapXMLObjTagFunctionsStruct, *ZMapXMLObjTagFunctions;


/* ATTRIBUTES */
/*!
 * \brief get an attribute's value
 * \param attribute
 */
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

void zMapXMLParserRaiseParsingError(zmapXMLParser parser, 
                                    char *error_string);

zmapXMLElement zMapXMLParser_getRoot(zmapXMLParser parser);

gboolean zMapXMLParser_reset(zmapXMLParser parser);
void zMapXMLParser_destroy(zmapXMLParser parser);


/* Expat stuff we need to be able to get hold of given a zmapXMLParser
 * when that type is opaque. e.g. in handlers */
/*!
 * \brief calls XML_GetBase assuming parser is valid
 * \param parser
 * \retval char * as XML_GetBase would
 */
char *zMapXMLParserGetBase(zmapXMLParser parser);
/*!
 * \brief calls XML_GetCurrentByteIndex assuming parser is valid
 * \param parser
 * \retval long as XML_GetCurrentByteIndex would
 */
long zMapXMLParserGetCurrentByteIndex(zmapXMLParser parser);


/*!
 * \brief Access to the xml at offset.
 * \param parser object
 * \param offset where element begins
 * \retval char * xml which needs to be free'd 

 * Useful if you need to preserve a slice of XML that you don't know
 * how to parse for a round trip.
 */
char *zMapXMLParserGetFullXMLTwig(zmapXMLParser parser, int offset);

#endif /* ZMAP_XML_H */

/*! @} end of zmapXML docs  */
