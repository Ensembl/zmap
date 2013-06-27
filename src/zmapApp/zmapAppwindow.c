/*  File: zmapappmain.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
#include <ZMap/zmapCmdLineArgs.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>
#include <ZMap/zmapControl.h>
#include <zmapApp_P.h>




/* define this to get some memory debugging. */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#define ZMAP_MEMORY_DEBUG
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#define CLEAN_EXIT_MSG "Exit clean - goodbye cruel world !"

static void checkForCmdLineVersionArg(int argc, char *argv[]) ;
static int checkForCmdLineSleep(int argc, char *argv[]) ;
static void checkForCmdLineSequenceArg(int argc, char *argv[], char **dataset_out, char **sequence_out) ;
static void checkForCmdLineStartEndArg(int argc, char *argv[], int *start_inout, int *end_inout) ;

static char *checkConfigDir(void) ;
static gboolean checkSequenceArgs(int argc, char *argv[],
				  ZMapFeatureSequenceMap seq_map_inout, char **err_msg_out) ;

static gboolean checkPeerID(ZMapAppContext app_context,
			    char **peer_name_out, char **peer_clipboard_out, int *peer_retries, int *peer_timeout_ms) ;

static gboolean removeZMapRowForeachFunc(GtkTreeModel *model, GtkTreePath *path,
                                         GtkTreeIter *iter, gpointer data);



static void initGnomeGTK(int argc, char *argv[]) ;
static gboolean getConfiguration(ZMapAppContext app_context) ;

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
static void setup_signal_handlers(void);


static void remoteInstaller(GtkWidget *widget, GdkEvent *event, gpointer app_context_data) ;

static gboolean pingHandler(gpointer data) ;

static gboolean configureLog(char *config_file) ;
static void consoleMsg(gboolean err_msg, char *format, ...) ;


static ZMapManagerCallbacksStruct app_window_cbs_G = {addZMapCB, removeZMapCB, infoSetCB, quitReqCB, NULL} ;


#define ZMAPLOG_FILENAME "zmap.log"


char *ZMAP_X_PROGRAM_G  = "ZMap" ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
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
  ZMapFeatureSequenceMap seq_map;
  char *err_msg = NULL ;
  gboolean remote_control = FALSE ;



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


  /* Since thread support is crucial we do compile and run time checks that its all intialised.
   * the function calls look obscure but its what's recommended in the glib docs. */
#if !defined G_THREADS_ENABLED || defined G_THREADS_IMPL_NONE || !defined G_THREADS_IMPL_POSIX
#error "Cannot compile, threads not properly enabled."
#endif

  /* Make sure glib threading is supported and initialised. */
  g_thread_init(NULL) ;
  if (!g_thread_supported())
    g_thread_init(NULL);

  setup_signal_handlers();

  /* Set up user type, i.e. developer or normal user. */
  zMapUtilsUserInit() ;

  /* Set up command line parsing object, globally available anywhere, this function exits if
   * there are bad command line args. */
  zMapCmdLineArgsCreate(&argc, argv) ;

  /* If user specified version flag, show zmap version and exit with zero return code. */
  checkForCmdLineVersionArg(argc, argv) ;

  /* The main control block. */
  app_context = createAppContext() ;


  /* default sequence to display -> if not run via XRemote (window_ID in cmd line args) */
  seq_map = app_context->default_sequence =  g_new0(ZMapFeatureSequenceMapStruct, 1) ;


  /* Set up configuration directory/files, this function exits if the directory/files can't be
   * accessed.... */
  seq_map->config_file = checkConfigDir() ;


  /* Set any global debug flags from config file. */
  zMapUtilsConfigDebug(NULL) ;

  /* Check if a peer program was specified on the command line or in the config file. */
  peer_name = peer_clipboard = NULL ;
  peer_retries = peer_timeout_ms = -1 ;
  if (!checkPeerID(app_context, &peer_name, &peer_clipboard, &peer_retries, &peer_timeout_ms))
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
  if (!zMapLogCreate(NULL) || !configureLog(seq_map->config_file))
    {
      consoleMsg(TRUE, "%s", "ZMap cannot create log file.") ;
      doTheExit(EXIT_FAILURE) ;
    }
  else
    {
      zMapWriteStartMsg() ;
    }

  /* Get general zmap configuration from config. file. */
  getConfiguration(app_context) ;


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
  if (!checkSequenceArgs(argc, argv, seq_map, &err_msg))
    {
      zMapLogCritical("%s", err_msg) ;
      consoleMsg(TRUE, "%s", err_msg) ;
      doTheExit(EXIT_FAILURE) ;
    }


  /*             GTK initialisation              */

  initGnomeGTK(argc, argv) ;					    /* May exit if checks fail. */

