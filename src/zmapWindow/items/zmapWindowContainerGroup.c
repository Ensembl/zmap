/*  File: zmapWindowContainerGroup.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <math.h>
#include <ZMap/zmapBase.h>				    /* for ZMAP_PARAM_STATIC_RW */
//#include <zmapWindowCanvas.h>
#include <zmapWindowContainerGroup_I.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindow_P.h>		/* for fullReposition only */


enum
  {
    CONTAINER_PROP_0,		/* zero is invalid */
    CONTAINER_PROP_VISIBLE,
  };


static void zmap_window_container_group_class_init  (ZMapWindowContainerGroupClass container_class);
static void zmap_window_container_group_init        (ZMapWindowContainerGroup      group);
static void zmap_window_container_group_set_property(GObject               *object,
						     guint                  param_id,
						     const GValue          *value,
						     GParamSpec            *pspec);
static void zmap_window_container_group_get_property(GObject               *object,
						     guint                  param_id,
						     GValue                *value,
						     GParamSpec            *pspec);
static void zmap_window_container_group_destroy     (GtkObject *gtkobject);


static void zmap_window_container_group_draw (FooCanvasItem *item, GdkDrawable *drawable,
					      GdkEventExpose *expose);
static void zmap_window_container_group_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags);



#ifdef NO_NEED
static FooCanvasGroupClass *group_parent_class_G = NULL;
#endif
static FooCanvasItemClass  *item_parent_class_G  = NULL;

gboolean print_debug_G = FALSE ;


#define UPDATE_DEBUG	0


/*!
 * \brief Get the GType for the ZMapWindowContainerGroup GObjects
 *
 * \return GType corresponding to the GObject as registered by glib.
 */

GType zmapWindowContainerGroupGetType(void)
{
  static GType group_type = 0;

  if (!group_type) {
    static const GTypeInfo group_info =
      {
	sizeof (zmapWindowContainerGroupClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) zmap_window_container_group_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (zmapWindowContainerGroup),
	0,              /* n_preallocs */
	(GInstanceInitFunc) zmap_window_container_group_init

      };

    group_type = g_type_register_static (FOO_TYPE_CANVAS_GROUP,
					 ZMAP_WINDOW_CONTAINER_GROUP_NAME,
					 &group_info,
					 0);
  }

  return group_type;
}

/*!
 * \brief Create a new ZMapWindowContainerGroup object.
 *
 * \param parent    This is the parent ZMapWindowContainerFeatures
 * \param level     The level the new ZMapWindowContainerGroup should be.
 * \param child_spacing The distance between the children of this container.
 * \param fill-colour   The colour to use for the background.
 * \param border-colour The colour to use for the border.
 *
 * \return ZMapWindowContainerGroup The new group will be of GType corresponding to the level supplied.
 *                                  It will be a ZMapWindowContainerGroup by inheritance though.
 */

ZMapWindowContainerGroup zmapWindowContainerGroupCreate(FooCanvasGroup  *parent,
							ZMapContainerLevelType level,
							double child_spacing,
							GdkColor *background_fill_colour,
							GdkColor *background_border_colour)
{
  ZMapWindowContainerGroup container;

  container = zmapWindowContainerGroupCreateFromFoo((FooCanvasGroup *)parent,
						    level, child_spacing,
						    background_fill_colour,
						    background_border_colour);

  return container;
}

/*!
 * \brief Create a new ZMapWindowContainerGroup object.
 *
 * \param parent    This is the parent FooCanvasGroup.
 * \param level     The level the new ZMapWindowContainerGroup should be.
 * \param child_spacing The distance between the children of this container.
 * \param fill-colour   The colour to use for the background.
 * \param border-colour The colour to use for the border.
 *
 * \return ZMapWindowContainerGroup The new group will be of GType corresponding to the level supplied.
 *                                  It will be a ZMapWindowContainerGroup by inheritance though.
 */

