/*  File: zmapWindowContainerGroup.c
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: May 24 15:23 2010 (edgrif)
 * Created: Wed Dec  3 10:02:22 2008 (rds)
 * CVS info:   $Id: zmapWindowContainerGroup.c,v 1.13 2010-05-24 14:23:50 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <math.h>
#include <ZMap/zmapBase.h>				    /* for ZMAP_PARAM_STATIC_RW */
#include <zmapWindowCanvas.h>
#include <zmapWindowContainerGroup_I.h>
#include <zmapWindowContainerUtils.h>


enum
  {
    CONTAINER_PROP_0,		/* zero is invalid */
    CONTAINER_PROP_VISIBLE,
    CONTAINER_PROP_COLUMN_REDRAW,
    CONTAINER_PROP_ZMAPWINDOW,
  };

/*! Simple struct to hold data about update hooks.  */
typedef struct
{
  ZMapWindowContainerUpdateHook hook_func; /*! The function.  */
  gpointer                      hook_data; /*! The data passed to the function.  */
} ContainerUpdateHookStruct, *ContainerUpdateHook;


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
static void zmap_window_container_group_reposition(ZMapWindowContainerGroup container_group, 
						   double rect_x1,   double rect_y1,
						   double rect_x2,   double rect_y2,
						   double *dx_repos, double *dy_repos);

static void maximise_background_rectangle(ZMapWindowContainerGroup this_container, 
					  FooCanvasItem           *container_item,
					  FooCanvasRE             *rect);
static void crop_rectangle_to_scroll_region(gpointer rectangle_data, gpointer points_data);
static void zmap_window_container_scroll_region_get_item_bounds(FooCanvasItem *item,
								double *x1, double *y1,
								double *x2, double *y2);
static void zmap_window_container_update_with_crop(FooCanvasItem *item, 
						   double i2w_dx, double i2w_dy,
						   FooCanvasPoints *itemised_scroll_region,
						   int flags);
static void invoke_update_hooks(ZMapWindowContainerGroup container, GSList *hooks_list,
				double x1, double y1, double x2, double y2);
#ifdef NOT_IMPLEMENTED
static void zmap_window_container_invoke_pre_update_hooks(ZMapWindowContainerGroup container,
							  double x1, double y1, double x2, double y2);
#endif /* NOT_IMPLEMENTED */
static void zmap_window_container_invoke_post_update_hooks(ZMapWindowContainerGroup container,
							   double x1, double y1, double x2, double y2);

static gint find_update_hook_cb(gconstpointer list_data, gconstpointer query_data);


#ifdef NO_NEED
static FooCanvasGroupClass *group_parent_class_G = NULL;
#endif
static FooCanvasItemClass  *item_parent_class_G  = NULL;


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

ZMapWindowContainerGroup zmapWindowContainerGroupCreate(ZMapWindowContainerFeatures parent,
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
  FooCanvasItem *features   = NULL;
  FooCanvasItem *overlay    = NULL;
  FooCanvasItem *underlay   = NULL;
  FooCanvasItem *background = NULL;
  FooCanvasItem *item;
  FooCanvasGroup *group;
  GType container_type;
  double this_spacing = 200.0;

  if(ZMAP_IS_CONTAINER_GROUP(parent))
    {
      zMapAssertNotReached();
      
      parent = (FooCanvasGroup *)zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)parent);
    }
  else
    {
      if((parent_container = (ZMapWindowContainerGroup)(((FooCanvasItem *)parent)->parent)))
	{
	  this_spacing     = parent_container->child_spacing;
	  level            = parent_container->level + 1;
	}
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
    case ZMAPCONTAINER_LEVEL_STRAND:
      container_type = ZMAP_TYPE_CONTAINER_STRAND;
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
      container->flags.column_redraw = FALSE;

      background = foo_canvas_item_new(group, ZMAP_TYPE_CONTAINER_BACKGROUND,
				       "original-background", background_fill_colour,
				       NULL);

      underlay   = foo_canvas_item_new(group, ZMAP_TYPE_CONTAINER_UNDERLAY, NULL);

      features   = foo_canvas_item_new(group, ZMAP_TYPE_CONTAINER_FEATURES, NULL);

      overlay    = foo_canvas_item_new(group, ZMAP_TYPE_CONTAINER_OVERLAY,  NULL);

      if(ZMAP_CONTAINER_GROUP_GET_CLASS(container)->post_create)
	(ZMAP_CONTAINER_GROUP_GET_CLASS(container)->post_create)(container);
    }
  
  return container;
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
      if(visible)
	foo_canvas_item_show((FooCanvasItem *)container_parent);
      else
	foo_canvas_item_hide((FooCanvasItem *)container_parent);

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

  if(context_container)
    {
      g_object_set(G_OBJECT(context_container),
		   "need-reposition", TRUE,
		   NULL);
    }

  return ;
}


