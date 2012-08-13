/*  File: zmapWindowFeatureList.c
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implements a sortable list of feature(s) displaying
 *              coords, strand and much else.
 *
 * Exported functions: See zmapWindow_P.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <ZMap/zmapFeature.h>
#include <ZMap/zmapBase.h>
#include <zmapWindowFeatureList_I.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainerUtils.h>

/* agh....there is no way this should be here................ */
#include <zmapWindowContainerFeatureSet_I.h>



#define ZMAP_WFL_SETDATASTRAND_COLUMN_NAME "-set-data-strand-"
#define ZMAP_WFL_SETDATAFRAME_COLUMN_NAME  "-set-data-frame-"

enum
  {
    ZMAP_WFL_NOPROP,          /* zero is invalid property id */

    ZMAP_WFL_FEATURE_TYPE,

    ZMAP_WFL_SETDATA_STRAND_INDEX,
    ZMAP_WFL_SETDATA_FRAME_INDEX,

  };


typedef struct
{
  ZMapFeatureAny feature;
  ZMapWindow     window;
} AddSimpleDataAnyStruct, *AddSimpleDataAny;

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
  parent_class_G = g_type_class_peek_parent(zmap_tv_class);

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
      feature_type = g_value_get_uint(value);

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
     feature->type != ZMAPSTYLE_MODE_INVALID)
    {
      zmap_tv_feature->feature_type = feature->type;
      setup_tree(zmap_tv_feature, feature->type);
    }

  /* Always add the feature... */
  add_simple.feature = (ZMapFeatureAny)feature;

  /* Only add the window if display_forward_coords == TRUE */
  if(zmap_tv_feature->window &&
     zmap_tv_feature->window->display_forward_coords == TRUE)
    add_simple.window = zmap_tv_feature->window;

  if(zmap_tv_feature->feature_type != ZMAPSTYLE_MODE_INVALID &&
     zmap_tv_feature->feature_type == feature->type &&
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
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeatureAny    feature_any = add_data->feature;

  if(G_VALUE_TYPE(value) == G_TYPE_STRING)
    g_value_set_string(value, (char *)g_quark_to_string(feature_any->original_id));
  else
    zMapAssertNotReached();

  return ;
}

static void feature_start_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature_any = (ZMapFeature)(add_data->feature);
  ZMapWindow             window = add_data->window;

  if(G_VALUE_TYPE(value) == G_TYPE_INT)
    {
      switch(feature_any->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        if(window)
          g_value_set_int(value, zmapWindowCoordToDisplay(window, feature_any->x1));
        else
          g_value_set_int(value, feature_any->x1);
        break;
      default:
        g_value_set_int(value, 0);
        break;
      }
    }
  else
    zMapAssertNotReached();

  return ;
}

static void feature_end_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature_any = (ZMapFeature)(add_data->feature);
  ZMapWindow            window  = add_data->window;

  if(G_VALUE_TYPE(value) == G_TYPE_INT)
    {
      switch(feature_any->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        if(window)
          g_value_set_int(value, zmapWindowCoordToDisplay(window, feature_any->x2));
        else
          g_value_set_int(value, feature_any->x2);
        break;
      default:
        g_value_set_int(value, 0);
        break;
      }
    }
  else
    zMapAssertNotReached();

  return ;
}

static void feature_strand_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature_any = (ZMapFeature)(add_data->feature);

  if(G_VALUE_TYPE(value) == G_TYPE_STRING)
    {
      switch(feature_any->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        g_value_set_string(value, zMapFeatureStrand2Str(feature_any->strand));
        break;
      default:
        g_value_set_string(value, ".");
        break;
      }
    }
  else
    zMapAssertNotReached();

  return ;
}

static void feature_frame_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature_any = (ZMapFeature)(add_data->feature);

  if(G_VALUE_TYPE(value) == G_TYPE_STRING)
    {
      switch(feature_any->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
#warning this used to use strand!  and this function i think has never been called
        g_value_set_string(value, zMapFeatureFrame2Str(zMapFeatureFrame(feature_any)));
        break;
      default:
        g_value_set_string(value, ".");
        break;
      }
    }
  else
    zMapAssertNotReached();

  return ;
}

