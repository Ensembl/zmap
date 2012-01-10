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

typedef enum {ENVELOPE_REQUEST, ENVELOPE_REPLY} EnvelopeType ;


/* Used by lots of the callback functions to return data from parsed xml. */
typedef struct ParseDataStructName
{
  char *message_type ;					    /* request or reply ? */

  /* Used for input and output of these fields according to function using this struct. */
  char *version ;
  char *peer_id ;
  char *request_id ;
  char *command ;

  gboolean valid ;					    /* If this is FALSE something failed. */

  /* These are used to check we actually find the elements. */
  gboolean zmap_start ;
  gboolean zmap_end ;
  gboolean req_reply_start ;
  gboolean req_reply_end ;
} ParseDataStruct, *ParseData ;


static GArray *createRequestReply(EnvelopeType type,
				  char *version, char *peer_id, char* request_id, int timeout_secs, char *command,
				  RemoteCommandRCType return_code, char *reason, char *result) ;
static gboolean getRequestAttrs(char *xml_request,
				char **req_version, char **req_peer_id, char **req_request_id, char **command,
				char **error_out) ;
static gboolean checkAttribute(ZMapXMLParser parser, ZMapXMLElement element,
			       char *attribute, char *expected_value, char **error_out) ;
static gboolean getAttribute(ZMapXMLParser parser, ZMapXMLElement element,
			     char *attribute, char **value_out, char **error_out) ;
static gboolean reqReplyValidate(char *msg_type, char *version, char *peer_id, char *xml_request, char **error_out) ;

static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;
static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;
static gboolean xml_request_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;
static gboolean xml_request_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;

static gboolean xml_request_attrs_cb(gpointer user_data, ZMapXMLElement request_element, ZMapXMLParser parser) ;

static gboolean xml_command_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;

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
    {"request", xml_command_start_cb},
    {NULL, NULL}
  };

static ZMapXMLObjTagFunctionsStruct get_attrs_ends_G[] =
  {
    { "request",    xml_return_true_cb    },
    { "zmap",    xml_return_true_cb    },
    {NULL, NULL}
  };




/* 
 *                External functions.
 */


/* Make a zmap xml request envelope:
 * 
 * <ZMap version="n.n" type="request" peer_id="xxxxx" request_id="yyy" [timeout="seconds2]>
 *   <request command="something_command">
 *   </request>
 * </zmap>
 * 
 * Add further content using zMapXMLUtilsAddStackToEventsArrayMiddle()
 * 
 * 
 *  */
GArray *zMapRemoteControlCreateRequest(char *peer_id, char *request_id, int timeout_secs, char *command)
{
  GArray *envelope = NULL ;

  envelope = createRequestReply(ENVELOPE_REQUEST, ZACP_VERSION, peer_id, request_id, timeout_secs, command,
				REMOTE_COMMAND_RC_OK, NULL, NULL) ;

  return envelope ;
}


/* Make a zmap xml reply envelope:
 * 
 * <ZMap version="n.n" type="reply" peer_id="xxxxx" request_id="yyy">
 *   <reply>
 *   </reply>
 * </zmap>
 * 
 * Add further content using zMapXMLUtilsAddStackToEventsArrayMiddle()
 * 
 * BUT NOTE, it's much safer to use zMapRemoteControlCreateReplyFromRequest()
 * as it will automatically make sure that the reply envelope
 * is correct for the given request.
 * 
 *  */
GArray *zMapRemoteControlCreateReply(char *peer_id, char *request_id)
{
  GArray *envelope = NULL ;

  envelope = createRequestReply(ENVELOPE_REPLY, ZACP_VERSION, peer_id, request_id, -1, NULL,
				REMOTE_COMMAND_RC_OK, NULL, NULL) ;

  return envelope ;
}



