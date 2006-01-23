/*  File: zmapappmenubar.c
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jan 23 13:12 2006 (edgrif)
 * Created: Thu Jul 24 14:36:59 2003 (edgrif)
 * CVS info:   $Id: zmapControlWindowMenubar.c,v 1.8 2006-01-23 14:14:37 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>

#include <zmapControl_P.h>



static void quitCB(gpointer cb_data, guint callback_action, GtkWidget *w) ;
static void featureDumpCB(gpointer cb_data, guint callback_action, GtkWidget *w);
static void redrawCB(gpointer cb_data, guint callback_action, GtkWidget *w);
static void print_hello( gpointer data, guint callback_action, GtkWidget *w ) ;
static void handle_option( gpointer data, guint callback_action, GtkWidget *w ) ;


GtkItemFactory *item_factory;


static GtkItemFactoryEntry menu_items[] = {
 { "/_File",         NULL,         NULL, 0, "<Branch>" },
 { "/File/_New",     "<control>N", print_hello, 2, NULL },
 { "/File/_Open",    "<control>O", print_hello, 0, NULL },
 { "/File/_Save",    "<control>S", print_hello, 0, NULL },
 { "/File/Save _As", NULL,         NULL, 0, NULL },
 { "/File/sep1",     NULL,         NULL, 0, "<Separator>" },
 { "/File/_Dump",    "<control>D", featureDumpCB, 0, NULL },
 { "/File/sep1",     NULL,         NULL, 0, "<Separator>" },
 { "/File/Quit",     "<control>Q", quitCB, 0, NULL },
 { "/_Edit",         NULL,         NULL, 0, "<Branch>" },
 { "/Edit/Cu_t",     "<control>X", print_hello, 0, NULL },
 { "/Edit/_Copy",    "<control>C", print_hello, 0, NULL },
 { "/Edit/_Paste",    "<control>V", print_hello, 0, NULL },
 { "/Edit/_Redraw",  NULL, redrawCB, 0, NULL },
 { "/_Options",      NULL,         NULL, 0, "<Branch>" },
 { "/Options/Option1",  NULL,      handle_option, 1, "<CheckItem>" },
 { "/Options/Option2",  NULL,      handle_option, 2, "<ToggleItem>" },
 { "/Options/Option3",  NULL,      handle_option, 3, "<CheckItem>" },
 { "/Options/Option4",  NULL,      handle_option, 4, "<ToggleItem>" },
 { "/_Help",         NULL,         NULL, 0, "<LastBranch>" },
 { "/Help/One",   NULL,            NULL, 0, NULL },
 { "/Help/Two",   NULL,            NULL, 0, "<Branch>" },
 { "/Help/Two/A",   NULL,          NULL, 0, "<RadioItem>" },
 { "/Help/Two/B",   NULL,          NULL, 0, "/Help/Two/A" },
 { "/Help/Two/C",   NULL,          NULL, 0, "/Help/Two/A" },
 { "/Help/Two/D",   NULL,          NULL, 0, "/Help/Two/A" },
 { "/Help/Two/E",   NULL,          NULL, 0, "/Help/Two/A" },
 { "/Help/Three",   NULL,          NULL, 0, NULL },
};


GtkWidget *zmapControlWindowMakeMenuBar(ZMap zmap)
{
  GtkWidget *menubar ;
  GtkAccelGroup *accel_group;
  gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

  accel_group = gtk_accel_group_new() ;

  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group) ;

  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, (gpointer)zmap);

  gtk_window_add_accel_group(GTK_WINDOW(zmap->toplevel), accel_group) ;

  menubar = gtk_item_factory_get_widget (item_factory, "<main>");

  return menubar ;
}


/* Should pop up a dialog box to ask for a file name....e.g. the file chooser. */
static void featureDumpCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  ZMap zmap = (ZMap)cb_data ;
  gchar *file = "ZMap.features" ;

  zmapViewFeatureDump(zmap->focus_viewwindow, file) ;

  return ;
}



/* Causes currently focussed zmap window to redraw itself. */
static void redrawCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  ZMap zmap = (ZMap)cb_data ;

  zMapViewRedraw(zmap->focus_viewwindow) ;

  return ;
}




static void quitCB(gpointer cb_data, guint callback_action, GtkWidget *w)
{
  ZMap zmap = (ZMap)cb_data ;

  zmapControlTopLevelKillCB(zmap) ;

  return ;
}

static void print_hello( gpointer data, guint callback_action, GtkWidget *w )
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	GtkWidget *myWidget;
	printf( "widget is %x data is %s\n", w, data );
	g_message ("Hello, World!\n");

	myWidget = gtk_item_factory_get_widget (item_factory, "/File/New");
	printf( "File/New is %x\n", myWidget );

	gtk_item_factory_delete_item( item_factory, "/Edit" );
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

}

static void handle_option( gpointer data, guint callback_action, GtkWidget *w )
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	GtkCheckMenuItem *checkMenuItem = (GtkCheckMenuItem *) w;

	printf( "widget is %x data is %s\n", w, data );
	g_message ("Hello, World!\n");
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

}