/*!
 * \brief Simple, Set the vertical dimension for the background of the container.
 *
 * \param container  The container that needs its size set.
 * \param height     The height the container needs to be.
 *
 * \return void
 */

void zmapWindowContainerGroupBackgroundSize(ZMapWindowContainerGroup container, double height)
{
  container->height = height;

  return ;
}

/*!
 * \brief A ZMapWindowContainerGroup may need to redraw it's children.  
 *        This sets a flag so that it happens
 *
 * \param container  The container that needs its flag set.
 * \param flag       TRUE = redraw required, FALSE = no redraw necessary
 *
 * \return void
 */

void zmapWindowContainerGroupChildRedrawRequired(ZMapWindowContainerGroup container, 
						 gboolean redraw_required)
{
  container->flags.column_redraw = redraw_required;

  return ;
}

/*!
 * \brief Simple, Set the background colour of the container.
 *
 * \param container  The container that needs its colour set.
 * \param colour     The colour the container needs to be.
 *
 * \return void
 */

void zmapWindowContainerGroupSetBackgroundColour(ZMapWindowContainerGroup container,
						 GdkColor *new_colour)
{
  ZMapWindowContainerBackground background;

  if((background = zmapWindowContainerGetBackground(container)))
    zmapWindowContainerBackgroundSetColour(background, new_colour);

  return ;
}

/*!
 * \brief Simple, Reset the background colour of the container.
 *
 * \param container  The container that needs its colour reset.
 *
 * \return void
 */

void zmapWindowContainerGroupResetBackgroundColour(ZMapWindowContainerGroup container)
{
  ZMapWindowContainerBackground background;

  if((background = zmapWindowContainerGetBackground(container)))
    zmapWindowContainerBackgroundResetColour(background);

  return ;
}

#ifdef NOT_IMPLEMENTED

/* The idea with the pre and post hooks was to allow for essentially
 * what the container execute code does, but on a more permanent basis
 * (every time the container is updated). I haven't got a use case for
 * the pre that I can robustly test it with, so it's dead in the water
 * at them moment, to be resurrected when I have. */

void zmapWindowContainerGroupAddPreUpdateHook(ZMapWindowContainerGroup container,
					      ZMapWindowContainerUpdateHook hook,
					      gpointer user_data)
{
  ContainerUpdateHook update_hook = NULL;

  if((update_hook = g_new0(ContainerUpdateHookStruct, 1)))
    {
      update_hook->hook_func = hook;
      update_hook->hook_data = user_data;

      container->pre_update_hooks = g_slist_append(container->pre_update_hooks, 
						   update_hook);
    }

  return ;
}

void zmapWindowContainerGroupRemovePreUpdateHook(ZMapWindowContainerGroup container,
						 ZMapWindowContainerUpdateHook hook,
						 gpointer user_data)
{
  ContainerUpdateHookStruct update_hook = {NULL};
  GSList *slist = NULL;

  update_hook.hook_func = hook;
  update_hook.hook_data = user_data;

  if((slist = g_slist_find_custom(container->pre_update_hooks,
				  &update_hook,
				  find_update_hook_cb)))
    {
      g_free(slist->data);
      container->pre_update_hooks = g_slist_remove(container->pre_update_hooks, slist->data);
    }
}
#endif /* NOT_IMPLEMENTED */

/*!
 * \brief Add an update hook to the ZMapWindowContainerGroup
 *
 * Internally the containers hold a list of these hook which get
 * called every time the container gets drawn.  This can be done
 * on a per container basis, rather than across all containers. 
 * In terms of utility and use case the block marking and navigator
 * are users of this.
 *
 * \param container  The container that needs an update hook.
 * \param hook       This must be a ZMapWindowContainerUpdateHook.
 * \param hook-data  The data that will be passed to the hook
 *
 * \return void
 */

