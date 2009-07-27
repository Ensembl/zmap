/*  File: zmapWindowSequenceFeature.c
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
 * Last edited: Jul 17 12:04 2009 (rds)
 * Created: Fri Jun 12 10:01:17 2009 (rds)
 * CVS info:   $Id: zmapWindowSequenceFeature.c,v 1.3 2009-07-27 03:13:28 rds Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindowSequenceFeature_I.h>
#include <zmapWindowSequenceFeatureCMarshal.h>
#include <zmapWindowTextItem.h>

enum
  {
    PROP_0,

    /* For the floating character */
    PROP_FLOAT_MIN_X,
    PROP_FLOAT_MAX_X,
    PROP_FLOAT_MIN_Y,
    PROP_FLOAT_MAX_Y,
    PROP_FLOAT_AXIS,



    PROP_LAST,
  };

static void zmap_window_sequence_feature_class_init  (ZMapWindowSequenceFeatureClass sequence_class);
static void zmap_window_sequence_feature_init        (ZMapWindowSequenceFeature      sequence);
static void zmap_window_sequence_feature_set_property(GObject               *object, 
						      guint                  param_id,
						      const GValue          *value,
						      GParamSpec            *pspec);
static void zmap_window_sequence_feature_get_property(GObject               *object,
						      guint                  param_id,
						      GValue                *value,
						      GParamSpec            *pspec);


static void zmap_window_sequence_feature_update  (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags);
static void zmap_window_sequence_feature_realize (FooCanvasItem *item);
static void zmap_window_sequence_feature_draw    (FooCanvasItem  *item, 
						  GdkDrawable    *drawable,
						  GdkEventExpose *expose);
static double zmap_window_sequence_feature_point      (FooCanvasItem  *item,
						       double            x,
						       double            y,
						       int               cx,
						       int               cy,
						       FooCanvasItem **actual_item);
static void zmap_window_sequence_feature_destroy     (GtkObject *gtkobject);


static gboolean zmap_window_sequence_feature_selected_signal(ZMapWindowSequenceFeature sequence_feature,
							     int text_first_char, int text_final_char);

static void save_scroll_region(FooCanvasItem *item);
static GType float_group_axis_get_type (void);

static gboolean sequence_feature_emit_signal(ZMapWindowSequenceFeature sequence_feature,
					     guint                     signal_id,
					     int first_index, int final_index);


static FooCanvasItemClass *canvas_parent_class_G;
static FooCanvasItemClass *group_parent_class_G;

GType zMapWindowSequenceFeatureGetType(void)
{
  static GType group_type = 0;
  
  if (!group_type) 
    {
      static const GTypeInfo group_info = 
	{
	  sizeof (zmapWindowSequenceFeatureClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_window_sequence_feature_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (zmapWindowSequenceFeature),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) zmap_window_sequence_feature_init      
	};
    
      group_type = g_type_register_static (ZMAP_TYPE_CANVAS_ITEM,
					   ZMAP_WINDOW_SEQUENCE_FEATURE_NAME,
					   &group_info,
					   0);
  }
  
  return group_type;
}

/*
 * \brief highlight/select the DNA 
 */
gboolean zMapWindowSequenceFeatureSelectByFeature(ZMapWindowSequenceFeature sequence_feature,
						  ZMapFeature               seed_feature,
						  int                       select_flags_ignored_atm)
{
  FooCanvasGroup *sequence_group;
  GList *list;
  gboolean result = TRUE;

  sequence_group = FOO_CANVAS_GROUP(sequence_feature);

  if((list = sequence_group->item_list))
    {
      do
	{
	  /* There should only ever be one iteration here! */
	  switch(seed_feature->type)
	    {
	    case ZMAPSTYLE_MODE_TRANSCRIPT:
	    default:
	      if(ZMAP_IS_WINDOW_TEXT_ITEM(list->data))
		{
		  ZMapWindowTextItem text_item = (ZMapWindowTextItem)(list->data);

		  zMapWindowTextItemSelect(text_item, 
					   seed_feature->x1, seed_feature->x2,
					   FALSE, FALSE);
		}
	      break;
	    }
	}
      while((list = list->next));
    }

  return result;
}

