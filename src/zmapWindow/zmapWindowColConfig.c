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
 * Last edited: Feb  9 14:23 2009 (rds)
 * Created: Thu Mar  2 09:07:44 2006 (edgrif)
 * CVS info:   $Id: zmapWindowColConfig.c,v 1.27 2009-02-09 14:55:08 rds Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>


/* Labels for column state, used in code and the help page. */
#define SHOW_LABEL     "Show"
#define SHOWHIDE_LABEL "Default"
#define HIDE_LABEL     "Hide"

#define RADIO_BUTTONS_CONTAINER "button_widget"

typedef struct _ColConfigureStruct *ColConfigure;

typedef void (*ColConfigureReturnFunc)(ColConfigure configure_data);

#define CONFIGURE_DATA "col_data"
typedef struct _ColConfigureStruct
{
  ZMapWindow window ;

  GtkWidget *forward_label_container ;
  GtkWidget *forward_button_container ;
  GtkWidget *reverse_label_container ;
  GtkWidget *reverse_button_container ;

  ColConfigureReturnFunc return_func;
  
  unsigned int has_apply_button : 1 ;
  unsigned int apply_now        : 1 ;
  unsigned int reposition       : 1 ;
} ColConfigureStruct;

/* Not really required anymore, but I've left it in as I can see that
 * it might need replacing in the future with a struct to get hold of
 * a valid column group if column groups appear/disappear... */
typedef struct
{
  FooCanvasGroup *column_group ;
} ButDataStruct, *ButData ;


typedef struct
{
  GtkWidget *radio_groups_container ;
  ZMapStyleColumnDisplayState select_button_state;
} SelectAllConfigureStruct, *SelectAllConfigure ;

/* Arrgh, pain this is needed.  Also, will fall over when multiple alignments... */
typedef struct
{
  GList *forward, *reverse;
}ForwardReverseColumnListsStruct, *ForwardReverseColumnLists;


/* required for the sizing of the widget... */
typedef struct
{
  unsigned int debug   : 1 ;

  GtkWidget *toplevel;

  GtkRequisition widget_requisition;
  GtkRequisition user_requisition;
} SizingDataStruct, *SizingData;


static GtkWidget *make_menu_bar(ColConfigure configure_data);
static GtkWidget *make_cols_panel(ZMapWindow window, char *frame_title, GList *column_groups,
				  GtkWidget **label_container_out, GtkWidget **button_container_out) ;
static char *label_text_from_column(FooCanvasGroup *column_group);
static GtkWidget *create_label(GtkWidget *parent, ButData *button_data_out, char *text);
static void create_radio_buttons(GtkWidget *parent, ButData button_data, 
				 ZMapWindowItemFeatureSetData set_data);
static GtkWidget *create_apply_button(GtkWidget *parent, ColConfigure configure_data);


static void colConfigure(ZMapWindow window, GList *forward_cols, GList *reverse_cols) ;
static void simpleConfigure(ZMapWindow window, ZMapWindowColConfigureMode configure_mode,
			    FooCanvasGroup *column_group) ;
static void requestDestroyCB(gpointer data, guint callback_action, GtkWidget *widget) ;
static void destroyCB(GtkWidget *widget, gpointer cb_data) ;
static void destroy_label_cb(GtkWidget *widget, gpointer cb_data) ;
static void helpCB(gpointer data, guint callback_action, GtkWidget *w) ;
static void showButCB(GtkToggleButton *togglebutton, gpointer user_data) ;

static ZMapStyleColumnDisplayState button_label_to_state(const char *button_label_text);

static void select_all_buttons(GtkWidget *button, gpointer user_data);
static void toggle_column_radio_group_activate_matching_state(GtkWidget *toplevel,
							      FooCanvasGroup *column_group,
							      ZMapWindowColConfigureMode mode);
static void set_column_lists_cb(FooCanvasGroup *container, FooCanvasPoints *points, 
				ZMapContainerLevelType level, gpointer user_data);


