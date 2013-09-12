/*  File: zmapRemoteCommand.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2011: Genome Research Ltd.
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
 * Description: Functions to handle processing of requests/responses
 *              for remote control.
 *
 * Exported functions: See ZMap/zmapRemoteCommand.h
 * HISTORY:
 * Last edited: Apr 11 11:23 2012 (edgrif)
 * Created: Mon Dec 19 10:21:32 2011 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <glib.h>

#include <ZMap/zmapEnum.h>
#include <ZMap/zmapXML.h>
#include <ZMap/zmapRemoteProtocol.h>
#include <ZMap/zmapRemoteCommand.h>
#include <zmapRemoteControl_P.h>



typedef enum {ENVELOPE_REQUEST, ENVELOPE_REPLY} EnvelopeType ;


/* Used by lots of the callback functions to return data from parsed xml. */
typedef struct ParseDataStructName
{
  EnvelopeType envelope_type ;

  /* get rid of this........ */
  GQuark message_type ;					    /* request or reply ? */

  /* These are used to check we actually find the elements. */
  gboolean validate_command ;				    /* TRUE => look for command attribute. */

  gboolean zmap_start ;
  gboolean zmap_content ;
  gboolean zmap_end ;
  gboolean req_reply_start ;
  gboolean req_reply_content ;
  gboolean req_reply_end ;


  /* Used for input and output of these fields according to function using this struct. */
  GQuark version ;
  GQuark app_id ;
  GQuark clipboard_id ;
  char *request_id ;

  char *command ;


  /* User for finding reply attributes. */
  char *reply ;
  RemoteCommandRCType return_code ;
  char *reason ;
  char *reply_body ;

} ParseDataStruct, *ParseData ;


/* Used by lots of the callback functions to return data from parsed xml. */
typedef struct ParseSingleDataStructName
{
  gboolean valid ;					    /* If this is FALSE something failed. */

  char *attribute ;

  char *attribute_value ;

} ParseSingleDataStruct, *ParseSingleData ;



static GArray *createRequestReply(EnvelopeType type, GQuark version,
				  GQuark app_id, GQuark clipboard_id,
				  char *request_id,
				  char *command, char *viwe, int timeout_secs,
				  RemoteCommandRCType return_code, char *reason, ZMapXMLUtilsEventStack result) ;
static gboolean getRequestAttrs(char *xml_request, GQuark *req_version,
				GQuark *app_id, GQuark *req_clipboard_id, char **req_request_id, char **command,
				char **error_out) ;
static gboolean getReplyAttrs(char *xml_request, GQuark *reply_version,
			      GQuark *reply_app_id, GQuark *reply_clipboard_id, char **reply_id, char **reply_command,
			      char **error_out) ;
static gboolean checkAttribute(ZMapXMLParser parser, ZMapXMLElement element,
			       char *attribute, GQuark expected_value, char **error_out) ;
static gboolean checkReplyAttrs(ZMapRemoteControl remote_control,
				char *original_request, char *reply, char **error_out) ;
static gboolean getAttribute(ZMapXMLParser parser, ZMapXMLElement element,
			     char *attribute, char **value_out, char **error_out) ;
static RemoteValidateRCType reqReplyValidate(ZMapRemoteControl remote_control,
					     GQuark msg_type, gboolean validate_command,
					     GQuark version,
					     GQuark app_id, GQuark clipboard_id, char *xml_request, char **error_out) ;
static char *getReplyContents(char *reply) ;

static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;
static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;
static gboolean xml_request_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;
static gboolean xml_request_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;
static gboolean xml_request_attrs_cb(gpointer user_data, ZMapXMLElement request_element, ZMapXMLParser parser) ;
static gboolean xml_command_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;
static gboolean xml_reply_attrs_cb(gpointer user_data, ZMapXMLElement request_element, ZMapXMLParser parser) ;
static gboolean xml_reply_body_cb(gpointer user_data, ZMapXMLElement request_element, ZMapXMLParser parser) ;
static gboolean xmlGetAttrCB(gpointer user_data, ZMapXMLElement request_element, ZMapXMLParser parser) ;
static gboolean xml_return_true_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;


static ZMapXMLObjTagFunctionsStruct validate_starts_G[] =
  {
    {"zmap",    xml_zmap_start_cb  },
    {"request", xml_request_start_cb},
    {"reply",   xml_request_start_cb},
    {NULL, NULL}
  };

static ZMapXMLObjTagFunctionsStruct validate_ends_G[] =
  {
    { "zmap",       xml_zmap_end_cb    },
    { "request",    xml_request_end_cb    },
    { "reply",      xml_request_end_cb    },
    {NULL, NULL}
  };



static ZMapXMLObjTagFunctionsStruct command_starts_G[] =
  {
    {"request", xml_command_start_cb},
    {NULL, NULL}
  };

static ZMapXMLObjTagFunctionsStruct command_ends_G[] =
  {
    { "request",    xml_return_true_cb    },
    {NULL, NULL}
  };

static ZMapXMLObjTagFunctionsStruct get_attrs_starts_G[] =
  {
    {"zmap", xml_request_attrs_cb},
    {NULL, xml_command_start_cb},			    /* Filled in at run time with request or reply. */
    {NULL, NULL}
  };

static ZMapXMLObjTagFunctionsStruct get_attrs_ends_G[] =
  {
    { NULL,    xml_return_true_cb    },			    /* Filled in at run time with request or reply. */
    { "zmap",    xml_return_true_cb    },
    {NULL, NULL}
  };


static ZMapXMLObjTagFunctionsStruct parse_reply_starts_G[] =
  {
    {"reply", xml_reply_attrs_cb},
    {NULL, NULL}
  } ;

static ZMapXMLObjTagFunctionsStruct parse_reply_ends_G[] =
  {
    { "reply", xml_reply_body_cb},
    {NULL, NULL}
  };

static ZMapXMLObjTagFunctionsStruct parse_single_attr_starts_G[] =
  {
    {NULL, xmlGetAttrCB},
    {NULL, NULL}
  } ;

static ZMapXMLObjTagFunctionsStruct parse_single_attr_ends_G[] =
  {
    {NULL, xml_return_true_cb},
    {NULL, NULL}
  };




/* 
 *                External functions.
 */


/* Make a zmap xml request envelope:
 * 
 * <ZMap version="n.n" type="request" app_id="xxxx" clipboard_id="yyyy" request_id="zzzz" [timeout="seconds"]>
 *   <request command="some_command">
 *   </request>
 * </zmap>
 * 
 * Add further content using zMapRemoteCommandRequestAddBody()
 * 
 * 
 *  */
