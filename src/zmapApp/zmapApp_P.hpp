/*  File: zmapApp_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapControl.hpp>
#include <ZMap/zmapRemoteControl.hpp>
#include <ZMap/zmapAppRemote.hpp>
#include <ZMap/zmapAppServices.hpp>


#define INIT_FORMAT "Init - %s"
#define EXIT_FORMAT "Exit - %s"

#define ZMAPLOG_FILENAME "zmap.log"


/* define this to get some memory debugging. */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#define ZMAP_MEMORY_DEBUG
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* Minimum GTK version supported. */
enum {ZMAP_GTK_MAJOR = 2, ZMAP_GTK_MINOR = 20, ZMAP_GTK_MICRO = 0} ;


#define CLEAN_EXIT_MSG "terminated cleanly, goodbye cruel world !"





#define zmapAppConsoleLogMsg(BOOL, FORMAT, ...)                 \
  G_STMT_START                                                  \
  {                                                             \
    if ((BOOL))                                                 \
      {                                                         \
        zMapLogMessage((FORMAT), __VA_ARGS__) ;                 \
        zmapAppConsoleMsg(TRUE, (FORMAT), __VA_ARGS__) ;        \
      }                                                         \
  } G_STMT_END





/* States for the application, needed especially because we can be waiting for threads to die. */
typedef enum
  {
    ZMAPAPP_INIT,					    /* Initialising, no threads. */
    ZMAPAPP_RUNNING,					    /* Normal execution. */
    ZMAPAPP_DYING					    /* Waiting for child ZMaps to dies so
							       app can exit. */
  } ZMapAppState ;



/* Some app settings for timeouts etc. */
enum
  {
    ZMAP_DEFAULT_EXIT_TIMEOUT = 10,			    /* Default time out (in seconds) for
							       zmap to wait to quit, if exceeded we crash out */
    ZMAP_DEFAULT_PING_TIMEOUT = 10,

    ZMAP_APP_REMOTE_TIMEOUT_S = 3600,                       /* How long to wait before warning user that
                                                               zmap has no sequence displayed and has
                                                               had no interaction with its
                                                               peer (seconds). */

    ZMAP_DEFAULT_MAX_LOG_SIZE = 100                         /* Max size of log file before we
                                                             * start warning user that log file is very
                                                             * big (in megabytes). */
  } ;




/*
 * We follow glib convention in error domain naming:
 *          "The error domain is called <NAMESPACE>_<MODULE>_ERROR"
 */
#define ZMAP_APP_ERROR g_quark_from_string("ZMAP_APP_ERROR")

typedef enum
{
  ZMAPAPP_ERROR_BAD_SEQUENCE_DETAILS,
  ZMAPAPP_ERROR_BAD_COORDS,
  ZMAPAPP_ERROR_NO_SEQUENCE,
  ZMAPAPP_ERROR_GFF_HEADER,
  ZMAPAPP_ERROR_GFF_PARSER,
  ZMAPAPP_ERROR_GFF_VERSION,
  ZMAPAPP_ERROR_OPENING_FILE,
  ZMAPAPP_ERROR_CHECK_FILE,
  ZMAPAPP_ERROR_NO_SOURCES,
  ZMAPAPP_ERROR_SEQUENCE_MAP
} ZMapAppError;





/* ZMapAppManager: allows user to manage zmaps and their views. */

/* Opaque type, controls interaction with all current ZMap windows. */
typedef struct _ZMapManagerStruct *ZMapManager ;

/* Specifies result of trying to add a new zmap to manager. */
typedef enum
 {
   ZMAPMANAGER_ADD_INVALID,
   ZMAPMANAGER_ADD_OK,					    /* Add succeeded, zmap connected. */
   ZMAPMANAGER_ADD_FAIL,				    /* ZMap could not be added. */
   ZMAPMANAGER_ADD_DISASTER				    /* There has been a serious error,
							       caller should abort. */
 } ZMapManagerAddResult ;

/* Callback function prototype for zMapManagerForAllZMaps() */
typedef void (*ZMapManagerForAllCallbackFunc)(ZMap zmap, void *user_data) ;




/* Opaque type, controls remote peer interaction with all current ZMap windows. */
typedef struct ZMapAppRemoteStructType *ZMapAppRemote ;

