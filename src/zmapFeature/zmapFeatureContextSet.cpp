/*  File: zmapFeatureContextSet.cpp
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
 * Description: sets are the children of blocks and the parents of
 *              features. Each set represents a set of features of
 *              a particular type which will be drawn into a column
 *              in the end.
 *
 * Exported functions: See ZMap/zmapFeature.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <zmapFeature_P.hpp>


typedef struct
{
  GQuark original_id ;
  int start, end ;
  GList *feature_list ;
  ZMapStrand strand ;
} FindFeaturesRangeStruct, *FindFeaturesRange ;


static void copy_to_new_featureset(gpointer key, gpointer hash_data, gpointer user_data) ;

static void findFeaturesRangeCB(gpointer key, gpointer value, gpointer user_data) ;
static void findFeaturesNameCB(gpointer key, gpointer value, gpointer user_data) ;
static void findFeaturesNameStrandCB(gpointer key, gpointer value, gpointer user_data) ;
static void update_style_from_feature(gpointer key, gpointer hash_data, gpointer user_data) ;



// 
//                External interface routines
//    

/* Features can be NULL if there are no features yet..... */
ZMapFeatureSet zMapFeatureSetCreate(const char *source, 
                                    GHashTable *features,
                                    ZMapConfigSource config_source)
{
  ZMapFeatureSet feature_set ;
  GQuark original_id, unique_id ;

  unique_id = zMapFeatureSetCreateID(source) ;
  original_id = g_quark_from_string(source) ;

  feature_set = zMapFeatureSetIDCreate(original_id, unique_id, NULL, features, config_source) ;

  return feature_set ;
}

void zMapFeatureSetStyle(ZMapFeatureSet feature_set, ZMapFeatureTypeStyle style)
{
  if (!feature_set || !style)
    return ;

  /* MH17: was for the column (historically)
   * new put back to be used for each feature
   */
  feature_set->style = style ;

  return ;
}


/* Features can be NULL if there are no features yet.....
 * original_id  the original name of the feature set
 * unique_id    some derivation of the original name or otherwise unique id to identify this
 *              feature set. */
ZMapFeatureSet zMapFeatureSetIDCreate(GQuark original_id, GQuark unique_id,
                                      ZMapFeatureTypeStyle style, GHashTable *features,
                                      ZMapConfigSource config_source)
{
  ZMapFeatureSet feature_set ;

  feature_set = (ZMapFeatureSet)zmapFeatureAnyCreateFeature(ZMAPFEATURE_STRUCT_FEATURESET, NULL,
                                                            original_id, unique_id,
                                                            features) ;
  //feature_set->style = style ;
  feature_set->source = config_source ;

  return feature_set ;
}


/* Update the style from values in the feature. Currently this just affects graph styles and
 * updates the max and min scores to sensible values based on all of the features that get
 * added. */
void zMapFeatureSetUpdateStyleFromFeature(ZMapFeature feature, ZMapFeatureTypeStyle style)
{
  if (feature && style)
    {
      // If this featureset has a graph style, set the min/max score in the style from the added features
      if (style->mode == ZMAPSTYLE_MODE_GRAPH)
        {
          // we don't seem to handle negative scores? disallow for now...
          if (feature->score > style->max_score && feature->score >= 0.0)
            {
              style->max_score = feature->score ;
            }

          if (feature->score < style->min_score && feature->score >= 0.0)
            {
              style->min_score = feature->score ;
            }
        }
    }
}


/* Update the feature set's style based on values in all of its features */
void zMapFeatureSetUpdateStyleFromFeatures(ZMapFeatureSet feature_set, ZMapFeatureTypeStyle style)
{
  if (feature_set && feature_set->features)
    g_hash_table_foreach(feature_set->features, update_style_from_feature, style) ;
}


/* Feature must not be null to be added we need at least the feature id and probably should.
 * check for more.
 *
 * Returns FALSE if feature is already in set.
 *  */
gboolean zMapFeatureSetAddFeature(ZMapFeatureSet feature_set, ZMapFeature feature)
{
  gboolean result = FALSE ;

  result = zmapFeatureAnyAddFeature((ZMapFeatureAny)feature_set, (ZMapFeatureAny)feature) ;

  return result ;
}


ZMapFeatureSet zMapFeatureSetCopy(ZMapFeatureSet feature_set)
{
  ZMapFeatureSet new_feature_set = NULL;

  new_feature_set = (ZMapFeatureSet)zMapFeatureAnyCopy((ZMapFeatureAny)feature_set);

  g_hash_table_foreach(feature_set->features, copy_to_new_featureset, new_feature_set);

  return new_feature_set;
}




/* Returns TRUE if the feature could be found in the feature_set, FALSE otherwise. */
gboolean zMapFeatureSetFindFeature(ZMapFeatureSet feature_set,
                                   ZMapFeature    feature)
{
  gboolean result = FALSE ;

  if (!feature_set || !feature)
    return result ;

  result = zMapFeatureAnyFindFeature((ZMapFeatureAny)feature_set, (ZMapFeatureAny)feature) ;

  return result ;
}

