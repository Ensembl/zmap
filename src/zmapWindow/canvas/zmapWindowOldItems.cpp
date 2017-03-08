/*  File: zmapWindowOldItems.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
 *         Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
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

#include <ZMap/zmap.hpp>

#include <math.h>
#include <string.h>

#include <ZMap/zmapUtilsFoo.hpp>
#include <zmapWindowCanvasItem_I.hpp>
#include <zmapWindowContainerGroup_I.hpp>
#include <zmapWindowContainerUtils.hpp>
#include <zmapWindowContainerAlignment_I.hpp>
#include <zmapWindowContainerBlock_I.hpp>
#include <zmapWindowContainerContext_I.hpp>
#include <zmapWindowContainerFeatureSet_I.hpp>




enum
  {
    CONTAINER_PROP_0,                /* zero is invalid */
    CONTAINER_PROP_VISIBLE,
  };


typedef struct EachIntervalDataStructType
{
  ZMapWindowCanvasItem parent;
  ZMapFeature          feature;
  ZMapFeatureSubPart sub_feature;
  ZMapStyleColourType  style_colour_type;
  int                         colour_flags;
  GdkColor            *default_fill_colour;
  GdkColor            *border_colour;
  ZMapWindowCanvasItemClass klass;
} EachIntervalDataStruct, *EachIntervalData;

typedef struct GetColIDsStructType
{
  GList *ids_list ;
  gboolean unique ;
  ZMapStrand strand ;
  gboolean visible ;
} GetColIDsStruct, *GetColIDs ;

typedef struct FindColStructType
{
  GQuark col_id ;
  ZMapWindowContainerFeatureSet feature_set ;
  ZMapStrand strand ;
} FindColStruct, *FindCol ;

   
  






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

static void getColIdsCB(gpointer data, gpointer user_data) ;
static void findColCB(gpointer data, gpointer user_data) ;





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
                                           (GTypeFlags)0);
    }

  return group_type;
}


static void zmap_window_container_context_class_init(ZMapWindowContainerContextClass context_class)
{
  GObjectClass *gobject_class = NULL ;
  FooCanvasItemClass *item_class = NULL ;
  zmapWindowContainerGroupClass *group_class = NULL ;

  zMapReturnIfFail(context_class) ;

  gobject_class = (GObjectClass *)context_class;
  item_class    = (FooCanvasItemClass *)context_class;
  group_class = (zmapWindowContainerGroupClass *)context_class ;

  gobject_class->set_property = zmap_window_container_context_set_property;
  gobject_class->get_property = zmap_window_container_context_get_property;

  parent_class_G = (GObjectClass *)g_type_class_peek_parent(context_class);
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
  zMapReturnIfFail(ZMAP_IS_CONTAINER_GROUP(gobject));

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
  zMapReturnIfFail(gobject) ;

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
  zMapReturnIfFail(parent_item_class_G) ;

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
                                           (GTypeFlags)0);
    }

  return group_type;
}


static void zmap_window_container_alignment_class_init(ZMapWindowContainerAlignmentClass container_align_class)
{
  GObjectClass *gobject_class = NULL ;
  zmapWindowContainerGroupClass *group_class = NULL ;

  zMapReturnIfFail(container_align_class) ;

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
                                           (GTypeFlags)0);
    }

  return group_type;
}

/* Test whether there are compressed cols. */
gboolean zmapWindowContainerBlockIsCompressedColumn(ZMapWindowContainerBlock block_data)
{
  gboolean result = FALSE ;

  zMapReturnValIfFail(block_data, result) ;

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
  zMapReturnValIfFail(container_block && feature, container_block) ;

  zmapWindowContainerAttachFeatureAny((ZMapWindowContainerGroup)container_block,
                                      (ZMapFeatureAny)feature);

  return container_block;
}



static void zmap_window_container_block_class_init(ZMapWindowContainerBlockClass block_data_class)
{
  ZMapWindowContainerGroupClass group_class = NULL ;
  GtkObjectClass *gtkobject_class = NULL ;
  GObjectClass *gobject_class = NULL ;

  zMapReturnIfFail(block_data_class) ;

  gobject_class   = (GObjectClass *)block_data_class;
  gtkobject_class = (GtkObjectClass *)block_data_class;
  group_class     = (ZMapWindowContainerGroupClass)block_data_class;

  gobject_class->set_property = zmap_window_container_block_set_property;
  gobject_class->get_property = zmap_window_container_block_get_property;

  parent_class_G = (GObjectClass *)g_type_class_peek_parent(block_data_class);

  group_class->obj_size = sizeof(zmapWindowContainerBlockStruct) ;
  group_class->obj_total = 0 ;


  gtkobject_class->destroy = zmap_window_container_block_destroy;

  return ;
}

