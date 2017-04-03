/*  File: zmapFeatureParams.c
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
 * Description: This is an attempt to use the gobject variable parameter
 *              string based resources setting/getting mechanism on
 *              our basic features. It's an experiment but on the whole
 *              not a good idea because we can have 10,000's features
 *              and this method is far too slow. It's use in our code
 *              is on the whole restricted to showing the user details
 *              of features they clicked on so performance in this
 *              instance is irrelevant which is why the code is still
 *              here.
 *
 * Exported functions: See ZMap/zmapFeature.hpp
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>
#include <glib-object.h>
#include <gobject/gvaluecollector.h>
#include <ZMap/zmapFeature.hpp>
#include <ZMap/zmapSO.hpp>
#include <ZMap/zmapStyle.hpp>


#define ZMAP_FEATURE_DATA_NAME "ZMapFeatureDataName"


/* GTypes are just gulongs really */

#define FEATURE_DATA_TYPE_FEATURE_CONTEXT    (ZMAPFEATURE_STRUCT_CONTEXT)
#define FEATURE_DATA_TYPE_FEATURE_ALIGNMENT  (ZMAPFEATURE_STRUCT_ALIGN)
#define FEATURE_DATA_TYPE_FEATURE_BLOCK      (ZMAPFEATURE_STRUCT_BLOCK)
#define FEATURE_DATA_TYPE_FEATURE_FEATURESET (ZMAPFEATURE_STRUCT_FEATURESET)
#define FEATURE_DATA_TYPE_FEATURE            (ZMAPFEATURE_STRUCT_FEATURE)

#define FEATURE_DATA_FEATURE_SHAPE_SHIFT (3) /* Up to 7 context levels, increase if required. */

/* There should be a smaller number of these. */
#define FEATURE_DATA_TYPE_BASIC         (FEATURE_DATA_TYPE_FEATURE + (ZMAPSTYLE_MODE_BASIC         << FEATURE_DATA_FEATURE_SHAPE_SHIFT))
#define FEATURE_DATA_TYPE_ALIGNMENT     (FEATURE_DATA_TYPE_FEATURE + (ZMAPSTYLE_MODE_ALIGNMENT     << FEATURE_DATA_FEATURE_SHAPE_SHIFT))
#define FEATURE_DATA_TYPE_TRANSCRIPT    (FEATURE_DATA_TYPE_FEATURE + (ZMAPSTYLE_MODE_TRANSCRIPT    << FEATURE_DATA_FEATURE_SHAPE_SHIFT))
#define FEATURE_DATA_TYPE_SEQUENCE  (FEATURE_DATA_TYPE_FEATURE + (ZMAPSTYLE_MODE_SEQUENCE  << FEATURE_DATA_FEATURE_SHAPE_SHIFT))
#define FEATURE_DATA_TYPE_ASSEMBLY_PATH (FEATURE_DATA_TYPE_FEATURE + (ZMAPSTYLE_MODE_ASSEMBLY_PATH << FEATURE_DATA_FEATURE_SHAPE_SHIFT))
#define FEATURE_DATA_TYPE_TEXT          (FEATURE_DATA_TYPE_FEATURE + (ZMAPSTYLE_MODE_TEXT          << FEATURE_DATA_FEATURE_SHAPE_SHIFT))
#define FEATURE_DATA_TYPE_GRAPH         (FEATURE_DATA_TYPE_FEATURE + (ZMAPSTYLE_MODE_GRAPH         << FEATURE_DATA_FEATURE_SHAPE_SHIFT))
#define FEATURE_DATA_TYPE_GLYPH         (FEATURE_DATA_TYPE_FEATURE + (ZMAPSTYLE_MODE_GLYPH         << FEATURE_DATA_FEATURE_SHAPE_SHIFT))


#define PROP_DATA_PARAM_LIST(_)                                    \
_(PROP_DATA_ZERO,         , "!invalid",     "zero is invalid", "") \
_(PROP_DATA_NAME,         , "name",         "name",            "") \
_(PROP_DATA_TERM,         , "term",         "term",            "") \
_(PROP_DATA_SOFA_TERM,    , "so-term",      "SO term",         "") \
_(PROP_DATA_TOTAL_LENGTH, , "total-length", "total length",    "") \
_(PROP_DATA_INDEX,        , "index",        "index",           "") \
_(PROP_DATA_START,        , "start",        "start",           "") \
_(PROP_DATA_END,          , "end",          "end",             "") \
_(PROP_DATA_LENGTH,       , "length",       "length",          "") \
_(PROP_DATA_STRAND,       , "strand",       "strand",          "") \
_(PROP_DATA_CDS_LENGTH,   , "cds-length",   "cds length",      "") \
_(PROP_DATA_5_UTR_LENGTH, , "utr-5-length", "5' utr length",   "") \
_(PROP_DATA_3_UTR_LENGTH, , "utr-3-length", "3' utr length",   "") \
_(PROP_DATA_LOCUS,        , "locus",        "locus",           "") \
_(PROP_DATA_QUERY_START,  , "query-start",  "query start",     "") \
_(PROP_DATA_QUERY_END,    , "query-end",    "query end",       "") \
_(PROP_DATA_QUERY_LENGTH, , "query-length", "query length",    "") \
_(PROP_DATA_QUERY_STRAND, , "query-strand", "query strand",    "") \
_(PROP_DATA_EXON_INDEX,   , "exon-index",   "exon index",      "") \
_(PROP_DATA_EXON_START,   , "exon-start",   "exon start",      "") \
_(PROP_DATA_EXON_END,     , "exon-end",     "exon end",        "") \
_(PROP_DATA_EXON_LENGTH,  , "exon-length",  "exon length",     "") \
_(PROP_DATA_EXON_TERM,    , "exon-term",    "exon term",       "") \
_(PROP_DATA_FINAL,        , "!final",       "also invalid",    "")


