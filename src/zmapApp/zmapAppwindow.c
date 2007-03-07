/*  File: zmapappmain.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Creates the first toplevel window in the zmap app.
 *              
 * Exported functions: None
 * HISTORY:
 * Last edited: Mar  7 14:43 2007 (edgrif)
 * Created: Thu Jul 24 14:36:27 2003 (edgrif)
 * CVS info:   $Id: zmapAppwindow.c,v 1.36 2007-03-07 14:45:35 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapCmdLineArgs.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapConfig.h>
#include <ZMap/zmapControl.h> 
#include <zmapApp_P.h>


static void toplevelDestroyCB(GtkWidget *widget, gpointer data) ;
static void quitCB(GtkWidget *widget, gpointer cb_data) ;

static void checkForCmdLineVersionArg(int argc, char *argv[]) ;
static void checkForCmdLineSequenceArgs(int argc, char *argv[],
					char **sequence_out, int *start_out, int *end_out) ;
static void checkConfigDir(void) ;
static gboolean removeZMapRowForeachFunc(GtkTreeModel *model, GtkTreePath *path,
                                         GtkTreeIter *iter, gpointer data);

static void infoSetCB(void *app_data, void *zmap) ;
static void removeZMapCB(void *app_data, void *zmap) ;
void quitReqCB(void *app_data, void *zmap_data_unused) ;

static void initGnomeGTK(int argc, char *argv[]) ;
static ZMapAppContext createAppContext(void) ;
static void exitApp(ZMapAppContext app_context) ;
static gboolean getConfiguration(ZMapAppContext app_context) ;


ZMapManagerCallbacksStruct app_window_cbs_G = {removeZMapCB, infoSetCB, quitReqCB} ;

int test_global = 10 ;
int test_overlap = 0 ;




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Place a copyright notice in the executable. */
char *obj_copyright_G = ZMAP_OBJ_COPYRIGHT_STRING(ZMAP_TITLE,
						  ZMAP_VERSION, ZMAP_RELEASE, ZMAP_UPDATE,
						  ZMAP_DESCRIPTION) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



