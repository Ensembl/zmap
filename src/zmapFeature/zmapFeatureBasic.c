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




