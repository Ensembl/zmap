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

  
typedef struct _GetFeaturesetCBDataStruct
{
  GQuark set_id;
  ZMapFeatureSet featureset;
} GetFeaturesetCBDataStruct, *GetFeaturesetCBData;


typedef struct _ScratchMergeDataStruct
{
  ZMapFeature orig_feature;
  ZMapFeature new_feature;
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

  /* Add this exon to the destination feature */
  zMapFeatureAddTranscriptExonIntron(merge_data->orig_feature, exon_span, NULL);

  /* If this is the first/last exon set the start/end coord */
  if (exon_span->x1 == merge_data->new_feature->x1)
    merge_data->orig_feature->x1 = exon_span->x1;
  
  if (exon_span->x2 == merge_data->new_feature->x2)
    merge_data->orig_feature->x2 = exon_span->x2;  
}


/*! 
 * \brief Merge a single exon into the scratch column feature
 */
static void scratchMergeIntronCB(gpointer intron, gpointer user_data)
{
  ZMapSpan intron_span = (ZMapSpan)intron;
  ScratchMergeData merge_data = (ScratchMergeData)user_data;

  /* Add this intron to the destination feature */
  zMapFeatureAddTranscriptExonIntron(merge_data->orig_feature, NULL, intron_span);

  /* If this is the first/last intron set the start/end coord */
  if (intron_span->x1 == merge_data->new_feature->x1)
    merge_data->orig_feature->x1 = intron_span->x1;
  
  if (intron_span->x2 == merge_data->new_feature->x2)
    merge_data->orig_feature->x2 = intron_span->x2;  
}


/*! 
 * \brief Add/merge a transcript feature to the scratch column
 */
static void scratchMergeTranscript(ScratchMergeData cb_data)
{
  zMapFeatureTranscriptExonForeach(cb_data->new_feature, scratchMergeExonCB, cb_data);
  zMapFeatureTranscriptIntronForeach(cb_data->new_feature, scratchMergeIntronCB, cb_data);
}


/*! 
 * \brief Add/merge a feature to the scratch column
 */
static void scratchMergeFeature(ZMapFeatureSet feature_set, 
                                ZMapFeature orig_feature, 
                                ZMapFeature new_feature)
{
  ScratchMergeDataStruct cb_data = {orig_feature, new_feature};

  switch (new_feature->type)
    {
      case ZMAPSTYLE_MODE_INVALID:
        break;
      case ZMAPSTYLE_MODE_BASIC:
        break;
      case ZMAPSTYLE_MODE_ALIGNMENT:
        break;
      case ZMAPSTYLE_MODE_TRANSCRIPT:
        scratchMergeTranscript(&cb_data);
        break;
      case ZMAPSTYLE_MODE_SEQUENCE:
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
}


/*!
 * \brief Copy the given feature to the scratch column 
 */
void zmapWindowScratchCopyFeature(ZMapWindow window, ZMapFeature new_feature)
{
  if (new_feature)
    {
      ZMapFeatureSet feature_set = scratchGetFeatureset(window);
      ZMapFeature orig_feature = scratchGetFeature(feature_set);

      //      ZMapFeature new_feature = (ZMapFeature)zMapFeatureAnyCopy((ZMapFeatureAny)feature);
  //  zMapFeatureSetAddFeature(feature_set, feature);
      
      if (feature_set && orig_feature)
        scratchMergeFeature(feature_set, orig_feature, new_feature);
    }
  else
    {
      zMapWarning("%s", "Error: no feature selected\n");
    }
}