gboolean zMapWindowSequenceFeatureSelectByRegion(ZMapWindowSequenceFeature sequence_feature,
						 int region_start, int region_end,
						 int select_flags_ignored_atm)
{
  FooCanvasGroup *sequence_group;
  GList *list;
  gboolean result = TRUE;

  sequence_group = FOO_CANVAS_GROUP(sequence_feature);

  foo_canvas_item_request_update((FooCanvasItem *)sequence_feature);

  if((list = sequence_group->item_list))
    {
      do
	{
	  /* There should only ever be one iteration here! */
	  if(ZMAP_IS_WINDOW_TEXT_ITEM(list->data))
	    {
	      ZMapWindowTextItem text_item = (ZMapWindowTextItem)(list->data);
	      
	      zMapWindowTextItemSelect(text_item, 
				       region_start, region_end,
				       TRUE, FALSE);
	    }
	}
      while((list = list->next));
    }

  return result;
}


static gboolean sequence_feature_selection_proxy_cb(ZMapWindowTextItem text,
						    double cellx1, double celly1,
						    double cellx2, double celly2,
						    int index1, int index2,
						    gpointer user_data)
{
  ZMapWindowSequenceFeature sequence_feature;
  ZMapWindowCanvasItem canvas_item;
  ZMapFeature feature;
  FooCanvasItem *item;
  double x1, y1, x2, y2;
  double wx1, wy1, wx2, wy2;
  int first_char_index = 0;
  int origin_index, current_index;
  guint signal_id;

  sequence_feature = ZMAP_WINDOW_SEQUENCE_FEATURE(user_data);
  canvas_item = ZMAP_CANVAS_ITEM(sequence_feature);
  item = FOO_CANVAS_ITEM(text);

  feature = canvas_item->feature;

  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2);
  wx1 = x1; wx1 = x2; wy1 = y1; wy2 = y2;
  foo_canvas_item_i2w(item, &wx1, &wy1);
  foo_canvas_item_i2w(item, &wx2, &wy2);

  if (feature->type == ZMAPSTYLE_MODE_PEP_SEQUENCE)
    {
      first_char_index = (int)(wy1 - feature->x1);
      first_char_index = (int)(first_char_index / 3) + 1;
    }
  else
    {
      first_char_index = (int)(wy1 - feature->x1 + 1);
    }
  
  origin_index  = index1 + first_char_index;
  current_index = index2 + first_char_index;

  /* this is needed to fix up what is probably unavoidable from the text item... [-ve coords] */
  if(origin_index < 1)
    origin_index = 1;
  /* this is needed to fix up what is probably unavoidable from the text item... [too long coords] */
  if(current_index > feature->feature.sequence.length)
    current_index = feature->feature.sequence.length;

  signal_id = ZMAP_WINDOW_SEQUENCE_FEATURE_GET_CLASS(sequence_feature)->signals[SEQUENCE_SELECTED_SIGNAL];

  sequence_feature_emit_signal(sequence_feature, signal_id,
			       origin_index, current_index);

  return TRUE;
}

static FooCanvasItem *zmap_window_sequence_feature_add_interval(ZMapWindowCanvasItem   sequence,
								ZMapFeatureSubPartSpan unused,
								double top,  double bottom,
								double left, double right)
{
  FooCanvasGroup *group;
  FooCanvasItem *item = NULL;
  ZMapFeatureTypeStyle style;
  ZMapFeature feature;
  char *font_name = "Lucida Console";
  char *tmp_font = NULL;

  group = FOO_CANVAS_GROUP(sequence);

  if(!(group->item_list))
    {
      feature = sequence->feature;
      style   = (ZMAP_CANVAS_ITEM_GET_CLASS(sequence)->get_style)(sequence);

      g_object_get(G_OBJECT(style),
		   "text-font", &tmp_font,
		   NULL);

      if(tmp_font != NULL)
	font_name = tmp_font;

      item = foo_canvas_item_new(group, ZMAP_TYPE_WINDOW_TEXT_ITEM,
				 "x",          left,  
				 "y",          top,
				 "anchor",     GTK_ANCHOR_NW,
				 "font"    ,   font_name,
				 "full-width", 30.0,
				 "wrap-mode",  PANGO_WRAP_CHAR,
				 NULL);
#ifdef RDS
      g_signal_connect(G_OBJECT(item), "text-selected", 
		       G_CALLBACK(sequence_feature_selection_proxy_cb),
		       sequence);
#endif
    }

  return item;
}

