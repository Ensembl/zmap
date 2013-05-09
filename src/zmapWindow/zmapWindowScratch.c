/*  Last edited: Nov 09 14:25 2012 (gb10) */
/*  File: zmapWindowEditColumn.c
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
 * Description: Implements the "scratch" or "edit" column in zmap. This
 *              is a column where the user can create and edit temporary 
 *              gene model objects.
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFeature.h>
#include <zmapWindow_P.h>	/* ZMapWindow */
#include <zmapWindowScratch_P.h>


/* This indicates whether the start/end of the scratch transcript 
 * has been set yet. If it's unset we set it from the new feature
 * when a feature is copied in (subsequent times the new feature
 * is merged so the transcript extent may not be updated if it
 * already includes the new extent). 
 * Note that we have two features - one for each strand - so we 
 * need two flags. */
static gboolean start_end_set_G = FALSE; 
static gboolean start_end_set_rev_G = FALSE; 

  
typedef struct _GetFeaturesetCBDataStruct
{
  GQuark set_id;
  ZMapFeatureSet featureset;
} GetFeaturesetCBDataStruct, *GetFeaturesetCBData;


typedef struct _ScratchMergeDataStruct
{
  ZMapWindow window;
  FooCanvasItem *src_item;
  ZMapFeature src_feature;     /* the new feature to be merged in, i.e. the source for the merge */ 
  ZMapFeatureSet dest_featureset; /* the scratch column featureset */
  ZMapFeature dest_feature;    /* the scratch column feature, i.e. the destination for the merge */
  double world_x;              /* clicked position */
  double world_y;              /* clicked position */
  gboolean use_subfeature;     /* if true, just use the clicked subfeature, otherwise use the whole feature */
} ScratchMergeDataStruct, *ScratchMergeData;
  


/*!
 * \brief Update the flag which indicates whether the start/end
 * of the feature have been set
 */
static void scratchSetStartEndFlag(ZMapWindow window, gboolean value)
{
  /* Update the relevant flag for the current strand */
  if (window->revcomped_features)
    start_end_set_rev_G = value;
  else
    start_end_set_G = value;
}


/*!
 * \brief Get the flag which indicates whether the start/end
 * of the feature have been set
 */