void zmapWindowContainerGroupAddUpdateHook(ZMapWindowContainerGroup container,
					   ZMapWindowContainerUpdateHook hook,
					   gpointer user_data)
{
  ContainerUpdateHook update_hook = NULL;

  if((update_hook = g_new0(ContainerUpdateHookStruct, 1)))
    {
      update_hook->hook_func = hook;
      update_hook->hook_data = user_data;

      container->post_update_hooks = g_slist_append(container->post_update_hooks, 
						    update_hook);
    }

  return ;
}

/*!
 * \brief Remove an update hook on a ZMapWindowContainerGroup.
 * 
 * Exactly the opposite of Add. Both the hook and the data are required
 * to successfully remove the hook.
 *
 * \param container  The container that needs an update hook removing.
 * \param hook       The hook that was added.
 * \param hook-data  The data that was added.
 *
 * \return void
 */

void zmapWindowContainerGroupRemoveUpdateHook(ZMapWindowContainerGroup container,
					      ZMapWindowContainerUpdateHook hook,
					      gpointer user_data)
{
  ContainerUpdateHookStruct update_hook = {NULL};
  GSList *slist = NULL;

  update_hook.hook_func = hook;
  update_hook.hook_data = user_data;

  if((slist = g_slist_find_custom(container->post_update_hooks,
				  &update_hook,
				  find_update_hook_cb)))
    {
      g_free(slist->data);
      container->post_update_hooks = g_slist_remove(container->post_update_hooks, slist->data);
    }
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

  g_object_class_install_property(gobject_class, CONTAINER_PROP_COLUMN_REDRAW,
				  g_param_spec_boolean("column-redraw", "column redraw",
						       "Column needs redrawing when zoom changes",
						       FALSE, ZMAP_PARAM_STATIC_RW));

  item_parent_class_G  = (FooCanvasItemClass *)(g_type_class_peek_parent(container_class));

  zMapAssert(item_parent_class_G);
  zMapAssert(item_parent_class_G->update);

  item_class->draw     = zmap_window_container_group_draw;
  item_class->update   = zmap_window_container_group_update;

  container_class->reposition_group = zmap_window_container_group_reposition;

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
  ZMapWindowContainerGroup container;
  GtkObjectClass *gtkobject_class;

  gtkobject_class = (GtkObjectClass *)item_parent_class_G;
  container = (ZMapWindowContainerGroup)gtkobject;

  if(container->pre_update_hooks)
    {
      g_slist_foreach(container->pre_update_hooks, (GFunc)g_free, NULL);
      g_slist_free(container->pre_update_hooks);
      container->pre_update_hooks = NULL;
    }
  
  if(container->post_update_hooks)
    {
      g_slist_foreach(container->post_update_hooks, (GFunc)g_free, NULL);
      g_slist_free(container->post_update_hooks);
      container->post_update_hooks = NULL;
    }

  if(gtkobject_class->destroy)
    (gtkobject_class->destroy)(gtkobject);

  return ;
}


static void zmap_window_container_group_draw (FooCanvasItem *item, GdkDrawable *drawable,
					      GdkEventExpose *expose)
{
  if(item_parent_class_G->draw)
    (item_parent_class_G->draw)(item, drawable, expose);

  return ;
}

static void maximise_background_rectangle(ZMapWindowContainerGroup this_container, 
					  FooCanvasItem           *container_item,
					  FooCanvasRE             *rect)
{
  FooCanvasItem *rect_item;
  double irx1, irx2, iry1, iry2;
  int container_x2, container_y2; /* container canvas coords, calculated from group->update above. */
  
  
  /* We can't trust item->x1 and item->y1 as empty child
   * groups return 0,0->0,0 hence extend all parents to
   * 0,0! */
  container_x2 = (int)(container_item->x2);
  container_y2 = (int)(container_item->y2);

  rect_item = (FooCanvasItem *)rect; /*  */

  irx1 = iry1 = 0.0;	/* placed @ 0,0 */
  foo_canvas_item_i2w(rect_item, &irx1, &iry1);
  foo_canvas_c2w(rect_item->canvas, container_x2, container_y2, &irx2, &iry2);

  if((iry2 - iry1 + 1) < this_container->height)
    iry2 = this_container->height + iry1 ;
  
  rect->x1 = irx1;
  rect->y1 = iry1;
  rect->x2 = irx2;
  rect->y2 = iry2;

  if(rect->x1 == rect->x2)
    rect->x2 = rect->x1 + this_container->this_spacing;

  foo_canvas_item_w2i(rect_item, &(rect->x1), &(rect->y1));
  foo_canvas_item_w2i(rect_item, &(rect->x2), &(rect->y2));

  /* rect->x1 & rect->y1 should == 0.0 */

  return ;
}

