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
 * Last edited: Jul 21 16:58 2004 (edgrif)
 * Created: Thu Jul 24 14:36:27 2003 (edgrif)
 * CVS info:   $Id: zmapAppwindow.c,v 1.10 2004-07-21 16:07:11 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <ZMap/zmapUtils.h>
#include <zmapApp_P.h>



static void initGnomeGTK(int argc, char *argv[]) ;
static ZMapAppContext createAppContext(void) ;
static void quitCB(GtkWidget *widget, gpointer data) ;
static void removeZmapRow(void *app_data, void *zmap) ;


/* Overall app. debugging output...not used currently. */
gboolean zmapApp_debug_G = TRUE ; 



int test_global = 10 ;
int test_overlap = 0 ;


int zmapMainMakeAppWindow(int argc, char *argv[])
{
  ZMapAppContext app_context ;
  GtkWidget *toplevel, *vbox, *menubar, *connect_frame, *manage_frame ;
  GtkWidget *kill_button, *quit_button ;


  if (argc > 1)
    test_global = atoi(argv[1]) ;

  if (argc > 2)
    test_overlap = 1 ;

  zmap_debug_G = TRUE ;
#ifdef G_THREADS_ENABLED
  zMapDebug("%s", "threads are enabled\n") ;
#endif

#ifdef G_THREADS_IMPL_NONE
  zMapDebug("%s", "but there is no thread implementation\n") ;
#endif

#ifdef G_THREADS_IMPL_POSIX
  zMapDebug("%s", "and posix threads are being used\n") ;
#endif
  zmap_debug_G = FALSE ;


  /* For threaded stuff we apparently need this..... */
  g_thread_init(NULL) ;

  app_context = createAppContext() ;

  /* Set up logging for application. */
  app_context->logger = zMapLogCreate(NULL) ;

  initGnomeGTK(argc, argv) ;					    /* May exit if checks fail. */

  app_context->app_widg = toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_window_set_title(GTK_WINDOW(toplevel), "ZMap - Son of FMap !") ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;
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

  gtk_main() ;


  return(EXIT_SUCCESS) ;
}


void zmapAppExit(ZMapAppContext app_context)
{


  /* This should probably be the last thing before exitting... */
  if (app_context->logger)
    {
      zMapLogMessage("%s", "Goodbye cruel world !") ;

      zMapLogDestroy(app_context->logger) ;
    }

  gtk_exit(EXIT_SUCCESS) ;
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
      zMapExit("%s\n", err_msg) ;

      gtk_exit(EXIT_FAILURE) ;
    }

  gtk_init(&argc, &argv) ;

  return ;
}


static ZMapAppContext createAppContext(void)
{
  ZMapAppContext app_context ;

  app_context = g_new0(ZMapAppContextStruct, 1) ;

  app_context->app_widg = app_context->sequence_widg = app_context->clist_widg = NULL ;

  app_context->zmap_manager = zMapManagerCreate(removeZmapRow, (void *)app_context) ;
  app_context->selected_zmap = NULL ;

  app_context->logger = NULL ;

  return app_context ;
}


static void quitCB(GtkWidget *widget, gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;

  zMapManagerDestroy(app_context->zmap_manager) ;

  zmapAppExit(app_context) ;				    /* Does not return. */

  return ;
}


static void removeZmapRow(void *app_data, void *zmap_data)
{
  ZMapAppContext app_context = (ZMapAppContext)app_data ;
  ZMap zmap = (ZMap)zmap_data ;
  int row ;

  row = gtk_clist_find_row_from_data(GTK_CLIST(app_context->clist_widg), zmap) ;

  if (app_context->selected_zmap == zmap)
    app_context->selected_zmap = NULL ;

  /* The remove call actually sets my data which I attached to the row to NULL
   * which is v. naughty, so I reset my data in the widget to NULL to avoid it being
   * messed with. */
  gtk_clist_set_row_data(GTK_CLIST(app_context->clist_widg), row, NULL) ;
  gtk_clist_remove(GTK_CLIST(app_context->clist_widg), row) ;

  return ;
}


