/*  Last edited: Oct 28 12:22 2011 (edgrif) */
/*  File: zmapRemoteControlXML.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2011
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
 *
 * Description: Functions to create valid xml for remote commands.
 *              
 * Exported functions: See XXXXXXXXXXXXX.h
 *              
 *-------------------------------------------------------------------
 */


/* note sure we need this separate file...put it all in command ?? */


/* Make an zmap xml command, constructs:
 * 
 * <zmap version="1.0">
 *   <request action="REQUEST" req_id="NNNN">
 *   </request>
 * </zmap>
 * 
 *  */
char *zMapRemoteControlXMLCreateRequestEnvelope(char *version, char *request, char *req_id)
{
  char *xml_command = NULL ;
  static ZMapXMLUtilsEventStackStruct document_start[] =
    {
      /* event type,                attr/ele name,  data type,         {data} */
      {ZMAPXML_START_ELEMENT_EVENT, "zmap",    ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "version", ZMAPXML_EVENT_DATA_QUARK, {"paint"}},
      {ZMAPXML_START_ELEMENT_EVENT, "request", ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "action",  ZMAPXML_EVENT_DATA_QUARK, {"paint"}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "req_id",  ZMAPXML_EVENT_DATA_QUARK, {"paint"}},
      {0}
    };
  static ZMapXMLUtilsEventStackStruct document_end[] =
    {
      {ZMAPXML_END_ELEMENT_EVENT, "request", ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_END_ELEMENT_EVENT, "zmap",    ZMAPXML_EVENT_DATA_NONE,  {0}},
      {0}
    } ;

  /* need to stick the request name in here.... */
  document_start[1].value.s = g_strdup(request) ;
  
  events = zMapXMLUtilsStackToEventsArray(&document_start[0]) ;

  events = zMapXMLUtilsAddStackToEventsArray(command_stack, command_stack) ;

  events = zMapXMLUtilsAddStackToEventsArray(&document_end[0], command_stack) ;

  return xml_command ;
}