int zmapMainMakeAppWindow(int argc, char *argv[])
{
  ZMapAppContext app_context ;
  GtkWidget *toplevel, *vbox, *menubar, *connect_frame, *manage_frame ;
  GtkWidget *quit_button ;
  char *sequence ;
  int start, end ;
  /* AppRealiseData app_data = g_new0(AppRealiseDataStruct, 1); */


  /*       Application initialisation.        */

  /* Since thread support is crucial we do compile and run time checks that its all intialised.
   * the function calls look obscure but its what's recommended in the glib docs. */
#if !defined G_THREADS_ENABLED || defined G_THREADS_IMPL_NONE || !defined G_THREADS_IMPL_POSIX
#error "Cannot compile, threads not properly enabled."
#endif

  g_thread_init(NULL) ;
  if (!g_thread_supported())
    g_thread_init(NULL);


  /* Set up command line parsing object, globally available anywhere, this function exits if
   * there are bad command line args. */
  zMapCmdLineArgsCreate(argc, argv) ;


  /* If user specified version flag, show zmap version and exit with zero return code. */
  checkForCmdLineVersionArg(argc, argv) ;


  /* Set up configuration directory/files, this function exits if the directory/files can't be
   * accessed. */
  checkConfigDir() ;


  /* Init manager, just happen just once in application. */
  zMapManagerInit(&app_window_cbs_G) ;

  /* app_data->app_context = */
  app_context = createAppContext() ;

  /* Set up logging for application. */
  app_context->logger = zMapLogCreate(NULL) ;

  getConfiguration(app_context) ;

  /*             GTK initialisation              */

  initGnomeGTK(argc, argv) ;					    /* May exit if checks fail. */
  app_context->app_widg = toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_window_set_title(GTK_WINDOW(toplevel), "ZMap - Son of FMap !") ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;

#ifdef RDS_DONT_INCLUDE_TESTING
  if(app_context->event_model &&
     (g_ascii_strncasecmp(app_context->event_model, "realize", 7)) == 0)
    g_signal_connect_after(G_OBJECT(toplevel), "realize",
                           G_CALLBACK(zmapAppRemoteInstaller), 
                           (gpointer)app_context);
  else if(app_context->event_model &&
          (g_ascii_strncasecmp(app_context->event_model, "expose_event", 12)) == 0)
    app_data->signal_id =  
      g_signal_connect_data(G_OBJECT(toplevel), "expose_event",
                            G_CALLBACK(exposeWorkAroundRealiseHandler), 
                            (gpointer)app_data,
                            (GClosureNotify)(appRealiseDataDestroy), 
                            0);
  else if(app_context->event_model &&
          (g_ascii_strncasecmp(app_context->event_model, "map", 3)) == 0)
    g_signal_connect(G_OBJECT(toplevel), "map",
                     G_CALLBACK(zmapAppRemoteInstaller), 
                     (gpointer)app_context);
  else
    g_signal_connect_after(G_OBJECT(toplevel), "realize",
                           G_CALLBACK(zmapAppRemoteInstaller), 
                           (gpointer)app_context);

  g_signal_connect(G_OBJECT(toplevel), "realize",
                   G_CALLBACK(appRealizeCallback), 
                   NULL);
  g_signal_connect(G_OBJECT(toplevel), "expose_event",
                   G_CALLBACK(appExposeCallback), 
                   NULL);
  g_signal_connect(G_OBJECT(toplevel), "map",
                   G_CALLBACK(appMapCallback), 
                   (gpointer)app_context);
  g_signal_connect(G_OBJECT(toplevel), "map_event",
                   G_CALLBACK(appMapEventCallback), 
                   NULL);
#endif /* RDS_DONT_INCLUDE_TESTING */


  /* This ensures that the widget *really* has a X Window id when it
   * comes to doing XChangeProperty.  Using realize doesn't and the 
   * expose_event means we can't hide the mainwindow. */
  g_signal_connect(G_OBJECT(toplevel), "map",
                   G_CALLBACK(zmapAppRemoteInstaller), 
                   (gpointer)app_context);
  
  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy", 
		     GTK_SIGNAL_FUNC(toplevelDestroyCB), (gpointer)app_context) ;

  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(toplevel), vbox) ;

  menubar = zmapMainMakeMenuBar(app_context) ;
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);

  connect_frame = zmapMainMakeConnect(app_context) ;
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
  if (!(app_context->show_mainwindow))
    gtk_widget_unmap(toplevel) ;


  /* If user specifyed a sequence in the config. file or on the command line then
   * display it straight away. */
  sequence = app_context->default_sequence ;
  start = 1 ;
  end = 0 ;

  if (!sequence)
    checkForCmdLineSequenceArgs(argc, argv, &sequence, &start, &end) ;
							    /* May exit if bad cmdline args. */
  if (sequence)
    {
      zmapAppCreateZMap(app_context, sequence, start, end) ;
    }

  app_context->state = ZMAPAPP_RUNNING ;



  /* Start the GUI. */
  gtk_main() ;



  zMapExit(EXIT_SUCCESS) ;				    /* exits.... */


  /* We should never get here, hence the failure return code. */
  return(EXIT_FAILURE) ;
}



/* Signals zmap to clean up and exit, this is asynchronous because of the underlying
 * threads. */
void zmapAppExit(ZMapAppContext app_context)
{
  /* Causes the destroy callback to be invoked which then cleans up. */
  gtk_widget_destroy(app_context->app_widg);

  return ;
}




/*
 *  ------------------- Internal functions -------------------
 */

static void initGnomeGTK(int argc, char *argv[])
{
  gchar *err_msg ;
  gchar *rc_dir, *rc_file;
  gtk_set_locale() ;            

  if ((err_msg = gtk_check_version(ZMAP_GTK_MAJOR, ZMAP_GTK_MINOR, ZMAP_GTK_MICRO)))
    {
      zMapLogCritical("%s\n", err_msg) ;
      zMapExitMsg("%s\n", err_msg) ;

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

  app_context->app_widg = app_context->sequence_widg = NULL ;
  app_context->tree_store_widg = NULL ;

  app_context->zmap_manager = zMapManagerCreate((void *)app_context) ;
  app_context->selected_zmap = NULL ;

  app_context->logger = NULL ;

  return app_context ;
}



/* This function gets called whenever there is a gtk_widget_destroy() to the top level
 * widget. Sometimes this is because of window manager action, sometimes one of our exit
 * routines does a gtk_widget_destroy() on the top level widget.
 * 
 * NOTE that we do not need to destroy the widget tree, it has already been destroyed.
 *  */
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;

  /* Record we are dying so we know when to quit as last zmap dies. */
  app_context->state = ZMAPAPP_DYING ;

  zMapManagerKillAllZMaps(app_context->zmap_manager) ;

  /* If we have a client object, clean it up */
  if(app_context->xremoteClient != NULL)
    zMapXRemoteDestroy(app_context->xremoteClient);

  return ;
}


