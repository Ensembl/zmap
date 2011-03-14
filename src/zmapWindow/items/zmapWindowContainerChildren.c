/*  File: zmapWindowContainerFeatures.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: May 24 15:17 2010 (edgrif)
 * Created: Mon Apr 27 18:01:23 2009 (rds)
 * CVS info:   $Id: zmapWindowContainerChildren.c,v 1.7 2011-03-14 11:35:18 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <ZMap/zmapBase.h>
#include <zmapWindowContainerGroup_I.h>
#include <zmapWindowContainerChildren_I.h>


enum
  {
    OVERLAY_PROP_0,
    OVERLAY_PROP_MAX_WIDTH,
    OVERLAY_PROP_MAX_HEIGHT,
  };

enum
  {
    BACKGROUND_PROP_0,			/* zero is invalid property id */
    BACKGROUND_ORIGINAL_BACKGROUND,
    BACKGROUND_PROP_LAST,
    BACKGROUND_OVERRIDE_WIDTH_PIXELS,
    BACKGROUND_OVERRIDE_WIDTH_UNITS,
  };


static void zmap_window_container_overlay_class_init  (ZMapWindowContainerOverlayClass overlay_class);
static void zmap_window_container_overlay_init        (ZMapWindowContainerOverlay overlay);
static void zmap_window_container_overlay_set_property(GObject               *object,
						       guint                  param_id,
						       const GValue          *value,
						       GParamSpec            *pspec);
static void zmap_window_container_overlay_get_property(GObject               *object,
						       guint                  param_id,
						       GValue                *value,
						       GParamSpec            *pspec);

static void maximise_child_rectangle_no_update(FooCanvasRE *rectangle,
					       double ix1, double iy1,
					       double ix2, double iy2,
					       gboolean in_x, gboolean in_y);
static gboolean invoke_maximise_child_rectangle(GList *item_list,
						double x1, double y1,
						double x2, double y2,
						gboolean in_x,
						gboolean in_y);

static void zmap_window_container_features_class_init  (ZMapWindowContainerFeaturesClass container_class);
static void zmap_window_container_features_init        (ZMapWindowContainerFeatures collection);
static void zmap_window_container_features_set_property(GObject               *object,
							guint                  param_id,
							const GValue          *value,
							GParamSpec            *pspec);
static void zmap_window_container_features_get_property(GObject               *object,
							guint                  param_id,
							GValue                *value,
							GParamSpec            *pspec);

static void zmap_window_container_underlay_class_init  (ZMapWindowContainerUnderlayClass underlay_class);
static void zmap_window_container_underlay_init        (ZMapWindowContainerUnderlay underlay);
static void zmap_window_container_underlay_set_property(GObject               *object,
							guint                  param_id,
							const GValue          *value,
							GParamSpec            *pspec);
static void zmap_window_container_underlay_get_property(GObject               *object,
							guint                  param_id,
							GValue                *value,
							GParamSpec            *pspec);

static void zmap_window_container_background_class_init  (ZMapWindowContainerBackgroundClass background_class);
static void zmap_window_container_background_init        (ZMapWindowContainerBackground background);
static void zmap_window_container_background_set_property(GObject               *object,
							  guint                  param_id,
							  const GValue          *value,
							  GParamSpec            *pspec);
static void zmap_window_container_background_get_property(GObject               *object,
							  guint                  param_id,
							  GValue                *value,
							  GParamSpec            *pspec);




/*
 *                     External routines
 */


/*
 *       Functions for functions within a group.
 *
 */
GType zmapWindowContainerFeaturesGetType(void)
{
  static GType group_type = 0;

  if (group_type == 0)
    {
      static const GTypeInfo group_info = {
	sizeof (zmapWindowContainerFeaturesClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) zmap_window_container_features_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (zmapWindowContainerFeatures),
	0,              /* n_preallocs */
	(GInstanceInitFunc) zmap_window_container_features_init
      };

      group_type = g_type_register_static (foo_canvas_group_get_type(),
					   ZMAP_WINDOW_CONTAINER_FEATURES_NAME,
					   &group_info,
					   0);
    } /* group_type == 0 */

  return group_type;
}



FooCanvasItem *zmapWindowContainerFeaturesGetNextSibling(FooCanvasItem             *current_item,
							 ZMapContainerItemDirection direction,
							 gboolean                   wrap,
							 zmapWindowContainerItemTestCallback item_test_func_cb,
							 gpointer                            user_data)
{
  FooCanvasItem *next_item = NULL;



  return next_item;
}



