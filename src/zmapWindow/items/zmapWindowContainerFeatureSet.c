/*  File: zmapWindowContainerFeatureSet.c
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jul 27 17:06 2010 (edgrif)
 * Created: Mon Jul 30 13:09:33 2007 (rds)
 * CVS info:   $Id: zmapWindowContainerFeatureSet.c,v 1.38 2010-11-01 09:59:07 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>





#include <string.h>		/* memset */

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsFoo.h>
#include <ZMap/zmapStyle.h>
#include <zmapWindowCanvasItem_I.h> /* ->feature access in SortFeatures */
#include <zmapWindowContainerGroup_I.h>
#include <zmapWindowContainerFeatureSet_I.h>
#include <zmapWindowContainerUtils.h>

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

typedef struct
{
  GValue     *gvalue;
  const char *spec_name;
  guint       param_id;
} ItemFeatureValueDataStruct, *ItemFeatureValueData;

typedef struct
{
  GList      *list;
  ZMapFeature feature;
}ListFeatureStruct, *ListFeature;

typedef struct
{
  GQueue     *queue;
  ZMapFeature feature;
}QueueFeatureStruct, *QueueFeature;



static void zmap_window_item_feature_set_class_init  (ZMapWindowContainerFeatureSetClass container_set_class);
static void zmap_window_item_feature_set_init        (ZMapWindowContainerFeatureSet container_set);
static void zmap_window_item_feature_set_set_property(GObject      *gobject,
						      guint         param_id,
						      const GValue *value,
						      GParamSpec   *pspec);
static void zmap_window_item_feature_set_get_property(GObject    *gobject,
						      guint       param_id,
						      GValue     *value,
						      GParamSpec *pspec);
static void zmap_window_item_feature_set_destroy     (GtkObject *gtkobject);

static gint comparePosition(gconstpointer a, gconstpointer b);
static gint comparePositionRev(gconstpointer a, gconstpointer b);

//static void extract_value_from_style_table(gpointer key, gpointer value, gpointer user_data);
//static void value_to_each_style_in_table(gpointer key, gpointer value, gpointer user_data);
//static void reset_bump_mode_cb(gpointer key, gpointer value, gpointer user_data);
static void removeList(gpointer data, gpointer user_data_unused) ;




static GObjectClass *parent_class_G = NULL;
//static gboolean debug_table_ids_G = FALSE;



/*
 *  YOU MIGHT HOPE THIS DEALT WITH THE HASH AND OTHER STUFF BUT IT DOESN'T AND SOME OF
 * IT'S FUNCTIONS (E.G. zmapWindowContainerFeatureSetRemoveAllItems()) DO NOT SIT
 * WELL WITH THE HASH.....IT HASN'T BEEN THOUGHT THROUGH....
 */



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

      container_set->window    = window;
      container_set->strand    = strand;
      container_set->frame     = frame;
      container_set->align_id  = align_id;
      container_set->block_id  = block_id;
      container_set->unique_id = feature_set_unique_id;
      container_set->original_id = feature_set_original_id;

      container_set->style = style;

#if MH17_NO_MORE_STYLE_TABLES
      GList *list;
      if((list = g_list_first(style_list)))
	{
	  do
	    {
	      ZMapFeatureTypeStyle style;

	      style = (ZMapFeatureTypeStyle)(list->data);

	      zmapWindowContainerFeatureSetStyleFromStyle(container_set, style);
	    }
	  while((list = g_list_next(list)));
	}
#endif

      zmapWindowContainerSetVisibility((FooCanvasGroup *)container_set, FALSE);
    }

  return container_set;
}


#if 1       /* MH17_NO_RECOVER */
/* replaced by utils/attach_feature_any
 * stats are not handled for the moment
 * Ed was looking at a base class to do that if needs be we can add something back
 * BUT if we remove it the we have to fix up stats ref'd fron the factory
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

#endif



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
 * \brief broken!
 */

#if NOT_USED
ZMapWindowStats zmapWindowContainerFeatureSetRecoverStats(ZMapWindowContainerFeatureSet container_set)
{
  ZMapWindowStats stats = NULL;

#ifdef STATS_GO_IN_PARENT_OBJECT
  if(container_set->has_stats)
    {
      stats = g_object_get_data(G_OBJECT(container_set->column_container), ITEM_FEATURE_STATS);

      if(!stats)
	{
	  g_warning("%s", "No Stats!");
	  container_set->has_stats = FALSE;
	}
    }
#endif

  return stats;
}
#endif

#if MH17_NO_MORE_STYLE_TABLES
/*!
 * \brief Columns require a _copy_ of the global list of styles in order to function.
 *        This function both accesses and creates the 'local' copy of the style that is
 *        cached in the container.
 *
 * The styles are stored in a hash for speed of access.  Drawing the features in the
 * column requires access to these styles and the more features the more the styles need
 * to be looked up.  Using a GList or GDatalist would be too slow.
 *
 * \param container  The container with the hash of styles.
 * \param style      The style that is to be accessed/copied.
 *
 * \return The cached style.
 */

ZMapFeatureTypeStyle zmapWindowContainerFeatureSetStyleFromStyle(ZMapWindowContainerFeatureSet container_set,
								 ZMapFeatureTypeStyle         style2copy)
{
  ZMapFeatureTypeStyle local_style = NULL;

  g_return_val_if_fail(ZMAP_IS_CONTAINER_FEATURESET(container_set), local_style);
  g_return_val_if_fail(ZMAP_IS_FEATURE_STYLE(style2copy), local_style);

  if(!(local_style = zmapWindowStyleTableFind(container_set->style_table, zMapStyleGetUniqueID(style2copy))))
    {
      local_style = zmapWindowStyleTableAddCopy(container_set->style_table, style2copy);
    }

  return local_style;
}

/*!
 * \brief  As per zmapWindowContainerFeatureSetStyleFromStyle, but requires only a
 *         unique style id, so cannot copy the style.
 *
 * \param container  The container with the hash of styles.
 * \param unique-id  The unique id of a style to access from the container.
 *
 * \return The cached style.
 */

