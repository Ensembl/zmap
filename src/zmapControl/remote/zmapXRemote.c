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
 * Last edited: May  6 13:34 2005 (rds)
 * Created: Wed Apr 13 19:04:48 2005 (rds)
 * CVS info:   $Id: zmapXRemote.c,v 1.2 2005-05-07 18:15:56 rds Exp $
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
                               "<message>Failed to open display '%s'</message>", 
                               display_str);
          free(display_str);
          free(object);
          return NULL;
        }
      object->init_called = FALSE;
      object->is_server   = FALSE;
    }

  //  XSynchronize(object->display, True);

  return object;
}

void zMapXRemoteSetWindowID(zMapXRemoteObj object, Window id)
{
  object->window_id = id;
  zmapXDebug("setting id to '0x%x' \n", (unsigned int) (object->window_id));
  return;
}

void zMapXRemoteSetRequestAtomName(zMapXRemoteObj object, char *name)
{
  zmapXDebug("zMapXRemoteSetRequestAtomName change to '%s'\n", name);
  object->request_atom = XInternAtom (object->display, name, False);
  zmapXDebug("New name is %s\n", zmapXRemoteGetAtomName(object, object->request_atom));
  return ;
}

void zMapXRemoteSetResponseAtomName(zMapXRemoteObj object, char *name)
{
  zmapXDebug("zMapXRemoteSetResponseAtomName change to '%s'\n", name);  
  object->response_atom = XInternAtom(object->display, name, False);
  zmapXDebug("New name is %s\n", zmapXRemoteGetAtomName(object, object->response_atom));
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
      object->version_sanity_atom = XInternAtom (object->display, ZMAP_XREMOTE_CURRENT_VERSION_ATOM, False);
      zmapXRemoteChangeProperty(object, object->version_sanity_atom, ZMAP_XREMOTE_CURRENT_VERSION);
    }
  if (! object->app_sanity_atom)
    {
      object->app_sanity_atom = XInternAtom(object->display, ZMAP_XREMOTE_APPLICATION_ATOM, False);
      zmapXRemoteChangeProperty(object, object->app_sanity_atom, appName);
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
    return result;

  result = zmapXRemoteCheckWindow(object);

  if(result != 0)
    return 9;

  zmapXDebug("remote: (writing %s '%s' to 0x%x)\n",
             zmapXRemoteGetAtomName(object, object->request_atom),
             command, (unsigned int) object->window_id);

  result = zmapXRemoteChangeProperty(object, object->request_atom, command);
  
  XSelectInput(object->display, object->window_id, event_mask);

  zmapXDebug("sent '%s'...\n", command);

  while (!isDone && !windowError)
    {
      zmapXDebug("%s"," - while: I'm still waiting...\n");

      //      if(XPending(dpy))
      XNextEvent (object->display, &event);

      zmapXDebug(" - while: got event type %d\n", event.type);

      if (windowError){
        result = 6;
        goto DONE;
      }

      if (event.xany.type == DestroyNotify &&
	  event.xdestroywindow.window == window)
	{
          zmapXRemoteSetErrMsg(ZMAPXREMOTE_UNAVAILABLE, 
                               "<display>%s</display>"
                               "<windowid>0x%0x</windowid>"
                               "<message>window was destroyed</message>", 
                               XDisplayString(dpy), window);
	  zmapXDebug("remote : window 0x%x was destroyed.\n",
		   (unsigned int) object->window_id);
	  result = 6;		/* invalid window */
	  goto DONE;
	}
      else if (event.xany.type        == PropertyNotify &&
	       event.xproperty.state  == PropertyNewValue &&
	       event.xproperty.window == object->window_id &&
	       event.xproperty.atom   == object->response_atom)
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
                                   "<display>%s</display>"
                                   "<windowid>0x%0x</windowid>"
                                   "<message>failed reading atom '%s' from window</message>", 
                                   XDisplayString(dpy), window, 
                                   zmapXRemoteGetAtomName(object, object->response_atom));
	      zmapXDebug("remote: failed reading %s from window 0x%0x.\n",
                         zmapXRemoteGetAtomName(object, object->response_atom),
                       (unsigned int) object->window_id);
	      result = 6;	/* invalid window */
	      isDone = True;
	    }
	  else
	    {
	      if (commandResult && *commandResult)
		{
		  zmapXDebug("cmd result| %s\n", commandResult);
		  result = 0;	/* everything OK */
		  isDone = True;
		}
	      else
		{
                  zmapXRemoteSetErrMsg(ZMAPXREMOTE_CONFLICT, 
                                       "<display>%s</display>"
                                       "<windowid>0x%0x</windowid>"
                                       "<message>invalid data on atom '%s'</message>", 
                                       XDisplayString(dpy), window,
                                       zmapXRemoteGetAtomName(object, object->response_atom));
		  zmapXDebug("remote: invalid data on %s property of window 0x%0x.\n",
                             zmapXRemoteGetAtomName(object, object->response_atom),
                             (unsigned int) object->window_id);
		  result = 6;	/* invalid window */
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

char *zMapXRemoteGetResponse(zMapXRemoteObj object)
{
  zmapXDebug("%s\n", "just a message to say we're in zMapXRemoteGetResponse");

  if(windowError)
    {
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
      gchar *response_text;
      char *copy_command ;

      copy_command = g_malloc0(nitems + 1) ;
      memcpy(copy_command, command_text, nitems);
      copy_command[nitems] = 0 ;			    /* command_text is not zero terminated */


      /* This just proxies the commands to the right place and gets a reply */
      response_text = (notifyStruct->callback)((char *)copy_command, user_data) ; 

      zMapAssert(response_text); /* Need an answer */

      zMapXRemoteSetReply(xremote, response_text);

      g_free(response_text) ;
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

char *zmapXRemoteGetAtomName(zMapXRemoteObj obj, Atom atom)
{
  char *name;
  name = g_strdup(XGetAtomName(obj->display, atom));
  return name;
}

int zmapXRemoteChangeProperty(zMapXRemoteObj object, Atom atom, char *change_to)
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

int zmapXRemoteCheckWindow (zMapXRemoteObj object)
{
  int r = 1;                    /* failure */
  r = zmapXRemoteCmpAtomString(object, object->version_sanity_atom, ZMAP_XREMOTE_CURRENT_VERSION);
  if(object->remote_app)
    r += zmapXRemoteCmpAtomString(object, object->app_sanity_atom, object->remote_app);
  return r;
}

int zmapXRemoteCmpAtomString (zMapXRemoteObj object, Atom atom, char *expected)
{
  Atom type;
  int format, x_status;
  unsigned long nitems, bytesafter;
  unsigned char *versionStr = 0;
  Window win;

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
                           "Warning : window 0x%x is not a valid remote-controllable window.",
                           (unsigned int) win
                           );
      zmapXDebug("Warning : window 0x%x is not a valid remote-controllable window.\n",	       
               (unsigned int) win
               );
      return 2;			/* failure */
    }

  if (strcmp ((char*)versionStr, expected) != 0)
    {
      zmapXRemoteSetErrMsg(ZMAPXREMOTE_PRECOND, 
                           "Warning : remote controllable window 0x%x uses "
                           "different version %s of remote control system, expected "

                           "version %s.",
                           (unsigned int) win,
                           versionStr, expected);
      zmapXDebug("Warning : remote controllable window 0x%x uses "
	       "different version %s of remote control system, expected "
	       "version %s.\n",
	       (unsigned int) win,
	       versionStr, expected);
    }

  XFree (versionStr);

  return 0;			/* success */
}

char *zmapXRemoteGetComputedContent(zMapXRemoteObj object, Atom atom, Bool atomic_delete)
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
                           "couldn't get the content from the atom with name %s",
                           zmapXRemoteGetAtomName(object, atom)
                           );
      return zmapXRemoteGetErrorAsResponse();
    }
  //////////////////#ifdef EEEEEEEEEEEEEEEEE
  else if(type != XA_STRING)
    {
      zmapXRemoteSetErrMsg(ZMAPXREMOTE_INTERNAL, 
                           "couldn't get the atom with name %s and type STRING, got type 0x%0x",
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

#define xml_simple_start "<simplemessage>"
#define xml_simple_end   "</simplemessage>"

/* =============================================================== */
char *zmapXRemoteGetErrorAsResponse(void) /* Translation for users */
/* =============================================================== */
{
  char *err;

  if(zmapXRemoteErrorText == NULL)
    {
      zMapXRemoteSimpleErrMsgStruct errorTable[] = 
        {
          /* 1xx  Informational */
          /* 2xx  Successful    */
          {ZMAPXREMOTE_METAERROR   , xml_simple_start "meta error" xml_simple_end},
          {ZMAPXREMOTE_NOCONTENT   , xml_simple_start "no content was returned" xml_simple_end}, 
          /* 3xx  Redirection   */
          /* Redirect??? I don't think so */          
          /* 4xx  Client Errors */
          {ZMAPXREMOTE_BADREQUEST  , xml_simple_start "you made a bad request to the server" xml_simple_end},
          {ZMAPXREMOTE_FORBIDDEN   , xml_simple_start "you are not allowed to see this" xml_simple_end},
          {ZMAPXREMOTE_UNKNOWNCMD  , xml_simple_start "unknown command" xml_simple_end},          
          {ZMAPXREMOTE_PRECOND     , xml_simple_start "something went wrong with sanity checking" xml_simple_end},          
          /* 5xx  Server Errors  */
          {ZMAPXREMOTE_INTERNAL    , xml_simple_start "Internal Server Error" xml_simple_end},
          {ZMAPXREMOTE_UNKNOWNATOM , xml_simple_start "Unknown Server Atom" xml_simple_end},
          {ZMAPXREMOTE_NOCREATE    , xml_simple_start "No Resource was created" xml_simple_end},
          {ZMAPXREMOTE_UNAVAILABLE , xml_simple_start "Unavailable Server Resource" xml_simple_end},
          {ZMAPXREMOTE_TIMEDOUT    , xml_simple_start "Server Timeout" xml_simple_end},
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
#undef xml_simple_start
#undef xml_simple_end


void zmapXRemoteSetErrMsg(zMapXRemoteStatus status, char *msg, ...)
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
  zmapXRemoteErrorText   = g_strdup_printf(ZMAP_XREMOTE_ERROR_XML_FORMAT, 
                                           callErr) ;

  g_free(callErr);

  return;
}


int zmapXErrorHandler(Display *dpy, XErrorEvent *e )
{
    char errorText[1024];

    windowError = True;

    XGetErrorText( dpy, e->error_code, errorText, sizeof(errorText) );
    /* N.B. The continuation of string here */
    zmapXRemoteSetErrMsg(zmapXRemoteErrorStatus, 
                         "<display>%s</display>"
                         "<windowid>0x%0x</windowid>"
                         "<message>%s</message>",
                         XDisplayString(dpy), e->resourceid, errorText);

    zmapXDebug("%s\n","**********************************");
    zmapXDebug("X Error: %s\n", errorText);
    zmapXDebug("%s\n","**********************************");

    return 1;                   /* This is ignored by the server */
}

void zmapXTrapErrors(void)
{
  windowError = False;
  XSetErrorHandler(zmapXErrorHandler);
  return ;
}
void zmapXUntrapErrors(void)
{
  XSetErrorHandler(NULL);
  if(!windowError)
    zmapXRemoteErrorStatus = ZMAPXREMOTE_INTERNAL;
  return ;
}

/* ======================================================= */
/* END */
/* ======================================================= */



#ifdef AN_ALTERNATE_ZMAPXREMOTEGETCOMPUTEDCONTENT
static Bool
zmapXRemoteGetComputedContent (zMapXRemoteObj object, Atom atom, char *content)
{
  Atom type;
  int format;
  unsigned long nitems, bytesafter;
  unsigned char *computedString = 0;
  int x_status;
  char *computedString;
  int result;

  *content = 0;

  zmapXTrapErrors();
  zmapXRemoteErrorStatus = ZMAPXREMOTE_INTERNAL;
  result = XGetWindowProperty (object->display,
                               object->window_id,
                               atom,
                               0, (65536 / sizeof (long)),
                               False, XA_STRING, 
                               &type, &format, &nitems,
                               &bytes_after, &computedString);  
  zmapXUntrapErrors();
  if (windowError || result != Success)
    return False;
  
  if (type != XA_STRING)
    {
      XFree (computedString);
      return False;
    }

  *content = *computedString;
  
  XFree (computedString);

  return True;
}
#endif

