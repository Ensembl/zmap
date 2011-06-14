/*  File: zmapXRemote.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implements the essential code for allows external client
 *              programs to communicate with ZMap. This same code is
 *              compiled both into ZMap and into a perl xls module for
 *              use by otterlace.
 *
 * Exported functions: See ZMap/zmapXRemote.h
 * HISTORY:
 * Last edited: Jun 14 15:13 2011 (edgrif)
 * Created: Wed Apr 13 19:04:48 2005 (rds)
 * CVS info:   $Id: zmapXRemote.c,v 1.54 2010-09-16 11:57:40 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include "zmapXRemote_P.h"



enum
  {
    XGETWINDOWPROPERTY_FAILURE = 1 << 0,
    NO_MATCHING_PROPERTY_TYPE  = 1 << 1,
    UNEXPECTED_FORMAT_SIZE     = 1 << 2,
    XGETWINDOWPROPERTY_ERROR   = 1 << 3
  };

typedef struct
{
  Time time;
  Bool set;
} PredicateDataStruct, *PredicateData;



/* Use this macro for _ALL_ logging calls in this code.
 *
 * If externalPerl is TRUE then logging messages are sent to stderr
 * otherwise the caller is expected to have initialised the zmap
 * logging system and that is called to output the message.
 *
 *  */
#define REMOTELOGMSG(SEVERITY, FORMAT_STR, ...)	     \
  do                                                 \
    {	\
      if(debug_G)			                     \
      { \
        if (externalPerl)                              \
	    fprintf(stderr, FORMAT_STR, __VA_ARGS__) ;   \
        else                                           \
	    zMapLog##SEVERITY(FORMAT_STR, __VA_ARGS__) ; \
     } \
    } while (0)




static Bool process_property_notify(ZMapXRemoteObj object,
				    PredicateData  predicate,
				    XEvent        *event_in,
				    char         **response);

static char *zmapXRemoteGetAtomName(Display *display, Atom atom);

static ZMapXRemoteSendCommandError zmapXRemoteCheckWindow(ZMapXRemoteObj object) ;
static ZMapXRemoteSendCommandError zmapXRemoteCmpAtomString(ZMapXRemoteObj object, Atom atom, char *expected) ;
static ZMapXRemoteSendCommandError zmapXRemoteChangeProperty(ZMapXRemoteObj object, Atom atom, char *change_to) ;

char *zmapXRemoteProcessForReply(ZMapXRemoteObj object, int statusCode, char *cb_output);

static gboolean zmapXRemoteGetPropertyFullString(Display *display,
                                                 Window   window,
                                                 Atom     xproperty,
                                                 gboolean delete, int *size,
                                                 char **full_string_out,
                                                 GError **error_out);

static void zmapXRemoteLock() ;
static void zmapXRemoteUnLock() ;
static gboolean zmapXRemoteIsLocked() ;



static XErrorHandler stored_xerror_handler(XErrorHandler e, gboolean store, gboolean clear) ;



/* An external global which controls the two modes of the code
 *
 * - compiled into ZMap
 *
 * - compiled into a Perl xls module for otterlace to use.
 *
 */
gboolean externalPerl = TRUE ;



/* Controls debugging output. */
static gboolean debug_G = FALSE ;
static char *trap_txt_G = NULL ;


/* Locking to stop deadlocks, Not thread safe! - But since this is the GUI threads are not a problem. */
static gboolean xremote_lock_G = FALSE ;


/* Used in detecting X windows call errors... */
static Bool window_error_G = False ;




/* 
 *                 External routines
 */



/* Can be called at any time and controls debugging for all instances of ZMapXRemoteObj,
 * TRUE turns debugging on and FALSE turns it off. */
void zMapXRemoteSetDebug(gboolean debug_on)
{
  debug_G = debug_on ;

  return ;
}




/*!
 * Create a ZMapXRemoteObj handle for later use
 *
 * Currently we don't know how to get the Display pointer
 * from Perl/Tk so when this code is called from otterlace
 * (see src/perl/X11-XRemote-0.01/XRemote.xs)
 * curr_display will be NULL so we will need to do the previous
 * trick of opening a new connection to the display just to
 * get a display pointer.
 *
 * The code however will continue to work if one day we find a
 * a way to get the display and pass it through BUT note that
 * the assert will need to be changed !
 *
 * returns a newly allocated object or NULL if no Display.
 */
ZMapXRemoteObj zMapXRemoteNew(Display *curr_display)
{
  ZMapXRemoteObj object = NULL ;

  zMapAssert((!externalPerl && (curr_display))
	     || (externalPerl && (!curr_display))) ;

  if (externalPerl)
    {
      char *env_string = NULL ;

      /* Get current display, open it to check it's valid
	 and store in struct to save having to do it again */
      if ((env_string = getenv("DISPLAY")))
	{
	  REMOTELOGMSG(Warning,"Using DISPLAY: %s\n", env_string) ;

	  if ((curr_display = XOpenDisplay (env_string)) == NULL)
	    {
	      REMOTELOGMSG(Warning,"Failed to open display '%s'\n", env_string) ;

	      zmapXRemoteSetErrMsg(ZMAPXREMOTE_PRECOND,
				   ZMAP_XREMOTE_META_FORMAT
				   ZMAP_XREMOTE_ERROR_FORMAT,
				   env_string, 0, "", "Failed to open display") ;
	    }
	}
    }

  if (curr_display)
    {
      object = g_new0(ZMapXRemoteObjStruct,1) ;
      object->display = curr_display ;
      object->timeout = 30.0 ;
    }

  return object ;
}


/*!
 * Free allocated resources of and including the ZMapXRemoteObj handle.
 *
 * This code used to do XDeleteProperty() calls for all the properties
 * set by the code for a window but this DOESN'T WORK because by the
 * time we are called the window may already be gone (e.g. if widget
 * is reparented as when user splits window) and then the calls fail
 * with a BadWindow error. In fact there is no need to do the call
 * because the properties get deleted automatically when the window goes.
 *
 * @param  The handle to free
 * @return  void
 */
void zMapXRemoteDestroy(ZMapXRemoteObj object)
{
  REMOTELOGMSG(Warning,"%s id: 0x%lx\n", "Destroying object", object->window_id);

  if (object->remote_app)
    g_free(object->remote_app);

  g_free(object);

  return ;
}



