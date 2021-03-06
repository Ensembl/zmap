/*  File: zmapFeatureUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Utility routines for handling features/sets/blocks etc.
 *
 * Exported functions: See ZMap/zmapFeature.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>
#include <unistd.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapPeptide.hpp>
#include <ZMap/zmapStyleTree.hpp>
#include <zmapFeature_P.hpp>



typedef struct SimpleParent2ChildDataStructType
{
  ZMapMapBlock map_block;
  int limit_start;
  int limit_end;
  int counter;
} SimpleParent2ChildDataStruct, *SimpleParent2ChildData;


/* Utility struct used for callback when finding featuresets for a given style */
typedef struct GetStyleFeaturesetsStruct_
{
  GList *featuresets ;
  ZMapFeatureTypeStyle style ;
} GetStyleFeaturesetsStruct, *GetStyleFeaturesets ;



static ZMapFrame feature_frame(ZMapFeature feature, int start_coord);
static ZMapFrame feature_frame_coords(int start_coord, int offset);

static void get_feature_list_extent(gpointer list_data, gpointer span_data);

//static ZMapFullExon feature_find_exon_at_coord(ZMapFeature feature, int coord) ;
static ZMapFullExon feature_find_closest_exon_at_coord(ZMapFeature feature, int coord) ;

static gint findStyleName(gconstpointer list_data, gconstpointer user_data) ;
static void addTypeQuark(gpointer style_id, gpointer data, gpointer user_data) ;

static void map_parent2child(gpointer exon_data, gpointer user_data);
static int sortGapsByTarget(gconstpointer a, gconstpointer b) ;

static int span_compare (gconstpointer a, gconstpointer b) ;

static int findExon(ZMapFeature feature, int exon_start, int exon_end) ;
static gboolean calcExonPhase(ZMapFeature feature, int exon_index,
                              int *exon_cds_start, int *exon_cds_end, ZMapPhase *phase_out) ;

static void printChildCB(gpointer key, gpointer value, gpointer user_data_unused) ;

static ZMapFeatureContextExecuteStatus get_style_featuresets_ids_cb(GQuark key, gpointer data,
                                                                    gpointer user_data, char **err_out) ;

static ZMapFeatureContextExecuteStatus get_style_featuresets_cb(GQuark key, gpointer data,
                                                                gpointer user_data, char **err_out) ;


/*!
 * Function to do some validity checking on a ZMapFeatureAny struct. Always more you
 * could do but this is better than nothing.
 *
 * Returns TRUE if the struct is OK, FALSE otherwise.
 *
 * @param   any_feature    The feature to validate.
 * @return  gboolean       TRUE if feature is valid, FALSE otherwise.
 *  */
gboolean zMapFeatureIsValid(ZMapFeatureAny any_feature)
{
  gboolean result = FALSE ;

  if (any_feature
      && zMapFeatureTypeIsValid(any_feature->struct_type)
      && any_feature->unique_id != ZMAPFEATURE_NULLQUARK
      && any_feature->original_id != ZMAPFEATURE_NULLQUARK)
    {
      switch (any_feature->struct_type)
        {
        case ZMAPFEATURE_STRUCT_CONTEXT:
        case ZMAPFEATURE_STRUCT_ALIGN:
        case ZMAPFEATURE_STRUCT_BLOCK:
        case ZMAPFEATURE_STRUCT_FEATURESET:
          result = TRUE ;
          break ;
        case ZMAPFEATURE_STRUCT_FEATURE:
          if (any_feature->children == NULL)
            result = TRUE ;
          break ;
        default:
          zMapWarnIfReached() ;
        }
    }

  return result ;
}

#ifdef NOT_YET
static int get_feature_allowed_types(ZMapStyleMode mode)
{
  int allowed = 0;

  switch(mode)
    {
    case ZMAPSTYLE_MODE_RAW_TEXT:
    case ZMAPSTYLE_MODE_RAW_SEQUENCE:
    case ZMAPSTYLE_MODE_PEP_SEQUENCE:
      allowed = (ZMAPFEATURE_TYPE_BASIC | ZMAPFEATURE_TYPE_TEXT);
      break;
    case ZMAPSTYLE_MODE_TRANSCRIPT:
      allowed = (ZMAPFEATURE_TYPE_BASIC | ZMAPFEATURE_TYPE_TRANSCRIPT);
      break;
    case ZMAPSTYLE_MODE_ALIGNMENT:
      allowed = (ZMAPFEATURE_TYPE_BASIC | ZMAPFEATURE_TYPE_ALIGNMENT);
      break;
    case ZMAPSTYLE_MODE_GRAPH:
    case ZMAPSTYLE_MODE_GLYPH:
      allowed = (ZMAPFEATURE_TYPE_BASIC);
      break;
    case ZMAPSTYLE_MODE_BASIC:
      allowed = (ZMAPFEATURE_TYPE_BASIC);
      break;
    case ZMAPSTYLE_MODE_META:
    case ZMAPSTYLE_MODE_INVALID:
    default:
      break;
    }

  return allowed;
}

gboolean zMapFeatureIsDrawable(ZMapFeatureAny any_feature)
{
  gboolean result = FALSE;

  switch(any_feature->struct_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
        ZMapFeature feature;
        int allowed = ZMAPFEATURE_TYPE_INVALID;

        feature = (ZMapFeature)any_feature;

        if(*feature->style)
          allowed = get_feature_allowed_types();

        if(allowed & feature->mode)
          result = TRUE;
              }
      break;
    default:
      break;
    }

  return result;
}
#endif /* NOT_YET */


/*!
 * Function to do some validity checking on a ZMapFeatureAny struct that in addition
 * checks to see if it is of the requested type.
 *
 * Returns TRUE if the struct is OK, FALSE otherwise.
 *
 * @param   any_feature    The feature to validate.
 * @param   type           The type that the feature must be.
 * @return  gboolean       TRUE if feature is valid, FALSE otherwise.
 *  */
gboolean zMapFeatureIsValidFull(ZMapFeatureAny any_feature, ZMapFeatureLevelType type)
{
  gboolean result = FALSE ;

  if (zMapFeatureIsValid(any_feature) && any_feature->struct_type == type)
    result = TRUE ;

  return result ;
}

gboolean zMapFeatureTypeIsValid(ZMapFeatureLevelType group_type)
{
  gboolean result = FALSE ;

  if (group_type >= ZMAPFEATURE_STRUCT_CONTEXT
      && group_type <= ZMAPFEATURE_STRUCT_FEATURE)
    result = TRUE ;

  return result ;
}

