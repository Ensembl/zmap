/*  File: zmapViewScratch.c
 *  Author: Gemma Barson (gb10@sanger.ac.uk)
 *  Copyright (c) 2012-2015: Genome Research Ltd.
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

#include <ZMap/zmap.hpp>
#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapFeature.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <zmapView_P.hpp>






typedef enum 
{
  ZMAPEDIT_MERGE, 
  ZMAPEDIT_SET_CDS_START,
  ZMAPEDIT_SET_CDS_END,
  ZMAPEDIT_SET_CDS_RANGE,
  ZMAPEDIT_DELETE, 
  ZMAPEDIT_CLEAR, 
  ZMAPEDIT_SAVE
} ZMapEditType ;


/* Data about each edit operation in the Scratch column */
typedef struct _EditOperationStruct
{
  ZMapEditType edit_type;      /* the type of edit operation */
  GList *src_feature_ids;      /* the IDs of the feature(s) to be merged in, i.e. the source for
                                * the merge */
  ZMapFeature src_feature;     /* A manually-edited feature to be merged in. This is used for the
                                * Save operation where the feature doesn't exist in a featureset
                                * so we take ownership of it. */
  long seq_start;              /* sequence coord (sequence features only) */
  long seq_end;                /* sequence coord (sequence features only) */
  ZMapFeatureSubPart subpart;  /* the subpart to use, if applicable */
  gboolean use_subfeature;     /* if true, use the clicked subfeature; otherwise use the entire feature */
  ZMapBoundaryType boundary;   /* the boundary type for a single-coord feature */
} EditOperationStruct, *EditOperation;


/* Data about the current merge/delete process */
typedef struct _ScratchMergeDataStruct
{
  ZMapView view;
  ZMapFeatureSet dest_featureset;        /* the scratch column featureset */
  ZMapFeature dest_feature;              /* the scratch column feature, i.e. the destination for the merge */
  GError **error;                        /* gets set if any problems */

  EditOperation operation;               /* details about the operation being performed */

} ScratchMergeDataStruct, *ScratchMergeData;


/* Local function declarations */
static void scratchFeatureRecreateExons(ZMapView view, ZMapFeature feature) ;
static void scratchFeatureRecreate(ZMapView view) ;
void scratchRemoveFeature(gpointer list_data, gpointer user_data) ;


/*!
 * \brief Get the flag which indicates whether the start/end
 * of the feature have been set
 */
static gboolean scratchGetStartEndFlag(ZMapView view)
{
  gboolean value = FALSE;

  value = view->scratch_start_end_set;

  return value;
}


/*!
 * \brief Update the flag which indicates whether the start/end
 * of the feature have been set
 */
static void scratchSetStartEndFlag(ZMapView view, gboolean value)
{
  view->scratch_start_end_set = value;
}


/*!
 * \brief destroy an EditOperation struct
 */
static void editOperationDestroy(EditOperation operation)
{
  if (operation)
    {
      if (operation->subpart)
        g_free(operation->subpart) ;

      g_free(operation);
    }
}


/*
 * \brief Does the same as g_list_free_full, which requires gtk 2.28
 */
static void freeListFull(GList *glist, GDestroyNotify free_func)
{
  GList *item = glist;

  for ( ; item; item = item->next)
    {
      /* Free the data and any memory we allocated in it */
      EditOperation operation = (EditOperation)(item->data) ;
      editOperationDestroy(operation) ;
      operation = NULL ;
    }

  g_list_free(glist);
}


/*!
 * \brief This function clears the redo stack. It should be called after a successful operation (other
 * than an undo or redo) to "reset" where to start doing redo's from.
 */
static gboolean scratchClearRedoStack(ZMapView view)
{
  gboolean result = TRUE ;

  /* Remove all of the features after the end pointer - these operations have been un-done */
  if (view->edit_list_end)
    {
      GList *item = view->edit_list_end->next ;

      while (item)
        {
          EditOperation operation = (EditOperation)(item->data);

          /* Get the next pointer before we remove the current one! */
          GList *next_item = item->next ;

          /* Remove the link from the list and delete the data */
          view->edit_list = g_list_delete_link(view->edit_list, item) ;
          item = next_item ;

          g_free(operation) ;
          operation = NULL ;
        }
    }
  else if (view->edit_list)
    {
      /* If the start/end pointers are null but the list exists it means we've un-done the entire
       * list. So, to clear the redo stack, clear the entire list. */
      freeListFull(view->edit_list, g_free);
      view->edit_list = NULL;
      view->edit_list_start = NULL;
      view->edit_list_end = NULL;
    }

  return result ;
}


/*!
 * \brief Add the given edit operation to the list
 */
static void scratchAddOperation(ZMapView view, EditOperation operation)
{
  /* Clear the redo stack first so we can just append the new operation at the end of the list */
  scratchClearRedoStack(view) ;

  view->edit_list = g_list_append(view->edit_list, operation) ;

  /* If this is the first item, also set the start/end pointers */
  if (!view->edit_list_end)
    {
      view->edit_list_start = view->edit_list ;
      view->edit_list_end = view->edit_list ;
    }
  else
    {
      /* Move the end pointer to include the new item */
      if (view->edit_list_end->next)
        view->edit_list_end = view->edit_list_end->next ;
      else
        zMapWarnIfReached() ; /* shouldn't get here because we should have a new pointer after
                                 the original end pointer! */
    }

  /* For "clear" and "save" operations we set the start pointer to the end of the list,
   * so that all of the operations before this are ignored (but they're still there
   * so we can undo the operation) */
  if (operation->edit_type == ZMAPEDIT_CLEAR || operation->edit_type == ZMAPEDIT_SAVE)
    {
      view->edit_list_start = view->edit_list_end ;
    }

  /* Now recreate the scratch feature from the new list of operations */
  scratchFeatureRecreate(view);
}


/*!
 * \brief Get the single featureset that resides in the scratch column
 *
 * \returns The ZMapFeatureSet, or NULL if the column doesn't exist
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
      feature_set = zmapFeatureContextGetFeaturesetFromId(view->features, set_id);
    }

  return feature_set;
}


/*!
 * \brief Get the single feature that resides in the scratch column featureset
 *
 * \returns The ZMapFeature, or NULL if there was a problem
 */
ZMapFeature zmapViewScratchGetFeature(ZMapView view)
{
  ZMapFeature feature = NULL ;
  ZMapFeatureSet feature_set = zmapViewScratchGetFeatureset(view);

  if (feature_set)
    {
      ZMapFeatureAny feature_any = (ZMapFeatureAny)feature_set;

      GHashTableIter iter;
      gpointer key, value;

      g_hash_table_iter_init (&iter, feature_any->children);

      /* Should only be one, so just get the first */
      while (g_hash_table_iter_next (&iter, &key, &value) && !feature)
        {
          feature = (ZMapFeature)(value);
        }
    }

  return feature;
}


