/*  File: zmapAppwindow.c
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
 *     Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and,
 *       Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *  Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Creates the first toplevel window in the zmap app.
 *
 * Exported functions: None
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapUtilsGUI.hpp>
#include <ZMap/zmapConfigDir.hpp>
#include <ZMap/zmapConfigIni.hpp>
#include <ZMap/zmapConfigStrings.hpp>
#include <ZMap/zmapControl.hpp>
#include <zmapApp_P.hpp>




static void initGnomeGTK(int argc, char *argv[]) ;
void quitReqCB(void *app_data, void *zmap_data_unused) ;
static void quitCB(GtkWidget *widget, gpointer cb_data) ;
static void destroyCB(GtkWidget *widget, gpointer cb_data) ;
static void topLevelDestroy(ZMapAppContext app_context) ;
static void killZMaps(ZMapAppContext app_context) ;
static gboolean timeoutHandler(gpointer data) ;
static void remoteLevelDestroy(ZMapAppContext app_context) ;
static void exitApp(ZMapAppContext app_context) ;
static void crashExitApp(ZMapAppContext app_context) ;
static void contextLevelDestroy(ZMapAppContext app_context, int exit_rc, const char *exit_msg) ;
static void killContext(ZMapAppContext app_context) ;
static void destroyAppContext(ZMapAppContext app_context) ;

static gboolean pingHandler(gpointer data) ;
static void hideMainWindow(ZMapAppContext app_context) ;





/*
 *                            Globals
 */



/* 
 *                    External routines.
 */


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

