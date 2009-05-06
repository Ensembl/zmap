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
 * Last edited: May  6 09:58 2009 (rds)
 * Created: Thu Mar  2 09:07:44 2006 (edgrif)
 * CVS info:   $Id: zmapWindowColConfig.c,v 1.30 2009-05-06 08:59:22 rds Exp $
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

#define NOTEBOOK_PAGE_DATA "notebook_page_data"

typedef struct _ColConfigureStruct *ColConfigure;

#define CONFIGURE_DATA "col_data"
typedef struct _ColConfigureStruct
{
  ZMapWindow window ;

  ZMapWindowColConfigureMode mode;

  GtkWidget *notebook;
  GtkWidget *loaded_page;
  GtkWidget *deferred_page;

  GtkWidget *vbox;

  unsigned int has_apply_button : 1 ;

} ColConfigureStruct;


typedef struct _NotebookPageStruct *NotebookPage;

typedef enum
  {
    NOTEBOOK_INVALID_FUNC,
    NOTEBOOK_POPULATE_FUNC,
    NOTEBOOK_APPLY_FUNC,
    NOTEBOOK_UPDATE_FUNC,
    NOTEBOOK_CLEAR_FUNC,
  } NotebookFuncType;

typedef void (*NotebookPageConstruct)(NotebookPage notebook_page, GtkWidget *page);

typedef void (*NotebookPageColFunc)(NotebookPage notebook_page, FooCanvasGroup *column_group);

typedef void (*NotebookPageFunc)(NotebookPage notebook_page);

typedef struct _NotebookPageStruct
{
  ColConfigure          configure_data;

  GtkWidget            *page_container;
  char                 *page_name;

  NotebookPageConstruct page_constructor; /* A+ */
  NotebookPageColFunc   page_populate;    /* B+ */
  NotebookPageFunc      page_apply;	  /* C  */
  NotebookPageColFunc   page_update;      /* D */
  NotebookPageFunc      page_clear;	  /* B- */
  NotebookPageFunc      page_destroy;	  /* A- */

  gpointer              page_data;
} NotebookPageStruct;


typedef struct _ShowHidePageDataStruct *ShowHidePageData;

typedef void (*ShowHidePageResetFunc)(ShowHidePageData page_data);

typedef struct _ShowHidePageDataStruct
{
  /* Forward strand lists of ShowHideButton pointers */
  GList *fwd_show_list;
  GList *fwd_default_list;
  GList *fwd_hide_list;
  /* Reverse strand lists of ShowHideButton pointers */
  GList *rev_show_list;
  GList *rev_default_list;
  GList *rev_hide_list;

  /*  */
  ShowHidePageResetFunc reset_func;

  unsigned int apply_now        : 1 ;
  unsigned int reposition       : 1 ;

} ShowHidePageDataStruct;


typedef struct
{
  ShowHidePageData            show_hide_data;
  ZMapStyleColumnDisplayState show_hide_state;
  FooCanvasGroup             *show_hide_column;
  GtkWidget                  *show_hide_button;
} ShowHideButtonStruct, *ShowHideButton;




typedef struct _DeferredPageDataStruct *DeferredPageData;

typedef void (*DeferredPageResetFunc)(DeferredPageData page_data);

typedef struct _DeferredPageDataStruct
{
  ZMapFeatureBlock block;

  GList *load_in_mark;
  GList *load_all;
  GList *load_none;

} DeferredPageDataStruct;


typedef struct
{
  DeferredPageData deferred_page_data;
  FooCanvasGroup  *column_group;
  GQuark           column_quark;
  GtkWidget       *column_button;
} DeferredButtonStruct, *DeferredButton;





/* Arrgh, pain this is needed.  Also, will fall over when multiple alignments... */
typedef struct
{
  GList *forward, *reverse;
  gboolean loaded_or_deferred;	/* loaded = FALSE, deferred = TRUE */
  ZMapWindowItemFeatureBlockData block_data;
  int mark1, mark2;
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
static GtkWidget *loaded_cols_panel(NotebookPage notebook_page, char *frame_title, GList *column_groups,
				  GList **show_list_out, GList **default_list_out, GList **hide_list_out) ;
static char *label_text_from_column(FooCanvasGroup *column_group);
static GtkWidget *create_label(GtkWidget *parent, char *text);
static GtkWidget *create_revert_apply_button(ColConfigure configure_data);

static GtkWidget *make_scrollable_vbox(GtkWidget *vbox);
static ColConfigure configure_create(ZMapWindow window, ZMapWindowColConfigureMode configure_mode);

static void requestDestroyCB(gpointer data, guint callback_action, GtkWidget *widget) ;
static void destroyCB(GtkWidget *widget, gpointer cb_data) ;

static void helpCB(gpointer data, guint callback_action, GtkWidget *w) ;
static void loaded_show_button_cb(GtkToggleButton *togglebutton, gpointer user_data) ;

static GtkWidget *configure_make_toplevel(ColConfigure configure_data);
static void configure_add_pages(ColConfigure configure_data);
static void configure_simple(ColConfigure configure_data, FooCanvasGroup *column_group) ;
static void configure_populate_containers(ColConfigure    configure_data, 
					  FooCanvasGroup *column_group);
static void configure_clear_containers(ColConfigure configure_data);

static gboolean show_press_button_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data);
static void apply_button_cb(GtkWidget *apply_button, gpointer user_data);
static void revert_button_cb(GtkWidget *apply_button, gpointer user_data);


static void select_all_buttons(GtkWidget *button, gpointer user_data);
static void set_column_lists_cb(FooCanvasGroup *container, FooCanvasPoints *points, 
				ZMapContainerLevelType level, gpointer user_data);


static void notebook_foreach_page(GtkWidget *notebook_widget, gboolean current_page_only,
				  NotebookFuncType func_type, gpointer foreach_data);

static void apply_state_cb(GtkWidget *widget, gpointer user_data);

static FooCanvasGroup *configure_get_point_block_container(ColConfigure configure_data, 
							   FooCanvasGroup *column_group);

static void loaded_radio_buttons(GtkWidget      *parent,
				 FooCanvasGroup *column_group,
				 ShowHideButton *show_out,
				 ShowHideButton *default_out,
				 ShowHideButton *hide_out);
