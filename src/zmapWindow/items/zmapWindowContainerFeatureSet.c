/*  File: zmapWindowContainerFeatureSet.c
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
 * Last edited: Jun  1 18:02 2009 (rds)
 * Created: Mon Jul 30 13:09:33 2007 (rds)
 * CVS info:   $Id: zmapWindowContainerFeatureSet.c,v 1.1 2009-06-02 11:20:24 rds Exp $
 *-------------------------------------------------------------------
 */
#include <string.h>		/* memset */

#include <ZMap/zmapUtils.h>
#include <zmapWindowContainerGroup_I.h>
#include <zmapWindowContainerFeatureSet_I.h>
#include <zmapWindowContainer.h>

enum
  {
    ITEM_FEATURE_SET_0,		/* zero == invalid prop value */
    ITEM_FEATURE_SET_WIDTH,
    ITEM_FEATURE_SET_VISIBLE,
    ITEM_FEATURE_SET_BUMP_MODE,
    ITEM_FEATURE_SET_DEFAULT_BUMP_MODE,
    ITEM_FEATURE_SET_FRAME_MODE,
    ITEM_FEATURE_SET_SHOW_WHEN_EMPTY,
    ITEM_FEATURE_SET_DEFERRED,
    ITEM_FEATURE_SET_STRAND_SPECIFIC,
    ITEM_FEATURE_SET_BUMP_SPACING,
    ITEM_FEATURE_SET_JOIN_ALIGNS,
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
static void zmap_window_item_feature_set_dispose     (GObject *object);
static void zmap_window_item_feature_set_finalize    (GObject *object);


static void extract_value_from_style_table(gpointer key, gpointer value, gpointer user_data);
static void reset_bump_mode_cb(gpointer key, gpointer value, gpointer user_data);
static void queueRemoveFromList(gpointer queue_data, gpointer user_data);
static void listRemoveFromList(gpointer list_data, gpointer user_data);
static void removeList(gpointer data, gpointer user_data_unused) ;
static void zmap_g_queue_replace(GQueue *queue, gpointer old, gpointer new);


static GObjectClass *parent_class_G = NULL;


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


ZMapWindowContainerFeatureSet zmapWindowContainerFeatureSetAugment(ZMapWindowContainerFeatureSet container_set,
								   ZMapWindow window,
								   GQuark     feature_set_unique_id,
								   GQuark     feature_set_original_id, /* unused! */
								   GList     *style_list,
								   ZMapStrand strand,
								   ZMapFrame  frame)
{
  g_return_val_if_fail(feature_set_unique_id != 0, container_set);

  if(ZMAP_IS_CONTAINER_FEATURESET(container_set))
    {
      GList *list;

      container_set->window    = window;
      container_set->strand    = strand;
      container_set->frame     = frame;
      container_set->unique_id = feature_set_unique_id;

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


      zmapWindowContainerSetVisibility((FooCanvasGroup *)container_set, FALSE);
    }

  return container_set;
}

gboolean zmapWindowContainerFeatureSetAttachFeatureSet(ZMapWindowContainerFeatureSet container_set,
						       ZMapFeatureSet feature_set_to_attach)
{
  gboolean status = FALSE;

  if(feature_set_to_attach && !container_set->settings.has_feature_set)
    {

      ZMapWindowContainerGroup container_group;

      container_group              = ZMAP_CONTAINER_GROUP(container_set);
      container_group->feature_any = (ZMapFeatureAny)feature_set_to_attach;

      container_set->settings.has_feature_set = status = TRUE;

#ifdef STATS_GO_IN_PARENT_OBJECT
      ZMapWindowStats stats = NULL;

      if((stats = zmapWindowStatsCreate((ZMapFeatureAny)feature_set_to_attach)))
	{
	  zmapWindowContainerSetData(container_set->column_container, ITEM_FEATURE_STATS, stats);
	  container_set->settings.has_stats = TRUE;
	}
#endif
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

ZMapFeatureSet zmapWindowContainerFeatureSetRecoverFeatureSet(ZMapWindowContainerFeatureSet container_set)
{
  ZMapFeatureSet feature_set = NULL;

  if(container_set->settings.has_feature_set)
    {
      feature_set = (ZMapFeatureSet)(ZMAP_CONTAINER_GROUP(container_set)->feature_any);

      if(!feature_set)
	{
	  g_warning("%s", "No Feature Set!");
	  container_set->settings.has_feature_set = FALSE;
	}
    }

  return feature_set;
}

ZMapWindowStats zmapWindowContainerFeatureSetRecoverStats(ZMapWindowContainerFeatureSet container_set)
{
  ZMapWindowStats stats = NULL;

#ifdef STATS_GO_IN_PARENT_OBJECT
  if(container_set->settings.has_stats)
    {
      stats = g_object_get_data(G_OBJECT(container_set->column_container), ITEM_FEATURE_STATS);

      if(!stats)
	{
	  g_warning("%s", "No Stats!");
	  container_set->settings.has_stats = FALSE;
	}
    }
#endif

  return stats;
}

/*! 
 * \brief Gets the style we must use to draw with, given a style
 */
ZMapFeatureTypeStyle zmapWindowContainerFeatureSetStyleFromStyle(ZMapWindowContainerFeatureSet container_set,
								 ZMapFeatureTypeStyle         style2copy)
{
  ZMapFeatureTypeStyle duplicated = NULL;

  g_return_val_if_fail(ZMAP_IS_CONTAINER_FEATURESET(container_set), duplicated);
  g_return_val_if_fail(ZMAP_IS_FEATURE_STYLE(style2copy), duplicated);

  if(!(duplicated = zmapWindowStyleTableFind(container_set->style_table, zMapStyleGetUniqueID(style2copy))))
    {
      int s = sizeof(container_set->lazy_loaded);

      duplicated = zmapWindowStyleTableAddCopy(container_set->style_table, style2copy);
      
      memset(&container_set->lazy_loaded, 0, s);
    }

  return duplicated;
}

ZMapFeatureTypeStyle zmapWindowContainerFeatureSetStyleFromID(ZMapWindowContainerFeatureSet container_set,
							 GQuark                       style_unique_id)
{
  ZMapFeatureTypeStyle duplicated = NULL;

  g_return_val_if_fail(ZMAP_IS_CONTAINER_FEATURESET(container_set), duplicated);

  if(!(duplicated = zmapWindowStyleTableFind(container_set->style_table, style_unique_id)))
    {
      zMapAssertNotReached();
    }

  return duplicated;
}

ZMapFeatureTypeStyle zmapWindowContainerFeatureSetGetStyle(ZMapWindowContainerFeatureSet container_set,
							   ZMapFeature                  feature)
{
  ZMapFeatureTypeStyle style = NULL;

  style = zmapWindowStyleTableFind(container_set->style_table, container_set->unique_id);

  return style;
}


/* Warning! This is dynamic and will pick the original id over unique id */
GQuark zmapWindowContainerFeatureSetColumnDisplayName(ZMapWindowContainerFeatureSet container_set)
{
  ZMapFeatureSet feature_set;
  GQuark display_id = 0;

  if((feature_set = zmapWindowContainerFeatureSetRecoverFeatureSet(container_set)))
    display_id = feature_set->original_id;
  else
    display_id = container_set->unique_id;

  return display_id;
}


ZMapStrand zmapWindowContainerFeatureSetGetStrand(ZMapWindowContainerFeatureSet container_set)
{
  ZMapStrand strand = ZMAPSTRAND_NONE;

  g_return_val_if_fail(ZMAP_IS_CONTAINER_FEATURESET(container_set), strand);

  strand = container_set->strand;

  return strand;
}

ZMapFrame  zmapWindowContainerFeatureSetGetFrame (ZMapWindowContainerFeatureSet container_set)
{
  ZMapFrame frame = ZMAPFRAME_NONE;

  g_return_val_if_fail(ZMAP_IS_CONTAINER_FEATURESET(container_set), frame);

  frame = container_set->frame;

  return frame;
}

double zmapWindowContainerFeatureSetGetWidth(ZMapWindowContainerFeatureSet container_set)
{
  double width = 0.0;

  g_object_get(G_OBJECT(container_set),
	       ZMAPSTYLE_PROPERTY_WIDTH, &width,
	       NULL);

  return width;
}

double zmapWindowContainerFeatureGetBumpSpacing(ZMapWindowContainerFeatureSet container_set)
{
  double spacing;

  g_object_get(G_OBJECT(container_set),
	       ZMAPSTYLE_PROPERTY_BUMP_SPACING, &spacing,
	       NULL);

  return spacing;
}

gboolean zmapWindowContainerFeatureSetGetMagValues(ZMapWindowContainerFeatureSet container_set, 
						   double *min_mag_out, double *max_mag_out)
{
  ZMapFeatureTypeStyle style = NULL;
  gboolean mag_sens = FALSE;

  if((style = zmapWindowStyleTableFind(container_set->style_table, container_set->unique_id)))
    {
      double min_mag;
      double max_mag;

      min_mag = zMapStyleGetMinMag(style) ;
      max_mag = zMapStyleGetMaxMag(style) ;

      if (min_mag != 0.0 || max_mag != 0.0)
	mag_sens = TRUE;

      if(min_mag_out)
	*min_mag_out = min_mag;
      if(max_mag_out)
	*max_mag_out = max_mag;
    }

  return mag_sens;
}

ZMapStyleColumnDisplayState zmapWindowContainerFeatureSetGetDisplay(ZMapWindowContainerFeatureSet container_set)
{
  ZMapStyleColumnDisplayState display = ZMAPSTYLE_COLDISPLAY_SHOW;

  g_object_get(G_OBJECT(container_set),
	       ZMAPSTYLE_PROPERTY_DISPLAY_MODE, &(container_set->settings.display_state),
	       NULL);

  display = container_set->settings.display_state;

  return display;
}

void zmapWindowContainerFeatureSetDisplay(ZMapWindowContainerFeatureSet container_set, ZMapStyleColumnDisplayState state)
{
  ZMapFeatureTypeStyle style;
  
  if((style = zmapWindowStyleTableFind(container_set->style_table, container_set->unique_id)))
    {
      g_object_set(G_OBJECT(style),
		   ZMAPSTYLE_PROPERTY_DISPLAY_MODE, state,
		   NULL);
    }

  return ;
}

gboolean zmapWindowContainerFeatureSetShowWhenEmpty(ZMapWindowContainerFeatureSet container_set)
{
  gboolean show = FALSE;

  if(!container_set->lazy_loaded.show_when_empty)
    {
      g_object_get(G_OBJECT(container_set),
		   ZMAPSTYLE_PROPERTY_SHOW_WHEN_EMPTY, &(container_set->settings.show_when_empty),
		   NULL);
      container_set->lazy_loaded.show_when_empty = 1;
    }
  
  show = container_set->settings.show_when_empty;

  return show;
}

ZMapStyle3FrameMode zmapWindowContainerFeatureSetGetFrameMode(ZMapWindowContainerFeatureSet container_set)
{
  ZMapStyle3FrameMode frame_mode = ZMAPSTYLE_3_FRAME_INVALID;

  g_return_val_if_fail(ZMAP_IS_CONTAINER_FEATURESET(container_set), frame_mode);

  if(!container_set->lazy_loaded.frame_mode)
    {
      g_object_get(G_OBJECT(container_set),
		   ZMAPSTYLE_PROPERTY_FRAME_MODE, &(container_set->settings.frame_mode),
		   NULL);
      //container_set->lazy_loaded.frame_mode = 1;
    }
  
  frame_mode = container_set->settings.frame_mode;

  return frame_mode;
}

gboolean zmapWindowContainerFeatureSetIsFrameSpecific(ZMapWindowContainerFeatureSet container_set,
						      ZMapStyle3FrameMode         *frame_mode_out)
{
  ZMapStyle3FrameMode frame_mode = ZMAPSTYLE_3_FRAME_INVALID;
  gboolean frame_specific = FALSE;
  
  g_return_val_if_fail(ZMAP_IS_CONTAINER_FEATURESET(container_set), FALSE);

  if(!container_set->lazy_loaded.frame_specific)
    {
      frame_mode = zmapWindowContainerFeatureSetGetFrameMode(container_set) ;
      
      //container_set->lazy_loaded.frame_specific = 1;
     
      if(frame_mode != ZMAPSTYLE_3_FRAME_NEVER)
	container_set->settings.frame_specific = TRUE;

      if(frame_mode == ZMAPSTYLE_3_FRAME_INVALID)
	{
	  zMapLogWarning("Frame mode for column %s is invalid.", g_quark_to_string(container_set->unique_id));
	  container_set->settings.frame_specific = FALSE;
	}
    }  

  frame_specific = container_set->settings.frame_specific;

  if(frame_mode_out)
    *frame_mode_out = frame_mode;

  return frame_specific;
}

gboolean zmapWindowContainerFeatureSetIsStrandSpecific(ZMapWindowContainerFeatureSet container_set)
{
  gboolean strand_specific = FALSE;

  g_object_get(G_OBJECT(container_set),
	       ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC, &(container_set->settings.strand_specific),
	       NULL);

  strand_specific = container_set->settings.strand_specific;

  return strand_specific;
}

ZMapStyleBumpMode zmapWindowContainerFeatureSetGetBumpMode(ZMapWindowContainerFeatureSet container_set)
{
  ZMapStyleBumpMode mode = ZMAPBUMP_UNBUMP;

  g_object_get(G_OBJECT(container_set),
	       ZMAPSTYLE_PROPERTY_BUMP_MODE, &(container_set->settings.bump_mode),
	       NULL);

  mode = container_set->settings.bump_mode;

  return mode;
}

ZMapStyleBumpMode zmapWindowContainerFeatureSetGetDefaultBumpMode(ZMapWindowContainerFeatureSet container_set)
{
  ZMapStyleBumpMode mode = ZMAPBUMP_UNBUMP;

  g_object_get(G_OBJECT(container_set),
	       ZMAPSTYLE_PROPERTY_DEFAULT_BUMP_MODE, &(container_set->settings.default_bump_mode),
	       NULL);

  mode = container_set->settings.default_bump_mode;

  return mode;
}

ZMapStyleBumpMode zmapWindowContainerFeatureSetResetBumpModes(ZMapWindowContainerFeatureSet container_set)
{
  ZMapStyleBumpMode mode = ZMAPBUMP_UNBUMP;
  ItemFeatureValueDataStruct value_data = {NULL};
  GValue value = {0};

  g_value_init(&value, G_TYPE_UINT);

  value_data.spec_name = ZMAPSTYLE_PROPERTY_BUMP_MODE;
  value_data.gvalue    = &value;
  value_data.param_id  = ITEM_FEATURE_SET_BUMP_MODE;

  g_hash_table_foreach(container_set->style_table, reset_bump_mode_cb, &value_data);

  mode = g_value_get_uint(&value);

  return mode;
}

gboolean zmapWindowContainerFeatureSetJoinAligns(ZMapWindowContainerFeatureSet container_set, unsigned int *threshold)
{
  gboolean result = FALSE;
  unsigned int tmp = 0;

  if(threshold)
    {
      g_object_get(G_OBJECT(container_set),
		   ZMAPSTYLE_PROPERTY_ALIGNMENT_BETWEEN_ERROR, &tmp,
		   NULL);

      if(tmp != 0)
	{
	  *threshold = tmp;
	  result = TRUE;
	}
    }

  return result;
}

gboolean zmapWindowContainerFeatureSetGetDeferred(ZMapWindowContainerFeatureSet container_set)
{
  gboolean is_deferred = FALSE;

  /* Not cached! */
  g_object_get(G_OBJECT(container_set),
	       ZMAPSTYLE_PROPERTY_DEFERRED, &is_deferred,
	       NULL);

  return is_deferred;
}

/* As we keep a list of the item we need to delete them at times.  This is actually _not_ 
 * used ATM (Apr 2008) as the reason it was written turned out to have a better solution
 * RT# 63281.  Anyway the code is here if needed.
 */
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

GList *zmapWindowContainerFeatureSetPopHiddenStack(ZMapWindowContainerFeatureSet container_set)
{
  GList *hidden_items = NULL;

  hidden_items = g_queue_pop_head(container_set->user_hidden_stack);

  return hidden_items;
}

void zmapWindowContainerFeatureSetPushHiddenStack(ZMapWindowContainerFeatureSet container_set,
						  GList *hidden_items_list)
{
  g_queue_push_head(container_set->user_hidden_stack, hidden_items_list) ;

  return ;
}

void zmapWindowContainerFeatureSetRemoveAllItems(ZMapWindowContainerFeatureSet container_set)
{
  ZMapWindowContainerFeatures container_features;

  if((container_features = zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)container_set)))
    {
      FooCanvasGroup *group;

      group = FOO_CANVAS_GROUP(container_features);

      zmapWindowContainerUtilsRemoveAllItems(group);
    }

  return ;
}

ZMapWindowContainerFeatureSet zmapWindowContainerFeatureSetDestroy(ZMapWindowContainerFeatureSet item_feature_set)
{
  g_object_unref(G_OBJECT(item_feature_set));

  item_feature_set = NULL;

  return item_feature_set;
}

/* This function is written the wrong way round.  It should be
 * re-written, along with extract_value_from_style_table so that
 * this function is part of utils and extract_value_from_style_table
 * calls it.  It wouldn't need to be here then! */
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
	      extract_value_from_style_table(GINT_TO_POINTER(unique_id), style, &value_data);
	    }
	  while((list = g_list_next(list)));
	}
    }

  return result;
}


