/*  File: zmapWindowCollectionFeature.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
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
 * Last edited: Feb 16 10:22 2010 (edgrif)
 * Created: Wed Dec  3 10:02:22 2008 (rds)
 * CVS info:   $Id: zmapWindowCollectionFeature.c,v 1.15 2010-02-17 16:00:33 mh17 Exp $
 *-------------------------------------------------------------------
 */
#include <math.h>
#include <zmapWindowCollectionFeature_I.h>

typedef enum {FIRST_MATCH, LAST_MATCH} MatchType ;

typedef struct
{
  ZMapWindowCanvasItem parent;
  FooCanvasGroup *new_parent;
  double x, y;
} ParentPositionStruct, *ParentPosition;


typedef struct
{
  ZMapWindowCanvasItem parent;
  FooCanvasItem       *previous;
  ZMapFeature          prev_feature;
  GdkColor             perfect;
  GdkColor             colinear;
  GdkColor             non_colinear;
  double               x;
  ZMapFeatureCompareFunc compare_func;
  gpointer               compare_data;
} ColinearMarkerDataStruct, *ColinearMarkerData;


static void zmap_window_collection_feature_class_init  (ZMapWindowCollectionFeatureClass collection_class);
static void zmap_window_collection_feature_init        (ZMapWindowCollectionFeature      group);
static void zmap_window_collection_feature_set_property(GObject               *object, 
							guint                  param_id,
							const GValue          *value,
							GParamSpec            *pspec);
static void zmap_window_collection_feature_get_property(GObject               *object,
							guint                  param_id,
							GValue                *value,
							GParamSpec            *pspec);
static void zmap_window_collection_feature_destroy     (GObject *object);


static double zmap_window_collection_feature_item_point (FooCanvasItem *item, double x, double y, int cx, int cy,
							 FooCanvasItem **actual_item);

static FooCanvasItem *zmap_window_collection_feature_add_interval(ZMapWindowCanvasItem   collection,
								  ZMapFeatureSubPartSpan unused,
								  double top,  double bottom,
								  double left, double right);
static ZMapFeatureTypeStyle zmap_window_collection_feature_get_style(ZMapWindowCanvasItem canvas_item);


/* Accessory functions, some from foo-canvas.c */
static double get_glyph_mid_point(FooCanvasItem *item, double glyph_width,
				  double *x1_out, double *y1_out,
				  double *x2_out, double *y2_out);
static void group_remove(gpointer data, gpointer user_data);
static void add_colinear_lines(gpointer data, gpointer user_data);
static void markMatchIfIncomplete(ZMapWindowCanvasItem collection,
				  ZMapStrand ref_strand, MatchType match_type,
				  FooCanvasItem *item, ZMapFeature feature) ;
static gboolean fragments_splice(char *fragment_a, char *fragment_b);
static void process_feature(ZMapFeature prev_feature);

static ZMapWindowCanvasItemClass canvas_parent_class_G = NULL;
static FooCanvasItemClass *item_parent_class_G = NULL;


GType zMapWindowCollectionFeatureGetType(void)
{
  static GType group_type = 0;
  
  if (!group_type) {
    static const GTypeInfo group_info = {
      sizeof (zmapWindowCollectionFeatureClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) zmap_window_collection_feature_class_init,
      NULL,           /* class_finalize */
      NULL,           /* class_data */
      sizeof (zmapWindowCollectionFeature),
      0,              /* n_preallocs */
      (GInstanceInitFunc) zmap_window_collection_feature_init
      
      
    };
    
    group_type = g_type_register_static (zMapWindowCanvasItemGetType(),
					 ZMAP_WINDOW_COLLECTION_FEATURE_NAME,
					 &group_info,
					 0);
  }
  
  return group_type;
}

