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


/* 
 *                       External routines.
 */





/* Checks to see if any boundary within feature coincides with either
 * boundary_start or boundary_end, the actual coinciding boundary for
 * each is returned in boundary_start_out and boundary_end_out if specified.
 * The actual boundary position may be different because the feature's
 * style might allow "slop" in the precise boundary position. 
 * 
 * If at least one boundary is found TRUE is returned and if 
 * boundary_start_out and/or boundary_end_out are specified the positions
 * or zero are returned. If neither boundary is found then FALSE is
 * returned and boundary_start_out and/or boundary_end_out are unaltered.
 * 
 * Note that currently boundaries are only searched for in basic, alignment
 * and transcript type features.
 * 
 *  */
gboolean zMapFeatureHasMatchingBoundary(ZMapFeature feature,
                                        int boundary_start, int boundary_end,
                                        int *boundary_start_out, int *boundary_end_out)
{
  gboolean result = FALSE ;

  zMapReturnValIfFail(zMapFeatureIsValid((ZMapFeatureAny)feature), FALSE) ;

  switch (feature->mode)
    {
    case ZMAPSTYLE_MODE_BASIC:
      {
        result = zmapFeatureBasicHasMatchingBoundary(feature,
                                                     boundary_start, boundary_end,
                                                     boundary_start_out, boundary_end_out) ;

        break ;
      }
    case ZMAPSTYLE_MODE_ALIGNMENT:
      {
        result = zmapFeatureAlignmentHasMatchingBoundary(feature,
                                                         boundary_start, boundary_end,
                                                         boundary_start_out, boundary_end_out) ;

        break ;
      }
    case ZMAPSTYLE_MODE_TRANSCRIPT:
      {
        result = zmapFeatureTranscriptHasMatchingBoundary(feature,
                                                          boundary_start, boundary_end,
                                                          boundary_start_out, boundary_end_out) ;

        break ;
      }
    default:
      {
        result = FALSE ;

        break ;
      }
    }


  return result ;
}




/* Checks to see if any boundary within feature coincides with either
 * boundary_start or boundary_end, the actual coinciding boundary for
 * each is returned in boundary_start_out and boundary_end_out if specified.
 * The actual boundary position may be different because the feature's
 * style might allow "slop" in the precise boundary position. 
 * 
 * If at least one boundary is found TRUE is returned and if 
 * boundary_start_out and/or boundary_end_out are specified the positions
 * or zero are returned. If neither boundary is found then FALSE is
 * returned and boundary_start_out and/or boundary_end_out are unaltered.
 * 
 * Note that currently boundaries are only searched for in basic, alignment
 * and transcript type features.
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


