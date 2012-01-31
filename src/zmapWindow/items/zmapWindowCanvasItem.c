/*  File: zmapWindowCanvasItem.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implements the base functions of a zmap canvas item
 *              which is derived from a foocanvas item.
 *
 * Exported functions: See zmapWindowCanvas.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>


#include <string.h>		/* memcpy */
#include <zmapWindowCanvas.h>
#include <zmapWindowFeatures.h>
#include <zmapWindowCanvasItem_I.h>


typedef enum
  {
    WINDOW_CANVAS_ITEM_0,		/* zero == invalid property id */
    WINDOW_CANVAS_ITEM_INTERVAL_TYPE,
    WINDOW_CANVAS_ITEM_FEATURE,
    WINDOW_CANVAS_ITEM_USER_HIDDEN,
    WINDOW_CANVAS_ITEM_CODE_HIDDEN,
    WINDOW_CANVAS_ITEM_DEBUG,
  } WindowCanvasItemType ;


/* Some convenience stuff */
#define GCI_UPDATE_MASK (FOO_CANVAS_UPDATE_REQUESTED | FOO_CANVAS_UPDATE_DEEP)
#define GCI_EPSILON 1e-18

#ifdef RDS_NEVER_INCLUDE

#define DEBUG_BACKGROUND_BOX

#define DEBUG_ITEM_MARK

#endif



typedef struct
{
  ZMapWindowCanvasItem parent;
  ZMapFeature          feature;
  ZMapStyleColourType  style_colour_type;
  int 			colour_flags;
  GdkColor            *default_fill_colour;
  GdkColor            *border_colour;
  ZMapWindowCanvasItemClass klass;
}EachIntervalDataStruct, *EachIntervalData;


static void zmap_window_canvas_item_class_init  (ZMapWindowCanvasItemClass class);
static void zmap_window_canvas_item_init        (ZMapWindowCanvasItem      group);
static void zmap_window_canvas_item_set_property(GObject               *object,
						 guint                  param_id,
						 const GValue          *value,
						 GParamSpec            *pspec);
static void zmap_window_canvas_item_get_property(GObject               *object,
						 guint                  param_id,
						 GValue                *value,
						 GParamSpec            *pspec);
static void zmap_window_canvas_item_destroy     (GtkObject *gtkobject);


//static void zmap_window_canvas_item_post_create(ZMapWindowCanvasItem canvas_item);
static void zmap_window_canvas_item_set_colour(ZMapWindowCanvasItem   canvas_item,
					       FooCanvasItem         *interval,
					       ZMapFeature 	      feature,
					       ZMapFeatureSubPartSpan unused,
					       ZMapStyleColourType    colour_type,
					       int 				colour_flags,
					       GdkColor              *default_fill,
                                     GdkColor              *border);

static ZMapFeatureTypeStyle zmap_window_canvas_item_get_style(ZMapWindowCanvasItem canvas_item);

/* FooCanvasItem interface methods */
static void   zmap_window_canvas_item_update      (FooCanvasItem *item,
						   double           i2w_dx,
						   double           i2w_dy,
						   int              flags);
static void   zmap_window_canvas_item_realize     (FooCanvasItem *item);
static void   zmap_window_canvas_item_unrealize   (FooCanvasItem *item);
static void   zmap_window_canvas_item_map         (FooCanvasItem *item);
static void   zmap_window_canvas_item_unmap       (FooCanvasItem *item);
static void   zmap_window_canvas_item_draw        (FooCanvasItem *item, GdkDrawable *drawable,
						   GdkEventExpose *expose);
static double zmap_window_canvas_item_point       (FooCanvasItem *item, double x, double y,
						   int cx, int cy,
						   FooCanvasItem **actual_item);
static void   zmap_window_canvas_item_translate   (FooCanvasItem *item, double dx, double dy);
static void   zmap_window_canvas_item_bounds      (FooCanvasItem *item, double *x1, double *y1,
						   double *x2, double *y2);
#ifdef WINDOW_CANVAS_ITEM_BOUNDS_REQUIRED
static void window_canvas_item_bounds (FooCanvasItem *item,
				       double *x1, double *y1,
				       double *x2, double *y2);
#endif /* WINDOW_CANVAS_ITEM_BOUNDS_REQUIRED */



static gboolean zmap_window_canvas_item_check_data(ZMapWindowCanvasItem canvas_item, GError **error);

/* A couple of accessories */
static double window_canvas_item_invoke_point (FooCanvasItem *item,
					       double x, double y,
					       int cx, int cy,
					       FooCanvasItem **actual_item);
static gboolean feature_is_drawable(ZMapFeature          feature_any,
				    ZMapFeatureTypeStyle feature_style,
				    ZMapStyleMode       *mode_to_use,
				    GType               *type_to_use);
static void window_canvas_invoke_set_colours(gpointer list_data, gpointer user_data);
static void window_item_feature_destroy(gpointer window_item_feature_data);

static GQuark zmap_window_canvas_item_get_domain(void) ;




/* Globals.... */

static FooCanvasItemClass *group_parent_class_G;
static size_t window_item_feature_size_G = 0;


static gboolean debug_point_method_G = FALSE;

static gboolean debug_item_G = FALSE ;



/*!
 * \brief Get the GType for ZMapWindowCanvasItem
 *
 * \details Registers the ZMapWindowCanvasItem class if necessary, and returns the type ID
 *          associated to it.
 *
 * \return  The type ID of the ZMapWindowCanvasItem class.
 **/

GType zMapWindowCanvasItemGetType (void)
{
  static GType group_type = 0;

  if (!group_type)
    {
      static const GTypeInfo group_info =
	{
	  sizeof (zmapWindowCanvasItemClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_window_canvas_item_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (zmapWindowCanvasItem),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) zmap_window_canvas_item_init,
	  NULL
	};

      group_type = g_type_register_static (foo_canvas_group_get_type (),
					   ZMAP_WINDOW_CANVAS_ITEM_NAME,
					   &group_info,
					   0);
  }

  return group_type;
}

/*!
 * \brief Intended to check the long item status of an item, but I
 * think I've usurped its remit and functionality now.
 *
 * \param canvas_item The ZMapWindowCanvasItem to check
 *
 * \return void
 */
void zMapWindowCanvasItemCheckSize(ZMapWindowCanvasItem canvas_item)
{

  return ;
}


/*!
 * \brief Create a ZMapWindowCanvasItem
 *
 * \details This is a wrapper around foo_canvas_item_new() that
 *          initialises the correct type for a ZMapWindowCanvasItem.
 *          Like foo_canvas_item_new() it returns an object with a
 *          ZMapWindowCanvasItem rather than the objects true type.
 *          The rules for which canvas item type created are encoded
 *          in feature_is_drawable() [internal] and the decision is
 *          based on the feature->type and style->mode pair.
 *
 * \param parent        The FooCanvasGroup which is to be the parent
 * \param feature_start The group->ypos. where is gets drawn in Y.
 * \param feature       The Feature that will be attached.
 * \param feature_style The style that relates to the feature.  This
 *                      is not and should not be held by the canvas item.
 *                      There is a specific interface method for accessing
 *                      the style from the canvas item and feature!
 *
 * \return The newly created ZMapWindowCanvasItem or NULL on failure
 */
