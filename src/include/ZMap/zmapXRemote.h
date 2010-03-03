/*  File: zmapXRemote.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Defines version, structure and response codes for
 *              the protocol for zmap <-> client program communications.
 *              
 *              See zmapXRemoteCmds.h for commands/attributes.
 *
 * Exported functions: See ZMap/zmapXRemote.h (this file)
 * HISTORY:
 * Last edited: Mar  2 10:44 2010 (edgrif)
 * Created: Wed Apr 13 19:02:52 2005 (rds)
 * CVS info:   $Id: zmapXRemote.h,v 1.24 2010-03-03 11:01:21 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAPXREMOTE_H
#define ZMAPXREMOTE_H 

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

/* These are here just to allow checking */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#define ZMAP_XREMOTE_CURRENT_VERSION      "$Revision: 1.24 $"
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#define ZMAP_XREMOTE_CURRENT_VERSION      "$Revision: 1.24 $"

#define ZMAP_XREMOTE_CURRENT_VERSION_ATOM "_ZMAP_XREMOTE_VERSION"
#define ZMAP_XREMOTE_APPLICATION_ATOM     "_ZMAP_XREMOTE_APP"
#define ZMAPXREMOTE_PING_COMMAND          "ping"

/* In HTTP the break between Header and body is \n\n, here we'll use : */
/* Everything before will be a status code (int), everything after response (str [xml]) */
/* e.g. "200:<xml><magnification>100</magnification></xml>" */

#define ZMAP_XREMOTE_STATUS_CONTENT_DELIMITER  ":"

/* ================================== */
#ifdef ZMAP_XREMOTE_WITHOUT_XML
#define ZMAP_XREMOTE_REPLY_FORMAT   "%d" ZMAP_XREMOTE_STATUS_CONTENT_DELIMITER "%s" 
#define ZMAP_XREMOTE_SUCCESS_FORMAT "%s"
#define ZMAP_XREMOTE_ERROR_START    "Error: "
#define ZMAP_XREMOTE_ERROR_END      ""
#define ZMAP_XREMOTE_CLIENT_FORMAT  "xwid = 0x%lx, request_atom = %s, response_atom = %s"
#define ZMAP_XREMOTE_META_FORMAT    "\n%s 0x%lx %s" ZMAP_XREMOTE_CURRENT_VERSION
/* ================================== */
#else
#define ZMAP_XREMOTE_REPLY_FORMAT   "%d" ZMAP_XREMOTE_STATUS_CONTENT_DELIMITER "<zmap>%s</zmap>" 
#define ZMAP_XREMOTE_SUCCESS_FORMAT "<response>%s</response>"
#define ZMAP_XREMOTE_ERROR_START    "<error><message>"
#define ZMAP_XREMOTE_ERROR_END      "</message></error>"
#define ZMAP_XREMOTE_CLIENT_FORMAT  \
"<client xwid=\"0x%lx\" request_atom=\"%s\" response_atom=\"%s\" />"
#define ZMAP_XREMOTE_META_FORMAT    \
"<meta display=\"%s\" windowid=\"0x%lx\" application=\"%s\" version=\"" ZMAP_XREMOTE_CURRENT_VERSION "\" />"
/* ================================== */
#endif /* ZMAP_XREMOTE_WITHOUT_XML */

#define ZMAP_XREMOTE_ERROR_FORMAT   ZMAP_XREMOTE_ERROR_START "%s" ZMAP_XREMOTE_ERROR_END

#define ZMAP_CLIENT_REQUEST_ATOM_NAME  "_CLIENT_REQUEST_NAME"
#define ZMAP_CLIENT_RESPONSE_ATOM_NAME "_CLIENT_RESPONSE_NAME"

/* Not sure these need to be public */
#define ZMAP_DEFAULT_REQUEST_ATOM_NAME  "_ZMAP_XREMOTE_COMMAND"
#define ZMAP_DEFAULT_RESPONSE_ATOM_NAME "_ZMAP_XREMOTE_RESPONSE"


#define ZMAPXREMOTE_CALLBACK(f)                    ((ZMapXRemoteCallback) (f))

typedef struct _ZMapXRemoteObjStruct  *ZMapXRemoteObj;

typedef char * (*ZMapXRemoteCallback) (char *command, gpointer user_data, int *statusCode);