static gboolean zmapAddSizingSignalHandlers(GtkWidget *widget, gboolean debug,
					    int width, int height);


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
  zmapWindowColumnConfigure(window, NULL, ZMAPWINDOWCOLUMN_CONFIGURE_ALL) ;

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
    case ZMAPWINDOWCOLUMN_HIDE:
    case ZMAPWINDOWCOLUMN_SHOW:
      simpleConfigure(window, configure_mode, column_group) ;
      break ;
    case ZMAPWINDOWCOLUMN_CONFIGURE:
    case ZMAPWINDOWCOLUMN_CONFIGURE_ALL:
      {
	GList *forward_columns = NULL, *reverse_columns = NULL  ;

	zmapWindowColumnConfigureDestroy(window) ;

	if (configure_mode == ZMAPWINDOWCOLUMN_CONFIGURE)
	  {
	    ZMapStrand strand ;

	    strand = zmapWindowContainerGetStrand(column_group);
	    zMapAssert(strand == ZMAPSTRAND_FORWARD || strand == ZMAPSTRAND_REVERSE) ;

	    if (strand == ZMAPSTRAND_FORWARD)
	      forward_columns = g_list_append(forward_columns, column_group) ;
	    else
	      reverse_columns = g_list_append(reverse_columns, column_group) ;
	  }
	else if(column_group)
	  {
	    zmapWindowContainerBlockGetColumnLists(column_group,
						   &forward_columns,
						   &reverse_columns);
	  }
	else
	  {
	    ForwardReverseColumnListsStruct for_rev_lists = {NULL};

	    zmapWindowContainerExecute(window->feature_root_group,
				       ZMAPCONTAINER_LEVEL_FEATURESET,
				       set_column_lists_cb, &for_rev_lists);

	    forward_columns = for_rev_lists.forward;
	    reverse_columns = for_rev_lists.reverse;
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

  if(window->col_config_window)
    {
      ColConfigure configure_data;

      if((configure_data = g_object_get_data(G_OBJECT(window->col_config_window),
					     CONFIGURE_DATA)))
	{
	  /* fix up the relevant flags */
	  if(!configure_data->has_apply_button)
	    {
	      configure_data->apply_now  = TRUE;
	      configure_data->reposition = TRUE;
	    }
	}

    }

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
      toggle_column_radio_group_activate_matching_state(window->col_config_window, 
							column_group, configure_mode) ;
    }
  else
    {
      if (configure_mode == ZMAPWINDOWCOLUMN_HIDE)
	col_state = ZMAPSTYLE_COLDISPLAY_HIDE ;
      else
	col_state = ZMAPSTYLE_COLDISPLAY_SHOW ;
      
      /* TRUE makes the function do a zmapWindowFullReposition() if needed */
      zmapWindowColumnSetState(window, column_group, col_state, TRUE) ;
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
  g_object_set_data(G_OBJECT(toplevel), CONFIGURE_DATA, configure_data) ;
  
  gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;
  title = zMapGUIMakeTitleString("Column configuration",
				 (char *)g_quark_to_string(window->feature_context->sequence_name)) ;
  gtk_window_set_title(GTK_WINDOW(toplevel), title) ;
  g_free(title) ;

  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(toplevel), vbox) ;

  menubar = make_menu_bar(configure_data) ;
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0) ;

  if (reverse_cols)
    {
      cols = make_cols_panel(window, "Reverse Strand", reverse_cols,
			     &(configure_data->reverse_label_container),
			     &(configure_data->reverse_button_container)) ;

      gtk_box_pack_start(GTK_BOX(hbox), cols, TRUE, TRUE, 0) ;
    }

  if (forward_cols)
    {
      cols = make_cols_panel(window, "Forward Strand", forward_cols,
			     &(configure_data->forward_label_container),
			     &(configure_data->forward_button_container)) ;

      gtk_box_pack_start(GTK_BOX(hbox), cols, TRUE, TRUE, 0) ;
    }

  if((reverse_cols && forward_cols) || 
     g_list_length(reverse_cols) > 1 || 
     g_list_length(forward_cols) > 1)
    configure_data->has_apply_button = TRUE;

  if(configure_data->has_apply_button)
    {
      GtkWidget *apply_button;

      apply_button = create_apply_button(vbox, configure_data);

      gtk_window_set_default(GTK_WINDOW(toplevel), apply_button) ;
    }

  gtk_widget_show_all(toplevel) ;

  return ;
}

/* Simple make menu bar function */
static GtkWidget *make_menu_bar(ColConfigure configure_data)
{
  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;
  GtkWidget *menubar ;
  gint nmenu_items = sizeof (menu_items_G) / sizeof (menu_items_G[0]);

  accel_group  = gtk_accel_group_new ();

  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group );

  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items_G,
				(gpointer)configure_data) ;

  gtk_window_add_accel_group(GTK_WINDOW(configure_data->window->col_config_window), accel_group) ;

  menubar = gtk_item_factory_get_widget(item_factory, "<main>");

  return menubar ;
}

static void free_select_all_data_cb(gpointer user_data)
{
  g_free(user_data);

  return ;
}

