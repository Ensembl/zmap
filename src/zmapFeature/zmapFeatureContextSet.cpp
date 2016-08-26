/*  File: zmapFeatureContextSet.cpp
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2015: Genome Research Ltd.
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
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *      Gemma Barson (Sanger Institute, UK) gb10@sanger.ac.uk
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
ZMapFeatureSet zMapFeatureSetCreate(const char *source, GHashTable *features)
{
  ZMapFeatureSet feature_set ;
  GQuark original_id, unique_id ;

  unique_id = zMapFeatureSetCreateID(source) ;
  original_id = g_quark_from_string(source) ;

  feature_set = zMapFeatureSetIDCreate(original_id, unique_id, NULL, features) ;

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
                                      ZMapFeatureTypeStyle style, GHashTable *features)
{
  ZMapFeatureSet feature_set ;

  feature_set = (ZMapFeatureSet)zmapFeatureAnyCreateFeature(ZMAPFEATURE_STRUCT_FEATURESET, NULL,
                                                            original_id, unique_id,
                                                            features) ;
  //feature_set->style = style ;

  return feature_set ;
}


/* If this feature has a default style, update the style from values in the feature. Currently
 * this just affects graph styles and updates the max_score to a sensible value based on all of
 * the feature that get added. */
bool zMapFeatureSetUpdateStyleFromFeature(ZMapFeature feature)
{
  bool changed = false ;

  ZMapFeatureSet feature_set = (feature ? (ZMapFeatureSet)feature->parent : NULL) ;

  // Currently only change default styles
  if (feature_set && feature_set->style && feature_set->style->is_default)
    {
      // If this featureset has a graph style, set the max score in the style from the added features
      if (feature_set->style->mode == ZMAPSTYLE_MODE_GRAPH && feature->score > feature_set->style->max_score)
        {
          changed = true ;
          feature_set->style->max_score = feature->score ;
        }
    }

  return changed ;
}


/* Update the feature set's style based on values in all of its features */
bool zMapFeatureSetUpdateStyleFromFeatures(ZMapFeatureSet feature_set)
{
  bool changed = false ;
  
  if (feature_set && feature_set->features)
    g_hash_table_foreach(feature_set->features, update_style_from_feature, &changed) ;

  return changed ;
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
  bool *changed = (bool*)user_data ;
  ZMapFeature feature = (ZMapFeature)value ;

  if (zMapFeatureSetUpdateStyleFromFeature(feature))
    *changed = true ;

  return ;
}
