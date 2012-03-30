/*  File: zmapNavigator_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Private header for the navigator code which displays
 *              positional information for sequences.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_NAVIGATOR_P_H
#define ZMAP_NAVIGATOR_P_H

#include <ZMap/zmapFeature.h>
#include <ZMap/zmapNavigator.h>
#include <ZMap/zmapWindowNavigator.h>

/* Data associated with the navigator. */
typedef struct _ZMapNavStruct
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

} ZMapNavStruct ;



#endif /* !ZMAP_NAVIGATOR_P_H */