ZMapWindowContainerGroup zmapWindowContainerGroupCreateFromFoo(FooCanvasGroup        *parent,
							       ZMapContainerLevelType level,
							       double                 child_spacing,
							       GdkColor              *background_fill_colour,
							       GdkColor              *background_border_colour)
{
  ZMapWindowContainerGroup container  = NULL;
  ZMapWindowContainerGroup parent_container  = NULL;
  FooCanvasItem *item;
  FooCanvasGroup *group;
  GType container_type;
  double this_spacing = 200.0;


  if(ZMAP_IS_CONTAINER_GROUP(parent))
    {
      parent = (FooCanvasGroup *)zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)parent);
	parent_container = (ZMapWindowContainerGroup) parent;

	this_spacing     = parent_container->child_spacing;
	level            = parent_container->level + 1;
    }

  container_type = ZMAP_TYPE_CONTAINER_GROUP;

  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_ROOT:
      container_type = ZMAP_TYPE_CONTAINER_CONTEXT;
      break;
    case ZMAPCONTAINER_LEVEL_ALIGN:
      container_type = ZMAP_TYPE_CONTAINER_ALIGNMENT;
      break;
    case ZMAPCONTAINER_LEVEL_BLOCK:
      container_type = ZMAP_TYPE_CONTAINER_BLOCK;
      break;
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      container_type = ZMAP_TYPE_CONTAINER_FEATURESET;
      break;
    default:
      container_type = ZMAP_TYPE_CONTAINER_GROUP;
      break;
    }

  item = foo_canvas_item_new(parent, container_type,
			     "x", 0.0,
			     "y", 0.0,
			     NULL);

  if(item && ZMAP_IS_CONTAINER_GROUP(item))
    {
      group     = FOO_CANVAS_GROUP(item);
      container = ZMAP_CONTAINER_GROUP(item);
      container->level = level;				    /* level */
      container->child_spacing = child_spacing;
      container->this_spacing = this_spacing;

	container->background_fill = background_fill_colour;
	container->background_border = background_border_colour;

      if(ZMAP_CONTAINER_GROUP_GET_CLASS(container)->post_create)
	(ZMAP_CONTAINER_GROUP_GET_CLASS(container)->post_create)(container);
    }

  return container;
}



GdkColor *zmapWindowContainerGroupGetFill(ZMapWindowContainerGroup group)
{
      return group->background_fill;

}

GdkColor *zmapWindowContainerGroupGetBorder(ZMapWindowContainerGroup group)
{
      return group->background_border;
}



/*!
 * \brief Set the visibility of a whole ZMapWindowContainerGroup.
 *
 * \param container_parent  A FooCanvasGroup which _must_ be a ZMapWindowContainerGroup.
 * \param visible           A boolean to specify visibility. TRUE = visible, FALSE = hidden.
 *
 * \return boolean set to TRUE if visiblity could be set.
 */

gboolean zmapWindowContainerSetVisibility(FooCanvasGroup *container_parent, gboolean visible)
{
  gboolean setable = FALSE;     /* Most columns aren't */

  /* Currently this function only works with columns, but the intention
   * is that it could work with blocks and aligns too at some later
   * point in time... THIS ISN'T TRUE. ANY ZMapWindowContainerGroup WILL WORK. */

  /* Check this is a container group. Any FooCanvasGroup could be passed. */
  if(ZMAP_IS_CONTAINER_GROUP(container_parent))
    {
      ZMapWindowContainerGroup group = (ZMapWindowContainerGroup) container_parent;

      group->flags.visible = visible;

      if(visible)
	foo_canvas_item_show((FooCanvasItem *)container_parent);
      else
	foo_canvas_item_hide((FooCanvasItem *)container_parent);

      zMapStopTimer("SetVis",visible ? "true" : "false");

      setable = TRUE;
    }

  return setable ;
}

/*!
 * \brief Set flag for the next update/draw cycle in the FooCanvas.
 *        When the flag is set the container code will do extra calculation
 *        to determine new positions.
 *
 * \param container Any container.  The code finds the root container.
 * \return void
 */

void zmapWindowContainerRequestReposition(ZMapWindowContainerGroup container)
{
  ZMapWindowContainerGroup context_container;

  context_container = zmapWindowContainerUtilsGetParentLevel(container, ZMAPCONTAINER_LEVEL_ROOT);

  zmapWindowFullReposition((ZMapWindowContainerGroup) container);

  return ;
}


/*!
 * Simple, set/get the vertical dimension for the background of the container.
 *
 * container  The container that needs its size set.
 * height     The height the container needs to be.
 *
 * void
 */

void zmapWindowContainerGroupBackgroundSize(ZMapWindowContainerGroup container, double height)
{
  container->height = height;

  return ;
}


double zmapWindowContainerGroupGetBackgroundSize(ZMapWindowContainerGroup container)
{
  return container->height ;
}







/*!
 * \brief Time to free the memory associated with the ZMapWindowContainerGroup.
 *
 * \code container = zmapWindowContainerGroupDestroy(container);
 *
 * \param container  The container to be free'd
 *
 * \return The container that has been free'd. i.e. NULL
 */

ZMapWindowContainerGroup zmapWindowContainerGroupDestroy(ZMapWindowContainerGroup container)
{
  gtk_object_destroy(GTK_OBJECT(container));

  container = NULL;

  return container;
}















