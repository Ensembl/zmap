/*  File: zmapXMLUtils.c
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Functions to create xml from a stack of xml "events"
 *
 * Exported functions: See ZMap/zmapXML.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapRemoteCommand.h>
#include <zmapXML_P.h>


static void transfer(ZMapXMLUtilsEventStack source, ZMapXMLWriterEvent dest) ;
static gboolean hasAttribute(GArray *events_array, int element_index, char *attribute_name, char *attribute_value) ;



/* 
 *                       Globals.
 */

static gboolean debug_G = FALSE ;





/* 
 *                       External Interface.
 */



/* Create an empty events array. */
GArray *zMapXMLUtilsCreateEventsArray(void)
{
  GArray *events;

  events = g_array_sized_new(TRUE, TRUE, sizeof(ZMapXMLWriterEventStruct), 512);

  return events;
}


/* Take an events stack and turn it into an events array. */
GArray *zMapXMLUtilsStackToEventsArray(ZMapXMLUtilsEventStackStruct *event_stack)
{
  GArray *events;

  events = zMapXMLUtilsCreateEventsArray();

  events = zMapXMLUtilsAddStackToEventsArrayEnd(events, event_stack);

  return events;
}



/* Add an events stack to the beginning of an events array. */
GArray *zMapXMLUtilsAddStackToEventsArrayStart(GArray *events_array, ZMapXMLUtilsEventStackStruct *event_stack)
{
  ZMapXMLUtilsEventStack input;
  ZMapXMLWriterEventStruct single = {0};
  gint size = 0;

  input = event_stack;

  while (input && input->event_type)
    {
      input++;
      size++;
    }

  for (input = &event_stack[--size] ; size >= 0 ; size--, input--)
    {
      transfer(input, &single) ;
      events_array = g_array_prepend_val(events_array, single) ;
    }

  return events_array;
}


/* Add the event stack to the end of the events array. */
GArray *zMapXMLUtilsAddStackToEventsArrayEnd(GArray *events_array, ZMapXMLUtilsEventStackStruct *event_stack)
{
  ZMapXMLUtilsEventStack input;
  ZMapXMLWriterEventStruct single = {0};

  input = event_stack;

  while(input && input->event_type)
    {
      transfer(input, &single);
      events_array = g_array_append_val(events_array, single);
      input++;
    }

  return events_array;
}



/* Add the event stack as a child of the element given by element_name. Use this to allow
 * insertion of new xml levels into a pre-existing nest of xml, e.g.
 * 
 * <Some_element>
 *     <<<<<<<--------------------- insert xml in here
 * </Some_element>
 * 
 * 
 * Optionally specify an attribute name and optional value that qualifies the element to
 * add the event_stack to and also an index if there is more than one element with this
 * attribute name/value. If there are multiple elements with the same name/attribute
 * and you want to add to the 2nd one then specify an attribute_index of 2 and so on.
 * If no attribute name/value/index given then the stack is inserted in the first element
 * with element_name.
 * 
 * 
 *  */