static void create_select_all_button(GtkWidget *button_box, char *label, 
				     GtkWidget *radio_group_container,
				     ZMapStyleColumnDisplayState state)
{
  GtkWidget *button;
  SelectAllConfigure select_all ;
  
  button = gtk_button_new_with_label(label);
  
  gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);

  select_all = g_new0(SelectAllConfigureStruct, 1);
  
  select_all->radio_groups_container = radio_group_container;
  select_all->select_button_state    = state;

  g_signal_connect_data(G_OBJECT(button), "clicked", 
			G_CALLBACK(select_all_buttons), select_all, 
			(GClosureNotify)free_select_all_data_cb, 0);
  return ;
}

static GtkWidget *make_cols_panel(ZMapWindow window, char *frame_title, GList *columns_list,
				  GtkWidget **label_container_out, GtkWidget **button_container_out)
{
  GtkWidget *cols_panel, *named_frame, *vbox, *hbox, *label_box, *column_box ;
  GtkWidget *scroll_vbox, *scrolled, *viewport;
  GList *column = NULL ;
  int list_length = 0;

  /* Create a new frame (Forward|Reverse Strand) */
  cols_panel = named_frame = gtk_frame_new(frame_title) ;
  gtk_container_set_border_width(GTK_CONTAINER(named_frame), 
                                 ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);
  /* A vbox to go in the frame */
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(named_frame), vbox) ;

  /* A scrolled window to go in the vbox ... */
  scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(vbox), scrolled);

  /* ... which needs a box to put the viewport in ... */
  scroll_vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_set_border_width(GTK_CONTAINER(scroll_vbox),
				 ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

  /* ... which we need to put the rows of radio buttons in for when
   * there's a large number of canvas columns. */
  viewport = gtk_viewport_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(viewport), scroll_vbox);
  gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport), GTK_SHADOW_NONE);

  gtk_container_add(GTK_CONTAINER(scrolled), viewport);

  /* A box for the label and column boxes */
  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(scroll_vbox), hbox) ;

  /* A box for the labels */
  label_box = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(hbox), label_box) ;
  *label_container_out = label_box ;

  /* A box for the columns */
  column_box = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(hbox), column_box) ;
  *button_container_out = column_box ;

  /* Make sure we get the correctly sized widget... */
  zmapAddSizingSignalHandlers(viewport, FALSE, -1, -1);

  if((column = g_list_first(columns_list)))
    {
      do
	{
	  FooCanvasGroup *column_group = FOO_CANVAS_GROUP(column->data) ;
	  ZMapWindowItemFeatureSetData set_data ;
	  ButData button_data ;
	  GtkWidget *label, *button_box;
	  char *label_text;

	  label_text = label_text_from_column(column_group);

	  /* create the label that the user can understand */
	  /* this also creates the button_data... */
	  label = create_label(label_box, &button_data, label_text);
	  
	  /* Set the button_data we need for the individual buttons */
	  button_data->column_group   = column_group ;

	  button_box   = gtk_hbox_new(FALSE, 0) ;
	  /* Show set of radio buttons for each column to change column display state. */
	  gtk_box_pack_start(GTK_BOX(column_box), button_box, TRUE, TRUE, 0);

	  g_object_set_data(G_OBJECT(label), RADIO_BUTTONS_CONTAINER, button_box);

	  /* create the actual radio buttons... */
	  set_data = g_object_get_data(G_OBJECT(column_group), ITEM_FEATURE_SET_DATA) ;
	  create_radio_buttons(button_box, button_data, set_data);

	  list_length++;
	}
      while ((column = g_list_next(column))) ;
    }

  /* We don't need this for a single column. */
  if (list_length > 1)
    {
      GtkWidget *frame, *button_box;

      frame = gtk_frame_new(NULL) ;

      gtk_container_set_border_width(GTK_CONTAINER(frame), 
				     ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

      gtk_box_pack_end(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

      button_box = gtk_hbutton_box_new();

      gtk_container_add(GTK_CONTAINER(frame), button_box) ;

      create_select_all_button(button_box, SHOW_LABEL " All",
			       column_box,
			       ZMAPSTYLE_COLDISPLAY_SHOW);

      create_select_all_button(button_box, SHOWHIDE_LABEL " All", 
			       column_box,
			       ZMAPSTYLE_COLDISPLAY_SHOW_HIDE);

      create_select_all_button(button_box, HIDE_LABEL " All", 
			       column_box,
			       ZMAPSTYLE_COLDISPLAY_HIDE);
    }

  return cols_panel ;
}

static char *label_text_from_column(FooCanvasGroup *column_group)
{
  ZMapWindowItemFeatureSetData set_data ;
  char *label_text;

  /* Get hold of the style. */
  set_data = g_object_get_data(G_OBJECT(column_group), ITEM_FEATURE_SET_DATA) ;
  zMapAssert(set_data) ;

  label_text = (char *)(g_quark_to_string(set_data->style_id));

  return label_text;
}

/* Quick function to create an aligned label with the name of the style.
 * Label has a destroy callback for the button_data!
 */
static GtkWidget *create_label(GtkWidget *parent, ButData *button_data_out, char *text)
{
  GtkWidget *label;
  ButData button_data ;

  /* show the column name. */
  label = gtk_label_new(text);
  
  /* x, y alignments between 0.0 (left, top) and 1.0 (right, bottom) [0.5 == centered] */
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  
  gtk_box_pack_start(GTK_BOX(parent), label, TRUE, TRUE, 0) ;
  
  /* Create button data for this column */
  button_data = g_new0(ButDataStruct, 1) ;

  /* Setup the destruction of the button_data too. */
  g_signal_connect(G_OBJECT(label), "destroy",
		   G_CALLBACK(destroy_label_cb), (gpointer)button_data) ;

  if(button_data_out)
    *button_data_out = button_data;

  return label;
}

static void finished_press_cb(ColConfigure configure_data)
{
  configure_data->reposition  = configure_data->apply_now = FALSE;
  configure_data->return_func = NULL;

  return;
}

/* This allows for the temporary immediate redraw, see
 * finished_press_cb and ->return_func invocation in showButCB */
/* Implemented like this as I couldn't find how to "wrap" the "toggle" signal */
static gboolean press_button_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  GdkModifierType unwanted = (GDK_LOCK_MASK | GDK_MOD2_MASK | GDK_MOD3_MASK | GDK_MOD4_MASK | GDK_MOD5_MASK);
  GtkWidget *toplevel;
  ColConfigure configure_data;

  switch(event->type)
    {
    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
      {
	GdkEventButton *button = (GdkEventButton *)event;
	GdkModifierType locks, control = GDK_CONTROL_MASK;

	locks    = button->state & unwanted;
	control |= locks;

	if(zMapGUITestModifiersOnly(button, control))
	  {
	    toplevel = zMapGUIFindTopLevel(widget);
	    
	    configure_data = g_object_get_data(G_OBJECT(toplevel), CONFIGURE_DATA);

	    if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	      {
		/* Only do this if we're going to toggle, otherwise it'll remain set... */
		configure_data->apply_now   = configure_data->reposition = TRUE;
		configure_data->return_func = finished_press_cb;
	      }
	  }
      }
      break;
    default:
      break;
    }
  
  return FALSE;
}