/* Utility to get a feature from a feature ID in a GList item */
ZMapFeature getFeature(ZMapView view, GList *list_item)
{
  ZMapFeature result = NULL ;
  zMapReturnValIfFailSafe(list_item, result) ;

  GQuark feature_id = GPOINTER_TO_INT(list_item->data) ;

  if (feature_id)
    {
      /* First look in our local cache for the feature pointer */
      if (view->feature_cache)
        result = (ZMapFeature)g_hash_table_lookup(view->feature_cache, GINT_TO_POINTER(feature_id)) ;

      if (!result)
        {
          /* If not found, look up the feature and cache it */
          ZMapFeatureAny feature_any = zMapFeatureAnyGetFeatureByID((ZMapFeatureAny)view->features,
                                                                    feature_id,
                                                                    ZMAPFEATURE_STRUCT_FEATURE) ;

          if (feature_any && feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
            {
              result = (ZMapFeature)feature_any ;
              g_hash_table_insert(view->feature_cache, GINT_TO_POINTER(feature_id), (ZMapFeature)feature_any) ;
            }
        }
    }

  return result ;
}


/*!
 * \brief Remove the features in the given list from the scratch column. Does nothing if the
 * features are not used in the construction of the scratch feature.
 */
void zmapViewScratchRemoveFeatures(ZMapView view, GList *feature_list)
{
  zMapReturnIfFailSafe(view && feature_list) ;

  g_list_foreach(feature_list, scratchRemoveFeature, view) ;
}


/*!
 * \brief For a ZMapFeature item from a list, remove the feature from the scratch column. Does
 * nothing if the features are not used in the construction of the scratch feature.
 */
void scratchRemoveFeature(gpointer list_data, gpointer user_data)
{
  ZMapFeature search_feature = (ZMapFeature)list_data ;
  ZMapView view = (ZMapView)user_data ;
  gboolean changed = FALSE ;

  /* Check the search feature is not the annotation feature itself (i.e. its parent is the
   * annotation featureset) */
  GQuark annotation_quark = zMapStyleCreateID(ZMAP_FIXED_STYLE_SCRATCH_NAME) ;

  if (search_feature && !(search_feature->parent && search_feature->parent->unique_id == annotation_quark))
    {
      GList *operation_item = view->edit_list ;

      for (; operation_item; operation_item = operation_item->next)
        {
          EditOperation operation = (EditOperation)(operation_item->data) ;
          GList *feature_item = operation->src_feature_ids ;

          while (feature_item)
            {
              ZMapFeature feature = getFeature(view, feature_item) ;

              /* Increment pointer before removing from list because that will invalidate the iterator */
              GList *cur_item = feature_item ;
              feature_item = feature_item->next ;

              if (feature && feature->unique_id == search_feature->unique_id)
                {
                  operation->src_feature_ids = g_list_delete_link(operation->src_feature_ids, cur_item) ;
                  changed = TRUE ;
                  char *msg = g_strdup_printf("Feature '%s' is no longer valid and has been removed from the Annotation column.\n",
                                              g_quark_to_string(feature->original_id)) ;
                  zMapWarning("%s", msg) ;
                  g_free(msg) ;
                }
            }
        }
    }

  /* If we've changed the list of source features we need to recreate the scratch feature */
  if (changed)
    scratchFeatureRecreate(view);
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
  EditOperation operation = merge_data->operation;

  if (operation->subpart && !scratchGetStartEndFlag(merge_data->view))
    {
      merge_data->dest_feature->x1 = coord1;
      merge_data->dest_feature->x2 = coord2;

      scratchSetStartEndFlag(merge_data->view, TRUE);
    }

  result = zMapFeatureTranscriptMergeExon(merge_data->dest_feature, coord1, coord2);

  return result;
}


/*!
 * \brief Merge a single coord into the scratch column feature
 */
static gboolean scratchMergeCoord(ScratchMergeData merge_data, const int coord)
{
  gboolean merged = FALSE;

  /* Check that the scratch feature already contains at least one
   * exon, otherwise we cannot merge a single coord. */
  if (zMapFeatureTranscriptGetNumExons(merge_data->dest_feature) > 0)
    {
      /* Merge in the new exons */
      merged = zMapFeatureTranscriptMergeCoord(merge_data->dest_feature,
                                               coord,
                                               &merge_data->operation->boundary,
                                               merge_data->error);
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
 * \brief Add/merge a single base into the scratch column
 */
static gboolean scratchMergeBase(ScratchMergeData merge_data)
{
  gboolean merged = FALSE;

  /* If it's a peptide sequence check if it's a met or stop codon and
   * set the boundary type accordingly (only do this if the span we're
   * looking at is a single base) */
  GList *feature_list = merge_data->operation->src_feature_ids ;

  if (merge_data->operation->seq_end - merge_data->operation->seq_start == 2 &&
      feature_list &&
      g_list_length(feature_list) > 0)
    {
      ZMapFeature feature = getFeature(merge_data->view, feature_list) ;

      if (feature && feature->mode == ZMAPSTYLE_MODE_SEQUENCE)
        {
          ZMapSequence sequence = &feature->feature.sequence ;

          if (sequence->type == ZMAPSEQUENCE_PEPTIDE &&
              sequence->length > 0)
            {
              /* Get 0-based coord and convert from dna to peptide coord */
              const int index = (merge_data->operation->seq_start - merge_data->view->view_sequence->start) / 3 ;

              if (index < sequence->length)
                {
                  const char cp = sequence->sequence[index] ;

                  if (cp == '*')
                    {
                      /* Decrement the coord so we don't include the stop codon in the exon */
                      --merge_data->operation->seq_start ;
                      merge_data->operation->boundary = ZMAPBOUNDARY_5_SPLICE ;
                    }
                  else if (cp == 'M' || cp == 'm')
                    {
                      merge_data->operation->boundary = ZMAPBOUNDARY_3_SPLICE ;
                    }
                }
            }
        }
    }

  merged = scratchMergeCoord(merge_data, merge_data->operation->seq_start);

  return merged;
}


/*!
 * \brief Add/merge a basic feature to the scratch column
 */
static gboolean scratchMergeBasic(ScratchMergeData merge_data, ZMapFeature feature)
{
  gboolean merged = TRUE;

  /* Just merge the start/end of the feature */
  merged = scratchMergeCoords(merge_data, feature->x1, feature->x2);

  return merged;
}


/*!
 * \brief Add/merge a splice feature to the scratch column
 */
static gboolean scratchMergeSplice(ScratchMergeData merge_data, ZMapFeature feature)
{
  gboolean merged = FALSE;

  /* Set the boundary type from the splice boundary */
  EditOperation operation = merge_data->operation;
  operation->boundary = feature->boundary_type;

  /* Just merge the start/end of the feature */
  merged = scratchMergeCoord(merge_data, feature->x1);

  return merged;
}


/*!
 * \brief Add/merge a transcript feature to the scratch column
 */
static gboolean scratchMergeTranscript(ScratchMergeData merge_data,
                                       ZMapFeature feature)
{
  gboolean merged = TRUE;

  /* Merge in all of the new exons from the new feature */
  zMapFeatureTranscriptExonForeach(feature, scratchMergeExonCB, merge_data);

  /* Copy CDS, if set */
  zMapFeatureMergeTranscriptCDS(feature, merge_data->dest_feature);

  return merged;
}


/*!
 * \brief Add/merge a variation to the scratch column
 */
static gboolean scratchMergeVariation(ScratchMergeData merge_data, ZMapFeature feature)
{
  gboolean merged = TRUE;

  /* Copying a variation means that we use the variation to modify the sequence of
   * the transcript (rather than it modifying the exon coords as for other features). */
  merged = zMapFeatureAddTranscriptVariation(merge_data->dest_feature, feature, merge_data->error) ;

  return merged;
}


static gboolean featureIsVariation(ZMapFeature feature)
{
  gboolean result = FALSE ;

  if (feature && feature->mode == ZMAPSTYLE_MODE_BASIC &&
      feature->feature.basic.variation_str)
    result = TRUE ;

  return result ;
}


/* Some features (just variations at present) need to be added as additional metadata rather than
 * being used to modify the feature itself. This function returns true if the given feature is
 * metadata type. */
static gboolean featureIsMetadata(ZMapFeature feature)
{
  return featureIsVariation(feature) ;
}


/*!
 * \brief Add/merge a feature to the scratch column
 */
static gboolean scratchMergeFeature(ScratchMergeData merge_data,
                                    ZMapFeature feature,
                                    const gboolean save_attributes)
{
  gboolean merged = FALSE ;
  zMapReturnValIfFail(feature, merged) ;

  if (save_attributes)
    {
      /* Save attributes that can't be saved in the feature itself, namely the feature name and
       * featureset. These can't be saved in the temp feature because the temp feature must have
       * a specific name and featureset (i.e. "temp_feature" in the "annotation"
       * featureset). Only do this if not already set because we don't want to overwrite user-set values. */
      if (feature && !merge_data->view->int_values[ZMAPINT_SCRATCH_ATTRIBUTE_FEATURE])
        zmapViewScratchSaveFeature(merge_data->view, feature->original_id) ;

      if (feature && feature->parent && !merge_data->view->int_values[ZMAPINT_SCRATCH_ATTRIBUTE_FEATURESET])
        zmapViewScratchSaveFeatureSet(merge_data->view, feature->parent->original_id) ;
    }

  /* If the start/end is not set, set it now from the feature extent */
  if (!scratchGetStartEndFlag(merge_data->view) && !featureIsMetadata(feature))
    {
      merge_data->dest_feature->x1 = feature->x1;
      merge_data->dest_feature->x2 = feature->x2;

      scratchSetStartEndFlag(merge_data->view, TRUE);
    }

  if (feature)
    {
      /*
       * (sm23) This is a bit of a hack and may not be the best place for this operation.
       *  But the new feature must be given an SO term!
       *
       * (gb10) The SO term must always be "transcript". We shouldn't be able to get here without this
       * being set because it should always be set when the feature is created. I'll leave this check 
       * in just in case, though... 
       */
      if (!merge_data->dest_feature->SO_accession)
        {
          zMapWarnIfReached() ;
          merge_data->dest_feature->SO_accession = g_quark_from_string("transcript") ;
        }

      switch (feature->mode)
        {
        case ZMAPSTYLE_MODE_ALIGNMENT:      /* fall through */
        case ZMAPSTYLE_MODE_BASIC:
          if (featureIsVariation(feature))
            merged = scratchMergeVariation(merge_data, feature);
          else
            merged = scratchMergeBasic(merge_data, feature);
          break;
        case ZMAPSTYLE_MODE_TRANSCRIPT:
          merged = scratchMergeTranscript(merge_data, feature);
          break;
        case ZMAPSTYLE_MODE_SEQUENCE:
          merged = scratchMergeBase(merge_data);
          break;
        case ZMAPSTYLE_MODE_GLYPH:
          merged = scratchMergeSplice(merge_data, feature);
          break;

        case ZMAPSTYLE_MODE_ASSEMBLY_PATH: /* fall through */
        case ZMAPSTYLE_MODE_TEXT:          /* fall through */
        case ZMAPSTYLE_MODE_GRAPH:         /* fall through */
        case ZMAPSTYLE_MODE_PLAIN:         /* fall through */
        case ZMAPSTYLE_MODE_META:          /* fall through */
          g_set_error(merge_data->error, g_quark_from_string("ZMap"), 99, "Copy of feature of type %d is not implemented.\n", feature->mode);
          break;

        case ZMAPSTYLE_MODE_INVALID:
          g_set_error(merge_data->error, g_quark_from_string("ZMap"), 99, "Tried to merge a feature that has been deleted.\n");
          break ;

        default:
          g_set_error(merge_data->error, g_quark_from_string("ZMap"), 99, "Unexpected feature type %d.\n", feature->mode);
          break ;
        };

      /* If any of the features fail to merge, exit */
      if (!merged)
        {
          /* If the error is not already set, set it now .*/
          if (*merge_data->error == NULL)
            {
              g_set_error(merge_data->error, g_quark_from_string("ZMap"), 99,
                          "Failed to merge feature '%s'.\n", g_quark_to_string(feature->original_id));
            }
        }
    }

  return merged ;
}


/*!
 * \brief Add/merge a feature to the scratch column
 */
static gboolean scratchDoMergeOperation(ScratchMergeData merge_data,
                                        const gboolean first_merge)
{
  gboolean merged = FALSE;
  EditOperation operation = merge_data->operation;

  if (operation->subpart)
    {
      /* We don't currently handle merging of introns */
      if (operation->subpart->subpart != ZMAPFEATURE_SUBPART_INTRON &&
          operation->subpart->subpart != ZMAPFEATURE_SUBPART_INTRON_CDS)
        {
          /* Just merge the subfeature. */
          if (operation->subpart->start == operation->subpart->end)
            merged = scratchMergeCoord(merge_data, operation->subpart->start);
          else
            merged = scratchMergeCoords(merge_data, operation->subpart->start, operation->subpart->end);
        }
      else
        {
          const char *subfeature_desc = zMapFeatureSubPart2Str(operation->subpart->subpart) ;

          g_set_error(merge_data->error, g_quark_from_string("ZMap"), 99,
                      "copy of subfeature of type '%s' is not implemented.\n",
                      subfeature_desc);
        }
    }
  else
    {
      /* Special treatment if it's the first feature to be merged. Only applicable if merging a
       * single feature and if merging the entire feature (i.e. not a subfeature).*/
      //gboolean first_feature = first_merge &&
      //  !merge_data->operation->use_subfeature &&
      //  (g_list_length(operation->src_feature_ids) == 1) ;

      /* Loop through each feature in the list */
      GList *item = operation->src_feature_ids ;

      for ( ; item ; item = item->next)
        {
          ZMapFeature feature = getFeature(merge_data->view, item) ;

          /* If the first feature is a transcript then we save its attributes so that the user
           * can easily overwrite the same transcript. Currently only applicable in standalone
           * zmap. */
          /* gb10: disable this for now because it causes confusion if exporting the temp feature
           * because the temp feature has the same name as the original feature */
          const gboolean save_attributes = FALSE ;
            //!merge_data->view->xremote_client &&
            //first_feature &&
            //!merge_data->operation->use_subfeature &&
            //feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT ;

          merged = scratchMergeFeature(merge_data, feature, save_attributes) ;

          if (!merged)
            break ;
        }
    }

  if (merge_data->error && *(merge_data->error))
    {
      if (merged)
        zMapWarning("Warning while adding feature to Annotation column: %s", (*(merge_data->error))->message);
      else
        zMapCritical("Error adding feature to Annotation column: %s", (*(merge_data->error))->message);

      g_error_free(*(merge_data->error));
      merge_data->error = NULL;
    }

  return merged;
}


/*!
 * \brief Set the CDS for the scratch feature
 */
static gboolean scratchDoSetCDSOperation(ScratchMergeData merge_data)
{
  gboolean merged = FALSE ;
  zMapReturnValIfFail(merge_data, merged) ;

  EditOperation operation = merge_data->operation;

  GList *feature_list = merge_data->operation->src_feature_ids ;
  ZMapFeature feature = getFeature(merge_data->view, feature_list) ;

  if (feature)
    {
      /* 0 means don't set it */
      int cds_start = 0 ;
      int cds_end = 0 ;
      gboolean ok = TRUE ;

      if (feature->mode == ZMAPSTYLE_MODE_SEQUENCE)
        {
          if (operation->edit_type == ZMAPEDIT_SET_CDS_START || operation->edit_type == ZMAPEDIT_SET_CDS_RANGE)
            cds_start = operation->seq_start ;

          if (operation->edit_type == ZMAPEDIT_SET_CDS_END || operation->edit_type == ZMAPEDIT_SET_CDS_RANGE)
            cds_end = operation->seq_end ;
        }
      else if (operation->use_subfeature && operation->subpart)
        {
          if (operation->edit_type == ZMAPEDIT_SET_CDS_START || operation->edit_type == ZMAPEDIT_SET_CDS_RANGE)
            cds_start = operation->subpart->start ;

          if (operation->edit_type == ZMAPEDIT_SET_CDS_END || operation->edit_type == ZMAPEDIT_SET_CDS_RANGE)
            cds_end = operation->subpart->end ;
        }
      else if (operation->use_subfeature)
        {
          if (operation->edit_type == ZMAPEDIT_SET_CDS_START || operation->edit_type == ZMAPEDIT_SET_CDS_RANGE)
            cds_start = feature->x1 ;

          if (operation->edit_type == ZMAPEDIT_SET_CDS_END || operation->edit_type == ZMAPEDIT_SET_CDS_RANGE)
            cds_end = feature->x2 ;
        }
      else
        {
          /* Use overall start/end of all features. Uses cds coords for transcripts unless none
           * set in which case use feature coords */
          GList *item = operation->src_feature_ids ;
          int min_start = 0 ;
          int max_end = 0 ;
          int min_cds_start = 0 ;
          int max_cds_end = 0 ;
          gboolean first = TRUE ;
          gboolean has_cds = FALSE ;

          for ( ; item ; item = item->next)
            {
              ZMapFeature feature = getFeature(merge_data->view, item) ;

              if (first)
                {
                  min_start = feature->x1 ;
                  max_end = feature->x2 ;

                  if (ZMAPFEATURE_HAS_CDS(feature))
                    {
                      has_cds = TRUE ;
                      min_cds_start = zMapFeatureTranscriptGetCDSStart(feature) ;
                      max_cds_end = zMapFeatureTranscriptGetCDSEnd(feature) ;
                    }
                }

              if (feature->x1 < min_start)
                min_start = feature->x1 ;

              if (feature->x2 > max_end)
                max_end = feature->x2 ;

              if (ZMAPFEATURE_HAS_CDS(feature))
                {
                  has_cds = TRUE ;

                  if (zMapFeatureTranscriptGetCDSStart(feature) < min_cds_start)
                    min_cds_start = zMapFeatureTranscriptGetCDSStart(feature) ;

                  if (zMapFeatureTranscriptGetCDSEnd(feature) > max_cds_end)
                    max_cds_end = zMapFeatureTranscriptGetCDSEnd(feature) ;
                }

              first = FALSE ;
            }

          if (operation->edit_type == ZMAPEDIT_SET_CDS_START || operation->edit_type == ZMAPEDIT_SET_CDS_RANGE)
            {
              if (has_cds)
                cds_start = min_cds_start ;
              else
                cds_start = min_start ;
            }
          
          if (operation->edit_type == ZMAPEDIT_SET_CDS_END || operation->edit_type == ZMAPEDIT_SET_CDS_RANGE)
            {
              if (has_cds)
                cds_end = max_cds_end ;
              else
                cds_end = max_end ;
            }
        }

      /* If both start and end are set, check start <= end */
      if (cds_start > 0 && cds_end > 0 && cds_start > cds_end)
        {
          ok = FALSE ;
          g_set_error(merge_data->error, g_quark_from_string("ZMap"), 99,
                      "CDS start must not be more than end (got %d %d)", cds_start, cds_end) ;
        }

      if (ok)
        {
          merged = zMapFeatureMergeTranscriptCDSCoords(merge_data->dest_feature, cds_start, cds_end);
        }
    }
  else
    {
      g_set_error(merge_data->error, g_quark_from_string("ZMap"), 99,
                  "No source feature or coordinate given to set the CDS from.") ;
    }

  return merged ;
}


/*!
 * \brief Delete a feature from the scratch column
 */
static gboolean scratchDoDeleteOperation(ScratchMergeData merge_data)
{
  gboolean merged = FALSE ;
  zMapReturnValIfFail(merge_data, merged) ;

  EditOperation operation = merge_data->operation;

  if (operation->subpart)
    {
      merged = zMapFeatureTranscriptDeleteSubfeatureAtCoord(merge_data->dest_feature, operation->subpart->start);
    }
  else
    {
      g_set_error(merge_data->error, g_quark_from_string("ZMap"), 99,
                  "Program error: a subfeature is required to delete an intron/exon.") ;
    }

  return merged ;
}


/*!
 * \brief Clear the scratch column
 */
static gboolean scratchDoClearOperation(ScratchMergeData merge_data)
{
  zmapViewScratchResetAttributes(merge_data->view) ;
  gboolean merged = TRUE ;
  return merged ;
}


/*!
 * \brief Save a manually-edited feature.
 */
static gboolean scratchDoSaveOperation(ScratchMergeData merge_data)
{
  gboolean merged = FALSE ;
  EditOperation operation = merge_data->operation;

  zMapReturnValIfFail(merge_data && operation, merged) ;

  if (operation->src_feature)
    {
      merged = scratchMergeFeature(merge_data, operation->src_feature, TRUE) ;
    }
  else
    {
      g_set_error(merge_data->error, g_quark_from_string("ZMap"), 99,
                  "Program error: no feature specified for Save operation.") ;
    }

  return merged ;
}


/*!
 * \brief Utility to delete all exons from the given feature
 */
static void scratchDeleteFeatureExons(ZMapView view, ZMapFeature feature, ZMapFeatureSet feature_set)
{
  if (feature)
    {
      zMapFeatureRemoveExons(feature);
    }
}


/*!
 * \brief Utility to delete all metadata from the given feature
 */
static void scratchDeleteFeatureMetadata(ZMapFeature feature)
{
  zMapFeatureRemoveTranscriptVariations(feature, NULL) ;
}


static void scratchEraseFeature(ZMapView zmap_view)
{
  g_return_if_fail(zmap_view && zmap_view->features) ;
  ZMapFeatureContext context = zmap_view->features ;

  /* Get the singleton features that exist in each strand of the scatch column */
  ZMapFeatureSet feature_set = zmapViewScratchGetFeatureset(zmap_view);
  g_return_if_fail(feature_set) ;

  ZMapFeature feature = zmapViewScratchGetFeature(zmap_view) ;
  g_return_if_fail(feature) ;

  /* Delete the exons and introns */
  scratchDeleteFeatureExons(zmap_view, feature, feature_set);

  /* Delete any metadata we've added */
  scratchDeleteFeatureMetadata(feature) ;

  /* Reset the start-/end-set flag */
  scratchSetStartEndFlag(zmap_view, FALSE);

  GList *feature_list = NULL ;
  ZMapFeatureContext context_copy = zmapViewCopyContextAll(context, feature, feature_set, &feature_list, NULL) ;

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

  ZMapStrand strand = ZMAPSTRAND_FORWARD ;

  /* Create the feature with default values */
  ZMapFeature feature = zMapFeatureCreateFromStandardData(SCRATCH_FEATURE_NAME,
                                                          NULL,
                                                          "transcript",
                                                          ZMAPSTYLE_MODE_TRANSCRIPT,
                                                          &feature_set->style,
                                                          0,
                                                          0,
                                                          FALSE,
                                                          0.0,
                                                          strand);

  zMapFeatureTranscriptInit(feature);
  zMapFeatureAddTranscriptStartEnd(feature, FALSE, 0, FALSE);
  //zMapFeatureAddTranscriptCDS(feature, TRUE, cds_start, cds_end);

  /* Create the exons */
  scratchFeatureRecreateExons(zmap_view, feature) ;

  /* Update the feature ID because the coords may have changed */
  feature->unique_id = zMapFeatureCreateID(feature->mode,
                                           (char *)g_quark_to_string(feature->original_id),
                                           feature->strand,
                                           feature->x1,
                                           feature->x2,
                                           0, 0);

  return feature ;
}


static void handBuiltInit(ZMapView zmap_view, ZMapFeatureSequenceMap sequence, ZMapFeatureContext context)
{
  zMapReturnIfFail(zmap_view);

  ZMapFeatureSet featureset = NULL ;
  ZMapFeatureTypeStyle style = NULL ;
  ZMapFeatureSetDesc f2c = NULL;
  ZMapFeatureSource src = NULL;
  GList *glist = NULL;
  ZMapFeatureColumn column = NULL;

  ZMapFeatureContextMap context_map = &zmap_view->context_map;

  if((style = context_map->styles.find_style(zMapStyleCreateID(ZMAP_FIXED_STYLE_HAND_BUILT_NAME))))
    {
      /* Create the featureset */
      featureset = zMapFeatureSetCreate(ZMAP_FIXED_STYLE_HAND_BUILT_NAME, NULL);
      style = zMapFeatureStyleCopy(style);
      featureset->style = style ;
      GQuark col_quark = g_quark_from_string(ZMAP_FIXED_STYLE_HAND_BUILT_NAME);
      GQuark col_id = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_HAND_BUILT_NAME);

      /* Create the context, align and block, and add the featureset to it */
      if (!context)
        context = zmapViewCreateContext(zmap_view, NULL, featureset);

	/* set up featureset2_column and anything else needed */
      if (context_map->featureset_2_column)
        {
          f2c = (ZMapFeatureSetDesc)g_hash_table_lookup(context_map->featureset_2_column, GUINT_TO_POINTER(featureset->unique_id));
          if(!f2c)	/* these just accumulate  and should be removed from the hash table on clear */
            {
              f2c = g_new0(ZMapFeatureSetDescStruct,1);

              f2c->column_id = col_id;
              f2c->column_ID = col_quark;
              f2c->feature_src_ID = col_quark;
              f2c->feature_set_text = ZMAP_FIXED_STYLE_HAND_BUILT_TEXT;
              g_hash_table_insert(context_map->featureset_2_column, GUINT_TO_POINTER(featureset->unique_id), f2c);
            }
        }

      if (context_map->source_2_sourcedata)
        {
          src = (ZMapFeatureSource)g_hash_table_lookup(context_map->source_2_sourcedata, GUINT_TO_POINTER(featureset->unique_id));
          if(!src)
            {
              src = g_new0(ZMapFeatureSourceStruct,1);
              src->source_id = col_quark;
              src->source_text = g_quark_from_string(ZMAP_FIXED_STYLE_HAND_BUILT_TEXT);
              src->style_id = style->unique_id;
              src->maps_to = col_quark;
              g_hash_table_insert(context_map->source_2_sourcedata, GUINT_TO_POINTER(featureset->unique_id), src);
            }
        }

      if (context_map->column_2_styles)
        {
          glist = (GList *)g_hash_table_lookup(context_map->column_2_styles,GUINT_TO_POINTER(col_id));
          if(!glist)
            {
              glist = g_list_prepend(glist,GUINT_TO_POINTER(src->style_id));
              g_hash_table_insert(context_map->column_2_styles,GUINT_TO_POINTER(col_id), glist);
            }
        }

      if (context_map->columns)
        {
          column = (*context_map->columns)[col_id] ;
          if(!column)
            {
              column = g_new0(ZMapFeatureColumnStruct,1);
              column->unique_id = col_id;
              column->column_id = col_quark ;
              column->column_desc = g_strdup(ZMAP_FIXED_STYLE_HAND_BUILT_TEXT);
              column->style_table = g_list_prepend(NULL, (gpointer)  style);
              column->order = zMapFeatureColumnOrderNext(FALSE) ;
              /* the rest shoudl get filled in elsewhere */
              (*context_map->columns)[col_id] = column ;
            }
        }
    }
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
  ScratchMergeDataStruct merge_data = {view, scratch_featureset, scratch_feature, &error, NULL};
  GList *list_item = view->edit_list_start;
  gboolean first_merge = TRUE ;

  while (list_item)
    {
      EditOperation operation = (EditOperation)(list_item->data);
      gboolean merged = FALSE;

      /* Add info about the current feature to the merge data */
      merge_data.operation = operation;

      /* Do the operation */
      switch (operation->edit_type)
        {
          case ZMAPEDIT_MERGE:
            merged = scratchDoMergeOperation(&merge_data, first_merge) ;
            first_merge = FALSE ;
            break ;
          case ZMAPEDIT_SET_CDS_START: /* fall through */
          case ZMAPEDIT_SET_CDS_END:   /* fall through */
          case ZMAPEDIT_SET_CDS_RANGE:
            merged = scratchDoSetCDSOperation(&merge_data) ;
            break ;
          case ZMAPEDIT_DELETE:
            merged = scratchDoDeleteOperation(&merge_data) ;
            break ;
          case ZMAPEDIT_CLEAR:
            merged = scratchDoClearOperation(&merge_data) ;
            first_merge = TRUE ;
            break ;
          case ZMAPEDIT_SAVE:
            merged = scratchDoSaveOperation(&merge_data) ;
            break ;
          default:
            zMapLogWarning("Program error: edit operation of type '%d' is not supported", operation->edit_type) ;
        }

      GList *next_item = list_item->next;

      /* If we attempted to merge but it failed, remove the feature from the list.
       * Remember the next pointer first because the current item will be removed. */
      if (!merged)
        {
          /* This is probably the end item in the list so update the end pointer */
          if (list_item == view->edit_list_end)
            {
              view->edit_list_end = view->edit_list_end->prev ;

              if (view->edit_list_end == NULL)
                {
                  /* This was the first item and it failed so reset start/end to null to indicate
                   * there's nothing valid in the list */
                  view->edit_list_end = view->edit_list_start = NULL ;
                }
            }
          else if (view->edit_list_start == list_item)
            {
              view->edit_list_start = view->edit_list_start->next ;
            }

          /* Now delete the item from the list and free the data */
          view->edit_list = g_list_delete_link(view->edit_list, list_item) ;
          list_item = NULL ;

          editOperationDestroy(operation) ;
          operation = NULL ;
        }

      /* If we're at the last item in the valid list, finish now */
      if (list_item == view->edit_list_end)
        break ;

      list_item = next_item ;
    }
}


/*!
 * /brief Recreate the scratch feature from the list of merged features
 */
static void scratchFeatureRecreate(ZMapView view)
{
  /* Erase the existing scratch feature */
  scratchEraseFeature(view) ;

  /* Create the new feature */
  ZMapFeature feature = scratchCreateNewFeature(view) ;

  /* Merge it in */
  ZMapFeatureSet feature_set = zmapViewScratchGetFeatureset(view);

  if (feature_set && feature)
    zmapViewMergeNewFeature(view, feature, feature_set) ;
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
  zMapReturnIfFail(zmap_view);

  ZMapFeatureSet scratch_featureset = NULL ;
  ZMapFeatureTypeStyle style = NULL ;
  ZMapFeatureSetDesc f2c = NULL;
  ZMapFeatureSource src = NULL;
  GList *glist = NULL;
  ZMapFeatureColumn column = NULL;

  ZMapFeatureContextMap context_map = &zmap_view->context_map;
  ZMapFeatureContext context = context_in;
  ZMapFeatureBlock block = block_in;

  if((style = context_map->styles.find_style(zMapStyleCreateID(ZMAP_FIXED_STYLE_SCRATCH_NAME))))
    {
      /* Create the featureset */
      scratch_featureset = zMapFeatureSetCreate(ZMAP_FIXED_STYLE_SCRATCH_NAME, NULL);
      style = zMapFeatureStyleCopy(style);
      scratch_featureset->style = style ;
      GQuark col_quark = g_quark_from_string(ZMAP_FIXED_STYLE_SCRATCH_NAME);
      GQuark col_id = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_SCRATCH_NAME);

      /* Create the context, align and block, and add the featureset to it */
      if (!context || !block)
        context = zmapViewCreateContext(zmap_view, NULL, scratch_featureset);
      else
        zMapFeatureBlockAddFeatureSet(block, scratch_featureset);

      /* set up featureset2_column and anything else needed */
      if (context_map->featureset_2_column)
        {
          f2c = (ZMapFeatureSetDesc)g_hash_table_lookup(context_map->featureset_2_column, GUINT_TO_POINTER(scratch_featureset->unique_id));
          if(!f2c)	/* these just accumulate  and should be removed from the hash table on clear */
            {
              f2c = g_new0(ZMapFeatureSetDescStruct,1);

              f2c->column_id = col_id;
              f2c->column_ID = col_quark;
              f2c->feature_src_ID = col_quark;
              f2c->feature_set_text = ZMAP_FIXED_STYLE_SCRATCH_TEXT;
              g_hash_table_insert(context_map->featureset_2_column, GUINT_TO_POINTER(scratch_featureset->unique_id), f2c);
            }
        }

      if (context_map->source_2_sourcedata)
        {
          src = (ZMapFeatureSource)g_hash_table_lookup(context_map->source_2_sourcedata, GUINT_TO_POINTER(scratch_featureset->unique_id));
          if(!src)
            {
              src = g_new0(ZMapFeatureSourceStruct,1);
              src->source_id = col_quark;
              src->source_text = g_quark_from_string(ZMAP_FIXED_STYLE_SCRATCH_TEXT);
              src->style_id = style->unique_id;
              src->maps_to = col_id;
              g_hash_table_insert(context_map->source_2_sourcedata, GUINT_TO_POINTER(scratch_featureset->unique_id), src);
            }
        }

      if (context_map->column_2_styles)
        {
          glist = (GList *)g_hash_table_lookup(context_map->column_2_styles,GUINT_TO_POINTER(col_id));
          if(!glist)
            {
              glist = g_list_prepend(glist,GUINT_TO_POINTER(src->style_id));
              g_hash_table_insert(context_map->column_2_styles,GUINT_TO_POINTER(col_id), glist);
            }
        }

      if (context_map->columns)
        {
          column = (*context_map->columns)[col_id] ;
          if(!column)
            {
              column = g_new0(ZMapFeatureColumnStruct,1);
              column->unique_id = col_id;
              column->column_id = col_quark ;
              column->column_desc = g_strdup(ZMAP_FIXED_STYLE_SCRATCH_TEXT);
              column->style_table = g_list_prepend(NULL, (gpointer)  style);
              column->order = zMapFeatureColumnOrderNext(FALSE);
              /* the rest shoudl get filled in elsewhere */
              (*context_map->columns)[col_id] = column ;
            }
        }

      /* Create the empty feature */
      ZMapFeature new_feature = zMapFeatureCreateEmpty() ;

      zMapFeatureAddStandardData(new_feature, "temp_feature", SCRATCH_FEATURE_NAME,
                                 NULL, "transcript",
                                 ZMAPSTYLE_MODE_TRANSCRIPT, &scratch_featureset->style,
                                 0, 0, FALSE, 0.0,
                                 ZMAPSTRAND_FORWARD);

      zMapFeatureTranscriptInit(new_feature);
      zMapFeatureAddTranscriptStartEnd(new_feature, FALSE, 0, FALSE);

      //zMapFeatureSequenceSetType(feature, ZMAPSEQUENCE_PEPTIDE);
      //zMapFeatureAddFrame(feature, ZMAPFRAME_NONE);

      zMapFeatureSetAddFeature(scratch_featureset, new_feature);
    }

  /* Also initialise the "hand_built" column. xace puts newly created
   * features in this column. */
  handBuiltInit(zmap_view, sequence, context);

  /* Merge our context into the view's context and view the diff context */
  if (!context_in)
    {
      ZMapFeatureContext diff_context ;
      ZMapFeatureContextMergeStats merge_stats ;

      diff_context = zmapViewMergeInContext(zmap_view, context, &merge_stats);

      zmapViewDrawDiffContext(zmap_view, &diff_context, NULL);

      /* Gemma, use the stats if you need to....Ed */
      g_free(merge_stats) ;
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

          view_window = (ZMapViewWindow)(list_item->data) ;

          zMapWindowToggleScratchColumn(view_window->window, 0, 0, force_to, force) ;
        }
      while ((list_item = g_list_next(list_item))) ;
    }
}


