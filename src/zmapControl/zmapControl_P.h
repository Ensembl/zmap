/*  File: zmapControl_P.h
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *
 * Description: Private header for interface that creates/manages/destroys
 *              instances of ZMaps.
 * HISTORY:
 * Last edited: Jul  2 18:57 2004 (edgrif)
 * Created: Thu Jul 24 14:39:06 2003 (edgrif)
 * CVS info:   $Id: zmapControl_P.h,v 1.4 2004-07-02 18:23:42 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONTROL_P_H
#define ZMAP_CONTROL_P_H

#include <gtk/gtk.h>
#include <ZMap/zmapSys.h>
#include <ZMap/zmapView.h>
#include <ZMap/zmapControl.h>



/* The overall state of the zmap, we need this because both the zmap window and the its threads
 * will die asynchronously so we need to block further operations while they are in this state.
 * Note that after a window is "reset" it goes back to the init state. */
typedef enum {
  ZMAP_INIT,						    /* Created, display but no views. */
  ZMAP_VIEWS,						    /* Display with views in normal state. */
  ZMAP_RESETTING,					    /* Display being reset. */
  ZMAP_DYING						    /* ZMap is dying for some reason,
							       cannot do anything in this state. */
} ZmapState ;



/* A ZMap Control struct represents a single top level window which is a "ZMap", within
 * this top level window there will be one or more zmap "Views". */
typedef struct _ZMapStruct
{
  gchar *zmap_id ;					    /* unique for each zmap.... */

  ZmapState state ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  void *app_data ;  
  ZMapCallbackFunc destroy_zmap_cb ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  GtkWidget *toplevel ;

  /* MOVED HERE FROM zmapWindow/    */
  ZMapPane        focuspane ;
  GNode          *panesTree ;
  gboolean        firstTime;
  GtkWidget      *hpane;  /* allows the user to minimise the navigator pane */
  GtkWidget      *navigator;
  GtkWidget      *frame;
  GtkWidget      *vbox;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  GtkItemFactory *itemFactory;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  FooCanvas      *navcanvas;
  GtkWidget      *displayvbox;
  GtkWidget      *hbox;






  GtkWidget *view_parent ;

  ZMapView curr_view ;
  GList *view_list ;

  GdkAtom zmap_atom ;

  /* caller registers a routine that gets called when this zmap is destroyed. */
  ZMapCallbackFunc app_zmap_destroyed_cb ;
  void *app_data ;

  ZMapWindow zMapWindow;

} ZMapStruct ;



typedef struct _ZMapPaneStruct {
  /* Data associated with one scrolling pane. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapWindow   window;     /* parent */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  ZMap zmap ;						    /* Back ptr to containing zmap. */

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

  int          DNAwidth;
  double       zoomFactor;
  int          step_increment;
} ZMapPaneStruct;


/* Functions internal to zmapControl. */
gboolean   zmapControlWindowCreate     (ZMap zmap, char *zmap_id) ;
GtkWidget *zmapControlWindowMakeMenuBar(ZMap zmap) ;
GtkWidget *zmapControlWindowMakeButtons(ZMap zmap) ;
GtkWidget *zmapControlWindowMakeFrame  (ZMap zmap) ;
void       zmapControlWindowDestroy    (ZMap zmap) ;

void zmapControlTopLevelKillCB(ZMap zmap) ;
void zmapControlLoadCB        (ZMap zmap) ;
void zmapControlResetCB       (ZMap zmap) ;
void zmapControlNewCB         (ZMap zmap, char *testing_text) ;

gboolean zMapDisplay(ZMap        zmap,
		     Activate_cb act_cb,
		     Calc_cb     calc_cb,
		     void       *region,
		     char       *seqspec, 
		     char       *fromspec, 
		     gboolean        isOldGraph);

void  addPane        (ZMap zmap, char orientation);

void  drawNavigator  (ZMap zmap) ;
void  drawWindow     (ZMapPane pane);
void  zMapZoomToolbar(ZMapWindow window);
void  navScale       (FooCanvas *canvas, float offset, int start, int end);

int          recordFocus               (GtkWidget *widget, GdkEvent *event, gpointer data); 

void         navUpdate                 (GtkAdjustment *adj, gpointer p);
void         navChange                 (GtkAdjustment *adj, gpointer p);




/* Moved from ZMap/zmapWindow.h, these may be in the wrong place or not even needed. */
ZMapPane     zMapWindowGetFocuspane    (ZMapWindow window);
void         zMapWindowSetFocuspane    (ZMapWindow window, ZMapPane pane);
GNode       *zMapWindowGetPanesTree    (ZMapWindow window);
void         zMapWindowSetPanesTree    (ZMapWindow window, GNode *node);
void         zMapWindowSetFirstTime    (ZMapWindow window, gboolean value);
GtkWidget   *zMapWindowGetHpane        (ZMapWindow window);
void         zMapWindowSetHpane        (ZMapWindow window, GtkWidget *hpane);




#endif /* !ZMAP_CONTROL_P_H */
