/*  Last edited: Dec 16 09:34 2011 (edgrif) */
/*  File: zmapRemoteCommand.h
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
 * Description: Defines interface for processing remote control 
 *              commands.
 *              These functions handle the sending/receiving of messages
 *              and check that requests/responses have matching ids,
 *              are the right protocol version.
 *
 * Created: Thu Dec 15 17:39:32 2011 (edgrif)
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_REMOTE_COMMAND_H
#define ZMAP_REMOTE_COMMAND_H

#include <glib.h>

#include <ZMap/zmapEnum.h>


/* Using the XXX_LIST macros to declare enums means we get lots of enum functions for free (see zmapEnum.h). */


/* Return code values from command processing function. */
#define REMOTE_COMMAND_RC_LIST(_)                                            \
_(REMOTE_COMMAND_RC_OK,        , "ok"       , "Command succeeded."    , "")  \
_(REMOTE_COMMAND_RC_UNKNOWN,   , "unknown"  , "Command not known."    , "")  \
_(REMOTE_COMMAND_RC_OTHER,     , "other"    , "Command known but is not for this resource."    , "")  \
_(REMOTE_COMMAND_RC_BAD_XML ,  , "bad_xml"  , "Command XML is malformed."  , "")  \
_(REMOTE_COMMAND_RC_BAD_ARGS,  , "bad_args" , "Command args are wrong."  , "")  \
_(REMOTE_COMMAND_RC_FAILED,    , "failed"   , "Command failed."       , "")

ZMAP_DEFINE_ENUM(RemoteCommandRCType, REMOTE_COMMAND_RC_LIST) ;



/* Command processing function typedef. */
/* THERE WILL BE OTHER ARGS FOR SURE !! */
typedef RemoteCommandRCType (*ZMapRemoteControlCommandFunc)(gpointer user_data, char *command, char **reply_out) ;

/* I think this is the place to add commands to write stuff..... */
GArray *zMapRemoteControlCreateRequest(char *peer_id, char* request_id, int timeout_secs, char *command) ;
GArray *zMapRemoteControlCreateReply(char *peer_id, char* request_id) ;
GArray *zMapRemoteControlCreateReplyFromRequest(char *xml_request,
						RemoteCommandRCType return_code, char *reason, char *result,
						char **error_out) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
GArray *zMapRemoteControlReplyAddResult(GArray *reply_envelope,
					RemoteCommandRCType return_code, char *reason,
					char *result) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



gboolean zMapRemoteControlValidateRequest(char *peer_id, char *xml_request, char **error_out) ;
gboolean zMapRemoteControlRequestIsCommand(char *request, char *command) ;
char *zMapRemoteControlRequestGetCommand(char *request) ;

ZMAP_ENUM_AS_NAME_STRING_DEC(zMapRemoteControlRC, RemoteCommandRCType) ;


#endif /* ZMAP_REMOTE_COMMAND_H */