/*!
 * \brief Utility to convert a GList of ZMapFeatures to a GList of quarks to store in the
 * operation .
 */
static void operationAddFeatureList(ZMapView view, EditOperation operation, GList *features)
{
  /* Add the feature IDs to our list. We don't store pointers to the actual features because
   * these can become invalidated e.g. on revcomp */
  GList *feature_item = features ;

  for ( ; feature_item; feature_item = feature_item->next)
    {
      ZMapFeature feature = (ZMapFeature)(feature_item->data) ;
      operation->src_feature_ids = g_list_append(operation->src_feature_ids, GINT_TO_POINTER(feature->unique_id)) ;
      
      /* Cache used feature pointers so that we can find them quickly. Gets cleared if pointers
       * are invalidated e.g. on revcomp. */
      if (!view->feature_cache)
        view->feature_cache = g_hash_table_new(NULL, NULL); 

      g_hash_table_insert(view->feature_cache, GINT_TO_POINTER(feature->unique_id), feature) ;
    }
}


/*!
 * \brief Copy the given feature(s) into the scratch column
 */
gboolean zmapViewScratchCopyFeatures(ZMapView view,
                                     GList *features,
                                     const long seq_start,
                                     const long seq_end,
                                     ZMapFeatureSubPart subpart,
                                     const gboolean use_subfeature)
{
  if (features)
    {
      /* If the Annotation column is disabled then enable it. Usually we will not get here if
         the Annotation column is disabled, but we can if the user uses a shortcut key to copy a
         feature. We could disallow this, but actually the user is unlikely to use this shortcut
         unless they want to enable the column so allowing it makes this a very quick and easy way
         for the user to enable the column (since it is disabled by default). */
      if (!zMapViewGetFlag(view, ZMAPFLAG_ENABLE_ANNOTATION))
        zMapViewSetFlag(view, ZMAPFLAG_ENABLE_ANNOTATION, TRUE) ;

      zMapViewSetFlag(view, ZMAPFLAG_SAVE_SCRATCH, TRUE) ;

      EditOperation operation = g_new0(EditOperationStruct, 1);

      operation->edit_type = ZMAPEDIT_MERGE ;
      operation->seq_start = seq_start;
      operation->seq_end = seq_end;
      operation->subpart = subpart ;
      operation->use_subfeature = use_subfeature ;
      operation->boundary = ZMAPBOUNDARY_NONE;

      operationAddFeatureList(view, operation, features) ;

      /* Add this operation to the list and recreate the scratch feature. */
      scratchAddOperation(view, operation);
    }

  return TRUE;
}


