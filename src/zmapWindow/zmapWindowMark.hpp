/*  File: zmapWindowMark.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * Description: Internal header for the window mark package..
 *
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_MARK_H
#define ZMAP_WINDOW_MARK_H

#include <ZMap/zmapWindow.hpp>
#include <zmapWindowContainerGroup.hpp>




/* ZMapWindowMark */
typedef struct _ZMapWindowMarkStruct *ZMapWindowMark ;


/* How to Create a Mark */
ZMapWindowMark zmapWindowMarkCreate(ZMapWindow window) ;

/* Is the mark set */
gboolean zmapWindowMarkIsSet(ZMapWindowMark mark) ;
/* Reset it */
void zmapWindowMarkReset(ZMapWindowMark mark) ;

/* Get/Set Colour */
void      zmapWindowMarkSetColour(ZMapWindowMark mark, const char *colour) ;
GdkColor *zmapWindowMarkGetColour(ZMapWindowMark mark) ;

/* Get/Set Stipple */
void       zmapWindowMarkSetStipple(ZMapWindowMark mark, GdkBitmap *stipple);
GdkBitmap *zmapWindowMarkGetStipple(ZMapWindowMark mark);

/* Set mark from item */
gboolean zmapWindowMarkSetItem(ZMapWindowMark mark, FooCanvasItem *item) ;

/* Set mark from coords (_world_) */
gboolean zmapWindowMarkSetWorldRange(ZMapWindowMark mark,
				     double world_x1, double world_y1, double world_x2, double world_y2) ;

/* Get mark position (_world_ coords). */
gboolean zmapWindowMarkGetWorldRange(ZMapWindowMark mark,
				     double *world_x1, double *world_y1,
				     double *world_x2, double *world_y2) ;

/* Get the sequence coords.  These are in the coord space of the block that is marked. */
gboolean zmapWindowMarkGetSequenceRange(ZMapWindowMark mark, int *start, int *end) ;

ZMapWindowContainerGroup zmapWindowMarkGetCurrentBlockContainer(ZMapWindowMark mark);

/* Once the block is marked it needs to call this function. */
gboolean zmapWindowMarkSetBlockContainer(ZMapWindowMark mark,
					 ZMapWindowContainerGroup container,
					 double sequence_start, double sequence_end,
					 double x1, double y1, double x2, double y2);


/* What to do when you've finished with one. */
void zmapWindowMarkDestroy(ZMapWindowMark mark) ;


#endif	/* ZMAP_WINDOW_MARK_H */
