/*  File: zmapXRemote.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See ZMap/zmapXRemote.h
 * HISTORY:
 * Last edited: May 31 17:26 2006 (rds)
 * Created: Wed Apr 13 19:04:48 2005 (rds)
 * CVS info:   $Id: zmapXRemote.c,v 1.16 2006-05-31 16:26:41 rds Exp $
 *-------------------------------------------------------------------
 */

#include "zmapXRemote_P.h"

zMapXRemoteObj zMapXRemoteNew(void)
{
  zMapXRemoteObj object;
  
  /* Get current display, open it to check it's valid 
     and store in struct to save having to do it again */
  if (getenv("DISPLAY"))
    {
      char *display_str;
      if((display_str = (char*)malloc (strlen(getenv("DISPLAY")) + 1)) == NULL)
        return NULL;
      
      strcpy (display_str, getenv("DISPLAY"));

      zmapXDebug("Using DISPLAY: %s\n", display_str);

      object = g_new0(zMapXRemoteObjStruct,1);
      if((object->display = XOpenDisplay (display_str)) == NULL)
        {
          zmapXDebug("Failed to open display '%s'\n", display_str);

          zmapXRemoteSetErrMsg(ZMAPXREMOTE_PRECOND, 
                               ZMAP_XREMOTE_META_FORMAT
                               ZMAP_XREMOTE_ERROR_FORMAT,
                               display_str, 0, "", "Failed to open display");
          free(display_str);
          free(object);
          return NULL;
        }
      object->init_called = FALSE;
      object->is_server   = FALSE;
    }
#ifdef DO_DEBUGGING
  XSynchronize(object->display, True);
#endif
  return object;
}

void zMapXRemoteDestroy(zMapXRemoteObj object)
{
  zmapXDebug("%s id: 0x%lx\n", "Destroying object", object->window_id);

  if(object->remote_app)
    g_free(object->remote_app);

  /* Don't think we need this bit
   * ****************************
   * zmapXTrapErrors();  
   * XDeleteProperty(object->display, object->window_id, object->request_atom);
   * XDeleteProperty(object->display, object->window_id, object->response_atom);
   * XDeleteProperty(object->display, object->window_id, object->app_sanity_atom);
   * XDeleteProperty(object->display, object->window_id, object->version_sanity_atom);
   * XSync(object->display, False);
   * zmapXUntrapErrors();
   * ****************************
   */

  g_free(object);
  return ;
}

void zMapXRemoteSetWindowID(zMapXRemoteObj object, Window id)
{
  object->window_id = id;
  zmapXDebug("setting id to '0x%x' \n", (unsigned int) (object->window_id));
  return;
}

void zMapXRemoteSetRequestAtomName(zMapXRemoteObj object, char *name)
{
  char *atom_name = NULL;
  zmapXDebug("zMapXRemoteSetRequestAtomName change to '%s'\n", name);
  object->request_atom = XInternAtom (object->display, name, False);

  if(!(atom_name = XGetAtomName(object->display, object->request_atom)))
    zMapLogFatal("Unable to set and get atom '%s'. Possible X Server problem.", name);

  zmapXDebug("New name is %s\n", zmapXRemoteGetAtomName(object, object->request_atom));
  /* XSync(object->display, True); */
  /* zmapXRemoteChangeProperty(object, object->request_atom, ""); */
  return ;
}

void zMapXRemoteSetResponseAtomName(zMapXRemoteObj object, char *name)
{
  char *atom_name = NULL;
  zmapXDebug("zMapXRemoteSetResponseAtomName change to '%s'\n", name);  
  object->response_atom = XInternAtom(object->display, name, False);

  if(!(atom_name = XGetAtomName(object->display, object->response_atom)))
    zMapLogFatal("Unable to set and get atom '%s'. Possible X Server problem.", name);

  zmapXDebug("New name is %s\n", zmapXRemoteGetAtomName(object, object->response_atom));
  /* XSync(object->display, True); */
  /* zmapXRemoteChangeProperty(object, object->response_atom, ""); */
  return ;
}