/* Create enough radio buttons for the user to toggle between all the states of column visibility */
static void create_radio_buttons(GtkWidget *parent, ButData button_data, 
				 ZMapWindowItemFeatureSetData set_data)
{
  GtkWidget *radio_show, *radio_maybe, *radio_hide;
  GtkRadioButton *radio_group_button;

  radio_show = gtk_radio_button_new_with_label(NULL, SHOW_LABEL) ;

  radio_group_button = GTK_RADIO_BUTTON(radio_show);

  gtk_box_pack_start(GTK_BOX(parent), radio_show, TRUE, TRUE, 0) ;

  radio_maybe = gtk_radio_button_new_with_label_from_widget(radio_group_button, 
							    SHOWHIDE_LABEL) ;

  gtk_box_pack_start(GTK_BOX(parent), radio_maybe, TRUE, TRUE, 0) ;
  
  radio_hide = gtk_radio_button_new_with_label_from_widget(radio_group_button, 
							   HIDE_LABEL) ;
  gtk_box_pack_start(GTK_BOX(parent), radio_hide, TRUE, TRUE, 0) ;

  if(set_data)
    {
      ZMapStyleColumnDisplayState col_state ;
      GtkWidget *active_button;
      
      col_state = zmapWindowItemFeatureSetGetDisplay(set_data) ;

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
    }

  g_signal_connect(G_OBJECT(radio_show), "toggled",
		   G_CALLBACK(showButCB), button_data) ;
  g_signal_connect(G_OBJECT(radio_show), "event",
		   G_CALLBACK(press_button_cb), button_data) ;

  g_signal_connect(G_OBJECT(radio_maybe), "toggled",
		   G_CALLBACK(showButCB), button_data);
  g_signal_connect(G_OBJECT(radio_maybe), "event",
		   G_CALLBACK(press_button_cb), button_data);

  g_signal_connect(G_OBJECT(radio_hide), "toggled",
		   G_CALLBACK(showButCB), button_data);
  g_signal_connect(G_OBJECT(radio_hide), "event",
		   G_CALLBACK(press_button_cb), button_data);

  return ;
}

