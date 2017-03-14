/*  File: zmapXMLHandler.h
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
 * Description: Header file for code that needs to get xremote/xml
 *              requests executed and optionally parse the results.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_XML_HANDLER_H
#define ZMAP_XML_HANDLER_H

#include <ZMap/zmapXML.hpp>


/* I'm not completely sure we need this, I wanted a small header that client code
 * could use so it didn't need the whole xml header stuff. */


/* For Xremote XML actions/events. */
typedef struct
{
  char *zmap_action ;					    /* What action is required. */
  gboolean handled ;					    /* Was the action successfully handled. */

  GArray *xml_events ;					    /* What xml events should be handled. */

  ZMapXMLObjTagFunctions start_handlers ;		    /* Optional xml event handlers and data for handlers. */
  ZMapXMLObjTagFunctions end_handlers ;
  gpointer handler_data ;
} ZMapXMLHandlerStruct, *ZMapXMLHandler ;


typedef struct
{
  gboolean handled ;
  char *error_message ;
} ZMapXMLTagHandlerStruct, *ZMapXMLTagHandler ;


/* There's kind of a quandry here. On one hand handler data per
   registered handler would be a nice way to go, but generally an xml
   document is only being parsed by one level in the software. Also to
   make things simpler when only having one piece of data you'd like
   to be able to set the data once, not multiple times in the structs
   you pass in...

   Another alternative is to add another pointer to the parser to
   pass into each of the handlers, but what happens if you want to add 
   another pointer? Also it means changing _all_ existing handlers...

   Another is to have a wrapping struct that both parties know about 
   which holds each of the private bits of data.  This doesn't seem 
   much better, but does mean that the handler interface doesn't change
*/

typedef struct
{
  ZMapXMLTagHandler tag_handler;
  gpointer          user_data;
}ZMapXMLTagHandlerWrapperStruct, *ZMapXMLTagHandlerWrapper;

#endif /* ZMAP_XML_HANDLER_H */

