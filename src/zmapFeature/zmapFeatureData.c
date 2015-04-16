/*  File: zmapFeatureData.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2014-2015: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk,
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description: Contains functions to do useful things with the
 *              data contained in ZMapFeature objects, e.g. find
 *              all overlapping boundaries.
 *
 * Exported functions: See ZMap/zmapFeature.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <glib.h>

#include <zmapFeature_P.h>


static gboolean isValidPart(ZMapFeaturePart part) ;
static gint sortParts(gconstpointer a, gconstpointer b) ;
static void freePartsCB(gpointer data, gpointer user_data) ;

static void freeSubPart(gpointer data, gpointer user_data_unused) ;



/* 
 *                       External routines.
 */




/* A GCompareFunc() to return which of the two features pointed to by a and b
 * should come first. */
gint zMapFeatureSortFeatures(gconstpointer a, gconstpointer b)
{
  gint result = 0 ;
  ZMapFeature feature_a = (ZMapFeature)a ;
  ZMapFeature feature_b = (ZMapFeature)b ;

  if (feature_a->x1 < feature_b->x1)
    result = -1 ;
  else if (feature_a->x1 > feature_b->x1)
    result = 1 ;
  else
    {
      if (feature_a->x2 < feature_b->x2)
        result = -1 ;
      else if (feature_a->x2 > feature_b->x2)
        result = 1 ;
      else
        result = 0 ;
    }

  return result ;
}





/* Checks to see if any boundary within feature coincides with any
 * boundary in boundaries.
 * 
 * Returns TRUE if there are matching boundaries, FALSE otherwise, in addition
 * will return a list of ZMapFeatureSubParts of the boundaries, those that
 * do not match have the value zero.
 * 
 * Note that the actual boundary position may be different from the original
 * because the feature's style might allow "slop" in the precise position.
 * 
 * If exact_match is TRUE then the function will return FALSE if _any_ boundary
 * in the feature does not match.
 * 
 * Note that currently only basic, alignment and transcript type features
 * are searched for boundaries.
 * 
 *  */
gboolean zMapFeatureHasMatchingBoundaries(ZMapFeature feature,
                                          ZMapFeatureSubPartType part_type,
                                          gboolean exact_match, gboolean cds_only, int slop,
                                          GList *boundaries, int boundary_start, int boundary_end,
                                          ZMapFeaturePartsList *matching_boundaries_out,
                                          ZMapFeaturePartsList *non_matching_boundaries_out)
{
  gboolean result = FALSE ;

  /* tmp hack while I arrange code to use this struct. */
  ZMapFeaturePartsListStruct subparts ;

  zMapReturnValIfFail(zMapFeatureIsValid((ZMapFeatureAny)feature), FALSE) ;

  subparts.min = boundary_start ;
  subparts.max = boundary_end ;
  subparts.parts = boundaries ;

  if (boundaries)
    {
      switch (feature->mode)
        {
        case ZMAPSTYLE_MODE_BASIC:
          {
            result = zmapFeatureBasicMatchingBoundaries(feature,
                                                        part_type, exact_match, slop,
                                                        &subparts,
                                                        matching_boundaries_out,
                                                        non_matching_boundaries_out) ;

            break ;
          }
        case ZMAPSTYLE_MODE_ALIGNMENT:
          {
            result = zmapFeatureAlignmentMatchingBoundaries(feature,
                                                            part_type, exact_match, slop,
                                                            &subparts,
                                                            matching_boundaries_out,
                                                            non_matching_boundaries_out) ;

            break ;
          }
        case ZMAPSTYLE_MODE_TRANSCRIPT:
          {
            result = zmapFeatureTranscriptMatchingBoundaries(feature,
                                                             part_type, exact_match, slop,
                                                             &subparts,
                                                             matching_boundaries_out,
                                                             non_matching_boundaries_out) ;

            break ;
          }
        default:
          {
            break ;
          }
        }
    }

  return result ;
}




/* Can be used to get non-overlapping or overlapping features of various types.
 * 
 * NOTE that this function filters the list IN PLACE, you get a pointer back to
 * your original list that may be shorter or even NULL.
 *  */