GArray *zMapRemoteCommandCreateRequest(ZMapRemoteControl remote_control,
				       char *command, char *view, int timeout_secs)
{
  GArray *envelope = NULL ;
  char *request_id ;

  request_id = zmapRemoteControlMakeReqID(remote_control) ;

  envelope = createRequestReply(ENVELOPE_REQUEST, remote_control->version,
				remote_control->receive->our_app_name,
				remote_control->send->their_unique_str,
				request_id,
				command, view, timeout_secs,
				REMOTE_COMMAND_RC_OK, NULL, NULL) ;

  return envelope ;
}


/* Given the original request xml string, make a valid xml reply
 * for that request (as an xml stack):
 * 
 * <ZMap version="n.n" type="reply" app_id="xxxx" clipboard_id="yyyy" request_id="zzzz">
 *   <reply command="something_command">
 *   </reply>
 * </zmap>
 * 
 * Add further content using zMapRemoteCommandRequestAddBody()
 *
 *  */
GArray *zMapRemoteCommandCreateReplyFromRequest(ZMapRemoteControl remote_control,
						char *xml_request,
						RemoteCommandRCType return_code, char *reason,
						ZMapXMLUtilsEventStack reply,
						char **error_out)
{
  GArray *envelope = NULL ;
  char *err_msg = NULL ;
  GQuark req_version ;
  GQuark req_app_id ;
  GQuark req_clipboard_id ;
  char *req_request_id ;
  char *req_command ;


  if (reqReplyValidate(remote_control, g_quark_from_string(ZACP_REQUEST), TRUE, 0, 0, 0, xml_request, &err_msg)
      != REMOTE_VALIDATE_RC_OK)
    {
      *error_out = err_msg ;
    }
  else
    {
      getRequestAttrs(xml_request, &req_version, &req_app_id, &req_clipboard_id,
		      &req_request_id, &req_command, &err_msg) ;


      if ((return_code == REMOTE_COMMAND_RC_OK && !reason) || (return_code != REMOTE_COMMAND_RC_OK && reason))
	envelope = createRequestReply(ENVELOPE_REPLY, remote_control->version,
				      remote_control->receive->our_app_name, req_clipboard_id,
				      req_request_id,
				      req_command, NULL, -1, 
				      return_code, reason, reply) ;
      else
	*error_out = g_strdup_printf("Bad args: %s.",
				     (return_code == REMOTE_COMMAND_RC_OK
				      ? "command succeeded but an error string was given"
				      : "command failed but no error string was given")) ;
    }

  return envelope ;
}


/* Given the original request xml string, make a valid xml reply envelope 
 * for that request (as an xml stack):
 * 
 * <ZMap version="n.n" type="reply" app_id="xxxx" clipboard_id="yyyy" request_id="zzzz">
 *
 * </zmap>
 * 
 * You would probably want to do this when the original request had a valid envelope
 * but an invalid body.
 * 
 *  Add further content using zMapRemoteCommandRequestAddBody()
 *
 *  */
GArray *zMapRemoteCommandCreateReplyEnvelopeFromRequest(ZMapRemoteControl remote_control,
							char *xml_request,
							RemoteCommandRCType return_code, char *reason,
							ZMapXMLUtilsEventStack reply,
							char **error_out)
{
  GArray *envelope = NULL ;
  RemoteValidateRCType valid_rc ;
  char *err_msg = NULL ;

  valid_rc = reqReplyValidate(remote_control, g_quark_from_string(ZACP_REQUEST), FALSE, 0, 0, 0,
			      xml_request, &err_msg) ;

  switch (valid_rc)
    {
    case REMOTE_VALIDATE_RC_ENVELOPE_CONTENT:
    case REMOTE_VALIDATE_RC_ENVELOPE_XML:
      {
	*error_out = err_msg ;
	break ;
      }
   default:
      {
	GQuark req_version = 0 ;
	GQuark req_app_id = 0 ;
	GQuark req_clipboard_id = 0 ;
	char *request_command = NULL ;
	char *req_request_id = NULL ;

	if (!getRequestAttrs(xml_request, &req_version, &req_app_id, &req_clipboard_id,
			     &req_request_id, &request_command, &err_msg))
	  {
	    *error_out = err_msg ;
	  }
	else
	  {
	    char *command ;

	    if (valid_rc == REMOTE_VALIDATE_RC_BODY_COMMAND)
	      command = "**error**" ;
	    else
	      command = request_command ;

	    if ((return_code == REMOTE_COMMAND_RC_OK && !reason) || (return_code != REMOTE_COMMAND_RC_OK && reason))
	      envelope = createRequestReply(ENVELOPE_REPLY, remote_control->version,
					    remote_control->receive->our_app_name, req_clipboard_id,
					    req_request_id,
					    command, NULL, -1, 
					    return_code, reason, reply) ;
	    else
	      *error_out = g_strdup_printf("Bad args: %s.",
					   (return_code == REMOTE_COMMAND_RC_OK
					    ? "command succeeded but an error string was given"
					    : "command failed but no error string was given")) ;
	  }

	break ;
      }
    }

  return envelope ;
}


/* Just a cover func for correct xml call...saves caller needing to know the xml package.
 * 
 *  */
GArray *zMapRemoteCommandAddBody(GArray *request_in_out, char *req_or_reply,
				 ZMapXMLUtilsEventStack request_body)
{
  GArray *xml_stack = request_in_out ;

  xml_stack = zMapXMLUtilsAddStackToEventsArrayToElement(xml_stack,
							 req_or_reply, 0,
							 NULL, NULL,
							 request_body) ;

  return xml_stack ;
}



/* Given an element only will produce:
 * 
 *  <element/>
 * 
 * or with an attribute and attribute_value:
 * 
 * <element attribute="attribute_value"/>
 * 
 * The intention is to provide a quick way to make this element without
 * the need for allocation/free. The stack is "read-only", if you need
 * to alter it then you need to take a copy of it.
 * 
 * The element, attribute and attribute_value you supply need to be around until the xml
 * has been made.
 *  */
