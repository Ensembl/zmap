/*  File: zmapWindowFeatureList.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: Implements a sortable list of feature(s) displaying
 *              coords, strand and much else.
 *
 * Exported functions: See zmapWindow_P.h
 *
 *-------------------------------------------------------------------
 */

#include<string.h>

#include <ZMap/zmap.hpp>

#include <ZMap/zmapString.hpp>
#include <ZMap/zmapFeature.hpp>
#include <ZMap/zmapBase.hpp>
#include <zmapWindowFeatureList_I.hpp>
#include <zmapWindow_P.hpp>



#define ZMAP_WFL_SETDATASTRAND_COLUMN_NAME "-set-data-strand-"
#define ZMAP_WFL_SETDATAFRAME_COLUMN_NAME  "-set-data-frame-"

enum
  {
    ZMAP_WFL_NOPROP,          /* zero is invalid property id */

    ZMAP_WFL_FEATURE_TYPE,

    ZMAP_WFL_SETDATA_STRAND_INDEX,
    ZMAP_WFL_SETDATA_FRAME_INDEX,

  };

/*
 * These are the data required to add a feature to the list; i.e.
 * the feature itself and the window that it's drawn in.
 */
typedef struct
{
  ZMapFeatureAny feature;
  ZMapWindow     window;
} AddSimpleDataFeatureStruct, *AddSimpleDataFeature;


static void zmap_windowfeaturelist_class_init(ZMapWindowFeatureListClass zmap_tv_class);
static void zmap_windowfeaturelist_init(ZMapWindowFeatureList zmap_tv);
static void zmap_windowfeaturelist_set_property(GObject *gobject,
                                     guint param_id,
                                     const GValue *value,
                                     GParamSpec *pspec);
static void zmap_windowfeaturelist_get_property(GObject *gobject,
                                     guint param_id,
                                     GValue *value,
                                     GParamSpec *pspec);
static void zmap_windowfeaturelist_dispose(GObject *object);
static void zmap_windowfeaturelist_finalize(GObject *object);


static void feature_add_simple(ZMapGUITreeView zmap_tv,
                         gpointer user_data);

/* Which calls some/all of these ZMapGUITreeViewCellFunc's */
static void feature_name_to_value       (GValue *value, gpointer feature_data);
static void feature_start_to_value      (GValue *value, gpointer feature_data);
static void feature_end_to_value        (GValue *value, gpointer feature_data);
static void feature_strand_to_value     (GValue *value, gpointer feature_data);
static void feature_frame_to_value      (GValue *value, gpointer feature_data);
static void feature_phase_to_value      (GValue *value, gpointer feature_data);
static void feature_qstart_to_value     (GValue *value, gpointer feature_data);
static void feature_qend_to_value       (GValue *value, gpointer feature_data);
static void feature_qstrand_to_value    (GValue *value, gpointer feature_data);
static void feature_score_to_value      (GValue *value, gpointer feature_data);
static void feature_featureset_to_value (GValue *value, gpointer feature_data);
static void feature_type_to_value       (GValue *value, gpointer feature_data);
static void feature_source_to_value     (GValue *value, gpointer feature_data);
static void feature_style_to_value      (GValue *value, gpointer feature_data);
static void feature_pointer_to_value    (GValue *value, gpointer feature_data);
/* Which were set up by */
static void setup_tree(ZMapWindowFeatureList zmap_tv, ZMapStyleMode feature_type);
/* from the hard coded lists in */
static void feature_type_get_titles_types_funcs(ZMapStyleMode feature_type,
                                                GList **titles_out,
                                                GList **types_out,
                                                GList **funcs_out,
                                                GList **vis_out);


static void featureItem2BumpHidden(GValue *value, gpointer feature_item_data) ;
static void featureItem2UserHidden(GValue *value, gpointer feature_item_data) ;
  static void featureItem2IsVisible(GValue *value, gpointer feature_item_data) ;



/* faster than g_type_class_peek_parent all the time */
static ZMapGUITreeViewClass parent_class_G = NULL;

/* Public functions */

GType zMapWindowFeatureListGetType (void)
{
  static GType type = 0;

  if (type == 0)
    {
      static const GTypeInfo info =
      {
        sizeof (zmapWindowFeatureListClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) zmap_windowfeaturelist_class_init,
        (GClassFinalizeFunc) NULL,
        NULL /* class_data */,
        sizeof (zmapWindowFeatureList),
        0 /* n_preallocs */,
        (GInstanceInitFunc) zmap_windowfeaturelist_init,
        NULL
      };

      type = g_type_register_static (zMapGUITreeViewGetType(), "ZMapWindowFeatureList", &info, (GTypeFlags)0);
    }

  return type;
}


ZMapWindowFeatureList zMapWindowFeatureCreate(ZMapStyleMode feature_type)
{
  ZMapWindowFeatureList zmap_tv = NULL;
  GParameter parameter = {"feature-type", {}};

  g_value_init(&(parameter.value), G_TYPE_UINT);
  g_value_set_uint(&(parameter.value), feature_type);

  zmap_tv = ((ZMapWindowFeatureList)g_object_newv(zMapWindowFeatureListGetType(),
                                      1, &parameter));

  return zmap_tv;
}

/* Some convenience functions, with more useful names... */
void zMapWindowFeatureListAddFeature(ZMapWindowFeatureList zmap_tv,
                             ZMapFeatureAny        feature)
{
  zMapGUITreeViewAddTuple(ZMAP_GUITREEVIEW(zmap_tv), feature);
  return ;
}

void zMapWindowFeatureListListAddFeatures(ZMapWindowFeatureList zmap_tv,
                                 GList *list_of_features)
{
  zMapGUITreeViewAddTuples(ZMAP_GUITREEVIEW(zmap_tv), list_of_features);
  return ;
}



/* Object code */
static void zmap_windowfeaturelist_class_init(ZMapWindowFeatureListClass zmap_tv_class)
{
  GObjectClass *gobject_class;
  ZMapGUITreeViewClass parent_class;

  gobject_class  = (GObjectClass *)zmap_tv_class;
  parent_class   = ZMAP_GUITREEVIEW_CLASS(zmap_tv_class);
  parent_class_G = (ZMapGUITreeViewClass)g_type_class_peek_parent(zmap_tv_class);

  gobject_class->set_property = zmap_windowfeaturelist_set_property;
  gobject_class->get_property = zmap_windowfeaturelist_get_property;

  g_object_class_install_property(gobject_class,
                          ZMAP_WFL_FEATURE_TYPE,
                          g_param_spec_uint("feature-type", "feature-type",
                                        "Feature Type that the view will be displaying.",
                                        ZMAPSTYLE_MODE_INVALID, 128,
                                        ZMAPSTYLE_MODE_INVALID,
                                        ZMAP_PARAM_STATIC_RW));

  /* override add_tuple_simple. */
  parent_class->add_tuple_simple = feature_add_simple;

  /* parent_class->add_tuples from parent is ok. */

  /* add_tuple_value_list _not_ implemented! Doesn't make sense. */
  parent_class->add_tuple_value_list = NULL;

  gobject_class->dispose  = zmap_windowfeaturelist_dispose;
  gobject_class->finalize = zmap_windowfeaturelist_finalize;

  return ;
}