static void zmap_window_sequence_feature_bounds (FooCanvasItem *item, 
						 double *x1, double *y1, 
						 double *x2, double *y2)
{

  if(canvas_parent_class_G->bounds)
    (canvas_parent_class_G->bounds)(item, x1, y1, x2, y2);

  return ;
}


static void zmap_window_sequence_feature_class_init  (ZMapWindowSequenceFeatureClass sequence_class)
{
  ZMapWindowCanvasItemClass canvas_class;
  FooCanvasItemClass *item_class;
  GObjectClass *gobject_class;
  GtkObjectClass *gtk_object_class;
  
  gobject_class = (GObjectClass *) sequence_class;
  gtk_object_class = (GtkObjectClass *) sequence_class;
  canvas_class  = (ZMapWindowCanvasItemClass)sequence_class;
  item_class    = (FooCanvasItemClass *)sequence_class;

  canvas_parent_class_G = (FooCanvasItemClass *)g_type_class_peek_parent(sequence_class);

  gobject_class->set_property = zmap_window_sequence_feature_set_property;
  gobject_class->get_property = zmap_window_sequence_feature_get_property;
  
  group_parent_class_G = g_type_class_peek (FOO_TYPE_CANVAS_GROUP);

  /* Properties for the floating character */
  g_object_class_install_property(gobject_class,
				  PROP_FLOAT_MIN_X,
				  g_param_spec_double("min-x",
						      NULL, NULL,
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
						       G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_FLOAT_MAX_X,
				  g_param_spec_double("max-x",
						      NULL, NULL,
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
						      G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_FLOAT_MIN_Y,
				  g_param_spec_double("min-y",
						      NULL, NULL,
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
						      G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_FLOAT_MAX_Y,
				  g_param_spec_double("max-y",
						      NULL, NULL,
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
						      G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_FLOAT_AXIS,
				  g_param_spec_enum ("float-axis", NULL, NULL,
						     float_group_axis_get_type(), 
						     ZMAP_FLOAT_AXIS_Y,
						     G_PARAM_READWRITE));

  /* floating... */
  item_class->draw    = zmap_window_sequence_feature_draw;
  item_class->update  = zmap_window_sequence_feature_update;
  item_class->realize = zmap_window_sequence_feature_realize;
  item_class->point   = zmap_window_sequence_feature_point;

  canvas_class->add_interval = zmap_window_sequence_feature_add_interval;

  item_class->bounds  = zmap_window_sequence_feature_bounds;

#ifdef RDS_DONT_INCLUDE
  canvas_class->set_colour   = zmap_window_sequence_feature_set_colour;
#endif


  sequence_class->signals[SEQUENCE_SELECTED_SIGNAL] = 
    g_signal_new("sequence-selected",
		 G_TYPE_FROM_CLASS(sequence_class),
		 G_SIGNAL_RUN_LAST,
		 G_STRUCT_OFFSET(zmapWindowSequenceFeatureClass, selected_signal),
		 NULL, NULL,
		 zmapWindowSequenceFeatureCMarshal_BOOL__INT_INT,
		 G_TYPE_BOOLEAN, 2, 
		 G_TYPE_INT, G_TYPE_INT);

  sequence_class->selected_signal = zmap_window_sequence_feature_selected_signal;

  //  gtk_object_class->destroy = zmap_window_sequence_feature_destroy;



  return ;
}

static void zmap_window_sequence_feature_init        (ZMapWindowSequenceFeature  sequence)
{
  ZMapWindowCanvasItem canvas_item;
  FooCanvasItem *item;

  canvas_item = ZMAP_CANVAS_ITEM(sequence);
  canvas_item->auto_resize_background = 1;

  /* initialise the floating stuff */
  sequence->float_settings.zoom_x = 0.0;
  sequence->float_settings.zoom_y = 0.0;
  sequence->float_settings.scr_x1 = 0.0;
  sequence->float_settings.scr_y1 = 0.0;
  sequence->float_settings.scr_x2 = 0.0;
  sequence->float_settings.scr_y2 = 0.0;

  item = FOO_CANVAS_ITEM(sequence);
  if(item->canvas)
    {
      sequence->float_settings.zoom_x = item->canvas->pixels_per_unit_x;
      sequence->float_settings.zoom_y = item->canvas->pixels_per_unit_y;
    }

  /* y axis only! */
  sequence->float_flags.float_axis = ZMAP_FLOAT_AXIS_Y;

  return ;
}

static void zmap_window_sequence_feature_set_property(GObject               *gobject, 
						      guint                  param_id,
						      const GValue          *value,
						      GParamSpec            *pspec)
{
  ZMapWindowSequenceFeature seq_feature;
  double d = 0.0;		/* dummy value for i2w() */

  seq_feature = ZMAP_WINDOW_SEQUENCE_FEATURE(gobject);

  switch (param_id) 
    {
    case PROP_FLOAT_MIN_X:
      seq_feature->float_settings.scr_x1 = g_value_get_double (value);
      foo_canvas_item_i2w(FOO_CANVAS_ITEM(gobject), &(seq_feature->float_settings.scr_x1), &d);
      seq_feature->float_flags.min_x_set = TRUE;
      break;
    case PROP_FLOAT_MAX_X:
      seq_feature->float_settings.scr_x2 = g_value_get_double (value);
      foo_canvas_item_i2w(FOO_CANVAS_ITEM(gobject), &(seq_feature->float_settings.scr_x2), &d);
      seq_feature->float_flags.max_x_set = TRUE;
      break;
    case PROP_FLOAT_MIN_Y:
      seq_feature->float_settings.scr_y1 = g_value_get_double (value);
      foo_canvas_item_i2w(FOO_CANVAS_ITEM(gobject), &d, &(seq_feature->float_settings.scr_y1));
      seq_feature->float_flags.min_y_set = TRUE;
      break;
    case PROP_FLOAT_MAX_Y:
      seq_feature->float_settings.scr_y2 = g_value_get_double (value);
      foo_canvas_item_i2w(FOO_CANVAS_ITEM(gobject), &d, &(seq_feature->float_settings.scr_y2));
      seq_feature->float_flags.max_y_set = TRUE;
      break;
    case PROP_FLOAT_AXIS:
      {
	int axis = g_value_get_enum(value);
	seq_feature->float_flags.float_axis = axis;
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
      break;
    }


  return ;
}

static void zmap_window_sequence_feature_get_property(GObject               *object,
						      guint                  param_id,
						      GValue                *value,
						      GParamSpec            *pspec)
{
  switch(param_id)
    {
    case PROP_FLOAT_MIN_X:
      break;
    case PROP_FLOAT_MAX_X:
      break;
    case PROP_FLOAT_MIN_Y:
      break;
    case PROP_FLOAT_MAX_Y:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }

  return ;
}


static void invoke_zeroing_update(gpointer item_data, gpointer unused_data)
{
  FooCanvasItem *item = (FooCanvasItem *)item_data;
  FooCanvasGroup *group;

  if(FOO_IS_CANVAS_GROUP(item))
    {
      group = (FooCanvasGroup *)item;
      g_list_foreach(group->item_list, invoke_zeroing_update, unused_data);
    }

  item->x1 = item->y1 = 0.0;
  item->x2 = item->y2 = 0.0;

  return ;
}

static void zmap_window_sequence_feature_update  (FooCanvasItem *item,
						  double i2w_dx, double i2w_dy, 
						  int flags)
{
  ZMapWindowSequenceFeature seq_feature;
  FooCanvasItem *parent;
  FooCanvasGroup *group;
  double x1, x2, y1, y2;
  double xpos, ypos;
  double scr_x1, scr_y1, scr_x2, scr_y2;
  gboolean force_intersect = FALSE;

  seq_feature = ZMAP_WINDOW_SEQUENCE_FEATURE(item);
  group = FOO_CANVAS_GROUP (item);
  xpos  = group->xpos;
  ypos  = group->ypos;

  /* First we move the group to the correct position. */

  /* run a "update" loop to zero all the items in this group */
  invoke_zeroing_update(item, NULL);

  /* Now we can safely get the bounds of our parent group */
  /* actually we want the ContainerGroup because they don't 
   * work in the same way as canvasitems (mistake) */
  if(ZMAP_IS_CONTAINER_FEATURES(item->parent))
    {
      parent = (FooCanvasItem *)zmapWindowContainerChildGetParent(item->parent);
    }
  else
    parent = item->parent;

  foo_canvas_item_get_bounds(parent, &x1, &y1, &x2, &y2);
  
  /* If the group x,y is outside the scroll region, move it back in! */
  foo_canvas_get_scroll_region(item->canvas, &scr_x1, &scr_y1, &scr_x2, &scr_y2);

  foo_canvas_item_w2i(item, &scr_x1, &scr_y1);
  foo_canvas_item_w2i(item, &scr_x2, &scr_y2);

  if(seq_feature->float_flags.float_axis & ZMAP_FLOAT_AXIS_X)
    {
      xpos = x1;
      if(scr_x1 >= x1 && scr_x1 < x2)
	xpos = scr_x1;
    }

  if(seq_feature->float_flags.float_axis & ZMAP_FLOAT_AXIS_Y)
    {
      ypos = y1;

      if(scr_y1 >= y1 && scr_y1 < y2)
	ypos = scr_y1;
    }

  /* round down to whole bases... We need to do this in a few places! */
  xpos = (double)((int)xpos);
  ypos = (double)((int)ypos);

  group->xpos = xpos;
  group->ypos = ypos;


  if(force_intersect && (item->object.flags & FOO_CANVAS_ITEM_VISIBLE))
    {
      int cx1, cx2, cy1, cy2;

      foo_canvas_w2c(item->canvas, 
		     seq_feature->float_settings.scr_x1, 
		     seq_feature->float_settings.scr_y1, 
		     &cx1, &cy1);
      foo_canvas_w2c(item->canvas, 
		     seq_feature->float_settings.scr_x2, 
		     seq_feature->float_settings.scr_y2, 
		     &cx2, &cy2);

      /* These must be set in order to make the seq_feature intersect with any
       * rectangle within the whole of the scroll region */
      if(seq_feature->float_flags.float_axis & ZMAP_FLOAT_AXIS_X)
	{
	  item->x1 = cx1; //seq_feature->scr_x1;
	  item->x2 = cx2; //seq_feature->scr_x2;
	}
      if(seq_feature->float_flags.float_axis & ZMAP_FLOAT_AXIS_Y)
	{
	  item->y1 = cy1; //seq_feature->scr_y1;
	  item->y2 = cy2; //seq_feature->scr_y2;
	}
    }

  if(canvas_parent_class_G->update)
    (* canvas_parent_class_G->update)(item, i2w_dx, i2w_dy, flags);
  

  return ;
}

static double zmap_window_sequence_feature_point      (FooCanvasItem  *item,
						       double            x,
						       double            y,
						       int               cx,
						       int               cy,
						       FooCanvasItem **actual_item)
{
  double dist = 1.0;

  if(group_parent_class_G->point)
    dist = (group_parent_class_G->point)(item, x, y, cx, cy, actual_item);

  return dist;
}

static void zmap_window_sequence_feature_realize (FooCanvasItem *item)
{
  ZMapWindowSequenceFeature seq_feature;

  seq_feature = ZMAP_WINDOW_SEQUENCE_FEATURE(item);

  if(item->canvas)
    {
      seq_feature->float_settings.zoom_x = item->canvas->pixels_per_unit_x;
      seq_feature->float_settings.zoom_y = item->canvas->pixels_per_unit_y;
      save_scroll_region(item);
    }
  
  if(canvas_parent_class_G->realize)
    (* canvas_parent_class_G->realize)(item);
}

static void zmap_window_sequence_feature_draw(FooCanvasItem  *item, 
					      GdkDrawable    *drawable,
					      GdkEventExpose *expose)
{
  ZMapWindowSequenceFeature seq_feature;
  FooCanvasGroup *group;

  group = FOO_CANVAS_GROUP (item);
  seq_feature = ZMAP_WINDOW_SEQUENCE_FEATURE(item);

  /* parent->draw? */
  if(canvas_parent_class_G->draw)
    (* canvas_parent_class_G->draw)(item, drawable, expose);

  return ;
}



static void zmap_window_sequence_feature_destroy     (GtkObject *gtkobject)
{
  if(GTK_OBJECT_CLASS(canvas_parent_class_G)->destroy)
    (* GTK_OBJECT_CLASS(canvas_parent_class_G)->destroy)(gtkobject);

  return ;
}

static gboolean zmap_window_sequence_feature_selected_signal(ZMapWindowSequenceFeature sequence_feature,
							     int text_first_char, int text_final_char)
{
#ifdef RDS_DONT_INCLUDE
  g_warning("%s sequence indices %d to %d", __PRETTY_FUNCTION__, text_first_char, text_final_char);
#endif /* RDS_DONT_INCLUDE */
  return FALSE;
}


static void save_scroll_region(FooCanvasItem *item)
{
  ZMapWindowSequenceFeature seq_feature;
  double x1, y1, x2, y2;
  double *x1_ptr, *y1_ptr, *x2_ptr, *y2_ptr;

  seq_feature = ZMAP_WINDOW_SEQUENCE_FEATURE(item);

  foo_canvas_get_scroll_region(item->canvas, &x1, &y1, &x2, &y2);
  
  x1_ptr = &(seq_feature->float_settings.scr_x1);
  y1_ptr = &(seq_feature->float_settings.scr_y1);
  x2_ptr = &(seq_feature->float_settings.scr_x2);
  y2_ptr = &(seq_feature->float_settings.scr_y2);

  if(item->canvas->pixels_per_unit_x <= seq_feature->float_settings.zoom_x)
    {
      if(!seq_feature->float_flags.min_x_set)
	seq_feature->float_settings.scr_x1 = x1;
      else
	x1_ptr = &x1;
      if(!seq_feature->float_flags.max_x_set)
	seq_feature->float_settings.scr_x2 = x2;
      else
	x2_ptr = &x2;
    }

  if(item->canvas->pixels_per_unit_y <= seq_feature->float_settings.zoom_y)
    {
      if(!seq_feature->float_flags.min_y_set)
	seq_feature->float_settings.scr_y1 = y1;
      else
	y1_ptr = &y1;
      if(!seq_feature->float_flags.max_y_set)
	seq_feature->float_settings.scr_y2 = y2;
      else
	y2_ptr = &y2;
    }
  
  foo_canvas_item_w2i(item, x1_ptr, y1_ptr);
  foo_canvas_item_w2i(item, x2_ptr, y2_ptr);

  return ;
}


static GType float_group_axis_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { ZMAP_FLOAT_AXIS_X, "ZMAP_FLOAT_AXIS_X", "x-axis" },
      { ZMAP_FLOAT_AXIS_Y, "ZMAP_FLOAT_AXIS_Y", "y-axis" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("ZMapWindowFloatGroupAxis", values);
  }
  return etype;
}


static gboolean sequence_feature_emit_signal(ZMapWindowSequenceFeature sequence_feature,
					     guint                     signal_id,
					     int first_index, int final_index)
{
#define SIGNAL_N_PARAMS 3
  GValue instance_params[SIGNAL_N_PARAMS] = {{0}}, sig_return = {0};
  GQuark detail = 0;
  int i;
  gboolean result = FALSE;
  
  g_value_init(instance_params, ZMAP_TYPE_WINDOW_SEQUENCE_FEATURE);
  g_value_set_object(instance_params, sequence_feature);

  g_value_init(instance_params + 1, G_TYPE_INT);
  g_value_set_int(instance_params + 1, first_index);
      
  g_value_init(instance_params + 2, G_TYPE_INT);
  g_value_set_int(instance_params + 2, final_index);
  
  g_value_init(&sig_return, G_TYPE_BOOLEAN);
  g_value_set_boolean(&sig_return, result);

  g_object_ref(G_OBJECT(sequence_feature));

  g_signal_emitv(instance_params, signal_id, detail, &sig_return);

  for(i = 0; i < SIGNAL_N_PARAMS; i++)
    {
      g_value_unset(instance_params + i);
    }

  g_object_unref(G_OBJECT(sequence_feature));

#undef SIGNAL_N_PARAMS

  return TRUE;
}

