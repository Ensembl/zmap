/*  File: zmapWindowItemFeatureBlock.c
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
 * Last edited: Apr 20 11:20 2009 (rds)
 * Created: Mon Jul 30 13:09:33 2007 (rds)
 * CVS info:   $Id: zmapWindowItemFeatureBlock.c,v 1.3 2009-04-20 11:05:28 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapSeqBitmap.h>
#include <zmapWindowItemFeatureBlock_I.h>
#include <zmapWindowItemFeatureSet.h>
#include <zmapWindow_P.h>	/* ITEM_FEATURE_SET_DATA */

enum
  {
    ITEM_FEATURE_BLOCK_0,		/* zero == invalid prop value */
  };

static ZMapSeqBitmap get_bitmap_for_style(ZMapWindowItemFeatureBlockData block_data,
					  ZMapFeatureBlock               block, 
					  ZMapFeatureTypeStyle           style);
static ZMapSeqBitmap get_bitmap_for_key(ZMapWindowItemFeatureBlockData block_data,
					ZMapFeatureBlock               block, 
					GQuark                         key);


static void zmap_window_item_feature_block_class_init  (ZMapWindowItemFeatureBlockDataClass block_data_class);
static void zmap_window_item_feature_block_init        (ZMapWindowItemFeatureBlockData block_data);
static void zmap_window_item_feature_block_set_property(GObject      *gobject, 
						      guint         param_id, 
						      const GValue *value, 
						      GParamSpec   *pspec);
static void zmap_window_item_feature_block_get_property(GObject    *gobject, 
						      guint       param_id, 
						      GValue     *value, 
						      GParamSpec *pspec);
static void zmap_window_item_feature_block_dispose     (GObject *object);
static void zmap_window_item_feature_block_finalize    (GObject *object);


static GObjectClass *parent_class_G = NULL;


GType zmapWindowItemFeatureBlockGetType(void)
{
  static GType group_type = 0;
  
  if (!group_type) 
    {
      static const GTypeInfo group_info = 
	{
	  sizeof (zmapWindowItemFeatureBlockDataClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_window_item_feature_block_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (zmapWindowItemFeatureBlockData),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) zmap_window_item_feature_block_init
      };
    
    group_type = g_type_register_static (G_TYPE_OBJECT,
					 "zmapWindowItemFeatureBlockData",
					 &group_info,
					 0);
  }
  
  return group_type;
}


ZMapWindowItemFeatureBlockData zmapWindowItemFeatureBlockCreate(ZMapWindow window)
{
  ZMapWindowItemFeatureBlockData block_data;

  if((block_data = g_object_new(zmapWindowItemFeatureBlockGetType(), NULL)))
    {
      block_data->window = window;
    }

  return block_data;
}

void zmapWindowItemFeatureBlockAddCompressedColumn(ZMapWindowItemFeatureBlockData block_data, 
						   FooCanvasGroup *container)
{

  block_data->compressed_cols = g_list_append(block_data->compressed_cols, container) ;

  return ;
}

GList *zmapWindowItemFeatureBlockRemoveCompressedColumns(ZMapWindowItemFeatureBlockData block_data)
{
  GList *list = NULL;

  list = block_data->compressed_cols;
  block_data->compressed_cols = NULL;

  return list;
}

void zmapWindowItemFeatureBlockAddBumpedColumn(ZMapWindowItemFeatureBlockData block_data, 
					       FooCanvasGroup *container)
{

  block_data->bumped_cols = g_list_append(block_data->bumped_cols, container) ;
  
  return ;
}

GList *zmapWindowItemFeatureBlockRemoveBumpedColumns(ZMapWindowItemFeatureBlockData block_data)
{
  GList *list = NULL;

  list = block_data->bumped_cols;
  block_data->bumped_cols = NULL;

  return list;
}

ZMapWindow zmapWindowItemFeatureBlockGetWindow(ZMapWindowItemFeatureBlockData block_data)
{
  ZMapWindow window = NULL;

  g_return_val_if_fail(ZMAP_IS_WINDOW_ITEM_FEATURE_BLOCK(block_data), window);

  window = block_data->window;

  return window;
}

void zmapWindowItemFeatureBlockMarkRegion(ZMapWindowItemFeatureBlockData block_data,
					  ZMapFeatureBlock               block)
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