GArray *zMapRemoteControlCreateReplyFromRequest(char *xml_request,
						RemoteCommandRCType return_code, char *reason, char *result,
						char **error_out)
{
  GArray *envelope = NULL ;
  char *err_msg = NULL ;
  char *req_version ;
  char *req_peer_id ;
  char *req_request_id ;
  char *req_command ;


  if (!reqReplyValidate(ZACP_REQUEST, ZACP_VERSION, NULL, xml_request, &err_msg))
    {
      *error_out = err_msg ;
    }
  else
    {
      getRequestAttrs(xml_request, &req_version, &req_peer_id, &req_request_id, &req_command, &err_msg) ;


      if ((return_code == REMOTE_COMMAND_RC_OK && !reason) || (return_code != REMOTE_COMMAND_RC_OK && reason))
	envelope = createRequestReply(ENVELOPE_REPLY, req_version, req_peer_id, req_request_id, -1, req_command,
				      return_code, reason, result) ;
      else
	*error_out = g_strdup_printf("Bad args: %s.",
				     (return_code == REMOTE_COMMAND_RC_OK
				      ? "command succeeded but an error string was given"
				      : "command failed but no error string was given")) ;
    }

  return envelope ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* We need a call to add the return code and the result of the command...... */
/* 
 * 
 * If return_code == REMOTE_COMMAND_RC_OK then reason == NULL.
 * 
 *  */
GArray *zMapRemoteControlReplyAddResult(GArray *reply_envelope,
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
      reply_start[1].value.q = g_quark_from_string(zMapRemoteControlRC(return_code)) ;


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






/* I think we need functions here to validate requests & replies...I thought remotecontrol might
 * do that but it's the wrong place I think....
 * 
 * validating replies should be against a unique id and a request id.
 * 
 * 
 * be good also to have a function that given a valid xml request produces a valid xml
 * reply envelope.
 * 
 * 
 * 
 *  */
gboolean zMapRemoteControlValidateRequest(char *peer_id, char *xml_request, char **error_out)
{
  gboolean result = FALSE ;

  result = reqReplyValidate(ZACP_REQUEST, ZACP_VERSION, peer_id, xml_request, error_out) ;

  return result ;
}


gboolean zMapRemoteControlValidateReply(char *peer_id, char *xml_request, char **error_out)
{
  gboolean result = FALSE ;

  result = reqReplyValidate(ZACP_REPLY, ZACP_VERSION, peer_id, xml_request, error_out) ;

  return result ;
}



/* Returns TRUE if the given request is for the given command, FALSE otherwise. */
gboolean zMapRemoteControlRequestIsCommand(char *request, char *command)
{
  gboolean result = FALSE ;
  ParseDataStruct command_data = {NULL, NULL, NULL, NULL, NULL, TRUE} ;
  ZMapXMLParser parser;

  parser = zMapXMLParserCreate(&command_data, FALSE, FALSE) ;

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &command_starts_G[0], &command_ends_G[0]) ;

  if ((zMapXMLParserParseBuffer(parser, request, strlen(request))) == TRUE)
    {
      if (!(command_data.valid))
	{
	  if (g_ascii_strcasecmp(command_data.command, command) == 0)
	    result = TRUE ;
	}
    }

  return result ;
}


/* Returns command attribute value or NULL if there is some error (e.g. bad xml). */
char *zMapRemoteControlRequestGetCommand(char *request)
{
  char *command = NULL ;
  ParseDataStruct command_data = {NULL, NULL, NULL, NULL, NULL, TRUE} ;
  ZMapXMLParser parser;

  parser = zMapXMLParserCreate(&command_data, FALSE, FALSE) ;

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &command_starts_G[0], &command_ends_G[0]) ;

  if ((zMapXMLParserParseBuffer(parser, request, strlen(request))) == TRUE)
    {
      if (!(command_data.valid))
	command = command_data.command ;
    }

  return command ;
}


/* Auto "function from macro list" definition, returns short text version of RemoteCommandRCType value,
 * i.e. given REMOTE_COMMAND_RC_BAD_XML, returns "bad_xml" */