static void zmap_windowfeaturelist_init(ZMapWindowFeatureList zmap_tv)
{
  /* Nothing to do */
  return ;
}

static void zmap_windowfeaturelist_set_property(GObject *gobject,
                                     guint param_id,
                                     const GValue *value,
                                     GParamSpec *pspec)
{
  ZMapWindowFeatureList zmap_tv;

  g_return_if_fail(ZMAP_IS_WINDOWFEATURELIST(gobject));

  zmap_tv = ZMAP_WINDOWFEATURELIST(gobject);

  switch(param_id)
    {
    case ZMAP_WFL_FEATURE_TYPE:
      {
      ZMapStyleMode feature_type;
      /* Should be g_value_get_enum(value) */
      feature_type = (ZMapStyleMode)g_value_get_uint(value);

      if(zmap_tv->feature_type == ZMAPSTYLE_MODE_INVALID &&
         feature_type != ZMAPSTYLE_MODE_INVALID)
        {
          zmap_tv->feature_type = feature_type;
          setup_tree(zmap_tv, feature_type);
        }
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_windowfeaturelist_get_property(GObject *gobject,
                                     guint param_id,
                                     GValue *value,
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

static void zmap_windowfeaturelist_dispose(GObject *object)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(parent_class_G);

  if(gobject_class->dispose)
    (*gobject_class->dispose)(object);

  return ;
}
static void zmap_windowfeaturelist_finalize(GObject *object)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(parent_class_G);

  if(gobject_class->finalize)
    (*gobject_class->finalize)(object);

  return ;
}

static void feature_add_simple(ZMapGUITreeView zmap_tv,
                         gpointer user_data)
{
  ZMapWindowFeatureList zmap_tv_feature;
  ZMapFeature feature = (ZMapFeature)user_data;
  AddSimpleDataFeatureStruct add_simple = {NULL};

  zmap_tv_feature = ZMAP_WINDOWFEATURELIST(zmap_tv);

  if(zmap_tv_feature->feature_type == ZMAPSTYLE_MODE_INVALID &&
     feature->mode != ZMAPSTYLE_MODE_INVALID)
    {
      zmap_tv_feature->feature_type = feature->mode;
      setup_tree(zmap_tv_feature, feature->mode);
    }

  /* Always add the feature... */
  add_simple.feature = (ZMapFeatureAny)feature;

  /* Only add the window if display_forward_coords == TRUE */
  if(zmap_tv_feature->window &&
     zmap_tv_feature->window->display_forward_coords == TRUE)
    add_simple.window = zmap_tv_feature->window;

  if(zmap_tv_feature->feature_type != ZMAPSTYLE_MODE_INVALID &&
     zmap_tv_feature->feature_type == feature->mode &&
     parent_class_G->add_tuple_simple)
    (* parent_class_G->add_tuple_simple)(zmap_tv, &add_simple);

  return ;
}


/* Internals */

static void setup_tree(ZMapWindowFeatureList zmap_tv,
                   ZMapStyleMode feature_type)
{
  GList *column_titles = NULL;
  GList *column_types  = NULL;
  GList *column_funcs  = NULL;
  GList *column_flags  = NULL;

  feature_type_get_titles_types_funcs(feature_type,
                              &column_titles,
                              &column_types,
                              &column_funcs,
                              &column_flags);

  g_object_set(G_OBJECT(zmap_tv),
             "row-counter-column",  TRUE,
             "data-ptr-column",     FALSE,
             "column_count",        g_list_length(column_titles),
             "column_names",        column_titles,
             "column_types",        column_types,
             "column_funcs",        column_funcs,
             "column_flags_list",   column_flags,
             "sortable",            TRUE,
             NULL);

  zMapGUITreeViewPrepare(ZMAP_GUITREEVIEW(zmap_tv));

  if(column_titles)
    g_list_free(column_titles);
  if(column_types)
    g_list_free(column_types);
  if(column_funcs)
    g_list_free(column_funcs);

  return ;
}


/* These have been written to handle ZMapFeatureAny, so should not crash,
 * but they _are_ limited to ZMapFeature types really. */
static void feature_name_to_value(GValue *value, gpointer feature_data)
{
  const char *the_name ;
  char *temp_name = NULL ;
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeatureAny    feature = NULL ;

  if (!add_data || !add_data->feature)
    return ;

  feature = add_data->feature;

  if(G_VALUE_TYPE(value) == G_TYPE_STRING)
    {
      the_name = g_quark_to_string(feature->original_id) ;

      if ((temp_name = (char *)zMapStringAbbrevStr(the_name)))
        the_name = temp_name ;

      g_value_set_string(value, the_name) ;

      if (temp_name)
        g_free(temp_name) ;


    }
  else
    zMapWarnIfReached();

  return ;
}

static void feature_start_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature = NULL ;
  ZMapWindow         window = NULL ;

  if (!add_data || !add_data->feature)
    return ;

  feature = (ZMapFeature)(add_data->feature);
  window = add_data->window;

  if(G_VALUE_TYPE(value) == G_TYPE_INT)
    {
      switch(feature->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        if(window)
          g_value_set_int(value, zmapWindowCoordToDisplay(window, window->display_coordinates, feature->x1));
        else
          g_value_set_int(value, feature->x1);
        break;
      default:
        g_value_set_int(value, 0);
        break;
      }
    }
  else
    {
      zMapWarnIfReached();
    }

  return ;
}

static void feature_end_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature = NULL ;
  ZMapWindow        window  = NULL ;

  if (!add_data || !add_data->feature)
    return ;

  feature = (ZMapFeature)(add_data->feature);
  window  = add_data->window;

  if(G_VALUE_TYPE(value) == G_TYPE_INT)
    {
      switch(feature->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        if(window)
          g_value_set_int(value, zmapWindowCoordToDisplay(window, window->display_coordinates, feature->x2));
        else
          g_value_set_int(value, feature->x2);
        break;
      default:
        g_value_set_int(value, 0);
        break;
      }
    }
  else
    {
      zMapWarnIfReached();
    }

  return ;
}

static void feature_strand_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature = NULL ;

  if (!add_data || !add_data->feature || !add_data->window)
    return ;

  feature = (ZMapFeature)(add_data->feature);

  if(G_VALUE_TYPE(value) == G_TYPE_STRING)
    {
      switch(feature->struct_type)
        {
        case ZMAPFEATURE_STRUCT_FEATURE:
          g_value_set_string(value, zMapFeatureStrand2Str(zmapWindowStrandToDisplay(add_data->window,
                                                                                    feature->strand))) ;
          break;
        default:
          g_value_set_string(value, ".");
          break;
        }
    }
  else
    {
      zMapWarnIfReached();
    }

  return ;
}

