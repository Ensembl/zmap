/*  File: zmapWindowContainerBlock.c
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <ZMap/zmapSeqBitmap.h>
#include <zmapWindowContainerBlock_I.h>
#include <zmapWindowContainerFeatureSet_I.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindow_P.h>	/* ITEM_FEATURE_SET_DATA */

enum
  {
    ITEM_FEATURE_BLOCK_0,		/* zero == invalid prop value */
  };




/* All the basic object functions */
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

static GObjectClass *parent_class_G = NULL;

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

/* Test whether there are compressed cols. */
gboolean zmapWindowContainerBlockIsCompressedColumn(ZMapWindowContainerBlock block_data)
{
  gboolean result = FALSE ;

  if (block_data->compressed_cols)
    result = TRUE ;

  return result ;
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

/* Test whether there are bumped cols. */
gboolean zmapWindowContainerBlockIsBumpedColumn(ZMapWindowContainerBlock block_data)
{
  gboolean result = FALSE ;

  if (block_data->bumped_cols)
    result = TRUE ;

  return result ;
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




/* --- */


ZMapWindowContainerBlock zmapWindowContainerBlockDestroy(ZMapWindowContainerBlock container_block)
{
  gtk_object_destroy(GTK_OBJECT(container_block));

  container_block = NULL;

  return container_block;
}


/*
 *                           INTERNAL
 */


/*
 * OBJECT CODE
 */
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