int zMapXRemoteInitClient(zMapXRemoteObj object, Window id)
{

  if(object->init_called == TRUE)
    return 1;

  zMapXRemoteSetWindowID(object, id);

  if (! object->request_atom)
    zMapXRemoteSetRequestAtomName(object, ZMAP_DEFAULT_REQUEST_ATOM_NAME);

  if (! object->response_atom)
    zMapXRemoteSetResponseAtomName(object, ZMAP_DEFAULT_RESPONSE_ATOM_NAME);

  if (! object->version_sanity_atom)
    object->version_sanity_atom = XInternAtom (object->display, ZMAP_XREMOTE_CURRENT_VERSION_ATOM, False);

  if (! object->app_sanity_atom)
    object->app_sanity_atom = XInternAtom(object->display, ZMAP_XREMOTE_APPLICATION_ATOM, False);

  object->init_called = TRUE;

  return 1;
}

int zMapXRemoteInitServer(zMapXRemoteObj object,  Window id, char *appName, char *requestName, char *responseName)
{

  if(object->init_called == TRUE)
    return 1;

  zMapXRemoteSetWindowID(object, id);

  if (!object->request_atom && requestName)
      zMapXRemoteSetRequestAtomName(object, requestName);

  if (!object->response_atom && responseName)
      zMapXRemoteSetResponseAtomName(object, responseName);

  if (! object->version_sanity_atom)
    {
      char *atom_name = NULL;
      object->version_sanity_atom = XInternAtom (object->display, ZMAP_XREMOTE_CURRENT_VERSION_ATOM, False);
      if(!(atom_name = XGetAtomName(object->display, object->version_sanity_atom)))
        zMapLogFatal("Unable to set and get atom '%s'. Possible X Server problem.", ZMAP_XREMOTE_CURRENT_VERSION_ATOM);
      if(zmapXRemoteChangeProperty(object, object->version_sanity_atom, ZMAP_XREMOTE_CURRENT_VERSION))
        zMapLogFatal("Unable to change atom '%s'. Possible X Server problem.", ZMAP_XREMOTE_CURRENT_VERSION_ATOM);
    }
  if (! object->app_sanity_atom)
    {
      char *atom_name = NULL;
      object->app_sanity_atom = XInternAtom(object->display, ZMAP_XREMOTE_APPLICATION_ATOM, False);
      if(!(atom_name = XGetAtomName(object->display, object->app_sanity_atom)))
        zMapLogFatal("Unable to set and get atom '%s'. Possible X Server problem.", ZMAP_XREMOTE_APPLICATION_ATOM);
      if(zmapXRemoteChangeProperty(object, object->app_sanity_atom, appName))
        zMapLogFatal("Unable to change atom '%s'. Possible X Server problem.", ZMAP_XREMOTE_APPLICATION_ATOM);
    }

  object->is_server   = TRUE;
  object->init_called = TRUE;

  return 1;
}

int zMapXRemoteSendRemoteCommands(zMapXRemoteObj object){
  int response_status = 0;
  
  zmapXDebug("\n trying %s \n", "a broken function...");

  return response_status;
}

/*=======================================================================*/
/* zMapXRemoteSendRemoteCommand is based on the original file 'remote.c'
 * which carried this notice : */