ZMAP_DEFINE_ENUM(ZMapFeatureDataProperty, PROP_DATA_PARAM_LIST);


typedef struct
{
  ZMapFeatureAny         feature_any;
  ZMapFeatureSubPart sub_feature; /* Can be NULL */
} FeatureSubFeatureStruct, *FeatureSubFeature;

typedef struct
{
  ZMapFeatureDataProperty param_id;
  GType                   g_type;
  int                    *apply_to;
} DataTypeForFeaturesStruct, *DataTypeForFeatures;

typedef struct _zmapFeatureDataClassStruct
{
  GObjectClass __parent__;
  GParamSpecPool *pspec_pool;
} zmapFeatureDataClassStruct, zmapFeatureDataClass, *ZMapFeatureDataClass;

typedef struct _zmapFeatureDataStruct
{
  GObject __parent__;
} zmapFeatureDataStruct, zmapFeatureData, *ZMapFeatureData;

typedef gpointer (*CreateVFunc)(gpointer user_data, guint n_params, GParameter *parameters);
typedef gboolean (*GetFunc)(gpointer user_data, guint param_spec_id, GValue *value, GParamSpec *pspec);


static GType featureAnyGType(ZMapFeatureAny feature_any) ;
static void zmap_feature_data_class_init (ZMapFeatureDataClass data_class);
static void zmap_feature_data_init (ZMapFeatureData data);
static void apply(GParamSpecPool *pool, const char *name,
                  const char *nick, const char *blurb,
                  DataTypeForFeatures children) ;
static gboolean alignment_get_sub_feature_info(gpointer user_data, guint param_spec_id,
                                               GValue *value, GParamSpec *pspec);
static gboolean transcript_get_sub_feature_info(gpointer user_data, guint param_spec_id,
                                                GValue *value, GParamSpec *pspec);
static gboolean basic_get_sub_feature_info(gpointer user_data, guint param_spec_id,
                                           GValue *value, GParamSpec *pspec);
static gboolean invoke_get_func_valist(gpointer        user_data,
                                       GParamSpecPool *pspec_pool,
                                       GType           pool_member_type,
                                       GetFunc         get_func,
                                       const gchar    *first_property_name,
                                       va_list        var_args);
static const char *gtype_to_message_string(GType feature_any_gtype);
static ZMAP_ENUM_TO_SHORT_TEXT_DEC(zmapFeatureDataPropertyNick, ZMapFeatureDataProperty);
bool getAlignSubCoords(ZMapFeatureSubPart sub_feature, GArray *align_coords,  guint param_spec_id, int *value_out) ;
bool getExonCoords(ZMapFeature feature, ZMapFeatureSubPart sub_feature, guint param_spec_id, int *value_out) ;

// 
//                      Globals
//    

static GParamSpecPool *pspec_pool_G = NULL;

static gboolean fail_on_bad_requests_G = FALSE;




// 
//                     External interface
//    


// I'm guessing this needs to be called once somewhere....
//   
GType zMapFeatureDataGetType(void)
{
  static GType type = 0;

  if(type == 0)
    {
      static const GTypeInfo info =
        {
          sizeof (zmapFeatureDataClass),
          (GBaseInitFunc) NULL,
          (GBaseFinalizeFunc) NULL,
          (GClassInitFunc) zmap_feature_data_class_init,
          NULL,           /* class_finalize */
          NULL,           /* class_data */
          sizeof(zmapFeatureData),
          0,              /* n_preallocs */
          (GInstanceInitFunc)zmap_feature_data_init,
          NULL
        };

      type = g_type_register_static (G_TYPE_OBJECT,
                                     ZMAP_FEATURE_DATA_NAME,
                                     &info, (GTypeFlags)0);
    }

  return type;
}