static void column_group_set_active_button(ZMapWindowItemFeatureSetData set_data,
					   GtkWidget *radio_show,
					   GtkWidget *radio_maybe,
					   GtkWidget *radio_hide);



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
void zmapWindowColumnConfigure(ZMapWindow                 window,
			       FooCanvasGroup            *column_group,
			       ZMapWindowColConfigureMode configure_mode)
{
  ColConfigure configure_data = NULL;
  int page_no = 0;

  zMapAssert(window);

  if(window->col_config_window)
    {
      configure_data = g_object_get_data(G_OBJECT(window->col_config_window),
					 CONFIGURE_DATA);
    }

  switch (configure_mode)
    {
    case ZMAPWINDOWCOLUMN_HIDE:
    case ZMAPWINDOWCOLUMN_SHOW:
      {
	ColConfigureStruct configure_struct = {NULL};

	if(!configure_data)
	  {
	    configure_data         = &configure_struct;
	    configure_data->window = window;
	    configure_data->mode   = configure_mode;
	  }

	configure_simple(configure_data, column_group) ;
      }
      break ;
    case ZMAPWINDOWCOLUMN_CONFIGURE:
    case ZMAPWINDOWCOLUMN_CONFIGURE_ALL:
      {
	gboolean created = FALSE;

	if(configure_data == NULL)
	  {
	    configure_data = configure_create(window, configure_mode);
	    created = TRUE;
    	  }
	else
	  {
	    configure_clear_containers(configure_data);
	    page_no = gtk_notebook_get_current_page(GTK_NOTEBOOK(configure_data->notebook));
	  }

	configure_populate_containers(configure_data, column_group);

	if(created && configure_data->has_apply_button)
	  {
	    GtkWidget *apply_button;

	    apply_button = create_revert_apply_button(configure_data);
	    
	    gtk_window_set_default(GTK_WINDOW(window->col_config_window), apply_button) ;
	  }

	break ;
      }
    default:
      zMapAssert("Coding error, unrecognised column configure type") ;
      break ;
    }

  if(window->col_config_window)
    {
      gtk_notebook_set_current_page(GTK_NOTEBOOK(configure_data->notebook), page_no);

      gtk_widget_show_all(window->col_config_window);
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


static void configure_simple(ColConfigure configure_data,
			     FooCanvasGroup *column_group)
{
  ZMapStyleColumnDisplayState col_state ;

  /* If there's a config window displayed then toggle correct button in window to effect
   * change, otherwise operate directly on column. */
  if (configure_data->window->col_config_window &&
      configure_data->notebook)
    {
      notebook_foreach_page(configure_data->notebook, FALSE,
			    NOTEBOOK_UPDATE_FUNC, column_group) ;
    }
  else
    {
      if (configure_data->mode == ZMAPWINDOWCOLUMN_HIDE)
	col_state = ZMAPSTYLE_COLDISPLAY_HIDE ;
      else
	col_state = ZMAPSTYLE_COLDISPLAY_SHOW ;
      
      /* TRUE makes the function do a zmapWindowFullReposition() if needed */
      zmapWindowColumnSetState(configure_data->window, column_group, col_state, TRUE) ;
    }

  return ;
}

/* create the widget and all the containers where we draw the buttons etc... */
static ColConfigure configure_create(ZMapWindow window, ZMapWindowColConfigureMode configure_mode)
{
  GtkWidget *toplevel, *vbox, *menubar;
  ColConfigure configure_data ;

  configure_data = g_new0(ColConfigureStruct, 1) ;
  
  configure_data->window = window ;
  configure_data->mode   = configure_mode;

  /* set up the top level window */
  configure_data->window->col_config_window = 
    toplevel = configure_make_toplevel(configure_data);

  /* vbox for everything below the toplevel */
  configure_data->vbox = vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(toplevel), vbox) ;

  /* menu... */
  menubar = make_menu_bar(configure_data) ;
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

  /* make notebook and pack it. */
  configure_data->notebook = gtk_notebook_new();
  gtk_box_pack_start(GTK_BOX(vbox), configure_data->notebook, TRUE, TRUE, 0) ;

  /* add loaded and deferred pages */
  configure_add_pages(configure_data);

  return configure_data;
}

static GtkWidget *configure_make_toplevel(ColConfigure configure_data)
{
  GtkWidget *toplevel = NULL;

  if(!configure_data->window->col_config_window)
    {
      char *title, *seq_name ;

      /* New toplevel */
      toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
      /* Add destroy func - destroyCB */
      g_signal_connect(GTK_OBJECT(toplevel), "destroy",
		       GTK_SIGNAL_FUNC(destroyCB), (gpointer)configure_data) ;

      /* Save a record of configure_data, so we only save the widget */
      g_object_set_data(G_OBJECT(toplevel), CONFIGURE_DATA, configure_data) ;
  
      gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;

      /* Get sequence name for the title */
      seq_name = (char *)g_quark_to_string(configure_data->window->feature_context->sequence_name);

      /* make the title, and set */
      if((title = zMapGUIMakeTitleString("Column configuration", seq_name)))
	{
	  gtk_window_set_title(GTK_WINDOW(toplevel), title) ;
	  g_free(title) ;	/* free me now. */
	}
    }

  return toplevel;
}



static void configure_get_column_lists(ColConfigure configure_data, 
				       FooCanvasGroup *column_group,
				       gboolean deferred_or_loaded, /* TRUE = deferred, FALSE = loaded */
				       GList **forward_columns_out, GList **reverse_columns_out)
{
  ZMapWindowColConfigureMode configure_mode;
  GList *forward_columns = NULL, *reverse_columns = NULL  ;
  ZMapWindow window;

  configure_mode = configure_data->mode;
  window         = configure_data->window;

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
  else
    {
      ForwardReverseColumnListsStruct forward_reverse_lists = {NULL};
      FooCanvasGroup *block_group;
      ZMapFeatureBlock block;
      gboolean use_mark_if_marked = FALSE;

      /* get block */
      block_group = configure_get_point_block_container(configure_data, column_group);

      block = g_object_get_data(G_OBJECT(block_group), ITEM_FEATURE_DATA);
      
      forward_reverse_lists.loaded_or_deferred = deferred_or_loaded;
      forward_reverse_lists.block_data = g_object_get_data(G_OBJECT(block_group), ITEM_FEATURE_BLOCK_DATA);

      if(use_mark_if_marked && window->mark && zmapWindowMarkIsSet(window->mark))
	{
	  zmapWindowGetMarkedSequenceRangeFwd(window, block, 
					      &(forward_reverse_lists.mark1),
					      &(forward_reverse_lists.mark2));
	}
      else
	{
	  forward_reverse_lists.mark1 = block->block_to_sequence.q1;
	  forward_reverse_lists.mark2 = block->block_to_sequence.q2;
	}

      zmapWindowContainerExecute(block_group,
				 ZMAPCONTAINER_LEVEL_FEATURESET,
				 set_column_lists_cb, &forward_reverse_lists);

      forward_columns = forward_reverse_lists.forward;
      reverse_columns = forward_reverse_lists.reverse;
    }

  if(forward_columns_out)
    *forward_columns_out = forward_columns;
  if(reverse_columns_out)
    *reverse_columns_out = reverse_columns ;
  
  return ;
}

static void loaded_page_construct(NotebookPage notebook_page, GtkWidget *page)
{
  notebook_page->page_container = page;

  return ;
}

static void loaded_page_populate (NotebookPage notebook_page, FooCanvasGroup *column_group)
{
  ShowHidePageData show_hide_data;
  ColConfigure configure_data;
  GtkWidget *hbox, *cols;
  GList *forward_cols, *reverse_cols;

  configure_data = notebook_page->configure_data;

  hbox = notebook_page->page_container;

  configure_get_column_lists(configure_data, 
			     column_group, FALSE,
			     &forward_cols, &reverse_cols);

  show_hide_data = (ShowHidePageData)(notebook_page->page_data);

  if((reverse_cols && forward_cols) || 
     g_list_length(reverse_cols) > 1 || 
     g_list_length(forward_cols) > 1)
    configure_data->has_apply_button = TRUE;
  else
    {
      show_hide_data->apply_now  = TRUE;
      show_hide_data->reposition = TRUE;
    }

  if (reverse_cols)
    {
      cols = loaded_cols_panel(notebook_page, 
			       "Reverse Strand", reverse_cols,
			       &(show_hide_data->rev_show_list),
			       &(show_hide_data->rev_default_list),
			       &(show_hide_data->rev_hide_list)) ;

      gtk_box_pack_start(GTK_BOX(hbox), cols, TRUE, TRUE, 0) ;

      g_list_free(reverse_cols);
    }

  if (forward_cols)
    {
      cols = loaded_cols_panel(notebook_page, 
			       "Forward Strand", forward_cols,
			       &(show_hide_data->fwd_show_list),
			       &(show_hide_data->fwd_default_list),
			       &(show_hide_data->fwd_hide_list)) ;

      gtk_box_pack_start(GTK_BOX(hbox), cols, TRUE, TRUE, 0) ;

      g_list_free(forward_cols);
    }


  return ;
}

static void each_button_toggle_if_active(gpointer list_data, gpointer user_data)
{
  ShowHideButton button_data  = (ShowHideButton)list_data;

  GtkWidget *widget;

  widget = button_data->show_hide_button;

  if(GTK_IS_TOGGLE_BUTTON(widget))
    {
      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	g_signal_emit_by_name(G_OBJECT(widget), "toggled");
    }


  return ;
}

static void loaded_page_apply  (NotebookPage notebook_page)
{
  ShowHidePageData show_hide_data;
  ColConfigure configure_data;
  gboolean save_apply_now, save_reposition;

  configure_data = notebook_page->configure_data;
  show_hide_data = (ShowHidePageData)(notebook_page->page_data);

  save_apply_now  = show_hide_data->apply_now;
  save_reposition = show_hide_data->reposition;

  show_hide_data->apply_now  = TRUE;
  show_hide_data->reposition = FALSE;

  g_list_foreach(show_hide_data->fwd_show_list,    each_button_toggle_if_active, configure_data);
  g_list_foreach(show_hide_data->fwd_default_list, each_button_toggle_if_active, configure_data);
  g_list_foreach(show_hide_data->fwd_hide_list,    each_button_toggle_if_active, configure_data);

  g_list_foreach(show_hide_data->rev_show_list,    each_button_toggle_if_active, configure_data);
  g_list_foreach(show_hide_data->rev_default_list, each_button_toggle_if_active, configure_data);
  g_list_foreach(show_hide_data->rev_hide_list,    each_button_toggle_if_active, configure_data);

  show_hide_data->apply_now  = save_apply_now;
  show_hide_data->reposition = save_reposition;

  zmapWindowFullReposition(configure_data->window);

 return ;
}

static void container_clear(GtkWidget *widget)
{
  if(GTK_IS_CONTAINER(widget))
    {
      GtkContainer *container;
      GList *children;

      container = GTK_CONTAINER(widget);

      children = gtk_container_get_children(container);

      g_list_foreach(children, (GFunc)gtk_widget_destroy, NULL);
    }

  return ;
}


static void loaded_page_clear  (NotebookPage notebook_page)
{
  ShowHidePageData page_data;
  container_clear(notebook_page->page_container);

  page_data = (ShowHidePageData)(notebook_page->page_data);

  g_list_foreach(page_data->fwd_show_list, (GFunc)g_free, NULL);
  g_list_free(page_data->fwd_show_list);
  page_data->fwd_show_list = NULL;

  g_list_foreach(page_data->fwd_default_list, (GFunc)g_free, NULL);
  g_list_free(page_data->fwd_default_list);
  page_data->fwd_default_list = NULL;

  g_list_foreach(page_data->fwd_hide_list, (GFunc)g_free, NULL);
  g_list_free(page_data->fwd_hide_list);
  page_data->fwd_hide_list = NULL;

  g_list_foreach(page_data->rev_show_list, (GFunc)g_free, NULL);
  g_list_free(page_data->rev_show_list);
  page_data->rev_show_list = NULL;

  g_list_foreach(page_data->rev_default_list, (GFunc)g_free, NULL);
  g_list_free(page_data->rev_default_list);
  page_data->rev_default_list = NULL;

  g_list_foreach(page_data->rev_hide_list, (GFunc)g_free, NULL);
  g_list_free(page_data->rev_hide_list);
  page_data->rev_hide_list = NULL;

  return ;
}

static void loaded_page_destroy(NotebookPage notebook_page)
{
  return ;
}

static void column_group_set_active_button(ZMapWindowItemFeatureSetData set_data,
					   GtkWidget *radio_show,
					   GtkWidget *radio_maybe,
					   GtkWidget *radio_hide)
{
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
	case ZMAPSTYLE_COLDISPLAY_SHOW:
	default: 
	  active_button = radio_show ;
	  break ;
	}
      
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(active_button), TRUE) ;
    }

  return ;
}

