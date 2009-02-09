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
 * Last edited: Feb  9 15:41 2009 (rds)
 * Created: Mon Jul 30 13:09:33 2007 (rds)
 * CVS info:   $Id: zmapWindowItemFeatureSet.c,v 1.4 2009-02-09 15:44:09 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapStyle.h>
#include <ZMap/zmapUtils.h>
#include <zmapWindowItemFeatureSet_I.h>

enum
  {
    ITEM_FEATURE_SET_0,		/* zero == invalid prop value */
    ITEM_FEATURE_SET_WIDTH,
    ITEM_FEATURE_SET_VISIBLE,
    ITEM_FEATURE_SET_OVERLAP_MODE,
    ITEM_FEATURE_SET_DEFAULT_OVERLAP_MODE,
    ITEM_FEATURE_SET_FRAME_MODE,
    ITEM_FEATURE_SET_SHOW_WHEN_EMPTY,
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

static void queueRemoveFromList(gpointer queue_data, gpointer user_data);
static void listRemoveFromList(gpointer list_data, gpointer user_data);
static void removeList(gpointer data, gpointer user_data_unused) ;
static void zmap_g_queue_replace(GQueue *queue, gpointer old, gpointer new);

GObjectClass *parent_class_G = NULL;


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
                                                            ZMapFeatureTypeStyle style,
                                                            ZMapStrand strand,
                                                            ZMapFrame frame)
{
  ZMapWindowItemFeatureSetData set_data;

  if((set_data = g_object_new(zmapWindowItemFeatureSetGetType(), NULL)))
    {
      set_data->window = window;
      set_data->strand = strand;
      set_data->frame  = frame;
      set_data->style_id  = zMapStyleGetID(style);
      set_data->unique_id = zMapStyleGetUniqueID(style);

      zmapWindowStyleTableAddCopy(set_data->style_table, style);
    }

  return set_data;
}

ZMapFeatureTypeStyle zmapWindowItemFeatureSetGetStyle(ZMapWindowItemFeatureSetData set_data,
						      ZMapFeature                  feature)
{
  ZMapFeatureTypeStyle style = NULL;

  style = zmapWindowStyleTableFind(set_data->style_table, feature->style_id);

  return style;
}

ZMapFeatureTypeStyle zmapWindowItemFeatureSetColumnStyle(ZMapWindowItemFeatureSetData set_data)
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

      if(min_mag != 0.0 && max_mag != 0.0)
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
      set_data->lazy_loaded.display_state = 1;
    }
  
  display = set_data->settings.display_state;

  return display;
}

void zmapWindowItemFeatureSetDisplay(ZMapWindowItemFeatureSetData set_data, ZMapStyleColumnDisplayState state)
{
  ZMapFeatureTypeStyle style;
  
  if((style = zmapWindowStyleTableFind(set_data->style_table, set_data->style_id)))
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

gboolean zmapWindowItemFeatureSetIsFrameSensitive(ZMapWindowItemFeatureSetData set_data)
{
  gboolean frame_sensitive = FALSE;

  if(!set_data->lazy_loaded.frame_sensitive)
    {
      g_object_get(G_OBJECT(set_data),
		   ZMAPSTYLE_PROPERTY_FRAME_MODE, &(set_data->settings.frame_sensitive),
		   NULL);
      set_data->lazy_loaded.frame_sensitive = 1;
    }
  
  frame_sensitive = set_data->settings.frame_sensitive;

  return frame_sensitive;
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

  /* Show when empty */
  g_object_class_install_property(gobject_class,
				  ITEM_FEATURE_SET_SHOW_WHEN_EMPTY,
				  g_param_spec_boolean(ZMAPSTYLE_PROPERTY_SHOW_WHEN_EMPTY, 
						       ZMAPSTYLE_PROPERTY_SHOW_WHEN_EMPTY,
						       "Does the Style get shown when empty",
						       TRUE, ZMAP_PARAM_STATIC_RO));


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
    case ITEM_FEATURE_SET_WIDTH:
    case ITEM_FEATURE_SET_VISIBLE:
    case ITEM_FEATURE_SET_OVERLAP_MODE:
    case ITEM_FEATURE_SET_DEFAULT_OVERLAP_MODE:
    case ITEM_FEATURE_SET_FRAME_MODE:
    case ITEM_FEATURE_SET_SHOW_WHEN_EMPTY:
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
    case ITEM_FEATURE_SET_SHOW_WHEN_EMPTY:
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
    default:
      break;
    }

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
