



/*  File: zmapWindowCanvasBasic.c
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
 *-------------------------------------------------------------------
 * ZMap is free software; you can refeaturesetstribute it and/or
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
 * implements callback functions for FeaturesetItem basic features (boxes)
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <string.h>

#include <ZMap/zmapFeature.h>
#include <ZMap/zmapUtilsFoo.h>
#include <zmapWindowCanvasFeatureset_I.h>

/* We want to remove these.... */
#include <zmapWindowContainerFeatureSet_I.h>
#include <zmapWindowContainerAlignment_I.h>
#include <zmapWindowContainerBlock_I.h>
#include <zmapWindowContainerContext_I.h>



/* The property param ids for the switch statements */
enum
  {
    ITEM_FEATURE_SET_0,		/* zero == invalid prop value */
    ITEM_FEATURE_SET_HIDDEN_BUMP_FEATURES,
    ITEM_FEATURE_SET_UNIQUE_ID,
    ITEM_FEATURE_SET_STYLE_TABLE,
    ITEM_FEATURE_SET_USER_HIDDEN_ITEMS,
    ITEM_FEATURE_SET_WIDTH,
    ITEM_FEATURE_SET_VISIBLE,
    ITEM_FEATURE_SET_BUMP_MODE,
    ITEM_FEATURE_SET_DEFAULT_BUMP_MODE,
    ITEM_FEATURE_SET_BUMP_UNMARKED,
    ITEM_FEATURE_SET_FRAME_MODE,
    ITEM_FEATURE_SET_SHOW_WHEN_EMPTY,
    ITEM_FEATURE_SET_DEFERRED,
    ITEM_FEATURE_SET_STRAND_SPECIFIC,
    ITEM_FEATURE_SET_SHOW_REVERSE_STRAND,
    ITEM_FEATURE_SET_BUMP_SPACING,
    ITEM_FEATURE_SET_JOIN_ALIGNS,

    /* The next values are for slightly different purpose. */
    ITEM_FEATURE__DIVIDE,
    ITEM_FEATURE__MIN_MAG,
    ITEM_FEATURE__MAX_MAG,
  };


enum
  {
    CONTAINER_PROP_0,		/* zero is invalid */
    CONTAINER_PROP_VISIBLE,
  };

typedef struct
{
  ZMapWindowCanvasItem parent;
  ZMapFeature          feature;
  ZMapFeatureSubPartSpan sub_feature;
  ZMapStyleColourType  style_colour_type;
  int 			colour_flags;
  GdkColor            *default_fill_colour;
  GdkColor            *border_colour;
  ZMapWindowCanvasItemClass klass;
}EachIntervalDataStruct, *EachIntervalData;


/* Surprisingly I thought this would be typedef somewhere in the GType header but not so... */
typedef GType (*MyGTypeFunc)(void) ;


typedef struct MyGObjectInfoStructName
{
  MyGTypeFunc get_type_func ;
  GType obj_type ;
  char *obj_name ;
  unsigned int obj_size ;
} MyGObjectInfoStruct, *MyGObjectInfo ;

static void window_canvas_invoke_set_colours(gpointer list_data, gpointer user_data);

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


static void zmap_window_container_group_init        (ZMapWindowContainerGroup container) ;
static void zmap_window_container_group_class_init  (ZMapWindowContainerGroupClass container_class) ;
static void zmap_window_container_group_set_property(GObject               *object,
						     guint                  param_id,
						     const GValue          *value,
						     GParamSpec            *pspec) ;
static void zmap_window_container_group_get_property(GObject               *object,
						     guint                  param_id,
						     GValue                *value,
						     GParamSpec            *pspec) ;
static void zmap_window_container_group_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags) ;
static void zmap_window_container_group_draw (FooCanvasItem *item, GdkDrawable *drawable,
					      GdkEventExpose *expose) ;
static void zmap_window_container_group_destroy     (GtkObject *gtkobject) ;


static void zmap_window_item_feature_set_class_init(ZMapWindowContainerFeatureSetClass container_set_class) ;
static void zmap_window_item_feature_set_init(ZMapWindowContainerFeatureSet container_set) ;
static void zmap_window_item_feature_set_set_property(GObject      *gobject,
						      guint         param_id,
						      const GValue *value,
						      GParamSpec   *pspec);
static void zmap_window_item_feature_set_get_property(GObject    *gobject,
						      guint       param_id,
						      GValue     *value,
						      GParamSpec *pspec);
static void zmap_window_item_feature_set_set_property(GObject      *gobject,
						      guint         param_id,
						      const GValue *value,
						      GParamSpec   *pspec);
static void zmap_window_item_feature_set_destroy     (GtkObject *gtkobject);


static FooCanvasItem *getNextFeatureItem(FooCanvasGroup *group,
					 FooCanvasItem *orig_item,
					 ZMapContainerItemDirection direction, gboolean wrap,
					 zmapWindowContainerItemTestCallback item_test_func_cb,
					 gpointer user_data) ;


static void zmap_window_canvas_item_class_init  (ZMapWindowCanvasItemClass class);
static void zmap_window_canvas_item_init        (ZMapWindowCanvasItem      group);
static ZMapFeatureTypeStyle zmap_window_canvas_item_get_style(ZMapWindowCanvasItem canvas_item) ;
static void zmap_window_canvas_item_destroy (GtkObject *gtkobject) ;


static MyGObjectInfo findObjectInfo(GType type) ;
static MyGObjectInfo initObjectDescriptions(void) ;


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
static void zmap_window_container_context_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags);

static void zmap_window_container_alignment_class_init  (ZMapWindowContainerAlignmentClass container_align_class);
static void zmap_window_container_alignment_init        (ZMapWindowContainerAlignment container_align);
static void zmap_window_container_alignment_set_property(GObject      *gobject, 
							 guint         param_id, 
							 const GValue *value, 
							 GParamSpec   *pspec);
static void zmap_window_container_alignment_get_property(GObject    *gobject, 
							 guint       param_id, 
							 GValue     *value, 
							 GParamSpec *pspec);


static double window_canvas_item_invoke_point (FooCanvasItem *item,
					       double x, double y,
					       int cx, int cy,
					       FooCanvasItem **actual_item) ;



static void removeList(gpointer data, gpointer user_data_unused) ;


static FooCanvasItemClass  *item_parent_class_G  = NULL;


static FooCanvasItemClass *group_parent_class_G;

static GObjectClass *parent_class_G = NULL;
static FooCanvasItemClass *parent_item_class_G = NULL;

static gboolean debug_point_method_G = FALSE;




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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  (GInstanceInitFunc) zmap_window_container_context_init
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
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









/* 
 *     FEATURESET ROUTINES
 */



GList *zmapWindowContainerFeatureSetGetFeatureSets(ZMapWindowContainerFeatureSet container_set)
{
  return container_set->featuresets;
}


/* replaced by utils/attach_feature_any
 * stats are not handled for the moment
 * Ed was looking at a base class to do that if needs be we can add something back
 * BUT if we remove it the we have to fix up stats ref'd from the factory
 */

/*!
 * \brief Attach a ZMapFeatureSet to the container.
 *
 * The container needs access to the feature set it represents quite often
 * so we store this here.
 *
 * \param container   The container.
 * \param feature-set The feature set the container needs.
 */

gboolean zmapWindowContainerFeatureSetAttachFeatureSet(ZMapWindowContainerFeatureSet container_set,
						       ZMapFeatureSet feature_set_to_attach)
{
  gboolean status = FALSE;

  if(feature_set_to_attach && !container_set->has_feature_set)
    {

      ZMapWindowContainerGroup container_group;

      container_group              = ZMAP_CONTAINER_GROUP(container_set);
      container_group->feature_any = (ZMapFeatureAny)feature_set_to_attach;

      container_set->has_feature_set = status = TRUE;

      //#ifdef STATS_GO_IN_PARENT_OBJECT
      ZMapWindowStats stats = NULL;

      if((stats = zmapWindowStatsCreate((ZMapFeatureAny)feature_set_to_attach)))
	{
	  //	  zmapWindowContainerSetData(container_set->column_container, ITEM_FEATURE_STATS, stats);
	  g_object_set_data(G_OBJECT(container_set),ITEM_FEATURE_STATS,stats);
	  container_set->has_stats = TRUE;
	}
      //#endif
    }
  else
    {
      /* We don't attach a feature set if there's already one
       * attached.  This works for the good as the merge will
       * create featuresets which get destroyed after drawing
       * in the case of a pre-exisiting featureset. We don't
       * want these attached, as the original will also get
       * destroyed and the one attached will point to a set
       * which has been freed! */
    }

  return status ;
}



/* Return the feature set item which has zoom and much else....
 * 
 * There is no checking here as it's a trial function...I need to add lots
 * if I keep this function.
 * 
 *  */
ZMapWindowFeaturesetItem ZMapWindowContainerGetFeatureSetItem(ZMapWindowContainerGroup container)
{
  ZMapWindowFeaturesetItem container_feature_list = NULL ;
  FooCanvasGroup *group = (FooCanvasGroup *)container ;
  GList *l ;

  for (l = group->item_list ; l ; l = l->next)
    {
      ZMapWindowFeaturesetItem temp ;

      temp = (ZMapWindowFeaturesetItem)(l->data) ;

      if (zMapWindowCanvasIsFeatureSet(temp))
	{
	  container_feature_list = temp ;

	  break ;
	}
    }

  return container_feature_list ;
}