ZMapWindowCanvasItem zMapWindowCanvasItemCreate(FooCanvasGroup *parent,
						double feature_start,
						ZMapFeature feature,
						ZMapFeatureTypeStyle feature_style)
{
  ZMapWindowCanvasItem canvas_item = NULL;
  ZMapStyleMode mode;
  GType sub_type;


  if (feature_is_drawable(feature, feature_style, &mode, &sub_type))
    {
      FooCanvasItem *item;

      item = foo_canvas_item_new(parent, sub_type,
				 "x", 0.0,
				 "y", feature_start,
				 NULL);

      if (item && ZMAP_IS_CANVAS_ITEM(item))
	{

	  ZMapWindowCanvasItemClass canvas_item_class ;

	  canvas_item = ZMAP_CANVAS_ITEM(item);
	  canvas_item->feature = feature;

	  canvas_item->debug = debug_item_G ;

	  if (ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->post_create)
	    (* ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->post_create)(canvas_item) ;

	  canvas_item_class = ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item) ;

// done by zmap_window_canvas_item_init()
// NOTE commenting this out doubles the stats!
	  zmapWindowItemStatsIncr(&(canvas_item_class->stats)) ;

	}

    }

  return canvas_item;
}


/*!
 * \brief   simply the opposite of create
 * \code
 *   canvas_item = zMapWindowCanvasItemDestroy(canvas_Item);
 * \endcode
 * \param   canvas_item The ZMapWindowCanvasItem to clean up.
 * \return  The ZMapWindowCanvasItem after it's been cleaned [NULL]
 */
ZMapWindowCanvasItem zMapWindowCanvasItemDestroy(ZMapWindowCanvasItem canvas_item)
{


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("In %s %s()\n", __FILE__, __PRETTY_FUNCTION__) ;

  printf("%s(): gtk_object_destroy() on %p\n",  __PRETTY_FUNCTION__, canvas_item) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* We need to do this rather than g_object_unref as the underlying
   * FooCanvasItem objects are gtk objects... */
  gtk_object_destroy(GTK_OBJECT(canvas_item));

  canvas_item = NULL;

  return canvas_item;
}

/*!
 * \brief   Function to access the ZMapFeature
 * \details Although a little surprising, NULL maybe returned legitimately,
 *          as not all ZMapWindowCanvasItem subclasses require features...
 *          It is architected this way so that we can have _all_ the items
 *          on the canvas below ZMapWindowContainerFeatureSet extend
 *          ZMapWindowCanvasItem .
 * \param   canvas_item The owning ZMapWindowCanvasItem
 * \return  The ZMapFeature or NULL. NULL maybe returned legitimately!
 */
ZMapFeature zMapWindowCanvasItemGetFeature(FooCanvasItem *any_item)
{
  ZMapWindowCanvasItem canvas_item ;
  ZMapFeature feature = NULL ;

  if (ZMAP_IS_CANVAS_ITEM(any_item))
    canvas_item = ZMAP_CANVAS_ITEM(any_item) ;
  else
    canvas_item = zMapWindowCanvasItemIntervalGetObject(any_item) ;

  if (canvas_item)
    feature = canvas_item->feature ;

  return feature ;
}

/* Returns TRUE if any_item is a subpart of a canvas_item, FALSE if the
 * item _is_ the canvas_item. */
gboolean zMapWindowCanvasItemIsSubPart(FooCanvasItem *any_item)
{
  gboolean is_subpart = FALSE ;

  if (!ZMAP_IS_CANVAS_ITEM(any_item) && (zMapWindowCanvasItemIntervalGetObject(any_item)))
    is_subpart = TRUE ;

  return is_subpart ;
}







/*!
 * \brief   Add an "Interval" to the canvas representation.
 *
 * \details An interval is the term I'm using for a region/span,
 *          whatever else... I chose interval over others for no
 *          particular reason other than it was different and easy
 *          to search for in the codebase.
 *
 * \param   canvas_item    The ZMapWindowCanvasItem to add it to.
 * \param   sub_feature_in The ZMapFeatureSubPartSpan that describes this interval in ZMapFeatureContext terms.
 * \param   top            The top coordinate [group relative]
 * \param   bottom         The bottom coordinate [group relative]
 * \param   left           The left coordinate [group relative]
 * \param   right          The right coordinate [group relative]
 *
 * \return  The FooCanvasItem that was created in place of this interval.
 */
FooCanvasItem *zMapWindowCanvasItemAddInterval(ZMapWindowCanvasItem   canvas_item,
					       ZMapFeatureSubPartSpan sub_feature_in,
					       double top,  double bottom,
					       double left, double right)
{
  ZMapWindowCanvasItemClass canvas_item_class;
  ZMapStyleColourType colour_type = ZMAPSTYLE_COLOURTYPE_NORMAL;
  ZMapFeatureSubPartSpanStruct local_struct = {0};
  ZMapFeatureSubPartSpan sub_feature;
  FooCanvasItem *interval = NULL;

  zMapLogReturnValIfFail(ZMAP_IS_CANVAS_ITEM(canvas_item), interval);

  canvas_item_class = ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item);

  if(canvas_item_class->add_interval)
    {
      if(sub_feature_in)
	sub_feature = sub_feature_in;
      else
	{
	  sub_feature = &local_struct;
	  sub_feature->subpart = ZMAPFEATURE_SUBPART_INVALID;
	}

      if(canvas_item->debug == TRUE)
	{
	  ZMapFeature feature;
	  feature = canvas_item->feature;
	  printf("->AddInterval(canvas_item=%p{unique_id=%s}, sub_feature=*{%d,%d,%d}, top=%f, bot=%f, left=%f, right%f)\n",
		 canvas_item, (feature ? g_quark_to_string(feature->unique_id) : "<no-feature>"),
		 sub_feature->subpart, sub_feature->start,
		 sub_feature->end, top, bottom, left, right);
	}

      interval = (* canvas_item_class->add_interval)(canvas_item, sub_feature,
						     top, bottom, left, right);

      if(interval && ZMAP_IS_CANVAS(interval->canvas))
	{
	  FooCanvasItem *long_interval;

	  long_interval = zmapWindowLongItemCheckPoint(interval);

	  if(long_interval != interval)
	    {
	      interval = long_interval;
	    }

	  if(sub_feature_in)
	    {
	      /* This means we can be the creators and destroyers of this data,
	       * callers need not be concerned in any way and can pass in
	       * pointers to structs.  We could use g_slice_alloc too! */
	      if((sub_feature = g_slice_alloc(window_item_feature_size_G)))
		{
		  memcpy(sub_feature, sub_feature_in, window_item_feature_size_G);
		  g_object_set_data_full(G_OBJECT(interval), ITEM_SUBFEATURE_DATA,
					 sub_feature, window_item_feature_destroy);
		}
	    }

	  if(canvas_item_class->set_colour)
	    {
	      (* canvas_item_class->set_colour)(canvas_item, interval, canvas_item->feature, sub_feature,
						colour_type, 0,  NULL,NULL);
	    }

	}
    }



  return interval;
}

/*!
 * \brief   Designed to fix a buglet in FooCanvasItem get_bounds()
 * \details Size of Foo canvas items are zero while not being mapped.
 *          This is not really sane for when you want to position items
 *          before mapping occurs... This function fixes that.
 * \param   canvas_item ZMapWindowCanvasItem you want the size of
 * \param   x1_out      The address to store the x1 coord [item relative] in.
 * \param   y1_out      The address to store the y1 coord [item relative] in.
 * \param   x2_out      The address to store the x2 coord [item relative] in.
 * \param   y2_out      The address to store the y2 coord [item relative] in.
 * \return  void
 */