/*!
 * \brief Set the X11 Window ID for the handle
 *
 * @param   The handle
 * @param   The Window
 * @return  nothing
 */
void zMapXRemoteSetWindowID(ZMapXRemoteObj object, Window id)
{
  object->window_id = id;

  return;
}


/*!
 * \brief Set time (in seconds) to wait for a response before returning a timeout.
 * If time is less than or equal to zero this means never timeout.
 *
 * @param   The handle
 * @param   timeout seconds
 * @return  nothing
 */
void zMapXRemoteSetTimeout(ZMapXRemoteObj object, double timeout_secs)
{
  object->timeout = timeout_secs ;

  return ;
}


/*!
 * \brief Set the Atom name used to pass the "request"
 *
 * @param  The handle
 * @param  The atom name
 *
 * @return nothing
 */
void zMapXRemoteSetRequestAtomName(ZMapXRemoteObj object, char *name)
{
  char *atom_name = NULL;

  object->request_atom = XInternAtom(object->display, name, False);

  if (!(atom_name = zmapXRemoteGetAtomName(object->display, object->request_atom)))
    {
      REMOTELOGMSG(Fatal, "Unable to set and get atom '%s'. Possible X Server problem.", name) ;
    }
  else
    {
      REMOTELOGMSG(Message,"Set/Check of Request atom %s successful.", atom_name);

      g_free(atom_name);
    }

  return ;
}


/*!
 * \brief Set the Atom name used to receive the "response"
 *
 * @param  The handle
 * @param  The atom name
 *
 * @return nothing
 */
void zMapXRemoteSetResponseAtomName(ZMapXRemoteObj object, char *name)
{
  char *atom_name = NULL;

  object->response_atom = XInternAtom(object->display, name, False);

  if(!(atom_name = zmapXRemoteGetAtomName(object->display, object->response_atom)))
    {
      REMOTELOGMSG(Fatal, "Unable to set and get atom '%s'. Possible X Server problem.", name) ;
    }
  else
    {
      REMOTELOGMSG(Message,"Set/Check of Response atom %s successful.", atom_name);
      g_free(atom_name);
    }

  return ;
}

/*!
 * \brief Initialise a handle as a client. That is one that can make requests to a "server"
 *
 * @param  The handle
 * @param  The Window to use in communications
 *
 * @return currently nothing of meaning...
 */
int zMapXRemoteInitClient(ZMapXRemoteObj object, Window id)
{

  if(object->init_called == TRUE)
    return 1;

  zMapXRemoteSetWindowID(object, id);

  if (!object->request_atom)
    zMapXRemoteSetRequestAtomName(object, ZMAP_DEFAULT_REQUEST_ATOM_NAME);

  if (!object->response_atom)
    zMapXRemoteSetResponseAtomName(object, ZMAP_DEFAULT_RESPONSE_ATOM_NAME);

  if (!object->version_sanity_atom)
    object->version_sanity_atom = XInternAtom (object->display, ZMAP_XREMOTE_CURRENT_VERSION_ATOM, False);

  if (!object->app_sanity_atom)
    object->app_sanity_atom = XInternAtom(object->display, ZMAP_XREMOTE_APPLICATION_ATOM, False);

  object->init_called = TRUE;

  return 1 ;
}


/*!
 * \brief Initialise a handle as a server.  That is one that will respond to requests.
 *
 * @param  The handle
 * @param  The Window to use in communications
 * @param  The application name (for samity checking)
 * @param  The request atom name
 * @param  The response atom name
 *
 * @return currently nothing of meaning...
 *
 * NOTE that when this code is compiled into zmap the "Fatal" log messages will cause
 * zmap to terminate, _BUT_ when the code is compiled into some other application the
 * code does not exit for these messages, hence the use of the status flag.
 *
 *  */
int zMapXRemoteInitServer(ZMapXRemoteObj object,  Window id, char *appName, char *requestName, char *responseName)
{
  int result = 1 ;					    /* Currently 1 is always returend. */
  gboolean status = TRUE ;

  if (object->init_called == TRUE)
    return result ;

  zMapXRemoteSetWindowID(object, id);

  if (!object->request_atom && requestName)
      zMapXRemoteSetRequestAtomName(object, requestName);

  if (!object->response_atom && responseName)
      zMapXRemoteSetResponseAtomName(object, responseName);

  if (status && !object->version_sanity_atom)
    {
      char *atom_name = NULL;
      object->version_sanity_atom = XInternAtom (object->display, ZMAP_XREMOTE_CURRENT_VERSION_ATOM, False);

      if (!(atom_name = zmapXRemoteGetAtomName(object->display, object->version_sanity_atom)))
	{
	  REMOTELOGMSG(Fatal, "Unable to set and get atom '%s'. Possible X Server problem.",
		       ZMAP_XREMOTE_CURRENT_VERSION_ATOM) ;
	  status = FALSE ;
	}
      else
	{
	  g_free(atom_name);
	}

      if (status)
	{
	  if (zmapXRemoteChangeProperty(object, object->version_sanity_atom, ZMAP_XREMOTE_CURRENT_VERSION))
	    REMOTELOGMSG(Fatal, "Unable to change atom '%s'. Possible X Server problem.",
			 ZMAP_XREMOTE_CURRENT_VERSION_ATOM);
	}
    }

  if (status && !object->app_sanity_atom)
    {
      char *atom_name = NULL;
      object->app_sanity_atom = XInternAtom(object->display, ZMAP_XREMOTE_APPLICATION_ATOM, False);

      if(!(atom_name = zmapXRemoteGetAtomName(object->display, object->app_sanity_atom)))
	{
	  REMOTELOGMSG(Fatal, "Unable to set and get atom '%s'. Possible X Server problem.",
		       ZMAP_XREMOTE_APPLICATION_ATOM);
	  status = FALSE ;
	}
      else
	{
	  g_free(atom_name);
	}

      if (status)
	{
	  if(zmapXRemoteChangeProperty(object, object->app_sanity_atom, appName))
	    REMOTELOGMSG(Fatal, "Unable to change atom '%s'. Possible X Server problem.",
			 ZMAP_XREMOTE_APPLICATION_ATOM);
	}
    }

  if (status)
    {
      object->is_server   = TRUE;
      object->init_called = TRUE;
    }

  return result ;
}