gboolean zmapMainMakeAppWindow(int argc, char *argv[], ZMapAppContext app_context, GList *seq_maps)
{
  gboolean result = FALSE ;
  GtkWidget *toplevel, *vbox, *menubar, *connect_frame, *manage_frame ;
  GtkWidget *quit_button ;
  PangoFont *tmp_font = NULL ;
  PangoFontDescription *tmp_font_desc = NULL ;
  int log_size ;


  /*             GTK initialisation              */

  zmapAppConsoleLogMsg(app_context->verbose_startup_logging, INIT_FORMAT, "Initing GTK.") ;

  initGnomeGTK(argc, argv) ;                                /* May exit if checks fail. */

#ifdef ZMAP_MEMORY_DEBUG
  g_mem_profile() ;
#endif


  zmapAppConsoleLogMsg(app_context->verbose_startup_logging, INIT_FORMAT, "Creating app window.") ;

  /* Set up app level cursors, the remote one will only be used if we have
   * a remote peer. */
  app_context->remote_busy_cursor = zMapGUICreateCursor("SB_H_DOUBLE_ARROW") ;


  app_context->app_widg = toplevel = zMapGUIToplevelNew(NULL, NULL) ;



  /* Now we have a widget, check for fixed width fonts, fatal if we can't find any
   * because we won't be able to display DNA or peptide sequence correctly. */
  GList *pref_families = NULL ;
  char *zoom_font_family = g_strdup(ZMAP_ZOOM_FONT_FAMILY) ;
  pref_families = g_list_append(pref_families, zoom_font_family) ;

  if(zMapGUIGetFixedWidthFont(GTK_WIDGET(toplevel),
                              pref_families,
                              ZMAP_ZOOM_FONT_SIZE, PANGO_WEIGHT_NORMAL,
                              &(tmp_font), &(tmp_font_desc)))
    {
      zmapAppConsoleLogMsg(app_context->verbose_startup_logging, INIT_FORMAT, "Fixed width font found.") ;
    }
  else
    {
      zMapLogWarning("Failed to find fixed width font: '%s'", ZMAP_ZOOM_FONT_FAMILY) ;
      zmapAppConsoleLogMsg(TRUE, "Failed to find fixed width font: '%s'", ZMAP_ZOOM_FONT_FAMILY) ;
      zmapAppDoTheExit(EXIT_FAILURE) ;
    }

  g_free(zoom_font_family) ;
  zoom_font_family = NULL ;

  g_list_free(pref_families) ;
  pref_families = NULL ;

  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;


  /* Handler for when main window is destroyed (by whichever route). */
  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
                     GTK_SIGNAL_FUNC(destroyCB), (gpointer)app_context) ;

  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(toplevel), vbox) ;

  menubar = zmapMainMakeMenuBar(app_context) ;
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);

  if (app_context->default_sequence)
    {
      app_context->default_sequence->constructSources(NULL, NULL) ;
    }

  connect_frame = zmapMainMakeConnect(app_context, app_context->default_sequence) ;
  gtk_box_pack_start(GTK_BOX(vbox), connect_frame, TRUE, TRUE, 0);

  manage_frame = zmapMainMakeManage(app_context) ;
  gtk_box_pack_start(GTK_BOX(vbox), manage_frame, TRUE, TRUE, 0);

  quit_button = gtk_button_new_with_label("Quit") ;
  gtk_signal_connect(GTK_OBJECT(quit_button), "clicked",
                     GTK_SIGNAL_FUNC(quitCB), (gpointer)app_context) ;
  gtk_box_pack_start(GTK_BOX(vbox), quit_button, FALSE, FALSE, 0) ;


  /* Always create the widgets, after this point they will all have windows. */
  gtk_widget_show_all(toplevel) ;

  zmapAppConsoleLogMsg(app_context->verbose_startup_logging, INIT_FORMAT, "Issued gtk_widget_show_all() to display all widgets.") ;

  /* Now we have a window we can set the standard cursor. */
  zMapGUISetCursor(toplevel, app_context->normal_cursor) ;


  /* We don't always want to show this window, for lace users it is useless.... */
  if (!(app_context->show_mainwindow))
    {
      zmapAppConsoleLogMsg(TRUE,  "ZMAP %s() - %s",
                           __PRETTY_FUNCTION__,
                           "Hiding main window.") ;

      hideMainWindow(app_context) ;

      zmapAppConsoleLogMsg(app_context->verbose_startup_logging, INIT_FORMAT, "Main window hidden.") ;
    }


  /* Check that log file has not got too big... */
  if ((log_size = zMapLogFileSize()) > (ZMAP_DEFAULT_MAX_LOG_SIZE * 1048576))
    zMapWarning("Log file was grown to %d bytes, you should think about archiving or removing it.", log_size) ;

  /* Only show default sequence if we are _not_ controlled via XRemote */
  if (!(app_context->remote_control))
    {
      gboolean ok = TRUE ;
      int num_views = seq_maps ? g_list_length(seq_maps) : 0 ;

      if (num_views > 1)
        {
          char *msg = g_strdup_printf("There are %d different sequences/regions in the input sources."
                                      " This will create %d views in ZMap. Are you sure you want to continue?",
                                      num_views, num_views) ;

          ok = zMapGUIMsgGetBool(NULL, ZMAP_MSG_WARNING, msg) ;
          g_free(msg) ;
        }

      if (ok)
        {
          ZMap zmap = NULL ;
          ZMapView view = NULL ;
          gboolean result ;

          result = zmapAppCreateZMap(app_context, app_context->default_sequence, &zmap, &view) ;
          
          if (result && num_views > 1)
            {
              /* There is more than one sequence map. We've added the first already but add
               * subsequence ones as new Views. */
              GList *seq_map_item = seq_maps->next ;

              for ( ; seq_map_item; seq_map_item = seq_map_item->next)
                {
                  ZMapFeatureSequenceMap seq_map = (ZMapFeatureSequenceMap)(seq_map_item->data) ;
                  char *err_msg = NULL ;

                  seq_map->constructSources(NULL, NULL) ;

                  zMapControlInsertView(zmap, seq_map, &err_msg) ;

                  if (err_msg)
                    {
                      zMapWarning("%s", err_msg) ;
                      g_free(err_msg) ;
                    }
                }
            }

          zmapAppConsoleLogMsg(app_context->verbose_startup_logging, INIT_FORMAT, "Displaying sequence(s).") ;
        }
    }


  app_context->state = ZMAPAPP_RUNNING ;


  zmapAppConsoleLogMsg(app_context->verbose_startup_logging, INIT_FORMAT, "Entering mainloop, startup finished.") ;


  /* Start the GUI. */
  gtk_main() ;

  /* If we get here then we will have exited successfully. */
  result = TRUE ;

  /* We should never get here, hence the failure return code. */
  return result ;
}



/* Calling this function will trigger the destroyCB() function which will kill the zmap app. */
void zmapAppSignalAppDestroy(ZMapAppContext app_context)
{
  gtk_widget_destroy(app_context->app_widg) ;

  return ;
}





/* Called when layers have signalled they want zmap to exit. */
void zmapAppQuitReq(ZMapAppContext app_context)
{
  /* Check if user has unsaved changes, gives them a chance to bomb out... */
  if (zMapManagerCheckIfUnsaved(app_context->zmap_manager))
    zmapAppSignalAppDestroy(app_context) ;
  else
    zMapLogMessage("%s", "Unsaved changes so cancelling exit." ) ;

  return ;
}


