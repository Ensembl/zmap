/*  File: zmapAppwindow.c
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
 *     Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and,
 *       Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *  Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Creates the first toplevel window in the zmap app.
 *
 * Exported functions: None
 *-------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <locale.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


#include <ZMap/zmap.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsGUI.h>
#include <ZMap/zmapCmdLineArgs.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>
#include <ZMap/zmapControl.h>
#include <ZMap/zmapGFF.h>
#include <zmapApp_P.h>



/* define this to get some memory debugging. */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#define ZMAP_MEMORY_DEBUG
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#define CLEAN_EXIT_MSG "Exit clean - goodbye cruel world !"

static void checkForCmdLineVersionArg(int argc, char *argv[]) ;
static int checkForCmdLineSleep(int argc, char *argv[]) ;
static void checkForCmdLineSequenceArg(int argc, char *argv[], char **dataset_out, char **sequence_out, GError **error) ;
static void checkForCmdLineStartEndArg(int argc, char *argv[], int *start_inout, int *end_inout, GError **error) ;
static void checkConfigDir(char **config_file_out, char **styles_file_out) ;
static gboolean checkSequenceArgs(int argc, char *argv[],
                                  char *config_file, char *styles_file,
                                  GList **seq_maps_inout, GError **error) ;
static gboolean checkPeerID(char *config_file,
    char **peer_name_out, char **peer_clipboard_out, int *peer_retries, int *peer_timeout_ms) ;

static gboolean removeZMapRowForeachFunc(GtkTreeModel *model, GtkTreePath *path,
                                         GtkTreeIter *iter, gpointer data);


static void initGnomeGTK(int argc, char *argv[]) ;
static gboolean getConfiguration(ZMapAppContext app_context, char *config_file) ;

static ZMapAppContext createAppContext(void) ;
static void destroyAppContext(ZMapAppContext app_context) ;

static void addZMapCB(void *app_data, void *zmap) ;
static void removeZMapCB(void *app_data, void *zmap) ;
static void infoSetCB(void *app_data, void *zmap) ;
static void quitCB(GtkWidget *widget, gpointer cb_data) ;

void quitReqCB(void *app_data, void *zmap_data_unused) ;
static void toplevelDestroyCB(GtkWidget *widget, gpointer data) ;

static void appExit(void *user_data) ;
static gboolean timeoutHandler(gpointer data) ;
static void signalFinalCleanUp(ZMapAppContext app_context, int exit_rc, char *exit_msg) ;
static void exitApp(ZMapAppContext app_context) ;
static void crashExitApp(ZMapAppContext app_context) ;
static void finalCleanUp(ZMapAppContext app_context) ;
static void doTheExit(int exit_code) ;

static void setupSignalHandlers(void);

static void remoteInstaller(GtkWidget *widget, GdkEvent *event, gpointer app_context_data) ;
static gboolean remoteInactiveHandler(gpointer data) ;
static gboolean pingHandler(gpointer data) ;

static gboolean configureLog(char *config_file, GError **error) ;
static void consoleMsg(gboolean err_msg, char *format, ...) ;

static void hideMainWindow(ZMapAppContext app_context) ;




/*
 *                            Globals
 */


static ZMapManagerCallbacksStruct app_window_cbs_G = {addZMapCB, removeZMapCB, infoSetCB, quitReqCB, NULL} ;


/* Why are these here ?? */
#define ZMAPLOG_FILENAME "zmap.log"


char *ZMAP_X_PROGRAM_G  = "ZMap" ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* RESURRECT ?? */

/* Place a copyright notice in the executable. */
char *obj_copyright_G = ZMAP_OBJ_COPYRIGHT_STRING(ZMAP_TITLE,
  ZMAP_VERSION, ZMAP_RELEASE, ZMAP_UPDATE,
  ZMAP_DESCRIPTION) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* This is the "real" main routine, called by either the C or C++ main
 * in zmapAppmain_c.c and zmapAppmain_c.cc respectively, allowing
 * compilation as a C or C++ program....this has not been tested any
 * time recently.
 *
 * Order is important here, if you change the order of calls you may
 * find you have some work to do to make it all work, e.g. logging is
 * set up as early as possible so we catch logged error messages.
 *
 *  */
