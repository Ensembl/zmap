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
 * Last edited: Jul 15 17:16 2004 (rnc)
 * Created: Thu Jul 24 14:39:06 2003 (edgrif)
 * CVS info:   $Id: zmapControl_P.h,v 1.7 2004-07-15 16:31:50 rnc Exp $
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
  gchar           *zmap_id ;	    /* unique for each zmap.... */

  ZmapState        state ;

  gboolean firstTime;

  GdkAtom zmap_atom ;					    /* Used for communicating with zmap */

  void *app_data ;					    /* Data passed back to all callbacks
							       registered for this ZMap. */

  /* Widget stuff for the Zmap. */
  GtkWidget *toplevel ;					    /* top level widget of zmap window. */

  GtkWidget *navview_frame ;				    /* Holds all the navigator/view stuff. */

  GtkWidget *hpane ;					    /* Holds the navigator and the view(s). */

  /* The navigator. */
  GtkWidget *navigator ;
  FooCanvas *navcanvas ;
  GtkWidget *navHBox, *navVBox;

  /* The panes and views. */

  //  GtkWidget       *displayvbox;
  //  GtkWidget       *hbox;
  /* I'm not completely sure this is necessary....revisit this later.... */
  GtkWidget      *pane_vbox ;

  /* Panes are windows where views get displayed. */
  ZMapPane        focuspane ;
  GNode          *panesTree ;

  //  ZMapView         curr_view ;

  /* List of views in this zmap. */
  GList *view_list ;


  /* In DAS2 terminology methods are types...easy to change if we don't like the name.
   * These are the stylesheets in effect for the feature sets. */
  GData *types ;


} ZMapStruct ;



/* Data associated with one scrolling pane. */
typedef struct _ZMapPaneStruct
{
  ZMap zmap ;						    /* Back ptr to containing zmap. */

  ZMapViewWindow curr_view_window ;

  GtkWidget   *pane ;
  GtkWidget   *frame ;
  GtkWidget   *view_parent_box ;

} ZMapPaneStruct ;


/* Functions internal to zmapControl. */
gboolean   zmapControlWindowCreate     (ZMap zmap) ;
GtkWidget *zmapControlWindowMakeMenuBar(ZMap zmap) ;
GtkWidget *zmapControlWindowMakeButtons(ZMap zmap) ;
GtkWidget *zmapControlWindowMakeFrame  (ZMap zmap) ;
void       zmapControlWindowDestroy    (ZMap zmap) ;


GtkWidget *zmapControlNavigatorCreate(FooCanvas **canvas_out) ;
void zmapControlNavigatorNewView(ZMapMapBlock sequence_to_parent) ;

void zmapControlTopLevelKillCB(ZMap zmap) ;
void zmapControlLoadCB        (ZMap zmap) ;
void zmapControlResetCB       (ZMap zmap) ;
void zmapControlNewCB         (ZMap zmap, char *testing_text) ;


void zmapRecordFocus(ZMapPane pane) ; 
ZMapPane zmapAddPane(ZMap zmap, char orientation) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
gboolean zMapDisplay(ZMap        zmap,
		     Activate_cb act_cb,
		     Calc_cb     calc_cb,
		     void       *region,
		     char       *seqspec, 
		     char       *fromspec, 
		     gboolean        isOldGraph);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




GtkWidget *splitPane(ZMap zmap) ;
GtkWidget *splitHPane(ZMap zmap) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* NOT CALLED FROM ANYWHERE ????? */
void  closePane       (GtkWidget *widget, gpointer data);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





void  drawNavigator  (ZMap zmap) ;
void  drawWindow     (ZMapPane pane);
void  zMapZoomToolbar(ZMapWindow window);
void  navScale       (FooCanvas *canvas, float offset, int start, int end);



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