static void apply_state_cb(GtkWidget *widget, gpointer user_data)
{
  if(GTK_IS_TOGGLE_BUTTON(widget))
    {
      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	g_signal_emit_by_name(G_OBJECT(widget), "toggled");
    }
  else if(GTK_IS_CONTAINER(widget))
    {
      gtk_container_foreach(GTK_CONTAINER(widget),
			    apply_state_cb, user_data);
    }
  else
    zMapLogWarning("widget '%s' is not a radio button", G_OBJECT_TYPE_NAME(G_OBJECT(widget)));


  return ;
}

static void apply_button_cb(GtkButton *apply_button, gpointer user_data)
{
  ColConfigure configure_data = (ColConfigure)user_data;
  gboolean save_apply_now, save_reposition;

  save_apply_now  = configure_data->apply_now;
  save_reposition = configure_data->reposition;

  configure_data->apply_now  = TRUE;
  configure_data->reposition = FALSE;

  if(configure_data->forward_button_container)
    gtk_container_foreach(GTK_CONTAINER(configure_data->forward_button_container),
			  apply_state_cb, configure_data);

  if(configure_data->reverse_button_container)
    gtk_container_foreach(GTK_CONTAINER(configure_data->reverse_button_container),
			  apply_state_cb, configure_data);

  configure_data->apply_now  = save_apply_now;
  configure_data->reposition = save_reposition;

  zmapWindowFullReposition(configure_data->window);

  return ;
}


static GtkWidget *create_apply_button(GtkWidget *parent, ColConfigure configure_data)
{
  GtkWidget *button_box, *apply_button, *frame;

  button_box = gtk_hbutton_box_new();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (button_box), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX(button_box), 
                       ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING);
  gtk_container_set_border_width (GTK_CONTAINER (button_box), 
                                  ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

  apply_button = gtk_button_new_with_label("Apply") ;
  gtk_container_add(GTK_CONTAINER(button_box), apply_button) ;

  gtk_signal_connect(GTK_OBJECT(apply_button), "clicked",
		     GTK_SIGNAL_FUNC(apply_button_cb), (gpointer)configure_data) ;

  /* set apply button as default. */
  GTK_WIDGET_SET_FLAGS(apply_button, GTK_CAN_DEFAULT) ;

  frame = gtk_frame_new(NULL) ;
  gtk_container_set_border_width(GTK_CONTAINER(frame), 
                                 ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);
  gtk_box_pack_start(GTK_BOX(parent), frame, FALSE, FALSE, 0) ;

  gtk_container_add(GTK_CONTAINER(frame), button_box);

  return apply_button;
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
    "\n"
    "To redraw the display, either click the \"Apply\" button, or if there is no button the\n"
    "display will be redrawn immediately.  To temporarily turn on the immediate redraw hold\n"
    "Control while selecting the radio button."
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


static void destroy_label_cb(GtkWidget *widget, gpointer cb_data)
{
  ButData button_data = (ButData)cb_data ;

  /* Just freeing the struct.  Nothing else is our responsibility */
  g_free(button_data) ;

  return ;
}


static ZMapStyleColumnDisplayState button_label_to_state(const char *button_label_text)
{
  ZMapStyleColumnDisplayState col_state = ZMAPSTYLE_COLDISPLAY_SHOW;

  if(button_label_text != NULL)
    {
      if(strcmp(SHOW_LABEL, button_label_text) == 0)
	col_state = ZMAPSTYLE_COLDISPLAY_SHOW;
      else if(strcmp(SHOWHIDE_LABEL, button_label_text) == 0)
	col_state = ZMAPSTYLE_COLDISPLAY_SHOW_HIDE;
      else if(strcmp(HIDE_LABEL, button_label_text) == 0)
	col_state = ZMAPSTYLE_COLDISPLAY_HIDE;
      else				/* Something wrong */
	zMapLogWarning("Label '%s', not recognised", button_label_text);
    }
  else
    zMapLogWarning("Label == NULL!", button_label_text);

  return col_state;
}

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
      const char *button_label_text;
      ButData button_data = (ButData)user_data;
      ColConfigure configure_data;
      ZMapStyleColumnDisplayState col_state ;
      GtkWidget *toplevel;

      /* Get the toplevel widget... */
      toplevel = zMapGUIFindTopLevel(GTK_WIDGET(togglebutton));

      /* ...so we can et hold of it's data. */
      configure_data = g_object_get_data(G_OBJECT(toplevel), CONFIGURE_DATA);

      /* which column state are we setting? Get label, */
      button_label_text = gtk_button_get_label(GTK_BUTTON(togglebutton));
      /* and translate it... */
      col_state = button_label_to_state(button_label_text);

      /* Set the correct state. */
      /* We only do this if there is no apply button.  */
      if(configure_data->apply_now)
	zmapWindowColumnSetState(configure_data->window, button_data->column_group, 
				 col_state, configure_data->reposition) ;

      if(configure_data->return_func)
	(configure_data->return_func)(configure_data);
    }

  return ;
}