/* -*- Mode:C; tab-width: 8 -*-
 * remote.c --- remote control of Netscape Navigator for Unix.
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
int zMapXRemoteSendRemoteCommand(zMapXRemoteObj object, char *command)
{
  int result = 0;
  Bool isDone   = False;
  XEvent event;
  Display *dpy  = object->display;
  Window window = object->window_id;
  unsigned long event_mask = (PropertyChangeMask | StructureNotifyMask);

  if(object->is_server == TRUE)
    {
      result = ZMAPXREMOTE_SENDCOMMAND_ISSERVER;
      goto DONE;
    }

  zmapXRemoteResetErrMsg();

  result = zmapXRemoteCheckWindow(object);

  if(result != 0)
    {
      result = ZMAPXREMOTE_SENDCOMMAND_VERSION_MISMATCH;
      goto DONE;
    }

  zmapXDebug("remote: (writing %s '%s' to 0x%x)\n",
             zmapXRemoteGetAtomName(object, object->request_atom),
             command, (unsigned int) object->window_id);

  XSelectInput(object->display, object->window_id, event_mask);

  result = zmapXRemoteChangeProperty(object, object->request_atom, command);
  
  zmapXDebug("sent '%s'...\n", command);

  while (!isDone && !windowError)
    {
      zmapXDebug("%s"," - while: I'm still waiting...\n");

      //      if(XPending(dpy))
      XNextEvent (object->display, &event);

      zmapXDebug(" - while: got event type %d\n", event.type);

      if (windowError){
        result = ZMAPXREMOTE_SENDCOMMAND_INVALID_WINDOW;
        goto DONE;
      }

      if (event.xany.type == DestroyNotify &&
	  event.xdestroywindow.window == window
          /* && !XPending(object->display) */)
	{
          zmapXRemoteSetErrMsg(ZMAPXREMOTE_UNAVAILABLE, 
                               ZMAP_XREMOTE_META_FORMAT
                               ZMAP_XREMOTE_ERROR_FORMAT,
                               /*"<display>%s</display>"
                               "<windowid>0x%0x</windowid>"
                               "<message>window was destroyed</message>", */
                               XDisplayString(dpy), window, "", "window was destroyed");
	  zmapXDebug("remote : window 0x%x was destroyed.\n",
		   (unsigned int) object->window_id);
	  result = ZMAPXREMOTE_SENDCOMMAND_INVALID_WINDOW; /* invalid window */
	  goto DONE;
	}
      else if (event.xany.type        == PropertyNotify &&
	       event.xproperty.state  == PropertyNewValue &&
	       event.xproperty.window == object->window_id &&
	       event.xproperty.atom   == object->response_atom
               /* && !XPending(object->display) */)
	{
	  Atom actual_type;
	  int actual_format;
	  unsigned long nitems, bytes_after;
	  unsigned char *commandResult;
	  int x_status;
	  
          zmapXTrapErrors();
	  x_status = XGetWindowProperty(object->display, object->window_id, 
                                        object->response_atom, /* XA_XREMOTE_RESPONSE,  */
                                        0, (65536 / sizeof (long)),
                                        /* True or False ?????????? wed 13 change, was true */
                                        False, /* atomic delete after */
                                        XA_STRING,
                                        &actual_type, &actual_format,
                                        &nitems, &bytes_after,
                                        &commandResult);
          zmapXUntrapErrors();

	  if (!windowError && x_status == Success && commandResult && *commandResult)
	    {
	      zmapXDebug("remote: (server sent %s '%s' to 0x%x.)\n",
                         zmapXRemoteGetAtomName(object, object->response_atom), 
                         commandResult, 
                         (unsigned int) object->window_id);
              
	    }

	  if (x_status != Success)
	    {
              zmapXRemoteSetErrMsg(ZMAPXREMOTE_UNAVAILABLE, 
                                   ZMAP_XREMOTE_META_FORMAT
                                   ZMAP_XREMOTE_ERROR_START
                                   "failed reading atom '%s' from window"
                                   ZMAP_XREMOTE_ERROR_END,
                                   XDisplayString(dpy), window, "", 
                                   zmapXRemoteGetAtomName(object, object->response_atom));
	      zmapXDebug("remote: failed reading %s from window 0x%0x.\n",
                         zmapXRemoteGetAtomName(object, object->response_atom),
                       (unsigned int) object->window_id);
	      result = ZMAPXREMOTE_SENDCOMMAND_INVALID_WINDOW; /* invalid window */
	      isDone = True;
	    }
	  else
	    {
	      if (commandResult && *commandResult)
		{
		  zmapXDebug("cmd result| %s\n", commandResult);
		  result = ZMAPXREMOTE_SENDCOMMAND_SUCCEED; /* everything OK */
		  isDone = True;
		}
	      else
		{
                  zmapXRemoteSetErrMsg(ZMAPXREMOTE_CONFLICT, 
                                       ZMAP_XREMOTE_META_FORMAT
                                       ZMAP_XREMOTE_ERROR_START
                                       "invalid data on atom '%s'"
                                       ZMAP_XREMOTE_ERROR_END, 
                                       XDisplayString(dpy), window, "",
                                       zmapXRemoteGetAtomName(object, object->response_atom));
		  zmapXDebug("remote: invalid data on %s property of window 0x%0x.\n",
                             zmapXRemoteGetAtomName(object, object->response_atom),
                             (unsigned int) object->window_id);
		  result = ZMAPXREMOTE_SENDCOMMAND_INVALID_WINDOW; /* invalid window */
		  isDone = True;
		}
	    }
	  
	  if (commandResult)
	    XFree (commandResult);
	}
      else if (event.xany.type == PropertyNotify &&
	       event.xproperty.window == (object->window_id) &&
	       event.xproperty.state == PropertyDelete &&
	       event.xproperty.atom == (object->request_atom))
	{
	  zmapXDebug("remote: (server 0x%0x has accepted %s)\n",
                     (unsigned int) object->window_id,
                     zmapXRemoteGetAtomName(object, object->request_atom)
                     );
	}
      else
        {
          zmapXDebug("%s\n"," - else: still waiting");
        }
    }

 DONE:
  zmapXDebug("%s\n"," - DONE: done");

  return result;  
}
/*! zMapXRemoteGetResponse 
 * ------------------------
 * Generally for the perl bit so it can get the response.
 * Tests for a windowError, then for an error message and 
 * if neither goes to the atom.  The windowError may be
 * false an error message exist when a precondition is not
 * met!
 */
