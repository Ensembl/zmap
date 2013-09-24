/*  Last edited: 30 Oct 16"27 2012 (gb10) */
/*  File: zmapViewScratchColumn.c
 *  Author: Gemma Barson (gb10@sanger.ac.uk)
 *  Copyright (c) 2012: Genome Research Ltd.
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Handles the 'scratch' or 'edit' column, which allows users to
 *              create and edit temporary features
 *
 * Exported functions: see zmapView_P.h
 *
 *-------------------------------------------------------------------
 */

#include <glib.h>

#include <ZMap/zmap.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFeature.h>
#include <zmapView_P.h>
#include <ZMap/zmapGLibUtils.h>


#define SCRATCH_FEATURE_NAME "temp_feature"

  
typedef struct _GetFeaturesetCBDataStruct
{
  GQuark set_id;
  ZMapFeatureSet featureset;
} GetFeaturesetCBDataStruct, *GetFeaturesetCBData;


/* Data about each feature that has been merged into the scratch column */
typedef struct _ScratchMergedFeatureStruct
{
  FooCanvasItem *src_item;
  ZMapFeature src_feature;     /* the new feature to be merged in, i.e. the source for the merge */ 
  double world_x;              /* clicked position */
  double world_y;              /* clicked position */
  gboolean use_subfeature;     /* if true, just use the clicked subfeature, otherwise use the whole feature */
  
  gboolean ignore;            /* set to false to ignore this feature i.e. so it's not part of the scratch feature */
} ScratchMergedFeatureStruct, *ScratchMergedFeature;


/* Data about the current merge process */
typedef struct _ScratchMergeDataStruct
{
  ZMapView view;
  ZMapFeatureSet dest_featureset;        /* the scratch column featureset */
  ZMapFeature dest_feature;              /* the scratch column feature, i.e. the destination for the merge */
  GError **error;                        /* gets set if any problems */

  FooCanvasItem *src_item;
  ZMapFeature src_feature;     /* the new feature to be merged in, i.e. the source for the merge */ 
  double world_x;              /* clicked position */
  double world_y;              /* clicked position */
  gboolean use_subfeature;     /* if true, just use the clicked subfeature, otherwise use the whole feature */
} ScratchMergeDataStruct, *ScratchMergeData;


/* Local function declarations */
static void scratchFeatureRecreateExons(ZMapView view, ZMapFeature feature) ;




/*!
 * \brief Get the flag which indicates whether the start/end
 * of the feature have been set
 */
static gboolean scratchGetStartEndFlag(ZMapView view)
{
  gboolean value = FALSE;
  
  /* Update the relevant flag for the current strand */
  if (view->flags[ZMAPFLAG_REVCOMPED_FEATURES])
    value = view->scratch_start_end_set_rev;
  else
    value = view->scratch_start_end_set;
  
  return value;
}


/*!
 * \brief Update the flag which indicates whether the start/end
 * of the feature have been set
 */
static void scratchSetStartEndFlag(ZMapView view, gboolean value)
{
  /* Update the relevant flag for the current strand */
  if (view->flags[ZMAPFLAG_REVCOMPED_FEATURES])
    view->scratch_start_end_set_rev = value;
  else
    view->scratch_start_end_set = value;
}


/*!
* \brief Get the list of merged features for the current strand 
* */
static GList* scratchGetMergedFeatures(ZMapView view)
{
  GList *list = NULL;
  
  if (view->flags[ZMAPFLAG_REVCOMPED_FEATURES] && view->scratch_features_rev)
    list = view->scratch_features_rev;
  else if (view->scratch_features)
    list = view->scratch_features;

  return list;
}


/*!
* \brief Get pointer to the list of merged features for the current strand 
* */
static GList** scratchGetMergedFeaturesPtr(ZMapView view)
{
  GList **list = NULL;
  
  if (view->flags[ZMAPFLAG_REVCOMPED_FEATURES])
    list = &view->scratch_features_rev;
  else
    list = &view->scratch_features;

  return list;
}


/*!
* \brief Get the first ignored item in the list of merged features
* */
static ScratchMergedFeature scratchGetFirstIgnoredFeature(ZMapView view)
{
  ScratchMergedFeature found_feature = NULL;

  GList *list = scratchGetMergedFeatures(view);
  GList *list_item = g_list_first(list);

  for ( ; list_item && !found_feature; list_item = list_item->next)
    {
      ScratchMergedFeature merge_feature = (ScratchMergedFeature)(list_item->data);
      if (merge_feature->ignore)
        found_feature = merge_feature;
    }

  return found_feature;
}


/*!
* \brief Get the last visible item in the list of merged features
* */
static ScratchMergedFeature scratchGetLastVisibleFeature(ZMapView view)
{
  ScratchMergedFeature found_feature = NULL;

  GList *list = scratchGetMergedFeatures(view);
  GList *list_item = g_list_last(list);

  for ( ; list_item && !found_feature; list_item = list_item->prev)
    {
      ScratchMergedFeature merge_feature = (ScratchMergedFeature)(list_item->data);
      if (!merge_feature->ignore)
        found_feature = merge_feature;
    }

  return found_feature;
}


