/*  File: zmapWindowContainerFeatureSetFeatureFuncs.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2014: Genome Research Ltd.
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
 * Description: Functions that do things like highlight splices,
 *              hide features and so on, i.e. that do things to
 *              feature display in a column.
 *
 * Exported functions: See zmapWindowContainerFeatureSet.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <zmapWindowContainerUtils.h>
#include <zmapWindowContainerFeatureSet_I.h>




/* Most features will be represented by a single feature_item but for aligns like EST's
 * we make a temporary feature that represents a set of feature_items representing all
 * the matches for a single align sequence.
 * 
 *  */
typedef struct MatchDataStructType
{
  gboolean tmp_feature ;                                    /* If TRUE then delete feature when finished. */

  ZMapFeature feature ;                                     /* Feature represented by feature_items */

  GList *feature_items ;                                    /* List of ZMapWindowCanvasFeature. */

} MatchDataStruct, *MatchData ;




/* 
 * Struct for splice or feature highlighting.
 */
typedef struct FeatureFilterStructType
{
  gboolean found_filter_sensitive ;                         /* TRUE => found filter-sensitive column(s). */
  gboolean found_filtered_features ;                        /* TRUE => found feature(s)x to filter/unfilter. */

  /* general */
  int seq_start, seq_end ;                                  /* only mark splices in this range. */
  gboolean cds_match ;                                      /* For transcripts do a CDS-section only match. */

  GList *filter_features ;                                  /* The filtered features (i.e. start/ends)
                                                               of the features. */

  ZMapFeaturePartsList splice_matches ;


  ZMapWindowContainerFilterType selected_type ;             /* Type of filtering of selected feature coords. */
  ZMapWindowContainerFilterType filter_type ;               /* Type of filtering of selected feature coords. */
  ZMapWindowContainerActionType filter_action ;             /* What action should be done on the filtered features. */
  ZMapWindowContainerTargetType filter_target ;             /* What should filtering be applied to. */


  ZMapWindowContainerFeatureSet selected_container_set ;
  ZMapWindowContainerFeatureSet target_column ;

  ZMapWindowContainerFeatureSet curr_target_column ;


  /* splice highlighting.... */
  ZMapWindowFeaturesetItem featureset_item ;


  /* general */

  int feat_y1, feat_y2 ;                                    /* extent of feature(s) from which filter coords
                                                               derived */
  double y1, y2 ;                                           /* Extent of filter coords. */

  GList *splices ;                                          /* The splices (i.e. start/ends) of the features. */


  double curr_start, curr_end ;                             /* Start same as y1, y2 but get reduced as we
                                                               move through the splices. */
  GList *curr_splices ;                                     /* As we move through splices we do not need
                                                               to do all splices so we move down through list. */

  GHashTable *match_features ;
  GList *tmp_match_list ;                                   /* tmp pointer used in a callback. */


  ZMapFeaturePartsList tmp_splice_matches ;
  ZMapFeaturePartsList tmp_non_matches ;

} FeatureFilterStruct, *FeatureFilter ;


/* general struct for callbacks processing feature_item's into matches. */
typedef struct FeatMatchStructType
{
  ZMapWindowContainerFilterType filter_type ;               /* Type of filtering of selected feature coords. */

  ZMapWindowCanvasFeature feature_item ;

  GList **matches ;
} FeatMatchStruct, *FeatMatch ;



static void getFeatureCoords(gpointer data, gpointer user_data) ;
static GList *getPartBoundaries(ZMapFeature feature, ZMapFeatureSubPartType requested_bounds,
                                int seq_start, int seq_end, int *start_out, int *end_out) ;
static GList *clipSubParts(GList *subpart_list, int start, int end) ;

static ZMapWindowContainerFilterRC unfilterFeatures(ZMapWindowContainerActionType filter_action,
                                                    ZMapWindowContainerFeatureSet menu_column,
                                                    ZMapWindowContainerFeatureSet target_column) ;
static void unfilterFeaturesCB(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                             ZMapContainerLevelType level, gpointer user_data) ;
static void unfilterFeatureCB(gpointer data, gpointer user_data) ;
static void unhighlightFeatureCB(gpointer data, gpointer user_data) ;

static void processFilterColumns(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                                 ZMapContainerLevelType level, gpointer user_data) ;

static void filterColumn(FeatureFilter splice_data, ZMapWindowFeaturesetItem featureset_item) ;
static void filterFeatureCB(gpointer data, gpointer user_data) ;
static void hideFeatureCB(gpointer data, gpointer user_data) ;

static void spliceColumn(FeatureFilter filter_data, ZMapWindowFeaturesetItem featureset_item) ;
static void highlightFeature(gpointer data, gpointer user_data) ;
static void addSplicesCB(gpointer data, gpointer user_data) ;

static GList *features2Matches(GList *features, FeatureFilter filter_data_unused) ;
static void feature2Match(gpointer data, gpointer user_data) ;
static GList *groups2TmpFeatures(GList *grouped_features, FeatureFilter filter_data_unused) ;
static void group2Feature(gpointer data, gpointer user_data) ;
static void freeTmpFeatures(GList *tmp_feature_list) ;
static void freeMatchDataCB(gpointer data, gpointer user_data_unused) ;

static ZMapFeatureSubPartType featureModeFilterType2SubpartType(ZMapWindowContainerFilterType filter_type,
                                                                ZMapFeature feature, gboolean cds_match) ;

static GList *removeSelectedFeatures(GList *filter_features, GList *feature_list) ;
static void removeCB(gpointer data, gpointer user_data) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printFeatureCB(gpointer data, gpointer user_data_unused) ;
#endif