char *zMapXRemoteGetResponse(zMapXRemoteObj object)
{
  zmapXDebug("%s\n", "just a message to say we're in zMapXRemoteGetResponse");

  if(windowError)
    {
      return zmapXRemoteGetErrorAsResponse();
    }
  else if(zmapXRemoteErrorText != NULL)
    {
      zmapXDebug("%s\n", "else if");
      return zmapXRemoteGetErrorAsResponse();
    }
  else
    {
      zmapXDebug("%s\n", "else");
      return zmapXRemoteGetComputedContent(object, object->response_atom, True);
    }
}

GdkAtom zMapXRemoteGdkRequestAtom(zMapXRemoteObj object){
  GdkAtom req;
  zmapXDebug("%s\n", "just a message to say we're in zMapXRemoteGdkRequestAtom");
  req = gdk_x11_xatom_to_atom(object->request_atom);
  return req;
}

GdkAtom zMapXRemoteGdkResponseAtom(zMapXRemoteObj object){
  GdkAtom res;
  zmapXDebug("%s\n", "just a message to say we're in zMapXRemoteGdkResponseAtom");
  res = gdk_x11_xatom_to_atom(object->response_atom);
  return res;
}

char *zMapXRemoteGetRequest(zMapXRemoteObj object)
{
  zmapXDebug("%s\n", "just a message to say we're in zMapXRemoteGetRequest");
  return zmapXRemoteGetComputedContent(object, object->request_atom, True);
}

int zMapXRemoteSetReply(zMapXRemoteObj object, char *content)
{
  int result = 0;

  if(object->is_server == FALSE)
    return result;
  zmapXDebug("%s\n", "just a message to say we're in zMapXRemoteSetReply");
  result = zmapXRemoteChangeProperty(object, object->response_atom, content);
  return result;
}