int zmapMainMakeAppWindow(int argc, char *argv[])
{
  ZMapAppContext app_context ;
  char *peer_name, *peer_clipboard ;
  int peer_retries, peer_timeout_ms ;
  GtkWidget *toplevel, *vbox, *menubar, *connect_frame, *manage_frame ;
  GtkWidget *quit_button ;
  int log_size ;
  int sleep_seconds = 0 ;
  GList *seq_maps = NULL ;
  gboolean remote_control = FALSE ;
  GError *g_error = NULL ;
  char *config_file = NULL ;
  char *styles_file = NULL ;


#ifdef ZMAP_MEMORY_DEBUG
  if (g_mem_is_system_malloc())
    zMapDebugPrintf("%s", "Using system malloc.") ;
  else
    zMapDebugPrintf("%s", "Using glib special malloc.") ;

  g_mem_set_vtable(glib_mem_profiler_table) ;
#endif



  /*       Application initialisation.        */


  /* User can ask for an immediate sleep, useful for attaching a debugger. */
  if ((sleep_seconds = checkForCmdLineSleep(argc, argv)))
    sleep(sleep_seconds) ;


  /* Since thread support is crucial we do compile and run time checks that its all initialised.
   * the function calls look obscure but its what's recommended in the glib docs. */
#if !defined G_THREADS_ENABLED || defined G_THREADS_IMPL_NONE || !defined G_THREADS_IMPL_POSIX
#error "Cannot compile, threads not properly enabled."
#endif


  /* Make sure glib threading is supported and initialised. */
  g_thread_init(NULL) ;
  if (!g_thread_supported())
    g_thread_init(NULL);


  /* Set up stack tracing for when we get killed by a signal. */
  setupSignalHandlers() ;


  /* Set up user type, i.e. developer or normal user. */
  zMapUtilsUserInit() ;


  /* Set up command line parsing object, globally available anywhere, this function exits if
   * there are bad command line args. */
  zMapCmdLineArgsCreate(&argc, argv) ;


  /* If user specified version flag, show zmap version and exit with zero return code. */
  checkForCmdLineVersionArg(argc, argv) ;


  /* The main control block. */
  app_context = createAppContext() ;


  /* Set up configuration directory/files */
  checkConfigDir(&config_file, &styles_file) ;


  /* Set any global debug flags from config file. */
  zMapUtilsConfigDebug(NULL) ;


  /* Check if a peer program was specified on the command line or in the config file. */
  peer_name = peer_clipboard = NULL ;
  peer_retries = peer_timeout_ms = -1 ;
  if (!checkPeerID(config_file, &peer_name, &peer_clipboard, &peer_retries, &peer_timeout_ms))
    {
      /* CAN'T LOG HERE....log has not been init'd.... */

      /* obscure...only an error if just one is specified... */
      if (!(!peer_name && !peer_clipboard))
        consoleMsg(TRUE, "No %s name for remote connection specified so remote interface cannot be created.",
           (!peer_name ? "peer" : "clipboard")) ;
    }
  else
    {
      /* set up the remote_request_func, subsystems check for this routine to determine
       * if they should make/service xremote calls. */
      app_window_cbs_G.remote_request_func = zmapAppRemoteControlGetRequestCB() ;
      app_window_cbs_G.remote_request_func_data = (void *)app_context ;

      remote_control = TRUE ;
      app_context->defer_hiding = TRUE ;
    }


  /* Init manager, must happen just once in application. */
  zMapManagerInit(&app_window_cbs_G) ;

  /* Add the ZMaps manager. */
  app_context->zmap_manager = zMapManagerCreate((void *)app_context) ;

  /* Set up logging for application....NO LOGGING BEFORE THIS. */
  if (!zMapLogCreate(NULL) || !configureLog(config_file, &g_error))
    {
      consoleMsg(TRUE, "Error creating log file: %s", (g_error ? g_error->message : "<no error message>")) ;
      doTheExit(EXIT_FAILURE) ;
    }
  else
    {
      if (g_error)
        {
          g_error_free(g_error) ;
          g_error = NULL ;
        }
      zMapWriteStartMsg() ;
    }

  /* Get general zmap configuration from config. file. */
  getConfiguration(app_context, config_file) ;


  {
    /* Check locale setting, vital for message handling, text parsing and much else. */
    char *default_locale = "POSIX";
    char *locale_in_use, *user_req_locale, *new_locale;

    locale_in_use = setlocale(LC_ALL, NULL);

    if (!(user_req_locale = app_context->locale))
      user_req_locale = app_context->locale = g_strdup(default_locale);

    /* should we store locale_in_use and new_locale somewhere? */
    if (!(new_locale = setlocale(LC_ALL, app_context->locale)))
      {
        zMapLogWarning("Failed setting locale to '%s'", app_context->locale) ;
        consoleMsg(TRUE, "Failed setting locale to '%s'", app_context->locale) ;
        doTheExit(EXIT_FAILURE) ;
      }
  }

  /* Check for sequence/start/end on command line or in config. file, must be
   * either completely correct or not specified. */
  if (!checkSequenceArgs(argc, argv, config_file, styles_file, &seq_maps, &g_error))
    {
      if (g_error)
        {
          zMapLogCritical("%s", g_error->message) ;
          consoleMsg(TRUE, "%s", g_error->message) ;
          g_error_free(g_error);
          g_error = NULL;
        }
      else
        {
          zMapLogCritical("%s", "Fatal error: no error message given") ;
          consoleMsg(TRUE, "%s", "Fatal error: no error message given") ;
        }

      doTheExit(EXIT_FAILURE) ;
    }

  /* Use the first sequence map as the default sequence */
  if (seq_maps && g_list_length(seq_maps) > 0)
    app_context->default_sequence = (ZMapFeatureSequenceMap)(seq_maps->data) ;


  /*             GTK initialisation              */

  initGnomeGTK(argc, argv) ;    /* May exit if checks fail. */

#ifdef ZMAP_MEMORY_DEBUG
  g_mem_profile() ;
#endif

  app_context->app_widg = toplevel = zMapGUIToplevelNew(NULL, NULL) ;

  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;


  /* If there is a peer program then set up xremote comms. */
  if (remote_control)
    {
      if (zmapAppRemoteControlCreate(app_context, peer_name, peer_clipboard, peer_retries, peer_timeout_ms))
        {
          /* Rest of initialisation requires a window so code is in a map callback
           * where we are guaranteed to have a window. */

          consoleMsg(TRUE, "ZMAP %s() - %s",
                     __PRETTY_FUNCTION__,
                     "Setting up remoteInstaller() as callback on \"map-event\"") ;

          app_context->mapCB_id = g_signal_connect(G_OBJECT(toplevel), "map-event",
           G_CALLBACK(remoteInstaller),
           (gpointer)app_context) ;


          zmapAppRemoteControlSetExitRoutine(app_context, appExit) ;
        }
      else
        {
          zMapCritical("%s", "Could not initialise remote control interface.") ;
        }
    }


  /* Handler for when main window is destroyed (by whichever route). */
  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
     GTK_SIGNAL_FUNC(toplevelDestroyCB), (gpointer)app_context) ;

  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(toplevel), vbox) ;

  menubar = zmapMainMakeMenuBar(app_context) ;
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);

  connect_frame = zmapMainMakeConnect(app_context, app_context->default_sequence) ;
  gtk_box_pack_start(GTK_BOX(vbox), connect_frame, TRUE, TRUE, 0);

  manage_frame = zmapMainMakeManage(app_context) ;
  gtk_box_pack_start(GTK_BOX(vbox), manage_frame, TRUE, TRUE, 0);

  quit_button = gtk_button_new_with_label("Quit") ;
  gtk_signal_connect(GTK_OBJECT(quit_button), "clicked",
     GTK_SIGNAL_FUNC(quitCB), (gpointer)app_context) ;
  gtk_box_pack_start(GTK_BOX(vbox), quit_button, FALSE, FALSE, 0) ;

  /* Always create the widget. */
  gtk_widget_show_all(toplevel) ;


  /* We don't always want to show this window, for lace users it is useless.... */
  if (!(app_context->show_mainwindow) && !(app_context->defer_hiding))
    {
      consoleMsg(TRUE,  "ZMAP %s() - %s",
                 __PRETTY_FUNCTION__,
                 "Hiding main window.") ;

      hideMainWindow(app_context) ;
    }


  /* Check that log file has not got too big... */
  if ((log_size = zMapLogFileSize()) > (ZMAP_DEFAULT_MAX_LOG_SIZE * 1048576))
    zMapWarning("Log file was grown to %d bytes, you should think about archiving or removing it.", log_size) ;


  /* Only show default sequence if we are _not_ controlled via XRemote */
  if (!remote_control)
     {
       gboolean ok = TRUE ;
       int num_views = seq_maps ? g_list_length(seq_maps) : 0 ;

       if (num_views > 2)
         {
           ok = zMapGUIMsgGetBool(NULL, ZMAP_MSG_WARNING,
                                  "There are %d different sequences/regions in the input sources. This will create %d views in ZMap. Are you sure you want to continue?",
                                  num_views, num_views) ;
         }

       if (ok)
         {
           ZMap zmap = NULL ;
           ZMapView view = NULL ;
           char *err_msg = NULL ;

           if (!zmapAppCreateZMap(app_context, app_context->default_sequence, &zmap, &view, &err_msg))
             {
               zMapWarning("%s", err_msg) ;
             }
           else if (num_views > 1)
             {
               /* There is more than one sequence map. We've added the first already but add
                * subsequence ones as new Views. */
               GList *seq_map_item = seq_maps->next ;

               for ( ; seq_map_item; seq_map_item = seq_map_item->next)
                 {
                   ZMapFeatureSequenceMap seq_map = (ZMapFeatureSequenceMap)(seq_map_item->data) ;
                   char *err_msg = NULL ;

                   zMapControlInsertView(zmap, seq_map, &err_msg) ;

                   if (err_msg)
                     {
                       zMapWarning("%s", err_msg) ;
                       g_free(err_msg) ;
                     }
                 }
            }
        }
    }


  app_context->state = ZMAPAPP_RUNNING ;

  /* Start the GUI. */
  gtk_main() ;

  doTheExit(EXIT_SUCCESS) ;    /* exits.... */


  /* We should never get here, hence the failure return code. */
  return(EXIT_FAILURE) ;
}



