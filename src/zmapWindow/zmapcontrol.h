/*  Last edited: May 14 10:34 2004 (rnc) */
/*  file: zmapcontrol.h
 *  Author: Simon Kelley (srk@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
 *-------------------------------------------------------------------
 * Zmap is free software; you can redistribute it and/or
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
 *      Rob Clack    (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 */

#ifndef ZMAPCONTROL_H
#define ZMAPCONTROL_H

#include <gtk/gtk.h>
#include <libfoocanvas/libfoocanvas.h>
#include <zmapcommon.h>
#include <seqregion.h>


typedef struct zMapWindow ZMapWindow;
typedef struct zMapPane ZMapPane;

typedef int   VisibleCoord;
typedef float ScreenCoord;
typedef char *Base;


struct zMapWindow {
  /* Data associated with whole window. */
  STORE_HANDLE    handle;
  GtkWidget      *window;
  GtkWidget      *vbox1;
  GtkItemFactory *itemFactory;
  GtkWidget      *infoSpace;
  Graph           navigator;
  FooCanvas      *navcanvas;
  InvarCoord      origin; /* that base which is VisibleCoord 1 */
  GtkWidget      *zoomvbox;
  GtkWidget      *toolbar;
  GtkWidget      *hbox;
  GtkWidget      *hpane;  /* allows the user to minimise the navigator pane */
  GNode          *panesTree;
  ZMapPane       *focuspane;
  BOOL            firstTime;
  /* navigator stuff */
  Coord           navStart, navEnd; /* Start drawing the Nav bar from here */
  ScreenCoord     scaleOffset;
};

struct zMapPane {
  /* Data associated with one scrolling pane. */
  ZMapWindow  *window; /* parent */
  ZMapRegion  *zMapRegion; /* the region holding all the SEGS */
  Graph        graph;
  GtkWidget   *graphWidget;
  GtkWidget   *vbox;
  GtkWidget   *pane;
  GtkWidget   *frame;
  GtkWidget   *scrolledWindow;
  FooCanvas   *canvas;    /* where we paint the display */
  FooCanvasItem *background;
  FooCanvasItem *group;
  GtkWidget   *combo;
  int          basesPerLine;
  InvarCoord   centre;
  int          graphHeight;
  int          dragBox, scrollBox;
  Array        cols;
  Array        box2seg, box2col;
  STORE_HANDLE drawHandle; /* gets freed on each redraw. */
  int          DNAwidth;
  double       zoomFactor;
  int          step_increment;
  gulong       hid1, hid2, hid3;
};

struct zMapColumn;
typedef struct zMapColumn ZMapColumn;

/* callback function prototypes********************************
 * These must be here as they're referred to in zMapColumn below
 */
typedef void (*colDrawFunc)  (ZMapPane *pane, ZMapColumn *col,
			      float *offset, int frame);
typedef void (*colConfFunc)  (void);
typedef void (*colInitFunc)  (ZMapPane *pane, ZMapColumn *col);
typedef void (*colSelectFunc)(ZMapPane *pane, ZMapColumn *col,
			      void *seg, int box, 
			      double x, double y,
			      BOOL isSelect);

/**************************************************************/

struct zMapColumn {
  ZMapPane *pane;
  colInitFunc initFunc;
  colDrawFunc drawFunc;
  colConfFunc configFunc;
  colSelectFunc selectFunc;
  BOOL isFrame;
  float priority;
  char *name;
  float startx, endx; /* filled in by drawing code */
  methodID meth; /* method */
  srType type;
  void *private;
};

struct ZMapColDefs {
  colInitFunc initFunc;
  colDrawFunc drawFunc;
  colConfFunc configFunc;
  colSelectFunc selectFunc;
  BOOL isFrame;
  float priority; /* only for default columns. */
  char *name;
  srType type;
}; 


typedef struct {
  ZMapWindow *window;             /* the window pane  */
  Calc_cb calc_cb;            /* callback routine */
  void *seqRegion;            /* AceDB region     */
} ZMapCallbackData;


/* function prototypes ************************************/

BOOL zMapDisplay(Activate_cb act_cb,
		 Calc_cb calc_cb,
		 void *region,
		 char *seqspec, 
		 char *fromspec, 
		 BOOL isOldGraph);

void zmRegBox(ZMapPane *pane, int box, ZMapColumn *col, void *seg);

/* Column drawing code ************************************/

void  zMapFeatureColumn(ZMapPane *pane, ZMapColumn *col,
			float *offset, int frame);
void  zMapDNAColumn    (ZMapPane *pane, ZMapColumn *col,
			float *offsetp, int frame);
void  buildCols        (ZMapPane *pane);
void  makezMapDefaultColumns(ZMapPane *pane);
/*float zmDrawScale     (FooCanvas *canvas, float offset, int start, int end);*/
float zmDrawScale       (float offset, int start, int end);
void  nbcInit           (ZMapPane *pane, ZMapColumn *col);
void  nbcSelect         (ZMapPane *pane, ZMapColumn *col,
			 void *seg, int box, double x, double y, BOOL isSelect);
void  zMapGeneDraw      (ZMapPane *pane, ZMapColumn *col, float *offset, int frame);
void  geneSelect        (ZMapPane *pane, ZMapColumn *col,
			 void *arg, int box, double x, double y, BOOL isSelect);

/* other routines *****************************************/

BOOL         zmIsOnScreen     (ZMapPane *pane, Coord coord1, Coord coord2);
VisibleCoord zmVisibleCoord   (ZMapWindow   *window  , Coord coord);
ScreenCoord  zmScreenCoord    (ZMapPane *pane, Coord coord);
Coord        zmCoordFromScreen(ZMapPane *pane, ScreenCoord coord);
void         addPane          (ZMapWindow *window, char orientation);
BOOL         Quit             (GtkWidget *widget, gpointer data);

     
#endif
/************************** end of file **********************************/