/* Create enough radio buttons for the user to toggle between all the states of column visibility */
static void loaded_radio_buttons(GtkWidget      *parent,
				 FooCanvasGroup *column_group,
				 ShowHideButton *show_out,
				 ShowHideButton *default_out,
				 ShowHideButton *hide_out)
{
  GtkWidget *radio_show, *radio_maybe, *radio_hide;
  GtkRadioButton *radio_group_button;
  ShowHideButton show_data, default_data, hide_data;

  /* Create the "show" radio button, which will create the group too. */
  radio_show = gtk_radio_button_new_with_label(NULL, SHOW_LABEL) ;

  gtk_box_pack_start(GTK_BOX(parent), radio_show, TRUE, TRUE, 0) ;

  /* Get the group so we can add the other buttons to the same group */
  radio_group_button = GTK_RADIO_BUTTON(radio_show);

  /* Create the "default" radio button */
  radio_maybe = gtk_radio_button_new_with_label_from_widget(radio_group_button, 
							    SHOWHIDE_LABEL) ;

  gtk_box_pack_start(GTK_BOX(parent), radio_maybe, TRUE, TRUE, 0) ;
  
  /* Create the "hide" radio button */
  radio_hide = gtk_radio_button_new_with_label_from_widget(radio_group_button, 
							   HIDE_LABEL) ;
  gtk_box_pack_start(GTK_BOX(parent), radio_hide, TRUE, TRUE, 0) ;

  /* Now create the data to attach to the buttons and connect the callbacks */

  /* Show */
  show_data = g_new0(ShowHideButtonStruct, 1);
  show_data->show_hide_button = radio_show;
  show_data->show_hide_state  = ZMAPSTYLE_COLDISPLAY_SHOW;
  show_data->show_hide_column = column_group;

  g_signal_connect(G_OBJECT(radio_show), "toggled",
		   G_CALLBACK(loaded_show_button_cb), show_data) ;
  g_signal_connect(G_OBJECT(radio_show), "event",
		   G_CALLBACK(show_press_button_cb), show_data) ;

  /* Default */
  default_data = g_new0(ShowHideButtonStruct, 1);
  default_data->show_hide_button = radio_maybe;
  default_data->show_hide_state  = ZMAPSTYLE_COLDISPLAY_SHOW_HIDE;
  default_data->show_hide_column = column_group;

  g_signal_connect(G_OBJECT(radio_maybe), "toggled",
		   G_CALLBACK(loaded_show_button_cb), default_data);
  g_signal_connect(G_OBJECT(radio_maybe), "event",
		   G_CALLBACK(show_press_button_cb), default_data);

  /* Hide */
  hide_data = g_new0(ShowHideButtonStruct, 1);
  hide_data->show_hide_button = radio_hide;
  hide_data->show_hide_state  = ZMAPSTYLE_COLDISPLAY_HIDE;
  hide_data->show_hide_column = column_group;

  g_signal_connect(G_OBJECT(radio_hide), "toggled",
		   G_CALLBACK(loaded_show_button_cb), hide_data);
  g_signal_connect(G_OBJECT(radio_hide), "event",
		   G_CALLBACK(show_press_button_cb), hide_data);


  /* Return the data... We need to add it to the list */
  if(show_out)
    *show_out = show_data;

  if(default_out)
    *default_out = default_data;

  if(hide_out)
    *hide_out = hide_data;

  return ;
}