/* Not great....access needed from zmapAppManager */
void zmapAppRemoteLevelDestroy(ZMapAppContext app_context)
{
  remoteLevelDestroy(app_context) ;

  return ;
}


/* NEW VERSION THAT WILL ACTUALLY DO A WIDGET DESTROY ON THE TOP LEVEL...
 * 
 * DO NOT CALL FROM A DESTROY CALLBACK AS THIS CALL WILL RESULT IN THE DESTROY
 * CALLBACK BEING CALLED AGAIN.
 * 
 *  */
void zmapAppDestroy(ZMapAppContext app_context)
{
  /* Need warning if app_widg is NULL */

  zmapAppSignalAppDestroy(app_context) ;

  return ;
}


void zmapAppExit(ZMapAppContext app_context)
{

  exitApp(app_context) ;

  return ;
}



void zmapAppPingStart(ZMapAppContext app_context)
{
  int interval = app_context->ping_timeout * 1000 ;    /* glib needs time in milliseconds. */

  app_context->stop_pinging = FALSE ;

  /* try setting a timer to ping periodically.... */
  g_timeout_add(interval, pingHandler, (gpointer)app_context) ;

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

      zmapAppDoTheExit(EXIT_FAILURE) ;
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



/*
 *        callbacks that destroy the application
 * 
 * Before changing this you must appreciate that it's complicated because we have
 * to kill zmap views if they exist (widgets, attached threads etc) and that is asynchronous.
 * Then we have to kill the zmap app window and then disconnect from our peer if we
 * have one and then finally clear up.
 * 
 * Because of different ways that zmap can be run and the different states it
 * can be in the various destroy routines are called from different places, the
 * important thing is that although the destroy can be called at different
 * levels the path down through these levels is constant, i.e. you can only
 * choose the level at which you enter, not the path through.
 */



/* Called when user selects ZMap app window "quit" button or Cntl-Q. */
static void quitCB(GtkWidget *widget, gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;

  zmapAppSignalAppDestroy(app_context) ;

  return ;
}


/* Called when user selects the window manager "close" button or when signalAppDestroy()
 * has been called as this issues a destroy for the top level widget which has
 * the same effect.
 * 
 * NOTE that at this point the top level (app_widg) widget is no longer valid having
 * been destroyed by gtk prior to calling this routine so we set it to NULL
 * to avoid illegal attempts to use it.
 *  */
static void destroyCB(GtkWidget *widget, gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;

  zmapAppConsoleLogMsg(app_context->verbose_startup_logging, EXIT_FORMAT, "Main window destroyed, cleaning up.") ;

  app_context->app_widg = NULL ;

  topLevelDestroy(app_context) ;

  return ;
}






/*
 *               exit/cleanup routines.
 */

/* This is the first level of the destroy, if we have zmap view then
 * they must be killed first, this is asynchronous and we get called
 * back into remoteLevelDestroy() when all views have gone.
 * If we don't then we go on directly to the remoteLevelDestroy(). */
static void topLevelDestroy(ZMapAppContext app_context)
{
  /* Record we are dying so we know when to quit as last zmap dies. */
  app_context->state = ZMAPAPP_DYING ;

  /* If there zmaps left then clear them up, when the last one has gone then
   * get called back to remoteLevelDestroy().
   * Otherwise if there is a remote control then call remoteLevelDestroy(). */
  if ((zMapManagerCount(app_context->zmap_manager)))
    {
      zmapAppConsoleLogMsg(app_context->verbose_startup_logging, EXIT_FORMAT, "Killing existing views.") ;

      killZMaps(app_context) ;
    }
  else
    {
      zmapAppConsoleLogMsg(app_context->verbose_startup_logging, EXIT_FORMAT, "No views so preparing to exit.") ;

      remoteLevelDestroy(app_context) ;
    }

  return ;
}


/* Signal all views to die (including their widgets). */
static void killZMaps(ZMapAppContext app_context)
{
  int interval = app_context->exit_timeout * 1000 ;    /* glib needs time in milliseconds. */

  zmapAppConsoleLogMsg(TRUE, EXIT_FORMAT, "Issuing requests to all ZMaps to disconnect from servers and quit.") ;

  /* N.B. we block for 2 seconds here to make sure user can see message. */
  zMapGUIShowMsgFull(NULL, "ZMap is disconnecting from its servers and quitting, please wait.",
                     ZMAP_MSG_EXIT,
                     GTK_JUSTIFY_CENTER, 2, FALSE) ;

  /* time out func makes sure that we exit if threads fail to report back. */
  g_timeout_add(interval, timeoutHandler, (gpointer)app_context) ;

  /* Tell all our zmaps to die, they will tell all their threads to die, once they
   * are all gone we will be called back to exit. */
  zMapManagerKillAllZMaps(app_context->zmap_manager) ;

  return ;
}

/* A GSourceFunc, called if we take too long to kill zmaps in which case we force the exit. */
static gboolean timeoutHandler(gpointer data)
{
  ZMapAppContext app_context = (ZMapAppContext)data ;

  zmapAppConsoleLogMsg(app_context->verbose_startup_logging, EXIT_FORMAT, "ZMap views not dying, forced exit !") ;

  crashExitApp(app_context) ;

  return FALSE ;
}


/* If we have a remote peer we disconnect from it, this is asychronous
 * and we get called back into exitApp() once the peer has signalled
 * that we are saying goodbye.
 * If we don't then we call exitApp() straight away.
 */
static void remoteLevelDestroy(ZMapAppContext app_context)
{
  gboolean remote_disconnecting = FALSE ;

  /* If we have a remote client then disconnect, we will be called back
   * in exitApp().
   * Otherwise call exitApp() to exit. */
  if (app_context->remote_control)
    {
      gboolean app_exit = TRUE ;

      zmapAppConsoleLogMsg(app_context->verbose_startup_logging, EXIT_FORMAT, "Disconnecting from remote peer.") ;

      remote_disconnecting = zmapAppRemoteControlDisconnect(app_context, app_exit) ;
    }

  /* If the disconnect didn't happen or failed or there is no remote then exit. */
  if (!(app_context->remote_control) || !remote_disconnecting)
    {
      zmapAppConsoleLogMsg(app_context->verbose_startup_logging, EXIT_FORMAT, "No remote peer, proceeding with exit.") ;

      exitApp(app_context) ;
    }

  return ;
}


/* Called on clean exit of zmap. */
static void exitApp(ZMapAppContext app_context)
{
  zmapAppConsoleLogMsg(app_context->verbose_startup_logging, EXIT_FORMAT, "Destroying view manager.") ;

  /* This must be done here as manager checks to see if all its zmaps have gone. */
  if (app_context->zmap_manager)
    zMapManagerDestroy(app_context->zmap_manager) ;

  contextLevelDestroy(app_context, EXIT_SUCCESS, CLEAN_EXIT_MSG) ;    /* exits app. */

  return ;
}

/* Called when clean up of zmaps and their threads times out, stuff is not
 * cleaned up and the application does not exit cleanly. */
static void crashExitApp(ZMapAppContext app_context)
{
  const char *exit_msg = "Exit timed out - WARNING: Zmap clean up of threads timed out, exit has been forced !" ;

  contextLevelDestroy(app_context, EXIT_FAILURE, exit_msg) ;    /* exits app. */

  return ;
}


/* By now all asychronous stuff is over and it's a straight forward
 * freeing/releasing of all the resources. */
static void contextLevelDestroy(ZMapAppContext app_context, int exit_rc, const char *exit_msg)
{
  app_context->exit_rc = exit_rc ;
  app_context->exit_msg = exit_msg ;

  /* Destroy the remote control object, note that we have by now already
   * said goodbye to the peer. */
  if (app_context->remote_control)
    {
      zmapAppConsoleLogMsg(TRUE, EXIT_FORMAT, "Destroying remote control interface (remote peer already gone).") ;

      zmapAppRemoteControlDestroy(app_context) ;
    }

  killContext(app_context) ;

  return ;
}



/* This is the final clean up routine for the ZMap application and exits the program. */
static void killContext(ZMapAppContext app_context)
{
  int exit_rc = app_context->exit_rc ;
  const char *exit_msg = app_context->exit_msg ;

  zMapConfigDirDestroy();
  destroyAppContext(app_context) ;

  if (exit_rc)
    zMapLogCritical("%s", exit_msg) ;

  zmapAppConsoleLogMsg(TRUE, EXIT_FORMAT,  exit_msg) ;

  zMapWriteStopMsg() ;
  zMapLogDestroy() ;

  zmapAppDoTheExit(exit_rc) ;

  return ;
}


/* Hide the zmap main window, currently just a trivial cover function for unmap call. */
static void hideMainWindow(ZMapAppContext app_context)
{
  gtk_widget_unmap(app_context->app_widg) ;


  return ;
}