/* Called when user clicks the big "Quit" button in the main window. */
static void quitCB(GtkWidget *widget, gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;

  zmapAppExit(app_context) ;

  return ;
}


/* Called every time a zmap window signals that it has gone. */
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

  /* When the last zmap has gone and we are dying then exit. */
  if ((app_context->state == ZMAPAPP_DYING || !(app_context->show_mainwindow))
      && ((zMapManagerCount(app_context->zmap_manager)) == 0))
    exitApp(app_context) ;

  return ;
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

/* Did user specify seqence/start/end on command line. */
static void checkForCmdLineSequenceArgs(int argc, char *argv[],
					char **sequence_out, int *start_out, int *end_out)
{
  ZMapCmdLineArgsType value ;
  char *sequence ;
  int start = 1, end = 0 ;

  if ((sequence = zMapCmdLineFinalArg()))
    {
    
      if (zMapCmdLineArgsValue(ZMAPARG_SEQUENCE_START, &value))
	start = value.i ;
      if (zMapCmdLineArgsValue(ZMAPARG_SEQUENCE_END, &value))
	end = value.i ;

      if (start != 1 || end != 0)
	{
	  if (start < 1 || (end != 0 && end < start))
	    {
	      fprintf(stderr, "Bad start/end values: start = %d, end = %d\n", start, end) ;
	      zMapExit(EXIT_FAILURE) ;
	    }
	}

      *sequence_out = sequence ;
      *start_out = start ;
      *end_out = end ;
    }


 
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
      printf("%s - %s\n", zMapGetAppName(), zMapGetVersionString()) ;

      exit(EXIT_SUCCESS) ;
    }

 
  return ;
}





