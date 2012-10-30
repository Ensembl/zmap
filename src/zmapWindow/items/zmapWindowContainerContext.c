/*  File: zmapWindowContainerContext.c
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






#include <ZMap/zmapBase.h>
//#include <zmapWindowCanvas.h>
#include <zmapWindowContainerGroup_I.h>
#include <zmapWindowContainerContext_I.h>
#include <ZMap/zmapWindow.h>


#if GROUP_REPOS
enum
  {
    CONTAINER_CONTEXT_PROP_0,	/* zero is invalid in gobject properties */
    CONTAINER_CONTEXT_PROP_REPOSITION,
    CONTAINER_CONTEXT_PROP_DEBUG_XML,
    CONTAINER_CONTEXT_PROP_DEBUG,
  };
#endif


static void zmap_window_container_context_class_init  (ZMapWindowContainerContextClass block_data_class);
static void zmap_window_container_context_init        (ZMapWindowContainerContext block_data);
static void zmap_window_container_context_set_property(GObject      *gobject,
						      guint         param_id,
						      const GValue *value,
						      GParamSpec   *pspec);
static void zmap_window_container_context_get_property(GObject    *gobject,
						      guint       param_id,
						      GValue     *value,
						      GParamSpec *pspec);
#ifdef EXTRA_DATA_NEEDS_FREE
static void zmap_window_container_context_dispose     (GObject *object);
static void zmap_window_container_context_finalize    (GObject *object);
#endif /* EXTRA_DATA_NEEDS_FREE */

static void zmap_window_container_context_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags);

static GObjectClass *parent_class_G = NULL;
static FooCanvasItemClass *parent_item_class_G = NULL;

GType zmapWindowContainerContextGetType(void)
{
  static GType group_type = 0;

  if (!group_type)
    {
      static const GTypeInfo group_info =
	{
	  sizeof (zmapWindowContainerContextClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_window_container_context_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (zmapWindowContainerContext),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) zmap_window_container_context_init
      };

    group_type = g_type_register_static (ZMAP_TYPE_CONTAINER_GROUP,
					 ZMAP_WINDOW_CONTAINER_CONTEXT_NAME,
					 &group_info,
					 0);
  }

  return group_type;
}



ZMapWindowContainerContext zmapWindowContainerContextDestroy(ZMapWindowContainerContext container_context)
{
  g_object_unref(G_OBJECT(container_context));

  container_context = NULL;

  return container_context;
}