/* GObject style access to feature data.
 *
 * details This makes ZMapFeatures kind of inside out
 * gobjects. ZMapFeatures are _NOT_ and should _NEVER_ be GObjects,
 * but it would be nice to have some of the functionality the GObject
 * code affords. Probably shouldn't be used in a loop! The code
 * inspects the feature and any supplied sub feature for the property
 * names supplied and fills in the pointers supplied just as
 * g_object_get would do.
 *
 * feature_any The ZMapFeatureAny feature to inspect.
 * sub_feature The ZMapFeatureSubPart to inspect (can be NULL)
 * first_property_name name of the first property followed by a NULL terminated
 *             list of other properties, at least the first must be supplied !!.
 *
 * returns TRUE on success, FALSE on failure.
 */

gboolean zMapFeatureGetInfo(ZMapFeatureAny feature_any, ZMapFeatureSubPart sub_feature,
                            const gchar *first_property_name, ...)
{
  ZMapFeature feature;
  GType feature_type = 0;
  GetFunc get_func_pointer = NULL;
  gboolean result = FALSE;
  va_list var_args;

  feature_type = featureAnyGType(feature_any) ;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
        feature = (ZMapFeature)feature_any;
        switch(feature->mode)
          {
          case ZMAPSTYLE_MODE_ALIGNMENT:
            get_func_pointer = alignment_get_sub_feature_info;
            break;
          case ZMAPSTYLE_MODE_TRANSCRIPT:
            get_func_pointer = transcript_get_sub_feature_info;
            break;
          case ZMAPSTYLE_MODE_BASIC:
          default:
            get_func_pointer = basic_get_sub_feature_info;
            break;
          }

        break;
      }
    default:
      {
        break;
      }
    }

  if (get_func_pointer && pspec_pool_G && feature_type != 0)
    {
      FeatureSubFeatureStruct get_info_data = {NULL};
      result = TRUE;

      get_info_data.feature_any = feature_any;
      get_info_data.sub_feature = sub_feature;

      va_start(var_args, first_property_name);

      if(result || (!fail_on_bad_requests_G))
        result = invoke_get_func_valist(&get_info_data, pspec_pool_G, feature_type,
                                        get_func_pointer, first_property_name, var_args);

      va_end(var_args);
    }

  return result;
}



// 
//                 Package routines
//    



// 
//                Internal routines
//    


static GType featureAnyGType(ZMapFeatureAny feature_any)
{
  GType gtype = 0;

  gtype = feature_any->struct_type;

  if(gtype == ZMAPFEATURE_STRUCT_FEATURE)
    gtype += (((ZMapFeature)feature_any)->mode) << FEATURE_DATA_FEATURE_SHAPE_SHIFT ;

  return gtype;
}