/* Did user specify a config directory and/or config file within that directory on the command line. */
static void checkConfigDir(void)
{
  ZMapCmdLineArgsType dir = {FALSE}, file = {FALSE} ;

  zMapCmdLineArgsValue(ZMAPARG_CONFIG_DIR, &dir) ;
  zMapCmdLineArgsValue(ZMAPARG_CONFIG_FILE, &file) ;

  if (!zMapConfigDirCreate(dir.s, file.s))
    {
      fprintf(stderr, "Could not access either/both of configuration directory \"%s\" "
	      "or file \"%s\" within that directory.\n",
	      zMapConfigDirGetDir(), zMapConfigDirGetFile()) ;
      zMapExit(EXIT_FAILURE) ;
    }

  return ;
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
static void infoSetCB(void *app_data, void *zmap)
{
  ZMapAppContext app = (ZMapAppContext)app_data;
  ZMap info_zmap = (ZMap)zmap;

  g_clear_error(&(app->info));
  /* Suck as much as we can from this zmap. Make it into xml perl can
   * parse that as easily as we can make it here. We should probably be 
   * escaping the printf string pointers (I think there's a glib function)
   * and also propagating any info/error into the appcontext's info too.
   * I've not specified any schema for this xml.  It seems too simple 
   * to require one at the moment.
   */
  app->info = 
    g_error_new(g_quark_from_string(__FILE__), /* Not sure why we need a domain, we're not gonna use it */
                1,
                "<zmapid>%s</zmapid><windowid>0x%lx</windowid>",
                zMapGetZMapID(info_zmap),
                zMapGetXID(info_zmap)
                );

}


/* Called from layers below when they want zmap to exit. */
void quitReqCB(void *app_data, void *zmap_data_unused)
{
  ZMapAppContext app_context = (ZMapAppContext)app_data ;

  zmapAppExit(app_context) ;

  return ;
}



/* Final clean up of zmap. */
static void exitApp(ZMapAppContext app_context)
{
  /* Destroy the manager. */
  zMapManagerDestroy(app_context->zmap_manager) ;

  /* This should probably be the last thing before exitting... */
  if (app_context->logger)
    {
      zMapLogMessage("%s", "Goodbye cruel world !") ;

      zMapLogDestroy(app_context->logger) ;
    }

  zMapExit(EXIT_SUCCESS) ;

  return ;
}



/* Read ZMap application defaults. */
static gboolean getConfiguration(ZMapAppContext app_context)
{
  gboolean result = FALSE ;
  ZMapConfig config ;
  ZMapConfigStanzaSet zmap_list = NULL ;
  ZMapConfigStanza zmap_stanza ;
  char *zmap_stanza_name = ZMAPSTANZA_APP_CONFIG ;
  ZMapConfigStanzaElementStruct zmap_elements[] = {{ZMAPSTANZA_APP_MAINWINDOW, ZMAPCONFIG_BOOL,   {NULL}},
						   {ZMAPSTANZA_APP_SEQUENCE,   ZMAPCONFIG_STRING, {NULL}},
                                                   /* {"event_model", ZMAPCONFIG_STRING, {NULL}}, */
						   {NULL, -1, {NULL}}} ;



  app_context->show_mainwindow
    = zMapConfigGetStructBool(zmap_elements, ZMAPSTANZA_APP_MAINWINDOW) = TRUE ;
							    /* By default show main window. */

  if ((config = zMapConfigCreate()))
    {
      zmap_stanza = zMapConfigMakeStanza(zmap_stanza_name, zmap_elements) ;

      if (zMapConfigFindStanzas(config, zmap_stanza, &zmap_list))
	{
	  ZMapConfigStanza next_zmap ;
	  
	  /* Get the first zmap stanza found, we will ignore any others. */
	  next_zmap = zMapConfigGetNextStanza(zmap_list, NULL) ;
	  
	  app_context->show_mainwindow = zMapConfigGetElementBool(next_zmap, ZMAPSTANZA_APP_MAINWINDOW) ;
	  if ((app_context->default_sequence
	       = zMapConfigGetElementString(next_zmap, ZMAPSTANZA_APP_SEQUENCE)))
	    app_context->default_sequence = g_strdup_printf(app_context->default_sequence) ;

          /*	  
          if ((app_context->event_model
	       = zMapConfigGetElementString(next_zmap, "event_model")))
	    app_context->event_model = g_strdup_printf(app_context->event_model) ;
          */

	  zMapConfigDeleteStanzaSet(zmap_list) ;		    /* Not needed anymore. */
	}
      
      zMapConfigDestroyStanza(zmap_stanza) ;
      
      zMapConfigDestroy(config) ;

      result = TRUE ;
    }

  return result ;
}



#ifdef RDS_DONT_INCLUDE_TESTING
typedef struct
{
  ZMapAppContext app_context;
  gulong signal_id;
}AppRealiseDataStruct, *AppRealiseData;

static gboolean exposeWorkAroundRealiseHandler(GtkWidget *widget, 
                                               GdkEventExpose *expose, 
                                               gpointer user_data)
{
  gboolean handled = TRUE;
  AppRealiseData app_data = (AppRealiseData)user_data;
  ZMapAppContext app_context = NULL;

  printf("[zmapAppwindow] Expose workaround callback\n");
  
  zMapAssert(GTK_WIDGET_REALIZED(widget));
  zMapAssert(app_data && app_data->signal_id);
  app_context = app_data->app_context;
  g_signal_handler_disconnect(G_OBJECT(widget), app_data->signal_id);

  zmapAppRemoteInstaller(widget, app_context);

  return handled;
}

static void appRealiseDataDestroy(gpointer user_data)
{
  AppRealiseData app_data = (AppRealiseData)user_data;

  app_data->app_context = NULL;
  app_data->signal_id   = 0;

  g_free(app_data);

  return ;
}

static gboolean appExposeCallback(GtkWidget *widget, 
                                  GdkEventExpose *expose, 
                                  gpointer user_data)
{
  printf("[zmapAppwindow] Expose callback\n");  
  return FALSE;
}
static void appRealizeCallback(GtkWidget *widget, gpointer user_data)
{
  printf("[zmapAppwindow] Realize callback\n");
  return ;
}
static void appMapCallback(GtkWidget *widget, gpointer user_data)
{
  printf("[zmapAppwindow] Map callback\n");
  return ;
}
static gboolean appMapEventCallback(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  gboolean handled = FALSE;
  printf("[zmapAppwindow] Map Event callback\n");
  return handled;
}
#endif /* RDS_DONT_INCLUDE_TESTING */