ZMAP_ENUM_AS_NAME_STRING_FUNC(zMapRemoteControlRC, RemoteCommandRCType, REMOTE_COMMAND_RC_LIST) ;




/* 
 *                Internal functions.
 */

/* Creates a request or reply according to the envelope type.
 * 
 * The last three args are for replies only, may need to rationalise args at some time.
 *  */
static GArray *createRequestReply(EnvelopeType type,
				  char *version, char *peer_id, char *request_id, int timeout_secs,
				  char *command,
				  RemoteCommandRCType return_code, char *reason, char *result)
{
  GArray *envelope = NULL ;
  static ZMapXMLUtilsEventStackStruct
    envelope_start[] =
    {
      {ZMAPXML_START_ELEMENT_EVENT, ZACP_TAG,       ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "version",      ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "type",         ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "peer_id",      ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "request_id",   ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_NULL_EVENT}
    },
    timeout_attr[] =
      {
	{ZMAPXML_ATTRIBUTE_EVENT,     "timeout",   ZMAPXML_EVENT_DATA_INTEGER, {0}},
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
	{ZMAPXML_ATTRIBUTE_EVENT, ZACP_REASON, ZMAPXML_EVENT_DATA_INTEGER, {0}},
	{ZMAPXML_NULL_EVENT}
      },
    reply_body[] =
      {
	{ZMAPXML_CHAR_DATA_EVENT, "", ZMAPXML_EVENT_DATA_STRING, {0}},
	{ZMAPXML_NULL_EVENT}
      },
    envelope_end[] =
      {
	{ZMAPXML_END_ELEMENT_EVENT, "",        ZMAPXML_EVENT_DATA_NONE,  {0}},
	{ZMAPXML_END_ELEMENT_EVENT, ZACP_TAG,  ZMAPXML_EVENT_DATA_NONE,  {0}},
	{ZMAPXML_NULL_EVENT}
      } ;
  char *envelope_type ;

  if (type == ENVELOPE_REQUEST)
    envelope_type = ZACP_REQUEST ;
  else
    envelope_type = ZACP_REPLY ;


  /* Create the envelope start. */
  envelope_start[1].value.q = g_quark_from_string(version) ;
  envelope_start[2].value.q = g_quark_from_string(envelope_type) ;
  envelope_start[3].value.q = g_quark_from_string(peer_id) ;
  envelope_start[4].value.q = g_quark_from_string(request_id) ;

  envelope = zMapXMLUtilsStackToEventsArray(&envelope_start[0]) ;

  /* Possibly add timeout for requests. */
  if (type == ENVELOPE_REQUEST && timeout_secs >= 0)
    {
      timeout_attr[0].value.i = timeout_secs ;

      envelope = zMapXMLUtilsAddStackToEventsArrayEnd(envelope, &timeout_attr[0]) ;
    }

  /* Add the request or reply start. */
  request_reply_start[0].name = envelope_type ;
  request_reply_start[1].value.q = g_quark_from_string(command) ;

  envelope = zMapXMLUtilsAddStackToEventsArrayEnd(envelope, &request_reply_start[0]) ;


  /* If it's a reply then add return code and either optional reply if command worked or reason if
   * it failed. */
  if (type == ENVELOPE_REPLY)
    {
      /* Fill in reply attributes. */
      reply_return_code[0].value.q = g_quark_from_string(zMapRemoteControlRC(return_code)) ;

      envelope = zMapXMLUtilsAddStackToEventsArrayEnd(envelope, &reply_return_code[0]) ;

      if (return_code == REMOTE_COMMAND_RC_OK)
	{
	  if (result)
	    {
	      reply_body[0].value.s = g_strdup(result) ;

	      envelope = zMapXMLUtilsAddStackToEventsArrayEnd(envelope, &reply_body[0]) ;
	    }
	}
      else
	{
	  reply_reason_attr[0].value.q = g_quark_from_string(reason) ;

	  envelope = zMapXMLUtilsAddStackToEventsArrayEnd(envelope, &reply_reason_attr[0]) ;
	}
    }

  /* Add the envelope end. */
  envelope_end[0].name = envelope_type ;

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



static gboolean getRequestAttrs(char *xml_request,
				char **req_version, char **req_peer_id, char **req_request_id, char **req_command,
				char **error_out)
{
  gboolean result = FALSE ;
  ParseDataStruct validate_data = {NULL, NULL, NULL, NULL, NULL, TRUE} ;
  ZMapXMLParser parser;

  parser = zMapXMLParserCreate(&validate_data, FALSE, FALSE) ;

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &get_attrs_starts_G[0], &get_attrs_ends_G[0]) ;

  if (!(result = zMapXMLParserParseBuffer(parser, xml_request, strlen(xml_request))))
    {
      *error_out = zMapXMLParserLastErrorMsg(parser) ;
    }
  else
    {
      *req_version = validate_data.version ;
      *req_peer_id = validate_data.peer_id ;
      *req_request_id = validate_data.request_id ;
      *req_command = validate_data.command ;

      result = TRUE ;
    }

  return result ;
}