/* Struct for zmap's remote control object. */
typedef struct ZMapAppRemoteStructType
{

  /* Peer's names/text etc. */
  const char *peer_name ;
  char *peer_socket ;

  /* Our names/text etc. */
  char *app_id ;					    /* zmap app name. */
  char *app_socket ;

  ZMapRemoteControl remote_controller ;


  ZMapAppRemoteExitFunc exit_routine ;                      /* Called to exit application after we
							       have signalled to peer we are going. */


  /* Remote Control callback function that we call with our reply to a request. */
  ZMapRemoteControlReturnReplyFunc return_reply_func ;
  void *return_reply_func_data ;


  /* App function to be called with peer's reply to the request. */
  ZMapRemoteAppProcessReplyFunc process_reply_func ;
  gpointer process_reply_func_data ;

  /* App function to be called if there was an error in sending the request, e.g. timeout. */
  ZMapRemoteAppErrorHandlerFunc error_handler_func ;
  gpointer error_handler_func_data ;


  /* Incoming request FROM a peer. */
  char *curr_peer_request ;				    /* DO WE NEED TO CACHE THIS ?? */
  char *curr_peer_command ;
  ZMapView curr_view_id ;


  /* Outgoing request TO a peer */
  char *curr_zmap_request ;



  /* There are some requests that can only be serviced _after_ we are sure the peer
   * has received our reply to their request e.g. "shutdown" where we can't exit
   * otherwise peer will never receive our reply. */
  gboolean deferred_action ;


  /* When operating with a peer we have an overall timeout time, this allows us to
   * warn the user if we have no view displayed and there has been no activity with
   * the peer for a long time. */
  guint inactive_timeout_interval_s ;                       /* Time out interval. */
  guint inactive_func_id ;                                  /* glib timeout handler func. id. */
  time_t last_active_time_s ;                               /* Last remote request, in or out. */


} ZMapAppRemoteStruct ;




/* Overall application control struct. */
typedef struct _ZMapAppContextStruct
{
  ZMapAppState state ;					    /* Needed to control exit in a clean way. */

  const char *app_id ;					    /* zmap app name. */

  gboolean verbose_startup_logging ;                        /* switches on lots of start/exit messages. */


  int exit_timeout ;					    /* time (s) to wait before forced exit. */

  /* If requested zmap will ping it's peer every ping_timeout seconds (approx), stop_pinging is
   * used by functions to tell the ping timeout callback to stop. */
  int ping_timeout ;
  gboolean stop_pinging ;

  int exit_rc ;
  const char *exit_msg ;

  gboolean abbrev_title_prefix ;			    /* Is window title prefix abbreviated ? */
  gboolean show_mainwindow ;				    /* Should main window be displayed. */
  gboolean defer_hiding ;				    /* Should hide of main window be deferred ? */

  /* Special colour to visually group zmap with it's peer window(s), colour is set by peer or user. */
  gboolean session_colour_set ;
  GdkColor session_colour ;

  /* App level cursors, busy cursors get propagated down through all widgets. */
  GdkCursor *normal_cursor ;
  GdkCursor *remote_busy_cursor ;


  GtkWidget *app_widg ;

  ZMapManager zmap_manager ;

  /* The list of opened zmaps. */
  GtkTreeStore *tree_store_widg ;
  ZMap selected_zmap ;
  ZMapView selected_view ;

  /* The new xremote. */
  gboolean remote_ok ;					    /* Set to FALSE if communication with peer fails. */
  ZMapAppRemote remote_control ;
  gboolean xremote_debug ;				    /* Turn on/off debugging for xremote
                                                               connections. */


  /* The func id for the map-event callback used to call functions that need
   * to be executed once the app has an X Window, e.g. remote control. */
  gulong mapCB_id ;


  /* Was a default sequence specified in the config. file.*/
  ZMapFeatureSequenceMap default_sequence;

  char *locale;

  char *script_dir;					    /* where scripts are kept for the pipeServer module
							     * can be set in [ZMap] or defaults to run-time directory
							     */

} ZMapAppContextStruct, *ZMapAppContext ;






gboolean zmapAppCreateZMap(ZMapAppContext app_context, ZMapFeatureSequenceMap sequence_map,
                           ZMap *zmap_out, ZMapView *view_out) ;
