/*  File: zmapApp_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2014: Genome Research Ltd.
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
 * and was written by
 *     Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *       Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *  Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Private header for application level of zmap.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_APP_PRIV_H
#define ZMAP_APP_PRIV_H

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapRemoteControl.h>
#include <ZMap/zmapAppRemote.h>
#include <ZMap/zmapManager.h>
#include <ZMap/zmapAppServices.h>


/* Minimum GTK version supported. */
enum {ZMAP_GTK_MAJOR = 2, ZMAP_GTK_MINOR = 2, ZMAP_GTK_MICRO = 4} ;


/* States for the application, needed especially because we can be waiting for threads to die. */
typedef enum
  {
    ZMAPAPP_INIT,					    /* Initialising, no threads. */
    ZMAPAPP_RUNNING,					    /* Normal execution. */
    ZMAPAPP_DYING					    /* Waiting for child ZMaps to dies so
							       app can exit. */
  } ZMapAppState ;


/* Some basic defines/defaults.. */
enum
  {
    ZMAP_DEFAULT_EXIT_TIMEOUT = 10,			    /* Default time out (in seconds) for
							       zmap to wait to quit, if exceeded we crash out */
    ZMAP_DEFAULT_PING_TIMEOUT = 10,

    ZMAP_WINDOW_TIMEOUT_MS = 2000,			    /* Length of timeout in milliseconds.  */

    ZMAP_WINDOW_RETRIES = 20,				    /* How many retries of window id when
							       we timeout for a command. */

    ZMAP_APP_REMOTE_TIMEOUT_S = 3600,                       /* How long to wait before warning user that
                                                               zmap has no sequence displayed and has
                                                               had no interaction with its
                                                               peer (seconds). */

    ZMAP_DEFAULT_MAX_LOG_SIZE = 100                         /* Max size of log file before we
                                                             * start warning user that log file is very
                                                             * big (in megabytes). */
  } ;



/* cols in connection list. */
enum {ZMAP_NUM_COLS = 4} ;

enum {
  ZMAPID_COLUMN,
  ZMAPSEQUENCE_COLUMN,
  ZMAPSTATE_COLUMN,
  ZMAPLASTREQUEST_COLUMN,
  ZMAPDATA_COLUMN,
  ZMAP_N_COLUMNS
};



/* General CB function typedef. */
typedef void (*ZMapAppCBFunc)(void *cb_data) ;


/* Struct for zmap's remote control object. */
typedef struct _ZMapAppRemoteStruct
{
  /* Our names/text etc. */
  char *app_id ;					    /* zmap app name. */
  char *app_unique_id ;

  Window app_window ;
  char *app_window_str ;

  /* Peer's names/text etc. */
  char *peer_name ;
  char *peer_clipboard ;

  /* If our peer gave us a window id in the handshake then we check that window
   * id to see if it is still there if we have timed out. 
   * We only do this a configurable number of times before really timing out (to avoid deadlock),
   * but note that counter is reset for each phase of sending/receiving. */
  Window peer_window ;
  char *peer_window_str ;
  int window_retries_max ;
  int window_retries_left ;

  /* When operating with a peer we have an overall timeout time, this allows us to
   * warn the user if we have no view displayed and there has been no activity with
   * the peer for a long time. */
  guint inactive_timeout_interval_s ;                       /* Time out interval. */
  guint inactive_func_id ;                                  /* glib timeout handler func. id. */
  time_t last_active_time_s ;                               /* Last remote request, in or out. */


  /* There are some requests that can only be serviced _after_ we are sure the peer
   * has received our reply to their request e.g. "shutdown" where we can't exit
   * otherwise peer will never receive our reply. */
  gboolean deferred_action ;

  ZMapAppCBFunc exit_routine ;				    /* Called to exit application after we
							       have signalled to peer we are going. */

  ZMapRemoteControl remote_controller ;


  /* Incoming request FROM a peer. */

  char *curr_peer_command ;
  char *curr_peer_request ;				    /* DO WE NEED TO CACHE THIS ?? */

  ZMapView curr_view_id ;


  /* Remote Control callback function that we call with our reply to a request. */
  ZMapRemoteControlReturnReplyFunc return_reply_func ;
  void *return_reply_func_data ;


  /* wrong ??? */
  /* Reply from peer to a request from zmap, zmap_reply_func() is zmap sub-system callback
   * that gets called with curr_reply. */
  char *curr_reply ;


  /* Outgoing request TO a peer */

  char *curr_zmap_command ;
  char *curr_zmap_request ;



  /* App function to be called with peer's reply to the request. */
  ZMapRemoteAppProcessReplyFunc process_reply_func ;
  gpointer process_reply_func_data ;

} ZMapAppRemoteStruct, *ZMapAppRemote ;