/*!
* \brief Same as scratchGetLastVisibleFeature but returns the GList item
* */
static GList* scratchGetLastVisibleFeatureItem(ZMapView view)
{
  GList *found_item = NULL;

  GList *list = scratchGetMergedFeatures(view);
  GList *list_item = g_list_last(list);

  for ( ; list_item && !found_item; list_item = list_item->prev)
    {
      ScratchMergedFeature merge_feature = (ScratchMergedFeature)(list_item->data);
      if (!merge_feature->ignore)
        found_item = list_item;
    }

  return found_item;
}


/*!
* \brief Add a feature to the list of merged features
* */
static void scratchAddFeature(ZMapView view, ScratchMergedFeature new_feature)
{
  GList **list_ptr = scratchGetMergedFeaturesPtr(view);
  GList *list = list_ptr ? *list_ptr : NULL;

  /* We may have ignored features at the end of the list. Insert
   * the new item after the last visible feature in the list otherwise
   * the undo/redo order will get messed up. */
  GList *sibling = scratchGetLastVisibleFeatureItem(view);

  if (sibling && sibling->next)
    *list_ptr = g_list_insert_before(list, sibling->next, new_feature);
  else if (sibling)
    *list_ptr = g_list_append(list, new_feature); /* sibling is last in list, so just append */
  else if (list_ptr && list)
    *list_ptr = g_list_prepend(list, new_feature); /* list only contains ignored items, so add new item at start */
  else
    *list_ptr = g_list_append(list, new_feature); /* empty list so append */
}


/*!
 * \brief Callback called on every child in a FeatureAny.
 * 
 * For each featureset, compares its id to the id in the user
 * data and if it matches it sets the result in the user data.
 */
static ZMapFeatureContextExecuteStatus getFeaturesetFromIdCB(GQuark key,
                                                             gpointer data,
                                                             gpointer user_data,
                                                             char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  
  GetFeaturesetCBData cb_data = (GetFeaturesetCBData) user_data ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURESET:
      if (feature_any->unique_id == cb_data->set_id)
        cb_data->featureset = (ZMapFeatureSet)feature_any;
      break;
      
    default:
      break;
    };
  
  return status;
}


/*!
 * \brief Find the featureset with the given id in the given view's context
 */
static ZMapFeatureSet getFeaturesetFromId(ZMapView view, GQuark set_id)
{
  GetFeaturesetCBDataStruct cb_data = { set_id, NULL };
  
  zMapFeatureContextExecute((ZMapFeatureAny)view->features,
                            ZMAPFEATURE_STRUCT_FEATURESET,
                            getFeaturesetFromIdCB,
                            &cb_data);

  return cb_data.featureset ;
}


/*!
 * \brief Get the single featureset that resides in the scratch column
 *
 * \returns The ZMapFeatureSet, or NULL if there was a problem
 */

ZMapFeatureSet zmapViewScratchGetFeatureset(ZMapView view)
{
  ZMapFeatureSet feature_set = NULL;

  GQuark column_id = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_SCRATCH_NAME);
  GList *fs_list = zMapFeatureGetColumnFeatureSets(&view->context_map, column_id, TRUE);
  
  /* There should be one (and only one) featureset in the column */
  if (g_list_length(fs_list) > 0)
    {         
      GQuark set_id = (GQuark)(GPOINTER_TO_INT(fs_list->data));
      feature_set = getFeaturesetFromId(view, set_id);
    }
  else
    {
      zMapWarning("No featureset for column '%s'\n", ZMAP_FIXED_STYLE_SCRATCH_NAME);
    }

  return feature_set;
}


/*! 
 * \brief Get the single feature that resides in the scratch column featureset for this strand
 *
 * \returns The ZMapFeature, or NULL if there was a problem
 */
ZMapFeature zmapViewScratchGetFeature(ZMapFeatureSet feature_set, ZMapStrand strand)
{
  ZMapFeature feature = NULL;
  
  ZMapFeatureAny feature_any = (ZMapFeatureAny)feature_set;

  GHashTableIter iter;
  gpointer key, value;
      
  g_hash_table_iter_init (&iter, feature_any->children);
  
  while (g_hash_table_iter_next (&iter, &key, &value) && !feature)
    {
      feature = (ZMapFeature)(value);
      
      if (feature->strand != strand)
        feature = NULL;
    }

  return feature;
}


/*!
 * \brief Merge the given start/end coords into the scratch column feature
 */
