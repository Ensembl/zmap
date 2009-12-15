/*  File: zmapXML.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * Last edited: Jun 27 17:34 2007 (rds)
 * Created: Tue Aug  2 16:27:08 2005 (rds)
 * CVS info:   $Id: zmapXML.h,v 1.23 2009-12-15 13:49:10 mh17 Exp $
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
 *
 * This should be read as 
 * zMapXMLParserCheck, if true, error and return TRUE.
 */
#define zMapXMLParserCheckIfTrueErrorReturn(EXPR, PARSER, MESSAGE) \
G_STMT_START{                                                      \
  if ((EXPR))                                                      \
    {                                                              \
      zMapXMLParserRaiseParsingError(PARSER, MESSAGE);             \
      return TRUE;                                                 \
    };                                                             \
}G_STMT_END

#define zMapXMLIsYes(STRING)                                       \
((g_ascii_strncasecmp(STRING, "yes", 3) == 0)  ? TRUE : FALSE)
#define zMapXMLIsTrue(STRING)                                      \
((g_ascii_strncasecmp(STRING, "true", 4) == 0) ? TRUE : FALSE)
#define zMapXMLIsTrueInt(STRING)                                   \
((g_ascii_strncasecmp(STRING, "1", 1) == 0)    ? TRUE : FALSE)

#define zMapXMLStringToBool(STRING)                                \
(zMapXMLIsTrueInt(STRING) ? TRUE :                                 \
  zMapXMLIsYes(STRING) ? TRUE :                                    \
    zMapXMLIsTrue(STRING) ? TRUE : FALSE)

#define zMapXMLElementContentsToInt(ELEMENT)        \
(strtol(ELEMENT->contents->str, (char **)NULL, 10))
#define zMapXMLElementContentsToDouble(ELEMENT)     \
(g_ascii_strtod(ELEMENT->contents->str, (char **)NULL))
#define zMapXMLElementContentsToBool(ELEMENT)       \
(zMapXMLStringToBool(ELEMENT->contents->str))

#define zMapXMLAttributeValueToStr(ATTRIBUTE)       \
  ((char *)g_quark_to_string(zMapXMLAttributeGetValue(ATTRIBUTE)))
#define zMapXMLAttributeValueToInt(ATTRIBUTE)       \
  (strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(ATTRIBUTE)), (char **)NULL, 10))
#define zMapXMLAttributeValueToDouble(ATTRIBUTE)       \
  (g_ascii_strtod(((char *)g_quark_to_string(zMapXMLAttributeGetValue(ATTRIBUTE))), (char **)NULL))
#define zMapXMLAttributeValueToBool(ATTRIBUTE)      \
  (zMapXMLStringToBool((char *)g_quark_to_string(zMapXMLAttributeGetValue(ATTRIBUTE))))

/* TYPEDEFS */

/*!
 * \brief A XML element object
 */
