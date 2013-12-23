/*  File: zmapWindowOldItems.c
 *  Author: ed (edgrif@sanger.ac.uk)
 *  Copyright (c) 2013: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Contains functions from Roy Storey's rewrite of the
 *              original drawing code. Roy's code was all in
 *              zmapWindow/items and has now been subsumed/replaced
 *              by Malcolm Hinsley's rewrite contained in
 *              zmapWindow/canvas.
 *              
 *              The intention is to gradually remove all of Roy's
 *              original code but for now it is in this file.
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <string.h>

#include <zmapWindowCanvasItem_I.h>
#include <ZMap/zmapUtilsFoo.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowContainerAlignment_I.h>
#include <zmapWindowContainerBlock_I.h>
#include <zmapWindowContainerContext_I.h>
#include <zmapWindowContainerFeatureSet_I.h>




enum
  {
    CONTAINER_PROP_0,		/* zero is invalid */
    CONTAINER_PROP_VISIBLE,
  };


typedef struct EachIntervalDataStructType
{
  ZMapWindowCanvasItem parent;
  ZMapFeature          feature;
  ZMapFeatureSubPartSpan sub_feature;
  ZMapStyleColourType  style_colour_type;
  int 			colour_flags;
  GdkColor            *default_fill_colour;
  GdkColor            *border_colour;
  ZMapWindowCanvasItemClass klass;
} EachIntervalDataStruct, *EachIntervalData;


/* Surprisingly I thought this would be typedef somewhere in the GType header but not so... */
typedef GType (*MyGTypeFunc)(void) ;


typedef struct MyGObjectInfoStructName
{
  MyGTypeFunc get_type_func ;
  GType obj_type ;
  char *obj_name ;
  unsigned int obj_size ;
} MyGObjectInfoStruct, *MyGObjectInfo ;




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
static void zmap_window_container_block_destroy     (GtkObject *gtkobject);



static void zmap_window_container_context_class_init  (ZMapWindowContainerContextClass block_data_class);
static void zmap_window_container_context_set_property(GObject      *gobject,
						       guint         param_id,
						       const GValue *value,
						       GParamSpec   *pspec);
static void zmap_window_container_context_get_property(GObject    *gobject,
						       guint       param_id,
						       GValue     *value,
						       GParamSpec *pspec);
static void zmap_window_container_context_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags);

static void zmap_window_container_alignment_class_init  (ZMapWindowContainerAlignmentClass container_align_class);
static void zmap_window_container_alignment_set_property(GObject      *gobject, 
							 guint         param_id, 
							 const GValue *value, 
							 GParamSpec   *pspec);
static void zmap_window_container_alignment_get_property(GObject    *gobject, 
							 guint       param_id, 
							 GValue     *value, 
							 GParamSpec *pspec);


static GObjectClass *parent_class_G = NULL ;
static FooCanvasItemClass *parent_item_class_G = NULL ;






/* 
 *             CONTEXT ROUTINES
 */

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
	  (GInstanceInitFunc) NULL
	};

      group_type = g_type_register_static (ZMAP_TYPE_CONTAINER_GROUP,
					   ZMAP_WINDOW_CONTAINER_CONTEXT_NAME,
					   &group_info,
					   0);
    }

  return group_type;
}


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

#ifdef EXTRA_DATA_NEEDS_FREE
  gobject_class->dispose  = zmap_window_container_context_dispose;
  gobject_class->finalize = zmap_window_container_context_finalize;
#endif /* EXTRA_DATA_NEEDS_FREE */

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


static void zmap_window_container_context_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags)
{
  if(parent_item_class_G->update)
    (parent_item_class_G->update)(item, i2w_dx, i2w_dy, flags);

  //  zMapWindowResetWindowWidth(item);

  return ;
}









/* 
 *    ALIGNMENT ROUTINES
 */


ZMapWindowContainerAlignment zmapWindowContainerAlignmentAugment(ZMapWindowContainerAlignment alignment,
								 ZMapFeatureAlignment feature)
{
  zmapWindowContainerAttachFeatureAny((ZMapWindowContainerGroup)alignment,
				      (ZMapFeatureAny)feature);

  return alignment;
}

GType zmapWindowContainerAlignmentGetType(void)
{
  static GType group_type = 0;
  
  if (!group_type) 
    {
      static const GTypeInfo group_info = 
	{
	  sizeof (zmapWindowContainerAlignmentClass),
	  (GBaseInitFunc)NULL,
	  (GBaseFinalizeFunc)NULL,
	  (GClassInitFunc)zmap_window_container_alignment_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (zmapWindowContainerAlignment),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc)NULL
	};
    
      group_type = g_type_register_static (ZMAP_TYPE_CONTAINER_GROUP,
					   ZMAP_WINDOW_CONTAINER_ALIGNMENT_NAME,
					   &group_info,
					   0);
    }
  
  return group_type;
}


static void zmap_window_container_alignment_class_init(ZMapWindowContainerAlignmentClass container_align_class)
{
  GObjectClass *gobject_class;
  zmapWindowContainerGroupClass *group_class ;

  gobject_class = (GObjectClass *)container_align_class;
  group_class = (zmapWindowContainerGroupClass *)container_align_class ;

  gobject_class->set_property = zmap_window_container_alignment_set_property;
  gobject_class->get_property = zmap_window_container_alignment_get_property;
#ifdef CHAINING_REQUIRED
  parent_class_G = g_type_class_peek_parent(container_align_class);
#endif /* CHAINING_REQUIRED */

  group_class->obj_size = sizeof(zmapWindowContainerAlignmentStruct) ;
  group_class->obj_total = 0 ;

#ifdef EXTRA_DATA_NEEDS_FREE
  gobject_class->dispose  = zmap_window_container_alignment_dispose;
  gobject_class->finalize = zmap_window_container_alignment_finalize;
#endif /* EXTRA_DATA_NEEDS_FREE */

  return ;
}