static gboolean scratchMergeCoords(ScratchMergeData merge_data, const int coord1, const int coord2)
{
  gboolean result = FALSE;
  
  /* If the start and end have not been set and we're merging in a single subfeature
   * then set the start/end from the subfeature (for a single subfeature, this function
   * only gets called once, so it's safe to set it here; when merging a 'full' feature
   * e.g. a transcript this will get called for each exon, so we need to set the feature
   * extent from a higher level). */
  if (merge_data->use_subfeature && !scratchGetStartEndFlag(merge_data->view))
    {
      merge_data->dest_feature->x1 = coord1;
      merge_data->dest_feature->x2 = coord2;
    }

  zMapFeatureTranscriptMergeExon(merge_data->dest_feature, coord1, coord2);
  result = TRUE;
  
  return result;
}


/*!
 * \brief Merge a single coord into the scratch column feature
 */
static gboolean scratchMergeCoord(ScratchMergeData merge_data, const int coord, const ZMapBoundaryType boundary)
{
  gboolean merged = FALSE;

  /* Check that the scratch feature already contains at least one
   * exon, otherwise we cannot merge a single coord. */
  if (zMapFeatureTranscriptGetNumExons(merge_data->dest_feature) > 0)
    {
      /* Merge in the new exons */
      merged = zMapFeatureTranscriptMergeCoord(merge_data->dest_feature, coord, boundary, merge_data->error);
    }
  else
    {
      g_set_error(merge_data->error, g_quark_from_string("ZMap"), 99, "Cannot merge a single coordinate");
    }
  
  return merged;
}


/*! 
 * \brief Merge a single exon into the scratch column feature
 */
static void scratchMergeExonCB(gpointer exon, gpointer user_data)
{
  ZMapSpan exon_span = (ZMapSpan)exon;
  ScratchMergeData merge_data = (ScratchMergeData)user_data;

  scratchMergeCoords(merge_data, exon_span->x1, exon_span->x2);
}


/*! 
 * \brief Merge a single alignment match into the scratch column feature
 */
static void scratchMergeMatchCB(gpointer match, gpointer user_data)
{
  ZMapAlignBlock match_block = (ZMapAlignBlock)match;
  ScratchMergeData merge_data = (ScratchMergeData)user_data;

  scratchMergeCoords(merge_data, match_block->q1, match_block->q2);
}


/*! 
 * \brief Add/merge a single base into the scratch column
 */
static gboolean scratchMergeBase(ScratchMergeData merge_data)
{
  gboolean merged = FALSE;
  long seq_start=0.0, seq_end=0.0;      

  /* Convert the world coords to sequence coords */
  if (zMapWindowItemGetSeqCoord(merge_data->src_item, TRUE, merge_data->world_x, merge_data->world_y, &seq_start, &seq_end))
    {
      merged = scratchMergeCoord(merge_data, seq_start, ZMAPBOUNDARY_NONE);
    }
  else
    {
      g_set_error(merge_data->error, g_quark_from_string("ZMap"), 99, "Program error: not a featureset item");
    }


  return merged;
}


/*! 
 * \brief Add/merge a basic feature to the scratch column
 */
static gboolean scratchMergeBasic(ScratchMergeData merge_data)
{
  gboolean merged = TRUE;
  
  /* Just merge the start/end of the feature */
  merged = scratchMergeCoords(merge_data, merge_data->src_feature->x1, merge_data->src_feature->x2);
  
  return merged;
}


/*! 
 * \brief Add/merge a splice feature to the scratch column
 */
static gboolean scratchMergeSplice(ScratchMergeData merge_data)
{
  gboolean merged = FALSE;
  
  /* Just merge the start/end of the feature */
  merged = scratchMergeCoord(merge_data, merge_data->src_feature->x1, merge_data->src_feature->boundary_type);
  
  return merged;
}


/*! 
 * \brief Add/merge an alignment feature to the scratch column
 */
static gboolean scratchMergeAlignment(ScratchMergeData merge_data)
{
  gboolean merged = FALSE;
  
  /* Loop through each match block and merge it in */
  if (!zMapFeatureAlignmentMatchForeach(merge_data->src_feature, scratchMergeMatchCB, merge_data))
    {
      /* The above returns false if no match blocks were found. If the alignment is 
       * gapped but we don't have this data then we can't process it. Otherwise, it's
       * an ungapped alignment so add the whole thing. */
      if (zMapFeatureAlignmentIsGapped(merge_data->src_feature))
        {
          g_set_error(merge_data->error, g_quark_from_string("ZMap"), 99, "Cannot copy feature; gapped alignment data is not available");
        }
      else
        {
          merged = scratchMergeCoords(merge_data, merge_data->src_feature->x1, merge_data->src_feature->x2);
        }
    }

  /* Copy CDS, if set */
  //  zMapFeatureMergeTranscriptCDS(merge_data->src_feature, merge_data->dest_feature);

  return merged;
}


/*! 
 * \brief Add/merge a transcript feature to the scratch column
 */