/* Gets called for _ALL_ property events on the window,
 * but only responds to the ones identifiably from xremote. */
/* This is a bit of a mis mash of gdk and zMapXRemote calls, can it be sorted  (rds) */
/* I've tried some further generalisattion of this code, although I'm not certain on it's strengths. */
gint zMapXRemotePropertyNotifyEvent(GtkWidget *widget, GdkEventProperty *ev, gpointer notify_data)
{
  gint result = FALSE ;
  gpointer user_data ;
  zMapXRemoteNotifyData notifyStruct = (zMapXRemoteNotifyData)notify_data;
  zMapXRemoteObj xremote;
  static GdkAtom request_atom  = 0 ;
  static GdkAtom response_atom = 0 ;
  static GdkAtom string_atom   = 0 ;
  GdkAtom actualType ;
  gint actualFormat ;
  gint nitems ;
  guchar *command_text = NULL ;

  xremote   = notifyStruct->xremote;
  user_data = notifyStruct->data;

  if (!request_atom)
    {
      request_atom  = zMapXRemoteGdkRequestAtom(xremote); //gdk_atom_intern(ZMAP_DEFAULT_REQUEST_ATOM_NAME, FALSE) ;
      response_atom = zMapXRemoteGdkResponseAtom(xremote);//gdk_atom_intern(ZMAP_DEFAULT_RESPONSE_ATOM_NAME, FALSE) ;
      string_atom   = gdk_atom_intern("STRING", FALSE) ;
    }

  if (ev->atom != request_atom)
    {
      /* not for us */
      result = FALSE ;
    }
  else if (ev->state != GDK_PROPERTY_NEW_VALUE)
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
  else if (!gdk_property_get(ev->window,
			     request_atom,
			     string_atom,
			     0,
			     (65536/ sizeof(long)),
			     TRUE,
			     &actualType,
			     &actualFormat,
			     &nitems,
			     &command_text))
    {
      zMapLogCritical("%s", "X-Atom remote control : unable to read _XREMOTE_COMMAND property") ;
      
      result = TRUE ;
    }
  else if (!command_text || nitems == 0)
    {
      zMapLogCritical("%s", "X-Atom remote control : invalid data on property") ;

      result = TRUE ;
    }
  else
    { 
      gchar *response_text, *xml_text, *xml_stub;
      char *copy_command ;
      int statusCode = ZMAPXREMOTE_INTERNAL;

      copy_command = g_malloc0(nitems + 1) ;
      memcpy(copy_command, command_text, nitems);
      copy_command[nitems] = 0 ;			    /* command_text is not zero terminated */

      /* Get an answer from the callback */
      xml_stub = (notifyStruct->callback)((char *)copy_command, user_data, &statusCode) ; 

      zMapAssert(xml_stub); /* Need an answer */

      /* Do some processing of the answer to make it fit the protocol */
      xml_text      = zmapXRemoteProcessForReply(xremote, statusCode, xml_stub);
      response_text = g_strdup_printf(ZMAP_XREMOTE_REPLY_FORMAT, statusCode, xml_text) ;

      printf("%s\n", response_text);


      /* actually do the replying */
      zMapXRemoteSetReply(xremote, response_text);

      /* clean up */
      g_free(response_text) ;
      g_free(xml_text) ;
      g_free(xml_stub) ;
      g_free(copy_command) ;

      result = TRUE ;
    }

  if (command_text)
    g_free(command_text) ;

  return result ;
}

/* ====================================================== */
/* INTERNALS */
/* ====================================================== */
static char *zmapXRemoteProcessForReply(zMapXRemoteObj object, int statusCode, char *cb_output)
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

static char *zmapXRemoteGetAtomName(zMapXRemoteObj obj, Atom atom)
{
  char *name;
  name = g_strdup(XGetAtomName(obj->display, atom));
  return name;
}