/* Object code */
static void zmap_window_item_feature_set_class_init(ZMapWindowContainerFeatureSetClass container_set_class)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *)container_set_class;

  gobject_class->set_property = zmap_window_item_feature_set_set_property;
  gobject_class->get_property = zmap_window_item_feature_set_get_property;

  parent_class_G = g_type_class_peek_parent(container_set_class);

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
  /* display mode */
  g_object_class_install_property(gobject_class, 
				  ITEM_FEATURE_SET_VISIBLE,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_DISPLAY_MODE, 
						    ZMAPSTYLE_PROPERTY_DISPLAY_MODE,
						    "[ hide | show_hide | show ]",
						    ZMAPSTYLE_COLDISPLAY_INVALID,
						    ZMAPSTYLE_COLDISPLAY_SHOW,
						    ZMAPSTYLE_COLDISPLAY_INVALID,
						    ZMAP_PARAM_STATIC_RO));

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
  /* bump default */
  g_object_class_install_property(gobject_class,
				  ITEM_FEATURE_SET_JOIN_ALIGNS,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_ALIGNMENT_BETWEEN_ERROR, 
						    ZMAPSTYLE_PROPERTY_ALIGNMENT_BETWEEN_ERROR,
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


  gobject_class->dispose  = zmap_window_item_feature_set_dispose;
  gobject_class->finalize = zmap_window_item_feature_set_finalize;

  return ;
}

