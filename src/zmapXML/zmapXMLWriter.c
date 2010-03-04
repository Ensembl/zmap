/*  File: zmapXMLWriter.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jun 14 20:10 2007 (rds)
 * Created: Tue Jul 18 16:49:49 2006 (rds)
 * CVS info:   $Id: zmapXMLWriter.c,v 1.7 2010-03-04 15:13:42 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <zmapXML_P.h>
#include <ZMap/zmapUtils.h>

typedef struct
{
  GQuark name;
  GArray *attribute_list;
}ElementStackMemberStruct, *ElementStackMember;

static void maybeFlush(ZMapXMLWriter writer);
static void pushElement(ZMapXMLWriter writer, ElementStackMember element);
static ElementStackMember peekElement(ZMapXMLWriter writer);
static ElementStackMember popElement(ZMapXMLWriter writer, GQuark element_name);
static gboolean attributeIsUnique(ElementStackMember element, GQuark attribute_name);
static void addAttribute(ElementStackMember element, GQuark attribute_name);
static void flushToOutput(ZMapXMLWriter writer);
static void setErrorCode(ZMapXMLWriter writer, ZMapXMLWriterErrorCode code);

ZMapXMLWriter zMapXMLWriterCreate(ZMapXMLWriterOutputCallback flush_callback, 
                                  gpointer flush_data)
{
  ZMapXMLWriter writer = NULL;

  writer = g_new0(ZMapXMLWriterStruct, 1);
  writer->errorCode = ZMAPXMLWRITER_OK;

  writer->xml_output = g_string_sized_new(500);

  writer->element_stack = g_array_sized_new(FALSE, FALSE, sizeof(ElementStackMemberStruct), 512);

  writer->output_callback = flush_callback;
  writer->output_userdata = flush_data;

  return writer;
}

ZMapXMLWriterErrorCode zMapXMLWriterStartElement(ZMapXMLWriter writer, char *element_name)
{
  ZMapXMLWriterErrorCode code;
  ElementStackMemberStruct curr_element = {0};
  int i = 0, depth = 0;

  depth = writer->element_stack->len;

  if(!(writer->stack_top_has_content) && depth > 0)
    {
      g_string_append_c(writer->xml_output, '>');
      g_string_append_c(writer->xml_output, '\n');
    }
  else
    writer->stack_top_has_content = FALSE;

  depth*=2;
  for(i=0;i<depth;i++)
    { g_string_append_c(writer->xml_output, ' '); }

  g_string_append_c(writer->xml_output, '<');

  g_string_append(writer->xml_output, element_name);
  
  curr_element.name = g_quark_from_string(element_name);
  //curr_element.has_content = FALSE; //not needed
  pushElement(writer, &curr_element);

  maybeFlush(writer);
  code = writer->errorCode;
  return code;
}

ZMapXMLWriterErrorCode zMapXMLWriterAttribute(ZMapXMLWriter writer, char *name, char *value)
{
  ZMapXMLWriterErrorCode code = ZMAPXMLWRITER_OK;
  ElementStackMember stack_element = NULL;
  GQuark name_q = 0;

  name_q = g_quark_from_string(name);

  if(!(stack_element = peekElement(writer)))
    setErrorCode(writer, ZMAPXMLWRITER_BAD_POSITION);

  if(code == ZMAPXMLWRITER_OK &&
     attributeIsUnique(stack_element, name_q))
    {
      g_string_append_printf(writer->xml_output, " %s=\"%s\"", name, value);
      addAttribute(stack_element, name_q);

      maybeFlush(writer);
    }
  else
    setErrorCode(writer, ZMAPXMLWRITER_DUPLICATE_ATTRIBUTE);

  code = writer->errorCode;

  return code;
}

ZMapXMLWriterErrorCode zMapXMLWriterElementContent(ZMapXMLWriter writer, char *content)
{
  ZMapXMLWriterErrorCode code;

  if(!(writer->stack_top_has_content))
    {
      writer->stack_top_has_content = TRUE;
      g_string_append_c(writer->xml_output, '>');
    }

  g_string_append(writer->xml_output, content);

  maybeFlush(writer);
  code = writer->errorCode;
  return code;
}

ZMapXMLWriterErrorCode zMapXMLWriterEndElement(ZMapXMLWriter writer, char *element)
{
  ZMapXMLWriterErrorCode code = ZMAPXMLWRITER_OK;
  GQuark name_quark = 0;
  ElementStackMember stack_head = NULL;
  int depth = 0, i = 0;

  name_quark = g_quark_from_string(element);

  if((stack_head = popElement(writer, name_quark)))
    {
      if(!(writer->stack_top_has_content))
        {
          /* g_string_append_c(writer->xml_output, ' '); */
          g_string_append_c(writer->xml_output, '/');
          g_string_append_c(writer->xml_output, '>');
          writer->stack_top_has_content = TRUE;
        }
      else
        {
          depth = (writer->element_stack->len) * 2;
          for(i=0;i<depth;i++)
            { g_string_append_c(writer->xml_output, ' '); }
          g_string_append_printf(writer->xml_output, "</%s>", element);
        }