static void newAddSplicesCB(gpointer data, gpointer user_data) ;



/*
 *                 Globals
 */





/*
 *                 External routines
 */



/* Various access functions to return state about column filtering settings. */

gboolean zMapWindowContainerFeatureSetIsFilterable(ZMapWindowContainerFeatureSet container_set)
{
  gboolean result = FALSE ;

  zMapReturnValIfFail(container_set, ZMAP_CANVAS_FILTER_INVALID) ;

  result = container_set->col_filter_sensitive ;

  return result ;
}


ZMapWindowContainerFilterType zMapWindowContainerFeatureSetGetSelectedType(ZMapWindowContainerFeatureSet container_set)
{
  ZMapWindowContainerFilterType filter_type = ZMAP_CANVAS_FILTER_INVALID ;

  zMapReturnValIfFail(container_set, ZMAP_CANVAS_FILTER_INVALID) ;

  filter_type = container_set->curr_selected_type ;

  return filter_type ;
}


ZMapWindowContainerFilterType zMapWindowContainerFeatureSetGetFilterType(ZMapWindowContainerFeatureSet container_set)
{
  ZMapWindowContainerFilterType filter_type = ZMAP_CANVAS_FILTER_INVALID ;

  zMapReturnValIfFail(container_set, ZMAP_CANVAS_FILTER_INVALID) ;

  filter_type = container_set->curr_filter_type ;

  return filter_type ;
}


ZMapWindowContainerActionType zMapWindowContainerFeatureSetGetActionType(ZMapWindowContainerFeatureSet container_set)
{
  ZMapWindowContainerActionType action_type = ZMAP_CANVAS_ACTION_INVALID ;

  zMapReturnValIfFail(container_set, ZMAP_CANVAS_ACTION_INVALID) ;

  action_type = container_set->curr_filter_action ;

  return action_type ;
}

gboolean zMapWindowContainerFeatureSetIsCDSMatch(ZMapWindowContainerFeatureSet container_set)
{
  gboolean cds_match = FALSE ;

  zMapReturnValIfFail(container_set, FALSE) ;

  cds_match = container_set->cds_match ;

  return cds_match ;
}






/* Adds splice highlighting data for all the splice matching features in the container_set,
 * the splices get highlighted when the column is redrawn. Any existing highlight data is
 * replaced with the new data.
 * 
 * Returns TRUE if there were splice-aware cols (regardless of whether any features were splice
 * highlighted), returns FALSE if there if there were no splice-aware cols. This latter should be
 * reported to the user otherwise they won't know why no splices appeared.
 * 
 * If splice_highlight_features is NULL this has the effect of turning off splice highlighting but
 * you should use zmapWindowContainerFeatureSetSpliceUnhighlightFeatures().
 * 
 * (See Splice_highlighting.html)
 *  */



ZMapWindowContainerFilterRC zMapWindowContainerFeatureSetFilterFeatures(ZMapWindowContainerFilterType selected_type,
                                                                        ZMapWindowContainerFilterType filter_type,
                                                                        ZMapWindowContainerActionType filter_action,
                                                                        ZMapWindowContainerTargetType filter_target,
                                                                        ZMapWindowContainerFeatureSet filter_column,
                                                                        GList *filter_features,
                                                                        ZMapWindowContainerFeatureSet target_column,
                                                                        int seq_start, int seq_end,
                                                                        gboolean cds_match)
{
  ZMapWindowContainerFilterRC result = ZMAP_CANVAS_FILTER_FAILED ;
  ZMapWindowContainerGroup container_strand ;

  zMapReturnValIfFail((filter_column || filter_features), ZMAP_CANVAS_FILTER_FAILED) ;

  if ((container_strand = zmapWindowContainerUtilsItemGetParentLevel(FOO_CANVAS_ITEM(filter_column),
                                                                     ZMAPCONTAINER_LEVEL_BLOCK)))
    {
      FeatureFilterStruct filter_data = {FALSE, FALSE,
                                         0, 0, FALSE,
                                         NULL, NULL,
                                         ZMAP_CANVAS_FILTER_INVALID, ZMAP_CANVAS_FILTER_INVALID,
                                         ZMAP_CANVAS_ACTION_INVALID,
                                         ZMAP_CANVAS_TARGET_INVALID,
                                         NULL, NULL, NULL,
                                         NULL,
                                         INT_MAX, 0, INT_MAX, 0,
                                         NULL,
                                         0, 0,
                                         NULL} ;
      ZMapContainerUtilsExecFunc process_func_cb ;
      gboolean filter ;

      filter_data.seq_start = seq_start ;
      filter_data.seq_end = seq_end ;
      filter_data.cds_match = cds_match ;
      filter_data.filter_features = filter_features ; 
      filter_data.selected_type = selected_type ;
      filter_data.filter_type = filter_type ;
      filter_data.filter_action = filter_action ;
      filter_data.filter_target = filter_target ;
      filter_data.selected_container_set = filter_column ;
      filter_data.target_column = target_column ;

      filter = (!(filter_data.selected_type == ZMAP_CANVAS_FILTER_NONE
                  && filter_data.filter_type == ZMAP_CANVAS_FILTER_NONE)) ;

      if (filter)
        {
          /* Get the splice coords of all the splice features, returns them in filter_data.splices. */
          g_list_foreach(filter_features, getFeatureCoords, &filter_data) ;

          process_func_cb = processFilterColumns ;
        }
      else
        {
          process_func_cb = unfilterFeaturesCB ;
        }

      zmapWindowContainerUtilsExecute(container_strand,
                                      ZMAPCONTAINER_LEVEL_FEATURESET,
                                      process_func_cb, &filter_data) ;

      if (filter)
        {
          /* Tidy up ! */
          zMapFeatureSubPartListFree(filter_data.splices) ;
        }

      /* Retrieve result. */
      if (!filter_data.found_filter_sensitive)
        result = ZMAP_CANVAS_FILTER_NOT_SENSITIVE ;
      else if (!filter_data.found_filtered_features)
        result = ZMAP_CANVAS_FILTER_NO_MATCHES ;
      else
        result = ZMAP_CANVAS_FILTER_OK ;
    }

  return result ;
}


