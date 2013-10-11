/*  File: zmapView.h
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Interface for controlling a single "view", a view
 *              comprises one or more windowsw which display data
 *              collected from one or more servers. Hence the view
 *              interface controls both windows and connections to
 *              servers.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAPVIEW_H
#define ZMAPVIEW_H

#include <gtk/gtk.h>

#include <ZMap/zmapEnum.h>
#include <ZMap/zmapWindow.h>
#include <ZMap/zmapWindowNavigator.h>
#include <ZMap/zmapXMLHandler.h>
#include <ZMap/zmapUrl.h>
#include <ZMap/zmapAppRemote.h>



/* Struct describing features loaded. */
typedef struct ZMapViewLoadFeaturesDataStructName
{
  char *err_msg;        // from the server mainly
  gchar *stderr_out;
  gint exit_code;
  int num_features;

  GList *feature_sets ;
  int start,end;        // requested coords
  gboolean status;      // load sucessful?
  unsigned long xwid ;  // X Window id for the xremote widg. */

} ZMapViewLoadFeaturesDataStruct, *ZMapViewLoadFeaturesData ;



/* Opaque type, represents an instance of a ZMapView. */
typedef struct _ZMapViewStruct *ZMapView ;


/* Opaque type, represents an instance of a ZMapView window. */
typedef struct _ZMapViewWindowStruct *ZMapViewWindow ;


/* Callers can specify callback functions which get called with ZMapView which made the call
 * and the applications own data pointer. If the callback is to made for a window event
 * then the ZMapViewWindow where the event took place will be returned as the first
 * parameter. If the callback is for an event that involves the whole view (e.g. destroy)
 * then the ZMapView where the event took place is returned. */
typedef void (*ZMapViewWindowCallbackFunc)(ZMapViewWindow view_window, void *app_data, void *view_data) ;
typedef void (*ZMapViewCallbackFunc)(ZMapView zmap_view, void *app_data, void *view_data) ;


/* Set of callback routines that allow the caller to be notified when events happen
 * to a window. */
typedef struct _ZMapViewCallbacksStruct
{
  ZMapViewWindowCallbackFunc enter ;
  ZMapViewWindowCallbackFunc leave ;
  ZMapViewCallbackFunc load_data ;
  ZMapViewWindowCallbackFunc focus ;
  ZMapViewWindowCallbackFunc select ;
  ZMapViewWindowCallbackFunc split_to_pattern;
  ZMapViewWindowCallbackFunc visibility_change ;
  ZMapViewCallbackFunc state_change ;
  ZMapViewCallbackFunc destroy ;

  ZMapRemoteAppMakeRequestFunc remote_request_func ;
  ZMapRemoteAppMakeRequestFunc remote_request_func_data ;

} ZMapViewCallbacksStruct, *ZMapViewCallbacks ;



/* The overall state of the zmapView, we need this because both the zmap window and the its threads
 * will die asynchronously so we need to block further operations while they are in this state. */
#define VIEW_STATE_LIST(_)                                                           \
_(ZMAPVIEW_INIT,            , "Init",          "View initialising.",            "") \
_(ZMAPVIEW_MAPPED,          , "Mapped",        "View displayed.",               "") \
_(ZMAPVIEW_CONNECTING,      , "Connecting",    "View connecting to data sources.",       "") \
_(ZMAPVIEW_CONNECTED,       , "Connected",     "View data sources connected.", "") \
_(ZMAPVIEW_LOADING,         , "Data loading",  "View loading data.",           "") \
_(ZMAPVIEW_LOADED,          , "Data loaded",   "View data loaded.",           "") \
_(ZMAPVIEW_UPDATING,        , "Data updating", "View data updating.",           "") \
_(ZMAPVIEW_RESETTING,       , "Resetting",     "View resetting.",           "") \
_(ZMAPVIEW_DYING,           , "Dying",         "View terminating.",     "")

ZMAP_DEFINE_ENUM(ZMapViewState, VIEW_STATE_LIST) ;



/* data passed back from view for destroy callback. */
typedef struct
{
  unsigned long xwid ;
} ZMapViewCallbackDestroyDataStruct, *ZMapViewCallbackDestroyData ;


typedef struct
{
  unsigned long xwid ;
  ZMapViewState state;
} ZMapViewCallbackFubarStruct, *ZMapViewCallbackFubar ;





/* Holds a sequence to be fetched and the server it should be fetched from. */
typedef struct
{
  char *sequence ;					    /* Sequence + start/end coords. */
  char *server ;					    /* Server to fetch sequence from. */
} ZMapViewSequence2ServerStruct, *ZMapViewSequence2Server ;




/* Holds structs/strings describing the selected item, this data actually comes from the
 * zmapWindow layer and is just passed on through view. */
typedef struct _ZMapViewSelectStruct
{
  ZMapWindowSelectType  type;
  ZMapFeatureDescStruct feature_desc ;
  ZMapWindowFilterStruct filter;
  char                 *secondary_text;
} ZMapViewSelectStruct, *ZMapViewSelect ;