/*!
 * \brief Set the CDS start and/or end from the given feature(s)
 */
gboolean zmapViewScratchSetCDS(ZMapView view,
                               GList *features,
                               const long seq_start,
                               const long seq_end,
                               ZMapFeatureSubPart subpart,
                               const gboolean use_subfeature,
                               const gboolean set_cds_start,
                               const gboolean set_cds_end)
{
  gboolean result = TRUE ;

  /* Disallow this operation if the Annotation column is not enabled. (Should only happen after a
   * key press event so should be fine to ignore this without user feedback as the Annotation
   * column keys have no effect if the column is disabled.) */
  if (features && zMapViewGetFlag(view, ZMAPFLAG_ENABLE_ANNOTATION))
    {
      zMapViewSetFlag(view, ZMAPFLAG_SAVE_SCRATCH, TRUE) ;

      EditOperation operation = g_new0(EditOperationStruct, 1);

      if (set_cds_start && set_cds_end)
        operation->edit_type = ZMAPEDIT_SET_CDS_RANGE ;
      else if (set_cds_start)
        operation->edit_type = ZMAPEDIT_SET_CDS_START ;
      else if (set_cds_end)
        operation->edit_type = ZMAPEDIT_SET_CDS_END ;
      else
        result = FALSE ;

      if (result)
        {
          operation->seq_start = seq_start;
          operation->seq_end = seq_end;
          operation->subpart = subpart ;
          operation->use_subfeature = use_subfeature ;
          operation->boundary = ZMAPBOUNDARY_NONE;

          operationAddFeatureList(view, operation, features) ;

          /* Add this operation to the list and recreate the scratch feature. */
          scratchAddOperation(view, operation);
        }
      else
        {
          zMapLogWarning("%s", "Error setting CDS: boundary not specified") ;
        }
    }

  return result;
}