static void zmap_feature_data_class_init (ZMapFeatureDataClass data_class)
{
  GParamSpecPool *pspec_pool;

  if((pspec_pool = pspec_pool_G) == NULL)
    {
      int apply_to_all[] = {
        14,/* how many?  */
        FEATURE_DATA_TYPE_FEATURE_CONTEXT,
        FEATURE_DATA_TYPE_FEATURE_ALIGNMENT,
        FEATURE_DATA_TYPE_FEATURE_BLOCK,
        FEATURE_DATA_TYPE_FEATURE_FEATURESET,
        FEATURE_DATA_TYPE_FEATURE,
        FEATURE_DATA_TYPE_BASIC,
        FEATURE_DATA_TYPE_ALIGNMENT,
        FEATURE_DATA_TYPE_TRANSCRIPT,
        FEATURE_DATA_TYPE_SEQUENCE,
        FEATURE_DATA_TYPE_ASSEMBLY_PATH,
        FEATURE_DATA_TYPE_TEXT,
        FEATURE_DATA_TYPE_GRAPH,
        FEATURE_DATA_TYPE_GLYPH,
      };
      int apply_to_alignments[]  = {1, FEATURE_DATA_TYPE_ALIGNMENT};
      int apply_to_transcripts[] = {1, FEATURE_DATA_TYPE_TRANSCRIPT};
      int apply_to_aligns_and_trans[] = {2, FEATURE_DATA_TYPE_ALIGNMENT, FEATURE_DATA_TYPE_TRANSCRIPT};

      DataTypeForFeaturesStruct fname       = { PROP_DATA_NAME,      G_TYPE_STRING, apply_to_all };
      DataTypeForFeaturesStruct term        = { PROP_DATA_TERM,      G_TYPE_STRING, apply_to_all };
      DataTypeForFeaturesStruct so_term     = { PROP_DATA_SOFA_TERM, G_TYPE_STRING, apply_to_all };

      DataTypeForFeaturesStruct total_length = { PROP_DATA_TOTAL_LENGTH, G_TYPE_INT, apply_to_all };
      DataTypeForFeaturesStruct index       = { PROP_DATA_INDEX,  G_TYPE_INT, apply_to_all };
      DataTypeForFeaturesStruct start       = { PROP_DATA_START,  G_TYPE_INT, apply_to_all };
      DataTypeForFeaturesStruct end         = { PROP_DATA_END,    G_TYPE_INT, apply_to_all };
      DataTypeForFeaturesStruct length      = { PROP_DATA_LENGTH, G_TYPE_INT, apply_to_all };
      DataTypeForFeaturesStruct strand      = { PROP_DATA_STRAND, G_TYPE_INT, apply_to_all };

      DataTypeForFeaturesStruct cds_length  = { PROP_DATA_CDS_LENGTH,   G_TYPE_INT, apply_to_transcripts};
      DataTypeForFeaturesStruct utr5_length = { PROP_DATA_5_UTR_LENGTH, G_TYPE_INT, apply_to_transcripts};
      DataTypeForFeaturesStruct utr3_length = { PROP_DATA_3_UTR_LENGTH, G_TYPE_INT, apply_to_transcripts};
      DataTypeForFeaturesStruct locus       = { PROP_DATA_LOCUS,      G_TYPE_STRING, apply_to_transcripts};

      DataTypeForFeaturesStruct query_start  = { PROP_DATA_QUERY_START,  G_TYPE_INT, apply_to_aligns_and_trans};
      DataTypeForFeaturesStruct query_end    = { PROP_DATA_QUERY_END,    G_TYPE_INT, apply_to_aligns_and_trans};

      DataTypeForFeaturesStruct query_length = { PROP_DATA_QUERY_LENGTH, G_TYPE_INT, apply_to_alignments};
      DataTypeForFeaturesStruct query_strand = { PROP_DATA_QUERY_STRAND, G_TYPE_INT, apply_to_alignments};

      DataTypeForFeaturesStruct exon_index       = { PROP_DATA_EXON_INDEX,  G_TYPE_INT, apply_to_all };
      DataTypeForFeaturesStruct exon_start       = { PROP_DATA_EXON_START,  G_TYPE_INT, apply_to_all };
      DataTypeForFeaturesStruct exon_end         = { PROP_DATA_EXON_END,    G_TYPE_INT, apply_to_all };
      DataTypeForFeaturesStruct exon_length      = { PROP_DATA_EXON_LENGTH, G_TYPE_INT, apply_to_all };
      DataTypeForFeaturesStruct exon_term        = { PROP_DATA_EXON_TERM,   G_TYPE_STRING, apply_to_all };

      /* now make the table */
      DataTypeForFeatures full_table[PROP_DATA_FINAL+1] =
        {
          NULL,
          &fname, &term, &so_term,
          &total_length, &index, &start, &end, &length, &strand,
          &cds_length, &utr5_length, &utr3_length, &locus,
          &query_start, &query_end, &query_length, &query_strand,
          &exon_index, &exon_start, &exon_end, &exon_length, &exon_term,
          NULL
      } ;
      DataTypeForFeatures *table; /* and a pointer */
      const char *name, *nick, *blurb = NULL;
      int i;

      pspec_pool = g_param_spec_pool_new(FALSE);

      table = &full_table[0];

      for(i = PROP_DATA_ZERO; i < PROP_DATA_FINAL; i++, table++)
        {
          name  = zmapFeatureDataPropertyNick((ZMapFeatureDataProperty)i);
          nick  = zmapFeatureDataPropertyNick((ZMapFeatureDataProperty)i);
#ifdef RDS_DONT_INCLUDE
          blurb = zmapFeatureDataPropertyBlurb(i);
#endif
          if (name && name[0] != '!' && table[0]->param_id == i) /* filter invalid names */
            {
              apply(pspec_pool, name, nick, blurb, table[0]);
            }
        }

      pspec_pool_G = data_class->pspec_pool = pspec_pool;
    }

  return ;
}


static void zmap_feature_data_init (ZMapFeatureData data)
{
  zMapLogWarning("%s", "not for instantiating !");

  return ;
}


static void apply(GParamSpecPool *pool,
                  const char *name, const char *nick, const char *blurb,
                  DataTypeForFeatures children)
{
  GParamSpec *pspec;
  GType g_type;
  int i, max;
  int *apply_to;

  if((apply_to = children->apply_to) && (max = apply_to[0]))
    {
      apply_to++;
      g_type = children->g_type;

      for(i = 0; i < max; i++, apply_to++)
        {
          if(g_type == G_TYPE_INT)
            {
              pspec = g_param_spec_int(name, nick, blurb, 0, G_MAXINT, 0, G_PARAM_READABLE);
            }
          else if(g_type == G_TYPE_STRING)
            {
              pspec = g_param_spec_string(name, nick, blurb, NULL, G_PARAM_READABLE);
            }
          else
            {
              pspec = NULL;
            }

          if(pspec)
            {
              pspec->param_id = children->param_id;
              g_param_spec_pool_insert(pool, pspec, apply_to[0]);
            }
        }
    }

  return ;
}


