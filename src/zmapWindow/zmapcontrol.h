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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 */

typedef struct zMapLook ZMapLook;
typedef struct zMapRoot ZMapRoot;
typedef struct zMapWindow ZMapWindow;

typedef int VisibleCoord;
typedef float ScreenCoord;
typedef char *Base;


BOOL zmIsOnScreen(ZMapWindow *window, Coord coord1, Coord coord2);
VisibleCoord zmVisibleCoord(ZMapRoot *root, Coord coord);
ScreenCoord zmScreenCoord(ZMapWindow *window, Coord coord);
Coord zmCoordFromScreen(ZMapWindow *window, ScreenCoord coord);

struct zMapLook {
  /* Data associated with whole window. */
  STORE_HANDLE handle;
  ZMapRoot *root1, *root2; 
  GtkWidget *gexWindow;
  GtkWidget  *vbox;
  GtkItemFactory *itemFactory;
  GtkWidget *buttonBar;
  GtkWidget *infoSpace;
  GtkWidget *pane;
};

struct zMapRoot {
  /* Data associated with whole window. */
  ZMapLook *look; /* parent */
  ZMapWindow *window1, *window2; /* linked list */
  Graph navigator;
  SeqRegion *region;
  InvarCoord origin; /* that base which is VisibleCoord 1 */
  GtkWidget *hbox, *vbox;
  GtkWidget *pane;
  GtkWidget *splitButton;
  /* navigator stuff */
  Coord navStart, navEnd; /* Start drawing the Nav bar from here */
  ScreenCoord scaleOffset;
};

struct zMapWindow {
  /* Data associated with one scrolling window. */
  ZMapLook *look; /* parent */
  ZMapRoot *root; /* parent */
  Graph graph;
  GtkWidget *graphWidget;
  GtkWidget *scrolledWindow;
  GtkWidget *vbox;
  GtkWidget *combo;
  int basesPerLine;
  InvarCoord centre;
  int graphHeight;
  int dragBox, scrollBox;
  Array cols;
  Array box2seg, box2col;
  STORE_HANDLE drawHandle; /* gets freed on each redraw. */
  int DNAwidth;
};

struct zMapColumn;
typedef struct zMapColumn ZMapColumn;
void zmRegBox(ZMapWindow *window, int box, ZMapColumn *col, void *seg);
typedef void (*colDrawFunc)(ZMapWindow *window, ZMapColumn *col,
			    float *offset, int frame);
typedef void (*colConfFunc)(void);
typedef void (*colInitFunc)(ZMapWindow *window, ZMapColumn *col);
typedef void (*colSelectFunc)(ZMapWindow *window, ZMapColumn *col,
			      void *seg, int box, double x, double y,
			      BOOL isSelect);
struct zMapColumn {
  ZMapWindow *window;
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


/* Column drawing code. */
void zMapFeatureColumn(ZMapWindow *window, ZMapColumn *col,
		       float *offset, int frame);
void zMapDNAColumn(ZMapWindow *window, ZMapColumn *col,
		   float *offsetp, int frame);
void buildCols(ZMapWindow *window);
void makezMapDefaultColumns(ZMapWindow *window);
float zmDrawScale(float offset, int start, int end);
void nbcInit(ZMapWindow *window, ZMapColumn *col);
void nbcSelect(ZMapWindow *window, ZMapColumn *col,
	     void *seg, int box, double x, double y, BOOL isSelect);
void zMapGeneDraw(ZMapWindow *window, ZMapColumn *col, float *offset, int frame);
void geneSelect(ZMapWindow *window, ZMapColumn *col,
		void *arg, int box, double x, double y, BOOL isSelect);
