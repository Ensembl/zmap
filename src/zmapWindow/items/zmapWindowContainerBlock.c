/*  File: zmapWindowContainerBlock.c
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
 * Last edited: Jun  8 10:20 2009 (rds)
 * Created: Mon Jul 30 13:09:33 2007 (rds)
 * CVS info:   $Id: zmapWindowContainerBlock.c,v 1.5 2009-06-08 09:43:37 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapSeqBitmap.h>
#include <zmapWindowContainerBlock_I.h>
#include <zmapWindowContainerFeatureSet_I.h>
#include <zmapWindowContainerChildren_I.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindow_P.h>	/* ITEM_FEATURE_SET_DATA */

enum
  {
    ITEM_FEATURE_BLOCK_0,		/* zero == invalid prop value */
  };

static ZMapSeqBitmap get_bitmap_for_key(ZMapWindowContainerBlock block_data,
					ZMapFeatureBlock               block, 
					GQuark                         key);


static void zmap_window_container_block_class_init  (ZMapWindowContainerBlockClass block_data_class);
static void zmap_window_container_block_init        (ZMapWindowContainerBlock block_data);
static void zmap_window_container_block_set_property(GObject      *gobject, 
						      guint         param_id, 
						      const GValue *value, 
						      GParamSpec   *pspec);
static void zmap_window_container_block_get_property(GObject    *gobject, 
						      guint       param_id, 
						      GValue     *value, 
						      GParamSpec *pspec);
static void zmap_window_container_block_dispose     (GObject *object);
static void zmap_window_container_block_finalize    (GObject *object);


static GObjectClass *parent_class_G = NULL;


GType zmapWindowContainerBlockGetType(void)
{
  static GType group_type = 0;
  
  if (!group_type) 
    {
      static const GTypeInfo group_info = 
	{
	  sizeof (zmapWindowContainerBlockClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_window_container_block_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (zmapWindowContainerBlock),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) zmap_window_container_block_init
      };
    
    group_type = g_type_register_static (ZMAP_TYPE_CONTAINER_GROUP,
					 ZMAP_WINDOW_CONTAINER_BLOCK_NAME,
					 &group_info,
					 0);
  }
  
  return group_type;
}


ZMapWindowContainerBlock zmapWindowContainerBlockAugment(ZMapWindowContainerBlock container_block)
{

  return container_block;
}

void zmapWindowContainerBlockAddCompressedColumn(ZMapWindowContainerBlock block_data, 
						   FooCanvasGroup *container)
{

  block_data->compressed_cols = g_list_append(block_data->compressed_cols, container) ;

  return ;
}

GList *zmapWindowContainerBlockRemoveCompressedColumns(ZMapWindowContainerBlock block_data)
{
  GList *list = NULL;

  list = block_data->compressed_cols;
  block_data->compressed_cols = NULL;

  return list;
}

void zmapWindowContainerBlockAddBumpedColumn(ZMapWindowContainerBlock block_data, 
					     FooCanvasGroup *container)
{

  if(ZMAP_IS_CONTAINER_FEATURESET(container))
    {
      block_data->bumped_cols = g_list_append(block_data->bumped_cols, container) ;
    }

  return ;
}

GList *zmapWindowContainerBlockRemoveBumpedColumns(ZMapWindowContainerBlock block_data)
{
  GList *list = NULL;

  list = block_data->bumped_cols;
  block_data->bumped_cols = NULL;

  return list;
}

static gboolean maximise_mark_items_cb(ZMapWindowContainerGroup group_updated, 
				       FooCanvasPoints         *group_bounds,
				       ZMapContainerLevelType   group_level,
				       gpointer                 user_data)
{
  ZMapWindowContainerBlock container_block = (ZMapWindowContainerBlock)group_updated;
  FooCanvasRE *mark_item_rectangle;
  gboolean status = TRUE;

  zMapAssert(group_level == ZMAPCONTAINER_LEVEL_BLOCK);

  if(container_block->mark.top_item)
    {
      mark_item_rectangle = (FooCanvasRE *)container_block->mark.top_item;

      mark_item_rectangle->x1 = group_bounds->coords[0];
      mark_item_rectangle->y1 = group_bounds->coords[1];
      mark_item_rectangle->x2 = group_bounds->coords[2];

      mark_item_rectangle->y2 = container_block->mark.start;
    }

  if(container_block->mark.bottom_item)
    {
      mark_item_rectangle = (FooCanvasRE *)container_block->mark.bottom_item;

      mark_item_rectangle->x1 = group_bounds->coords[0];
      /*  */
      mark_item_rectangle->y1 = container_block->mark.end;
      mark_item_rectangle->x2 = group_bounds->coords[2];
      mark_item_rectangle->y2 = group_bounds->coords[3];
    }

  return status;
}