static gboolean scratchMergeTranscript(ScratchMergeData merge_data)
{
  gboolean merged = TRUE;
  
  /* Merge in all of the new exons from the new feature */
  zMapFeatureTranscriptExonForeach(merge_data->src_feature, scratchMergeExonCB, merge_data);
  
  /* Copy CDS, if set */
  zMapFeatureMergeTranscriptCDS(merge_data->src_feature, merge_data->dest_feature);

  return merged;
}


/*! 
 * \brief Add/merge a feature to the scratch column
 */
static void scratchMergeFeature(ScratchMergeData merge_data)
{
  gboolean merged = FALSE;

  if (merge_data->use_subfeature)
    {
      /* Just merge the clicked subfeature */
      ZMapFeatureSubPartSpan sub_feature = NULL;
      zMapWindowItemGetInterval(merge_data->src_item, merge_data->world_x, merge_data->world_y, &sub_feature);
      
      if (sub_feature)
        {
          if (sub_feature->start == sub_feature->end)
            merged = scratchMergeCoord(merge_data, sub_feature->start, ZMAPBOUNDARY_NONE);
          else
            merged = scratchMergeCoords(merge_data, sub_feature->start, sub_feature->end);
        }
      else
        {
          g_set_error(merge_data->error, g_quark_from_string("ZMap"), 99, 
                      "Could not find subfeature in feature '%s' at coords %d, %d", 
                      g_quark_to_string(merge_data->src_feature->original_id),
                      (int)merge_data->world_x, (int)merge_data->world_y);
        }
    }
  else
    {
      /* If the start/end is not set, set it now from the feature extent */
      if (!scratchGetStartEndFlag(merge_data->view))
        {
          merge_data->dest_feature->x1 = merge_data->src_feature->x1;
          merge_data->dest_feature->x2 = merge_data->src_feature->x2;
        }

      switch (merge_data->src_feature->type)
        {
        case ZMAPSTYLE_MODE_BASIC:
          merged = scratchMergeBasic(merge_data);
          break;
        case ZMAPSTYLE_MODE_ALIGNMENT:
          merged = scratchMergeAlignment(merge_data);
          break;
        case ZMAPSTYLE_MODE_TRANSCRIPT:
          merged = scratchMergeTranscript(merge_data);
          break;
        case ZMAPSTYLE_MODE_SEQUENCE:
          merged = scratchMergeBase(merge_data);
          break;
        case ZMAPSTYLE_MODE_GLYPH:
          merged = scratchMergeSplice(merge_data);
          break;

        case ZMAPSTYLE_MODE_INVALID:       /* fall through */
        case ZMAPSTYLE_MODE_ASSEMBLY_PATH: /* fall through */
        case ZMAPSTYLE_MODE_TEXT:          /* fall through */
        case ZMAPSTYLE_MODE_GRAPH:         /* fall through */
        case ZMAPSTYLE_MODE_PLAIN:         /* fall through */
        case ZMAPSTYLE_MODE_META:          /* fall through */
        default:
          g_set_error(merge_data->error, g_quark_from_string("ZMap"), 99, "copy of feature of type %d is not implemented.\n", merge_data->src_feature->type);
          break;
        };
    }
  
  /* Once finished merging, the start/end should now be set */
  if (merged)
    {
      scratchSetStartEndFlag(merge_data->view, TRUE);
    }
  
  if (merge_data->error && *(merge_data->error))
    {
      if (merged)
        zMapWarning("Warning while copying feature: %s", (*(merge_data->error))->message);
      else
        zMapCritical("Error copying feature: %s", (*(merge_data->error))->message);
      
      g_error_free(*(merge_data->error));
      merge_data->error = NULL;
    }
}


/*!
 * \brief Utility to delete all exons from the given feature
 */
static void scratchDeleteFeatureExons(ZMapView view, ZMapFeature feature, ZMapFeatureSet feature_set)
{
  if (feature)
    {
      zMapFeatureRemoveExons(feature);
      zMapFeatureRemoveIntrons(feature);
    }  
}