ZMapFeatureTypeStyle zmapWindowContainerFeatureSetStyleFromID(ZMapWindowContainerFeatureSet container_set,
							      GQuark                       style_unique_id)
{
  ZMapFeatureTypeStyle local_style = NULL;

  g_return_val_if_fail(ZMAP_IS_CONTAINER_FEATURESET(container_set), local_style);

  if(!(local_style = zmapWindowStyleTableFind(container_set->style_table, style_unique_id)))
    {
//      zMapAssertNotReached();
// mh17: we need to handle 'not there' for legacy code (see get_sub_feature_style() in collection feature.c

    }

  return local_style;
}
#endif

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

GQuark zmapWindowContainerFeatureSetGetColumnId(ZMapWindowContainerFeatureSet container_set)
{
  return container_set->unique_id;
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

  g_return_val_if_fail(ZMAP_IS_CONTAINER_FEATURESET(container_set), strand);

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
 * \brief Access the width of a column.
 *
 * \param The container to interrogate.
 *
 * \return The width of the container.
 */

double zmapWindowContainerFeatureSetGetWidth(ZMapWindowContainerFeatureSet container_set)
{
  double width = 0.0;
#if MH17_NO_MORE_STYLE_TABLES

  g_object_get(G_OBJECT(container_set),
	       ZMAPSTYLE_PROPERTY_WIDTH, &width,
	       NULL);
#else
  width = zMapStyleGetWidth(container_set->style);
#endif
  return width;
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

#if MH17_NO_MORE_STYLE_TABLES
  g_object_get(G_OBJECT(container_set),
	       ZMAPSTYLE_PROPERTY_BUMP_SPACING, &spacing,
	       NULL);
#else
  spacing = zMapStyleGetBumpSpace(container_set->style);
#endif
  return spacing;
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

#if MH17_NO_MORE_STYLE_TABLES
  ItemFeatureValueDataStruct value_data = {NULL};
  GValue value = {0};

  g_value_init(&value, G_TYPE_DOUBLE);

  g_value_set_double(&value, 0.0);

  value_data.spec_name = "not required";
  value_data.gvalue    = &value;

  if(1)
    {
      value_data.param_id  = ITEM_FEATURE__MIN_MAG;

      g_hash_table_foreach(container_set->style_table, extract_value_from_style_table, &value_data);

      min_mag = g_value_get_double(&value);
    }

  if(1)
    {
      value_data.param_id  = ITEM_FEATURE__MAX_MAG;

      g_hash_table_foreach(container_set->style_table, extract_value_from_style_table, &value_data);

      max_mag = g_value_get_double(&value);
    }

  g_value_unset(&value);
#else
  min_mag = zMapStyleGetMinMag(container_set->style);
  max_mag = zMapStyleGetMaxMag(container_set->style);
#endif

  if (min_mag != 0.0 || max_mag != 0.0)
    mag_sens = TRUE ;

  if (min_mag && min_mag_out)
    *min_mag_out = min_mag ;

  if (max_mag && max_mag_out)
    *max_mag_out = max_mag ;

  return mag_sens ;
}


/*!
>> a big thank you to roy for writing complicated code that is never used <<

 *  Functions to set/get display state of column, i.e. show, show_hide or hide. Complicated
 * by having an overall state for the column and potentially sub-states for sub-features.
 *
 * \param The container to interrogate.
 *
 * \return The display state of the container.
 */
ZMapStyleColumnDisplayState zmapWindowContainerFeatureSetGetDisplay(ZMapWindowContainerFeatureSet container_set)
{
  ZMapStyleColumnDisplayState display = ZMAPSTYLE_COLDISPLAY_SHOW;

#if MH17_NO_MORE_STYLE_TABLES
  g_object_get(G_OBJECT(container_set),
	       ZMAPSTYLE_PROPERTY_DISPLAY_MODE, &(container_set->settings.display_state),
	       NULL);

  display = container_set->settings.display_state ;
#else
  display = container_set->display_state ;
  if(display == ZMAPSTYLE_COLDISPLAY_INVALID)
      display = zMapStyleGetDisplay(container_set->style);   /* get initial state from set style */
#endif
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
					     ZMapStyleColumnDisplayState state)
{
#if MH17_NO_MORE_STYLE_TABLES
  ItemFeatureValueDataStruct value_data = {NULL};
  GValue value = {0};

  g_object_set(G_OBJECT(container_set),
	       ZMAPSTYLE_PROPERTY_DISPLAY_MODE, state,
	       NULL);

  g_value_init(&value, G_TYPE_UINT);

  g_value_set_uint(&value, state);

  value_data.spec_name = ZMAPSTYLE_PROPERTY_DISPLAY_MODE;
  value_data.gvalue    = &value;
  value_data.param_id  = ITEM_FEATURE_SET_VISIBLE;

  if(debug_table_ids_G)
    {
      zMapLogMessage("container_set->unique_id = '%s'", g_quark_to_string(container_set->unique_id));
    }

  g_hash_table_foreach(container_set->style_table, value_to_each_style_in_table, &value_data);

  g_value_unset(&value);
#else
      container_set->display_state = state;
#endif

  return ;
}




#if MH17_NO_MORE_STYLE_TABLES

/*!
 * \brief Example of how to change the setting in only one style.
 NOT USED
 *
 * Assuming the style is found the setting in the local copy is changed.
 *
 * \param container  The container which hold the style.
 * \param style      The style to change. This could only be a unique id actually.
 * \param state      The new value for display state in the style.
 *
 * \return void
 */

void zmapWindowContainerFeatureSetStyleDisplay(ZMapWindowContainerFeatureSet container_set,
					       ZMapFeatureTypeStyle          style,
					       ZMapStyleColumnDisplayState   state)
{
  ZMapFeatureTypeStyle local_style = NULL;
  ItemFeatureValueDataStruct value_data = {NULL};
  GValue value = {0};
  GQuark style_id = 0;

  style_id = zMapStyleGetUniqueID(style);

  if ((local_style = zmapWindowStyleTableFind(container_set->style_table, style_id)))
    {
      g_value_init(&value, G_TYPE_UINT);

      g_value_set_uint(&value, state);

      value_data.spec_name = ZMAPSTYLE_PROPERTY_DISPLAY_MODE;
      value_data.gvalue    = &value;
      value_data.param_id  = ITEM_FEATURE_SET_VISIBLE;

      value_to_each_style_in_table(GUINT_TO_POINTER(style_id), (gpointer) local_style, &value_data);

      g_value_unset(&value);
    }

  return ;
}

#endif

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

#if MH17_NO_MORE_STYLE_TABLES
  g_object_get(G_OBJECT(container_set),
	       ZMAPSTYLE_PROPERTY_SHOW_WHEN_EMPTY, &(container_set->settings.show_when_empty),
	       NULL);

  show = container_set->settings.show_when_empty;
#else
  show = zMapStyleGetShowWhenEmpty(container_set->style);
#endif
  return show;
}


