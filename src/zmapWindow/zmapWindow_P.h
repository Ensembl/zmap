/*  File: zmapWindow_P.h
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
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jun 28 15:09 2004 (rnc)
 * Created: Fri Aug  1 16:45:58 2003 (edgrif)
 * CVS info:   $Id: zmapWindow_P.h,v 1.8 2004-06-28 14:28:26 rnc Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_P_H
#define ZMAP_WINDOW_P_H

#include <gtk/gtk.h>
#include <ZMap/zmapWindow.h>


/* Test scaffoling */
#include <ZMap/zmapFeature.h>

typedef struct _ZMapWindowStruct
{
  gchar *sequence ;

  GtkWidget *parent_widget ;
  GtkWidget *toplevel ;
  GtkWidget *text ;

  GdkAtom zmap_atom ;

  zmapVoidIntCallbackFunc app_routine ;
  void *app_data ;

  STORE_HANDLE    handle;
  GtkWidget      *frame;
  GtkWidget      *vbox;
  GtkItemFactory *itemFactory;
  GtkWidget      *infoSpace;
  GtkWidget      *navigator;
  FooCanvas      *navcanvas;
  InvarCoord      origin; /* that base which is VisibleCoord 1 */
  GtkWidget      *zoomvbox;
  GtkWidget      *toolbar;
  GtkWidget      *hbox;
  GtkWidget      *hpane;  /* allows the user to minimise the navigator pane */
  GNode          *panesTree;
  ZMapPane        focuspane;
  BOOL            firstTime;
  /* navigator stuff */
  Coord           navStart, navEnd; /* Start drawing the Nav bar from here */
  ScreenCoord     scaleOffset;
} ZMapWindowStruct ;


typedef struct _ZMapPaneStruct {
  /* Data associated with one scrolling pane. */
  ZMapWindow   window;     /* parent */
  ZMapRegion  *zMapRegion; /* the region holding all the SEGS */
  Graph        graph;
  GtkWidget   *graphWidget;
  GtkWidget   *vbox;
  GtkWidget   *pane;
  GtkWidget   *frame;
  GtkWidget   *scrolledWindow;
  FooCanvas   *canvas;     /* where we paint the display */
  FooCanvasItem *background;
  FooCanvasItem *group;
  GtkWidget   *combo;
  int          basesPerLine;
  InvarCoord   centre;
  int          graphHeight;
  int          dragBox, scrollBox;
  GPtrArray    cols;
  GArray       *box2seg, *box2col;
  STORE_HANDLE drawHandle; /* gets freed on each redraw. */
  int          DNAwidth;
  double       zoomFactor;
  int          step_increment;
} ZMapPaneStruct;


typedef struct
{
  ZMapWindow window ;
  void *data ;						    /* void for now, union later ?? */
} zmapWindowDataStruct, *zmapWindowData ;



/* Used in our event communication.... */
#define ZMAP_ATOM  "ZMap_Atom"


GtkWidget *zmapWindowMakeMenuBar(ZMapWindow window) ;
GtkWidget *zmapWindowMakeButtons(ZMapWindow window) ;
GtkWidget *zmapWindowMakeFrame(ZMapWindow window) ;




/* TEST SCAFFOLDING............... */
ZMapFeatureContext testGetGFF(void) ;



#endif /* !ZMAP_WINDOW_P_H */