static gboolean scratchGetStartEndFlag(ZMapWindow window)
{
  gboolean value = FALSE;
  
  /* Update the relevant flag for the current strand */
  if (window->revcomped_features)
    value = start_end_set_rev_G;
  else
    value = start_end_set_G;
  
  return value;
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
 * \brief Find the featureset with the given id in the given window's context
 */
static ZMapFeatureSet zmapWindowGetFeaturesetFromId(ZMapWindow window, GQuark set_id)
{
  GetFeaturesetCBDataStruct cb_data = { set_id, NULL };
  
  zMapFeatureContextExecute((ZMapFeatureAny)window->feature_context,
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
static ZMapFeatureSet scratchGetFeatureset(ZMapWindow window)
{
  ZMapFeatureSet feature_set = NULL;

  GQuark column_id = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_SCRATCH_NAME);
  GList *fs_list = zMapFeatureGetColumnFeatureSets(window->context_map, column_id, TRUE);
  
  /* There should be one (and only one) featureset in the column */
  if (g_list_length(fs_list) > 0)
    {         
      GQuark set_id = (GQuark)(GPOINTER_TO_INT(fs_list->data));
      feature_set = zmapWindowGetFeaturesetFromId(window, set_id);
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
static ZMapFeature scratchGetFeature(ZMapFeatureSet feature_set, ZMapStrand strand)
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
static void scratchMergeCoords(ScratchMergeData merge_data, const int coord1, const int coord2)
{
  /* If this is the first/last coord in the new feature 
   * and the start/end has not yet been set, then set it now */
  if (!scratchGetStartEndFlag(merge_data->window))
    {
      if (coord1 == merge_data->src_feature->x1)
        merge_data->dest_feature->x1 = coord1;

      if (coord2 == merge_data->src_feature->x2)
        merge_data->dest_feature->x2 = coord2;
    }
  
  zMapFeatureTranscriptMergeExon(merge_data->dest_feature, coord1, coord2);
}


/*!
 * \brief Merge a single coord into the scratch column feature
 */
static gboolean scratchMergeCoord(ScratchMergeData merge_data, const int coord, const ZMapBoundaryType boundary, GError **error)
{
  gboolean merged = FALSE;
  
  zMapFeatureRemoveIntrons(merge_data->dest_feature);
  
  /* Merge in the new exons */
  merged = zMapFeatureTranscriptMergeCoord(merge_data->dest_feature, coord, boundary, error);
  
  if (merged)
    zMapFeatureTranscriptRecreateIntrons(merge_data->dest_feature);

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
static gboolean scratchMergeBase(ScratchMergeData merge_data, GError **error)
{
  gboolean merged = FALSE;

  if (ZMAP_IS_WINDOW_FEATURESET_ITEM(merge_data->src_item))
    {
      /* Convert the world coords to sequence coords */
      long seq_start=0.0, seq_end=0.0;
      
      zMapWindowCanvasFeaturesetGetSeqCoord((ZMapWindowFeaturesetItem)(merge_data->src_item),
                                            TRUE,
                                            merge_data->world_x,
                                            merge_data->world_y, 
                                            &seq_start,
                                            &seq_end);

      merged = scratchMergeCoord(merge_data, seq_start, ZMAPBOUNDARY_NONE, error);
    }
  else
    {
      g_set_error(error, g_quark_from_string("ZMap"), 99, "Program error: not a featureset item");
    }

  return merged;
}


/*! 
 * \brief Add/merge a basic feature to the scratch column
 */
static gboolean scratchMergeBasic(ScratchMergeData merge_data, GError **error)
{
  gboolean merged = TRUE;
  
  zMapFeatureRemoveIntrons(merge_data->dest_feature);

  /* Just merge the start/end of the feature */
  scratchMergeCoords(merge_data, merge_data->src_feature->x1, merge_data->src_feature->x2);

  /* Recreate the introns */
  zMapFeatureTranscriptRecreateIntrons(merge_data->dest_feature);
  
  return merged;
}


/*! 
 * \brief Add/merge a splice feature to the scratch column
 */
static gboolean scratchMergeSplice(ScratchMergeData merge_data, GError **error)
{
  gboolean merged = TRUE;
  
  zMapFeatureRemoveIntrons(merge_data->dest_feature);

  /* Just merge the start/end of the feature */
  scratchMergeCoord(merge_data, merge_data->src_feature->x1, merge_data->src_feature->boundary_type, error);

  /* Recreate the introns */
  zMapFeatureTranscriptRecreateIntrons(merge_data->dest_feature);
  
  return merged;
}


/*! 
 * \brief Add/merge an alignment feature to the scratch column
 */
static gboolean scratchMergeAlignment(ScratchMergeData merge_data, GError **error)
{
  gboolean merged = FALSE;
  
  zMapFeatureRemoveIntrons(merge_data->dest_feature);

  /* Loop through each match block and merge it in */
  if (!zMapFeatureAlignmentMatchForeach(merge_data->src_feature, scratchMergeMatchCB, merge_data))
    {
      /* The above returns false if no match blocks were found. If the alignment is 
       * gapped but we don't have this data then we can't process it. Otherwise, it's
       * an ungapped alignment so add the whole thing. */
      if (zMapFeatureAlignmentIsGapped(merge_data->src_feature))
        {
          g_set_error(error, g_quark_from_string("ZMap"), 99, "Cannot copy feature; gapped alignment data is not available");
        }
      else
        {
          scratchMergeCoords(merge_data, merge_data->src_feature->x1, merge_data->src_feature->x2);
          merged = TRUE;
        }
    }

  /* Copy CDS, if set */
  //  zMapFeatureMergeTranscriptCDS(merge_data->src_feature, merge_data->dest_feature);

  /* Recreate the introns */
  zMapFeatureTranscriptRecreateIntrons(merge_data->dest_feature);

  return merged;
}


/*! 
 * \brief Add/merge a transcript feature to the scratch column
 */
static gboolean scratchMergeTranscript(ScratchMergeData merge_data, GError **error)
{
  gboolean merged = TRUE;
  
  zMapFeatureRemoveIntrons(merge_data->dest_feature);

  /* Merge in all of the new exons from the new feature */
  zMapFeatureTranscriptExonForeach(merge_data->src_feature, scratchMergeExonCB, merge_data);
  
  /* Copy CDS, if set */
  zMapFeatureMergeTranscriptCDS(merge_data->src_feature, merge_data->dest_feature);

  /* Recreate the introns */
  zMapFeatureTranscriptRecreateIntrons(merge_data->dest_feature);

  return merged;
}


/*! 
 * \brief Add/merge a feature to the scratch column
 */
static void scratchMergeFeature(ScratchMergeData merge_data)
{
  gboolean merged = FALSE;
  GError *error = NULL;

  if (merge_data->use_subfeature)
    {
      /* Just merge the clicked subfeature */
      ZMapFeatureSubPartSpan sub_feature = NULL;
      ZMapWindowCanvasItem canvas_item = ZMAP_CANVAS_ITEM(merge_data->src_item);

      zMapWindowCanvasItemGetInterval(canvas_item, merge_data->world_x, merge_data->world_y, &sub_feature);
      
      if (sub_feature)
        {
          scratchMergeCoords(merge_data, sub_feature->start, sub_feature->end);
          merged = TRUE;
        }
      else
        {
          g_set_error(&error, g_quark_from_string("ZMap"), 99, 
                      "Could not find subfeature in feature '%s' at coords %d, %d", 
                      g_quark_to_string(merge_data->src_feature->original_id),
                      (int)merge_data->world_x, (int)merge_data->world_y);
        }
    }
  else
    {
      switch (merge_data->src_feature->type)
        {
        case ZMAPSTYLE_MODE_BASIC:
          merged = scratchMergeBasic(merge_data, &error);
          break;
        case ZMAPSTYLE_MODE_ALIGNMENT:
          merged = scratchMergeAlignment(merge_data, &error);
          break;
        case ZMAPSTYLE_MODE_TRANSCRIPT:
          merged = scratchMergeTranscript(merge_data, &error);
          break;
        case ZMAPSTYLE_MODE_SEQUENCE:
          merged = scratchMergeBase(merge_data, &error);
          break;
        case ZMAPSTYLE_MODE_GLYPH:
          merged = scratchMergeSplice(merge_data, &error);
          break;

        case ZMAPSTYLE_MODE_INVALID:       /* fall through */
        case ZMAPSTYLE_MODE_ASSEMBLY_PATH: /* fall through */
        case ZMAPSTYLE_MODE_TEXT:          /* fall through */
        case ZMAPSTYLE_MODE_GRAPH:         /* fall through */
        case ZMAPSTYLE_MODE_PLAIN:         /* fall through */
        case ZMAPSTYLE_MODE_META:          /* fall through */
        default:
          g_set_error(&error, g_quark_from_string("ZMap"), 99, "copy of feature of type %d is not implemented.\n", merge_data->src_feature->type);
          break;
        };
    }
  
  /* Once finished merging, the start/end should now be set */
  if (merged)
    {
      scratchSetStartEndFlag(merge_data->window, TRUE);
    }
  
  if (error)
    {
      if (merged)
        zMapWarning("Warning while copying feature: %s", error->message);
      else
        zMapCritical("Error copying feature: %s", error->message);
      
      g_error_free(error);
      error = NULL;
    }
}


/*!
 * \brief Copy the given feature to the scratch column
 *
 * \param window
 * \param feature The new feature to be copied in
 * \param item The foo canvas item for the new feature
 * \param world_x The clicked x coord
 * \param world_y The clicked y coord
 * \param merge_subfeature If true, just merge the clicked subfeature, otherwise merge the whole feature
 */
void zmapWindowScratchCopyFeature(ZMapWindow window, 
                                  ZMapFeature feature, 
                                  FooCanvasItem *item, 
                                  const double world_x,
                                  const double world_y,
                                  const gboolean use_subfeature)
{
  if (window && feature)
    {
      ZMapStrand strand = ZMAPSTRAND_FORWARD;
      ZMapFeatureSet scratch_featureset = scratchGetFeatureset(window);
      ZMapFeature scratch_feature = scratchGetFeature(scratch_featureset, strand);

      if (scratch_featureset && scratch_feature)
        {
          ScratchMergeDataStruct merge_data = {window, item, feature, scratch_featureset, scratch_feature, world_x, world_y, use_subfeature};
          scratchMergeFeature(&merge_data);

          /* Call back to the View to update all windows */
          ZMapWindowCallbacks window_cbs_G = zmapWindowGetCBs() ;
          ZMapWindowCallbackCommandScratch scratch_cmd = g_new0(ZMapWindowCallbackCommandScratchStruct, 1) ;
          
          /* Set up general command field for callback. */
          scratch_cmd->cmd = ZMAPWINDOW_CMD_COPYTOSCRATCH ;
          scratch_cmd->sequence = window->sequence;
          scratch_cmd->feature = scratch_feature;
          scratch_cmd->feature_set = scratch_featureset;
          scratch_cmd->context = window->feature_context;

          (*(window_cbs_G->command))(window, window->app_data, scratch_cmd) ;
        }
    }
  else
    {
      zMapWarning("%s", "Error: no feature selected\n");
    }
}


/*!
 * \brief Utility to delete all exons from the given feature
 */
static void scratchDeleteFeatureExons(ZMapWindow window, ZMapFeature feature, ZMapFeatureSet feature_set)
{
  if (feature)
    {
      zMapFeatureRemoveExons(feature);
      zMapFeatureRemoveIntrons(feature);
    }  
}


/*!
 * \brief Clear the scratch column
 */
void zmapWindowScratchClear(ZMapWindow window)
{
  if (!window)
    return;
  
  /* Get the singleton features that exist in each strand of the scatch column */
  ZMapFeatureSet feature_set = scratchGetFeatureset(window);
  ZMapFeature feature = scratchGetFeature(feature_set, ZMAPSTRAND_FORWARD);

  /* Delete the exons and introns (the singleton feature always
   * exists so we don't delete the whole thing). */
  scratchDeleteFeatureExons(window, feature, feature_set);

  /* Reset the start-/end-set flag */
  scratchSetStartEndFlag(window, FALSE);
  
  /* Call the callback to the view to redraw everything */
  ZMapWindowCallbacks window_cbs_G = zmapWindowGetCBs() ;
  ZMapWindowCallbackCommandScratch scratch_cmd = g_new0(ZMapWindowCallbackCommandScratchStruct, 1) ;
  
  /* Set up general command field for callback. */
  scratch_cmd->cmd = ZMAPWINDOW_CMD_CLEARSCRATCH ;
  scratch_cmd->sequence = window->sequence;
  scratch_cmd->feature = feature;
  scratch_cmd->feature_set = feature_set;
  scratch_cmd->context = window->feature_context;
  
  (*(window_cbs_G->command))(window, window->app_data, scratch_cmd) ;
}