static void toggle_radio_group_activate_matching_state(gpointer widget_data, gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET(widget_data);

  if(GTK_IS_RADIO_BUTTON(widget))
    {
      ZMapStyleColumnDisplayState button_col_state, user_state;
      const char *button_label_text = NULL;

      button_label_text = gtk_button_get_label(GTK_BUTTON(widget));
  
      button_col_state  = button_label_to_state(button_label_text);
      
      user_state = GPOINTER_TO_INT(user_data);

      if(button_col_state == user_state)
	{
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE) ;      
	}
    }
  else if(GTK_IS_CONTAINER(widget))
    {
      gtk_container_foreach(GTK_CONTAINER(widget),
			    (GtkCallback)toggle_radio_group_activate_matching_state, user_data);
    }
  else
    zMapLogWarning("widget '%s' is not a radio button", G_OBJECT_TYPE_NAME(G_OBJECT(widget)));

  return ;
}

static void select_all_buttons(GtkWidget *button, gpointer user_data)
{
  GList *button_boxes;
  gboolean needs_reposition;
  ColConfigure configure_data;
  GtkWidget *radio_groups_container, *toplevel;
  SelectAllConfigure select_all = (SelectAllConfigure)user_data;

  toplevel = zMapGUIFindTopLevel(button);

  configure_data = g_object_get_data(G_OBJECT(toplevel), CONFIGURE_DATA);

  radio_groups_container = select_all->radio_groups_container;

  button_boxes = gtk_container_get_children(GTK_CONTAINER(radio_groups_container));

  needs_reposition = configure_data->reposition;

  configure_data->reposition = FALSE;

  g_list_foreach(button_boxes, toggle_radio_group_activate_matching_state, 
		 GINT_TO_POINTER(select_all->select_button_state));
  
  if((configure_data->reposition = needs_reposition))
    zmapWindowFullReposition(configure_data->window);

  return ;
}

static gint match_label_with_text_cb(gconstpointer label_data, gconstpointer text_data)
{
  GtkWidget *label = GTK_WIDGET(label_data);
  char *text = (char *)text_data;
  const char *label_text = NULL;
  int match = -1;

  label_text = gtk_label_get_text(GTK_LABEL(label));
  
  if(strcmp(text, label_text) == 0)
    match = 0;

  return match;
}

/* Finds the button which represents column_group and sets its state to match configure_mode. */

/* Tactic.  
 * - Get the correct strand container for labels
 * - Get the label matching the column_group
 * - Get the children of the labels->button_data->button_box
 * - Search these for the correct radio button to toggle.
 * */

static void toggle_column_radio_group_activate_matching_state(GtkWidget *toplevel,
							      FooCanvasGroup *column_group,
							      ZMapWindowColConfigureMode mode)
{
  ColConfigure configure_data;
  GtkWidget *label_container, *column_label = NULL;
  ZMapStrand strand;
  GList *labels, *matching_label;
  char *column_text = NULL;

  /* First get the configure_data  */
  configure_data = g_object_get_data(G_OBJECT(toplevel), CONFIGURE_DATA);

  /* - Get the correct strand container for labels */
  strand = zmapWindowContainerGetStrand(column_group);
  zMapAssert(strand == ZMAPSTRAND_FORWARD || strand == ZMAPSTRAND_REVERSE) ;

  if (strand == ZMAPSTRAND_FORWARD)
    label_container = configure_data->forward_label_container ;
  else
    label_container = configure_data->reverse_label_container ;

  labels = gtk_container_get_children(GTK_CONTAINER(label_container)) ;
  
  /* - Get the label matching the column_group. We use the text for this... */
  column_text = label_text_from_column(column_group);

  if((matching_label = g_list_find_custom(labels, column_text, match_label_with_text_cb)))
    column_label = GTK_WIDGET(matching_label->data);

  /* so long as we've got a label, proceed... */
  if(column_label)
    {
      ZMapStyleColumnDisplayState col_state;
      GtkWidget *button_container;
      GList *buttons = NULL;
      gboolean save_apply_now, save_reposition;

      /* - Get the children of the labels->button_data->button_box */
      button_container = g_object_get_data(G_OBJECT(column_label), RADIO_BUTTONS_CONTAINER);

      buttons = gtk_container_get_children(GTK_CONTAINER(button_container));

      if (mode == ZMAPWINDOWCOLUMN_HIDE)
	col_state = ZMAPSTYLE_COLDISPLAY_HIDE ;
      else
	col_state = ZMAPSTYLE_COLDISPLAY_SHOW ;

      /* - Search these for the correct radio button to toggle. */
      save_apply_now  = configure_data->apply_now;
      save_reposition = configure_data->reposition;

      configure_data->apply_now  = TRUE;
      configure_data->reposition = TRUE;

      g_list_foreach(buttons, toggle_radio_group_activate_matching_state, 
		     GINT_TO_POINTER(col_state));

      configure_data->apply_now  = save_apply_now;
      configure_data->reposition = save_reposition;
    }

  return ;
}