static void activate_matching_column(gpointer list_data, gpointer user_data)
{
  ShowHideButton button_data = (ShowHideButton)list_data;
  FooCanvasGroup *column_group = FOO_CANVAS_GROUP(user_data);

  if(button_data->show_hide_column == column_group)
    {
      GtkWidget *widget;

      widget = button_data->show_hide_button;

      if(GTK_IS_TOGGLE_BUTTON(widget))
	{
	  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	    g_signal_emit_by_name(G_OBJECT(widget), "toggled");
	}
    }

  return ;
}

static void loaded_page_update(NotebookPage notebook_page,
			       FooCanvasGroup *column_group)
{
  ColConfigure configure_data;
  ZMapStrand strand;
  GList *show_hide_button_list = NULL;
  ZMapWindowColConfigureMode mode;
  ShowHidePageData show_hide_data;

  /* First get the configure_data  */
  configure_data = notebook_page->configure_data;

  mode = configure_data->mode;

  /* - Get the correct strand container for labels */
  strand = zmapWindowContainerGetStrand(column_group);
  zMapAssert(strand == ZMAPSTRAND_FORWARD || strand == ZMAPSTRAND_REVERSE) ;

  switch(mode)
    {
    case ZMAPWINDOWCOLUMN_HIDE:
      if(strand == ZMAPSTRAND_FORWARD)
	show_hide_button_list = show_hide_data->fwd_hide_list;
      else
	show_hide_button_list = show_hide_data->rev_hide_list;
      break;
    case ZMAPWINDOWCOLUMN_SHOW:
    default:
      if(strand == ZMAPSTRAND_FORWARD)
	show_hide_button_list = show_hide_data->fwd_show_list;
      else
	show_hide_button_list = show_hide_data->rev_show_list;
      break;
    }
  
  if(show_hide_button_list)
    {
      gboolean save_reposition, save_apply_now;

      save_apply_now  = show_hide_data->apply_now;
      save_reposition = show_hide_data->reposition;

      /* We're only aiming to find one button to toggle so reposition = TRUE is ok. */
      show_hide_data->apply_now  = TRUE;
      show_hide_data->reposition = TRUE;

      g_list_foreach(show_hide_button_list, activate_matching_column, column_group);

      show_hide_data->apply_now  = save_apply_now;
      show_hide_data->reposition = save_reposition;
    }

  return ;
}

static NotebookPage loaded_page_create(ColConfigure configure_data, char **page_name_out)
{
  NotebookPage page_data = NULL;

  if((page_data = g_new0(NotebookPageStruct, 1)))
    {
      page_data->configure_data = configure_data;
      page_data->page_name      = g_strdup("Current Columns");

      if(page_name_out)
	*page_name_out = page_data->page_name;

      page_data->page_constructor = loaded_page_construct;
      page_data->page_populate    = loaded_page_populate;
      page_data->page_apply       = loaded_page_apply;
      page_data->page_update      = loaded_page_update;
      page_data->page_clear       = loaded_page_clear;
      page_data->page_destroy     = loaded_page_destroy;

      page_data->page_data = g_new0(ShowHidePageDataStruct, 1);
    }

  return page_data;
}


static void deferred_page_construct(NotebookPage notebook_page, GtkWidget *page)
{
  notebook_page->page_container = page;
}

static void deferred_radio_buttons(GtkWidget *parent, FooCanvasGroup *column_group, gboolean loaded_in_mark,
				   DeferredButton *all_out, DeferredButton *mark_out, DeferredButton *none_out)
{
  GtkWidget *radio_load_all, *radio_load_mark, *radio_deferred;
  GtkRadioButton *radio_group_button;
  DeferredButton all, mark, none;

  radio_load_all = gtk_radio_button_new_with_label(NULL, "All Data") ;

  radio_group_button = GTK_RADIO_BUTTON(radio_load_all);

  gtk_box_pack_start(GTK_BOX(parent), radio_load_all, TRUE, TRUE, 0) ;

  radio_load_mark = gtk_radio_button_new_with_label_from_widget(radio_group_button, 
								"Marked Area") ;

  gtk_box_pack_start(GTK_BOX(parent), radio_load_mark, TRUE, TRUE, 0) ;
  
  radio_deferred = gtk_radio_button_new_with_label_from_widget(radio_group_button, 
							       "Do Not Load/No Data") ;
  gtk_box_pack_start(GTK_BOX(parent), radio_deferred, TRUE, TRUE, 0) ;

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_deferred), TRUE) ;

  if(loaded_in_mark)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_load_mark), TRUE) ;

  all = g_new0(DeferredButtonStruct, 1);
  all->column_button = radio_load_all;
  all->column_group  = column_group;
  mark = g_new0(DeferredButtonStruct, 1);
  mark->column_button = radio_load_mark;
  mark->column_group  = column_group;
  none = g_new0(DeferredButtonStruct, 1);
  none->column_button = radio_deferred;
  none->column_group  = column_group;

  if(all_out)
    *all_out = all;
  if(mark_out)
    *mark_out = mark;
  if(none_out)
    *none_out = none;

  return ;
}