void zmapAppQuitReq(ZMapAppContext app_context) ;
void zmapAppRemoteLevelDestroy(ZMapAppContext app_context) ;
void zmapAppDestroy(ZMapAppContext app_context) ;
void zmapAppExit(ZMapAppContext app_context) ;

gboolean zmapMainMakeAppWindow(int argc, char *argv[], ZMapAppContext app_context, GList *seq_maps) ;
GtkWidget *zmapMainMakeMenuBar(ZMapAppContext app_context) ;
GtkWidget *zmapMainMakeConnect(ZMapAppContext app_context, ZMapFeatureSequenceMap sequence_map) ;
GtkWidget *zmapMainMakeManage(ZMapAppContext app_context) ;

/* Add in all the manager stuff here..... */
void zMapManagerInit(ZMapRemoteAppMakeRequestFunc remote_control_cb_func, void *remote_control_cb_data) ;
ZMapManager zMapManagerCreate(void *gui_data) ;
ZMapManagerAddResult zMapManagerAdd(ZMapManager zmaps, ZMapFeatureSequenceMap sequence_map,
                                    ZMap *zmap_out, ZMapView *view_out, GError **error) ;

ZMap zMapManagerFindZMap(ZMapManager manager, gpointer view_id, gpointer *view_ptr_out) ;
gpointer zMapManagerFindView(ZMapManager manager, gpointer view_id) ;
void zMapManagerDestroyView(ZMapManager zmaps, ZMap zmap, ZMapView view) ;
guint zMapManagerCount(ZMapManager zmaps);
gboolean zMapManagerReset(ZMap zmap) ;
gboolean zMapManagerRaise(ZMap zmap) ;
gboolean zMapManagerProcessRemoteRequest(ZMapManager manager,
					 char *command_name, char *request,
					 ZMap zmap, gpointer view_id,
					 ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data) ;
void zMapManagerForAllZMaps(ZMapManager manager, ZMapManagerForAllCallbackFunc user_func_cb, void *user_func_data) ;
gboolean zMapManagerCheckIfUnsaved(ZMapManager zmaps) ;
void zMapManagerKill(ZMapManager zmaps, ZMap zmap) ;
gboolean zMapManagerKillAllZMaps(ZMapManager zmaps);
gboolean zMapManagerDestroy(ZMapManager zmaps) ;
void zmapAppManagerUpdate(ZMapAppContext app_context, ZMap zmap, ZMapView view) ;
void zmapAppManagerRemove(ZMapAppContext app_context, ZMap zmap, ZMapView view) ;



void zmapAppConsoleMsg(gboolean err_msg, const char *format, ...) ;
void zmapAppDoTheExit(int exit_code) ;

void zmapAppSignalAppDestroy(ZMapAppContext app_context) ;

void zmapAppPingStart(ZMapAppContext app_context) ;
void zmapAppPingStop(ZMapAppContext app_context) ;

/* New remote control interface */
gboolean zmapAppRemoteControlIsAvailable(void) ;
ZMapRemoteAppMakeRequestFunc zmapAppRemoteControlGetRequestCB(void) ;
gboolean zmapAppRemoteControlCreate(ZMapAppContext app_context,
				    char *peer_socket, const char *peer_timeout_list) ;
gboolean zmapAppRemoteControlInit(ZMapAppContext app_context) ;
gboolean zmapAppRemoteControlConnect(ZMapAppContext app_context) ;
gboolean zmapAppRemoteControlDisconnect(ZMapAppContext app_context, gboolean app_exit) ;
gboolean zmapAppRemoteControlPing(ZMapAppContext app_context) ;
void zmapAppRemoteControlSetExitRoutine(ZMapAppContext app_context, ZMapAppRemoteExitFunc exit_routine) ;
void zmapAppProcessAnyRequest(ZMapAppContext app_context, char *request,
			      ZMapRemoteAppReturnReplyFunc replyHandlerFunc) ;
void zmapAppProcessAnyReply(ZMapAppContext app_context, char *reply) ;
void zmapAppRemoteControlReplySent(ZMapAppContext app_context) ;
void zmapAppRemoteControlOurRequestEndedCB(void *user_data) ;
void zmapAppRemoteControlTheirRequestEndedCB(void *user_data) ;
void zmapAppRemoteControlDestroy(ZMapAppContext app_context) ;


#endif /* !ZMAP_APP_PRIV_H */
