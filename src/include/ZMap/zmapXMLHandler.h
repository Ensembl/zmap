/*  File: zmapXMLHandler.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * Last edited: Jun 15 13:32 2007 (edgrif)
 * Created: Wed Jun 13 08:45:06 2007 (edgrif)
 * CVS info:   $Id: zmapXMLHandler.h,v 1.1 2007-06-15 12:34:12 edgrif Exp $
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


#endif /* ZMAP_XML_HANDLER_H */