static ZMapFeatureContext copyContextAll(ZMapFeatureContext context, 
                                         ZMapFeature feature,
                                         ZMapFeatureSet feature_set,
                                         GList **feature_list,
                                         ZMapFeature *feature_copy_out)
{
  ZMapFeatureContext context_copy = NULL ;
 
  g_return_val_if_fail(context && context->master_align && feature && feature_set, NULL) ;
 
  context_copy = (ZMapFeatureContext)zMapFeatureAnyCopy((ZMapFeatureAny)context) ;
  context_copy->req_feature_set_names = g_list_append(context_copy->req_feature_set_names, GINT_TO_POINTER(feature_set->unique_id)) ;
  
  ZMapFeatureAlignment align_copy = (ZMapFeatureAlignment)zMapFeatureAnyCopy((ZMapFeatureAny)context->master_align) ;
  zMapFeatureContextAddAlignment(context_copy, align_copy, FALSE) ;
  
  ZMapFeatureBlock block = zMap_g_hash_table_nth(context->master_align->blocks, 0) ;
  g_return_val_if_fail(block, NULL) ;

  ZMapFeatureBlock block_copy = (ZMapFeatureBlock)zMapFeatureAnyCopy((ZMapFeatureAny)block) ;
  zMapFeatureAlignmentAddBlock(align_copy, block_copy) ;
  
  ZMapFeatureSet feature_set_copy = (ZMapFeatureSet)zMapFeatureAnyCopy((ZMapFeatureAny)feature_set) ;
  zMapFeatureBlockAddFeatureSet(block_copy, feature_set_copy) ;
  
  ZMapFeature feature_copy = (ZMapFeature)zMapFeatureAnyCopy((ZMapFeatureAny)feature) ;
  zMapFeatureSetAddFeature(feature_set_copy, feature_copy) ;
  
  if (feature_list)
    *feature_list = g_list_append(*feature_list, feature_copy) ;
  
  if (feature_copy_out)
    *feature_copy_out = feature_copy ;
  
  return context_copy ;
}


static void scratchEraseFeature(ZMapView zmap_view)
{
  g_return_if_fail(zmap_view && zmap_view->features) ;
  ZMapFeatureContext context = zmap_view->features ;
  
  /* Get the singleton features that exist in each strand of the scatch column */
  ZMapFeatureSet feature_set = zmapViewScratchGetFeatureset(zmap_view);
  g_return_if_fail(feature_set) ;

  ZMapFeature feature = zmapViewScratchGetFeature(feature_set, ZMAPSTRAND_FORWARD);
  g_return_if_fail(feature) ;

  GList *feature_list = NULL ;
  ZMapFeatureContext context_copy = copyContextAll(context, feature, feature_set, &feature_list, NULL) ;
  
  if (context_copy && feature_list)
    zmapViewEraseFeatures(zmap_view, context_copy, &feature_list) ;

  if (context_copy)
    zMapFeatureContextDestroy(context_copy, TRUE) ;
}


static ZMapFeature scratchCreateNewFeature(ZMapView zmap_view)
{
  g_return_val_if_fail(zmap_view, NULL) ;
  
  ZMapFeatureSet feature_set = zmapViewScratchGetFeatureset(zmap_view);
  g_return_val_if_fail(feature_set, NULL) ;

  /* Create the feature with default values */
  ZMapFeature feature = zMapFeatureCreateFromStandardData(SCRATCH_FEATURE_NAME,
                                                          NULL,
                                                          NULL,
                                                          ZMAPSTYLE_MODE_TRANSCRIPT,
                                                          &feature_set->style,
                                                          0, 
                                                          0,
                                                          FALSE,
                                                          0.0,
                                                          ZMAPSTRAND_FORWARD);

  zMapFeatureTranscriptInit(feature);
  zMapFeatureAddTranscriptStartEnd(feature, FALSE, 0, FALSE);    
  //zMapFeatureSetAddFeature(feature_set, feature);  

  /* Create the exons */
  scratchFeatureRecreateExons(zmap_view, feature) ;
  
  return feature ;
}


static void scratchMergeNewFeature(ZMapView zmap_view, ZMapFeature feature)
{
  g_return_if_fail(zmap_view && zmap_view->features) ;
  //g_return_if_fail(feature) ;

  /* Get the current featureset and context */
  ZMapFeatureSet feature_set = zmapViewScratchGetFeatureset(zmap_view);
  g_return_if_fail(feature_set) ;
  
  //ZMapFeature feature = zmapViewScratchGetFeature(feature_set, ZMAPSTRAND_FORWARD);
  g_return_if_fail(feature) ;
  
  ZMapFeatureContext context = zmap_view->features ;

  GList *feature_list = NULL ;
  ZMapFeature feature_copy = NULL ;
  ZMapFeatureContext context_copy = copyContextAll(context, feature, feature_set, &feature_list, &feature_copy) ;

  if (context_copy && feature_list)
    zmapViewMergeNewFeatures(zmap_view, &context_copy, &feature_list) ;
  
  if (context_copy && feature_copy)
    zmapViewDrawDiffContext(zmap_view, &context_copy, feature_copy) ;

  if (context_copy)
    zMapFeatureContextDestroy(context_copy, TRUE) ;
}