/*!
 * \brief Access the frame mode of a column.
 *
 * \param The container to interrogate.
 *
 * \return The frame mode of the container.
 */
ZMapStyle3FrameMode zmapWindowContainerFeatureSetGetFrameMode(ZMapWindowContainerFeatureSet container_set)
{
  ZMapStyle3FrameMode frame_mode = ZMAPSTYLE_3_FRAME_INVALID;

#if MH17_NO_MORE_STYLE_TABLES
  g_return_val_if_fail(ZMAP_IS_CONTAINER_FEATURESET(container_set), frame_mode);

  g_object_get(G_OBJECT(container_set),
	       ZMAPSTYLE_PROPERTY_FRAME_MODE, &(container_set->settings.frame_mode),
	       NULL);

  frame_mode = container_set->settings.frame_mode;
#else
  frame_mode = zMapStyleGetFrameMode(container_set->style);
#endif
  return frame_mode;
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

#if MH17_NO_MORE_STYLE_TABLES
  g_return_val_if_fail(ZMAP_IS_CONTAINER_FEATURESET(container_set), FALSE);

  frame_mode = zmapWindowContainerFeatureSetGetFrameMode(container_set) ;

  if(frame_mode != ZMAPSTYLE_3_FRAME_NEVER)
    container_set->settings.frame_specific = TRUE;

  if(frame_mode == ZMAPSTYLE_3_FRAME_INVALID)
    {
      container_set->settings.frame_specific = FALSE;
    }

  frame_specific = container_set->settings.frame_specific;
#else
  frame_mode = zMapStyleGetFrameMode(container_set->style);
  frame_specific = zMapStyleIsFrameSpecific(container_set->style);
#endif

  if(frame_mode_out)
    *frame_mode_out = frame_mode;
  return frame_specific;
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

#if MH17_NO_MORE_STYLE_TABLES

  g_object_get(G_OBJECT(container_set),
	       ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC, &(container_set->settings.strand_specific),
	       ZMAPSTYLE_PROPERTY_SHOW_REVERSE_STRAND, &(container_set->settings.show_reverse_strand),
	       NULL) ;


  if (container_set->strand == ZMAPSTRAND_FORWARD
      || (container_set->settings.strand_specific && container_set->settings.show_reverse_strand))
    strand_show = TRUE ;
#else
  if (container_set->strand == ZMAPSTRAND_FORWARD
      || (zMapStyleIsStrandSpecific(container_set->style) && zMapStyleIsShowReverseStrand(container_set->style)))
    strand_show = TRUE ;
#endif

  return strand_show ;
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
#if MH17_NO_MORE_STYLE_TABLES
  g_object_get(G_OBJECT(container_set),
	       ZMAPSTYLE_PROPERTY_BUMP_MODE, &(container_set->settings.bump_mode),
	       NULL);
  mode = container_set->settings.bump_mode;
#else
  mode = container_set->bump_mode;

  if(mode == ZMAPBUMP_INVALID)
      mode = zMapStyleGetInitialBumpMode(container_set->style);
  if(mode == ZMAPBUMP_INVALID)
      mode = ZMAPBUMP_UNBUMP;
#endif

  return mode;
}


ZMapFeatureTypeStyle zMapWindowContainerFeatureSetGetStyle(ZMapWindowContainerFeatureSet container)
{
      return container->style;
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

#if MH17_NO_MORE_STYLE_TABLES
  g_object_get(G_OBJECT(container_set),
	       ZMAPSTYLE_PROPERTY_DEFAULT_BUMP_MODE, &(container_set->settings.default_bump_mode),
	       NULL);

  mode = container_set->settings.default_bump_mode;
#else
  mode = zMapStyleGetDefaultBumpMode(container_set->style);
#endif

  return mode;
}


ZMapStyleBumpMode zmapWindowContainerFeatureSetGetBumpUnmarked(ZMapWindowContainerFeatureSet container_set)
{
  gboolean x = TRUE;

#if MH17_NO_MORE_STYLE_TABLES
  container_set->settings.bump_unmarked = ZMAPSTYLE_COLDISPLAY_INVALID;

      // this gets the data from a lot of styles
      // perhaps it should be assigned when the property is set??
  g_object_get(G_OBJECT(container_set),
             ZMAPSTYLE_PROPERTY_ALIGNMENT_UNMARKED_COLINEAR, &(container_set->settings.bump_unmarked),
             NULL);

  if(container_set->settings.bump_unmarked &&
            container_set->settings.bump_unmarked != ZMAPSTYLE_COLDISPLAY_SHOW)
      x = FALSE;
#else
  ZMapStyleColumnDisplayState state  = zMapStyleGetUnmarked(container_set->style);
  if(state && state != ZMAPSTYLE_COLDISPLAY_SHOW)
      x = FALSE;
#endif

  return x;
}