int zMapXRemoteSendRemoteCommands(ZMapXRemoteObj object)
{
  int response_status = 0;

  REMOTELOGMSG(Warning,"\n trying %s \n", "a broken function...");

  return response_status;
}

/*=======================================================================*/
/* zMapXRemoteSendRemoteCommand is based on the original file 'remote.c'
 * which carried this notice : */

/* remote.c --- remote control of Netscape Navigator for Unix.
 * version 1.1.3, for Netscape Navigator 1.1 and newer.
 *
 * Copyright (c) 1996 Netscape Communications Corporation, all rights reserved.
 * Created: Jamie Zawinski <jwz@netscape.com>, 24-Dec-94.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 *************
 * NOTE:
 * the file has been modified, to use a different protocol from the one
 * described in http://home.netscape.com/newsref/std/x-remote.html
 *************
 */

/*!
 * \brief Send a command to a remote window.
 *
 * @param object   The ZMapXRemoteObj describing how to communicate with window
 * @param command  The message to send
 * @param response The response. This will ONLY be valid if return == ZMAPXREMOTE_SENDCOMMAND_SUCCEED
 *
 * @return int     0 on success other random values otherwise. (this bit needs work)
 */
int zMapXRemoteSendRemoteCommand(ZMapXRemoteObj object, char *command, char **response)
{
  GTimer *timeout_timer = NULL ;
  double elapsed ;
  unsigned long event_mask = (PropertyChangeMask | StructureNotifyMask);
  gulong ignore;
  ZMapXRemoteSendCommandError result = ZMAPXREMOTE_SENDCOMMAND_SUCCEED;
  PredicateDataStruct send_time = {0};
  Bool isDone = False;
  gboolean check_names = TRUE;
  char *atom_name = "";

  REMOTELOGMSG(Warning, "[XRemote] send command: %s",command);

  if (!object->init_called)
    REMOTELOGMSG(Critical, "%s", "XRemote object is not initialised !!") ;

  if (object->timeout > 0.0)
    {
      timeout_timer = g_timer_new();
      g_timer_start(timeout_timer);
    }

  if (response)
    *response = NULL;

  if (object->is_server == TRUE)
    {
      result |= ZMAPXREMOTE_SENDCOMMAND_ISSERVER;
      goto DONE;
    }

  zmapXRemoteResetErrMsg();

  result |= zmapXRemoteCheckWindow(object);

  if (result != 0)
    {
      if(window_error_G)
        result = ZMAPXREMOTE_SENDCOMMAND_INVALID_WINDOW;
      else
        result = ZMAPXREMOTE_SENDCOMMAND_VERSION_MISMATCH;
      goto DONE;
    }

  if(zmapXRemoteIsLocked())
    {
      result = ZMAPXREMOTE_SENDCOMMAND_UNAVAILABLE;
      zmapXRemoteSetErrMsg(ZMAPXREMOTE_UNAVAILABLE,
                           "Avoiding Deadlock on Window ID 0x%lx", object->window_id);
      goto DONE;
    }

  if (check_names)
    {

      if (!(atom_name = zmapXRemoteGetAtomName(object->display, object->request_atom)))
	{
	  REMOTELOGMSG(Fatal, "Unable to set and get atom 0x%lx. Possible X Server problem.", object->request_atom) ;
	}
      else
 	{
	  REMOTELOGMSG(Warning,"Request atom %s on window 0x%lx is fine.",
		       atom_name, (unsigned long)object->window_id) ;

	  g_free(atom_name) ;
	}
    }


  XSelectInput(object->display, object->window_id, event_mask);

  REMOTELOGMSG(Warning,"Sending command using atom %s on window 0x%lx: \"%s\"",
	       atom_name, (unsigned long)object->window_id, command) ;

  if ((result = zmapXRemoteChangeProperty(object, object->request_atom, command)))
    {
      REMOTELOGMSG(Warning,"Command send using atom %s on window 0x%lx successful",
		   atom_name, (unsigned long)object->window_id) ;
    }


  if (window_error_G)
    {
      result = ZMAPXREMOTE_SENDCOMMAND_INVALID_WINDOW ;
      goto DONE;
    }



  /* WARNING: THIS EVENT LOOP DOES NOT WORK SAFELY, IT BASICALLY BLOCKS UNTIL IT GETS THE EVENT
   * IT WANTS _BUT_ WHILE DOING THIS IT IS REMOVING EVENTS FROM THE EVENT QUEUE (using
   * XNextEvent() WHICH MAY BE WANTED BY OTHER PARTS OF ZMAP. THE LASSO CODE IN ZMAPWINDOW
   * WAS BROKEN BY THIS BUT I FIXED IT BY NOT CALLING THIS CODE UNTIL THE LASSO BUTTON
   * IS RELEASED.
   *
   * THIS CODE NEEDS TO BE REWRITTEN TO PEEK AT EVENTS AND DISPATCH THEM FOR PROCESSING IF THEY
   * ARE NOT THE ONES WE WANT SO THAT OTHER CALLBACKS IN ZMAP GET A CHANCE TO USE THEM !!!!!!!!
   *
   * NOTE see below for a possible fix
   */

  do
    {
      XEvent event = {0};


      if (timeout_timer)
	{
	  if ((elapsed = g_timer_elapsed(timeout_timer, &ignore)) > object->timeout)
	    {
	      isDone = TRUE;

	      zmapXRemoteSetErrMsg(ZMAPXREMOTE_UNAVAILABLE,
				   ZMAP_XREMOTE_META_FORMAT
				   ZMAP_XREMOTE_ERROR_FORMAT,
				   XDisplayString(object->display),
				   object->window_id, "", "Timeout");

	      result = ZMAPXREMOTE_SENDCOMMAND_TIMEOUT; /* invalid window */

	      goto DONE;
	    }
	}


      /* I think this is wrong because it's removing events which may be wanted elsewhere
       * in the application.....if should be using XPeekEvent() and XSendEvent() somehow...
      if (XPending(object->display))
        XNextEvent(object->display, &event);
      */

        /* MH17: OK let's try to do this, esp wrt RT 176591
         * Google for XNextEvent to find a good enough manual online
         */
        if(!XCheckWindowEvent(object->display, object->window_id, event_mask, &event))
            continue;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if (event.type == ButtonPress || event.type == ButtonRelease || event.type == MotionNotify)
	{
	  printf("found %s\n",
		 (event.type == ButtonPress ? "ButtonPress"
		  : (event.type == ButtonRelease ? "ButtonRelease" : "MotionNotify"))) ;

	  fflush(stdout) ;
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      if (event.xany.window == object->window_id)
        {
          switch(event.type)
            {
            case PropertyNotify:
              {
		isDone = process_property_notify(object, &send_time, &event, response) ;

		break;
              }

            case DestroyNotify:
              {
		if (XCheckTypedWindowEvent(object->display, object->window_id, PropertyNotify, &event))
		  {
		    g_printerr("\n(%s, line %d) - Found destroy before property notify, processing...\n\n",
			       __FILE__, __LINE__);
		    isDone = process_property_notify(object, &send_time, &event, response);
		  }
		else
		  {
		    zmapXRemoteSetErrMsg(ZMAPXREMOTE_UNAVAILABLE,
					 ZMAP_XREMOTE_META_FORMAT
					 ZMAP_XREMOTE_ERROR_FORMAT,
					 XDisplayString(object->display),
					 object->window_id, "", "window was destroyed");
		    REMOTELOGMSG(Warning,"remote : window 0x%lx was destroyed.\n", object->window_id) ;

		    result = ZMAPXREMOTE_SENDCOMMAND_INVALID_WINDOW; /* invalid window */

		    goto DONE;
		  }

		break;
              }

            case CirculateNotify:
            case ConfigureNotify:
            case CreateNotify:
            case GravityNotify:
            case MapNotify:
            case ReparentNotify:
            case UnmapNotify:
              /* CirculateNotify: ConfigureNotify: CreateNotify:
               * GravityNotify: MapNotify: ReparentNotify:
               * UnmapNotify: */

              /* These events are a possibilty from the
               * SubstructureNotifyMask */

              break;

            default:
              REMOTELOGMSG(Warning,"Got event of type %d\n", event.type);
              break;
            } /* switch(event.type) */
        }
    }
  while(!isDone && !window_error_G);


 DONE:
  REMOTELOGMSG(Warning,"%s\n", " - DONE: done");

  if (timeout_timer)
    g_timer_destroy(timeout_timer) ;


  XSelectInput(object->display, object->window_id, 0);


  return result;
}



gboolean zMapXRemoteResponseIsError(ZMapXRemoteObj object, char *response)
{
  gboolean is_error = TRUE;

  zMapAssert(response);

  switch(*response)
    {
    case '2':
      is_error = FALSE;
      break;
    default:
      is_error = TRUE;
    }

  return is_error;
}

void zMapXRemoteResponseSplit(ZMapXRemoteObj object, char *full_response, int *code, char **response)
{
  char *code_str = full_response, *tmp_out = full_response;
  char *d = ZMAP_XREMOTE_STATUS_CONTENT_DELIMITER;

  tmp_out+=4;

  if(code)
    {
      if(code_str[3] == d[0])
        *code = strtol(code_str, &d, 10);
      else
        *code = 0;
    }

  if(response)
    *response = tmp_out;

  return ;
}

int zMapXRemoteIsPingCommand(char *command, int *statusCode, char **reply)
{
  int is = 0;

  if(strncmp(command, ZMAPXREMOTE_PING_COMMAND, 4) == 0)
    {
      is = 1;
      *statusCode = ZMAPXREMOTE_OK;
      *reply = g_strdup("echo");
    }

  return is;
}

/* Get the window id. */
Window zMapXRemoteGetWindowID(ZMapXRemoteObj object)
{
  zMapAssert(object) ;

  return object->window_id ;
}

/*! zMapXRemoteGetResponse
 * ------------------------
 * Generally for the perl bit so it can get the response.
 * Tests for a window_error_G, then for an error message and
 * if neither goes to the atom.  The window_error_G may be
 * false an error message exist when a precondition is not
 * met!
 *
 * As there's a possibility that the user wants to get the
 * error message using this call it now accepts NULL for
 * the object so that the error state doesn't get overwritten
 * with another error!!!
 *
 * May return NULL!
 */
char *zMapXRemoteGetResponse(ZMapXRemoteObj object)
{
  char *response = NULL;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  REMOTELOGMSG(Warning,"%s\n", "just a message to say we're in zMapXRemoteGetResponse");
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  if(window_error_G || (zmapXRemoteErrorText != NULL))
    {
      response = zmapXRemoteGetErrorAsResponse();
    }
  else if(object)
    {
      GError *error = NULL;
      int size;

      if(!zmapXRemoteGetPropertyFullString(object->display,
                                           object->window_id,
                                           object->response_atom,
                                           FALSE, &size,
                                           &response, &error))
        {
          response = NULL;

	  REMOTELOGMSG(Critical, "%s", error->message);

	  g_error_free(error);
        }

    }
  else
    {
      response = NULL;
    }

  return response;
}

char *zMapXRemoteGetRequest(ZMapXRemoteObj object)
{
  GError *error = NULL;
  char *request = NULL;
  int size;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  REMOTELOGMSG(Warning,"%s\n", "just a message to say we're in zMapXRemoteGetRequest");
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  if(!zmapXRemoteGetPropertyFullString(object->display,
                                       object->window_id,
                                       object->request_atom,
                                       FALSE,
                                       &size, &request, &error))
    {
      REMOTELOGMSG(Critical, "%s", error->message);

      g_error_free(error);
    }

  return request;
}

int zMapXRemoteSetReply(ZMapXRemoteObj object, char *content)
{
  int result = ZMAPXREMOTE_SENDCOMMAND_SUCCEED;

  if(object->is_server == FALSE)
    return result;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  REMOTELOGMSG(Warning,"%s: %s\n", "just a message to say we're in zMapXRemoteSetReply",content);
#endif

  result = zmapXRemoteChangeProperty(object, object->response_atom, content);

  return result;
}


/* Gets called for _ALL_ property events on the window,
 * but only responds to the ones identifiably from xremote. */
/* This is a bit of a mis mash of gdk and zMapXRemote calls, can it be sorted  (rds) */
/* I've tried some further generalisattion of this code, although I'm not certain on it's strengths. */
gint zMapXRemoteHandlePropertyNotify(ZMapXRemoteObj xremote,
                                     Atom           event_atom,
                                     guint          event_state,
                                     ZMapXRemoteCallback callback,
                                     gpointer       cb_data)
{
  gint result = FALSE ;
  gint nitems ;
  char *command_text = NULL ;
  GError *error = NULL;

  if (event_atom != xremote->request_atom)
    {
      /* not for us */
      result = FALSE ;
    }
  else if (event_state != GDK_PROPERTY_NEW_VALUE)
    {
      /* THIS IS PROBABLY SIMONS COMMENT FROM ACEDB, I HAVEN'T INVESTIGATED THIS.... */

      /* zero is the value for PropertyNewValue, in X.h there's no gdk equivalent that I can find. */
      /* This looks the equivalent to me */
      /* typedef enum
         {
         GDK_PROPERTY_NEW_VALUE,
         GDK_PROPERTY_DELETE
         } GdkPropertyState;
      */
      result = TRUE ;
    }
  else if(!zmapXRemoteGetPropertyFullString(xremote->display,
                                            xremote->window_id,
                                            xremote->request_atom,
                                            TRUE, &nitems,
                                            &command_text, &error))
    {
      REMOTELOGMSG(Critical, "%s", "X-Atom remote control : unable to read _XREMOTE_COMMAND property") ;
      REMOTELOGMSG(Critical, "%s", error->message);

      g_error_free(error);
      result = TRUE ;
    }
  else if (!command_text || nitems == 0)
    {
      REMOTELOGMSG(Critical, "%s", "X-Atom remote control : invalid data on property") ;

      result = TRUE ;
    }
  else
    {
      gchar *response_text = NULL, *xml_text = NULL, *xml_stub = NULL ;
      int statusCode = ZMAPXREMOTE_INTERNAL;

      REMOTELOGMSG(Warning, "[XREMOTE receive] %s", command_text);

      if(zMapXRemoteIsPingCommand(command_text, &statusCode, &xml_stub) == 0)
        {
          /* Losing control here... as part of the callback
           * zMapXRemoteSendRemoteCommand might be called.  If this is on
           * the same process as the one sending this request a deadlock
           * is pretty certain */
          zmapXRemoteLock();
          /* Get an answer from the callback */
          xml_stub = (callback)(command_text, cb_data, &statusCode, xremote) ;
          zmapXRemoteUnLock();

	  /*
	   * Further info on locking.  Imagine the situation where
	   * callback{
	   *   ask_client something()
	   * }
	   *
	   * client (app1) --- request -> server (app2) [callback] --- request -> client (app1)
	   *
	   * client (app1) is waiting for the response from the server
	   * (app2) so cannot process the request it gets from the
	   * server (app2), which is now a client (app2) and the
	   * server is app1.
	   */
        }

      /* for new view we wait for the map event from X and return null
       * and reply from that callback when the view can accept commands
       * otterlace will wait in a tight loop till we get our window realised and mapped
         zMapAssert(xml_stub);
       */
      if(xml_stub)
        {
          /* Do some processing of the answer to make it fit the protocol */
          xml_text = zMapXRemoteProcessForReply(xremote, statusCode, xml_stub);
          response_text = g_strdup_printf(ZMAP_XREMOTE_REPLY_FORMAT, statusCode, xml_text) ;

          REMOTELOGMSG(Warning, "[XREMOTE respond] %s", response_text);

          /* actually do the replying */
          zMapXRemoteSetReply(xremote, response_text);

          /* clean up */
          g_free(response_text) ;
          g_free(xml_text) ;
          g_free(xml_stub) ;
        }
      result = TRUE ;
    }

  if (command_text)
    g_free(command_text) ;

  return result ;
}





/*
 *                          INTERNALS
 */

/*
      we change the property of the otherh window's request atom and get a notify for that
      then we set the time
      then we should get a notify for the response atom
      and read the reply
*/

static Bool process_property_notify(ZMapXRemoteObj object,
				    PredicateData  predicate,
				    XEvent        *event_in,
				    char         **response)
{
  Bool isDone = False;
  XEvent event = *event_in;
  gboolean atomic_delete = FALSE;


  if (event.type == PropertyNotify
      && (predicate->set == True && event.xproperty.time >= predicate->time)
      && event.xproperty.atom  == object->response_atom)
    {
      switch(event.xproperty.state)
	{
	case PropertyNewValue:
	  {
	    GError *error = NULL;
	    char *commandResult;
	    int nitems;

	    if(!zmapXRemoteGetPropertyFullString(object->display,
						 object->window_id,
						 object->response_atom,
						 atomic_delete, &nitems,
						 &commandResult, &error))
	      {
		/* what about zmapXRemoteSetErrMsg */
		*response = g_strdup(error->message);
		g_error_free(error);
	      }
	    else if (nitems > 0 && commandResult && *commandResult)
	      {
		char *atom_name = NULL;

		if (!(atom_name = zmapXRemoteGetAtomName(object->display, object->response_atom)))
		  {
		    REMOTELOGMSG(Fatal, "Unable to set and get atom 0x%lx. Possible X Server problem.",
				 object->response_atom) ;
		  }
		else
		  {
		    *response = commandResult ;
		    
		    REMOTELOGMSG(Warning,"Received at window '0x%lx' on atom '%s': '%s'\n",
				 object->window_id, atom_name, *response) ;

		    g_free(atom_name);
		  }
	      }
	    else
	      {
		*response = g_strdup("assert_not_reached()");
	      }

	    isDone = TRUE;
	  }
	  break;
	case PropertyDelete:
	  break;
	default:
	  break;
	} /* switch(event.xproperty.state) */
    }
  else if (event.xproperty.atom == object->request_atom)
    {
      char *atom_name = NULL;

      atom_name = zmapXRemoteGetAtomName(object->display, event.xproperty.atom);

      switch(event.xproperty.state)
	{
	case PropertyNewValue:
/*	  REMOTELOGMSG(Warning,"We created the request_atom '%s'\n", atom_name);*/
	  predicate->time = event.xproperty.time;
	  predicate->set  = True;
	  break;
	case PropertyDelete:
/*	  REMOTELOGMSG(Warning,"Server accepted atom '%s'\n", atom_name);*/
	  break;
	default:
	  break;
	} /* switch(event.xproperty.state) */

      if(atom_name)
	g_free(atom_name);
    }
  else
    {
      char *atom_name = NULL;
      atom_name = zmapXRemoteGetAtomName(object->display, event.xproperty.atom);
      REMOTELOGMSG(Warning,"atom '%s' is not the atom we want\n", atom_name);
      if(atom_name)
	g_free(atom_name);
    }

  return isDone;
}

char *zMapXRemoteProcessForReply(ZMapXRemoteObj object, int statusCode, char *cb_output)
{
  char *reply = NULL;
  if(statusCode >= ZMAPXREMOTE_BADREQUEST)
    {
      reply = g_strdup_printf(ZMAP_XREMOTE_ERROR_FORMAT ZMAP_XREMOTE_META_FORMAT,
                              cb_output,
                              XDisplayString(object->display),
                              object->window_id,
                              ""
                              );
    }
  else
    {
      reply = g_strdup_printf(ZMAP_XREMOTE_SUCCESS_FORMAT ZMAP_XREMOTE_META_FORMAT,
                              cb_output,
                              XDisplayString(object->display),
                              object->window_id,
                              ""
                              );
    }

  return reply;
}




static char *zmapXRemoteGetAtomName(Display *display, Atom atom)
{
  char *name = NULL ;
  char *atom_id ;
  char *xname ;

  atom_id = g_strdup_printf("Atom = %x", (unsigned int)atom) ;

  if (!atom)
    {
      REMOTELOGMSG(Critical,"Null atom received: %s", atom_id) ;
    }
  else
    {
      zmapXTrapErrors((char *)__PRETTY_FUNCTION__, "XGetAtomName", atom_id) ;

      if ((xname = XGetAtomName(display, atom)))
	{
	  name = g_strdup(xname) ;

	  XFree(xname) ;
	}

      zmapXUntrapErrors() ;
    }


  g_free(atom_id) ;


  return name ;
}


static ZMapXRemoteSendCommandError zmapXRemoteChangeProperty(ZMapXRemoteObj object, Atom atom, char *change_to)
{
  ZMapXRemoteSendCommandError result = ZMAPXREMOTE_SENDCOMMAND_SUCCEED ;
  char *err_txt ;
  Window win;
  char *atom_name = NULL;

  win = object->window_id;

  atom_name = zmapXRemoteGetAtomName(object->display, atom);

  err_txt = g_strdup_printf("Atom \"%s\", Property \"%s\"", atom_name, change_to) ;

  zmapXTrapErrors((char *)__PRETTY_FUNCTION__, "XChangeProperty", err_txt) ;

  g_free(err_txt) ;

  zmapXRemoteErrorStatus = ZMAPXREMOTE_PRECOND ;

  XChangeProperty(object->display, win,
		  atom, XA_STRING, 8,
		  PropModeReplace, (unsigned char *)change_to,
		  strlen(change_to));

  REMOTELOGMSG(Warning,"Sent to window '0x%lx' on atom '%s': '%s'\n", win, atom_name, change_to);

  XSync(object->display, False);

  zmapXUntrapErrors() ;

  if(window_error_G)
    result = ZMAPXREMOTE_SENDCOMMAND_PROPERTY_ERROR;

  g_free(atom_name);

  return result;
}



/* Check the client and server were compiled using the same <ZMap/zmapXRemote.h> */
static ZMapXRemoteSendCommandError zmapXRemoteCheckWindow(ZMapXRemoteObj object)
{
  ZMapXRemoteSendCommandError result = ZMAPXREMOTE_SENDCOMMAND_SUCCEED;

  if(zmapXRemoteCmpAtomString(object, object->version_sanity_atom,
                              ZMAP_XREMOTE_CURRENT_VERSION))
    result |= ZMAPXREMOTE_SENDCOMMAND_VERSION_MISMATCH ;

  if(object->remote_app)
    {
      if(zmapXRemoteCmpAtomString(object, object->app_sanity_atom,
                                  object->remote_app))
        result |= ZMAPXREMOTE_SENDCOMMAND_APP_MISMATCH;
    }

  return result;
}



static ZMapXRemoteSendCommandError zmapXRemoteCmpAtomString (ZMapXRemoteObj object, Atom atom, char *expected)
{
  GError *error = NULL;
  char *atom_string = NULL;
  int nitems;
  ZMapXRemoteSendCommandError unmatched = ZMAPXREMOTE_SENDCOMMAND_SUCCEED; /* success */

  if (!zmapXRemoteGetPropertyFullString(object->display,
					object->window_id,
					atom, False,
					&nitems, &atom_string,
					&error))
    {
      if(error)
        {
          REMOTELOGMSG(Warning,"Warning : window 0x%lx is not a valid remote-controllable window, error = %d/%s.\n",
                     object->window_id,(int) error->code,error->message);

          switch(error->code)
            {
            case XGETWINDOWPROPERTY_ERROR:
            case XGETWINDOWPROPERTY_FAILURE:
              zmapXRemoteSetErrMsg(ZMAPXREMOTE_PRECOND,
                                   ZMAP_XREMOTE_META_FORMAT
                                   ZMAP_XREMOTE_ERROR_FORMAT,
                                   XDisplayString(object->display),
                                   (unsigned int)(object->window_id),
                                   "",
                                   error->message);
              break;
            case NO_MATCHING_PROPERTY_TYPE:
            case UNEXPECTED_FORMAT_SIZE:
            default:
              zmapXRemoteSetErrMsg(ZMAPXREMOTE_PRECOND,
                                   ZMAP_XREMOTE_META_FORMAT
                                   ZMAP_XREMOTE_ERROR_FORMAT,
                                   XDisplayString(object->display),
                                   (unsigned int)(object->window_id),
                                   "",
                                   "Is not a valid remote-controllable window");
              break;
            }

	  g_error_free(error);
        }

      unmatched = ZMAPXREMOTE_SENDCOMMAND_INVALID_WINDOW;
    }
  else if (!(nitems > 0 && atom_string && *atom_string))
    {
      unmatched = ZMAPXREMOTE_SENDCOMMAND_INVALID_WINDOW;
    }
  else if (strcmp(atom_string, expected) != 0)
    {
      zmapXRemoteSetErrMsg(ZMAPXREMOTE_PRECOND,
                           ZMAP_XREMOTE_META_FORMAT
                           ZMAP_XREMOTE_ERROR_START
                           "remote controllable window uses"
                           " different version %s of remote"
                           " control system, expected version %s."
                           ZMAP_XREMOTE_ERROR_END,
                           XDisplayString(object->display),
                           (unsigned int)(object->window_id), "",
                           atom_string, expected);

      REMOTELOGMSG(Warning,"Warning : remote controllable window 0x%lx uses "
                 "different version %s of remote control system, expected "
                 "version %s.\n",
                 object->window_id,
                 atom_string, expected);

      g_free(atom_string);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      unmatched = ZMAPXREMOTE_SENDCOMMAND_INVALID_WINDOW; /* failure */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      unmatched = ZMAPXREMOTE_SENDCOMMAND_SUCCEED;
    }
  else
    {
      unmatched = ZMAPXREMOTE_SENDCOMMAND_SUCCEED;
    }

  return unmatched;
}




static gboolean zmapXRemoteGetPropertyFullString(Display *display,
                                                 Window   xwindow,
                                                 Atom     xproperty,
                                                 gboolean delete, int *size,
                                                 char **full_string_out,
                                                 GError **error_out)
{
  gboolean success = TRUE;
  Atom xtype = XA_STRING;
  Atom xtype_return;
  GString *output  = NULL;
  GError  *error   = NULL;
  GQuark   domain  = 0;
  gint result, format_return;
  gulong req_offset, req_length;
  gulong nitems_return, bytes_after;
  gchar *property_data;
  int i = 0, attempts = 2;

  domain = g_quark_from_string(__FILE__);
  output = g_string_sized_new(1 << 8); /* 1 << 8 = 256 */

  req_offset = 0;
  req_length = 0;

  /* We need to go through at least once, i controls a second time */
  do
    {
      gboolean atomic_delete = FALSE;
      char *atom_id ;

      /* No delete the first time, but then delete according to user's request. */
      if (i == 1)
	{
	  atomic_delete = delete;
	}

      /* first get the total number, then get the full data */
      atom_id = g_strdup_printf("XProperty Atom = %x", (unsigned int)xproperty) ;
      zmapXTrapErrors((char *)__PRETTY_FUNCTION__, "XGetWindowProperty", atom_id) ;
      g_free(atom_id) ;

      result = XGetWindowProperty(display, xwindow,
				  xproperty, /* requested property */
				  req_offset, req_length, /* offset and length */
				  atomic_delete, xtype,
				  &xtype_return, &format_return,
				  &nitems_return, &bytes_after,
				  (guchar **)&property_data) ;
      zmapXUntrapErrors() ;

      /* First test for an X Error... Copy it to the GError which the
       * user will be checking... */
      if(window_error_G)
        {
          error = g_error_new(domain, XGETWINDOWPROPERTY_ERROR,
                              "XError %s", zmapXRemoteRawErrorText);
          success = FALSE;
          i += attempts;              /* no more attempts */
        }
      /* make sure we use Success from the X11 definition for success... */
      else if(result != Success)
        {
          if(xtype_return == None && format_return == 0)
            error = g_error_new(domain, XGETWINDOWPROPERTY_FAILURE, "%s",
                                "Call to XGetWindowProperty returned Failure"
                                " and return types unexpected.");
          else
            error = g_error_new(domain, XGETWINDOWPROPERTY_FAILURE, "%s",
                                "Call to XGetWindowProperty returned Failure");
          success = FALSE;
          i += attempts;              /* no more attempts */
        }
      else if((xtype != AnyPropertyType) && (xtype_return != xtype))
        {
	  char *atom_type_name, *atom_return_name;

	  atom_type_name = zmapXRemoteGetAtomName(display, xtype);
	  atom_return_name = zmapXRemoteGetAtomName(display, xtype_return);

          XFree(property_data);

	  /* We seem to be getting here with increasing frequency. */
          error = g_error_new(domain, NO_MATCHING_PROPERTY_TYPE,
                              "No matching property type. "
                              "Requested %s, got %s",
                              atom_type_name,
                              atom_return_name);

	  g_free(atom_type_name);
	  g_free(atom_return_name);

          success = FALSE;
          i += attempts;              /* no more attempts */
        }
      else if(i == 0)
        {
          /* First time just update offset and length... */
          req_offset = 0;
          req_length = bytes_after;
        }
      else if(property_data && *property_data && nitems_return && format_return)
        {
          switch(format_return)
            {
            case 8:
              g_string_append_len(output, property_data, nitems_return);
              break;
            case 16:
            case 32:
            default:
              error = g_error_new(domain, UNEXPECTED_FORMAT_SIZE,
                                  "Unexpected format size %d for string",
                                  format_return);
              success = FALSE;
              i += attempts;              /* no more attempts */
              break;
            }
          XFree(property_data);
        }
      else
        {
          zMapAssertNotReached();
        }

      i++;                      /* Important increment! */
    }
  while(i < attempts);          /* attempt == 2 */

  if(size)
    *size = output->len;
  if (full_string_out)
    *full_string_out = g_string_free(output, FALSE);
  if(error_out)
    *error_out = error;

  if(!success)
    zMapAssert(error != NULL);

  return success;
}


/*============ ERROR FUNCTIONS BELOW ===================  */

/* =============================================================== */
static char *zmapXRemoteGetErrorAsResponse(void) /* Translation for users */
/* =============================================================== */
{
  char *err;

  if(zmapXRemoteErrorText == NULL)
    {
      zMapXRemoteSimpleErrMsgStruct errorTable[] =
        {
          /* 1xx  Informational */
          /* 2xx  Successful    */
          {ZMAPXREMOTE_METAERROR   , ZMAP_XREMOTE_ERROR_START "meta error" ZMAP_XREMOTE_ERROR_END},
          {ZMAPXREMOTE_NOCONTENT   , ZMAP_XREMOTE_ERROR_START "no content was returned" ZMAP_XREMOTE_ERROR_END},
          /* 3xx  Redirection   */
          /* Redirect??? I don't think so */
          /* 4xx  Client Errors */
          {ZMAPXREMOTE_BADREQUEST  , ZMAP_XREMOTE_ERROR_START "you made a bad request to the server" ZMAP_XREMOTE_ERROR_END},
          {ZMAPXREMOTE_FORBIDDEN   , ZMAP_XREMOTE_ERROR_START "you are not allowed to see this" ZMAP_XREMOTE_ERROR_END},
          {ZMAPXREMOTE_UNKNOWNCMD  , ZMAP_XREMOTE_ERROR_START "unknown command" ZMAP_XREMOTE_ERROR_END},
          {ZMAPXREMOTE_PRECOND     , ZMAP_XREMOTE_ERROR_START "something went wrong with sanity checking" ZMAP_XREMOTE_ERROR_END},
          /* 5xx  Server Errors  */
          {ZMAPXREMOTE_INTERNAL    , ZMAP_XREMOTE_ERROR_START "Internal Server Error" ZMAP_XREMOTE_ERROR_END},
          {ZMAPXREMOTE_UNKNOWNATOM , ZMAP_XREMOTE_ERROR_START "Unknown Server Atom" ZMAP_XREMOTE_ERROR_END},
          {ZMAPXREMOTE_NOCREATE    , ZMAP_XREMOTE_ERROR_START "No Resource was created" ZMAP_XREMOTE_ERROR_END},
          {ZMAPXREMOTE_UNAVAILABLE , ZMAP_XREMOTE_ERROR_START "Unavailable Server Resource" ZMAP_XREMOTE_ERROR_END},
          {ZMAPXREMOTE_TIMEDOUT    , ZMAP_XREMOTE_ERROR_START "Server Timeout" ZMAP_XREMOTE_ERROR_END},
          /* NULL to end the while loop below! */
          {ZMAPXREMOTE_OK          , NULL}
        } ;
      zMapXRemoteSimpleErrMsg curr_err;

      curr_err = &errorTable[0];
      while (curr_err->message != NULL)
        {
          if(curr_err->status == zmapXRemoteErrorStatus)
            {
              zmapXRemoteSetErrMsg(curr_err->status, curr_err->message);
              break;
            }
          else
            curr_err++;
        }
    }

  err = g_strdup_printf(ZMAP_XREMOTE_REPLY_FORMAT,
                        zmapXRemoteErrorStatus,
                        zmapXRemoteErrorText);
  return err;
}

static void zmapXRemoteResetErrMsg(void)
{
  if (zmapXRemoteErrorText != NULL)
    {
      g_free(zmapXRemoteErrorText);
      zmapXRemoteErrorText = NULL;
    }

  return ;
}


static void zmapXRemoteSetErrMsg(ZMapXRemoteStatus status, char *msg, ...)
{
  va_list args;
  char *callErr;

  if (zmapXRemoteErrorText != NULL)
    {
      g_free(zmapXRemoteErrorText);
      zmapXRemoteErrorText = NULL;
    }

  /* Format the supplied error message. */
  va_start(args, msg) ;
  callErr = g_strdup_vprintf(msg, args) ;
  va_end(args) ;

  zmapXRemoteErrorStatus = status;
  zmapXRemoteErrorText   = g_strdup( callErr ) ;

  g_free(callErr);

  return;
}


/*!
 * \brief Set the XErrorHandler to be zmapXErrorHandler, saving whichever is the current one.
 *
 * It should be noted that nested calls to this and untrap will _not_ function correctly!
 */
static void zmapXTrapErrors(char *where, char *what, char *text)
{
  XErrorHandler current = NULL ;

  window_error_G = False ;

  trap_txt_G = g_strdup_printf("X Error in \"%s\" calling \"%s\"() with \"%s\"",
			       where, what, text) ;

  if ((current = XSetErrorHandler(zmapXErrorHandler)))
    {
      stored_xerror_handler(current, TRUE, FALSE) ;
    }

  return ;
}


static int zmapXErrorHandler(Display *dpy, XErrorEvent *e )
{
  char errorText[1024] ;

  window_error_G = True ;

  XGetErrorText(dpy, e->error_code, errorText, sizeof(errorText)) ;

  /* N.B. The continuation of string here */
  zmapXRemoteSetErrMsg(zmapXRemoteErrorStatus,
		       ZMAP_XREMOTE_META_FORMAT
		       ZMAP_XREMOTE_ERROR_FORMAT " Reason: %s",
		       XDisplayString(dpy), e->resourceid, "", errorText, trap_txt_G) ;

  /* This is due to some non-ideal coding that means we sometimes (1 case)
   * double process the error with the meta and error format strings. If
   * there is ever more than one case (see zmapXRemoteGetPropertyFullString)
   * find a better solution.
   */
  if(zmapXRemoteRawErrorText != NULL)
    g_free(zmapXRemoteRawErrorText);

  zmapXRemoteRawErrorText = g_strdup(errorText) ;

  REMOTELOGMSG(Warning, "**** X11 Error: %s **** Reason: %s", errorText, trap_txt_G) ;

  return 1 ;						    /* This seems to be ignored by the server (!) */
}


static void zmapXUntrapErrors(void)
{
  XErrorHandler restore = NULL;

  restore = stored_xerror_handler(NULL, FALSE, FALSE) ;
  XSetErrorHandler(restore) ;

  stored_xerror_handler(NULL, TRUE, TRUE) ;

  if (!window_error_G)
    zmapXRemoteErrorStatus = ZMAPXREMOTE_INTERNAL ;

  g_free(trap_txt_G) ;
  trap_txt_G = NULL ;

  return ;
}


/*
 * If store == TRUE, store e unless stored != NULL or e == zmapXErrorHandler
 * If clear == TRUE, reset stored to NULL
 */
static XErrorHandler stored_xerror_handler(XErrorHandler e, gboolean store, gboolean clear)
{
  static XErrorHandler stored = NULL;

  if(e == zmapXErrorHandler)
    store = FALSE;              /* We mustn't store this */

  if(clear)
    stored = NULL;

  if(store)
    {
      if(stored == NULL)
        stored = e;
      else
        {
          REMOTELOGMSG(Warning,"I'm not forgetting %p, but not storing %p either!\n", stored, e);

	  REMOTELOGMSG(Warning, "I'm not forgetting %p, but not storing %p either!\n", stored, e);
        }
    }

  return stored;
}





/*
 * Locking, I understand none of this because I have not looked through the calling code.
 */

static void zmapXRemoteLock()
{
  xremote_lock_G = TRUE;
  return ;
}

static void zmapXRemoteUnLock()
{
  xremote_lock_G = FALSE;
  return ;
}


static gboolean zmapXRemoteIsLocked()
{
  return xremote_lock_G;
}