void zMapWindowCanvasItemGetBumpBounds(ZMapWindowCanvasItem canvas_item,
				       double *x1_out, double *y1_out,
				       double *x2_out, double *y2_out)
{
  double x1, x2;

  zmap_window_canvas_item_bounds(FOO_CANVAS_ITEM(canvas_item), &x1, y1_out, &x2, y2_out);

  if(x1_out && x2_out)
    {
      ZMapFeatureTypeStyle style;
      double width;

      style = (ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->get_style)(canvas_item);

      width = zMapStyleGetWidth(style);

      if(0 && x1 + width > x2)
	x2 = x1 + width;

      if(x1_out)
	*x1_out = x1;
      if(x2_out)
	*x2_out = x2;
    }

  return ;
}

/*!
 * \brief unused?
 */
gboolean zMapWindowCanvasItemCheckData(ZMapWindowCanvasItem canvas_item, GError **error)
{
  gboolean result = TRUE;

  if(result && !canvas_item->feature)
    {
      *error = g_error_new(zmap_window_canvas_item_get_domain(), 1,
			   "No ZMapFeatureAny attached to item");

      result = FALSE;
    }

  if(result && ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->check_data)
    {
      result = ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->check_data(canvas_item, error);
    }

  return result;
}

/*!
 * \brief   Calls the interface function clear
 * \details Designed to destroy all the child FooCanvasItems the ZMapWindowCanvasItem knows
 *          about.  As the ZMapWindowCanvasItem has the ZMapFeature, in theory it could
 *          redraw itself. That's where the idea for this comes from.
 * \param   canvas_item The ZMapWindowCanvasItem to clear
 * \return  void
 *
 * \code
 *   // currently fantasy psuedo code
 *   transcript_feature = zMapWindowCanvasItemGetFeature(canvas_item);
 *   zMapWindowCanvasItemClear(canvas_item);
 *   feature_style = find_feature_style(transcript_feature);
 *   alter_style(feature_style, "cds_colour", &blue, NULL);
 *   zMapWindowCanvasItemDraw(canvas_item);
 * \endcode
 */
void zMapWindowCanvasItemClear(ZMapWindowCanvasItem canvas_item)
{
  if(ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->clear)
    {
      ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->clear(canvas_item);
    }

  return ;
}



ZMapFeatureSubPartSpan zMapWindowCanvasItemIntervalGetData(FooCanvasItem *item)
{
  ZMapFeatureSubPartSpan sub_feature;

  sub_feature = g_object_get_data(G_OBJECT(item), ITEM_SUBFEATURE_DATA);

  return sub_feature;
}

FooCanvasItem *zMapWindowCanvasItemGetInterval(ZMapWindowCanvasItem canvas_item,
					       double x, double y,
					       ZMapFeatureSubPartSpan *sub_feature_out)
{
  FooCanvasItem *matching_interval = NULL;
  FooCanvasItem *point_item;
  FooCanvasItem *item, *check;
  FooCanvasGroup *group;
  FooCanvas *canvas;
  GList *list;
  double ix1, ix2, iy1, iy2;
  double wx1, wx2, wy1, wy2;
  double x1, x2, y1, y2;
  double gx, gy, best, dist;
  int cx, cy, i = 0;
  int has_point, small_enough;

  zMapLogReturnValIfFail(ZMAP_IS_CANVAS_ITEM(canvas_item), matching_interval);

  /* The background can be clipped by the long items code, so we
   * need to use the groups position as the background extends
   * this far really! */

  item   = (FooCanvasItem *)canvas_item;
  group  = (FooCanvasGroup *)item;
  canvas = item->canvas;
  gx = x;
  gy = y;

  foo_canvas_item_w2i(item, &gx, &gy);

  foo_canvas_w2c(canvas, x, y, &cx, &cy);

  if(debug_point_method_G)
    printf("GetInterval like ->point(canvas_item=%p, gx=%f, gy=%f, cx=%d, cy=%d)\n", item, gx, gy, cx, cy);

  foo_canvas_item_get_bounds(item, &ix1, &iy1, &ix2, &iy2);
  wx1 = ix1;
  wy1 = iy1;
  wx2 = ix2;
  wy2 = iy2;
  foo_canvas_item_i2w(item, &wx1, &wy1);
  foo_canvas_item_i2w(item, &wx2, &wy2);

  if(debug_point_method_G)
    printf("ItemCoords(%f,%f,%f,%f), WorldCoords(%f,%f,%f,%f)\n",
	   ix1, iy1, ix2, iy2,
	   wx1, wy1, wx2, wy2);

  dist = window_canvas_item_invoke_point(item, gx, gy, cx, cy, &check);

  small_enough = (int)(dist * canvas->pixels_per_unit_x + 0.5);

  if(small_enough <= canvas->close_enough)
    {
      /* we'll continue! */
      gx -= group->xpos;
      gy -= group->ypos;

      if(debug_point_method_G)
	printf("GetInterval like ->point(canvas_item=%p, gx=%f, gy=%f, cx=%d, cy=%d)\n", item, gx, gy, cx, cy);

      x1 = cx - canvas->close_enough;
      y1 = cy - canvas->close_enough;
      x2 = cx + canvas->close_enough;
      y2 = cy + canvas->close_enough;

      best = 1e18;

      /* Go through each interval (FooCanvasGroup's item list).
       * Backgrounds/overlays/underlays not in this list! */
      for (list = group->item_list; list; i++, list = list->next)
	{
	  FooCanvasItem *child;

	  child = list->data;

	  if(debug_point_method_G)
	    printf("examining child[%d]\n", i);

	  /* We actaully want the full width of the item here... lines and score by width columns... */
	  /* filtering only by y coords, effectively... */
	  if ((child->y1 > y2) || (child->y2 < y1))
	    continue;

	  point_item = NULL; /* cater for incomplete item implementations */

	  if(debug_point_method_G)
	    {
	      printf("further examination...");

	      foo_canvas_item_get_bounds(child, &ix1, &iy1, &ix2, &iy2);
	      wx1 = ix1;
	      wy1 = iy1;
	      wx2 = ix2;
	      wy2 = iy2;
	      foo_canvas_item_i2w(child, &wx1, &wy1);
	      foo_canvas_item_i2w(child, &wx2, &wy2);
	      printf("ItemCoords(%f,%f,%f,%f), WorldCoords(%f,%f,%f,%f)\n",
		     ix1, iy1, ix2, iy2,
		     wx1, wy1, wx2, wy2);
	    }

	  /* filter out unmapped items */
	  if ((child->object.flags & FOO_CANVAS_ITEM_MAPPED)
	      && FOO_CANVAS_ITEM_GET_CLASS (child)->point)
	    {
	      /* There's some quirks here to get round for lines and rectangles with no fill... */

	      /* We could just check with y coords of item, but prefer to use the
	       * foo_canvas_item_point() code. This ensures FooCanvasGroups can be
	       * intervals. Useful? */

	      if(FOO_IS_CANVAS_RE(child))
		{
		  int save_fill_set;
		  save_fill_set = FOO_CANVAS_RE(child)->fill_set;
		  FOO_CANVAS_RE(child)->fill_set = 1;
		  /* foo_canvas_re_point() has a bug/feature where "empty" rectangles
		   * points only include the outline. Temporarily setting fill_set
		   * subverts this... */
		  dist = window_canvas_item_invoke_point (child, gx, gy, cx, cy, &point_item);
		  FOO_CANVAS_RE(child)->fill_set = save_fill_set;
		  if(debug_point_method_G)
		    printf("child[%d] is RE has dist=%f ", i, dist);
		}
	      else if(FOO_IS_CANVAS_LINE(child))
		{
		  dist = window_canvas_item_invoke_point (child, gx, gy, cx, cy, &point_item);
		  /* If the is distance is less than the width then it must be this line... */
		  if(dist < ix2 - ix1)
		    dist = 0.0;
		  if(debug_point_method_G)
		    printf("child[%d] is LINE has dist=%f ", i, dist);
		}
	      else if(FOO_IS_CANVAS_TEXT(child))
		{
		  point_item = child;
		  dist = 0.0; /* text point is broken. the x factor is _not_ good enough. */
		  if(debug_point_method_G)
		    printf("child[%d] is TEXT has dist=%f ", i, dist);
		}
	      else if(ZMAP_IS_WINDOW_GLYPH_ITEM(child))
		{
		  /* glyph items are quite complex and really need to check bounds rather than point. */
		  point_item = child;
		  dist = 0.0;
		  if(debug_point_method_G)
		    printf("child[%d] is GLYPH has dist=%f ", i, dist);
		}
	      else
	      {
	      		/* enables groups to be included in the ZMapWindowCanvasItem tree */
			dist = window_canvas_item_invoke_point (child, gx, gy, cx, cy, &point_item);
		}

	      has_point = TRUE;
	    }
	  else
	    has_point = FALSE;

	  /* guessing that the x factor is OK here. RNGC. */
	  /* Well it's ok, but not ideal... RDS */
	  if (has_point && point_item && best >= dist)
	    {
	      int small_enough;

	      small_enough = (int) (dist * canvas->pixels_per_unit_y + 0.5);

	      if(small_enough <= canvas->close_enough)
		{
		  if(debug_point_method_G)
		    printf("child[%d] small enough (%d)\n", i, small_enough);

		  best = dist;
		  matching_interval = point_item;
		}
	      else if(debug_point_method_G)
		printf("child[%d] _not_ small enough (%d)\n", i, small_enough);
	    }
	  else if(debug_point_method_G)
	    printf("child[%d] has no point\n", i);
	}

      if(matching_interval == NULL)
	{
	  /* we could check start and end and pick the most relevant... */
	  /* but we'll just pick the first */
	  if(group->item_list)
	    matching_interval = (FooCanvasItem *)(group->item_list->data);

	}
    }

  if(matching_interval == NULL)
    g_warning("No matching interval!");
  else if(sub_feature_out)
  {
  	if(ZMAP_IS_WINDOW_FEATURESET_ITEM(matching_interval))
  	{
  		/* returns a static dara structure */
  		*sub_feature_out =
  			zMapWindowCanvasFeaturesetGetSubPartSpan(matching_interval, zMapWindowCanvasItemGetFeature(item) ,x,y);
  	}
  	else
  		*sub_feature_out = g_object_get_data(G_OBJECT(matching_interval), ITEM_SUBFEATURE_DATA);
  }

  return matching_interval;
}