typedef struct _ZMapViewSplittingStruct
{
  GArray *split_patterns;
  GList *touched_window_list;   /* A list of view_windows affected by the split */
}ZMapViewSplittingStruct, *ZMapViewSplitting;




void zMapViewInit(ZMapViewCallbacks callbacks) ;
ZMapViewWindow zMapViewCreate(GtkWidget *view_container,
			      ZMapFeatureSequenceMap sequence_map, void *app_data) ;
void zMapViewSetupNavigator(ZMapViewWindow view_window, GtkWidget *canvas_widget);
gboolean zMapViewGetDefaultWindow(ZMapWindow *window_out) ;
ZMapViewWindow zMapViewCopyWindow(ZMapView zmap_view, GtkWidget *parent_widget,
				  ZMapWindow copy_window, ZMapWindowLockType window_locking) ;
ZMapViewWindow zMapViewGetDefaultViewWindow(ZMapView view) ;
ZMapViewWindow zMapViewRemoveWindow(ZMapViewWindow view_window) ;

void zMapViewRedraw(ZMapViewWindow view_window) ;
gboolean zMapViewConnect(ZMapFeatureSequenceMap sequence_map, ZMapView zmap_view, char *config_str) ;
gboolean zMapViewReset(ZMapView zmap_view) ;
gboolean zMapViewReverseComplement(ZMapView zmap_view) ;
gboolean zMapViewGetRevCompStatus(ZMapView zmap_view) ;

gboolean zMapViewSessionGetAsText(ZMapViewWindow view_window, GString *session_data_inout) ;

void zMapViewZoom(ZMapView zmap_view, ZMapViewWindow view_window, double zoom) ;
char *zMapViewGetSequence(ZMapView zmap_view) ;
char *zMapViewGetSequenceName(ZMapFeatureSequenceMap sequence_map);
void zMapViewGetSourceNameTitle(ZMapView zmap_view, char **name, char **title) ;
ZMapFeatureContext zMapViewGetFeatures(ZMapView zmap_view) ;
void zMapViewGetVisible(ZMapViewWindow view_window, double *top, double *bottom) ;
ZMapViewState zMapViewGetStatus(ZMapView zmap_view) ;
char *zMapViewGetLoadStatusStr(ZMapView view, char **loading_sources_out, char **failed_sources_out) ;
GtkWidget *zMapViewGetXremote(ZMapView view) ;
gboolean zMapViewGetFeaturesSpan(ZMapView zmap_view, int *start, int *end) ;
ZMapWindow zMapViewGetWindow(ZMapViewWindow view_window) ;
ZMapView zMapViewGetView(ZMapViewWindow view_window) ;
GHashTable *zMapViewGetStyles(ZMapViewWindow view_window) ;
ZMapWindowNavigator zMapViewGetNavigator(ZMapView view);
int zMapViewNumWindows(ZMapViewWindow view_window) ;
GList *zMapViewGetWindowList(ZMapViewWindow view_window);
void   zMapViewSetWindowList(ZMapViewWindow view_window, GList *list);
char *zMapViewGetDataset(ZMapView zmap_view) ;

gboolean zMapViewProcessRemoteRequest(ZMapViewWindow view_window,
				      char *command_name, char *request,
				      ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data) ;
gpointer zMapViewFindView(ZMapView view, gpointer view_id) ;

ZMapFeatureSequenceMap zMapViewGetSequenceMap(ZMapView zmap_view);
ZMapFeatureSource zMapViewGetFeatureSetSource(ZMapView view, GQuark f_id);
void zMapViewSetFeatureSetSource(ZMapView view, GQuark f_id, ZMapFeatureSource src);
GList *zmapViewGetIniSources(char *config_file, char *config_str,char **stylesfile);

gboolean zMapViewRequestServer(ZMapView view, ZMapFeatureBlock block_orig, GList *req_featuresets,
			       gpointer server, /* ZMapConfigSource */
			       int req_start, int req_end,
			       gboolean dna_requested, gboolean terminate, gboolean show_warning);

void zMapViewShowLoadStatus(ZMapView view);

void zmapViewFeatureDump(ZMapViewWindow view_window, char *file) ;

void zMapViewHighlightFeatures(ZMapView view,
			       ZMapViewWindow view_window, ZMapFeatureContext context, gboolean multiple);

void zMapViewReadConfigBuffer(ZMapView zmap_view, char *buffer);

char *zMapViewRemoteReceiveAccepts(ZMapView view);

void zMapViewDestroy(ZMapView zmap_view) ;








/* HACK! not really to be used... */
ZMapFeatureContext zMapViewGetContextAsEmptyCopy(ZMapView do_not_use);

ZMapGuiNotebookChapter zMapViewBlixemGetConfigChapter(ZMapView view, ZMapGuiNotebook note_book_parent) ;

ZMapGuiNotebookChapter zMapViewGetPrefsChapter(ZMapView view, ZMapGuiNotebook note_book_parent);

gboolean zMapViewGetHighlightFilteredColumns(ZMapView);

void zMapViewUpdateColumnBackground(ZMapView view);


ZMAP_ENUM_AS_NAME_STRING_DEC(zMapView2Str, ZMapViewState) ;


#endif /* !ZMAPVIEW_H */