/* like g_object_get_valist */
static gboolean invoke_get_func_valist(gpointer        user_data,
                                       GParamSpecPool *pspec_pool,
                                       GType           pool_member_type,
                                       GetFunc         get_func,
                                       const gchar    *first_property_name,
                                       va_list        var_args)
{
  gboolean result = FALSE ;
  const gchar *name ;


  name = first_property_name;

  while (name)
    {
      GValue value = { 0, };
      GParamSpec *pspec;
      gchar *error;

      result = FALSE;

      pspec = g_param_spec_pool_lookup (pspec_pool,
                                        name,
                                        pool_member_type,
                                        TRUE);

      if (fail_on_bad_requests_G && !pspec)
        {
          zMapLogWarning("%s: type `%s' has no property named `%s'",
                         G_STRFUNC,
                         gtype_to_message_string(pool_member_type),
                         name);
          break;
        }
      else if(pspec)
        {
          if (fail_on_bad_requests_G && !(pspec->flags & G_PARAM_READABLE))
            {
              zMapLogWarning("%s: property `%s' of object class `%s' is not readable",
                             G_STRFUNC,
                             pspec->name,
                             gtype_to_message_string(pool_member_type));
              break;
            }

          g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));

          if(get_func)
            result = (get_func)(user_data, pspec->param_id, &value, pspec);

          G_VALUE_LCOPY (&value, var_args, 0, &error);

          if (error)
            {
              zMapLogWarning("%s: %s", G_STRFUNC, error);
              g_free (error);
              g_value_unset (&value);
              break;
            }

          g_value_unset (&value);
        }

      name = va_arg (var_args, gchar*);
    }

  return result;
}


static const char *gtype_to_message_string(GType feature_any_gtype)
{
  static const char *string_array[1 << 16] = {NULL};
  const char *message = NULL;

  if(string_array[FEATURE_DATA_TYPE_FEATURE_CONTEXT] == NULL)
    {
      int i;

      string_array[FEATURE_DATA_TYPE_FEATURE_CONTEXT]    = "ZMapFeatureContext";
      string_array[FEATURE_DATA_TYPE_FEATURE_ALIGNMENT]  = "ZMapFeatureAlignment";
      string_array[FEATURE_DATA_TYPE_FEATURE_BLOCK]      = "ZMapFeatureBlock";
      string_array[FEATURE_DATA_TYPE_FEATURE_FEATURESET] = "ZMapFeatureSet";
      string_array[FEATURE_DATA_TYPE_FEATURE]            = "ZMapFeature";

      for(i = ZMAPSTYLE_MODE_BASIC; i <= ZMAPSTYLE_MODE_GLYPH; i++)
        {
          string_array[FEATURE_DATA_TYPE_FEATURE + (i << FEATURE_DATA_FEATURE_SHAPE_SHIFT)] =
            g_strdup_printf("ZMapFeature [%s]", zMapStyleMode2ExactStr((ZMapStyleMode)i));
        }
    }

  message = string_array[feature_any_gtype];

  return message;
}


static gboolean basic_get_sub_feature_info(gpointer user_data, guint param_spec_id,
                                           GValue *value, GParamSpec *pspec)
{
  gboolean result;

  switch(param_spec_id)
    {
    case PROP_DATA_INDEX:
    case PROP_DATA_START:
    case PROP_DATA_END:
    case PROP_DATA_LENGTH:
      {
        FeatureSubFeature feature_data = (FeatureSubFeature)user_data;
        ZMapFeature feature;

        feature = (ZMapFeature)feature_data->feature_any;

        if (feature_data->sub_feature == NULL)
          {
            if(param_spec_id == PROP_DATA_START)
              g_value_set_int(value, feature->x1);
            else if(param_spec_id == PROP_DATA_END)
              g_value_set_int(value, feature->x2);
            else
              g_value_set_int(value, feature->x2 - feature->x1 + 1);
          }
        else
          {
            if (param_spec_id == PROP_DATA_INDEX)
              g_value_set_int(value, feature_data->sub_feature->index) ;
            else if (param_spec_id == PROP_DATA_START)
              g_value_set_int(value, feature_data->sub_feature->start);
            else if (param_spec_id == PROP_DATA_END)
              g_value_set_int(value, feature_data->sub_feature->end);
            else
              g_value_set_int(value, feature_data->sub_feature->end - feature_data->sub_feature->start + 1);
          }

        result = TRUE;

        break;
      }

    case PROP_DATA_TERM:
    case PROP_DATA_SOFA_TERM:
      {
        FeatureSubFeature feature_data = (FeatureSubFeature)user_data ;
        ZMapFeature feature;

        feature = (ZMapFeature)feature_data->feature_any ;

        if (feature_data->sub_feature == NULL)
          {
            GQuark term_id;

            if (feature->SO_accession)
              {
                term_id = zMapSOAcc2TermID(feature->SO_accession) ;
              }
            else
              {
                switch(feature->mode)
                  {
                  case ZMAPSTYLE_MODE_BASIC:
                    term_id = g_quark_from_string("Basic") ;
                    break ;
                  case ZMAPSTYLE_MODE_TRANSCRIPT:
                    term_id = g_quark_from_string("Transcript") ;
                    break ;
                  case ZMAPSTYLE_MODE_ASSEMBLY_PATH:
                    term_id = g_quark_from_string("Assembly Path") ;
                    break ;
                  case ZMAPSTYLE_MODE_ALIGNMENT:
                    term_id = g_quark_from_string("Alignment") ;
                    break ;
                  case ZMAPSTYLE_MODE_GRAPH:
                    term_id = g_quark_from_string("Graph") ;
                    break ;
                  case ZMAPSTYLE_MODE_GLYPH:
                    term_id = g_quark_from_string("Glyph") ;
                    break ;
                  case ZMAPSTYLE_MODE_TEXT:
                    term_id = g_quark_from_string("Text") ;
                    break ;
                  case ZMAPSTYLE_MODE_SEQUENCE:
                    term_id = g_quark_from_string("Sequence") ;
                    break ;
                  default:
                    term_id = g_quark_from_string("<UNKNOWN>") ;
                    break ;
                  }
              }

            g_value_set_static_string(value, g_quark_to_string(term_id)) ;
          }
        else
          {
            GQuark term_id;

            switch(feature_data->sub_feature->subpart)
              {
              case ZMAPFEATURE_SUBPART_GAP:
                term_id = g_quark_from_string("Gap");
                break;
              case ZMAPFEATURE_SUBPART_MATCH:
                term_id = g_quark_from_string("Match");
                break;
              case ZMAPFEATURE_SUBPART_EXON_CDS:
                term_id = g_quark_from_string("Coding Exon");
                break;
              case ZMAPFEATURE_SUBPART_EXON:
                if (feature->feature.transcript.flags.cds)
                  term_id = g_quark_from_string("Non-coding Exon") ;
                else
                  term_id = g_quark_from_string("Exon") ;
                break;
              case ZMAPFEATURE_SUBPART_INTRON:
              case ZMAPFEATURE_SUBPART_INTRON_CDS:
                term_id = g_quark_from_string("Intron");
                break;
              default:
                term_id = g_quark_from_string("<UNKNOWN>");
                break;
              }

            g_value_set_static_string(value, g_quark_to_string(term_id));
          }

        result = TRUE;

        break;
      }

    default:
      {
        result = FALSE;
        break;
      }
    }

  return result;
}