/* for composite items (eg ZMapWindowGraphDensity)
 * set the pointed at feature to be the one nearest to cursor
 * We need this to stabilise the feature while processing a mouse click (item/feature select)
 * point used to set this bu the cursor is still moving while we trundle thro the code
 * causing the feature to change with unpredictable results
 * write up in density.html when i get round to correcting it.
 */
gboolean zMapWindowCanvasItemSetFeature(ZMapWindowCanvasItem item, double x, double y)
{
	ZMapWindowCanvasItemClass class = ZMAP_CANVAS_ITEM_GET_CLASS(item);
	gboolean ret = FALSE;

	if(class->set_feature)
		ret = class->set_feature((FooCanvasItem *) item,x,y);

	return ret;
}


/* a pointless function created due to scope issues */
gboolean zMapWindowCanvasItemSetFeaturePointer(ZMapWindowCanvasItem item, ZMapFeature feature)
{
	int pop = 0;

	/* collpased features have 0 population, the one that was displayed has the total
	 * if we use window seacrh then select a collapsed feature we have to update this
	 * see zmapViewfeatureCollapse.c etc; scan for 'population' and 'collasped'
	 */

	if(item->feature)
		pop = item->feature->population;

	if(feature->flags.collapsed)
		feature->population = pop;

	item->feature = feature;
	return(TRUE);
}

gboolean zMapWindowCanvasItemSetStyle(ZMapWindowCanvasItem item, ZMapFeatureTypeStyle style)
{
	ZMapWindowCanvasItemClass class = ZMAP_CANVAS_ITEM_GET_CLASS(item);
	gboolean ret = FALSE;

	if(class->set_style)
		ret = class->set_style((FooCanvasItem *) item, style);

	return ret;
}

/* like foo but for a ZMap thingy */
gboolean zMapWindowCanvasItemShowHide(ZMapWindowCanvasItem item, gboolean show)
{
	ZMapWindowCanvasItemClass class = ZMAP_CANVAS_ITEM_GET_CLASS(item);
	gboolean ret = FALSE;

	if(class->showhide)
		ret = class->showhide((FooCanvasItem *) item, show);
	else
	{
		if(show)
			foo_canvas_item_show(FOO_CANVAS_ITEM(item));
		else
			foo_canvas_item_hide(FOO_CANVAS_ITEM(item));
	}

	return ret;

}




/* Get at parent... */
ZMapWindowCanvasItem zMapWindowCanvasItemIntervalGetObject(FooCanvasItem *item)
{
  ZMapWindowCanvasItem canvas_item = NULL;

  if(ZMAP_IS_CANVAS_ITEM(item))
    canvas_item = ZMAP_CANVAS_ITEM(item);
  else if(FOO_IS_CANVAS_ITEM(item) && item->parent)
    {
      if(ZMAP_IS_CANVAS_ITEM(item->parent))
	canvas_item = ZMAP_CANVAS_ITEM(item->parent);
    }

  return canvas_item;
}

ZMapWindowCanvasItem zMapWindowCanvasItemIntervalGetTopLevelObject(FooCanvasItem *item)
{
  ZMapWindowCanvasItem canvas_item = NULL;

  canvas_item = zMapWindowCanvasItemIntervalGetObject(item) ;

  return canvas_item ;
}


GList *zMapWindowCanvasItemGetChildren(ZMapWindowCanvasItem *parent)
{
  GList *children =  NULL ;
  FooCanvasGroup *foo_group ;

  if (ZMAP_IS_CANVAS_ITEM(parent)
      && FOO_IS_CANVAS_GROUP(parent)
      && (foo_group = FOO_CANVAS_GROUP(parent)))
      children = foo_group->item_list ;

  return children ;
}

gboolean zMapWindowCanvasItemIsMasked(ZMapWindowCanvasItem item,gboolean andHidden)
{
      ZMapFeature feature;
      ZMapFeatureTypeStyle style;

      feature = item->feature;
      style = feature->style;

      if(style->mode == ZMAPSTYLE_MODE_ALIGNMENT && feature->feature.homol.flags.masked)
      {
            if(andHidden && feature->feature.homol.flags.displayed)
                  return FALSE;
            return TRUE;
      }
      return FALSE;
}



/* If item is the parent item then the whole feature is coloured, otherwise just the sub-item
 * is coloured... */