/* Direct copy from zmapWindowContainer.c ... */
static void set_column_lists_cb(FooCanvasGroup *container, FooCanvasPoints *points, 
				ZMapContainerLevelType level, gpointer user_data)
{
  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      {
	ForwardReverseColumnLists lists_data;
	ZMapWindowItemFeatureSetData set_data;

	lists_data = (ForwardReverseColumnLists)user_data;

	if((set_data = g_object_get_data(G_OBJECT(container), ITEM_FEATURE_SET_DATA)))
	  {
	    if(set_data->strand == ZMAPSTRAND_FORWARD)
	      lists_data->forward = g_list_append(lists_data->forward, container);
	    else
	      lists_data->reverse = g_list_append(lists_data->reverse, container);
	  }
      }
      break;
    default:
      break;
    }

  return ;
}


/* *********************************************************
 * * Code to do the resizing of the widget, according to   *
 * * available space, and to add scrollbar when required   *
 * *********************************************************
 */

/* Entry point is zmapAddSizingSignalHandlers() */


/* Called by widget_size_request() */

static void handle_scrollbars(GtkWidget      *widget,
			      GtkRequisition *requisition_inout)
{
  GtkWidget *parent;

  if((parent = gtk_widget_get_parent(widget)) &&
     (GTK_IS_SCROLLED_WINDOW(parent)))
    {
      GtkWidget *scrollbar;
      
      if((scrollbar = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(parent))))
	{
	  GtkRequisition v_requisition;
	  int scrollbar_spacing = 0;
	  
	  gtk_widget_size_request(scrollbar, &v_requisition);
	  
	  gtk_widget_style_get(parent,
			       "scrollbar-spacing", &scrollbar_spacing,
			       NULL);
	  
	  v_requisition.width      += scrollbar_spacing;
	  requisition_inout->width += v_requisition.width;
	}
      
      if((scrollbar = gtk_scrolled_window_get_hscrollbar(GTK_SCROLLED_WINDOW(parent))))
	{
	  GtkRequisition h_requisition;
	  int scrollbar_spacing = 0;
	  
	  gtk_widget_size_request(scrollbar, &h_requisition);
	  
	  gtk_widget_style_get(parent,
			       "scrollbar-spacing", &scrollbar_spacing,
			       NULL);
	  
	  h_requisition.height      += scrollbar_spacing;
	  requisition_inout->height += h_requisition.height;
	}
    }

  return ;
}

/* Called by widget_size_request() */

static void restrict_screen_size_to_toplevel(GtkWidget      *widget,
					     GtkRequisition *screen_size)
{
  GtkWidget *toplevel = zMapGUIFindTopLevel( widget );
  int widget_dx, widget_dy;

  widget_dx = toplevel->allocation.width  - widget->allocation.width;
  widget_dy = toplevel->allocation.height - widget->allocation.height;

  screen_size->width  -= widget_dx;
  screen_size->height -= widget_dy;

  return ;
}

/* Called by widget_size_request() */