#ifdef ZMAP_MEMORY_DEBUG
  g_mem_profile() ;
#endif

  app_context->app_widg = toplevel = zMapGUIToplevelNew(NULL, NULL) ;

  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;

  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;


  /* **NEW XREMOTE** THIS IS THE NEW HANDLER... */
  if (remote_control)
    {
      if (zmapAppRemoteControlCreate(app_context, peer_name, peer_clipboard, peer_retries, peer_timeout_ms))
	{
	  /* Rest of initialisation requires a window so code is in a map callback
	   * where we are guaranteed to have a window. */
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

  connect_frame = zmapMainMakeConnect(app_context, seq_map) ;
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
    gtk_widget_unmap(toplevel) ;


  /* Check that log file has not got too big... */
  if ((log_size = zMapLogFileSize()) > (ZMAP_DEFAULT_MAX_LOG_SIZE * 1048576))
    zMapWarning("Log file was grown to %d bytes, you should think about archiving or removing it.", log_size) ;


  /* Only show default sequence if we are _not_ controlled via XRemote */
  if (!remote_control)
     {
      ZMap zmap = NULL ;
      ZMapView view = NULL ;
      char *err_msg = NULL ;

      if (!zmapAppCreateZMap(app_context, seq_map, &zmap, &view, &err_msg))
	zMapWarning("%s", err_msg) ;
    }

  app_context->state = ZMAPAPP_RUNNING ;

  /* Start the GUI. */
  gtk_main() ;

  doTheExit(EXIT_SUCCESS) ;				    /* exits.... */


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
  int interval = app_context->ping_timeout * 1000 ;	    /* glib needs time in milliseconds. */

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

  app_context->sent_finalised = FALSE;

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

  return TRUE ;
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

  finalCleanUp(app_context) ;				    /* exits program. */

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
      guint timeout_func_id ;
      int interval = app_context->exit_timeout * 1000 ;	    /* glib needs time in milliseconds. */

      zMapLogMessage("%s", "Issuing requests to all ZMaps to disconnect from servers and quit.") ;

      /* N.B. we block for 2 seconds here to make sure user can see message. */
      zMapGUIShowMsgFull(NULL, "ZMap is disconnecting from its servers and quitting, please wait.",
			 ZMAP_MSG_EXIT,
			 GTK_JUSTIFY_CENTER, 2, FALSE) ;

      /* time out func makes sure that we exit if threads fail to report back. */
      timeout_func_id = g_timeout_add(interval, timeoutHandler, (gpointer)app_context) ;
      zMapAssert(timeout_func_id) ;

      /* Tell all our zmaps to die, they will tell all their threads to die. */
      zMapManagerKillAllZMaps(app_context->zmap_manager) ;
    }

  return ;
}





/* Called on clean exit of zmap. */
static void exitApp(ZMapAppContext app_context)
{
/* This must be done here as manager checks to see if all its zmaps have gone. */
  if (app_context->zmap_manager)
    zMapManagerDestroy(app_context->zmap_manager) ;

  signalFinalCleanUp(app_context, EXIT_SUCCESS, CLEAN_EXIT_MSG) ;	    /* exits app. */

  return ;
}

/* Called when clean up of zmaps and their threads times out, stuff is not
 * cleaned up and the application does not exit cleanly. */