ZMapWindowCanvasItem zMapWindowCollectionFeatureCreate(FooCanvasGroup *parent)
{
  ZMapWindowCanvasItem canvas_item = NULL;
  FooCanvasItem *item;

  //#define DEBUG_PINK_COLLECTION_FEATURE_BACKGROUND
  item = foo_canvas_item_new(parent, zMapWindowCollectionFeatureGetType(), 
			     "x", 0.0, 
			     "y", 0.0, 
			     NULL);
      
  if(item && ZMAP_IS_CANVAS_ITEM(item))
    {
#ifdef DEBUG_PINK_COLLECTION_FEATURE_BACKGROUND
      FooCanvasItem *background;
#endif /* DEBUG_PINK_COLLECTION_FEATURE_BACKGROUND */

      canvas_item = ZMAP_CANVAS_ITEM(item);
      
      if(ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->post_create)
	(* ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->post_create)(canvas_item);

#ifdef DEBUG_PINK_COLLECTION_FEATURE_BACKGROUND
      if((background = canvas_item->items[WINDOW_ITEM_BACKGROUND]))
	{
	  GdkColor fill = {0};
	  
	  gdk_color_parse("pink", &fill);

	  foo_canvas_item_set(background, "fill_color_gdk", &fill, NULL);
	}
#endif

      zMapWindowCanvasItemCheckSize(canvas_item);      
    }
  
  return canvas_item;
}

void zMapWindowCollectionFeatureRemoveSubFeatures(ZMapWindowCanvasItem collection, 
						  gboolean keep_in_place_x, 
						  gboolean keep_in_place_y)
{
  ParentPositionStruct parent_data = {NULL};
  FooCanvasItem *item;
  FooCanvasGroup *group;

  item  = FOO_CANVAS_ITEM(collection);
  group = FOO_CANVAS_GROUP(item);

  parent_data.new_parent = FOO_CANVAS_GROUP(item->parent);
  parent_data.parent     = collection;

  parent_data.x = parent_data.y = 0.0;

  if(keep_in_place_x)
    parent_data.x = group->xpos;
  if(keep_in_place_y)
    parent_data.y = group->ypos;

  g_list_foreach(group->item_list, group_remove, &parent_data);

  g_list_free(group->item_list);
  group->item_list = group->item_list_end = NULL;

  return ;
}

void zMapWindowCollectionFeatureStaticReparent(ZMapWindowCanvasItem reparentee, 
					       ZMapWindowCanvasItem new_parent)
{
  FooCanvasItem *item;
  FooCanvasGroup *parent_group, *child_group;
  double x, y;

  parent_group = FOO_CANVAS_GROUP(new_parent);
  child_group  = FOO_CANVAS_GROUP(reparentee);
  item         = FOO_CANVAS_ITEM(reparentee);

  zMapWindowCanvasItemReparent(item, parent_group);

  x = child_group->xpos - parent_group->xpos;
  y = child_group->ypos - parent_group->ypos;

  foo_canvas_item_set(item, 
		      "x", x,
		      "y", y,
		      NULL);

  new_parent->feature = NULL;//reparentee->feature;

  return;
}

void zMapWindowCollectionFeatureAddColinearMarkers(ZMapWindowCanvasItem   collection,
						   ZMapFeatureCompareFunc compare_func,
						   gpointer               compare_data)
{
  ColinearMarkerDataStruct colinear_data = {NULL};
  FooCanvasGroup *group;

  group = FOO_CANVAS_GROUP(collection);

  colinear_data.parent = collection;

  if(group->item_list)
    {
      ZMapFeatureTypeStyle style;
      double x2;
      char *perfect_colour     = ZMAP_WINDOW_MATCH_PERFECT ;
      char *colinear_colour    = ZMAP_WINDOW_MATCH_COLINEAR ;
      char *noncolinear_colour = ZMAP_WINDOW_MATCH_NOTCOLINEAR ;

      colinear_data.previous     = FOO_CANVAS_ITEM(group->item_list->data);
      colinear_data.prev_feature = ZMAP_CANVAS_ITEM(colinear_data.previous)->feature;

      colinear_data.compare_func = compare_func;
      colinear_data.compare_data = compare_data;

      style = (ZMAP_CANVAS_ITEM_GET_CLASS(colinear_data.previous)->get_style)(ZMAP_CANVAS_ITEM(colinear_data.previous));

      x2 = zMapStyleGetWidth(style);

      colinear_data.x = (x2 * 0.5);

      gdk_color_parse(perfect_colour,     &colinear_data.perfect) ;
      gdk_color_parse(colinear_colour,    &colinear_data.colinear) ;
      gdk_color_parse(noncolinear_colour, &colinear_data.non_colinear) ;

      g_list_foreach(group->item_list->next, add_colinear_lines, &colinear_data);
    }

  return ;
}