static void feature_phase_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature_any = (ZMapFeature)(add_data->feature);

  if(G_VALUE_TYPE(value) == G_TYPE_STRING)
    {
      switch(feature_any->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
#warning refer to other calls to zMapFeaturePhase2Str()
#warning this used to say strand! phase is buried in obscure places and this function i think has never been called
        g_value_set_string(value, zMapFeaturePhase2Str(zMapFeatureFrame(feature_any)));
        break;
      default:
        g_value_set_string(value, ".");
        break;
      }
    }
  else
    zMapAssertNotReached();

  return ;
}

static void feature_qstart_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature_any = (ZMapFeature)(add_data->feature);

  if(G_VALUE_TYPE(value) == G_TYPE_INT)
    {
      switch(feature_any->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        {
          switch(feature_any->type)
            {
            case ZMAPSTYLE_MODE_ALIGNMENT:
            g_value_set_int(value, feature_any->feature.homol.y1);
            break;
            default:
            g_value_set_int(value, 0);
            break;
            } /* switch(feature->type) */
        }
        break;
      default:
        g_value_set_int(value, 0);
        break;
      } /* switch(feature->struct_type) */
    }
  else
    zMapAssertNotReached();

  return ;
}

static void feature_qend_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature_any = (ZMapFeature)(add_data->feature);


  if(G_VALUE_TYPE(value) == G_TYPE_INT)
    {
      switch(feature_any->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        {
          switch(feature_any->type)
            {
            case ZMAPSTYLE_MODE_ALIGNMENT:
            g_value_set_int(value, feature_any->feature.homol.y2);
            break;
            default:
            g_value_set_int(value, 0);
            break;
            }     /* switch(feature->type) */
        }
        break;
      default:
        g_value_set_int(value, 0);
        break;
      } /* switch(feature->struct_type) */
    }
  else
    zMapAssertNotReached();

  return ;
}

static void feature_qstrand_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature_any = (ZMapFeature)(add_data->feature);

  if(G_VALUE_TYPE(value) == G_TYPE_STRING)
    {
      switch(feature_any->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        {
          switch(feature_any->type)
            {
            case ZMAPSTYLE_MODE_ALIGNMENT:
            g_value_set_string(value, zMapFeatureStrand2Str(feature_any->feature.homol.strand));
            break;
            default:
            g_value_set_string(value, ".");
            break;
            }     /* switch(feature->type) */
        }
        break;
      default:
        g_value_set_string(value, ".");
        break;
      } /* switch(feature->struct_type) */
    }
  else
    zMapAssertNotReached();

  return ;
}

static void feature_score_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature_any = (ZMapFeature)(add_data->feature);

  if(G_VALUE_TYPE(value) == G_TYPE_FLOAT)
    {
      switch(feature_any->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        g_value_set_float(value, feature_any->score);
        break;
      default:
        g_value_set_float(value, 0.0);
        break;
      }
    }
  else
    zMapAssertNotReached();

  return ;
}

static void feature_featureset_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature_any = (ZMapFeature)(add_data->feature);

  if(G_VALUE_TYPE(value) == G_TYPE_STRING)
    {
      switch(feature_any->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        add_data->feature = feature_any->parent;
        feature_name_to_value(value, add_data);
        /* Need to reset this! */
        add_data->feature = (ZMapFeatureAny)feature_any;
        break;
      case ZMAPFEATURE_STRUCT_FEATURESET:
        feature_name_to_value(value, add_data);
        break;
      default:
        break;
      }
    }
  else
    zMapAssertNotReached();

  return ;
}

static void feature_type_to_value(GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature_any = (ZMapFeature)(add_data->feature);

  if(G_VALUE_TYPE(value) == G_TYPE_STRING)
    {
      switch(feature_any->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        g_value_set_string(value, zMapStyleMode2ExactStr(feature_any->type));
        break;
      default:
        break;
      }
    }
  else
    zMapAssertNotReached();

  return ;
}

static void feature_source_to_value (GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature_any = (ZMapFeature)(add_data->feature);

  if(G_VALUE_TYPE(value) == G_TYPE_STRING)
    {
      switch(feature_any->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        g_value_set_string(value, g_quark_to_string(feature_any->source_id));
        break;
      default:
        break;
      }
    }
  else
    zMapAssertNotReached();

  return ;
}