void zmapWindowContainerBlockMark(ZMapWindowContainerBlock container_block,
				  GdkColor  *mark_colour,
				  GdkBitmap *mark_stipple,
				  double start, double end)
{
  ZMapWindowContainerGroup container;
  ZMapWindowContainerOverlay overlay;
  ZMapWindowContainerBackground background;

  container = (ZMapWindowContainerGroup)container_block;

  if((overlay = zmapWindowContainerGetOverlay(container)) &&
     (background = zmapWindowContainerGetBackground(container)))
    {
      FooCanvasGroup *parent;
      double x1, x2, y1, y2;
#ifdef DEBUG_MARK_WITH_OUTLINE
      GdkColor outline;

      gdk_color_parse("red", &outline);
#endif /* DEBUG_MARK_WITH_OUTLINE */
      parent = (FooCanvasGroup *)overlay;

      /* We get the current bounds of the background.  It will have
       * been cropped to the scroll region. The UpdateHook takes care
       * of maximising it. */
      foo_canvas_item_get_bounds((FooCanvasItem *)background, &x1, &y1, &x2, &y2);

      container_block->mark.start = start;

      container_block->mark.top_item = 
	foo_canvas_item_new(parent,
			    FOO_TYPE_CANVAS_RECT,
#ifdef DEBUG_MARK_WITH_OUTLINE
			    "outline_color_gdk", &outline,
			    "width_pixels",   1,
#endif /* DEBUG_MARK_WITH_OUTLINE */
			    "fill_color_gdk", mark_colour,
			    "fill_stipple",   mark_stipple,
			    "x1",             x1,
			    "x2",             x2,
			    "y1",             y1,
			    "y2",             start,
			    NULL);

      container_block->mark.end = end;

      container_block->mark.bottom_item =
	foo_canvas_item_new(parent,
			    FOO_TYPE_CANVAS_RECT,
#ifdef DEBUG_MARK_WITH_OUTLINE
			    "outline_color_gdk", &outline,
			    "width_pixels",   1,
#endif /* DEBUG_MARK_WITH_OUTLINE */
			    "fill_color_gdk", mark_colour,
			    "fill_stipple",   mark_stipple,
			    "x1",             x1,
			    "x2",             x2,
			    "y1",             end,
			    "y2",             y2,
			    NULL);

      zmapWindowContainerGroupAddUpdateHook(container, maximise_mark_items_cb, 
					    &(container_block->mark));
    }

  return ;
}

void zmapWindowContainerBlockUnmark(ZMapWindowContainerBlock container_block)
{
  ZMapWindowContainerGroup container;
  ZMapWindowContainerOverlay overlay;

  container = (ZMapWindowContainerGroup)container_block;

  zmapWindowContainerGroupRemoveUpdateHook(container, maximise_mark_items_cb,
					   &(container_block->mark));
  container_block->mark.top_item    = NULL;
  container_block->mark.bottom_item = NULL;
  container_block->mark.start = 0.0;
  container_block->mark.end   = 0.0;

  if((overlay = zmapWindowContainerGetOverlay(container)))
    {
      zmapWindowContainerUtilsRemoveAllItems((FooCanvasGroup *)overlay);
    }

  return ;
}

void zmapWindowContainerBlockMarkRegion(ZMapWindowContainerBlock block_data,
					ZMapFeatureBlock         block)
{
  ZMapSeqBitmap bitmap;

  if((bitmap = get_bitmap_for_key(block_data, block, block->unique_id)))
    {
      if(block->features_start == 0)
	block->features_start = block->block_to_sequence.q1;
      
      if(block->features_end == 0)
	block->features_end = block->block_to_sequence.q2;

      zmapSeqBitmapMarkRegion(bitmap, block->features_start, block->features_end);
    }

  return ;
}

void zmapWindowContainerBlockMarkRegionForColumn(ZMapWindowContainerBlock       container_block,
						 ZMapFeatureBlock               block, 
						 ZMapWindowContainerFeatureSet  container_set)
{
  if(ZMAP_IS_CONTAINER_BLOCK(container_block))
    {
      ZMapSeqBitmap bitmap;

      if((bitmap = get_bitmap_for_key(container_block, block, container_set->unique_id)))
	{
	  if(block->features_start == 0)
	    block->features_start = block->block_to_sequence.q1;
	  
	  if(block->features_end == 0)
	    block->features_end = block->block_to_sequence.q2;
	  
	  zmapSeqBitmapMarkRegion(bitmap, block->features_start, block->features_end);
	}
    }

  return ;
}

