/*  Last edited: Feb 29 16:15 2012 (edgrif) */
/*  File: zmapApp.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 *     Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and,
 *          Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *       Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Private header for application level of zmap.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_APP_PRIV_H
#define ZMAP_APP_PRIV_H

#include <gtk/gtk.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapRemoteControl.h>
#include <ZMap/zmapAppRemote.h>
#include <ZMap/zmapManager.h>

#include <ZMap/zmapXRemote.h>


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


/* Default time out (in seconds) for zmap to wait to quit, if exceeded we crash out. */
enum {ZMAP_DEFAULT_EXIT_TIMEOUT = 10, ZMAP_DEFAULT_PING_TIMEOUT = 10} ;


/* Max size of log file before we start warning user that log file is very big (in megabytes). */
enum {ZMAP_DEFAULT_MAX_LOG_SIZE = 100} ;


/* General CB function typedef. */
typedef void (*ZMapAppCBFunc)(void *cb_data) ;




/*                   NEW XREMOTE                                  */


typedef struct _ZMapAppRemoteStruct
{
  char *app_id ;					    /* zmap app name. */
  char *app_unique_id ;

  char *peer_name ;
  char *peer_clipboard ;

  /* Currently shutdown is the only command that needs to run _after_ we are sure
   * the peer has received our reply.....if it turns out other commands need to do
   * this then we will need to store their names/params here instead of this simple
   * boolean. */
  gboolean deferred_shutdown ;

  ZMapAppCBFunc exit_routine ;				    /* Called to exit application after we
							       have signalled to peer we are going. */

  ZMapRemoteControl remote_controller ;

  /* Default id to send commands to. Initially NULL meaning only app level commands
   * will work, gets filled in with first zmap.view.window created. */
  ZMapAppRemoteViewIDStruct default_view_id ;


  /* Incoming request FROM a peer. */

  /* remote_reply_func() is RemoteControl callback that gets called
   * when we receive a reply from a zmap sub-system to this request. */
  char *curr_command ;
  char *curr_request ;					    /* DO WE NEED TO CACHE THIS ?? */


  ZMapAppRemoteViewIDStruct curr_view_id ;


  /* Remote Control callback function that we call with our reply to a request. */
  ZMapRemoteControlReturnReplyFunc return_reply_func ;
  void *return_reply_func_data ;


  /* wrong ??? */
  /* Reply from peer to a request from zmap, zmap_reply_func() is zmap sub-system callback
   * that gets called with curr_reply. */
  char *curr_reply ;


  /* Outgoing request TO a peer */

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

  GtkWidget *sequence_widg ;
  GtkWidget *start_widg ;
  GtkWidget *end_widg ;

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
  ZMapAppRemote remote_control ;


  /* old xremote stuff... */
  gulong property_notify_event_id;
  ZMapXRemoteObj xremote_client ;			    /* The external program we are sending
							       commands to. */
  gboolean xremote_debug ;				    /* Turn on/off debugging for xremote connections. */



  gboolean show_mainwindow ;				    /* Should main window be displayed. */

  /* Was a default sequence specified in the config. file.*/
  ZMapFeatureSequenceMap default_sequence;

  char *locale;
  gboolean sent_finalised ;

  char *script_dir;					    /* where scripts are kept for the pipeServer module
							     * can be set in [ZMap] or defaults to run-time directory
							     */

} ZMapAppContextStruct, *ZMapAppContext ;


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


int zmapMainMakeAppWindow(int argc, char *argv[]) ;
GtkWidget *zmapMainMakeMenuBar(ZMapAppContext app_context) ;
GtkWidget *zmapMainMakeConnect(ZMapAppContext app_context) ;
GtkWidget *zmapMainMakeManage(ZMapAppContext app_context) ;
gboolean zmapAppCreateZMap(ZMapAppContext app_context, ZMapFeatureSequenceMap sequence_map,
			   ZMap *zmap_out, ZMapView *view_out, char **err_msg_out) ;
void zmapAppExit(ZMapAppContext app_context) ;

void zmapAppPingStart(ZMapAppContext app_context) ;
void zmapAppPingStop(ZMapAppContext app_context) ;

/* old remote stuff.... */
void zmapAppRemoteInstaller(GtkWidget *widget, gpointer app_context_data);
void zmapAppRemoteSendFinalised(ZMapAppContext app_context);


/* New remote control interface */
ZMapRemoteAppMakeRequestFunc zmapAppRemoteControlGetRequestCB(void) ;
gboolean zmapAppRemoteControlCreate(ZMapAppContext app_context, char *peer_name, char *peer_clipboard) ;
gboolean zmapAppRemoteControlInit(ZMapAppContext app_context) ;
gboolean zmapAppRemoteControlConnect(ZMapAppContext app_context) ;
gboolean zmapAppRemoteControlDisconnect(ZMapAppContext app_context, gboolean app_exit) ;
gboolean zmapAppRemoteControlPing(ZMapAppContext app_context) ;
void zmapAppRemoteControlSetExitRoutine(ZMapAppContext app_context, ZMapAppCBFunc exit_routine) ;
gboolean zmapAppProcessAnyRequest(ZMapAppContext app_context, char *request,
				  ZMapRemoteAppReturnReplyFunc replyHandlerFunc) ;
void zmapAppProcessAnyReply(ZMapAppContext app_context, char *reply) ;
void zmapAppRemoteControlOurRequestEndedCB(void *user_data) ;
void zmapAppRemoteControlTheirRequestEndedCB(void *user_data) ;
void zmapAppRemoteControlDestroy(ZMapAppContext app_context) ;


#endif /* !ZMAP_APP_PRIV_H */