#if MH17_NO_STYLE_BUMP

/*!
 * \brief reset the bump modes
 *
 * \param The container to reset bump modes for.
 *
 * \return The bump mode of the container.
 */

ZMapStyleBumpMode zmapWindowContainerFeatureSetResetBumpModes(ZMapWindowContainerFeatureSet container_set)
{
  ZMapStyleBumpMode mode = ZMAPBUMP_UNBUMP;
#if MH17_NO_MORE_STYLE_TABLES
  ItemFeatureValueDataStruct value_data = {NULL};
  GValue value = {0};

  g_value_init(&value, G_TYPE_UINT);

  value_data.spec_name = ZMAPSTYLE_PROPERTY_BUMP_MODE;
  value_data.gvalue    = &value;
  value_data.param_id  = ITEM_FEATURE_SET_BUMP_MODE;

  g_hash_table_foreach(container_set->style_table, reset_bump_mode_cb, &value_data);

  mode = g_value_get_uint(&value);
#else
  mode = container_set->bump_mode;
#endif
  return mode;
}
#endif


/*!
 * \brief Access the join aligns mode of a column.
 *
 * \param The container to interrogate.
 * \param an address to store the threshold value.
 *
 * \return The join aligns mode of the container.
 */

gboolean zmapWindowContainerFeatureSetJoinAligns(ZMapWindowContainerFeatureSet container_set, unsigned int *threshold)
{
  gboolean result = FALSE;
  unsigned int tmp = 0;

  if(threshold)
    {
#if MH17_NO_MORE_STYLE_TABLES
      g_object_get(G_OBJECT(container_set),
		   ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_ALIGN, &tmp,
		   NULL);
#else
      tmp = zMapStyleGetWithinAlignError(container_set->style);
#endif
      if(tmp != 0)
	{
	  *threshold = tmp;
	  result = TRUE;
	}
    }

  return result;
}

#if MH17_NO_DEFERRED

/*!
 * \brief Access the deferred mode of a column.
 *
 * \param The container to interrogate.
 *
 * \return whether the column is deferred.
 */

gboolean zmapWindowContainerFeatureSetGetDeferred(ZMapWindowContainerFeatureSet container_set)
{
  gboolean is_deferred = FALSE;

  /* Not cached! */
  g_object_get(G_OBJECT(container_set),
	       ZMAPSTYLE_PROPERTY_DEFERRED, &is_deferred,
	       NULL);

  return is_deferred;
}
#endif

#if DOCUMENTED_AS_NOT_USED

static void queueRemoveFromList(gpointer queue_data, gpointer user_data);
static void listRemoveFromList(gpointer list_data, gpointer user_data);
static void zmap_g_queue_replace(GQueue *queue, gpointer old, gpointer new);

/*!
 * \brief Remove a feature?
 *
 * As we keep a list of the item we need to delete them at times.  This is actually _not_
 * used ATM (Apr 2008) as the reason it was written turned out to have a better solution
 * RT# 63281.  Anyway the code is here if needed.
 *
 * \param container
 * \param feature to remove?
 *
 * \return nothing
 */


static void listRemoveFromList(gpointer list_data, gpointer user_data)
{
  ListFeature list_feature = (ListFeature)user_data;
  ZMapFeature item_feature;
  ZMapWindowCanvasItem canvas_item;

  zMapAssert(FOO_IS_CANVAS_ITEM(list_data));

  canvas_item  = ZMAP_CANVAS_ITEM(list_data);
  item_feature = zMapWindowCanvasItemGetFeature(FOO_CANVAS_ITEM(canvas_item)) ;
  zMapAssert(item_feature);

  if(item_feature == list_feature->feature)
    list_feature->list = g_list_remove(list_feature->list, canvas_item);

  return ;
}

static void zmap_g_queue_replace(GQueue *queue, gpointer old, gpointer new)
{
  int length, index;

  if((length = g_queue_get_length(queue)))
    {
      if((index = g_queue_index(queue, old)) != -1)
      {
        gpointer popped = g_queue_pop_nth(queue, index);
        zMapAssert(popped == old);
        g_queue_push_nth(queue, new, index);
      }
    }
  else
    g_queue_push_head(queue, new);

  return ;
}


static void queueRemoveFromList(gpointer queue_data, gpointer user_data)
{
  GList *item_list = (GList *)queue_data;
  QueueFeature queue_feature = (QueueFeature)user_data;
  ListFeatureStruct list_feature;

  list_feature.list    = item_list;
  list_feature.feature = (ZMapFeature)queue_feature->feature;

  g_list_foreach(item_list, listRemoveFromList, &list_feature);

  if(list_feature.list != item_list)
    zmap_g_queue_replace(queue_feature->queue, item_list, list_feature.list);

  return;
}


void zmapWindowContainerFeatureSetFeatureRemove(ZMapWindowContainerFeatureSet item_feature_set,
						ZMapFeature feature)
{
  QueueFeatureStruct queue_feature;

  queue_feature.queue   = item_feature_set->user_hidden_stack;
  queue_feature.feature = feature;
  /* user_hidden_stack is a copy of the list in focus. We need to
   * remove items when they get destroyed */
  g_queue_foreach(queue_feature.queue, queueRemoveFromList, &queue_feature);

  return ;
}
#endif

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
 * \brief Sort all the features in a columns.
 *
 * \param container  The container to be sorted
 * \param direction  The order in which to sort them.
 *
 * \return nothing
 */

void zmapWindowContainerFeatureSetSortFeatures(ZMapWindowContainerFeatureSet container_set,
					       gint direction)
{
  ZMapWindowContainerFeatures container_features;
  FooCanvasGroup *features_group;

  if(container_set->sorted == FALSE)
    {
      if((container_features = zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)container_set)))
	{
	  GCompareFunc compare_func;

	  features_group = (FooCanvasGroup *)container_features;

	  if(direction == 0) /* MH17: NOTE that direction is always 0 */
	    compare_func = comparePosition;
	  else
	    compare_func = comparePositionRev;

	  zMap_foo_canvas_sort_items(features_group, compare_func);
	}

      container_set->sorted = TRUE;
    }

  return ;
}


