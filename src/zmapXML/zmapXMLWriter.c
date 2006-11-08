/*  File: zmapXMLWriter.c
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Oct 24 12:07 2006 (rds)
 * Created: Tue Jul 18 16:49:49 2006 (rds)
 * CVS info:   $Id: zmapXMLWriter.c,v 1.4 2006-11-08 09:25:40 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <zmapXML_P.h>
#include <ZMap/zmapUtils.h>

typedef struct
{
  GQuark name;
  GArray *attribute_list;
}ElementStackMemberStruct, *ElementStackMember;

void my_flush(ZMapXMLWriter writer);
void my_push_onto_stack(ZMapXMLWriter writer, ElementStackMember element);
ElementStackMember my_peek_stack(ZMapXMLWriter writer);
ElementStackMember my_pop_off_stack(ZMapXMLWriter writer, GQuark element_name);
gboolean my_attribute_is_unique(ElementStackMember element, GQuark attribute_name);
void my_attribute_add(ElementStackMember element, GQuark attribute_name);
int memorycallback(ZMapXMLWriter writer, char *xml, int xml_length, void *user_data);
void flushToOutput(ZMapXMLWriter writer);


ZMapXMLWriter zMapXMLWriterCreate(ZMapXMLWriterOutputCallback flush_callback, 
                                  gpointer flush_data)
{
  ZMapXMLWriter writer = NULL;

  writer = g_new0(ZMapXMLWriterStruct, 1);
  writer->initialised = FALSE;

  writer->xml_output = g_string_sized_new(500);

  writer->element_stack = g_array_sized_new(FALSE, FALSE, sizeof(ElementStackMemberStruct), 512);

  writer->output_callback = flush_callback;
  writer->output_userdata = flush_data;

  return writer;
}

ZMapXMLWriterErrorCode zMapXMLWriterStartElement(ZMapXMLWriter writer, char *element_name)
{
  ZMapXMLWriterErrorCode code = ZMAPXMLWRITER_OK;
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
  my_push_onto_stack(writer, &curr_element);

  my_flush(writer);
  code = writer->errorCode;
  return code;
}

ZMapXMLWriterErrorCode zMapXMLWriterAttribute(ZMapXMLWriter writer, char *name, char *value)
{
  ZMapXMLWriterErrorCode code = ZMAPXMLWRITER_OK;
  ElementStackMember stack_element = NULL;
  GQuark name_q = 0;

  name_q = g_quark_from_string(name);

  if(!(stack_element = my_peek_stack(writer)))
    code = ZMAPXMLWRITER_BAD_POSITION;

  if(code == ZMAPXMLWRITER_OK &&
     my_attribute_is_unique(stack_element, name_q))
    {
      g_string_append_printf(writer->xml_output, " %s=\"%s\"", name, value);
      my_attribute_add(stack_element, name_q);

      my_flush(writer);
      code = writer->errorCode;
    }
  else
    code = ZMAPXMLWRITER_DUPLICATE_ATTRIBUTE;

  return code;
}

ZMapXMLWriterErrorCode zMapXMLWriterElementContent(ZMapXMLWriter writer, char *content)
{
  ZMapXMLWriterErrorCode code = ZMAPXMLWRITER_OK;

  if(!(writer->stack_top_has_content))
    {
      writer->stack_top_has_content = TRUE;
      g_string_append_c(writer->xml_output, '>');
    }

  g_string_append(writer->xml_output, content);

  my_flush(writer);
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

  if((stack_head = my_pop_off_stack(writer, name_quark)))
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
  

  my_flush(writer);
  code = writer->errorCode;
  return code;
}

ZMapXMLWriterErrorCode zMapXMLWriterStartDocument(ZMapXMLWriter writer, char *document_root_tag)
{
  ZMapXMLWriterErrorCode code = ZMAPXMLWRITER_OK;
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

  my_flush(writer);
  code = writer->errorCode;
  return code;
}

ZMapXMLWriterErrorCode zMapXMLWriterEndDocument(ZMapXMLWriter writer)
{
  ZMapXMLWriterErrorCode code = ZMAPXMLWRITER_OK;
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
          first  = (char *)g_quark_to_string(event->data.simple);
          status = zMapXMLWriterStartElement(writer, first);
          break;
        case ZMAPXML_END_ELEMENT_EVENT:
          first  = (char *)g_quark_to_string(event->data.simple);
          status = zMapXMLWriterEndElement(writer, first);
          break;
        case ZMAPXML_CHAR_DATA_EVENT:
          first  = (char *)g_quark_to_string(event->data.simple);
          status = zMapXMLWriterElementContent(writer, first);
          break;
        case ZMAPXML_START_DOC_EVENT:
          first  = (char *)g_quark_to_string(event->data.simple);
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

  g_free(writer);

  return code;
}



/* internal code */
void my_flush(ZMapXMLWriter writer)
{
  writer->flush_counter++;

  if(!(writer->flush_counter % ZMAPXMLWRITER_FLUSH_COUNT))
    flushToOutput(writer);

  return ;
}

void my_push_onto_stack(ZMapXMLWriter writer, ElementStackMember element)
{
  g_array_append_val(writer->element_stack, *element);
  return ;
}

ElementStackMember my_peek_stack(ZMapXMLWriter writer)
{
  ElementStackMember peeked = NULL;
  int last = 0;

  last = writer->element_stack->len - 1;

  peeked = &(g_array_index(writer->element_stack, ElementStackMemberStruct, last));

  return peeked;
}

ElementStackMember my_pop_off_stack(ZMapXMLWriter writer, GQuark element_name)
{
  ElementStackMember popped = NULL;
  int last = 0;

  last = writer->element_stack->len - 1;

  popped = &(g_array_index(writer->element_stack, ElementStackMemberStruct, last));

  if(popped->name == element_name)
    g_array_remove_index(writer->element_stack, last);
  else
    {
      popped = NULL;
      writer->errorCode = ZMAPXMLWRITER_MISMATCHED_TAG;
    }

  return popped;
}

gboolean my_attribute_is_unique(ElementStackMember element, GQuark attribute_name)
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

void my_attribute_add(ElementStackMember element, GQuark attribute_name)
{
  if(!(element->attribute_list))
    element->attribute_list = g_array_sized_new(FALSE, FALSE, sizeof(GQuark), 20);

  g_array_append_val(element->attribute_list, attribute_name);

  return ;
}

int memorycallback(ZMapXMLWriter writer, char *xml, int xml_length, void *user_data)
{
  int length = xml_length;

  

  return length;
}

void flushToOutput(ZMapXMLWriter writer)
{
  ZMapXMLWriterErrorCode code = ZMAPXMLWRITER_OK;
  int length2flush = 0;
  code = writer->errorCode;

  if(code == ZMAPXMLWRITER_OK && (length2flush = writer->xml_output->len))
    {                           /* Carry on with flushing */
      int length_flushed = 0;
      writer->errorCode  = ZMAPXMLWRITER_INCOMPLETE_FLUSH;
  
      length_flushed = (writer->output_callback)(writer, 
                                                 writer->xml_output->str, 
                                                 length2flush,
                                                 writer->output_userdata);

      if(length_flushed == length2flush)
        {
          g_string_truncate(writer->xml_output, 0);
          writer->errorCode = ZMAPXMLWRITER_OK;
        }
      else
        writer->errorCode = ZMAPXMLWRITER_FAILED_FLUSHING;
    }

  return ;
}