static void crop_rectangle_to_scroll_region(gpointer rectangle_data, gpointer points_data)
{
  FooCanvasRE *rect;
  FooCanvas *foo_canvas;
  FooCanvasItem *crop_item;
  FooCanvasPoints *scroll_region = (FooCanvasPoints *)points_data;
  double scroll_x1, scroll_y1, scroll_x2, scroll_y2;
  double iwx1, iwy1, iwx2, iwy2;
  
  rect       = (FooCanvasRE *)rectangle_data;
  crop_item  = (FooCanvasItem *)rect;
  foo_canvas = crop_item->canvas;

  iwx1 = rect->x1;
  iwy1 = rect->y1;
  iwx2 = rect->x2;
  iwy2 = rect->y2;

  if((scroll_region == NULL))
    {
      /* x unused ATM */
      scroll_x1 = foo_canvas->scroll_x1;
      scroll_x2 = foo_canvas->scroll_x2;
      
      scroll_y1 = foo_canvas->scroll_y1;
      scroll_y2 = foo_canvas->scroll_y2;
      
      foo_canvas_item_w2i(crop_item, &scroll_x1, &scroll_y1);
      foo_canvas_item_w2i(crop_item, &scroll_x2, &scroll_y2);
    }
  else
    {
      scroll_x1 = scroll_region->coords[0];
      scroll_y1 = scroll_region->coords[1];
      scroll_x2 = scroll_region->coords[2];
      scroll_y2 = scroll_region->coords[3];
    }

  if (!(iwy2 < scroll_y1) && !(iwy1 > scroll_y2) && ((iwy1 < scroll_y1) || (iwy2 > scroll_y2)))
    {
      if(iwy1 < scroll_y1)
	{
	  rect->y1 = scroll_y1 - 1.0;
	}
      
      if(iwy2 > scroll_y2)
	{
	  rect->y2 = scroll_y2 + 1.0;
	}
    }

  return ;
}

static void zmap_window_container_scroll_region_get_item_bounds(FooCanvasItem *item,
								double *x1, double *y1,
								double *x2, double *y2)
{
  FooCanvas *foo_canvas;
  double scroll_x1, scroll_y1, scroll_x2, scroll_y2;

  foo_canvas = item->canvas;

  scroll_x1 = foo_canvas->scroll_x1;
  scroll_x2 = foo_canvas->scroll_x2;

  scroll_y1 = foo_canvas->scroll_y1;
  scroll_y2 = foo_canvas->scroll_y2;
  
  foo_canvas_item_w2i(item, &scroll_x1, &scroll_y1);
  foo_canvas_item_w2i(item, &scroll_x2, &scroll_y2);
  
  if(x1)
    *x1 = scroll_x1;
  if(y1)
    *y1 = scroll_y1;
  if(x2)
    *x2 = scroll_x2;
  if(y2)
    *y2 = scroll_y2;

  return ;
}

static void zmap_window_container_update_with_crop(FooCanvasItem *item, 
						   double i2w_dx, double i2w_dy,
						   FooCanvasPoints *itemised_scroll_region,
						   int flags)
{
  if(FOO_IS_CANVAS_GROUP(item))
    {
      FooCanvasGroup *group;
      GList *list;

      group = (FooCanvasGroup *)item;

      if((list = group->item_list))
	{
	  double sub_i2w_dx, sub_i2w_dy;

	  sub_i2w_dx = i2w_dx + group->xpos;
	  sub_i2w_dy = i2w_dx + group->ypos;

	  itemised_scroll_region->coords[0] -= group->xpos;
	  itemised_scroll_region->coords[1] -= group->ypos;
	  itemised_scroll_region->coords[2] -= group->xpos;
	  itemised_scroll_region->coords[3] -= group->ypos;

	  do
	    {
	      zmap_window_container_update_with_crop((FooCanvasItem *)(list->data), 
						     sub_i2w_dx, sub_i2w_dy,
						     itemised_scroll_region, flags);
	    }
	  while((list = list->next));

	  itemised_scroll_region->coords[0] += group->xpos;
	  itemised_scroll_region->coords[1] += group->ypos;
	  itemised_scroll_region->coords[2] += group->xpos;
	  itemised_scroll_region->coords[3] += group->ypos;

	}
    }
  else if(flags & ZMAP_CANVAS_UPDATE_CROP_REQUIRED)
    {
      if(FOO_IS_CANVAS_RE(item))
	crop_rectangle_to_scroll_region((FooCanvasRE *)item, NULL);
    }

  if(FOO_CANVAS_ITEM_GET_CLASS(item)->update)
    (FOO_CANVAS_ITEM_GET_CLASS(item)->update)(item, i2w_dx, i2w_dy, flags);

  return ;
}

