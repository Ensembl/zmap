/*  File: zmapXRemote_P.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: None
 *-------------------------------------------------------------------
 */
#ifndef ZMAPXREMOTE_P_H
#define ZMAPXREMOTE_P_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ZMap/zmapUtils.h>     /* For logging. */
#include <ZMap/zmapXRemote.h>            /* Public header */


typedef struct _ZMapXRemoteObjStruct
{
  Display *display ;					    /* The display XOpenDisplay() succeeded in creating this */
  Window window_id ;					    /* The window id of either ourselves (server) or
							       foreign window (client) */

  Atom version_sanity_atom ;				    /* The atom to check version sanity */
  Atom app_sanity_atom ;				    /* The atom to check application sanity */
  Atom request_atom ;					    /* The request atom */
  Atom response_atom ;					    /* The response atom */

  char *remote_app ;

  gboolean init_called ; /* Just keeps track of initialisation, shouldn't really be needed */
  gboolean is_server ;   /* TRUE if we're a server */

  gdouble timeout ;					    /* timeout: how long to wait in
							       seconds for a response (default 30s). */
} ZMapXRemoteObjStruct ;

/*========= For Error Stuff below =========== */
typedef struct
{
  ZMapXRemoteStatus status ;
  char *message ;
} zMapXRemoteSimpleErrMsgStruct, *zMapXRemoteSimpleErrMsg ;


/*====================== DEBUGGING =========================*/

extern char *ZMAP_X_PROGRAM_G ;


#define zmapXDebug(FORMAT, ...)                           \
G_STMT_START {                                             \
  if (debug_G)                                             \
    g_printerr("[%s] [" ZMAP_MSG_FORMAT_STRING "] " FORMAT,	\
	       ZMAP_X_PROGRAM_G,				\
	       ZMAP_MSG_FUNCTION_MACRO,				\
	       __VA_ARGS__) ;					\
} G_STMT_END

/*==========================================================*/


/*==================================================================*/
/* Xlib error trapping stuff. Needed to stop bombing by X on errors */

static void zmapXTrapErrors(char *where, char *what, char *text) ;
static void zmapXUntrapErrors(void) ;
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
static ZMapXRemoteStatus zmapXRemoteErrorStatus = ZMAPXREMOTE_INTERNAL;
static char *zmapXRemoteErrorText = NULL;
static char *zmapXRemoteRawErrorText = NULL;
static void zmapXRemoteSetErrMsg(ZMapXRemoteStatus status, char *msg, ...);
static void zmapXRemoteResetErrMsg(void);
static char *zmapXRemoteGetErrorAsResponse(void);
/* End of Xlib Error stuff                                  */
/*==========================================================*/

#endif  /* ZMAPXREMOTE_P_H */