// inspired by zMapWindowCanvasItemGetBumpBounds() in zmapWindowCanvasItem.c
ZMapStyleGlyphType zmapWindowCanvasItemGetGlyph(ZMapWindowCanvasItem collection)
{
  ZMapFeatureTypeStyle style;
  ZMapStyleGlyphType gt = ZMAP_GLYPH_ITEM_STYLE_DIAMOND;

  style = (ZMAP_CANVAS_ITEM_GET_CLASS(collection)->get_style)(collection);

  g_object_get(G_OBJECT(style),
             ZMAPSTYLE_PROPERTY_ALIGNMENT_INCOMPLETE_GLYPH, &gt,
             NULL);
  // mh17: g_object_get does not appear to use the default set or avoid overwriting gt if not set
  // need to make styles config/ g_object work as designed, but that might be a lot of typing
  // so here's a quick bodge:
  if(gt <= ZMAPSTYLE_GLYPH_TYPE_INVALID || gt > ZMAPSTYLE_GLYPH_TYPE_CIRCLE)
      gt = ZMAPSTYLE_GLYPH_TYPE_DIAMOND;  // previous hard coded value

  return gt;
}


void zMapWindowCollectionFeatureAddIncompleteMarkers(ZMapWindowCanvasItem collection,
						     gboolean revcomped_features)
{
  FooCanvasGroup *group;

  if ((group = FOO_CANVAS_GROUP(collection)) && (group->item_list))
    {
      ZMapWindowCanvasItem canvas_item ;
      FooCanvasItem *first_item, *last_item ;
      ZMapFeature first_feature, last_feature ;
      ZMapStrand ref_strand ;

      /* Assume that match is forward so first match is first item, last match is last item. */
      first_item   = FOO_CANVAS_ITEM(group->item_list->data);
      canvas_item  = ZMAP_CANVAS_ITEM(first_item);
      first_feature = canvas_item->feature;

      last_item    = FOO_CANVAS_ITEM(group->item_list_end->data);
      canvas_item  = ZMAP_CANVAS_ITEM(last_item);
      last_feature = canvas_item->feature;

      /* Now we have a feature we can retrieve which strand of the reference sequence the
       * alignment is to. */
      ref_strand = first_feature->strand ;

      /* If the alignment is to the reverse strand of the reference sequence then swop first and last. */
      if (ref_strand == ZMAPSTRAND_REVERSE)
	{
	  last_item   = FOO_CANVAS_ITEM(group->item_list->data);
	  canvas_item  = ZMAP_CANVAS_ITEM(last_item);
	  last_feature = canvas_item->feature;

	  first_item    = FOO_CANVAS_ITEM(group->item_list_end->data);
	  canvas_item  = ZMAP_CANVAS_ITEM(first_item);
	  first_feature = canvas_item->feature;
	}

      /* If either the first or last match features are truncated then add the incomplete marker. */
      markMatchIfIncomplete(collection, ref_strand, FIRST_MATCH, first_item, first_feature) ;

      markMatchIfIncomplete(collection, ref_strand, LAST_MATCH, last_item, last_feature) ;
    }
  

  return ;
}



