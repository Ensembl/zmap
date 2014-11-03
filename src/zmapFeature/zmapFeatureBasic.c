/*  File: zmapFeatureBasic.c
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

#include <ZMap/zmap.h>

#include <glib.h>

#include <ZMap/zmapFeature.h>
#include <zmapFeature_P.h>
#include <string.h>


#define VARIATION_SEPARATOR_CHAR '/'


/* 
 *                 External routines.
 */







/* 
 *                 Package routines.
 */


/* Do boundary_start/boundary_end match the start/end of the basic feature ? */
gboolean zmapFeatureBasicHasMatchingBoundary(ZMapFeature feature,
                                             int boundary_start, int boundary_end,
                                             int *boundary_start_out, int *boundary_end_out)
{
  gboolean result = FALSE ;
  int slop ;

  zMapReturnValIfFail(zMapFeatureIsValid((ZMapFeatureAny)feature), FALSE) ;

  slop = zMapStyleSpliceHighlightTolerance(*(feature->style)) ;

  result = zmapFeatureCoordsMatch(slop, boundary_start, boundary_end,
                                  feature->x1, feature->x2,
                                  boundary_start_out, boundary_end_out) ;

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



//static gboolean variationIsMulti(const char *variation_str)
//{
//  gboolean result = FALSE ;
//  zMapReturnValIfFail(variation_str && *variation_str != 0, result) ;
//
//  /* If the string contains more than one separator, it is a multiple variation */
//  char *cp = strchr(variation_str, VARIATION_SEPARATOR_CHAR) ;
//
//  if (cp)
//    {
//
//      cp = strchr(cp + 1, VARIATION_SEPARATOR_CHAR) ;
//
//      if (cp)
//        result = TRUE ;
//    }
//
//  return result ;
//}


