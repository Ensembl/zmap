/*  File: zmapWindowContainerContext.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * Last edited: Jun  3 21:55 2009 (rds)
 * Created: Mon Jul 30 13:09:33 2007 (rds)
 * CVS info:   $Id: zmapWindowContainerContext.c,v 1.2 2009-06-03 22:29:08 rds Exp $
 *-------------------------------------------------------------------
 */
#include <zmapWindowCanvas.h>
#include <zmapWindowContainerContext_I.h>

enum
  {
    CONTAINER_CONTEXT_PROP_0,	/* zero is invalid in gobject properties */
    CONTAINER_CONTEXT_PROP_REPOSITION,
  };

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


ZMapWindowContainerContext zmapWindowContainerContextCreate(ZMapWindow window)
{
  ZMapWindowContainerContext block_data;

  return block_data;
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

  gobject_class = (GObjectClass *)context_class;
  item_class    = (FooCanvasItemClass *)context_class;

  gobject_class->set_property = zmap_window_container_context_set_property;
  gobject_class->get_property = zmap_window_container_context_get_property;

  parent_class_G = g_type_class_peek_parent(context_class);
  parent_item_class_G = (FooCanvasItemClass *)parent_class_G;

  item_class->update = zmap_window_container_context_update;

  g_object_class_install_property(gobject_class, CONTAINER_CONTEXT_PROP_REPOSITION,
				  g_param_spec_boolean("need-reposition", "need reposition",
						       "Container needs repositioning in update",
						       FALSE, ZMAP_PARAM_STATIC_RW));

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
    case CONTAINER_CONTEXT_PROP_REPOSITION:
      {
	container->flags.need_reposition = TRUE;
	foo_canvas_item_request_update((FooCanvasItem *)container);
      }
      break;
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


static void reposition_update(FooCanvasItemClass *item_class,
			      FooCanvasItem      *item,
			      double i2w_dx, 
			      double i2w_dy, 
			      int    flags)
{
  ZMapWindowContainerGroup container;
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

  if(item_class->update)
    (item_class->update)(item, i2w_dx, i2w_dy, flags);
      
  container->flags.need_reposition = FALSE;
  container->reposition_x = container->reposition_y = 0.0;
  
  return ;
}

static void zmap_window_container_context_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags)
{
  
  reposition_update(parent_item_class_G, item, i2w_dx, i2w_dy, flags);

  return ;
}

