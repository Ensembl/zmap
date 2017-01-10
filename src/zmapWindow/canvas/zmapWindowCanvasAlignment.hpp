/*  File: zmapWindowCanvasAlignment.h
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
 * Description:
 *
 * implements callback functions for FeaturesetItem alignment features
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CANVAS_ALIGNMENT_H
#define ZMAP_CANVAS_ALIGNMENT_H

#include <zmapWindowAllBase.hpp>


/* Different parts of a gapped alignment. */
typedef enum
  {
    GAP_BOX,                                                /* Match box. */
    GAP_HLINE,                                              /* Drawing a match boundary where there is no gap. */
    GAP_VLINE,                                              /* Drawing gap line between
                                                               matches. */
    GAP_VLINE_INTRON,                                       /* Intron in the match. */
    GAP_VLINE_UNSEQUENCED,                                  // Unsequenced section in a match,
                                                            // e.g. gap between readpairs.
    GAP_VLINE_MISSING                                       // Missing sequence between two matches.


} GappedAlignFeaturesType ;


struct AlignGapStruct
{
  int y1, y2 ;                                              /* in pixel coords from feature y1 */

  GappedAlignFeaturesType type ;                            /* See GAP_XXXX above... */

  ColinearityType colinearity ;                             // colinear type if gap type is GAP_VLINE_INTRON

  gboolean edge ;                                           /* for squashed short reads: edge blocks are diff colour */

  AlignGapStruct *next ;
} ;

typedef AlignGapStruct *AlignGap ;


void zMapWindowCanvasAlignmentInit(void);
AlignGap zMapWindowCanvasAlignmentMakeGapped(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                                             int start, int end, GArray *align_gaps, gboolean is_forward) ;
void zMapWindowCanvasAlignmentFreeGapped(AlignGap ag) ;
ZMapFeatureSubPart zMapWindowCanvasGetGappedSubPart(GArray *aligns_array, ZMapStrand strand, double y) ;


#endif /* !ZMAP_CANVAS_ALIGNMENT_H */