gboolean zMapFeatureIsSane(ZMapFeature feature, char **insanity_explained)
{
  gboolean sane = TRUE ;
  char *insanity = NULL ;

  if (sane)
    {
      if (feature->mode <= ZMAPSTYLE_MODE_INVALID ||
  feature->mode >  ZMAPSTYLE_MODE_META) /* Keep in step with zmapStyle.h */
        {
          insanity = g_strdup_printf("Feature '%s' [%s] has invalid type.", /* keep in step with zmapStyle.h */
                                     (char *)g_quark_to_string(feature->original_id),
                                     (char *)g_quark_to_string(feature->unique_id));
          sane = FALSE;
        }
    }

  if (sane)
    {
      if (feature->x1 > feature->x2)
        {
          insanity = g_strdup_printf("Feature '%s' [%s] has start > end.",
                                     (char *)g_quark_to_string(feature->original_id),
                                     (char *)g_quark_to_string(feature->unique_id));
          sane = FALSE;
        }
    }

  if(sane)
    {
      switch(feature->mode)
        {
        case ZMAPSTYLE_MODE_TRANSCRIPT:
          {
            GArray *an_array;
            ZMapSpan span;
            int i = 0;

            if(sane && (an_array = feature->feature.transcript.exons))
              {
                for(i = 0; sane && i < (int)an_array->len; i++)
                  {
                    span = &(g_array_index(an_array, ZMapSpanStruct, i));
                    if(span->x1 > span->x2)
                      {
                        insanity = g_strdup_printf("Exon %d in feature '%s' has start > end.",
                                                   i + 1,
                                                   (char *)g_quark_to_string(feature->original_id));
                        sane = FALSE;
                      }
                  }
              }

            if(sane && (an_array = feature->feature.transcript.introns))
              {
                for(i = 0; sane && i < (int)an_array->len; i++)
                  {
                    span = &(g_array_index(an_array, ZMapSpanStruct, i));
                    if(span->x1 > span->x2)
                      {
                        insanity = g_strdup_printf("Intron %d in feature '%s' has start > end.",
                                                   i + 1,
                                                   (char *)g_quark_to_string(feature->original_id));
                        sane = FALSE;
                      }
                  }
              }
            if(sane && feature->feature.transcript.flags.cds)
              {
                if(feature->feature.transcript.cds_start > feature->feature.transcript.cds_end)
                  {
                    insanity = g_strdup_printf("CDS for feature '%s' has start > end.",
                                               (char *)g_quark_to_string(feature->original_id));
                    sane = FALSE;
                  }
              }
          }
          break;

        case ZMAPSTYLE_MODE_ALIGNMENT:
        case ZMAPSTYLE_MODE_BASIC:
        case ZMAPSTYLE_MODE_SEQUENCE:
        case ZMAPSTYLE_MODE_TEXT:
        case ZMAPSTYLE_MODE_GRAPH:
        case ZMAPSTYLE_MODE_GLYPH:
          zMapLogWarning("%s", "This part of zMapFeatureIsSane() needs writing!");
          break;
        case ZMAPSTYLE_MODE_META:
          break;
        default:
          zMapWarnIfReached();
          break;
        }
    }

  if(insanity_explained)
    *insanity_explained = insanity;
  else if (insanity)
    g_free(insanity) ;

  return sane;
}


/* we might get off with insanity. */
gboolean zMapFeatureAnyIsSane(ZMapFeatureAny feature, char **insanity_explained)
{
  gboolean sane = TRUE ;
  char *insanity = NULL ;

  if (sane && !zMapFeatureIsValid(feature))
    {
      if (feature->original_id == ZMAPFEATURE_NULLQUARK)
        insanity = g_strdup("Feature has bad name.") ;
      else if (feature->unique_id == ZMAPFEATURE_NULLQUARK)
        insanity = g_strdup("Feature has bad identifier.") ;
      else
        insanity = g_strdup_printf("Feature '%s' [%s] has bad type.",
                                   (char *)g_quark_to_string(feature->original_id),
                                   (char *)g_quark_to_string(feature->unique_id));

      sane = FALSE ;
    }

  if (sane)
    {
      switch(feature->struct_type)
        {
        case ZMAPFEATURE_STRUCT_FEATURE:
          {
            ZMapFeature real_feature = (ZMapFeature)feature;

            sane = zMapFeatureIsSane(real_feature, &insanity);

    break;
          }
        case ZMAPFEATURE_STRUCT_CONTEXT:
          zMapLogWarning("%s", "This part of zMapFeatureAnyIsSane() needs writing!");
          break;
        case ZMAPFEATURE_STRUCT_ALIGN:
        case ZMAPFEATURE_STRUCT_BLOCK:
        case ZMAPFEATURE_STRUCT_FEATURESET:
          /* Nothing to check beyond ZMapFeatureAny basics */
          break;
        default:
          sane = FALSE ;
          insanity = g_strdup_printf("Feature '%s' [%s] has bad type.",
                                     (char *)g_quark_to_string(feature->original_id),
                                     (char *)g_quark_to_string(feature->unique_id));

          zMapWarnIfReached();
          break;
        }
    }

  if (insanity)
    {
      if (insanity_explained)
        *insanity_explained = insanity;
      else
        g_free(insanity);
    }

  return sane;
}




void zMapFeatureRevComp(int seq_start, int seq_end, int *coord_1, int *coord_2)
{
  zmapFeatureRevComp(seq_start, seq_end, coord_1, coord_2) ;

  return ;
}





/* Returns the original name of any feature type. The returned string belongs
 * to the feature and must _NOT_ be free'd. This function can never return
 * NULL as all features must have valid names. */
char *zMapFeatureName(ZMapFeatureAny any_feature)
{
  char *feature_name = NULL ;

  zMapReturnValIfFail(zMapFeatureIsValid(any_feature), NULL) ;

  feature_name = (char *)g_quark_to_string(any_feature->original_id) ;

  return feature_name ;
}


/* Returns the unique name of any feature type. The returned string belongs
 * to the feature and must _NOT_ be free'd. This function can never return
 * NULL as all features must have valid unique names. */
char *zMapFeatureUniqueName(ZMapFeatureAny any_feature)
{
  char *feature_name = NULL ;

  zMapReturnValIfFail(zMapFeatureIsValid(any_feature), NULL) ;

  feature_name = (char *)g_quark_to_string(any_feature->unique_id) ;

  return feature_name ;
}


/*!
 * Does a case <i>insensitive</i> comparison of the features name and
 * the supplied name, return TRUE if they are the same.
 *
 * @param   any_feature    The feature.
 * @param   name           The name to be compared..
 * @return  gboolean       TRUE if the names are the same.
 *  */
gboolean zMapFeatureNameCompare(ZMapFeatureAny any_feature, char *name)
{
  gboolean result = FALSE ;

  if (!zMapFeatureIsValid(any_feature) || !name || !*name)
    return result ;

  if (g_ascii_strcasecmp(g_quark_to_string(any_feature->original_id), name) == 0)
    result = TRUE ;

  return result ;
}


/*!
 * Function to return the _parent_ group of group_type of the feature any_feature.
 * This is a generalised function to stop all the poking about through the context
 * hierachy that is otherwise required. Note you can only go _UP_ the tree with
 * this function because going down is a one-to-many mapping.
 *
 * Returns the feature group or NULL if there is no parent group or there is some problem
 * with the arguments like asking for a group at or below the level of any_feature.
 *
 * @param   any_feature    The feature for which you wish to find the parent group.
 * @param   group_type     The type/level of the parent group you want to find.
 * @return  ZMapFeatureAny The parent group or NULL.
 *  */
ZMapFeatureAny zMapFeatureGetParentGroup(ZMapFeatureAny any_feature, ZMapFeatureLevelType group_type)
{
  ZMapFeatureAny result = NULL ;

  if (zMapFeatureIsValid(any_feature) &&
      group_type >= ZMAPFEATURE_STRUCT_CONTEXT &&
      group_type <= ZMAPFEATURE_STRUCT_FEATURE &&
      any_feature->struct_type >= group_type)
    {
      ZMapFeatureAny group = any_feature ;

      while (group && group->struct_type > group_type)
        {
          group = group->parent ;
        }

      result = group ;
    }
  else
    {
      g_warn_if_fail(zMapFeatureIsValid(any_feature));
      g_warn_if_fail(group_type >= ZMAPFEATURE_STRUCT_CONTEXT && group_type <= ZMAPFEATURE_STRUCT_FEATURE);
    }

  return result ;
}




