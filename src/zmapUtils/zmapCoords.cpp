/*  File: zmapCoords.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: Various coordinate transforming functions.
 *
 * Exported functions: See ZMap/zmapUtils.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>


#include <glib.h>




/* Set of coords utilities for all the stuff that is so easy to get wrong... */


/* Clamp start_inout/end_inout to be within range_start/range_end.
 *
 * Returns FALSE if coords do not overlap range and leaves coords unaltered, otherwise
 * returns TRUE and if coords had to be clamped they are returned in start_inout/end_inout. */
gboolean zMapCoordsClamp(int range_start, int range_end, int *start_inout, int *end_inout)
{
  gboolean result = FALSE ;

  if (*start_inout > range_start && *end_inout < range_end)
    {
      /* complete overlap */
      result = TRUE ;
    }
  else if (*end_inout < range_start || *start_inout > range_end)
    {
      /* no_overlap */
      result = FALSE ;
    }
  else
    {
      result = TRUE ;

      if (*start_inout < range_start)
        *start_inout = range_start ;
      if (*end_inout > range_end)
        *end_inout = range_end ;
    }

  return result ;
}



/* Converts a start/end pair of coords into a zero based pair of coords,
 *
 * e.g.  range  3 -> 6  becomes  0 -> 3
 *
 */
void zmapCoordsZeroBased(int *start_inout, int *end_inout)
{
  *end_inout = *end_inout - *start_inout ;		    /* do this first before zeroing start ! */

  *start_inout = 0.0 ;

  return ;
}


/* Convert given coords to offsets from base. Offsets start at start_value which would
 * commonly be 0 or 1 or give zero or one-based coords.
 *
 * Coords are assumed to be >= base and start_value >= 0
 *  */
void zMapCoordsToOffset(int base, int start_value, int *start_inout, int *end_inout)
{
  if (start_inout)
    *start_inout = (*start_inout - base) + start_value ;

  if (end_inout)
    *end_inout = (*end_inout - base) + start_value ;

  return ;
}
