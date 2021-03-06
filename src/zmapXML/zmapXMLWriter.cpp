/*  File: zmapXMLWriter.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <zmapXML_P.hpp>
#include <ZMap/zmapUtils.hpp>



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




/* On creation a callback function and data can be specified for custom
 * output of the final xml text.
 * 
 * If compress_empty_elements == TRUE then elements like this:
 * 
 * <request action="xxx">
 * </request>
 * 
 * are compressed into this:
 * 
 * <request action="xxx"/>
 * 
 * With current code, allowing compression will make it impossible to insert elements that should be nested
 * inside the compressed element.
 *  */
ZMapXMLWriter zMapXMLWriterCreate(ZMapXMLWriterOutputCallback flush_callback, gpointer flush_data)
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

ZMapXMLWriterErrorCode zMapXMLWriterStartElement(ZMapXMLWriter writer, const char *element_name)
{
  ZMapXMLWriterErrorCode code;
  ElementStackMemberStruct curr_element = {0};
  int i, depth ;

  depth = writer->element_stack->len ;

  if(!(writer->stack_top_has_content) && depth > 0)
    {
      g_string_append_c(writer->xml_output, '>');
      g_string_append_c(writer->xml_output, '\n');
    }
  else
    {
      writer->stack_top_has_content = FALSE;
    }


  for (i = 0 ; i < (depth * 2) ; i++)
    {
      g_string_append_c(writer->xml_output, ' ') ;
    }

  g_string_append_c(writer->xml_output, '<');

  g_string_append(writer->xml_output, element_name);
  
  curr_element.name = g_quark_from_string(element_name);
  //curr_element.has_content = FALSE; //not needed

  pushElement(writer, &curr_element);

  maybeFlush(writer);
  code = writer->errorCode;

  return code;
}

ZMapXMLWriterErrorCode zMapXMLWriterAttribute(ZMapXMLWriter writer, const char *name, const char *value)
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

ZMapXMLWriterErrorCode zMapXMLWriterElementContent(ZMapXMLWriter writer, const char *content)
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

