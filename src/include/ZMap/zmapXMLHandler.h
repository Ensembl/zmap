/*  File: zmapXMLHandler.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Header file for code that needs to get xremote/xml
 *              requests executed and optionally parse the results.
 *
 * HISTORY:
 * Last edited: Jun 28 11:50 2007 (rds)
 * Created: Wed Jun 13 08:45:06 2007 (edgrif)
 * CVS info:   $Id: zmapXMLHandler.h,v 1.3 2010-03-04 15:15:33 mh17 Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_XML_HANDLER_H
#define ZMAP_XML_HANDLER_H

#include <ZMap/zmapXML.h>


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