/*
 *                 Package routines
 */


/* NONE CURRENTLY */





/*
 *                    Internal routines
 */


/* A GFunc() called to get the coordinates of the features in a list, note that
 * these coords are clipped to the seq_start/end if supplied, this allows us
 * to support clipping to the mask. */
static void getFeatureCoords(gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  FeatureFilter filter_data = (FeatureFilter)user_data ;
  int seq_start, seq_end ;
  int start = 0, end = 0 ;
  ZMapFeatureSubPartType subpart_type = ZMAPFEATURE_SUBPART_INVALID ;
  GList *subparts = NULL ;


  if (filter_data->seq_start)
    seq_start = filter_data->seq_start ;
  else
    seq_start = feature->x1 ;

  if (filter_data->seq_end)
    seq_end = filter_data->seq_end ;
  else
    seq_end = feature->x2 ;

  if (!(feature->x1 > seq_end || feature->x2 < seq_start))
    {
      /* set feature coords in here.... */
      /* filter_data y1 and y2 need to be the extent of all features/subparts so reduce as necessary. */
      if (feature->x1 < filter_data->feat_y1)
        filter_data->feat_y1 = feature->x1 ;
      if (feature->x2 > filter_data->feat_y2)
        filter_data->feat_y2 = feature->x2 ;
      
      subpart_type = featureModeFilterType2SubpartType(filter_data->selected_type, feature,
                                                       filter_data->cds_match) ;

      if (subpart_type)
        {
          subparts = getPartBoundaries(feature, subpart_type, seq_start, seq_end, &start, &end) ;

          filter_data->splices = g_list_concat(filter_data->splices, subparts) ;
          
          /* filter_data y1 and y2 need to be the extent of all features/subparts so reduce as necessary. */
          if (start < filter_data->y1)
            filter_data->curr_start = filter_data->y1 = start ;
          if (end > filter_data->y2)
            filter_data->curr_end = filter_data->y2 = end ;
        }
    }

  return ;
}



static GList *getPartBoundaries(ZMapFeature feature, ZMapFeatureSubPartType requested_bounds,
                                int seq_start, int seq_end, int *start_out, int *end_out)
{
  GList *boundaries = NULL ;
  ZMapFeaturePartsList subparts ;

  /* Get extent of feature and also coords of any subparts, note that both will
   * be clipped to seq_start/end. */
  if ((subparts = zMapFeatureSubPartsGet(feature, requested_bounds)))
    {
      int start = 0, end = 0 ;

      /* Remove subparts list from the subpart struct as we only need to return the list. */
      start = subparts->min ;
      end = subparts->max ;
      boundaries = subparts->parts ;
      subparts->parts = NULL ;
      zMapFeaturePartsListDestroy(subparts) ;

      if (start < seq_start || end > seq_end)
        {
          boundaries = clipSubParts(boundaries, seq_start, seq_end) ;

          start = seq_start ;
          end = seq_end ;
        }

      *start_out = start ;
      *end_out = end ;
    }

  return boundaries ;
}


/* BETTER AS A ZMAPFEATURE routine ?? */
/* Clip the subpart list of ZMapFeatureSubPartStruct to lie within start/end,
 * subparts outside of start/end are removed, subparts spanning start/end are
 * clamped to start/end. */
static GList *clipSubParts(GList *subpart_list, int start, int end)
{
  GList *clip_list ;
  GList *curr, *next ;

  clip_list = curr = subpart_list ;

  while (curr)
    {
      ZMapFeatureSubPart span = (ZMapFeatureSubPart)(curr->data) ;

      next = g_list_next(curr) ;

      if (span->start > end || span->end < start)
        {
          g_free(curr->data) ;
          clip_list = g_list_delete_link(clip_list, curr) ;
        }
      else if (span->start >= start && span->end <= end)
        {
          ;
        }
      else
        {
          if (span->start < start)
            span->start = start ;
          if (span->end < end)
            span->end = end ;
        }

      curr = next ;
    }

  return clip_list ;
}



/* Unfiltering routines:
 * 
 *  called for each column to see if it has any filtering applied,
 * if so the filtering is undone. */
