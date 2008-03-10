/*  File: zmapWindowColConfig.c
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Functions to implement column configuration.
 *              
 * Exported functions: See ZMap/zmapWindow.h
 * HISTORY:
 * Last edited: Mar 10 20:27 2008 (rds)
 * Created: Thu Mar  2 09:07:44 2006 (edgrif)
 * CVS info:   $Id: zmapWindowColConfig.c,v 1.19 2008-03-10 20:28:44 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>


/* Labels for column state, used in code and the help page. */
#define SHOW_LABEL     "Show"
#define SHOWHIDE_LABEL "Default"
#define HIDE_LABEL     "Hide"


#define CONF_DATA "col_data"
typedef struct
{
  ZMapWindow window ;

  GtkWidget *forward_label_container ;
  GtkWidget *forward_button_container ;
  GtkWidget *reverse_label_container ;
  GtkWidget *reverse_button_container ;
} ColConfigureStruct, *ColConfigure ;


#define BUTTON_DATA "button_data"
typedef struct
{
  ZMapWindow window ;
  FooCanvasGroup *column_group ;
  GtkWidget *button_box ;
} ButDataStruct, *ButData ;



#define SELECT_DATA "select_data"
typedef struct
{
  ZMapWindow window ;

  GtkWidget *frame_widg ;
} SelectAllConfigureStruct, *SelectAllConfigure ;



typedef struct
{
  ZMapStyleColumnDisplayState col_state ;
  char *but_text ;
  GtkWidget *button ;
} FindButStruct, *FindBut ;



static GtkWidget *makeMenuBar(ColConfigure search_data) ;
static GtkWidget *makeColsPanel(ZMapWindow window, char *frame_title, GList *column_groups,
				GtkWidget **label_container_out, GtkWidget **button_container_out) ;
static void makeColList(ZMapWindow window, GList **forward_cols, GList **reverse_cols) ;
static void getSetGroupCB(FooCanvasGroup *container, FooCanvasPoints *points, 
                          ZMapContainerLevelType level, gpointer user_data) ;
static void colConfigure(ZMapWindow window, GList *forward_cols, GList *reverse_cols) ;
static void simpleConfigure(ZMapWindow window, ZMapWindowColConfigureMode configure_mode,
			    FooCanvasGroup *column_group) ;
static void requestDestroyCB(gpointer data, guint callback_action, GtkWidget *widget) ;
static void destroyCB(GtkWidget *widget, gpointer cb_data) ;
static void destroyLabelCB(GtkWidget *widget, gpointer cb_data) ;
static void destroyFrameCB(GtkWidget *widget, gpointer cb_data) ;
static void helpCB(gpointer data, guint callback_action, GtkWidget *w) ;
static void showButCB(GtkToggleButton *togglebutton, gpointer user_data) ;
static void selectButtons(GtkWidget *button, gpointer user_data) ;
static void allButtonsToggleCB(gpointer data, gpointer user_data);
static void changeButtonState(GtkWidget *toplevel,
			      FooCanvasGroup *column_group, ZMapWindowColConfigureMode configure_mode) ;
static gint findGroupCB(gconstpointer a, gconstpointer b) ;
static GtkWidget *findButton(GtkWidget *hbox, ZMapStyleColumnDisplayState col_state) ;
static void findButtonCB(GtkWidget *widget, gpointer data) ;


static GtkItemFactoryEntry menu_items_G[] =
  {
    { "/_File",           NULL,          NULL,             0, "<Branch>",      NULL},
    { "/File/Close",      "<control>W",  requestDestroyCB, 0, NULL,            NULL},
    { "/_Help",           NULL,          NULL,             0, "<LastBranch>",  NULL},
    { "/Help/General",    NULL,          helpCB,           0, NULL,            NULL}
  };



/* 
 *          External Public functions.
 */


/*! Shows a list of displayable columns for forward/reverse strands with radio buttons
 * for turning them on/off.
 */
