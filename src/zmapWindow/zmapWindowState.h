/*  File: zmapWindowState.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_STATE_H
#define ZMAP_WINDOW_STATE_H

#include <zmapWindow_P.h>


/* create, copy, destroy */
ZMapWindowState zmapWindowStateCreate(void);
ZMapWindowState zmapWindowStateCopy(ZMapWindowState state);
ZMapWindowState zmapWindowStateDestroy(ZMapWindowState state);


/* set/save state information */
gboolean zmapWindowStateSaveMark(ZMapWindowState state, ZMapWindow window);
gboolean zmapWindowStateSavePosition(ZMapWindowState state, ZMapWindow window);
gboolean zmapWindowStateSaveZoom(ZMapWindowState state, double zoom_factor);
gboolean zmapWindowStateSaveFocusItems(ZMapWindowState state,
				       ZMapWindow      window);
gboolean zmapWindowStateSaveBumpedColumns(ZMapWindowState state,
					  ZMapWindow window);


/* get :( I think we should be able to do without it... */
gboolean zmapWindowStateGetScrollRegion(ZMapWindowState state, 
					double *x1, double *y1, 
					double *x2, double *y2);
/* restore everything */
void zmapWindowStateRestore(ZMapWindowState state, ZMapWindow window);


/* The Queue functions */
ZMapWindowStateQueue zmapWindowStateQueueCreate(void);
int zmapWindowStateQueueLength(ZMapWindowStateQueue queue);
gboolean zMapWindowHasHistory(ZMapWindow window);
gboolean zmapWindowStateGetPrevious(ZMapWindow window, ZMapWindowState *state_out, gboolean pop);
gboolean zmapWindowStateQueueStore(ZMapWindow window, ZMapWindowState state_in, gboolean clear_current);
/* Unfortunately gboolean pop to get previous wasn't enough. */
void zmapWindowStateQueueRemove(ZMapWindowStateQueue queue, ZMapWindowState state_del);
void zmapWindowStateQueueClear(ZMapWindowStateQueue queue);
gboolean zmapWindowStateQueueIsRestoring(ZMapWindowStateQueue queue);
ZMapWindowStateQueue zmapWindowStateQueueDestroy(ZMapWindowStateQueue queue);

#endif /* ZMAP_WINDOW_STATE_H */