static void handBuiltInit(ZMapView zmap_view, ZMapFeatureSequenceMap sequence, ZMapFeatureContext context)
{
  ZMapFeatureSet featureset = NULL ;
  ZMapFeatureTypeStyle style = NULL ;
  ZMapFeatureSetDesc f2c;
  ZMapFeatureSource src;
  GList *list;
  ZMapFeatureColumn column;

  ZMapFeatureContextMap context_map = &zmap_view->context_map;

  if((style = zMapFindStyle(context_map->styles, zMapStyleCreateID(ZMAP_FIXED_STYLE_HAND_BUILT_NAME))))
    {
      /* Create the featureset */
      featureset = zMapFeatureSetCreate(ZMAP_FIXED_STYLE_HAND_BUILT_NAME, NULL);
      style = zMapFeatureStyleCopy(style);
      featureset->style = style ;

      /* Create the context, align and block, and add the featureset to it */
      if (!context)
        context = zmapViewCreateContext(zmap_view, NULL, featureset);

	/* set up featureset2_column and anything else needed */
      f2c = g_hash_table_lookup(context_map->featureset_2_column, GUINT_TO_POINTER(featureset->unique_id));
      if(!f2c)	/* these just accumulate  and should be removed from the hash table on clear */
	{
		f2c = g_new0(ZMapFeatureSetDescStruct,1);

		f2c->column_id = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_HAND_BUILT_NAME);
		f2c->column_ID = g_quark_from_string(ZMAP_FIXED_STYLE_HAND_BUILT_NAME);
		f2c->feature_src_ID = g_quark_from_string(ZMAP_FIXED_STYLE_HAND_BUILT_NAME);
		f2c->feature_set_text = ZMAP_FIXED_STYLE_HAND_BUILT_TEXT;
		g_hash_table_insert(context_map->featureset_2_column, GUINT_TO_POINTER(featureset->unique_id), f2c);
	}

      src = g_hash_table_lookup(context_map->source_2_sourcedata, GUINT_TO_POINTER(featureset->unique_id));
      if(!src)
	{
		src = g_new0(ZMapFeatureSourceStruct,1);
		src->source_id = f2c->feature_src_ID;
		src->source_text = g_quark_from_string(ZMAP_FIXED_STYLE_HAND_BUILT_TEXT);
		src->style_id = style->unique_id;
		src->maps_to = f2c->column_id;
		g_hash_table_insert(context_map->source_2_sourcedata, GUINT_TO_POINTER(featureset->unique_id), src);
	}

      list = g_hash_table_lookup(context_map->column_2_styles,GUINT_TO_POINTER(f2c->column_id));
      if(!list)
	{
		list = g_list_prepend(list,GUINT_TO_POINTER(src->style_id));
		g_hash_table_insert(context_map->column_2_styles,GUINT_TO_POINTER(f2c->column_id), list);
	}

      column = g_hash_table_lookup(context_map->columns,GUINT_TO_POINTER(f2c->column_id));
      if(!column)
	{
		column = g_new0(ZMapFeatureColumnStruct,1);
		column->unique_id = f2c->column_id;
		column->style_table = g_list_prepend(NULL, (gpointer)  style);
		/* the rest shoudl get filled in elsewhere */
		g_hash_table_insert(context_map->columns, GUINT_TO_POINTER(f2c->column_id), column);
	}
    }  
}


/************ PUBLIC FUNCTIONS **************/


/*!
 * \brief Initialise the Scratch column
 *
 * \param[in] zmap_view
 * \param[in] sequence
 *
 * \return void
 */