/* Given a feature name produce the canonicalised name as used that is used in producing
 * unique feature names.
 *
 * NOTE that the name is canonicalised in place so caller must provide a string for this
 * to be done in.
 *  */
char *zMapFeatureCanonName(char *feature_name)
{
  char *ptr = NULL ;
  int len ;

  if (!feature_name || !*feature_name)
    return ptr ;

  len = strlen(feature_name);

  /* lower case the feature name, only the feature part though,
   * numbers don't matter. Here we do as g_strdown does, but in place
   * rather than a g_strdup first. */
  for(ptr = feature_name; ptr <= feature_name + len; ptr++)
    {
      *ptr = g_ascii_tolower(*ptr);
    }

  return feature_name ;
}



/* This function creates a unique id for a feature. This is essential if we are to use the
 * g_hash_table package to hold and reference features. Code should _ALWAYS_ use this function
 * to produce these IDs.
 *
 * Caller must g_free() returned string when finished with.
 *
 * OK, I THINK THERE IS A PROBLEM HERE...WE COULD HAVE FEATURES WITH SAME COORDS ON
 * OPPOSITE STRAND WHILE AN ANNOTATOR IS EDITING FEATURES....DOH.....
 *
 */
char *zMapFeatureCreateName(ZMapStyleMode feature_type,
                            const char *feature_name,
                            ZMapStrand strand, int start, int end, int query_start, int query_end)
{
  char *feature_unique_name = NULL ;
  const char *strand_str ;
  char *ptr ;
  int len ;

  if (!feature_type || !feature_name || !*feature_name)
    return feature_unique_name ;

  strand_str = zMapFeatureStrand2Str(strand) ;

  if (feature_type == ZMAPSTYLE_MODE_ALIGNMENT)
    feature_unique_name = g_strdup_printf("%s_'%s'_%d.%d_%d.%d", feature_name,
                                          strand_str, start, end, query_start, query_end) ;
  else
    feature_unique_name = g_strdup_printf("%s_'%s'_%d.%d", feature_name, strand_str, start, end) ;

  /* lower case the feature name, only the feature part though,
   * numbers don't matter. Here we do as g_strdown does, but in place
   * rather than a g_strdup first. */
  len = strlen(feature_name) ;
  for (ptr = feature_unique_name ; ptr <= feature_unique_name + len ; ptr++)
    {
      *ptr = g_ascii_tolower(*ptr) ;
    }

  return feature_unique_name ;
}


/* Like zMapFeatureCreateName() but returns a quark representing the feature name. */
GQuark zMapFeatureCreateID(ZMapStyleMode feature_type,
                           const char *feature,
                           ZMapStrand strand, int start, int end,
                           int query_start, int query_end)
{
  GQuark feature_id = 0 ;
  char *feature_name ;

  if ((feature_name = zMapFeatureCreateName(feature_type, feature, strand, start, end,
    query_start, query_end)))
    {
      feature_id = g_quark_from_string(feature_name) ;
      g_free(feature_name) ;
    }

  return feature_id ;
}


GQuark zMapFeatureBlockCreateID(int ref_start, int ref_end, ZMapStrand ref_strand,
                                int non_start, int non_end, ZMapStrand non_strand)
{
  GQuark block_id = 0;
  char *id_base ;

  id_base = g_strdup_printf("%d.%d.%s_%d.%d.%s",
    ref_start, ref_end,
    (ref_strand == ZMAPSTRAND_FORWARD ? "+" : "-"),
    non_start, non_end,
    (non_strand == ZMAPSTRAND_FORWARD ? "+" : "-")) ;
  block_id = g_quark_from_string(id_base) ;
  g_free(id_base) ;

  return block_id;
}

gboolean zMapFeatureBlockDecodeID(GQuark id,
                                  int *ref_start, int *ref_end, ZMapStrand *ref_strand,
                                  int *non_start, int *non_end, ZMapStrand *non_strand)
{
  gboolean valid = FALSE ;
  char *block_id ;
  const char *format_str = "%d.%d.%1c_%d.%d.%1c" ;
  char ref_strand_str[2] = {'\0'}, non_strand_str[2] = {'\0'} ;
  int fields ;
  enum {EXPECTED_FIELDS = 6} ;

  block_id = (char *)g_quark_to_string(id) ;

  if ((fields = sscanf(block_id, format_str,
       ref_start, ref_end, &ref_strand_str[0],
       non_start, non_end, &non_strand_str[0])) != EXPECTED_FIELDS)
    {
      *ref_start = 0 ;
      *ref_end   = 0 ;
      *non_start = 0 ;
      *non_end   = 0 ;
    }
  else
    {
      zMapFeatureFormatStrand(&ref_strand_str[0], ref_strand) ;
      zMapFeatureFormatStrand(&non_strand_str[0], non_strand) ;
      valid = TRUE ;
    }

  return valid ;
}

GQuark zMapFeatureSetCreateID(const char *set_name)
{
  return zMapStyleCreateID(set_name);
}


void zMapFeatureSortGaps(GArray *gaps)
{
  if (gaps)
    {
      /* Sort the array of gaps. performance hit? */
      g_array_sort(gaps, sortGapsByTarget);
    }

  return ;
}



/* In zmap we hold coords in the forward orientation always and get strand from the strand
 * member of the feature struct. This function looks at the supplied strand and sets the
 * coords accordingly. */
/* ACTUALLY I REALISE I'M NOT QUITE SURE WHAT I WANT THIS FUNCTION TO DO.... */
gboolean zMapFeatureSetCoords(ZMapStrand strand, int *start, int *end, int *query_start, int *query_end)
{
  gboolean result = FALSE ;

  if (!start || !end || !query_start || !query_end)
    return result ;

  if (strand == ZMAPSTRAND_REVERSE)
    {
      if ((start && end) && start > end)
        {
          int tmp ;

          tmp = *start ;
          *start = *end ;
          *end = tmp ;

          if (query_start && query_end)
            {
              tmp = *query_start ;
              *query_start = *query_end ;
              *query_end = tmp ;
            }

          result = TRUE ;
        }
    }
  else
    result = TRUE ;


  return result ;
}


char *zMapFeatureSetGetName(ZMapFeatureSet feature_set)
{
  char *set_name ;

  set_name = (char *)g_quark_to_string(feature_set->original_id) ;

  return set_name ;
}



/* Retrieve a style struct for the given style id. */
ZMapFeatureTypeStyle zMapFindStyle(GHashTable *styles, GQuark style_id)
{
  ZMapFeatureTypeStyle style = NULL ;

  style = (ZMapFeatureTypeStyle) g_hash_table_lookup(styles, GUINT_TO_POINTER(style_id));

  return style ;
}


ZMapFeatureTypeStyle zMapFindFeatureStyle(GHashTable *styles, GQuark style_id, ZMapStyleMode feature_type)
{
  ZMapFeatureTypeStyle feature_style = NULL ;
  char *type;

  if(!(feature_style = zMapFindStyle(styles, style_id)))
  {
    /* feature_style_id is as configured or defaults to the same name as the featureset
     * if not defined try a style with the same name as the feature type
     */

    type = (char *) zMapStyleMode2ExactStr(feature_type);
    style_id = zMapStyleCreateID(type);
    feature_style = zMapFindStyle(styles, style_id);

    if (!feature_style)
      {
        /* Try again with the short text version of the style name (e.g. "basic" instead of
         * "zmapstyle_mode_basic" */
        style_id = g_quark_from_string(zMapStyleMode2ShortText(feature_type)) ;
        feature_style = zMapFindStyle(styles, style_id) ;
      }
  }
  return feature_style;
}