void zMapWindowColumnList(ZMapWindow window)
{
  zmapWindowColumnConfigure(window, NULL, ZMAPWWINDOWCOLUMN_CONFIGURE_ALL) ;

  return ;
}



/* 
 *          External Private functions.
 */


/* Note that column_group is not looked at for configuring all columns. */
void zmapWindowColumnConfigure(ZMapWindow window, FooCanvasGroup *column_group,
			       ZMapWindowColConfigureMode configure_mode)
{

  switch (configure_mode)
    {
    case ZMAPWWINDOWCOLUMN_HIDE:
    case ZMAPWWINDOWCOLUMN_SHOW:
      simpleConfigure(window, configure_mode, column_group) ;
      break ;

    case ZMAPWWINDOWCOLUMN_CONFIGURE:
    case ZMAPWWINDOWCOLUMN_CONFIGURE_ALL:
      {
	GList *forward_columns = NULL, *reverse_columns = NULL  ;

	zmapWindowColumnConfigureDestroy(window) ;

	if (configure_mode == ZMAPWWINDOWCOLUMN_CONFIGURE)
	  {
	    ZMapStrand strand ;

	    strand = zmapWindowContainerGetStrand(column_group);
	    zMapAssert(strand == ZMAPSTRAND_FORWARD || strand == ZMAPSTRAND_REVERSE) ;

	    if (strand == ZMAPSTRAND_FORWARD)
	      forward_columns = g_list_append(forward_columns, column_group) ;
	    else
	      reverse_columns = g_list_append(reverse_columns, column_group) ;
	  }
	else
	  {
	    makeColList(window, &forward_columns, &reverse_columns) ;
	  }

	colConfigure(window, forward_columns, reverse_columns) ;
	
	g_list_free(forward_columns) ;
	g_list_free(reverse_columns) ;
	
	break ;
      }
    default:
      zMapAssert("Coding error, unrecognised column configure type") ;
      break ;
    }

  if (configure_mode == ZMAPWWINDOWCOLUMN_HIDE || configure_mode == ZMAPWWINDOWCOLUMN_SHOW)
    zmapWindowFullReposition(window) ;
  
  return ;
}


/* Destroy the column configuration window if there is one. */
void zmapWindowColumnConfigureDestroy(ZMapWindow window)
{
  if (window->col_config_window)
    {
      gtk_widget_destroy(window->col_config_window) ;
      window->col_config_window =  NULL ;
    }

  return ;
}






/*
 *                 Internal functions
 */


static void simpleConfigure(ZMapWindow window, ZMapWindowColConfigureMode configure_mode,
			    FooCanvasGroup *column_group)
{
  ZMapStyleColumnDisplayState col_state ;

  /* If there's a config window displayed then toggle correct button in window to effect
   * change, otherwise operate directly on column. */
  if (window->col_config_window)
    {
      changeButtonState(window->col_config_window, column_group, configure_mode) ;
    }
  else
    {
      if (configure_mode == ZMAPWWINDOWCOLUMN_HIDE)
	col_state = ZMAPSTYLE_COLDISPLAY_HIDE ;
      else
	col_state = ZMAPSTYLE_COLDISPLAY_SHOW ;

      zmapWindowColumnSetState(window, column_group, col_state, TRUE) ;

      zmapWindowFullReposition(window) ;
    }

  return ;
}