void zmapViewScratchInit(ZMapView zmap_view, ZMapFeatureSequenceMap sequence, ZMapFeatureContext context_in, ZMapFeatureBlock block_in)
{
  ZMapFeatureSet scratch_featureset = NULL ;
  ZMapFeatureTypeStyle style = NULL ;
  ZMapFeatureSetDesc f2c;
  ZMapFeatureSource src;
  GList *list;
  ZMapFeatureColumn column;

  ZMapFeatureContextMap context_map = &zmap_view->context_map;
  ZMapFeatureContext context = context_in;
  ZMapFeatureBlock block = block_in;

  if((style = zMapFindStyle(context_map->styles, zMapStyleCreateID(ZMAP_FIXED_STYLE_SCRATCH_NAME))))
    {
      /* Create the featureset */
      scratch_featureset = zMapFeatureSetCreate(ZMAP_FIXED_STYLE_SCRATCH_NAME, NULL);
      style = zMapFeatureStyleCopy(style);
      scratch_featureset->style = style ;

      /* Create the context, align and block, and add the featureset to it */
      if (!context || !block)
        context = zmapViewCreateContext(zmap_view, NULL, scratch_featureset);
      else
        zMapFeatureBlockAddFeatureSet(block, scratch_featureset);
      
	/* set up featureset2_column and anything else needed */
      f2c = g_hash_table_lookup(context_map->featureset_2_column, GUINT_TO_POINTER(scratch_featureset->unique_id));
      if(!f2c)	/* these just accumulate  and should be removed from the hash table on clear */
	{
		f2c = g_new0(ZMapFeatureSetDescStruct,1);

		f2c->column_id = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_SCRATCH_NAME);
		f2c->column_ID = g_quark_from_string(ZMAP_FIXED_STYLE_SCRATCH_NAME);
		f2c->feature_src_ID = g_quark_from_string(ZMAP_FIXED_STYLE_SCRATCH_NAME);
		f2c->feature_set_text = ZMAP_FIXED_STYLE_SCRATCH_TEXT;
		g_hash_table_insert(context_map->featureset_2_column, GUINT_TO_POINTER(scratch_featureset->unique_id), f2c);
	}

      src = g_hash_table_lookup(context_map->source_2_sourcedata, GUINT_TO_POINTER(scratch_featureset->unique_id));
      if(!src)
	{
		src = g_new0(ZMapFeatureSourceStruct,1);
		src->source_id = f2c->feature_src_ID;
		src->source_text = g_quark_from_string(ZMAP_FIXED_STYLE_SCRATCH_TEXT);
		src->style_id = style->unique_id;
		src->maps_to = f2c->column_id;
		g_hash_table_insert(context_map->source_2_sourcedata, GUINT_TO_POINTER(scratch_featureset->unique_id), src);
	}

      list = g_hash_table_lookup(context_map->column_2_styles,GUINT_TO_POINTER(f2c->column_id));
      if(!list)
	{
		list = g_list_prepend(list,GUINT_TO_POINTER(src->style_id));
		g_hash_table_insert(context_map->column_2_styles,GUINT_TO_POINTER(f2c->column_id), list);
	}

      column = g_hash_table_lookup(context_map->columns,GUINT_TO_POINTER(f2c->column_id));
      if(!column)
	{
		column = g_new0(ZMapFeatureColumnStruct,1);
		column->unique_id = f2c->column_id;
		column->style_table = g_list_prepend(NULL, (gpointer)  style);
                column->order = zMapFeatureColumnOrderNext();
		/* the rest shoudl get filled in elsewhere */
		g_hash_table_insert(context_map->columns, GUINT_TO_POINTER(f2c->column_id), column);
	}

      /* Create two empty features, one for each strand */
      ZMapFeature feature_fwd = zMapFeatureCreateEmpty() ;
      ZMapFeature feature_rev = zMapFeatureCreateEmpty() ;
      
      zMapFeatureAddStandardData(feature_fwd, "temp_feature_fwd", SCRATCH_FEATURE_NAME,
                                 NULL, NULL,
                                 ZMAPSTYLE_MODE_TRANSCRIPT, &scratch_featureset->style,
                                 0, 0, FALSE, 0.0,
                                 ZMAPSTRAND_FORWARD);

      zMapFeatureAddStandardData(feature_rev, "temp_feature_rev", SCRATCH_FEATURE_NAME,
                                 NULL, NULL,
                                 ZMAPSTYLE_MODE_TRANSCRIPT, &scratch_featureset->style,
                                 0, 0, FALSE, 0.0,
                                 ZMAPSTRAND_REVERSE);

      zMapFeatureTranscriptInit(feature_fwd);
      zMapFeatureTranscriptInit(feature_rev);
      zMapFeatureAddTranscriptStartEnd(feature_fwd, FALSE, 0, FALSE);
      zMapFeatureAddTranscriptStartEnd(feature_rev, FALSE, 0, FALSE);
      
      //zMapFeatureSequenceSetType(feature, ZMAPSEQUENCE_PEPTIDE);
      //zMapFeatureAddFrame(feature, ZMAPFRAME_NONE);
      
      zMapFeatureSetAddFeature(scratch_featureset, feature_fwd);      
      zMapFeatureSetAddFeature(scratch_featureset, feature_rev);      
    }  

  /* Also initialise the "hand_built" column. xace puts newly created
   * features in this column. */
  handBuiltInit(zmap_view, sequence, context);

  /* Merge our context into the view's context and view the diff context */
  if (!context_in)
    {
      ZMapFeatureContext diff_context = zmapViewMergeInContext(zmap_view, context);
      zmapViewDrawDiffContext(zmap_view, &diff_context, NULL);
    }
}


void zMapViewToggleScratchColumn(ZMapView view, gboolean force_to, gboolean force)
{
  GList* list_item ;
  
  if((list_item = g_list_first(view->window_list)))
    {
      do
        {
          ZMapViewWindow view_window ;
          
          view_window = list_item->data ;
          
          zMapWindowToggleScratchColumn(view_window->window, 0, 0, force_to, force) ;
        }
      while ((list_item = g_list_next(list_item))) ;
    }
  
  
}


/*!
 * /brief Reset the scratch feature
 */
void scratchFeatureReset(ZMapView view)
{
  /* Get the singleton features that exist in each strand of the scatch column */
  ZMapFeatureSet scratch_featureset = zmapViewScratchGetFeatureset(view);
  ZMapFeature scratch_feature = zmapViewScratchGetFeature(scratch_featureset, ZMAPSTRAND_FORWARD);

  //zmapViewWindowsRemoveFeatureset(view, scratch_featureset) ;

  /* Delete the exons and introns (the singleton feature always
   * exists so we don't delete the whole thing). */
  scratchDeleteFeatureExons(view, scratch_feature, scratch_featureset);

  /* Reset the start-/end-set flag */
  scratchSetStartEndFlag(view, FALSE);
}


