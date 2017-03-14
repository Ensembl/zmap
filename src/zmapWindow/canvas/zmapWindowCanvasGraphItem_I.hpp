/*  File: zmapWindowCanvasGraphItem_I.h
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
 * Description: Private header for graph "objects", i.e. graph
 *              columns in zmap.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_GRAPH_DENSITY_ITEM_I_H
#define ZMAP_WINDOW_GRAPH_DENSITY_ITEM_I_H

#include <zmapWindowCanvasFeatureset_I.hpp>
#include <zmapWindowCanvasGraphItem.hpp>



/*
 * minimal data struct to define a graph segment
 * handle boxes as y1,y2 + width
 * handle lines as a series of points y2 + width
 * Is the same as zmapWindowCanvasFeatureStruct as defined in zmapWindowCanvasFeatureset_I.h
 */


/* Private struct representing a graph column, this is held in featureset->opt */

/* ACTUALLY WE COULD FIND THIS OUT DYNAMICALLY FROM THE SCREEN SIZE...NOT HARD....AND MALCOLM'S
 * COMMENT WILL SOON BE UNTRUE WITH THE ADVENT OF SCREENS LIKE THE MAC "RETINA" ETC ETC. */
/* this could be dynamic based on screen size because actually Malcolm some screens are this size.... */
#define N_POINTS	2000	/* will never run out as we only display one screen's worth */

typedef struct ZMapWindowCanvasGraphStructType
{
  /* Cache our colours.... */
  GdkColor fill_col ;
  GdkColor draw_col ;
  GdkColor border_col ;


  /* Cache of points either to be drawn or being drawn.
   * n.b. the +4 is +2 for gaps between bins, inserting a line, plus allow for end of data trailing lines */
  GdkPoint points[N_POINTS + 4] ;                           /* The points to be drawn. */
  int n_points ;					    /* Number of points to be drawn. */

  /* N.B. feature point, gy, is drawn midway between start and end of feature. */
  double last_gx ;					    /* Last drawn feature point, used to
                                                               join up next graph. */
  double last_gy ;					    /* Last drawn feature point, used to join up next graph. */
  double last_width ;                                       /* Width of last drawn feature. */

} ZMapWindowCanvasGraphStruct, *ZMapWindowCanvasGraph ;




#endif /* ZMAP_WINDOW_GRAPH_DENSITY_ITEM_I_H */