void zMapWindowCanvasItemSetIntervalColours(FooCanvasItem *item, ZMapFeature feature,
					    ZMapStyleColourType colour_type,
					    int colour_flags,
                                  GdkColor *default_fill_colour,
                                  GdkColor *border_colour)
{
  ZMapWindowCanvasItem canvas_item ;
  GList *item_list, dummy_item = {NULL} ;
  EachIntervalDataStruct interval_data = {NULL};


  zMapLogReturnIfFail(FOO_IS_CANVAS_ITEM(item)) ;

  if (ZMAP_IS_CANVAS_ITEM(item))
    {
      canvas_item = ZMAP_CANVAS_ITEM(item) ;
      item_list = FOO_CANVAS_GROUP(canvas_item)->item_list ;
    }
  else
    {
      canvas_item = zMapWindowCanvasItemIntervalGetTopLevelObject(item) ;
      dummy_item.data = item ;
      item_list = &dummy_item ;
    }

  /* Oh gosh...why is this code in here....it isn't up to the object to decide if its raised..... */
  /* mh17: i move this code into zmapWindowFocus.c
   * which is where stuff should be raised/lowered
   * here is caused a bug scanning through the list and using SELECTED rather than NORMAL
   * raise/lower re-orders the list
   * it would be better to copy to the overlay list for items on top
   * Given that the old focus code remembered an item's place so as to be able to put it back
   * this is all very odd.
   */
  /*
  if(colour_type == ZMAPSTYLE_COLOURTYPE_SELECTED)
    foo_canvas_item_raise_to_top(FOO_CANVAS_ITEM(canvas_item)) ;
  else
    foo_canvas_item_lower_to_bottom(FOO_CANVAS_ITEM(canvas_item)) ;
   */

  interval_data.parent = canvas_item ;
  interval_data.feature = feature; // zMapWindowCanvasItemGetFeature(FOO_CANVAS_ITEM(canvas_item)) ;
  interval_data.style_colour_type = colour_type ;
  interval_data.colour_flags = colour_flags;
  interval_data.default_fill_colour = default_fill_colour ;
  interval_data.border_colour = border_colour ;
  interval_data.klass = ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item) ;

  g_list_foreach(item_list, window_canvas_invoke_set_colours, &interval_data) ;

  return ;
}






/*
 *                Internals routines
 */


static GQuark zmap_window_canvas_item_get_domain(void)
{
  GQuark domain;

  domain = g_quark_from_string(ZMAP_WINDOW_CANVAS_ITEM_NAME);

  return domain;
}





static void redraw_and_repick_if_mapped (FooCanvasItem *item)
{
  if (item->object.flags & FOO_CANVAS_ITEM_MAPPED)
    {
      foo_canvas_item_request_redraw (item);
      item->canvas->need_repick = TRUE;
    }

  return ;
}

static int is_descendant (FooCanvasItem *item, FooCanvasItem *parent)
{
  for (; item; item = item->parent)
    {
      if (item == parent)
	return TRUE;
    }

  return FALSE;
}

/* Adds an item to a group */
static void group_add (FooCanvasGroup *group, FooCanvasItem *item)
{
  g_object_ref (GTK_OBJECT (item));
  gtk_object_sink (GTK_OBJECT (item));

  if (!group->item_list)
    {
      group->item_list = g_list_append (group->item_list, item);
      group->item_list_end = group->item_list;
    }
  else
    group->item_list_end = g_list_append (group->item_list_end, item)->next;

  if (item->object.flags & FOO_CANVAS_ITEM_VISIBLE &&
      group->item.object.flags & FOO_CANVAS_ITEM_MAPPED) {
    if (!(item->object.flags & FOO_CANVAS_ITEM_REALIZED))
      (* FOO_CANVAS_ITEM_GET_CLASS (item)->realize) (item);

    if (!(item->object.flags & FOO_CANVAS_ITEM_MAPPED))
      (* FOO_CANVAS_ITEM_GET_CLASS (item)->map) (item);
  }
  return ;
}

/* Removes an item from a group */
static void group_remove (FooCanvasGroup *group, FooCanvasItem *item)
{
  GList *children;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("In %s %s()\n", __FILE__, __PRETTY_FUNCTION__) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  zMapLogReturnIfFail (FOO_IS_CANVAS_GROUP (group));
  zMapLogReturnIfFail (FOO_IS_CANVAS_ITEM (item));

  for (children = group->item_list; children; children = children->next)
    {
      if (children->data == item)
	{
	  if (item->object.flags & FOO_CANVAS_ITEM_MAPPED)
	    (* FOO_CANVAS_ITEM_GET_CLASS (item)->unmap) (item);

	  if (item->object.flags & FOO_CANVAS_ITEM_REALIZED)
	    (* FOO_CANVAS_ITEM_GET_CLASS (item)->unrealize) (item);

	  /* Unparent the child */

	  item->parent = NULL;
	  /* item->canvas = NULL; */ /* Just to comment this, without rebuilding! */
	  g_object_unref (item);

	  /* Remove it from the list */

	  if (children == group->item_list_end)
	    group->item_list_end = children->prev;

	  group->item_list = g_list_remove_link (group->item_list, children);
	  g_list_free (children);
	  break;
	}
    }

  return ;
}

void zMapWindowCanvasItemReparent(FooCanvasItem *item, FooCanvasGroup *new_group)
{
  zMapLogReturnIfFail (FOO_IS_CANVAS_ITEM (item));
  zMapLogReturnIfFail (FOO_IS_CANVAS_GROUP (new_group));

  /* Both items need to be in the same canvas */
  zMapLogReturnIfFail (item->canvas == FOO_CANVAS_ITEM (new_group)->canvas);

  /* The group cannot be an inferior of the item or be the item itself --
   * this also takes care of the case where the item is the root item of
   * the canvas.  */
  zMapLogReturnIfFail (!is_descendant (FOO_CANVAS_ITEM (new_group), item));

  /* Everything is ok, now actually reparent the item */

  g_object_ref (GTK_OBJECT (item)); /* protect it from the unref in group_remove */

  foo_canvas_item_request_redraw (item);

  group_remove (FOO_CANVAS_GROUP (item->parent), item);
  item->parent = FOO_CANVAS_ITEM (new_group);
  /* item->canvas is unchanged.  */
  group_add (new_group, item);

  /* Redraw and repick */

  redraw_and_repick_if_mapped (item);

  g_object_unref (GTK_OBJECT (item));

  return ;
}




/*
 *                Internal routines.
 */


