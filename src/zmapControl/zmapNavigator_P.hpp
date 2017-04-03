/*  File: zmapNavigator_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Private header for the navigator code which displays
 *              positional information for sequences.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_NAVIGATOR_P_H
#define ZMAP_NAVIGATOR_P_H

#include <ZMap/zmapFeature.hpp>


/* callback registered with zmapnavigator, gets called when user releases the scrollbar for
 * the window locator. */
typedef void (*ZMapNavigatorScrollValue)(void *user_data, double start, double end) ;

/* Data associated with the navigator. */
typedef struct ZMapNavigatorStructType
{
  ZMapSpanStruct parent_span ;		    /* Start/end of parent of sequence. */
  ZMapSpanStruct sequence_span ;		    /* where this sequence maps to parent. */

  /* The region locator showing the position/extent of this sequence region
   * within the total sequence. */
  GtkWidget *pane ;
  GtkWidget *navVBox ;
  GtkWidget *navVScroll ;
  GtkWidget *topLabel ;
  GtkWidget *botLabel ;

  /* The window locator showing the position/extent of the window within the region, this changes
   * as the user zooms and when they move the scrolling window via the special keys. */
  GtkWidget *wind_vbox ;
  GtkWidget *wind_scroll ;
  GtkWidget *wind_top_label ;
  GtkWidget *wind_bot_label ;
  double wind_top, wind_bot ;				    /* These seem to be unused ? */

  GtkWidget *locator_widget;

  int left_pane_width, right_pane_width;

  /* Caller can register a call back which we call when user releases button when moving window
   * locator. */
  ZMapNavigatorScrollValue cb_func ;
  void *user_data ;

} ZMapNavigatorStruct, *ZMapNavigator ;




ZMapNavigator zmapNavigatorCreate(GtkWidget **top_widg_out, GtkWidget **canvas_out) ;
void zmapNavigatorSetWindowCallback(ZMapNavigator navigator,
					   ZMapNavigatorScrollValue cb_func, void *user_data) ;
int zmapNavigatorGetMaxWidth(ZMapNavigator navigator);
int zmapNavigatorSetWindowPos(ZMapNavigator navigator, double top_pos, double bot_pos) ;
void zmapNavigatorSetView(ZMapNavigator navigator, ZMapFeatureContext features,
			  double top, double bottom) ;
void zmapNavigatorDestroy(ZMapNavigator navigator) ;



#endif /* !ZMAP_NAVIGATOR_P_H */