static void feature_frame_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature = NULL ;

  if (!add_data || !add_data->feature)
    return ;

  feature = (ZMapFeature)(add_data->feature) ;

  if(G_VALUE_TYPE(value) == G_TYPE_STRING)
    {
      switch(feature->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        /*! \todo #warning this used to use strand!  and this function i think has never been called */
        g_value_set_string(value, zMapFeatureFrame2Str(zMapFeatureFrame(feature)));
        break;
      default:
        g_value_set_string(value, ".");
        break;
      }
    }
  else
    {
      zMapWarnIfReached();
    }

  return ;
}

static void feature_phase_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature = NULL ;

  if (!add_data || !add_data->feature)
    return ;

  feature = (ZMapFeature)(add_data->feature);

  if(G_VALUE_TYPE(value) == G_TYPE_STRING)
    {
      switch(feature->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        g_value_set_string(value, zMapFeaturePhase2Str(zMapFeaturePhase(feature)));
        break;
      default:
        g_value_set_string(value, ".");
        break;
      }
    }
  else
    {
      zMapWarnIfReached();
    }

  return ;
}

static void feature_qstart_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature = NULL ;

  if (!add_data || !add_data->feature )
    return ;

  feature = (ZMapFeature)(add_data->feature) ;

  if(G_VALUE_TYPE(value) == G_TYPE_INT)
    {
      switch(feature->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        {
          switch(feature->mode)
            {
            case ZMAPSTYLE_MODE_ALIGNMENT:
            g_value_set_int(value, feature->feature.homol.y1);
            break;
            default:
            g_value_set_int(value, 0);
            break;
            } /* switch(feature->mode) */
        }
        break;
      default:
        g_value_set_int(value, 0);
        break;
      } /* switch(feature->struct_type) */
    }
  else
    {
      zMapWarnIfReached();
    }

  return ;
}

static void feature_qend_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature = NULL ;

  if (!add_data || !add_data->feature)
    return ;

  feature = (ZMapFeature)(add_data->feature) ;

  if(G_VALUE_TYPE(value) == G_TYPE_INT)
    {
      switch(feature->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        {
          switch(feature->mode)
            {
            case ZMAPSTYLE_MODE_ALIGNMENT:
            g_value_set_int(value, feature->feature.homol.y2);
            break;
            default:
            g_value_set_int(value, 0);
            break;
            }     /* switch(feature->mode) */
        }
        break;
      default:
        g_value_set_int(value, 0);
        break;
      } /* switch(feature->struct_type) */
    }
  else
    {
      zMapWarnIfReached();
    }

  return ;
}

static void feature_qstrand_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature = NULL ;

  if (!add_data || !add_data->feature)
    return ;

  feature = (ZMapFeature)(add_data->feature) ;

  if(G_VALUE_TYPE(value) == G_TYPE_STRING)
    {
      switch(feature->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        {
          switch(feature->mode)
            {
            case ZMAPSTYLE_MODE_ALIGNMENT:
            g_value_set_string(value, zMapFeatureStrand2Str(feature->feature.homol.strand));
            break;
            default:
            g_value_set_string(value, ".");
            break;
            }     /* switch(feature->mode) */
        }
        break;
      default:
        g_value_set_string(value, ".");
        break;
      } /* switch(feature->struct_type) */
    }
  else
    {
      zMapWarnIfReached();
    }

  return ;
}

static void feature_score_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature = NULL ;

  if (!add_data || !add_data->feature)
    return ;

  feature = (ZMapFeature)(add_data->feature);

  if(G_VALUE_TYPE(value) == G_TYPE_FLOAT)
    {
      switch(feature->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        g_value_set_float(value, feature->score);
        break;
      default:
        g_value_set_float(value, 0.0);
        break;
      }
    }
  else
    {
      zMapWarnIfReached();
    }

  return ;
}

static void feature_featureset_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature = NULL ;

  if (!add_data || !add_data->feature)
    return ;

  feature = (ZMapFeature)(add_data->feature) ;

  if(G_VALUE_TYPE(value) == G_TYPE_STRING)
    {
      switch(feature->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        add_data->feature = feature->parent;
        feature_name_to_value(value, add_data);
        /* Need to reset this! */
        add_data->feature = (ZMapFeatureAny)feature;
        break;
      case ZMAPFEATURE_STRUCT_FEATURESET:
        feature_name_to_value(value, add_data);
        break;
      default:
        break;
      }
    }
  else
    {
      zMapWarnIfReached();
    }

  return ;
}

static void feature_type_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature = NULL ;

  if (!add_data || !add_data->feature)
    return ;

  feature = (ZMapFeature)(add_data->feature);

  if(G_VALUE_TYPE(value) == G_TYPE_STRING)
    {
      switch(feature->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        g_value_set_string(value, zMapStyleMode2ExactStr(feature->mode));
        break;
      default:
        break;
      }
    }
  else
    {
      zMapWarnIfReached();
    }

  return ;
}

static void feature_source_to_value (GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature =  NULL ;

  if (!add_data || !add_data->feature)
    return ;

  feature = (ZMapFeature)(add_data->feature);

  if(G_VALUE_TYPE(value) == G_TYPE_STRING)
    {
      switch(feature->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        g_value_set_string(value, g_quark_to_string(feature->source_id));
        break;
      default:
        break;
      }
    }
  else
    {
      zMapWarnIfReached();
    }

  return ;
}

static void feature_style_to_value (GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature = NULL ;

  if (!add_data || !add_data->feature)
    return ;

  feature = (ZMapFeature)(add_data->feature);

  if(G_VALUE_TYPE(value) == G_TYPE_STRING)
    {
      switch(feature->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        g_value_set_string(value, g_quark_to_string((*feature->style)->unique_id));
        break;
      default:
        break;
      }
    }
  else
    {
      zMapWarnIfReached();
    }

  return ;
}

static void feature_pointer_to_value(GValue *value, gpointer feature_item_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_item_data;
  ZMapFeatureAny        feature = NULL ;

  if (!add_data || !add_data->feature)
    return ;

  feature = add_data->feature;

  g_value_set_pointer(value, feature);

  return ;
}