ZMapXMLUtilsEventStack zMapRemoteCommandCreateElement(char *element, char *attribute, char *attribute_value)
{
  static ZMapXMLUtilsEventStackStruct
    stack[] =
      {
	{ZMAPXML_NULL_EVENT, NULL, ZMAPXML_EVENT_DATA_NONE,  {0}},
	{ZMAPXML_NULL_EVENT, NULL, ZMAPXML_EVENT_DATA_NONE, {0}},
	{ZMAPXML_NULL_EVENT, NULL, ZMAPXML_EVENT_DATA_NONE,  {0}},
	{ZMAPXML_NULL_EVENT}
      } ;
  int stack_index = 0 ;

  stack[stack_index].event_type = ZMAPXML_START_ELEMENT_EVENT ;
  stack[stack_index].name = element ;
  stack_index++ ;

  if (attribute)
    {
      stack[stack_index].event_type = ZMAPXML_ATTRIBUTE_EVENT ;
      stack[stack_index].name = attribute ;
      stack[stack_index].data_type = ZMAPXML_EVENT_DATA_QUARK ;
      stack[stack_index].value.q = g_quark_from_string(attribute_value) ;

      stack_index++ ;
    }

  stack[stack_index].event_type = ZMAPXML_END_ELEMENT_EVENT ;
  stack[stack_index].name = element ;

  return &(stack[0]) ;
}


/* Given a Reply message return a stack which will make the following xml:
 * 
 *  <message>
 *     message text
 *  </message>
 * 
 * The intention is to provide a quick way to make this element without
 * the need for allocation/free. The stack is "read-only", if you need
 * to alter it then you need to take a copy of it.
 *  */
ZMapXMLUtilsEventStack zMapRemoteCommandMessage2Element(char *message)
{
  static ZMapXMLUtilsEventStackStruct
    message_stack[] =
    {
      {ZMAPXML_START_ELEMENT_EVENT, ZACP_MESSAGE,   ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_CHAR_DATA_EVENT,     NULL,    ZMAPXML_EVENT_DATA_STRING, {0}},
      {ZMAPXML_END_ELEMENT_EVENT,   ZACP_MESSAGE,      ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_NULL_EVENT}
    } ;

  message_stack[1].value.s = message ;

  return &(message_stack[0]) ;
}







/* Returns the current command for either a request made or a request received,
 * returns NULL if there is no command being handled. */
const char *zMapRemoteCommandGetCurrCommand(ZMapRemoteControl remote_control)
{
  char *command = NULL ;

  command = zMapRemoteCommandRequestGetCommand(remote_control->curr_remote->any_request) ;

  return command ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* We need a call to add the return code and the result of the command...... */
/* 
 * 
 * If return_code == REMOTE_COMMAND_RC_OK then reason == NULL.
 * 
 *  */
GArray *zMapRemoteCommandReplyAddResult(GArray *reply_envelope,
					RemoteCommandRCType return_code, char *reason,
					char *result)
{
  GArray *full_reply = NULL ;
  static ZMapXMLUtilsEventStackStruct

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    reply_start[] =
    {
      {ZMAPXML_START_ELEMENT_EVENT, ZACP_REPLY,  ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     ZACP_CMD,    ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_NULL_EVENT}
    },
    reason_attr[] =
      {
	{ZMAPXML_ATTRIBUTE_EVENT,     ZACP_REASON, ZMAPXML_EVENT_DATA_INTEGER, {0}},
	{ZMAPXML_NULL_EVENT}
      },
    reply_end[] =
      {
	{ZMAPXML_END_ELEMENT_EVENT, ZACP_REPLY, ZMAPXML_EVENT_DATA_NONE,  {0}},
	{ZMAPXML_NULL_EVENT}
      } ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    reply_start[] =
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      {ZMAPXML_START_ELEMENT_EVENT, ZACP_REPLY,       ZMAPXML_EVENT_DATA_NONE,  {0}},
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      {ZMAPXML_ATTRIBUTE_EVENT,     ZACP_RETURN_CODE, ZMAPXML_EVENT_DATA_QUARK, {0}},



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      {ZMAPXML_END_ELEMENT_EVENT,   ZACP_REPLY,       ZMAPXML_EVENT_DATA_NONE,  {0}},
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      {ZMAPXML_NULL_EVENT}
    } ;



  /* Do some checking, there should be no reason string if the return_code is REMOTE_COMMAND_RC_OK. */
  if ((return_code == REMOTE_COMMAND_RC_OK && !reason) || (return_code != REMOTE_COMMAND_RC_OK && reason))
    {
      /* Fill in reply attributes. */
      reply_start[1].value.q = g_quark_from_string(zMapRemoteCommandRC(return_code)) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if (reason)
	reason_attr[0].value.s = g_strdup(reason) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Create the stack of xml.... */
      full_reply = zMapXMLUtilsStackToEventsArray(&reply_start[0]) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if (reason)
	full_reply = zMapXMLUtilsAddStackToEventsArrayEnd(full_reply, &reason_attr[0]) ;

      full_reply = zMapXMLUtilsAddStackToEventsArrayEnd(full_reply, &reply_end[0]) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



      full_reply = zMapXMLUtilsAddStackToEventsArrayAfterElement(reply_envelope, ZACP_TAG, reply_start) ;
    }


  return full_reply ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */






/* Validates only the envelope part of the message:
 * 
 * <zmap type="request" version="2.0" app_id="remotecontrol" clipboard_id="ZMap-30531-1334066834" request_id="3">
 * 
 * </zmap>
 * 
 * In doing this the parser must validate the body of the message too so will return
 * the following error codes:
 * 
 * REMOTE_VALIDATE_RC_OK - envelope is good and body is valid xml
 * REMOTE_VALIDATE_RC_BODY_XML - problem in body
 * REMOTE_VALIDATE_RC_ENVELOPE_CONTENT, REMOTE_VALIDATE_RC_ENVELOPE_XML - problem in envelope
 * 
 *  */
RemoteValidateRCType zMapRemoteCommandValidateEnvelope(ZMapRemoteControl remote_control,
						       char *xml_request, char **error_out)
{
  RemoteValidateRCType result = REMOTE_VALIDATE_RC_OK ;
  gboolean validate_command = FALSE ;

  result = reqReplyValidate(remote_control,
			    g_quark_from_string(ZACP_REQUEST), validate_command,
			    remote_control->version,
			    (remote_control->send ? remote_control->send->their_app_name : 0),
			    remote_control->receive->our_unique_str,
			    xml_request, error_out) ;

  return result ;
}


/* Validates the envelope and that there is a command:
 * 
 * <zmap type="request" version="2.0" app_id="remotecontrol" clipboard_id="ZMap-30531-1334066834" request_id="3">
 *   <request command="XXXX">
 * 
 *   </request>
 * </zmap>
 * 
 *  */
RemoteValidateRCType zMapRemoteCommandValidateRequest(ZMapRemoteControl remote_control,
						      char *xml_request, char **error_out)
{
  RemoteValidateRCType result = REMOTE_VALIDATE_RC_OK ;
  gboolean validate_command = TRUE ;

  result = reqReplyValidate(remote_control,
			    g_quark_from_string(ZACP_REQUEST), validate_command,
			    remote_control->version,
			    (remote_control->send ? remote_control->send->their_app_name : 0),
			    remote_control->receive->our_unique_str,
			    xml_request, error_out) ;

  return result ;
}


RemoteValidateRCType zMapRemoteCommandValidateReply(ZMapRemoteControl remote_control,
						    char *original_request, char *reply, char **error_out)
{
  RemoteValidateRCType result = REMOTE_VALIDATE_RC_OK ;

  result = checkReplyAttrs(remote_control, original_request, reply, error_out) ;

  return result ;
}



/* Returns TRUE if the given request is for the given command, FALSE otherwise. */
gboolean zMapRemoteCommandRequestIsCommand(char *request, char *command)
{
  gboolean result = FALSE ;
  ParseDataStruct command_data = {ENVELOPE_REQUEST, 0, TRUE,
				  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
				  0, 0, 0, NULL,
				  NULL,
				  NULL, -1, NULL, NULL} ;
  ZMapXMLParser parser;

  parser = zMapXMLParserCreate(&command_data, FALSE, FALSE) ;

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &command_starts_G[0], &command_ends_G[0]) ;

  if ((zMapXMLParserParseBuffer(parser, request, strlen(request))) == TRUE)
    {
      if ((command_data.req_reply_content))
	{
	  if (g_ascii_strcasecmp(command_data.command, command) == 0)
	    result = TRUE ;
	}
    }

  return result ;
}


