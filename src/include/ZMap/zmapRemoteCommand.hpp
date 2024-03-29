/*  File: zmapRemoteCommand.h
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

#include <ZMap/zmapEnum.hpp>
#include <ZMap/zmapXML.hpp>
#include <ZMap/zmapRemoteControl.hpp>



/* Using the XXX_LIST macros to declare enums means we get lots of enum functions for free (see zmapEnum.h). */


/* Return code values from command processing function. */
#define REMOTE_COMMAND_RC_LIST(_)                                                           \
_(REMOTE_COMMAND_RC_INVALID,   = -1, "invalid",        "Invalid rc.",                   "") \
_(REMOTE_COMMAND_RC_OK,            , "ok",             "Command succeeded.",            "") \
_(REMOTE_COMMAND_RC_FAILED,        , "cmd_failed",     "Command failed.",               "") \
_(REMOTE_COMMAND_RC_BAD_ARGS,      , "bad_args",       "Command args are wrong.",       "") \
_(REMOTE_COMMAND_RC_VIEW_UNKNOWN,  , "view_unknown",   "Command target view id is unknown.", "") \
_(REMOTE_COMMAND_RC_CMD_UNKNOWN,   , "cmd_unknown",    "Command is unknown.",           "") \
_(REMOTE_COMMAND_RC_BAD_XML,       , "bad_xml",        "Command XML is malformed.",     "")

ZMAP_DEFINE_ENUM(RemoteCommandRCType, REMOTE_COMMAND_RC_LIST) ;


/* Return codes for message validation. */
#define REMOTE_VALIDATE_RC_LIST(_)                                                                     \
_(REMOTE_VALIDATE_RC_OK,               , "ok",               "Validation succeeded.",              "") \
_(REMOTE_VALIDATE_RC_BODY_COMMAND,     , "body_command",     "Error in message body command.",     "") \
_(REMOTE_VALIDATE_RC_BODY_CONTENT,     , "body_content",     "Error in message body content.",     "") \
_(REMOTE_VALIDATE_RC_BODY_XML,         , "body_xml",         "Error in message body xml.",         "") \
_(REMOTE_VALIDATE_RC_ENVELOPE_CONTENT, , "envelope_content", "Error in message envelope content.", "") \
_(REMOTE_VALIDATE_RC_ENVELOPE_XML,     , "envelope_xml",     "Error in message envelope xml.",     "")

ZMAP_DEFINE_ENUM(RemoteValidateRCType, REMOTE_VALIDATE_RC_LIST) ;



/* Types used for asking for certain attributes from the message envelope. */
#define REMOTE_ENVELOPE_ATTR_LIST(_)                                                                     \
_(REMOTE_ENVELOPE_ATTR_INVALID,    , "invalid",    "Invalid attribute.",                "") \
_(REMOTE_ENVELOPE_ATTR_COMMAND,    , "command",    "Command attribute of envelope.",    "") \
_(REMOTE_ENVELOPE_ATTR_REQUEST_ID, , "request_id", "Request id attribute of envelope.", "") \
_(REMOTE_ENVELOPE_ATTR_TIMESTAMP,  , "timestamp",  "Timestamp attribute of envelope.",  "")

ZMAP_DEFINE_ENUM(RemoteEnvelopeAttrType, REMOTE_ENVELOPE_ATTR_LIST) ;



GArray *zMapRemoteCommandCreateRequest(ZMapRemoteControl remote_control,
                                       const char *command, int timeout_secs, ...) ;
GArray *zMapRemoteCommandCreateReplyFromRequest(ZMapRemoteControl remote_control,
                                                char *original_request,
                                                RemoteCommandRCType return_code, const char *reason,
                                                ZMapXMLUtilsEventStack reply,
                                                char **error_out) ;
GArray *zMapRemoteCommandCreateReplyEnvelopeFromRequest(ZMapRemoteControl remote_control,
							char *xml_request,
							RemoteCommandRCType return_code, const char *reason,
							ZMapXMLUtilsEventStack reply,
							char **error_out) ;
ZMapXMLUtilsEventStack zMapRemoteCommandCreateElement(const char *element, const char *attribute, const char *attribute_value) ;
ZMapXMLUtilsEventStack zMapRemoteCommandMessage2Element(const char *message) ;
GArray *zMapRemoteCommandAddBody(GArray *request_in_out, const char *req_or_reply,
				 ZMapXMLUtilsEventStack request_body) ;
char *zMapRemoteCommandStack2XML(GArray *xml_stack, char **error_out) ;

const char *zMapRemoteCommandGetCurrCommand(ZMapRemoteControl remote_control) ;

RemoteValidateRCType zMapRemoteCommandValidateEnvelope(ZMapRemoteControl remote_control,
						       char *xml_request, char **error_out) ;
RemoteValidateRCType zMapRemoteCommandValidateRequest(ZMapRemoteControl remote_control,
						      char *request, char **error_out) ;
gboolean zMapRemoteCommandValidateReply(ZMapRemoteControl remote_control,
                                        char *original_request, char *reply, char **error_out) ;
gboolean zMapRemoteCommandRequestsIdentical(ZMapRemoteControl remote_control,
                                            char *request_1, char *request_2, char **error_out) ;
gboolean zMapRemoteCommandRequestIsCommand(char *request, char *command) ;
char *zMapRemoteCommandRequestGetCommand(char *request) ;
int zMapRemoteCommandGetPriority(char *request) ;
char *zMapRemoteCommandRequestGetEnvelopeAttr(char *request, RemoteEnvelopeAttrType attr) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
char *zMapRemoteCommandRequestGetRequestID(char *request) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




gboolean zMapRemoteCommandReplyGetAttributes(const char *reply,
					     char **command_out,
					     RemoteCommandRCType *return_code_out,
                                             char **reason_out,
					     char **reply_body_out,
					     char **error_out) ;
gboolean zMapRemoteCommandGetAttribute(const char *message, const char *element, const char *attribute,
                                       char **attribute_value_out, char **error_out) ;

ZMAP_ENUM_TO_SHORT_TEXT_DEC(zMapRemoteCommandRC2Str, RemoteCommandRCType) ;
ZMAP_ENUM_FROM_SHORT_TEXT_DEC(zMapRemoteCommandStr2RC, RemoteCommandRCType) ;

ZMAP_ENUM_TO_SHORT_TEXT_DEC(zMapRemoteCommandRC2Desc, RemoteValidateRCType) ;

#endif /* ZMAP_REMOTE_COMMAND_H */