static void colConfigure(ZMapWindow window, GList *forward_cols, GList *reverse_cols)
{
  GtkWidget *toplevel, *vbox, *hbox, *menubar, *cols;
  ColConfigure configure_data ;
  char *title ;

  zMapAssert(window) ;

  configure_data = g_new0(ColConfigureStruct, 1) ;

  configure_data->window = window ;

  /* set up the top level window */
  configure_data->window->col_config_window =  toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
  g_signal_connect(GTK_OBJECT(toplevel), "destroy",
		   GTK_SIGNAL_FUNC(destroyCB), (gpointer)configure_data) ;
  g_object_set_data(G_OBJECT(toplevel), CONF_DATA, configure_data) ;
  
  gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;
  title = zMapGUIMakeTitleString("Column configuration",
				 (char *)g_quark_to_string(window->feature_context->sequence_name)) ;
  gtk_window_set_title(GTK_WINDOW(toplevel), title) ;
  g_free(title) ;

  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(toplevel), vbox) ;

  menubar = makeMenuBar(configure_data) ;
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0) ;

  if (reverse_cols)
    {
      cols = makeColsPanel(window, "Reverse Strand", reverse_cols,
			   &(configure_data->reverse_label_container),
			   &(configure_data->reverse_button_container)) ;

      gtk_box_pack_start(GTK_BOX(hbox), cols, TRUE, TRUE, 0) ;
    }

  if (forward_cols)
    {
      cols = makeColsPanel(window, "Forward Strand", forward_cols,
			   &(configure_data->forward_label_container),
			   &(configure_data->forward_button_container)) ;

      gtk_box_pack_start(GTK_BOX(hbox), cols, TRUE, TRUE, 0) ;
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* We don't need this just yet.... */

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
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  gtk_widget_show_all(toplevel) ;

  return ;
}


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

  gtk_window_add_accel_group(GTK_WINDOW(configure_data->window->col_config_window), accel_group) ;

  menubar = gtk_item_factory_get_widget(item_factory, "<main>");

  return menubar ;
}