/* Returns command attribute value or NULL if there is some error (e.g. bad xml). */
char *zMapRemoteCommandRequestGetCommand(char *request)
{
  char *command = NULL ;
  ParseDataStruct command_data = {ENVELOPE_REQUEST, 0, TRUE,
				  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
				  0, 0, 0, NULL,
				  NULL,
				  NULL, -1, NULL, NULL} ;
  ZMapXMLParser parser;

  parser = zMapXMLParserCreate(&command_data, FALSE, FALSE) ;

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &command_starts_G[0], &command_ends_G[0]) ;

  if ((zMapXMLParserParseBuffer(parser, request, strlen(request))) == TRUE)
    {
      if ((command_data.req_reply_content))
	command = command_data.command ;
    }

  return command ;
}


/* Pulls out various parts of the reply which is in one if two formats:
 * 
 * If command successful:
 * 
 * <zmap version="N.N" type="request" app_id="XXXX" clipboard_id="XXXX" request_id="YYYY">
 *   <reply command="ZZZZ"  return_code="ok">
 *           .
 *          body
 *           .
 *   </reply>
 * </zmap>
 * 
 * If command unsuccessful:
 * 
 * <zmap version="N.N" type="request" app_id="XXXX" clipboard_id="XXXX" request_id="YYYY">
 *   <reply command="ZZZZ" return_code="ok" reason="short error description"/>
 * </zmap>
 * 
 * Return TRUE if parsing was successful and fills in command_out, return_code_out
 * and either reason_out or reply_body_out. Otherwise returns FALSE, sets return_code_out
 * to REMOTE_COMMAND_RC_BAD_XML and gives the error in error_out.
 * 
 */
gboolean zMapRemoteCommandReplyGetAttributes(char *reply,
					     char **command_out,
					     RemoteCommandRCType *return_code_out, char **reason_out,
					     char **reply_body_out,
					     char **error_out)
{
  gboolean result = FALSE ;
  ParseDataStruct command_data = {-1, 0, TRUE,
				  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
				  0, 0, 0, NULL,
				  NULL,
				  NULL, -1, NULL, NULL} ;
  ZMapXMLParser parser ;

  command_data.reply = reply ;

  parser = zMapXMLParserCreate(&command_data, FALSE, FALSE) ;

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &parse_reply_starts_G[0], &parse_reply_ends_G[0]) ;

  if (!(result = zMapXMLParserParseBuffer(parser, reply, strlen(reply))))
    {
      *error_out = zMapXMLParserLastErrorMsg(parser) ;

      result = FALSE ;
    }
  else
    {
      /* Parsing was successful but the command may have suceeded or not.... */

      *command_out = command_data.command ;
      *return_code_out = command_data.return_code ;
      if (command_data.reason)
	*reason_out = command_data.reason ;
      else if (command_data.reply_body)
	*reply_body_out = command_data.reply_body ;
      
      result = TRUE ;
    }

  return result ;
}


/* Returns the attribute value in:
 * 
 * <element attribute="attribute_value">
 * 
 * Returns TRUE if attribute found, returns FALSE and an error otherwise,
 * error should be g_free'd when finished with.
 * 
 *  */
gboolean zMapRemoteCommandGetAttribute(char *message,
				       char *element, char *attribute, char **attribute_value_out,
				       char **error_out)
{
  gboolean result = FALSE ;
  ParseSingleDataStruct attribute_data = {TRUE, NULL, NULL} ;
  ZMapXMLParser parser ;

  parse_single_attr_starts_G[0].element_name = element ;
  parse_single_attr_ends_G[0].element_name = element ;

  attribute_data.attribute = attribute ;

  parser = zMapXMLParserCreate(&attribute_data, FALSE, FALSE) ;

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &parse_single_attr_starts_G[0], &parse_single_attr_ends_G[0]) ;

  if (!(result = zMapXMLParserParseBuffer(parser, message, strlen(message))))
    {
      *error_out = g_strdup(zMapXMLParserLastErrorMsg(parser)) ;
    }
  else if (!(attribute_data.attribute_value))
    {
      *error_out = g_strdup_printf("%s not found in %s.", attribute, message) ;

      result = FALSE ;
    }
  else
    {
      *attribute_value_out = attribute_data.attribute_value ;
      
      result = TRUE ;
    }

  return result ;
}



/* Auto "function from macro list" definition, returns short text version of RemoteCommandRCType value,
 * i.e. given REMOTE_COMMAND_RC_BAD_XML, returns "bad_xml" */
ZMAP_ENUM_AS_NAME_STRING_FUNC(zMapRemoteCommandRC2Str, RemoteCommandRCType, REMOTE_COMMAND_RC_LIST) ;

