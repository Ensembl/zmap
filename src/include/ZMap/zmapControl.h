/*  File: zmapControl.h
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Interface for creating, controlling and destroying ZMaps.
 *              
 * HISTORY:
 * Last edited: Jul  2 10:50 2004 (rnc)
 * Created: Mon Nov 17 08:04:32 2003 (edgrif)
 * CVS info:   $Id: zmapControl.h,v 1.3 2004-07-02 13:27:45 rnc Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONTROL_H
#define ZMAP_CONTROL_H


/* It turns out we need some way to refer to zmapviews at this level, while I don't really
 * want to expose all the zmapview stuff I do want the opaque ZMapView type.
 * Think about this some more. */
#include <ZMap/zmapView.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapWindow.h>

/* Opaque type, represents an instance of a ZMap. */
typedef struct _ZMapStruct *ZMap ;

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



/* Applications can register functions that will be called back with their own
 * data and a reference to the zmap that made the callback. */
typedef void (*ZMapCallbackFunc)(ZMap zmap, void *app_data) ;



ZMap zMapCreate(void *app_data, ZMapCallbackFunc zmap_destroyed_cb) ;

/* NOW THIS IS WHERE WE NEED GERROR........ */
ZMapView zMapAddView(ZMap zmap, char *sequence) ;
gboolean zMapConnectView(ZMap zmap, ZMapView view) ;
gboolean zMapLoadView(ZMap zmap, ZMapView view) ;
gboolean zMapStopView(ZMap zmap, ZMapView view) ;
gboolean zMapDeleteView(ZMap zmap, ZMapView view) ;

char *zMapGetZMapID(ZMap zmap) ;
char *zMapGetZMapStatus(ZMap zmap) ;
gboolean zMapReset(ZMap zmap) ;
gboolean zMapDestroy(ZMap zmap) ;

GPtrArray   *zMapPaneGetCols           (ZMapPane pane);
void         zMapPaneNewBox2Col        (ZMapPane pane, int elements);
ZMapColumn  *zMapPaneGetBox2Col        (ZMapPane pane, int index);
GArray      *zMapPaneSetBox2Col        (ZMapPane pane, ZMapColumn *col, int index);
void         zMapPaneFreeBox2Col       (ZMapPane pane);
void         zMapPaneNewBox2Seg        (ZMapPane pane, int elements);
ZMapFeature zMapPaneGetBox2Seg        (ZMapPane pane, int index);
GArray      *zMapPaneSetBox2Seg        (ZMapPane pane, ZMapColumn *seg, int index);
void         zMapPaneFreeBox2Seg       (ZMapPane pane);
ZMapRegion  *zMapPaneGetZMapRegion     (ZMapPane pane);
FooCanvasItem *zMapPaneGetGroup        (ZMapPane pane);
ZMapWindow   zMapPaneGetZMapWindow     (ZMapPane pane);
FooCanvas   *zMapPaneGetCanvas         (ZMapPane pane);
int          zMapPaneGetDNAwidth       (ZMapPane pane);
void         zMapPaneSetDNAwidth       (ZMapPane pane, int width);
void         zMapPaneSetStepInc        (ZMapPane pane, int incr);
int          zMapPaneGetHeight         (ZMapPane pane);
InvarCoord   zMapPaneGetCentre         (ZMapPane pane);
float        zMapPaneGetBPL            (ZMapPane pane);


#endif /* !ZMAP_CONTROL_H */