/*!
 * \brief Delete a subfeature from the scratch column
 */
gboolean zmapViewScratchDeleteFeatures(ZMapView view,
                                       GList *features,
                                       const long seq_start,
                                       const long seq_end,
                                       ZMapFeatureSubPart subpart,
                                       const gboolean use_subfeature)
{
  /* Disallow this operation if the Annotation column is not enabled. (Should only happen after a
   * key press event so should be fine to ignore this without user feedback as the Annotation
   * column keys have no effect if the column is disabled.) */
  if (features && zMapViewGetFlag(view, ZMAPFLAG_ENABLE_ANNOTATION))
    {
      zMapViewSetFlag(view, ZMAPFLAG_SAVE_SCRATCH, TRUE) ;

      EditOperation operation = g_new0(EditOperationStruct, 1);

      operation->edit_type = ZMAPEDIT_DELETE ;
      operation->seq_start = seq_start;
      operation->seq_end = seq_end;
      operation->subpart = subpart ;
      operation->use_subfeature = use_subfeature ;
      operation->boundary = ZMAPBOUNDARY_NONE;

      operationAddFeatureList(view, operation, features) ;

      /* Add this operation to the list and recreate the scratch feature. */
      scratchAddOperation(view, operation);
    }

  return TRUE;
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
  /* Disallow this operation if the Annotation column is not enabled. (Should only happen after a
   * key press event so should be fine to ignore this without user feedback as the Annotation
   * column keys have no effect if the column is disabled.) */
  if (zMapViewGetFlag(view, ZMAPFLAG_ENABLE_ANNOTATION))
    {
      /* Only do anything if we have a visible feature i.e. the operations list is not empty */
      if (view->edit_list_start)
        {
          /* There will be nothing in the scratch column now so nothing needs saving */
          zMapViewSetFlag(view, ZMAPFLAG_SAVE_SCRATCH, FALSE) ;

          EditOperation operation = g_new0(EditOperationStruct, 1) ;

          operation->edit_type = ZMAPEDIT_CLEAR ;

          /* Add this feature to the list of merged features and recreate the scratch feature. */
          scratchAddOperation(view, operation);
        }
    }

  return TRUE;
}


