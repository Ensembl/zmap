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
 * Last edited: Jun 29 11:56 2004 (edgrif)
 * Created: Thu Jul 24 15:21:56 2003 (edgrif)
 * CVS info:   $Id: zmapWindow.h,v 1.5 2004-06-30 09:11:38 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_H
#define ZMAP_WINDOW_H

#include <glib.h>
#include <libfoocanvas/libfoocanvas.h>

#include <ZMap/zmapSys.h>		       /* For callback funcs... */
#include <ZMap/zmapFeature.h>


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <ZMap/zmapcommon.h>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

//#include <zmapcontrol.h>




/* Opaque type, represents an individual ZMap window. */
typedef struct _ZMapWindowStruct *ZMapWindow ;

/* Opaque type, represents an individual ZMap display pane */
typedef struct _ZMapPaneStruct *ZMapPane;


// zmapcolumn * a coupla others are temporarily public just for my own convenience.

//typedef struct _ZMapColumnStruct ZMapColumn;
//struct zMapColumn;
typedef struct zMapColumn ZMapColumn;

/* callback function prototypes********************************
 * These must be here as they're referred to in zMapColumn below
 */
typedef void (*colDrawFunc)  (ZMapPane pane, ZMapColumn *col,
			      float *offset, int frame);
typedef void (*colConfFunc)  (void);
typedef void (*colInitFunc)  (ZMapPane pane, ZMapColumn *col);
typedef void (*colSelectFunc)(ZMapPane pane, ZMapColumn *col,
			      void *seg, int box, 
			      double x, double y,
			      gboolean isSelect);

/**************************************************************/

struct zMapColumn {
  ZMapPane      pane;
  colInitFunc   initFunc;
  colDrawFunc   drawFunc;
  colConfFunc   configFunc;
  colSelectFunc selectFunc;
  gboolean          isFrame;
  float         priority;
  char         *name;
  float         startx, endx; /* filled in by drawing code */
  methodID      meth;         /* method */
  ZMapFeatureType        type;
  void         *private;
};


struct ZMapColDefs {
  colInitFunc   initFunc;
  colDrawFunc   drawFunc;
  colConfFunc   configFunc;
  colSelectFunc selectFunc;
  gboolean          isFrame;
  float         priority; /* only for default columns. */
  char         *name;
  ZMapFeatureType        type;
}; 


typedef struct {
  ZMapWindow window;             /* the window pane  */
  Calc_cb    calc_cb;            /* callback routine */
  void      *seqRegion;          /* AceDB region     */
} ZMapCallbackData;



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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
STORE_HANDLE zMapWindowGetHandle       (ZMapWindow window);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

void         zMapWindowSetHandle       (ZMapWindow window);
void         zMapWindowCreateRegion    (ZMapWindow window);
GNode       *zMapWindowGetPanesTree    (ZMapWindow window);
void         zMapWindowSetPanesTree    (ZMapWindow window, GNode *node);
void         zMapWindowSetFirstTime    (ZMapWindow window, gboolean value);
ZMapPane     zMapWindowGetFocuspane    (ZMapWindow window);
void         zMapWindowSetFocuspane    (ZMapWindow window, ZMapPane pane);
int          zMapWindowGetRegionLength (ZMapWindow window);
Coord        zMapWindowGetRegionArea   (ZMapWindow window, int area);
void         zMapWindowSetRegionArea   (ZMapWindow window, Coord area, int num);
gboolean         zMapWindowGetRegionReverse(ZMapWindow window);
ScreenCoord  zMapWindowGetScaleOffset  (ZMapWindow window);
void         zMapWindowSetScaleOffset  (ZMapWindow window, ScreenCoord offset);
Coord        zMapWindowGetCoord        (ZMapWindow window, char *field);
void         zMapWindowSetCoord        (ZMapWindow window, char *field, int size);
ScreenCoord  zMapWindowGetScreenCoord  (ZMapWindow window, Coord coord, int height);
ScreenCoord  zMapWindowGetScreenCoord1 (ZMapWindow window, int height);
ScreenCoord  zMapWindowGetScreenCoord2 (ZMapWindow window, int height);
InvarCoord   zMapWindowGetOrigin       (ZMapWindow window);
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
GtkWidget   *zMapWindowGetZoomVbox     (ZMapWindow window);
void         zMapWindowSetZoomVbox     (ZMapWindow window, GtkWidget *vbox);

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

GArray      *zMapPaneNewBox2Col        (ZMapPane pane, int elements);
ZMapColumn  *zMapPaneGetBox2Col        (ZMapPane pane, int index);
GArray      *zMapPaneSetBox2Col        (ZMapPane pane, ZMapColumn *col, int index);
void         zMapPaneFreeBox2Col       (ZMapPane pane);
GArray      *zMapPaneNewBox2Seg        (ZMapPane pane, int elements);
ZMapFeature zMapPaneGetBox2Seg        (ZMapPane pane, int index);
GArray      *zMapPaneSetBox2Seg        (ZMapPane pane, ZMapColumn *seg, int index);
void         zMapPaneFreeBox2Seg       (ZMapPane pane);
ZMapRegion  *zMapPaneGetZMapRegion     (ZMapPane pane);
FooCanvasItem *zMapPaneGetGroup        (ZMapPane pane);
ZMapWindow   zMapPaneGetZMapWindow     (ZMapPane pane);
FooCanvas   *zMapPaneGetCanvas         (ZMapPane pane);
GPtrArray   *zMapPaneGetCols           (ZMapPane pane);
int          zMapPaneGetDNAwidth       (ZMapPane pane);
void         zMapPaneSetDNAwidth       (ZMapPane pane, int width);
void         zMapPaneSetStepInc        (ZMapPane pane, int incr);
int          zMapPaneGetHeight         (ZMapPane pane);
InvarCoord   zMapPaneGetCentre         (ZMapPane pane);
float        zMapPaneGetBPL            (ZMapPane pane);

void         addPane                   (ZMapWindow  window, char orientation);
int          recordFocus               (GtkWidget *widget, GdkEvent *event, gpointer data); 
void         navUpdate                 (GtkAdjustment *adj, gpointer p);
void         navChange                 (GtkAdjustment *adj, gpointer p);

void         drawNavigatorWind         (ZMapPane pane);

#endif /* !ZMAP_WINDOW_H */