GList *zMapFeatureGetOverlapFeatures(GList *feature_list, int start, int end, ZMapFeatureRangeOverlapType overlap)
{
  GList *new_list = feature_list ;
  GList *curr ;

  curr = new_list ;
  while (curr)
    {
      GList *next ;
      ZMapFeature feature ;
      ZMapFeatureRangeOverlapType feature_overlap = ZMAPFEATURE_OVERLAP_NONE ;

      next = g_list_next(curr) ;
      feature = (ZMapFeature)(curr->data) ;

      if (feature->x1 >= start && feature->x2 <= end)
        feature_overlap = ZMAPFEATURE_OVERLAP_COMPLETE ;
      else if (feature->x1 < start && feature->x2 > end)
        feature_overlap = ZMAPFEATURE_OVERLAP_PARTIAL_EXTERNAL ;
      else if ((feature->x1 >= start && feature->x1 <= end)
               || (feature->x2 >= start && feature->x2 <= end))
        feature_overlap = ZMAPFEATURE_OVERLAP_PARTIAL_INTERNAL ;
      else
        feature_overlap = ZMAPFEATURE_OVERLAP_NONE ;
    
      if (!((overlap == ZMAPFEATURE_OVERLAP_ALL && feature_overlap != ZMAPFEATURE_OVERLAP_NONE)
            || (overlap == feature_overlap)))
        new_list = g_list_delete_link(new_list, curr) ;

      curr = next ;
    }

  return new_list ;
}


/* 
 * Functions for manipulating lists of parts (feature subparts, matches etc.).
 */


ZMapFeaturePartsList zMapFeaturePartsListCreate(ZMapFeatureFreePartFunc free_part_func)
{
  ZMapFeaturePartsList parts_list = NULL ;

  parts_list = g_new0(ZMapFeaturePartsListStruct, 1) ;

  parts_list->min = INT_MAX ;
  parts_list->max = 0 ;
  parts_list->parts = NULL ;
  parts_list->free_part_func = free_part_func ;

  return parts_list ;
}


gboolean zMapFeaturePartsListAdd(ZMapFeaturePartsList parts, ZMapFeaturePart part)
{
  gboolean result = FALSE ;

  if (isValidPart(part))
    {
      if (part->start < parts->min)
        parts->min = part->start ;

      if (part->end > parts->max)
        parts->max = part->end ;

      parts->parts = g_list_insert_sorted(parts->parts, part, sortParts) ;

      result = TRUE ;
    }

  return result ;
}


void zMapFeaturePartsListDestroy(ZMapFeaturePartsList parts)
{
  zMapReturnIfFail(parts) ;

  if (parts->parts)
    g_list_foreach(parts->parts, freePartsCB, parts) ;

  g_free(parts) ;

  return ;
}









/* 
 * Functions for manipulating subparts (e.g. exon, intron, gap, extent.
 */

ZMapFeatureSubPart zMapFeatureSubPartCreate(ZMapFeatureSubPartType subpart_type,
                                            int index, int start, int end)
{
  ZMapFeatureSubPart subpart ;

  subpart = g_new0(ZMapFeatureSubPartStruct, 1) ;
  subpart->subpart = subpart_type ;
  subpart->index = index ;
  subpart->start = start ;
  subpart->end = end ;

  return subpart ;
}

void zMapFeatureSubPartListFree(GList *sub_parts)
{
  g_list_foreach(sub_parts, freeSubPart, NULL) ;

  return ;
}


ZMapFeaturePartsList zMapFeatureSubPartsGet(ZMapFeature feature, ZMapFeatureSubPartType requested_bounds)
{
  ZMapFeaturePartsList subparts = NULL ;

  switch (feature->mode)
    {
    case ZMAPSTYLE_MODE_BASIC:
      {
        subparts = zmapFeatureBasicSubPartsGet(feature, requested_bounds) ;

       break ;
      }
    case ZMAPSTYLE_MODE_ALIGNMENT:
      {
        subparts = zmapFeatureAlignmentSubPartsGet(feature, requested_bounds) ;

        break ;
      }
    case ZMAPSTYLE_MODE_TRANSCRIPT:
      {
        subparts = zmapFeatureTranscriptSubPartsGet(feature, requested_bounds) ;

        break ;
      }
    default:
      {
        subparts = zmapFeatureBasicSubPartsGet(feature, requested_bounds) ;

        break ;
      }
    }

  return subparts ;
}


void zMapFeatureSubPartDestroy(ZMapFeatureSubPart subpart)
{
  g_free(subpart) ;

  return ;
}