/* Class initialization function for ZMapWindowCanvasItemClass */
static void zmap_window_canvas_item_class_init (ZMapWindowCanvasItemClass window_class)
{
  GObjectClass *gobject_class;
  GtkObjectClass *object_class;
  FooCanvasItemClass *item_class;
  GType canvas_item_type, parent_type;
  int make_clickable_bmp_height  = 4;
  int make_clickable_bmp_width   = 16;
  char make_clickable_bmp_bits[] =
    {
      0x00, 0x00,
      0x00, 0x00,
      0x00, 0x00,
      0x00, 0x00
    } ;

  gobject_class = (GObjectClass *)window_class;
  object_class  = (GtkObjectClass *)window_class;
  item_class    = (FooCanvasItemClass *)window_class;

  canvas_item_type = g_type_from_name(ZMAP_WINDOW_CANVAS_ITEM_NAME);
  parent_type      = g_type_parent(canvas_item_type);
  group_parent_class_G = gtk_type_class(parent_type);

  gobject_class->set_property = zmap_window_canvas_item_set_property;
  gobject_class->get_property = zmap_window_canvas_item_get_property;

  g_object_class_install_property(gobject_class, WINDOW_CANVAS_ITEM_INTERVAL_TYPE,
				  g_param_spec_uint("interval-type", "interval type",
						    "The type to use when creating a new interval",
						    0, 127, 0, ZMAP_PARAM_STATIC_WO));

  g_object_class_install_property(gobject_class, WINDOW_CANVAS_ITEM_FEATURE,
				  g_param_spec_pointer("feature", "feature",
						       "ZMapFeatureAny pointer",
						       ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class, WINDOW_CANVAS_ITEM_USER_HIDDEN,
				  g_param_spec_boolean("user-hidden", "user hidden",
						       "Item was hidden by a user action.",
						       FALSE, ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class, WINDOW_CANVAS_ITEM_CODE_HIDDEN,
				  g_param_spec_boolean("code-hidden", "code hidden",
						       "Item was hidden by coding logic.",
						       FALSE, ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class, WINDOW_CANVAS_ITEM_DEBUG,
				  g_param_spec_boolean("debug", "debug",
						       "Turn on debug printing.",
						       FALSE, ZMAP_PARAM_STATIC_RW));


  object_class->destroy = zmap_window_canvas_item_destroy;

  item_class->update    = zmap_window_canvas_item_update;
  item_class->realize   = zmap_window_canvas_item_realize;
  item_class->unrealize = zmap_window_canvas_item_unrealize;
  item_class->map       = zmap_window_canvas_item_map;
  item_class->unmap     = zmap_window_canvas_item_unmap;

  item_class->draw      = zmap_window_canvas_item_draw;
  item_class->point     = zmap_window_canvas_item_point;
  item_class->translate = zmap_window_canvas_item_translate;
  item_class->bounds    = zmap_window_canvas_item_bounds;

//  window_class->post_create  = zmap_window_canvas_item_post_create;
  window_class->check_data   = zmap_window_canvas_item_check_data;
  window_class->set_colour   = zmap_window_canvas_item_set_colour;
  window_class->get_style    = zmap_window_canvas_item_get_style;

  window_class->fill_stipple = gdk_bitmap_create_from_data(NULL, &make_clickable_bmp_bits[0],
							   make_clickable_bmp_width,
							   make_clickable_bmp_height) ;

  window_item_feature_size_G = sizeof(ZMapFeatureSubPartSpanStruct);

  /* init the stats fields. */
  zmapWindowItemStatsInit(&(window_class->stats), ZMAP_TYPE_CANVAS_ITEM) ;

  return ;
}


/* Object initialization function for ZMapWindowCanvasItem */
static void zmap_window_canvas_item_init (ZMapWindowCanvasItem canvas_item)
{
  FooCanvasGroup *group;
  ZMapWindowCanvasItemClass canvas_item_class ;

  canvas_item_class = ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item) ;

  group = FOO_CANVAS_GROUP(canvas_item);
  group->xpos = 0.0;
  group->ypos = 0.0;

  zmapWindowItemStatsIncr(&(canvas_item_class->stats)) ;

  return ;
}


/* Set_property handler for canvas groups */
static void zmap_window_canvas_item_set_property (GObject *gobject, guint param_id,
						  const GValue *value, GParamSpec *pspec)
{
  FooCanvasItem *item;
  FooCanvasGroup *group;
  gboolean moved;

  zMapLogReturnIfFail (FOO_IS_CANVAS_GROUP (gobject));

  item  = FOO_CANVAS_ITEM (gobject);
  group = FOO_CANVAS_GROUP (gobject);

  moved = FALSE;

  switch (param_id)
    {
    case WINDOW_CANVAS_ITEM_INTERVAL_TYPE:
      {
	guint type = 0;

	type = g_value_get_uint(value);

	switch(type)
	  {
	  default:
	    g_warning("%s", "interval-type is expected to be overridden");
	    break;
	  }
      }
      break;
    case WINDOW_CANVAS_ITEM_FEATURE:
      {
	ZMapWindowCanvasItem canvas_item;

	canvas_item = ZMAP_CANVAS_ITEM(gobject);

	canvas_item->feature = g_value_get_pointer(value);
      }
      break;
    case WINDOW_CANVAS_ITEM_DEBUG:
      {
	ZMapWindowCanvasItem canvas_item;

	canvas_item = ZMAP_CANVAS_ITEM(gobject);

	canvas_item->debug = g_value_get_boolean(value);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
      break;
    }

  return ;
}

/* Get_property handler for canvas groups */
static void zmap_window_canvas_item_get_property (GObject *gobject, guint param_id,
						  GValue *value, GParamSpec *pspec)
{
  FooCanvasItem *item;
  FooCanvasGroup *group;

  zMapLogReturnIfFail (FOO_IS_CANVAS_GROUP (gobject));

  item  = FOO_CANVAS_ITEM (gobject);
  group = FOO_CANVAS_GROUP (gobject);

  switch (param_id)
    {
    case WINDOW_CANVAS_ITEM_FEATURE:
      {
	ZMapWindowCanvasItem canvas_item;

	canvas_item = ZMAP_CANVAS_ITEM(gobject);

	g_value_set_pointer(value, canvas_item->feature);

	break;
      }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
      break;
    }

  return ;
}

/* Destroy handler for canvas groups */
static void zmap_window_canvas_item_destroy (GtkObject *gtkobject)
{
  ZMapWindowCanvasItem canvas_item;
  ZMapWindowCanvasItemClass canvas_item_class ;
//  int i;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("In %s %s()\n", __FILE__, __PRETTY_FUNCTION__) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  canvas_item = ZMAP_CANVAS_ITEM(gtkobject);
  canvas_item_class = ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item) ;


  canvas_item->feature = NULL;

  if(GTK_OBJECT_CLASS (group_parent_class_G)->destroy)
    (GTK_OBJECT_CLASS (group_parent_class_G)->destroy)(GTK_OBJECT(gtkobject));

  zmapWindowItemStatsDecr(&(canvas_item_class->stats)) ;

  return ;
}

#ifdef NEVER_INCLUDE_DEBUG_EVENTS
static gboolean canvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  switch(event->type)
    {
    case GDK_BUTTON_PRESS:
      {
	FooCanvasRE *rect;
	double ix1, ix2, iy1, iy2;
	double wx1, wx2, wy1, wy2;

	rect = FOO_CANVAS_RE(item);

	foo_canvas_item_get_bounds(item, &ix1, &iy1, &ix2, &iy2);

	wx1 = ix1;
	wx2 = ix2;
	wy1 = iy1;
	wy2 = iy2;

	foo_canvas_item_i2w(item, &wx1, &wy1);
	foo_canvas_item_i2w(item, &wx2, &wy2);

	printf("Scroll: (%f,%f)\n", item->canvas->scroll_y1, item->canvas->scroll_y2);
	printf("Rect: (%f,%f), Item: (%f,%f)\n", rect->y1, rect->y2, item->y1, item->y2);
	printf("Item: (%f,%f), World: (%f, %f)\n", iy1, iy2, wy1, wy2);

      }
      break;
    default:
      break;
    }
  return FALSE;
}
#endif /* NEVER_INCLUDE_DEBUG_EVENTS */




static ZMapFeatureTypeStyle zmap_window_canvas_item_get_style(ZMapWindowCanvasItem canvas_item)
{
  ZMapFeatureTypeStyle style = NULL;
  FooCanvasItem *item;
  ZMapWindowCanvasItem canvas_item_parent = NULL;

  zMapLogReturnValIfFail(canvas_item != NULL, NULL);
  zMapLogReturnValIfFail(canvas_item->feature != NULL, NULL);

  style = canvas_item->feature->style;

  if(!style)
  {
  zMapLogWarning("%s","legacy get_style() code called");

  canvas_item_parent = zMapWindowCanvasItemIntervalGetTopLevelObject((FooCanvasItem *)canvas_item);

  item = FOO_CANVAS_ITEM(canvas_item_parent);

  if(item->parent && item->parent->parent)
    {
      style = canvas_item->feature->style;
    }
  }
  return style;
}



/* Update handler for canvas groups */
static void zmap_window_canvas_item_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags)
{
  FooCanvasGroup *canvas_group;
  gboolean visible;
  double xpos, ypos;

  visible = ((item->object.flags & FOO_CANVAS_ITEM_VISIBLE) == FOO_CANVAS_ITEM_VISIBLE) ;

  /* chain up, to update all the interval items that are legitimately part of the group, first... */
  (* group_parent_class_G->update) (item, i2w_dx, i2w_dy, flags);

  canvas_group = FOO_CANVAS_GROUP(item);

  xpos = canvas_group->xpos;
  ypos = canvas_group->ypos;


  return ;
}