GArray *zMapXMLUtilsAddStackToEventsArrayToElement(GArray *events_array,
						   char *element_name, int element_index,
						   char *attribute_name, char *attribute_value,
						   ZMapXMLUtilsEventStackStruct *event_stack)
{
  GArray *result = NULL ;
  ZMapXMLUtilsEventStack input ;
  ZMapXMLWriterEventStruct single = {0} ;
  int insert_pos ;
  int i ;
  gboolean found_element ;
  int attr_index = 1 ;

  /* Normalise the particular incidence of an element we are looking for. */
  if (element_index < 1)
    element_index = 1 ;

  /* Find first position in the array after the named element _and_ that elements attributes.  */
  for (i = 0, insert_pos = 0, found_element = FALSE ; i < events_array->len ; i++)
    {
      ZMapXMLWriterEvent event ;

      event = &(g_array_index(events_array, ZMapXMLWriterEventStruct, i)) ;

      zMapDebugPrint(debug_G, "%s\n", zMapXMLWriterEvent2Txt(event)) ;

      if (event->type == ZMAPXML_START_ELEMENT_EVENT
	  && g_ascii_strcasecmp(g_quark_to_string(event->data.name), element_name) == 0)
	{
	  int index = i ;				    /* Pass in start of element. */

	  if (!attribute_name
	      || hasAttribute(events_array, index, attribute_name, attribute_value))
	    {
	      if (attr_index == element_index)
		found_element = TRUE ;
	      else
		attr_index++ ;
	    }
	}
      else if (found_element && (event->type == ZMAPXML_END_ELEMENT_EVENT
				 && g_ascii_strcasecmp(g_quark_to_string(event->data.name), element_name) == 0))
	{
	  insert_pos = i ;
	  break ;
	}
    }

  if (found_element)
    {
      result = events_array ;
      input = event_stack ;

      while (input && input->event_type)
	{
	  transfer(input, &single) ;

	  result = g_array_insert_val(result, insert_pos, single) ;

	  insert_pos++ ;

	  input++ ;
	}
    }


  if (debug_G)
    {
      char *request ;
      char *err_msg = NULL ;

      request = zMapXMLUtilsStack2XML(result, &err_msg, FALSE) ;

      zMapDebugPrint(debug_G, "%s\n", request) ;
    }


  return result ;
}



/* Add the event stack after the xml element given by element_name. Typically used
 * to add multiple copies of the same element, e.g.
 * 
 * <Some_element>
 * </Some_element>
 * <<<<<<<--------------------- insert xml in here 
 * 
 * 
 * Optionally specify an attribute name and optionally value that qualifies the element to
 * add the event_stack to and also an index if there is more than one element with this
 * attribute name/value. If there are multiple elements with the same name/attribute
 * and you want to add to the 2nd one then specify an attribute_index of 2 and so on.
 * If no index or attribute name/value given then the stack is inserted after the
 * first element with element_name.
 * 
 *  */
GArray *zMapXMLUtilsAddStackToEventsArrayAfterElement(GArray *events_array,
						      char *element_name, int element_index,
						      char *attribute_name, char *attribute_value,
						      ZMapXMLUtilsEventStackStruct *event_stack)
{
  GArray *result = NULL ;
  ZMapXMLUtilsEventStack input ;
  ZMapXMLWriterEventStruct single = {0} ;
  int insert_pos ;
  int i ;
  gboolean found_element ;
  int attr_index = 1 ;

  /* Normalise the particular incidence of an element we are looking for. */
  if (attribute_name && element_index < 1)
    element_index = 1 ;

  /* Find position of end of named element in the array.  */
  for (i = 0, insert_pos = 0, found_element = FALSE ; i < events_array->len ; i++)
    {
      ZMapXMLWriterEvent event ;

      event = &(g_array_index(events_array, ZMapXMLWriterEventStruct, i)) ;

      zMapDebugPrint(debug_G, "%s\n", zMapXMLWriterEvent2Txt(event)) ;

      if (event->type == ZMAPXML_START_ELEMENT_EVENT
	  && g_ascii_strcasecmp(g_quark_to_string(event->data.name), element_name) == 0)
	{
	  int index = i ;				    /* Pass in start of element. */

	  if (!attribute_name
	      || hasAttribute(events_array, index, attribute_name, attribute_value))
	    {
	      if (attr_index == element_index)
		found_element = TRUE ;
	      else
		attr_index++ ;
	    }


	}
      else if (found_element && (event->type == ZMAPXML_END_ELEMENT_EVENT
				 && g_ascii_strcasecmp(g_quark_to_string(event->data.name), element_name) == 0))
	{
	  insert_pos = i + 1 ;				    /* bump past end element. */
	  break ;
	}
    }

  if (found_element)
    {
      result = events_array ;
      input = event_stack ;

      while (input && input->event_type)
	{
	  transfer(input, &single) ;

	  result = g_array_insert_val(result, insert_pos, single) ;

	  insert_pos++ ;

	  input++ ;
	}
    }


  if (debug_G)
    {
      char *request ;
      char *err_msg = NULL ;

      request = zMapXMLUtilsStack2XML(result, &err_msg, FALSE) ;

      zMapDebugPrint(debug_G, "%s\n", request) ;
    }


  return result ;
}