ZMapFeatureSet zmapWindowContainerGetFeatureSet(ZMapWindowContainerGroup feature_group)
{
  ZMapFeatureSet feature_set = NULL;
  ZMapFeatureAny feature_any;

  if(zmapWindowContainerGetFeatureAny(feature_group, &feature_any) &&
     (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURESET))
    {
      feature_set = (ZMapFeatureSet)feature_any;
    }

  return feature_set;
}


/* show or hide all the masked features in this column */
void zMapWindowContainerFeatureSetShowHideMaskedFeatures(ZMapWindowContainerFeatureSet container, gboolean show, gboolean set_colour)
{
  ZMapWindowContainerFeatures container_features ;
  ZMapStyleBumpMode bump_mode;      /* = container->settings.bump_mode; */

  container->masked = !show;
  //printf("show_hide 1\n");
  bump_mode = zMapWindowContainerFeatureSetGetContainerBumpMode(container);

  //printf("show_hide 2\n");
  if(bump_mode > ZMAPBUMP_UNBUMP)
    {
      zmapWindowColumnBump(FOO_CANVAS_ITEM(container),ZMAPBUMP_UNBUMP);
    }

  //printf("show_hide 3\n");

  if ((container_features = zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)container)))
    {
      FooCanvasGroup *group ;
      GList *list;
      //	ZMapWindowCanvasItem item;
      //	ZMapFeature feature;
      //	ZMapFeatureTypeStyle style;

      group = FOO_CANVAS_GROUP(container_features) ;

      for(list = group->item_list;list;)
        {
	  //printf("show_hide 3.1\n");
	  //		item = ZMAP_CANVAS_ITEM(list->data);
	  //		feature = item->feature;
	  //		style = *feature->style;

	  if(ZMAP_IS_WINDOW_FEATURESET_ITEM(list->data))
	    {
	      zMapWindowCanvasFeaturesetShowHideMasked((FooCanvasItem *) list->data, show, set_colour);
	      list = list->next;
	    }
        }
    }
  //printf("show_hide 4\n");
  /* if we are adding/ removing features we may need to compress and/or rebump */
  if(bump_mode > ZMAPBUMP_UNBUMP)
    {
      zmapWindowColumnBump(FOO_CANVAS_ITEM(container),bump_mode);
    }
  //printf("show_hide 5\n");

}



/*!
 * \brief removes everything from a column
 *
 * \param The container.
 *
 * \return nothing.
 */

void zmapWindowContainerFeatureSetRemoveAllItems(ZMapWindowContainerFeatureSet container_set)
{
  ZMapWindowContainerFeatures container_features ;

  if ((container_features = zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)container_set)))
    {
      FooCanvasGroup *group ;

      group = FOO_CANVAS_GROUP(container_features) ;

      zmapWindowContainerUtilsRemoveAllItems(group) ;
    }

  return ;
}




/*!
 * \brief Once a new ZMapWindowContainerFeatureSet object has been created,
 *        various parameters need to be set.  This sets all the params.
 *
 * \param container     The container to set everything on.
 * \param window        The container needs to know its ZMapWindow.
 * \param align-id      The container needs access to this quark
 * \param block-id      The container needs access to this quark
 * \param set-unique-id The container needs access to this quark
 * \param set-orig-id   The container needs access to this quark, actually this is unused!
 * \param styles        A list of styles the container needs to be able to draw everything.
 * \param strand        The strand of the container.
 * \param frame         The frame of the container.
 *
 * \return ZMapWindowContainerFeatureSet that was edited.
 */

/* this is only called on column creation */
ZMapWindowContainerFeatureSet zmapWindowContainerFeatureSetAugment(ZMapWindowContainerFeatureSet container_set,
								   ZMapWindow window,
								   GQuark     align_id,
								   GQuark     block_id,
								   GQuark     feature_set_unique_id,
								   GQuark     feature_set_original_id,
								   ZMapFeatureTypeStyle style,
								   ZMapStrand strand,
								   ZMapFrame  frame)
{
  g_return_val_if_fail(feature_set_unique_id != 0, container_set);

  if(ZMAP_IS_CONTAINER_FEATURESET(container_set))
    {
      gboolean visible;

      container_set->window    = window;
      container_set->strand    = strand;
      container_set->frame     = frame;
      container_set->align_id  = align_id;
      container_set->block_id  = block_id;
      container_set->unique_id = feature_set_unique_id;
      container_set->original_id = feature_set_original_id;

      container_set->style = style;

      visible = zmapWindowGetColumnVisibility(window,(FooCanvasGroup *) container_set);

      zmapWindowContainerSetVisibility((FooCanvasGroup *)container_set, visible);
    }

  return container_set;
}





/*!
 * \brief Get the GType for the ZMapWindowContainerFeatureSet GObjects
 *
 * \return GType corresponding to the GObject as registered by glib.
 */


GType zmapWindowContainerFeatureSetGetType(void)
{
  static GType group_type = 0;

  if (!group_type)
    {
      static const GTypeInfo group_info =
	{
	  sizeof (zmapWindowContainerFeatureSetClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_window_item_feature_set_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (zmapWindowContainerFeatureSet),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) zmap_window_item_feature_set_init
	};

      group_type = g_type_register_static (ZMAP_TYPE_CONTAINER_GROUP,
					   ZMAP_WINDOW_CONTAINER_FEATURESET_NAME,
					   &group_info,
					   0);
    }

  return group_type;
}



static void zmap_window_item_feature_set_class_init(ZMapWindowContainerFeatureSetClass container_set_class)
{
  GObjectClass *gobject_class;
  GtkObjectClass *gtkobject_class;
  zmapWindowContainerGroupClass *group_class ;


  gobject_class = (GObjectClass *)container_set_class;
  gtkobject_class = (GtkObjectClass *)container_set_class;
  group_class = (zmapWindowContainerGroupClass *)container_set_class ;


  gobject_class->set_property = zmap_window_item_feature_set_set_property;
  gobject_class->get_property = zmap_window_item_feature_set_get_property;

  parent_class_G = g_type_class_peek_parent(container_set_class);

  group_class->obj_size = sizeof(zmapWindowContainerFeatureSetStruct) ;
  group_class->obj_total = 0 ;

  /* hidden bump features */
  g_object_class_install_property(gobject_class,
				  ITEM_FEATURE_SET_HIDDEN_BUMP_FEATURES,
				  g_param_spec_boolean("hidden-bump-features",
						       "Hidden bump features",
						       "Some features are hidden because of current bump mode.",
						       FALSE, ZMAP_PARAM_STATIC_RW)) ;
  /* unique id */
  g_object_class_install_property(gobject_class,
				  ITEM_FEATURE_SET_UNIQUE_ID,
				  g_param_spec_uint("unique-id",
						    "Column unique id",
						    "The unique name/id for the column.",
						    0, G_MAXUINT32, 0, ZMAP_PARAM_STATIC_RW)) ;


  g_object_class_install_property(gobject_class, ITEM_FEATURE_SET_USER_HIDDEN_ITEMS,
				  g_param_spec_pointer("user-hidden-items", "User hidden items",
						       "Feature items explicitly hidden by user.",
						       ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class, ITEM_FEATURE_SET_STYLE_TABLE,
				  g_param_spec_pointer("style-table", "ZMapStyle table",
						       "GHashTable of ZMap styles for column.",
						       ZMAP_PARAM_STATIC_RW));


  /* display mode */
  g_object_class_install_property(gobject_class,
				  ITEM_FEATURE_SET_VISIBLE,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_DISPLAY_MODE,
						    ZMAPSTYLE_PROPERTY_DISPLAY_MODE,
						    "[ hide | show_hide | show ]",
						    ZMAPSTYLE_COLDISPLAY_INVALID,
						    ZMAPSTYLE_COLDISPLAY_SHOW,
						    ZMAPSTYLE_COLDISPLAY_INVALID,
						    ZMAP_PARAM_STATIC_RW));

  /* bump mode */
  g_object_class_install_property(gobject_class,
				  ITEM_FEATURE_SET_BUMP_MODE,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_BUMP_MODE,
						    ZMAPSTYLE_PROPERTY_BUMP_MODE,
						    "The Bump Mode",
						    ZMAPBUMP_INVALID,
						    ZMAPBUMP_END,
						    ZMAPBUMP_INVALID,
						    ZMAP_PARAM_STATIC_RO));

  gtkobject_class->destroy  = zmap_window_item_feature_set_destroy;

  return ;
}


static void zmap_window_item_feature_set_init(ZMapWindowContainerFeatureSet container_set)
{

  //  container_set->style_table       = zmapWindowStyleTableCreate();
  container_set->user_hidden_stack = g_queue_new();

  return ;
}


