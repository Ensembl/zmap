/*  Last edited: Oct 28 14:14 2011 (edgrif) */
/*  File: example_xml_writer.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <ZMap/zmapXML.h>



static GArray *simple_xml_document(void);
static GArray *complex_xml_document(void);
static GArray *xml_document_from_xml(void);
static GArray *multiple_elements(GArray *document);
static int event_handler(ZMapXMLWriter writer, char *xml, int len, gpointer user_data);




int main(int argc, char **argv)
{
  int status = 0;
  ZMapXMLWriterErrorCode xml_status;
  ZMapXMLWriter writer;
  GString *buffer;
  GArray *events;

  buffer = g_string_sized_new(500);

  if((writer = zMapXMLWriterCreate(event_handler, buffer)))
    {
      /* A SIMPLE XML DOCUMENT */
      events = simple_xml_document();

      if((xml_status = zMapXMLWriterProcessEvents(writer, events, FALSE)) == ZMAPXMLWRITER_OK)
        printf("\nXMLDocument:\n%s", buffer->str);
      else
        printf("%s\n", zMapXMLWriterErrorMsg(writer));

      g_array_free(events, TRUE);
      g_string_truncate(buffer, 0);

      /* A MORE COMPLEX XML DOCUMENT */
      events = complex_xml_document();

      if((xml_status = zMapXMLWriterProcessEvents(writer, events, FALSE)) == ZMAPXMLWRITER_OK)
        printf("\nXMLDocument:\n%s", buffer->str);
      else
        printf("%s\n", zMapXMLWriterVerboseErrorMsg(writer));

      g_array_free(events, TRUE);
      g_string_truncate(buffer, 0);


      /* PARSE AND OUTPUT AN XML DOCUMENT */
      if(0)
        events = xml_document_from_xml();


      if((xml_status = zMapXMLWriterProcessEvents(writer, events, FALSE)) == ZMAPXMLWRITER_OK)
        printf("XMLDocument:\n\n%s", buffer->str);
      else
        printf("%s\n", zMapXMLWriterVerboseErrorMsg(writer));
    }

  return status;
}


static GArray *simple_xml_document(void)
{
  GArray *events;
  static ZMapXMLUtilsEventStackStruct document[] =
    {
      /* event type,                attr/ele name,  data type,        qdata, idata, ddata */
      {ZMAPXML_START_ELEMENT_EVENT, "zmap",   ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "action", ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_END_ELEMENT_EVENT,   "zmap",   ZMAPXML_EVENT_DATA_NONE,  {0}},
      {0}
    };

  document[1].value.s = g_strdup("run") ;

  events = zMapXMLUtilsStackToEventsArray(&document[0]);

  return events;
}

static GArray *complex_xml_document(void)
{
  GArray *events;
  static ZMapXMLUtilsEventStackStruct document_start[] =
    {
      /* event type,                attr/ele name,  data type,         {data} */
      {ZMAPXML_START_ELEMENT_EVENT, "set",    ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "align",  ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "block",  ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_END_ELEMENT_EVENT,   "set",    ZMAPXML_EVENT_DATA_QUARK, {0}},
      {0}
    };
  static ZMapXMLUtilsEventStackStruct document_end[] =
    {
      {ZMAPXML_END_ELEMENT_EVENT, "zmap",   ZMAPXML_EVENT_DATA_NONE,  {0}},
      {0}
    };
  static ZMapXMLUtilsEventStackStruct document_start_start[] =
    {
      /* event type,                attr/ele name,  data type,         {data} */
      {ZMAPXML_START_ELEMENT_EVENT, "zmap",   ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "action", ZMAPXML_EVENT_DATA_QUARK, {0}},
      {0}
    };

  document_start[1].value.s = g_strdup("chrX.1000-10000") ;
  document_start[2].value.s = g_strdup("1.0_1.0") ;
  events = zMapXMLUtilsStackToEventsArray(&document_start[0]);

  events = multiple_elements(events);

  events = zMapXMLUtilsAddStackToEventsArrayEnd(events, &document_end[0]);

  document_start[1].value.s = g_strdup("paint") ;
  events = zMapXMLUtilsAddStackToEventsArrayStart(events, &document_start_start[0]);

  return events;
}

static GArray *xml_document_from_xml(void)
{

#ifdef NOT_SURE_THIS_WORTH_IT
  GArray *events;

  static char *xml_frag = "<zmap> <request action=\"delete\"> <set>";
  static ZMapXMLUtilsEventStackStruct document_end[] =
    {
      {ZMAPXML_END_ELEMENT_EVENT, "set",    ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_END_ELEMENT_EVENT, "zmap",   ZMAPXML_EVENT_DATA_NONE,  {0}},
      {0}
    };

  events = zMapXMLUtilsEventsFromXMLFragment(xml_frag);

  events = multiple_elements(events);

  events = zMapXMLUtilsAddStackToEventsArrayEnd(events, &document_end[0]);

  return events;
#else
  return NULL;
#endif
}

static GArray *multiple_elements(GArray *document)
{
  static ZMapXMLUtilsEventStackStruct elements[] =
    {
      {ZMAPXML_START_ELEMENT_EVENT, "feature",  ZMAPXML_EVENT_DATA_NONE,    {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "start",    ZMAPXML_EVENT_DATA_INTEGER, {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "end",      ZMAPXML_EVENT_DATA_INTEGER, {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "ontology", ZMAPXML_EVENT_DATA_QUARK,   {0}},
      {ZMAPXML_END_ELEMENT_EVENT,   "feature",  ZMAPXML_EVENT_DATA_NONE,    {0}},
      {0}
    }, *event;
  static int features[] =
    {12345, 12555, 23456, 23666, 34567, 34777, 0, 0}, *tmp;

  elements[3].value.s = g_strdup("exon") ;

  tmp = &features[0];

  while(tmp && *tmp != 0)
    {
      event = &elements[0];
      event++;

      event->value.i = *tmp;

      event++;
      tmp++;

      event->value.i = *tmp;

      document = zMapXMLUtilsAddStackToEventsArrayEnd(document, &elements[0]);
      tmp++;
    }

  return document;
}

static int event_handler(ZMapXMLWriter writer, char *xml, int len, gpointer user_data)
{
  GString *buffer = (GString *)user_data;

  g_string_append_len(buffer, xml, len);

  return len;
}