/*!
 * /brief Recreate the scratch feature's list of exons from the list of merged features
 */
static void scratchFeatureRecreateExons(ZMapView view, ZMapFeature scratch_feature)
{
  /* Get the singleton features that exist in each strand of the scatch column */
  ZMapFeatureSet scratch_featureset = zmapViewScratchGetFeatureset(view);

  /* Loop through each feature in the merge list and merge it in */
  GError *error = NULL;
  ScratchMergeDataStruct merge_data = {view, scratch_featureset, scratch_feature, &error, NULL, NULL, 0, 0, FALSE};
  GList *list_item = view->flags[ZMAPFLAG_REVCOMPED_FEATURES] ? view->scratch_features_rev : view->scratch_features;
  
  for ( ; list_item; list_item = list_item->next)
    {
      ScratchMergedFeature merge_feature = (ScratchMergedFeature)(list_item->data);
      
      if (!merge_feature->ignore)
        {
          /* Add info about the current feature to the merge data
           * \todo Could tidy this and just point to the merge_feature */
          merge_data.src_feature = merge_feature->src_feature;
          merge_data.src_item = merge_feature->src_item;
          merge_data.world_x = merge_feature->world_x;
          merge_data.world_y = merge_feature->world_y;
          merge_data.use_subfeature = merge_feature->use_subfeature;
          
          /* Do the merge */
          scratchMergeFeature(&merge_data);
        }
    }

  zMapFeatureTranscriptRecreateIntrons(scratch_feature);
}


/*!
 * /brief Recreate the scratch feature from the list of merged features
 */
void scratchFeatureRecreate(ZMapView view)
{
  /* Erase the existing scratch feature */
  scratchEraseFeature(view) ;

  /* Create the new feature */
  ZMapFeature feature = scratchCreateNewFeature(view) ;

  /* Merge it in */
  scratchMergeNewFeature(view, feature) ;
}


/*!
 * \brief Update any changes to the given featureset
 */
gboolean zmapViewScratchCopyFeature(ZMapView view, 
                                    ZMapFeature feature,
                                    FooCanvasItem *item,
                                    const double world_x,
                                    const double world_y,
                                    const gboolean use_subfeature)
{
  if (feature)
    {
      ScratchMergedFeature merge_feature = g_new0(ScratchMergedFeatureStruct, 1);
      merge_feature->src_feature = feature;
      merge_feature->src_item = item;
      merge_feature->world_x = world_x;
      merge_feature->world_y = world_y;
      merge_feature->use_subfeature = use_subfeature;
      merge_feature->ignore = FALSE;

      /* Add this feature to the list of merged features and recreate the scratch feature */
      scratchAddFeature(view, merge_feature);
      scratchFeatureRecreate(view);
    }

  return TRUE;
}


/* Does the same as g_list_free_full, which requires gtk 2.28 */
static void freeListFull(GList *list, GDestroyNotify free_func)
{
  GList *item = list;
  for ( ; item; item = item->next)
    g_free(item->data);
  
  g_list_free(list);
}


/*!
 * \brief Clear the scratch column
 *
 * This removes the exons and introns from the feature.
 * It doesn't remove the feature itself because this 
 * singleton feature always exists.
 */
gboolean zmapViewScratchClear(ZMapView view)
{ 
  /* Clear the list of features in the scratch column */
  if (view->flags[ZMAPFLAG_REVCOMPED_FEATURES] && view->scratch_features_rev)
    {
      freeListFull(view->scratch_features_rev, g_free);
      view->scratch_features_rev = NULL;
    }
  else if (view->scratch_features)
    {
      freeListFull(view->scratch_features, g_free);
      view->scratch_features = NULL;
    }

  scratchFeatureRecreate(view);

  return TRUE;
}


/*!
 * \brief Undo the last feature-copy into the scratch column
 */
gboolean zmapViewScratchUndo(ZMapView view)
{ 
  /* Find the last visible item and disable it */
  ScratchMergedFeature feature = scratchGetLastVisibleFeature(view);
  
  if (feature)
    {
      feature->ignore = TRUE;
      scratchFeatureRecreate(view);
    }
  else
    {
      g_critical("No operations to redo");
    }
  
  return TRUE;
}


/*!
 * \brief Redo the last operation
 */
gboolean zmapViewScratchRedo(ZMapView view)
{ 
  /* Get the first ignored feature and re-enable it */
  ScratchMergedFeature feature = scratchGetFirstIgnoredFeature(view);
  
  if (feature)
    {
      feature->ignore = FALSE;
      scratchFeatureRecreate(view);
    }
  else
    {
      g_critical("No operations to redo\n");
    }
  
  return TRUE;
}