#ifdef RDS_DONT_INCLUDE_UNUSED
#warning FIX MEM LEAK
      if(stack_head->attribute_list)
        g_array_free(stack_head->attribute_list, TRUE);

      if(stack_head)
        g_free(stack_head);
#endif

      g_string_append_c(writer->xml_output, '\n');
    }
  

  maybeFlush(writer);
  code = writer->errorCode;
  return code;
}

ZMapXMLWriterErrorCode zMapXMLWriterStartDocument(ZMapXMLWriter writer, char *document_root_tag)
{
  ZMapXMLWriterErrorCode code;
  char *version = "1.1", *encoding = "UTF-8";
  char *doctype = NULL;

  g_string_append(writer->xml_output, "<?xml");

  if(version)
    g_string_append_printf(writer->xml_output, " version=\"%s\"", version);
  if(encoding)
    g_string_append_printf(writer->xml_output, " encoding=\"%s\"", encoding);
  
  g_string_append(writer->xml_output, "?>");

  if(doctype)
    g_string_append_printf(writer->xml_output, "%s", doctype);

  maybeFlush(writer);
  code = writer->errorCode;
  return code;
}

ZMapXMLWriterErrorCode zMapXMLWriterEndDocument(ZMapXMLWriter writer)
{
  ZMapXMLWriterErrorCode code;
  char *doc_tag = NULL;

  zMapXMLWriterEndElement(writer, doc_tag);

  flushToOutput(writer);

  code = writer->errorCode;
  return code;
}

/* Event processing code */
/* FIFO logic, processed in exactly the same order as created... */
ZMapXMLWriterErrorCode zMapXMLWriterProcessEvents(ZMapXMLWriter writer, GArray *events)
{
  int event_count = 0, i = 0;
  ZMapXMLWriterEvent event = NULL;
  ZMapXMLWriterErrorCode status = ZMAPXMLWRITER_OK;
  event_count = events->len;

  for(i = 0; ((status == ZMAPXMLWRITER_OK) && (i < event_count)); i++)
    {
      char *first = NULL, *second = NULL;
      event = &(g_array_index(events, ZMapXMLWriterEventStruct, i));
      switch(event->type)
        {
        case ZMAPXML_START_ELEMENT_EVENT:
          first  = (char *)g_quark_to_string(event->data.name);
          status = zMapXMLWriterStartElement(writer, first);
          break;
        case ZMAPXML_END_ELEMENT_EVENT:
          first  = (char *)g_quark_to_string(event->data.name);
          status = zMapXMLWriterEndElement(writer, first);
          break;
        case ZMAPXML_CHAR_DATA_EVENT:
          first  = (char *)g_quark_to_string(event->data.name);
          status = zMapXMLWriterElementContent(writer, first);
          break;
        case ZMAPXML_START_DOC_EVENT:
          first  = (char *)g_quark_to_string(event->data.name);
          status = zMapXMLWriterStartDocument(writer, first);
          break;
        case ZMAPXML_END_DOC_EVENT:
          status = zMapXMLWriterEndDocument(writer);
          break;
        case ZMAPXML_ATTRIBUTE_EVENT:
          {
            gboolean free_second = TRUE;
            first  = (char *)g_quark_to_string(event->data.comp.name);
            
            if (event->data.comp.data == ZMAPXML_EVENT_DATA_QUARK)
              {
                second = (char *)g_quark_to_string(event->data.comp.value.quark);
                free_second = FALSE;
              }
            else if(event->data.comp.data == ZMAPXML_EVENT_DATA_INTEGER)
              second = g_strdup_printf("%d", event->data.comp.value.integer);
            else if(event->data.comp.data == ZMAPXML_EVENT_DATA_DOUBLE)
              second = g_strdup_printf("%f", event->data.comp.value.flt);
            else
              zMapAssertNotReached();

            status = zMapXMLWriterAttribute(writer, first, second);
            if(free_second && second)
              g_free(second);
          }
          break;
        case ZMAPXML_NULL_EVENT:
          break;
        case ZMAPXML_UNSUPPORTED_EVENT:
        case ZMAPXML_UNKNOWN_EVENT:
        default:
          zMapAssertNotReached();
          break;
        }
    }

  if(status == ZMAPXMLWRITER_OK)
    {
      flushToOutput(writer);
      status = writer->errorCode;
    }

  return status;
}

ZMapXMLWriterErrorCode zMapXMLWriterDestroy(ZMapXMLWriter writer)
{
  ZMapXMLWriterErrorCode code = ZMAPXMLWRITER_OK;

  writer->output_callback = writer->output_userdata = NULL;

  g_string_free(writer->xml_output, TRUE);

  if(writer->error_msg)
    g_free(writer->error_msg);

  g_free(writer);

  return code;
}

char *zMapXMLWriterErrorMsg(ZMapXMLWriter writer)
{
  return writer->error_msg;
}