/*!
 * \brief Undo the last feature-copy into the scratch column
 */
gboolean zmapViewScratchUndo(ZMapView view)
{
  /* Disallow this operation if the Annotation column is not enabled. (Should only happen after a
   * key press event so should be fine to ignore this without user feedback as the Annotation
   * column keys have no effect if the column is disabled.) */
  if (zMapViewGetFlag(view, ZMAPFLAG_ENABLE_ANNOTATION))
    {
      if (view->edit_list_end)
        {
          zMapViewSetFlag(view, ZMAPFLAG_SAVE_SCRATCH, TRUE) ;

          /* Special treatment if the last operation was a "clear" or "save" because these shift the start
           * pointer to be the same as the end pointer, so we need to move it back (to the start of
           * the list or to the last clear/save operation, if there is one). */
          EditOperation operation = (EditOperation)(view->edit_list_end->data) ;

          if (operation->edit_type == ZMAPEDIT_CLEAR || operation->edit_type == ZMAPEDIT_SAVE)
            {
              while (view->edit_list_start && view->edit_list_start->prev)
                {
                  EditOperation prev_operation = (EditOperation)(view->edit_list_start->prev->data) ;

                  /* If the previous operaition is a clear then leave the start at the current
                   * operation (i.e. the one after the clear, which is where the feature
                   * construction starts). */
                  if (prev_operation->edit_type == ZMAPEDIT_CLEAR)
                    break ;

                  /* Move the start back to the prev operation */
                  view->edit_list_start = view->edit_list_start->prev ;

                  /* If the operation is a save then stop here (unlike "clear" we want this to be
                   * the current start operation because the save operation constructs the feature) */
                  if (prev_operation->edit_type == ZMAPEDIT_SAVE)
                    break ;
                }
            }

          /* In all cases, move the end pointer back one place */
          view->edit_list_end = view->edit_list_end->prev ;

          /* If we have no operations left in the list, null the start pointer too */
          if (view->edit_list_end == NULL)
            view->edit_list_start = NULL ;

          if (view->edit_list_start)
            {

            }

          scratchFeatureRecreate(view);
        }
      else
        {
          zMapWarning("%s", "No operations to undo\n");
        }
    }

  return TRUE;
}