GtkWidget *makeColsPanel(ZMapWindow window, char *frame_title, GList *columns_list,
			 GtkWidget **label_container_out, GtkWidget **button_container_out)
{
  GtkWidget *cols_panel, *frame, *vbox, *hbox, *label_box, *column_box ;
  GList *column = NULL ;

  cols_panel = frame = gtk_frame_new(frame_title) ;
  gtk_container_set_border_width(GTK_CONTAINER(frame), 
                                 ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(frame), vbox) ;


  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(vbox), hbox) ;


  label_box = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(hbox), label_box) ;
  *label_container_out = label_box ;

  column_box = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(hbox), column_box) ;
  *button_container_out = column_box ;

  column = g_list_first(columns_list) ;
  do
    {
      FooCanvasGroup *column_group = (FooCanvasGroup *)(column->data) ;
      ButData button_data ;
      ZMapWindowItemFeatureSetData set_data ;
      ZMapFeatureTypeStyle style ;
      GtkWidget *col_box, *active_button, *label, *radio_show, *radio_maybe, *radio_hide ;
      ZMapStyleColumnDisplayState col_state ;


      button_data = g_new0(ButDataStruct, 1) ;
      button_data->window = window ;
      button_data->column_group = column_group ;


      /* Get hold of the style. */
      set_data = g_object_get_data(G_OBJECT(column_group), ITEM_FEATURE_SET_DATA) ;
      zMapAssert(set_data) ;
      style = set_data->style ;


      /* show the column name. */
      label = gtk_label_new(g_quark_to_string(zMapStyleGetID(set_data->style))) ;
      /* x, y alignments between 0.0 (left, top) and 1.0 (right, bottom) [0.5 == centered] */
      gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
      gtk_box_pack_start(GTK_BOX(label_box), label, TRUE, TRUE, 0) ;
      g_object_set_data(G_OBJECT(label), BUTTON_DATA, button_data) ;
      g_signal_connect(GTK_OBJECT(label), "destroy",
		       GTK_SIGNAL_FUNC(destroyLabelCB), (gpointer)button_data) ;


      /* Show set of radio buttons for each column to change column display state. */
      col_box = gtk_hbox_new(FALSE, 0) ;
      gtk_box_pack_start(GTK_BOX(column_box), col_box, TRUE, TRUE, 0);

      button_data->button_box = col_box ;

      radio_show = gtk_radio_button_new_with_label(NULL, SHOW_LABEL) ;
      gtk_box_pack_start(GTK_BOX(col_box), radio_show, TRUE, TRUE, 0) ;
      g_signal_connect_data(G_OBJECT(radio_show), "toggled",
                            G_CALLBACK(showButCB), GINT_TO_POINTER(ZMAPSTYLE_COLDISPLAY_SHOW),
                            NULL, 0) ;
      g_object_set_data(G_OBJECT(radio_show), BUTTON_DATA, button_data) ;

      radio_maybe = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_show), SHOWHIDE_LABEL) ;
      gtk_box_pack_start(GTK_BOX(col_box), radio_maybe, TRUE, TRUE, 0) ;
      g_signal_connect_data(G_OBJECT(radio_maybe), "toggled",
                            G_CALLBACK(showButCB), GINT_TO_POINTER(ZMAPSTYLE_COLDISPLAY_SHOW_HIDE),
                            NULL, 0) ;
      g_object_set_data(G_OBJECT(radio_maybe), BUTTON_DATA, button_data) ;

      radio_hide = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_show), HIDE_LABEL) ;
      gtk_box_pack_start(GTK_BOX(col_box), radio_hide, TRUE, TRUE, 0) ;
      g_signal_connect_data(G_OBJECT(radio_hide), "toggled",
                            G_CALLBACK(showButCB), GINT_TO_POINTER(ZMAPSTYLE_COLDISPLAY_HIDE),
                            NULL, 0) ;
      g_object_set_data(G_OBJECT(radio_hide), BUTTON_DATA, button_data) ;


      col_state = zMapStyleGetDisplay(style) ;
      switch(col_state)
	{
	case ZMAPSTYLE_COLDISPLAY_HIDE:
	  active_button = radio_hide ;
	  break ;
	case ZMAPSTYLE_COLDISPLAY_SHOW_HIDE:
	  active_button = radio_maybe ;
	  break ;
	default: /* ZMAPSTYLE_COLDISPLAY_SHOW */
	  active_button = radio_show ;
	  break ;
	}

      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(active_button), TRUE) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Not needed yet.... */

      /* This should be a button at the bottom of the window... */
      button = gtk_button_new_with_label("Advanced configuration") ;
      gtk_box_pack_end(GTK_BOX(col_box), button, FALSE, FALSE, 0) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }
  while ((column = g_list_next(column))) ;


  /* We don't need this for a single column. */
  if (g_list_length(columns_list) > 1)
    {
      SelectAllConfigure select_data ;
      GtkWidget *frame, *button_box, *button;

      select_data = g_new0(SelectAllConfigureStruct, 1) ;
      select_data->window = window ;
      select_data->frame_widg = column_box ;


      frame = gtk_frame_new(NULL) ;
      gtk_container_set_border_width(GTK_CONTAINER(frame), ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);
      g_signal_connect(GTK_OBJECT(frame), "destroy",
		       GTK_SIGNAL_FUNC(destroyFrameCB), (gpointer)select_data) ;
      gtk_box_pack_end(GTK_BOX(vbox), frame, FALSE, FALSE, 0);


      button_box = gtk_hbutton_box_new();
      gtk_container_add(GTK_CONTAINER(frame), button_box) ;

      button = gtk_button_new_with_label(SHOW_LABEL " All");
      gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);
      g_signal_connect_data(G_OBJECT(button), "clicked",
                            G_CALLBACK(selectButtons), 
                            GINT_TO_POINTER(ZMAPSTYLE_COLDISPLAY_SHOW),
                            NULL,
                            0);
      g_object_set_data(G_OBJECT(button), SELECT_DATA, select_data) ;

      button = gtk_button_new_with_label(SHOWHIDE_LABEL " All");
      gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);
      g_signal_connect_data(G_OBJECT(button), "clicked",
                            G_CALLBACK(selectButtons), 
                            GINT_TO_POINTER(ZMAPSTYLE_COLDISPLAY_SHOW_HIDE), 
                            NULL, 0) ;
      g_object_set_data(G_OBJECT(button), SELECT_DATA, select_data) ;

      button = gtk_button_new_with_label(HIDE_LABEL " All");
      gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);
      g_signal_connect_data(G_OBJECT(button), "clicked",
                            G_CALLBACK(selectButtons), 
                            GINT_TO_POINTER(ZMAPSTYLE_COLDISPLAY_HIDE), 
                            NULL, 0) ;
      g_object_set_data(G_OBJECT(button), SELECT_DATA, select_data) ;
    }

  return cols_panel ;
}