/* INTERNAL */
/* Object code */
static void zmap_window_container_context_class_init(ZMapWindowContainerContextClass context_class)
{
  GObjectClass *gobject_class;
  FooCanvasItemClass *item_class;
  zmapWindowContainerGroupClass *group_class ;

  gobject_class = (GObjectClass *)context_class;
  item_class    = (FooCanvasItemClass *)context_class;
  group_class = (zmapWindowContainerGroupClass *)context_class ;

  gobject_class->set_property = zmap_window_container_context_set_property;
  gobject_class->get_property = zmap_window_container_context_get_property;

  parent_class_G = g_type_class_peek_parent(context_class);
  parent_item_class_G = (FooCanvasItemClass *)parent_class_G;

  group_class->obj_size = sizeof(zmapWindowContainerContextStruct) ;
  group_class->obj_total = 0 ;

  item_class->update = zmap_window_container_context_update;

#if GROUP_REPOS
  g_object_class_install_property(gobject_class, CONTAINER_CONTEXT_PROP_REPOSITION,
				  g_param_spec_boolean("need-reposition", "need reposition",
						       "Container needs repositioning in update",
						       FALSE, ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class, CONTAINER_CONTEXT_PROP_DEBUG_XML,
				  g_param_spec_boolean("debug-xml", "debug xml",
						       "For reposition updates print debug xml",
						       FALSE, ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class, CONTAINER_CONTEXT_PROP_DEBUG,
				  g_param_spec_boolean("debug", "debug",
						       "For reposition update print debug text",
						       FALSE, ZMAP_PARAM_STATIC_RW));
#endif

#ifdef EXTRA_DATA_NEEDS_FREE
  gobject_class->dispose  = zmap_window_container_context_dispose;
  gobject_class->finalize = zmap_window_container_context_finalize;
#endif /* EXTRA_DATA_NEEDS_FREE */

  return ;
}

static void zmap_window_container_context_init(ZMapWindowContainerContext block_data)
{

  return ;
}

static void zmap_window_container_context_set_property(GObject      *gobject,
						       guint         param_id,
						       const GValue *value,
						       GParamSpec   *pspec)
{
  ZMapWindowContainerGroup container;

  g_return_if_fail(ZMAP_IS_CONTAINER_GROUP(gobject));

  container = ZMAP_CONTAINER_GROUP(gobject);

  switch(param_id)
    {
#if GROUP_REPOS
    case CONTAINER_CONTEXT_PROP_REPOSITION:
      {
	container->flags.need_reposition = g_value_get_boolean(value);
	if(container->flags.need_reposition)
	  foo_canvas_item_request_update((FooCanvasItem *)container);
      }
      break;
    case CONTAINER_CONTEXT_PROP_DEBUG_XML:
    case CONTAINER_CONTEXT_PROP_DEBUG:
      {
	gboolean flag = FALSE;
	flag = g_value_get_boolean(value);
	if(param_id == CONTAINER_CONTEXT_PROP_DEBUG)
	  container->flags.debug_text = flag;
	else
	  container->flags.debug_xml = flag;
      }
      break;
#endif

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_window_container_context_get_property(GObject    *gobject,
						       guint       param_id,
						       GValue     *value,
						       GParamSpec *pspec)
{
  ZMapWindowContainerContext container_context;

  container_context = ZMAP_CONTAINER_CONTEXT(gobject);

  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

#ifdef EXTRA_DATA_NEEDS_FREE
static void zmap_window_container_context_dispose(GObject *object)
{
  return ;
}

static void zmap_window_container_context_finalize(GObject *object)
{
  return ;
}
#endif /* EXTRA_DATA_NEEDS_FREE */


#if GROUP_REPOS
static void dump_item_xml(gpointer item_data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)item_data;
  double x1, y1, x2, y2;
  int *indent = (int *)user_data;
  int i;


  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2);
  for(i = 0; i < *indent; i++)
    {
      printf("  ");
    }

  printf("<%s world=\"%f,%f,%f,%f\" visible=\"%s\" canvas=\"%f,%f,%f,%f\" ",
	 G_OBJECT_TYPE_NAME(item),
	 x1, y1, x2, y2,
	 item->object.flags & FOO_CANVAS_ITEM_VISIBLE ? "yes" : "no",
	 item->x1, item->y1, item->x2, item->y2);



  if(FOO_IS_CANVAS_GROUP(item))
    {
      FooCanvasGroup *group;
      group = (FooCanvasGroup *)item;
      if(group->item_list)
	{
	  (*indent)++;
	  printf(">\n");
	  g_list_foreach(group->item_list, dump_item_xml, user_data);
	  (*indent)--;
	  for(i = 0; i < *indent; i++)
	    {
	      printf("  ");
	    }
	  printf("</%s>\n", G_OBJECT_TYPE_NAME(item));
	}
      else
	printf("/>\n");
    }
  else
    printf("/>\n");

  return ;
}

static void dump_item(gpointer item_data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)item_data;
  double x1, y1, x2, y2;
  int *indent = (int *)user_data;
  int i;


  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2);
  for(i = 0; i < *indent; i++)
    {
      printf("  ");
    }

  printf("item (%s) %f,%f -> %f,%f [visible=%s, canvas=%f,%f -> %f,%f]\n",
	 G_OBJECT_TYPE_NAME(item),
	 x1, y1, x2, y2,
	 item->object.flags & FOO_CANVAS_ITEM_VISIBLE ? "yes" : "no",
	 item->x1, item->y1, item->x2, item->y2);

  if(FOO_IS_CANVAS_GROUP(item))
    {
      FooCanvasGroup *group;
      (*indent)++;
      group = (FooCanvasGroup *)item;
      g_list_foreach(group->item_list, dump_item, user_data);
      (*indent)--;
    }

  return ;
}


static void dump_container(ZMapWindowContainerGroup container)
{
  int indent = 0;

  if(container->flags.debug_xml)
    {
      FooCanvas *canvas;
      indent++;
      canvas = ((FooCanvasItem *)container)->canvas;
      printf("<%s pointer=\"%p\" >\n",
	     G_OBJECT_TYPE_NAME(canvas), canvas);

      dump_item_xml(container, &indent);
      printf("</%s>\n", G_OBJECT_TYPE_NAME(canvas));
    }
  else if(container->flags.debug_text)
    {
      printf("From Canvas %p\n", ((FooCanvasItem *)container)->canvas);
      dump_item(container, &indent);
    }

  return ;
}
#endif






#if GROUP_REPOS

static void reposition_update(FooCanvasItemClass *item_class,
			      FooCanvasItem      *item,
			      double i2w_dx,
			      double i2w_dy,
			      int    flags)
{

  FooCanvas *canvas;
  gboolean need_crop = FALSE;

  container = (ZMapWindowContainerGroup)item;

  container->reposition_x = container->reposition_y = 0.0;

  if((canvas = item->canvas))
    {
      int pixel_height, max = (1 << 15) - 1000;

      pixel_height = (int)(container->height * canvas->pixels_per_unit_y);
      if(pixel_height > max)
	{
	  need_crop = TRUE;
	  flags |= ZMAP_CANVAS_UPDATE_CROP_REQUIRED;
	}
    }

  /* this is a little bit of a hack to make sure we visit every item
   * to do the repositioning! */
  if(container->flags.need_reposition == TRUE)
    flags |= FOO_CANVAS_UPDATE_DEEP | ZMAP_CANVAS_UPDATE_NEED_REPOSITION;

  if(flags & FOO_CANVAS_UPDATE_DEEP)
    flags |= ZMAP_CANVAS_UPDATE_NEED_REPOSITION;

  if(item_class->update)
    (item_class->update)(item, i2w_dx, i2w_dy, flags);


  if(flags & ZMAP_CANVAS_UPDATE_NEED_REPOSITION)
    {
      dump_container(container);
    }

  container->flags.need_reposition = FALSE;
  container->reposition_x = container->reposition_y = 0.0;

  return ;
}
#endif



static void zmap_window_container_context_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags)
{
  if(parent_item_class_G->update)
    (parent_item_class_G->update)(item, i2w_dx, i2w_dy, flags);

  zMapWindowResetWindowWidth(item);

  return ;
}