/*!
 * \brief Redo the last operation
 */
gboolean zmapViewScratchRedo(ZMapView view)
{
  /* Disallow this operation if the Annotation column is not enabled. (Should only happen after a
   * key press event so should be fine to ignore this without user feedback as the Annotation
   * column keys have no effect if the column is disabled.) */
  if (zMapViewGetFlag(view, ZMAPFLAG_ENABLE_ANNOTATION))
    {
      /* Move the end pointer forward */
      if (view->edit_list_end && view->edit_list_end->next)
        {
          zMapViewSetFlag(view, ZMAPFLAG_SAVE_SCRATCH, TRUE) ;
          view->edit_list_end = view->edit_list_end->next ;

          /* For clear/save operations we need to move the start pointer to the same as the end pointer */
          EditOperation operation = (EditOperation)(view->edit_list_end->data) ;
          if (operation->edit_type == ZMAPEDIT_CLEAR || operation->edit_type == ZMAPEDIT_SAVE)
            view->edit_list_start = view->edit_list_end ;

          scratchFeatureRecreate(view);
        }
      else if (!view->edit_list_end && view->edit_list)
        {
          zMapViewSetFlag(view, ZMAPFLAG_SAVE_SCRATCH, TRUE) ;

          /* If the list exists but start/end pointers are null then we had un-done the entire list,
           * so we just need to set the start/end pointers to the first item in the list */
          view->edit_list_start = view->edit_list_end = view->edit_list ;

          /* For "clear" and "save" operations we need to move the start pointer to the
           * same as the end pointer */
          EditOperation operation = (EditOperation)(view->edit_list_end->data) ;
          if (operation->edit_type == ZMAPEDIT_CLEAR || operation->edit_type == ZMAPEDIT_SAVE)
            view->edit_list_start = view->edit_list_end ;

          scratchFeatureRecreate(view);
        }
      else
        {
          zMapWarning("%s", "No operations to redo\n");
        }
    }

  return TRUE;
}