/* This is not the way to do help, we should really used html and have a set of help files. */
static void helpCB(gpointer data, guint callback_action, GtkWidget *w)
{
  char *title = "Column Configuration Window" ;
  char *help_text =
    "The ZMap Column Configuration Window allows you to change the way columns are displayed.\n"
    "\n"
    "The window displays columns separately for the forward and reverse strands. You can set\n"
    "the display state of each column to one of:\n"
    "\n"
    "\"" SHOW_LABEL "\"  - always show the column\n"
    "\n"
    "\"" SHOWHIDE_LABEL "\"  - show the column depending on ZMap settings (e.g. zoom, compress etc.)\n"
    "\n"
    "\"" HIDE_LABEL "\"  - always hide the column\n"
    "\n"
    "By default column display is controlled by settings in the Style for that column,\n"
    "including the min and max zoom levels at which the column should be shown, how the\n"
    "the column is bumped, the window mark and compress options. Column display can be\n"
    "overridden however to always show or always hide columns.\n"
    "\n" ;

  zMapGUIShowText(title, help_text, FALSE) ;

  return ;
}




/* requests window destroy, ends up with gtk calling destroyCB(). */
static void requestDestroyCB(gpointer cb_data, guint callback_action, GtkWidget *widget)
{
  ColConfigure configure_data = (ColConfigure)cb_data ;

  gtk_widget_destroy(configure_data->window->col_config_window) ;

  return ;
}


static void destroyCB(GtkWidget *widget, gpointer cb_data)
{
  ColConfigure configure_data = (ColConfigure)cb_data ;

  configure_data->window->col_config_window = NULL ;

  g_free(configure_data) ;

  return ;
}


static void destroyLabelCB(GtkWidget *widget, gpointer cb_data)
{
  ButData button_data = (ButData)cb_data ;

  g_free(button_data) ;

  return ;
}