/* object impl */
static void zmap_window_container_group_class_init  (ZMapWindowContainerGroupClass container_class)
{
  ZMapWindowContainerGroupClass canvas_class;
  FooCanvasItemClass *item_class;
  GtkObjectClass *gtkobject_class;
  GObjectClass *gobject_class;
  GParamSpec *param_spec;

  gtkobject_class = (GtkObjectClass *) container_class;
  gobject_class = (GObjectClass *) container_class;
  canvas_class  = (ZMapWindowContainerGroupClass)container_class;
  item_class    = (FooCanvasItemClass *)container_class;

  gobject_class->set_property = zmap_window_container_group_set_property;
  gobject_class->get_property = zmap_window_container_group_get_property;

  if((param_spec = g_object_class_find_property(gobject_class, "visible")))
    {
      g_object_class_override_property(gobject_class, CONTAINER_PROP_VISIBLE,
				       g_param_spec_get_name(param_spec));
    }

  canvas_class->obj_size = sizeof(zmapWindowContainerGroupStruct) ;
  canvas_class->obj_total = 0 ;

#if NOT_USED
  g_object_class_install_property(gobject_class, CONTAINER_PROP_COLUMN_REDRAW,
				  g_param_spec_boolean("column-redraw", "column redraw",
						       "Column needs redrawing when zoom changes",
						       FALSE, ZMAP_PARAM_STATIC_RW));
#endif

  item_parent_class_G  = (FooCanvasItemClass *)(g_type_class_peek_parent(container_class));

  zMapAssert(item_parent_class_G);
  zMapAssert(item_parent_class_G->update);

  item_class->draw     = zmap_window_container_group_draw;
  item_class->update   = zmap_window_container_group_update;

  gtkobject_class->destroy = zmap_window_container_group_destroy;

  return ;
}

static void zmap_window_container_group_init        (ZMapWindowContainerGroup container)
{

  g_object_set_data(G_OBJECT(container), CONTAINER_TYPE_KEY, GINT_TO_POINTER(CONTAINER_GROUP_PARENT));

  return ;
}

static void zmap_window_container_group_set_property(GObject               *object,
						     guint                  param_id,
						     const GValue          *value,
						     GParamSpec            *pspec)

{
  ZMapWindowContainerGroup container;

  g_return_if_fail(ZMAP_IS_CONTAINER_GROUP(object));

  container = ZMAP_CONTAINER_GROUP(object);

  switch(param_id)
    {
    case CONTAINER_PROP_VISIBLE:
      {
	gboolean visible = FALSE;

	visible = g_value_get_boolean(value);
	switch(container->level)
	  {
	  case ZMAPCONTAINER_LEVEL_FEATURESET:
	    if(visible)
	      {
		foo_canvas_item_show(FOO_CANVAS_ITEM(object));
	      }
	    else
	      {
		foo_canvas_item_hide(FOO_CANVAS_ITEM(object));
	      }
	    break;
	  default:
	    break;
	  } /* switch(container->level) */
      }
      break;
#if NOT_USED
    case CONTAINER_PROP_COLUMN_REDRAW:
      {
	switch(container->level)
	  {
	  case ZMAPCONTAINER_LEVEL_FEATURESET:
	    container->flags.column_redraw = g_value_get_boolean(value);
	    break;
	  default:
	    break;
	  } /* switch(container->level) */
      }
      break;
#endif
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    } /* switch(param_id) */

  return ;
}