char *zMapXMLWriterVerboseErrorMsg(ZMapXMLWriter writer)
{
  char *verbose;

  verbose = g_strdup_printf("Error (%d) Occurred:\n%s\n%s\n",
                            writer->errorCode,
                            writer->error_msg,
                            writer->xml_output->str);

  return verbose;
}

/* internal code */
static void maybeFlush(ZMapXMLWriter writer)
{
  writer->flush_counter++;

  if(!(writer->flush_counter % ZMAPXMLWRITER_FLUSH_COUNT))
    flushToOutput(writer);

  return ;
}

static void pushElement(ZMapXMLWriter writer, ElementStackMember element)
{
  g_array_append_val(writer->element_stack, *element);
  return ;
}

static ElementStackMember peekElement(ZMapXMLWriter writer)
{
  ElementStackMember peeked = NULL;
  int last = 0;

  last = writer->element_stack->len - 1;

  peeked = &(g_array_index(writer->element_stack, ElementStackMemberStruct, last));

  return peeked;
}

static ElementStackMember popElement(ZMapXMLWriter writer, GQuark element_name)
{
  ElementStackMember popped;
  int last;

  last = writer->element_stack->len - 1;

  popped = &(g_array_index(writer->element_stack, ElementStackMemberStruct, last));

  if(popped->name == element_name)
    g_array_remove_index(writer->element_stack, last);
  else
    {
      popped = NULL;
      setErrorCode(writer, ZMAPXMLWRITER_MISMATCHED_TAG);
    }

  return popped;
}

static gboolean attributeIsUnique(ElementStackMember element, GQuark attribute_name)
{
  gboolean unique = TRUE;

  if(!(element->attribute_list))
    element->attribute_list = g_array_sized_new(FALSE, FALSE, sizeof(GQuark), 20);
  else
    {
      int i, length = element->attribute_list->len;
      for(i = 0; unique && i < length; i++)
        {
          GQuark member = g_array_index(element->attribute_list, GQuark, i);
          if(member == attribute_name)
            unique = FALSE;
        }
    }

  return unique;
}

static void addAttribute(ElementStackMember element, GQuark attribute_name)
{
  if(!(element->attribute_list))
    element->attribute_list = g_array_sized_new(FALSE, FALSE, sizeof(GQuark), 20);

  g_array_append_val(element->attribute_list, attribute_name);

  return ;
}

#ifdef EXAMPLE_MEM_CALLBACK
static int memorycallback(ZMapXMLWriter writer, char *xml, int xml_length, void *user_data);
static int memorycallback(ZMapXMLWriter writer, char *xml, int xml_length, void *user_data)
{
  int length = xml_length;

  

  return length;
}
#endif

static void flushToOutput(ZMapXMLWriter writer)
{
  ZMapXMLWriterErrorCode code;
  int length2flush = 0;

  code = writer->errorCode;

  if(code == ZMAPXMLWRITER_OK && (length2flush = writer->xml_output->len))
    {                           /* Carry on with flushing */
      int length_flushed = 0;
      setErrorCode(writer, ZMAPXMLWRITER_INCOMPLETE_FLUSH);
  
      length_flushed = (writer->output_callback)(writer, 
                                                 writer->xml_output->str, 
                                                 length2flush,
                                                 writer->output_userdata);

      if(length_flushed == length2flush)
        {
          g_string_truncate(writer->xml_output, 0);
          setErrorCode(writer, ZMAPXMLWRITER_OK);
        }
      else
        setErrorCode(writer, ZMAPXMLWRITER_FAILED_FLUSHING);
    }

  return ;
}

static void setErrorCode(ZMapXMLWriter writer, ZMapXMLWriterErrorCode code)
{
  if(!writer->error_msg)
    {
      switch(code)
        {
        case ZMAPXMLWRITER_OK:
        case ZMAPXMLWRITER_INCOMPLETE_FLUSH:
          break;
        case ZMAPXMLWRITER_FAILED_FLUSHING:
          writer->error_msg = g_strdup_printf("ZMapXMLWriter failed flushing output."
                                              " User Callback = %p,"
                                              " User Data = %p.", 
                                              writer->output_callback,
                                              writer->output_userdata);
          break;
        case ZMAPXMLWRITER_MISMATCHED_TAG:
          {
            ElementStackMember popped = NULL;
            int last = 0;
            
            last = writer->element_stack->len - 1;
            
            popped = &(g_array_index(writer->element_stack, ElementStackMemberStruct, last));
            
            writer->error_msg = g_strdup_printf("ZMapXMLWriter encountered a mis-matched tag '%s'",
                                                g_quark_to_string(popped->name));
          }
          break;
        case ZMAPXMLWRITER_DUPLICATE_ATTRIBUTE:
          writer->error_msg = g_strdup("ZMapXMLWriter found a duplicate attribute");
          break;
        case ZMAPXMLWRITER_BAD_POSITION:
          writer->error_msg = g_strdup("ZMapXMLWriter: Element has bad position");
        default:
          writer->error_msg = g_strdup("ZMapXMLWriter: Unknown Error Code.");
          break;
        }
    }
  
  writer->errorCode = code;

  return ;
}
