/*  File: zmapViewScratch.c
 *  Author: Gemma Barson (gb10@sanger.ac.uk)
 *  Copyright (c) 2012-2014: Genome Research Ltd.
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
#include <ZMap/zmapGLibUtils.h>
#include <zmapView_P.h>





#define SCRATCH_FEATURE_NAME "temp_feature"


typedef enum {ZMAPEDIT_MERGE, ZMAPEDIT_DELETE, ZMAPEDIT_CLEAR} ZMapEditType ;


/* Data about each edit operation in the Scratch column */
typedef struct _EditOperationStruct
{
  ZMapEditType edit_type;      /* the type of edit operation */
  GList *src_feature_ids;      /* the IDs of the feature(s) to be merged in, i.e. the source for the merge */
  long seq_start;              /* sequence coord (sequence features only) */
  long seq_end;                /* sequence coord (sequence features only) */
  ZMapFeatureSubPartSpan subpart; /* the subpart to use, if applicable */
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
static void freeListFull(GList *list, GDestroyNotify free_func)
{
  GList *item = list;

  for ( ; item; item = item->next)
    {
      /* Free the data and any memory we allocated in it */
      EditOperation operation = (EditOperation)(item->data) ;
      editOperationDestroy(operation) ;
      operation = NULL ;
    }

  g_list_free(list);
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

  /* For "clear" operations we set the start pointer to the end of the list, so that all of the
   * features are ignored (but they're still there so we can undo the operation) */
  if (operation->edit_type == ZMAPEDIT_CLEAR)
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
ZMapFeature zmapViewScratchGetFeature(ZMapFeatureSet feature_set)
{
  ZMapFeature feature = NULL;

  ZMapFeatureAny feature_any = (ZMapFeatureAny)feature_set;

  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init (&iter, feature_any->children);

  /* Should only be one, so just get the first */
  while (g_hash_table_iter_next (&iter, &key, &value) && !feature)
    {
      feature = (ZMapFeature)(value);
    }

  return feature;
}


/*!
 * \brief Utility to find a feature from a glist item which is the feature's unique_id.
 */
ZMapFeature getFeature(ZMapView view, GList *list_item)
{
  ZMapFeature result = NULL ;
  zMapReturnValIfFailSafe(list_item, result) ;

  GQuark feature_id = GPOINTER_TO_INT(list_item->data) ;

  if (feature_id)
    {
      ZMapFeatureAny feature_any = zMapFeatureAnyGetFeatureByID((ZMapFeatureAny)view->features, 
                                                                feature_id, 
                                                                ZMAPFEATURE_STRUCT_FEATURE) ;
  
      if (feature_any && feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
        result = (ZMapFeature)feature_any ;
    }

  return result ;
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
static gboolean scratchMergeTranscript(ScratchMergeData merge_data, ZMapFeature feature)
{
  gboolean merged = TRUE;

  /* Merge in all of the new exons from the new feature */
  zMapFeatureTranscriptExonForeach(feature, scratchMergeExonCB, merge_data);

  /* Copy CDS, if set */
  zMapFeatureMergeTranscriptCDS(feature, merge_data->dest_feature);

  return merged;
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
 * \brief Add/merge a feature to the scratch column
 */
static gboolean scratchDoMergeOperation(ScratchMergeData merge_data)
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
      /* Loop through each feature in the list */
      GList *item = operation->src_feature_ids ;

      for ( ; item ; item = item->next)
        {
          ZMapFeature feature = getFeature(merge_data->view, item) ;

          /* If the start/end is not set, set it now from the feature extent */
          if (!scratchGetStartEndFlag(merge_data->view))
           {
              merge_data->dest_feature->x1 = feature->x1;
              merge_data->dest_feature->x2 = feature->x2;
            }

          if (feature)
            {
              switch (feature->mode)
                {
                case ZMAPSTYLE_MODE_ALIGNMENT:      /* fall through */
                case ZMAPSTYLE_MODE_BASIC:
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
                  break ;
                }
            }
        }
    }

  /* Once finished merging, the start/end should now be set */
  if (merged)
    {
      scratchSetStartEndFlag(merge_data->view, TRUE);
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
 * \brief Utility to delete all exons from the given feature
 */
static void scratchDeleteFeatureExons(ZMapView view, ZMapFeature feature, ZMapFeatureSet feature_set)
{
  if (feature)
    {
      zMapFeatureRemoveExons(feature);
    }
}


static void scratchEraseFeature(ZMapView zmap_view)
{
  g_return_if_fail(zmap_view && zmap_view->features) ;
  ZMapFeatureContext context = zmap_view->features ;

  /* Get the singleton features that exist in each strand of the scatch column */
  ZMapFeatureSet feature_set = zmapViewScratchGetFeatureset(zmap_view);
  g_return_if_fail(feature_set) ;

  ZMapFeature feature = zmapViewScratchGetFeature(feature_set) ;
  g_return_if_fail(feature) ;

  /* Delete the exons and introns */
  scratchDeleteFeatureExons(zmap_view, feature, feature_set);

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
                                                          NULL,
                                                          ZMAPSTYLE_MODE_TRANSCRIPT,
                                                          &feature_set->style,
                                                          0,
                                                          0,
                                                          FALSE,
                                                          0.0,
                                                          strand);

  zMapFeatureTranscriptInit(feature);
  zMapFeatureAddTranscriptStartEnd(feature, FALSE, 0, FALSE);
  //zMapFeatureSetAddFeature(feature_set, feature);

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
  zMapReturnIfFail(zmap_view && zmap_view->context_map.styles);

  ZMapFeatureSet featureset = NULL ;
  ZMapFeatureTypeStyle style = NULL ;
  ZMapFeatureSetDesc f2c = NULL;
  ZMapFeatureSource src = NULL;
  GList *list = NULL;
  ZMapFeatureColumn column = NULL;

  ZMapFeatureContextMap context_map = &zmap_view->context_map;

  if((style = zMapFindStyle(context_map->styles, zMapStyleCreateID(ZMAP_FIXED_STYLE_HAND_BUILT_NAME))))
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
          f2c = g_hash_table_lookup(context_map->featureset_2_column, GUINT_TO_POINTER(featureset->unique_id));
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
          src = g_hash_table_lookup(context_map->source_2_sourcedata, GUINT_TO_POINTER(featureset->unique_id));
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
          list = g_hash_table_lookup(context_map->column_2_styles,GUINT_TO_POINTER(col_id));
          if(!list)
            {
              list = g_list_prepend(list,GUINT_TO_POINTER(src->style_id));
              g_hash_table_insert(context_map->column_2_styles,GUINT_TO_POINTER(col_id), list);
            }
        }

      if (context_map->columns)
        {
          column = g_hash_table_lookup(context_map->columns,GUINT_TO_POINTER(col_id));
          if(!column)
            {
              column = g_new0(ZMapFeatureColumnStruct,1);
              column->unique_id = col_id;
              column->style_table = g_list_prepend(NULL, (gpointer)  style);
              /* the rest shoudl get filled in elsewhere */
              g_hash_table_insert(context_map->columns, GUINT_TO_POINTER(col_id), column);
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

  while (list_item)
    {
      EditOperation operation = (EditOperation)(list_item->data);
      gboolean merged = FALSE;

      /* Add info about the current feature to the merge data */
      merge_data.operation = operation;

      /* Do the operation */
      if (operation->edit_type == ZMAPEDIT_CLEAR)
        merged = TRUE ;                                   /* nothing to do */
      else if (operation->edit_type == ZMAPEDIT_MERGE)
        merged = scratchDoMergeOperation(&merge_data) ;
      else if (operation->edit_type == ZMAPEDIT_DELETE)
        merged = scratchDoDeleteOperation(&merge_data) ;
      else
        zMapLogWarning("Program error: edit operation of type '%d' is not supported", operation->edit_type) ;

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
  zMapReturnIfFail(zmap_view && zmap_view->context_map.styles);

  ZMapFeatureSet scratch_featureset = NULL ;
  ZMapFeatureTypeStyle style = NULL ;
  ZMapFeatureSetDesc f2c = NULL;
  ZMapFeatureSource src = NULL;
  GList *list = NULL;
  ZMapFeatureColumn column = NULL;

  ZMapFeatureContextMap context_map = &zmap_view->context_map;
  ZMapFeatureContext context = context_in;
  ZMapFeatureBlock block = block_in;

  if((style = zMapFindStyle(context_map->styles, zMapStyleCreateID(ZMAP_FIXED_STYLE_SCRATCH_NAME))))
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
          f2c = g_hash_table_lookup(context_map->featureset_2_column, GUINT_TO_POINTER(scratch_featureset->unique_id));
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
          src = g_hash_table_lookup(context_map->source_2_sourcedata, GUINT_TO_POINTER(scratch_featureset->unique_id));
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
          list = g_hash_table_lookup(context_map->column_2_styles,GUINT_TO_POINTER(col_id));
          if(!list)
            {
              list = g_list_prepend(list,GUINT_TO_POINTER(src->style_id));
              g_hash_table_insert(context_map->column_2_styles,GUINT_TO_POINTER(col_id), list);
            }
        }

      if (context_map->columns)
        {
          column = g_hash_table_lookup(context_map->columns,GUINT_TO_POINTER(col_id));
          if(!column)
            {
              column = g_new0(ZMapFeatureColumnStruct,1);
              column->unique_id = col_id;
              column->style_table = g_list_prepend(NULL, (gpointer)  style);
              column->order = zMapFeatureColumnOrderNext();
              /* the rest shoudl get filled in elsewhere */
              g_hash_table_insert(context_map->columns, GUINT_TO_POINTER(col_id), column);
            }
        }

      /* Create two empty features, one for each strand */
      ZMapFeature feature_fwd = zMapFeatureCreateEmpty() ;

      zMapFeatureAddStandardData(feature_fwd, "temp_feature_fwd", SCRATCH_FEATURE_NAME,
                                 NULL, NULL,
                                 ZMAPSTYLE_MODE_TRANSCRIPT, &scratch_featureset->style,
                                 0, 0, FALSE, 0.0,
                                 ZMAPSTRAND_FORWARD);

      zMapFeatureTranscriptInit(feature_fwd);
      zMapFeatureAddTranscriptStartEnd(feature_fwd, FALSE, 0, FALSE);

      //zMapFeatureSequenceSetType(feature, ZMAPSEQUENCE_PEPTIDE);
      //zMapFeatureAddFrame(feature, ZMAPFRAME_NONE);

      zMapFeatureSetAddFeature(scratch_featureset, feature_fwd);
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

          view_window = list_item->data ;

          zMapWindowToggleScratchColumn(view_window->window, 0, 0, force_to, force) ;
        }
      while ((list_item = g_list_next(list_item))) ;
    }


}


/*!
 * \brief Utility to convert a GList of ZMapFeatures to a GList of quarks to store in the
 * operation .
 */
static void operationAddFeatureList(EditOperation operation, GList *features)
{
  /* Add the feature IDs to our list. We don't store pointers to the actual features because
   * these can become invalidated e.g. on revcomp */
  GList *feature_item = features ;

  for ( ; feature_item; feature_item = feature_item->next)
    {
      ZMapFeature feature = (ZMapFeature)(feature_item->data) ;
      operation->src_feature_ids = g_list_append(operation->src_feature_ids, GINT_TO_POINTER(feature->unique_id)) ;
    }
}

/*!
 * \brief Copy the given feature(s) into the scratch column
 */
gboolean zmapViewScratchCopyFeatures(ZMapView view,
                                     GList *features,
                                     const long seq_start,
                                     const long seq_end,
                                     ZMapFeatureSubPartSpan subpart,
                                     const gboolean use_subfeature)
{
  if (features)
    {
      view->flags[ZMAPFLAG_SCRATCH_NEEDS_SAVING] = TRUE ;

      EditOperation operation = g_new0(EditOperationStruct, 1);
      
      operation->edit_type = ZMAPEDIT_MERGE ;
      operation->seq_start = seq_start;
      operation->seq_end = seq_end;
      operation->subpart = subpart ;
      operation->use_subfeature = use_subfeature ;
      operation->boundary = ZMAPBOUNDARY_NONE;

      operationAddFeatureList(operation, features) ;

      /* Add this operation to the list and recreate the scratch feature. */
      scratchAddOperation(view, operation);
    }

  return TRUE;
}


/*!
 * \brief Delete a subfeature from the scratch column
 */
gboolean zmapViewScratchDeleteFeatures(ZMapView view,
                                       GList *features,
                                       const long seq_start,
                                       const long seq_end,
                                       ZMapFeatureSubPartSpan subpart,
                                       const gboolean use_subfeature)
{
  if (features)
    {
      view->flags[ZMAPFLAG_SCRATCH_NEEDS_SAVING] = TRUE ;

      EditOperation operation = g_new0(EditOperationStruct, 1);
      
      operation->edit_type = ZMAPEDIT_DELETE ;
      operation->seq_start = seq_start;
      operation->seq_end = seq_end;
      operation->subpart = subpart ;
      operation->use_subfeature = use_subfeature ;
      operation->boundary = ZMAPBOUNDARY_NONE;

      operationAddFeatureList(operation, features) ;

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
  /* Only do anything if we have a visible feature i.e. the operations list is not empty */
  if (view->edit_list_start)
    {
      /* There will be nothing in the scratch column now so nothing needs saving */
      view->flags[ZMAPFLAG_SCRATCH_NEEDS_SAVING] = FALSE ;

      EditOperation operation = g_new0(EditOperationStruct, 1) ;

      operation->edit_type = ZMAPEDIT_CLEAR ;

      /* Add this feature to the list of merged features and recreate the scratch feature. */
      scratchAddOperation(view, operation);
    }

  return TRUE;
}


/*!
 * \brief Undo the last feature-copy into the scratch column
 */
gboolean zmapViewScratchUndo(ZMapView view)
{
  if (view->edit_list_end)
    {
      view->flags[ZMAPFLAG_SCRATCH_NEEDS_SAVING] = TRUE ;

      /* Special treatment if the last operation was a "clear" because clear shifts the start
       * pointer to be the same as the end pointer, so we need to move it back (to the start of
       * the list or to the last "clear" operation, if there is one). */
      EditOperation operation = (EditOperation)(view->edit_list_end->data) ;

      if (operation->edit_type == ZMAPEDIT_CLEAR)
        {
          while (view->edit_list_start && view->edit_list_start->prev)
            {
              EditOperation prev_operation = (EditOperation)(view->edit_list_end->prev->data) ;
              
              if (prev_operation->edit_type == ZMAPEDIT_CLEAR)
                break ;

              view->edit_list_start = view->edit_list_start->prev ;
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

  return TRUE;
}


/*!
 * \brief Redo the last operation
 */
gboolean zmapViewScratchRedo(ZMapView view)
{
  /* Move the end pointer forward */
  if (view->edit_list_end && view->edit_list_end->next)
    {
      view->flags[ZMAPFLAG_SCRATCH_NEEDS_SAVING] = TRUE ;
      view->edit_list_end = view->edit_list_end->next ;

      /* For "clear" operations we need to move the start pointer to the same as the end pointer */
      EditOperation operation = (EditOperation)(view->edit_list_end->data) ;
      if (operation->edit_type == ZMAPEDIT_CLEAR)
        view->edit_list_start = view->edit_list_end ;

      scratchFeatureRecreate(view);
    }
  else if (!view->edit_list_end && view->edit_list)
    {
      view->flags[ZMAPFLAG_SCRATCH_NEEDS_SAVING] = TRUE ;

      /* If the list exists but start/end pointers are null then we had un-done the entire list,
       * so we just need to set the start/end pointers to the first item in the list */
      view->edit_list_start = view->edit_list_end = view->edit_list ;

      /* For "clear" operations we need to move the start pointer to the same as the end pointer */
      EditOperation operation = (EditOperation)(view->edit_list_end->data) ;
      if (operation->edit_type == ZMAPEDIT_CLEAR)
        view->edit_list_start = view->edit_list_end ;

      scratchFeatureRecreate(view);
    }
  else
    {
      zMapWarning("%s", "No operations to redo\n");
    }

  return TRUE;
}