ZMapFeatureTypeStyle zMapFindFeatureStyle(ZMapStyleTree *styles, GQuark style_id, ZMapStyleMode feature_type)
{
  ZMapFeatureTypeStyle style = NULL ;

  if (styles)
    style = zMapFindFeatureStyle(*styles, style_id, feature_type) ;

  return style ;
}


ZMapFeatureTypeStyle zMapFindFeatureStyle(ZMapStyleTree &styles, GQuark style_id, ZMapStyleMode feature_type)
{
  ZMapFeatureTypeStyle feature_style = NULL ;
  char *type;

  if(!(feature_style = styles.find_style(style_id)))
  {
    /* feature_style_id is as configured or defaults to the same name as the featureset
     * if not defined try a style with the same name as the feature type
     */

    type = (char *) zMapStyleMode2ExactStr(feature_type);
    style_id = zMapStyleCreateID(type);
    feature_style = styles.find_style(style_id);

    if (!feature_style)
      {
        /* Try again with the short text version of the style name (e.g. "basic" instead of
         * "zmapstyle_mode_basic" */
        style_id = g_quark_from_string(zMapStyleMode2ShortText(feature_type)) ;
        feature_style = styles.find_style(style_id) ;
      }
  }
  return feature_style;
}


/* Check that a style name exists in a list of styles. */
gboolean zMapStyleNameExists(GList *style_name_list, char *style_name)
{
  gboolean result = FALSE ;
  GList *glist ;
  GQuark style_id ;

  style_id = zMapStyleCreateID(style_name) ;

  if ((glist = g_list_find_custom(style_name_list, GUINT_TO_POINTER(style_id), findStyleName)))
    result = TRUE ;

  return result ;
}



/* Retrieve a Glist of the names of all the styles... */
GList *zMapStylesGetNames(GHashTable *styles)
{
  GList *quark_list = NULL ;

  if (styles)
    g_hash_table_foreach(styles, addTypeQuark, (void *)&quark_list) ;

  return quark_list ;
}


/* Get the list of featureset IDs (as GQuarks) that use the given style. Result
 * should be free'd by the caller with g_list_free. */
GList* zMapStyleGetFeaturesetsIDs(ZMapFeatureTypeStyle style, ZMapFeatureAny feature_any)
{
  GList *result = NULL ;

  GetStyleFeaturesetsStruct get_data = {NULL, style} ;

  zMapFeatureContextExecute(feature_any, ZMAPFEATURE_STRUCT_FEATURESET,
                            get_style_featuresets_ids_cb, &get_data) ;

  result = get_data.featuresets ;

  return result ;
}


/* Get the list of featuresets (as ZMapFeatureSet structs) that use the given style. Result
 * should be free'd by the caller with g_list_free. */
GList* zMapStyleGetFeaturesets(ZMapFeatureTypeStyle style, ZMapFeatureAny feature_any)
{
  GList *result = NULL ;

  GetStyleFeaturesetsStruct get_data = {NULL, style} ;

  zMapFeatureContextExecute(feature_any, ZMAPFEATURE_STRUCT_FEATURESET,
                            get_style_featuresets_cb, &get_data) ;

  result = get_data.featuresets ;

  return result ;
}


/* Callback to add this featureset ID to the result list if it has the given style */
static ZMapFeatureContextExecuteStatus get_style_featuresets_ids_cb(GQuark key, gpointer data,
                                                                    gpointer user_data, char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny) data ;
  GetStyleFeaturesets get_data = (GetStyleFeaturesets)user_data ;

  if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURESET)
    {
      ZMapFeatureSet feature_set = (ZMapFeatureSet)feature_any ;

      if (feature_set->style == get_data->style)
        get_data->featuresets = g_list_append(get_data->featuresets, GINT_TO_POINTER(feature_set->original_id)) ;
    }

  return status ;
}


/* Callback to add this featureset to the result list if it has the given style */
static ZMapFeatureContextExecuteStatus get_style_featuresets_cb(GQuark key, gpointer data,
                                                                gpointer user_data, char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny) data ;
  GetStyleFeaturesets get_data = (GetStyleFeaturesets)user_data ;

  if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURESET)
    {
      ZMapFeatureSet feature_set = (ZMapFeatureSet)feature_any ;

      if (feature_set->style == get_data->style)
        get_data->featuresets = g_list_append(get_data->featuresets, feature_set) ;
    }

  return status ;
}


/* GFunc() callback function, appends style names to a string, its called for lists
 * of either style name GQuarks or lists of style structs. */
static void addTypeQuark(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;
  GList **quarks_out = (GList **)user_data ;
  GList *quark_list = *quarks_out ;

  quark_list = g_list_append(quark_list, GUINT_TO_POINTER(zMapStyleGetUniqueID(style)));

  *quarks_out = quark_list ;

  return ;
}


/*
 * For new columns that appear out of nowhere put them on the right
 * this does not get reset on a new view, but with 100 columns it will take a very long time to wrap round
 * We can be called with the reset flag to start again from 0. In this case, the return value
 * will be 0, which is not a valid value and should not be used. 
*/
int zMapFeatureColumnOrderNext(const gboolean reset)
{
  int result = 0;
  static int which = 0;

  if (reset)
    {
      which = 0 ;
      result = 0 ; /* result should not be used when resetting */
    }
  else 
    {
      ++which ; /* 0 is invalid */
      result = which ;
    }

  return result ;
}


/* For blocks within alignments other than the master alignment, it is not possible to simply
 * use the x1,x2 positions in the feature struct as these are the positions in the original
 * feature. We need to know the coordinates in the master alignment. (ie as in the parent span) */
void zMapFeature2MasterCoords(ZMapFeature feature, double *feature_x1, double *feature_x2)
{
  double master_x1 = 0.0, master_x2 = 0.0 ;
  ZMapFeatureBlock block ;
  double feature_offset = 0.0 ;

  if (!feature->parent || !feature->parent->parent)
    return ;

  block = (ZMapFeatureBlock)feature->parent->parent ;

  feature_offset = block->block_to_sequence.parent.x1 - block->block_to_sequence.block.x1;

  master_x1 = feature->x1 + feature_offset ;
  master_x2 = feature->x2 + feature_offset ;

  *feature_x1 = master_x1 ;
  *feature_x2 = master_x2 ;

  return ;
}



void zMapFeature2BlockCoords(ZMapFeatureBlock block, int *x1_inout, int *x2_inout)
{
  int offset ;

  offset = block->block_to_sequence.block.x1 ;

  if (x1_inout)
    *x1_inout = (*x1_inout - offset) + 1 ;

  if (x2_inout)
    *x2_inout = (*x2_inout - offset) + 1 ;

  return ;
}



void zMapBlock2FeatureCoords(ZMapFeatureBlock block, int *x1_inout, int *x2_inout)
{
  int offset ;

  offset = block->block_to_sequence.block.x1 ;

  if (x1_inout)
    *x1_inout = (*x1_inout + offset) - 1 ;

  if (x2_inout)
    *x2_inout = (*x2_inout + offset) - 1 ;

  return ;
}