void zmapWindowItemFeatureBlockMarkRegionForColumn(ZMapWindowItemFeatureBlockData block_data,
						   ZMapFeatureBlock               block, 
						   ZMapWindowItemFeatureSetData   set_data)
{
  ZMapSeqBitmap bitmap;

  if((bitmap = get_bitmap_for_key(block_data, block, set_data->unique_id)))
    {
      if(block->features_start == 0)
	block->features_start = block->block_to_sequence.q1;
      
      if(block->features_end == 0)
	block->features_end = block->block_to_sequence.q2;

      zmapSeqBitmapMarkRegion(bitmap, block->features_start, block->features_end);
    }

  return ;
}

GList *zmapWindowItemFeatureBlockFilterMarkedColumns(ZMapWindowItemFeatureBlockData block_data,
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

gboolean zmapWindowItemFeatureBlockIsColumnLoaded(ZMapWindowItemFeatureBlockData block_data,
						  FooCanvasGroup *column_group, int world1, int world2)
{
  ZMapWindowItemFeatureSetData set_data;
  ZMapSeqBitmap bitmap;
  gboolean fully_marked = FALSE;

  if((set_data = g_object_get_data(G_OBJECT(column_group), ITEM_FEATURE_SET_DATA)))
    {
      if((bitmap = get_bitmap_for_key(block_data, NULL, set_data->unique_id)))
	{
	  fully_marked = zmapSeqBitmapIsRegionFullyMarked(bitmap, world1, world2);
	}
    }

  return fully_marked;
}

ZMapWindowItemFeatureBlockData zmapWindowItemFeatureBlockDestroy(ZMapWindowItemFeatureBlockData item_feature_block)
{
  g_object_unref(G_OBJECT(item_feature_block));

  item_feature_block = NULL;

  return item_feature_block;
}


/* INTERNAL */

static ZMapSeqBitmap get_bitmap_for_style(ZMapWindowItemFeatureBlockData block_data,
					  ZMapFeatureBlock               block, 
					  ZMapFeatureTypeStyle           style)
{
  ZMapSeqBitmap bitmap;
  GQuark key;
  
  key = zMapStyleGetUniqueID(style);

  bitmap = get_bitmap_for_key(block_data, block, key);
  
  return bitmap;
}

static ZMapSeqBitmap get_bitmap_for_key(ZMapWindowItemFeatureBlockData block_data,
					ZMapFeatureBlock               block, 
					GQuark                         key)
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
static void zmap_window_item_feature_block_class_init(ZMapWindowItemFeatureBlockDataClass block_data_class)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *)block_data_class;

  gobject_class->set_property = zmap_window_item_feature_block_set_property;
  gobject_class->get_property = zmap_window_item_feature_block_get_property;

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

  gobject_class->dispose  = zmap_window_item_feature_block_dispose;
  gobject_class->finalize = zmap_window_item_feature_block_finalize;

  return ;
}

static void zmap_window_item_feature_block_init(ZMapWindowItemFeatureBlockData block_data)
{
  block_data->loaded_region_hash = g_hash_table_new_full(NULL, NULL, NULL, (GDestroyNotify)zmapSeqBitmapDestroy);
  block_data->compressed_cols    = NULL;
  return ;
}

static void zmap_window_item_feature_block_set_property(GObject      *gobject, 
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

static void zmap_window_item_feature_block_get_property(GObject    *gobject, 
							guint       param_id, 
							GValue     *value, 
							GParamSpec *pspec)
{
  ZMapWindowItemFeatureBlockData block_data;

  block_data = ZMAP_WINDOW_ITEM_FEATURE_BLOCK(gobject);

  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_window_item_feature_block_dispose(GObject *object)
{
  ZMapWindowItemFeatureBlockData block_data;
  GObjectClass *gobject_class = G_OBJECT_CLASS(parent_class_G);

  block_data = ZMAP_WINDOW_ITEM_FEATURE_BLOCK(object);

  block_data->window = NULL;	/* not ours */

  /* compressed and bumped columns are not ours. canvas owns them, just free the lists */
  if(block_data->compressed_cols)
    g_list_free(block_data->compressed_cols) ;

  if(block_data->bumped_cols)
    g_list_free(block_data->bumped_cols);

  if(block_data->loaded_region_hash)
    g_hash_table_destroy(block_data->loaded_region_hash);

  if(gobject_class->dispose)
    (*gobject_class->dispose)(object);

  return ;
}

static void zmap_window_item_feature_block_finalize(GObject *object)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(parent_class_G);
  
  if(gobject_class->finalize)
    (*gobject_class->finalize)(object);

  return ;
}