static gboolean reqReplyValidate(char *msg_type, char *version, char *peer_id, char *xml_request, char **error_out)
{
  gboolean result = FALSE ;
  ParseDataStruct validate_data = {NULL, NULL, NULL, NULL, NULL, TRUE} ;
  ZMapXMLParser parser ;

  validate_data.message_type = msg_type ;
  validate_data.version = version ;
  validate_data.peer_id = peer_id ;

  parser = zMapXMLParserCreate(&validate_data, FALSE, FALSE) ;

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &validate_starts_G[0], &validate_ends_G[0]) ;

  if (!(result = zMapXMLParserParseBuffer(parser, xml_request, strlen(xml_request))))
    {
      *error_out = zMapXMLParserLastErrorMsg(parser) ;
    }
  else
    {
      /* parsing successful but check all elements parsed. */
      if (!validate_data.zmap_start || !validate_data.zmap_end
	  || !validate_data.req_reply_start || !validate_data.req_reply_end)
	{
	  *error_out = g_strdup_printf("<%s%s> element missing from %s.",
				       ((!validate_data.zmap_end || !validate_data.req_reply_end) ? "/" : ""),
				       ((!validate_data.zmap_start || !validate_data.zmap_end) ? ZACP_TAG : msg_type),
				       msg_type) ;
	  result = FALSE ;
	}
      else
	{
	  result = TRUE ;
	}
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

  if (result)
    result = checkAttribute(parser, zmap_element,"version", validate_data->version, &err_msg) ;

  if (result)
    result = checkAttribute(parser, zmap_element,"type", validate_data->message_type, &err_msg) ;


  if (result)
    result = checkAttribute(parser, zmap_element,"peer_id", validate_data->peer_id, &err_msg) ;

  if (result)
    result = checkAttribute(parser, zmap_element,"request_id", NULL, &err_msg) ;

  if (!result)
    {
      zMapXMLParserRaiseParsingError(parser, err_msg) ;
      g_free(err_msg) ;
    }

  validate_data->valid = result ;

  return result ;
}


static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser)
{
  gboolean result = FALSE ;
  ParseData validate_data = (ParseData)user_data ;

  validate_data->zmap_end = TRUE ;

  validate_data->valid = result ;

  return result ;
}


static gboolean xml_request_start_cb(gpointer user_data, ZMapXMLElement request_element, ZMapXMLParser parser)
{
  gboolean result = FALSE ;
  ParseData validate_data = (ParseData)user_data ;
  char *err_msg = NULL ;

  validate_data->req_reply_start = TRUE ;

  result = checkAttribute(parser, request_element,"command", NULL, &err_msg) ;

  if (!result)
    {
      zMapXMLParserRaiseParsingError(parser, err_msg) ;
      g_free(err_msg) ;
    }

  validate_data->valid = result ;

  return result ;
}