/* The opposite function, given "bad_xml", returns REMOTE_COMMAND_RC_BAD_XML */
ZMAP_ENUM_FROM_STRING_FUNC(zMapRemoteCommandStr2RC, RemoteCommandRCType, -1, REMOTE_COMMAND_RC_LIST, dummy, dummy) ;


ZMAP_ENUM_TO_SHORT_TEXT_FUNC(zMapRemoteCommandRC2Desc, RemoteValidateRCType, REMOTE_VALIDATE_RC_LIST) ;





/* 
 *                Internal functions.
 */




/* Creates a request or reply according to the envelope type.
 * 
 * The last three args are for replies only, may need to rationalise args at some time.
 *  */
static GArray *createRequestReply(EnvelopeType type, GQuark version,
				  GQuark app_id, GQuark clipboard_id,
				  char *request_id,
				  char *command, char *view_id, int timeout_secs,
				  RemoteCommandRCType return_code, char *reason, ZMapXMLUtilsEventStack result)
{
  GArray *envelope = NULL ;
  static ZMapXMLUtilsEventStackStruct
    envelope_start[] =
    {
      {ZMAPXML_START_ELEMENT_EVENT, ZACP_TAG,          ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     ZACP_TYPE,         ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     ZACP_VERSION_ID,   ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     ZACP_APP_ID,       ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     ZACP_CLIPBOARD_ID, ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     ZACP_REQUEST_ID,   ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_NULL_EVENT}
    },
    view_id_attr[] =
      {
	{ZMAPXML_ATTRIBUTE_EVENT,     ZACP_VIEWID,   ZMAPXML_EVENT_DATA_QUARK, {0}},
	{ZMAPXML_NULL_EVENT}
      },
    timeout_attr[] =
      {
	{ZMAPXML_ATTRIBUTE_EVENT,     ZACP_TIMEOUT,   ZMAPXML_EVENT_DATA_INTEGER, {0}},
	{ZMAPXML_NULL_EVENT}
      },
    request_reply_start[] =
      {
	{ZMAPXML_START_ELEMENT_EVENT, "",         ZMAPXML_EVENT_DATA_NONE,  {0}},
	{ZMAPXML_ATTRIBUTE_EVENT,     ZACP_CMD,   ZMAPXML_EVENT_DATA_QUARK, {0}},
	{ZMAPXML_NULL_EVENT}
      },
    reply_return_code[] =
      {
	{ZMAPXML_ATTRIBUTE_EVENT, ZACP_RETURN_CODE,   ZMAPXML_EVENT_DATA_QUARK, {0}},
	{ZMAPXML_NULL_EVENT}
      },
    reply_reason_attr[] =
      {
	{ZMAPXML_ATTRIBUTE_EVENT, ZACP_REASON, ZMAPXML_EVENT_DATA_QUARK, {0}},
	{ZMAPXML_NULL_EVENT}
      },

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    reply_body[] =
      {
	{ZMAPXML_CHAR_DATA_EVENT, "", ZMAPXML_EVENT_DATA_STRING, {0}},
	{ZMAPXML_NULL_EVENT}
      },
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    envelope_end[] =
      {
	{ZMAPXML_END_ELEMENT_EVENT, "",        ZMAPXML_EVENT_DATA_NONE,  {0}},
	{ZMAPXML_END_ELEMENT_EVENT, ZACP_TAG,  ZMAPXML_EVENT_DATA_NONE,  {0}},
	{ZMAPXML_NULL_EVENT}
      } ;
  GQuark envelope_type ;

  if (type == ENVELOPE_REQUEST)
    envelope_type = g_quark_from_string(ZACP_REQUEST) ;
  else
    envelope_type = g_quark_from_string(ZACP_REPLY) ;


  /* Create the envelope start. */
  envelope_start[1].value.q = envelope_type ;
  envelope_start[2].value.q = version ;
  envelope_start[3].value.q = app_id ;
  envelope_start[4].value.q = clipboard_id ;
  envelope_start[5].value.q = g_quark_from_string(request_id) ;

  envelope = zMapXMLUtilsStackToEventsArray(&envelope_start[0]) ;


  /* Possibly add timeout for requests. */
  if (type == ENVELOPE_REQUEST && timeout_secs >= 0)
    {
      timeout_attr[0].value.i = timeout_secs ;

      envelope = zMapXMLUtilsAddStackToEventsArrayEnd(envelope, &timeout_attr[0]) ;
    }



  /* Only do this bit if there is a command..... */

  /* WHAT WAS I DOING HERE....SIGH.... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (!command)
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  if (command)
    {    
      /* Add the request or reply start. */
      request_reply_start[0].name = (char *)g_quark_to_string(envelope_type) ;
      request_reply_start[1].value.q = g_quark_from_string(command) ;

      envelope = zMapXMLUtilsAddStackToEventsArrayEnd(envelope, &request_reply_start[0]) ;

      /* Possibly add viewid for requests. */
      if (type == ENVELOPE_REQUEST && view_id)
	{
	  view_id_attr[0].value.i = g_quark_from_string(view_id) ;

	  envelope = zMapXMLUtilsAddStackToEventsArrayEnd(envelope, &view_id_attr[0]) ;
	}

      /* If it's a reply then add return code and either optional reply if command worked or reason if
       * it failed. */
      if (type == ENVELOPE_REPLY)
	{
	  /* Fill in reply attributes. */
	  reply_return_code[0].value.q = g_quark_from_string(zMapRemoteCommandRC2Str(return_code)) ;

	  envelope = zMapXMLUtilsAddStackToEventsArrayEnd(envelope, &reply_return_code[0]) ;

	  if (return_code == REMOTE_COMMAND_RC_OK)
	    {
	      if (result)
		{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		  reply_body[0].value.s = g_strdup(result) ;

		  envelope = zMapXMLUtilsAddStackToEventsArrayEnd(envelope, &reply_body[0]) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
		  envelope = zMapXMLUtilsAddStackToEventsArrayEnd(envelope, result) ;
		}
	    }
	  else
	    {
	      reply_reason_attr[0].value.q = g_quark_from_string(reason) ;

	      envelope = zMapXMLUtilsAddStackToEventsArrayEnd(envelope, &reply_reason_attr[0]) ;
	    }
	}
    }


  /* Add the envelope end. */
  envelope_end[0].name = (char *)g_quark_to_string(envelope_type) ;


  envelope = zMapXMLUtilsAddStackToEventsArrayEnd(envelope, &envelope_end[0]) ;


  return envelope ;
}



