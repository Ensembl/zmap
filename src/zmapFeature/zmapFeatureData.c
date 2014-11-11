/*  File: zmapFeatureData.c
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



static void freeSubPart(gpointer data, gpointer user_data_unused) ;



/* 
 *                       External routines.
 */


/* Checks to see if any boundary within feature coincides with any
 * boundary in boundaries. Returns a list of ZMapSpanStruct of those that
 * do or NULL if there are none.
 * Note that the actual boundary position may be different from the original
 * because the feature's style might allow "slop" in the precise position. 
 * 
 * Note that currently only basic, alignment and transcript type features
 * are searched for boundaries.
 * 
 *  */
GList *zMapFeatureHasMatchingBoundaries(ZMapFeature feature, GList *boundaries)
{
  GList *matching_boundaries = NULL ;

  zMapReturnValIfFail(zMapFeatureIsValid((ZMapFeatureAny)feature), NULL) ;

  if (boundaries)
    {
      switch (feature->mode)
        {
        case ZMAPSTYLE_MODE_BASIC:
          {
            matching_boundaries = zmapFeatureBasicHasMatchingBoundaries(feature, boundaries) ;

            break ;
          }
        case ZMAPSTYLE_MODE_ALIGNMENT:
          {
            matching_boundaries = zmapFeatureAlignmentHasMatchingBoundaries(feature, boundaries) ;

            break ;
          }
        case ZMAPSTYLE_MODE_TRANSCRIPT:
          {
            matching_boundaries = zmapFeatureTranscriptHasMatchingBoundaries(feature, boundaries) ;

            break ;
          }
        default:
          {
            matching_boundaries = NULL ;

            break ;
          }
        }
    }

  return matching_boundaries ;
}




/* If the feature is basic, align or transcript its start/end and any
 * subparts are returned and the function returns TRUE, otherwise 
 * returns FALSE.
 * 
 * Note that subparts are a list of newly allocated ZMapSpanStruct
 * which should be freed by the caller, the function zMapFeatureFreeSubParts()
 * can be used to do this.
 * 
 *  */
gboolean zMapFeatureGetBoundaries(ZMapFeature feature, int *start_out, int *end_out, GList **subparts_out)
{
  gboolean result = FALSE ;
  int start = 0, end = 0 ;
  GList *boundaries = NULL ;

  zMapReturnValIfFail(zMapFeatureIsValid((ZMapFeatureAny)feature), FALSE) ;

  switch (feature->mode)
    {
    case ZMAPSTYLE_MODE_BASIC:
      {
        start = feature->x1 ;
        end = feature->x2 ;
        boundaries = NULL ;

        result = TRUE ;

        break ;
      }
    case ZMAPSTYLE_MODE_ALIGNMENT:
      {
        start = feature->x1 ;
        end = feature->x2 ;

        boundaries = zmapFeatureGetSubparts(feature) ;

        result = TRUE ;

        break ;
      }
    case ZMAPSTYLE_MODE_TRANSCRIPT:
      {
        start = feature->x1 ;
        end = feature->x2 ;

        boundaries = zmapFeatureGetSubparts(feature) ;

        result = TRUE ;

        break ;
      }
    default:
      {
        result = FALSE ;

        break ;
      }
    }


  if (result)
    {
      *start_out = start ;
      *end_out = end ;
      *subparts_out = boundaries ;
    }


  return result ;
}


void zMapFeatureFreeSubParts(GList *sub_parts)
{
  g_list_foreach(sub_parts, freeSubPart, NULL) ;

  return ;
}


/* Can be used to get non-overlapping or overlapping features of various types. */
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
 *                    Package routines
 */

/* Should be calls to feature specific code in feature specific code....doh.... */
GList *zmapFeatureGetSubparts(ZMapFeature feature)
{
  GList *subparts = NULL ;

  switch (feature->mode)
    {
    case ZMAPSTYLE_MODE_ALIGNMENT:
      {
        if (feature->feature.homol.align)
          {
            GArray *gaps_array = feature->feature.homol.align ;
            int i ;

            for (i = 0 ; i < gaps_array->len ; i++)
              {
                ZMapAlignBlock block = NULL ;
                ZMapSpan span ;

                block = &(g_array_index(gaps_array, ZMapAlignBlockStruct, i)) ;
                span = g_new0(ZMapSpanStruct, 1) ;
                span->x1 = block->t1 ;
                span->x2 = block->t2 ;

                subparts = g_list_append(subparts, span) ;
              }
          }

        break ;
      }
    case ZMAPSTYLE_MODE_TRANSCRIPT:
      {
        GArray *array = feature->feature.transcript.exons ;
        int i ;

        for (i = 0 ; i < array->len; ++i)
          {
            ZMapSpan exon, new_exon ;

            exon = &(g_array_index(array, ZMapSpanStruct, i)) ;
            new_exon = g_new0(ZMapSpanStruct, 1) ;
            *new_exon = *exon ;

            subparts = g_list_append(subparts, new_exon) ;
          }

        break ;
      }
    default:
      {
        break ;
      }
    }

  return subparts ;
}





/* 
 *                    Internal routines
 */


/* A GFunc() called to free the g_new'd data for a list element, in this case it's a
 * ZMapSpanStruct but we don't really need to know that. */
static void freeSubPart(gpointer data, gpointer user_data_unused)
{
  g_free(data) ;

  return ;
}

