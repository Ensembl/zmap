/*  File: zmapControl_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *     Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and,
 *          Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *       Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Private header for interface that creates/manages/destroys
 *              instances of ZMaps.
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONTROL_P_H
#define ZMAP_CONTROL_P_H

#include <gtk/gtk.h>

#include <ZMap/zmapView.h>
#include <ZMap/zmapNavigator.h>
#include <ZMap/zmapControl.h>
#include <ZMap/zmapControlImportFile.h>
#include <ZMap/zmapXRemote.h>



/* Windows are 90% of screen height by default...but normally we automatically set window to fill screen taking
 * into account window manager tool bars etc. */
#define ZMAPWINDOW_VERT_PROP 0.90

#define USE_REGION	0	/* scroll bar pane on left that does nothing */

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



/* When splitting a window you may want the existing window to be first or last. */
typedef enum {ZMAPCONTROL_SPLIT_INVALID, ZMAPCONTROL_SPLIT_FIRST, ZMAPCONTROL_SPLIT_LAST} ZMapControlSplitOrder ;



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

  gboolean remote_control ;

  ZMapFeatureSequenceMap default_sequence;             /* piinter to app_context default_sequence */


  /* Widget stuff for the Zmap. */
  GtkTooltips     *tooltips ;

  GtkWidget       *toplevel ;				    /* top level widget of zmap window. */


  /* Show status of focus view sequence as:  <strand> <seq_coords> <zmap/view status> */
  GtkWidget       *status_revcomp ;
  GtkWidget       *status_coords ;
  GtkWidget       *status_entry ;


  GtkWidget       *navview_frame ;			    /* Holds all the navigator/view stuff. */
  GtkWidget       *nav_canvas ;

  GtkWidget       *hpane ;				    /* Holds the navigator and the view(s). */

  GtkWidget       *button_info_box;

  /* Main control buttons. */
  GtkWidget *stop_button, *load_button,
    *hsplit_button, *vsplit_button,
    *unlock_but, *unsplit_but,
    *zoomin_but, *zoomout_but,
    *filter_but,
    *revcomp_but, *column_but,
    *quit_button, *frame3_but,
    *dna_but, *back_button;

  ZMapWindowFilterStruct filter;
  gboolean filter_spin_pressed;			/* flag to prevent value changed signal handling when spinning button */

#ifdef SEE_INFOPANEL_STRUCT
  /* Feature details display. */
  GtkWidget *feature_name, *feature_strand,
    *feature_coords, *sub_feature_coords,
    *feature_frame, *feature_score, *feature_type,
    *feature_set, *feature_style ;
#endif /* SEE_INFOPANEL_STRUCT */

  GtkTooltips *feature_tooltips ;

  GtkWidget *info_panel_vbox;
  GHashTable *view2infopanel;

  /* The navigator. */
  ZMapNavigator    navigator ;


  /* The panes and views and current focus window. */
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


  /* Old stuff...??? */
  ZMapXRemoteObj xremote_client;
  ZMapXRemoteObj xremote_server;          /* that we reply to */



  gulong map_handler ;					    /* Needed for disconnecting map handler cb. */

} ZMapStruct ;

typedef struct
{
  GtkWidget *feature_name, *feature_strand,
    *feature_coords, *sub_feature_coords,
    *feature_frame, *feature_population, *feature_score, *feature_type,
    *feature_set, *feature_source ;

  GtkWidget *hbox;

} ZMapInfoPanelLabelsStruct, *ZMapInfoPanelLabels;

#define VIEW_XREMOTE_WIDGET "view_xremote_widget"	    /* Key used for setting/getting view
							       on xremote widget. */


/* Functions internal to zmapControl. */
gboolean   zmapControlWindowCreate     (ZMap zmap) ;
GtkWidget *zmapControlWindowMakeMenuBar(ZMap zmap) ;
GtkWidget *zmapControlWindowMakeButtons(ZMap zmap) ;
GtkWidget *zmapControlWindowMakeInfoPanel(ZMap zmap, ZMapInfoPanelLabels labels) ;
GtkWidget *zmapControlWindowMakeFrame  (ZMap zmap) ;
void       zmapControlWindowDestroy    (ZMap zmap) ;

