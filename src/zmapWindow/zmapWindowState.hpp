/*  File: zmapWindowState.h
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
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_STATE_H
#define ZMAP_WINDOW_STATE_H

#include <zmapWindow_P.hpp>


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