void zMapWindowCollectionFeatureAddSpliceMarkers(ZMapWindowCanvasItem collection)
{
  char *splice_colour = "blue" ;
  FooCanvasGroup *group;
  GdkColor marker_colour;
  double width = 6.0;
  double x_coord;
  gboolean canonical ;

  group = FOO_CANVAS_GROUP(collection);

  canonical = FALSE ;
  /* splce_colour needs to come from the style! */
  gdk_color_parse(splice_colour, &marker_colour) ;
      
  if(group->item_list && group->item_list->next)
    {
      GList *list;
      ZMapFeature prev_feature;
      ZMapFeature curr_feature;

      if((prev_feature = zMapWindowCanvasItemGetFeature(group->item_list->data)))
	process_feature(prev_feature);

      if((list = group->item_list->next))
	x_coord = get_glyph_mid_point(list->data, width, NULL, NULL, NULL, NULL);

      while(list && prev_feature)
	{
	  char *prev, *curr;
	  gboolean prev_reversed, curr_reversed;
	  
	  curr_feature = zMapWindowCanvasItemGetFeature(list->data);
	  
	  prev_reversed = prev_feature->strand == ZMAPSTRAND_REVERSE;
	  curr_reversed = curr_feature->strand == ZMAPSTRAND_REVERSE;

	  process_feature(curr_feature);

	  prev = zMapFeatureGetDNA((ZMapFeatureAny)prev_feature,
				   prev_feature->x2 + 1,
				   prev_feature->x2 + 2,
				   prev_reversed);

	  curr = zMapFeatureGetDNA((ZMapFeatureAny)curr_feature,
				   curr_feature->x1 - 2,
				   curr_feature->x1 - 1,
				   curr_reversed);

	  if(prev && curr && fragments_splice(prev, curr))
	    {
	      FooCanvasGroup *parent;

	      parent = FOO_CANVAS_GROUP(collection->items[WINDOW_ITEM_OVERLAY]);

	      foo_canvas_item_new(parent,
				  zMapWindowGlyphItemGetType(),
				  "fill_color_gdk",    &marker_colour,
				  "outline_color_gdk", &marker_colour,
				  "x",                 x_coord,
				  "y",                 prev_feature->x2 - group->ypos,
				  "width",             width,
				  "height",            width,
				  "glyph_style",       6,     // see function zMapWindowCollectionFeatureAddIncompleteMarkers() above, NB this function is never called, see zmapWindowColBump.c
				  "line_width",        1,
				  NULL);

	      foo_canvas_item_new(parent,
				  zMapWindowGlyphItemGetType(),
				  "fill_color_gdk",    &marker_colour,
				  "outline_color_gdk", &marker_colour,
				  "x",                 x_coord,
				  "y",                 curr_feature->x1 - group->ypos - 1,
				  "width",             width,
				  "height",            width,
				  "glyph_style",       6,
				  "line_width",        1,
				  NULL);	    
	    }

	  /* free */
	  if(prev)
	    g_free(prev);
	  if(curr)
	    g_free(curr);

	  /* move on. */
	  prev_feature = curr_feature;

	  list = list->next;
	}
      

    }

  return ;
}







/* 
 *                    Internal routines.
 */



static FooCanvasItem *zmap_window_collection_feature_add_interval(ZMapWindowCanvasItem   collection,
								  ZMapFeatureSubPartSpan unused,
								  double top,  double bottom,
								  double left, double right)
{
  FooCanvasItem *item = NULL;

  /* these will be the joining bits */

  return item;
}

static void zmap_window_collection_feature_set_colour(ZMapWindowCanvasItem   canvas_item,
						      FooCanvasItem         *interval,
						      ZMapFeatureSubPartSpan sub_feature,
						      ZMapStyleColourType    colour_type,
						      GdkColor              *default_fill)
{

  if(ZMAP_IS_CANVAS_ITEM(interval))
    {
      if(ZMAP_CANVAS_ITEM_GET_CLASS(interval)->set_colour)
	(ZMAP_CANVAS_ITEM_GET_CLASS(interval)->set_colour)(canvas_item, interval, sub_feature,
							      colour_type, default_fill);
    }
  else
    {
      /* chain up? Actually I think we need to do something else... */
      if(canvas_parent_class_G->set_colour)
	(canvas_parent_class_G->set_colour)(canvas_item, interval, sub_feature,
					    colour_type, default_fill);
    }

  return ;
}

