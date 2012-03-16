/*  File: zmapRemoteControl.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2010: Genome Research Ltd.
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
 *      Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: External interface to remote control package.
 *
 * HISTORY:
 * Last edited: Feb  1 19:01 2012 (edgrif)
 * Created: Fri Sep 24 14:51:35 2010 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_REMOTE_PROTOCOL_H
#define ZMAP_REMOTE_PROTOCOL_H


/* 
 *     ZMap Annotation Command Protocol (ZACP)
 * 
 * Note that _all_ element and attribute keywords must be in lowercase
 * because XML is case sensitive.
 * 
 * 
 */


/* Protocol "types", describe what type of message it is.
 * 
 * This is version 2 of the protocol because there will be some incompatible changes
 * from version 1.
 *  */
#define ZACP_TAG "zmap"
#define ZACP_VERSION  "2.0"



/* Request and Reply envelope elements and attributes. */
#define ZACP_REQUEST  "request"
#define ZACP_REPLY    "reply"

#define ZACP_TYPE          "type"
#define ZACP_VERSION_ID    "version"
#define ZACP_APP_ID        "app_id"
#define ZACP_CLIPBOARD_ID  "clipboard_id"
#define ZACP_REQUEST_ID    "request_id"



/* Commands, attributes and values. */
#define ZACP_CMD "command"

#define ZACP_HANDSHAKE     "handshake"
#define ZACP_PING          "ping"
#define ZACP_NEWVIEW       "new_view"
#define ZACP_ADD_TO_VIEW   "add_to_view"
#define ZACP_CLOSEVIEW     "close_view"
#define ZACP_VIEW_CREATED  "view_created"
#define ZACP_VIEW_DELETED  "view_deleted"
#define ZACP_GOODBYE       "goodbye"
#define ZACP_SHUTDOWN      "shutdown"


#define ZACP_VIEWID  "view_id"
#define ZACP_TIMEOUT "timeout"


#define ZACP_SEQUENCE_TAG   "sequence"
#define ZACP_SEQUENCE_NAME  "name"
#define ZACP_SEQUENCE_START "start"
#define ZACP_SEQUENCE_END   "end"


#define ZACP_GOODBYE_TYPE "type"
#define ZACP_CLOSE        "close"
#define ZACP_EXIT         "exit"


#define ZACP_SHUTDOWN_TAG  "shutdown"
#define ZACP_SHUTDOWN_TYPE "type"
#define ZACP_CLEAN         "clean"
#define ZACP_ABORT         "abort"



/* Command results, elements, attributes and values. */

#define ZACP_RETURN_CODE "return_code"
#define ZACP_REASON      "reason"

#define ZACP_MESSAGE "message"

#define ZACP_VIEW "view"




/* for the close view we need a "force" option when the close is done from xremote
   because we don't want the user having to intervene....currently we ask the user if
   they really want to close the view. */






/* uM....NOT REALLY USEFUL AS XML WRITER DOES ALL THIS........ */

/* Protocol envelope format:
 * 
 *   <zmap version="v.r" type=["request" | "reply"] peer_id="xxxxx" request_id="nnnnn" [timeout="secs"]>
 * 
 */
#define ZACP_ENVELOPE_FORMAT "<" ZACP_TAG " version=\"" ZACP_VERSION "\" type=\"%s\" peer_id=\"%s\" request_id=\"%s\">"



#endif /* ZMAP_REMOTE_PROTOCOL_H */