void zmapAppExit(ZMapAppContext app_context)
{
  gboolean remote_disconnecting = FALSE ;

  /* Record we are dying so we know when to quit as last zmap dies. */
  app_context->state = ZMAPAPP_DYING ;

  /* If we have a remote client we need to tell them we are going and wait
   * for their reply, otherwise we can just get on with it. */
  if (app_context->remote_control)
    {
      gboolean app_exit = TRUE ;

      remote_disconnecting = zmapAppRemoteControlDisconnect(app_context, app_exit) ;
    }

  if (!(app_context->remote_control) || !remote_disconnecting)
    {
      appExit(app_context) ;
    }

  return ;
}


void zmapAppPingStart(ZMapAppContext app_context)
{
  guint timeout_func_id ;
  int interval = app_context->ping_timeout * 1000 ;    /* glib needs time in milliseconds. */

  app_context->stop_pinging = FALSE ;

  /* try setting a timer to ping periodically.... */
  timeout_func_id = g_timeout_add(interval, pingHandler, (gpointer)app_context) ;

  return ;
}

void zmapAppPingStop(ZMapAppContext app_context)
{
  app_context->stop_pinging = TRUE ;

  return ;
}




/*
 *                 Internal routines
 */

static void initGnomeGTK(int argc, char *argv[])
{
  gchar *err_msg ;
  gchar *rc_dir, *rc_file;
  gtk_set_locale() ;

  if ((err_msg = (char *)gtk_check_version(ZMAP_GTK_MAJOR, ZMAP_GTK_MINOR, ZMAP_GTK_MICRO)))
    {
      zMapLogCritical("%s", err_msg) ;
      zMapExitMsg("%s", err_msg) ;

      gtk_exit(EXIT_FAILURE) ;
    }

  if ((rc_dir = zMapConfigDirGetDir()))
    {
      rc_file = g_strdup_printf("%s/%s", rc_dir, ".gtkrc");
      /* This needs to be done before gtk_init()
       * Call to gtk_set_locale() should make fetching locale specific rc
       * files work too. For instance, if LANG is set to ja_JP.ujis,
       * when loading the default file rc_file then GTK+ looks for rc_file.ja_JP
       * and rc_file.ja, and parses the first of those that exists.
       */
      gtk_rc_add_default_file(rc_file);
      g_free(rc_file);
    }

  gtk_init(&argc, &argv) ;

  return ;
}


static ZMapAppContext createAppContext(void)
{
  ZMapAppContext app_context ;

  app_context = g_new0(ZMapAppContextStruct, 1) ;

  app_context->state = ZMAPAPP_INIT ;

  app_context->app_id = zMapGetAppTitle() ;

  app_context->exit_timeout = ZMAP_DEFAULT_EXIT_TIMEOUT ;
  app_context->ping_timeout = ZMAP_DEFAULT_PING_TIMEOUT ;

  return app_context ;
}


static void destroyAppContext(ZMapAppContext app_context)
{
  if (app_context->locale)
    g_free(app_context->locale) ;

  if(app_context->default_sequence)
    {
      if(app_context->default_sequence->sequence)
        g_free(app_context->default_sequence->sequence);

      g_free(app_context->default_sequence);
    }
  g_free(app_context) ;

  return ;
}



/* A GSourceFunc, called after requested time interval for pings until this function returns FALSE. */
static gboolean pingHandler(gpointer data)
{
  gboolean result = TRUE ;
  ZMapAppContext app_context = (ZMapAppContext)data ;

  if (app_context->stop_pinging)
    result = FALSE ;
  else
    zmapAppRemoteControlPing(app_context) ;

  return result ;
}


/* Called when layer below signals that a zmap has been added. */
void addZMapCB(void *app_data, void *zmap_data)
{
  ZMapAppContext app_context = (ZMapAppContext)app_data ;
  ZMap zmap = (ZMap)zmap_data ;
  GtkTreeIter iter1 ;

  /* Update our list of zmaps.... */
  gtk_tree_store_append (app_context->tree_store_widg, &iter1, NULL);
  gtk_tree_store_set (app_context->tree_store_widg, &iter1,
      ZMAPID_COLUMN, zMapGetZMapID(zmap),
      ZMAPSEQUENCE_COLUMN,"<dummy>" ,
      ZMAPSTATE_COLUMN, zMapGetZMapStatus(zmap),
      ZMAPLASTREQUEST_COLUMN, "blah, blah, blaaaaaa",
      ZMAPDATA_COLUMN, (gpointer)zmap,
      -1);

  return ;
}



/* UM...I'M NOT SURE THIS COMMENT IS CORRECT....SHOULD BE CALLED WHEN
 * USER CLOSES A WINDOW OR QUITS....NEED TO DISTINGUISH WHICH.... */
/* Called when user clicks the big "Quit" button in the main window.
 * Called every time a zmap window signals that it has gone. */
void removeZMapCB(void *app_data, void *zmap_data)
{
  ZMapAppContext app_context = (ZMapAppContext)app_data ;
  ZMap zmap = (ZMap)zmap_data ;

  /* This has the potential to remove multiple rows, but currently
     doesn't as the first found one that matches, gets removed an
     returns true. See
     http://scentric.net/tutorial/sec-treemodel-remove-many-rows.html
     for an implementation of mutliple deletes */
  gtk_tree_model_foreach(GTK_TREE_MODEL(app_context->tree_store_widg),
                         (GtkTreeModelForeachFunc)removeZMapRowForeachFunc,
                         (gpointer)zmap);

  if (app_context->selected_zmap == zmap)
    app_context->selected_zmap = NULL ;



  /* UMMM....THIS EXITING HERE DOESN'T LOOK RIGHT TO ME.......... */

  /* When the last zmap has gone and we are dying then exit. */
  if (app_context->state == ZMAPAPP_DYING && ((zMapManagerCount(app_context->zmap_manager)) == 0))
    exitApp(app_context) ;

  return ;
}







/*
 *        callbacks that destroy the application
 */

/* Called when user selects "quit2 button in main window. */
static void quitCB(GtkWidget *widget, gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;

  zmapAppExit(app_context) ;

  return ;
}

/* Called from layers below when they want zmap to exit. */
void quitReqCB(void *app_data, void *zmap_data_unused)
{
  ZMapAppContext app_context = (ZMapAppContext)app_data ;

  zmapAppExit(app_context) ;

  return ;
}

/* This function gets called whenever there is a gtk_widget_destroy() to the top level
 * widget. Sometimes this is because of window manager action, sometimes one of our exit
 * routines does a gtk_widget_destroy() on the top level widget.
 *
 * It is the final clean up routine for the ZMap application and exits the program.
 *
 *  */
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;

  finalCleanUp(app_context) ;    /* exits program. */

  return ;
}