static gboolean alignment_get_sub_feature_info(gpointer user_data, guint param_spec_id,
                                               GValue *value, GParamSpec *pspec)
{
  gboolean result = FALSE ;
  FeatureSubFeature feature_data = (FeatureSubFeature)user_data ;
  ZMapFeature feature ;

  feature = (ZMapFeature)feature_data->feature_any;

  switch(param_spec_id)
    {
    case PROP_DATA_INDEX:
    case PROP_DATA_START:
    case PROP_DATA_END:
    case PROP_DATA_LENGTH:
    case PROP_DATA_TERM:
    case PROP_DATA_SOFA_TERM:
      {
        // All forwarded to basic feature code.   
        result = basic_get_sub_feature_info(user_data, param_spec_id, value, pspec);

        break;
      }

    case PROP_DATA_TOTAL_LENGTH:
      {
        if (feature->feature.homol.length)
          {
            g_value_set_int(value, feature->feature.homol.length) ;

            result = TRUE ;
          }

        break;
      }

    case PROP_DATA_QUERY_START:
    case PROP_DATA_QUERY_END:
    case PROP_DATA_QUERY_LENGTH:
      {
        ZMapFeatureSubPart sub_feature ;
        GArray *gaps_array ;

        // If there is a subfeature and a gaps array then report coords/length of just that sub match
        // of the alignment match otherwise report total coords/length of whole alignment match.
        if ((sub_feature = feature_data->sub_feature) && (gaps_array  = feature->feature.homol.align))
          {
            int align_value = 0 ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
            if (sub_feature->subpart == ZMAPFEATURE_SUBPART_MATCH)
              {
                ZMapAlignBlock align_block ;
                int i ;

                /* easy case, just look through the gaps array */
                for (i = 0; i < (int)gaps_array->len; i++)
                  {
                    align_block = &(g_array_index(gaps_array, ZMapAlignBlockStruct, i)) ;

                    if (align_block->t1 == sub_feature->start && align_block->t2 == sub_feature->end)
                      {
                        if (param_spec_id == PROP_DATA_QUERY_START)
                          g_value_set_int(value, align_block->q1);
                        else if (param_spec_id == PROP_DATA_QUERY_END)
                          g_value_set_int(value, align_block->q2);
                        else
                          g_value_set_int(value, align_block->q2 - align_block->q1 + 1) ;
                      }
                  }

                result = TRUE ;
              }
            else if (sub_feature->subpart == ZMAPFEATURE_SUBPART_GAP)
              {
                /* need to run through and find matching matches and calculate the gap... */

                // OH GOSH, WE JUST DON'T REPORT THE LENGTH OF THE GAP......   
              }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

            if (getAlignSubCoords(sub_feature, gaps_array, param_spec_id, &align_value))
              {
                g_value_set_int(value, align_value) ;
                result = TRUE ;
              }
          }
        else
          {
            if(param_spec_id == PROP_DATA_QUERY_START)
              g_value_set_int(value, feature->feature.homol.y1);
            else if(param_spec_id == PROP_DATA_QUERY_END)
              g_value_set_int(value, feature->feature.homol.y2);
            else
              g_value_set_int(value, feature->feature.homol.y2 - feature->feature.homol.y1 + 1);

            result = TRUE;
          }
        break;
      }

    case PROP_DATA_QUERY_STRAND:
      {
        g_value_set_int(value, feature->feature.homol.strand);

        result = TRUE;

        break;
      }

    default:
      {
        result = FALSE;

        break;
      }
    }

  return result;
}

