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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 *
 * Description: Defines interface to code that creates/handles a
 *              window displaying genome data.
 *              
 * HISTORY:
 * Last edited: Jul  2 14:32 2004 (rnc)
 * Created: Thu Jul 24 15:21:56 2003 (edgrif)
 * CVS info:   $Id: zmapWindow.h,v 1.7 2004-07-02 13:33:12 rnc Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_H
#define ZMAP_WINDOW_H

#include <glib.h>
#include <libfoocanvas/libfoocanvas.h>

#include <ZMap/zmapSys.h>		       /* For callback funcs... */
#include <ZMap/zmapFeature.h>



/* Opaque type, represents an individual ZMap window. */
typedef struct _ZMapWindowStruct *ZMapWindow ;
typedef struct _ZMapPaneStruct *ZMapPane;

//typedef struct {
//  ZMapWindow window;             /* the window pane  */
//  Calc_cb    calc_cb;            /* callback routine */
//  void      *seqRegion;          /* AceDB region     */
//} ZMapCallbackData;



/******************* end of public stuff that might end up private */


/* Window stuff, callbacks, will need changing.... */
typedef enum {ZMAP_WINDOW_INIT, ZMAP_WINDOW_LOAD,
	      ZMAP_WINDOW_STOP, ZMAP_WINDOW_QUIT} ZmapWindowCmd ;

ZMapWindow   zMapWindowCreate          (GtkWidget *parent_widget, char *sequence,
					zmapVoidIntCallbackFunc app_routine, void *app_data) ;
void         zMapWindowDisplayData     (ZMapWindow window, void *data) ;
void         zMapWindowReset           (ZMapWindow window) ;
void         zMapWindowDestroy         (ZMapWindow window) ;

ZMapWindow   zMapWindowCreateZMapWindow(void);

void         zMapWindowSetHandle       (ZMapWindow window);
void         zMapWindowCreateRegion    (ZMapWindow window);
GNode       *zMapWindowGetPanesTree    (ZMapWindow window);
void         zMapWindowSetPanesTree    (ZMapWindow window, GNode *node);
void         zMapWindowSetFirstTime    (ZMapWindow window, gboolean value);
ZMapPane     zMapWindowGetFocuspane    (ZMapWindow window);
void         zMapWindowSetFocuspane    (ZMapWindow window, ZMapPane pane);

/* I don't know whether we're even going to use these datatypes, so for
** expediency I'm commenting these out until it becomes clearer. */

//ScreenCoord  zMapWindowGetScaleOffset  (ZMapWindow window);
//void         zMapWindowSetScaleOffset  (ZMapWindow window, ScreenCoord offset);
//Coord        zMapWindowGetCoord        (ZMapWindow window, char *field);
//void         zMapWindowSetCoord        (ZMapWindow window, char *field, int size);
//ScreenCoord  zMapWindowGetScreenCoord  (ZMapWindow window, Coord coord, int height);
//ScreenCoord  zMapWindowGetScreenCoord1 (ZMapWindow window, int height);
//ScreenCoord  zMapWindowGetScreenCoord2 (ZMapWindow window, int height);
//InvarCoord   zMapWindowGetOrigin       (ZMapWindow window);

int          zMapWindowGetRegionSize   (ZMapWindow window);

GtkWidget   *zMapWindowGetFrame        (ZMapWindow window);
void         zMapWindowSetFrame        (ZMapWindow window, GtkWidget *frame);
GtkWidget   *zMapWindowGetVbox         (ZMapWindow window);
void         zMapWindowSetVbox         (ZMapWindow window, GtkWidget *vbox);
void         zMapWindowSetBorderWidth  (GtkWidget *container, int width);
GtkWidget   *zMapWindowGetHbox         (ZMapWindow window);
void         zMapWindowSetHbox         (ZMapWindow window, GtkWidget *hbox);
GtkWidget   *zMapWindowGetHpane        (ZMapWindow window);
void         zMapWindowSetHpane        (ZMapWindow window, GtkWidget *hpane);
GtkWidget   *zMapWindowGetNavigator    (ZMapWindow window);
void         zMapWindowSetNavigator    (ZMapWindow window, GtkWidget *navigator);
FooCanvas   *zMapWindowGetNavCanvas    (ZMapWindow window);
void         zMapWindowSetNavCanvas    (ZMapWindow window, FooCanvas *navcanvas);
GtkWidget   *zMapWindowGetDisplayVbox  (ZMapWindow window);
void         zMapWindowSetDisplayVbox  (ZMapWindow window, GtkWidget *vbox);

float        zmMainScale               (FooCanvas *canvas, float offset, int start, int end);

GPtrArray   *zMapRegionNewMethods      (ZMapRegion *region);
GPtrArray   *zMapRegionGetMethods      (ZMapRegion *region);
GPtrArray   *zMapRegionGetOldMethods   (ZMapRegion *region);
void         zMapRegionFreeMethods     (ZMapRegion *region);
void         zMapRegionFreeOldMethods  (ZMapRegion *region);
GArray      *zMapRegionNewSegs         (ZMapRegion *region);
GArray      *zMapRegionGetSegs         (ZMapRegion *region);
void         zMapRegionFreeSegs        (ZMapRegion *region);
GArray      *zMapRegionGetDNA          (ZMapRegion *region);
void         zMapRegionFreeDNA         (ZMapRegion *region);

int          recordFocus               (GtkWidget *widget, GdkEvent *event, gpointer data); 
void         navUpdate                 (GtkAdjustment *adj, gpointer p);
void         navChange                 (GtkAdjustment *adj, gpointer p);

void         drawNavigatorWind         (ZMapPane pane);

void         zMapWindowDrawFeatures    (ZMapWindow window, ZMapFeatureContext feature_context);


/* TEST SCAFFOLDING............... */
ZMapFeatureContext testGetGFF(void) ;



#endif /* !ZMAP_WINDOW_H */