static void zmap_window_item_feature_set_get_property(GObject    *gobject,
						      guint       param_id,
						      GValue     *value,
						      GParamSpec *pspec)
{
  ZMapWindowContainerFeatureSet container_set;

  container_set = ZMAP_CONTAINER_FEATURESET(gobject);

  switch(param_id)
    {
    case ITEM_FEATURE_SET_HIDDEN_BUMP_FEATURES:
      g_value_set_boolean(value, container_set->hidden_bump_features) ;
      break ;
    case ITEM_FEATURE_SET_UNIQUE_ID:
      g_value_set_uint(value, container_set->unique_id) ;
      break ;
    case ITEM_FEATURE_SET_USER_HIDDEN_ITEMS:
      g_value_set_pointer(value, container_set->user_hidden_stack) ;
      break ;
    case ITEM_FEATURE_SET_BUMP_MODE:
      g_value_set_uint(value, container_set->bump_mode) ;
      break;
    case ITEM_FEATURE_SET_VISIBLE:
      g_value_set_uint(value, container_set->display_state) ;
      break ;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_window_item_feature_set_set_property(GObject      *gobject,
						      guint         param_id,
						      const GValue *value,
						      GParamSpec   *pspec)
{
  ZMapWindowContainerFeatureSet container_feature_set ;

  container_feature_set = ZMAP_CONTAINER_FEATURESET(gobject);

  switch(param_id)
    {
      //    case ITEM_FEATURE_SET_STYLE_TABLE:
      //      container_feature_set->style_table = g_value_get_pointer(value) ;
      //      break ;
    case ITEM_FEATURE_SET_USER_HIDDEN_ITEMS:
      container_feature_set->user_hidden_stack = g_value_get_pointer(value) ;
      break ;
    case ITEM_FEATURE_SET_VISIBLE:
      container_feature_set->display_state = g_value_get_uint(value) ;
      break ;
    case ITEM_FEATURE_SET_HIDDEN_BUMP_FEATURES:
      container_feature_set->hidden_bump_features = g_value_get_boolean(value) ;
      break ;
    case ITEM_FEATURE_SET_UNIQUE_ID:
      container_feature_set->unique_id = g_value_get_uint(value) ;
      break ;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}



/*!
 * \brief Return the feature set the container represents.
 *
 * \param container.
 *
 * \return The feature set.  Can be NULL.
 */

ZMapFeatureSet zmapWindowContainerFeatureSetRecoverFeatureSet(ZMapWindowContainerFeatureSet container_set)
{
  ZMapFeatureSet feature_set = NULL;

  if(container_set->has_feature_set)
    {
      feature_set = (ZMapFeatureSet)(ZMAP_CONTAINER_GROUP(container_set)->feature_any);

      if(!feature_set)
	{
	  g_warning("%s", "No Feature Set!");
	  container_set->has_feature_set = FALSE;
	}
    }

  return feature_set;
}



/*!
 * \brief Is the strand shown for this column?
 *
 * \param The container to interrogate.
 *
 * \return whether the strand is shown.
 */

gboolean zmapWindowContainerFeatureSetIsStrandShown(ZMapWindowContainerFeatureSet container_set)
{
  gboolean strand_show = FALSE ;

  if (container_set->strand == ZMAPSTRAND_FORWARD
      || (zMapStyleIsStrandSpecific(container_set->style) && zMapStyleIsShowReverseStrand(container_set->style)))
    strand_show = TRUE ;

  return strand_show ;
}



static void zmap_window_item_feature_set_destroy(GtkObject *gtkobject)
{
  ZMapWindowContainerFeatureSet container_set;
  GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS(parent_class_G);

  container_set = ZMAP_CONTAINER_FEATURESET(gtkobject);


  if (container_set->user_hidden_stack)
    {
      if(!g_queue_is_empty(container_set->user_hidden_stack))
	g_queue_foreach(container_set->user_hidden_stack, removeList, NULL) ;

      g_queue_free(container_set->user_hidden_stack);

      container_set->user_hidden_stack = NULL;
    }

#if MH17_NO_IDEA_WHY
  {
    char *col_name ;

    col_name = (char *) g_quark_to_string(zmapWindowContainerFeatureSetColumnDisplayName(container_set)) ;
    if (g_ascii_strcasecmp("3 frame translation", col_name) !=0)
      {
      }
  }
#else
  {
    {
#endif
	
      if (gtkobject_class->destroy)
	(gtkobject_class->destroy)(gtkobject);
    }
  }
	
  return ;
}



/*!
 * \brief Access the bump spacing of a column.
 *
 * \param The container to interrogate.
 *
 * \return The bump spacing of the container.
 */

double zmapWindowContainerFeatureGetBumpSpacing(ZMapWindowContainerFeatureSet container_set)
{
  double spacing;

  spacing = zMapStyleGetBumpSpace(container_set->style);
  return spacing;
}


/*!
 * \brief Access the bump mode of a column.
 *
 * \param The container to interrogate.
 *
 * \return The bump mode of the container.
 */

ZMapStyleBumpMode zmapWindowContainerFeatureSetGetBumpMode(ZMapWindowContainerFeatureSet container_set)
{
  ZMapStyleBumpMode mode = ZMAPBUMP_UNBUMP;
  mode = container_set->bump_mode;

  if(mode == ZMAPBUMP_INVALID)
    mode = zMapStyleGetInitialBumpMode(container_set->style);
  if(mode == ZMAPBUMP_INVALID)
    mode = ZMAPBUMP_UNBUMP;

  return mode;
}


GQuark zmapWindowContainerFeatureSetGetColumnId(ZMapWindowContainerFeatureSet container_set)
{
  if(!ZMAP_IS_CONTAINER_FEATURESET(container_set))
    return 0;
  return container_set->unique_id;
}



ZMapWindow zMapWindowContainerFeatureSetGetWindow(ZMapWindowContainerFeatureSet container_set)
{
  return container_set->window;
}





ZMapStyleBumpMode zMapWindowContainerFeatureSetGetContainerBumpMode(ZMapWindowContainerFeatureSet container_set)
{
  ZMapStyleBumpMode mode = container_set->bump_mode;

  return(mode);
}



gboolean zMapWindowContainerFeatureSetSetBumpMode(ZMapWindowContainerFeatureSet container_set, ZMapStyleBumpMode bump_mode)
{
  gboolean result = FALSE;
  if(bump_mode >=ZMAPBUMP_INVALID && bump_mode <= ZMAPBUMP_END)
    {
      container_set->bump_mode = bump_mode;
      result = TRUE;
    }
  return(result);
}


ZMapFeatureTypeStyle zMapWindowContainerFeatureSetGetStyle(ZMapWindowContainerFeatureSet container)
{
  return container->style;
}





/*!
 *  Functions to set/get display state of column, i.e. show, show_hide or hide. Complicated
 * by having an overall state for the column and potentially sub-states for sub-features.
 *
 * \param The container to interrogate.
 *
 * \return The display state of the container.
 */
ZMapStyleColumnDisplayState zmapWindowContainerFeatureSetGetDisplay(ZMapWindowContainerFeatureSet container_set)
{
  ZMapStyleColumnDisplayState display;

  display = container_set->display_state ;
  if(display == ZMAPSTYLE_COLDISPLAY_INVALID)
    display = zMapStyleGetDisplay(container_set->style);   /* get initial state from set style */
  return display ;
}


/*! Sets the given state both on the column _and_ in all the styles for the features
 * within that column.
 *
 * This function needs to update all the styles in the local cache of styles that the column holds.
 * However this does not actually do the foo_canvas_show/hide of the column, as that is
 * application logic that is held elsewhere.
 *
 * \param container  The container to set.
 * \param state      The new display state.
 *
 * \return void
 */
void zmapWindowContainerFeatureSetSetDisplay(ZMapWindowContainerFeatureSet container_set,
					     ZMapStyleColumnDisplayState state,
                                             ZMapWindow window)
{
  container_set->display_state = state;

  return ;
}



/*!
 * \brief Access the strand of a column.
 *
 * \param The container to interrogate.
 *
 * \return The strand of the container.
 */

ZMapStrand zmapWindowContainerFeatureSetGetStrand(ZMapWindowContainerFeatureSet container_set)
{
  ZMapStrand strand = ZMAPSTRAND_NONE;

  if(ZMAP_IS_CONTAINER_FEATURESET(container_set))
    strand = container_set->strand;

  return strand;
}

/*!
 * \brief Access the frame of a column.
 *
 * \param The container to interrogate.
 *
 * \return The frame of the container.
 */

ZMapFrame  zmapWindowContainerFeatureSetGetFrame (ZMapWindowContainerFeatureSet container_set)
{
  ZMapFrame frame = ZMAPFRAME_NONE;

  g_return_val_if_fail(ZMAP_IS_CONTAINER_FEATURESET(container_set), frame);

  frame = container_set->frame;

  return frame;
}



/*!
 * \brief Access whether the column has min, max magnification values and if so, their values.
 *
 * Non-obvious interface: returns FALSE _only_ if neither min or max mag are not set,
 * returns TRUE otherwise.
 *
 * \param container  The container to interrogate.
 * \param min-out    The address to somewhere to store the minimum.
 * \param max-out    The address to somewhere to store the maximum.
 *
 * \return boolean, TRUE = either min or max are set, FALSE = neither are set.
 */

gboolean zmapWindowContainerFeatureSetGetMagValues(ZMapWindowContainerFeatureSet container_set,
						   double *min_mag_out, double *max_mag_out)
{
  gboolean mag_sens = FALSE ;
  double min_mag ;
  double max_mag ;

  min_mag = zMapStyleGetMinMag(container_set->style);
  max_mag = zMapStyleGetMaxMag(container_set->style);

  if (min_mag != 0.0 || max_mag != 0.0)
    mag_sens = TRUE ;

  if (min_mag && min_mag_out)
    *min_mag_out = min_mag ;

  if (max_mag && max_mag_out)
    *max_mag_out = max_mag ;

  return mag_sens ;
}


/*!
 * \brief  The display name for the column.
 *
 * This is  indepenadant of the featureset(s) attached/ in the column
 * but defaults to the name of the featureset if there is only one
 *
 * \param container  The container to interrogate.
 *
 * \return The quark that represents the current display name.
 */

GQuark zmapWindowContainerFeatureSetColumnDisplayName(ZMapWindowContainerFeatureSet container_set)
{
  GQuark display_id = 0;

  display_id = container_set->original_id;
  if(!display_id)
    display_id = container_set->unique_id;

  return display_id;
}





/*!
 * \brief Access whether the column is frame specific.
 *
 * \param The container to interrogate.
 *
 * \return whether the column is frame specific.
 */

gboolean zmapWindowContainerFeatureSetIsFrameSpecific(ZMapWindowContainerFeatureSet container_set,
						      ZMapStyle3FrameMode         *frame_mode_out)
{
  ZMapStyle3FrameMode frame_mode = ZMAPSTYLE_3_FRAME_INVALID;
  gboolean frame_specific = FALSE;

  frame_mode = zMapStyleGetFrameMode(container_set->style);
  frame_specific = zMapStyleIsFrameSpecific(container_set->style);

  if(frame_mode_out)
    *frame_mode_out = frame_mode;
  return frame_specific;
}


/*!
 * \brief Access the _default_ bump mode of a column.
 *
 * \param The container to interrogate.
 *
 * \return The default bump mode of the container.
 */

ZMapStyleBumpMode zmapWindowContainerFeatureSetGetDefaultBumpMode(ZMapWindowContainerFeatureSet container_set)
{
  ZMapStyleBumpMode mode = ZMAPBUMP_UNBUMP;

  mode = zMapStyleGetDefaultBumpMode(container_set->style);

  return mode;
}


/*!
 * \brief Access to the stack of hidden items
 *
 * \param container to interrogate and pop a list from.
 *
 * \return The most recent list.
 */

GList *zmapWindowContainerFeatureSetPopHiddenStack(ZMapWindowContainerFeatureSet container_set)
{
  GList *hidden_items = NULL;

  hidden_items = g_queue_pop_head(container_set->user_hidden_stack);

  return hidden_items;
}

/*!
 * \brief Access to the stack of hidden items.  This adds a list.
 *
 * \param The container to add a hidden item list to.
 *
 * \return nothing.
 */

void zmapWindowContainerFeatureSetPushHiddenStack(ZMapWindowContainerFeatureSet container_set,
						  GList *hidden_items_list)
{
  g_queue_push_head(container_set->user_hidden_stack, hidden_items_list) ;

  return ;
}





/* 
 *    CONTAINER ROUTINES
 */



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
    }

  zMapWindowContainerGroupSortByLayer((FooCanvasGroup *) item->parent);

  return container;
}




/* Features */
gboolean zmapWindowContainerAttachFeatureAny(ZMapWindowContainerGroup container, ZMapFeatureAny feature_any)
{
  gboolean status = FALSE;

  if(feature_any && ZMAP_IS_CONTAINER_GROUP(container))
    {
      switch(feature_any->struct_type)
	{
	case ZMAPFEATURE_STRUCT_CONTEXT:
	  status = ZMAP_IS_CONTAINER_CONTEXT(container);
	  break;
	case ZMAPFEATURE_STRUCT_ALIGN:
	  status = ZMAP_IS_CONTAINER_ALIGNMENT(container);
	  break;
	case ZMAPFEATURE_STRUCT_BLOCK:
	  status = ZMAP_IS_CONTAINER_BLOCK(container);
	  break;
	case ZMAPFEATURE_STRUCT_FEATURESET:
#if 1 /* MH17_NO_RECOVER */
	  /* I'd like to remove this but that will mean having to remove stats code
	   * in 3 files iffed by RDS_REMOVED_STATS
	   *
	   * this is the only place this function gets called
	   *
	   * some months later ... when trying tp ding the block feature from a CanvasBlock ...
	   * all the canvas items/groups have a feature_any (except strand) and not attaching one might cause errors
	   * but as several featuresets can go in a column (featureset in canvas speak) then we don't get a 1-1 mapping
	   */
	  if((status = ZMAP_IS_CONTAINER_FEATURESET(container)))
	    {
	      status = zmapWindowContainerFeatureSetAttachFeatureSet((ZMapWindowContainerFeatureSet)container,
								     (ZMapFeatureSet)feature_any);
	    }
#else
	  status = ZMAP_IS_CONTAINER_FEATURESET(container);
#endif
	  break;
	case ZMAPFEATURE_STRUCT_FEATURE:
	case ZMAPFEATURE_STRUCT_INVALID:
	default:
	  status = FALSE;
	  break;
	}

      if(status)
	{
	  container->feature_any = feature_any;
#ifdef RDS_DONT_INCLUDE
	  g_object_set_data(G_OBJECT(container), ITEM_FEATURE_DATA, feature_any);
#endif
	}
    }

  return status;
}





/* look up a group in the the item_list
 * rather tediously the canvas is structured differently than the feature context and FToIHash
 * - it has a strand layer under block
 * - 3-frame columns all have the same id, whereas in the FToI hash the names are mangled
 */
static ZMapWindowContainerGroup getChildById(ZMapWindowContainerGroup group,GQuark id, ZMapStrand strand, ZMapFrame frame)
{
  ZMapWindowContainerGroup g;
  GList *l;
  FooCanvasGroup *children = (FooCanvasGroup *) zmapWindowContainerGetFeatures(group) ;
  /* this gets the group one of the four item positions */

  for(l = children->item_list;l;l = l->next)
    {
      if(!ZMAP_IS_CONTAINER_GROUP(l->data))	/* now we can have background CanvasFeaturesets in top level groups, just one item list */
	continue;

      g = (ZMapWindowContainerGroup) l->data;

      if(g->level == ZMAPCONTAINER_LEVEL_FEATURESET)
	{
	  ZMapWindowContainerFeatureSet set = ZMAP_CONTAINER_FEATURESET(g);;

	  if(set->unique_id == id && set->frame == frame && set->strand == strand)
	    return g;
	}
      else
	{
	  zMapAssert(g->feature_any);
	  if(g->feature_any->unique_id == id)
	    return g;
	}
    }
  return NULL;
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



ZMapWindowContainerGroup zmapWindowContainerCanvasItemGetContainer(FooCanvasItem *item)
{
  ZMapWindowContainerGroup container_group = NULL;
  FooCanvasItem *container_item = NULL;
  FooCanvasItem *parent;

  g_return_val_if_fail(item != NULL, NULL);
  g_return_val_if_fail(FOO_IS_CANVAS_ITEM(item), NULL);

  if (ZMAP_IS_CONTAINER_GROUP(item))
    {
      container_item = item;
    }
  else if((parent = item->parent))
    {
      do
	{
	  if(ZMAP_IS_CONTAINER_GROUP(parent))
	    container_item = parent;
	}
      while((container_item == NULL) && parent && (parent = parent->parent));
    }

  if(ZMAP_IS_CONTAINER_GROUP(container_item))
    {
      container_group = ZMAP_CONTAINER_GROUP(container_item);
    }
  else
    {
      zMapLogWarning("Failed to find container in path root -> ... -> item (%p).", item);
    }

  return container_group;
}


/* for composite items (eg ZMapWindowGraphDensity)
 * set the pointed at feature to be the one nearest to cursor
 * We need this to stabilise the feature while processing a mouse click (item/feature select)
 * point used to set this but the cursor is still moving while we trundle thro the code
 * causing the feature to change with unpredictable results
 * write up in density.html when i get round to correcting it.
 */
gboolean zMapWindowCanvasItemSetFeature(ZMapWindowCanvasItem item, double x, double y)
{
  ZMapWindowCanvasItemClass class = ZMAP_CANVAS_ITEM_GET_CLASS(item);
  gboolean ret = FALSE;

  if(class->set_feature)
    ret = class->set_feature((FooCanvasItem *) item,x,y);

  return ret;
}



void zMapWindowCanvasItemSetConnected(ZMapWindowCanvasItem  item, gboolean val)
{
  item->connected = val;
}

gboolean zMapWindowCanvasItemIsConnected(ZMapWindowCanvasItem item)
{
  return item->connected;
}





FooCanvasItem *zMapFindCanvasColumn(ZMapWindowContainerGroup group,
				    GQuark align, GQuark block, GQuark set, ZMapStrand strand, ZMapFrame frame)
{
  if(group)
    group = getChildById(group,align, ZMAPSTRAND_NONE, ZMAPFRAME_NONE);
  if(group)
    group = getChildById(group,block,ZMAPSTRAND_NONE, ZMAPFRAME_NONE);
  if(group)
    group = getChildById(group,set, strand, frame);

  return FOO_CANVAS_ITEM(group);
}









/* this used to be the features list as distinct from background item or underlay/overlay lists
 * so to preserve the meaning we return TRUE if there are real features
 * ie we ignore background
 * there is a loop here but we expect only on background feature
 * and there for we will not scan loads
 */
gboolean zmapWindowContainerHasFeatures(ZMapWindowContainerGroup container)
{
  ZMapWindowContainerFeatures features;
  GList *l;
  gint layer;

  if((features = zmapWindowContainerGetFeatures(container)))
    {
      for (l = ((FooCanvasGroup *)features)->item_list ; l ; l = l->next)
	{
	  if(FOO_IS_CANVAS_GROUP(l->data))
	    continue;
	  if(!ZMAP_IS_WINDOW_FEATURESET_ITEM(l->data))	/* a real foo canvas item? should not happen */
	    return(TRUE);

	  layer = zMapWindowCanvasFeaturesetGetLayer((ZMapWindowFeaturesetItem) l->data);
	  if(!(layer & ZMAP_CANVAS_LAYER_DECORATION))
	    return TRUE;
	}
    }

  return FALSE;
}



/*!
 * \brief Access the show when empty property of a column.
 *
 * \param The container to interrogate.
 *
 * \return The value for the container.
 */

gboolean zmapWindowContainerFeatureSetShowWhenEmpty(ZMapWindowContainerFeatureSet container_set)
{
  gboolean show = FALSE;

  show = zMapStyleGetShowWhenEmpty(container_set->style);
  return show;
}


/* Wrapper around zMapWindowCanvasItemGetInterval to take a generic item
 * and call the former if it's a canvas item. */
FooCanvasItem *zMapWindowItemGetInterval(FooCanvasItem *item,
                                         double x, double y,
                                         ZMapFeatureSubPartSpan *sub_feature_out)
{
  FooCanvasItem *result = NULL;
  
  if (ZMAP_IS_CANVAS_ITEM(item))
    {
      ZMapWindowCanvasItem canvas_item = ZMAP_CANVAS_ITEM(item);
      result = zMapWindowCanvasItemGetInterval(canvas_item, x, y, sub_feature_out);
    }
    
  return result;
}






void zmapWindowContainerUtilsRemoveAllItems(FooCanvasGroup *group)
{
  GList *list,*l;

  if((list = g_list_first(group->item_list)))
    {
      do
	{
	  GtkObject *gtk_item_object;

	  gtk_item_object = GTK_OBJECT(list->data);

	  l = list;
	  list = list->next;
	  //        g_free(l);      /* mh17: oddly this was not done */

	  gtk_object_destroy(gtk_item_object);
	}
      while((list));
    }
  group->item_list = NULL; /* nor this */
  group->item_list_end = NULL;
  return ;
}



/* If item is the parent item then the whole feature is coloured, otherwise just the sub-item
 * is coloured... */
/* NOTE
 * this is a legacy interface, sub-feature is used for sequence features (recent mod)
 * now we don't have parent items, so we always do the whole
 * this relates to split alignment features and whole transcripts
 * which is a user requirement possibly implemented by fluke due the structure of the data given to ZMap
 */
void zMapWindowCanvasItemSetIntervalColours(FooCanvasItem *item, ZMapFeature feature, ZMapFeatureSubPartSpan sub_feature,
					    ZMapStyleColourType colour_type,
					    int colour_flags,
					    GdkColor *default_fill_colour,
					    GdkColor *border_colour)
{
  ZMapWindowCanvasItem canvas_item ;
  GList *item_list, dummy_item = {NULL} ;
  EachIntervalDataStruct interval_data = {NULL};

  zMapLogReturnIfFail(FOO_IS_CANVAS_ITEM(item)) ;

  canvas_item = zMapWindowCanvasItemIntervalGetObject(item) ;
  dummy_item.data = item ;
  item_list = &dummy_item ;

  interval_data.parent = canvas_item ;
  interval_data.feature = feature;
  interval_data.sub_feature = sub_feature;
  interval_data.style_colour_type = colour_type ;
  interval_data.colour_flags = colour_flags;
  interval_data.default_fill_colour = default_fill_colour ;
  interval_data.border_colour = border_colour ;
  interval_data.klass = ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item) ;

  g_list_foreach(item_list, window_canvas_invoke_set_colours, &interval_data) ;

  return ;
}




FooCanvasItem *zMapWindowCanvasItemGetInterval(ZMapWindowCanvasItem canvas_item,
					       double x, double y,
					       ZMapFeatureSubPartSpan *sub_feature_out)
{
  FooCanvasItem *matching_interval = NULL;
  FooCanvasItem *item, *check;
  FooCanvas *canvas;
  double ix1, ix2, iy1, iy2;
  double wx1, wx2, wy1, wy2;
  double gx, gy, dist;
  int cx, cy;
  int small_enough;

  zMapLogReturnValIfFail(ZMAP_IS_CANVAS_ITEM(canvas_item), matching_interval);

  /* The background can be clipped by the long items code, so we
   * need to use the groups position as the background extends
   * this far really! */

  item   = (FooCanvasItem *)canvas_item;
  canvas = item->canvas;
  gx = x;
  gy = y;

  foo_canvas_item_w2i(item, &gx, &gy);

  foo_canvas_w2c(canvas, x, y, &cx, &cy);

  if(debug_point_method_G)
    printf("GetInterval like ->point(canvas_item=%p, gx=%f, gy=%f, cx=%d, cy=%d)\n", item, gx, gy, cx, cy);

  foo_canvas_item_get_bounds(item, &ix1, &iy1, &ix2, &iy2);
  wx1 = ix1;
  wy1 = iy1;
  wx2 = ix2;
  wy2 = iy2;
  foo_canvas_item_i2w(item, &wx1, &wy1);
  foo_canvas_item_i2w(item, &wx2, &wy2);

  if(debug_point_method_G)
    printf("ItemCoords(%f,%f,%f,%f), WorldCoords(%f,%f,%f,%f)\n",
	   ix1, iy1, ix2, iy2,
	   wx1, wy1, wx2, wy2);

  /* mh17 NOTE this invoke function work with simple foo canvas items and groups */
  dist = window_canvas_item_invoke_point(item, gx, gy, cx, cy, &check);

  small_enough = (int)(dist * canvas->pixels_per_unit_x + 0.5);

  if(small_enough <= canvas->close_enough)
    {
      /* we'll continue! */

      matching_interval = item;
    }

  if(matching_interval == NULL)
    g_warning("No matching interval!");
  else if(sub_feature_out)
    {
      if(ZMAP_IS_WINDOW_FEATURESET_ITEM(matching_interval) && zMapWindowCanvasItemGetFeature(item))
        {
          /* returns a static data structure */
          *sub_feature_out =
            zMapWindowCanvasFeaturesetGetSubPartSpan(matching_interval, zMapWindowCanvasItemGetFeature(item) ,x,y);
  	}
    }
  
  return matching_interval;
}



/*
 * This routine invokes the point method of the item.
 * The arguments x, y should be in the parent item local coordinates.
 */

static double window_canvas_item_invoke_point (FooCanvasItem *item,
					       double x, double y,
					       int cx, int cy,
					       FooCanvasItem **actual_item)
{
  /* Calculate x & y in item local coordinates */

  if (FOO_CANVAS_ITEM_GET_CLASS (item)->point)
    return FOO_CANVAS_ITEM_GET_CLASS (item)->point (item, x, y, cx, cy, actual_item);

  return 1e18;
}


gboolean zmapWindowContainerGetFeatureAny(ZMapWindowContainerGroup container, ZMapFeatureAny *feature_any_out)
{
  gboolean status = FALSE;

  if(ZMAP_IS_CONTAINER_GROUP(container) && feature_any_out)
    {
      if(ZMAP_IS_CONTAINER_FEATURESET(container))
	status = TRUE;
      else if(ZMAP_IS_CONTAINER_BLOCK(container))
	status = TRUE;
      else if(ZMAP_IS_CONTAINER_ALIGNMENT(container))
	status = TRUE;
      else if(ZMAP_IS_CONTAINER_CONTEXT(container))
	status = TRUE;
      else
	{
	  status = FALSE;
	  zMapLogWarning("%s", "request for feature from unexpected container");
	}

      if(status)
	{
	  *feature_any_out = container->feature_any;
	}
    }

  return status;
}



gboolean zMapWindowCanvasItemSetStyle(ZMapWindowCanvasItem item, ZMapFeatureTypeStyle style)
{
  ZMapWindowCanvasItemClass class = ZMAP_CANVAS_ITEM_GET_CLASS(item);
  gboolean ret = FALSE;

  if(class->set_style)
    ret = class->set_style((FooCanvasItem *) item, style);

  return ret;
}




ZMapContainerLevelType zmapWindowContainerUtilsGetLevel(FooCanvasItem *item)
{
  ZMapWindowContainerGroup container_group = NULL;
  ZMapContainerLevelType level = ZMAPCONTAINER_LEVEL_INVALID;

  if(ZMAP_IS_CONTAINER_GROUP(item) && (container_group = ZMAP_CONTAINER_GROUP(item)))
    level = container_group->level;

  return level;
}


ZMapWindowContainerGroup zmapWindowContainerUtilsItemGetParentLevel(FooCanvasItem *item, ZMapContainerLevelType level)
{
  ZMapWindowContainerGroup container_group = NULL;

  if((container_group = zmapWindowContainerCanvasItemGetContainer(item)))
    {
      container_group = zmapWindowContainerUtilsGetParentLevel(container_group, level);
    }

  return container_group;
}

ZMapWindowContainerGroup zmapWindowContainerUtilsGetParentLevel(ZMapWindowContainerGroup container_group,
								ZMapContainerLevelType   level)
{
  ZMapContainerLevelType curr_level;

  while((curr_level = container_group->level) != level)
    {
      FooCanvasItem *item = (FooCanvasItem *)(container_group);

      if(!(container_group = zmapWindowContainerGetNextParent(item)))
	break;
    }

  return container_group;
}









/* The following are a set of simple functions to init, increment and decrement stats counters. */

void zmapWindowItemStatsInit(ZMapWindowItemStats stats, GType item_type)
{
  MyGObjectInfo object_data ;

  if ((object_data = findObjectInfo(item_type)))
    {
      stats->type = object_data->obj_type ;
      stats->name = object_data->obj_name ;
      stats->size = object_data->obj_size ;
      stats->total = stats->curr = 0 ;
    }

  return ;
}

void zmapWindowItemStatsIncr(ZMapWindowItemStats stats)
{
  stats->total++ ;
  stats->curr++ ;

  return ;
}

/* "total" counter is a cumulative total so is not decremented. */
void zmapWindowItemStatsDecr(ZMapWindowItemStats stats)
{
  stats->curr-- ;

  return ;
}


/* Find the array entry for a particular type. */
static MyGObjectInfo findObjectInfo(GType type)
{
  MyGObjectInfo object_data = NULL ;
  static MyGObjectInfo objects = NULL ;
  MyGObjectInfo tmp ;

  if (!objects)
    objects = initObjectDescriptions() ;

  tmp = objects ;
  while (tmp->get_type_func)
    {
      if (tmp->obj_type == type)
	{
	  object_data = tmp ;
	  break ;
	}

      tmp++ ;
    }

  return object_data ;
}



/* Fills in an array of structs which can be used for various operations on all the
 * items we use on the canvas in zmap.
 *
 * NOTE that the array is ordered so that for the getType/print functions the
 * subclassing order and final type will be correct, i.e. we have parent classes
 * followed by derived classes.
 *
 *  */
static MyGObjectInfo initObjectDescriptions(void)
{
  static MyGObjectInfoStruct object_descriptions[] =
    {
      /* Original foocanvas items */
      {foo_canvas_item_get_type, 0, NULL, sizeof(FooCanvasItem)},
      {foo_canvas_group_get_type, 0, NULL, sizeof(FooCanvasGroup)},
      {foo_canvas_line_get_type, 0, NULL, sizeof(FooCanvasLine)},
      {foo_canvas_pixbuf_get_type, 0, NULL, sizeof(FooCanvasPixbuf)},
      {foo_canvas_polygon_get_type, 0, NULL, sizeof(FooCanvasPolygon)},
      {foo_canvas_re_get_type, 0, NULL, sizeof(FooCanvasRE)},
      {foo_canvas_text_get_type, 0, NULL, sizeof(FooCanvasText)},
      {foo_canvas_widget_get_type, 0, NULL, sizeof(FooCanvasWidget)},

      /* zmap types added to foocanvas */
      {foo_canvas_float_group_get_type, 0, NULL, sizeof(FooCanvasFloatGroup)},
      {foo_canvas_line_glyph_get_type, 0, NULL, sizeof(FooCanvasLineGlyph)},
      {foo_canvas_zmap_text_get_type, 0, NULL, sizeof(FooCanvasZMapText)},

      /* zmap item types */
      {zMapWindowCanvasItemGetType, 0, NULL, sizeof(zmapWindowCanvasItem)},
      {zmapWindowContainerGroupGetType, 0, NULL, sizeof(zmapWindowContainerGroup)},
      {zmapWindowContainerAlignmentGetType, 0, NULL, sizeof(zmapWindowContainerAlignment)},
      {zmapWindowContainerBlockGetType, 0, NULL, sizeof(zmapWindowContainerBlock)},
      {zmapWindowContainerContextGetType, 0, NULL, sizeof(zmapWindowContainerContext)},
      {zmapWindowContainerFeatureSetGetType, 0, NULL, sizeof(zmapWindowContainerFeatureSet)},
      /* end of array. */
      {NULL, 0, NULL, 0}
    } ;
  MyGObjectInfo tmp ;

  tmp = object_descriptions ;

  while (tmp->get_type_func)
    {
      tmp->obj_type = (tmp->get_type_func)() ;

      tmp->obj_name = (char *)g_type_name(tmp->obj_type) ;

      tmp++ ;
    }

  return object_descriptions ;
}




/*!
 * \brief Get the GType for ZMapWindowCanvasItem
 *
 * \details Registers the ZMapWindowCanvasItem class if necessary, and returns the type ID
 *          associated to it.
 *
 * \return  The type ID of the ZMapWindowCanvasItem class.
 **/

GType zMapWindowCanvasItemGetType (void)
{
  static GType group_type = 0;

  if (!group_type)
    {
      static const GTypeInfo group_info =
	{
	  sizeof (zmapWindowCanvasItemClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_window_canvas_item_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (zmapWindowCanvasItem),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) zmap_window_canvas_item_init,
	  NULL
	};

      group_type = g_type_register_static (foo_canvas_item_get_type (),
					   ZMAP_WINDOW_CANVAS_ITEM_NAME,
					   &group_info,
					   0);
    }

  return group_type;
}



/* Class initialization function for ZMapWindowCanvasItemClass */
static void zmap_window_canvas_item_class_init (ZMapWindowCanvasItemClass window_class)
{
  //  GtkObjectClass *object_class;
  GtkObjectClass *object_class;
  FooCanvasItemClass *item_class;
  GType canvas_item_type, parent_type;
  int make_clickable_bmp_height  = 4;
  int make_clickable_bmp_width   = 16;
  char make_clickable_bmp_bits[] =
    {
      0x00, 0x00,
      0x00, 0x00,
      0x00, 0x00,
      0x00, 0x00
    } ;

  //  gobject_class = (GObjectClass *)window_class;
  object_class  = (GtkObjectClass *)window_class;
  item_class    = (FooCanvasItemClass *)window_class;

  canvas_item_type = g_type_from_name(ZMAP_WINDOW_CANVAS_ITEM_NAME);
  parent_type      = g_type_parent(canvas_item_type);

  group_parent_class_G = gtk_type_class(parent_type);


  object_class->destroy = zmap_window_canvas_item_destroy;

  window_class->get_style    = zmap_window_canvas_item_get_style;

  window_class->fill_stipple = gdk_bitmap_create_from_data(NULL, &make_clickable_bmp_bits[0],
							   make_clickable_bmp_width,
							   make_clickable_bmp_height) ;

  /* init the stats fields. */
  zmapWindowItemStatsInit(&(window_class->stats), ZMAP_TYPE_CANVAS_ITEM) ;

  return ;
}


/* Object initialization function for ZMapWindowCanvasItem */
static void zmap_window_canvas_item_init (ZMapWindowCanvasItem canvas_item)
{
  ZMapWindowCanvasItemClass canvas_item_class ;

  canvas_item_class = ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item) ;

  zmapWindowItemStatsIncr(&(canvas_item_class->stats)) ;

  return ;
}