gboolean zMapFeatureGetFeatureListExtent(GList *feature_list, int *start_out, int *end_out)
{
  gboolean done = FALSE;
  ZMapSpanStruct span = {0};
  ZMapFeature feature;

  if(feature_list && (feature = (ZMapFeature)(g_list_nth_data(feature_list, 0))))
    {
      span.x1 = feature->x1;
      span.x2 = feature->x2;

      g_list_foreach(feature_list, get_feature_list_extent, &span);

      if(start_out)
        *start_out = span.x1;

      if(end_out)
        *end_out = span.x2;

      done = TRUE;
    }

  return done;
}



GArray *zMapFeatureWorld2TranscriptArray(ZMapFeature feature)
{
  GArray *t_array = NULL, *exon_array;

  if(ZMAPFEATURE_HAS_EXONS(feature))
    {
      ZMapSpanStruct span;
      int i;

      t_array    = g_array_sized_new(FALSE, FALSE, sizeof(ZMapSpanStruct), 128);
      exon_array = feature->feature.transcript.exons;

      for(i = 0; i < (int)exon_array->len; i++)
      {
        span = g_array_index(exon_array, ZMapSpanStruct, i);
        zMapFeatureWorld2Transcript(feature, span.x1, span.x2, &(span.x1), &(span.x2));
        t_array = g_array_append_val(t_array, span);
      }
    }

  return t_array;
}


gboolean zMapFeatureWorld2Transcript(ZMapFeature feature,
     int w1, int w2,
     int *t1, int *t2)
{
  gboolean is_transcript = FALSE;

  if(feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      if(feature->x1 > w2 || feature->x2 < w1)
            is_transcript = FALSE;
      else
          {
          SimpleParent2ChildDataStruct parent_data = { NULL };
          ZMapMapBlockStruct map_data = { {0,0}, {0,0}, FALSE };
          map_data.parent.x1 = w1;
          map_data.parent.x2 = w2;

          parent_data.map_block   = &map_data;
          parent_data.limit_start = feature->x1;
          parent_data.limit_end   = feature->x2;
          parent_data.counter     = 0;

          zMapFeatureTranscriptExonForeach(feature, map_parent2child,
                                           &parent_data);
          if(t1)
            *t1 = map_data.block.x1;
          if(t2)
            *t2 = map_data.block.x2;

          is_transcript = TRUE;
        }
    }
  else
    is_transcript = FALSE;

  return is_transcript;
}


/* Sort introns/exons in ascending order. */
gboolean zMapFeatureTranscriptSortExons(ZMapFeature feature)
{
  gboolean result = FALSE ;

  if (!zMapFeatureIsValid((ZMapFeatureAny)feature))
    return result ;

  if (ZMAPFEATURE_IS_TRANSCRIPT(feature)
      && (ZMAPFEATURE_HAS_EXONS(feature) || ZMAPFEATURE_HAS_INTRONS(feature)))
    {
      if (ZMAPFEATURE_HAS_EXONS(feature))
        g_array_sort(feature->feature.transcript.exons, span_compare) ;

      if (ZMAPFEATURE_HAS_INTRONS(feature))
        g_array_sort(feature->feature.transcript.introns, span_compare) ;

      result = TRUE ;
    }

  return result ;
}




/* Returns FALSE if not a transcript feature or no exons, TRUE otherwise. */
gboolean zMapFeatureTranscriptExonForeach(ZMapFeature feature, GFunc function, gpointer user_data)
{
  gboolean result = FALSE ;

  result = zMapFeatureTranscriptChildForeach(feature, ZMAPFEATURE_SUBPART_EXON,
     function, user_data) ;

  return result ;
}


/* Returns FALSE if not a transcript feature or no exons/introns, TRUE otherwise. */
gboolean zMapFeatureTranscriptChildForeach(ZMapFeature feature, ZMapFeatureSubPartType child_type,
   GFunc function, gpointer user_data)
{
  gboolean result = FALSE ;

  if (!zMapFeatureIsValid((ZMapFeatureAny)feature) || !function)
    return result ;

  if (ZMAPFEATURE_IS_TRANSCRIPT(feature)
      && ((child_type == ZMAPFEATURE_SUBPART_EXON && ZMAPFEATURE_HAS_EXONS(feature))
          || (child_type == ZMAPFEATURE_SUBPART_INTRON && ZMAPFEATURE_HAS_INTRONS(feature))))
    {
      GArray *children ;
      unsigned index ;
      int multiplier = 1, start = 0, end, i ;
      gboolean forwd = TRUE ;

      if (child_type == ZMAPFEATURE_SUBPART_EXON)
        children = feature->feature.transcript.exons ;
      else
        children = feature->feature.transcript.introns ;


      /* Sort this out...should be a much simpler way of doing this..... */
      if (children->len > 1)
        {
          ZMapSpan first, last ;

          first = &(g_array_index(children, ZMapSpanStruct, 0));
          last  = &(g_array_index(children, ZMapSpanStruct, children->len - 1));

          if (first->x1 > last->x1)
            forwd = FALSE ;
        }

      if (forwd)
        {
          end = children->len ;
        }
      else
        {
          multiplier = -1 ;
          start = (children->len * multiplier) + 1 ;
          end = 1 ;
        }

      for (i = start; i < end; i++)
        {
          ZMapSpan child_span ;

          index = i * multiplier ;

          child_span = &(g_array_index(children, ZMapSpanStruct, index)) ;

          (function)(child_span, user_data) ;
        }

      result = TRUE ;
    }

  return result ;
}


void zMapFeatureTranscriptIntronForeach(ZMapFeature feature, GFunc function, gpointer user_data)
{
  GArray *introns;
  unsigned index;
  int multiplier = 1, start = 0, end, i;
  gboolean forwd = TRUE;

  if (feature->mode != ZMAPSTYLE_MODE_TRANSCRIPT)
    return ;

  introns = feature->feature.transcript.introns;

  if (introns->len > 1)
    {
      ZMapSpan first, last;
      first = &(g_array_index(introns, ZMapSpanStruct, 0));
      last  = &(g_array_index(introns, ZMapSpanStruct, introns->len - 1));

      if(first->x1 > last->x1)
        forwd = FALSE;
    }

  if (forwd)
    {
      end = introns->len;
    }
  else
    {
      multiplier = -1;
      start = (introns->len * multiplier) + 1;
      end   = 1;
    }

  for (i = start; i < end; i++)
    {
      ZMapSpan intron_span;

      index = i * multiplier;

      intron_span = &(g_array_index(introns, ZMapSpanStruct, index));

      (function)(intron_span, user_data);
    }

  return ;
}


/* Returns FALSE if not an alignment feature or no matches, TRUE otherwise. */
gboolean zMapFeatureAlignmentMatchForeach(ZMapFeature feature, GFunc function, gpointer user_data)
{
  gboolean result = FALSE ;

  if (!zMapFeatureIsValid((ZMapFeatureAny)feature) ||
      !function ||
      !ZMAPFEATURE_IS_ALIGNMENT(feature) ||
      !feature->feature.homol.align)
    {
      return result;
    }

  result = TRUE;

  GArray *matches = feature->feature.homol.align;

  int i = 0;
  for ( ; i < (int)matches->len; ++i)
    {
      ZMapAlignBlock match_block = &g_array_index(matches, ZMapAlignBlockStruct, 0) ;
      (function)(match_block, user_data);
    }

  return result ;
}