/* Functions to test/get "request" elements command attribute. */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Returns TRUE if everything is ok, struct says what happened. */
gboolean getCommandAttribute(char *request, ParseCommand command_data)
{
  gboolean result = FALSE ;
  ZMapXMLParser parser;

  parser = zMapXMLParserCreate(command_data, FALSE, FALSE) ;

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &request_starts_G[0], &request_ends_G[0]) ;

  if ((zMapXMLParserParseBuffer(parser, request, strlen(request))) == TRUE)
    result = command_data->error ;

  return result ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static gboolean checkReplyAttrs(ZMapRemoteControl remote_control,
				char *original_request, char *reply, char **error_out)
{
  gboolean result = FALSE ;
  GQuark req_version = 0, req_app_id = 0, req_clipboard_id = 0 ;
  char *req_request_id = NULL, *req_command = NULL ;
  GQuark reply_version = 0, reply_app_id = 0, reply_clipboard_id = 0 ;
  char *reply_id = NULL, *reply_command = NULL ;

  if ((result = getRequestAttrs(original_request,
				&req_version, &req_app_id, &req_clipboard_id, &req_request_id, &req_command,
				error_out))
      && (result = getReplyAttrs(reply,
				 &reply_version, &reply_app_id, &reply_clipboard_id, &reply_id, &reply_command,
				 error_out)))
    {

      /* check app_id ??? probably not....changes between request/reply always .... */

      if (req_version == reply_version
	  && remote_control->send->their_app_name == reply_app_id
	  && remote_control->send->their_unique_str == reply_clipboard_id
	  && g_ascii_strcasecmp(req_request_id, reply_id) == 0
	  && g_ascii_strcasecmp(req_command, reply_command) == 0)
	result = TRUE ;
    }

  return result ;
}



/* req_command is optional. */
static gboolean getRequestAttrs(char *xml_request, GQuark *req_version,
				GQuark *req_app_id, GQuark *req_clipboard_id,
				char **req_request_id, char **req_command,
				char **error_out)
{
  gboolean result = FALSE ;
  ParseDataStruct validate_data = {ENVELOPE_REQUEST, 0, TRUE,
				   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
				   0, 0, 0, NULL,
				   NULL,
				   NULL, -1, NULL, NULL} ;
  ZMapXMLParser parser;

  if (!req_command)
    validate_data.validate_command = FALSE ;

  parser = zMapXMLParserCreate(&validate_data, FALSE, FALSE) ;

  get_attrs_starts_G[1].element_name = ZACP_REQUEST ;
  get_attrs_ends_G[0].element_name = ZACP_REQUEST ;

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &get_attrs_starts_G[0], &get_attrs_ends_G[0]) ;

  if (!(result = zMapXMLParserParseBuffer(parser, xml_request, strlen(xml_request))))
    {
      *error_out = zMapXMLParserLastErrorMsg(parser) ;
    }
  else
    {
      *req_version = validate_data.version ;
      *req_app_id = validate_data.app_id ;
      *req_clipboard_id = validate_data.clipboard_id ;
      *req_request_id = validate_data.request_id ;

      if (req_command)
	*req_command = validate_data.command ;

      result = TRUE ;
    }

  return result ;
}


static gboolean getReplyAttrs(char *xml_reply, GQuark *reply_version,
			      GQuark *reply_app_id, GQuark *reply_clipboard_id, char **reply_id, char **reply_command,
			      char **error_out)
{
  gboolean result = FALSE ;
  ParseDataStruct validate_data = {ENVELOPE_REPLY, 0, TRUE,
				   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
				   0, 0, 0, NULL,
				   NULL,
				   NULL, -1, NULL, NULL} ;
  ZMapXMLParser parser;

  parser = zMapXMLParserCreate(&validate_data, FALSE, FALSE) ;

  get_attrs_starts_G[1].element_name = ZACP_REPLY ;
  get_attrs_ends_G[0].element_name = ZACP_REPLY ;

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &get_attrs_starts_G[0], &get_attrs_ends_G[0]) ;

  if (!(result = zMapXMLParserParseBuffer(parser, xml_reply, strlen(xml_reply))))
    {
      *error_out = zMapXMLParserLastErrorMsg(parser) ;
    }
  else
    {
      *reply_version = validate_data.version ;
      *reply_app_id = validate_data.app_id ;
      *reply_clipboard_id = validate_data.clipboard_id ;
      *reply_id = validate_data.request_id ;
      *reply_command = validate_data.command ;

      result = TRUE ;
    }

  return result ;
}





static RemoteValidateRCType reqReplyValidate(ZMapRemoteControl remote_control,
					     GQuark msg_type,  gboolean validate_command,
					     GQuark version,
					     GQuark app_id, GQuark clipboard_id, char *xml_request, char **error_out)
{
  RemoteValidateRCType result = REMOTE_VALIDATE_RC_OK ;
  gboolean parse_result ;
  ParseDataStruct validate_data = {ENVELOPE_REQUEST, 0, TRUE,
				   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
				   0, 0, 0, NULL,
				   NULL,
				   NULL, -1, NULL, NULL} ;
  ZMapXMLParser parser ;

  validate_data.message_type = msg_type ;
  validate_data.validate_command = validate_command ;
  validate_data.version = remote_control->version ;
  validate_data.app_id = app_id ;
  validate_data.clipboard_id = clipboard_id ;

  parser = zMapXMLParserCreate(&validate_data, FALSE, FALSE) ;

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &validate_starts_G[0], &validate_ends_G[0]) ;

  if (!(parse_result = zMapXMLParserParseBuffer(parser, xml_request, strlen(xml_request)))
      || (!validate_data.zmap_start || !validate_data.req_reply_start
	  || !validate_data.req_reply_end || !validate_data.zmap_end))
    {
      char *err_msg = NULL ;

      err_msg = zMapXMLParserLastErrorMsg(parser) ;

      if (!validate_data.zmap_start)
	result = REMOTE_VALIDATE_RC_ENVELOPE_XML ;
      else if (validate_data.zmap_start && !validate_data.zmap_content)
	result = REMOTE_VALIDATE_RC_ENVELOPE_CONTENT ;
      else if (!validate_data.req_reply_start)
	result = REMOTE_VALIDATE_RC_BODY_XML ;
      else if (!validate_data.validate_command)
	result = REMOTE_VALIDATE_RC_BODY_COMMAND ;
      else if (validate_data.validate_command && !validate_data.req_reply_content)
	result = REMOTE_VALIDATE_RC_BODY_CONTENT ;
      else if (!validate_data.req_reply_end)
	result = REMOTE_VALIDATE_RC_BODY_XML ;
      else if (!validate_data.zmap_end)
	result = REMOTE_VALIDATE_RC_ENVELOPE_XML ;

      if (!err_msg)
	err_msg = g_strdup_printf("\"%s\" in \"%s\"",
				  zMapRemoteCommandRC2Desc(result),
				  xml_request) ;

      *error_out = err_msg ;
    }
  else
    {
      result = REMOTE_VALIDATE_RC_OK ;
    }

  return result ;
}



