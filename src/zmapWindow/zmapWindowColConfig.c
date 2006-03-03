/*  File: zmapWindowColConfig.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2006
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Functions to implement column configuration.
 *              
 * Exported functions: See ZMap/zmapWindow.h
 * HISTORY:
 * Last edited: Mar  2 17:55 2006 (edgrif)
 * Created: Thu Mar  2 09:07:44 2006 (edgrif)
 * CVS info:   $Id: zmapWindowColConfig.c,v 1.1 2006-03-03 07:55:20 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>

typedef struct
{
  ZMapWindow window ;
  GtkWidget *toplevel ;


} ColConfigureStruct, *ColConfigure ;


typedef struct
{
  ZMapWindow window ;
  FooCanvasGroup *column_group ;
} ButDataStruct, *ButData ;


static GtkWidget *makeMenuBar(ColConfigure search_data) ;
static GtkWidget *makeColsPanel(ColConfigure search_data, GList *column_groups) ;
static GList *makeColList(FooCanvasGroup *column_group) ;
static void getSetGroupCB(gpointer data, gpointer user_data) ;
static void colConfigure(ZMapWindow window, GList *column_groups) ;

static void quitCB(GtkWidget *widget, gpointer cb_data) ;
static void applyCB(GtkWidget *widget, gpointer cb_data) ;
static void quitMenuCB(gpointer data, guint callback_action, GtkWidget *widget) ;
static void noHelpCB(gpointer data, guint callback_action, GtkWidget *w) ;
static void showButCB(GtkToggleButton *togglebutton, gpointer user_data) ;
static void killButCB(GtkToggleButton *togglebutton, gpointer user_data) ;

static GtkItemFactoryEntry menu_items_G[] = {
 { "/_File",           NULL,          NULL,          0, "<Branch>",      NULL},
 { "/File/Close",      "<control>C",  quitMenuCB,    0, NULL,            NULL},
 { "/_Help",           NULL,          NULL,          0, "<LastBranch>",  NULL},
 { "/Help/One",        NULL,          noHelpCB,      0, NULL,            NULL}
};




/* NOTE: this function sets the hide/show status of columns column but it DOES NOT move
 * any columns,
 * to do that you need to use zmapWindowNewReposition(). The split is made because
 * we don't always want to reposition all the columns following every col. adjustment, e.g. when we
 * are creating a zmap window. */
void zmapWindowColumnConfigure(ZMapWindow window, FooCanvasGroup *column_group,
			       ZMapWindowColConfigureMode configure_mode)
{

  switch (configure_mode)
    {
    case ZMAPWWINDOWCOLUMN_HIDE:
      foo_canvas_item_hide(FOO_CANVAS_ITEM(column_group)) ;
      break ;
    case ZMAPWWINDOWCOLUMN_SHOW:
      foo_canvas_item_show(FOO_CANVAS_ITEM(column_group)) ;
      break ;
    case ZMAPWWINDOWCOLUMN_CONFIGURE:
    case ZMAPWWINDOWCOLUMN_CONFIGURE_ALL:
      {
	GList *columns = NULL ;

	if (configure_mode == ZMAPWWINDOWCOLUMN_CONFIGURE)
	  {
	    columns = g_list_append(columns, column_group) ;
	  }
	else
	  {
	    columns = makeColList(column_group) ;
	  }

	
	colConfigure(window, columns) ;
	
	g_list_free(columns) ; 
	
	break ;
      }
    default:
      zMapAssert("Coding error, unrecognised column configure type") ;
      break ;
    }

  if (configure_mode == ZMAPWWINDOWCOLUMN_HIDE || configure_mode == ZMAPWWINDOWCOLUMN_SHOW)
    zmapWindowNewReposition(window) ;
  
  return ;
}




static void colConfigure(ZMapWindow window, GList *column_groups)
{
  GtkWidget *toplevel, *vbox, *menubar, *hbox, *cols, *frame,
    *apply_button, *fields, *filters, *buttonBox ;
  ColConfigure configure_data ;
  char *title ;

  zMapAssert(window) ;

  configure_data = g_new0(ColConfigureStruct, 1) ;

  configure_data->window = window ;

  /* set up the top level window */
  configure_data->toplevel = toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
  g_signal_connect(GTK_OBJECT(toplevel), "destroy",
		   GTK_SIGNAL_FUNC(quitCB), (gpointer)configure_data) ;
  
  gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;
  title = zMapGUIMakeTitleString("Column configuration",
				 (char *)g_quark_to_string(window->feature_context->sequence_name)) ;
  gtk_window_set_title(GTK_WINDOW(toplevel), title) ;
  g_free(title) ;

  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(toplevel), vbox) ;

  menubar = makeMenuBar(configure_data) ;
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

  cols = makeColsPanel(configure_data, column_groups) ;
  gtk_box_pack_start(GTK_BOX(vbox), cols, TRUE, TRUE, 0) ;

  buttonBox = gtk_hbutton_box_new();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonBox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX(buttonBox), 
                       ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING);
  gtk_container_set_border_width (GTK_CONTAINER (buttonBox), 
                                  ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

  apply_button = gtk_button_new_with_label("Apply") ;
  gtk_container_add(GTK_CONTAINER(buttonBox), apply_button) ;
  gtk_signal_connect(GTK_OBJECT(apply_button), "clicked",
		     GTK_SIGNAL_FUNC(applyCB), (gpointer)configure_data) ;
  /* set apply button as default. */
  GTK_WIDGET_SET_FLAGS(apply_button, GTK_CAN_DEFAULT) ;
  gtk_window_set_default(GTK_WINDOW(toplevel), apply_button) ;

  frame = gtk_frame_new(NULL) ;
  gtk_container_set_border_width(GTK_CONTAINER(frame), 
                                 ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0) ;

  gtk_container_add(GTK_CONTAINER(frame), buttonBox);

  gtk_widget_show_all(toplevel) ;

  return ;
}