static void each_button_activate(gpointer list_data, gpointer user_data)
{
  DeferredButton button_data = (DeferredButton)list_data;
  GtkWidget *widget;

  widget = button_data->column_button;

  if(GTK_IS_RADIO_BUTTON(widget))
    {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
    }

  return ;
}

static gboolean deferred_activate_all(GtkWidget *button, gpointer user_data)
{
  GList *deferred_button_list = (GList *)user_data;

  g_list_foreach(deferred_button_list, each_button_activate, NULL);

  return TRUE;
}

static gint find_name_cb(gconstpointer list_data, gconstpointer user_data)
{
  const char *list_column_name;
  const char *query_column_name;
  gint match = -1;

  list_column_name  = (const char *)list_data;
  query_column_name = (const char *)user_data;

  match = g_ascii_strcasecmp(list_column_name, query_column_name);

  return match;
}

static GtkWidget *deferred_cols_panel(NotebookPage notebook_page,
				      GList       *columns_list)
{
  DeferredPageData deferred_page_data;
  GtkWidget *frame = NULL, *vbox, *hbox;
  GtkWidget *label_box, *column_box, *scroll_vbox;
  ZMapWindow window;
  gboolean mark_set = TRUE;
  GList *column;
  int list_length = 0, mark1, mark2;

  deferred_page_data = (DeferredPageData)(notebook_page->page_data);

  window = notebook_page->configure_data->window;

  if((mark_set = zmapWindowMarkIsSet(window->mark)))
    {      
      zmapWindowGetMarkedSequenceRangeFwd(window, deferred_page_data->block, 
					  &mark1, &mark2);
    }
  
  frame = gtk_frame_new("Available Columns");
  gtk_container_set_border_width(GTK_CONTAINER(frame), 
                                 ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  scroll_vbox = make_scrollable_vbox(vbox);

  /* A box for the label and column boxes */
  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(scroll_vbox), hbox) ;

  /* A box for the labels */
  label_box = gtk_vbox_new(FALSE, 0) ;


  /* A box for the columns */
  column_box = gtk_vbox_new(FALSE, 0) ;



  if((column = g_list_first(columns_list)))
    {
      ZMapWindowItemFeatureBlockData block_data;
      FooCanvasGroup *block_group;
      FooCanvasGroup *first_column_group = FOO_CANVAS_GROUP(column->data);
      GList *make_unique = NULL;

      if((block_group = configure_get_point_block_container(notebook_page->configure_data, first_column_group)))
	{
	  block_data = g_object_get_data(G_OBJECT(block_group), ITEM_FEATURE_BLOCK_DATA);
	}

      do
	{
	  GtkWidget *label, *button_box;
	  FooCanvasGroup *column_group = FOO_CANVAS_GROUP(column->data);
	  char *column_name;
	  DeferredButton all, mark, none;
	  gboolean loaded_in_mark = FALSE;
	  
	  column_name = label_text_from_column(column_group);
	  
	  if(!g_list_find_custom(make_unique, column_name, find_name_cb))
	    {
	      label = create_label(label_box, column_name);
	      
	      button_box = gtk_hbox_new(FALSE, 0);
	      
	      gtk_box_pack_start(GTK_BOX(column_box), button_box, TRUE, TRUE, 0);
	      
	      if(mark_set && block_data)
		loaded_in_mark = zmapWindowItemFeatureBlockIsColumnLoaded(block_data, column_group, 
									  mark1, mark2);
	      
	      deferred_radio_buttons(button_box, column_group, loaded_in_mark, 
				     &all, &mark, &none);
	      
	      all->deferred_page_data = mark->deferred_page_data = 
		none->deferred_page_data = deferred_page_data;
	      
	      gtk_widget_set_sensitive(mark->column_button, mark_set);
	      
	      deferred_page_data->load_all     = g_list_append(deferred_page_data->load_all, all);
	      deferred_page_data->load_in_mark = g_list_append(deferred_page_data->load_in_mark, mark);
	      deferred_page_data->load_none    = g_list_append(deferred_page_data->load_none, none);
	      
	      /* If no mark set then we need to make mark button insensitive */
	      
	      list_length++;
	      make_unique = g_list_append(make_unique, column_name);
	    }
	}
      while((column = g_list_next(column)));
 
      g_list_free(make_unique);
    }

  if(list_length > 0)
    {
      gtk_container_add(GTK_CONTAINER(hbox), label_box) ;
      gtk_container_add(GTK_CONTAINER(hbox), column_box) ;
      notebook_page->configure_data->has_apply_button = TRUE;
    }

  if(list_length > 1)
    {
      GtkWidget *button, *frame, *button_box;

      frame = gtk_frame_new(NULL) ;

      gtk_container_set_border_width(GTK_CONTAINER(frame), 
				     ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

      gtk_box_pack_end(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

      button_box = gtk_hbutton_box_new();

      gtk_container_add(GTK_CONTAINER(frame), button_box) ;

      /* button for selecting all full loads */
      button = gtk_button_new_with_label("Load All Columns Fully");
  
      gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);

      g_signal_connect(G_OBJECT(button), "clicked",
		       G_CALLBACK(deferred_activate_all), deferred_page_data->load_all);

      /* button for selecting all full loads */
      button = gtk_button_new_with_label("Load All Columns Marked");
  
      gtk_widget_set_sensitive(button, mark_set);

      gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);

      g_signal_connect(G_OBJECT(button), "clicked",
		       G_CALLBACK(deferred_activate_all), deferred_page_data->load_in_mark);

      /* button for selecting all full loads */
      button = gtk_button_new_with_label("Load None");
  
      gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);

      g_signal_connect(G_OBJECT(button), "clicked",
		       G_CALLBACK(deferred_activate_all), deferred_page_data->load_none);      
    }
  else if(list_length == 0)
    {
      GtkWidget *label;

      label = gtk_label_new("No extra columns available");

      gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.0);

      gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
    }

  return frame;
}

static void set_block(FooCanvasGroup *container, FooCanvasPoints *points, 
		      ZMapContainerLevelType level, gpointer user_data)
{
  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_BLOCK:
      {
	FooCanvasGroup **block = (FooCanvasGroup **)user_data;

	if(block && !(*block))
	  *block = container;
      }
      break;
    default:
      break;
    }

  return ;
}

static FooCanvasGroup *configure_get_point_block_container(ColConfigure configure_data, 
							   FooCanvasGroup *column_group)
{
  FooCanvasGroup *block = NULL;

  if(!column_group)
    {
      ZMapWindow window;
      gboolean mark_set = FALSE;

      window = configure_data->window;

      /* Either look in the mark, or get the first block */
      if(window->mark)
	mark_set = zmapWindowMarkIsSet(window->mark);

      if(mark_set)
	{
	  double x1, y1, x2, y2;
	  int wy1,wy2;

	  zmapWindowMarkGetWorldRange(window->mark, &x1, &y1, &x2, &y2);

	  zmapWindowWorld2SeqCoords(window, x1, y1, x2, y2, &block, &wy1, &wy2);
	}
      else
	{
	  /* Just get the first one! */
	  zmapWindowContainerExecute(window->feature_root_group,
				     ZMAPCONTAINER_LEVEL_BLOCK,
				     set_block, &block);
	}
    }
  else
    {
      /* parent block (canvas container) should have block attached... */
      block = zmapWindowContainerGetParentLevel(FOO_CANVAS_ITEM(column_group),
						ZMAPCONTAINER_LEVEL_BLOCK);
    }

  return block;
}