/* A GSourceFunc, called if we take too long to exit, in which case we force the exit. */
static gboolean timeoutHandler(gpointer data)
{
  ZMapAppContext app_context = (ZMapAppContext)data ;

  crashExitApp(app_context) ;

  return FALSE ;
}




/*
 *               exit/cleanup routines.
 */


static void appExit(void *user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;


  /* If there are no zmaps left we can exit here, otherwise we must signal all the zmaps
   * to die and wait for them to signal they have died or timeout and exit. */
  if (!(zMapManagerCount(app_context->zmap_manager)))
    {
      signalFinalCleanUp(app_context, EXIT_SUCCESS, CLEAN_EXIT_MSG) ;
    }
  else
    {
      /* Check if we need to save anything first */
      if (zMapManagerCheckIfUnsaved(app_context->zmap_manager))
        {
          guint timeout_func_id ;
          int interval = app_context->exit_timeout * 1000 ;    /* glib needs time in milliseconds. */

          consoleMsg(TRUE, "%s", "Issuing requests to all ZMaps to disconnect from servers and quit.") ;

          /* N.B. we block for 2 seconds here to make sure user can see message. */
          zMapGUIShowMsgFull(NULL, "ZMap is disconnecting from its servers and quitting, please wait.",
                             ZMAP_MSG_EXIT,
                             GTK_JUSTIFY_CENTER, 2, FALSE) ;

          /* time out func makes sure that we exit if threads fail to report back. */
          timeout_func_id = g_timeout_add(interval, timeoutHandler, (gpointer)app_context) ;

          /* Tell all our zmaps to die, they will tell all their threads to die. */
          zMapManagerKillAllZMaps(app_context->zmap_manager) ;
        }
    }

  return ;
}





/* Called on clean exit of zmap. */
static void exitApp(ZMapAppContext app_context)
{
/* This must be done here as manager checks to see if all its zmaps have gone. */
  if (app_context->zmap_manager)
    zMapManagerDestroy(app_context->zmap_manager) ;

  signalFinalCleanUp(app_context, EXIT_SUCCESS, CLEAN_EXIT_MSG) ;    /* exits app. */

  return ;
}

/* Called when clean up of zmaps and their threads times out, stuff is not
 * cleaned up and the application does not exit cleanly. */
static void crashExitApp(ZMapAppContext app_context)
{
  char *exit_msg = "Exit timed out - WARNING: Zmap clean up of threads timed out, exit has been forced !" ;

  signalFinalCleanUp(app_context, EXIT_FAILURE, exit_msg) ;    /* exits app. */

  return ;
}

/* Sends finalise to client if there is one and then signals main window to exit which
 * will then call our final destroy callback. */
static void signalFinalCleanUp(ZMapAppContext app_context, int exit_rc, char *exit_msg)
{
  app_context->exit_rc = exit_rc ;
  app_context->exit_msg = exit_msg ;

  /* **NEW XREMOTE** destroy, Note that at this point we have already told the peer
   * that we are quitting so now we just need to clear up. */
  if (app_context->remote_control)
    {
      zmapAppRemoteControlDestroy(app_context) ;
    }

  /* Causes the destroy callback to be invoked which then calls the final clean up. */
  gtk_widget_destroy(app_context->app_widg);

  return ;
}

/* Cleans up and exits. */
static void finalCleanUp(ZMapAppContext app_context)
{
  int exit_rc = app_context->exit_rc ;
  char *exit_msg = app_context->exit_msg ;

  zMapConfigDirDestroy();
  destroyAppContext(app_context) ;

  if (exit_rc)
    zMapLogCritical("%s", exit_msg) ;
  else
    consoleMsg(TRUE, "%s", exit_msg) ;

  zMapWriteStopMsg() ;
  zMapLogDestroy() ;

  doTheExit(exit_rc) ;

  return ;
}


/*
 * Use this routine to exit the application with a portable (as in POSIX) return
 * code. If exit_code == 0 then application exits with EXIT_SUCCESS, otherwise
 * exits with EXIT_FAILURE.
 *
 * Note: gtk now deprecates use of gtk_exit() for exit().
 *
 * exit_code = 0 for success, anything else for failure.
 *  */
static void doTheExit(int exit_code)
{
  int true_exit_code ;

  if (exit_code)
    true_exit_code = EXIT_FAILURE ;
  else
    true_exit_code = EXIT_SUCCESS ;

  exit(true_exit_code) ;

  return ;    /* we never get here. */
}


static gboolean removeZMapRowForeachFunc(GtkTreeModel *model, GtkTreePath *path,
                                         GtkTreeIter *iter, gpointer data)
{
  ZMap zmap = (ZMap)data;
  ZMap row_zmap;

  gtk_tree_model_get(model, iter,
                     ZMAPDATA_COLUMN, &row_zmap,
                     -1);

  if (zmap == row_zmap)
    {
      gtk_tree_store_remove(GTK_TREE_STORE(model), iter);
      return TRUE;
    }

  return FALSE;                 /* TRUE means it stops! */
}

/* Did user specify seqence/start/end on command line? */
static void checkForCmdLineSequenceArg(int argc, char *argv[], char **dataset_out, char **sequence_out, GError **error)
{
  ZMapCmdLineArgsType value ;
  char *sequence ;

  if (zMapCmdLineArgsValue(ZMAPARG_SEQUENCE, &value))
    {
      sequence = value.s ;

      if(dataset_out)
        {
          gchar *delim;

          delim = g_strstr_len(sequence,-1,"/");
          if(delim)
            {
              *dataset_out = sequence;
              *delim++ = 0;
              sequence = delim;
          }
      }

    *sequence_out = sequence ;
  }

  return ;
}


/* Did user specify seqence/start/end on command line. */
static void checkForCmdLineStartEndArg(int argc, char *argv[], int *start_inout, int *end_inout, GError **error)
{
  GError *tmp_error = NULL ;
  ZMapCmdLineArgsType start_value ;
  ZMapCmdLineArgsType end_value ;

  gboolean got_start = zMapCmdLineArgsValue(ZMAPARG_SEQUENCE_START, &start_value) ;
  gboolean got_end = zMapCmdLineArgsValue(ZMAPARG_SEQUENCE_END, &end_value) ;

  if (got_start && got_end)
    {
      *start_inout = start_value.i ;
      *end_inout = end_value.i ;
    }
  else if (got_start)
    {
      g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_BAD_COORDS,
                  "The start coord was specified on the command line but the end coord is missing: must specify both or none") ;
    }
  else if (got_end)
    {
      g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_BAD_COORDS,
                  "The end coord was specified on the command line but the start coord is missing: must specify both or none") ;
    }

  if (tmp_error)
    g_propagate_error(error, tmp_error) ;

  return ;
}




/* Did user specify the version arg. */
static void checkForCmdLineVersionArg(int argc, char *argv[])
{
  ZMapCmdLineArgsType value ;
  gboolean show_version = FALSE ;

  if (zMapCmdLineArgsValue(ZMAPARG_VERSION, &value))
    show_version = value.b ;

  if (show_version)
    {
      printf("%s - %s\n", zMapGetAppName(), zMapGetAppVersionString()) ;

      exit(EXIT_SUCCESS) ;
    }
  else if (zMapCmdLineArgsValue(ZMAPARG_RAW_VERSION, &value) && value.b)
    {
      printf("%s\n", zMapGetAppVersionString()) ;

      exit(EXIT_SUCCESS) ;
    }


  return ;
}