/*
 *                 Internal functions
 */


GtkWidget *makeMenuBar(ColConfigure configure_data)
{
  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;
  GtkWidget *menubar ;
  gint nmenu_items = sizeof (menu_items_G) / sizeof (menu_items_G[0]);

  accel_group = gtk_accel_group_new ();

  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group );

  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items_G,
				(gpointer)configure_data) ;

  gtk_window_add_accel_group(GTK_WINDOW(configure_data->toplevel), accel_group) ;

  menubar = gtk_item_factory_get_widget(item_factory, "<main>");

  return menubar ;
}


GtkWidget *makeColsPanel(ColConfigure configure_data, GList *columns_list)
{
  GtkWidget *cols_panel, *frame, *column_box ;
  ZMapWindow window = configure_data->window ;
  ZMapFeatureContext context = window->feature_context ;
  GList *column ;


  cols_panel = frame = gtk_frame_new(NULL) ;
  gtk_container_set_border_width(GTK_CONTAINER(frame), 
                                 ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

  column_box = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(frame), column_box) ;


  column = g_list_first(columns_list) ;
  do
    {
      FooCanvasGroup *column_group = (FooCanvasGroup *)(column->data) ;
      ButData button_data ;
      ZMapFeatureTypeStyle style ;
      GtkWidget *col_box, *button ;
      gboolean col_visible ;
      
      button_data = g_new0(ButDataStruct, 1) ;
      button_data->window = window ;
      button_data->column_group = column_group ;

      style = g_object_get_data(G_OBJECT(column_group), "item_feature_style") ;
      zMapAssert(style) ;

      col_box = gtk_hbox_new(FALSE, 0) ;
      gtk_box_pack_start(GTK_BOX(column_box), col_box, TRUE, TRUE, 0);

      col_visible = zmapWindowItemIsShown(FOO_CANVAS_ITEM(column_group)) ;

      button = gtk_check_button_new_with_label(g_quark_to_string(style->original_id)) ;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), col_visible) ;
      gtk_box_pack_start(GTK_BOX(col_box), button, FALSE, FALSE, 0) ;
      g_signal_connect(GTK_OBJECT(button), "toggled",
		       GTK_SIGNAL_FUNC(showButCB), (gpointer)button_data) ;
      g_signal_connect(GTK_OBJECT(button), "destroy",
		       GTK_SIGNAL_FUNC(killButCB), (gpointer)button_data) ;


      /* This should be a button at the bottom of the window... */
      button = gtk_button_new_with_label("Advanced configuration") ;
      gtk_box_pack_end(GTK_BOX(col_box), button, FALSE, FALSE, 0) ;
    }
  while ((column = g_list_next(column))) ;


  return cols_panel ;
}


static void quitMenuCB(gpointer cb_data, guint callback_action, GtkWidget *widget)
{

  /* To avoid lots of destroys we call our other quit routine. */
  quitCB(widget, cb_data) ;

  return ;
}


static void noHelpCB(gpointer data, guint callback_action, GtkWidget *w)
{

  printf("sorry, no help\n") ;

  return ;
}



static void quitCB(GtkWidget *widget, gpointer cb_data)
{
  ColConfigure configure_data = (ColConfigure)cb_data ;

  gtk_widget_destroy(configure_data->toplevel) ;

  return ;
}



static void applyCB(GtkWidget *widget, gpointer cb_data)
{
  ColConfigure configure_data = (ColConfigure)cb_data ;

  /* WE NEED TO CALL COLUMN REPOSITION HERE.... OR MAYBE MORE THAN THAT... */
  printf("not implemented yet\n") ;

  return ;
}



static void showButCB(GtkToggleButton *togglebutton, gpointer user_data)
{
  ButData button_data = (ButData)user_data ;
  gboolean but_pressed ;

  but_pressed = gtk_toggle_button_get_active(togglebutton) ;

  if (but_pressed)
    foo_canvas_item_show(FOO_CANVAS_ITEM(button_data->column_group)) ;
  else
    foo_canvas_item_hide(FOO_CANVAS_ITEM(button_data->column_group)) ;

  zmapWindowNewReposition(button_data->window) ;

  return ;
}


/* Clean up resources... */
static void killButCB(GtkToggleButton *togglebutton, gpointer user_data)
{
  g_free(user_data) ;

  return ;
}



/* May want to make this into a container group function.... */
static GList *makeColList(FooCanvasGroup *column_group)
{
  GList *col_list = NULL ;
  FooCanvasGroup *column_group_parent, *strand_parent ;

  strand_parent = zmapWindowContainerGetSuperGroup(column_group) ;

  zmapWindowContainerExecute(strand_parent, ZMAPCONTAINER_LEVEL_FEATURESET,
			     getSetGroupCB, &col_list,
			     NULL, NULL) ;

  return col_list ;
}



/* Adds this columns group to the list supplied as user data. */
static void getSetGroupCB(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;
  GList **return_list = (GList **)user_data ;
  GList *col_list = *return_list ;
  ZMapContainerLevelType level ;

  level = zmapWindowContainerGetLevel(container) ;

  if (zmapWindowContainerGetLevel(container) == ZMAPCONTAINER_LEVEL_FEATURESET)
    {
      col_list = g_list_append(col_list, container) ;
      *return_list = col_list ;
    }


  return ;
}