typedef enum
  {
    ZMAPXREMOTE_SENDCOMMAND_SUCCEED          = 0,
    ZMAPXREMOTE_SENDCOMMAND_ISSERVER         = 1 << 0,
    ZMAPXREMOTE_SENDCOMMAND_INVALID_WINDOW   = 1 << 1,
    ZMAPXREMOTE_SENDCOMMAND_VERSION_MISMATCH = 1 << 2,
    ZMAPXREMOTE_SENDCOMMAND_APP_MISMATCH     = 1 << 3,
    ZMAPXREMOTE_SENDCOMMAND_PROPERTY_ERROR   = 1 << 4,
    ZMAPXREMOTE_SENDCOMMAND_TIMEOUT          = 1 << 5,
    ZMAPXREMOTE_SENDCOMMAND_UNAVAILABLE      = 1 << 6,
    ZMAPXREMOTE_SENDCOMMAND_UNKNOWN          = 1 << 15
  } ZMapXRemoteSendCommandError;

typedef enum {
  /* 1xx  Informational */

  /* 2xx  Successful    */
  ZMAPXREMOTE_OK         = 200, /* Everything went ok */
  ZMAPXREMOTE_METAERROR  = 203,
  ZMAPXREMOTE_NOCONTENT  = 204, /* Everything was good, but empty content was returned */

  /* 3xx  Redirection   */
  /* Redirect??? I don't think so */

  /* 4xx  Client Errors */
  ZMAPXREMOTE_BADREQUEST = 400, /* caller has supplied bad args. */
  ZMAPXREMOTE_FAILED     = 402,				    /* command failed. */
  ZMAPXREMOTE_FORBIDDEN  = 403, /* Just in case */
  ZMAPXREMOTE_UNKNOWNCMD = 404, /* no command by that name for the atom supplied. */

  ZMAPXREMOTE_CONFLICT   = 409, /* invlid data on atom */

  ZMAPXREMOTE_PRECOND    = 412, /* Precondition/SANITY CHECK FAILED */

  ZMAPXREMOTE_OUTOFRANGE = 416,
  ZMAPXREMTOE_UNEXPECTED = 417,

  /* 5xx  Server Errors  */
  ZMAPXREMOTE_INTERNAL    = 500, /* Internal Error */
  ZMAPXREMOTE_UNKNOWNATOM = 501, /* When the server doesn't have an atom by that name??? */
  ZMAPXREMOTE_NOCREATE    = 502, /* Could not create connection */
  ZMAPXREMOTE_UNAVAILABLE = 503, /* Unavailable No Window with that id */
  ZMAPXREMOTE_TIMEDOUT    = 504, /* Connection timed out. */

} ZMapXRemoteStatus ;

extern gboolean externalPerl;

/* ================================================ */
/* COMMON MODE METHODS */
/* ================================================ */

void zMapXRemoteSetDebug(gboolean debug_on) ;

ZMapXRemoteObj zMapXRemoteNew(void);     /* This just returns the object and checks XOpenDisplay(getenv(DISPLAY)) */

void zMapXRemoteSetRequestAtomName(ZMapXRemoteObj object, char *name); /* Better set in zMapXRemoteInitServer if Server though */
void zMapXRemoteSetResponseAtomName(ZMapXRemoteObj object, char *name); /* Ditto */
void zMapXRemoteSetWindowID(ZMapXRemoteObj object, unsigned long window_id); /* Ditto */
void zMapXRemoteSetTimeout(ZMapXRemoteObj object, double timeout_secs) ;

char *zMapXRemoteGetResponse(ZMapXRemoteObj object);
Window zMapXRemoteGetWindowID(ZMapXRemoteObj object) ;

void zMapXRemoteResponseSplit(ZMapXRemoteObj object, char *full_response, int *code, char **response);
gboolean zMapXRemoteResponseIsError(ZMapXRemoteObj object, char *response);
int zMapXRemoteIsPingCommand(char *command, int *statusCode, char **reply);

void zMapXRemoteDestroy(ZMapXRemoteObj object);
/* ================================================ */
/* CLIENT MODE ONLY METHODS */
/* ================================================ */
int zMapXRemoteInitClient(ZMapXRemoteObj object, Window id); /* Initialise Client */
int zMapXRemoteSendRemoteCommand(ZMapXRemoteObj object, char *command, char **response);

