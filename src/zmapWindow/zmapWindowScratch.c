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
#include <zmapWindow_P.h>	/* ZMapWindow */
#include <zmapWindowScratch_P.h>


/* This indicates whether the start/end of the scratch transcript 
 * has been set yet. If it's unset we set it from the new feature
 * when a feature is copied in (subsequent times the new feature
 * is merged so the transcript extent may not be updated if it
 * already includes the new extent). */
static gboolean start_end_set_G = FALSE; 

  
typedef struct _GetFeaturesetCBDataStruct
{
  GQuark set_id;
  ZMapFeatureSet featureset;
} GetFeaturesetCBDataStruct, *GetFeaturesetCBData;


typedef struct _ScratchMergeDataStruct
{
  ZMapWindow window;
  ZMapFeatureSet feature_set;
  ZMapFeature orig_feature;    /* the scratch column feature, i.e. the destination feature */
  ZMapFeature new_feature;     /* the new feature to be merged in, i.e. the source feature */ 
  int y_pos;
} ScratchMergeDataStruct, *ScratchMergeData;
  

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
  if (g_list_length(fs_list) == 1)
    {         
      GQuark set_id = (GQuark)(GPOINTER_TO_INT(fs_list->data));
      feature_set = zmapWindowGetFeaturesetFromId(window, set_id);
    }
  else
    {
      zMapWarning("Expected 1 featureset in column '%s' but found %d\n", ZMAP_FIXED_STYLE_SCRATCH_NAME, g_list_length(fs_list));
    }

  return feature_set;
}


/*! 
 * \brief Get the single feature that resides in the scratch column featureset
 *
 * \returns The ZMapFeature, or NULL if there was a problem
 */
static ZMapFeature scratchGetFeature(ZMapFeatureSet feature_set)
{
  ZMapFeature feature = NULL;
  
  ZMapFeatureAny feature_any = (ZMapFeatureAny)feature_set;

  if (g_hash_table_size(feature_any->children) != 1)
    {
      zMapWarning("Error merging feature into '%s' column; expected 1 existing feature but found %d\n", g_quark_to_string(feature_any->unique_id), g_hash_table_size(feature_any->children));
    }
  else
    {
      GHashTableIter iter;
      gpointer key, value;
      
      g_hash_table_iter_init (&iter, feature_any->children);
      g_hash_table_iter_next (&iter, &key, &value);
      
      if (value)
        {
          feature = (ZMapFeature)(value);
        }
    }

  return feature;
}


/*! 
 * \brief Merge a single exon into the scratch column feature
 */
static void scratchMergeExonCB(gpointer exon, gpointer user_data)
{
  ZMapSpan exon_span = (ZMapSpan)exon;
  ScratchMergeData merge_data = (ScratchMergeData)user_data;

  /* If this is the first/last exon in the new feature set the start/end */
  if (!start_end_set_G)
    {
      if (exon_span->x1 == merge_data->new_feature->x1)
        merge_data->orig_feature->x1 = exon_span->x1;

      if (exon_span->x2 == merge_data->new_feature->x2)
        merge_data->orig_feature->x2 = exon_span->x2;
    }
  
  zMapFeatureTranscriptMergeExon(merge_data->orig_feature, exon_span->x1, exon_span->x2);
}


/*! 
 * \brief Merge a single alignment match into the scratch column feature
 */
static void scratchMergeMatchCB(gpointer match, gpointer user_data)
{
  ZMapAlignBlock match_block = (ZMapAlignBlock)match;
  ScratchMergeData merge_data = (ScratchMergeData)user_data;

  /* If this is the first/last match in the new feature set the start/end */
  if (!start_end_set_G)
    {
      if (match_block->q1 == merge_data->new_feature->x1)
        merge_data->orig_feature->x1 = match_block->q1;

      if (match_block->q2 == merge_data->new_feature->x2)
        merge_data->orig_feature->x2 = match_block->q2;
    }
  
  zMapFeatureTranscriptMergeExon(merge_data->orig_feature, match_block->q1, match_block->q2);
}


/*! 
 * \brief Add/merge a single base into the scratch column
 */
static void scratchMergeBase(ScratchMergeData merge_data)
{
  zMapFeatureRemoveIntrons(merge_data->orig_feature);

  /* Merge in the new exons */
  zMapFeatureTranscriptMergeBase(merge_data->orig_feature, merge_data->y_pos);

  /* Recreate the introns */
  zMapFeatureTranscriptRecreateIntrons(merge_data->orig_feature);
}


/*! 
 * \brief Add/merge an alignment feature to the scratch column
 */
static void scratchMergeAlignment(ScratchMergeData merge_data)
{
  zMapFeatureRemoveIntrons(merge_data->orig_feature);

  /* Loop through each match block and merge it in */
  if (!zMapFeatureAlignmentMatchForeach(merge_data->new_feature, scratchMergeMatchCB, merge_data))
    {
      /* The above returns false if no match blocks were found. 
       * If the alignment is gapped but we don't have this data
       * then we can't process it. Otherwise, add the whole thing
       * because it's an ungapped alignment. */
      if (zMapFeatureAlignmentIsGapped(merge_data->new_feature))
        {
          zMapWarning("%s", "Cannot copy feature; gapped alignment data is not available");
        }
      else
        {
          if (!start_end_set_G)
            {
              merge_data->orig_feature->x1 = merge_data->new_feature->x1;
              merge_data->orig_feature->x2 = merge_data->new_feature->x2;
            }

          zMapFeatureTranscriptMergeExon(merge_data->orig_feature, merge_data->new_feature->x1, merge_data->new_feature->x2);          
        }
    }

  /* Copy CDS, if set */
  //  zMapFeatureMergeTranscriptCDS(merge_data->new_feature, merge_data->orig_feature);

  /* Recreate the introns */
  zMapFeatureTranscriptRecreateIntrons(merge_data->orig_feature);
}