ZMapFeatureBoundaryMatch zMapFeatureBoundaryMatchCreate(ZMapFeatureSubPartType subpart_type,
                                                        int index, int start, int end,
                                                        ZMapFeatureBoundaryMatchType match_type)
{
  ZMapFeatureBoundaryMatch match ;

  match = g_new0(ZMapFeatureBoundaryMatchStruct, 1) ;
  match->subpart = subpart_type ;
  match->index = index ;
  match->start = start ;
  match->end = end ;
  match->match_type = match_type ;

  return match ;
}

void zMapFeatureBoundaryMatchDestroy(ZMapFeatureBoundaryMatch match)
{
  g_free(match) ;

  return ;
}












/* 
 *                    Package routines
 */


/* Checks to see if boundary_start/boundary_end match start/end within +- slop.
 * If full_match == TRUE then _both_ coords must match, otherwise returns
 * TRUE if just one coord matches.
 * 
 * Can be used to test just one value by only specifying boundary_start/start
 * or boundary_end/end.
 * 
 * Boundaries and start/end must be > 0.
 * 
 */
gboolean zmapFeatureCoordsMatch(int slop, gboolean full_match,
                                int boundary_start, int boundary_end, int start, int end,
                                int *start_match_out, int *end_match_out)
{
  gboolean result = FALSE ;
  gboolean start_match = FALSE, end_match = FALSE ;


  zMapReturnValIfFail(((boundary_start && start) || (boundary_end && end)), FALSE) ;

  if (boundary_start > 0 && start > 0)
    {
      if ((abs(start - boundary_start) <= slop))
        {
          start_match = TRUE ;
          *start_match_out = start ;
        }
    }

  if (boundary_end > 0 && end > 0)
    {
      if ((abs(end - boundary_end) <= slop))
        {
          end_match = TRUE ;
          *end_match_out = end ;
        }
    }

  if (full_match)
    {
      if (start_match && end_match)
        result = TRUE ;
    }
  else
    {
      if (start_match || end_match)
        result = TRUE ;
    }

  return result ;
}



/* Given a ZMapFeatureSubParts of boundaries returns a ZMapFeatureSubParts
 * containing the features start or end if either matches any of the boundaries
 * or returns NULL.
 * 
 * Notes:
 *  - if exact_match is TRUE then returns NULL if either the features start or end does not match.
 * 
 */