static ZMapWindowContainerFilterRC unfilterFeatures(ZMapWindowContainerActionType filter_action,
                                                    ZMapWindowContainerFeatureSet menu_column,
                                                    ZMapWindowContainerFeatureSet target_column)
{
  ZMapWindowContainerFilterRC result = ZMAP_CANVAS_FILTER_FAILED ;
  ZMapWindowContainerGroup container_strand ;

  zMapReturnValIfFail(menu_column, ZMAP_CANVAS_FILTER_FAILED) ;

  if ((container_strand = zmapWindowContainerUtilsItemGetParentLevel(FOO_CANVAS_ITEM(menu_column),
                                                                     ZMAPCONTAINER_LEVEL_BLOCK)))
    {
      FeatureFilterStruct filter_data = {FALSE, FALSE,
                                         0, 0, FALSE,
                                         NULL, NULL,
                                         ZMAP_CANVAS_FILTER_INVALID, ZMAP_CANVAS_FILTER_INVALID,
                                         ZMAP_CANVAS_ACTION_INVALID,
                                         ZMAP_CANVAS_TARGET_INVALID,
                                         NULL, NULL, NULL,
                                         NULL,
                                         INT_MAX, 0, INT_MAX, 0,
                                         NULL,
                                         0, 0,
                                         NULL} ;

      filter_data.target_column = target_column ;

      /* Unhighlight all existing splice highlights. */
      zmapWindowContainerUtilsExecute(container_strand,
                                      ZMAPCONTAINER_LEVEL_FEATURESET,
                                      unfilterFeaturesCB, &filter_data) ;

      /* Retrieve result. */
      if (!filter_data.found_filter_sensitive)
        result = ZMAP_CANVAS_FILTER_NOT_SENSITIVE ;
      else if (!filter_data.found_filtered_features)
        result = ZMAP_CANVAS_FILTER_NO_MATCHES ;
      else
        result = ZMAP_CANVAS_FILTER_OK ;
    }

  return result ;
}


static void unfilterFeaturesCB(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                             ZMapContainerLevelType level, gpointer user_data)
{
  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      {
        ZMapWindowContainerFeatureSet container_set ;
        FeatureFilter filter_data = (FeatureFilter)user_data ;

        container_set = ZMAP_CONTAINER_FEATURESET(zmapWindowContainerChildGetParent(FOO_CANVAS_ITEM(container))) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
        char *col_name ;

        col_name = zmapWindowContainerFeaturesetGetColumnName(container_set) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

        if (container_set->col_filter_sensitive
            && (!(filter_data->target_column) || container_set == filter_data->target_column))
          {
            GFunc filter_cb ;
            ZMapWindowFeaturesetItem featureset_item ;

            filter_data->found_filter_sensitive = TRUE ;

            if (container_set->curr_filter_action == ZMAP_CANVAS_ACTION_HIGHLIGHT_SPLICE)
              filter_cb = unhighlightFeatureCB ;
            else
              filter_cb = unfilterFeatureCB ;

            if ((featureset_item = zmapWindowContainerGetFeatureSetItem(container_set)))
              {
                g_list_foreach(container_set->filtered_features, filter_cb, featureset_item) ;

                g_list_free(container_set->filtered_features) ;
                container_set->filtered_features = NULL ;

                container_set->curr_selected_type = filter_data->selected_type ;
                container_set->curr_filter_type = filter_data->filter_type ;
                container_set->curr_filter_action = filter_data->filter_action ;
                container_set->cds_match = filter_data->cds_match ; 

                /* Record that there was at least one splice-aware column. */
                filter_data->found_filtered_features = TRUE ;
                
              }
          }

        break ;
      }

    default:
      {
        break ;
      }
    }

  return ;
}

/* GFunc() to unhide a canvas feature. */
static void unfilterFeatureCB(gpointer data, gpointer user_data)
{
  ZMapWindowCanvasFeature feature_item = (ZMapWindowCanvasFeature)data ;
  ZMapWindowFeaturesetItem featureset_item = (ZMapWindowFeaturesetItem)user_data ;

  zmapWindowFeaturesetItemCanvasFeatureShowHide(featureset_item, feature_item,
                                                TRUE, ZMWCF_HIDE_USER) ;

  zMapWindowCanvasFeatureRemoveSplicePos(feature_item) ;

  return ;
}

/* GFunc() to remove splice markers from a canvas feature. */
static void unhighlightFeatureCB(gpointer data, gpointer user_data)
{
  ZMapWindowCanvasFeature feature_item = (ZMapWindowCanvasFeature)data ;

  zMapWindowCanvasFeatureRemoveSplicePos(feature_item) ;

  return ;
}




/* Filtering routines:
 * 
 * Called for each column to see if it is to be splice-highlighted and on the same strand as container.
 * If it is then any features that share splices are marked for highlighting.  */
static void processFilterColumns(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                                 ZMapContainerLevelType level, gpointer user_data)
{
  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      {
        ZMapWindowContainerFeatureSet container_set ;
        FeatureFilter filter_data = (FeatureFilter)user_data ;
        ZMapWindowContainerFeatureSet selected_container_set = filter_data->selected_container_set ;
        ZMapWindowFeaturesetItem featureset_item ;

        char *col_name ;


        container_set = ZMAP_CONTAINER_FEATURESET(zmapWindowContainerChildGetParent(FOO_CANVAS_ITEM(container))) ;
        filter_data->curr_target_column = container_set ;

        col_name = zmapWindowContainerFeaturesetGetColumnName(container_set) ;

        /* For filter-sensitive column(s) on same strand as the selected feature(s):
         * if there's no target column (== do all cols) or this is the target column
         * or this column is the selected column and it can be filtered, e.g. multiple transcripts,
         * and we find the featureset_item then do the filtering. */
        if ((container_set->col_filter_sensitive && container_set->strand == selected_container_set->strand)
            && (!(filter_data->target_column) || container_set == filter_data->target_column)
            && !(filter_data->filter_target == ZMAP_CANVAS_TARGET_NOT_SOURCE_COLUMN
                 && filter_data->curr_target_column == filter_data->selected_container_set)
            && (featureset_item = zmapWindowContainerGetFeatureSetItem(container_set)))
          {
            filter_data->found_filter_sensitive = TRUE ;

            /* Get rid of any existing filter type....should be calling the external routine here ?
             * currently don't handle any error here, just go ahead and apply next filter. */
            if (filter_data->curr_target_column->curr_filter_type != ZMAP_CANVAS_FILTER_NONE)
              {
                ZMapWindowContainerFilterRC result ;

                result = unfilterFeatures(filter_data->filter_action,
                                          selected_container_set,
                                          filter_data->curr_target_column) ;
              }

            if (filter_data->filter_action != ZMAP_CANVAS_ACTION_HIGHLIGHT_SPLICE)
              {
                filterColumn(filter_data, featureset_item) ;
              }
            else
              {
                spliceColumn(filter_data, featureset_item) ;
              }

            /* Record type of filtering done on column. */
            filter_data->curr_target_column->curr_selected_type = filter_data->selected_type ;
            filter_data->curr_target_column->curr_filter_type = filter_data->filter_type ;
            filter_data->curr_target_column->curr_filter_action = filter_data->filter_action ;
            filter_data->curr_target_column->cds_match = filter_data->cds_match ; 
          }

        filter_data->curr_target_column = NULL ;

        break ;
      }

    default:
      {
        break ;
      }
    }

  return ;
}