GArray *zMapFeatureWorld2CDSArray(ZMapFeature feature)
{
  GArray *cds_array = NULL ;

  if (ZMAPFEATURE_HAS_CDS(feature) && ZMAPFEATURE_HAS_EXONS(feature))
    {
      GArray *exon_array ;
      ZMapSpanStruct span;
      int i;

      cds_array  = g_array_sized_new(FALSE, FALSE, sizeof(ZMapSpanStruct), 128);
      exon_array = feature->feature.transcript.exons;

      for(i = 0; i < (int)exon_array->len; i++)
        {
          span = g_array_index(exon_array, ZMapSpanStruct, i);
          zMapFeatureWorld2CDS(feature, span.x1, span.x2, &(span.x1), &(span.x2));
          cds_array = g_array_append_val(cds_array, span);
        }
    }

  return cds_array ;
}



gboolean zMapFeatureWorld2CDS(ZMapFeature feature,
      int exon1, int exon2, int *cds1, int *cds2)
{
  gboolean cds_exon = FALSE;

  if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT && feature->feature.transcript.flags.cds)
    {
      int cds_start, cds_end;

      cds_start = feature->feature.transcript.cds_start;
      cds_end   = feature->feature.transcript.cds_end;

      if (cds_start > exon2 || cds_end < exon1)
        {
          cds_exon = FALSE;
        }
      else
        {
          SimpleParent2ChildDataStruct exon_cds_data = { NULL };
          ZMapMapBlockStruct map_data = { {0,0}, {0,0}, FALSE } ;

          map_data.parent.x1 = exon1;
          map_data.parent.x2 = exon2;

          exon_cds_data.map_block   = &map_data;
          exon_cds_data.limit_start = cds_start;
          exon_cds_data.limit_end   = cds_end;
          exon_cds_data.counter     = 0;

          zMapFeatureTranscriptExonForeach(feature, map_parent2child,
           &exon_cds_data);
          if (cds1)
            *cds1 = map_data.block.x1;
          if (cds2)
            *cds2 = map_data.block.x2;

          cds_exon = TRUE;
        }
    }

  return cds_exon;
}


/* Given a transcript feature and the start/end of an exon within that transcript feature,
 * returns TRUE and gives the coords (in reference sequence space) of the CDS section
 * of the exon and also it's phase. Returns FALSE if the exon is not in the transcript
 * or if the exon has no cds section. */
gboolean zMapFeatureExon2CDS(ZMapFeature feature,
                             int exon_start, int exon_end, int *exon_cds_start, int *exon_cds_end, ZMapPhase *phase_out)
{
  gboolean is_cds_exon = FALSE ;
  int exon_index =0;

  if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT && feature->feature.transcript.flags.cds
              && (exon_index = findExon(feature, exon_start, exon_end)) > -1)
    {
      int cds_start=0, cds_end=0 ;

      cds_start = feature->feature.transcript.cds_start ;
      cds_end = feature->feature.transcript.cds_end ;

      if (!(cds_start > exon_end || cds_end < exon_start))
        {
          /* Exon has a cds section so calculate it and find the exons phase. */
          int start=0, end =0;
          ZMapPhase phase = ZMAPPHASE_NONE ;

          if (calcExonPhase(feature, exon_index, &start, &end, &phase))
            {
              *exon_cds_start = start ;
              *exon_cds_end = end ;
              *phase_out = phase ;

              is_cds_exon = TRUE ;
            }
        }
    }

  return is_cds_exon ;
}




ZMapFrame zMapFeatureFrame(ZMapFeature feature)
{
  ZMapFrame frame = ZMAPFRAME_NONE ;

  frame = feature_frame(feature, feature->x1);

  return frame ;
}


ZMapFrame zMapFeatureFrameFromCoords(int block, int feature)
{
return feature_frame_coords(block, feature);
}

ZMapFrame zMapFeatureFrameAtCoord(ZMapFeature feature, int coord)
{
  ZMapFrame frame = zMapFeatureFrame(feature) ; /* default to feature frame */

  ZMapFullExon exon = feature_find_closest_exon_at_coord(feature, coord) ;

  if (exon)
    {
      int x = exon->sequence_span.x1 - exon->start_phase ;
      int block_offset = 0 ;

      if (feature && feature->parent->parent)
        {
          ZMapFeatureBlock block = (ZMapFeatureBlock)(feature->parent->parent);
          block_offset = block->block_to_sequence.block.x1;   /* start of block in sequence/parent */
        }

      /* If there are any variations in this transcript they may affect the frame */
      if (feature && feature->mode == ZMAPSTYLE_MODE_SEQUENCE && feature->feature.sequence.variations)
        {
          GList *variations = feature->feature.sequence.variations ;

          /* Calculate the total offset caused by variations in this exon up to the given coord */
          int variation_diff = zmapFeatureDNACalculateVariationDiff(exon->sequence_span.x1, coord, variations) ;

          x -= variation_diff ;
        }

      frame = feature_frame_coords(block_offset, x) ;
    }

  return frame ;
}

int zMapFeatureSplitCodonOffset(ZMapFeature feature, int coord)
{
  int result = 0 ;

  ZMapFullExon exon = feature_find_closest_exon_at_coord(feature, coord) ;

  if (exon)
    {
      /* Only apply the offset to the first peptide in the sequence */
      if (coord == exon->sequence_span.x1)
        result = exon->start_phase ;
    }

  return result ;
}


ZMapFrame zMapFeatureSubPartFrame(ZMapFeature feature, int coord)
{
  ZMapFrame frame = ZMAPFRAME_NONE ;

  if(coord >= feature->x1 && coord <= feature->x2)
    frame = feature_frame(feature, coord);

  return frame ;
}

/* Returns ZMAPFRAME_NONE if not a transcript */
ZMapFrame zMapFeatureTranscriptFrame(ZMapFeature feature)
{
  ZMapFrame frame = ZMAPFRAME_NONE;

  if(feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      int start;
      if(feature->feature.transcript.flags.cds)
        start = feature->feature.transcript.cds_start;
      else
        start = feature->x1;

      frame = feature_frame(feature, start);
    }
  else
    {
      zMapLogWarning("Feature %s is not a Transcript.", g_quark_to_string(feature->unique_id));
    }

  return frame;
}


ZMapPhase zMapFeaturePhase(ZMapFeature feature)
{
  ZMapPhase result = ZMAPPHASE_NONE ;

  if (feature && feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT &&
      feature->feature.transcript.flags.start_not_found)
    {
      result = (ZMapPhase)feature->feature.transcript.start_not_found ;
    }

  return result ;
}


char *zMapFeatureTranscriptTranslation(ZMapFeature feature, int *length, gboolean pad)
{
  char *pep_str = NULL ;
  ZMapPeptide peptide ;
  char *dna_str, *name, *free_me ;

  if ((dna_str = zMapFeatureGetTranscriptDNA(feature, TRUE, feature->feature.transcript.flags.cds)))
    {
      free_me = dna_str;    /* as we potentially move ptr. */
      name    = (char *)g_quark_to_string(feature->original_id);

      if (feature->feature.transcript.flags.start_not_found)
        dna_str += (feature->feature.transcript.start_not_found - 1) ;

      peptide = zMapPeptideCreate(name, NULL, dna_str, NULL, TRUE);

      if(length)
        {
          *length = zMapPeptideLength(peptide);
          if (zMapPeptideHasStopCodon(peptide))
            *length = *length - 1;
        }

      pep_str = zMapPeptideSequence(peptide);
      pep_str = g_strdup(pep_str);

      zMapPeptideDestroy(peptide);
      g_free(free_me);
    }

  return pep_str ;
}

