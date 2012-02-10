/*  Last edited: Oct 28 14:41 2011 (edgrif) */
/*  File: zmapXMLUtils.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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

#include <string.h>

#include <ZMap/zmapUtils.h>
#include <zmapXML_P.h>


static void transfer(ZMapXMLUtilsEventStack source, ZMapXMLWriterEvent dest) ;



/* 
 *                       External Interface.
 */


/* GHASTLY....SURELY THE THING THAT PERSISTS SHOULD BE THE FIRST PARAM IN ALL THE
 * CALLS SINCE IT IS THE INVARIANT IN ALL THE CALLS.... */

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


/* Add the event stack to the middle of the events array. Use this to allow
 * insertion of new xml levels into a pre-existing nest of xml.
 * 
 *  */
GArray *zMapXMLUtilsAddStackToEventsArrayAfterElement(GArray *events_array, char *element_name,
						      ZMapXMLUtilsEventStackStruct *event_stack)
{
  GArray *result = NULL ;

  ZMapXMLUtilsEventStack input ;

  ZMapXMLWriterEventStruct single = {0} ;
  
  int insert_pos ;
  int i ;
  gboolean found_element ;



  /* Find first position in the array after the named element _and_ that elements attributes.  */
  for (i = 0, insert_pos = 0, found_element = FALSE ; i < events_array->len ; i++)
    {
      ZMapXMLWriterEvent event ;

      event = &(g_array_index(events_array, ZMapXMLWriterEventStruct, i)) ;

      if (event->type == ZMAPXML_START_ELEMENT_EVENT
	  && g_ascii_strcasecmp(g_quark_to_string(event->data.name), element_name) == 0)
	{
	  found_element = TRUE ;
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

  return result ;
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
	    zMapAssertNotReached();
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