static void zmap_window_container_alignment_set_property(GObject      *gobject, 
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

static void zmap_window_container_alignment_get_property(GObject    *gobject, 
							 guint       param_id, 
							 GValue     *value, 
							 GParamSpec *pspec)
{
  ZMapWindowContainerAlignment container_align;

  container_align = ZMAP_CONTAINER_ALIGNMENT(gobject);

  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}



/* 
 *             BLOCK ROUTINES
 */


/*!
 * \brief Get the GType for the ZMapWindowContainerBlock GObjects
 *
 * \return GType corresponding to the GObject as registered by glib.
 */

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

/* Test whether there are compressed cols. */
gboolean zmapWindowContainerBlockIsCompressedColumn(ZMapWindowContainerBlock block_data)
{
  gboolean result = FALSE ;

  if (block_data->compressed_cols)
    result = TRUE ;

  return result ;
}



/*!
 * \brief Once a new ZMapWindowContainerBlock object has been created,
 *        various parameters need to be set.  This sets all the params.
 *
 * \param container     The container to set everything on.
 * \param feature       The container needs to know its Block.
 *
 * \return ZMapWindowContainerBlock that was edited.
 */

ZMapWindowContainerBlock zmapWindowContainerBlockAugment(ZMapWindowContainerBlock container_block,
							 ZMapFeatureBlock feature)
{
  zmapWindowContainerAttachFeatureAny((ZMapWindowContainerGroup)container_block,
				      (ZMapFeatureAny)feature);

  return container_block;
}



static void zmap_window_container_block_class_init(ZMapWindowContainerBlockClass block_data_class)
{
  ZMapWindowContainerGroupClass group_class;
  GtkObjectClass *gtkobject_class;
  GObjectClass *gobject_class;

  gobject_class   = (GObjectClass *)block_data_class;
  gtkobject_class = (GtkObjectClass *)block_data_class;
  group_class     = (ZMapWindowContainerGroupClass)block_data_class;

  gobject_class->set_property = zmap_window_container_block_set_property;
  gobject_class->get_property = zmap_window_container_block_get_property;

  parent_class_G = g_type_class_peek_parent(block_data_class);

  group_class->obj_size = sizeof(zmapWindowContainerBlockStruct) ;
  group_class->obj_total = 0 ;


  gtkobject_class->destroy = zmap_window_container_block_destroy;

  return ;
}

static void zmap_window_container_block_init(ZMapWindowContainerBlock block_data)
{
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



static void zmap_window_container_block_destroy(GtkObject *gtkobject)
{
  ZMapWindowContainerBlock block_data;
  GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS(parent_class_G);

  block_data = ZMAP_CONTAINER_BLOCK(gtkobject);

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


  if(gtkobject_class->destroy)
    (gtkobject_class->destroy)(gtkobject);

  return ;
}


/*!
 * \brief Remove columns that have been 'compressed' by the application.
 *
 * ZMap allows for the compressing/temporary hiding of columns and we need
 * to keep a list of these.  This gives access to the list and effectively
 * pops the list from the block.
 *
 * \param container   The container.
 *
 * \return the list of columns that the block holds as compressed.
 */
GList *zmapWindowContainerBlockRemoveCompressedColumns(ZMapWindowContainerBlock block_data)
{
  GList *list = NULL;

  list = block_data->compressed_cols;
  block_data->compressed_cols = NULL;

  return list;
}

/*!
 * \brief Remove columns that have been 'bumped' by the application.
 *
 * ZMap allows for the bumping of columns and we need to keep a list of these.
 * This gives access to the list and effectively pops the list from the block.
 *
 * \param container   The container.
 *
 * \return the list of columns that the block holds as bumped.
 */
GList *zmapWindowContainerBlockRemoveBumpedColumns(ZMapWindowContainerBlock block_data)
{
  GList *list = NULL;

  list = block_data->bumped_cols;
  block_data->bumped_cols = NULL;

  return list;
}

/*!
 * \brief Add a column that has been 'bumped' by the application.
 *
 * ZMap allows for the bumping of columns and we need to keep a list of these.
 *
 * \param container   The container.
 * \param column      The column that has been bumped.
 *
 * \return nothing
 */
void zmapWindowContainerBlockAddBumpedColumn(ZMapWindowContainerBlock block_data, FooCanvasGroup *container)
{

  if(ZMAP_IS_CONTAINER_FEATURESET(container))
    {
      block_data->bumped_cols = g_list_append(block_data->bumped_cols, container) ;
    }

  return ;
}

/*!
 * \brief Add a column that has been 'compressed' by the application.
 *
 * ZMap allows for the compressing/temporary hiding of columns and we need
 * to keep a list of these.
 *
 * \param container   The container.
 * \param column      The column that has been compressed.
 *
 * \return nothing
 */
void zmapWindowContainerBlockAddCompressedColumn(ZMapWindowContainerBlock block_data,
						 FooCanvasGroup *container)
{

  block_data->compressed_cols = g_list_append(block_data->compressed_cols, container) ;

  return ;
}