/* Did user ask for zmap to sleep initially - useful for attaching a debugger.
 * Returns number of seconds to sleep or zero if user did not specify sleep.
 *  */
static int checkForCmdLineSleep(int argc, char *argv[])
{
  int seconds = 0 ;
  char **curr_arg ;

  /* We do this by steam because the cmdline args stuff is not set up immediately. */

  curr_arg = argv ;
  while (*curr_arg)
    {
      if (g_str_has_prefix(*curr_arg, "--" ZMAPSTANZA_APP_SLEEP "="))
        {
          seconds = atoi((strstr(*curr_arg, "=") + 1)) ;
          break ;
        }

      curr_arg++ ;
    }

  return seconds ;
}




/* Did user specify a config directory and/or config file within that directory on the command
 * line. Also check for the stylesfile on the command line or in the config dir. */
static void checkConfigDir(char **config_file_out, char **styles_file_out)
{
  ZMapCmdLineArgsType dir = {FALSE}, file = {FALSE}, stylesfile = {FALSE} ;

  zMapCmdLineArgsValue(ZMAPARG_CONFIG_DIR, &dir) ;
  zMapCmdLineArgsValue(ZMAPARG_CONFIG_FILE, &file) ;
  zMapCmdLineArgsValue(ZMAPARG_STYLES_FILE, &stylesfile) ;

  if (zMapConfigDirCreate(dir.s, file.s))
    {
      *config_file_out = zMapConfigDirGetFile() ;

      /* If the stylesfile is a relative path, look for it in the config dir. If it
       * was not given on the command line, check if there's one with the default name */
      if (stylesfile.s)
        *styles_file_out = zMapConfigDirFindFile(stylesfile.s) ;
      else
        *styles_file_out = zMapConfigDirFindFile("styles.ini") ;
    }
  else if (stylesfile.s)
    {
      *styles_file_out = stylesfile.s;
    }
}


/* Did caller specify a peer program id on the command line or in a config file. */
static gboolean checkPeerID(char *config_file,
    char **peer_name_out, char **peer_clipboard_out, int *peer_retries, int *peer_timeout_ms)
{
  gboolean result = FALSE ;
  ZMapCmdLineArgsType name_value = {FALSE}, clipboard_value = {FALSE} ;
  ZMapConfigIniContext context ;


  /* Command line params override config file. */
  if (zMapCmdLineArgsValue(ZMAPARG_PEER_NAME, &name_value)
      && zMapCmdLineArgsValue(ZMAPARG_PEER_CLIPBOARD, &clipboard_value))
    {
      if (name_value.s && clipboard_value.s)
        {
          *peer_name_out = g_strdup(name_value.s) ;
          *peer_clipboard_out = g_strdup(clipboard_value.s) ;
        }
    }

  if ((context = zMapConfigIniContextProvide(config_file)))
    {
      char *tmp_string  = NULL;
      int tmp_int = 0;

      /* If peer app name/clipboard not set then look in the config file... */
      if (!(*peer_name_out))
        {
          if (zMapConfigIniContextGetString(context, ZMAPSTANZA_PEER_CONFIG, ZMAPSTANZA_PEER_CONFIG,
                                            ZMAPSTANZA_PEER_NAME, &tmp_string))
            *peer_name_out = g_strdup(tmp_string) ;

        
          /* peer unique id to use in remote control. */
          if (zMapConfigIniContextGetString(context, ZMAPSTANZA_PEER_CONFIG, ZMAPSTANZA_PEER_CONFIG,
                                            ZMAPSTANZA_PEER_CLIPBOARD, &tmp_string))
            *peer_clipboard_out = g_strdup(tmp_string) ;
        }


      /* How many times to retry the peer on timeout. */
      if (zMapConfigIniContextGetInt(context, ZMAPSTANZA_PEER_CONFIG, ZMAPSTANZA_PEER_CONFIG,
                                     ZMAPSTANZA_PEER_RETRIES, &tmp_int))
*peer_retries = tmp_int ;


      /* How long to wait in ms before timeout waiting for peer. */
      if (zMapConfigIniContextGetInt(context, ZMAPSTANZA_PEER_CONFIG, ZMAPSTANZA_PEER_CONFIG,
                                     ZMAPSTANZA_PEER_TIMEOUT, &tmp_int))
        *peer_timeout_ms = tmp_int ;


      zMapConfigIniContextDestroy(context);
    }


  /* Both must be specified...... */
  if (*peer_name_out && *peer_clipboard_out)
    result = TRUE ;

  return result ;
}




/* This function isn't very intelligent at the moment.  It's function
 * is to set the info (GError) object of the appcontext so that the
 * remote control code can set the reply atom when opening, closing ...
 * a child zmap.  It's just sucking info out of the zmap at the moment
 * without worrying about context.  Maybe this will be what we want, but
 * I doubt it.
 * I doubt we should want to support too many remote calls through the
 * appcontext window though, currently I can think of open/close maybe
 * raise.  Anything more and we'll need to worry about how to get more
 * info and the calls should *REALLY* be going direct to the zmap itself
 */
static void infoSetCB(void *app_data, void *zmap_data)
{
  /* I WANT TO GET RID OF ALL OF THIS...I DON'T THINK WE NEED IT NOW.... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

  ZMapAppContext app_context = (ZMapAppContext)app_data;
  ZMap zmap = (ZMap)zmap_data;
  char *zmap_xml = NULL;
  GQuark domain = g_quark_from_string(__FILE__);

  g_clear_error(&(app_context->info));

  zmap_xml = zMapControlRemoteReceiveAccepts(zmap);

  app_context->info = g_error_new(domain, 200, "%s", zmap_xml) ;

  if(zmap_xml)
    g_free(zmap_xml);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}


/* Read ZMap application defaults. */
static gboolean getConfiguration(ZMapAppContext app_context, char *config_file)
{
  gboolean result = FALSE ;
  ZMapConfigIniContext context;

  app_context->show_mainwindow = TRUE;
  app_context->exit_timeout = ZMAP_DEFAULT_EXIT_TIMEOUT;

  if ((context = zMapConfigIniContextProvide(config_file)))
    {
      gboolean tmp_bool = FALSE;
      char *tmp_string  = NULL;
      int tmp_int = 0;

      /* Do we show the main window? if not then on ZMap close we should exit */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                         ZMAPSTANZA_APP_MAINWINDOW, &tmp_bool))
        app_context->show_mainwindow = tmp_bool ;

      /* Should window title prefix be abbreviated ? */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                         ZMAPSTANZA_APP_ABBREV_TITLE, &tmp_bool))
        app_context->abbrev_title_prefix = tmp_bool ;
      zMapGUISetAbbrevTitlePrefix(app_context->abbrev_title_prefix) ;


      /* How long to wait when closing, before timeout */
      if (zMapConfigIniContextGetInt(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                             ZMAPSTANZA_APP_EXIT_TIMEOUT, &tmp_int))
        app_context->exit_timeout = tmp_int ;

      if (app_context->exit_timeout < 0)
        app_context->exit_timeout = ZMAP_DEFAULT_EXIT_TIMEOUT ;


      /* help url to use */
      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                        ZMAPSTANZA_APP_HELP_URL, &tmp_string))
        zMapGUISetHelpURL(tmp_string) ;

      /* session colour for visual grouping of applications. */
      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                        ZMAPSTANZA_APP_SESSION_COLOUR, &tmp_string))
        {
          gdk_color_parse(tmp_string, &(app_context->session_colour)) ;
          app_context->session_colour_set = TRUE ;
        }

      /* locale to use */
      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                        ZMAPSTANZA_APP_LOCALE, &tmp_string))
        app_context->locale = tmp_string;

      /* Turn on/off debugging for xremote connection to peer client. */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                         ZMAPSTANZA_APP_XREMOTE_DEBUG, &tmp_bool))
        {
          app_context->xremote_debug = tmp_bool ;
        }

      zMapConfigIniContextDestroy(context);

      result = TRUE;
    }

  return result ;
}