static void zmap_window_container_group_get_property(GObject               *object,
						     guint                  param_id,
						     GValue                *value,
						     GParamSpec            *pspec)
{
  ZMapWindowContainerGroup container;

  g_return_if_fail(ZMAP_IS_CONTAINER_GROUP(object));

  container = ZMAP_CONTAINER_GROUP(object);

  switch(param_id)
    {
    case CONTAINER_PROP_VISIBLE:
      {
	FooCanvasItem *item;
	item = FOO_CANVAS_ITEM(object);
	g_value_set_boolean (value, item->object.flags & FOO_CANVAS_ITEM_VISIBLE);

	break;
      }
#if NOT_USED
    case CONTAINER_PROP_COLUMN_REDRAW:
      {
	switch(container->level)
	  {
	  case ZMAPCONTAINER_LEVEL_FEATURESET:
	    g_value_set_boolean(value, container->flags.column_redraw);
	    break;
	  default:
	    break;
	  } /* switch(container->level) */

	break;
      }
#endif
    default:
      {
	G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);

	break;
      }
    } /* switch(param_id) */

  return ;
}
#ifdef POINT_REQUIRED
static double window_container_group_invoke_point (FooCanvasItem *item,
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
static double zmap_window_container_group_item_point (FooCanvasItem *item,
						      double x,  double y,
						      int    cx, int    cy,
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
	  dist = window_container_group_invoke_point (child, gx, gy, cx, cy, &point_item);
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
#endif

static void zmap_window_container_group_destroy     (GtkObject *gtkobject)
{
  GtkObjectClass *gtkobject_class;

  gtkobject_class = (GtkObjectClass *)item_parent_class_G;

  if(gtkobject_class->destroy)
    (gtkobject_class->destroy)(gtkobject);

  return ;
}


static void zmap_window_container_group_draw (FooCanvasItem *item, GdkDrawable *drawable,
					      GdkEventExpose *expose)
{

#if 0
  // draw background if set
  // not ideal no gc, need a window gc to share held in the container
  zMap_draw_rect(drawable, ZMapWindowFeaturesetItem featureset, gint cx1, gint cy1, gint cx2, gint cy2, gboolean fill)
  gdk_draw_rectangle (drawable, featureset->gc, fill, cx1, cy1, cx2 - cx1, cy2 - cy1);

  this is only used for the strand separator...
  instead let-s add a CanvasFeatureset with a fixed width and make that draw the background
  do this in zmapWindowDrawFeatures and make hit markers find the feature set not add it
#endif

  if(item_parent_class_G->draw)
    (item_parent_class_G->draw)(item, drawable, expose);

  return ;
}








/* This takes care of the x positioning of the containers as well as the maximising in the y coords. */
/* colunns have been ordered before calling, place each one to the right of the previous one */

static void zmap_window_container_group_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags)
{
  FooCanvasGroup *canvas_group;
  gboolean item_visible;


canvas_group   = (FooCanvasGroup *)item;
  item_visible   = ((item->object.flags & FOO_CANVAS_ITEM_VISIBLE) == FOO_CANVAS_ITEM_VISIBLE);


  item->x2 = item->x1;		/* reset group to zero size to avoid stretchy items expanding it by proxy */
  item->y2 = item->y1;

  (item_parent_class_G->update)(item, i2w_dx, i2w_dy, flags);


  if(item_visible)
    {
	GList *l;
	FooCanvasItem *foo;
	ZMapWindowFeaturesetItem featureset;
	double size;

	for(l = canvas_group->item_list; l ; l = l->next)
	{
		foo = (FooCanvasItem *) l->data;
		featureset = (ZMapWindowFeaturesetItem) foo;

#if 0
char *x = "?";
if(ZMAP_IS_WINDOW_FEATURESET_ITEM(foo))	/* these could be groups or even normal foo items */
{
	x = g_strdup_printf("%s layer %x",g_quark_to_string(zMapWindowCanvasFeaturesetGetId(featureset)),
				     zMapWindowCanvasFeaturesetGetLayer(featureset));
}
else if(ZMAP_IS_CONTAINER_GROUP(foo))
{
	char *name = "none";

	ZMapFeatureAny f = ((ZMapWindowContainerGroup) foo)->feature_any;

	if(f)
		name = (char *) g_quark_to_string(f->unique_id);
	x = g_strdup_printf("group %s level %d",name, ((ZMapWindowContainerGroup) foo)->level);
}

printf("group update %s\n", x);
#endif
		if(ZMAP_IS_WINDOW_FEATURESET_ITEM(foo))	/* these could be groups or even normal foo items */
		{
			guint layer = zMapWindowCanvasFeaturesetGetLayer(featureset);

			// this will automatically stretch the mark sideways
			// and also strand backgrounds if used
			// and also navigator locator background and cursor
			// and also column backgrounds
			// as all are implemented as empty ZMapWindowCanvasFeaturesets
			/* the background expands to cover the data inside the group */

//printf("featureset %s %x %f\n",g_quark_to_string(zMapWindowCanvasFeaturesetGetId(featureset)), zMapWindowCanvasFeaturesetGetLayer(featureset), zMapWindowCanvasFeaturesetGetWidth(featureset));

			if(layer & ZMAP_CANVAS_LAYER_STRETCH_X)
			{
				foo->x1 = item->x1;
				foo->x2 = item->x2;
				foo_canvas_c2w(foo->canvas,foo->x2 - foo->x1, 0, &size, NULL);
				zMapWindowCanvasFeaturesetSetWidth(featureset,size);
			}

			if(layer & ZMAP_CANVAS_LAYER_STRETCH_Y)
			{
				double y1, y2;

				/* NOTE for featureset level groups we set Y extend by containing block sequence coords */
				/* this is used for higher level groups */
				foo->y1 = item->y1;
				foo->y2 = item->y2;
				foo_canvas_c2w(foo->canvas,0, foo->y1, NULL, &y1);
				foo_canvas_c2w(foo->canvas,0, foo->y2, NULL, &y2);
				zMapWindowCanvasFeaturesetSetSequence(featureset,y1,y2);
			}
		}
	}



    }

//printf("group width = %f\n", item->x2 - item->x1);


  return ;
}