static ZMapFeatureTypeStyle zmap_window_canvas_item_get_style(ZMapWindowCanvasItem canvas_item)
{
  ZMapFeatureTypeStyle style = NULL;

  zMapLogReturnValIfFail(canvas_item != NULL, NULL);
  zMapLogReturnValIfFail(canvas_item->feature != NULL, NULL);

  style = *canvas_item->feature->style;

  return style;
}




/* Destroy handler for canvas groups */
static void zmap_window_canvas_item_destroy (GtkObject *gtkobject)
{
  ZMapWindowCanvasItem canvas_item;
  ZMapWindowCanvasItemClass canvas_item_class ;
  //  int i;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("In %s %s()\n", __FILE__, __PRETTY_FUNCTION__) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  canvas_item = ZMAP_CANVAS_ITEM(gtkobject);
  canvas_item_class = ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item) ;

  canvas_item->feature = NULL;

  /* NOTE FooCanavsItems call destroy not destroy, which is in a Gobject  but not a GTK Object */
  if(G_OBJECT_CLASS (group_parent_class_G)->dispose)
    (G_OBJECT_CLASS (group_parent_class_G)->dispose)(G_OBJECT(gtkobject));

  zmapWindowItemStatsDecr(&(canvas_item_class->stats)) ;

  return ;
}