/* GLists _must_ be freed, but all contents are static (DO NOT FREE) */
static void feature_type_get_titles_types_funcs(ZMapStyleMode feature_type,
                                    GList **titles_out,
                                    GList **types_out,
                                    GList **funcs_out,
                                    GList **flags_out)
{
  GList *titles, *types, *funcs, *flags;
  gboolean frame_and_phase = FALSE;
  unsigned int flags_set = (ZMAP_GUITREEVIEW_COLUMN_VISIBLE |
                      ZMAP_GUITREEVIEW_COLUMN_CLICKABLE);

  titles = types = funcs = flags = NULL;

  /* First the generic ones */

  /* N.B. Order here creates order of columns */

  /* Feature Name */
  titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_NAME_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, (void *)feature_name_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* Feature Start */
  titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_START_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
  funcs  = g_list_append(funcs, (void *)feature_start_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* Feature End */
  titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_END_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
  funcs  = g_list_append(funcs, (void *)feature_end_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* Feature Strand */
  titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_STRAND_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, (void *)feature_strand_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  if(frame_and_phase)
    {
      /* Feature Frame */
      titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_FRAME_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
      funcs  = g_list_append(funcs, (void *)feature_frame_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

      /* Feature Phase */
      titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_PHASE_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
      funcs  = g_list_append(funcs, (void *)feature_phase_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));
    }

  if(feature_type == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      /* Not much */
    }
  else if(feature_type == ZMAPSTYLE_MODE_ALIGNMENT)
    {
      /* Feature Query Start */
      titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_QSTART_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
      funcs  = g_list_append(funcs, (void *)feature_qstart_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

      /* Feature Query End */
      titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_QEND_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
      funcs  = g_list_append(funcs, (void *)feature_qend_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

      /* Feature Query Strand */
      titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_QSTRAND_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
      funcs  = g_list_append(funcs, (void *)feature_qstrand_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

      /* Feature Score */
      titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_SCORE_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_FLOAT));
      funcs  = g_list_append(funcs, (void *)feature_score_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));
    }
  else if(feature_type == ZMAPSTYLE_MODE_BASIC)
    {
      /* Not much */
    }

  /* Feature's feature set  */
  titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_SET_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, (void *)feature_featureset_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* Feature's Type */
  titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_SOURCE_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, (void *)feature_source_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));


  /* Feature pointer */
  /* Using ZMAP_GUITREEVIEW_DATA_PTR_COLUMN_NAME make g_object_get() work :) */
  titles = g_list_append(titles, (void *)ZMAP_GUITREEVIEW_DATA_PTR_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_POINTER));
  funcs  = g_list_append(funcs, (void *)feature_pointer_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(ZMAP_GUITREEVIEW_COLUMN_NOTHING));


  /* Now return the lists to caller's supplied pointers */
  if(titles_out)
    *titles_out = titles;
  if(types_out)
    *types_out  = types;
  if(funcs_out)
    *funcs_out  = funcs;
  if(flags_out)
    *flags_out  = flags;

  return ;
}

/* ----------------------- */
/* FeatureItem Object code */
/* ----------------------- */

typedef struct
{
  ZMapWindowFeatureItemList feature_list;
  ZMapWindow               window;
  GHashTable               *context_to_item;
  GList                    *row_ref_list; /* GList * of GtkTreeRowReference * */
  GtkTreeModel             *model; /* model we're stepping through...sadly... */
  gboolean                  list_incomplete;
} ModelForeachStruct, *ModelForeach;

typedef struct
{
  ZMapFeatureAny feature;
  ZMapWindow     window;
  FooCanvasItem *item;
} AddSimpleDataFeatureItemStruct, *AddSimpleDataFeatureItem;

/*!
 * \brief The data we hold on the data pointer. See zMapWindowFeatureItemListGetItem()
 */
typedef struct
{
  GQuark align_id,
    block_id,
    set_id,
    feature_id;
} SerialisedFeatureSearchStruct, *SerialisedFeatureSearch;

static void zmap_windowfeatureitemlist_class_init(ZMapWindowFeatureItemListClass zmap_tv_class);
static void zmap_windowfeatureitemlist_init(ZMapWindowFeatureItemList zmap_tv);
static void zmap_windowfeatureitemlist_set_property(GObject *gobject,
                                     guint param_id,
                                     const GValue *value,
                                     GParamSpec *pspec);
static void zmap_windowfeatureitemlist_get_property(GObject *gobject,
                                     guint param_id,
                                     GValue *value,
                                     GParamSpec *pspec);
static void zmap_windowfeatureitemlist_dispose(GObject *object);
static void zmap_windowfeatureitemlist_finalize(GObject *object);


static void feature_item_add_simple(ZMapGUITreeView zmap_tv,
                            gpointer user_data);

/* Which calls some/all of these ZMapGUITreeViewCellFunc's */
static void feature_item_data_strand_to_value(GValue *value, gpointer feature_item_data);
static void feature_item_data_frame_to_value (GValue *value, gpointer feature_item_data);
static void feature_pointer_serialised_to_value (GValue *value, gpointer feature_item_data);
/* Which were set up by */
static void setup_item_tree(ZMapWindowFeatureItemList zmap_tv, ZMapStyleMode feature_type);
/* from the hard coded lists in */
static void feature_item_type_get_titles_types_funcs(ZMapStyleMode feature_type,
                                         GList **titles_out,
                                         GList **types_out,
                                         GList **funcs_out,
                                         GList **vis_out);
static gboolean update_foreach_cb(GtkTreeModel *model,
                          GtkTreePath  *path,
                          GtkTreeIter  *iter,
                          gpointer      user_data);
static void free_serialised_data_cb(gpointer user_data);
static void invoke_tuple_remove(gpointer list_data, gpointer user_data);
static gboolean fetch_lookup_data(ZMapWindowFeatureItemList zmap_tv,
                          GtkTreeModel             *model,
                          GtkTreeIter              *iter,
                          SerialisedFeatureSearch  *lookup_data_out,
                          ZMapStrand               *strand_out,
                          ZMapFrame                *frame_out);

static ZMapGUITreeViewClass feature_item_parent_class_G = NULL;

/* Public functions */

GType zMapWindowFeatureItemListGetType (void)
{
  static GType type = 0;

  if (type == 0)
    {
      static const GTypeInfo info =
      {
        sizeof (zmapWindowFeatureItemListClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) zmap_windowfeatureitemlist_class_init,
        (GClassFinalizeFunc) NULL,
        NULL /* class_data */,
        sizeof (zmapWindowFeatureItemList),
        0 /* n_preallocs */,
        (GInstanceInitFunc) zmap_windowfeatureitemlist_init,
        NULL
      };

      type = g_type_register_static (zMapGUITreeViewGetType(), "ZMapWindowFeatureItemList", &info, (GTypeFlags)0);
    }

  return type;
}


ZMapWindowFeatureItemList zMapWindowFeatureItemListCreate(ZMapStyleMode feature_type)
{
  ZMapWindowFeatureItemList zmap_tv = NULL;
  GParameter parameter = {"feature-type", {}};

  g_value_init(&(parameter.value), G_TYPE_UINT);
  g_value_set_uint(&(parameter.value), feature_type);

  zmap_tv = ((ZMapWindowFeatureItemList)g_object_newv(zMapWindowFeatureItemListGetType(),
                                           1, &parameter));

  return zmap_tv;
}