ZMapXMLWriterErrorCode zMapXMLWriterEndElement(ZMapXMLWriter writer, const char *element, gboolean full_format)
{
  ZMapXMLWriterErrorCode code = ZMAPXMLWRITER_OK;
  GQuark name_quark = 0;
  ElementStackMember stack_head = NULL;
  int depth = 0, i = 0;

  name_quark = g_quark_from_string(element);

  if ((stack_head = popElement(writer, name_quark)))
    {
      if (!(writer->stack_top_has_content) && !full_format)
        {
          /* g_string_append_c(writer->xml_output, ' '); */
          g_string_append_c(writer->xml_output, '/');
          g_string_append_c(writer->xml_output, '>');
          writer->stack_top_has_content = TRUE;
        }
      else
        {
  if (!(writer->stack_top_has_content) && full_format)
    {
      g_string_append(writer->xml_output, ">\n");
      writer->stack_top_has_content = TRUE;
    }

          depth = (writer->element_stack->len) * 2;
          for(i=0;i<depth;i++)
            { g_string_append_c(writer->xml_output, ' '); }
          g_string_append_printf(writer->xml_output, "</%s>", element);
        }

#ifdef RDS_DONT_INCLUDE_UNUSED
      /*! \todo #warning FIX MEM LEAK */
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

ZMapXMLWriterErrorCode zMapXMLWriterStartDocument(ZMapXMLWriter writer, const char *document_root_tag)
{
  ZMapXMLWriterErrorCode code;
  const char *version = "1.1", *encoding = "UTF-8";
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

  zMapXMLWriterEndElement(writer, doc_tag, FALSE);

  flushToOutput(writer);

  code = writer->errorCode;

  return code;
}


/* Event processing code.  FIFO logic, processed in exactly the same order as created...
 * 
 * If full_format TRUE then both start and end elements are output even if there is no
 * content. This is useful when the result is to be passed to other code.
 * 
 *  */
ZMapXMLWriterErrorCode zMapXMLWriterProcessEvents(ZMapXMLWriter writer, GArray *events, gboolean full_format)
{
  int event_count = 0, i = 0 ;
  ZMapXMLWriterEvent event = NULL ;
  ZMapXMLWriterErrorCode status = ZMAPXMLWRITER_OK ;
  static gboolean debug = FALSE ;


  for (i = 0, event_count = events->len ; ((status == ZMAPXMLWRITER_OK) && (i < event_count)) ; i++)
    {
      const char *first = NULL, *second = NULL;
      event = &(g_array_index(events, ZMapXMLWriterEventStruct, i));

      zMapDebugPrint(debug, "element: %s, attribute: %s",
     g_quark_to_string(event->data.name), g_quark_to_string(event->data.comp.name)) ;

      switch(event->type)
        {
        case ZMAPXML_START_ELEMENT_EVENT:
          first  = (char *)g_quark_to_string(event->data.name);
          status = zMapXMLWriterStartElement(writer, first);
          break;
        case ZMAPXML_END_ELEMENT_EVENT:
          first  = (char *)g_quark_to_string(event->data.name);
          status = zMapXMLWriterEndElement(writer, first, full_format);
          break;
        case ZMAPXML_CHAR_DATA_EVENT:

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          first  = (char *)g_quark_to_string(event->data.name);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

          first  = event->data.comp.value.s ;
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
            else if (event->data.comp.data == ZMAPXML_EVENT_DATA_INTEGER)
              {
                second = g_strdup_printf("%d", event->data.comp.value.integer);
              }
            else if (event->data.comp.data == ZMAPXML_EVENT_DATA_DOUBLE)
              {
                second = g_strdup_printf("%f", event->data.comp.value.flt);
              }
            else if (event->data.comp.data == ZMAPXML_EVENT_DATA_STRING)
              {
                second = (char *)g_quark_to_string(event->data.comp.value.quark) ;
                free_second = FALSE;
              }
            else
              {
                zMapWarnIfReached();
              }

            if (second)
              status = zMapXMLWriterAttribute(writer, first, second);

            if(free_second && second)
              g_free((void *)second);
          }
          break;
        case ZMAPXML_NULL_EVENT:
          break;
        case ZMAPXML_UNSUPPORTED_EVENT:
        case ZMAPXML_UNKNOWN_EVENT:
        default:
          zMapWarnIfReached();
          break;
        }
    }

  if (status == ZMAPXMLWRITER_OK)
    {
      flushToOutput(writer);
      status = writer->errorCode;
    }

  return status;
}


/* The string returned belongs to zMapXMLWriter so should not be free'd. */
char *zMapXMLWriterGetXMLStr(ZMapXMLWriter writer)
{
  char *xml_str = NULL ;

  if (writer->xml_output->len)
    xml_str = writer->xml_output->str ;

  return xml_str ;
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



ZMapXMLWriterErrorCode zMapXMLWriterDestroy(ZMapXMLWriter writer)
{
  ZMapXMLWriterErrorCode code = ZMAPXMLWRITER_OK;

  writer->output_callback = NULL;
  writer->output_userdata = NULL;

  g_string_free(writer->xml_output, TRUE);

  if(writer->error_msg)
    g_free(writer->error_msg);

  g_free(writer);

  return code;
}






/*
 *                    internal routines
 */


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
  if (writer->output_callback)
    {
      ZMapXMLWriterErrorCode code ;
      int length2flush = 0 ;

      code = writer->errorCode ;

      if (code == ZMAPXMLWRITER_OK && (length2flush = writer->xml_output->len))
        {
          /* Carry on with flushing */
          int length_flushed = 0 ;
        
          setErrorCode(writer, ZMAPXMLWRITER_INCOMPLETE_FLUSH) ;
          
          length_flushed = (writer->output_callback)(writer, 
             writer->xml_output->str, 
             length2flush,
             writer->output_userdata) ;

          if (length_flushed == length2flush)
            {
              g_string_truncate(writer->xml_output, 0) ;
              setErrorCode(writer, ZMAPXMLWRITER_OK) ;
            }
          else
            {
              setErrorCode(writer, ZMAPXMLWRITER_FAILED_FLUSHING) ;
            }
        }
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