static void zmap_window_canvas_item_realize (FooCanvasItem *item)
{

  (* group_parent_class_G->realize)(item);

  return ;
}


/* Unrealize handler for canvas groups */
static void zmap_window_canvas_item_unrealize (FooCanvasItem *item)
{

  (* group_parent_class_G->unrealize) (item);

  return ;
}

/* Map handler for canvas groups */
static void zmap_window_canvas_item_map (FooCanvasItem *item)
{
  ZMapWindowCanvasItem canvas_item;

  canvas_item = ZMAP_CANVAS_ITEM(item);

  if (canvas_item->debug)
    {
      printf("%p - zmap_window_canvas_item_map: canvas_item->x1=%g, canvas_item->y1=%g, canvas_item->x2=%g, canvas_item->y2=%g\n",
	     item, item->x1, item->y1, item->x2, item->y2);

    }

  (* group_parent_class_G->map) (item);

  return ;
}

/* Unmap handler for canvas groups */
static void zmap_window_canvas_item_unmap (FooCanvasItem *item)
{

  (* group_parent_class_G->unmap) (item);


  return ;
}

/* Draw handler for canvas groups */
static void zmap_window_canvas_item_draw (FooCanvasItem *item, GdkDrawable *drawable,
					  GdkEventExpose *expose)
{
  ZMapWindowCanvasItem canvas_item;
  FooCanvas *foo_canvas;

  foo_canvas = item->canvas;

  zMapLogReturnIfFail(ZMAP_IS_CANVAS_ITEM(item));

  canvas_item = ZMAP_CANVAS_ITEM(item);


#if MH17_REVCOMP_DEBUG > 1
      zMapLogWarning("canvas item draw %s %p %f,%f - %f,%f",
            g_quark_to_string(canvas_item->feature->unique_id),
            item->canvas,item->y1,item->x1,item->y2,item->x2) ;
#endif


  if (canvas_item->debug)
    {
      printf("%p - zmap_window_canvas_item_draw: canvas_item->x1=%g, canvas_item->y1=%g, canvas_item->x2=%g, canvas_item->y2=%g\n",
	     item, item->x1, item->y1, item->x2, item->y2);

    }


  (* group_parent_class_G->draw)(item, drawable, expose) ;

  return ;
}


/* Point handler for canvas groups */
static double zmap_window_canvas_item_point (FooCanvasItem *item, double x, double y, int cx, int cy,
					     FooCanvasItem **actual_item)
{
    return (*group_parent_class_G->point)(item, x, y, cx, cy, actual_item) ;
}

static void zmap_window_canvas_item_translate (FooCanvasItem *item, double dx, double dy)
{
  FooCanvasGroup *group;

  group = FOO_CANVAS_GROUP (item);

  group->xpos += dx;
  group->ypos += dy;

  return ;
}

/* Bounds handler for canvas groups */
static void zmap_window_canvas_item_bounds (FooCanvasItem *item,
					    double *x1_out, double *y1_out,
					    double *x2_out, double *y2_out)
{

  if(ZMAP_IS_CANVAS_ITEM(item))
    {
      double x1, x2, y1, y2;

	{
	      if (!(item->object.flags & FOO_CANVAS_ITEM_REALIZED))
		(*group_parent_class_G->realize)(item);

	      if (!(item->object.flags & FOO_CANVAS_ITEM_MAPPED))
		(*group_parent_class_G->map)(item);

	      (*group_parent_class_G->bounds)(item, &x1, &y1, &x2, &y2) ;

	}

      if(x1_out)
      *x1_out = x1;
      if(x2_out)
      *x2_out = x2;

      if(y1_out)
      *y1_out = y1;
      if(y2_out)
      *y2_out = y2;
    }

  return ;
}

#ifdef WINDOW_CANVAS_ITEM_BOUNDS_REQUIRED
static void window_canvas_item_bounds (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
  (* group_parent_class_G->bounds)(item, x1, y1, x2, y2);
  return ;
}
#endif /* WINDOW_CANVAS_ITEM_BOUNDS_REQUIRED */


void zmapWindowCanvasItemGetColours(ZMapFeatureTypeStyle style, ZMapStrand strand, ZMapFrame frame,
                                    ZMapStyleColourType    colour_type,
                                    GdkColor **fill, GdkColor **draw, GdkColor **outline,
                                    GdkColor              *default_fill,
                                    GdkColor              *border)
{
      ZMapStyleParamId colour_target = STYLE_PROP_COLOURS;

      if (zMapStyleIsFrameSpecific(style))
      {
        switch (frame)
          {
          case ZMAPFRAME_0:
            colour_target = STYLE_PROP_FRAME0_COLOURS ;
            break ;
          case ZMAPFRAME_1:
            colour_target = STYLE_PROP_FRAME1_COLOURS ;
            break ;
          case ZMAPFRAME_2:
            colour_target = STYLE_PROP_FRAME2_COLOURS ;
            break ;
          default:
//            zMapAssertNotReached() ; no longer valid: frame specific style by eg for swissprot we display in one col on startup
            break ;
          }

        zMapStyleGetColours(style, colour_target, colour_type,
                        fill, draw, outline);
      }

      colour_target = STYLE_PROP_COLOURS;
      if (strand == ZMAPSTRAND_REVERSE && zMapStyleColourByStrand(style))
      {
        colour_target = STYLE_PROP_REV_COLOURS;
      }

      if (*fill == NULL && *draw == NULL && *outline == NULL)
            zMapStyleGetColours(style, colour_target, colour_type, fill, draw, outline);


  if (colour_type == ZMAPSTYLE_COLOURTYPE_SELECTED)
    {
      if(default_fill)
            *fill = default_fill;
      if(border)
            *outline = border;
    }
}

static void zmap_window_canvas_item_set_colour(ZMapWindowCanvasItem   canvas_item,
					       FooCanvasItem         *interval,
					       ZMapFeature 		feature,
					       ZMapFeatureSubPartSpan unused,
					       ZMapStyleColourType    colour_type,
					       int 				colour_flags,
					       GdkColor              *default_fill,
                                     GdkColor              *border)