ZMapFeature zMapFeatureSetGetFeatureByID(ZMapFeatureSet feature_set, GQuark feature_id)
{
  ZMapFeature feature = NULL ;
  zMapReturnValIfFail(feature_set, feature) ;

  feature = (ZMapFeature)zMapFeatureParentGetFeatureByID((ZMapFeatureAny)feature_set, feature_id) ;

  return feature ;
}


/* Feature must exist in set to be removed.
 *
 * Returns FALSE if feature is not in set.
 *  */
gboolean zMapFeatureSetRemoveFeature(ZMapFeatureSet feature_set, ZMapFeature feature)
{
  gboolean result = FALSE ;

  if (!(feature_set && feature && feature->unique_id != ZMAPFEATURE_NULLQUARK) )
    return result ;

  result = zMapFeatureAnyRemoveFeature((ZMapFeatureAny)feature_set, (ZMapFeatureAny)feature) ;

  return result ;
}



GList *zMapFeatureSetGetRangeFeatures(ZMapFeatureSet feature_set, int start, int end)
{
  GList *feature_list = NULL ;
  FindFeaturesRangeStruct find_data ;

  find_data.start = start ;
  find_data.end = end ;
  find_data.feature_list = NULL ;
  find_data.strand = ZMAPSTRAND_NONE ;

  g_hash_table_foreach(feature_set->features, findFeaturesRangeCB, &find_data) ;

  feature_list = find_data.feature_list ;

  return feature_list ;
}

GList *zMapFeatureSetGetNamedFeatures(ZMapFeatureSet feature_set, GQuark original_id)
{
  GList *feature_list = NULL ;
  FindFeaturesRangeStruct find_data = {0} ;

  find_data.original_id = original_id ;
  find_data.feature_list = NULL ;
  find_data.strand = ZMAPSTRAND_NONE ;

  g_hash_table_foreach(feature_set->features, findFeaturesNameCB, &find_data) ;

  feature_list = find_data.feature_list ;

  return feature_list ;
}


/* Return a list of all features with the given name and strand */
GList *zMapFeatureSetGetNamedFeaturesForStrand(ZMapFeatureSet feature_set, GQuark original_id, ZMapStrand strand)
{
  GList *feature_list = NULL ;
  FindFeaturesRangeStruct find_data = {0} ;

  find_data.original_id = original_id ;
  find_data.feature_list = NULL ;
  find_data.strand = strand ;

  g_hash_table_foreach(feature_set->features, findFeaturesNameStrandCB, &find_data) ;

  feature_list = find_data.feature_list ;

  return feature_list ;
}

void zMapFeatureSetDestroy(ZMapFeatureSet feature_set, gboolean free_data)
{
  if (!feature_set)
    return ;

  zmapDestroyFeatureAnyWithChildren((ZMapFeatureAny)feature_set, free_data) ;

  return ;
}


void zMapFeatureSetDestroyFeatures(ZMapFeatureSet feature_set)
{
  if (!feature_set)
    return ;

  g_hash_table_destroy(feature_set->features) ;
  feature_set->features = NULL ;

  return ;
}


// 
//                Internal routines
//    


static void copy_to_new_featureset(gpointer key, gpointer hash_data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)user_data;
  ZMapFeature    new_feature;

  new_feature = (ZMapFeature)zMapFeatureAnyCopy((ZMapFeatureAny)hash_data);

  zMapFeatureSetAddFeature(feature_set, new_feature);

  return ;
}

/* A GHFunc() to add a feature to a list if it within a given range */
static void findFeaturesRangeCB(gpointer key, gpointer value, gpointer user_data)
{
  FindFeaturesRange find_data = (FindFeaturesRange)user_data ;
  ZMapFeature feature = (ZMapFeature)value ;

  if (feature->x1 >= find_data->start && feature->x1 <= find_data->end)
    find_data->feature_list = g_list_append(find_data->feature_list, feature) ;

  return ;
}


/* A GHFunc() to add a feature to a list if it has the given name */
static void findFeaturesNameCB(gpointer key, gpointer value, gpointer user_data)
{
  FindFeaturesRange find_data = (FindFeaturesRange)user_data ;
  ZMapFeature feature = (ZMapFeature)value ;

  if (feature->original_id == find_data->original_id)
    find_data->feature_list = g_list_append(find_data->feature_list, feature) ;

  return ;
}

/* A GHFunc() to add a feature to a list if it has the given name and strand */
static void findFeaturesNameStrandCB(gpointer key, gpointer value, gpointer user_data)
{
  FindFeaturesRange find_data = (FindFeaturesRange)user_data ;
  ZMapFeature feature = (ZMapFeature)value ;

  if (feature->original_id == find_data->original_id && feature->strand == find_data->strand)
    find_data->feature_list = g_list_append(find_data->feature_list, feature) ;

  return ;
}

/* A GHFunc() to update a featureset style from a given feature */
static void update_style_from_feature(gpointer key, gpointer value, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)user_data ;
  ZMapFeature feature = (ZMapFeature)value ;

  zMapFeatureSetUpdateStyleFromFeature(feature, style) ;

  return ;
}