/* Called  */
static void setupSignalHandlers(void)
{
  /* Not much point if there's no sigaction! */
#ifndef NO_SIGACTION
  struct sigaction signal_action, prev;

  sigemptyset (&(signal_action.sa_mask));

  /* Block other signals while handler runs. */
  sigaddset (&(signal_action.sa_mask), SIGSEGV);
  sigaddset (&(signal_action.sa_mask), SIGBUS);
  sigaddset (&(signal_action.sa_mask), SIGABRT);

  signal_action.sa_flags   = 0;
  signal_action.sa_handler = zMapSignalHandler;

  sigaction(SIGSEGV, &signal_action, &prev);
  sigaction(SIGBUS,  &signal_action, &prev);
  sigaction(SIGABRT, &signal_action, &prev);
#endif /* NO_SIGACTION */

  return ;
}



/* **NEW XREMOTE**
 *
 * create the new xremote object.
 */
static void remoteInstaller(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;

  consoleMsg(TRUE, "ZMAP %s() - %s",
             __PRETTY_FUNCTION__,
             "In remoteInstaller, about init RemoteControl.") ;

  /* Stop ourselves being called again. */
  g_signal_handler_disconnect(app_context->app_widg, app_context->mapCB_id) ;
  app_context->mapCB_id = 0 ;

  /* Set up our remote handler. */
  if (zmapAppRemoteControlInit(app_context))
    {
      if (!zmapAppRemoteControlConnect(app_context))
        zMapCritical("Failed to connect to peer %s using clipboard %s.",
     app_context->remote_control->peer_name,
     app_context->remote_control->peer_clipboard) ;
    }
  else
    {
      zMapLogCritical("%s", "Cannot initialise zmap remote control interface.") ;
    }



  consoleMsg(TRUE, "ZMAP %s() - %s",
             __PRETTY_FUNCTION__,
             "In remoteInstaller, about to hide main window.") ;

  /* Hide main window if required. */
  if (!(app_context->show_mainwindow) && app_context->defer_hiding)
    hideMainWindow(app_context) ;

  /* Set an inactivity timeout, so we can warn the user if nothing happens for a long time in case
   * zmap has become disconnected from the peer. (note timeout is in milliseconds) */
  app_context->remote_control->inactive_timeout_interval_s = ZMAP_APP_REMOTE_TIMEOUT_S ;
  app_context->remote_control->inactive_func_id = g_timeout_add(ZMAP_APP_REMOTE_TIMEOUT_S * 1000,
                                                                remoteInactiveHandler, (gpointer)app_context) ;

  /* Now set the time of day we did this. */
  app_context->remote_control->last_active_time_s = zMapUtilsGetRawTime() ;

  return ;
}


/* Read logging configuration from ZMap stanza and apply to log. */
static gboolean configureLog(char *config_file, GError **error)
{
  gboolean result = TRUE ;/* if no config, we can't fail to configure */
  GError *g_error = NULL ;
  ZMapConfigIniContext context ;
  gboolean logging, log_to_file, show_process, show_code, show_time, catch_glib, echo_glib ;
  char *full_dir, *log_name, *logfile_path ;

  /* ZMap's default values (as opposed to the logging packages...). */
  logging = TRUE ;
  log_to_file = TRUE  ;
  show_process = FALSE ;
  show_code = TRUE ;
  show_time = FALSE ;
  catch_glib = TRUE ;
  echo_glib = TRUE ;
  /* if we run config free we use .ZMap, creating it if necessary */
  full_dir = ZMAP_USER_CONFIG_DIR ;
  log_name = g_strdup(ZMAPLOG_FILENAME) ;


  if (config_file && (context = zMapConfigIniContextProvide(config_file)))
    {
      gboolean tmp_bool ;
      char *tmp_string = NULL;

      /* logging at all */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_LOGGING, &tmp_bool))
        logging = tmp_bool ;

      /* logging to the file */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_FILE, &tmp_bool))
        log_to_file = tmp_bool ;

      /* how much detail to show...process/pid... */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_SHOW_PROCESS, &tmp_bool))
        show_process = tmp_bool ;

      /* how much detail to show...code... */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_SHOW_CODE, &tmp_bool))
        show_code = tmp_bool;

      /* how much detail to show...time... */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_SHOW_TIME, &tmp_bool))
        show_time = tmp_bool;

      /* catch GLib errors, else they stay on stdout */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_CATCH_GLIB, &tmp_bool))
        catch_glib = tmp_bool;

      /* catch GLib errors, else they stay on stdout */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_ECHO_GLIB, &tmp_bool))
        echo_glib = tmp_bool;

      /* user specified dir, default to config dir */
      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_LOG_CONFIG,
                                        ZMAPSTANZA_LOG_CONFIG,
                                        ZMAPSTANZA_LOG_DIRECTORY, &tmp_string))
        {
          full_dir = zMapGetDir(tmp_string, TRUE, TRUE) ;
        }
      else
        {
          full_dir = g_strdup(zMapConfigDirGetDir()) ;
        }


      /* user specified file, default to zmap.log */
      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_LOG_CONFIG,
                                        ZMAPSTANZA_LOG_CONFIG,
                                        ZMAPSTANZA_LOG_FILENAME, &tmp_string))
        {
          g_free(log_name) ;
          log_name = tmp_string;
        }

      /* config context needs freeing */
      zMapConfigIniContextDestroy(context);

    }
  else
    {
      /* Use the default directory. Create the full path and mkdir if necessary. */
      full_dir = zMapGetDir(full_dir, TRUE, TRUE) ;
    }

  logfile_path = zMapGetFile(full_dir, log_name, TRUE, &g_error) ;

  /* all our strings need freeing */
  g_free(log_name) ;
  g_free(full_dir) ;

  if (!g_error)
    {
      result = zMapLogConfigure(logging, log_to_file,
                                show_process, show_code, show_time,
                                catch_glib, echo_glib,
                                logfile_path, &g_error) ;
    }

  if (g_error)
    {
      result = FALSE ;
      g_propagate_error(error, g_error) ;
    }

  return result ;
}