/*! 
 * \brief Add/merge a transcript feature to the scratch column
 */
static void scratchMergeTranscript(ScratchMergeData merge_data)
{
  zMapFeatureRemoveIntrons(merge_data->orig_feature);

  /* Merge in the new exons */
  zMapFeatureTranscriptExonForeach(merge_data->new_feature, scratchMergeExonCB, merge_data);

  /* Copy CDS, if set */
  zMapFeatureMergeTranscriptCDS(merge_data->new_feature, merge_data->orig_feature);

  /* Recreate the introns */
  zMapFeatureTranscriptRecreateIntrons(merge_data->orig_feature);
}


/*! 
 * \brief Add/merge a feature to the scratch column
 */
static void scratchMergeFeature(ZMapWindow window,
                                ZMapFeatureSet feature_set,
                                ZMapFeature orig_feature,
                                ZMapFeature new_feature,
                                const int y_pos)
{
  ScratchMergeDataStruct merge_data = {window, feature_set, orig_feature, new_feature, y_pos};
  gboolean merged = FALSE;

  switch (new_feature->type)
    {
      case ZMAPSTYLE_MODE_INVALID:
        break;
      case ZMAPSTYLE_MODE_BASIC:
        break;
      case ZMAPSTYLE_MODE_ALIGNMENT:
        scratchMergeAlignment(&merge_data);
        merged = TRUE;
        break;
      case ZMAPSTYLE_MODE_TRANSCRIPT:
        scratchMergeTranscript(&merge_data);
        merged = TRUE;
        break;
      case ZMAPSTYLE_MODE_SEQUENCE:
        scratchMergeBase(&merge_data);
        merged = TRUE;
        break;
      case ZMAPSTYLE_MODE_ASSEMBLY_PATH:
        break;
      case ZMAPSTYLE_MODE_TEXT:
        break;
      case ZMAPSTYLE_MODE_GRAPH:
        break;
      case ZMAPSTYLE_MODE_GLYPH:
        break;
      case ZMAPSTYLE_MODE_PLAIN:
        break;
      case ZMAPSTYLE_MODE_META:
        break;
      default:
        zMapWarning("Unrecognised feature type %d\n", new_feature->type);
        break;
    };

  /* Once finished merging, the start/end should now be set */
  if (merged)
    start_end_set_G = TRUE;
  else
    zMapWarning("Cannot merge features of type %d", new_feature->type);
}


/*!
 * \brief Copy the given feature to the scratch column 
 */
void zmapWindowScratchCopyFeature(ZMapWindow window, 
                                  ZMapFeature new_feature, 
                                  FooCanvasItem *item, 
                                  const double y_pos_in)
{
  if (new_feature)
    {
      ZMapFeatureSet feature_set = scratchGetFeatureset(window);
      ZMapFeature orig_feature = scratchGetFeature(feature_set);

      if (feature_set && orig_feature)
        {
          /* Work out where we are.... */
          int y_pos = 0;
          zmapWindowWorld2SeqCoords(window, item, 0, y_pos_in, 0,0, NULL, &y_pos, NULL) ;

          scratchMergeFeature(window, feature_set, orig_feature, new_feature, y_pos);


          ZMapWindowCallbacks window_cbs_G = zmapWindowGetCBs() ;
          ZMapWindowCallbackCommandScratch scratch_cmd = g_new0(ZMapWindowCallbackCommandScratchStruct, 1) ;
          
          /* Set up general command field for callback. */
          scratch_cmd->cmd = ZMAPWINDOW_CMD_COPYTOSCRATCH ;
          scratch_cmd->sequence = window->sequence;
          scratch_cmd->feature = orig_feature;
          scratch_cmd->feature_set = feature_set;
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
 * \brief Clear the scratch column
 */
void zmapWindowScratchClear(ZMapWindow window)
{
  /* Get the singleton featureset and feature that exist in the scatch column */
  ZMapFeatureSet feature_set = scratchGetFeatureset(window);
  ZMapFeature feature = scratchGetFeature(feature_set);

  if (feature_set && feature)
    {
      ZMapWindowCallbacks window_cbs_G = zmapWindowGetCBs() ;
      ZMapWindowCallbackCommandScratch scratch_cmd = g_new0(ZMapWindowCallbackCommandScratchStruct, 1) ;
          
      /* Set up general command field for callback. */
      scratch_cmd->cmd = ZMAPWINDOW_CMD_CLEARSCRATCH ;
      scratch_cmd->sequence = window->sequence;
      scratch_cmd->feature = feature;
      scratch_cmd->feature_set = feature_set;
      scratch_cmd->context = window->feature_context;
      
      (*(window_cbs_G->command))(window, window->app_data, scratch_cmd) ;
      
      start_end_set_G = FALSE;
    }
}