char *zMapFeatureTranslation(ZMapFeature feature, int *length, gboolean pad)
{
  char *seq;

  if(feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      seq = zMapFeatureTranscriptTranslation(feature, length, pad);
    }
  else
    {
      GArray *rubbish;
      int i, l;
      char c = '.';

      l = feature->x2 - feature->x1 + 1;

      rubbish = g_array_sized_new(TRUE, TRUE, sizeof(char), l);

      for(i = 0; i < l; i++)
        {
          g_array_append_val(rubbish, c);
        }

      seq = rubbish->data;

      if(length)
        *length = l;

      g_array_free(rubbish, FALSE);
    }

  return seq;
}


gboolean zMapFeaturePrintChildNames(ZMapFeatureAny feature_any)
{
  gboolean result = FALSE ;

  if (zMapFeatureIsValid(feature_any) && (feature_any->children))
    {
      g_hash_table_foreach(feature_any->children, printChildCB, NULL) ;

      result = TRUE ;
    }

  return result ;
}




/*
 *              Package routines.
 */



/*
 *              Internal routines.
 */




/* Encapulates the rules about which frame a feature is in and what enum to return.
 *
 * For ZMap this amounts to:
 *
 * ((coord mod 3) + 1) gives the enum....
 *
 * Using the offset of 1 is almost certainly wrong for the reverse strand and
 * possibly wrong for forward.  Need to think about this one ;)
 *  */

/*
 * using block offset gives frame 1,2,3 in order on display
 * the downside is that if we extend the block upwards we need to recalc translation columns
 */

static ZMapFrame feature_frame(ZMapFeature feature, int start_coord)
{
  ZMapFrame frame;
  int offset;
  ZMapFeatureBlock block;

  g_return_val_if_fail(zMapFeatureIsValid((ZMapFeatureAny)feature), ZMAPFRAME_NONE) ;
  g_return_val_if_fail(feature->parent && feature->parent->parent, ZMAPFRAME_NONE);

  block = (ZMapFeatureBlock)(feature->parent->parent);
  offset = block->block_to_sequence.block.x1;   /* start of block in sequence/parent */

  frame = feature_frame_coords(offset, start_coord);

  return frame;
}

static ZMapFrame feature_frame_coords( int offset, int start_coord)
{
  int fval;
  ZMapFrame frame;

  fval = ((start_coord - offset) % 3) + ZMAPFRAME_0 ;
  if(fval < ZMAPFRAME_0) /* eg feature starts before the block */
  fval += 3;
  frame  = (ZMapFrame) fval;

  return frame;
}


static void get_feature_list_extent(gpointer list_data, gpointer span_data)
{
  ZMapFeature feature = (ZMapFeature)list_data;
  ZMapSpan span = (ZMapSpan)span_data;

  if(feature->x1 < span->x1)
    span->x1 = feature->x1;
  if(feature->x2 > span->x2)
    span->x2 = feature->x2;

  return ;
}


/* For peptide sequences only. Find the exon that the given
 * coord lies in. For other feature types, returns null. */
//static ZMapFullExon feature_find_exon_at_coord(ZMapFeature feature, int coord)
//{
//  if (feature && feature->feature.sequence.exon_list)
//    {
//      GList *item = feature->feature.sequence.exon_list ;
//
//      for ( ; item; item = item->next)
//        {
//          ZMapFullExon exon = (ZMapFullExon)(item->data) ;
//
//          /* Only consider coding exons */
//          if (exon->region_type == EXON_CODING ||
//              exon->region_type == EXON_SPLIT_CODON_5 ||
//              exon->region_type == EXON_SPLIT_CODON_3)
//            {
//              int x1 = exon->sequence_span.x1 ;
//              int x2 = exon->sequence_span.x2 ;
//
//              if (coord >= x1 - 2 && coord <= x2 + 2)
//                {
//                  result = exon ;
//                  break ;
//                }
//            }
//        }
//    }
//}


/* For peptide sequences only. Find the nearest exon to the given
 * coord. For other feature types, returns null. */
