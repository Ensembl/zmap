/*  File: zmapWindow.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 * and was written by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Defines interface to code that creates/handles a
 *              window displaying genome data.
 *              
 * HISTORY:
 * Last edited: Feb 10 15:58 2005 (edgrif)
 * Created: Thu Jul 24 15:21:56 2003 (edgrif)
 * CVS info:   $Id: zmapWindow.h,v 1.31 2005-02-10 16:34:27 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_H
#define ZMAP_WINDOW_H

#include <glib.h>
/* SHOULD CANVAS BE HERE...MAYBE, MAYBE NOT...... */
#include <libfoocanvas/libfoocanvas.h>

#include <ZMap/zmapSys.h>		       /* For callback funcs... */
#include <ZMap/zmapFeature.h>



/* Opaque type, represents an individual ZMap window. */
typedef struct _ZMapWindowStruct *ZMapWindow ;



/* indicates how far the zmap is zoomed, n.b. ZMAP_ZOOM_FIXED implies that the whole sequence
 * is displayed at the maximum zoom. */
typedef enum {ZMAP_ZOOM_INIT, ZMAP_ZOOM_MIN, ZMAP_ZOOM_MID, ZMAP_ZOOM_MAX,
	      ZMAP_ZOOM_FIXED} ZMapWindowZoomStatus ;


/* Data returned to the visibilityChange callback routine. */
typedef struct _ZMapWindowVisibilityChangeStruct
{
  ZMapWindowZoomStatus zoom_status ;

  /* Top/bottom coords for section of sequence that can be scrolled currently in window. */
  double scrollable_top ;
  double scrollable_bot ;

} ZMapWindowVisibilityChangeStruct, *ZMapWindowVisibilityChange ;


/* Callback functions that can be registered with ZMapWindow, functions are registered all in one.
 * go via the ZMapWindowCallbacksStruct. */
typedef void (*ZMapWindowCallbackFunc)(ZMapWindow window, void *caller_data, void *window_data) ;

typedef struct _ZMapWindowCallbacksStruct
{
  ZMapWindowCallbackFunc enter ;
  ZMapWindowCallbackFunc leave ;
  ZMapWindowCallbackFunc scroll ;
  ZMapWindowCallbackFunc click ;
  ZMapWindowCallbackFunc setZoomStatus;
  ZMapWindowCallbackFunc visibilityChange ;
  ZMapWindowCallbackFunc destroy ;
} ZMapWindowCallbacksStruct, *ZMapWindowCallbacks ;






#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* I AM UNCERTAIN ABOUT HOW MUCH THIS IS NEEDED....NEEDS LOOKING AT.... */
/* Window stuff, callbacks, will need changing.... */
typedef enum {ZMAP_WINDOW_INIT, ZMAP_WINDOW_LOAD,
	      ZMAP_WINDOW_STOP, ZMAP_WINDOW_QUIT} ZmapWindowCmd ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




void       zMapWindowInit       (ZMapWindowCallbacks callbacks) ;
ZMapWindow zMapWindowCreate     (GtkWidget *parent_widget, char *sequence, void *app_data) ;
ZMapWindow zMapWindowCopy(GtkWidget *parent_widget, char *sequence, 
			  void *app_data, ZMapWindow old, ZMapFeatureContext features, GData *types) ;
void zMapWindowDisplayData(ZMapWindow window,
			   ZMapFeatureContext current_features, ZMapFeatureContext new_features,
			   GData *types) ;
void zMapWindowZoom(ZMapWindow window, double zoom_factor) ;
void zMapWindowMove(ZMapWindow window, double start, double end) ;
void zMapWindowReset(ZMapWindow window) ;
GtkWidget *zMapWindowGetWidget(ZMapWindow window);
ZMapWindowZoomStatus zMapWindowGetZoomStatus(ZMapWindow window) ;
void zMapWindowSetZoomStatus(ZMapWindow window) ;
double zMapWindowGetZoomFactor(ZMapWindow window);
void zMapWindowSetZoomFactor(ZMapWindow window, double zoom_factor);
void zMapWindowSetMinZoom   (ZMapWindow window);

void zMapWindowGetVisible(ZMapWindow window, double *top_out, double *bottom_out) ;


void zMapWindowDestroy(ZMapWindow window) ;



gboolean     zMapWindowScrollToItem    (ZMapWindow window, gchar *type, GQuark feature_id);
void         zMapWindowDestroyLists    (ZMapWindow window);
GQuark       zMapWindowGetFocusQuark   (ZMapWindow window);
gchar       *zMapWindowGetTypeName     (ZMapWindow window);



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* TEST SCAFFOLDING............... */
ZMapFeatureContext testGetGFF(void) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#endif /* !ZMAP_WINDOW_H */
