/*  File: zmapControl_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 *       Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *  Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Private header for interface that creates/manages/destroys
 *              instances of ZMaps.
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONTROL_P_H
#define ZMAP_CONTROL_P_H

#include <gtk/gtk.h>

#include <ZMap/zmapView.hpp>
#include <ZMap/zmapControl.hpp>
#include <ZMap/zmapControlImportFile.hpp>
#include <zmapNavigator_P.hpp>                                /* WHY is this here ?? */




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

  ZMapFeatureSequenceMap default_sequence;		    /* pointer to app_context default_sequence */

  /* Set a colour for visual grouping of zmap with peer application windows. */
  GdkColor session_colour ;
  gboolean session_colour_set ;


  /* Widget stuff for the Zmap. */
  GtkWidget       *toplevel ;				    /* top level widget of zmap window. */

  gboolean shrinkable ;                                     /* TRUE => allow user to shrink window to min. */

  GtkWidget *menubar ;

  /* info panel tooltips to show meaning of info panel fields. */
  GtkTooltips     *tooltips ;

  /* parent of event boxes used to monitor xremote stuff...not needed in new xremote....
   * must be a container type widget. */
  GtkWidget *event_box_parent ;

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
    *dna_but, *back_button, *scratch_but;

  ZMapWindowFilterStruct filter;
  gboolean filter_spin_pressed;			/* flag to prevent value changed signal handling when spinning button */

  GtkTooltips *feature_tooltips ;

  GtkWidget *info_panel_vbox;
  GHashTable *view2infopanel;

  /* The navigator. */
  ZMapNavigator    navigator ;

  /* Notebook for preferences, should only be one per zmap. */
  ZMapGuiNotebook preferences_note_book ;

  /* New sequence dialog. */
  GtkWidget *sequence_dialog ;

  /* The panes and views and current focus window. */
  GtkWidget *pane_vbox ;				    /* Is the parent of all the panes. */

  /* List of views in this zmap. */
  GList          *view_list ;

  ZMapViewWindow focus_viewwindow ;


  GHashTable* viewwindow_2_parent ;			    /* holds hash to go from a view window
							       to that windows parent widget
							       (currently a frame). */

  GError         *info;                 /* This is an object to hold a code
                                         * and a message as info for the
                                         * remote control simple IPC stuff */



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


ZMapCallbacks zmapControlGetCallbacks(void) ;
gboolean zmapControlWindowCreate (ZMap zmap, GdkCursor *normal_cursor) ;
GtkWidget *zmapControlWindowMakeMenuBar(ZMap zmap) ;
GtkWidget *zmapControlWindowMakeButtons(ZMap zmap) ;
GtkWidget *zmapControlWindowMakeInfoPanel(ZMap zmap, ZMapInfoPanelLabels labels) ;
GtkWidget *zmapControlWindowMakeFrame (ZMap zmap) ;
void zmapControlWindowDestroy (ZMap zmap) ;

void zmapControlButtonTooltips(ZMap zmap) ;
void zmapControlInfoPanelTooltips(ZMap zmap, ZMapFeatureDesc feature_desc) ;
void zmapControlInfoPanelSetText(ZMap zmap, ZMapInfoPanelLabels labels, ZMapFeatureDesc feature_desc) ;

ZMapViewWindow zmapControlNewWindow(ZMap zmap, ZMapFeatureSequenceMap sequence_map) ;
void zmapControlSplitWindow(ZMap zmap, GtkOrientation orientation, ZMapControlSplitOrder window_order) ;

void zmapControlClose(ZMap zmap) ;

/* these may not need to be exposed.... */
GtkWidget *zmapControlAddWindow(ZMap zmap, GtkWidget *curr_frame,
				GtkOrientation orientation,
				ZMapControlSplitOrder window_order,
				char *view_title) ;
void zmapControlRemoveWindow(ZMap zmap, ZMapViewWindow view_window, GList **destroyed_views_inout) ;
void zmapControlCloseFull(ZMap zmap, ZMapView view) ;

ZMapView zmapControlInsertView(ZMap zmap, ZMapFeatureSequenceMap sequence_map, char **err_msg) ;
ZMapViewWindow zmapControlAddView(ZMap zmap, ZMapFeatureSequenceMap sequence_map) ;
void zmapControlRemoveView(ZMap zmap, ZMapView view, GList **destroyed_views_inout) ;
ZMapViewWindow zmapControlFindViewWindow(ZMap zmap, ZMapView view) ;
int zmapControlNumViews(ZMap zmap) ;

gboolean zmapConnectViewConfig(ZMap zmap, ZMapView view, char *config);
void zmapControlShowPreferences(ZMap zmap) ;

gboolean zmapControlWindowToggleDisplayCoordinates(ZMap zmap) ;

gboolean zmapControlWindowDoTheZoom(ZMap zmap, double zoom) ;
void zmapControlWindowSetZoomButtons(ZMap zmap, ZMapWindowZoomStatus zoom_status) ;

void zmapControlSetWindowFocus(ZMap zmap, ZMapViewWindow new_viewwindow) ;
void zmapControlUnSetWindowFocus(ZMap zmap, ZMapViewWindow new_viewwindow) ;

void zmapControlSignalKill(ZMap zmap) ;
void zmapControlDoKill(ZMap zmap, GList **destroyed_views_out) ;

void zmapControlLoadCB        (ZMap zmap) ;
void zmapControlResetCB       (ZMap zmap) ;

void zmapControlSendViewCreated(ZMap zmap, ZMapView view, ZMapWindow window) ;
void zmapControlSendViewDeleted(ZMap zmap, GList *destroyed_views_inout) ;

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

void zmapControlPrintView(ZMap zmap, ZMapView view, char *action, gboolean print_xid) ;
void zmapControlPrintAllViews(ZMap zmap, gboolean print_xids) ;

void zmapControlWindowMaximize(GtkWidget *widget, ZMap map) ;

#endif /* !ZMAP_CONTROL_P_H */