/* Some convenience functions, with more useful names... */
void zMapWindowFeatureItemListAddItem(ZMapWindowFeatureItemList zmap_tv,
                              ZMapWindow                window,
                              FooCanvasItem *feature_item)
{
  zmap_tv->window = NULL;
  zMapGUITreeViewAddTuple(ZMAP_GUITREEVIEW(zmap_tv), feature_item);
  zmap_tv->window = NULL;
  return ;
}

void zMapWindowFeatureItemListAddItems(ZMapWindowFeatureItemList zmap_tv,
                               ZMapWindow                window,
                               GList                    *list_of_feature_items)
{
  zmap_tv->window = window;
  zMapGUITreeViewAddTuples(ZMAP_GUITREEVIEW(zmap_tv), list_of_feature_items);
  zmap_tv->window = NULL;
  return ;
}

void zMapWindowFeatureItemListUpdateItem(ZMapWindowFeatureItemList zmap_tv,
                               ZMapWindow                window,
                               GtkTreeIter              *tree_iterator,
                               FooCanvasItem            *feature_item)
{
  SerialisedFeatureSearch feature_data;

  if(fetch_lookup_data(zmap_tv, ZMAP_GUITREEVIEW(zmap_tv)->tree_model,
                   tree_iterator, &feature_data, NULL, NULL))
    free_serialised_data_cb(feature_data);

  zmap_tv->window = window;
  zMapGUITreeViewUpdateTuple(ZMAP_GUITREEVIEW(zmap_tv), tree_iterator, feature_item);
  zmap_tv->window = NULL;

  return ;
}

gboolean zMapWindowFeatureItemListUpdateAll(ZMapWindowFeatureItemList zmap_tv,
                                  ZMapWindow                window,
                                  GHashTable               *context_to_item)
{
  ModelForeachStruct full_data = {NULL};
  gboolean success = FALSE;

  full_data.feature_list    = zmap_tv;
  full_data.window          = window;
  full_data.context_to_item = context_to_item;
  full_data.row_ref_list    = NULL;
  full_data.list_incomplete = success;

  g_object_get(G_OBJECT(zmap_tv),
             "tree-model", &full_data.model,
             NULL);

  zMapGUITreeViewPrepare(ZMAP_GUITREEVIEW(zmap_tv));

  zmap_tv->window = window;
  gtk_tree_model_foreach(full_data.model, update_foreach_cb, &full_data);
  zmap_tv->window = NULL;

  /* The update_foreach_cb creates row references in order to remove
   * rows, otherwise the foreach iterator gets it's knickers in a
   * knot and fails to visit every tuple. Here we have to revisit
   * the marked rows and really do the removal... */

  g_list_foreach(full_data.row_ref_list, invoke_tuple_remove, &full_data);
  g_list_foreach(full_data.row_ref_list, (GFunc)gtk_tree_row_reference_free, NULL);
  g_list_free(full_data.row_ref_list);

  zMapGUITreeViewAttach(ZMAP_GUITREEVIEW(zmap_tv));

  success = (gboolean)!(full_data.list_incomplete);

  return success;
}

FooCanvasItem *zMapWindowFeatureItemListGetItem(ZMapWindow window,
                                    ZMapWindowFeatureItemList zmap_tv,
                                    GHashTable  *context_to_item,
                                    GtkTreeIter *tree_iterator)
{
  FooCanvasItem *item = NULL;
  GtkTreeModel *model = NULL;
  SerialisedFeatureSearch feature_data = NULL;
  ZMapStrand set_strand ;
  ZMapFrame set_frame ;
  int data_index = 0, strand_index, frame_index;


  g_object_get(G_OBJECT(zmap_tv),
             "data-ptr-index",   &data_index,
             "set-strand-index", &strand_index,
             "set-frame-index",  &frame_index,
             "tree-model",       &model,
             NULL);

  gtk_tree_model_get(model, tree_iterator,
                 data_index,   &feature_data,
                 strand_index, &set_strand,
                 frame_index,  &set_frame,
                 -1) ;

  if(feature_data)
    {
      item = zmapWindowFToIFindItemFull(window,context_to_item,
                              feature_data->align_id,
                              feature_data->block_id,
                              feature_data->set_id,
                              set_strand, set_frame,
                              feature_data->feature_id);
    }

  return item;
}

ZMapFeature zMapWindowFeatureItemListGetFeature(ZMapWindow window,
                                    ZMapWindowFeatureItemList zmap_tv,
                                    GHashTable  *context_to_item,
                                    GtkTreeIter *tree_iterator)
{
  ZMapFeature feature = NULL;
  FooCanvasItem *feature_item = NULL;

  if((feature_item = zMapWindowFeatureItemListGetItem(window,zmap_tv, context_to_item, tree_iterator)))
    feature = zmapWindowItemGetFeature(feature_item);

  return feature;
}

ZMapWindowFeatureItemList zMapWindowFeatureItemListDestroy(ZMapWindowFeatureItemList zmap_tv)
{
  g_object_unref(G_OBJECT(zmap_tv));

  zmap_tv = NULL;

  return zmap_tv;
}

