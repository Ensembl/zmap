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
 * Last edited: Jul 19 10:05 2006 (edgrif)
 * Created: Thu Jul 24 14:39:06 2003 (edgrif)
 * CVS info:   $Id: zmapControl_P.h,v 1.44 2006-07-19 09:09:12 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONTROL_P_H
#define ZMAP_CONTROL_P_H

#include <gtk/gtk.h>

#include <ZMap/zmapView.h>
#include <ZMap/zmapNavigator.h>
#include <ZMap/zmapControl.h>
#include <ZMap/zmapXRemote.h>


/* The overall state of the zmap, we need this because both the zmap window and the its threads
 * will die asynchronously so we need to block further operations while they are in this state. */
typedef enum {
  ZMAP_INIT,						    /* ZMap with no views. */
  ZMAP_VIEWS,						    /* ZMap with at least one view. */

  /* I think this must be redundant now....its the view that is reset, not the control zmap... */
  ZMAP_RESETTING,					    /* Display being reset. */


  ZMAP_DYING						    /* ZMap is dying for some reason,
							       cannot do anything in this state. */
} ZmapState ;




/* A ZMap Control struct represents a single top level window which is a "ZMap", within
 * this top level window there will be one or more zmap "Views". */
typedef struct _ZMapStruct
{
  ZMapCallbacks zmap_cbs_G ;				    /* Callbacks to our creator. */

  gchar           *zmap_id ;				    /* unique for each zmap.... */

  ZmapState        state ;

  GdkAtom          zmap_atom ;			            /* Used for communicating with zmap */

  void            *app_data ;				    /* Data passed back to all callbacks
							       registered for this ZMap. */

  int window_height ;					    /* Overall height of zmap window. */

  /* Widget stuff for the Zmap. */
  GtkTooltips     *tooltips ;

  GtkWidget       *toplevel ;				    /* top level widget of zmap window. */


  /* Show status of focus view sequence as:  <strand> <seq_coords> <zmap/view status> */
  GtkWidget       *status_revcomp ;
  GtkWidget       *status_coords ;
  GtkWidget       *status_entry ;


  GtkWidget       *navview_frame ;			    /* Holds all the navigator/view stuff. */

  GtkWidget       *hpane ;				    /* Holds the navigator and the view(s). */


  /* Main control buttons. */
  GtkWidget *stop_button, *load_button,
    *hsplit_button, *vsplit_button, 
    *unlock_but, *unsplit_but,
    *zoomin_but, *zoomout_but,
    *revcomp_but, *column_but,
    *quit_button, *sequence_but ;

  /* Feature details display. */
  GtkWidget *feature_name,
    *feature_strand,
    *feature_coords, *sub_feature_coords,
    *feature_score, *feature_type, *feature_set ;
  GtkTooltips *feature_tooltips ;


  /* The navigator. */
  ZMapNavigator    navigator ;

  /* The panes and views. */
  GtkWidget      *pane_vbox ;				    /* Is the parent of all the panes. */


  ZMapViewWindow focus_viewwindow ;
  GHashTable* viewwindow_2_parent ;			    /* holds hash to go from a view window
							       to that windows parent widget
							       (currently a frame). */

  /* List of views in this zmap. */
  GList          *view_list ;

  GError         *info;                 /* This is an object to hold a code
                                         * and a message as info for the
                                         * remote control simple IPC stuff */
  zMapXRemoteObj client;
  zMapXRemoteNotifyData propertyNotifyData;

} ZMapStruct ;




/* Functions internal to zmapControl. */
gboolean   zmapControlWindowCreate     (ZMap zmap) ;
GtkWidget *zmapControlWindowMakeMenuBar(ZMap zmap) ;
GtkWidget *zmapControlWindowMakeButtons(ZMap zmap) ;
GtkWidget *zmapControlWindowMakeInfoPanel(ZMap zmap) ;
GtkWidget *zmapControlWindowMakeFrame  (ZMap zmap) ;
void       zmapControlWindowDestroy    (ZMap zmap) ;

void zmapControlButtonTooltips(ZMap zmap) ;
void zmapControlInfoPanelTooltips(ZMap zmap, ZMapFeatureDesc feature_desc) ;

ZMapView zmapControlNewWindow(ZMap zmap, char *sequence, int start, int end) ;
void zmapControlSplitWindow(ZMap zmap, GtkOrientation orientation) ;

void zmapControlClose(ZMap zmap) ;
void zmapControlRemoveWindow(ZMap zmap) ;

ZMapView zmapControlAddView(ZMap zmap, char *sequence, int start, int end) ;
int zmapControlNumViews(ZMap zmap) ;

/* these may not need to be exposed.... */
GtkWidget *zmapControlAddWindow(ZMap zmap, GtkWidget *curr_frame,
				GtkOrientation orientation, char *view_title) ;


gboolean zmapControlWindowDoTheZoom(ZMap zmap, double zoom) ;
void zmapControlWindowSetZoomButtons(ZMap zmap, ZMapWindowZoomStatus zoom_status) ;
void zmapControlSetWindowFocus(ZMap zmap, ZMapViewWindow new_viewwindow) ;
void zmapControlUnSetWindowFocus(ZMap zmap, ZMapViewWindow new_viewwindow) ;

void zmapControlTopLevelKillCB(ZMap zmap) ;
void zmapControlLoadCB        (ZMap zmap) ;
void zmapControlResetCB       (ZMap zmap) ;

void zmapControlRemoteInstaller(GtkWidget *widget, gpointer zmap);
void zmapControlWriteWindowIdFile(Window id, char *window_name);

void zmapControlInfoOverwrite(void *data, int code, char *format, ...);
void zmapControlInfoSet(void *data, int code, char *format, ...);


void zmapControlWindowSetStatus(ZMap zmap) ;
void zmapControlWindowSetGUIState(ZMap zmap) ;
void zmapControlWindowSetButtonState(ZMap zmap) ;

#endif /* !ZMAP_CONTROL_P_H */
