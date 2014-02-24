/*  File: zmapWindowCanvasLocus_I.h
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2014: Genome Research Ltd.
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
 * implements callback functions for FeaturesetItem locus features
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CANVAS_LOCUS_I_H
#define ZMAP_CANVAS_LOCUS_I_H

#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasLocus.h>


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