static gboolean transcript_get_sub_feature_info(gpointer user_data, guint param_spec_id,
                                                GValue *value, GParamSpec *pspec)
{
  gboolean result = FALSE;
  FeatureSubFeature feature_data = (FeatureSubFeature)user_data ;
  ZMapFeature feature ;
  bool gapped_exon ;
  ZMapFeatureSubPart sub_part ;

  feature = (ZMapFeature)feature_data->feature_any ;
  sub_part = feature_data->sub_feature ;

  // If we are handed a subfeature that is a gap or match then we know it's a gapped_exon.
  gapped_exon = ((sub_part)
                 && (sub_part->subpart ==ZMAPFEATURE_SUBPART_GAP || sub_part->subpart == ZMAPFEATURE_SUBPART_MATCH)) ;

  switch(param_spec_id)
    {
    case PROP_DATA_TERM:
    case PROP_DATA_SOFA_TERM:
      {
        result = basic_get_sub_feature_info(user_data, param_spec_id, value, pspec) ;

        break;
      }
    case PROP_DATA_INDEX:
    case PROP_DATA_START:
    case PROP_DATA_END:
    case PROP_DATA_LENGTH:
      {
        result = basic_get_sub_feature_info(user_data, param_spec_id, value, pspec) ;

        break;
      }

    case PROP_DATA_QUERY_START:
    case PROP_DATA_QUERY_END:
    case PROP_DATA_QUERY_LENGTH:
      {
        if (gapped_exon)
          {
            GArray *exon_aligns ;
            int exon_index ;
            int align_value = 0 ;


            // OK...NEED THIS HERE BUT NOT THE SUB_FEATURE INDEX....WE NEED THE EXON INDEX......
            // Note...doesn't matter if we use start or end for this call as match is guaranteed
            // to be within exon.   

            if (getExonCoords(feature, sub_part, PROP_DATA_EXON_INDEX, &exon_index))
              {
                exon_aligns = zMapFeatureTranscriptGetAlignParts(feature, (exon_index - 1)) ;

                if (getAlignSubCoords(sub_part, exon_aligns, param_spec_id, &align_value))
                  {
                    g_value_set_int(value, align_value) ;
                    result = TRUE ;
                  }
              }
          }

        break;
      }


    case PROP_DATA_EXON_INDEX:
    case PROP_DATA_EXON_START:
    case PROP_DATA_EXON_END:
    case PROP_DATA_EXON_LENGTH:
    case PROP_DATA_EXON_TERM:
      {
        // Given a match subpart return the corresponding exon details....   
        if (gapped_exon)
          {
            if (param_spec_id == PROP_DATA_EXON_TERM)
              {
                g_value_set_static_string(value, "Exon") ;

                result = TRUE ;
              }
            else
              {
                int exon_value = 0 ;
            
                if (getExonCoords(feature, sub_part, param_spec_id, &exon_value))
                  {
                    g_value_set_int(value, exon_value) ;
                    result = TRUE ;
                  }
              }
          }

        break ;
      }

    case PROP_DATA_LOCUS:
      {
        FeatureSubFeature feature_data = (FeatureSubFeature)user_data;
        ZMapFeature feature;

        feature = (ZMapFeature)feature_data->feature_any;

        if (feature->feature.transcript.locus_id)
          g_value_set_static_string(value, g_quark_to_string(feature->feature.transcript.locus_id));


        result = TRUE;

        break;
      }
    default:
      {
        result = FALSE;
        break;
      }
    }

  return result;
}



static ZMAP_ENUM_TO_SHORT_TEXT_FUNC(zmapFeatureDataPropertyNick, ZMapFeatureDataProperty, PROP_DATA_PARAM_LIST);