static ZMapFeatureSequenceMap createSequenceMap(char *sequence, int start, int end, 
                                                char *config_file, char *styles_file, 
                                                GList **seq_maps_inout)
{
  ZMapFeatureSequenceMap seq_map = g_new0(ZMapFeatureSequenceMapStruct, 1) ;

  seq_map->sequence = sequence ;
  seq_map->start = start ;
  seq_map->end = end ;
  seq_map->config_file = config_file ;
  seq_map->stylesfile = styles_file ;

  *seq_maps_inout = g_list_append(*seq_maps_inout, seq_map) ;

  return seq_map ;
}


/* Check the given file to see if we can extract sequence details */
static void checkInputFileForSequenceDetails(const char* const filename,
                                             char *config_file,
                                             char *styles_file,
                                             GList **seq_maps_inout,
                                             const gboolean merge_details,
                                             GError **error)
{
  gboolean result = FALSE ;
  GError *tmp_error = NULL ;
  GError *gff_pipe_err = NULL ;
  int gff_version = 0 ;
  ZMapGFFParser parser = NULL ;
  GString *gff_line = NULL ;
  GIOStatus status = G_IO_STATUS_NORMAL ;
  GIOChannel *gff_pipe = NULL ;

  zMapReturnIfFail(filename) ;

  gff_line = g_string_sized_new(2000) ; /* Probably not many lines will be > 2k chars. */

  /* Check for the special case "-" which means stdin */
  if (!strcmp(filename, "-"))
    gff_pipe = g_io_channel_unix_new(0) ; /* file descriptor for stdin is 0 */
  else
    gff_pipe = g_io_channel_new_file(filename, "r", &gff_pipe_err) ;


  if (!gff_pipe)
    {
      g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_OPENING_FILE,
                  "Could not open file %s", filename);
    }
  else
    {
      if (!zMapGFFGetVersionFromGIO(gff_pipe, gff_line,
                                    &gff_version,
                                    &status, &gff_pipe_err))
        {
          char *tmp ;

          if (status == G_IO_STATUS_EOF)
            tmp = "No data returned from pipe" ;
          else
            tmp = "Serious GIO error returned from file" ;

          g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_GFF_VERSION,
                      "Could not get gff-version from file %s: %s.",
                      filename, tmp);
        }
      else
        {
          parser = zMapGFFCreateParser(gff_version, NULL, 0, 0) ;

          if (parser)
            {
              gsize terminator_pos = 0 ;
              gboolean done_header = FALSE ;
              ZMapGFFHeaderState header_state = GFF_HEADER_NONE ; /* got all the ones we need ? */

              zMapGFFParserSetSequenceFlag(parser);  // reset done flag for seq else skip the data

              /* Read the header, needed for feature coord range. */
              while ((status = g_io_channel_read_line_string(gff_pipe, gff_line,
                                                             &terminator_pos,
                                                             &gff_pipe_err)) == G_IO_STATUS_NORMAL)
                {
                  *(gff_line->str + terminator_pos) = '\0' ; /* Remove terminating newline. */

                  if (zMapGFFParseHeader(parser, gff_line->str, &done_header, &header_state))
                    {
                      if (done_header)
                        {
                          result = TRUE ;
                          break ;
                        }
                      else
                        {
                          gff_line = g_string_truncate(gff_line, 0) ;  /* Reset line to empty. */
                        }
                    }
                  else
                    {
                      g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_GFF_HEADER,
                                  "Error reading GFF header for file %s", filename) ;
                      break ;
                    }
                }
            }
          else
            {
              g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_GFF_PARSER,
                          "Error creating GFF parser for file %s", filename);
            }
        }
    }

  if (result && !tmp_error)
    {
      /* Cache the sequence details in a sequence map - merge with an existing sequence map if
       * possible, otherwise create a new one. */
      char *sequence = zMapGFFGetSequenceName(parser) ;
      int start = zMapGFFGetFeaturesStart(parser) ;
      int end = zMapGFFGetFeaturesEnd(parser) ;
      ZMapFeatureSequenceMap seq_map = NULL ;

      GList *seq_map_item = seq_maps_inout ? g_list_first(*seq_maps_inout) : NULL ;

      for ( ; seq_map_item; seq_map_item = seq_map_item->next)
        {
          seq_map = (ZMapFeatureSequenceMap)(seq_map_item->data) ;

          zMapAppMergeSequenceName(seq_map, sequence, merge_details, &tmp_error) ;

          if (!tmp_error)
            zMapAppMergeSequenceCoords(seq_map, start, end, FALSE, merge_details, &tmp_error) ;

          if (!tmp_error)
            {
              /* We successfully merged, so we've found our seq_map and we're done */
              break ;
            }
          else
            {
              seq_map = NULL ;

              /* The error just indicates we failed to merge so reset it before trying the next one */
              g_error_free(tmp_error) ;
              tmp_error = NULL ;
            }
        }

      if (!tmp_error)
        {
          /* If we didn't manage to merge with an existing seq map, create a new one. Use the 
           * config/styles file if given because we want to inherit all of the config 
           * apart from the sequence details */
          if (!seq_map)
            seq_map = createSequenceMap(sequence, start, end, config_file, styles_file, seq_maps_inout) ;

          /* Cache the filename and parser state */
          seq_map->file_list = g_slist_append(seq_map->file_list, g_strdup(filename)) ;

          ZMapFeatureParserCache parser_cache = g_new0(ZMapFeatureParserCacheStruct, 1) ;
          parser_cache->parser = (gpointer)parser ;
          parser_cache->line = gff_line ;
          parser_cache->pipe = gff_pipe ;
          parser_cache->pipe_status = status ;

          seq_map->cached_parsers = g_hash_table_new(NULL, NULL) ;
          g_hash_table_insert(seq_map->cached_parsers, GINT_TO_POINTER(g_quark_from_string(filename)), parser_cache) ;
        }
    }
  else if (!tmp_error)
    {
      /* If result is false then tmp_error should be set, but it's possible it might not be if we
       * didn't find a full header in the GFF file. */
      g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_CHECK_FILE,
                  "Error reading GFF header information for file %s", filename);
    }

  if (tmp_error && parser)
    zMapGFFDestroyParser(parser) ;

  if (tmp_error)
    g_propagate_error(error, tmp_error) ;
}


/* Check the input file(s) on the command line, if any. Checks the sequence details are valid and/or
 * sets the sequence details if they are not already set. */