/*!
 * \brief Called when the user saves manual edits to the scratch feature.
 *
 * The given feature is the feature that has been constructed from the manual information
 * but this feature does not exist in any featureset etc. so we take ownership of it.
 */
gboolean zmapViewScratchSave(ZMapView view, ZMapFeature feature)
{
  if (zMapViewGetFlag(view, ZMAPFLAG_ENABLE_ANNOTATION) && feature)
    {
      zMapViewSetFlag(view, ZMAPFLAG_SAVE_SCRATCH, TRUE) ;

      /* Save the cached feature name and featureset in the source feature (instead of the
       * temp feature name and featureset which are no longer needed now we're taking
       * ownership of this feature struct) */
      GQuark feature_id = view->int_values[ZMAPINT_SCRATCH_ATTRIBUTE_FEATURE] ;
      GQuark feature_set_id = view->int_values[ZMAPINT_SCRATCH_ATTRIBUTE_FEATURESET] ;

      if (feature_id)
        feature->original_id = feature_id ;

      if (feature_set_id)
        feature->parent = zMapFeatureAnyGetFeatureByID((ZMapFeatureAny)view->features, feature_set_id, ZMAPFEATURE_STRUCT_FEATURESET) ;

      /* Now create the save operation */
      EditOperation operation = g_new0(EditOperationStruct, 1);

      operation->edit_type = ZMAPEDIT_SAVE ;
      operation->src_feature = feature ;

      /* Add this operation to the list and recreate the scratch feature. */
      scratchAddOperation(view, operation) ;
    }

  return TRUE;
}


/*!
 * \brief Called when the user selects to show evidence. Constructs a list of evidence and then
 * calls the given callback (usually to highlight the evidence).
 */
void zmapViewScratchFeatureGetEvidence(ZMapView view, ZMapWindowGetEvidenceCB evidence_cb, gpointer evidence_cb_data)
{
  zMapReturnIfFail(view && evidence_cb) ;

  GList *evidence_list = NULL ;

  /* Loop through each edit operation and append the feature name(s) to the evidence list. We
   * must ignore SAVE operations because they use a fake feature. Also, if the history
   * contains SAVE operations then they will curtail the previous history, so we must loop back
   * beyond them to the first operation in the list (or the last CLEAR operation). */
  GList *operation_item = view->edit_list_start ;

  for ( ; operation_item; operation_item = operation_item->prev)
    {
      /* If this is the first in the list then it's the first that we want */
      if (operation_item->prev == NULL)
        break ;

      /* If this or the previous operation is a clear then it's the first that we want */
      EditOperation operation = (EditOperation)(operation_item->data) ;
      EditOperation operation_prev = (EditOperation)(operation_item->prev->data); 

      if (operation->edit_type == ZMAPEDIT_CLEAR || operation_prev->edit_type == ZMAPEDIT_CLEAR)
        break ;
    }

  for ( ; operation_item; operation_item = operation_item->next)
    {
      EditOperation operation = (EditOperation)(operation_item->data) ;

      if (operation->edit_type != ZMAPEDIT_SAVE)
        {
          /* Append the feature ids from the operation to the result list. Note that concat takes
           * ownership of the second list so make a copy of it first. */
          GList *tmp = g_list_copy(operation->src_feature_ids) ;
          evidence_list = g_list_concat(evidence_list, tmp) ;
        }

      /* If this is the last operation in the redo stack then finish here */
      if (operation_item == view->edit_list_end)
        break ;
    }

  evidence_cb(evidence_list, evidence_cb_data) ;
}


/*
 * \brief Save the feature name to be used when we create the real feature from the scratch feature.
 */
void zmapViewScratchSaveFeature(ZMapView view, GQuark feature_id)
{
  view->int_values[ZMAPINT_SCRATCH_ATTRIBUTE_FEATURE] = feature_id ;
}


/*
 * \brief Save the featureset to be used when we create the real feature from the scratch feature.
 */
void zmapViewScratchSaveFeatureSet(ZMapView view, GQuark feature_set_id)
{
  view->int_values[ZMAPINT_SCRATCH_ATTRIBUTE_FEATURESET] = feature_set_id ;
}


/*
 * \brief Reset the saved attributes
 */
void zmapViewScratchResetAttributes(ZMapView view)
{
  view->int_values[ZMAPINT_SCRATCH_ATTRIBUTE_FEATURE] = 0 ;
  view->int_values[ZMAPINT_SCRATCH_ATTRIBUTE_FEATURESET] = 0 ;
}