static void zmap_window_item_feature_set_init(ZMapWindowContainerFeatureSet container_set)
{

  container_set->style_table       = zmapWindowStyleTableCreate();
  container_set->user_hidden_stack = g_queue_new();

  return ;
}

static void zmap_window_item_feature_set_set_property(GObject      *gobject, 
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

static void zmap_window_item_feature_set_get_property(GObject    *gobject, 
						      guint       param_id, 
						      GValue     *value, 
						      GParamSpec *pspec)
{
  ZMapWindowContainerFeatureSet container_set;

  container_set = ZMAP_CONTAINER_FEATURESET(gobject);

  switch(param_id)
    {
    case ITEM_FEATURE_SET_BUMP_SPACING:
    case ITEM_FEATURE_SET_WIDTH:
    case ITEM_FEATURE_SET_VISIBLE:
    case ITEM_FEATURE_SET_BUMP_MODE:
    case ITEM_FEATURE_SET_DEFAULT_BUMP_MODE:
    case ITEM_FEATURE_SET_FRAME_MODE:
    case ITEM_FEATURE_SET_SHOW_WHEN_EMPTY:
    case ITEM_FEATURE_SET_DEFERRED:
    case ITEM_FEATURE_SET_STRAND_SPECIFIC:
    case ITEM_FEATURE_SET_JOIN_ALIGNS:
      {
	ItemFeatureValueDataStruct value_data = {NULL};

	value_data.spec_name = g_param_spec_get_name(pspec);
	value_data.gvalue    = value;
	value_data.param_id  = param_id;

	g_hash_table_foreach(container_set->style_table, extract_value_from_style_table, &value_data);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_window_item_feature_set_dispose(GObject *object)
{
  ZMapWindowContainerFeatureSet container_set;
  GObjectClass *gobject_class = G_OBJECT_CLASS(parent_class_G);

  container_set = ZMAP_CONTAINER_FEATURESET(object);

  if(container_set->style_table)
    {
      zmapWindowStyleTableDestroy(container_set->style_table);
      container_set->style_table = NULL;
    }

  if(container_set->user_hidden_stack)
    {
      if(!g_queue_is_empty(container_set->user_hidden_stack))
	g_queue_foreach(container_set->user_hidden_stack, removeList, NULL) ;

      g_queue_free(container_set->user_hidden_stack);

      container_set->user_hidden_stack = NULL;
    }

  if(gobject_class->dispose)
    (*gobject_class->dispose)(object);

  return ;
}

static void zmap_window_item_feature_set_finalize(GObject *object)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(parent_class_G);
  
  if(gobject_class->finalize)
    (*gobject_class->finalize)(object);

  return ;
}


/* INTERNAL */

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
      }
      break;
    case ITEM_FEATURE_SET_STRAND_SPECIFIC:
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
      }
      break;
    case ITEM_FEATURE_SET_FRAME_MODE:
    case ITEM_FEATURE_SET_VISIBLE:
    case ITEM_FEATURE_SET_BUMP_MODE:
    case ITEM_FEATURE_SET_DEFAULT_BUMP_MODE:
      {
	guint style_version = 0, current;

	current = g_value_get_uint(value_data->gvalue);

	if(!current)
	  {
	    g_object_get(G_OBJECT(style),
			 value_data->spec_name, &style_version,
			 NULL);
	    
	    if(style_version)
	      g_value_set_uint(value_data->gvalue, style_version);
	  }
      }
      break;
    case ITEM_FEATURE_SET_JOIN_ALIGNS:
      {
	guint style_version = 0, current;

	current = g_value_get_uint(value_data->gvalue);
	
	if(!current)
	  {
	    if(zMapStyleGetJoinAligns(style, &style_version))
	      {
		g_value_set_uint(value_data->gvalue, style_version);
	      }
	  }
      }
      break;
    default:
      break;
    }

  return ;
}

static void reset_bump_mode_cb(gpointer key, gpointer value, gpointer user_data)
{
  ZMapFeatureTypeStyle style;

  style = ZMAP_FEATURE_STYLE(value);

  zMapStyleResetBumpMode(style);

  extract_value_from_style_table(key, value, user_data);

  return ;
}

static void removeList(gpointer data, gpointer user_data_unused)
{
  GList *user_hidden_items = (GList *)data ;

  g_list_free(user_hidden_items) ;

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



static void listRemoveFromList(gpointer list_data, gpointer user_data)
{
  ListFeature list_feature = (ListFeature)user_data;
  ZMapFeature item_feature;
  FooCanvasItem *item;

  zMapAssert(FOO_IS_CANVAS_ITEM(list_data));

  item         = FOO_CANVAS_ITEM(list_data);
  item_feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);
  zMapAssert(item_feature);
  
  if(item_feature == list_feature->feature)
    list_feature->list = g_list_remove(list_feature->list, item);

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
