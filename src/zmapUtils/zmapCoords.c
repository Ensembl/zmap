/*  File: zmapCoords.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Various coordinate transforming functions.
 *
 * Exported functions: See ZMap/zmapUtils.h
 *              
 * HISTORY:
 * Last edited: Nov 18 15:58 2009 (edgrif)
 * Created: Tue Nov 17 13:29:50 2009 (edgrif)
 * CVS info:   $Id: zmapCoords.c,v 1.3 2010-06-14 15:40:14 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






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




