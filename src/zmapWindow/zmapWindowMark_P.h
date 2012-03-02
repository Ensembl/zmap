/*  File: zmapWindowMark_P.h
 *  Author: Roy Storey (roy@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Internal header for the window mark package..
 *
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_MARK_P_H
#define ZMAP_WINDOW_MARK_P_H

#include <ZMap/zmapWindow.h>
#include <zmapWindowContainerGroup.h>

/* ZMapWindowMark */
typedef struct _ZMapWindowMarkStruct *ZMapWindowMark ;


/* How to Create a Mark */
ZMapWindowMark zmapWindowMarkCreate(ZMapWindow window) ;

/* Is the mark set */
gboolean zmapWindowMarkIsSet(ZMapWindowMark mark) ;
/* Reset it */
void zmapWindowMarkReset(ZMapWindowMark mark) ;

/* Get/Set Colour */
void      zmapWindowMarkSetColour(ZMapWindowMark mark, char *colour) ;
GdkColor *zmapWindowMarkGetColour(ZMapWindowMark mark) ;
/* Get/Set Stipple */
void       zmapWindowMarkSetStipple(ZMapWindowMark mark, GdkBitmap *stipple);
GdkBitmap *zmapWindowMarkGetStipple(ZMapWindowMark mark);

/* Get/Set the item to use as src for mark coords */
gboolean zmapWindowMarkSetItem(ZMapWindowMark mark, FooCanvasItem *item) ;
FooCanvasItem *zmapWindowMarkGetItem(ZMapWindowMark mark) ;

/* Get/Set the mark coords (_world_) */
gboolean zmapWindowMarkSetWorldRange(ZMapWindowMark mark,
				     double world_x1, double world_y1, double world_x2, double world_y2) ;
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


#endif	/* ZMAP_WINDOW_MARK_P_H */