static void deferred_page_populate(NotebookPage notebook_page, FooCanvasGroup *column_group)
{
  GList *all_deferred_columns = NULL;
  GtkWidget *hbox, *frame;
  FooCanvasGroup *point_block = NULL;
  DeferredPageData page_data;

  point_block = configure_get_point_block_container(notebook_page->configure_data, column_group);
  page_data   = (DeferredPageData)notebook_page->page_data;

  page_data->block = g_object_get_data(G_OBJECT(point_block), ITEM_FEATURE_DATA);;

  configure_get_column_lists(notebook_page->configure_data,
			     NULL, TRUE, 
			     &all_deferred_columns, NULL);

  hbox = notebook_page->page_container;
  
  if((frame = deferred_cols_panel(notebook_page, all_deferred_columns)))
    {
      gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
    }
  
  return ;
}

static void add_name_to_list(gpointer list_data, gpointer user_data)
{
  DeferredButton button_data = (DeferredButton)list_data;
  GList **list_out = (GList **)user_data;
  GtkWidget *widget;

  widget = button_data->column_button;

  if(GTK_IS_RADIO_BUTTON(widget))
    {
      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	{
	  ZMapWindowItemFeatureSetData set_data;
	  set_data = g_object_get_data(G_OBJECT(button_data->column_group), ITEM_FEATURE_SET_DATA);
	  *list_out = g_list_append(*list_out, GUINT_TO_POINTER(set_data->unique_id));
	}
    }

  return ;
}

static void deferred_page_apply(NotebookPage notebook_page)
{
  DeferredPageData deferred_data;
  ColConfigure configure_data;
  FooCanvasGroup *block_group;
  ZMapFeatureBlock block = NULL;
  GList *mark_list = NULL, *all_list = NULL;

  /* Get the data we need */
  configure_data = notebook_page->configure_data;
  deferred_data  = (DeferredPageData)(notebook_page->page_data);

  if((block_group = configure_get_point_block_container(configure_data, NULL)))
    {
      block = g_object_get_data(G_OBJECT(block_group), ITEM_FEATURE_DATA);
      
      /* Go through the mark only ones... */
      g_list_foreach(deferred_data->load_in_mark, add_name_to_list, &mark_list);
      
      /* Go through the load all ones... */
      g_list_foreach(deferred_data->load_all, add_name_to_list, &all_list);
      
      zmapWindowFetchData(configure_data->window, block, mark_list, TRUE);
      zmapWindowFetchData(configure_data->window, block, all_list, FALSE);
    }

  return ;
}

static void deferred_page_clear(NotebookPage notebook_page)
{
  DeferredPageData page_data;
  container_clear(notebook_page->page_container);

  page_data = (DeferredPageData)(notebook_page->page_data);
  g_list_foreach(page_data->load_in_mark, (GFunc)g_free, NULL);
  g_list_free(page_data->load_in_mark);
  page_data->load_in_mark = NULL;

  g_list_foreach(page_data->load_all, (GFunc)g_free, NULL);
  g_list_free(page_data->load_all);
  page_data->load_all = NULL;

  g_list_foreach(page_data->load_none, (GFunc)g_free, NULL);
  g_list_free(page_data->load_none);
  page_data->load_none = NULL;

  return ;
}
static void deferred_page_destroy(NotebookPage notebook_page)
{
  return ;
}


static NotebookPage deferred_page_create(ColConfigure configure_data, char **page_name_out)
{
  NotebookPage page_data = NULL;

  if((page_data = g_new0(NotebookPageStruct, 1)))
    {
      page_data->configure_data = configure_data;
      page_data->page_name      = g_strdup("Available Columns");

      if(page_name_out)
	*page_name_out = page_data->page_name;

      page_data->page_constructor = deferred_page_construct;
      page_data->page_populate    = deferred_page_populate;
      page_data->page_apply       = deferred_page_apply;
      page_data->page_clear       = deferred_page_clear;
      page_data->page_destroy     = deferred_page_destroy;

      page_data->page_data = g_new0(DeferredPageDataStruct, 1);
    }

  return page_data;
}



static void configure_add_pages(ColConfigure configure_data)
{
  NotebookPage page_data;
  GtkWidget *label, *new_page;
  char *page_name = NULL;

  if((page_data = loaded_page_create(configure_data, &page_name)))
    {
      label = gtk_label_new(page_name);

      configure_data->loaded_page = new_page = gtk_hbox_new(FALSE, 0);

      g_object_set_data(G_OBJECT(new_page), NOTEBOOK_PAGE_DATA, page_data);

      gtk_notebook_append_page(GTK_NOTEBOOK(configure_data->notebook),
			       new_page, label);

      if(page_data->page_constructor)
	(page_data->page_constructor)(page_data, new_page);
    }

  if((page_data = deferred_page_create(configure_data, &page_name)))
    {
      label = gtk_label_new(page_name);

      configure_data->deferred_page = new_page = gtk_hbox_new(FALSE, 0);

      g_object_set_data(G_OBJECT(new_page), NOTEBOOK_PAGE_DATA, page_data);

      gtk_notebook_append_page(GTK_NOTEBOOK(configure_data->notebook),
			       new_page, label);

      if(page_data->page_constructor)
	(page_data->page_constructor)(page_data, new_page);
    }

  return ;
}

static void configure_populate_containers(ColConfigure configure_data, FooCanvasGroup *column_group)
{

  notebook_foreach_page(configure_data->notebook, FALSE, NOTEBOOK_POPULATE_FUNC, column_group);
  
  return ;
}

static void configure_clear_containers(ColConfigure configure_data)
{
  notebook_foreach_page(configure_data->notebook, FALSE, NOTEBOOK_CLEAR_FUNC, NULL);

  return;
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

static void create_select_all_button(GtkWidget *button_box, char *label,
				     GList *show_hide_button_list)
{
  GtkWidget *button;
  
  button = gtk_button_new_with_label(label);
  
  gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);

  g_signal_connect(G_OBJECT(button), "clicked", 
		   G_CALLBACK(select_all_buttons), show_hide_button_list);

  return ;
}


static GtkWidget *make_scrollable_vbox(GtkWidget *vbox)
{
  GtkWidget *scroll_vbox, *scrolled, *viewport;

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

  /* Make sure we get the correctly sized widget... */
  zmapAddSizingSignalHandlers(viewport, FALSE, -1, -1);

  return scroll_vbox;
}