gboolean zmapFeatureMatchingBoundaries(ZMapFeature feature,
                                       gboolean exact_match, int slop,
                                       ZMapFeaturePartsList boundaries,
                                       ZMapFeaturePartsList *matching_boundaries_out,
                                       ZMapFeaturePartsList *non_matching_boundaries_out)
{
  gboolean result = FALSE ;
  ZMapFeatureBoundaryMatchType match_type = ZMAPBOUNDARY_MATCH_TYPE_NONE ;
  int feature_start = 0, feature_end = 0 ;
  ZMapFeaturePartsList matches = NULL ;

  zMapReturnValIfFail(zMapFeatureIsValid((ZMapFeatureAny)feature), FALSE) ;

  if (feature->x2 < boundaries->min || feature->x1 > boundaries->max)
    {
      /* feature outside of boundaries. */
      feature_start = feature->x1 ;
      feature_end = feature->x2 ;

      result = FALSE ;
    }
  else
    {
      GList *curr ;

      result = TRUE ;

      curr = boundaries->parts ;
      while (curr)
        {
          ZMapFeatureSubPart curr_boundary = (ZMapFeatureSubPart)(curr->data) ;
          int match_boundary_start = 0, match_boundary_end = 0 ;

          if ((curr_boundary->end) && feature->x1 > curr_boundary->end)
            {
              /* Have not reached feature yet so continue.... */
              curr = g_list_next(curr) ;
              continue ;
            }
          else if ((curr_boundary->start) && feature->x2 < curr_boundary->start)
            {
              /* Gone past feature so stop.... */
              break ;
            }
          else
            {
              /* ok, work to do. */
              if (zmapFeatureCoordsMatch(slop, FALSE, curr_boundary->start, curr_boundary->end,
                                         feature->x1, feature->x2,
                                         &match_boundary_start, &match_boundary_end))
                {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                  if (match_boundary_start)
                    feature_start = feature->x1 ;
                  if (match_boundary_end)
                    feature_end = feature->x2 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
                  if (match_boundary_start)
                    match_type |= ZMAPBOUNDARY_MATCH_TYPE_5_MATCH ;

                  if (match_boundary_end)
                    match_type |= ZMAPBOUNDARY_MATCH_TYPE_3_MATCH ;

                  feature_start = feature->x1 ;
                  feature_end = feature->x2 ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                  /* CAN'T REMEMBER WHY I'VE COMMENTED THIS OUT ????? */

                  /* Got both so no point in continuing. */
                  if (feature_start && feature_end)
                    break ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
                }
              else
                {
                  /* temp code...... */

                  feature_start = feature->x1 ;
                  feature_end = feature->x2 ;

                }
            }

          curr = g_list_next(curr) ;
        }


      if (!((exact_match
             && (match_type & ZMAPBOUNDARY_MATCH_TYPE_5_MATCH) && (match_type & ZMAPBOUNDARY_MATCH_TYPE_3_MATCH))
            || ((match_type & ZMAPBOUNDARY_MATCH_TYPE_5_MATCH) || (match_type & ZMAPBOUNDARY_MATCH_TYPE_3_MATCH))))
        {
          result = FALSE ;
        }



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      {
        ZMapFeatureBoundaryMatch match_boundary ;

        matches = zMapFeaturePartsListCreate((ZMapFeatureFreePartFunc)zMapFeatureBoundaryMatchDestroy) ;

        match_boundary = zMapFeatureBoundaryMatchCreate(ZMAPFEATURE_SUBPART_FEATURE, 0,
                                                        feature_start, feature_end, match_type) ;

        if (zMapFeaturePartsListAdd(matches, (ZMapFeaturePart)match_boundary))
          {
            if (result)
              *matching_boundaries_out = matches ;
            else
              *non_matching_boundaries_out = matches ;
          }
        else
          {
            zMapFeaturePartsListDestroy(matches) ;
            result = FALSE ;
          }
      }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    }





      {
        ZMapFeatureBoundaryMatch match_boundary ;

        matches = zMapFeaturePartsListCreate((ZMapFeatureFreePartFunc)zMapFeatureBoundaryMatchDestroy) ;

        match_boundary = zMapFeatureBoundaryMatchCreate(ZMAPFEATURE_SUBPART_FEATURE, 0,
                                                        feature_start, feature_end, match_type) ;

        if (zMapFeaturePartsListAdd(matches, (ZMapFeaturePart)match_boundary))
          {
            if (result)
              *matching_boundaries_out = matches ;
            else
              *non_matching_boundaries_out = matches ;
          }
        else
          {
            zMapFeaturePartsListDestroy(matches) ;
            result = FALSE ;
          }
      }


  return result ;
}





/* 
 *                    Internal routines
 */



/* A GFunc() called to free the g_new'd data for a list element, in this case it's a
 * ZMapFeatureSubPartStruct but we don't really need to know that. */
static void freeSubPart(gpointer data, gpointer user_data_unused)
{
  zMapFeatureSubPartDestroy(data) ;

  return ;
}



static gboolean isValidPart(ZMapFeaturePart part)
{
  gboolean result = FALSE ;

  if ((part->start >= 0 && part->end >= 0)
      && (((part->start && part->end) && (part->start <= part->end))
          || (part->start || part->end)))
    result = TRUE ;

  return result ;
}



/* A GCompareFunc() to return which of the two subparts pointed to by a and b
 * should come first. */
static gint sortParts(gconstpointer a, gconstpointer b)
{
  gint result = 0 ;
  ZMapFeaturePart part_a = (ZMapFeaturePart)a ;
  ZMapFeaturePart part_b = (ZMapFeaturePart)b ;

  if (part_a->start < part_b->start)
    result = -1 ;
  else if (part_a->start > part_b->start)
    result = 1 ;
  else
    {
      if (part_a->end < part_b->end)
        result = -1 ;
      else if (part_a->end > part_b->end)
        result = 1 ;
      else
        result = 0 ;
    }

  return result ;
}



static void freePartsCB(gpointer data, gpointer user_data)
{
  ZMapFeaturePartsList parts = (ZMapFeaturePartsList)user_data ;

  parts->free_part_func(data) ;

  return ;
}