static void get_screen_size(GtkWidget      *widget,
			    double          screen_percentage,
			    GtkRequisition *requisition_inout)
{
  GdkScreen *screen;
  gboolean has_screen;

  if((has_screen = gtk_widget_has_screen(widget)) &&
     (screen     = gtk_widget_get_screen(widget)) &&
     (requisition_inout))  
    {
      int screen_width, screen_height;

      screen_width   = gdk_screen_get_width(screen);
      screen_height  = gdk_screen_get_height(screen);
      
      screen_width  *= screen_percentage;
      screen_height *= screen_percentage;

      requisition_inout->width  = screen_width;
      requisition_inout->height = screen_height;

#ifdef DEBUG_SCREEN_SIZE
      requisition_inout->width  = 600;
      requisition_inout->height = 400;
#endif
    }

  return ;
}


static void widget_size_request(GtkWidget      *widget,
				GtkRequisition *requisition_inout,
				gpointer        user_data)
{
  SizingData sizing_data = (SizingData)user_data;
  GtkRequisition screen_requisition = {-1, -1};

  gtk_widget_size_request(widget, requisition_inout);

  handle_scrollbars(widget, requisition_inout);

  if(!sizing_data->toplevel)
    sizing_data->toplevel = zMapGUIFindTopLevel(widget);

  get_screen_size(sizing_data->toplevel, 0.85, &screen_requisition);

  restrict_screen_size_to_toplevel(widget, &screen_requisition);

  requisition_inout->width  = MIN(requisition_inout->width,  screen_requisition.width);
  requisition_inout->height = MIN(requisition_inout->height, screen_requisition.height);

  requisition_inout->width  = MAX(requisition_inout->width,  -1);
  requisition_inout->height = MAX(requisition_inout->height, -1);

  return ;
}

/* 
 * The following 4 functions are to modify the sizing behaviour of the
 * GtkTreeView Widget.  By default it uses a very small size and
 * although it's possible to resize it, this is not useful for the
 * users who would definitely prefer _not_ to have to do this for
 * _every_ window that is opened.
 */
static void  zmap_widget_size_request(GtkWidget      *widget,
				      GtkRequisition *requisition,
				      gpointer        user_data)
{

  widget_size_request(widget, requisition, user_data);

  return ;
}

static gboolean zmap_widget_map(GtkWidget      *widget,
				gpointer        user_data)
{
  SizingData sizing_data = (SizingData)user_data;

  sizing_data->widget_requisition = widget->requisition;

  widget_size_request(widget, &(sizing_data->widget_requisition), user_data);

#ifdef DEBUGGING_ONLY
  if(sizing_data->debug)
    g_warning("map: %d x %d", sizing_data->widget_requisition.width, sizing_data->widget_requisition.height);
#endif /* DEBUGGING_ONLY */

  gtk_widget_set_size_request(widget,
			      sizing_data->widget_requisition.width,
			      sizing_data->widget_requisition.height);

  return FALSE;
}

static gboolean zmap_widget_unmap(GtkWidget *widget,
				  gpointer   user_data)
{
  SizingData sizing_data = (SizingData)user_data;

  gtk_widget_set_size_request(widget, 
			      sizing_data->user_requisition.width,
			      sizing_data->user_requisition.height);

#ifdef DEBUGGING_ONLY
  if(sizing_data->debug)
    g_warning("unmap: %d x %d", widget->requisition.width, widget->requisition.height);
#endif /* DEBUGGING_ONLY */

  return FALSE;
}

static void zmap_widget_size_allocation(GtkWidget     *widget,
					GtkAllocation *allocation,
					gpointer       user_data)
{
  SizingData sizing_data = (SizingData)user_data;

  gtk_widget_set_size_request(widget, 
			      sizing_data->user_requisition.width,
			      sizing_data->user_requisition.height);

#ifdef DEBUGGING_ONLY
  if(sizing_data->debug)
    g_warning("alloc: %d x %d", allocation->width, allocation->height);
#endif /* DEBUGGING_ONLY */

  return ;
}

static gboolean zmapAddSizingSignalHandlers(GtkWidget *widget, gboolean debug,
					    int width, int height)
{
  SizingData sizing_data = g_new0(SizingDataStruct, 1);
  
  sizing_data->debug  = debug;

  sizing_data->user_requisition.width  = width;
  sizing_data->user_requisition.height = height;

  g_signal_connect(G_OBJECT(widget), "size-request",
		   G_CALLBACK(zmap_widget_size_request), sizing_data);

  g_signal_connect(G_OBJECT(widget), "map",
		   G_CALLBACK(zmap_widget_map), sizing_data);

  g_signal_connect(G_OBJECT(widget), "size-allocate",
		   G_CALLBACK(zmap_widget_size_allocation), sizing_data);

  g_signal_connect(G_OBJECT(widget), "unmap",
		   G_CALLBACK(zmap_widget_unmap), sizing_data);


  return TRUE;
}