/*
      take a focus item from the front of the container/foo canvas group item_list
      and move it to where it should be when sorted
      we skip over the first n items as these are the focus itmes and out of order
      subsequent ones are sorted

      this has to be in this file to use the same compare functions
*/
gboolean zmapWindowContainerFeatureSetItemLowerToMiddle(ZMapWindowContainerFeatureSet container_set,
            ZMapWindowCanvasItem item,int n_focus,int direction)
{
  ZMapWindowContainerFeatures container_features;
  FooCanvasGroup *features_group;
  ZMapWindowCanvasItem list_item;
  GList *item_list,*prev_list= NULL,*next_list,*my_list = NULL;

  if(!(container_set->sorted))
  {
      /* we expect it should ahve been but extra data could have arrived ?? */
      /* this could mess up the focus list */
      zmapWindowContainerFeatureSetSortFeatures(container_set, direction);
      return(FALSE);
  }
  else

  if((container_features = zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)container_set)))
    {
      GCompareFunc compare_func;

      features_group = (FooCanvasGroup *) container_features;

      if(direction == 0)    /* MH17: NOTE that direction is always 0 */
        compare_func = comparePosition;
      else
        compare_func = comparePositionRev;

      /* initialise and skip the focus items */
      for(item_list = features_group->item_list;n_focus-- && item_list;item_list = item_list->next)
      {
            if(item_list->data == item)
                  my_list = item_list;
            prev_list = item_list;
      }
      zMapAssert(my_list);

      /* now find the item before the one that should go after this one */
      for(;item_list;item_list = item_list->next)
      {
            list_item = ZMAP_CANVAS_ITEM(item_list->data);
            /* unfortunately even though merge sort is stable
             * at this point we have no way of knowing where this feature was
             * to be rock solid we'd have to implement before pointers
             * and handle all the nasty overlapping cases
             * so we just choose the left most place
             */
            if(compare_func(item,list_item) <= 0)
                  break;

            prev_list = item_list;
      }

      /* now move item to after the previous */
      /* there's a few case to consider but..
       * - only one item total
       * - one focus item that goes at the front of the list
       * - focus item goes in the middle
       * - focus item goes to the end
       */
      if(prev_list != my_list)
      {
            features_group->item_list = g_list_remove_link(features_group->item_list,my_list);

            next_list = prev_list->next;
            if(next_list)
            {
                  my_list->next = next_list;
                  next_list->prev = my_list;
            }
            if(prev_list != my_list)      /* only one focus item */
            {
                  my_list->prev = prev_list;
                  prev_list->next = my_list;
            }
      }
    }

  return TRUE ;
}

ZMapWindow zMapWindowContainerFeatureSetGetWindow(ZMapWindowContainerFeatureSet container_set)
{
      return container_set->window;
}




/* show or hide all the masked features in this column */
void zMapWindowContainerFeatureSetShowHideMaskedFeatures(ZMapWindowContainerFeatureSet container, gboolean set_colour)
{
  ZMapWindowContainerFeatures container_features ;
  ZMapStyleBumpMode bump_mode;      /* = container->settings.bump_mode; */
  gboolean show = !container->masked;

  bump_mode = zMapWindowContainerFeatureSetGetContainerBumpMode(container);

  if(bump_mode > ZMAPBUMP_UNBUMP)
    {
       zmapWindowColumnBump(FOO_CANVAS_ITEM(container),ZMAPBUMP_UNBUMP);
    }


  if ((container_features = zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)container)))
    {
      FooCanvasGroup *group ;
      GList *list,*del;
      gboolean delete = FALSE;

      group = FOO_CANVAS_GROUP(container_features) ;

      for(list = group->item_list;list;)
        {
            ZMapWindowCanvasItem item;
            ZMapFeature feature;
            ZMapFeatureTypeStyle style;

            item = ZMAP_CANVAS_ITEM(list->data);
            feature = item->feature;
            style = feature->style;
            del = list;
            list = list->next;
            delete = FALSE;

            if(style->mode == ZMAPSTYLE_MODE_ALIGNMENT && feature->feature.homol.flags.masked)
              {
                if(set_colour)
                {
                      GdkColor *fill,*outline;

                      delete = !zMapWindowGetMaskedColour(container->window,&fill,&outline);

                      if(!delete)
                        {
                          zMapWindowCanvasItemSetIntervalColours(FOO_CANVAS_ITEM(item),
                              ZMAPSTYLE_COLOURTYPE_NORMAL,  /* SELECTED used to re-order this list... */
                              fill,outline);
                        }
                }

                if(delete)
                  {
                    /* if colours are not defined we should remove the item from the canvas here */
                    group->item_list = g_list_delete_link(group->item_list,del);
                    gtk_object_destroy(GTK_OBJECT(item)) ;
                    feature->feature.homol.flags.displayed = FALSE;
                  }
                else if(show)
                  {
                    foo_canvas_item_show(FOO_CANVAS_ITEM(item));
                    feature->feature.homol.flags.displayed = TRUE;
                  }
                else
                  {
                    foo_canvas_item_hide(FOO_CANVAS_ITEM(item));
                    feature->feature.homol.flags.displayed = FALSE;
                  }
              }
        }
    }
            /* if we are adding/ removing features we may need to compress and/or rebump */
    if(bump_mode > ZMAPBUMP_UNBUMP)
    {
      zmapWindowColumnBump(FOO_CANVAS_ITEM(container),bump_mode);
    }
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

ZMapStyleBumpMode zMapWindowContainerFeatureSetGetContainerBumpMode(ZMapWindowContainerFeatureSet container_set)
{
  ZMapStyleBumpMode mode = container_set->bump_mode;

#if MH17_NO_MORE_STYLE_TABLES
  if(mode == ZMAPBUMP_INVALID)
      mode = zmapWindowContainerFeatureSetGetBumpMode(container_set) ;      /* from style tables */
#endif
  return(mode);
}