/* Validate <zmap> element. */
static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser)
{
  gboolean result = TRUE ;
  ParseData validate_data = (ParseData)user_data ;
  char *err_msg = NULL ;

  validate_data->zmap_start = TRUE ;
  validate_data->zmap_content = TRUE ;

  if (result)
    result = checkAttribute(parser, zmap_element,"version", validate_data->version, &err_msg) ;

  if (result)
    result = checkAttribute(parser, zmap_element,"type", validate_data->message_type, &err_msg) ;

  if (result)
    result = checkAttribute(parser, zmap_element,"app_id", validate_data->app_id, &err_msg) ;

  if (result)
    result = checkAttribute(parser, zmap_element,"clipboard_id", validate_data->clipboard_id, &err_msg) ;

  if (result)
    result = checkAttribute(parser, zmap_element,"request_id", 0, &err_msg) ;

  if (!result)
    {
      zMapXMLParserRaiseParsingError(parser, err_msg) ;

      g_free(err_msg) ;

      validate_data->zmap_content = FALSE ;
    }

  return result ;
}


static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser)
{
  gboolean result = TRUE ;
  ParseData validate_data = (ParseData)user_data ;

  validate_data->zmap_end = TRUE ;

  return result ;
}


static gboolean xml_request_start_cb(gpointer user_data, ZMapXMLElement request_element, ZMapXMLParser parser)
{
  gboolean result = FALSE ;
  ParseData validate_data = (ParseData)user_data ;
  char *err_msg = NULL ;

  result = validate_data->req_reply_start = TRUE ;

  if ((validate_data->validate_command))
    {
      validate_data->req_reply_content = TRUE ;      

      if (!(result = checkAttribute(parser, request_element,"command", 0, &err_msg)))
	{
	  zMapXMLParserRaiseParsingError(parser, err_msg) ;

	  g_free(err_msg) ;

	  validate_data->req_reply_content = FALSE ;      
	}
    }

  return result ;
}

static gboolean xml_request_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser)
{
  gboolean result = FALSE ;
  ParseData validate_data = (ParseData)user_data ;

  result = validate_data->req_reply_end = TRUE ;

  return result ;
}


static gboolean xml_request_attrs_cb(gpointer user_data, ZMapXMLElement request_element, ZMapXMLParser parser)
{
  gboolean result = TRUE ;
  ParseData validate_data = (ParseData)user_data ;
  char *err_msg = NULL ;
  char *value = NULL ;
  GQuark envelope_str ;

  if (validate_data->envelope_type == ENVELOPE_REQUEST)
    envelope_str = g_quark_from_string(ZACP_REQUEST) ;
  else
    envelope_str = g_quark_from_string(ZACP_REPLY) ;


  /* Check that it's really the correct type of envelope. */
  if (result)
    {
      if ((result = checkAttribute(parser, request_element, ZACP_TYPE, envelope_str, &err_msg)))
	{
	  validate_data->message_type = envelope_str ;	    /* not really needed. */
	}
      else
	{
	  zMapXMLParserRaiseParsingError(parser, err_msg) ;

	  g_free(err_msg) ;

	  result = FALSE ;
	}
    }

  if (result)
    {
      if ((result = getAttribute(parser, request_element, ZACP_VERSION_ID, &value, &err_msg)))
	{
	  validate_data->version = g_quark_from_string(value) ;
	}
      else
	{
	  zMapXMLParserRaiseParsingError(parser, err_msg) ;

	  g_free(err_msg) ;

	  result = FALSE ;
	}
    }

  if (result)
    {
      if ((result = getAttribute(parser, request_element, ZACP_APP_ID, &value, &err_msg)))
	{
	  validate_data->app_id = g_quark_from_string(value) ;
	}
      else
	{
	  zMapXMLParserRaiseParsingError(parser, err_msg) ;

	  g_free(err_msg) ;

	  result = FALSE ;
	}
    }

  if (result)
    {
      if ((result = getAttribute(parser, request_element, ZACP_CLIPBOARD_ID, &value, &err_msg)))
	{
	  validate_data->clipboard_id = g_quark_from_string(value) ;
	}
      else
	{
	  zMapXMLParserRaiseParsingError(parser, err_msg) ;

	  g_free(err_msg) ;

	  result = FALSE ;
	}
    }

  if (result)
    {
      if ((result = getAttribute(parser, request_element, ZACP_REQUEST_ID, &value, &err_msg)))
	{
	  validate_data->request_id = value ;
	}
      else
	{
	  zMapXMLParserRaiseParsingError(parser, err_msg) ;

	  g_free(err_msg) ;

	  result = FALSE ;
	}
    }

  validate_data->req_reply_content = result ;

  return result ;
}



static gboolean xml_command_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser)
{
  gboolean result = FALSE ;
  ZMapXMLAttribute attr = NULL;
  ParseData command_data = (ParseData)user_data ;

  if (!(command_data->validate_command))
    {
      result = TRUE ;
    }
  else
    {
      if ((attr = zMapXMLElementGetAttributeByName(zmap_element, "command")) != NULL)
	{
	  GQuark type = 0 ;

	  type = zMapXMLAttributeGetValue(attr) ;

	  command_data->command = (char *)g_quark_to_string(type) ;

	  result = TRUE ;
	}
      else
	{
	  zMapXMLParserRaiseParsingError(parser, "\"command\" is a required attribute for the request element.") ;

	  result = FALSE ;
	}
    }

  command_data->req_reply_content = result ;

  return result ;
}