static void invoke_update_hooks(ZMapWindowContainerGroup container, GSList *hooks_list,
				double x1, double y1, double x2, double y2)
{
  FooCanvasItem *item;
  FooCanvasPoints bounds;
  double coords[4] = {0.0};
  GSList *hooks;
  gboolean canvas_in_update = TRUE;
  gboolean container_need_update = FALSE;
  guint item_flags = 0;

  hooks = hooks_list;

  coords[0] = x1;
  coords[1] = y1;
  coords[2] = x2;
  coords[3] = y2;
  
  bounds.coords     = &coords[0];
  bounds.ref_count  = 1;
  bounds.num_points = 2;
  
  item = (FooCanvasItem *)container;

  if(canvas_in_update)		/* actually we might not want to do this. */
    canvas_in_update = item->canvas->doing_update;
  

  if(canvas_in_update)
    {
      /* Stops the g_return_if_fail(!item->cavnas->doing_update) failing!!! */
      item->canvas->doing_update = FALSE;

      /* monkey with item->object.flags */
      item_flags = item->object.flags;

      /* Stops us getting all the way up the tree to foo_canvas_request_update. */
      if((container_need_update = ((item_flags & FOO_CANVAS_ITEM_NEED_UPDATE) == FOO_CANVAS_ITEM_NEED_UPDATE)) == FALSE)
	item->object.flags |= FOO_CANVAS_ITEM_NEED_UPDATE;
    }

  do
    {
      ContainerUpdateHook update_hook;
      
      update_hook = (ContainerUpdateHook)(hooks->data);
      
      if(update_hook->hook_func)
	(update_hook->hook_func)(container, &bounds, container->level, update_hook->hook_data);
    }
  while((hooks = hooks->next));

  if(canvas_in_update)
    {
      /* Need to reset everything */
      item->canvas->doing_update = TRUE;
      item->object.flags |= item_flags;
      if(!container_need_update)
	item->object.flags &= ~FOO_CANVAS_ITEM_NEED_UPDATE;
    }

  return ;
}

#ifdef NOT_IMPLEMENTED
static void zmap_window_container_invoke_pre_update_hooks(ZMapWindowContainerGroup container,
							  double x1, double y1, double x2, double y2)
{
  if(container->pre_update_hooks)
    {
      invoke_update_hooks(container, container->pre_update_hooks, x1, y1, x2, y2);
    }

  return ;
}
#endif /* NOT_IMPLEMENTED */

static void zmap_window_container_invoke_post_update_hooks(ZMapWindowContainerGroup container,
							   double x1, double y1, double x2, double y2)
{
  if(container->post_update_hooks)
    {
      invoke_update_hooks(container, container->post_update_hooks, x1, y1, x2, y2);
    }

  return ;
}

