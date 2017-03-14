/*  File: zmapWindowCanvasLocus_I.h
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
 * implements callback functions for FeaturesetItem locus features
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CANVAS_LOCUS_I_H
#define ZMAP_CANVAS_LOCUS_I_H

#include <zmapWindowCanvasFeatureset_I.hpp>
#include <zmapWindowCanvasLocus.hpp>


/* we display loci like this:  (# is area of locus, not displayed)
   in case of no overlap, then no need to offset the text
   we get the featureset to de-overlap the names, replacing zmapWindowTextPositioner.c


       ENSTMUSG0001234567
      /
     /
    /
   #---XZYYY.2
   #
   #

   So we need:
	feature->y1,y2 as for a normal feature
	y coord for the LH end of the line
	y coord for the RH end of the line
	x-offset of the text (can vary if bumped)
*/


#define ZMAP_LOCUS_LINE_WIDTH	20



typedef struct
{
  double y1,y2;
  double height;
  int id;

} LocusGroupStruct, *LocusGroup;



typedef struct _zmapWindowCanvasLocusStruct
{
  zmapWindowCanvasFeatureStruct feature;	/* all the common stuff, has locus extent as feature->feature->x1,x2 */
  /* has canvas feature extent as feature->y1,y2 */

  double ylocus, ytext;		/* line coordinates, text appears around y2 */
  double x_off;			/* of the text = RH x coord of line */
  double x_wid;

  LocusGroup group;

} zmapWindowCanvasLocusStruct, *ZMapWindowCanvasLocus;


typedef struct _zmapWindowCanvasLocusSetStruct
{
  zmapWindowCanvasPangoStruct pango;
  /* allow for addition of other pango things eg diff font for diff locus types */

  double text_h;			/* height of text in world coords */
  GList *filter;			/* list of prefixes to filter by */

} zmapWindowCanvasLocusSetStruct, *ZMapWindowCanvasLocusSet;





#endif /* !ZMAP_CANVAS_LOCUS_I_H */