GType zmapWindowContainerOverlayGetType(void)
{
  static GType group_type = 0;

  if (!group_type)
    {
      static const GTypeInfo group_info = {
	sizeof (zmapWindowContainerOverlayClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) zmap_window_container_overlay_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (zmapWindowContainerOverlay),
	0,              /* n_preallocs */
	(GInstanceInitFunc) zmap_window_container_overlay_init
      };

      group_type = g_type_register_static (foo_canvas_group_get_type(),
					   ZMAP_WINDOW_CONTAINER_OVERLAY_NAME,
					   &group_info,
					   0);
    } /* group_type == 0 */

  return group_type;
}

gboolean zmapWindowContainerOverlayMaximiseItems(ZMapWindowContainerOverlay overlay,
						 double x1, double y1,
						 double x2, double y2)
{
  gboolean need_update = FALSE;
  FooCanvasGroup *group;
  GList *list;

  group = (FooCanvasGroup *)overlay;

  if((overlay->flags.max_width || overlay->flags.max_height) &&
     (list = g_list_first(group->item_list)))
    {
      need_update = invoke_maximise_child_rectangle(list, x1, y1, x2, y2,
						    overlay->flags.max_width,
						    overlay->flags.max_height);
    }

  return need_update;
}



GType zmapWindowContainerUnderlayGetType(void)
{
  static GType group_type = 0;

  if (!group_type)
    {
      static const GTypeInfo group_info = {
	sizeof (zmapWindowContainerUnderlayClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) zmap_window_container_underlay_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (zmapWindowContainerUnderlay),
	0,              /* n_preallocs */
	(GInstanceInitFunc) zmap_window_container_underlay_init
      };

      group_type = g_type_register_static (FOO_TYPE_CANVAS_GROUP,
					   ZMAP_WINDOW_CONTAINER_UNDERLAY_NAME,
					   &group_info,
					   0);
    } /* group_type == 0 */

  return group_type;
}

gboolean zmapWindowContainerUnderlayMaximiseItems(ZMapWindowContainerUnderlay underlay,
						  double x1, double y1,
						  double x2, double y2)
{
  gboolean need_update = FALSE;
  FooCanvasGroup *group;
  GList *list;

  group = (FooCanvasGroup *)underlay;

  if((underlay->flags.max_width || underlay->flags.max_height) &&
     (list = g_list_first(group->item_list)))
    {
      need_update = invoke_maximise_child_rectangle(list, x1, y1, x2, y2,
						    underlay->flags.max_width,
						    underlay->flags.max_height);
    }

  return need_update;
}




GType zmapWindowContainerBackgroundGetType(void)
{
  static GType group_type = 0;

  if (!group_type)
    {
      static const GTypeInfo group_info =
	{
	  sizeof (zmapWindowContainerBackgroundClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_window_container_background_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (zmapWindowContainerBackground),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) zmap_window_container_background_init
	};

      group_type = g_type_register_static (foo_canvas_rect_get_type(),
					   ZMAP_WINDOW_CONTAINER_BACKGROUND_NAME,
					   &group_info,
					   0);
    } /* group_type == 0 */

  return group_type;
}

void zmapWindowContainerBackgroundSetColour(ZMapWindowContainerBackground background,
					    GdkColor *new_colour)
{
  if(FOO_IS_CANVAS_ITEM(background))
    {
      foo_canvas_item_set((FooCanvasItem *)background,
			  "fill-color-gdk", new_colour,
			  NULL);
    }

  return ;
}

void zmapWindowContainerBackgroundResetColour(ZMapWindowContainerBackground background)
{
  GdkColor *background_colour;

  if(background->has_bg_colour)
    {
      background_colour = &(background->original_colour);
      zmapWindowContainerBackgroundSetColour(background, background_colour);
    }

  return ;
}







/*
 *                     Internal routines
 */

static FooCanvasItemClass  *feature_item_parent_class_G  = NULL;

static void zmap_window_container_features_draw (FooCanvasItem *item, GdkDrawable *drawable,
                                    GdkEventExpose *expose)
{

#if MH17_REVCOMP_DEBUG > 1
 ZMapWindowContainerFeatures group = (ZMapWindowContainerFeatures) item;
      zMapLogWarning("container features draw @ %f,%f - %f,%f,  (%d items), canvas %p", item->y1,item->x1,item->y2,item->x2, g_list_length(group->__parent__.item_list), item->canvas) ;
#endif

  if(feature_item_parent_class_G->draw)
    (feature_item_parent_class_G->draw)(item, drawable, expose);

  return ;
}