/* This takes care of the x positioning of the containers as well as the maximising in the y coords. */
static void zmap_window_container_group_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags)
{
  ZMapWindowContainerGroup   this_container = NULL;
  ZMapWindowContainerGroup parent_container = NULL;
  ZMapWindowContainerOverlay   overlay = NULL;
  ZMapWindowContainerUnderlay underlay = NULL;
  FooCanvasRE *rect = NULL;
  FooCanvasItem *parent_parent = NULL;
  FooCanvasGroup *canvas_group;
  GList *item_list;
  double current_x = 0.0;
  double current_y = 0.0;
  gboolean item_visible;
  gboolean doing_reposition;
  gboolean need_cropping;
  gboolean add_strand_border = TRUE;

  canvas_group   = (FooCanvasGroup *)item;
  item_visible   = ((item->object.flags & FOO_CANVAS_ITEM_VISIBLE) == FOO_CANVAS_ITEM_VISIBLE);

  this_container = (ZMapWindowContainerGroup)item;
  this_container->reposition_x = current_x;
  this_container->reposition_y = current_y;

  /* This was in the previous version of the code, copying across... */
  if(add_strand_border && this_container->level == ZMAPCONTAINER_LEVEL_STRAND)
    this_container->reposition_x += this_container->child_spacing;

  if(item->parent && (parent_parent = item->parent->parent))
    {
      parent_container = (ZMapWindowContainerGroup)parent_parent;
#ifdef ACTUALLY_FLAGS_SUBVERSION_IS_BETTER
      /* we could subvert flags parameter, but this is slightly
       * better.  Needs to be propgated through the tree. */
      if(!this_container->flags.need_reposition)
	this_container->flags.need_reposition = parent_container->flags.need_reposition;

      this_container->flags.need_cropping = parent_container->flags.need_cropping;
#endif
      if(this_container->flags.need_reposition)
	flags |= ZMAP_CANVAS_UPDATE_NEED_REPOSITION;

      current_x = parent_container->reposition_x;
      current_y = parent_container->reposition_y;
    }

  doing_reposition = ((flags & ZMAP_CANVAS_UPDATE_NEED_REPOSITION) == ZMAP_CANVAS_UPDATE_NEED_REPOSITION);
  need_cropping    = ((flags & ZMAP_CANVAS_UPDATE_CROP_REQUIRED)   == ZMAP_CANVAS_UPDATE_CROP_REQUIRED);

  if(doing_reposition)
    {
      GList *list, *list_end, tmp_features = {NULL}, tmp_background = {NULL}; 
      gboolean print_debug = FALSE;

      if((item_list = canvas_group->item_list))
	{
	  /* reposition immediate descendants (features, background, overlay, underlay) */
	  do
	    {
	      if(FOO_IS_CANVAS_GROUP(item_list->data))
		{
		  FooCanvasGroup *group = (FooCanvasGroup *)(item_list->data);
		  
		  if(group->xpos != 0.0)
		    group->xpos = 0.0;
		  if(group->ypos != 0.0)
		    group->ypos = 0.0;
		  
		  if(ZMAP_IS_CONTAINER_OVERLAY(item_list->data))
		    overlay = (ZMapWindowContainerOverlay)(item_list->data);
		  else if(ZMAP_IS_CONTAINER_UNDERLAY(item_list->data))
		    underlay = (ZMapWindowContainerUnderlay)(item_list->data);
		}
	      else if(ZMAP_IS_CONTAINER_BACKGROUND(item_list->data))
		{
		  rect = FOO_CANVAS_RE(item_list->data);
		  
		  if(rect->x1 != 0.0)
		    rect->x1 = 0.0;
		  if(rect->y1 != 0.0)
		    rect->y1 = 0.0;
		  
		  rect->x2 = 1.0;	/* There's no way to know width */
		  rect->y2 = this_container->height; /* We know height though. */
		}
	      /* no recursion here... */
	    }
	  while((item_list = item_list->next));
	}


      if(print_debug)
	{
	  switch(this_container->level)
	    {
	    case ZMAPCONTAINER_LEVEL_ROOT:       printf("context: ");    break;
	    case ZMAPCONTAINER_LEVEL_ALIGN:      printf("align: ");      break;
	    case ZMAPCONTAINER_LEVEL_BLOCK:      printf("block: ");      break;
	    case ZMAPCONTAINER_LEVEL_STRAND:     printf("strand: ");     break;
	    case ZMAPCONTAINER_LEVEL_FEATURESET: printf("featureset: "); break;
	    default:
	      break;
	    }
	  
	  printf("current_x=%f, current_y=%f\n", current_x, current_y);
	}

      if(item_visible)
	{
	  FooCanvasGroup *real_group;

	  real_group       = (FooCanvasGroup *)this_container;

	  /* There's _no_ need to use group->translate, nor move by dx,dy. Just set the positions */
	  real_group->xpos = current_x;
	  /* We don't do y at the moment. no real idea what should happen here. */
	  /* real_group->ypos = current_y; */
	}

      /* We _only_ update the background and features at this time. Underlays and overlays will get done later */
      tmp_background.next = &tmp_features;
      tmp_background.data = zmapWindowContainerGetBackground(this_container);
      tmp_background.prev = NULL;

      tmp_features.next = NULL;
      tmp_features.data = zmapWindowContainerGetFeatures(this_container);
      tmp_features.prev = &tmp_background;

      list     = canvas_group->item_list;
      list_end = canvas_group->item_list_end;

      canvas_group->item_list     = &tmp_background;
      canvas_group->item_list_end = &tmp_features;

      (item_parent_class_G->update)(item, i2w_dx, i2w_dy, flags);

      canvas_group->item_list     = list;
      canvas_group->item_list_end = list_end;

    }
  else
    {
      (item_parent_class_G->update)(item, i2w_dx, i2w_dy, flags);
    }

  if(rect && item_visible)
    {
      gboolean need_2nd_update = TRUE;

      if(doing_reposition)
	{
	  maximise_background_rectangle(this_container, item, rect);

	  /* Update the current reposition_x, and reposition_y coords */
	  if(parent_container)
	    {
	      double dx, dy;
	      
	      if(ZMAP_CONTAINER_GROUP_GET_CLASS(this_container)->reposition_group)
		(ZMAP_CONTAINER_GROUP_GET_CLASS(this_container)->reposition_group)(this_container, 
										   rect->x1, rect->y1, 
										   rect->x2, rect->y2, 
										   &dx, &dy);
	      
	      parent_container->reposition_x += dx;
	      parent_container->reposition_y += dy;
	    }

	  zmap_window_container_invoke_post_update_hooks(this_container, 
							 rect->x1, rect->y1,
							 rect->x2, rect->y2);
	}

      /* The background needs updating now so that the canvas knows
       * where it is (canvas coords) for events. We are only setting
       * the points to match the containers bounds or within so no
       * need to re-update the whole tree...phew... */
      if(need_2nd_update)
	{
	  FooCanvasItem *update_items[3] = {NULL};
	  FooCanvasPoints scroll_region;
	  double coords[4];
	  int i = 0, update_flags = flags;

	  i2w_dx += canvas_group->xpos;
	  i2w_dy += canvas_group->ypos;

	  update_items[i++] = (FooCanvasItem *)rect;
	  update_items[i++] = (FooCanvasItem *)overlay;
	  update_items[i++] = (FooCanvasItem *)underlay;

	  zmap_window_container_scroll_region_get_item_bounds(update_items[0], 
							      &coords[0], &coords[1],
							      &coords[2], &coords[3]);
	  scroll_region.coords     = &coords[0];
	  scroll_region.num_points = 2;
	  scroll_region.ref_count  = 1;

	  /* We need to maximise overlays and underlays if required too. */
	  if(overlay)
	    zmapWindowContainerOverlayMaximiseItems(overlay, 
						    rect->x1, rect->y1,
						    rect->x2, rect->y2);

	  if(underlay)
	    zmapWindowContainerUnderlayMaximiseItems(underlay, 
						     rect->x1, rect->y1,
						     rect->x2, rect->y2);

	  for(i = 0; i < 3; i++)
	    {
	      zmap_window_container_update_with_crop(update_items[i], i2w_dx, i2w_dy, &scroll_region, update_flags);
	    }
	}
    }


  /* Always do these, whatever else went on. No question! */
  this_container->reposition_x          = 0.0;
  this_container->reposition_y          = 0.0;
  this_container->flags.need_reposition = FALSE;

  return ;
}


static void zmap_window_container_group_reposition(ZMapWindowContainerGroup container_group, 
						   double  rect_x1,  double  rect_y1,
						   double  rect_x2,  double  rect_y2,
						   double *dx_repos, double *dy_repos)
{
  double dx, dy;

  dx = dy = 0.0;

  dx = (rect_x2 - rect_x1 + 1) + container_group->this_spacing;

  if(dx_repos)
    *dx_repos = dx;

  if(dy_repos)
    *dy_repos = dy;

  return ;
}


/* helper to zmapWindowContainerGroupRemoveUpdateHook() */
static gint find_update_hook_cb(gconstpointer list_data, gconstpointer query_data)
{
  ContainerUpdateHook current, query;
  gint zero_when_matched = -1;

  current = (ContainerUpdateHook)list_data;
  query   = (ContainerUpdateHook)query_data;

  if(current->hook_func == query->hook_func &&
     current->hook_data == query->hook_data)
    zero_when_matched = 0;

  return zero_when_matched;
}