/* Called to filter a single column, does all but the splicing markers. */
static void filterColumn(FeatureFilter filter_data, ZMapWindowFeaturesetItem featureset_item)
{
  ZMapWindowContainerFeatureSet selected_set, target_set ;
  GList *grouped_features ;
  GList *match_list ;
  /* debug..... */
  char *col_name ;
  zmapWindowCanvasFeatureType feature_type ;


  feature_type = zMapWindowCanvasFeaturesetGetFeatureType(featureset_item) ;


  /* Need to check that feature is not one of the selected features if that target option
   * is specified...... */

  selected_set = filter_data->selected_container_set ;
  target_set = filter_data->curr_target_column ;


  col_name = zmapWindowContainerFeaturesetGetColumnName(target_set) ;


  filter_data->curr_splices = filter_data->splices ;
  filter_data->featureset_item = featureset_item ;


  /* We may run into trouble doing the grouping on loads of alignment features...probably
   * something to be revisited during testing.... */

  if (feature_type != FEATURE_ALIGN)
    {
      GList *feature_list ;

      /* Get all features that overlap with the highlight feature(s). */
      feature_list = zMapWindowFeaturesetFindFeatures(featureset_item, filter_data->feat_y1, filter_data->feat_y2) ;

      /* Exclude the selected features from the list if required. */
      if (target_set == selected_set
          && filter_data->filter_target == ZMAP_CANVAS_TARGET_NOT_SOURCE_FEATURES)
        feature_list = removeSelectedFeatures(filter_data->filter_features, feature_list) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      g_list_foreach(feature_list, printFeatureCB, NULL) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      match_list = features2Matches(feature_list, filter_data) ;
    }
  else
    {
      grouped_features = zMapWindowFeaturesetFindGroupedFeatures(featureset_item,
                                                                 filter_data->feat_y1, filter_data->feat_y2, 
                                                                 TRUE) ;

      /* convert the list of lists of features into matches.... */
      match_list = groups2TmpFeatures(grouped_features, filter_data) ;

      /* tidy up. */
      zMapWindowFeaturesetFreeGroupedFeatures(grouped_features) ;
    }

  /* Filter features in target column. */
  g_list_foreach(match_list, filterFeatureCB, filter_data) ;

  /* Free the match list. */
  freeTmpFeatures(match_list) ;
  


  return ;
}


static GList *removeSelectedFeatures(GList *filter_features, GList *feature_list)
{
  GList *feature_list_out = feature_list ;
  
  g_list_foreach(filter_features, removeCB, &feature_list_out) ;

  return feature_list_out ;
}


static void removeCB(gpointer data, gpointer user_data)
{
  GList **feature_list_p = (GList **)user_data ;
  GList *feature_list = *feature_list_p ;
  GList *curr ;
  ZMapFeature selected_feature = (ZMapFeature)data ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  char *feature_name = zMapFeatureName((ZMapFeatureAny)selected_feature) ; 
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  
  curr = feature_list ;

  do
    {
      ZMapWindowCanvasFeature feature_item = (ZMapWindowCanvasFeature)(curr->data) ;
      ZMapFeature feature = zMapWindowCanvasFeatureGetFeature(feature_item) ;

      if (selected_feature->original_id == feature->original_id)
        {
          GList *tmp = curr->next ;

          feature_list = g_list_delete_link(feature_list, curr) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          zMapDebugPrintf("Found selected feature: %s", feature_name) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

          curr = tmp ;
        }
      else
        {
          curr = curr->next ;
        }

    } while (curr) ;

  *feature_list_p = feature_list ;

  return ;
}