static gboolean xml_request_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser)
{
  gboolean result = FALSE ;
  ParseData validate_data = (ParseData)user_data ;

  validate_data->req_reply_end = TRUE ;

  validate_data->valid = result ;

  return result ;
}


static gboolean xml_request_attrs_cb(gpointer user_data, ZMapXMLElement request_element, ZMapXMLParser parser)
{
  gboolean result = TRUE ;
  ParseData validate_data = (ParseData)user_data ;
  char *err_msg = NULL ;
  char *value = NULL ;

  /* Check that it's really a request. */
  if (result && (result = checkAttribute(parser, request_element, ZACP_TYPE, ZACP_REQUEST, &err_msg)))
    {
      validate_data->message_type = ZACP_REQUEST ;	    /* not really needed. */
    }
  else
    {
      zMapXMLParserRaiseParsingError(parser, err_msg) ;
      g_free(err_msg) ;

      result = FALSE ;
    }


  if (result && (result = getAttribute(parser, request_element, ZACP_VERSION_ID, &value, &err_msg)))
    {
      validate_data->version = value ;
    }
  else
    {
      zMapXMLParserRaiseParsingError(parser, err_msg) ;
      g_free(err_msg) ;

      result = FALSE ;
    }

  if (result && (result = getAttribute(parser, request_element, ZACP_PEER_ID, &value, &err_msg)))
    {
      validate_data->peer_id = value ;
    }
  else
    {
      zMapXMLParserRaiseParsingError(parser, err_msg) ;
      g_free(err_msg) ;

      result = FALSE ;
    }

  if (result && (result = getAttribute(parser, request_element, ZACP_REQUEST_ID, &value, &err_msg)))
    {
      validate_data->request_id = value ;
    }
  else
    {
      zMapXMLParserRaiseParsingError(parser, err_msg) ;
      g_free(err_msg) ;

      result = FALSE ;
    }


  /* IS THIS NEEDED......... */
  validate_data->valid = result ;

  return result ;
}



static gboolean xml_command_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser)
{
  gboolean result = FALSE ;
  ZMapXMLAttribute attr = NULL;
  ParseData command_data = (ParseData)user_data ;

  if ((attr = zMapXMLElementGetAttributeByName(zmap_element, "command")) != NULL)
    {
      GQuark type = 0 ;

      type = zMapXMLAttributeGetValue(attr) ;

      command_data->command = (char *)g_quark_to_string(type) ;
      command_data->valid = FALSE ;

      result = TRUE ;
    }
  else
    {
      zMapXMLParserRaiseParsingError(parser, "\"command\" is a required attribute for the request element.") ;
      command_data->valid = TRUE ;

      result = FALSE ;
    }

  return result ;
}


static gboolean xml_return_true_cb(gpointer user_data,
                                   ZMapXMLElement zmap_element,
                                   ZMapXMLParser parser)
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
			       char *attribute, char *expected_value, char **error_out) 
{
  gboolean result = TRUE ;
  ZMapXMLAttribute attr ;

  if (!(attr = zMapXMLElementGetAttributeByName(element, attribute)))
    {
      result = FALSE ;
      *error_out = g_strdup_printf("<%s> \"%s\" is a required attribute.",
				   g_quark_to_string(element->name), attribute) ;
    }
  else if (expected_value && *expected_value)
    {
      GQuark value ;
      char *value_str ;

      value = zMapXMLAttributeGetValue(attr) ;
      value_str = (char *)g_quark_to_string(value) ;

      if (strcmp(value_str, expected_value) != 0)
	{
	  result = FALSE ;
	  *error_out = g_strdup_printf("<%s> \"%s=%s\" specified but value should be \"%s\".",
				       g_quark_to_string(element->name), attribute, value_str, expected_value) ;
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

