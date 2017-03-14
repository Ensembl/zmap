/*  File: zmapWindowCanvasAlignment.h
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
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