static void zmap_window_container_features_class_init  (ZMapWindowContainerFeaturesClass container_class)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) container_class;
  FooCanvasItemClass *item_class    = (FooCanvasItemClass *)container_class;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Not included yet....is it correct this is here ?? */

  canvas_class->obj_size = sizeof(zmapWindowContainerFeaturesStruct) ;
  canvas_class->obj_total = 0 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  feature_item_parent_class_G  = (FooCanvasItemClass *)(g_type_class_peek_parent(container_class));
  item_class->draw = zmap_window_container_features_draw;

  gobject_class->set_property = zmap_window_container_features_set_property;
  gobject_class->get_property = zmap_window_container_features_get_property;

  return ;
}

static void zmap_window_container_features_init        (ZMapWindowContainerFeatures collection)
{

  /* nothing to do here... */
  g_object_set_data(G_OBJECT(collection), CONTAINER_TYPE_KEY, GINT_TO_POINTER(CONTAINER_GROUP_FEATURES));

  return ;
}

static void zmap_window_container_features_set_property(GObject               *object,
							guint                  param_id,
							const GValue          *value,
							GParamSpec            *pspec)

{
  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_window_container_features_get_property(GObject               *object,
							guint                  param_id,
							GValue                *value,
							GParamSpec            *pspec)
{
  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }

  return ;
}




static void zmap_window_container_overlay_class_init  (ZMapWindowContainerOverlayClass overlay_class)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) overlay_class;

  gobject_class->set_property = zmap_window_container_overlay_set_property;
  gobject_class->get_property = zmap_window_container_overlay_get_property;

  g_object_class_install_property(gobject_class, OVERLAY_PROP_MAX_WIDTH,
				  g_param_spec_boolean("maximise-x", "maximise x axis",
						       "Column needs maximising in the x axis",
						       FALSE, ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class, OVERLAY_PROP_MAX_HEIGHT,
				  g_param_spec_boolean("maximise-y", "maximise y axis",
						       "Column needs maximising in the y axis",
						       FALSE, ZMAP_PARAM_STATIC_RW));


  return ;
}

static void zmap_window_container_overlay_init        (ZMapWindowContainerOverlay overlay)
{

  g_object_set_data(G_OBJECT(overlay), CONTAINER_TYPE_KEY, GINT_TO_POINTER(CONTAINER_GROUP_OVERLAYS));

  overlay->flags.max_width  = FALSE;
  overlay->flags.max_height = FALSE;

  return ;
}

static void zmap_window_container_overlay_set_property(GObject               *object,
						       guint                  param_id,
						       const GValue          *value,
						       GParamSpec            *pspec)

{
  ZMapWindowContainerOverlay overlay;

  overlay = ZMAP_CONTAINER_OVERLAY(object);

  switch(param_id)
    {
    case OVERLAY_PROP_MAX_HEIGHT:
      overlay->flags.max_height = g_value_get_boolean(value);
      break;
    case OVERLAY_PROP_MAX_WIDTH:
      overlay->flags.max_width  = g_value_get_boolean(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_window_container_overlay_get_property(GObject               *object,
						       guint                  param_id,
						       GValue                *value,
						       GParamSpec            *pspec)
{
  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }

  return ;
}



static void zmap_window_container_underlay_class_init  (ZMapWindowContainerUnderlayClass underlay_class)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) underlay_class;

  gobject_class->set_property = zmap_window_container_underlay_set_property;
  gobject_class->get_property = zmap_window_container_underlay_get_property;

  return ;
}

static void zmap_window_container_underlay_init        (ZMapWindowContainerUnderlay underlay)
{

  g_object_set_data(G_OBJECT(underlay), CONTAINER_TYPE_KEY, GINT_TO_POINTER(CONTAINER_GROUP_OVERLAYS));

  return ;
}

static void zmap_window_container_underlay_set_property(GObject               *object,
							guint                  param_id,
							const GValue          *value,
							GParamSpec            *pspec)

{
  ZMapWindowContainerUnderlay underlay;

  underlay = ZMAP_CONTAINER_UNDERLAY(object);

  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_window_container_underlay_get_property(GObject               *object,
							guint                  param_id,
							GValue                *value,
							GParamSpec            *pspec)
{
  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }

  return ;
}