void zmapControlButtonTooltips(ZMap zmap) ;
void zmapControlInfoPanelTooltips(ZMap zmap, ZMapFeatureDesc feature_desc) ;
void zmapControlInfoPanelSetText(ZMap zmap, ZMapInfoPanelLabels labels, ZMapFeatureDesc feature_desc) ;

ZMapViewWindow zmapControlNewWindow(ZMap zmap, ZMapFeatureSequenceMap sequence_map) ;
void zmapControlSplitWindow(ZMap zmap, GtkOrientation orientation, ZMapControlSplitOrder window_order) ;

void zmapControlClose(ZMap zmap) ;
void zmapControlRemoveWindow(ZMap zmap) ;

ZMapView zmapControlInsertView(ZMap zmap, ZMapFeatureSequenceMap sequence_map, char **err_msg) ;
ZMapView zmapControlAddView(ZMap zmap, ZMapFeatureSequenceMap sequence_map) ;
int zmapControlNumViews(ZMap zmap) ;
void zmapControlRemoveView(ZMap zmap, ZMapView view, ZMapViewWindowTree destroyed_zmap_inout) ;

gboolean zmapConnectViewConfig(ZMap zmap, ZMapView view, char *config);
void zmapControlShowPreferences(ZMap zmap) ;

/* these may not need to be exposed.... */
GtkWidget *zmapControlAddWindow(ZMap zmap, GtkWidget *curr_frame,
				GtkOrientation orientation,
				ZMapControlSplitOrder window_order,
				char *view_title) ;


gboolean zmapControlWindowDoTheZoom(ZMap zmap, double zoom) ;
void zmapControlWindowSetZoomButtons(ZMap zmap, ZMapWindowZoomStatus zoom_status) ;
void zmapControlSetWindowFocus(ZMap zmap, ZMapViewWindow new_viewwindow) ;
void zmapControlUnSetWindowFocus(ZMap zmap, ZMapViewWindow new_viewwindow) ;
void zmapControlSignalKill(ZMap zmap) ;
void zmapControlDoKill(ZMap zmap, ZMapViewWindowTree *destroyed_zmaps) ;

void zmapControlLoadCB        (ZMap zmap) ;
void zmapControlResetCB       (ZMap zmap) ;



/* new remote stuff.... */
void zmapControlSendViewCreated(ZMap zmap, ZMapView view, ZMapWindow window) ;
void zmapControlSendViewDeleted(ZMap zmap, ZMapViewWindowTree destroyed_zmaps) ;



/* old remote stuff.... */
void zmapControlRemoteInstaller(GtkWidget *widget, GdkEvent  *event, gpointer user_data) ;
gboolean zmapControlRemoteAlertClient(ZMap zmap,
                                      char *action, GArray *xml_events,
                                      ZMapXMLObjTagFunctions start_handlers,
                                      ZMapXMLObjTagFunctions end_handlers,
                                      gpointer *handler_data);
gboolean zmapControlRemoteAlertClients(ZMap zmap, GList *clients,
                                       char *action, GArray *xml_events,
				       ZMapXMLObjTagFunctions start_handlers,
				       ZMapXMLObjTagFunctions end_handlers,
				       gpointer *handler_data);




void zmapControlWriteWindowIdFile(Window id, char *window_name);

void zmapControlInfoOverwrite(void *data, int code, char *format, ...);
void zmapControlInfoSet(void *data, int code, char *format, ...);


void zmapControlWindowSetStatus(ZMap zmap) ;
void zmapControlWindowSetGUIState(ZMap zmap) ;
void zmapControlWindowSetButtonState(ZMap zmap, ZMapWindowFilter filter) ;
ZMapViewWindow zmapControlNewWidgetAndWindowForView(ZMap zmap,
                                                    ZMapView zmap_view,
                                                    ZMapWindow zmap_window,
                                                    GtkWidget *curr_container,
                                                    GtkOrientation orientation,
						    ZMapControlSplitOrder window_order,
                                                    char *view_title);

#endif /* !ZMAP_CONTROL_P_H */
