/*  File: zmapWindowTextItem.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2009: Genome Research Ltd.
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jun 19 11:40 2009 (rds)
 * Created: Fri Jan 16 11:20:07 2009 (rds)
 * CVS info:   $Id: zmapWindowTextItem.c,v 1.2 2009-06-19 10:48:58 rds Exp $
 *-------------------------------------------------------------------
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <zmapWindowTextItem_I.h>
#include <zmapWindowTextItemCMarshal.h>


/* ==========================================================
 * |                                                        | 
 * |   ZMapWindowTextItem.c                                 | 
 * |                                                        |
 * ==========================================================
 */


/* Object argument IDs */
enum {
	PROP_0,

	PROP_WRAP_MODE,

	PROP_TEXT_ALLOCATE_FUNC,
	PROP_TEXT_FETCH_TEXT_FUNC,
	PROP_TEXT_CALLBACK_DATA,

	PROP_TEXT_SELECT_COLOR_GDK,

	PROP_TEXT_REQUESTED_WIDTH,
	PROP_TEXT_REQUESTED_HEIGHT
};

enum 
  {
    NO_NOTIFY_MASK            = 0,
    ZOOM_NOTIFY_MASK          = 1,
    SCROLL_REGION_NOTIFY_MASK = 2,
    KEEP_WITHIN_SCROLL_REGION = 4
  };



static void zmap_window_text_item_class_init   (ZMapWindowTextItemClass class);
static void zmap_window_text_item_init         (ZMapWindowTextItem text);
static void zmap_window_text_item_destroy      (GtkObject *object);
static void zmap_window_text_item_set_property (GObject            *object,
						 guint               param_id,
						 const GValue       *value,
						 GParamSpec         *pspec);
static void zmap_window_text_item_get_property (GObject            *object,
						 guint               param_id,
						 GValue             *value,
						 GParamSpec         *pspec);

static void   zmap_window_text_item_update     (FooCanvasItem  *item,
						 double            i2w_dx,
						 double            i2w_dy,
						 int               flags);
static void   zmap_window_text_item_map        (FooCanvasItem  *item);
static void   zmap_window_text_item_unmap      (FooCanvasItem  *item);
static void   zmap_window_text_item_realize    (FooCanvasItem  *item);
static void   zmap_window_text_item_unrealize  (FooCanvasItem  *item);
static void   zmap_window_text_item_draw       (FooCanvasItem  *item,
						 GdkDrawable      *drawable,
						 GdkEventExpose   *expose);
static double zmap_window_text_item_point      (FooCanvasItem  *item,
						 double            x,
						 double            y,
						 int               cx,
						 int               cy,
						 FooCanvasItem **actual_item);
#ifdef RDS_DONT_INCLUDE
static void   zmap_window_text_item_translate  (FooCanvasItem  *item,
						 double            dx,
						 double            dy);
#endif
static void   zmap_window_text_item_bounds     (FooCanvasItem  *item,
						 double           *x1,
						 double           *y1,
						 double           *x2,
						 double           *y2);


static void zmap_window_text_item_select(ZMapWindowTextItem text_item, 
					 int index1, int index2, 
					 gboolean deselect,
					 gboolean signal);

static void zmap_window_text_item_deselect(ZMapWindowTextItem text_item,
					   gboolean signal);

/* helper functions */
static void allocate_buffer_size(ZMapWindowTextItem zmap);
static gboolean canvas_has_changed(FooCanvasItem *item, ZMapTextItemDrawData draw_data);
static gboolean invoke_get_text(FooCanvasItem  *item);
static void invoke_allocate_width_height(FooCanvasItem *item);
static void save_origin(FooCanvasItem *item);
static void sync_data(ZMapTextItemDrawData from, ZMapTextItemDrawData to);

/* print out cached data functions */
static void print_private_data(ZMapWindowTextItem zmap);
static void print_draw_data(ZMapTextItemDrawData draw_data);
static void print_calculate_buffer_data(ZMapWindowTextItem zmap,
					ZMapTextItemDrawData   draw_data,
					int cx1, int cy1, 
					int cx2, int cy2);

/* ************** our gdk drawing functions *************** */
static PangoRenderer *zmap_get_renderer(GdkDrawable *drawable,
					GdkGC       *gc,
					GdkColor    *foreground,
					GdkColor    *background);
static void zmap_release_renderer (PangoRenderer *renderer);
static void zmap_pango_renderer_draw_layout_get_size (PangoRenderer    *renderer,
						      PangoLayout      *layout,
						      int               x,
						      int               y,
						      GdkEventExpose   *expose,
						      GdkRectangle     *size_in_out);
static void zmap_gdk_draw_layout_get_size_within_expose(GdkDrawable    *drawable,
							GdkGC          *gc,
							int             x,
							int             y,
							PangoLayout    *layout,
							GdkEventExpose *expose,
							int            *width_max_in_out,
							int            *height_max_in_out);
/* ******************************************************* */

static void highlight_coords_from_cell_bounds(FooCanvasPoints *overlay_points,
					      double          *first, 
					      double          *last, 
					      double           minx, 
					      double           maxx);
static void text_item_pango2item(FooCanvas *canvas, int px, int py, double *ix, double *iy);
static int zmap_pango_layout_iter_skip_lines(PangoLayoutIter *iterator,
					     int              line_count);
static gboolean pick_line_get_bounds(ZMapWindowTextItem zmap_text,
				     ItemEvent       dna_item_event,
				     PangoLayoutIter   *iter,
				     PangoLayoutLine   *line,
				     PangoRectangle    *logical_rect,
				     int                iter_line_index,
				     double             i2w_dx,
				     double             i2w_dy);
static gboolean event_to_char_cell_coords(ZMapWindowTextItem zmap_text,
					  ItemEvent       dna_item_event);

static int text_item_world2text_index(FooCanvasItem *item, double x, double y);
static int text_item_item2text_index(FooCanvasItem *item, double x, double y);
static gboolean zmap_window_text_selected_signal(ZMapWindowTextItem text_item,
						 double x1, double y1,
						 double x2, double y2,
						 int start, int end);
static gboolean emit_signal(ZMapWindowTextItem text_item,
			    guint              signal_id,
			    GQuark             detail);


/* debug highlight is like this so as to simplify the code by reducing conditionals */
#define DEBUG_HIGHLIGHT

static gboolean debug_text_highlight_G = FALSE;
static gboolean debug_allocate  = FALSE;
static gboolean debug_functions = FALSE;
static gboolean debug_area      = FALSE;
static gboolean debug_table     = FALSE;

static FooCanvasItemClass *parent_class_G;


GType zMapWindowTextItemGetType (void)
{
  static GType text_type = 0;
  
  if (!text_type) 
    {
      /* FIXME: Convert to gobject style.  */
      static const GTypeInfo text_info = 
	{
	  sizeof (zmapWindowTextItemClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_window_text_item_class_init,
	  NULL, 
	  NULL, /* reserved_2 */
	  sizeof(zmapWindowTextItem),
	  0,
	  (GInstanceInitFunc) zmap_window_text_item_init
	};
    
      text_type = g_type_register_static (FOO_TYPE_CANVAS_TEXT,
					  ZMAP_WINDOW_TEXT_ITEM_NAME,
					  &text_info, 0);
    }
  
  return text_type;
}

void zMapWindowTextItemSelect(ZMapWindowTextItem text_item, int start, int end, 
			      gboolean deselect_first, gboolean emit_signal)
{
  ZMapWindowTextItemClass text_class;

  text_class = ZMAP_WINDOW_TEXT_ITEM_GET_CLASS(text_item);

  if(text_class->select)
    (text_class->select)(text_item, start, end, deselect_first, emit_signal);

  return;
}

void zMapWindowTextItemDeselect(ZMapWindowTextItem text_item,
				gboolean emit_signal)
{
  ZMapWindowTextItemClass text_class;

  text_class = ZMAP_WINDOW_TEXT_ITEM_GET_CLASS(text_item);

  if(text_class->deselect)
    (text_class->deselect)(text_item, emit_signal);

  return;
}



static gboolean text_item_text_index2item(FooCanvasItem *item, 
					  int index, 
					  int *index_out,
					  double *item_coords_out)
{
  ZMapWindowTextItem zmap;
  gboolean index_found;

  if(ZMAP_IS_WINDOW_TEXT_ITEM(item) &&
     (zmap = ZMAP_WINDOW_TEXT_ITEM(item)))
    {
      FooCanvasGroup *parent_group;
      ZMapTextItemDrawData draw_data;
      ItemEvent dna_item_event;
      double w, h;
      int row_idx, row, width;
      double x1, x2, y1, y2;

      foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2);
      
      parent_group = FOO_CANVAS_GROUP(item->parent);

      if((dna_item_event = &(zmap->item_event)))
	{
#ifdef RDS_DONT_INCLUDE
	  /* We need to translate between the last time we updated and this time. */
	  g_warning("index=%d, last_ypost=%f, current_ypos=%f",
		    index, dna_item_event->last_update_ypos, parent_group->ypos);
#endif /* RDS_DONT_INCLUDE */
	  index += (dna_item_event->last_update_ypos - parent_group->ypos);
	}

      zmap = zmap;
      draw_data    = &(zmap->update_cache);
      
      if(draw_data->table.truncated)
	width = draw_data->table.untruncated_width;
      else
	width = draw_data->table.width;
      
      row_idx = index % width;
      row     = (index - row_idx) / width;

      if(row_idx == 0)
	{
	  row--;
	  row_idx = draw_data->table.width;
	}

      if(row_idx > draw_data->table.width)
	row_idx = draw_data->table.width;
      
      row_idx--;		/* zero based. */
      
      if(item_coords_out)
	{
	  w = (draw_data->table.ch_width / draw_data->zx);
	  h = ((draw_data->table.ch_height + draw_data->table.spacing) / draw_data->zy);
	  (item_coords_out[0]) = row_idx * w;
	  (item_coords_out[1]) = row     * h;
	  row++;
	  row_idx++;
	  (item_coords_out[2]) = row_idx * w;
	  (item_coords_out[3]) = row     * h;

	  /* keep within the item_bounds (only x are correct ATM)... */
	  
	  if(item_coords_out[0] < x1)
	    {
	      item_coords_out[0] = x1;
	      item_coords_out[2] = x1;
	    }
	  if(item_coords_out[2] > x2)
	    {
	      item_coords_out[0] = x2;
	      item_coords_out[2] = x2;
	    }

	  /* As y coords aren't reliable, we need to long item crop. */
	  foo_canvas_get_scroll_region(item->canvas, &x1, &y1, &x2, &y2);
	  foo_canvas_item_w2i(item, &x1, &y1);
	  foo_canvas_item_w2i(item, &x2, &y2);

	  if(item_coords_out[1] < y1)
	    {
	      item_coords_out[1] = y1;
	      item_coords_out[3] = y1;
	    }

	  if(item_coords_out[3] > y2)
	    {
	      item_coords_out[1] = y2;
	      item_coords_out[3] = y2;
	    }

	  index_found = TRUE;
	}
      if(index_out)
	*index_out = index;
    }
  else
    index_found = FALSE;

  return index_found;
}