/* like foo but for a ZMap thingy */
gboolean zMapWindowCanvasItemShowHide(ZMapWindowCanvasItem item, gboolean show)
{
  ZMapWindowCanvasItemClass class = ZMAP_CANVAS_ITEM_GET_CLASS(item);
  gboolean ret = FALSE;

  if(class->showhide)
    ret = class->showhide((FooCanvasItem *) item, show);
  else
    {
      if(show)
	foo_canvas_item_show(FOO_CANVAS_ITEM(item));
      else
	foo_canvas_item_hide(FOO_CANVAS_ITEM(item));
    }

  return ret;

}



/* gross tree access. any item -> container group */

ZMapWindowContainerGroup zmapWindowContainerChildGetParent(FooCanvasItem *item)
{
  ZMapWindowContainerGroup container_group = NULL;

  if (ZMAP_IS_CONTAINER_GROUP(item))
    {
      container_group = ZMAP_CONTAINER_GROUP(item);
    }
  return container_group;
}




ZMapWindowContainerGroup zmapWindowContainerGetNextParent(FooCanvasItem *item)
{
  ZMapWindowContainerGroup container_group = NULL;

  if ((container_group = zmapWindowContainerChildGetParent(item)))
    {
      ZMapWindowContainerGroup tmp_group;
      FooCanvasItem *parent = NULL;
      FooCanvasItem *tmp_item;

      tmp_group = container_group;
      tmp_item  = FOO_CANVAS_ITEM(container_group);

      container_group = NULL;

      if(tmp_group->level == ZMAPCONTAINER_LEVEL_ROOT)
	{
	  parent = FOO_CANVAS_ITEM(tmp_group);
	}
      else if(tmp_item->parent)
	{
	  parent = tmp_item->parent;
	}

      if(parent && ZMAP_IS_CONTAINER_GROUP(parent))
	container_group = ZMAP_CONTAINER_GROUP(parent);
    }

  return container_group;
}