static GtkWidget *loaded_cols_panel(NotebookPage notebook_page, 
				    char        *frame_title, 
				    GList       *columns_list,
				    GList      **show_list_out, 
				    GList      **default_list_out, 
				    GList      **hide_list_out)
{
  GtkWidget *cols_panel, *named_frame, *vbox, *hbox, *label_box, *column_box ;
  GtkWidget *scroll_vbox;
  GList *column   = NULL,	/* If only GList's were better */
    *show_list    = NULL,
    *default_list = NULL,
    *hide_list    = NULL;

  int list_length = 0;

  /* Create a new frame (Forward|Reverse Strand) */
  cols_panel = named_frame = gtk_frame_new(frame_title) ;
  gtk_container_set_border_width(GTK_CONTAINER(named_frame), 
                                 ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);
  /* A vbox to go in the frame */
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(named_frame), vbox) ;

  /* We need a scrollable vbox, which resizes correctly... */
  scroll_vbox = make_scrollable_vbox(vbox);

  /* A box for the label and column boxes */
  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(scroll_vbox), hbox) ;

  /* A box for the labels */
  label_box = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(hbox), label_box) ;

  /* A box for the columns */
  column_box = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(hbox), column_box) ;

  if((column = g_list_first(columns_list)))
    {
      GList *make_unique = NULL;

      do
	{
	  FooCanvasGroup *column_group = FOO_CANVAS_GROUP(column->data) ;
	  ZMapWindowItemFeatureSetData set_data ;
	  ShowHideButton show_data, default_data, hide_data;
	  GtkWidget *label, *button_box;
	  char *label_text;

	  label_text = label_text_from_column(column_group);

	  if(!g_list_find_custom(make_unique, label_text, find_name_cb))
	    {
	      /* create the label that the user can understand */
	      /* this also creates the button_data... */
	      label = create_label(label_box, label_text);
	      
	      button_box   = gtk_hbox_new(FALSE, 0) ;
	      /* Show set of radio buttons for each column to change column display state. */
	      gtk_box_pack_start(GTK_BOX(column_box), button_box, TRUE, TRUE, 0);
	      
	      g_object_set_data(G_OBJECT(label), RADIO_BUTTONS_CONTAINER, button_box);
	      
	      /* create the actual radio buttons... */
	      set_data = g_object_get_data(G_OBJECT(column_group), ITEM_FEATURE_SET_DATA) ;
	      
	      loaded_radio_buttons(button_box, column_group, &show_data, &default_data, &hide_data);
	      
	      show_data->show_hide_data      = 
		default_data->show_hide_data = 
		hide_data->show_hide_data    = (ShowHidePageData)(notebook_page->page_data);
	      
	      show_list    = g_list_append(show_list, show_data);
	      default_list = g_list_append(default_list, default_data);
	      hide_list    = g_list_append(hide_list, hide_data);
	      
	      column_group_set_active_button(set_data, 
					     show_data->show_hide_button,
					     default_data->show_hide_button,
					     hide_data->show_hide_button);
	      
	      list_length++;

	      make_unique = g_list_append(make_unique, label_text);
	    }
	}
      while ((column = g_list_next(column))) ;

      g_list_free(make_unique);

      if(show_list_out)
	*show_list_out = show_list;
      if(default_list_out)
	*default_list_out = default_list;
      if(hide_list_out)
	*hide_list_out = hide_list;
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

      create_select_all_button(button_box, SHOW_LABEL " All", show_list);

      create_select_all_button(button_box, SHOWHIDE_LABEL " All", default_list);

      create_select_all_button(button_box, HIDE_LABEL " All", hide_list);
    }

  return cols_panel ;
}

static char *label_text_from_column(FooCanvasGroup *column_group)
{
  ZMapWindowItemFeatureSetData set_data ;
  GQuark display_id;
  char *label_text;

  /* Get hold of the style. */
  set_data = g_object_get_data(G_OBJECT(column_group), ITEM_FEATURE_SET_DATA) ;
  zMapAssert(set_data) ;

  display_id = zmapWindowItemFeatureSetColumnDisplayName(set_data);

  label_text = (char *)(g_quark_to_string(display_id));

  return label_text;
}

/* Quick function to create an aligned label with the name of the style.
 * Label has a destroy callback for the button_data!
 */
static GtkWidget *create_label(GtkWidget *parent, char *text)
{
  GtkWidget *label;

  /* show the column name. */
  label = gtk_label_new(text);
  
  /* x, y alignments between 0.0 (left, top) and 1.0 (right, bottom) [0.5 == centered] */
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  
  gtk_box_pack_start(GTK_BOX(parent), label, TRUE, TRUE, 0) ;

  return label;
}

static void finished_press_cb(ShowHidePageData show_hide_data)
{
  show_hide_data->reposition = show_hide_data->apply_now = FALSE;
  show_hide_data->reset_func = NULL;

  return;
}

/* This allows for the temporary immediate redraw, see
 * finished_press_cb and ->reset_func invocation in loaded_show_button_cb */
/* Implemented like this as I couldn't find how to "wrap" the "toggle" signal */
static gboolean show_press_button_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  GdkModifierType unwanted = (GDK_LOCK_MASK | GDK_MOD2_MASK | GDK_MOD3_MASK | GDK_MOD4_MASK | GDK_MOD5_MASK);
  ShowHideButton button_data = (ShowHideButton)user_data;
  ShowHidePageData show_hide_data;

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
	    if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
	      {
		show_hide_data = (ShowHidePageData)(button_data->show_hide_data);
		/* Only do this if we're going to toggle, otherwise it'll remain set... */
		show_hide_data->reposition = show_hide_data->apply_now = TRUE;
		show_hide_data->reset_func = finished_press_cb;
	      }
	  }
      }
      break;
    default:
      break;
    }
  
  return FALSE;
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