/* Class initialization function for the text item */
static void zmap_window_text_item_class_init (ZMapWindowTextItemClass zmap_class)
{
  GObjectClass *gobject_class;
  GtkObjectClass *object_class;
  FooCanvasItemClass *item_class;
  FooCanvasItemClass *parent_class;
  
  gobject_class = (GObjectClass *) zmap_class;
  object_class = (GtkObjectClass *) zmap_class;
  item_class = (FooCanvasItemClass *) zmap_class;
  
  parent_class_G = gtk_type_class(foo_canvas_text_get_type());
  parent_class   = gtk_type_class(foo_canvas_text_get_type());
  
  gobject_class->set_property = zmap_window_text_item_set_property;
  gobject_class->get_property = zmap_window_text_item_get_property;
  
  g_object_class_install_property(gobject_class,
				  PROP_TEXT_ALLOCATE_FUNC,
				  g_param_spec_pointer("allocate-func",
						       "Size Allocate text area.",
						       "User function to allocate the correct table cell dimensions (not-implemented)",
						       G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,
				  PROP_TEXT_FETCH_TEXT_FUNC,
				  g_param_spec_pointer("fetch-text-func",
						       "Get Text Function",
						       "User function to copy text to buffer",
						       G_PARAM_READWRITE));
  
  g_object_class_install_property(gobject_class,
				  PROP_TEXT_CALLBACK_DATA,
				  g_param_spec_pointer("callback-data",
						       "Get Text Function Data",
						       "User function data to use to copy text to buffer",
						       G_PARAM_READWRITE));
  
  g_object_class_install_property(gobject_class,
				  PROP_TEXT_SELECT_COLOR_GDK,
				  g_param_spec_boxed ("select-color-gdk", NULL, NULL,
						      GDK_TYPE_COLOR,
						      G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,
				  PROP_TEXT_REQUESTED_WIDTH,
				  g_param_spec_double("full-width",
						      NULL, NULL,
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
						      G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_TEXT_REQUESTED_HEIGHT,
				  g_param_spec_double("full-height",
						      NULL, NULL,
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
						      G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_WRAP_MODE,
				  g_param_spec_enum ("wrap-mode", NULL, NULL,
						     PANGO_TYPE_WRAP_MODE, PANGO_WRAP_WORD,
						     G_PARAM_READWRITE));
  
  object_class->destroy = zmap_window_text_item_destroy;
  
  item_class->map       = zmap_window_text_item_map;
  item_class->unmap     = zmap_window_text_item_unmap;
  item_class->realize   = zmap_window_text_item_realize;
  item_class->unrealize = zmap_window_text_item_unrealize;
  item_class->draw      = zmap_window_text_item_draw;
  item_class->update    = zmap_window_text_item_update;
  item_class->bounds    = zmap_window_text_item_bounds;

  item_class->point     = zmap_window_text_item_point;

  zmap_class->selected_signal = zmap_window_text_selected_signal;

  zmap_class->select   = zmap_window_text_item_select;
  zmap_class->deselect = zmap_window_text_item_deselect;

  zmap_class->signals[TEXT_ITEM_SELECTED_SIGNAL] = 
    g_signal_new("text-selected",
		 G_TYPE_FROM_CLASS(zmap_class),
		 G_SIGNAL_RUN_LAST,
		 G_STRUCT_OFFSET(zmapWindowTextItemClass, selected_signal),
		 NULL, NULL,
		 zmapWindowTextItemCMarshal_BOOL__DOUBLE_DOUBLE_DOUBLE_DOUBLE_INT_INT,
		 G_TYPE_BOOLEAN, 6, 
		 G_TYPE_DOUBLE, G_TYPE_DOUBLE, 
		 G_TYPE_DOUBLE, G_TYPE_DOUBLE,
		 G_TYPE_INT, G_TYPE_INT);

  return ;
}


static gboolean text_event_handler_cb(GtkWidget *text_widget, GdkEvent *event, gpointer data)
{
  ItemEvent dna_item_event = (ItemEvent)data;
  gboolean handled = FALSE;

  switch(event->type)
    {
    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
      {
	ZMapWindowTextItemClass text_class;
	ZMapWindowTextItem text;
	GdkEventButton *button = (GdkEventButton *)event;
	
	text       = ZMAP_WINDOW_TEXT_ITEM(text_widget);
	text_class = ZMAP_WINDOW_TEXT_ITEM_GET_CLASS(text);

	if (event->type == GDK_BUTTON_RELEASE)
	  {
	    if (dna_item_event->selected_state & TEXT_ITEM_SELECT_EVENT)
	      {
		FooCanvasItem *item;
		int start_index, end_index;
		
		item       = FOO_CANVAS_ITEM(text);

		start_index = text_item_world2text_index(item, 
							 dna_item_event->origin_x,
							 dna_item_event->origin_y);
		
		end_index = text_item_world2text_index(item, 
						       dna_item_event->event_x,
						       dna_item_event->event_y);
		if (start_index > end_index)
		  {
		    dna_item_event->start_index = end_index;
		    dna_item_event->end_index   = start_index;
		  }
		else
		  {
		    dna_item_event->start_index = start_index;
		    dna_item_event->end_index   = end_index;
		  }

		/* Turn on signalling */
		dna_item_event->selected_state |= TEXT_ITEM_SELECT_SIGNAL;

		emit_signal(text, text_class->signals[TEXT_ITEM_SELECTED_SIGNAL], 0);

		/* Turn off event handling */
		dna_item_event->selected_state &= ~(TEXT_ITEM_SELECT_EVENT);

		/* and makethe highlight endure */
		dna_item_event->selected_state |= TEXT_ITEM_SELECT_ENDURE;
		dna_item_event->origin_index = 0;
#ifdef RDS_DONT_INCLUDE
		if(dna_item_event->event_x == dna_item_event->origin_x &&
		   dna_item_event->event_y == dna_item_event->origin_y)
		  {
#endif /* RDS_DONT_INCLUDE */
		    /* If no GDK_MOTION_NOTIFY received... We _only_ do it if that's the case! */
		    foo_canvas_item_request_update((FooCanvasItem *)text);
#ifdef RDS_DONT_INCLUDE
		  }
		else
		  {
		    /* as we're async w.r.t update this needs to happen, but I think there's a 
		     * case for calling the ->update method here, but I guess it would annoy
		     * the canvas' event handling, so we only request update and live with it. */
		    dna_item_event->selected_state &= ~(TEXT_ITEM_SELECT_SIGNAL);
		  }
#endif /* RDS_DONT_INCLUDE */
		handled = TRUE;
              }
          }
        else if (button->button == 1)
          {
	    dna_item_event->origin_index = 0;
	    dna_item_event->selected_state |= TEXT_ITEM_SELECT_EVENT;
	    dna_item_event->index_bounds[3] = 0.0;

	    dna_item_event->origin_x = button->x;
	    dna_item_event->origin_y = button->y;
	    dna_item_event->event_x  = button->x;
	    dna_item_event->event_y  = button->y;

	    if(text_class->deselect)
	      (text_class->deselect)(text, TRUE);

	    handled = TRUE;
	  }
        else if(button->button == 3)
          {
            
          }
      }
      break;
    case GDK_MOTION_NOTIFY:
      {
	ZMapWindowTextItem text = ZMAP_WINDOW_TEXT_ITEM(text_widget);
	GdkEventMotion *motion = (GdkEventMotion *)event ;

        if(dna_item_event->selected_state & TEXT_ITEM_SELECT_EVENT)
          {
	    dna_item_event->event_x  = motion->x;
	    dna_item_event->event_y  = motion->y;

	    foo_canvas_item_request_update((FooCanvasItem *)text);
	    handled = TRUE;
	  }
      }
      break;
    case GDK_LEAVE_NOTIFY:
    case GDK_ENTER_NOTIFY:
      handled = FALSE;
      break;
    default:
      handled = FALSE;
      break;
    }

  return handled;
}

static FooCanvasItem *create_detached_polygon(ZMapWindowTextItem text)
{
  GList *list, *list_end, *new_list_end;
  FooCanvasItem *detached_polygon = NULL, *item;
  FooCanvasGroup *group;

  item = (FooCanvasItem *)text;
  if((group = (FooCanvasGroup *)(item->parent)))
    {
      list     = group->item_list;
      list_end = group->item_list_end;
      
      detached_polygon = foo_canvas_item_new(group,
					     FOO_TYPE_CANVAS_POLYGON,
#ifdef DEBUG_HIGHLIGHT
					     "fill_color_gdk", &(text->select_colour),
#endif /* DEBUG_HIGHLIGHT */
					     NULL);
      
      if((new_list_end = group->item_list_end))
	{
	  /* Item was created, now remove */
	  
	  if(list_end == NULL)
	    {
	      /* highlight item was the first one in the list... unlikely? */
	      g_list_free(group->item_list);
	      group->item_list = group->item_list_end = NULL;
	    }
	  else
	    {
	      group->item_list_end = list_end;
	      list_end = g_list_delete_link(list_end, new_list_end);
	    }
	}

      if(detached_polygon)
	{
	  if (detached_polygon->object.flags & FOO_CANVAS_ITEM_MAPPED)
	    (* FOO_CANVAS_ITEM_GET_CLASS (detached_polygon)->unmap) (detached_polygon);
	  
	  if (detached_polygon->object.flags & FOO_CANVAS_ITEM_REALIZED)
	    (* FOO_CANVAS_ITEM_GET_CLASS (detached_polygon)->unrealize) (detached_polygon);
	}
    }

  return detached_polygon;
}

static void initialise_hightlight(ZMapWindowTextItem text)
{
  text->highlight = create_detached_polygon(text);
    
  g_signal_connect(G_OBJECT(text), "event", G_CALLBACK(text_event_handler_cb), &(text->item_event));

  return ;
}

/* Object initialization function for the text item */
static void zmap_window_text_item_init (ZMapWindowTextItem text)
{
  text->draw_cache.zx = text->update_cache.zx = 0.0;
  text->draw_cache.zy = text->update_cache.zy = 0.0;
  text->draw_cache.x1 = text->update_cache.x1 = 0.0;
  text->draw_cache.y1 = text->update_cache.y1 = 0.0;
  text->draw_cache.x2 = text->update_cache.x2 = 0.0;
  text->draw_cache.y2 = text->update_cache.y2 = 0.0;
  text->draw_cache.wx = text->update_cache.wx = 0.0;
  text->draw_cache.wy = text->update_cache.wy = 0.0;
  
  text->update_cache.table.spacing = 
    text->draw_cache.table.spacing = 0.0;
  text->update_cache.table.ch_width = 
    text->draw_cache.table.ch_width = 0;
  text->update_cache.table.ch_height = 
    text->draw_cache.table.ch_height = 0;
  text->update_cache.table.width = 
    text->draw_cache.table.width = 0;
  text->update_cache.table.height = 
    text->draw_cache.table.height = 0;
  text->update_cache.table.untruncated_width = 
    text->draw_cache.table.untruncated_width = 0;
  text->update_cache.table.truncated = 
    text->draw_cache.table.truncated = FALSE;

  text->flags = (ZOOM_NOTIFY_MASK | 
		 SCROLL_REGION_NOTIFY_MASK | 
		 KEEP_WITHIN_SCROLL_REGION);

#ifdef DEBUG_HIGHLIGHT
  gdk_color_parse("red", &(text->select_colour));
#endif /* DEBUG_HIGHLIGHT */

  return ;
}

/* Destroy handler for the text item */
static void zmap_window_text_item_destroy (GtkObject *object)
{
  ZMapWindowTextItem text;
  
  g_return_if_fail (ZMAP_IS_WINDOW_TEXT_ITEM(object));
  
  text = ZMAP_WINDOW_TEXT_ITEM(object);
  
  /* remember, destroy can be run multiple times! */

  if (GTK_OBJECT_CLASS (parent_class_G)->destroy)
    (* GTK_OBJECT_CLASS (parent_class_G)->destroy) (object);

  return ;
}

/* Set_arg handler for the text item */
static void zmap_window_text_item_set_property (GObject            *object,
						 guint               param_id,
						 const GValue       *value,
						 GParamSpec         *pspec)
{
  FooCanvasItem *item;
  ZMapWindowTextItem text;
  FooCanvasText *parent_text;
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (ZMAP_IS_WINDOW_TEXT_ITEM (object));
  
  text = ZMAP_WINDOW_TEXT_ITEM (object);
  item = FOO_CANVAS_ITEM(object);
  parent_text = FOO_CANVAS_TEXT(object);

  if(!parent_text->layout)
    {
      parent_text->layout = gtk_widget_create_pango_layout(GTK_WIDGET(item->canvas), NULL);
    }
  
  switch (param_id) 
    {
    case PROP_TEXT_ALLOCATE_FUNC:
      text->allocate_func    = g_value_get_pointer(value);
      break;
    case PROP_TEXT_FETCH_TEXT_FUNC:
      text->fetch_text_func  = g_value_get_pointer(value);
      break;
    case PROP_TEXT_CALLBACK_DATA:
      text->callback_data    = g_value_get_pointer(value);
      break;
    case PROP_TEXT_REQUESTED_WIDTH:
      text->requested_width  = g_value_get_double(value);
      break;
    case PROP_TEXT_REQUESTED_HEIGHT:
      text->requested_height = g_value_get_double(value);
      break;
    case PROP_WRAP_MODE:
      {
	int mode = g_value_get_enum(value);

	pango_layout_set_wrap(parent_text->layout, mode);
      }	
      break;
    case PROP_TEXT_SELECT_COLOR_GDK:
      {
	GdkColor *colour;

	if((colour = g_value_get_boxed(value)))
	  text->select_colour = *colour;
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
  
  foo_canvas_item_request_update (item);

  return ;
}

/* Get_arg handler for the text item */
static void zmap_window_text_item_get_property (GObject            *object,
						 guint               param_id,
						 GValue             *value,
						 GParamSpec         *pspec)
{
  ZMapWindowTextItem text;
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (ZMAP_IS_WINDOW_TEXT_ITEM (object));
  
  text = ZMAP_WINDOW_TEXT_ITEM (object);
  
  switch (param_id) 
    {
    case PROP_TEXT_ALLOCATE_FUNC:
      g_value_set_pointer(value, text->allocate_func);
      break;
    case PROP_TEXT_FETCH_TEXT_FUNC:
      g_value_set_pointer(value, text->fetch_text_func);
      break;
    case PROP_TEXT_CALLBACK_DATA:
      g_value_set_pointer(value, text->callback_data);
      break;
    case PROP_TEXT_REQUESTED_WIDTH:
      g_value_set_double(value, text->requested_width);
      break;
    case PROP_TEXT_REQUESTED_HEIGHT:
      g_value_set_double(value, text->requested_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }

  return ;
}

static void update_detached_polygon(FooCanvasItem *highlight, double i2w_dx, double i2w_dy, int flags,
				    ZMapWindowTextItem zmap, ItemEvent item_event, gboolean canvas_changed)
{
  FooCanvasItem *zmap_item;
  gboolean item_needs_update_call = FALSE; /* we always need it.  This is for debugging. */

  if(item_event->selected_state & (TEXT_ITEM_SELECT_EVENT | TEXT_ITEM_SELECT_ENDURE | TEXT_ITEM_SELECT_SIGNAL))
    {
      FooCanvasItem *parent;
      FooCanvasPoints points;
      double coords[16] = {0.0};
      guint item_flags = 0;
      gboolean canvas_in_update = TRUE;
      gboolean parent_need_update = FALSE;
      
      points.coords     = &coords[0];
      points.num_points = 8;
      points.ref_count  = 1;
      
      item_event->points = &points;
      parent = highlight->parent;

      zmap_item = (FooCanvasItem *)zmap;
      
      if(canvas_in_update)		/* actually we might not want to do this. */
	canvas_in_update = highlight->canvas->doing_update;
      
      if(canvas_in_update)
	{
	  highlight->canvas->doing_update = FALSE;
	  item_flags = parent->object.flags;
	  
	  if((parent_need_update = ((item_flags & FOO_CANVAS_ITEM_NEED_UPDATE) == FOO_CANVAS_ITEM_NEED_UPDATE)) == FALSE)
	    parent->object.flags |= FOO_CANVAS_ITEM_NEED_UPDATE;
	}

      if(!(highlight->object.flags & FOO_CANVAS_ITEM_REALIZED))
	(FOO_CANVAS_ITEM_GET_CLASS(highlight)->realize)(highlight);
      
      if(!(highlight->object.flags & FOO_CANVAS_ITEM_MAPPED) && 
	 FOO_CANVAS_ITEM_GET_CLASS(highlight)->map)
	(FOO_CANVAS_ITEM_GET_CLASS(highlight)->map)(highlight);
      
      if((item_event->selected_state & TEXT_ITEM_SELECT_EVENT) ||
	 (item_event->selected_state & TEXT_ITEM_SELECT_SIGNAL))
	{
	  /* calculate the highlight region based on the event coords */
	  foo_canvas_item_show(highlight);
	  if(event_to_char_cell_coords(zmap, item_event))
	    {
	      foo_canvas_item_set(highlight, "points", &points, NULL);
	      item_needs_update_call = TRUE;
	    }

	  /* always turn off the signal flag. */
	  item_event->selected_state &= ~TEXT_ITEM_SELECT_SIGNAL;

	  /* as the update is async w.r.t the emit_signal() call
	   * [event handling] we need the TEXT_ITEM_SELECT_SIGNAL flag
	   * to allow an update in case of none happening between
	   * GDK_BUTTON_PRESS & GDK_BUTTON_RELEASE. This flag
	   * _absolutely_ must be turned off, otherwise we'll 
	   * _always_ be updating... (It's a "force" flag really) */
	}
      else if(canvas_changed && (item_event->selected_state & TEXT_ITEM_SELECT_ENDURE))
	{
	  gboolean first_found, last_found;
	  double start_bounds[4] = {0.0};
	  double end_bounds[4] = {0.0};
	  double item_bounds[4] = {0.0};
	  int new_start_index, new_end_index;
	  
	  item_needs_update_call = TRUE;

	  /* calculate the highlight region based on index coords... */
	  /* Why not always do it this way! It's less reliable. */
	  
	  first_found = text_item_text_index2item(zmap_item, item_event->start_index, 
						  &new_start_index, &start_bounds[0]);
	  last_found  = text_item_text_index2item(zmap_item, item_event->end_index, 
						  &new_end_index, &end_bounds[0]);
	  
	  if(first_found && last_found)
	    {
	      item_event->start_index = new_start_index;
	      item_event->end_index   = new_end_index;
	      foo_canvas_item_get_bounds(zmap_item, 
					 &item_bounds[0], &item_bounds[1],
					 &item_bounds[2], &item_bounds[3]);
	      
	      highlight_coords_from_cell_bounds(&points, start_bounds, end_bounds, 
						item_bounds[0], item_bounds[2]);
	      
	      foo_canvas_item_set(highlight, "points", &points, NULL);
#ifdef RDS_DONT_INCLUDE
	      foo_canvas_item_show(highlight);
#endif /* RDS_DONT_INCLUDE */
	    }
	  else
	    {
	      double zero_coords[16] = {0.0};
	      points.coords = zero_coords;
	      
	      foo_canvas_item_set(highlight, "points", &points, NULL);
	      
	      g_warning("failed to find both indices in current display");
	    }

	}
      else if(!(item_event->selected_state & TEXT_ITEM_SELECT_ENDURE))
	{
#ifdef RDS_DONT_INCLUDE
	  foo_canvas_item_hide(highlight);
#endif /* RDS_DONT_INCLUDE */
	}
      
      if(canvas_in_update)
	{
	  highlight->canvas->doing_update = canvas_in_update;
	  parent->object.flags |= item_flags;
	  if(!parent_need_update)
	    parent->object.flags &= ~FOO_CANVAS_ITEM_NEED_UPDATE;
	}
      
      item_event->last_update_ypos = ((FooCanvasGroup *)zmap_item->parent)->ypos;
    }
  else
    {

    }

  if(!item_needs_update_call)
    {
      g_assert(!item_needs_update_call);
    }


  /* can we crop/resize the highlight here? */
  if(FOO_CANVAS_ITEM_GET_CLASS(highlight)->update)
    (FOO_CANVAS_ITEM_GET_CLASS(highlight)->update)(highlight, i2w_dx, i2w_dy, flags);

  return ;
}

/* Update handler for the text item */
static void zmap_window_text_item_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags)
{
  ZMapWindowTextItem zmap;
  FooCanvasText *foo_text;
  PangoLayout *layout;
  GList *list;
  gboolean canvas_changed = FALSE;

  zmap = ZMAP_WINDOW_TEXT_ITEM(item);
  foo_text = FOO_CANVAS_TEXT(item);

#ifdef __GNUC__
  if(debug_functions)
    printf("%s\n", __PRETTY_FUNCTION__);
#endif /* __GNUC__ */

  canvas_changed = canvas_has_changed(item, &(zmap->update_cache));

  if(canvas_changed && (layout = foo_text->layout))
    {
      ZMapTextItemDrawDataStruct clear = {0};

      /* This simple bit of code speeds everything up ;) */
      pango_layout_set_text(layout, "", 0);

      invoke_allocate_width_height(item);

      /* clear the draw cache... to absolutely guarantee the draw
       * code sees the canvas has changed! */
      zmap->draw_cache = clear;
    }

  if(parent_class_G->update)
    (*parent_class_G->update)(item, i2w_dx, i2w_dy, flags);

  if(zmap->highlight)
    update_detached_polygon(zmap->highlight, i2w_dx, i2w_dy, flags, zmap, 
			    &(zmap->item_event), canvas_changed);

  zmap->item_event.selected_state &= ~TEXT_ITEM_SELECT_SIGNAL;

  if((list = g_list_first(zmap->selections)))
    {
      FooCanvasItem *highlight;
      HighlightItemEvent highlight_event;

      do
	{
	  highlight_event = (HighlightItemEvent)(list->data);
	  highlight = highlight_event->highlight;

	  update_detached_polygon(highlight_event->highlight, i2w_dx, i2w_dy, flags, 
				  zmap, &(highlight_event->user_select), canvas_changed);
	}
      while((list = list->next));
    }

  return ;
}

static void   zmap_window_text_item_map        (FooCanvasItem  *item)
{
  ZMapWindowTextItem text;

  if(parent_class_G->map)
    (* parent_class_G->map)(item);

  text = ZMAP_WINDOW_TEXT_ITEM(item);

  if(!text->highlight)
    {
      initialise_hightlight(text);
    }

  if(text->highlight)
    {
      if(!(text->highlight->object.flags & FOO_CANVAS_ITEM_REALIZED))
	(FOO_CANVAS_ITEM_GET_CLASS(text->highlight)->realize)(text->highlight);

      if(FOO_CANVAS_ITEM_GET_CLASS(text->highlight)->map)
	(FOO_CANVAS_ITEM_GET_CLASS(text->highlight)->map)(text->highlight);
    }

  return ;
}
static void   zmap_window_text_item_unmap      (FooCanvasItem  *item)
{
  ZMapWindowTextItem text;

  if(parent_class_G->map)
    (* parent_class_G->map)(item);

  text = ZMAP_WINDOW_TEXT_ITEM(item);

  if(text->highlight)
    {
      if(!(text->highlight->object.flags & FOO_CANVAS_ITEM_REALIZED))
	(FOO_CANVAS_ITEM_GET_CLASS(text->highlight)->realize)(text->highlight);

      if(FOO_CANVAS_ITEM_GET_CLASS(text->highlight)->unmap)
	(FOO_CANVAS_ITEM_GET_CLASS(text->highlight)->unmap)(text->highlight);
    }

  return ;
}


/* Realize handler for the text item */
static void zmap_window_text_item_realize (FooCanvasItem *item)
{
  ZMapWindowTextItem text;
  FooCanvasText *foo_text;
  PangoLayout *layout;
  PangoLayoutIter *iter = NULL;
  const char *current = NULL;
  int l = 0;

  text = ZMAP_WINDOW_TEXT_ITEM (item);
  foo_text = FOO_CANVAS_TEXT(item);

#ifdef __GNUC__
  if(debug_functions)
    printf("%s\n", __PRETTY_FUNCTION__);
#endif /* __GNUC__ */

  /* First time it's realized  */
  save_origin(item);

  layout = foo_text->layout;

  if((current = pango_layout_get_text(layout)) && (l = strlen(current)) == 0)
    {
      current = NULL;
      pango_layout_set_text(layout, "0", 1);
    }
  
  if((iter = pango_layout_get_iter(layout)))
    {
      pango_layout_iter_get_char_extents(iter, &(text->char_extents));
      
      pango_layout_iter_free(iter);
    }

  if(!current)
    pango_layout_set_text(layout, "", 0);
  
  allocate_buffer_size(text);

  invoke_allocate_width_height(item);

  if (parent_class_G->realize)
    (* parent_class_G->realize) (item);

  return ;
}

/* Unrealize handler for the text item */
static void zmap_window_text_item_unrealize (FooCanvasItem *item)
{
  ZMapWindowTextItem text;
  text = ZMAP_WINDOW_TEXT_ITEM (item);

#ifdef __GNUC__
  if(debug_functions)
    printf("%s\n", __PRETTY_FUNCTION__);
#endif /* __GNUC__ */

  /* make sure it's all zeros */
  if(text && text->buffer)
    memset(text->buffer, 0, text->buffer_size);
  
  if (parent_class_G->unrealize)
    (* parent_class_G->unrealize) (item);



  return ;
}

/* Draw handler for the text item */
static void zmap_window_text_item_draw (FooCanvasItem  *item, 
					GdkDrawable    *drawable,
					GdkEventExpose *expose)
{
  ZMapWindowTextItem zmap;
  FooCanvasText     *text;
  GdkRectangle       rect;
  GList *selections;

  zmap = ZMAP_WINDOW_TEXT_ITEM (item);
  text = FOO_CANVAS_TEXT(item);

#ifdef __GNUC__
  if(debug_functions)
    printf("%s\n", __PRETTY_FUNCTION__);
#endif /* __GNUC__ */

  if(zmap->highlight && zmap->highlight->object.flags & FOO_CANVAS_ITEM_VISIBLE)
    {
      if(FOO_CANVAS_ITEM_GET_CLASS(zmap->highlight)->draw)
	(FOO_CANVAS_ITEM_GET_CLASS(zmap->highlight)->draw)(zmap->highlight, drawable, expose);
    }

  if((selections = g_list_first(zmap->selections)))
    {
      HighlightItemEvent highlight_event;
      do
	{
	  highlight_event = (HighlightItemEvent)(selections->data);
	  if(FOO_CANVAS_ITEM_GET_CLASS(highlight_event->highlight)->draw)
	    (FOO_CANVAS_ITEM_GET_CLASS(highlight_event->highlight)->draw)(highlight_event->highlight, drawable, expose);
	}
      while((selections = selections->next));
    }

  /* from here is identical logic to parent, but calls internal
   * zmap_gdk_draw_layout_to_expose instead of gdk_draw_layout */

  if (text->clip) 
    {
      rect.x = text->clip_cx;
      rect.y = text->clip_cy;
      rect.width  = text->clip_cwidth;
      rect.height = text->clip_cheight;
      
      gdk_gc_set_clip_rectangle (text->gc, &rect);
    }
  
  if (text->stipple)
    foo_canvas_set_stipple_origin (item->canvas, text->gc);
  
  /* this is conditionally calling the fetch text from creating caller */
  invoke_get_text(item);

  /* This is faster than the gdk_draw_layout as it filters based on
   * the intersection of lines within the expose region. */
  zmap_gdk_draw_layout_get_size_within_expose (drawable, text->gc, 
					       text->cx, text->cy, 
					       text->layout, 
					       expose,
					       &(text->max_width),
					       &(text->height));

  if (text->clip)
    gdk_gc_set_clip_rectangle (text->gc, NULL);

  /* always turn off */
  zmap->item_event.selected_state &= ~TEXT_ITEM_SELECT_CANVAS_CHANGED;

  return ;
}

/* Point handler for the text item */
static double zmap_window_text_item_point (FooCanvasItem *item, 
					   double x, double y,
					   int cx, int cy, 
					   FooCanvasItem **actual_item)
{
  double dist = 1.0;

  if(parent_class_G->point)
    dist = (parent_class_G->point)(item, x, y, cx, cy, actual_item);

  return dist;
}
#ifdef RDS_DONT_INCLUDE
static void zmap_window_text_item_translate (FooCanvasItem *item, double dx, double dy)
{
  
  return ;
}
#endif

static void zmap_window_text_item_bounds (FooCanvasItem *item, 
					  double *x1, double *y1, 
					  double *x2, double *y2)
{
  ZMapWindowTextItem zmap = NULL;
  FooCanvasText *text = NULL;

  zmap = ZMAP_WINDOW_TEXT_ITEM(item);
  text = FOO_CANVAS_TEXT(item);

#ifdef __GNUC__
  if(debug_functions)
    printf("%s\n", __PRETTY_FUNCTION__);
#endif /* __GNUC__ */

  //run_update_cycle_loop(item);

  if(parent_class_G->bounds)
    (*parent_class_G->bounds)(item, x1, y1, x2, y2);

#ifdef __GNUC__
  if(debug_area)
    printf("%s: %f %f %f %f\n", __PRETTY_FUNCTION__, *x1, *y1, *x2, *y2);
#endif /* __GNUC__ */

  return ;
}
static gboolean selection_signals_G = TRUE;
static void zmap_window_text_item_select(ZMapWindowTextItem text_item, 
					 int index1, int index2, 
					 gboolean deselect,
					 gboolean signal)
{
  HighlightItemEvent highlight_event;

  if(deselect)
    {
      if(ZMAP_WINDOW_TEXT_ITEM_GET_CLASS(text_item)->deselect)
	(ZMAP_WINDOW_TEXT_ITEM_GET_CLASS(text_item)->deselect)(text_item, signal);
    }

  /* Ok now select... */
  

  /* create the TextItemEvent */
  if((highlight_event = g_new0(HighlightItemEventStruct, 1)))
    {
      text_item->selections = g_list_append(text_item->selections,
					    highlight_event);

      highlight_event->highlight = create_detached_polygon(text_item);

      highlight_event->user_select.start_index = index1;
      highlight_event->user_select.end_index   = index2;
      highlight_event->user_select.selected_state = (TEXT_ITEM_SELECT_ENDURE);

      if(selection_signals_G && signal)
	g_warning("[select]");
    }

  return ;
}

static void zmap_window_text_item_deselect(ZMapWindowTextItem text_item,
					   gboolean signal)
{
  GList *selections;

  if((selections = g_list_first(text_item->selections)))
    {
      HighlightItemEvent highlight_event;
      do
	{
	  highlight_event = (HighlightItemEvent)(selections->data);

	  /* There's no deselect signal at the moment */
	  if(selection_signals_G && signal)
	    g_warning("[deselect]");

	  gtk_object_destroy(GTK_OBJECT(highlight_event->highlight));

	  g_free(highlight_event);
	}
      while((selections = selections->next));

      g_list_free(text_item->selections);
      text_item->selections = NULL;
    }

  return ;
}



static void print_draw_data(ZMapTextItemDrawData draw_data)
{

  printf("  zoom x, y  : %f, %f\n", draw_data->zx, draw_data->zy);
  printf("  world x, y : %f, %f\n", draw_data->wx, draw_data->wy);
  printf("  scr x1,x2,r: %f, %f, %f\n", draw_data->x1, draw_data->x2, draw_data->x2 - draw_data->x1 + 1);
  printf("  scr y1,y2,r: %f, %f, %f\n", draw_data->y1, draw_data->y2, draw_data->y2 - draw_data->y1 + 1);

  return ;
}

static void print_private_data(ZMapWindowTextItem zmap)
{
  if(debug_area || debug_table)
    {
      printf("Private Data\n");
      printf(" buffer size: %d\n",     zmap->buffer_size);
      printf(" req height : %f\n",     zmap->requested_height);
      printf(" req width  : %f\n",     zmap->requested_width);
      printf(" zx, zy     : %f, %f\n", zmap->zx, zmap->zy);
      printf(" char w, h  : %d, %d\n", zmap->char_extents.width, zmap->char_extents.height);
      
      printf("Update draw data\n");
      print_draw_data(&(zmap->update_cache));
      
      printf("Draw draw data\n");
      print_draw_data(&(zmap->draw_cache));
    }

  return ;
}

static void print_calculate_buffer_data(ZMapWindowTextItem zmap,
					ZMapTextItemDrawData   draw_data,
					int cx1, int cy1, 
					int cx2, int cy2)
{
  if(debug_table)
    {
      printf("foo y1,y2,r: %d, %d, %d\n", 
	     cy1, cy2, cy2 - cy1 + 1);
      printf("world range/canvas range = %f\n", 
	     ((draw_data->y2 - draw_data->y1 + 1) / (cy2 - cy1 + 1)));
      printf("world range (txt)/canvas range = %f\n", 
	     (((draw_data->y2 - draw_data->y1 + 1) * draw_data->table.ch_height) / ((cy2 - cy1 + 1))));
    }

  return ;
}

int zMapWindowTextItemCalculateZoomBufferSize(FooCanvasItem   *item,
					      ZMapTextItemDrawData draw_data,
					      int              max_buffer_size)
{
  ZMapWindowTextItem zmap = ZMAP_WINDOW_TEXT_ITEM(item);
  int buffer_size = max_buffer_size;
  int char_per_line, line_count;
  int cx1, cy1, cx2, cy2, cx, cy; /* canvas coords of scroll region and item */
  int table;
  int width, height;
  double world_range, raw_chars_per_line, raw_lines;
  int real_chars_per_line, canvas_range;
  int real_lines, real_lines_space;

  if(draw_data->y1 < 0.0)
    {
      double t = 0.0 - draw_data->y1;
      draw_data->y1  = 0.0;
      draw_data->y2 -= t;
    }
  
  foo_canvas_w2c(item->canvas, draw_data->x1, draw_data->y1, &cx1, &cy1);
  foo_canvas_w2c(item->canvas, draw_data->x2, draw_data->y2, &cx2, &cy2);
  foo_canvas_w2c(item->canvas, draw_data->wx, draw_data->wy, &cx,  &cy);

  /* We need to know the extents of a character */
  width  = draw_data->table.ch_width;
  height = draw_data->table.ch_height;

  /* possibly print out some debugging info */
  print_calculate_buffer_data(zmap, draw_data, cx1, cy1, cx2, cy2);
  print_private_data(zmap);

  /* world & canvas range to work out rough chars per line */
  world_range        = (draw_data->y2 - draw_data->y1 + 1);
  canvas_range       = (cy2 - cy1 + 1);
  raw_chars_per_line = ((world_range * height) / canvas_range);
  
  if((double)(real_chars_per_line = (int)raw_chars_per_line) < raw_chars_per_line)
    real_chars_per_line++;
  
  raw_lines = world_range / real_chars_per_line;
  
  if((double)(real_lines = (int)raw_lines) < raw_lines)
    real_lines++;
  
  real_lines_space = real_lines * height;
  
  if(real_lines_space > canvas_range)
    {
      real_lines--;
      real_lines_space = real_lines * height;
    }
  
  if(real_lines_space < canvas_range)
    {
      double spacing_dbl = canvas_range;
      double delta_factor = 0.15;
      int spacing;
      spacing_dbl -= (real_lines * height);
      spacing_dbl /= real_lines;
      
      /* need a fudge factor here! We want to round up if we're
       * within delta factor of next integer */
      
      if(((double)(spacing = (int)spacing_dbl) < spacing_dbl) &&
	 ((spacing_dbl + delta_factor) > (double)(spacing + 1)))
	spacing_dbl = (double)(spacing + 1);
      
      draw_data->table.spacing = (int)(spacing_dbl * PANGO_SCALE);
      draw_data->table.spacing = spacing_dbl;
    }
  
  line_count = real_lines;
  
  char_per_line = (int)(zmap->requested_width);

  draw_data->table.untruncated_width = real_chars_per_line;

  if(real_chars_per_line <= char_per_line)
    {
      char_per_line = real_chars_per_line;
      draw_data->table.truncated = FALSE;
    }
  else
    {
      draw_data->table.truncated = TRUE;
    }

  draw_data->table.width  = char_per_line;
  draw_data->table.height = line_count;

  table = char_per_line * line_count;

  buffer_size = MIN(buffer_size, table);

  return buffer_size;
}

static void allocate_buffer_size(ZMapWindowTextItem zmap)
{
  if(zmap->buffer_size == 0)
    {
      FooCanvasText *parent_text = FOO_CANVAS_TEXT(zmap);
      int char_per_line, line_count;
      int pixel_size_x, pixel_size_y;
      int spacing, max_canvas;

      /* work out how big the buffer should be. */
      pango_layout_get_pixel_size(parent_text->layout, 
				  &pixel_size_x, &pixel_size_y);
      spacing = pango_layout_get_spacing(parent_text->layout);

      char_per_line = pixel_size_y + spacing;
      line_count    = 1 << 15;	/* 32768 Maximum canvas size ever */
      max_canvas    = 1 << 15;

      /* This is on the side of caution, but possibly by a factor of 10. */
      zmap->buffer_size = char_per_line * line_count;

      /* The most that can really be displayed is the requested width
       * when the zoom allows drawing this at exactly at the height of
       * the text. The canvas will never be bigger than 32768 so that
       * means 32768 / pixel_size_y * requested_width 
       */
      zmap->buffer_size = max_canvas / char_per_line * zmap->requested_width;

      zmap->buffer = g_new0(char, zmap->buffer_size + 1);

#ifdef __GNUC__
      if(debug_allocate)
	printf("%s: allocated %d.\n", __PRETTY_FUNCTION__, zmap->buffer_size);
#endif /* __GNUC__ */
    }

  return ;
}

static void invoke_allocate_width_height(FooCanvasItem *item)
{
  ZMapWindowTextItem zmap;
  ZMapTextItemDrawData draw_data;
  ZMapTextItemDrawDataStruct draw_data_stack = {};
  FooCanvasText *text;
  int spacing, width, height, actual_size;

  zmap = ZMAP_WINDOW_TEXT_ITEM(item);
  text = FOO_CANVAS_TEXT(item);

  draw_data = &(zmap->update_cache);

  draw_data_stack = *draw_data; /* struct copy! */
  
  /* We need to know the extents of a character */
  draw_data_stack.table.ch_width  = PANGO_PIXELS(zmap->char_extents.width);
  draw_data_stack.table.ch_height = PANGO_PIXELS(zmap->char_extents.height);

  if(zmap->allocate_func)
    actual_size = (zmap->allocate_func)(item, &draw_data_stack, 
					(int)(zmap->requested_width),
					zmap->buffer_size,
					zmap->callback_data);
  else
    actual_size = zMapWindowTextItemCalculateZoomBufferSize(item,
							    &draw_data_stack,
							    zmap->buffer_size);

  /* The only part we allow to be copied back... */
  draw_data->table = draw_data_stack.table;	/* struct copy */

  if(actual_size > 0)
    {
      double ztmp;
      if(actual_size != (draw_data->table.width * draw_data->table.height))
	g_warning("Allocated size of %d does not match table allocation of %d x %d.",
		  actual_size, draw_data->table.width, draw_data->table.height);
      
      if(zmap->buffer_size <= (draw_data->table.width * draw_data->table.height))
	{
	  g_warning("Allocated size of %d is _too_ big. Buffer is only %d long!", 
		    actual_size, zmap->buffer_size);

	  draw_data->table.height = zmap->buffer_size / draw_data->table.width;
	  actual_size = draw_data->table.width * draw_data->table.height;
	  g_warning("Reducing table size to %d by reducing rows to %d. Please fix %p.", 
		    actual_size, draw_data->table.height, zmap->allocate_func);	  
	}
      
      if(debug_table)
	printf("spacing %f = %f\n", 
	       draw_data_stack.table.spacing, 
	       draw_data_stack.table.spacing * PANGO_SCALE);
      
      spacing = (int)(draw_data_stack.table.spacing   * PANGO_SCALE);
      width   = (int)(draw_data_stack.table.ch_width  * 
		      draw_data_stack.table.width     * PANGO_SCALE);
      
      /* Need to add the spacing otherwise we lose the last bits as the update/bounds
       * calculation which group_draw filters on will not draw the extra bit between
       * (height * rows) and ((height + spacing) * rows).
       */
      ztmp    = (draw_data_stack.zy > 1.0 ? draw_data_stack.zy : 1.0);
	
      height  = (int)((draw_data_stack.table.ch_height + 
		       draw_data_stack.table.spacing) * ztmp *
		      draw_data_stack.table.height    * PANGO_SCALE);
      
      
      /* I'm seriously unsure about the width & height calculations here.
       * ( mainly the height one as zoom is always fixed in x for us ;) )
       * Without the conditional (zoom factor @ high factors) text doesn't get drawn 
       * below a certain point, but including the zoom factor when < 1.0 has the same
       * effect @ lower factors.  I haven't investigated enough to draw a good conclusion
       * and there doesn't seem to be any affect on speed. So it's been left in!
       *
       * Thoughts are it could be to do with scroll offsets or rounding or some
       * interaction with the floating group.
       * RDS
       */

      pango_layout_set_spacing(text->layout, spacing);
      
      pango_layout_set_width(text->layout, width);

      width = PANGO_PIXELS(width);
      height= PANGO_PIXELS(height);
      text->max_width = width;
      text->height    = height;
    }

  return ;
}

static gboolean invoke_get_text(FooCanvasItem *item)
{
  FooCanvasText *parent_text;
  ZMapWindowTextItem zmap;
  gboolean got_text = FALSE;

  parent_text  = FOO_CANVAS_TEXT(item);
  zmap         = ZMAP_WINDOW_TEXT_ITEM(item);

  /* has the canvas changed according to the last time we drew */
  if(zmap->fetch_text_func && 
     zmap->item_event.selected_state & TEXT_ITEM_SELECT_CANVAS_CHANGED)
    {
      ZMapTextItemDrawData draw_data;
      ZMapTextItemDrawDataStruct draw_data_stack = {};
      int actual_size = 0;
#ifdef RDS_DONT_INCLUDE
      sync_data(&(zmap->update_cache),
		&(zmap->draw_cache));
#endif
      draw_data       = &(zmap->update_cache);
      draw_data_stack = *draw_data; /* struct copy! */
      
      /* item coords. */
      draw_data_stack.wx = parent_text->x;
      draw_data_stack.wy = parent_text->y;
      
      /* translate to world... */
      foo_canvas_item_i2w(item, 
			  &(draw_data_stack.wx),
			  &(draw_data_stack.wy));

      /* try and get the text from the user specified callback. */
      actual_size = (zmap->fetch_text_func)(item, 
					    &draw_data_stack,
					    zmap->buffer, 
					    zmap->buffer_size,
					    zmap->callback_data);
      
      draw_data->table = draw_data_stack.table;	/* struct copy */
      
      if(actual_size > 0 && zmap->buffer[0] != '\0')
	{
	  /* If actual size is too big Pango will be slower than it needs to be. */
	  if(actual_size > (draw_data->table.width * draw_data->table.height))
	    g_warning("Returned size of %d is _too_ big. Text table is only %d.", 
		      actual_size, (draw_data->table.width * draw_data->table.height));
	  if(actual_size > zmap->buffer_size)
	    g_warning("Returned size of %d is _too_ big. Buffer is only %d long!", 
		      actual_size, zmap->buffer_size);

	  /* So that foo_canvas_zmap_text_get_property() returns text. */
	  parent_text->text = zmap->buffer;
	  /* Set the text in the pango layout */
	  pango_layout_set_text(parent_text->layout, zmap->buffer, actual_size);

	  foo_canvas_item_request_update(item);
 	}
    }

  if(parent_text->text)
    got_text = TRUE;

  return got_text;
}

static gboolean canvas_has_changed(FooCanvasItem   *item, 
				   ZMapTextItemDrawData draw_data)
{
  FooCanvasText *parent_text;
  ZMapWindowTextItem zmap;
  gboolean canvas_changed = FALSE;
  int no_change = 0;

  zmap         = ZMAP_WINDOW_TEXT_ITEM(item);
  parent_text  = FOO_CANVAS_TEXT(item);

  if((zmap->flags & ZOOM_NOTIFY_MASK))
    {
      if(draw_data->zy == item->canvas->pixels_per_unit_y &&
	 draw_data->zx == item->canvas->pixels_per_unit_x)
	{
	  no_change |= ZOOM_NOTIFY_MASK;
	}
      else
	{
	  draw_data->zx = item->canvas->pixels_per_unit_x;
	  draw_data->zy = item->canvas->pixels_per_unit_y;
	}
    }

  if((zmap->flags & SCROLL_REGION_NOTIFY_MASK))
    {
      double sx1, sy1, sx2, sy2;

      foo_canvas_get_scroll_region(item->canvas, &sx1, &sy1, &sx2, &sy2);

      if((draw_data->x1 != sx1) ||
	 (draw_data->y1 != sy1) ||
	 (draw_data->x2 != sx2) ||
	 (draw_data->y2 != sy2))
	{
	  draw_data->x1 = sx1;
	  draw_data->y1 = sy1;
	  draw_data->x2 = sx2;
	  draw_data->y2 = sy2;
	}
      else
	no_change |= SCROLL_REGION_NOTIFY_MASK;
    }

  /* if no change in zoom and scroll region */
  if((no_change & (ZOOM_NOTIFY_MASK)) && 
     (no_change & (SCROLL_REGION_NOTIFY_MASK)))
    canvas_changed = FALSE;
  else
    {
      canvas_changed = TRUE;
      zmap->item_event.selected_state |= TEXT_ITEM_SELECT_CANVAS_CHANGED;
    }

  return canvas_changed;
}


static void save_origin(FooCanvasItem *item)
{
  ZMapWindowTextItem text;
  FooCanvasText *parent;

  text   = ZMAP_WINDOW_TEXT_ITEM (item);
  parent = FOO_CANVAS_TEXT(item);

  text->update_cache.wx = parent->x;
  text->update_cache.wy = parent->y;

  foo_canvas_get_scroll_region(item->canvas, 
			       &(text->update_cache.x1),
			       &(text->update_cache.y1),
			       &(text->update_cache.x2),
			       &(text->update_cache.y2));

  sync_data(&(text->update_cache),
	    &(text->draw_cache));

  return ;
}

static void sync_data(ZMapTextItemDrawData from, ZMapTextItemDrawData to)
{
  /* Straight forward copy... */
  to->wx = from->wx;
  to->wy = from->wy;

  to->x1 = from->x1;
  to->y1 = from->y1;
  to->x2 = from->x2;
  to->y2 = from->y2;

  to->table = from->table;	/* struct copy */

  return ;
}

/* ******** faster gdk drawing code. ******** */

/* direct copy of get_renderer from gdkpango.c */
static PangoRenderer *zmap_get_renderer(GdkDrawable *drawable,
					GdkGC       *gc,
					GdkColor    *foreground,
					GdkColor    *background)
{
  GdkScreen *screen = gdk_drawable_get_screen (drawable);
  PangoRenderer *renderer = gdk_pango_renderer_get_default (screen);
  GdkPangoRenderer *gdk_renderer = GDK_PANGO_RENDERER (renderer);
 
  gdk_pango_renderer_set_drawable (gdk_renderer, drawable);
  gdk_pango_renderer_set_gc (gdk_renderer, gc);  
 
  gdk_pango_renderer_set_override_color (gdk_renderer,
					 PANGO_RENDER_PART_FOREGROUND,
					 foreground);
  gdk_pango_renderer_set_override_color (gdk_renderer,
					 PANGO_RENDER_PART_UNDERLINE,
					 foreground);
  gdk_pango_renderer_set_override_color (gdk_renderer,
					 PANGO_RENDER_PART_STRIKETHROUGH,
					 foreground);
 
  gdk_pango_renderer_set_override_color (gdk_renderer,
					 PANGO_RENDER_PART_BACKGROUND,
					 background);
 
  pango_renderer_activate (renderer);
 
  return renderer;
}

/* direct copy of release_renderer from gdkpango.c */
static void zmap_release_renderer (PangoRenderer *renderer)
{
  GdkPangoRenderer *gdk_renderer = GDK_PANGO_RENDERER (renderer);
   
  pango_renderer_deactivate (renderer);
   
  gdk_pango_renderer_set_override_color (gdk_renderer,
					 PANGO_RENDER_PART_FOREGROUND,
					 NULL);
  gdk_pango_renderer_set_override_color (gdk_renderer,
					 PANGO_RENDER_PART_UNDERLINE,
					 NULL);
  gdk_pango_renderer_set_override_color (gdk_renderer,
					 PANGO_RENDER_PART_STRIKETHROUGH,
					 NULL);
  gdk_pango_renderer_set_override_color (gdk_renderer,
					 PANGO_RENDER_PART_BACKGROUND,
					 NULL);
   
  gdk_pango_renderer_set_drawable (gdk_renderer, NULL);
  gdk_pango_renderer_set_gc (gdk_renderer, NULL);
  
  return ;
}

/* based on pango_renderer_draw_layout, but additionally filters based
 * on the expose region */
static void zmap_pango_renderer_draw_layout_get_size (PangoRenderer    *renderer,
						      PangoLayout      *layout,
						      int               x,
						      int               y,
						      GdkEventExpose   *expose,
						      GdkRectangle     *size_in_out)
{
  PangoLayoutIter *iter;
  g_return_if_fail (PANGO_IS_RENDERER (renderer));
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  /* some missing code here that deals with matrices, but as none is
   * used for the canvas, its removed. */
   
  pango_renderer_activate (renderer);
 
  iter = pango_layout_get_iter (layout);

   do
     {
       GdkRectangle     line_rect;
       GdkOverlapType   overlap;
       PangoRectangle   logical_rect;
       PangoLayoutLine *line;
       int              baseline;
       
       line = pango_layout_iter_get_line (iter);
       
       pango_layout_iter_get_line_extents (iter, NULL, &logical_rect);
       baseline = pango_layout_iter_get_baseline (iter);
       
       line_rect.x      = PANGO_PIXELS(x + logical_rect.x);
       line_rect.y      = PANGO_PIXELS(y + logical_rect.y);
       line_rect.width  = PANGO_PIXELS(logical_rect.width);
       line_rect.height = PANGO_PIXELS(logical_rect.height);

       /* additionally in this code we want to get the size of the text. */
       if(size_in_out)
	 gdk_rectangle_union(&line_rect, size_in_out, size_in_out);

       if((overlap = gdk_region_rect_in(expose->region, &line_rect)) != GDK_OVERLAP_RECTANGLE_OUT)
	 {
	   pango_renderer_draw_layout_line (renderer,
					    line,
					    x + logical_rect.x,
					    y + baseline);
	 }
     }
   while (pango_layout_iter_next_line (iter));
 
   pango_layout_iter_free (iter);
   
   pango_renderer_deactivate (renderer);

   return ;
}

/* similar to gdk_draw_layout/gdk_draw_layout_with_colours, but calls
 * the functions above. */
static void zmap_gdk_draw_layout_get_size_within_expose(GdkDrawable    *drawable,
							GdkGC          *gc,
							int             x,
							int             y,
							PangoLayout    *layout,
							GdkEventExpose *expose,
							int            *width_max_in_out,
							int            *height_max_in_out)
{
  GdkRectangle size = {0}, *size_ptr = NULL;
  PangoRenderer *renderer;
  GdkColor *foreground = NULL;
  GdkColor *background = NULL;

  /* gdk_draw_layout just calls gdk_draw_layout_with_colours, which is
   * the source of this function. */

  /* get_renderer */
  renderer = zmap_get_renderer (drawable, gc, foreground, background);

  if(width_max_in_out && height_max_in_out)
    {
      size.x      = x;
      size.y      = y;
      size.width  = *width_max_in_out;
      size.height = *height_max_in_out;
      size_ptr = &size;
    }

  /* draw_lines of text */
  zmap_pango_renderer_draw_layout_get_size(renderer, layout, 
					   x * PANGO_SCALE, 
					   y * PANGO_SCALE, 
					   expose,
					   size_ptr);
   
  if(size_ptr)
    {
      *width_max_in_out  = size_ptr->width;
      *height_max_in_out = size_ptr->height;
    }

  /* release renderer */
  zmap_release_renderer (renderer);

  return ;
}


/* This function translates the bounds of the first and last cells
 * into the bounds for a polygon to highlight both cells and all those
 * in between according to logical text flow. */
/* e.g. a polygon like
 *           \/first cell
 *           ___________________
 * __________|AGCGGAGTGAGCTACGT|
 * |ACTACGACGGACAGCGAGCAGCGATTT|
 * |CGTTGCATTATATCCG__ACGATCG__|
 * |__CGATCGATCGTA__|
 *                 ^last cell
 * 
 */
static void highlight_coords_from_cell_bounds(FooCanvasPoints *overlay_points,
					      double          *first, 
					      double          *last, 
					      double           minx, 
					      double           maxx)
{
  if(first[ITEMTEXT_CHAR_BOUND_TL_Y] > last[ITEMTEXT_CHAR_BOUND_TL_Y])
    {
      double *tmp;
      tmp   = last;
      last  = first;
      first = tmp;
    }

  /* We use the first and last cell coords to 
   * build the coords for the polygon.  We
   * also set defaults for the 4 widest x coords
   * as we can't know from the cell position how 
   * wide the column is.  The exception to this
   * is dealt with below, for the case when we're 
   * only overlaying across part or all of one 
   * row, where we don't want to extend to the 
   * edge of the column. */
              
  overlay_points->coords[0]    = 
    overlay_points->coords[14] = first[ITEMTEXT_CHAR_BOUND_TL_X];
  overlay_points->coords[1]    = 
    overlay_points->coords[3]  = first[ITEMTEXT_CHAR_BOUND_TL_Y];
  overlay_points->coords[2]    =
    overlay_points->coords[4]  = maxx;
  overlay_points->coords[5]    = 
    overlay_points->coords[7]  = last[ITEMTEXT_CHAR_BOUND_TL_Y];
  overlay_points->coords[6]    = 
    overlay_points->coords[8]  = last[ITEMTEXT_CHAR_BOUND_BR_X];
  overlay_points->coords[9]    =
    overlay_points->coords[11] = last[ITEMTEXT_CHAR_BOUND_BR_Y];
  overlay_points->coords[10]   = 
    overlay_points->coords[12] = minx;
  overlay_points->coords[13]   =
    overlay_points->coords[15] = first[ITEMTEXT_CHAR_BOUND_BR_Y];
  
  /* Do some fixing of the default values if we're only on one row */
  if(first[ITEMTEXT_CHAR_BOUND_TL_Y] == last[ITEMTEXT_CHAR_BOUND_TL_Y])
    overlay_points->coords[2]   =
      overlay_points->coords[4] = last[ITEMTEXT_CHAR_BOUND_BR_X];
  
  if(first[ITEMTEXT_CHAR_BOUND_BR_Y] == last[ITEMTEXT_CHAR_BOUND_BR_Y])
    overlay_points->coords[10]   =
      overlay_points->coords[12] = first[ITEMTEXT_CHAR_BOUND_TL_X];

  return ;
}

static void text_item_pango2item(FooCanvas *canvas, int px, int py, double *ix, double *iy)
{
  double zoom_x, zoom_y;

  g_return_if_fail (FOO_IS_CANVAS (canvas));
  
  zoom_x = canvas->pixels_per_unit_x;
  zoom_y = canvas->pixels_per_unit_y;
  
  if (ix)
    *ix = ((((double)px) / PANGO_SCALE) / zoom_x);
  if (iy)
    *iy = ((((double)py) / PANGO_SCALE) / zoom_y);

  return ;
}

static int zmap_pango_layout_iter_skip_lines(PangoLayoutIter *iterator,
					     int              line_count)
{
  int lines_skipped = 0;

  if(line_count > 0)
    {
      for(lines_skipped = 0; 
	  lines_skipped < line_count && (pango_layout_iter_next_line(iterator));
	  lines_skipped++){ /* nothing else to do */ }
    }
  
  return lines_skipped;
}


static gboolean pick_line_get_bounds(ZMapWindowTextItem zmap_text,
				     ItemEvent       dna_item_event,
				     PangoLayoutIter   *iter,
				     PangoLayoutLine   *line,
				     PangoRectangle    *logical_rect,
				     int                iter_line_index,
				     double             i2w_dx,
				     double             i2w_dy)
{
  FooCanvasItem *item;
  double ewx, ewy;		/* event x,y in world space (button or motion) */
  double x1, y1, x2, y2;	/* world coords of line */
  gboolean finished = FALSE;
  gboolean valid_x;

  item = (FooCanvasItem *)zmap_text;

  ewx = dna_item_event->event_x;
  ewy = dna_item_event->event_y;
  
  text_item_pango2item(item->canvas, logical_rect->x, logical_rect->y, &x1, &y1);
  text_item_pango2item(item->canvas, 
			logical_rect->x + logical_rect->width, 
			logical_rect->y + logical_rect->height, 
			&x2, &y2);
  
  /* item to world */
  x1 += i2w_dx;
  x2 += i2w_dx;
  y1 += i2w_dy;
  y2 += i2w_dy;
  
  
  if(ewy > y2 + 0.5)
    {
      valid_x = FALSE;
      if(debug_text_highlight_G)
	printf("%f should be next row (higher) %f / %d = %f too much (%f)\n", 
	       ewy, ewy - y2, iter_line_index, ((ewy - y2)/ iter_line_index),
	       (((ewy - y2)/ iter_line_index) * 1024 * item->canvas->pixels_per_unit_y));
    }
  else if(ewy < y1 - 0.5)
    {
      valid_x  = FALSE;
      finished = TRUE;
      if(debug_text_highlight_G)
	printf("%f should be previous row (lower) %f / %d = %f too little (%f)\n",
	       ewy, y1 - ewy, iter_line_index, ((y1 - ewy) / iter_line_index),
	       (((y1 - ewy)/ iter_line_index) * 1024 * item->canvas->pixels_per_unit_y));
    }
  else
    {
      double this_bounds[ITEMTEXT_CHAR_BOUND_COUNT];
      int index;
      
      if((ewx < x1) || (ewx > x2))
	{
	  PangoRectangle char_logical_rect;
	  double chx1, chx2;
	  int px1, px2;
	  pango_layout_iter_get_char_extents(iter, &char_logical_rect);
	  
	  if(ewx < x1)
	    {
	      index = line->start_index;
	      px1   = char_logical_rect.x;
	      px2   = char_logical_rect.x + char_logical_rect.width;
	    }
	  else
	    {
	      index = line->start_index + line->length - 1;
	      px1   = char_logical_rect.x + ((line->length - 1) * char_logical_rect.width);
	      px2   = char_logical_rect.x + ((line->length    ) * char_logical_rect.width);
	    }
	  
	  text_item_pango2item(item->canvas, px1, 1234, &chx1, NULL);
	  text_item_pango2item(item->canvas, px2, 5678, &chx2, NULL);
	  
	  this_bounds[0] = chx1;
	  this_bounds[2] = chx2;
	  
	  valid_x = TRUE;
	}
      else
	{
	  PangoRectangle char_logical_rect;
	  double chx1, chx2, iwidth;
	  int px1, px2;
	  
	  pango_layout_iter_get_char_extents(iter, &char_logical_rect);
	  
	  text_item_pango2item(item->canvas, char_logical_rect.width, 0, &iwidth, NULL);
	  
	  index = (int)((ewx - x1) / iwidth);
	  
	  px1   = char_logical_rect.x + ((index    ) * char_logical_rect.width);
	  px2   = char_logical_rect.x + ((index + 1) * char_logical_rect.width);
	  
	  text_item_pango2item(item->canvas, px1, 1234, &chx1, NULL);
	  text_item_pango2item(item->canvas, px2, 5678, &chx2, NULL);
	  
	  this_bounds[0] = chx1;
	  this_bounds[2] = chx2;
	  
	  index  += line->start_index;
	  valid_x = TRUE;
	}
      
      this_bounds[1] = y1 - i2w_dy;
      this_bounds[3] = y2 - i2w_dy;
      
      if(valid_x)
	{
	  int index1, index2;
	  double minx, maxx;
	  double *bounds1, *bounds2;
	  
	  if(dna_item_event->origin_index == 0 && dna_item_event->index_bounds[3] == 0.0)
	    {
	      dna_item_event->origin_index = index;
	      memcpy(&(dna_item_event->index_bounds[0]), &this_bounds[0], sizeof(this_bounds));
	    }
	  
	  if(dna_item_event->origin_index < index)
	    {
	      index1  = dna_item_event->origin_index;
	      index2  = index;
	      bounds1 = &(dna_item_event->index_bounds[0]);
	      bounds2 = &this_bounds[0];
	    }
	  else
	    {
	      index2  = dna_item_event->origin_index;
	      index1  = index;
	      bounds1 = &this_bounds[0];
	      bounds2 = &(dna_item_event->index_bounds[0]);
	    }
	  
	  minx = x1 - i2w_dx;
	  maxx = x2 - i2w_dx;
	  
	  highlight_coords_from_cell_bounds(dna_item_event->points, 
					    bounds1, bounds2, 
					    minx, maxx);
	  dna_item_event->result = TRUE;
	}
      finished = dna_item_event->result;
      
    }

  return finished;
}

static gboolean event_to_char_cell_coords(ZMapWindowTextItem zmap_text,
					  ItemEvent       dna_item_event)
{
  PangoLayoutIter *iterator;
  FooCanvasText *text;
  FooCanvasItem *item;
  double ewx, ewy;
  double eix, eiy;
  gboolean set = FALSE;
  
  dna_item_event->result = set;
  
  text = FOO_CANVAS_TEXT(zmap_text);
  item = FOO_CANVAS_ITEM(zmap_text);
  
  /* itemise world event coords ... */
  eix = ewx = dna_item_event->event_x; /* copy from event */
  eiy = ewy = dna_item_event->event_y; /* copy from event */
  
  /* ... and itemise */
  foo_canvas_item_w2i(item, &eix, &eiy);
  /* for calculating i2w_dx, i2w_dy */
  
  if((iterator = pango_layout_get_iter(text->layout)))
    {
      PangoLayoutLine *line;
      PangoRectangle   logical_rect;
      double x1, y1, x2, y2;
      double i2w_dx, i2w_dy;
      int current_line = 0, ecx, ecy, cx, cy;
      
      i2w_dx = ewx - eix;
      i2w_dy = ewy - eiy;
      
      line = pango_layout_iter_get_line(iterator);
      
      /* Get the first line... From this we'll work out which actual line to get. */
      pango_layout_iter_get_line_extents(iterator, NULL, &logical_rect);
      
      foo_canvas_w2c(item->canvas, ewx, ewy, &ecx, &ecy);
      foo_canvas_w2c(item->canvas, i2w_dx, i2w_dy, &cx, &cy);
      
      text_item_pango2item(item->canvas, logical_rect.x, logical_rect.y, &x1, &y1);
      text_item_pango2item(item->canvas, 
			   logical_rect.x + logical_rect.width, 
			   logical_rect.y + logical_rect.height, 
			   &x2, &y2);
      
      x1 += i2w_dx;
      x2 += i2w_dx;
      y1 += i2w_dy;
      y2 += i2w_dy;
      
      /* Event y > current line?. check not already on last line! */
      if(ewy > y2 && !pango_layout_iter_at_last_line(iterator))
	{
	  int spacing = pango_layout_get_spacing(text->layout);
	  double height_d;
	  double nominator, denominator;
	  
	  text_item_pango2item(item->canvas, 
			       0, (logical_rect.height + spacing), 
			       NULL, &height_d);
	  
	  nominator    = ewy - i2w_dy;
	  denominator  = height_d;
	  
	  current_line = (int)(nominator / denominator);
	  
	  if(debug_text_highlight_G)
	    printf("I think %f / %f = a current line of %d instead of %f\n", 
		   nominator, 
		   denominator,
		   current_line,
		   (nominator / denominator));
	  
	  current_line = zmap_pango_layout_iter_skip_lines(iterator, current_line);
	  
	  line = pango_layout_get_line(text->layout, current_line);
	  /* found the correct line, update logical extents */
	  pango_layout_iter_get_line_extents (iterator, NULL, &logical_rect);
	}
      else
	{
	  /* This line is the correct line!  */
	}
      

      /* from logical rect */
      pick_line_get_bounds(zmap_text, dna_item_event,
			   iterator, line, 
			   &logical_rect, current_line,
			   i2w_dx, i2w_dy);
      
      if(debug_text_highlight_G && !(dna_item_event->result))
	printf("miss (rounding?)\n");
      
      pango_layout_iter_free(iterator);
    }
  
  set = dna_item_event->result;
  
  if(debug_text_highlight_G && !set)
    printf("not set miss\n");

  return set;
}

static int text_item_world2text_index(FooCanvasItem *item, double x, double y)
{
  int index = -1;

  foo_canvas_item_w2i(item, &x, &y);

  index = text_item_item2text_index(item, x, y);

  return index;
}

static int text_item_item2text_index(FooCanvasItem *item, double x, double y)
{
  ZMapWindowTextItem zmap;
  int index = -1;

  if(ZMAP_IS_WINDOW_TEXT_ITEM(item) && 
     (zmap = ZMAP_WINDOW_TEXT_ITEM(item)))
    {
      double x1, x2, y1, y2;
      gboolean min = FALSE, max = FALSE;
      foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2);
      
      min = (x < x1);
      max = (x > x2);

      if(y < y1)      { y = y1 + 0.5; }
      else if(y > y2) { y = y2 - 0.5; }
	
      if(!(y < y1 || y > y2))
	{
	  ZMapTextItemDrawData draw_data;
	  double ix, iy, w, h;
	  int row_idx, row, width;

	  ix = x - x1;
	  iy = y - y1;

	  zmap = zmap;
	  draw_data = &(zmap->update_cache);

	  if(draw_data->table.truncated)
	    width = draw_data->table.untruncated_width;
	  else
	    width = draw_data->table.width;

	  w = (draw_data->table.ch_width / draw_data->zx);
	  h = ((draw_data->table.ch_height + draw_data->table.spacing) / draw_data->zy);

	  row_idx = (int)(ix / w);
	  row     = (int)(iy / h);

	  if(min){ row_idx = 0; }
	  if(max){ row_idx = width - 1; }

	  index   = row * width + row_idx;
	}
      else
	{
	  g_warning("Out of range");
	}
    }
  else
    g_warning("Item _must_ be a ZMAP_WINDOW_TEXT_ITEM item.");

  return index;
}

static gboolean zmap_window_text_selected_signal(ZMapWindowTextItem text_item,
						 double x1, double y1,
						 double x2, double y2,
						 int start, int end)
{
#ifdef RDS_DONT_INCLUDE
  g_warning("text was selected @ %d to %d", start, end);
#endif /* RDS_DONT_INCLUDE */
  return FALSE;			/* carry on */
}

static gboolean emit_signal(ZMapWindowTextItem text_item,
			    guint              signal_id,
			    GQuark             detail)
{
#define SIGNAL_N_PARAMS 7
  GValue instance_params[SIGNAL_N_PARAMS] = {{0}}, sig_return = {0};
  ItemEvent dna_item_event;
  int i;  gboolean result = FALSE;
  
  dna_item_event = &(text_item->item_event);

  g_value_init(instance_params, ZMAP_TYPE_WINDOW_TEXT_ITEM);
  g_value_set_object(instance_params, text_item);

  g_value_init(instance_params + 1, G_TYPE_DOUBLE);
  g_value_set_double(instance_params + 1, dna_item_event->origin_x);
      
  g_value_init(instance_params + 2, G_TYPE_DOUBLE);
  g_value_set_double(instance_params + 2, dna_item_event->origin_y);

  g_value_init(instance_params + 3, G_TYPE_DOUBLE);
  g_value_set_double(instance_params + 3, dna_item_event->event_x);
      
  g_value_init(instance_params + 4, G_TYPE_DOUBLE);
  g_value_set_double(instance_params + 4, dna_item_event->event_y);

  g_value_init(instance_params + 5, G_TYPE_INT);
  g_value_set_int(instance_params + 5, dna_item_event->start_index);
      
  g_value_init(instance_params + 6, G_TYPE_INT);
  g_value_set_int(instance_params + 6, dna_item_event->end_index);
  
  g_value_init(&sig_return, G_TYPE_BOOLEAN);
  g_value_set_boolean(&sig_return, result);

  g_object_ref(G_OBJECT(text_item));

  g_signal_emitv(instance_params, signal_id, detail, &sig_return);

  for(i = 0; i < SIGNAL_N_PARAMS; i++)
    {
      g_value_unset(instance_params + i);
    }

  g_object_unref(G_OBJECT(text_item));

#undef SIGNAL_N_PARAMS

  return TRUE;
}