typedef struct _zmapXMLElementStruct   *ZMapXMLElement;
typedef struct _zmapXMLElementStruct 
{
  gboolean dirty;               /* internal flag */
  GQuark   name;                /* element name */
  GString *contents;            /* contents <first>This is content</first> */
  GList   *attributes;          /* zmapXMLAttribute list */

  ZMapXMLElement parent;        /* parent element */
  GList   *children;            /* child elements */
  gboolean contents_stolen;
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
typedef struct _zmapXMLAttributeStruct *ZMapXMLAttribute;
typedef struct _zmapXMLDocumentStruct  *ZMapXMLDocument;
typedef struct _zmapXMLParserStruct    *ZMapXMLParser;
typedef struct _ZMapXMLWriterStruct    *ZMapXMLWriter;

typedef enum 
  {
    ZMAPXMLWRITER_OK                = 0, 
    ZMAPXMLWRITER_FAILED_FLUSHING   = 101,
    ZMAPXMLWRITER_INCOMPLETE_FLUSH,
    ZMAPXMLWRITER_BAD_POSITION,
    ZMAPXMLWRITER_DUPLICATE_ATTRIBUTE,
    ZMAPXMLWRITER_MISMATCHED_TAG
  } ZMapXMLWriterErrorCode;

typedef enum
  { 
    ZMAPXML_NULL_EVENT          = 0,

    ZMAPXML_START_ELEMENT_EVENT = 1 << 0,
    ZMAPXML_ATTRIBUTE_EVENT     = 1 << 1 ,
    ZMAPXML_CHAR_DATA_EVENT     = 1 << 2,
    ZMAPXML_END_ELEMENT_EVENT   = 1 << 3,
    ZMAPXML_START_DOC_EVENT     = 1 << 4,
    ZMAPXML_END_DOC_EVENT       = 1 << 5,

    ZMAPXML_UNSUPPORTED_EVENT   = 1 << 15,
    ZMAPXML_UNKNOWN_EVENT       = 1 << 16
  } ZMapXMLWriterEventType;

typedef enum
  {
    ZMAPXML_EVENT_DATA_NONE,
    ZMAPXML_EVENT_DATA_QUARK,
    ZMAPXML_EVENT_DATA_INTEGER,
    ZMAPXML_EVENT_DATA_DOUBLE,
    ZMAPXML_EVENT_DATA_INVALID
  } ZMapXMLWriterEventDataType;

typedef struct _ZMapXMLUtilsEventStackStruct
{
  ZMapXMLWriterEventType event_type;
  char *name;
  ZMapXMLWriterEventDataType data_type;
  union
  {
    char  *s;
    int    i;
    double d;
  }value;

}ZMapXMLUtilsEventStackStruct, *ZMapXMLUtilsEventStack;

typedef struct _ZMapXMLWriterEventStruct
{
  ZMapXMLWriterEventType type ;

  union
  {
    GQuark name;              /* simple string for element names etc... */

    struct
    {
      GQuark name;
      ZMapXMLWriterEventDataType data;
      union
      {
        GQuark quark   ;
        int    integer ;
        double flt;
      } value;
    } comp ;                    /* complex for attributes and namespaced elements */

  } data ;

} ZMapXMLWriterEventStruct, *ZMapXMLWriterEvent ;


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
(*ZMapXMLMarkupObjectHandler)(void *userData, ZMapXMLElement element, ZMapXMLParser parser);

typedef int 
(*ZMapXMLWriterOutputCallback)(ZMapXMLWriter writer, char *flushed_xml, int flushed_length, gpointer user_data);


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
GQuark zMapXMLAttributeGetValue(ZMapXMLAttribute attr);



/* DOCUMENTS */
ZMapXMLDocument zMapXMLDocumentCreate(const XML_Char *version,
                                       const XML_Char *encoding,
                                       int standalone);
void zMapXMLDocumentSetRoot(ZMapXMLDocument doc,
                              ZMapXMLElement root);
char *zMapXMLDocumentVersion(ZMapXMLDocument doc);
char *zMapXMLDocumentEncoding(ZMapXMLDocument doc);
gboolean zMapXMLDocumentIsStandalone(ZMapXMLDocument doc);
void zMapXMLDocumentReset(ZMapXMLDocument doc);
void zMapXMLDocumentDestroy(ZMapXMLDocument doc);



/* ELEMENTS */
ZMapXMLElement zMapXMLElementNextSibling(ZMapXMLElement ele);
ZMapXMLElement zMapXMLElementPreviousSibling(ZMapXMLElement ele);

ZMapXMLElement zMapXMLElementGetChildByPath(ZMapXMLElement parent,
                                             char *path);

ZMapXMLElement zMapXMLElementGetChildByName(ZMapXMLElement parent,
                                             char *name);
ZMapXMLElement zMapXMLElementGetChildByName1(ZMapXMLElement parent,
                                              GQuark name);
GList *zMapXMLElementGetChildrenByName(ZMapXMLElement parent,
                                        GQuark name,
                                        int expect);
ZMapXMLAttribute zMapXMLElementGetAttributeByName(ZMapXMLElement ele,
                                                   char *name);
ZMapXMLAttribute zMapXMLElementGetAttributeByName1(ZMapXMLElement ele,
                                                    GQuark name);
char *zMapXMLElementStealContent(ZMapXMLElement element);


/* PARSER */
ZMapXMLParser zMapXMLParserCreate(void *userData, gboolean validating, gboolean debug);
void zMapXMLParserSetUserData(ZMapXMLParser parser, void *user_data);

void zMapXMLParserSetMarkupObjectHandler(ZMapXMLParser parser, 
                                         ZMapXMLMarkupObjectHandler start,
                                         ZMapXMLMarkupObjectHandler end);
void zMapXMLParserSetMarkupObjectTagHandlers(ZMapXMLParser parser,
                                             ZMapXMLObjTagFunctions starts,
                                             ZMapXMLObjTagFunctions end);

gboolean zMapXMLParserParseFile(ZMapXMLParser parser,
                                FILE *file);
gboolean zMapXMLParserParseBuffer(ZMapXMLParser parser, 
                                  void *data, 
                                  int size);
char *zMapXMLParserLastErrorMsg(ZMapXMLParser parser);

void zMapXMLParserPauseParsing(ZMapXMLParser parser);

void zMapXMLParserRaiseParsingError(ZMapXMLParser parser, 
                                    char *error_string);

ZMapXMLElement zMapXMLParserGetRoot(ZMapXMLParser parser);

gboolean zMapXMLParserReset(ZMapXMLParser parser);
void zMapXMLParserDestroy(ZMapXMLParser parser);


/* Expat stuff we need to be able to get hold of given a ZMapXMLParser
 * when that type is opaque. e.g. in handlers */
/*!
 * \brief calls XML_GetBase assuming parser is valid
 * \param parser
 * \retval char * as XML_GetBase would
 */
char *zMapXMLParserGetBase(ZMapXMLParser parser);
/*!
 * \brief calls XML_GetCurrentByteIndex assuming parser is valid
 * \param parser
 * \retval long as XML_GetCurrentByteIndex would
 */
long zMapXMLParserGetCurrentByteIndex(ZMapXMLParser parser);


/*!
 * \brief Access to the xml at offset.
 * \param parser object
 * \param offset where element begins
 * \retval char * xml which needs to be free'd 

 * Useful if you need to preserve a slice of XML that you don't know
 * how to parse for a round trip.
 */
char *zMapXMLParserGetFullXMLTwig(ZMapXMLParser parser, int offset);


/* WRITER */
ZMapXMLWriter zMapXMLWriterCreate(ZMapXMLWriterOutputCallback flush_callback, 
                                  gpointer flush_data);
ZMapXMLWriterErrorCode zMapXMLWriterStartElement(ZMapXMLWriter writer, char *element_name);
ZMapXMLWriterErrorCode zMapXMLWriterAttribute(ZMapXMLWriter writer, char *name, char *value);
ZMapXMLWriterErrorCode zMapXMLWriterElementContent(ZMapXMLWriter writer, char *content);
ZMapXMLWriterErrorCode zMapXMLWriterEndElement(ZMapXMLWriter writer, char *element);
ZMapXMLWriterErrorCode zMapXMLWriterEndDocument(ZMapXMLWriter writer);
ZMapXMLWriterErrorCode zMapXMLWriterStartDocument(ZMapXMLWriter writer, char *document_root_tag);
ZMapXMLWriterErrorCode zMapXMLWriterProcessEvents(ZMapXMLWriter writer, GArray *events);
ZMapXMLWriterErrorCode zMapXMLWriterDestroy(ZMapXMLWriter writer);
char *zMapXMLWriterErrorMsg(ZMapXMLWriter writer);
char *zMapXMLWriterVerboseErrorMsg(ZMapXMLWriter writer);

/* UTILS */

GArray *zMapXMLUtilsCreateEventsArray(void);
GArray *zMapXMLUtilsAddStackToEventsArray(ZMapXMLUtilsEventStackStruct *event_stack,
                                          GArray *events_array);
GArray *zMapXMLUtilsAddStackToEventsArrayStart(ZMapXMLUtilsEventStackStruct *event_stack,
                                               GArray *events_array);
GArray *zMapXMLUtilsStackToEventsArray(ZMapXMLUtilsEventStackStruct *event_stack);

#endif /* ZMAP_XML_H */

/*! @} end of zmapXML docs  */
