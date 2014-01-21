/*  File: zmapWindowContainerFeatureSet.c
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See zmapWindowContainerFeatureSet.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <string.h>

#include <ZMap/zmapBase.h>
#include <zmapWindow_P.h>                                   /* for ZMapWindowStats... */
#include <zmapWindowContainerFeatureSet_I.h>



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

static ZMapWindowContainerGroup getChildById(ZMapWindowContainerGroup group,
                                             GQuark id, ZMapStrand strand, ZMapFrame frame) ;
static void removeList(gpointer data, gpointer user_data_unused) ;



/* 
 *                 Globals
 */

static GObjectClass *parent_class_G = NULL ;





/* 
 *                 External routines
 */



/* 
 * object method functions.
 */


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



/* Get the GType for the ZMapWindowContainerFeatureSet GObjects
 *
 * returns GType corresponding to the GObject as registered by glib.
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



FooCanvasItem *zmapWindowContainerFeatureSetFindCanvasColumn(ZMapWindowContainerGroup group,
                                                             GQuark align, GQuark block,
                                                             GQuark set, ZMapStrand strand, ZMapFrame frame)
{
  FooCanvasItem *column_item = NULL ;

  if (group)
    {
      ZMapWindowContainerGroup column_group = group ;

      if ((column_group = getChildById(column_group, align, ZMAPSTRAND_NONE, ZMAPFRAME_NONE))
	  && (column_group = getChildById(column_group, block, ZMAPSTRAND_NONE, ZMAPFRAME_NONE))
	  && (column_group = getChildById(column_group, set, strand, frame)))
	column_item = FOO_CANVAS_ITEM(column_group) ;
    }

  return column_item ;
}


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
void zMapWindowContainerFeatureSetShowHideMaskedFeatures(ZMapWindowContainerFeatureSet container,
                                                         gboolean show, gboolean set_colour)
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

/* Access to the stack of hidden items, returns the most recent list. */
GList *zmapWindowContainerFeatureSetPopHiddenStack(ZMapWindowContainerFeatureSet container_set)
{
  GList *hidden_items = NULL;

  hidden_items = g_queue_pop_head(container_set->user_hidden_stack);

  return hidden_items;
}

/* Access to the stack of hidden items.  This adds a list. */
void zmapWindowContainerFeatureSetPushHiddenStack(ZMapWindowContainerFeatureSet container_set,
						  GList *hidden_items_list)
{
  g_queue_push_head(container_set->user_hidden_stack, hidden_items_list) ;

  return ;
}


ZMapWindow zMapWindowContainerFeatureSetGetWindow(ZMapWindowContainerFeatureSet container_set)
{
  return container_set->window;
}


GQuark zmapWindowContainerFeatureSetGetColumnId(ZMapWindowContainerFeatureSet container_set)
{
  GQuark column_id = 0 ;

  if (ZMAP_IS_CONTAINER_FEATURESET(container_set))
    column_id = container_set->unique_id ;

  return column_id ;
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

ZMapStyleBumpMode zMapWindowContainerFeatureSetGetContainerBumpMode(ZMapWindowContainerFeatureSet container_set)
{
  ZMapStyleBumpMode mode ;

  mode = container_set->bump_mode ;

  return mode ;
}

gboolean zMapWindowContainerFeatureSetSetBumpMode(ZMapWindowContainerFeatureSet container_set,
                                                  ZMapStyleBumpMode bump_mode)
{
  gboolean result = FALSE;

  if(bump_mode >=ZMAPBUMP_INVALID && bump_mode <= ZMAPBUMP_END)
    {
      container_set->bump_mode = bump_mode;
      result = TRUE;
    }

  return(result);
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





/* 
 *                    Internal routines
 */


/* look up a group in the the item_list
 * rather tediously the canvas is structured differently than the feature context and FToIHash
 * - it has a strand layer under block
 * - 3-frame columns all have the same id, whereas in the FToI hash the names are mangled
 */
static ZMapWindowContainerGroup getChildById(ZMapWindowContainerGroup group,
                                             GQuark id, ZMapStrand strand, ZMapFrame frame)
{
  ZMapWindowContainerGroup g;
  GList *l;
  FooCanvasGroup *children = (FooCanvasGroup *) zmapWindowContainerGetFeatures(group) ;
  /* this gets the group one of the four item positions */

  for(l = children->item_list;l;l = l->next)
    {
      /* now we can have background CanvasFeaturesets in top level groups, just one item list */
      if(!ZMAP_IS_CONTAINER_GROUP(l->data))
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
	  if(g->feature_any->unique_id == id)
	    return g;
	}
    }
  return NULL;
}



static void removeList(gpointer data, gpointer user_data_unused)
{
  GList *user_hidden_items = (GList *)data ;

  g_list_free(user_hidden_items) ;

  return ;
}


