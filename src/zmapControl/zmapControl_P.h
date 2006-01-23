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
 * Last edited: Jan 18 10:28 2006 (edgrif)
 * Created: Thu Jul 24 14:39:06 2003 (edgrif)
 * CVS info:   $Id: zmapControl_P.h,v 1.37 2006-01-23 14:14:36 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONTROL_P_H
#define ZMAP_CONTROL_P_H

#include <gtk/gtk.h>
#include <ZMap/zmapSys.h>
#include <ZMap/zmapView.h>
#include <ZMap/zmapNavigator.h>
#include <ZMap/zmapControl.h>
#include <ZMap/zmapXRemote.h>


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
  gchar           *zmap_id ;				    /* unique for each zmap.... */

  ZmapState        state ;

  GdkAtom          zmap_atom ;			            /* Used for communicating with zmap */

  void            *app_data ;				    /* Data passed back to all callbacks
							       registered for this ZMap. */

  /* Widget stuff for the Zmap. */
  GtkWidget       *toplevel ;				    /* top level widget of zmap window. */

  GtkWidget       *info_panel;                              /* show details of object clicked on */

  GtkWidget       *navview_frame ;			    /* Holds all the navigator/view stuff. */

  GtkWidget       *hpane ;				    /* Holds the navigator and the view(s). */


  /* buttons etc. */
  GtkWidget *zoomin_but, *zoomout_but, *close_but, *revcomp_but, *unlock_but ;

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
GtkWidget *zmapControlWindowMakeFrame  (ZMap zmap) ;
void       zmapControlWindowDestroy    (ZMap zmap) ;


void zmapControlSplitInsertWindow(ZMap zmap, ZMapView new_view, GtkOrientation orientation) ;
void zmapControlRemoveWindow(ZMap zmap) ;

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
void zmapControlNewViewCB(ZMap zmap, char *new_sequence) ;

void zmapControlRemoteInstaller(GtkWidget *widget, gpointer zmap);
void zmapControlWriteWindowIdFile(Window id, char *window_name);

void zmapControlInfoOverwrite(void *data, int code, char *format, ...);
void zmapControlInfoSet(void *data, int code, char *format, ...);


#endif /* !ZMAP_CONTROL_P_H */