/* Take a stack of xml parts and convert to the string containing the xml. */
char *zMapXMLUtilsStack2XML(GArray *xml_stack, char **err_msg_out, gboolean full_format)
{
  char *xml_string = NULL ;
  ZMapXMLWriter writer ;

  if ((writer = zMapXMLWriterCreate(NULL, NULL)))
    {
      ZMapXMLWriterErrorCode xml_status ;

      if ((xml_status = zMapXMLWriterProcessEvents(writer, xml_stack, full_format)) != ZMAPXMLWRITER_OK)
        *err_msg_out = g_strdup(zMapXMLWriterErrorMsg(writer)) ;
      else
	xml_string = g_strdup(zMapXMLWriterGetXMLStr(writer)) ;

      zMapXMLWriterDestroy(writer) ;
    }

  return xml_string ;
}



/* Returns event as a string, note if you want to keep the returned string
 * you have to copy it between calls to this function as it uses a static buffer. */
char *zMapXMLUtilsEvent2Txt(ZMapXMLUtilsEventStack event)
{
  char *event_text = NULL ;
  static GString *event_str = NULL ;

  if (!event_str)
    event_str = g_string_sized_new(1024) ;
  else
    g_string_set_size(event_str, 0) ;

  g_string_append_printf(event_str, "%s\t", event->name) ;

  switch (event->data_type)
    {
    case ZMAPXML_EVENT_DATA_QUARK:
      g_string_append(event_str, g_quark_to_string(event->value.q)) ;
      break ;
    case ZMAPXML_EVENT_DATA_STRING:
      g_string_append(event_str, event->value.s) ;
      break ;
    case ZMAPXML_EVENT_DATA_INTEGER:
      g_string_append_printf(event_str, "%d", event->value.i) ;
      break ;
    case ZMAPXML_EVENT_DATA_DOUBLE:
      g_string_append_printf(event_str, "%g", event->value.d) ;
      break ;
    default:
      g_string_append(event_str, "No Value") ;
      break ;
    }

  event_text = event_str->str ;

  return event_text ;
}



/* Returns event as a string, note if you want to keep the returned string
 * you have to copy it between calls to this function as it uses a static buffer. */
char *zMapXMLWriterEvent2Txt(ZMapXMLWriterEvent event)
{
  char *event_text = NULL ;
  static GString *event_str = NULL ;

  if (!event_str)
    event_str = g_string_sized_new(1024) ;
  else
    g_string_set_size(event_str, 0) ;

  g_string_append_printf(event_str, "%s\t", g_quark_to_string(event->data.name)) ;

  switch (event->data.comp.data)
    {
      char *str ;

    case ZMAPXML_EVENT_DATA_QUARK:
      g_string_append(event_str,
		      ((str = (char *)g_quark_to_string(event->data.comp.value.quark))
		       ? str : "No Value")) ;
      break ;
    case ZMAPXML_EVENT_DATA_STRING:
      g_string_append(event_str,
		      ((str = event->data.comp.value.s) ? str : "No Value"))  ;
      break ;
    case ZMAPXML_EVENT_DATA_INTEGER:
      g_string_append_printf(event_str, "%d", event->data.comp.value.integer) ;
      break ;
    case ZMAPXML_EVENT_DATA_DOUBLE:
      g_string_append_printf(event_str, "%g", event->data.comp.value.flt) ;
      break ;
    default:
      g_string_append(event_str, "No Value") ;
      break ;
    }

  event_text = event_str->str ;

  return event_text ;
}


/* Functions to escape a string for xml, just cover functions for the glib routines.
 * 
 * Returns NULL if conversion failed, otherwise returned string
 * should be g_free'd when no longer returned.
 * 
 */
char *zMapXMLUtilsEscapeStr(char *str)
{
  char *escaped_str = NULL ;

  escaped_str = g_markup_escape_text(str, -1) ;		    /* -1 signals str is null-terminated. */

  return escaped_str ;
}