/*!
 * \brief Unset the sorted flag for the featureset to force a re-sort on display eg after adding a feature
 *
 * \param container  The container to be sorted
 *
 * \return nothing
 */

void zMapWindowContainerFeatureSetMarkUnsorted(ZMapWindowContainerFeatureSet container_set)
{
      container_set->sorted = FALSE;
}


/*!
 * \brief Time to free the memory associated with the ZMapWindowContainerFeatureSet.
 *
 * \code container = zmapWindowContainerFeatureSetDestroy(container);
 *
 * \param container  The container to be free'd
 *
 * \return The container that has been free'd. i.e. NULL
 */

ZMapWindowContainerFeatureSet zmapWindowContainerFeatureSetDestroy(ZMapWindowContainerFeatureSet item_feature_set)
{
  g_object_unref(G_OBJECT(item_feature_set));

  if(item_feature_set->featuresets)
  {
      g_list_free(item_feature_set->featuresets);
      item_feature_set->featuresets = NULL;
  }
  item_feature_set = NULL;

  return item_feature_set;
}


#if MH17_NO_MORE_STYLE_TABLES
/* This function is written the wrong way round.  It should be
 * re-written, along with extract_value_from_style_table so that
 * this function is part of utils and extract_value_from_style_table
 * calls it.  It wouldn't need to be here then! */
#warning function needs re-writing (zmapWindowStyleListGetSetting)
gboolean zmapWindowStyleListGetSetting(GList *list_of_styles,
				       char *setting_name,
				       GValue *value_in_out)
{
  GList *list;
  gboolean result = FALSE;

  if(value_in_out && (list = g_list_first(list_of_styles)))
    {
      ItemFeatureValueDataStruct value_data = {};
      GObjectClass *object_class;
      GParamSpec *param_spec;

      object_class = g_type_class_peek(ZMAP_TYPE_CONTAINER_FEATURESET);

      if((param_spec = g_object_class_find_property(object_class, setting_name)))
	{
	  value_data.param_id  = param_spec->param_id;
	  value_data.spec_name = setting_name;
	  value_data.gvalue    = value_in_out;

	  result = TRUE;

	  do
	    {
	      ZMapFeatureTypeStyle style;
	      GQuark unique_id;

	      style = ZMAP_FEATURE_STYLE(list->data);
	      unique_id = zMapStyleGetUniqueID(style);
	      extract_value_from_style_table(GINT_TO_POINTER(unique_id), (gpointer) style, &value_data);
	    }
	  while((list = g_list_next(list)));
	}
    }

  return result;
}
#endif


/*
 *  OBJECT CODE
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

#if MH17_NO_MORE_STYLE_TABLES
  g_object_class_install_property(gobject_class, ITEM_FEATURE_SET_STYLE_TABLE,
                          g_param_spec_pointer("style-table", "ZMapStyle table",
                                           "GHashTable of ZMap styles for column.",
                                           ZMAP_PARAM_STATIC_RW));
  /* width */
  g_object_class_install_property(gobject_class,
				  ITEM_FEATURE_SET_WIDTH,
				  g_param_spec_double(ZMAPSTYLE_PROPERTY_WIDTH,
						      ZMAPSTYLE_PROPERTY_WIDTH,
						      "The minimum width the column should be displayed at.",
						      0.0, 32000.00, 16.0,
						      ZMAP_PARAM_STATIC_RO));
  /* Bump spacing */
  g_object_class_install_property(gobject_class,
				  ITEM_FEATURE_SET_BUMP_SPACING,
				  g_param_spec_double(ZMAPSTYLE_PROPERTY_BUMP_SPACING,
						      ZMAPSTYLE_PROPERTY_BUMP_SPACING,
						      "The x coord spacing between features when bumping.",
						      0.0, 32000.00, 1.0,
						      ZMAP_PARAM_STATIC_RO));

  /* bump default */
  g_object_class_install_property(gobject_class,
                          ITEM_FEATURE_SET_DEFAULT_BUMP_MODE,
                          g_param_spec_uint(ZMAPSTYLE_PROPERTY_DEFAULT_BUMP_MODE,
                                        ZMAPSTYLE_PROPERTY_DEFAULT_BUMP_MODE,
                                        "The Default Bump Mode",
                                        ZMAPBUMP_INVALID,
                                        ZMAPBUMP_END,
                                        ZMAPBUMP_INVALID,
                                        ZMAP_PARAM_STATIC_RO));
  /* bump unmarked */
  g_object_class_install_property(gobject_class,
                          ITEM_FEATURE_SET_BUMP_UNMARKED,
                          g_param_spec_uint(ZMAPSTYLE_PROPERTY_ALIGNMENT_UNMARKED_COLINEAR,
                                        ZMAPSTYLE_PROPERTY_ALIGNMENT_UNMARKED_COLINEAR,
                                        "[ hide | show_hide | show ]",
                                        ZMAPSTYLE_COLDISPLAY_INVALID,
                                        ZMAPSTYLE_COLDISPLAY_SHOW,
                                        ZMAPSTYLE_COLDISPLAY_INVALID,
                                        ZMAP_PARAM_STATIC_RO));
  /* join aligns */
  g_object_class_install_property(gobject_class,
                          ITEM_FEATURE_SET_JOIN_ALIGNS,
                          g_param_spec_uint(ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_ALIGN,
                                        ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_ALIGN,
                                        "match threshold",
                                        0, 1000, 0,
                                        ZMAP_PARAM_STATIC_RO));
  /* Frame mode */
  g_object_class_install_property(gobject_class,
                          ITEM_FEATURE_SET_FRAME_MODE,
                          g_param_spec_uint(ZMAPSTYLE_PROPERTY_FRAME_MODE,
                                        "3 frame display mode",
                                        "Defines frame sensitive display in 3 frame mode.",
                                        ZMAPSTYLE_3_FRAME_INVALID,
                                        ZMAPSTYLE_3_FRAME_ONLY_1,
                                        ZMAPSTYLE_3_FRAME_INVALID,
                                        ZMAP_PARAM_STATIC_RO));

  /* Strand specific */
  g_object_class_install_property(gobject_class,
                          ITEM_FEATURE_SET_STRAND_SPECIFIC,
                          g_param_spec_boolean(ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC,
                                           "Strand specific",
                                           "Defines strand sensitive display.",
                                           TRUE, ZMAP_PARAM_STATIC_RO));

  /* Show reverse strand */
  g_object_class_install_property(gobject_class,
                          ITEM_FEATURE_SET_SHOW_REVERSE_STRAND,
                          g_param_spec_boolean(ZMAPSTYLE_PROPERTY_SHOW_REVERSE_STRAND,
                                           "Show reverse strand",
                                           "Defines whether reverse strand is displayed.",
                                           TRUE, ZMAP_PARAM_STATIC_RO));

  /* Show when empty */
  g_object_class_install_property(gobject_class,
                          ITEM_FEATURE_SET_SHOW_WHEN_EMPTY,
                          g_param_spec_boolean(ZMAPSTYLE_PROPERTY_SHOW_WHEN_EMPTY,
                                           ZMAPSTYLE_PROPERTY_SHOW_WHEN_EMPTY,
                                           "Does the Style get shown when empty",
                                           TRUE, ZMAP_PARAM_STATIC_RO));


  /* Deferred */
  g_object_class_install_property(gobject_class,
                          ITEM_FEATURE_SET_DEFERRED,
                          g_param_spec_boolean(ZMAPSTYLE_PROPERTY_DEFERRED,
                                           ZMAPSTYLE_PROPERTY_DEFERRED,
                                           "Is this deferred",
                                           FALSE, ZMAP_PARAM_STATIC_RO));