static ZMapFeatureTypeStyle zmap_window_collection_feature_get_style(ZMapWindowCanvasItem canvas_item)
{
  ZMapFeatureTypeStyle style = NULL;
  ZMapWindowContainerGroup container_parent;
  FooCanvasItem *item;
  FooCanvasGroup *group;

  g_return_val_if_fail(canvas_item != NULL, NULL);

  item  = FOO_CANVAS_ITEM(canvas_item);
  group = FOO_CANVAS_GROUP(canvas_item);

  if(item->parent && item->parent->parent && group->item_list)
    {
      ZMapWindowCanvasItem first_item;

      first_item = ZMAP_CANVAS_ITEM(group->item_list->data);

      container_parent = zmapWindowContainerCanvasItemGetContainer(item);

      /* can optimise this a bit, by including the featureset_i header... */
      style = zmapWindowContainerFeatureSetStyleFromID((ZMapWindowContainerFeatureSet)container_parent,
						       first_item->feature->style_id);
    }

  return style;
}


/* object impl */
static void zmap_window_collection_feature_class_init  (ZMapWindowCollectionFeatureClass collection_class)
{
  ZMapWindowCanvasItemClass canvas_class;
  FooCanvasItemClass *item_class;
  GObjectClass *gobject_class;
  
  gobject_class = (GObjectClass *) collection_class;
  canvas_class  = (ZMapWindowCanvasItemClass)collection_class;
  item_class    = (FooCanvasItemClass *)collection_class;

  gobject_class->set_property = zmap_window_collection_feature_set_property;
  gobject_class->get_property = zmap_window_collection_feature_get_property;
  
  //  g_object_class_override_property(gobject_class, COLLECTION_INTERVAL_TYPE,
  //			   ZMAP_WINDOW_CANVAS_INTERVAL_TYPE);

  item_class->point = zmap_window_collection_feature_item_point;

  item_parent_class_G = g_type_class_peek (foo_canvas_group_get_type());

  canvas_parent_class_G = g_type_class_peek(zMapWindowCanvasItemGetType());

  item_class->bounds  = item_parent_class_G->bounds;

  canvas_class->add_interval = zmap_window_collection_feature_add_interval;
  canvas_class->set_colour   = zmap_window_collection_feature_set_colour;
  canvas_class->check_data   = NULL;
  canvas_class->get_style    = zmap_window_collection_feature_get_style;

  gobject_class->dispose = zmap_window_collection_feature_destroy;

  return ;
}

static void zmap_window_collection_feature_init        (ZMapWindowCollectionFeature collection)
{
  ZMapWindowCanvasItem canvas_item;
  canvas_item = ZMAP_CANVAS_ITEM(collection);
  canvas_item->auto_resize_background = TRUE;
  return ;
}

static void zmap_window_collection_feature_set_property(GObject               *object, 
							guint                  param_id,
							const GValue          *value,
							GParamSpec            *pspec)