/* Returns the nth item in the container (0-based), there are two special values for
 * nth_item for the first and last items:  ZMAPCONTAINER_ITEM_FIRST, ZMAPCONTAINER_ITEM_LAST */
FooCanvasItem *zmapWindowContainerGetNthFeatureItem(ZMapWindowContainerGroup container, int nth_item)
{
  FooCanvasItem *item = NULL ;
  FooCanvasGroup *features ;

  features = (FooCanvasGroup *)zmapWindowContainerGetFeatures(container) ;

  if (nth_item == 0 || nth_item == ZMAPCONTAINER_ITEM_FIRST)
    {
      if (features->item_list)
        item = FOO_CANVAS_ITEM(features->item_list->data) ;
    }
  else if (nth_item == ZMAPCONTAINER_ITEM_LAST)
    {
      if (features->item_list_end)
        item = FOO_CANVAS_ITEM(features->item_list_end->data) ;
    }
  else
    {
      if (features->item_list)
        item = FOO_CANVAS_ITEM((g_list_nth(features->item_list, nth_item))->data) ;
    }

  return item ;
}




/*!
 * \brief Iterate through feature items
 *
 * Given any item that is a direct child of a column group (e.g. not a subfeature), returns
 * the previous or next item that optionally satisfies item_test_func_cb(). The function skips
 * over items that fail these tests.
 *
 * direction controls which way the item list is processed and if wrap is TRUE then processing
 * wraps around on reaching the end of the list.
 *
 * If no item can be found then the original will be returned, note that if item_test_func_cb()
 * was specified and the original item does not satisfy item_test_func_cb() then NULL is returned.
 *
 * */