#endif

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

#if MH17_NO_MORE_STYLE_TABLES
    case ITEM_FEATURE_SET_STYLE_TABLE:
      g_value_set_pointer(value, container_set->style_table) ;
      break ;
    case ITEM_FEATURE_SET_BUMP_SPACING:
    case ITEM_FEATURE_SET_WIDTH:
    case ITEM_FEATURE_SET_DEFAULT_BUMP_MODE:
    case ITEM_FEATURE_SET_BUMP_UNMARKED:
    case ITEM_FEATURE_SET_FRAME_MODE:
    case ITEM_FEATURE_SET_SHOW_WHEN_EMPTY:
    case ITEM_FEATURE_SET_DEFERRED:
    case ITEM_FEATURE_SET_STRAND_SPECIFIC:
    case ITEM_FEATURE_SET_SHOW_REVERSE_STRAND:
    case ITEM_FEATURE_SET_JOIN_ALIGNS:
      {
	ItemFeatureValueDataStruct value_data = {NULL};

	value_data.spec_name = g_param_spec_get_name(pspec);
	value_data.gvalue    = value;
	value_data.param_id  = param_id;

	g_hash_table_foreach(container_set->style_table, extract_value_from_style_table, &value_data);
      }
      break;
#endif
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

#if MH17_NO_MORE_STYLE_TABLES
  if (container_set->style_table)
    {
      zmapWindowStyleTableDestroy(container_set->style_table);
      container_set->style_table = NULL;
    }
#endif

  if (container_set->user_hidden_stack)
    {
      if(!g_queue_is_empty(container_set->user_hidden_stack))
	g_queue_foreach(container_set->user_hidden_stack, removeList, NULL) ;

      g_queue_free(container_set->user_hidden_stack);

      container_set->user_hidden_stack = NULL;
    }

  zMapWindowContainerFeatureSetRemoveSubFeatures(container_set) ;

  {
    char *col_name ;

    col_name = (char *) g_quark_to_string(zmapWindowContainerFeatureSetColumnDisplayName(container_set)) ;
    if (g_ascii_strcasecmp("3 frame translation", col_name) !=0)
      {
	if (gtkobject_class->destroy)
	  (gtkobject_class->destroy)(gtkobject);
      }



  }

  return ;
}



/*
 *                               INTERNAL
 */

/* simple function to compare feature positions. */
static gint comparePosition(gconstpointer a, gconstpointer b)
{
  ZMapWindowCanvasItem item1 = NULL, item2 = NULL;
  ZMapFeature feature1, feature2 ;
  gint result = -1 ;

  zMapAssert(ZMAP_IS_CANVAS_ITEM(a));
  zMapAssert(ZMAP_IS_CANVAS_ITEM(b));

  item1 = (ZMapWindowCanvasItem)a;
  item2 = (ZMapWindowCanvasItem)b;

  feature1 = item1->feature;
  feature2 = item2->feature;

  zMapAssert(zMapFeatureIsValid((ZMapFeatureAny)feature1)) ;
  zMapAssert(zMapFeatureIsValid((ZMapFeatureAny)feature2)) ;


  if (feature1->x1 > feature2->x1)
    result = 1 ;
  else if (feature1->x1 == feature2->x1)
    {
      int diff1, diff2 ;

      diff1 = feature1->x2 - feature1->x1 ;
      diff2 = feature2->x2 - feature2->x1 ;

      if (diff1 < diff2)
	result = 1 ;
      else if (diff1 == diff2)
	result = 0 ;
    }

  return result ;
}

/* opposite order of comparePosition */
static gint comparePositionRev(gconstpointer a, gconstpointer b)
{
  gint result = 1;

  result = comparePosition(a, b) * -1;

  return result;
}


#if MH17_NO_MORE_STYLE_TABLES