/* ================================================ */
/* SERVER MODE ONLY METHODS */
/* ================================================ */
int zMapXRemoteInitServer(ZMapXRemoteObj object, Window id, char *appName, char *requestName, char *responseName);
gint zMapXRemoteHandlePropertyNotify(ZMapXRemoteObj xremote, 
                                     Atom           event_atom, 
                                     guint          event_state, 
                                     ZMapXRemoteCallback callback, 
                                     gpointer       cb_data);

/* ================================================ */
/* METHODS TO HELP PERL INTEGRATION */
/* the perl stuff can't do these very easily so here they are */
/* ================================================ */
int zMapXRemoteSetReply(ZMapXRemoteObj object, char *content);
char *zMapXRemoteGetRequest(ZMapXRemoteObj object); /* */


/* Broken Methods */
#ifdef MULTIPLE_COMMANDS_DONT_WORK
int  zMapXRemoteSendRemoteCommands(ZMapXRemoteObj object);
#endif /* MULTIPLE_COMMANDS_DONT_WORK */




/* ================================================ */
/*
  # Some info about status codes
  # 100:<xml><info>this is info about something</info></xml>
  # 200:<xml><magnification>100</magnification></xml>
  # 30X:<xml><></error></xml>
  # 40X:<xml><error>you crazy fool</error></xml>
  # 50X:<xml><error>I'm a crazy fool</error></xml>

These are HTTP1.1 codes, which is what I've based the {1,2,3,4,5}00 codes on
1xx  Informational
100  CONTINUE - the client should continue with request.  
101  SWITCHING PROTOCOLS - the server will switch protocols as necessary.  

2xx  Successful  
200  OK - the request was fulfilled.  
201  CREATED - following a POST command.  
202  ACCEPTED - accepted for processing, but processing is not completed.  
203  NON-AUTHORITATIVE INFORMATION - the returned metainformation is not the definitive set from the original server.  
204  NO CONTENT - request received but no information exists to send back.  
205  RESET CONTENT - the server has fulfilled the request and the user agent should reset the document view.  
206  PARTIAL CONTENT - the server has fulfilled the partial GET request.  

3xx  Redirection  
300  MULTIPLE CHOICES - the requested resource has many representations.  
301  MOVED PERMANENTLY - the data requested has a new location and the change is permanent.  
302  FOUND - the data requested has a different URL temporarily.  
303  SEE OTHER - a suggestion for the client to try another location.  
304  NOT MODIFIED - the document has not been modified as expected.  
305  USE PROXY - The requested resource must be accessed through the specified proxy.  
306  UNUSED  
307  TEMPORARY REDIRECT - the requested data resides temporarily at a new location.  

4xx  Client Errors  
400  BAD REQUEST - syntax problem in the request or it could not be satisfied.  
401  UNAUTHORIZED - the client is not authorized to access data.  
402  PAYMENT REQUIRED - indicates a charging scheme is in effect.  
403  FORBIDDEN - access not required even with authorization. 
404  NOT FOUND - server could not find the given resource.  
405  METHOD NOT ALLOWED  
406  NOT ACCEPTABLE  
407  PROXY AUTHENTICATION REQUIRED - the client must first authenticate with the proxy for access.  
408  REQUEST TIMEOUT - the client did not produce a request within the time the server was prepared to wait.  
409  CONFLICT - the request could not be completed due to a conflict with the current state of the resource.  
410  GONE - the requested resource is no longer available.  
411  LENGTH REQUIRED - the server refused to accept the request without a defined Content Length.  
412  PRECONDITION FAILED  
413  REQUESTED ENTITY TOO LARGE - the server is refusing to process a request because it is larger than the server is willing or able to process.  
414  REQUEST-URI TOO LONG - the server is refusing to process a request because the URI is longer than the server is willing or able to process.  
415  UNSUPPORTED MEDIA TYPE - requested resource format is not supported.  
416  REQUESTED RANGE NOT SATISFIABLE  
417  EXPECTATION FAILED  

5xx  Server Errors  
500  INTERNAL ERROR - the server could not fulfill the request because of an unexpected condition.  
501  NOT IMPLEMENTED - the sever does not support the facility requested.  
502  BAD GATEWAY - received an invalid response from an upstream sever.  
503  SERVICE UNAVAILABLE - the server is currently unable to handle a request.  
504  GATEWAY TIMEOUT - The server, acting as a gateway/proxy, did not receive a timely response from an upstream server.  
505  HTTP VERSION NOT SUPPORTED  
*/    

#endif /* ZMAPXREMOTE_H */
