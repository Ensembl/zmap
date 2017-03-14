/*  File: zmapAppmenubar.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: Implements zmap main window menubar.
 *              
 * Exported functions: See zmapApp_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <stdlib.h>
#include <stdio.h>

#include <ZMap/zmapUtilsGUI.hpp>
#include <zmapApp_P.hpp>


static void quitCB(gpointer cb_data, guint callback_action, GtkWidget *w) ;
static void aboutCB(gpointer cb_data, guint callback_action, GtkWidget *window) ;
static void allHelpCB(gpointer cb_data, guint callback_action, GtkWidget *w);



/* 
 *                 Globals
 */

static GtkItemFactoryEntry menu_items[] = {
 { (char *)"/_File",                  NULL,                 NULL,                              0,                          (char *)"<Branch>",      NULL},
 { (char *)"/File/Quit",              (char *)"<control>Q", (GtkItemFactoryCallback)quitCB,    0,                          NULL,                    NULL},
 { (char *)"/_Help",                  NULL,                 NULL,                              0,                          (char *)"<LastBranch>" },
 { (char *)"/Help/Quick Start Guide", NULL,                 (GtkItemFactoryCallback)allHelpCB, ZMAPGUI_HELP_QUICK_START,   NULL },
 { (char *)"/Help/User Manual",       NULL,                 (GtkItemFactoryCallback)allHelpCB, ZMAPGUI_HELP_MANUAL,        NULL },
 { (char *)"/Help/Release Notes",     NULL,                 (GtkItemFactoryCallback)allHelpCB, ZMAPGUI_HELP_RELEASE_NOTES, NULL },
 { (char *)"/Help/About ZMap",        NULL,                 (GtkItemFactoryCallback)aboutCB,   0,                          NULL }
};




/* 
 *                Package routines
 */


GtkWidget *zmapMainMakeMenuBar(ZMapAppContext app_context)
{
  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;
  GtkWidget *menubar ;
  gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

  accel_group = gtk_accel_group_new ();

  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group );

  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, (gpointer)app_context);

  gtk_window_add_accel_group(GTK_WINDOW(app_context->app_widg), accel_group) ;

  menubar = gtk_item_factory_get_widget (item_factory, "<main>");

  return menubar ;
}


/* 
 *                  Internal routines.
 */


static void quitCB(gpointer cb_data, guint callback_action, GtkWidget *w)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;

  zmapAppDestroy(app_context) ;

  return ;
}


/* Show the usual tedious "About" dialog. */
static void aboutCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  zMapGUIShowAbout() ;

  return ;
}


/* Show the web page of release notes. */
static void allHelpCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  zMapGUIShowHelp((ZMapHelpType)callback_action) ;


  return ;
}