/* helps out the get_property function as that needs to foreach a hash. */
static void extract_value_from_style_table(gpointer key, gpointer value, gpointer user_data)
{
  ItemFeatureValueData value_data;
  ZMapFeatureTypeStyle style;
  GQuark style_id;

  style_id = GPOINTER_TO_INT(key);
  style    = ZMAP_FEATURE_STYLE(value);
  value_data = (ItemFeatureValueData)user_data;

  switch(value_data->param_id)
    {
    case ITEM_FEATURE_SET_BUMP_SPACING:
    case ITEM_FEATURE_SET_WIDTH:
      {
	double tmp_width, style_width;

	tmp_width = g_value_get_double(value_data->gvalue);

	g_object_get(G_OBJECT(style),
		     value_data->spec_name, &style_width,
		     NULL);

	if(style_width > tmp_width)
	  g_value_set_double(value_data->gvalue, style_width);

	break;
      }

    case ITEM_FEATURE_SET_STRAND_SPECIFIC:
    case ITEM_FEATURE_SET_SHOW_REVERSE_STRAND:
    case ITEM_FEATURE_SET_SHOW_WHEN_EMPTY:
    case ITEM_FEATURE_SET_DEFERRED:
      {
	gboolean style_version = FALSE, current;

	current = g_value_get_boolean(value_data->gvalue);

	if(!current)
	  {
	    g_object_get(G_OBJECT(style),
			 value_data->spec_name, &style_version,
			 NULL);

	    if(style_version)
	      g_value_set_boolean(value_data->gvalue, style_version);

	  }

	break;
      }

    case ITEM_FEATURE_SET_FRAME_MODE:
    case ITEM_FEATURE_SET_BUMP_MODE:
    case ITEM_FEATURE_SET_DEFAULT_BUMP_MODE:
      {
	guint style_version = 0, current;

	current = g_value_get_uint(value_data->gvalue);

	if (!current)
	  {
	    g_object_get(G_OBJECT(style),
			 value_data->spec_name, &style_version,
			 NULL);

	    if(style_version)
	      g_value_set_uint(value_data->gvalue, style_version);
	  }

	break;
      }

    case ITEM_FEATURE_SET_BUMP_UNMARKED:
        if(zMapStyleGetMode(style) != ZMAPSTYLE_MODE_ALIGNMENT)
          break;
    case ITEM_FEATURE_SET_VISIBLE:
      {
	guint style_version = 0, current;

	current = g_value_get_uint(value_data->gvalue);

	if (!current)
	  {
	    g_object_get(G_OBJECT(style),
			 value_data->spec_name, &style_version,
			 NULL);

	    if (style_version)
	      g_value_set_uint(value_data->gvalue, style_version);
	  }

	break;
      }


    case ITEM_FEATURE_SET_JOIN_ALIGNS:
      {
	guint style_version = 0, current;

	current = g_value_get_uint(value_data->gvalue);

	if(!current)
	  {
//	    if(zMapStyleGetJoinAligns(style, &style_version))
            style_version = zMapStyleGetWithinAlignError(style);
            if(style_version) // if it's set we only care if it's not zero
	      {
		g_value_set_uint(value_data->gvalue, style_version);
	      }
	  }
      }
      break;
    case ITEM_FEATURE__MIN_MAG:
      {
	double all_min_mag, this_min_mag;

	all_min_mag  = this_min_mag = g_value_get_double(value_data->gvalue);

	this_min_mag = zMapStyleGetMinMag(style);

	/* We want the minimum across all styles here. */
	if(all_min_mag != this_min_mag)
	  {
	    if((this_min_mag == 0.0) || (this_min_mag != 0.0 && this_min_mag > all_min_mag))
	      {
		this_min_mag = all_min_mag;
	      }
	  }

	g_value_set_double(value_data->gvalue, this_min_mag);
      }
      break;
    case ITEM_FEATURE__MAX_MAG:
      {
	double all_max_mag, this_max_mag;

	all_max_mag  = this_max_mag = g_value_get_double(value_data->gvalue);

	this_max_mag = zMapStyleGetMaxMag(style);

	/* We want the maximum across all styles here. */
	if(all_max_mag != this_max_mag)
	  {
	    if((this_max_mag == 0.0) || (this_max_mag != 0.0 && this_max_mag < all_max_mag))
	      {
		this_max_mag = all_max_mag;
	      }
	  }

	g_value_set_double(value_data->gvalue, this_max_mag);
      }
      break;
    case ITEM_FEATURE__DIVIDE:
      /* This isn't valid. */
    default:
      break;
    }

  return ;
}

/* helps out functions that need to set settings in styles. can be in a hash_foreach */
static void value_to_each_style_in_table(gpointer key, gpointer value, gpointer user_data)
{
  ItemFeatureValueData value_data;
  ZMapFeatureTypeStyle style;
  GQuark style_id;

  style_id = GPOINTER_TO_INT(key);
  style    = ZMAP_FEATURE_STYLE(value);
  value_data = (ItemFeatureValueData)user_data;

  if(debug_table_ids_G)
    {
      zMapLogMessage("style_id = '%s'", g_quark_to_string(style_id));
    }

  switch(value_data->param_id)
    {
    case ITEM_FEATURE_SET_FRAME_MODE:
    case ITEM_FEATURE_SET_VISIBLE:
    case ITEM_FEATURE_SET_BUMP_MODE:
    case ITEM_FEATURE_SET_DEFAULT_BUMP_MODE:
      {
	guint new = g_value_get_uint(value_data->gvalue);

	g_object_set(G_OBJECT(style),
		     value_data->spec_name, new,
		     NULL);
      }
      break;
    default:
      break;
    }

  return ;
}


/* helps out resetting the bump more.  hash foreach function */
static void reset_bump_mode_cb(gpointer key, gpointer value, gpointer user_data)
{
  ZMapFeatureTypeStyle style;

  style = ZMAP_FEATURE_STYLE(value);

  zMapStyleResetBumpMode(style);

  extract_value_from_style_table(key, value, user_data);

  return ;
}
#endif


static void removeList(gpointer data, gpointer user_data_unused)
{
  GList *user_hidden_items = (GList *)data ;

  g_list_free(user_hidden_items) ;

  return ;
}