/* Object code */
static void zmap_windowfeatureitemlist_class_init(ZMapWindowFeatureItemListClass zmap_tv_class)
{
  GObjectClass *gobject_class;
  ZMapGUITreeViewClass parent_class;

  gobject_class = (GObjectClass *)zmap_tv_class;
  parent_class  = ZMAP_GUITREEVIEW_CLASS(zmap_tv_class);
  feature_item_parent_class_G = (ZMapGUITreeViewClass)g_type_class_peek_parent(zmap_tv_class);

  gobject_class->set_property = zmap_windowfeatureitemlist_set_property;
  gobject_class->get_property = zmap_windowfeatureitemlist_get_property;

  g_object_class_install_property(gobject_class,
                          ZMAP_WFL_FEATURE_TYPE,
                          g_param_spec_uint("feature-type", "feature-type",
                                        "Feature Type that the view will be displaying.",
                                        ZMAPSTYLE_MODE_INVALID, 128,
                                        ZMAPSTYLE_MODE_INVALID,
                                        ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_WFL_SETDATA_STRAND_INDEX,
                          g_param_spec_uint("set-strand-index", "set-strand-index",
                                        "The index for the set data strand.",
                                        0, 128, 0,
                                        ZMAP_PARAM_STATIC_RO));

  g_object_class_install_property(gobject_class,
                          ZMAP_WFL_SETDATA_FRAME_INDEX,
                          g_param_spec_uint("set-frame-index", "set-frame-index",
                                        "The index for the set data frame.",
                                        0, 128, 0,
                                        ZMAP_PARAM_STATIC_RO));

  /* override add_tuple_simple. */
  parent_class->add_tuple_simple = feature_item_add_simple;

  /* parent_class->add_tuples from parent is ok. */

  /* add_tuple_value_list _not_ implemented! Doesn't make sense. */
  parent_class->add_tuple_value_list = NULL;

  gobject_class->dispose  = zmap_windowfeatureitemlist_dispose;
  gobject_class->finalize = zmap_windowfeatureitemlist_finalize;

  return ;
}


static void zmap_windowfeatureitemlist_init(ZMapWindowFeatureItemList zmap_tv)
{
  /* Nothing to do */
  return ;
}

static void zmap_windowfeatureitemlist_set_property(GObject *gobject,
                                         guint param_id,
                                         const GValue *value,
                                         GParamSpec *pspec)
{
  ZMapWindowFeatureItemList zmap_tv;

  g_return_if_fail(ZMAP_IS_WINDOWFEATUREITEMLIST(gobject));

  zmap_tv = ZMAP_WINDOWFEATUREITEMLIST(gobject);

  switch(param_id)
    {
    case ZMAP_WFL_FEATURE_TYPE:
      {
      ZMapStyleMode feature_type;
      /* Should be g_value_get_enum(value) */
      feature_type = (ZMapStyleMode)g_value_get_uint(value);

      if(zmap_tv->feature_type == ZMAPSTYLE_MODE_INVALID &&
         feature_type != ZMAPSTYLE_MODE_INVALID)
        {
          zmap_tv->feature_type = feature_type;
#ifdef NOT_SURE_ON_THIS
          setup_item_tree(zmap_tv, feature_type);
#endif /* NOT_SURE_ON_THIS */
        }
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_windowfeatureitemlist_get_property(GObject *gobject,
                                         guint param_id,
                                         GValue *value,
                                         GParamSpec *pspec)
{

  switch(param_id)
    {
    case ZMAP_WFL_SETDATA_STRAND_INDEX:
      {
      ZMapGUITreeView zmap_tree_view;
      int index = -1;

      zmap_tree_view = ZMAP_GUITREEVIEW(gobject);

      index = zMapGUITreeViewGetColumnIndexByName(zmap_tree_view,
                                        ZMAP_WFL_SETDATASTRAND_COLUMN_NAME);

      g_value_set_uint(value, index);
      }
      break;
    case ZMAP_WFL_SETDATA_FRAME_INDEX:
      {
      ZMapGUITreeView zmap_tree_view;
      int index = -1;

      zmap_tree_view = ZMAP_GUITREEVIEW(gobject);

      index = zMapGUITreeViewGetColumnIndexByName(zmap_tree_view,
                                        ZMAP_WFL_SETDATAFRAME_COLUMN_NAME);

      g_value_set_uint(value, index);
      }
      break;
    default:
      break;
    }

  return ;
}

static void zmap_windowfeatureitemlist_dispose(GObject *object)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(feature_item_parent_class_G);

  if(gobject_class->dispose)
    (*gobject_class->dispose)(object);

  return ;
}

static void zmap_windowfeatureitemlist_finalize(GObject *object)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(feature_item_parent_class_G);

  if(gobject_class->finalize)
    (*gobject_class->finalize)(object);

  return ;
}

/*
 * (sm23) Note that the second test below to add a feature previously
 * ensured that only features with the same ZMapStyleMode as the zmap_tv
 * could be added. This is incorrect, since we often have (for example)
 * features from featuresets with different style modes in the same
 * column, and wish to display them in the list window.
 */
static void feature_item_add_simple(ZMapGUITreeView zmap_tv,
                            gpointer user_data)
{
  ZMapWindowFeatureItemList zmap_tv_feature;
  ID2Canvas id2c = (ID2Canvas) user_data;
  FooCanvasItem *item = FOO_CANVAS_ITEM(id2c->item);
  ZMapFeature feature = (ZMapFeature) id2c->feature_any;
  AddSimpleDataFeatureItemStruct add_simple = {NULL};

  zmap_tv_feature = ZMAP_WINDOWFEATUREITEMLIST(zmap_tv);

  if((feature))        /* = zmapWindowItemGetFeature(item))) */
    {
      if (zmap_tv_feature->feature_type == ZMAPSTYLE_MODE_INVALID &&
        feature->mode != ZMAPSTYLE_MODE_INVALID)
      {
        zmap_tv_feature->feature_type = feature->mode;
        setup_item_tree(zmap_tv_feature, feature->mode);
      }

        /* Always add the feature & the item... */
      add_simple.feature   = (ZMapFeatureAny)feature;
      add_simple.item      = item;

      if(zmap_tv_feature->window &&
         zmap_tv_feature->window->display_forward_coords)
      add_simple.window  = zmap_tv_feature->window;

      if (  zmap_tv_feature->feature_type != ZMAPSTYLE_MODE_INVALID &&
            /* zmap_tv_feature->feature_type == feature->mode && */
            feature->mode != ZMAPSTYLE_MODE_INVALID &&
            feature_item_parent_class_G->add_tuple_simple)
        {
          (* feature_item_parent_class_G->add_tuple_simple)(zmap_tv, &add_simple);
        }
    }

  return ;
}


static void setup_item_tree(ZMapWindowFeatureItemList zmap_tv,
                      ZMapStyleMode feature_type)
{
  GList *column_titles = NULL;
  GList *column_types  = NULL;
  GList *column_funcs  = NULL;
  GList *column_flags  = NULL;

  feature_item_type_get_titles_types_funcs(feature_type,
                                 &column_titles,
                                 &column_types,
                                 &column_funcs,
                                 &column_flags);

  g_object_set(G_OBJECT(zmap_tv),
             "row-counter-column",  TRUE,
             "data-ptr-column",     FALSE,
             "column_count",        g_list_length(column_titles),
             "column_names",        column_titles,
             "column_types",        column_types,
             "column_funcs",        column_funcs,
             "column_flags_list",   column_flags,
             "sortable",            TRUE,
             NULL);

  zMapGUITreeViewPrepare(ZMAP_GUITREEVIEW(zmap_tv));

  if(column_titles)
    g_list_free(column_titles);
  if(column_types)
    g_list_free(column_types);
  if(column_funcs)
    g_list_free(column_funcs);
  if(column_flags)
    g_list_free(column_flags);

  return ;
}

/* GLists _must_ be freed, but all contents are static (DO NOT FREE) */
static void feature_item_type_get_titles_types_funcs(ZMapStyleMode feature_type,
                                         GList **titles_out,
                                         GList **types_out,
                                         GList **funcs_out,
                                         GList **flags_out)
{
  GList *titles, *types, *funcs, *flags;
  gboolean frame_and_phase = FALSE;
  unsigned int flags_set = (ZMAP_GUITREEVIEW_COLUMN_VISIBLE |
                      ZMAP_GUITREEVIEW_COLUMN_CLICKABLE);

  titles = types = funcs = flags = NULL;

  /* First the generic ones */

  /* N.B. Order here dictates order of columns */

  /* Feature Name */
  titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_NAME_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, (void *)feature_name_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* Feature Start */
  titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_START_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
  funcs  = g_list_append(funcs, (void *)feature_start_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* Feature End */
  titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_END_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
  funcs  = g_list_append(funcs, (void *)feature_end_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* Feature Strand */
  titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_STRAND_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, (void *)feature_strand_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  if(frame_and_phase)
    {
      /* Feature Frame */
      titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_FRAME_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
      funcs  = g_list_append(funcs, (void *)feature_frame_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

      /* Feature Phase */
      titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_PHASE_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
      funcs  = g_list_append(funcs, (void *)feature_phase_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));
    }

  if(feature_type == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      /* Not much */
    }
  else if(feature_type == ZMAPSTYLE_MODE_ALIGNMENT)
    {
      /* Feature Query Start */
      titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_QSTART_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
      funcs  = g_list_append(funcs, (void *)feature_qstart_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

      /* Feature Query End */
      titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_QEND_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
      funcs  = g_list_append(funcs, (void *)feature_qend_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

      /* Feature Query Strand */
      titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_QSTRAND_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
      funcs  = g_list_append(funcs, (void *)feature_qstrand_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

      /* Feature Score */
      titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_SCORE_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_FLOAT));
      funcs  = g_list_append(funcs, (void *)feature_score_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));
    }
  else if(feature_type == ZMAPSTYLE_MODE_BASIC)
    {
      /* Not much */
    }

  /* Feature's feature set  */
  titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_SET_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, (void *)feature_featureset_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* Feature's Type */
  titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_SOURCE_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, (void *)feature_source_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* Columns not visible */

  /* These should be enums, not uints */
  /* FeatureSet Data Strand */
  titles = g_list_append(titles, (void *)ZMAP_WFL_SETDATASTRAND_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_UINT));
  funcs  = g_list_append(funcs, (void *)feature_item_data_strand_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(ZMAP_GUITREEVIEW_COLUMN_NOTHING));

  /* FeatureSet Data Frame*/
  titles = g_list_append(titles, (void *)ZMAP_WFL_SETDATAFRAME_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_UINT));
  funcs  = g_list_append(funcs, (void *)feature_item_data_frame_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(ZMAP_GUITREEVIEW_COLUMN_NOTHING));

  /* Feature pointer */
  /* Using ZMAP_GUITREEVIEW_DATA_PTR_COLUMN_NAME make g_object_get() work :) */
  titles = g_list_append(titles, (void *)ZMAP_GUITREEVIEW_DATA_PTR_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_POINTER));
  funcs  = g_list_append(funcs, (void *)feature_pointer_serialised_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(ZMAP_GUITREEVIEW_COLUMN_NOTHING));


  /* ONLY THIS BIT FOR DEVELOPERS ??????? */
  if(feature_type == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      titles = g_list_append(titles, (void *)"-bump-hidden-");
      types  = g_list_append(types,  GINT_TO_POINTER(G_TYPE_STRING));
      funcs  = g_list_append(funcs,  (void *)featureItem2BumpHidden) ;
      flags  = g_list_append(flags,  GINT_TO_POINTER(flags_set));

      titles = g_list_append(titles, (void *)"-user-hidden-");
      types  = g_list_append(types,  GINT_TO_POINTER(G_TYPE_STRING));
      funcs  = g_list_append(funcs,  (void *)featureItem2UserHidden) ;
      flags  = g_list_append(flags,  GINT_TO_POINTER(flags_set));

      titles = g_list_append(titles, (void *)"-is-visible-");
      types  = g_list_append(types,  GINT_TO_POINTER(G_TYPE_STRING));
      funcs  = g_list_append(funcs,  (void *)featureItem2IsVisible) ;
      flags  = g_list_append(flags,  GINT_TO_POINTER(flags_set));
    }


  /* FeatureSet Data Frame*/
  titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_STYLE_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, (void *)feature_style_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* feature->mode ... */
  titles = g_list_append(titles, (void *)ZMAP_WINDOWFEATURELIST_TYPE_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, (void *)feature_type_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(ZMAP_GUITREEVIEW_COLUMN_NOTHING));


  /* Now return the lists to caller's supplied pointers */
  if(titles_out)
    *titles_out = titles;
  if(types_out)
    *types_out  = types;
  if(funcs_out)
    *funcs_out  = funcs;
  if(flags_out)
    *flags_out  = flags;

  return ;
}

static void feature_item_data_strand_to_value(GValue *value, gpointer feature_item_data)
{
  AddSimpleDataFeatureItem add_data = (AddSimpleDataFeatureItem)feature_item_data;
  FooCanvasItem *feature_item = FOO_CANVAS_ITEM(add_data->item);
  ZMapWindowContainerGroup feature_set_container;
  ZMapWindowContainerFeatureSet container;

  if((feature_set_container = zmapWindowContainerCanvasItemGetContainer(feature_item)))
    {
      container = (ZMapWindowContainerFeatureSet)feature_set_container;

      g_value_set_uint(value, zmapWindowContainerFeatureSetGetStrand(container)) ;
    }
  else
    {
      zMapLogWarning("%s", "Failed to get Parent Contianer.");
    }

  return ;
}

static void feature_item_data_frame_to_value(GValue *value, gpointer feature_item_data)
{
  AddSimpleDataFeatureItem add_data = (AddSimpleDataFeatureItem)feature_item_data;
  FooCanvasItem *feature_item = FOO_CANVAS_ITEM(add_data->item);
  ZMapWindowContainerGroup feature_set_container;
  ZMapWindowContainerFeatureSet container;

  if((feature_set_container = zmapWindowContainerCanvasItemGetContainer(feature_item)))
    {
      container = (ZMapWindowContainerFeatureSet)feature_set_container;

      g_value_set_uint(value, zmapWindowContainerFeatureSetGetFrame(container)) ;
    }
  else
    {
      zMapLogWarning("%s", "Failed to get Parent Contianer.");
    }

  return ;
}

static void feature_pointer_serialised_to_value (GValue *value, gpointer feature_item_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_item_data;
  ZMapFeatureAny        feature = add_data->feature;
  SerialisedFeatureSearch feature_data = NULL;

  if((feature_data = g_new0(SerialisedFeatureSearchStruct, 1)))
    {
      feature_data->feature_id = feature->unique_id;

      if(feature->parent)
      {
        ZMapFeatureSet fset = (ZMapFeatureSet) feature->parent;

        feature_data->set_id = fset->unique_id;
/*        feature_data->set_id = feature->parent->unique_id;*/
        if(feature->parent->parent)
          {
            feature_data->block_id = feature->parent->parent->unique_id;
            if(feature->parent->parent->parent)
            {
              feature_data->align_id = feature->parent->parent->parent->unique_id;
            }
          }
      }
#ifdef DEBUG_FREE_DATA
      zMapShowMsg(ZMAP_MSG_INFORMATION,
              "creating a data struct for feature %s",
              g_quark_to_string(feature_data->feature_id));
#endif /* DEBUG_FREE_DATA */
      /* Someone needs to free this! done in free_serialised_data_cb */
      g_value_set_pointer(value, feature_data);
    }
  else
    {
      zMapShowMsg(ZMAP_MSG_INFORMATION, "%s", "Failed creating cache data");
    }

  return ;
}

static gboolean update_foreach_cb(GtkTreeModel *model,
                          GtkTreePath  *path,
                          GtkTreeIter  *iter,
                          gpointer      user_data)
{
  ModelForeach full_data = (ModelForeach)user_data;
  FooCanvasItem *item;
  /*  ZMapFeature feature;*/
  SerialisedFeatureSearch feature_data = NULL;
  SerialisedFeatureSearchStruct fail_data = {0};
  ZMapFrame frame;
  ZMapStrand strand;
  gboolean result = FALSE;    /* We keep going to visit all the rows */

  /* Attempt to get the item */
  if(!fetch_lookup_data(full_data->feature_list, model, iter,
                  &feature_data, &strand, &frame))
    {
      /* If above fails then use this data to ensure no item
       * found and the row gets removed. */
      fail_data.align_id = fail_data.block_id = 1;
      fail_data.set_id = fail_data.feature_id = 1;
      feature_data = &fail_data;
    }

  /* Here the cached data is the old ids.  This is likely not going to
   * find the item on the canvas if the feature's name has changed.
   * Otherwise, coordinate etc, changes should be handled...
   */
  if(!(item = zmapWindowFToIFindItemFull(full_data->window,full_data->context_to_item,
                               feature_data->align_id,
                               feature_data->block_id,
                               feature_data->set_id,
                               strand, frame,
                               feature_data->feature_id)))
    {
      item = zmapWindowFToIFindItemFull(full_data->window,full_data->context_to_item,
                              feature_data->align_id,
                              feature_data->block_id,
                              feature_data->set_id,
                              (strand == ZMAPSTRAND_FORWARD ? ZMAPSTRAND_REVERSE : ZMAPSTRAND_FORWARD),
                              frame,
                              feature_data->feature_id);
    }

  /* IF, we find the item we can update the tuple from the current
   * feature.  Failure though means we remove the tuple. */
  if(item)
    {
      AddSimpleDataFeatureItemStruct add_simple = { NULL };
      ZMapFeatureAny feature = NULL;

      feature = zmapWindowItemGetFeatureAny(item);

      add_simple.feature = feature;
      add_simple.item    = item;

      if(full_data->feature_list->window &&
       full_data->feature_list->window->display_forward_coords == TRUE)
      add_simple.window = full_data->feature_list->window;

      /* We need to free this data here as it got allocated earlier
       * and will be allocated again (from the feature) as a result
       * of the update */

      free_serialised_data_cb(feature_data);

      zMapGUITreeViewUpdateTuple(ZMAP_GUITREEVIEW(full_data->feature_list), iter, &add_simple);
    }
  else
    {
      GtkTreeRowReference *row_ref;

      zMapShowMsg(ZMAP_MSG_INFORMATION,
              "The row containing feature '%s' will be removed.",
              g_quark_to_string(feature_data->feature_id));

      /* We need to create a row reference here in order to remove
       * rows, otherwise the foreach iterator gets it's knickers in a
       * knot and fails to visit every tuple. */

      row_ref = gtk_tree_row_reference_new(model, path);

      full_data->row_ref_list = g_list_append(full_data->row_ref_list, row_ref);
      full_data->list_incomplete = TRUE;
    }

  return result;
}

static void free_serialised_data_cb(gpointer user_data)
{
  SerialisedFeatureSearch feature_data = (SerialisedFeatureSearch)user_data;

#ifdef DEBUG_FREE_DATA
  zMapShowMsg(ZMAP_MSG_INFORMATION,
            "Freeing data for feature %s",
            g_quark_to_string(feature_data->feature_id));
#endif /* DEBUG_FREE_DATA */

  g_free(feature_data);

  return ;
}

static void invoke_tuple_remove(gpointer list_data, gpointer user_data)
{
  ModelForeach full_data = (ModelForeach)user_data;
  GtkTreeRowReference *row_ref = (GtkTreeRowReference *)(list_data);
  GtkTreePath *path;

  if((path = gtk_tree_row_reference_get_path(row_ref)))
    {
      GtkTreeModel *model;
      GtkTreeIter iter;

      model = full_data->model;

      if(gtk_tree_model_get_iter(model, &iter, path))
      {
        SerialisedFeatureSearch feature_data;

        /* We need to free this data here, as ait got allocated earlier */
        if(fetch_lookup_data(full_data->feature_list, model, &iter,
                         &feature_data, NULL, NULL))
          free_serialised_data_cb(feature_data);

        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
      }

      /* free path??? */
    }

  return ;
}

static gboolean fetch_lookup_data(ZMapWindowFeatureItemList zmap_tv,
                          GtkTreeModel             *model,
                          GtkTreeIter              *iter,
                          SerialisedFeatureSearch  *lookup_data_out,
                          ZMapStrand               *strand_out,
                          ZMapFrame                *frame_out)
{
  SerialisedFeatureSearch feature_data = NULL;
  ZMapFrame frame;
  ZMapStrand strand;
  int data_index, strand_index, frame_index;
  gboolean data_fetched = FALSE;

  g_object_get(G_OBJECT(zmap_tv),
             "data-ptr-index",   &data_index,
             "set-strand-index", &strand_index,
             "set-frame-index",  &frame_index,
             NULL);

  gtk_tree_model_get(model, iter,
                 data_index,   &feature_data,
                 strand_index, &strand,
                 frame_index,  &frame,
                 -1);

  if(feature_data != NULL)
    {
      if(lookup_data_out)
      *lookup_data_out = feature_data;
      if(strand_out)
      *strand_out = strand;
      if(frame_out)
      *frame_out = frame;

      data_fetched = TRUE;
    }

  return data_fetched;
}



static void featureItem2BumpHidden(GValue *value, gpointer feature_item_data)
{
  AddSimpleDataFeatureItem add_data = (AddSimpleDataFeatureItem)feature_item_data ;
  FooCanvasItem *feature_item = FOO_CANVAS_ITEM(add_data->item) ;

  g_value_set_string(value,
                     (zMapWindowContainerFeatureSetHasHiddenBumpFeatures(feature_item) ? "yes" : "no")) ;

  return ;
}


static void featureItem2UserHidden(GValue *value, gpointer feature_item_data)
{
  AddSimpleDataFeatureItem add_data = (AddSimpleDataFeatureItem)feature_item_data;
  FooCanvasItem *feature_item = FOO_CANVAS_ITEM(add_data->item);

  g_value_set_string(value, (zMapWindowContainerFeatureSetIsUserHidden(feature_item) ? "yes" : "no")) ;

  return ;
}


static void featureItem2IsVisible(GValue *value, gpointer feature_item_data)
{
  AddSimpleDataFeatureItem add_data = (AddSimpleDataFeatureItem)feature_item_data;
  FooCanvasItem *feature_item = FOO_CANVAS_ITEM(add_data->item);

  g_value_set_string(value, (zMapWindowContainerFeatureSetIsVisible(feature_item) ? "yes" : "no")) ;

  return ;
}
