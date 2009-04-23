/*  File: zmapWindowItemFeatureSet.c
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
 * Last edited: Apr 23 09:19 2009 (rds)
 * Created: Mon Jul 30 13:09:33 2007 (rds)
 * CVS info:   $Id: zmapWindowItemFeatureSet.c,v 1.14 2009-04-23 08:50:07 rds Exp $
 *-------------------------------------------------------------------
 */
#include <string.h>		/* memset */

#include <ZMap/zmapUtils.h>
#include <zmapWindowItemFeatureSet_I.h>
#include <zmapWindowContainer.h>

enum
  {
    ITEM_FEATURE_SET_0,		/* zero == invalid prop value */
    ITEM_FEATURE_SET_WIDTH,
    ITEM_FEATURE_SET_VISIBLE,
    ITEM_FEATURE_SET_OVERLAP_MODE,
    ITEM_FEATURE_SET_DEFAULT_OVERLAP_MODE,
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

static void zmap_window_item_feature_set_class_init  (ZMapWindowItemFeatureSetDataClass set_data_class);
static void zmap_window_item_feature_set_init        (ZMapWindowItemFeatureSetData set_data);
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
static void reset_overlap_mode_cb(gpointer key, gpointer value, gpointer user_data);
static void queueRemoveFromList(gpointer queue_data, gpointer user_data);
static void listRemoveFromList(gpointer list_data, gpointer user_data);
static void removeList(gpointer data, gpointer user_data_unused) ;
static void zmap_g_queue_replace(GQueue *queue, gpointer old, gpointer new);


static GObjectClass *parent_class_G = NULL;

gboolean zmap_g_return_moan(char *file, int line)
{
  g_warning("Failed @ %s:%d", file, line);

  return FALSE;
}

GType zmapWindowItemFeatureSetGetType(void)
{
  static GType group_type = 0;
  
  if (!group_type) 
    {
      static const GTypeInfo group_info = 
	{
	  sizeof (zmapWindowItemFeatureSetDataClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_window_item_feature_set_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (zmapWindowItemFeatureSetData),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) zmap_window_item_feature_set_init
      };
    
    group_type = g_type_register_static (G_TYPE_OBJECT,
					 "zmapWindowItemFeatureSetData",
					 &group_info,
					 0);
  }
  
  return group_type;
}


ZMapWindowItemFeatureSetData zmapWindowItemFeatureSetCreate(ZMapWindow window,
							    FooCanvasGroup *column_container,
							    GQuark feature_set_unique_id,
							    GQuark feature_set_original_id, /* unused! */
							    GList *style_list,
                                                            ZMapStrand strand,
                                                            ZMapFrame frame)
{
  ZMapWindowItemFeatureSetData set_data = NULL;

  g_return_val_if_fail(window != NULL, set_data);
  g_return_val_if_fail(column_container != NULL, set_data);
  g_return_val_if_fail(feature_set_unique_id != 0, set_data);

  if((set_data = g_object_new(zmapWindowItemFeatureSetGetType(), NULL)))
    {
      GList *list;

      set_data->window = window;
      set_data->strand = strand;
      set_data->frame  = frame;
      /* set_data->style_id  = 0;	/* feature_set_original_id */
      set_data->unique_id = feature_set_unique_id;

      if((list = g_list_first(style_list)))
	{
	  do
	    {
	      ZMapFeatureTypeStyle style;

	      style = (ZMapFeatureTypeStyle)(list->data);

	      zmapWindowItemFeatureSetStyleFromStyle(set_data, style);
	    }
	  while((list = g_list_next(list)));
	}

      set_data->column_container = column_container;

      zmapWindowContainerSetVisibility(column_container, FALSE);

      zmapWindowContainerSetData(column_container, ITEM_FEATURE_SET_DATA, set_data);
    }

  return set_data;
}

void zmapWindowItemFeatureSetAttachFeatureSet(ZMapWindowItemFeatureSetData set_data,
					      ZMapFeatureSet feature_set_to_attach)
{

  if(feature_set_to_attach && !set_data->settings.has_feature_set)
    {
      ZMapWindowStats stats = NULL;

      zmapWindowContainerSetData(set_data->column_container, ITEM_FEATURE_DATA, feature_set_to_attach) ;
      set_data->settings.has_feature_set = TRUE;

      if((stats = zmapWindowStatsCreate((ZMapFeatureAny)feature_set_to_attach)))
	{
	  zmapWindowContainerSetData(set_data->column_container, ITEM_FEATURE_STATS, stats);
	  set_data->settings.has_stats = TRUE;
	}
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

  return ;
}

ZMapFeatureSet zmapWindowItemFeatureSetRecoverFeatureSet(ZMapWindowItemFeatureSetData set_data)
{
  ZMapFeatureSet feature_set = NULL;

  if(set_data->settings.has_feature_set)
    {
      feature_set = g_object_get_data(G_OBJECT(set_data->column_container), ITEM_FEATURE_DATA);

      if(!feature_set)
	{
	  g_warning("%s", "No Feature Set!");
	  set_data->settings.has_feature_set = FALSE;
	}
    }

  return feature_set;
}

ZMapWindowStats zmapWindowItemFeatureSetRecoverStats(ZMapWindowItemFeatureSetData set_data)
{
  ZMapWindowStats stats = NULL;

  if(set_data->settings.has_stats)
    {
      stats = g_object_get_data(G_OBJECT(set_data->column_container), ITEM_FEATURE_STATS);

      if(!stats)
	{
	  g_warning("%s", "No Stats!");
	  set_data->settings.has_stats = FALSE;
	}
    }

  return stats;
}

/*! 
 * \brief Gets the style we must use to draw with, given a style
 */
ZMapFeatureTypeStyle zmapWindowItemFeatureSetStyleFromStyle(ZMapWindowItemFeatureSetData set_data,
							    ZMapFeatureTypeStyle         style2copy)
{
  ZMapFeatureTypeStyle duplicated = NULL;

  g_return_val_if_fail(ZMAP_IS_WINDOW_ITEM_FEATURE_SET(set_data), duplicated);
  g_return_val_if_fail(ZMAP_IS_FEATURE_STYLE(style2copy), duplicated);

  if(!(duplicated = zmapWindowStyleTableFind(set_data->style_table, zMapStyleGetUniqueID(style2copy))))
    {
      int s = sizeof(set_data->lazy_loaded);

      duplicated = zmapWindowStyleTableAddCopy(set_data->style_table, style2copy);
      
      memset(&set_data->lazy_loaded, 0, s);
    }

  return duplicated;
}

ZMapFeatureTypeStyle zmapWindowItemFeatureSetStyleFromID(ZMapWindowItemFeatureSetData set_data,
							 GQuark                       style_unique_id)
{
  ZMapFeatureTypeStyle duplicated = NULL;

  g_return_val_if_fail(ZMAP_IS_WINDOW_ITEM_FEATURE_SET(set_data), duplicated);

  if(!(duplicated = zmapWindowStyleTableFind(set_data->style_table, style_unique_id)))
    {
      zMapAssertNotReached();
    }

  return duplicated;
}

/* Warning! This is dynamic and will pick the original id over unique id */
GQuark zmapWindowItemFeatureSetColumnDisplayName(ZMapWindowItemFeatureSetData set_data)
{
  ZMapFeatureSet feature_set;
  GQuark display_id = 0;

  if((feature_set = zmapWindowItemFeatureSetRecoverFeatureSet(set_data)))
    display_id = feature_set->original_id;
  else
    display_id = set_data->unique_id;

  return display_id;
}

ZMapWindow zmapWindowItemFeatureSetGetWindow(ZMapWindowItemFeatureSetData set_data)
{
  ZMapWindow window = NULL;

  g_return_val_if_fail(ZMAP_IS_WINDOW_ITEM_FEATURE_SET(set_data), window);

  window = set_data->window;

  return window;
}

ZMapStrand zmapWindowItemFeatureSetGetStrand(ZMapWindowItemFeatureSetData set_data)
{
  ZMapStrand strand = ZMAPSTRAND_NONE;

  g_return_val_if_fail(ZMAP_IS_WINDOW_ITEM_FEATURE_SET(set_data), strand);

  strand = set_data->strand;

  return strand;
}

ZMapFrame  zmapWindowItemFeatureSetGetFrame (ZMapWindowItemFeatureSetData set_data)
{
  ZMapFrame frame = ZMAPFRAME_NONE;

  g_return_val_if_fail(ZMAP_IS_WINDOW_ITEM_FEATURE_SET(set_data), frame);

  frame = set_data->frame;

  return frame;
}


ZMapFeatureTypeStyle zmapWindowItemFeatureSetGetStyle(ZMapWindowItemFeatureSetData set_data,
						      ZMapFeature                  feature)
{
  ZMapFeatureTypeStyle style = NULL;

  style = zmapWindowStyleTableFind(set_data->style_table, set_data->unique_id);

  return style;
}

double zmapWindowItemFeatureSetGetWidth(ZMapWindowItemFeatureSetData set_data)
{
  double width = 0.0;

  g_object_get(G_OBJECT(set_data),
	       ZMAPSTYLE_PROPERTY_WIDTH, &width,
	       NULL);

  return width;
}

double zmapWindowItemFeatureGetBumpSpacing(ZMapWindowItemFeatureSetData set_data)
{
  double spacing;

  g_object_get(G_OBJECT(set_data),
	       ZMAPSTYLE_PROPERTY_BUMP_SPACING, &spacing,
	       NULL);

  return spacing;
}

gboolean zmapWindowItemFeatureSetGetMagValues(ZMapWindowItemFeatureSetData set_data, 
					      double *min_mag_out, double *max_mag_out)
{
  ZMapFeatureTypeStyle style = NULL;
  gboolean mag_sens = FALSE;

  if((style = zmapWindowStyleTableFind(set_data->style_table, set_data->unique_id)))
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

ZMapStyleColumnDisplayState zmapWindowItemFeatureSetGetDisplay(ZMapWindowItemFeatureSetData set_data)
{
  ZMapStyleColumnDisplayState display = ZMAPSTYLE_COLDISPLAY_SHOW;

  if(!set_data->lazy_loaded.display_state)
    {
      g_object_get(G_OBJECT(set_data),
		   ZMAPSTYLE_PROPERTY_DISPLAY_MODE, &(set_data->settings.display_state),
		   NULL);
    }
  
  display = set_data->settings.display_state;

  return display;
}

void zmapWindowItemFeatureSetDisplay(ZMapWindowItemFeatureSetData set_data, ZMapStyleColumnDisplayState state)
{
  ZMapFeatureTypeStyle style;
  
  if((style = zmapWindowStyleTableFind(set_data->style_table, set_data->unique_id)))
    {
      g_object_set(G_OBJECT(style),
		   ZMAPSTYLE_PROPERTY_DISPLAY_MODE, state,
		   NULL);
    }

  return ;
}

gboolean zmapWindowItemFeatureSetShowWhenEmpty(ZMapWindowItemFeatureSetData set_data)
{
  gboolean show = FALSE;

  if(!set_data->lazy_loaded.show_when_empty)
    {
      g_object_get(G_OBJECT(set_data),
		   ZMAPSTYLE_PROPERTY_SHOW_WHEN_EMPTY, &(set_data->settings.show_when_empty),
		   NULL);
      set_data->lazy_loaded.show_when_empty = 1;
    }
  
  show = set_data->settings.show_when_empty;

  return show;
}

ZMapStyle3FrameMode zmapWindowItemFeatureSetGetFrameMode(ZMapWindowItemFeatureSetData set_data)
{
  ZMapStyle3FrameMode frame_mode = ZMAPSTYLE_3_FRAME_INVALID;

  g_return_val_if_fail(ZMAP_IS_WINDOW_ITEM_FEATURE_SET(set_data), frame_mode);

  if(!set_data->lazy_loaded.frame_mode)
    {
      g_object_get(G_OBJECT(set_data),
		   ZMAPSTYLE_PROPERTY_FRAME_MODE, &(set_data->settings.frame_mode),
		   NULL);
      //set_data->lazy_loaded.frame_mode = 1;
    }
  
  frame_mode = set_data->settings.frame_mode;

  return frame_mode;
}

gboolean zmapWindowItemFeatureSetIsFrameSpecific(ZMapWindowItemFeatureSetData set_data,
						 ZMapStyle3FrameMode         *frame_mode_out)
{
  ZMapStyle3FrameMode frame_mode = ZMAPSTYLE_3_FRAME_INVALID;
  gboolean frame_specific = FALSE;
  
  g_return_val_if_fail(ZMAP_IS_WINDOW_ITEM_FEATURE_SET(set_data), FALSE);

  if(!set_data->lazy_loaded.frame_specific)
    {
      frame_mode = zmapWindowItemFeatureSetGetFrameMode(set_data) ;
      
      //set_data->lazy_loaded.frame_specific = 1;
     
      if(frame_mode != ZMAPSTYLE_3_FRAME_NEVER)
	set_data->settings.frame_specific = TRUE;

      if(frame_mode == ZMAPSTYLE_3_FRAME_INVALID)
	{
	  zMapLogWarning("Frame mode for column %s is invalid.", g_quark_to_string(set_data->unique_id));
	  set_data->settings.frame_specific = FALSE;
	}
    }  

  frame_specific = set_data->settings.frame_specific;

  if(frame_mode_out)
    *frame_mode_out = frame_mode;

  return frame_specific;
}

gboolean zmapWindowItemFeatureSetIsStrandSpecific(ZMapWindowItemFeatureSetData set_data)
{
  gboolean strand_specific = FALSE;

  g_object_get(G_OBJECT(set_data),
	       ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC, &(set_data->settings.strand_specific),
	       NULL);

  strand_specific = set_data->settings.strand_specific;

  return strand_specific;
}

ZMapStyleOverlapMode zmapWindowItemFeatureSetGetOverlapMode(ZMapWindowItemFeatureSetData set_data)
{
  ZMapStyleOverlapMode mode = ZMAPOVERLAP_COMPLETE;

  g_object_get(G_OBJECT(set_data),
	       ZMAPSTYLE_PROPERTY_OVERLAP_MODE, &(set_data->settings.overlap_mode),
	       NULL);

  mode = set_data->settings.overlap_mode;

  return mode;
}

ZMapStyleOverlapMode zmapWindowItemFeatureSetGetDefaultOverlapMode(ZMapWindowItemFeatureSetData set_data)
{
  ZMapStyleOverlapMode mode = ZMAPOVERLAP_COMPLETE;

  g_object_get(G_OBJECT(set_data),
	       ZMAPSTYLE_PROPERTY_DEFAULT_OVERLAP_MODE, &(set_data->settings.default_overlap_mode),
	       NULL);

  mode = set_data->settings.default_overlap_mode;

  return mode;
}

ZMapStyleOverlapMode zmapWindowItemFeatureSetResetOverlapModes(ZMapWindowItemFeatureSetData set_data)
{
  ZMapStyleOverlapMode mode = ZMAPOVERLAP_COMPLETE;
  ItemFeatureValueDataStruct value_data = {NULL};
  GValue value = {0};

  g_value_init(&value, G_TYPE_UINT);

  value_data.spec_name = ZMAPSTYLE_PROPERTY_OVERLAP_MODE;
  value_data.gvalue    = &value;
  value_data.param_id  = ITEM_FEATURE_SET_OVERLAP_MODE;

  g_hash_table_foreach(set_data->style_table, reset_overlap_mode_cb, &value_data);

  mode = g_value_get_uint(&value);

  return mode;
}

gboolean zmapWindowItemFeatureSetJoinAligns(ZMapWindowItemFeatureSetData set_data, unsigned int *threshold)
{
  gboolean result = FALSE;
  unsigned int tmp = 0;

  if(threshold)
    {
      g_object_get(G_OBJECT(set_data),
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

gboolean zmapWindowItemFeatureSetGetDeferred(ZMapWindowItemFeatureSetData set_data)
{
  gboolean is_deferred = FALSE;

  /* Not cached! */
  g_object_get(G_OBJECT(set_data),
	       ZMAPSTYLE_PROPERTY_DEFERRED, &is_deferred,
	       NULL);

  return is_deferred;
}

/* As we keep a list of the item we need to delete them at times.  This is actually _not_ 
 * used ATM (Apr 2008) as the reason it was written turned out to have a better solution
 * RT# 63281.  Anyway the code is here if needed.
 */
void zmapWindowItemFeatureSetFeatureRemove(ZMapWindowItemFeatureSetData item_feature_set,
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


ZMapWindowItemFeatureSetData zmapWindowItemFeatureSetDestroy(ZMapWindowItemFeatureSetData item_feature_set)
{
  g_object_unref(G_OBJECT(item_feature_set));

  item_feature_set = NULL;

  return item_feature_set;
}


/* Object code */
static void zmap_window_item_feature_set_class_init(ZMapWindowItemFeatureSetDataClass set_data_class)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *)set_data_class;

  gobject_class->set_property = zmap_window_item_feature_set_set_property;
  gobject_class->get_property = zmap_window_item_feature_set_get_property;

  parent_class_G = g_type_class_peek_parent(set_data_class);

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

  /* overlap mode */
  g_object_class_install_property(gobject_class,
				  ITEM_FEATURE_SET_OVERLAP_MODE,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_OVERLAP_MODE, 
						    ZMAPSTYLE_PROPERTY_OVERLAP_MODE,
						    "The Overlap Mode", 
						    ZMAPOVERLAP_INVALID, 
						    ZMAPOVERLAP_END, 
						    ZMAPOVERLAP_INVALID, 
						    ZMAP_PARAM_STATIC_RO));
  /* overlap default */
  g_object_class_install_property(gobject_class,
				  ITEM_FEATURE_SET_DEFAULT_OVERLAP_MODE,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_DEFAULT_OVERLAP_MODE, 
						    ZMAPSTYLE_PROPERTY_DEFAULT_OVERLAP_MODE,
						    "The Default Overlap Mode", 
						    ZMAPOVERLAP_INVALID, 
						    ZMAPOVERLAP_END, 
						    ZMAPOVERLAP_INVALID, 
						    ZMAP_PARAM_STATIC_RO));
  /* overlap default */
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

static void zmap_window_item_feature_set_init(ZMapWindowItemFeatureSetData set_data)
{

  set_data->style_table       = zmapWindowStyleTableCreate();
  set_data->user_hidden_stack = g_queue_new();

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
  ZMapWindowItemFeatureSetData set_data;

  set_data = ZMAP_WINDOW_ITEM_FEATURE_SET(gobject);

  switch(param_id)
    {
    case ITEM_FEATURE_SET_BUMP_SPACING:
    case ITEM_FEATURE_SET_WIDTH:
    case ITEM_FEATURE_SET_VISIBLE:
    case ITEM_FEATURE_SET_OVERLAP_MODE:
    case ITEM_FEATURE_SET_DEFAULT_OVERLAP_MODE:
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

	g_hash_table_foreach(set_data->style_table, extract_value_from_style_table, &value_data);
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
  ZMapWindowItemFeatureSetData set_data;
  GObjectClass *gobject_class = G_OBJECT_CLASS(parent_class_G);

  set_data = ZMAP_WINDOW_ITEM_FEATURE_SET(object);

  set_data->window = NULL;	/* not ours */
  
  if(set_data->style_table)
    {
      zmapWindowStyleTableDestroy(set_data->style_table);
      set_data->style_table = NULL;
    }

  if(set_data->user_hidden_stack)
    {
      if(!g_queue_is_empty(set_data->user_hidden_stack))
	g_queue_foreach(set_data->user_hidden_stack, removeList, NULL) ;

      g_queue_free(set_data->user_hidden_stack);

      set_data->user_hidden_stack = NULL;
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
    case ITEM_FEATURE_SET_OVERLAP_MODE:
    case ITEM_FEATURE_SET_DEFAULT_OVERLAP_MODE:
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

static void reset_overlap_mode_cb(gpointer key, gpointer value, gpointer user_data)
{
  ZMapFeatureTypeStyle style;

  style = ZMAP_FEATURE_STYLE(value);

  zMapStyleResetOverlapMode(style);

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