static gboolean xml_reply_attrs_cb(gpointer user_data, ZMapXMLElement request_element, ZMapXMLParser parser)
{
  gboolean result = TRUE ;
  ParseData validate_data = (ParseData)user_data ;
  char *err_msg = NULL ;
  char *value = NULL ;

  if (result)
    {
      if ((result = getAttribute(parser, request_element, ZACP_CMD, &value, &err_msg)))
	{
	  validate_data->command = value ;
	}
      else
	{
	  zMapXMLParserRaiseParsingError(parser, err_msg) ;

	  g_free(err_msg) ;

	  result = FALSE ;
	}
    }

  if (result)
    {
      if ((result = getAttribute(parser, request_element, ZACP_RETURN_CODE, &value, &err_msg)))
	{
	  validate_data->return_code = zMapRemoteCommandStr2RC(value) ;
	}
      else
	{
	  zMapXMLParserRaiseParsingError(parser, err_msg) ;

	  g_free(err_msg) ;

	  result = FALSE ;
	}
    }

  if (result)
    {
      /* if the command failed then get the reason, otherwise get the reply body but in the _end_
       * handler function, it's not available here. */
      if (validate_data->return_code != REMOTE_COMMAND_RC_OK)
	{
	  if ((result = getAttribute(parser, request_element, ZACP_REASON, &value, &err_msg)))
	    {
	      validate_data->reason = value ;
	    }
	  else
	    {
	      zMapXMLParserRaiseParsingError(parser, err_msg) ;

	      g_free(err_msg) ;

	      result = FALSE ;
	    }
	}
    }

  return result ;
}


static gboolean xml_reply_body_cb(gpointer user_data, ZMapXMLElement request_element, ZMapXMLParser parser)
{
  gboolean result = TRUE ;
  ParseData validate_data = (ParseData)user_data ;

  if (result)
    {
      /* if the command succeeded then get the reply body. */
      if (validate_data->return_code == REMOTE_COMMAND_RC_OK)
	{
	  char *reply_body ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  /* This does not work if what you we want are the nested tags within the reply
	   * tags....expat does not (correctly) include them in the reply tag "body"
	   * as they are tags in their own right. */
	  if ((reply_body = zMapXMLElementContentsToString(request_element)))
	    {
	      validate_data->reply_body = g_strdup(reply_body) ;
	    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  if ((reply_body = getReplyContents(validate_data->reply)))
	    {
	      validate_data->reply_body = g_strdup(reply_body) ;
	    }
	  else
	    {
	      char *err_msg ;

	      err_msg = g_strdup("no reply body.") ;
	      zMapXMLParserRaiseParsingError(parser, err_msg) ;
	      g_free(err_msg) ;

	      result = FALSE ;
	    }
	}
    }

  return result ;
}


static gboolean xmlGetAttrCB(gpointer user_data, ZMapXMLElement request_element, ZMapXMLParser parser)
{
  gboolean result = TRUE ;
  ParseSingleData attribute_data = (ParseSingleData)user_data ;
  char *err_msg = NULL ;
  char *value = NULL ;

  if (result)
    {
      if ((result = getAttribute(parser, request_element, attribute_data->attribute, &value, &err_msg)))
	{
	  attribute_data->attribute_value = value ;
	}
      else
	{
	  zMapXMLParserRaiseParsingError(parser, err_msg) ;

	  g_free(err_msg) ;

	  result = FALSE ;
	}
    }

  return result ;
}


/* Catch all routine, just returns true whatever data is gets. */
static gboolean xml_return_true_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser)
{
  gboolean result = TRUE ;

  return result ;
}










/* Returns TRUE if attribute is present in element and if an expected value is given
 * then checks that the attribute has that value otherwise returns FALSE and an
 * error message in error_out which should be g_free'd when finished with.
 * 
 * Probably needs widening to look at data type of attribute to handle ints etc... */
static gboolean checkAttribute(ZMapXMLParser parser, ZMapXMLElement element,
			       char *attribute, GQuark expected_value, char **error_out) 
{
  gboolean result = TRUE ;
  ZMapXMLAttribute attr ;

  if (!(attr = zMapXMLElementGetAttributeByName(element, attribute)))
    {
      result = FALSE ;
      *error_out = g_strdup_printf("<%s> \"%s\" is a required attribute.",
				   g_quark_to_string(element->name), attribute) ;
    }
  else if (expected_value)
    {
      GQuark value ;
      char *value_str ;
      char *exp_str ;

      value = zMapXMLAttributeGetValue(attr) ;

      value_str = (char *)g_quark_to_string(value) ;
      exp_str = (char *)g_quark_to_string(expected_value) ;

      if (value != expected_value)
	{
	  result = FALSE ;
	  *error_out = g_strdup_printf("<%s> \"%s=%s\" specified but value should be \"%s\".",
				       g_quark_to_string(element->name), attribute, value_str,
				       g_quark_to_string(expected_value)) ;
	}
    }

  return result ;
}




/* Returns TRUE if attribute is present in element and value can be returned in
 * value_out (note that string pointed to by value_out must _NOT_ be free'd),
 * returns FALSE otherwise and an error message in error_out which
 * should be g_free'd when finished with.
 * 
 * Probably needs widening to look at data type of attribute to handle ints etc... */
static gboolean getAttribute(ZMapXMLParser parser, ZMapXMLElement element,
			     char *attribute, char **value_out, char **error_out)
{
  gboolean result = TRUE ;
  ZMapXMLAttribute attr ;

  if (!(attr = zMapXMLElementGetAttributeByName(element, attribute)))
    {
      *error_out = g_strdup_printf("<%s> \"%s\" is a required attribute.",
				   g_quark_to_string(element->name), attribute) ;

      result = FALSE ;
    }
  else
    {
      GQuark value ;

      value = zMapXMLAttributeGetValue(attr) ;
      *value_out = (char *)g_quark_to_string(value) ;

      result = TRUE ;
    }

  return result ;
}


/* Takes a full zmap reply and returns all the characters between the reply
 * tags:
 * 
 * <zmap....>
 *  <reply....>
 *    RETURNS ALL THE STUFF IN HERE......
 *  </reply....>
 * </zmap>
 *
 * Returns NULL on error.
 *  */
static char *getReplyContents(char *reply)
{
  char *reply_contents = NULL ;
  char *start, *end ;
  size_t bytes ;

  /* Look for "<reply", then look for closing ">", then look for"</reply" and
   * copy all text in between. */
  if ((start = strstr(reply, "<reply"))
      && (start = strstr(start, ">"))
      && (end = strstr(start, "</reply")))
    {
      start++ ;
      end -- ;
      bytes = (end - start) + 1 ;
      reply_contents = g_strndup(start, bytes) ;
    }

  return reply_contents ;
}