static void feature_style_to_value (GValue *value, gpointer feature_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_data;
  ZMapFeature       feature_any = (ZMapFeature)(add_data->feature);

  if(G_VALUE_TYPE(value) == G_TYPE_STRING)
    {
      switch(feature_any->struct_type)
      {
      case ZMAPFEATURE_STRUCT_FEATURE:
        g_value_set_string(value, g_quark_to_string(feature_any->style_id));
        break;
      default:
        break;
      }
    }
  else
    zMapAssertNotReached();

  return ;
}

static void feature_pointer_to_value(GValue *value, gpointer feature_item_data)
{
  AddSimpleDataFeature add_data = (AddSimpleDataFeature)feature_item_data;
  ZMapFeatureAny        feature = add_data->feature;

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
  titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_NAME_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, feature_name_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* Feature Start */
  titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_START_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
  funcs  = g_list_append(funcs, feature_start_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* Feature End */
  titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_END_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
  funcs  = g_list_append(funcs, feature_end_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* Feature Strand */
  titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_STRAND_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, feature_strand_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  if(frame_and_phase)
    {
      /* Feature Frame */
      titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_FRAME_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
      funcs  = g_list_append(funcs, feature_frame_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

      /* Feature Phase */
      titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_PHASE_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
      funcs  = g_list_append(funcs, feature_phase_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));
    }

  if(feature_type == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      /* Not much */
    }
  else if(feature_type == ZMAPSTYLE_MODE_ALIGNMENT)
    {
      /* Feature Query Start */
      titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_QSTART_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
      funcs  = g_list_append(funcs, feature_qstart_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

      /* Feature Query End */
      titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_QEND_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
      funcs  = g_list_append(funcs, feature_qend_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

      /* Feature Query Strand */
      titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_QSTRAND_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
      funcs  = g_list_append(funcs, feature_qstrand_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

      /* Feature Score */
      titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_SCORE_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_FLOAT));
      funcs  = g_list_append(funcs, feature_score_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));
    }
  else if(feature_type == ZMAPSTYLE_MODE_BASIC)
    {
      /* Not much */
    }

  /* Feature's feature set  */
  titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_SET_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, feature_featureset_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* Feature's Type */
  titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_SOURCE_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, feature_source_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));


  /* Feature pointer */
  /* Using ZMAP_GUITREEVIEW_DATA_PTR_COLUMN_NAME make g_object_get() work :) */
  titles = g_list_append(titles, ZMAP_GUITREEVIEW_DATA_PTR_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_POINTER));
  funcs  = g_list_append(funcs, feature_pointer_to_value);
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
                               GtkTreeIter              *iterator,
                               FooCanvasItem            *feature_item)
{
  SerialisedFeatureSearch feature_data;

  if(fetch_lookup_data(zmap_tv, ZMAP_GUITREEVIEW(zmap_tv)->tree_model,
                   iterator, &feature_data, NULL, NULL))
    free_serialised_data_cb(feature_data);

  zmap_tv->window = window;
  zMapGUITreeViewUpdateTuple(ZMAP_GUITREEVIEW(zmap_tv), iterator, feature_item);
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
                                    GtkTreeIter *iterator)
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

  gtk_tree_model_get(model, iterator,
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
                                    GtkTreeIter *iterator)
{
  ZMapFeature feature = NULL;
  FooCanvasItem *feature_item = NULL;

  if((feature_item = zMapWindowFeatureItemListGetItem(window,zmap_tv, context_to_item, iterator)))
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
  feature_item_parent_class_G = g_type_class_peek_parent(zmap_tv_class);

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
      feature_type = g_value_get_uint(value);

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

static void feature_item_add_simple(ZMapGUITreeView zmap_tv,
                            gpointer user_data)
{
  ZMapWindowFeatureItemList zmap_tv_feature;
  ID2Canvas id2c = (ID2Canvas) user_data;
  FooCanvasItem *item = FOO_CANVAS_ITEM(id2c->item);
  ZMapFeature feature = (ZMapFeature) id2c->feature_any;
  AddSimpleDataFeatureItemStruct add_simple = {NULL};

  zmap_tv_feature = ZMAP_WINDOWFEATUREITEMLIST(zmap_tv);

  if((feature))	// = zmapWindowItemGetFeature(item)))
    {
      if(zmap_tv_feature->feature_type == ZMAPSTYLE_MODE_INVALID &&
       feature->type != ZMAPSTYLE_MODE_INVALID)
      {
        zmap_tv_feature->feature_type = feature->type;
        setup_item_tree(zmap_tv_feature, feature->type);
      }

        /* Always add the feature & the item... */
      add_simple.feature   = (ZMapFeatureAny)feature;
      add_simple.item      = item;

      if(zmap_tv_feature->window &&
       zmap_tv_feature->window->display_forward_coords)
      add_simple.window  = zmap_tv_feature->window;


      if(zmap_tv_feature->feature_type != ZMAPSTYLE_MODE_INVALID &&
       zmap_tv_feature->feature_type == feature->type &&
       feature_item_parent_class_G->add_tuple_simple)
      (* feature_item_parent_class_G->add_tuple_simple)(zmap_tv, &add_simple);
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

#define DEBUG_MISSING_OBJECTS
#ifdef DEBUG_MISSING_OBJECTS
static void feature_item_to_bump_hidden_value(GValue *value, gpointer feature_item_data);
static void feature_item_to_user_hidden_value(GValue *value, gpointer feature_item_data);
static void feature_item_to_is_visible_value (GValue *value, gpointer feature_item_data);
#endif      /* DEBUG_MISSING_OBJECTS */

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
  titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_NAME_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, feature_name_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* Feature Start */
  titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_START_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
  funcs  = g_list_append(funcs, feature_start_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* Feature End */
  titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_END_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
  funcs  = g_list_append(funcs, feature_end_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* Feature Strand */
  titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_STRAND_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, feature_strand_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  if(frame_and_phase)
    {
      /* Feature Frame */
      titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_FRAME_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
      funcs  = g_list_append(funcs, feature_frame_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

      /* Feature Phase */
      titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_PHASE_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
      funcs  = g_list_append(funcs, feature_phase_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));
    }

  if(feature_type == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      /* Not much */
    }
  else if(feature_type == ZMAPSTYLE_MODE_ALIGNMENT)
    {
      /* Feature Query Start */
      titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_QSTART_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
      funcs  = g_list_append(funcs, feature_qstart_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

      /* Feature Query End */
      titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_QEND_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
      funcs  = g_list_append(funcs, feature_qend_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

      /* Feature Query Strand */
      titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_QSTRAND_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
      funcs  = g_list_append(funcs, feature_qstrand_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

      /* Feature Score */
      titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_SCORE_COLUMN_NAME);
      types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_FLOAT));
      funcs  = g_list_append(funcs, feature_score_to_value);
      flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));
    }
  else if(feature_type == ZMAPSTYLE_MODE_BASIC)
    {
      /* Not much */
    }

  /* Feature's feature set  */
  titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_SET_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, feature_featureset_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* Feature's Type */
  titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_SOURCE_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, feature_source_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* Columns not visible */

  /* These should be enums, not uints */
  /* FeatureSet Data Strand */
  titles = g_list_append(titles, ZMAP_WFL_SETDATASTRAND_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_UINT));
  funcs  = g_list_append(funcs, feature_item_data_strand_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(ZMAP_GUITREEVIEW_COLUMN_NOTHING));

  /* FeatureSet Data Frame*/
  titles = g_list_append(titles, ZMAP_WFL_SETDATAFRAME_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_UINT));
  funcs  = g_list_append(funcs, feature_item_data_frame_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(ZMAP_GUITREEVIEW_COLUMN_NOTHING));

  /* Feature pointer */
  /* Using ZMAP_GUITREEVIEW_DATA_PTR_COLUMN_NAME make g_object_get() work :) */
  titles = g_list_append(titles, ZMAP_GUITREEVIEW_DATA_PTR_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_POINTER));
  funcs  = g_list_append(funcs, feature_pointer_serialised_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(ZMAP_GUITREEVIEW_COLUMN_NOTHING));