/* debug.... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printFeatureCB(gpointer data, gpointer user_data_unused)
{
  ZMapWindowCanvasFeature feature_item = (ZMapWindowCanvasFeature)data ;
  ZMapFeature feature = zMapWindowCanvasFeatureGetFeature(feature_item) ;
  char *feature_name = zMapFeatureName((ZMapFeatureAny)feature) ; 
  
  zMapDebugPrintf("Feature found: %s\n", feature_name) ;

  return ;
}
#endif



/* A GFunc() Called for each feature in the target column that overlaps the selected list. */
static void filterFeatureCB(gpointer data, gpointer user_data)
{
  MatchData match_data = (MatchData)data ;
  FeatureFilter filter_data = (FeatureFilter)user_data ;
  ZMapFeature feature ;
  GList *curr ;
  ZMapFeaturePartsList splice_matches = NULL, non_matches = NULL ;
  ZMapFeatureSubPartType req_sub_part ;
  gboolean result ;
  int slop ;


  char *feature_name ;

  feature = match_data->feature ;

  feature_name = zMapFeatureName((ZMapFeatureAny)feature) ; /* debug.... */

  filter_data->tmp_splice_matches = filter_data->tmp_non_matches = NULL ;


  /* Keep the head of the splices to be compared moving down through the coords list
   * as we move through the features, note we can do this because splices and features are
   * position sorted. */
  curr = filter_data->curr_splices ;
  while (curr)
    {
      ZMapFeatureSubPart splice_span = (ZMapFeatureSubPart)(curr->data) ;

      if (feature->x1 > splice_span->end)
        {
          curr = g_list_next(curr) ;
        }
      else
        {
          /* ok, found a splice to compare. */

          filter_data->curr_splices = curr ;

          filter_data->curr_start = ((ZMapFeatureSubPart)(curr->data))->start ;

          break ;
        }
    }

  /* Map the requested filtering type to the appropriate part of the feature to ask for. */
  req_sub_part = featureModeFilterType2SubpartType(filter_data->filter_type, feature,
                                                   filter_data->cds_match) ;

  slop = zMapStyleFilterTolerance(*(feature->style)) ;

  result = zMapFeatureHasMatchingBoundaries(feature, req_sub_part,
                                            TRUE, filter_data->cds_match, slop,
                                            curr, filter_data->curr_start, filter_data->curr_end,
                                            &splice_matches, &non_matches) ;

  if (result)
    {
      if (filter_data->filter_action == ZMAP_CANVAS_ACTION_HIDE)
        {
          filter_data->splice_matches = splice_matches ;

          g_list_foreach(match_data->feature_items, hideFeatureCB, filter_data) ;

          filter_data->tmp_non_matches = non_matches ;
        }
      else if (filter_data->filter_action == ZMAP_CANVAS_ACTION_SHOW)
        {
          /* Why do we do this...???? */
          filter_data->splice_matches = non_matches ;

          filter_data->tmp_splice_matches = splice_matches ;
        }

      filter_data->found_filtered_features = TRUE ;                   /* Record we found at least
                                                                         one. */
    }
  else 
    {
      if (!splice_matches && !non_matches)
        {
          /* This means feature does not match because it does not match in any way, e.g.
           * feature is wholly contained in an intron or exon...just now a relevant match. */

          g_list_foreach(match_data->feature_items, hideFeatureCB, filter_data) ;
        }
      else
        {
          if (filter_data->filter_action == ZMAP_CANVAS_ACTION_SHOW)
            {
              filter_data->splice_matches = splice_matches ;

              g_list_foreach(match_data->feature_items, hideFeatureCB, filter_data) ;
            }
          else
            {
              filter_data->tmp_splice_matches = splice_matches ;
              filter_data->tmp_non_matches = non_matches ;
            }

          filter_data->found_filtered_features = TRUE ;                   /* Record we found at least
                                                                             one. */
        }
    }

  /* Add any splice markers for matching or non-matching splices. */
  if (filter_data->tmp_splice_matches || filter_data->tmp_non_matches)
    g_list_foreach(match_data->feature_items, newAddSplicesCB, filter_data) ;
      

  if (splice_matches)
    zMapFeaturePartsListDestroy(splice_matches) ;
  if (non_matches)
    zMapFeaturePartsListDestroy(non_matches) ;


  return ;
}


/* Hide the item..... */
static void hideFeatureCB(gpointer data, gpointer user_data)
{
  ZMapWindowCanvasFeature feature_item = (ZMapWindowCanvasFeature)data ;
  FeatureFilter filter_data = (FeatureFilter)user_data ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* debug..... */
  ZMapFeature feature = zMapWindowCanvasFeatureGetFeature(feature_item) ;
  char *feature_name = zMapFeatureName((ZMapFeatureAny)feature) ; 

  zMapDebugPrintf("Feature being hidden is: %s\n", feature_name) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  /* record this feature item in the list of filtered features. */
  filter_data->curr_target_column->filtered_features
    = g_list_append(filter_data->curr_target_column->filtered_features, feature_item) ;

  /* Hide the item */
  zmapWindowFeaturesetItemCanvasFeatureShowHide(filter_data->featureset_item, feature_item,
                                                FALSE, ZMWCF_HIDE_USER) ;

  return ;
}



/* Add splice position/type to the feature_item representing the feature. */
static void newAddSplicesCB(gpointer data, gpointer user_data)
{
  ZMapWindowCanvasFeature feature_item = (ZMapWindowCanvasFeature)data ;
  FeatureFilter filter_data = (FeatureFilter)user_data ;
  ZMapWindowContainerFeatureSet target_column = filter_data->curr_target_column ;

  if (filter_data->tmp_splice_matches)
    g_list_foreach(filter_data->tmp_splice_matches->parts, addSplicesCB, feature_item) ;

  if (filter_data->tmp_non_matches)
    g_list_foreach(filter_data->tmp_non_matches->parts, addSplicesCB, feature_item) ;

  /* record this feature item in the list of splice highlighted features. */
  target_column->filtered_features
    = g_list_append(target_column->filtered_features, feature_item) ;


  return ;
}


/* Called to filter a single column and add splice markers to any feature that matches
 * the selected features. */