FooCanvasItem *zmapWindowContainerGetNextFeatureItem(FooCanvasItem *orig_item,
						     ZMapContainerItemDirection direction, gboolean wrap,
						     zmapWindowContainerItemTestCallback item_test_func_cb,
						     gpointer user_data)
{
  FooCanvasItem *item = NULL ;
  ZMapWindowContainerGroup container_group;
  ZMapWindowContainerFeatures container_features;
  FooCanvasGroup *group ;

  if (ZMAP_IS_CONTAINER_GROUP(orig_item))
    {
      /* If it's a container, get it's parents so we can step through containers. */
      container_group = zmapWindowContainerGetNextParent(orig_item) ;

      container_features = zmapWindowContainerGetFeatures(container_group);
    }
  else
    {
      /* If its a feature then return the column so we can step through features. */
      container_group = zmapWindowContainerCanvasItemGetContainer(orig_item);

      container_features = zmapWindowContainerGetFeatures(container_group);
    }

  group = FOO_CANVAS_GROUP(container_features) ;

  item = getNextFeatureItem(group, orig_item, direction, wrap,
			    item_test_func_cb, user_data) ;

  return item ;
}




static FooCanvasItem *getNextFeatureItem(FooCanvasGroup *group,
					 FooCanvasItem *orig_item,
					 ZMapContainerItemDirection direction, gboolean wrap,
					 zmapWindowContainerItemTestCallback item_test_func_cb,
					 gpointer user_data)
{
  FooCanvasItem *item = NULL ;
  FooCanvasGroup *features ;
  GList *feature_ptr ;
  FooCanvasItem *found_item = NULL ;

  features = group ;

  feature_ptr = zMap_foo_canvas_find_list_item(features, orig_item) ;

  while (feature_ptr && !item && found_item != orig_item)
    {
      if (direction == ZMAPCONTAINER_ITEM_NEXT)
	{
	  feature_ptr = g_list_next(feature_ptr) ;

	  if (!feature_ptr && wrap)
	    {
	      feature_ptr = g_list_first(features->item_list);

	      /* check that the group really knows where the start of
	       * the list is */

	      if(feature_ptr != features->item_list)
		features->item_list = feature_ptr;
	    }
	}
      else
	{
	  feature_ptr = g_list_previous(feature_ptr) ;

	  if (!feature_ptr && wrap)
	    {
	      feature_ptr = g_list_last(features->item_list) ;

	      /* check that the group really knows where the end of
	       * the list is */

	      if(feature_ptr != features->item_list_end)
		features->item_list_end = feature_ptr;
	    }
	}

      /* If wrap is FALSE then feature_ptr can be NULL */
      if (feature_ptr)
	{
	  found_item = (FooCanvasItem *)(feature_ptr->data) ;



	  if (!item_test_func_cb || item_test_func_cb(found_item, user_data))
	    item = found_item ;
	}
    }

  return item ;
}




gboolean zMapWindowCanvasItemSetFeaturePointer(ZMapWindowCanvasItem item, ZMapFeature feature)
{
  int pop = 0;

  /* collpased features have 0 population, the one that was displayed has the total
   * if we use window search then select a collapsed feature we have to update this
   * see zmapViewfeatureCollapse.c etc; scan for 'population' and 'collasped'
   */

  if(item->feature)
    pop = item->feature->population;

  if(feature->flags.collapsed)
    feature->population = pop;

  item->feature = feature;

  return(TRUE);
}





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

    default:
      {
	G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);

	break;
      }
    } /* switch(param_id) */

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
#define DEBUG	0
#if DEBUG
  ZMapWindowContainerGroup zg = (ZMapWindowContainerGroup) item;
#endif

  canvas_group   = (FooCanvasGroup *)item;
  item_visible   = ((item->object.flags & FOO_CANVAS_ITEM_VISIBLE) == FOO_CANVAS_ITEM_VISIBLE);

#if DEBUG
  {
    char *x = "?";

    if(ZMAP_IS_CONTAINER_GROUP(item) && zg->level <= ZMAPCONTAINER_LEVEL_FEATURESET)
      {
	char *name = "none";

	ZMapFeatureAny f = ((ZMapWindowContainerGroup) item)->feature_any;

	if(f)
	  name = (char *) g_quark_to_string(f->unique_id);
	x = g_strdup_printf("group %s level %d",name, ((ZMapWindowContainerGroup) item)->level);
	printf("group update 1 %s: %f %f %f %f\n", x, item->x1, item->y1, item->x2, item->y2);
      }
  }
#endif

  item->x2 = item->x1;		/* reset group to zero size to avoid stretchy items expanding it by proxy */
  item->y2 = item->y1;

  (item_parent_class_G->update)(item, i2w_dx, i2w_dy, flags);

#if DEBUG
  {
    char *x = "?";

    if(ZMAP_IS_CONTAINER_GROUP(item) && zg->level < ZMAPCONTAINER_LEVEL_FEATURESET)
      {
	char *name = "none";

	ZMapFeatureAny f = ((ZMapWindowContainerGroup) item)->feature_any;

	if(f)
	  name = (char *) g_quark_to_string(f->unique_id);
	x = g_strdup_printf("group %s level %d",name, ((ZMapWindowContainerGroup) item)->level);
	printf("group update 2 %s: %f %f %f %f\n", x, item->x1, item->y1, item->x2, item->y2);
      }
  }
#endif

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

#if DEBUG
	  if(zg->level <= ZMAPCONTAINER_LEVEL_FEATURESET)
	    {
	      char *x = "?";

	      if(ZMAP_IS_WINDOW_FEATURESET_ITEM(foo))	/* these could be groups or even normal foo items */
		x = g_strdup_printf("%s layer %x",g_quark_to_string(zMapWindowCanvasFeaturesetGetId(featureset)),
				    zMapWindowCanvasFeaturesetGetLayer(featureset));
	      else if(ZMAP_IS_CONTAINER_GROUP(item))
		x = g_strdup_printf("group %s", g_quark_to_string(((ZMapWindowContainerGroup) item) ->feature_any->unique_id));
	      else
		x = g_strdup_printf("foo %d %d",FOO_IS_CANVAS_LINE(foo),FOO_IS_CANVAS_RECT(foo));
	      printf("child %s %f %f %f %f\n", x, foo->x1, foo->y1, foo->x2, foo->y2);
	    }
