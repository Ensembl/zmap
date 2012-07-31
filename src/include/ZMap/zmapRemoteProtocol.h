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



/* 
 *    Data type descriptors for the GtkSelectionData used for passing requests/replies.
 */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Wanted to use our own but lookds like Perl/Tk won't support it.... */
#define ZACP_DATA_TYPE   "ZACP_COMMAND_STR"
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#define ZACP_DATA_TYPE   "STRING"
#define ZACP_DATA_FORMAT 8			    /* Bits per unit. */



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

#define ZACP_ZOOM_TO       "zoom_to"
#define ZACP_GET_MARK      "get_mark"

#define ZACP_GET_FEATURE_NAMES  "get_feature_names"
#define ZACP_LOAD_FEATURES      "load_features"
#define ZACP_FEATURES_LOADED    "features_loaded"
#define ZACP_DUMP_FEATURES      "export_context"


#define ZACP_CREATE_FEATURE       "create_feature"
#define ZACP_REPLACE_FEATURE      "replace_feature"
#define ZACP_SELECT_FEATURE       "single_select"
#define ZACP_SELECT_MULTI_FEATURE "multiple_select"
#define ZACP_DETAILS_FEATURE      "feature_details"
#define ZACP_EDIT_FEATURE         "edit"
#define ZACP_FIND_FEATURE         "find_feature"
#define ZACP_DELETE_FEATURE       "delete_feature"


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

#define ZACP_MARK   "mark"
#define ZACP_START  "start"
#define ZACP_END    "end"

#define ZACP_ALIGN      "align"
#define ZACP_BLOCK      "block"
#define ZACP_FEATURESET "featureset"
#define ZACP_FEATURE    "feature"


#define ZACP_SCORE           "score"
#define ZACP_START_NOT_FOUND "start_not_found"
#define ZACP_END_NOT_FOUND   "end_not_found"
#define ZACP_LOCUS           "locus"
#define ZACP_


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
