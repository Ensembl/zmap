/*  File: zmapappmain.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Feb 17 09:52 2006 (edgrif)
 * Created: Thu Jul 24 14:36:27 2003 (edgrif)
 * CVS info:   $Id: zmapAppwindow.c,v 1.23 2006-02-17 10:44:32 edgrif Exp $
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
#include <ZMap/zmapControl.h> 
#include <zmapApp_P.h>


static void initGnomeGTK(int argc, char *argv[]) ;
static ZMapAppContext createAppContext(void) ;
static void quitCB(GtkWidget *widget, gpointer data) ;
static void removeZmapRow(void *app_data, void *zmap) ;
static void infoSetCB(void *app_data, void *zmap) ;
static void checkForCmdLineVersionArg(int argc, char *argv[]) ;
static void checkForCmdLineSequenceArgs(int argc, char *argv[],
					char **sequence_out, int *start_out, int *end_out) ;
static void checkConfigDir(void) ;
static gboolean removeZmapRowForeachFunc(GtkTreeModel *model, GtkTreePath *path,
                                         GtkTreeIter *iter, gpointer data);

void exitCB(void *app_data, void *zmap_data_unused) ;
static void appExit(ZMapAppContext app_context) ;



ZMapManagerCallbacksStruct app_window_cbs_G = {removeZmapRow, infoSetCB, exitCB} ;

int test_global = 10 ;
int test_overlap = 0 ;


int zmapMainMakeAppWindow(int argc, char *argv[])
{
  ZMapAppContext app_context ;
  GtkWidget *toplevel, *vbox, *menubar, *connect_frame, *manage_frame ;
  GtkWidget *quit_button ;

  char *sequence ;
  int start, end ;

  /* this all needs revving up sometime.... */
  if (argc > 1)
    test_global = atoi(argv[1]) ;

  if (argc > 2)
    test_overlap = 1 ;


  /* 
   *       Application initialisation. 
   */


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


  app_context = createAppContext() ;


  /* Set up logging for application. */
  app_context->logger = zMapLogCreate(NULL) ;




  /* 
   *             GTK initialisation 
   */

  initGnomeGTK(argc, argv) ;					    /* May exit if checks fail. */

  app_context->app_widg = toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_window_set_title(GTK_WINDOW(toplevel), "ZMap - Son of FMap !") ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;

  g_signal_connect(G_OBJECT(toplevel), "realize",
                   G_CALLBACK(zmapAppRemoteInstaller), (gpointer)app_context);
  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy", 
		     GTK_SIGNAL_FUNC(quitCB), (gpointer)app_context) ;

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

  gtk_widget_show_all(toplevel) ;


  /* Did user specify a sequence on the command line. */
  sequence = NULL ;
  start = 1 ;
  end = 0 ;
  checkForCmdLineSequenceArgs(argc, argv, &sequence, &start, &end) ; /* May exit if bad cmdline args. */
  if (sequence)
    {
      zmapAppCreateZMap(app_context, sequence, start, end) ;
    }


  /* 
   *       Start the GUI. 
   */

  gtk_main() ;



  zMapExit(EXIT_SUCCESS) ;				    /* exits.... */


  /* We should never get here, hence the failure return code. */
  return(EXIT_FAILURE) ;
}


void zmapAppExit(ZMapAppContext app_context)
{


  /* This should probably be the last thing before exitting... */
  if (app_context->logger)
    {
      zMapLogMessage("%s", "Goodbye cruel world !") ;

      zMapLogDestroy(app_context->logger) ;
    }

  zMapExit(EXIT_SUCCESS) ;
}



/*
 *  ------------------- Internal functions -------------------
 */

static void initGnomeGTK(int argc, char *argv[])
{
  gchar *err_msg ;

  gtk_set_locale() ;

  if ((err_msg = gtk_check_version(ZMAP_GTK_MAJOR, ZMAP_GTK_MINOR, ZMAP_GTK_MICRO)))
    {
      zMapLogCritical("%s\n", err_msg) ;
      zMapExitMsg("%s\n", err_msg) ;

      gtk_exit(EXIT_FAILURE) ;
    }

  gtk_init(&argc, &argv) ;

  return ;
}


static ZMapAppContext createAppContext(void)
{
  ZMapAppContext app_context ;

  app_context = g_new0(ZMapAppContextStruct, 1) ;

  app_context->app_widg = app_context->sequence_widg = NULL ;
  app_context->tree_store_widg = NULL ;

  app_context->zmap_manager = zMapManagerCreate((void *)app_context) ;
  app_context->selected_zmap = NULL ;

  app_context->logger = NULL ;

  return app_context ;
}


static void quitCB(GtkWidget *widget, gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;

  appExit(app_context) ;

  return ;
}


void removeZmapRow(void *app_data, void *zmap_data)
{
  ZMapAppContext app_context = (ZMapAppContext)app_data ;
  ZMap zmap = (ZMap)zmap_data ;

  /* This has the potential to remove multiple rows, but currently
     doesn't as the first found one that matches, gets removed an
     returns true. See
     http://scentric.net/tutorial/sec-treemodel-remove-many-rows.html
     for an implementation of mutliple deletes */
  gtk_tree_model_foreach(GTK_TREE_MODEL(app_context->tree_store_widg), 
                         (GtkTreeModelForeachFunc)removeZmapRowForeachFunc,
                         (gpointer)zmap);

  //  if (app_context->selected_zmap == zmap)
  //  app_context->selected_zmap = NULL ;

  /* The remove call actually sets my data which I attached to the row to NULL
   * which is v. naughty, so I reset my data in the widget to NULL to avoid it being
   * messed with. */
  //  gtk_clist_set_row_data(GTK_CLIST(app_context->clist_widg), row, NULL) ;
  //  gtk_clist_remove(GTK_CLIST(app_context->clist_widg), row) ;

  return ;
}

static gboolean removeZmapRowForeachFunc(GtkTreeModel *model, GtkTreePath *path,
                                         GtkTreeIter *iter, gpointer data)
{
  ZMap zmap = (ZMap)data;
  ZMap row_zmap;

  gtk_tree_model_get(model, iter, 
                     ZMAPDATA_COLUMN, &row_zmap,
                     -1);
  if(zmap == row_zmap)
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



void exitCB(void *app_data, void *zmap_data_unused)
{
  ZMapAppContext app_context = (ZMapAppContext)app_data ;

  appExit(app_context) ;

  return ;
}


static void appExit(ZMapAppContext app_context)
{
  zMapManagerDestroy(app_context->zmap_manager) ;

  zmapAppExit(app_context) ;				    /* Does not return. */

  return ;
}
