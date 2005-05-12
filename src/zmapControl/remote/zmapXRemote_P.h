/*  File: zmapXRemote_P.h
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
 * Exported functions: None
 * HISTORY:
 * Last edited: May  9 15:05 2005 (rds)
 * Created: Thu Apr 14 13:07:51 2005 (rds)
 * CVS info:   $Id: zmapXRemote_P.h,v 1.3 2005-05-12 16:03:44 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAPXREMOTE_P_H
#define ZMAPXREMOTE_P_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include <ZMap/zmapXRemote.h>            /* Public header */
#include <ZMap/zmapUtils.h>

typedef struct _zMapXRemoteObjStruct
{
  Display *display; /* The display XOpenDisplay() succeeded in creating this */
  Window window_id; /* The window id of either ourselves (server) or
                       foreign window (client)*/

  Atom version_sanity_atom;     /* The atom to check version sanity */
  Atom app_sanity_atom;         /* The atom to check application sanity */
  Atom request_atom;            /* The request atom */
  Atom response_atom;           /* The response atom */

  char *remote_app;

  gboolean init_called; /* Just keeps track of initialisation, shouldn't really be needed */
  gboolean is_server;   /* TRUE if we're a server */

} zMapXRemoteObjStruct ;

/*========= For Error Stuff below =========== */
typedef struct
{
  zMapXRemoteStatus status ;
  char *message ;
} zMapXRemoteSimpleErrMsgStruct, *zMapXRemoteSimpleErrMsg ;


/*========= Some Private functions ===========*/
static char *zmapXRemoteGetComputedContent(zMapXRemoteObj object, Atom atom, Bool atomic_delete);
static char *zmapXRemoteGetAtomName(zMapXRemoteObj obj, Atom atom);

static int zmapXRemoteCheckWindow   (zMapXRemoteObj object);
static int zmapXRemoteCmpAtomString (zMapXRemoteObj object, Atom atom, char *expected);
static int zmapXRemoteChangeProperty(zMapXRemoteObj object, Atom atom, char *change_to);


/*====================== DEBUGGING =========================*/
//////////////////////////////////////////#define DO_DEBUGGING
#ifdef DO_DEBUGGING
#define ZMAP_MSG_FORMAT_STRING  "(%s, line %d) - "
#define zmapXDebug(FORMAT, ...)                           \
G_STMT_START{                                             \
       g_printerr(ZMAP_MSG_FORMAT_STRING FORMAT,          \
		  __FILE__,			          \
		  __LINE__,				  \
		  __VA_ARGS__) ;                          \
}G_STMT_END
#else
#define zmapXDebug(FORMAT, ...)
#endif
/*==========================================================*/


/*==================================================================*/
/* Xlib error trapping stuff. Needed to stop bombing by X on errors */
static Bool windowError = False;

static void zmapXTrapErrors(void);
static void zmapXUntrapErrors(void);
static int  zmapXErrorHandler(Display *dpy, XErrorEvent *e);

/* This is quite nauseating...  
 *
 * 1. you need to set zmapXRemoteErrorStatus after zmapXTrapErrors and
 *    before call that might fail.
 *
 * 2. zmapXErrorHandler passes zmapXRemoteErrorStatus to
 *    zmapXRemoteSetErrMsg which just sets it to the same thing again.
 * 
 * 3. behaviour of zmapXUntrapErrors() depends on value of windowError.
 *
 * 4. So typical usage is:
 *    zmapXTrapErrors();
 *    zmapXRemoteErrorStatus = ZMAPXREMOTE_(error context code bit) unless ZMAPXREMOTE_INTERNAL is appropriate
 *    XLibraryCall(...);
 *    zmapXUntrapErrors();
 *    if(windowError)
 *      return zmapXRemoteErrorStatus;

 *  However, this is preferable to using gdk calls for a couple of reasons.
 *  1. we get our own error strings.
 *  2. we get the error string from XErrorEvent rather than just an error code
 *  Typical gdk version is basically the same
 *    gdk_error_trap_push ();
 *    XLibraryCall(...);
 *    gdk_flush ();

 *    if (code = gdk_error_trap_pop ())
 *    {
 *       // we'd end up calling this every time, rather than the above, which I
 *       // consider to be a bit better encapsulated.
 *       char errorText[1024];
 *       XGetErrorText(gdk_x11_get_default_xdisplay(), code, errorText, sizeof(errorText));
 *    }
 *
 *
 */
static zMapXRemoteStatus zmapXRemoteErrorStatus = ZMAPXREMOTE_INTERNAL;
static char *zmapXRemoteErrorText = NULL;
static void zmapXRemoteSetErrMsg(zMapXRemoteStatus status, char *msg, ...);
static char *zmapXRemoteGetErrorAsResponse(void);
/* End of Xlib Error stuff                                  */
/*==========================================================*/

#endif  /* ZMAPXREMOTE_P_H */