#ifdef DEBUG_MISSING_OBJECTS
  if(feature_type == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      titles = g_list_append(titles, "-bump-hidden-");
      types  = g_list_append(types,  GINT_TO_POINTER(G_TYPE_STRING));
      funcs  = g_list_append(funcs,  feature_item_to_bump_hidden_value);
      flags  = g_list_append(flags,  GINT_TO_POINTER(flags_set));

      titles = g_list_append(titles, "-user-hidden-");
      types  = g_list_append(types,  GINT_TO_POINTER(G_TYPE_STRING));
      funcs  = g_list_append(funcs,  feature_item_to_user_hidden_value);
      flags  = g_list_append(flags,  GINT_TO_POINTER(flags_set));

      titles = g_list_append(titles, "-is-visible-");
      types  = g_list_append(types,  GINT_TO_POINTER(G_TYPE_STRING));
      funcs  = g_list_append(funcs,  feature_item_to_is_visible_value);
      flags  = g_list_append(flags,  GINT_TO_POINTER(flags_set));
    }
#endif      /* DEBUG_MISSING_OBJECTS */

  /* FeatureSet Data Frame*/
  titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_STYLE_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, feature_style_to_value);
  flags  = g_list_append(flags, GINT_TO_POINTER(flags_set));

  /* feature->type... */
  titles = g_list_append(titles, ZMAP_WINDOWFEATURELIST_TYPE_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, feature_type_to_value);
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
      g_value_set_uint(value, container->strand);
    }
  else
    g_warning("%s", "Failed to get Parent Contianer.");

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
      g_value_set_uint(value, container->frame);
    }
  else
    g_warning("%s", "Failed to get Parent Contianer.");

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

