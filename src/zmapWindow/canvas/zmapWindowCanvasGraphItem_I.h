/*  File: zmapWindowGraphItem_I.h
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 * Description: Private header for graph "objects", i.e. graph
 *              columns in zmap.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_GRAPH_DENSITY_ITEM_I_H
#define ZMAP_WINDOW_GRAPH_DENSITY_ITEM_I_H

#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasGraphItem.h>



/*
 * minimal data struct to define a graph segment
 * handle boxes as y1,y2 + width
 * handle lines as a series of points y2 + width
 * Is the same as zmapWindowCanvasFeatureStruct as defined in zmapWindowCanvasFeatureset_I.h
 */

/*
 * The original graph density code had extra stuff to deal with re-binned data
 * this has been moved into CanvasFeatureset and can be re-used for other type of pre display processing if necessary
 */



/* ACTUALLY WE COULD FIND THIS OUT DYNAMICALLY FROM THE SCREEN SIZE...NOT HARD....AND MALCOLM'S 
 * COMMENT WILL SOON BE UNTRUE WITH THE ADVENT OF SCREENS LIKE THE MAC "RETINA" ETC ETC. */
/* this could be dynamic based on screen size because actually Malcolm some screens are this size.... */
#define N_POINTS	2000	/* will never run out as we only display one screen's worth */


/* Private struct representing a graph column, this is held in featureset->opt */
typedef struct ZMapWindowCanvasGraphStructType
{
  /* Cache of points either to be drawn or being drawn.
   * n.b. the +4 is +2 for gaps between bins, inserting a line, plus allow for end of data trailing lines */
  GdkPoint points[N_POINTS+4] ;				    /* The points to be drawn this time. */
  int n_points ;					    /* Number of points to be drawn this time. */

  /* N.B. feature point, gy, is drawn midway between start and end of feature. */
  double last_gx ;					    /* Last drawn feature point, used to
                                                               join up next graph. */
  double last_gy ;					    /* Last drawn feature point, used to join up next graph. */
  double last_width ;                                       /* Width of last drawn feature. */

} ZMapWindowCanvasGraphStruct, *ZMapWindowCanvasGraph ;




#endif /* ZMAP_WINDOW_GRAPH_DENSITY_ITEM_I_H */