static int zmapXRemoteChangeProperty(zMapXRemoteObj object, Atom atom, char *change_to)
{
  Window win;

  win = object->window_id;

  zmapXTrapErrors();
  zmapXRemoteErrorStatus = ZMAPXREMOTE_PRECOND;

  zmapXDebug("Changing atom '%s' to value '%s'\n", zmapXRemoteGetAtomName(object, atom), change_to);
  XChangeProperty (object->display, win,
                   atom, XA_STRING, 8,
                   PropModeReplace, (unsigned char *)change_to,
                   strlen (change_to)
                   );
  XSync(object->display, False);
  if(windowError)
    return 2;
  zmapXUntrapErrors();

  return 0;
}

/* Check the client and server were compiled using the same <ZMap/zmapXRemote.h> */

static int zmapXRemoteCheckWindow (zMapXRemoteObj object)
{
  int r = 1;                    /* failure */
  r = zmapXRemoteCmpAtomString(object, object->version_sanity_atom, ZMAP_XREMOTE_CURRENT_VERSION);
  if(object->remote_app)
    r += zmapXRemoteCmpAtomString(object, object->app_sanity_atom, object->remote_app);
  return r;
}

static int zmapXRemoteCmpAtomString (zMapXRemoteObj object, Atom atom, char *expected)
{
  Atom type;
  int format, x_status, unmatched;
  unsigned long nitems, bytesafter;
  unsigned char *versionStr = 0;
  Window win;
  unmatched = 0; 			/* success */

  win = object->window_id;

  zmapXTrapErrors();
  zmapXRemoteErrorStatus = ZMAPXREMOTE_PRECOND;
  x_status = XGetWindowProperty (object->display, win, 
                                 atom,
				 0, (65536 / sizeof (long)),
				 False, XA_STRING,
				 &type, &format, &nitems, &bytesafter,
				 &versionStr);
  zmapXUntrapErrors();

  if(windowError)
    return 4;                   /* failure */

  if (x_status != Success || !versionStr)
    {
      zmapXRemoteSetErrMsg(ZMAPXREMOTE_PRECOND, 
                           ZMAP_XREMOTE_META_FORMAT
                           ZMAP_XREMOTE_ERROR_FORMAT,
                           XDisplayString(object->display),                           
                           (unsigned int) win,
                           "",
                           "not a valid remote-controllable window"
                           );
      zmapXDebug("Warning : window 0x%x is not a valid remote-controllable window.\n",	       
               (unsigned int) win
               );
      return 2;			/* failure */
    }

  if (strcmp ((char*)versionStr, expected) != 0)
    {
      zmapXRemoteSetErrMsg(ZMAPXREMOTE_PRECOND, 
                           ZMAP_XREMOTE_META_FORMAT
                           ZMAP_XREMOTE_ERROR_START
                           "remote controllable window uses"
                           " different version %s of remote"
                           " control system, expected version %s."
                           ZMAP_XREMOTE_ERROR_END,
                           XDisplayString(object->display),                           
                           (unsigned int) win, "",
                           versionStr, expected);
      zmapXDebug("Warning : remote controllable window 0x%x uses "
	       "different version %s of remote control system, expected "
	       "version %s.\n",
	       (unsigned int) win,
	       versionStr, expected);
      unmatched = 8;
    }

  XFree (versionStr);

  return unmatched;
}