#ifdef DEBUG_MISSING_OBJECTS
static void feature_item_to_bump_hidden_value(GValue *value, gpointer feature_item_data)
{
  AddSimpleDataFeatureItem add_data = (AddSimpleDataFeatureItem)feature_item_data;
  FooCanvasItem *feature_item = FOO_CANVAS_ITEM(add_data->item);
  ZMapWindowContainerGroup feature_set_container;
  ZMapWindowContainerFeatureSet container;

  if((feature_set_container = zmapWindowContainerCanvasItemGetContainer(feature_item)))
    {
      container = (ZMapWindowContainerFeatureSet)feature_set_container;
      char *yes = "yes";
      char *no  = "no";
      g_value_set_string(value, (container->hidden_bump_features ? yes : no));
    }
  else
    g_warning("%s", "Failed to get Parent Contianer.");

  return ;
}

static gint find_item_in_user_hidden_stack(gconstpointer list_data, gconstpointer item_data)
{
  GList *list = (GList *)list_data, *found;;
  gint result = -1;
  if((found = g_list_find(list, item_data)))
    result = 0;
  return result;
}
static gboolean in_user_hidden_stack(GQueue *queue, FooCanvasItem *item)
{
  gboolean result = FALSE;
  GList *found;
  if((found = g_queue_find_custom(queue, item, find_item_in_user_hidden_stack)))
    result = TRUE;
  return result;
}
static void feature_item_to_user_hidden_value(GValue *value, gpointer feature_item_data)
{
  AddSimpleDataFeatureItem add_data = (AddSimpleDataFeatureItem)feature_item_data;
  FooCanvasItem *feature_item = FOO_CANVAS_ITEM(add_data->item);
  ZMapWindowContainerGroup feature_set_container;
  ZMapWindowContainerFeatureSet container;

  if((feature_set_container = zmapWindowContainerCanvasItemGetContainer(feature_item)))
    {
      gboolean in_stack;
      char *yes = "yes";
      char *no  = "no";

      container = (ZMapWindowContainerFeatureSet)feature_set_container;

      in_stack = in_user_hidden_stack(container->user_hidden_stack, feature_item);

      g_value_set_string(value, (in_stack ? yes : no));
    }
  else
    g_warning("%s", "Failed to get Parent Contianer.");

  return ;
}

static void feature_item_to_is_visible_value(GValue *value, gpointer feature_item_data)
{
  AddSimpleDataFeatureItem add_data = (AddSimpleDataFeatureItem)feature_item_data;
  FooCanvasItem *feature_item = FOO_CANVAS_ITEM(add_data->item);
  ZMapWindowContainerGroup feature_set_container;
  ZMapWindowContainerFeatureSet container;

  if((feature_set_container = zmapWindowContainerCanvasItemGetContainer(feature_item)))
    {
      char *yes = "yes";
      char *no  = "no";
      gboolean is_visible = FALSE;

      container = (ZMapWindowContainerFeatureSet)feature_set_container;

      if(feature_item->object.flags & FOO_CANVAS_ITEM_VISIBLE)
      is_visible = TRUE;

      g_value_set_string(value, (is_visible ? yes : no));
    }
  else
    g_warning("%s", "Failed to get Parent Contianer.");

  return ;
}
#endif      /* DEBUG_MISSING_OBJECTS */

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