char *zMapXMLUtilsEscapeStrPrintf(char *format, ...)
{
  char *escaped_str = NULL ;
  va_list args1, args2 ;

  va_start(args1, format) ;
  G_VA_COPY(args2, args1) ;

  escaped_str = g_markup_printf_escaped(format, args2) ;

  va_end(args1) ;
  va_end(args2) ;

  return escaped_str ;
}





/* I think there may be a glib function to do the string replacement ! */
/* copy a string and replace &thing; */
/* sadly needed to unescape &apos;, no other characters are handled */
/* quickly hacked for a bug fix, will review when the new XRemote is introduced */
char *zMapXMLUtilsUnescapeStrdup(char *str)
{
  char *result = g_strdup(str);
  char *p = result;

  while (*p)
    {
      if (!g_ascii_strncasecmp(p, "&apos;", 6))
	{
	  *p++ = '\'';
	  strcpy(p, p+5);
	}
      else
	{
	  p++;
	}
    }

  return result ;
}






/* 
 *                        Internal routines.
 */

static void transfer(ZMapXMLUtilsEventStack source, ZMapXMLWriterEvent dest)
{
  ZMapXMLWriterEventDataType data_type;

  switch((dest->type = source->event_type))
    {
    case ZMAPXML_START_ELEMENT_EVENT:
    case ZMAPXML_END_ELEMENT_EVENT:
      dest->data.name = g_quark_from_string(source->name);
      break;

    case ZMAPXML_ATTRIBUTE_EVENT:
      {
	dest->data.comp.name = g_quark_from_string(source->name);
	dest->data.comp.data = data_type = source->data_type;

	switch (data_type)
	  {
	  case ZMAPXML_EVENT_DATA_INTEGER:
	    dest->data.comp.value.integer = source->value.i;
	    break ;
	  case ZMAPXML_EVENT_DATA_DOUBLE:
	    dest->data.comp.value.flt = source->value.d ;
	    break ;
	  case ZMAPXML_EVENT_DATA_QUARK:
	    dest->data.comp.value.quark = source->value.q ;
	    break ;
	  case ZMAPXML_EVENT_DATA_STRING:
	    dest->data.comp.value.quark = g_quark_from_string(source->value.s) ;
	    break ;
	  default:
            zMapWarnIfReached();
	    break ;
	  }

	break;
      }

      /* I'M TRYING THIS HERE...SEEMS TO BE MISSING ?? */
    case ZMAPXML_CHAR_DATA_EVENT:
      dest->data.comp.value.s = source->value.s ;
      break ;

    default:
      break;
    }

  return ;
}


/* Can attribute with attribute_name and attribute_value be found in the element pointed to
 * by element index ? returns TRUE if it is, FALSE otherwise.
 * 
 * Note, routine expects index to point to start of the element, _not_ the first attribute.
 *  */
static gboolean hasAttribute(GArray *events_array, int element_index, char *attribute_name, char *attribute_value)
{
  gboolean result = FALSE ;
  ZMapXMLWriterEvent event ;

  event = &(g_array_index(events_array, ZMapXMLWriterEventStruct, element_index)) ;

  if (event->type == ZMAPXML_START_ELEMENT_EVENT)
    {
      int i ;

      for (i = element_index + 1 ; i < events_array->len ; i++)
	{
	  event = &(g_array_index(events_array, ZMapXMLWriterEventStruct, i)) ;

	  zMapDebugPrint(debug_G, "%s\n", zMapXMLWriterEvent2Txt(event)) ;

	  if (event->type == ZMAPXML_ATTRIBUTE_EVENT
	      && g_ascii_strcasecmp(g_quark_to_string(event->data.name), attribute_name) == 0
	      && g_ascii_strcasecmp(g_quark_to_string(event->data.comp.value.quark), attribute_value) == 0)
	    {
	      result = TRUE ;
	      break ;
	    }
	  else if (event->type == ZMAPXML_END_ELEMENT_EVENT)
	    {
	      break ;
	    }
	}
    }

  return result ;
}
