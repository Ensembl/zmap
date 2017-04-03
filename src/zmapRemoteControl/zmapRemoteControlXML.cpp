/*  File: zmapRemoteControlXML.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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