static void zmap_window_container_background_class_init  (ZMapWindowContainerBackgroundClass background_class)
{
  GObjectClass *gobject_class;
  GParamSpec *param_spec;
  const gchar *override_properties[] = {
    "width_pixels", "width_units",
    NULL
  };
  const gchar **property;
  guint param_id = BACKGROUND_PROP_LAST;


  gobject_class = (GObjectClass *)background_class;

  gobject_class->get_property = zmap_window_container_background_get_property;
  gobject_class->set_property = zmap_window_container_background_set_property;

  g_object_class_install_property(gobject_class,
				  BACKGROUND_ORIGINAL_BACKGROUND,
				  g_param_spec_boxed ("original-background", NULL, NULL,
						      GDK_TYPE_COLOR,
						      G_PARAM_READWRITE));

  /* replacing zMapDrawBoxSolid() by making border widths unavailable. */

  property = &override_properties[0];

  while(property && *property)
    {
      if((param_spec = g_object_class_find_property(gobject_class, *property)))
	{
	  g_object_class_override_property(gobject_class, param_id,
					   g_param_spec_get_name(param_spec));
	}
      param_id++;
      property++;
    }

  return ;
}

static void zmap_window_container_background_init        (ZMapWindowContainerBackground collection)
{
  FooCanvasRE *box;

  /* replacing zMapDrawBoxSolid(). i.e. a rectangle with _no_ border */

  box = FOO_CANVAS_RE(collection);

  g_object_set_data(G_OBJECT(collection), CONTAINER_TYPE_KEY, GINT_TO_POINTER(CONTAINER_GROUP_BACKGROUND));

  box->width_pixels = FALSE;
  box->width        = 0;

  return ;
}

static void zmap_window_container_background_set_property(GObject               *object,
							  guint                  param_id,
							  const GValue          *value,
							  GParamSpec            *pspec)
{
  ZMapWindowContainerBackground background;
  FooCanvasRE *box;

  g_return_if_fail(object != NULL);
  g_return_if_fail(ZMAP_IS_CONTAINER_BACKGROUND(object));

  switch(param_id)
    {
    case BACKGROUND_ORIGINAL_BACKGROUND:
      {
	GdkColor *colour;

	if((colour = g_value_get_boxed(value)))
	  {
	    g_object_set(object, "fill-color-gdk", colour, NULL);

	    background = ZMAP_CONTAINER_BACKGROUND(object);

	    background->original_colour = *colour; /* struct copy */
	    background->has_bg_colour   = TRUE;
	  }
      }
      break;
    case BACKGROUND_OVERRIDE_WIDTH_UNITS:
    case BACKGROUND_OVERRIDE_WIDTH_PIXELS:
      {
	box = FOO_CANVAS_RE(object);

	box->width_pixels = FALSE;
	box->width        = 0;
      }
      break;
    case BACKGROUND_PROP_LAST:
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_window_container_background_get_property(GObject               *object,
							  guint                  param_id,
							  GValue                *value,
							  GParamSpec            *pspec)
{
  g_return_if_fail(object != NULL);
  g_return_if_fail(ZMAP_IS_CONTAINER_BACKGROUND(object));

  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }

  return ;
}


/* The common functions */

static void maximise_child_rectangle_no_update(FooCanvasRE *rectangle,
					       double ix1, double iy1,
					       double ix2, double iy2,
					       gboolean in_x, gboolean in_y)
{
  if(in_x)
    {
      rectangle->x1 = ix1;
      rectangle->x2 = ix2;
    }

  if(in_y)
    {
      rectangle->y1 = iy1;
      rectangle->y2 = iy2;
    }

  return ;
}

static gboolean invoke_maximise_child_rectangle(GList *item_list,
						double x1, double y1,
						double x2, double y2,
						gboolean in_x,
						gboolean in_y)
{
  gboolean need_update = FALSE;
  gboolean maximise_non_rectangles = FALSE;	/* setting to true will require extra work! */

  do
    {
      /* We only maximise rectangles... Didn't I say that. */
      if(FOO_IS_CANVAS_RE(item_list->data))
	{
	  maximise_child_rectangle_no_update((FooCanvasRE *)(item_list->data),
					     x1, y1, x2, y2,
					     in_x, in_y);
	  need_update = TRUE;
	}
      else if(maximise_non_rectangles)
	{
	  double nx1, ny1, nx2, ny2; /* new coords */

	  foo_canvas_item_get_bounds((FooCanvasItem *)(item_list->data),
				     &nx1, &ny1, &nx2, &ny2);

	  if(in_x)
	    {
	      nx1 = x1;
	      nx2 = x2;
	    }
	  if(in_y)
	    {
	      ny1 = y1;
	      ny2 = y2;
	    }

	  /* This won't work for items that don't have x1,x2,y1,y2 properties! */
	  foo_canvas_item_set((FooCanvasItem *)(item_list->data),
			      "x1", nx1, "y1", ny1,
			      "x2", nx2, "y2", ny2,
			      NULL);
	}
    }
  while((item_list = item_list->next));

  return need_update;
}