static ZMapFullExon feature_find_closest_exon_at_coord(ZMapFeature feature, int coord)
{
  ZMapFullExon result = NULL;
  zMapReturnValIfFail(feature && feature->mode == ZMAPSTYLE_MODE_SEQUENCE, result) ;

  gboolean first_time = TRUE;
  /*gboolean found = FALSE;*/
  int closest_dist = 0;
  GList *exon_item = feature->feature.sequence.exon_list;

  for ( ; exon_item; exon_item = exon_item->next)
    {
      ZMapFullExon current_exon = (ZMapFullExon)(exon_item->data) ;
      int x1 = current_exon->sequence_span.x1 ;
      int x2 = current_exon->sequence_span.x2 ;
      int y = coord ;

      /* Only consider coding exons */
      if (current_exon->region_type == EXON_CODING ||
          current_exon->region_type == EXON_SPLIT_CODON_5 ||
          current_exon->region_type == EXON_SPLIT_CODON_3)
        {
          if (y >= x1 && y <= x2)
            {
              /* Inside the exon - done */
              //zMapDebugPrintf("Coord %d found in exon %d,%d", y, x1, x2) ;
              result = current_exon ;
              /*found = TRUE ;*/
              break ;
            }
          else if (y < x1)
            {
              int cur_dist = x1 - y ;

              if (first_time || cur_dist < closest_dist)
                {
                  first_time = FALSE ;
                  closest_dist = cur_dist ;
                  result = current_exon ;
                }
            }
          else /* y > x2 */
            {
              int cur_dist = y - x2 ;

              if (first_time || cur_dist < closest_dist)
                {
                  first_time = FALSE ;
                  closest_dist = cur_dist ;
                  result = current_exon ;
                }
            }
        }
    }

//  if (feature->feature.sequence.exon_list)
//    {
//      if (!result)
//        zMapDebugPrintf("Could not find exon at coordinate %d", coord);
//      else if (!found)
//        zMapDebugPrintf("Coord %d nearest exon is %d,%d (dist=%d)", coord, result->sequence_span.x1, result->sequence_span.x2, closest_dist);
//    }

  return result ;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* GCompareFunc function, called for each member of a list of styles to see if the supplied
 * style id matches the that in the style. */
static gint findStyle(gconstpointer list_data, gconstpointer user_data)
{
  gint result = -1 ;
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)list_data ;
  GQuark style_quark =  GPOINTER_TO_UINT(user_data) ;

  if (style_quark == style->unique_id)
    result = 0 ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("Looking for: %s   Found: %s\n",
 g_quark_to_string(style_quark), g_quark_to_string(style->unique_id)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return result ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* GCompareFunc function, called for each member of a list of styles ids to see if the supplied
 * style id matches one in the style list. */
static gint findStyleName(gconstpointer list_data, gconstpointer user_data)
{
  gint result = -1 ;
  GQuark style_list_id = GPOINTER_TO_UINT(list_data) ;
  GQuark style_quark =  GPOINTER_TO_UINT(user_data) ;

  if (style_quark == style_list_id)
    result = 0 ;

  return result ;
}


static void map_parent2child(gpointer exon_data, gpointer user_data)
{
  ZMapSpan exon_span = (ZMapSpan)exon_data;
  SimpleParent2ChildData p2c_data = (SimpleParent2ChildData)user_data;

  if(!(p2c_data->limit_start > exon_span->x2 ||
       p2c_data->limit_end   < exon_span->x1))
    {
      if(exon_span->x1 <= p2c_data->map_block->parent.x1 &&
         exon_span->x2 >= p2c_data->map_block->parent.x1)
        {
          /* update the c1 coord*/
          p2c_data->map_block->block.x1  = (p2c_data->map_block->parent.x1 - p2c_data->limit_start + 1);
          p2c_data->map_block->block.x1 += p2c_data->counter;
        }

      if(exon_span->x1 <= p2c_data->map_block->parent.x2 &&
         exon_span->x2 >= p2c_data->map_block->parent.x2)
        {
          /* update the c2 coord */
          p2c_data->map_block->block.x2  = (p2c_data->map_block->parent.x2 - p2c_data->limit_start + 1);
          p2c_data->map_block->block.x2 += p2c_data->counter;
        }

      p2c_data->counter += (exon_span->x2 - exon_span->x1 + 1);
    }

  return ;
}


/* *************************************************************
 * Not entirely sure of the wisdom of this (mainly performance
 * concerns), but everywhere else we have start < end!.  previously
 * loadGaps didn't even fill in the strand or apply the start < end
 * idiom and gaps array required a test when iterating through
 * it (the GArray). The GArray will now be ordered as it almost
 * certainly should be to fit with the start < end idiom.  RDS
 */
static int sortGapsByTarget(gconstpointer a, gconstpointer b)
{
  ZMapAlignBlock alignA = (ZMapAlignBlock)a,
    alignB = (ZMapAlignBlock)b;
  return (alignA->t1 == alignB->t1 ? 0 : (alignA->t1 > alignB->t1 ? 1 : -1));
}


/* Find an exon in a features list given it's start/end coords, returns -1
 * if exon not found, otherwise returns index of exon in exon array. */
static int findExon(ZMapFeature feature, int exon_start, int exon_end)
{
  int exon_index = -1 ;
  GArray *exons ;
  int i ;

  exons = feature->feature.transcript.exons ;

  for (i = 0 ; i < (int)exons->len ; i++)
    {
      ZMapSpan next_exon ;

      next_exon = &(g_array_index(exons, ZMapSpanStruct, i)) ;

      if (next_exon->x1 == exon_start && next_exon->x2 == exon_end)
        {
          exon_index = i ;
          break ;
        }
    }

  return exon_index ;
}


/* Returns the coords (in reference sequence coords) of the cds section of the given exon
 * and also it's phase. */
static gboolean calcExonPhase(ZMapFeature feature, int exon_index,
                              int *exon_cds_start_out, int *exon_cds_end_out, ZMapPhase * p_phase_out)
{
  gboolean result = FALSE ;
  int cds_start, cds_end ;
  GArray *exons ;
  int i, incr, end ;
  int cds_bases ;
  gboolean first_exon ;

  cds_start = feature->feature.transcript.cds_start ;
  cds_end = feature->feature.transcript.cds_end ;
  cds_bases = 0 ;

  exons = feature->feature.transcript.exons ;
  first_exon = FALSE ;


  /* Go forwards through exons for forward strand genes and backwards for
   * reverse strand genes. */
  if (feature->strand == ZMAPSTRAND_FORWARD)
    {
      i = 0 ;
      end = feature->feature.transcript.exons->len ;
      incr = 1 ;
    }
  else
    {
      i = exons->len - 1 ;
      end = -1 ;
      incr = -1 ;
    }

  for ( ; i != end ; i += incr)
    {
      ZMapSpan next_exon ;

      next_exon = &(g_array_index(exons, ZMapSpanStruct, i)) ;

      if (cds_start > next_exon->x2 || cds_end < next_exon->x1)
        {
          continue ;
        }
      else
        {
          int start, end, phase ;

          start = next_exon->x1 ;
          end = next_exon->x2 ;

          if (cds_start >= start && cds_start <= end)
            {
              start = cds_start ;

              if (feature->strand == ZMAPSTRAND_FORWARD)
                first_exon = TRUE ;
            }

          if (cds_end >= start && cds_end <= end)
            {
              end = cds_end ;

              if (feature->strand == ZMAPSTRAND_REVERSE)
                first_exon = TRUE ;
            }

          if (i == exon_index)
            {
              /* The first exon must have phase 0 unless it has been annotated as
               * starting with start_not_found, all others are calculated from
               * CDS bases so far. */

              if (first_exon)
                {
                  // N.B. phase = (start_not_found - 1)
                  if (feature->feature.transcript.flags.start_not_found)
                    phase = (feature->feature.transcript.start_not_found - 1) ;
                  else
                    phase = 0 ;
                }
              else
                {
                  phase = (3 - (cds_bases % 3)) % 3 ;
                }

              *exon_cds_start_out = start ;
              *exon_cds_end_out = end ;
              switch (phase)
                {
                  case 0:
                    *p_phase_out = ZMAPPHASE_0 ;
                    break ;
                  case 1:
                    *p_phase_out = ZMAPPHASE_1 ;
                    break ;
                  case 2:
                    *p_phase_out = ZMAPPHASE_2 ;
                    break ;
                  default:
                    *p_phase_out = ZMAPPHASE_NONE ;
                    break ;
                } ;


              result = TRUE ;

              break ;
            }

          /* Keep a running count of bases so far, caculate phase of next exon from this. */
          cds_bases += (end - start) + 1 ;

          first_exon = FALSE ;
        }

    }

  return result ;
}


/* Compares span objects (e.g. introns or exons) and returns whether
 * they are before or after each other according to their coords.
 *
 * Note that this assumes that span objects do not overlap.
 *
 *  */
static int span_compare(gconstpointer a, gconstpointer b)
{
  int result = 0 ;
  ZMapSpan sa = (ZMapSpan)a ;
  ZMapSpan sb = (ZMapSpan)b ;

  if (sa->x1 < sb->x1)
    result = -1 ;
  else if (sa->x1 > sb->x1)
    result = 1 ;
  else
    result = 0 ;

  return result ;
}


/* A GHFunc() */
static void printChildCB(gpointer key, gpointer value, gpointer user_data_unused)
{
  gboolean debug = TRUE ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)value ;

  zMapDebugPrint(debug, "Feature %s - %s (%s)",
                 zMapFeatureLevelType2Str(feature_any->struct_type),
                 zMapFeatureName(feature_any),
                 g_quark_to_string(feature_any->unique_id)) ;

  return ;
}


/* Invert the coordinates within the given range. Also converts them to 1-based coordinates. */
void zmapFeatureInvert(int *coord, const int seq_start, const int seq_end)
{
  /* Subtract 1 here instead of seq_start in order to make the coord 1-based */
  *coord = seq_end - *coord + 1 ;
}


/* Revcomp the feature coords (swops, inverts, and makes them 1-based) */
void zmapFeatureRevComp(const int seq_start, const int seq_end, int *coord1, int *coord2)
{
  int tmp = *coord1;
  *coord1 = *coord2;
  *coord2 = tmp;

  zmapFeatureInvert(coord1, seq_start, seq_end) ;
  zmapFeatureInvert(coord2, seq_start, seq_end) ;
}