static GtkWidget *create_revert_apply_button(ColConfigure configure_data)
{
  GtkWidget *button_box, *apply_button, *frame, *parent, *revert_button;

  parent = configure_data->vbox;

  button_box = gtk_hbutton_box_new();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (button_box), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX(button_box), 
                       ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING);
  gtk_container_set_border_width (GTK_CONTAINER (button_box), 
                                  ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

  revert_button = gtk_button_new_with_label("Revert") ;
  gtk_box_pack_start(GTK_BOX(button_box), revert_button, FALSE, FALSE, 0) ;

  gtk_signal_connect(GTK_OBJECT(revert_button), "clicked",
		     GTK_SIGNAL_FUNC(revert_button_cb), (gpointer)configure_data) ;

  apply_button = gtk_button_new_with_label("Apply") ;
  gtk_box_pack_end(GTK_BOX(button_box), apply_button, FALSE, FALSE, 0) ;

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


/* Called when user selects one of the display state radio buttons.
 * 
 * NOTE: selecting a button will deselect the other buttons, this routine
 * gets called for each of those deselections as well. */
static void loaded_show_button_cb(GtkToggleButton *togglebutton, gpointer user_data)
{
  gboolean but_pressed ;

  /* Only do something if the button was pressed on. */
  if ((but_pressed = gtk_toggle_button_get_active(togglebutton)))
    {
      ShowHideButton button_data = (ShowHideButton)user_data; 
      ShowHidePageData page_data;
      ColConfigure configure_data;
      GtkWidget *toplevel;

      /* Get the toplevel widget... */
      if((toplevel = zMapGUIFindTopLevel(GTK_WIDGET(togglebutton))))
	{
	  /* ...so we can et hold of it's data. */
	  configure_data = g_object_get_data(G_OBJECT(toplevel), CONFIGURE_DATA);
	  
	  /* Set the correct state. */
	  /* We only do this if there is no apply button.  */
	  page_data = button_data->show_hide_data;
	  
	  if(page_data->apply_now)
	    {
	      ZMapWindow window;

	      window = configure_data->window;

	      if(window->display_3_frame)
		{
		  ZMapWindowItemFeatureSetData set_data;
		  ZMapFeatureSet feature_set;
		  int i;
		  
		  set_data = g_object_get_data(G_OBJECT(button_data->show_hide_column),
					       ITEM_FEATURE_SET_DATA);

		  if((feature_set = zmapWindowItemFeatureSetRecoverFeatureSet(set_data)))
		    {
		      
		      for(i = ZMAPFRAME_NONE ; i <= ZMAPFRAME_2; i++)
			{
			  FooCanvasItem *frame_column;
			  ZMapFrame frame = (ZMapFrame)i;

			  frame_column = zmapWindowFToIFindSetItem(window->context_to_item, feature_set, 
								   set_data->strand, frame);
			  
			  if(frame_column && zmapWindowContainerHasFeatures(FOO_CANVAS_GROUP(frame_column)))
			    zmapWindowColumnSetState(window, 
						     FOO_CANVAS_GROUP(frame_column),
						     button_data->show_hide_state, 
						     FALSE) ;
			}

		      if(page_data->reposition)
			zmapWindowFullReposition(window);
		    }
		  else
		    {
		      
		    }
		}
	      else
		{
		  zmapWindowColumnSetState(window, 
					   button_data->show_hide_column, 
					   button_data->show_hide_state, 
					   page_data->reposition) ;
		}
	    }

	  if(page_data->reset_func)
	    (page_data->reset_func)(page_data);
	}
    }

  return ;
}

static void each_radio_button_activate(gpointer list_data, gpointer user_data)
{
  ShowHideButton show_hide_button = (ShowHideButton)list_data;
  GtkWidget *widget;

  widget = show_hide_button->show_hide_button;

  if(GTK_IS_RADIO_BUTTON(widget))
    {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE) ;      
    }
  else
    zMapLogWarning("widget '%s' is not a radio button", G_OBJECT_TYPE_NAME(G_OBJECT(widget)));

  return ;
}

static void select_all_buttons(GtkWidget *button, gpointer user_data)
{
  GList *show_hide_button_list = (GList *)user_data;
  gboolean needs_reposition;
  ColConfigure configure_data;
  ShowHidePageData show_hide_data;

  if(show_hide_button_list)
    {
      ShowHideButton first;

      first = (ShowHideButton)(show_hide_button_list->data);

      show_hide_data = first->show_hide_data;

      needs_reposition = show_hide_data->reposition;
      
      show_hide_data->reposition = FALSE;
      
      g_list_foreach(show_hide_button_list, each_radio_button_activate, NULL);
      
      if((show_hide_data->reposition = needs_reposition))
	zmapWindowFullReposition(configure_data->window);
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
	    gboolean want_deferred, is_deferred, is_empty;
	    gboolean loaded = FALSE;

	    want_deferred = lists_data->loaded_or_deferred;
	    is_deferred   = zmapWindowItemFeatureSetGetDeferred(set_data);
	    is_empty      = (zmapWindowContainerHasFeatures(container) == TRUE ? FALSE : TRUE);

	    if(want_deferred && is_deferred)
	      {
		loaded = zmapWindowItemFeatureBlockIsColumnLoaded(lists_data->block_data,
								  container,
								  lists_data->mark1,
								  lists_data->mark2);
	      }
	    else if(!is_empty && !want_deferred)
	      is_deferred = FALSE;

	    if(( want_deferred &&  is_deferred && !loaded) ||
	       (!want_deferred && !is_deferred && !is_empty))
	      {
		if(set_data->strand == ZMAPSTRAND_FORWARD)
		  lists_data->forward = g_list_append(lists_data->forward, container);
		else
		  lists_data->reverse = g_list_append(lists_data->reverse, container);
	      }
	  }
      }
      break;
    default:
      break;
    }

  return ;
}

static void notebook_foreach_page(GtkWidget       *notebook_widget, 
				  gboolean         current_page_only,
				  NotebookFuncType func_type, 
				  gpointer         foreach_data)
{
  GtkNotebook *notebook = GTK_NOTEBOOK(notebook_widget); 
  int pages = 0, i, current_page;

  pages = gtk_notebook_get_n_pages(notebook);

  if(current_page_only)
    current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook_widget));

  for(i = 0; i < pages; i++)
    {
      NotebookPage page_data;
      GtkWidget *page;

      page = gtk_notebook_get_nth_page(notebook, i);

      if(current_page_only && current_page != i)
	continue;

      if((page_data = g_object_get_data(G_OBJECT(page), NOTEBOOK_PAGE_DATA)))
	{
	  switch(func_type)
	    {
	    case NOTEBOOK_APPLY_FUNC:
	      {
		NotebookPageFunc apply_func;
		
		if((apply_func = page_data->page_apply))
		  {
		    (apply_func)(page_data);
		  }
	      }
	      break;
	    case NOTEBOOK_POPULATE_FUNC:
	      {
		NotebookPageColFunc pop_func;
		
		if((pop_func = page_data->page_populate))
		  {
		    (pop_func)(page_data, FOO_CANVAS_GROUP(foreach_data));
		  }
	      }
	      break;
	    case NOTEBOOK_UPDATE_FUNC:
	      {
		NotebookPageColFunc pop_func;
		
		if((pop_func = page_data->page_update))
		  {
		    (pop_func)(page_data, FOO_CANVAS_GROUP(foreach_data));
		  }
	      }
	      break;
	    case NOTEBOOK_CLEAR_FUNC:
	      {
		NotebookPageFunc clear_func;

		if((clear_func = page_data->page_clear))
		  {
		    (clear_func)(page_data);
		  }
	      }
	      break;
	    default:
	      break;
	    }
	}
    }

  return ;
}


static void apply_button_cb(GtkWidget *apply_button, gpointer user_data)
{
  ColConfigure configure_data = (ColConfigure)user_data;

  notebook_foreach_page(configure_data->notebook, FALSE, NOTEBOOK_APPLY_FUNC, NULL);

  return ;
}

static void revert_button_cb(GtkWidget *apply_button, gpointer user_data)
{
  ColConfigure configure_data = (ColConfigure)user_data;

  zmapWindowColumnConfigure(configure_data->window, NULL, configure_data->mode);

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