static void checkInputFiles(char *config_file, char *styles_file, GList **seq_maps_inout, GError **error)
{
  GError *tmp_error = NULL ;
  char **file_list = zMapCmdLineFinalArg() ;
  ZMapFeatureSequenceMap seq_map = NULL ;

  if (seq_maps_inout && *seq_maps_inout)
    seq_map = (ZMapFeatureSequenceMap)g_list_first(*seq_maps_inout) ;

  if (file_list)
    {
      /* If details have been set by the command line or config file then take those as absolute
       * and don't merge the ranges from the input files - just check that they are valid. If
       * they are not already set, merge the ranges from the input files to get the full extent. */
      const gboolean merge_details = (seq_map && !seq_map->sequence && !seq_map->start && !seq_map->end);

      /* Loop through the input files */
      char **file = file_list ;

      for (; file && *file; ++file)
        {
          checkInputFileForSequenceDetails(*file, config_file, styles_file, seq_maps_inout, merge_details, &tmp_error) ;

          if (tmp_error)
            {
              /* This is not a fatal error so just give a warning and continue */
              zMapLogWarning("Cannot open file '%s': %s", *file, tmp_error->message) ;
              consoleMsg(TRUE, "Cannot open file '%s': %s", *file, tmp_error->message) ;
              g_error_free(tmp_error) ;
              tmp_error = NULL ;
            }
        }

      g_strfreev(file_list) ;
    }

  if (tmp_error)
    g_propagate_error(error, tmp_error) ;
}


/* Validate the sequence details set in the sequence map. Sets the error if there is a problem. */
static gboolean validateSequenceDetails(GList *seq_maps, GError **error)
{
  gboolean result = TRUE ;
  GError *tmp_error = NULL ;
  
  GList *seq_map_item = seq_maps ;

  for ( ; seq_map_item && !tmp_error; seq_map_item = seq_map_item->next)
    {
      ZMapFeatureSequenceMap seq_map = (ZMapFeatureSequenceMap)(seq_map_item->data) ;

      /* The sequence name, start and end must all be specified, or none of them. */
      if ((seq_map->sequence && seq_map->start && seq_map->end)
          || (!(seq_map->sequence) && !(seq_map->start) && !(seq_map->end)))
        {
          /* We must have a config file or some files on the command line (otherwise we have no
           * sources) */
          if (seq_map->config_file || seq_map->file_list)
            {

            }
          else
            {
              result = FALSE ;

              g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_NO_SOURCES,
                          "No data sources - you must specify a config file, or pass data in GFF files on the command line") ;
            }
        }
      else
        {
          result = FALSE ;

          g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_BAD_SEQUENCE_DETAILS,
                      "Bad sequence args: must set all or none of sequence name, start and end. Got: %s, %d, %d",
                      (!seq_map->sequence ? "<no sequence name>" : seq_map->sequence),
                      seq_map->start, seq_map->end) ;
        }
    }

  if (tmp_error)
    g_propagate_error(error, tmp_error) ;

  return result ;
}


/* Check to see if user specied a sequence/start/end on the command line or in
 * a config file, only returns FALSE if they tried to specify it and it was
 * wrong. If FALSE an error message is returned in err_msg_out which should
 * be g_free'd by caller. */
static gboolean checkSequenceArgs(int argc, char *argv[],
                                  char *config_file, char *styles_file,
                                  GList **seq_maps_inout, GError **error)
{
  gboolean result = FALSE ;
  GError *tmp_error = NULL ;
  char *dataset = NULL ;
  char *sequence = NULL ;
  int start = 0 ;
  int end = 0 ;
  ZMapFeatureSequenceMap seq_map = NULL ;

  /* Check for sequence on command-line */
  if (!tmp_error)
    checkForCmdLineSequenceArg(argc, argv, &dataset, &sequence, &tmp_error) ;

  /* Check for coords on command-line */
  if (!tmp_error)
    checkForCmdLineStartEndArg(argc, argv, &start, &end, &tmp_error) ;

  /* Create the sequence map from the details so far (this is blank if none given) */
  if (!tmp_error)
    seq_map = createSequenceMap(sequence, start, end, config_file, styles_file, seq_maps_inout) ;

  /* Check for sequence details in the config file, if one was given. If the sequence details
   * are already set then this just validates the config file values against the existing ones. */
  if (!tmp_error)
    zMapAppGetSequenceConfig(seq_map, &tmp_error) ;

  /* Next check any input file(s). If there are multiple files for different sequences/regions
   * then we end up with multiple sequence maps */
  if (!tmp_error)
    checkInputFiles(config_file, styles_file, seq_maps_inout, &tmp_error) ;

  /* If details were set, check that they are valid. */
  if (!tmp_error)
    result = validateSequenceDetails(*seq_maps_inout, &tmp_error) ;

  if (tmp_error)
    g_propagate_error(error, tmp_error) ;

  return result ;
}


static void consoleMsg(gboolean err_msg, char *format, ...)
{
  va_list args ;
  char *msg_string ;
  FILE *stream ;

  va_start(args, format) ;
  msg_string = g_strdup_vprintf(format, args) ;
  va_end(args) ;

  if (err_msg)
    stream = stderr ;
  else
    stream = stdout ;

  /* show message and flush to make sure it appears in a timely way. */
  fprintf(stream, "%s\n", msg_string) ;
  fflush(stream) ;


  g_free(msg_string) ;

  return ;
}


/* Hide the zmap main window, currently just a trivial cover function for unmap call. */
static void hideMainWindow(ZMapAppContext app_context)
{
  gtk_widget_unmap(app_context->app_widg) ;


  return ;
}




/* A GSourceFunc, only called when there is a peer program. If there is no currently
 * displayed sequence and there has been no communication with the peer since before
 * the timeout started then we warn the user. The user can elect to continue or to
 * exit zmap. If the function returns FALSE it will not be called again.
 */
static gboolean remoteInactiveHandler(gpointer data)
{
  gboolean result = TRUE ;
  ZMapAppContext app_context = (ZMapAppContext)data ;
  guint zmap_num ;
  time_t current_time_s ;

  current_time_s = zMapUtilsGetRawTime() ;

  /* Only do this if there are no displayed zmaps and inactivity has been longer than timeout. */
  if (((current_time_s - app_context->remote_control->last_active_time_s) > ZMAP_APP_REMOTE_TIMEOUT_S)
      && !(zmap_num = zMapManagerCount(app_context->zmap_manager)))
    {
      char *msg ;
      long pid, ppid ;
      char *log_msg ;

      pid = getpid() ;
      ppid = getppid() ;

      log_msg = g_strdup_printf("This zmap (PID = %ld) has been running for more than %d mins"
                                " with no sequence displayed"
                                " and without any interaction with its peer \"%s\" (PID = %ld).",
                                pid, (ZMAP_APP_REMOTE_TIMEOUT_S / 60),
                                app_context->remote_control->peer_name, ppid) ;

      /*
       * (sm23) for debugging this must also be made available to logging and
       * stderr (sent to otterlace log)
       */
      zMapLogWarning("%s", log_msg) ;
      zMapDebugPrintf("%s", log_msg) ;

      msg = g_strdup_printf("%s Do you want to continue ?", log_msg) ;


      if (!zMapGUIMsgGetBoolFull((GtkWindow *)(app_context->app_widg), ZMAP_MSG_WARNING, msg,
                                 "Continue", "Exit"))
        {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          /* This temporary fix means we just exit without trying to tell the peer we are going.
           * The full fix needs our timeout and error handlers at this level to be redone. */
          zmapAppExit(app_context) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
          appExit(app_context) ;

          result = FALSE ;
        }

      g_free(log_msg) ;
      g_free(msg) ;
    }

  return result ;
}