static void destroyFrameCB(GtkWidget *widget, gpointer cb_data)
{
  SelectAllConfigure select_data = (SelectAllConfigure)cb_data ;

  g_free(select_data) ;

  return ;
}





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void applyCB(GtkWidget *widget, gpointer cb_data)
{
#ifdef RDS_DONT_INCLUDE_UNUSED
  ColConfigure configure_data = (ColConfigure)cb_data ;
#endif
  /* WE NEED TO CALL COLUMN REPOSITION HERE.... OR MAYBE MORE THAN THAT... */
  printf("not implemented yet\n") ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Called when user selects one of the display state radio buttons.
 * 
 * NOTE: selecting a button will deselect the other buttons, this routine
 * gets called for each of those deselections as well. */
static void showButCB(GtkToggleButton *togglebutton, gpointer user_data)
{
  gboolean but_pressed ;

  /* Only do something if the button was pressed on. */
  if ((but_pressed = gtk_toggle_button_get_active(togglebutton)))
    {
      ButData button_data ;
      ZMapStyleColumnDisplayState col_state ;

      /* Set column state according to which button was clicked. */
      button_data = g_object_get_data(G_OBJECT(togglebutton), BUTTON_DATA) ;

      col_state = GPOINTER_TO_INT(user_data) ;

      zmapWindowColumnSetState(button_data->window, button_data->column_group, col_state, TRUE) ;
    }

  return ;
}


/* May want to make this into a container group function as it contains some quite awkward
 * stuff.... */
static void makeColList(ZMapWindow window, GList **forward_cols_out, GList **reverse_cols_out)
{
  GList *forward_col_list = NULL, *reverse_col_list = NULL ;
  FooCanvasGroup *curr_level ;
  FooCanvasGroup *strands, *strand_parent ;
  GList *strands_list ;

  /* Starting at root move down to the block level so we can get at the forward and reverse strands. */
  curr_level = window->feature_root_group ;
  while (zmapWindowContainerGetLevel(curr_level) != ZMAPCONTAINER_LEVEL_BLOCK)
    {
      FooCanvasGroup *child_group ;
      GList *children ;

      child_group = zmapWindowContainerGetFeatures(curr_level) ;

      children = child_group->item_list ;

      curr_level = children->data ;
    }
  strands = zmapWindowContainerGetFeatures(curr_level) ;
  strands_list = strands->item_list ;

  /* Get the reverse strand which will be the first group in the block level "features" */
  strands_list = g_list_first(strands_list) ;

  strand_parent = strands_list->data ;

  zmapWindowContainerExecute(strand_parent, ZMAPCONTAINER_LEVEL_FEATURESET,
                             getSetGroupCB, &reverse_col_list);

  /* Get the forward strand which will be the first group in the block level "features" */
  strands_list = g_list_next(strands_list) ;

  strand_parent = strands_list->data ;

  zmapWindowContainerExecute(strand_parent, ZMAPCONTAINER_LEVEL_FEATURESET,
                             getSetGroupCB, &forward_col_list);

  /* Return the column lists. */
  *forward_cols_out = forward_col_list ;
  *reverse_cols_out = reverse_col_list ;

  return ;
}



/* Adds this columns group to the list supplied as user data. */
static void getSetGroupCB(FooCanvasGroup *container, FooCanvasPoints *points, ZMapContainerLevelType level, gpointer user_data)
{
  GList **return_list = (GList **)user_data ;
  GList *col_list = *return_list ;

  if (level == ZMAPCONTAINER_LEVEL_FEATURESET)
    {
      col_list = g_list_append(col_list, container) ;
      *return_list = col_list ;
    }


  return ;
}


/* Set all column buttons for a strand to the state given by user_data. */
static void selectButtons(GtkWidget *button, gpointer user_data)
{
  SelectAllConfigure select_data ;
  GList *button_boxes ;

  select_data = g_object_get_data(G_OBJECT(button), SELECT_DATA) ;

  button_boxes = gtk_container_get_children(GTK_CONTAINER(select_data->frame_widg)) ;

  g_list_foreach(button_boxes, allButtonsToggleCB, user_data) ;

  zmapWindowFullReposition(select_data->window) ;

  return ;
}


static void allButtonsToggleCB(gpointer data, gpointer user_data)
{
  GtkWidget *hbox = (GtkWidget *)data ;
  ZMapStyleColumnDisplayState col_state = GPOINTER_TO_INT(user_data) ;
  GtkWidget *button ;
  ButData button_data = NULL;
  gint blocked = 0;

  /* Find the right button to toggle on for the given state. */
  button = findButton(hbox, col_state) ;

  /* We do the toggling by blocking the callbacks otherwise there is a load
   * of flickering...???? */
  if ((button_data = g_object_get_data(G_OBJECT(button), BUTTON_DATA))
      && (blocked = g_signal_handlers_block_matched(G_OBJECT(button), 
						   G_SIGNAL_MATCH_FUNC,
						   0, 0,
						   NULL,
						   showButCB,
						   NULL)))
    {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE) ;

      zmapWindowColumnSetState(button_data->window, button_data->column_group, col_state, FALSE) ;

      g_signal_handlers_unblock_matched(G_OBJECT(button), 
                                        G_SIGNAL_MATCH_FUNC,
                                        0, 0,
                                        NULL,
                                        showButCB,
					NULL);
    }

  return ;
}