bool getAlignSubCoords(ZMapFeatureSubPart sub_feature, GArray *align_coords,  guint param_spec_id, int *value_out)
{
  bool result = FALSE ;

  if (sub_feature->subpart == ZMAPFEATURE_SUBPART_MATCH)
    {
      bool done ;
      ZMapAlignBlock align_block ;
      int i ;
      int value = 0 ;

      /* easy case, just look through the gaps array */
      for (i = 0, done = false ; !done && i < (int)align_coords->len; i++)
        {
          align_block = &(g_array_index(align_coords, ZMapAlignBlockStruct, i)) ;

          if (align_block->t1 == sub_feature->start && align_block->t2 == sub_feature->end)
            {
              if (param_spec_id == PROP_DATA_QUERY_START)
                {
                  value = align_block->q1 ;
                  done = true ;
                }
              else if (param_spec_id == PROP_DATA_QUERY_END)
                {
                  value = align_block->q2 ;
                  done = true ;
                }
              else
                {
                  value = (align_block->q2 - align_block->q1) + 1 ;
                  done = true ;
                }
            }
        }

      if (done)
        {
          *value_out = value ;
          result = TRUE ;
        }
    }
  else if (sub_feature->subpart == ZMAPFEATURE_SUBPART_GAP)
    {
      /* need to run through and find matching matches and calculate the gap... */

      // OH GOSH, WE JUST DON'T REPORT THE LENGTH OF THE GAP......   
    }

  return result ;
}


bool getExonCoords(ZMapFeature feature, ZMapFeatureSubPart sub_feature, guint param_spec_id, int *value_out)
{
  bool result = FALSE ;

  if (sub_feature->subpart == ZMAPFEATURE_SUBPART_MATCH || sub_feature->subpart == ZMAPFEATURE_SUBPART_GAP)
    {
      bool done ;
      ZMapSpan exon ;
      int i ;
      int value = 0 ;

      /* look through exons array to see where match or gap is. */
      for (i = 0, done = false ; !done && i < (int)feature->feature.transcript.exons->len ; i++)
        {
          exon = &(g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i)) ;

          if (exon->x1 <= sub_feature->start && exon->x2 >= sub_feature->end)
            {
              switch(param_spec_id)
                {
                case PROP_DATA_EXON_INDEX:
                  value = i + 1 ;
                  done = true ;
                  break ;
                case PROP_DATA_EXON_START:
                  value = exon->x1 ;
                  done = true ;
                  break ;
                case PROP_DATA_EXON_END:
                  value = exon->x2 ;
                  done = true ;
                  break ;
                default: // PROP_DATA_EXON_LENGTH:
                  value = (exon->x2 - exon->x1) + 1 ;
                  done = true ;
                  break ;
                }
            }
        }

      if (done)
        {
          *value_out = value ;
          result = TRUE ;
        }
    }
  else if (sub_feature->subpart == ZMAPFEATURE_SUBPART_GAP)
    {
      /* need to run through and find matching matches and calculate the gap... */

      // OH GOSH, WE JUST DON'T REPORT THE LENGTH OF THE GAP......   
    }

  return result ;
}






#ifdef NEVER_GONNA_BE_FOR_FEATURES

// I DON'T KNOW WHAT THIS REALLY MEANS AND I DON'T HAVE THE TIME TO FIND OUT....I'M LEAVING IT
// HERE BUT ACTUALLY I CAN'T IMAGINE WHEN WE NEED TO DO ANYTHING ABOUT IT.......   


/* like g_object_new_valist */
static gpointer invoke_create_func_valist(gpointer        user_data,
  GParamSpecPool *pspec_pool,
  GType           pool_member_type,
  CreateVFunc     create_func,
  const gchar    *first_property_name,
  va_list         var_args)
{
  gpointer return_pointer = NULL;
  GParameter *params;
  const gchar *name;
  guint n_params = 0, n_alloced_params = 16;

  if(!create_func)
    return return_pointer;

  if (!first_property_name)
    {
      return_pointer = (create_func)(user_data, 0, NULL);
    }
  else
    {
      params = g_new (GParameter, n_alloced_params);
      name   = first_property_name;
      while (name)
        {
          gchar *error = NULL;
          GParamSpec *pspec = g_param_spec_pool_lookup (pspec_pool,
                                                        name,
                                                        pool_member_type,
                                                        TRUE);
          if (!pspec)
            {
              zMapLogWarning("%s: type `%s' has no property named `%s'",
                             G_STRFUNC,
                             g_type_name (pool_member_type),
                             name);
              break;
            }

          if (n_params >= n_alloced_params)
            {
              n_alloced_params += 16;
              params = g_renew (GParameter, params, n_alloced_params);
            }

          params[n_params].name = name;
          params[n_params].value.g_type = 0;
          g_value_init (&params[n_params].value, G_PARAM_SPEC_VALUE_TYPE (pspec));
          G_VALUE_COLLECT (&params[n_params].value, var_args, 0, &error);
          if (error)
            {
              zMapLogWarning("%s: %s", G_STRFUNC, error);
              g_free (error);
              g_value_unset (&params[n_params].value);
              break;
            }
          n_params++;
          name = va_arg (var_args, gchar*);
        }

      return_pointer = (create_func)(user_data, n_params, params);

      while (n_params--)
        {
          g_value_unset (&params[n_params].value);
        }
      g_free (params);

    }

  return return_pointer;

}

#endif /* NEVER_GONNA_BE_FOR_FEATURES */