{
  ZMapFeatureTypeStyle style;
  GdkColor *draw = NULL, *fill = NULL, *outline = NULL;
  GType interval_type;

  zMapLogReturnIfFail(canvas_item != NULL);
  zMapLogReturnIfFail(interval    != NULL);
  zMapLogReturnIfFail(canvas_item->feature != NULL);

  if((style = (ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->get_style)(canvas_item)))
    {
      ZMapFrame frame;
      ZMapStrand strand;

      frame = zMapFeatureFrame(canvas_item->feature);
      strand = canvas_item->feature->strand;
      zmapWindowCanvasItemGetColours(style, strand, frame, colour_type, &fill, &draw, &outline, default_fill, border);
    }


  interval_type = G_OBJECT_TYPE(interval);

  if (interval_type == ZMAP_TYPE_WINDOW_LONG_ITEM)
    {
      g_object_get(G_OBJECT(interval),
		   "item-type", &interval_type,
		   NULL);
    }

  if (g_type_is_a(interval_type, FOO_TYPE_CANVAS_LINE))
      // mh17: not used || g_type_is_a(interval_type, FOO_TYPE_CANVAS_LINE_GLYPH))
    {
      /* Using the fill colour seems a mis-nomer.... */

      foo_canvas_item_set(interval,
			  "fill_color_gdk", fill,
			  NULL);
    }
  else if (g_type_is_a(interval_type, FOO_TYPE_CANVAS_TEXT))
    {
      /* For text it seems to be more intuitive to use the "draw" colour for the text. */

      foo_canvas_item_set(interval,
			  "fill_color_gdk", draw,
			  NULL);
    }
  else if(g_type_is_a(interval_type, FOO_TYPE_CANVAS_RE)      ||
	  g_type_is_a(interval_type, FOO_TYPE_CANVAS_POLYGON) ||
	  g_type_is_a(interval_type, ZMAP_TYPE_WINDOW_GLYPH_FEATURE))
    {
      foo_canvas_item_set(interval,
			  "fill_color_gdk", fill,
			  NULL);

      if (outline)
	foo_canvas_item_set(interval,
			    "outline_color_gdk", outline,
			    NULL);
    }
  else
    {
      g_warning("Interval has unknown FooCanvasItem type '%s'.", g_type_name(interval_type));
    }

  return ;
}



static gboolean zmap_window_canvas_item_check_data(ZMapWindowCanvasItem canvas_item, GError **error)
{
  FooCanvasGroup *group;
  gboolean result = TRUE;
  GList *list;

  group = FOO_CANVAS_GROUP(canvas_item);

  list = group->item_list;

  while(result && list)
    {
      gpointer data;

      if(!(data = g_object_get_data(G_OBJECT(list->data), ITEM_SUBFEATURE_DATA)))
	{
	  *error = g_error_new(zmap_window_canvas_item_get_domain(), 2,
			       "Interval has no '%s' data", ITEM_SUBFEATURE_DATA);
	  result = FALSE;
	}

      list = list->next;
    }

  return result;
}




/*
 * This routine invokes the point method of the item.
 * The arguments x, y should be in the parent item local coordinates.
 */

static double window_canvas_item_invoke_point (FooCanvasItem *item,
					       double x, double y,
					       int cx, int cy,
					       FooCanvasItem **actual_item)
{
  /* Calculate x & y in item local coordinates */

  if (FOO_CANVAS_ITEM_GET_CLASS (item)->point)
    return FOO_CANVAS_ITEM_GET_CLASS (item)->point (item, x, y, cx, cy, actual_item);

  return 1e18;
}


static gboolean feature_is_drawable(ZMapFeature          feature_any,
				    ZMapFeatureTypeStyle feature_style,
				    ZMapStyleMode       *mode_to_use,
				    GType               *type_to_use)
{
  GType type;
  ZMapStyleMode mode;
  gboolean result = FALSE;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
	ZMapFeature feature;
	feature = (ZMapFeature)feature_any;

	mode = feature->type;
	mode = zMapStyleGetMode(feature_style);

	switch(mode)
	  {
	  case ZMAPSTYLE_MODE_TRANSCRIPT:
	    type = ZMAP_TYPE_WINDOW_TRANSCRIPT_FEATURE;
	    break;
	  case ZMAPSTYLE_MODE_ALIGNMENT:
	    type = ZMAP_TYPE_WINDOW_ALIGNMENT_FEATURE;
	    break;
	  case ZMAPSTYLE_MODE_TEXT:
	    type = ZMAP_TYPE_WINDOW_TEXT_FEATURE;
	    break;
	  case ZMAPSTYLE_MODE_SEQUENCE:
	    type = ZMAP_TYPE_WINDOW_SEQUENCE_FEATURE;
	    break;
	  case ZMAPSTYLE_MODE_ASSEMBLY_PATH:
	    type = ZMAP_TYPE_WINDOW_ASSEMBLY_FEATURE;
	    break;
        case ZMAPSTYLE_MODE_GRAPH:
          type = ZMAP_TYPE_WINDOW_GRAPH_ITEM;
          break;
        case ZMAPSTYLE_MODE_GLYPH:
//          type = ZMAP_TYPE_WINDOW_GLYPH_FEATURE;
//          break;
	  case ZMAPSTYLE_MODE_BASIC:
	  default:
	    type = ZMAP_TYPE_WINDOW_BASIC_FEATURE;
	    break;
	  }

	result = TRUE;
      }
      break;
    default:
      break;
    }


  if(mode_to_use && result)
    *mode_to_use = mode;
  if(type_to_use && result)
    *type_to_use = type;

  return result;
}


static void window_canvas_invoke_set_colours(gpointer list_data, gpointer user_data)
{
  FooCanvasItem *interval        = FOO_CANVAS_ITEM(list_data);
  EachIntervalData interval_data = (EachIntervalData)user_data;

  if(interval_data->feature)
    {
      ZMapFeatureSubPartSpanStruct local_struct;
      ZMapFeatureSubPartSpan sub_feature = NULL;

      if(!(sub_feature = g_object_get_data(G_OBJECT(interval), ITEM_SUBFEATURE_DATA)))
	{
	  sub_feature = &local_struct;
	  sub_feature->subpart = ZMAPFEATURE_SUBPART_INVALID;
	}

      if(interval_data->klass->set_colour)
	interval_data->klass->set_colour(interval_data->parent, interval, interval_data->feature, sub_feature,
					 interval_data->style_colour_type,
					 interval_data->colour_flags,
					 interval_data->default_fill_colour,
                               interval_data->border_colour);
    }

  return ;
}

static void window_item_feature_destroy(gpointer window_item_feature_data)
{
#ifdef NEVER_INCLUDE_BUT_TYPE_OF_DATA_HERE_IS
  ZMapFeatureSubPartSpan sub_feature = (ZMapFeatureSubPartSpan)window_item_feature_data;
#endif


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("In " ZMAP_MSG_FORMAT_STRING "\n", ZMAP_MSG_FUNCTION_MACRO) ;

  printf("%s(): g_slice_free1() on %p\n", __PRETTY_FUNCTION__, window_item_feature_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  g_slice_free1(window_item_feature_size_G, window_item_feature_data);

  return ;
}

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printBounds(FooCanvasItem *item, char *txt)
{
  double x1, y1, x2, y2 ;
  double world_x1, world_y1, world_x2, world_y2 ;

  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

  world_x1 = x1 ;
  world_y1 = y1 ;
  world_x2 = x2 ;
  world_y2 = y2 ;

  foo_canvas_item_i2w(item, &world_x1, &world_y1) ;
  foo_canvas_item_i2w(item, &world_x2, &world_y2) ;

  printf("%s\n"
	 "\tposition of %p within %p:\tx1 = %g, y1 = %g, x2 = %g, y2 = %g\n"
	 "\t    world position of %p:\tx1 = %g, y1 = %g, x2 = %g, y2 = %g\n",
	 txt,
	 item, item->parent, x1, y1, x2, y2,
	 item, world_x1, world_y1, world_x2, world_y2) ;

  fflush(stdout) ;

  return ;
}
#endif



/*  LocalWords:  zmapWindowCanvasItem ZMap Griffiths
 */
