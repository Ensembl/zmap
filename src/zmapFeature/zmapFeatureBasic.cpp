/*  File: zmapFeatureBasic.c
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description: Implements functions that operate on "basic" features,
 *              i.e. features that have no subparts.
 *
 * Exported functions: See ZMap/zmapFeature.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <glib.h>

#include <ZMap/zmapFeature.hpp>
#include <zmapFeature_P.hpp>
#include <string.h>


#define VARIATION_SEPARATOR_CHAR '/'


/*
 *                 External routines.
 */







/*
 *                 Package routines.
 */


ZMapFeaturePartsList zmapFeatureBasicSubPartsGet(ZMapFeature feature, ZMapFeatureSubPartType requested_bounds)
{
  ZMapFeaturePartsList subparts = NULL ;

  if (requested_bounds == ZMAPFEATURE_SUBPART_FEATURE)
    {
      ZMapFeatureSubPart feature_span ;

      subparts = g_new0(ZMapFeaturePartsListStruct, 1) ;

      feature_span = zMapFeatureSubPartCreate(ZMAPFEATURE_SUBPART_FEATURE, 0, feature->x1, feature->x2) ;

      subparts->min_val = feature->x1 ;
      subparts->max_val = feature->x2 ;
      subparts->parts = g_list_append(subparts->parts, feature_span) ;
    }

  return subparts ;
}





/* Do any of the supplied boundaries match the start/end of the basic feature ? */
gboolean zmapFeatureBasicMatchingBoundaries(ZMapFeature feature,
                                            ZMapFeatureSubPartType part_type, gboolean exact_match, int slop,
                                            ZMapFeaturePartsList boundaries,
                                            ZMapFeaturePartsList *matching_boundaries_out,
                                            ZMapFeaturePartsList *non_matching_boundaries_out)
{
  gboolean result = FALSE ;

  if (part_type == ZMAPFEATURE_SUBPART_FEATURE)
    {
      result = zmapFeatureMatchingBoundaries(feature, exact_match, slop, boundaries,
                                             matching_boundaries_out, non_matching_boundaries_out) ;
    }

  return result ;
}



/* Get the sections before and after the separator in a variation string. Note that at the moment
 * this ignores anything after the second separator, if there is one. (This function could be
 * modified to return a list of new strings if we want to return all of the alternatives.)
 * Returns the diff between new_len and old len (which is negative if a deletion, positive for an
 * insertion or 0 for a replacement of the same number of chars). */
int zMapFeatureVariationGetSections(const char *variation_str,
                                    char **old_str_out, char **new_str_out,
                                    int *old_len_out, int *new_len_out)
{
  int diff = 0 ;
  zMapReturnValIfFail(variation_str && *variation_str != 0, diff) ;

  const char *separator_pos = strchr(variation_str, VARIATION_SEPARATOR_CHAR) ;
  const int len = strlen(variation_str) ;

  if (separator_pos)
    {
      /* The end of the replacement string is at another separator or the end of the string */
      const char *end = strchr(separator_pos + 1, VARIATION_SEPARATOR_CHAR) ;

      if (!end)
        end = variation_str + len ;

      int old_len = separator_pos - variation_str ;
      int new_len = end - separator_pos - 1 ;

      char *old_str = NULL ;
      char *new_str = NULL ;

      if (old_len > 0)
        {
          old_str = g_strndup(variation_str, old_len) ;

          if (strcmp(old_str, "-") == 0) /* insertion */
            old_len = 0 ;
        }

      if (new_len > 0)
        {
          new_str = g_strndup(separator_pos + 1, new_len) ;

          if (strcmp(new_str, "-") == 0) /* deletion */
            new_len = 0 ;
        }

      /* Assign output variables */
      diff = new_len - old_len ;

      if (old_len_out)
        *old_len_out = old_len ;

      if (new_len_out)
        *new_len_out = new_len ;

      if (old_str_out && old_len > 0)
        *old_str_out = old_str ;
      else
        g_free(old_str) ;

      if (new_str_out && new_len > 0)
        *new_str_out = new_str ;
      else
        g_free(new_str) ;
    }

  return diff ;
}