#endif
	  if(ZMAP_IS_WINDOW_FEATURESET_ITEM(foo))	/* these could be groups or even normal foo items */
	    {
	      guint layer = zMapWindowCanvasFeaturesetGetLayer(featureset);

#if DEBUG
	      {
		char *x = "?";

		x = g_strdup_printf("%s layer %x",g_quark_to_string(zMapWindowCanvasFeaturesetGetId(featureset)),
				    zMapWindowCanvasFeaturesetGetLayer(featureset));
		if(zg->level < ZMAPCONTAINER_LEVEL_FEATURESET)
		  printf("group update bgnd %s: %f %f %f %f\n", x, item->x1, item->y1, item->x2, item->y2);
	      }
#endif
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
#if DEBUG
		  if(zg->level < 4) printf("update %d set x %s = %f %f\n",zg->level, g_quark_to_string(zMapWindowCanvasFeaturesetGetId(featureset)), foo->x1, foo->x2);
#endif
		  zMapWindowCanvasFeaturesetSetWidth(featureset,size);
		}

	      if(layer & ZMAP_CANVAS_LAYER_STRETCH_Y)
		{
		  double y1, y2;

		  /* NOTE for featureset level groups we set Y extend by containing block sequence coords */
		  /* this is used for higher level groups */
		  foo->y1 = item->y1;
		  foo->y2 = item->y2;
#if DEBUG
		  if(zg->level < 4) printf("update %d set y %s = %f %f\n", zg->level, g_quark_to_string(zMapWindowCanvasFeaturesetGetId(featureset)), foo->y1, foo->y2);
#endif
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



static void zmap_window_container_group_destroy     (GtkObject *gtkobject)
{
  GtkObjectClass *gtkobject_class;

  gtkobject_class = (GtkObjectClass *)item_parent_class_G;

  if(gtkobject_class->destroy)
    (gtkobject_class->destroy)(gtkobject);

  return ;
}











static void eachContainer(gpointer data, gpointer user_data) ;


typedef struct ContainerRecursionDataStruct_
{
  ZMapContainerLevelType     stop ;

  ZMapContainerUtilsExecFunc container_enter_cb   ;
  gpointer                   container_enter_data ;

  ZMapContainerUtilsExecFunc container_leave_cb   ;
  gpointer                   container_leave_data ;

} ContainerRecursionDataStruct, *ContainerRecursionData;




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





/*!
 * \brief   Function to access the ZMapFeature
 * \details Although a little surprising, NULL maybe returned legitimately,
 *          as not all ZMapWindowCanvasItem subclasses require features...
 *          It is architected this way so that we can have _all_ the items
 *          on the canvas below ZMapWindowContainerFeatureSet extend
 *          ZMapWindowCanvasItem .
 * \param   canvas_item The owning ZMapWindowCanvasItem
 * \return  The ZMapFeature or NULL. NULL maybe returned legitimately!
 */
ZMapFeature zMapWindowCanvasItemGetFeature(FooCanvasItem *any_item)
{
  ZMapWindowCanvasItem canvas_item ;
  ZMapFeature feature = NULL ;

  if (ZMAP_IS_CANVAS_ITEM(any_item))
    canvas_item = ZMAP_CANVAS_ITEM(any_item) ;
  else
    canvas_item = zMapWindowCanvasItemIntervalGetObject(any_item) ;

  if (canvas_item)
    feature = canvas_item->feature ;

  return feature ;
}



/* Get at parent... */
/* mh17 NOTE this is now a null function, unless we have naked foo items on the canvas */
ZMapWindowCanvasItem zMapWindowCanvasItemIntervalGetObject(FooCanvasItem *item)
{
  ZMapWindowCanvasItem canvas_item = NULL;

  if(ZMAP_IS_CANVAS_ITEM(item))
    canvas_item = ZMAP_CANVAS_ITEM(item);

  return canvas_item;
}











/* Recursion... */

/* unified way to descend and do things to ALL and every or just some */
void zmapWindowContainerUtilsExecuteFull(ZMapWindowContainerGroup   container_group,
					 ZMapContainerLevelType     stop_at_type,
					 ZMapContainerUtilsExecFunc container_enter_cb,
					 gpointer                   container_enter_data,
					 ZMapContainerUtilsExecFunc container_leave_cb,
					 gpointer                   container_leave_data)
{
  ContainerRecursionDataStruct data  = {0,NULL};
  //  ZMapWindowCanvas zmap_canvas;
  FooCanvasItem *parent;

  zMapAssert(stop_at_type >= ZMAPCONTAINER_LEVEL_ROOT &&
	     stop_at_type <= ZMAPCONTAINER_LEVEL_FEATURESET) ;

  parent = (FooCanvasItem *)container_group;

  data.stop                    = stop_at_type;

  data.container_enter_cb   = container_enter_cb;
  data.container_enter_data = container_enter_data;
  data.container_leave_cb   = container_leave_cb;
  data.container_leave_data = container_leave_data;



  eachContainer((gpointer)container_group, &data) ;

  return ;
}

/* unified way to descend and do things to ALL and every or just some */
void zmapWindowContainerUtilsExecute(ZMapWindowContainerGroup   parent,
				     ZMapContainerLevelType     stop_at_type,
				     ZMapContainerUtilsExecFunc container_enter_cb,
				     gpointer                   container_enter_data)
{
  zmapWindowContainerUtilsExecuteFull(parent, stop_at_type,
				      container_enter_cb,
				      container_enter_data,
				      NULL, NULL);
  return ;
}


/* Called for every container while descending.... */
static void eachContainer(gpointer data, gpointer user_data)
{
  ZMapWindowContainerGroup container = (ZMapWindowContainerGroup)data ;
  ContainerRecursionData   all_data  = (ContainerRecursionData)user_data ;
  ZMapContainerLevelType   level     = ZMAPCONTAINER_LEVEL_INVALID;
  FooCanvasGroup *children ;
  FooCanvasPoints this_points_data = {NULL}, parent_points_data = {NULL};
  FooCanvasPoints *this_points = NULL, *parent_points = NULL;
  double *bound = NULL, spacing = 0.0, bound_data = 0.0, coords[4] = {0.0, 0.0, 0.0, 0.0};

  if(!ZMAP_IS_CONTAINER_GROUP(container))	/* now we can have background CanvasFeaturesets in top level groups, just one item list */
    return;

  children = (FooCanvasGroup *)zmapWindowContainerGetFeatures(container) ;

  /* We need to get what we need in case someone destroys it under us. */
  level   = container->level;
  spacing = container->this_spacing;

  zMapAssert(level >  ZMAPCONTAINER_LEVEL_INVALID &&
             level <= ZMAPCONTAINER_LEVEL_FEATURESET);

  this_points   = &this_points_data;
  parent_points = &parent_points_data;
  bound         = &bound_data;

  this_points->coords   = &coords[0];
  parent_points->coords = &coords[0];

  /* Execute pre-recursion function. */
  if(all_data->container_enter_cb)
    (all_data->container_enter_cb)(container, this_points, level, all_data->container_enter_data);

  /* Recurse ? */
  if (children)
    {
      if (all_data->stop != level)
	{
	  g_list_foreach(children->item_list, eachContainer, user_data) ;
	}
    }


  /* Execute post-recursion function. */
  if(all_data->container_leave_cb)
    (all_data->container_leave_cb)(container, this_points, level, all_data->container_leave_data);

  return ;
}



void zmapWindowCanvasItemGetColours(ZMapFeatureTypeStyle style, ZMapStrand strand, ZMapFrame frame,
                                    ZMapStyleColourType    colour_type,
                                    GdkColor **fill, GdkColor **draw, GdkColor **outline,
                                    GdkColor              *default_fill,
                                    GdkColor              *border)
{
  ZMapStyleParamId colour_target = STYLE_PROP_COLOURS;

  if (zMapStyleIsFrameSpecific(style))
    {
      switch (frame)
	{
	case ZMAPFRAME_0:
	  colour_target = STYLE_PROP_FRAME0_COLOURS ;
	  break ;
	case ZMAPFRAME_1:
	  colour_target = STYLE_PROP_FRAME1_COLOURS ;
	  break ;
	case ZMAPFRAME_2:
	  colour_target = STYLE_PROP_FRAME2_COLOURS ;
	  break ;
	default:
	  //            zMapAssertNotReached() ; no longer valid: frame specific style by eg for swissprot we display in one col on startup
	  break ;
	}

      zMapStyleGetColours(style, colour_target, colour_type,
			  fill, draw, outline);
    }

  colour_target = STYLE_PROP_COLOURS;
  if (strand == ZMAPSTRAND_REVERSE && zMapStyleColourByStrand(style))
    {
      colour_target = STYLE_PROP_REV_COLOURS;
    }

  if (*fill == NULL && *draw == NULL && *outline == NULL)
    zMapStyleGetColours(style, colour_target, colour_type, fill, draw, outline);


  if (colour_type == ZMAPSTYLE_COLOURTYPE_SELECTED)
    {
      if(default_fill)
	*fill = default_fill;
      if(border)
	*outline = border;
    }
}



static void window_canvas_invoke_set_colours(gpointer list_data, gpointer user_data)
{
  FooCanvasItem *interval        = FOO_CANVAS_ITEM(list_data);
  EachIntervalData interval_data = (EachIntervalData)user_data;

  if(interval_data->feature)
    {
      ZMapFeatureSubPartSpan sub_feature = NULL;


      /* new interface via calling stack */
      sub_feature = interval_data->sub_feature;

      if(interval_data->klass->set_colour)
	interval_data->klass->set_colour(interval_data->parent, interval, interval_data->feature, sub_feature,
					 interval_data->style_colour_type,
					 interval_data->colour_flags,
					 interval_data->default_fill_colour,
					 interval_data->border_colour);
    }

  return ;
}



static void removeList(gpointer data, gpointer user_data_unused)
{
  GList *user_hidden_items = (GList *)data ;

  g_list_free(user_hidden_items) ;

  return ;
}