static void spliceColumn(FeatureFilter filter_data, ZMapWindowFeaturesetItem featureset_item)
{
  GList *feature_list ;

  /* Get all features that overlap with the splice highlight features. */
  feature_list = zMapWindowFeaturesetFindFeatures(featureset_item, filter_data->feat_y1, filter_data->feat_y2) ;

  /* highlight all splices for those features that match the splice highlight features. */
  filter_data->curr_splices = filter_data->splices ;
  g_list_foreach(feature_list, highlightFeature, filter_data) ;

  g_list_free(feature_list) ;

  return ;
}


/* Called for each feature in the target column that overlaps the splice list. */
static void highlightFeature(gpointer data, gpointer user_data)
{
  ZMapWindowCanvasFeature feature_item = (ZMapWindowCanvasFeature)data ;
  FeatureFilter filter_data = (FeatureFilter)user_data ;
  ZMapFeature feature ;
  ZMapWindowContainerFeatureSet target_column = filter_data->curr_target_column ;
  GList *curr ;
  ZMapFeaturePartsList splice_matches = NULL, non_matches = NULL ;
  ZMapFeatureSubPartType subpart_types ;
  gboolean result ;
  int slop ;



  feature = zMapWindowCanvasFeatureGetFeature(feature_item) ;

  /* Keep the head of the splices to be compared moving down through the coords list
   * as we move through the features, note we can do this because splices and features are
   * position sorted. */
  curr = filter_data->curr_splices ;
  while (curr)
    {
      ZMapFeatureSubPart splice_span = (ZMapFeatureSubPart)(curr->data) ;

      if (feature->x1 > splice_span->end)
        {
          curr = g_list_next(curr) ;

          filter_data->curr_splices = curr ;

          filter_data->curr_start = ((ZMapFeatureSubPart)(curr->data))->start ;
        }
      else
        {
          /* ok, found a splice to compare. */
          break ;
        }
    }

  subpart_types = featureModeFilterType2SubpartType(filter_data->filter_type, feature,
                                                    filter_data->cds_match) ;

  slop = zMapStyleFilterTolerance(*(feature->style)) ;

  if ((result = zMapFeatureHasMatchingBoundaries(feature, subpart_types,
                                                 FALSE, filter_data->cds_match, slop,
                                                 curr, filter_data->curr_start, filter_data->curr_end,
                                                 &splice_matches, &non_matches)))
    {
      /* record this feature item in the list of splice highlighted features. */
      target_column->filtered_features
        = g_list_append(target_column->filtered_features, feature_item) ;

      if (splice_matches)
        g_list_foreach(splice_matches->parts, addSplicesCB, feature_item) ;

      if (non_matches)
        g_list_foreach(non_matches->parts, addSplicesCB, feature_item) ;


      filter_data->found_filtered_features = TRUE ;                   /* Record we found one. */
    }

  else
    {
      /* record this feature item in the list of splice highlighted features. */
      target_column->filtered_features
        = g_list_append(target_column->filtered_features, feature_item) ;


      if (splice_matches)
        g_list_foreach(splice_matches->parts, addSplicesCB, feature_item) ;

      if (non_matches)
        g_list_foreach(non_matches->parts, addSplicesCB, feature_item) ;

      filter_data->found_filtered_features = TRUE ;                   /* Record we found one. */

    }

  if (splice_matches)
    zMapFeaturePartsListDestroy(splice_matches) ;

  return ;
}




/* Add splice position/type to the feature_item representing the feature. */
static void addSplicesCB(gpointer data, gpointer user_data)
{
  ZMapFeatureBoundaryMatch match = (ZMapFeatureBoundaryMatch)data ;
  ZMapWindowCanvasFeature feature_item = (ZMapWindowCanvasFeature)user_data ;

  /* Damn....looks like we need to be able to add this feature item into the list
   * with splice positions.... */



  zMapWindowCanvasFeatureAddSplicePos(feature_item, match->start,
                                      (match->match_type & ZMAPBOUNDARY_MATCH_TYPE_5_MATCH ? TRUE : FALSE),
                                      ZMAPBOUNDARY_5_SPLICE) ;

  zMapWindowCanvasFeatureAddSplicePos(feature_item, match->end,
                                      (match->match_type & ZMAPBOUNDARY_MATCH_TYPE_3_MATCH ? TRUE : FALSE),
                                      ZMAPBOUNDARY_3_SPLICE) ;

  return ;
}



/* 
 *             Some utilities.
 */


/* Some features, e.g. transripts, will have multiple entries in the list of feature_items
 * so we use a hash to make sure we only create one record per feature. */
static GList *features2Matches(GList *features, FeatureFilter filter_data)
{
  GList *match_list = NULL ;

  filter_data->match_features = g_hash_table_new (NULL, NULL) ;

  filter_data->tmp_match_list = NULL ;

  g_list_foreach(features, feature2Match, filter_data) ;

  match_list = filter_data->tmp_match_list ; 
  filter_data->tmp_match_list = NULL ;

  g_hash_table_destroy(filter_data->match_features) ;
  filter_data->match_features = NULL ;

  return match_list ;
}


static void feature2Match(gpointer data, gpointer user_data)
{
  ZMapWindowCanvasFeature feature_item = (ZMapWindowCanvasFeature)data ;
  FeatureFilter filter_data = (FeatureFilter)user_data ;
  ZMapFeature feature ;
  MatchData match_data ;

  char *feature_name ;

  feature = zMapWindowCanvasFeatureGetFeature(feature_item) ;


  feature_name = zMapFeatureUniqueName((ZMapFeatureAny)feature) ;

  if (!(match_data = (MatchData)g_hash_table_lookup(filter_data->match_features, GUINT_TO_POINTER(feature->unique_id))))
    {
      match_data = g_new0(MatchDataStruct, 1) ;

      match_data->feature = feature ;

      filter_data->tmp_match_list = g_list_append(filter_data->tmp_match_list, match_data) ;

      g_hash_table_insert(filter_data->match_features, GUINT_TO_POINTER(feature->unique_id), match_data) ;
    }

  match_data->feature_items = g_list_append(match_data->feature_items, feature_item) ;

  return ;
}