static void crashExitApp(ZMapAppContext app_context)
{
  char *exit_msg = "Exit timed out - WARNING: Zmap clean up of threads timed out, exit has been forced !" ;

  signalFinalCleanUp(app_context, EXIT_FAILURE, exit_msg) ;	    /* exits app. */

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
    zMapLogMessage("%s", exit_msg) ;

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

  return ;						    /* we never get here. */
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
static void checkForCmdLineSequenceArg(int argc, char *argv[], char **dataset_out, char **sequence_out)
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
static void checkForCmdLineStartEndArg(int argc, char *argv[], int *start_inout, int *end_inout)
{
  ZMapCmdLineArgsType value ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  int start = *start_inout, end = *end_inout ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  if (zMapCmdLineArgsValue(ZMAPARG_SEQUENCE_START, &value))
    *start_inout = value.i ;
  if (zMapCmdLineArgsValue(ZMAPARG_SEQUENCE_END, &value))
    *end_inout = value.i ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (start != *start_inout || end != *end_inout)
    {
      if (start < 1 || (end != 0 && end < start))
	{
	  fprintf(stderr, "Bad start/end values: start = %d, end = %d\n", start, end) ;
	  doTheExit(EXIT_FAILURE) ;
	}
      else
	{
	  *start_inout = start ;
	  *end_inout = end ;
	}
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

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




/* Did user specify a config directory and/or config file within that directory on the command line. */
static char *checkConfigDir(void)
{
  char *config_file = NULL ;
  ZMapCmdLineArgsType dir = {FALSE}, file = {FALSE};

  zMapCmdLineArgsValue(ZMAPARG_CONFIG_DIR, &dir) ;
  zMapCmdLineArgsValue(ZMAPARG_CONFIG_FILE, &file) ;


  if (!zMapConfigDirCreate(dir.s, file.s))
    {
	fprintf(stderr, "Could not access either/both of configuration directory \"%s\" "
	      "or file \"%s\" within that directory.\n",
	      zMapConfigDirGetDir(), zMapConfigDirGetFile()) ;
      doTheExit(EXIT_FAILURE) ;

    }
  else
    {
      config_file = zMapConfigDirGetFile() ;
    }

  return config_file ;
}


/* Did caller specify a peer program id on the command line or in a config file. */
static gboolean checkPeerID(ZMapAppContext app_context,
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
  
  if ((context = zMapConfigIniContextProvide(app_context->default_sequence->config_file)))
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
				     ZMAPSTANZA_PEER_RETRIES, &tmp_int))
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
static gboolean getConfiguration(ZMapAppContext app_context)
{
  gboolean result = FALSE ;
  ZMapConfigIniContext context;

  app_context->show_mainwindow = TRUE;
  app_context->exit_timeout = ZMAP_DEFAULT_EXIT_TIMEOUT;

  if ((context = zMapConfigIniContextProvide(app_context->default_sequence->config_file)))
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

      /* locale to use */
      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
					ZMAPSTANZA_APP_LOCALE, &tmp_string))
	app_context->locale = tmp_string;

      /* Turn on/off debugging for xremote connection to peer client. */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
					 ZMAPSTANZA_APP_XREMOTE_DEBUG, &tmp_bool))
	{
	  app_context->xremote_debug = tmp_bool ;
	  zMapXRemoteSetDebug(app_context->xremote_debug) ;
	}

      zMapConfigIniContextDestroy(context);

      result = TRUE;
    }

  return result ;
}


/* Called  */
static void setup_signal_handlers(void)
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





  /* Hide main window if required. */
  if (!(app_context->show_mainwindow) && app_context->defer_hiding)
    gtk_widget_unmap(app_context->app_widg) ;

  return ;
}


/* Read logging configuration from ZMap stanza and apply to log. */
static gboolean configureLog(char *config_file)
{
  gboolean result = TRUE ;	/* if no config, we can't fail to configure */
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
  /* if we run config free we put the log file in the cwd */
  full_dir = g_strdup("./") ;
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
	  g_free (full_dir);
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

  logfile_path = zMapGetFile(full_dir, log_name, TRUE) ;

  /* all our strings need freeing */
  g_free(log_name) ;
  g_free(full_dir) ;

  result = zMapLogConfigure(logging, log_to_file,
			    show_process, show_code, show_time,
			    catch_glib, echo_glib,
			    logfile_path) ;

  return result ;
}


/* Check to see if user specied a sequence/start/end on the command line or in
 * a config file, only returns FALSE if they tried to specify it and it was
 * wrong. If FALSE an error message is returned in err_msg_out which should
 * be g_free'd by caller. */
static gboolean checkSequenceArgs(int argc, char *argv[],
				  ZMapFeatureSequenceMap seq_map_inout, char **err_msg_out)
{
  gboolean result = FALSE ;
  char *source = NULL ;

  /* Check command line first, calls will exit if there is a problem if flag is completely wrong. */
  checkForCmdLineSequenceArg(argc, argv, &seq_map_inout->dataset, &seq_map_inout->sequence);
  checkForCmdLineStartEndArg(argc, argv, &seq_map_inout->start, &seq_map_inout->end) ;

    /* Nothing specified on command line so check config file. */
  if (!(seq_map_inout->sequence) && !(seq_map_inout->start) && !(seq_map_inout->end))
    {
      zMapAppGetSequenceConfig(seq_map_inout) ;

      source = "in config file" ;
    }
  else
    {
      source = "on command line" ;
    }

  /* Everything must be specified or nothing. */
  if ((seq_map_inout->sequence && seq_map_inout->start && seq_map_inout->end)
      || (!(seq_map_inout->sequence) && !(seq_map_inout->start) && !(seq_map_inout->end)))
    {
      result = TRUE ;
    }
  else
    {
      result = FALSE ;
      *err_msg_out = g_strdup_printf("Bad sequence args %s: %s",
				     source,
				     (!seq_map_inout->sequence ? "no sequence name"
				      : (seq_map_inout->start <= 1 ? "start less than 1" : "end less than start"))) ;
    }

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

  fprintf(stream, "%s\n", msg_string) ;

  g_free(msg_string) ;

  return ;
}