static void zmap_window_container_block_init(ZMapWindowContainerBlock block_data)
{
  zMapReturnIfFail(block_data) ;

  block_data->compressed_cols = NULL;

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
  GtkObjectClass *gtkobject_class = NULL ;

  zMapReturnIfFail(gtkobject && parent_class_G) ;

  gtkobject_class = GTK_OBJECT_CLASS(parent_class_G);

  block_data = ZMAP_CONTAINER_BLOCK(gtkobject);

  block_data->window = NULL;        /* not ours */


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

ZMapWindowContainerFeatureSet zmapWindowContainerBlockFindColumn(ZMapWindowContainerBlock block_container,
                                                                 const char *col_name, ZMapStrand strand)
{
  ZMapWindowContainerFeatureSet feature_set_container = NULL ;
  ZMapWindowContainerGroup container_group ;
  FooCanvasGroup *foo_group ;
  FindColStruct find_col_data = {0} ;
  char *canon_name ;

  zMapReturnValIfFail(ZMAP_IS_CONTAINER_BLOCK(block_container), NULL) ;
  zMapReturnValIfFail((col_name && *col_name), NULL) ;

  container_group = ZMAP_CONTAINER_GROUP(block_container) ;
  foo_group = &(container_group->__parent__) ;

  canon_name = g_strdup(col_name) ;
  canon_name = zMapFeatureCanonName(canon_name) ;
  find_col_data.col_id = g_quark_from_string(canon_name) ;
  find_col_data.strand = strand ;

  g_list_foreach(foo_group->item_list, findColCB, &find_col_data) ;

  feature_set_container = find_col_data.feature_set ;

  g_free(canon_name) ;

  return feature_set_container ;
}




GList *zmapWindowContainerBlockColumnList(ZMapWindowContainerBlock block_container,
                                          gboolean unique, ZMapStrand strand, gboolean visible)
{
  GList *column_ids = NULL ;
  GetColIDsStruct get_ids_data = {NULL} ;
  ZMapWindowContainerGroup container_group ;
  FooCanvasGroup *foo_group ;
  
  zMapReturnValIfFail(ZMAP_IS_CONTAINER_BLOCK(block_container), NULL) ;

  container_group = ZMAP_CONTAINER_GROUP(block_container) ;
  foo_group = &(container_group->__parent__) ;

  
  get_ids_data.unique = unique ;
  get_ids_data.strand = strand ;
  get_ids_data.visible = visible ;

  g_list_foreach(foo_group->item_list, getColIdsCB, &get_ids_data) ;

  column_ids = get_ids_data.ids_list ;

  return column_ids ;
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
  GList *glist = NULL;

  zMapReturnValIfFail(block_data, glist) ;

  glist = block_data->compressed_cols;
  block_data->compressed_cols = NULL;

  return glist;
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
  GList *glist = NULL;

  zMapReturnValIfFail(block_data, glist) ;

  glist = block_data->bumped_cols;
  block_data->bumped_cols = NULL;

  return glist;
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
  zMapReturnIfFail(block_data) ;

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
  zMapReturnIfFail(block_data) ;

  block_data->compressed_cols = g_list_append(block_data->compressed_cols, container) ;

  return ;
}





//
//                 Internal routines
//



// called for each column group including both forward and reverse strand groups
// and the strand separator group.
static void getColIdsCB(gpointer data, gpointer user_data)
{
  ZMapWindowContainerGroup container_group = (ZMapWindowContainerGroup)data ;
  ZMapWindowContainerFeatureSet feature_set = (ZMapWindowContainerFeatureSet)data ;
  GetColIDs get_ids_data = (GetColIDs)user_data ;
  static GQuark strand_id = 0 ;

  // We don't want to return the strand id, it's not a column in the usual sense.
  if (!strand_id)
    {
      char *canon_strand ;

      canon_strand = g_strdup(ZMAP_FIXED_STYLE_STRAND_SEPARATOR) ;

      canon_strand = zMapFeatureCanonName(canon_strand) ;

      strand_id = g_quark_from_string(canon_strand) ;

      g_free(canon_strand) ;
    }

  if (container_group->level == ZMAPCONTAINER_LEVEL_FEATURESET
      && feature_set->original_id != strand_id
      && feature_set->strand == get_ids_data->strand
      && container_group->flags.visible == get_ids_data->visible)
    {
      GQuark name_id ;

      if (get_ids_data->unique)
        name_id = feature_set->unique_id ;
      else
        name_id = feature_set->original_id ;

      // zMapDebugPrintf("col name: \"%s\"", g_quark_to_string(name_id)) ;

      get_ids_data->ids_list = g_list_append(get_ids_data->ids_list, GINT_TO_POINTER(name_id)) ;
    }
  
  return ;
}

// Called to check a column group to see if it's the requested one.   
static void findColCB(gpointer data, gpointer user_data)
{
  ZMapWindowContainerGroup container_group = (ZMapWindowContainerGroup)data ;
  ZMapWindowContainerFeatureSet feature_set = (ZMapWindowContainerFeatureSet)data ;
  FindCol find_col_data = (FindCol)user_data ;

  if (container_group->level == ZMAPCONTAINER_LEVEL_FEATURESET
      && feature_set->strand == find_col_data->strand)
    {
      if (feature_set->unique_id == find_col_data->col_id)
        find_col_data->feature_set = feature_set ;
    }
  
  return ;
}