GList *zmapWindowContainerBlockFilterMarkedColumns(ZMapWindowContainerBlock block_data,
						   GList *list, int world1, int world2)
{
  GList *key_list;

  key_list = list;

  while(key_list)
    {
      ZMapSeqBitmap bitmap;
      GQuark key;
      GList *tmp_list;

      tmp_list = key_list->next;

      key = GPOINTER_TO_UINT(key_list->data);

      /* we don't want to create anything... */
      if((bitmap = get_bitmap_for_key(block_data, NULL, key)))
	{
	  if(zmapSeqBitmapIsRegionFullyMarked(bitmap, world1, world2))
	    list = g_list_delete_link(list, key_list);
	}

      key_list = tmp_list;
    }

  return list;
}

gboolean zmapWindowContainerBlockIsColumnLoaded(ZMapWindowContainerBlock      container_block,
						ZMapWindowContainerFeatureSet container_set, 
						int world1, int world2)
{
  ZMapSeqBitmap bitmap;
  gboolean fully_marked = FALSE;

  if((bitmap = get_bitmap_for_key(container_block, NULL, container_set->unique_id)))
    {
      fully_marked = zmapSeqBitmapIsRegionFullyMarked(bitmap, world1, world2);
    }
    
  return fully_marked;
}

ZMapWindowContainerBlock zmapWindowContainerBlockDestroy(ZMapWindowContainerBlock container_block)
{
  g_object_unref(G_OBJECT(container_block));

  container_block = NULL;

  return container_block;
}


/* INTERNAL */

static ZMapSeqBitmap get_bitmap_for_key(ZMapWindowContainerBlock block_data,
					ZMapFeatureBlock         block, 
					GQuark                   key)
{
  ZMapSeqBitmap bitmap = NULL;

  if(!(bitmap = g_hash_table_lookup(block_data->loaded_region_hash, GUINT_TO_POINTER(key))))
    {
      if(block)
	{
	  int length;
	  
	  length = block->block_to_sequence.q2 - block->block_to_sequence.q1 + 1;
	  
	  bitmap = zmapSeqBitmapCreate(block->block_to_sequence.q1, length, 9000);
	  
	  g_hash_table_insert(block_data->loaded_region_hash, GUINT_TO_POINTER(key), bitmap);
	}
    }

  return bitmap;
}



/* Object code */
static void zmap_window_container_block_class_init(ZMapWindowContainerBlockClass block_data_class)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *)block_data_class;

  gobject_class->set_property = zmap_window_container_block_set_property;
  gobject_class->get_property = zmap_window_container_block_get_property;

  parent_class_G = g_type_class_peek_parent(block_data_class);

#ifdef RDS_DONT_INCLUDE
  /* width */
  g_object_class_install_property(gobject_class, 
				  ITEM_FEATURE_BLOCK_WIDTH,
				  g_param_spec_double(ZMAPSTYLE_PROPERTY_WIDTH, 
						      ZMAPSTYLE_PROPERTY_WIDTH,
						      "The minimum width the column should be displayed at.",
						      0.0, 32000.00, 16.0, 
						      ZMAP_PARAM_STATIC_RO));
#endif

  gobject_class->dispose  = zmap_window_container_block_dispose;
  gobject_class->finalize = zmap_window_container_block_finalize;

  return ;
}

static void zmap_window_container_block_init(ZMapWindowContainerBlock block_data)
{
  block_data->loaded_region_hash = g_hash_table_new_full(NULL, NULL, NULL, (GDestroyNotify)zmapSeqBitmapDestroy);
  block_data->compressed_cols    = NULL;
  return ;
}

static void zmap_window_container_block_set_property(GObject      *gobject, 
						      guint         param_id, 
						      const GValue *value, 
						      GParamSpec   *pspec)
{

  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_window_container_block_get_property(GObject    *gobject, 
							guint       param_id, 
							GValue     *value, 
							GParamSpec *pspec)
{
  ZMapWindowContainerBlock block_data;

  block_data = ZMAP_CONTAINER_BLOCK(gobject);

  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_window_container_block_dispose(GObject *object)
{
  ZMapWindowContainerBlock block_data;
  GObjectClass *gobject_class = G_OBJECT_CLASS(parent_class_G);

  block_data = ZMAP_CONTAINER_BLOCK(object);

  block_data->window = NULL;	/* not ours */

  /* compressed and bumped columns are not ours. canvas owns them, just free the lists */
  if(block_data->compressed_cols)
    {
      g_list_free(block_data->compressed_cols) ;
      block_data->compressed_cols = NULL;
    }

  if(block_data->bumped_cols)
    {
      g_list_free(block_data->bumped_cols);
      block_data->bumped_cols = NULL;
    }

  if(block_data->loaded_region_hash)
    {
      g_hash_table_destroy(block_data->loaded_region_hash);
      block_data->loaded_region_hash = NULL;
    }

  if(gobject_class->dispose)
    (*gobject_class->dispose)(object);

  return ;
}

static void zmap_window_container_block_finalize(GObject *object)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(parent_class_G);
  
  if(gobject_class->finalize)
    (*gobject_class->finalize)(object);

  return ;
}