{
  ZMapWindowCanvasItem canvas_item;
  ZMapWindowCollectionFeature collection;

  g_return_if_fail(ZMAP_IS_WINDOW_COLLECTION_FEATURE(object));

  collection  = ZMAP_WINDOW_COLLECTION_FEATURE(object);
  canvas_item = ZMAP_CANVAS_ITEM(object);

  switch(param_id)
    {

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_window_collection_feature_get_property(GObject               *object,
							guint                  param_id,
							GValue                *value,
							GParamSpec            *pspec)
{
  return ;
}

static double window_collection_feature_invoke_point (FooCanvasItem *item, 
						      double x, double y, 
						      int cx, int cy,
						      FooCanvasItem **actual_item)
{
  /* Calculate x & y in item local coordinates */
  
  if (FOO_CANVAS_ITEM_GET_CLASS (item)->point)
    return FOO_CANVAS_ITEM_GET_CLASS (item)->point (item, x, y, cx, cy, actual_item);
  
  return 1e18;
}

/* Point handler for canvas groups */
static double zmap_window_collection_feature_item_point (FooCanvasItem *item, double x, double y, int cx, int cy,
							 FooCanvasItem **actual_item)
{
  FooCanvasGroup *group;
  FooCanvasItem *child, *point_item;
  GList *list = NULL;
  int x1, y1, x2, y2;
  double gx, gy;
  double dist, best;

  group = FOO_CANVAS_GROUP (item);
  
  x1 = cx - item->canvas->close_enough;
  y1 = cy - item->canvas->close_enough;
  x2 = cx + item->canvas->close_enough;
  y2 = cy + item->canvas->close_enough;
  
  best = 0.0;
  *actual_item = NULL;
  
  gx = x - group->xpos;
  gy = y - group->ypos;

  dist = 0.0; /* keep gcc happy */

  list = group->item_list;

  while(list)
    {
      child = list->data;

      if ((child->object.flags & FOO_CANVAS_ITEM_MAPPED)
	  && FOO_CANVAS_ITEM_GET_CLASS (child)->point) 
	{
	  dist = window_collection_feature_invoke_point (child, gx, gy, cx, cy, &point_item);
	  if(point_item && ((int)(dist * item->canvas->pixels_per_unit_x + 0.5) <= item->canvas->close_enough) &&
	     (dist <= best))
	    {
	      best = dist;
	      *actual_item = point_item;
	    }
	} 
      list = list->next;
    }
  
  if(actual_item == NULL && item_parent_class_G->point)
    best = (item_parent_class_G->point)(item, x, y, cx, cy, actual_item);

  return best;
}


static void zmap_window_collection_feature_destroy     (GObject *object)
{
  GObjectClass *object_class;

  zMapWindowCollectionFeatureRemoveSubFeatures(ZMAP_CANVAS_ITEM(object), TRUE, TRUE);

  object_class = (GObjectClass *)canvas_parent_class_G;

  if(object_class->dispose)
    (object_class->dispose)(object);

  return ;
}



/* Accessory functions, some copied as they were statics in foo-canvas.c */


/* Hopefully item will be centered so will present _the_ mid point of
 * the column.  As we get the bounds, we might as well get all of
 * them, and return if requested. */

static double get_glyph_mid_point(FooCanvasItem *item, double glyph_width,
				  double *x1_out, double *y1_out,
				  double *x2_out, double *y2_out)
{
  double x, x1, x2, y1, y2;

  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2);
  /*     centre point           - half width          + rounding */
  x =  (((x2 - x1) * 0.5) + x1) - (glyph_width * 0.5) + 0.5;

  if(x1_out)
    *x1_out = x1;

  if(x2_out)
    *x2_out = x2;

  if(y1_out)
    *y1_out = y1;

  if(y2_out)
    *y2_out = y2;

  return x;
}

static void group_remove(gpointer data, gpointer user_data)
{
  FooCanvasItem *item2remove = FOO_CANVAS_ITEM(data);
  ParentPosition parent_data = (ParentPosition)user_data;
  double x, y;

  if(ZMAP_IS_CANVAS_ITEM(data))
    {
      g_object_get(G_OBJECT(item2remove),
		   "x", &x,
		   "y", &y,
		   NULL);
      
      zMapWindowCanvasItemReparent(item2remove, parent_data->new_parent);
      
      x += parent_data->x;
      y += parent_data->y;
      
      foo_canvas_item_set(item2remove,
			  "x", x, 
			  "y", y,
			  NULL);
    }

  return ;
}

static void add_colinear_lines(gpointer data, gpointer user_data)
{
  ColinearMarkerData colinear_data = (ColinearMarkerData)user_data;
  ZMapWindowCanvasItem canvas_item = NULL;
  FooCanvasItem *current = FOO_CANVAS_ITEM(data);
  FooCanvasItem *previous;
  ZMapFeature prev_feature, curr_feature;
  GdkColor *draw_colour;
  FooCanvasPoints line_points;
  double coords[4], y1, y2;
  ColinearityType colinearity = 0;
  enum {COLINEAR_INVALID, COLINEAR_NOT, COLINEAR_IMPERFECT, COLINEAR_PERFECT};

  previous = colinear_data->previous;
  colinear_data->previous = current;

  prev_feature = colinear_data->prev_feature;
  curr_feature = ZMAP_CANVAS_ITEM(current)->feature;
  colinear_data->prev_feature = curr_feature;

  canvas_item = ZMAP_CANVAS_ITEM(colinear_data->parent);

  if(colinear_data->compare_func)
    {
      colinearity = (colinear_data->compare_func)(prev_feature, curr_feature, 
						  colinear_data->compare_data);
      if(colinearity != 0)
	{
	  FooCanvasGroup *canvas_group;
	  FooCanvasItem *colinear_line;
	  double py1, py2, cy1, cy2;

	  if (colinearity == COLINEAR_NOT)
	    draw_colour = &colinear_data->non_colinear ;
	  else if (colinearity == COLINEAR_IMPERFECT)
	    draw_colour = &colinear_data->colinear ;
	  else
	    draw_colour = &colinear_data->perfect ;


	  foo_canvas_item_get_bounds(previous, NULL, &py1, NULL, &py2);
	  foo_canvas_item_get_bounds(current,  NULL, &cy1, NULL, &cy2);

	  canvas_group = (FooCanvasGroup *)canvas_item;

	  y1 = floor(py2);
	  y2 = ceil (cy1);

	  y1 = prev_feature->x2 - canvas_group->ypos;
	  y2 = curr_feature->x1 - canvas_group->ypos - 1.0; /* Ext2Zero */

	  coords[0] = colinear_data->x;
	  coords[1] = y1;
	  coords[2] = colinear_data->x;
	  coords[3] = y2;

	  line_points.coords     = coords;
	  line_points.num_points = 2;
	  line_points.ref_count  = 1;

	  colinear_line = foo_canvas_item_new(FOO_CANVAS_GROUP(canvas_item->items[WINDOW_ITEM_UNDERLAY]),
					      foo_canvas_line_get_type(),
					      "width_pixels",   1,
					      "points",         &line_points,
					      "fill_color_gdk", draw_colour,
					      NULL);
	  
	  zmapWindowLongItemCheckPointFull(colinear_line, &line_points, 0.0, 0.0, 0.0, 0.0);
	}
    }

  return ;
}



/* Mark a first or last feature in a series of alignment features as incomplete.
 * The first feature is incomplete if it's start > 1, the last is incomplete
 * if it's end < homol_length.
 * 
 * Code is complicated by markers/coord testing needing to be reversed for
 * alignments to reverse strand. */
static void markMatchIfIncomplete(ZMapWindowCanvasItem collection,
				  ZMapStrand ref_strand, MatchType match_type,
				  FooCanvasItem *item, ZMapFeature feature)
{
  int start, end ;

  if (match_type == FIRST_MATCH)
    {
      start = 1 ;
      end = feature->feature.homol.y1 ;
    }
  else
    {
      start = feature->feature.homol.y2 ;
      end = feature->feature.homol.length ;
    }

  if (start < end)
    {
      double x_coord, y_coord, y_coord_start, y_coord_end ;
      double width = 6.0 ;
      GdkColor *marker_fill=NULL,*marker_draw=NULL,*marker_border=NULL ;
      GdkColor fill,outline ;
      ZMapStyleGlyphType glyph_style ;
      ZMapFeatureTypeStyle style ;

      /* get from style, default to previous hard coded value ('6') */
      glyph_style = zmapWindowCanvasItemGetGlyph(collection);

      /* From style? */
      style = (ZMAP_CANVAS_ITEM_GET_CLASS(collection)->get_style)(collection) ;

      if (!zMapStyleGetColoursGlyphDefault(style,&marker_fill,&marker_draw,&marker_border))
	{
	  gdk_color_parse("red", &fill) ;
	  gdk_color_parse("black", &outline) ;
	  marker_fill = &fill ;
	  marker_border = &outline ;
	}

      x_coord = get_glyph_mid_point(item, width, 
				    NULL, &y_coord_start, NULL, &y_coord_end) ;

      if ((match_type == FIRST_MATCH && ref_strand == ZMAPSTRAND_FORWARD)
	  || (match_type == LAST_MATCH && ref_strand == ZMAPSTRAND_REVERSE))
	{
	  y_coord = ceil(y_coord_start);		/* line_thickness */
	  y_coord = feature->x1 - ((FooCanvasGroup *)collection)->ypos - 1.0 ; /* Ext2Zero */
	}
      else
	{
	  y_coord = floor(y_coord_end);		/* line_thickness */
	  y_coord = feature->x2 - ((FooCanvasGroup *)collection)->ypos;
	}
printf("set glyph: y = %f, x1,x2 = %d,%d ypos = %f\n",y_coord,feature->x1,feature->x2,((FooCanvasGroup *)collection)->ypos);

      foo_canvas_item_new(FOO_CANVAS_GROUP(collection->items[WINDOW_ITEM_OVERLAY]),
			  zMapWindowGlyphItemGetType(),
			  "x",           x_coord,
			  "y",           y_coord,
			  "width",       width,
			  "height",      width,
			  "glyph_style", glyph_style,
			  "line_width",  1,
			  "fill_color_gdk",    marker_fill,
			  "outline_color_gdk", marker_border,
			  NULL);
    }

  return ;
}



static gboolean fragments_splice(char *fragment_a, char *fragment_b)
{
  gboolean splice = FALSE;
  char spliceosome[5];

  if(fragment_a == NULL || fragment_b == NULL)
    {
      splice = FALSE;
    }
  else
    {
      spliceosome[0] = fragment_a[0];
      spliceosome[1] = fragment_a[1];
      spliceosome[2] = fragment_b[0];
      spliceosome[3] = fragment_b[1];
      spliceosome[4] = '\0';
      
      if(g_ascii_strcasecmp(&spliceosome[0], "GTAG") == 0)
	{
	  splice = TRUE;
	}
      else if(g_ascii_strcasecmp(&spliceosome[0], "GCAG") == 0)
	{
	  splice = TRUE;
	}
      else if(g_ascii_strcasecmp(&spliceosome[0], "ATAC") == 0)
	{
	  splice = TRUE;
	}
    }

  if(splice)
    {
      printf("splices: %s\n", &spliceosome[0]);
    }

  return splice;
}

static void process_feature(ZMapFeature prev_feature)
{
  int i;
  gboolean reversed;

  reversed = prev_feature->strand == ZMAPSTRAND_REVERSE;
  
  if(prev_feature->feature.homol.align &&
     prev_feature->feature.homol.align->len > 1)
    {
      ZMapAlignBlock prev_align, curr_align;
      prev_align = &(g_array_index(prev_feature->feature.homol.align, 
				   ZMapAlignBlockStruct, 0));;
      
      for(i = 1; i < prev_feature->feature.homol.align->len; i++)
	{
	  char *prev, *curr;
	  curr_align = &(g_array_index(prev_feature->feature.homol.align, 
				       ZMapAlignBlockStruct, i));
	  
	  if(prev_align->t2 + 4 < curr_align->t1)
	    {
	      prev = zMapFeatureGetDNA((ZMapFeatureAny)prev_feature, 
				       prev_align->t2 + 1, 
				       prev_align->t2 + 2, 
				       reversed);
	      curr = zMapFeatureGetDNA((ZMapFeatureAny)prev_feature,
				       curr_align->t1 - 2,
				       curr_align->t1 - 1,
				       reversed);
	      if(prev && curr)
		fragments_splice(prev, curr);

	      if(prev)
		g_free(prev);
	      if(curr)
		g_free(curr);
	    }
	  
	  prev_align = curr_align;
	}
    }

  return ;
}