static char *zmapXRemoteGetComputedContent(zMapXRemoteObj object, Atom atom, Bool atomic_delete)
{
  Atom type;
  int format;
  unsigned long nitems, bytesafter;
  unsigned char *computedString = 0;
  int x_status;
  Window win;
  char *atom_content;

  win = object->window_id;

  zmapXTrapErrors();
  zmapXRemoteErrorStatus = ZMAPXREMOTE_INTERNAL;
  
  x_status = XGetWindowProperty (object->display, win, 
                                 atom,
				 0, (65536 / sizeof (long)),
				 atomic_delete, XA_STRING,
				 &type, &format, &nitems, &bytesafter,
				 &computedString);
  zmapXUntrapErrors();
  if(windowError || x_status != Success)
    {
      zmapXRemoteSetErrMsg(ZMAPXREMOTE_INTERNAL, 
                           ZMAP_XREMOTE_META_FORMAT
                           ZMAP_XREMOTE_ERROR_START
                           "couldn't get the content from the atom with name %s"
                           ZMAP_XREMOTE_ERROR_END,
                           XDisplayString(object->display),
                           win, "",
                           zmapXRemoteGetAtomName(object, atom)
                           );
      return zmapXRemoteGetErrorAsResponse();
    }
  //////////////////#ifdef EEEEEEEEEEEEEEEEE
  else if(type != XA_STRING)
    {
      zmapXRemoteSetErrMsg(ZMAPXREMOTE_INTERNAL, 
                           ZMAP_XREMOTE_META_FORMAT
                           ZMAP_XREMOTE_ERROR_START
                           "couldn't get the atom with name %s"
                           " and type STRING, got type 0x%0x"
                           ZMAP_XREMOTE_ERROR_END,
                           XDisplayString(object->display),
                           win, "",
                           zmapXRemoteGetAtomName(object, atom), type
                           );
      return zmapXRemoteGetErrorAsResponse();      
    }
  ////////////#endif
  else 
    {
      if(computedString && *computedString)
        {
          zmapXDebug("atom %s, value %s, \n", zmapXRemoteGetAtomName(object, atom), computedString);
          atom_content = g_strdup((char *)computedString);
          return atom_content;
        }
      else
        {
          zmapXDebug("%s\n", "empty atom");
          return "";
        }
    }
  if(computedString)
    XFree(computedString);

  return "";
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
  if(zmapXRemoteErrorText != NULL)
    g_free(zmapXRemoteErrorText);
  zmapXRemoteErrorText = NULL;
  return ;
}
static void zmapXRemoteSetErrMsg(zMapXRemoteStatus status, char *msg, ...)
{
  va_list args;
  char *callErr;

  if(zmapXRemoteErrorText != NULL)
    g_free(zmapXRemoteErrorText);

  /* Format the supplied error message. */
  va_start(args, msg) ;
  callErr = g_strdup_vprintf(msg, args) ;
  va_end(args) ;


  zmapXRemoteErrorStatus = status;
  /* zmapXRemoteErrorText   = g_strdup_printf(ZMAP_XREMOTE_ERROR_FORMAT, 
     callErr) ; */
  zmapXRemoteErrorText   = g_strdup( callErr ) ;

  g_free(callErr);

  return;
}


static int zmapXErrorHandler(Display *dpy, XErrorEvent *e )
{
    char errorText[1024];

    windowError = True;

    XGetErrorText( dpy, e->error_code, errorText, sizeof(errorText) );
    /* N.B. The continuation of string here */
    zmapXRemoteSetErrMsg(zmapXRemoteErrorStatus, 
                         ZMAP_XREMOTE_META_FORMAT
                         ZMAP_XREMOTE_ERROR_FORMAT,
                         XDisplayString(dpy), e->resourceid, "", errorText);

    zmapXDebug("%s\n","**********************************");
    zmapXDebug("X Error: %s\n", errorText);
    zmapXDebug("%s\n","**********************************");

    zMapLogWarning("%s","**********************************");
    zMapLogWarning("X Error: %s", errorText);
    zMapLogWarning("%s","**********************************");

    return 1;                   /* This is ignored by the server */
}

static void zmapXTrapErrors(void)
{
  windowError = False;
  XSetErrorHandler(zmapXErrorHandler);
  return ;
}
static void zmapXUntrapErrors(void)
{
  XSetErrorHandler(NULL);
  if(!windowError)
    zmapXRemoteErrorStatus = ZMAPXREMOTE_INTERNAL;
  return ;
}

/* ======================================================= */
/* END */
/* ======================================================= */
