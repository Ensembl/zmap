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
#include <zmapXML_P.h>
#include <ZMap/zmapUtils.h>

static void transfer(ZMapXMLUtilsEventStack source, ZMapXMLWriterEvent dest);


GArray *zMapXMLUtilsAddStackToEventsArray(ZMapXMLUtilsEventStackStruct *event_stack,
                                          GArray *events_array)
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

GArray *zMapXMLUtilsAddStackToEventsArrayStart(ZMapXMLUtilsEventStackStruct *event_stack,
                                               GArray *events_array)
{
  ZMapXMLUtilsEventStack input;
  ZMapXMLWriterEventStruct single = {0};
  gint size = 0;

  input = event_stack;

  while(input && input->event_type){ input++; size++; }

  for(input = &event_stack[--size]; size >= 0; size--, input--)
    {
      transfer(input, &single);
      events_array = g_array_prepend_val(events_array, single);
    }

  return events_array;
}

GArray *zMapXMLUtilsStackToEventsArray(ZMapXMLUtilsEventStackStruct *event_stack)
{
  GArray *events;

  events = zMapXMLUtilsCreateEventsArray();

  events = zMapXMLUtilsAddStackToEventsArray(event_stack, events);

  return events;
}

GArray *zMapXMLUtilsCreateEventsArray(void)
{
  GArray *events;

  events = g_array_sized_new(TRUE, TRUE, sizeof(ZMapXMLWriterEventStruct), 512);

  return events;
}







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
      dest->data.comp.name = g_quark_from_string(source->name);
      dest->data.comp.data = data_type = source->data_type;
      if(data_type == ZMAPXML_EVENT_DATA_QUARK)
        dest->data.comp.value.quark = g_quark_from_string(source->value.s);
      else if(data_type == ZMAPXML_EVENT_DATA_INTEGER)
        dest->data.comp.value.integer = source->value.i;
      else if(data_type == ZMAPXML_EVENT_DATA_DOUBLE)
        dest->data.comp.value.flt = source->value.d;
      else
        zMapAssertNotReached();
      break;
    default:
      break;
    }
  return ;
}



/* copy a string and replace &thing; */
/* sadly needed to unescape &apos;, no other characters are handled */
/* quickly hacked for a bug fix, will review when the new XRemote is introduced */
char *zMapXMLUtilsUnescapeStrdup(char *str)
{
	char *result = g_strdup(str);
	char *p = result;

	while (*p)
	{
		if(!g_ascii_strncasecmp(p,"&apos;",6))
		{
			*p++ = '\'';
			strcpy(p,p+5);
		}
		else
		{
			p++;
		}
	}
	return(result);
}