static GtkWidget *findButton(GtkWidget *hbox, ZMapStyleColumnDisplayState col_state)
{
  GtkWidget *button = NULL ;
  FindButStruct cb_data = {col_state, NULL, NULL} ;

  switch(col_state)
    {
    case ZMAPSTYLE_COLDISPLAY_HIDE:
      cb_data.but_text = HIDE_LABEL ;
      break ;
    case ZMAPSTYLE_COLDISPLAY_SHOW_HIDE:
      cb_data.but_text = SHOWHIDE_LABEL ;
      break ;
    default: /* ZMAPSTYLE_COLDISPLAY_SHOW */
      cb_data.but_text = SHOW_LABEL ;
      break ;
    }

  gtk_container_foreach(GTK_CONTAINER(hbox), findButtonCB, &cb_data) ;

  if (cb_data.button)
    button = cb_data.button ;

  return button ;
}



/* A GtkCallback() to find a button with a given label. */
static void findButtonCB(GtkWidget *widget, gpointer data)
{
  FindBut cb_data = (FindBut)data ;

  if (g_ascii_strcasecmp(cb_data->but_text, gtk_button_get_label(GTK_BUTTON(widget))) == 0)
    cb_data->button = widget ;

  return ;
}



/* Finds the button which represents column_group and sets its state to match configure_mode. */
static void changeButtonState(GtkWidget *toplevel,
			      FooCanvasGroup *column_group, ZMapWindowColConfigureMode configure_mode)
{
  ColConfigure configure_data ;
  GList *label_el = NULL ;
  GtkWidget *label, *button ;
  gboolean col_visible ;
  ZMapStrand strand ;
  GtkWidget *label_container, *button_container ;
  GList *labels ;
  ButData button_data ;
  ZMapStyleColumnDisplayState col_state ;

  strand = zmapWindowContainerGetStrand(column_group);
  zMapAssert(strand == ZMAPSTRAND_FORWARD || strand == ZMAPSTRAND_REVERSE) ;

  configure_data = g_object_get_data(G_OBJECT(toplevel), CONF_DATA) ;

  if (strand == ZMAPSTRAND_FORWARD)
    {
      label_container = configure_data->forward_label_container ;
      button_container = configure_data->forward_button_container ;
    }
  else
    {
      label_container = configure_data->reverse_label_container ;
      button_container = configure_data->reverse_button_container ;
    }

  labels = gtk_container_get_children(GTK_CONTAINER(label_container)) ;

  label_el = g_list_find_custom(labels, column_group, findGroupCB) ;
  zMapAssert(label_el) ;

  label = label_el->data ;

  button_data = g_object_get_data(G_OBJECT(label), BUTTON_DATA) ;

  if (configure_mode == ZMAPWWINDOWCOLUMN_HIDE)
    {
      col_state = ZMAPSTYLE_COLDISPLAY_HIDE ;
    }
  else
    {
      col_state = ZMAPSTYLE_COLDISPLAY_SHOW ;
    }
  col_visible = TRUE ;

  button = findButton(button_data->button_box, col_state) ;

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), col_visible) ;

  return ;
}


/* A  GCompareFunc() to look for the canvas group attached to a button. */
static gint findGroupCB(gconstpointer a, gconstpointer b)
{
  gint result = -1 ;
  GtkWidget *button = (GtkWidget *)a ;
  FooCanvasGroup *column_group = (FooCanvasGroup *)b ;
  ButData button_data ;

  button_data = g_object_get_data(G_OBJECT(button), BUTTON_DATA) ;

  if (button_data->column_group == column_group)
    result = 0 ;

  return result ;
}