static GList *groups2TmpFeatures(GList *grouped_features, FeatureFilter filter_data_unused)
{
  GList *feature_list = NULL ;

  g_list_foreach(grouped_features, group2Feature, &feature_list) ;

  return feature_list ;
}

/* 
 * 
 * Notes
 * - code assumes each list of features has the same mode and
 *   this must be true or there is an error in the canvas
 *   skiplist grouping code.
 * 
 *  */
/* This routine returns potential matches that are made from the "introns" or "gaps" between
 * consecutive but separate align features, note these features are from the _same_ match sequence
 * and so may represent exons.... */
static void group2Feature(gpointer data, gpointer user_data)
{
  GList *group_list = (GList *)data ;
  GList **feature_list = (GList **)user_data ;
  ZMapWindowCanvasFeature feature_item ;
  ZMapFeature feature ;
  MatchData match_data ;
  GList *curr ;




  feature_item = (ZMapWindowCanvasFeature)(group_list->data) ;
  feature = zMapWindowCanvasFeatureGetFeature(feature_item) ;



  /* Now here we need to construct the fake feature from the first and second objects.... */


  match_data = g_new0(MatchDataStruct, 1) ;


  match_data->tmp_feature = TRUE ;



  /* OK, I'm going to try making a fake transcript, we don't need all the info. that there is an
   * alignment feature..... */
  match_data->feature = zMapFeatureCreateFromStandardData(zMapFeatureName((ZMapFeatureAny)feature),
                                                          NULL, NULL,
                                                          ZMAPSTYLE_MODE_TRANSCRIPT,
                                                          feature->style,
                                                          0, 0,
                                                          FALSE, 0.0,
                                                          feature->strand) ;

  curr = group_list ;
  while (curr)
    {
      ZMapWindowCanvasFeature tmp_item ;
      ZMapFeature tmp_feature ;
      gboolean result ;

      tmp_item = (ZMapWindowCanvasFeature)(curr->data) ;
      tmp_feature = zMapWindowCanvasFeatureGetFeature(tmp_item) ;

      /* Build up the "exons" array using the matches as exons. */
      result = zMapFeatureTranscriptMergeExon(match_data->feature, tmp_feature->x1, tmp_feature->x2) ;

      match_data->feature_items = g_list_append(match_data->feature_items, tmp_item) ;

      curr = g_list_next(curr) ;
    }

  *feature_list = g_list_append(*feature_list, match_data) ;

  return ;
}



static void freeTmpFeatures(GList *tmp_feature_list)
{
  g_list_foreach(tmp_feature_list, freeMatchDataCB, NULL) ;

  g_list_free(tmp_feature_list) ;

  return ;
}

static void freeMatchDataCB(gpointer data, gpointer user_data_unused)
{
  MatchData match_data = (MatchData)data ;

  if (match_data->tmp_feature)
    zMapFeatureDestroy(match_data->feature) ;

  g_list_free(match_data->feature_items) ;

  g_free(match_data) ;

  return ;
}



/* Map from a particular filter_type and feature to the subpart that we need to request.
 * 
 * Relationship is complicated by fact that we are using multiple alignment objects to
 * represent a kind of "transcript" object for the purposes of matching stuff.
 *  */
static ZMapFeatureSubPartType featureModeFilterType2SubpartType(ZMapWindowContainerFilterType filter_type,
                                                                ZMapFeature feature, gboolean cds_match)
{
  ZMapFeatureSubPartType sub_part = ZMAPFEATURE_SUBPART_INVALID ;

  switch (feature->mode)
    {
    case ZMAPSTYLE_MODE_BASIC:
      {
        /* We can't do gaps on a basic feature. */
        switch (filter_type)
          {
          case ZMAP_CANVAS_FILTER_FEATURE:
          case ZMAP_CANVAS_FILTER_PARTS:
            sub_part = ZMAPFEATURE_SUBPART_FEATURE ;
            break ;
          default:
            break ;
          }

        break ;
      }
    case ZMAPSTYLE_MODE_ALIGNMENT:
      {
        /* I'm not doing the sub gaps for aligns just now..can revisit another time.... */
        sub_part = ZMAPFEATURE_SUBPART_FEATURE ;


        break ;
      }
    case ZMAPSTYLE_MODE_TRANSCRIPT:
      {
        switch (filter_type)
          {
          case ZMAP_CANVAS_FILTER_PARTS:
            if (cds_match)
              sub_part = ZMAPFEATURE_SUBPART_EXON_CDS ;
            else
              sub_part = ZMAPFEATURE_SUBPART_EXON ;
            break ;

          case ZMAP_CANVAS_FILTER_GAPS:
            if (cds_match)
              sub_part = ZMAPFEATURE_SUBPART_INTRON_CDS ;
            else
              sub_part = ZMAPFEATURE_SUBPART_INTRON ;
            break ;

          case ZMAP_CANVAS_FILTER_FEATURE:
            sub_part = ZMAPFEATURE_SUBPART_FEATURE ;
            break ;

          default:
            break ;
          }
        break ;
      }
    default:
      {
        break ;
      }
    }

  return sub_part ;
}