/* Overall application control struct. */
typedef struct _ZMapAppContextStruct
{
  ZMapAppState state ;					    /* Needed to control exit in a clean way. */

  char *app_id ;					    /* zmap app name. */

  int exit_timeout ;					    /* time (s) to wait before forced exit. */

  /* If requested zmap will ping it's peer every ping_timeout seconds (approx), stop_pinging is
   * used by functions to tell the ping timeout callback to stop. */
  int ping_timeout ;
  gboolean stop_pinging ;

  int exit_rc ;
  char *exit_msg ;


  GtkWidget *app_widg ;

  gboolean abbrev_title_prefix ;			    /* Is window title prefix abbreviated ? */

  GtkTreeStore *tree_store_widg ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Is this needed any more ?? */
  GError *info;                 /* This is an object to hold a code
                                 * and a message as info for the
                                 * remote control simple IPC stuff */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  ZMapManager zmap_manager ;
  ZMap selected_zmap ;



  /* The func id for the map-event callback used to call functions that need
   * to be executed once the app has an X Window, e.g. remote control. */
  gulong mapCB_id ;


  /* The new xremote. */
  gboolean remote_ok ;					    /* Set to FALSE if communication with peer fails. */
  ZMapAppRemote remote_control ;


  gboolean show_mainwindow ;				    /* Should main window be displayed. */
  gboolean defer_hiding ;				    /* Should hide of main window be deferred ? */


  /* Was a default sequence specified in the config. file.*/
  ZMapFeatureSequenceMap default_sequence;

  char *locale;

  char *script_dir;					    /* where scripts are kept for the pipeServer module
							     * can be set in [ZMap] or defaults to run-time directory
							     */

  gboolean xremote_debug ;				    /* Turn on/off debugging for xremote connections. */


} ZMapAppContextStruct, *ZMapAppContext ;


int zmapMainMakeAppWindow(int argc, char *argv[]) ;
GtkWidget *zmapMainMakeMenuBar(ZMapAppContext app_context) ;
GtkWidget *zmapMainMakeConnect(ZMapAppContext app_context, ZMapFeatureSequenceMap sequence_map) ;
GtkWidget *zmapMainMakeManage(ZMapAppContext app_context) ;
gboolean zmapAppCreateZMap(ZMapAppContext app_context, ZMapFeatureSequenceMap sequence_map,
			   ZMap *zmap_out, ZMapView *view_out, char **err_msg_out) ;
void zmapAppExit(ZMapAppContext app_context) ;

void zmapAppPingStart(ZMapAppContext app_context) ;
void zmapAppPingStop(ZMapAppContext app_context) ;

/* New remote control interface */
ZMapRemoteAppMakeRequestFunc zmapAppRemoteControlGetRequestCB(void) ;
gboolean zmapAppRemoteControlCreate(ZMapAppContext app_context,
				    char *peer_name, char *peer_clipboard, int peer_retries, int peer_timeout_ms) ;
gboolean zmapAppRemoteControlInit(ZMapAppContext app_context) ;
gboolean zmapAppRemoteControlConnect(ZMapAppContext app_context) ;
gboolean zmapAppRemoteControlDisconnect(ZMapAppContext app_context, gboolean app_exit) ;
gboolean zmapAppRemoteControlPing(ZMapAppContext app_context) ;
void zmapAppRemoteControlSetExitRoutine(ZMapAppContext app_context, ZMapAppCBFunc exit_routine) ;
void zmapAppProcessAnyRequest(ZMapAppContext app_context, char *request,
			      ZMapRemoteAppReturnReplyFunc replyHandlerFunc) ;
void zmapAppProcessAnyReply(ZMapAppContext app_context, char *reply) ;
void zmapAppRemoteControlOurRequestEndedCB(void *user_data) ;
void zmapAppRemoteControlTheirRequestEndedCB(void *user_data) ;
void zmapAppRemoteControlDestroy(ZMapAppContext app_context) ;


#endif /* !ZMAP_APP_PRIV_H */
