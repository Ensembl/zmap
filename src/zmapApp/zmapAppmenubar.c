/*  File: zmapAppmenubar.c
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
 * Description: Implements zmap main window menubar.
 *              
 * Exported functions: See zmapApp_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <stdlib.h>
#include <stdio.h>

#include <ZMap/zmapUtilsGUI.h>
#include <zmapApp_P.h>


static void quitCB(gpointer cb_data, guint callback_action, GtkWidget *w) ;
static void aboutCB(gpointer cb_data, guint callback_action, GtkWidget *window) ;
static void allHelpCB(gpointer cb_data, guint callback_action, GtkWidget *w);



/* 
 *                 Globals
 */

static GtkItemFactoryEntry menu_items[] = {
 { "/_File",           NULL,          NULL,          0, "<Branch>",  NULL},
 { "/File/Quit",       "<control>Q",  quitCB,        0, NULL,  NULL},
 { "/_Help",         NULL,         NULL, 0, "<LastBranch>" },
 { "/Help/General Help", NULL,     allHelpCB, ZMAPGUI_HELP_GENERAL, NULL },
 { "/Help/Release Notes", NULL,    allHelpCB, ZMAPGUI_HELP_RELEASE_NOTES, NULL },
 { "/Help/About ZMap",    NULL,    aboutCB, 0, NULL }
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



