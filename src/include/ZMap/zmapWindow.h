/*  Last edited: Feb 14 20:31 2012 (edgrif) */
/*  File: zmapWindow.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 *     Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *       Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *  Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Defines interface to code that creates/handles a
 *              window displaying genome data.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_H
#define ZMAP_WINDOW_H

#include <glib.h>

/* I think the canvas and features headers should not be in here...they need to be removed
 * in time.... */

/* SHOULD CANVAS BE HERE...MAYBE, MAYBE NOT...... */
#include <libzmapfoocanvas/libfoocanvas.h>

#include <ZMap/zmapUtilsGUI.h>
#include <ZMap/zmapFeature.h>

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <ZMap/zmapRemoteCommand.h>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#include <ZMap/zmapAppRemote.h>

/* should be able to get rid of this shortly... */
#include <ZMap/zmapXRemote.h>

#include <ZMap/zmapXMLHandler.h>


/* X Windows has some limits that are part of the protocol, this means they cannot
 * be changed any time soon...they impinge on the canvas which could have a very
 * large windows indeed. Unfortunately X just silently resets the window to size
 * 1 when you try to create larger windows....now read on...
 *
 * The largest X window that can be created has max dimensions of 65535 (i.e. an
 * unsigned short), you can easily test this for a server by creating a window and
 * then querying its size, you should find this is the max. window size you can have.
 *
 * BUT....you can't really make use of a window this size _because_ when positioning
 * anything (other windows, lines etc.), the coordinates are given via _signed_ ints.
 * This means that the maximum position you can specify must in the range -32768
 * to 32767. In a way this makes sense because it means that you can have a window
 * that covers this entire span and so position things anywhere inside it. In a way
 * its completely crap and confusing.......
 *
 * BUT....it means that in the visible window you are limited to the range 0 - 32767.
 * by sticking at 30,000 we allow ourselves a margin for error that should work on any
 * machine.
 *
 */

#define ZMAP_WINDOW_MAX_WINDOW 	30000			    /* Largest canvas window. */



/*! Opaque type, represents an individual ZMap window. */
typedef struct _ZMapWindowStruct *ZMapWindow ;



/*! opaque */
typedef struct _ZMapWindowStateStruct *ZMapWindowState;
typedef struct _GQueue *ZMapWindowStateQueue;


/*! indicates how far the zmap is zoomed, n.b. ZMAP_ZOOM_FIXED implies that the whole sequence
 * is displayed at the maximum zoom. */
typedef enum {ZMAP_ZOOM_INIT, ZMAP_ZOOM_MIN, ZMAP_ZOOM_MID, ZMAP_ZOOM_MAX,
	      ZMAP_ZOOM_FIXED} ZMapWindowZoomStatus ;

/*! Should the original window and the new window be locked together for scrolling and zooming.
 * vertical means that the vertical scrollbars should be locked together, specifying vertical
 * or horizontal means locking of zoom as well. */
typedef enum {ZMAP_WINLOCK_NONE, ZMAP_WINLOCK_VERTICAL, ZMAP_WINLOCK_HORIZONTAL} ZMapWindowLockType ;


/* Controls display of 3 frame translations and/or 3 frame columns when 3 frame mode is selected. */
typedef enum
  {
    ZMAP_WINDOW_3FRAME_INVALID,
    ZMAP_WINDOW_3FRAME_TRANS,				    /* 3 frame translation display. */
    ZMAP_WINDOW_3FRAME_COLS,				    /* 3 frame other cols display. */
    ZMAP_WINDOW_3FRAME_ALL				    /* All 3 frame cols. */
  } ZMapWindow3FrameMode ;


/*! ZMap Window has various callbacks which will return different types of data for various actions. */


/*! Data returned to the visibilityChange callback routine which is called whenever the scrollable
 * section of the window changes, e.g. when zooming. */
typedef struct
{
  ZMapWindowZoomStatus zoom_status ;

  /* Top/bottom coords for section of sequence that can be scrolled currently in window. */
  double scrollable_top ;
  double scrollable_bot ;
} ZMapWindowVisibilityChangeStruct, *ZMapWindowVisibilityChange ;


/* info to operate spin button for filter by score */
/* NOTE this works only with a ZMapWindowCanvasFeatureset, not with normal Foo items, on account of speed */


typedef int (*ZMapWindowFilterCallbackFunc)(gpointer filter, double value) ;

typedef struct
{
	double min,max;			/* for the spin button */
	double value;
	ZMapWindow window;
	gpointer featureset;		/* where to filter, is a ZMapWindowCanvasFeatureset... scope :-( */
	gpointer column;			/* where to filter, is a ZMapWindowCanvasFeaturesetItem... scope :-( */

	ZMapWindowFilterCallbackFunc func;		/* function to do it */

	int n_filtered;			/* how many we hid */
	gboolean enable;

} ZMapWindowFilterStruct, *ZMapWindowFilter;




/*! Data returned to the focus callback routine, called whenever a feature is selected. */

typedef enum {ZMAPWINDOW_SELECT_SINGLE, ZMAPWINDOW_SELECT_DOUBLE} ZMapWindowSelectType ;

typedef struct
{
  ZMapWindowSelectType type;				    /* SINGLE or DOUBLE */

  FooCanvasItem *highlight_item ;			    /* The feature selected to be highlighted, may be null
							       if a column was selected. */

  gboolean replace_highlight_item ;			    /* TRUE means highlight item replaces
							       existing highlighted item, FALSE
							       means its added to the list of
							       highlighted items. */

  gboolean highlight_same_names ;			    /* TRUE means highlight all other
							       features with the same name in the
							       same feature set. */

  ZMapFeatureDescStruct feature_desc ;			    /* Text descriptions of selected feature. */

  char *secondary_text ;				    /* Simple string description. */


  ZMapWindowFilterStruct filter;



  /* Old xremote stuff.... */

  /* For Xremote XML actions/events. */
  ZMapXRemoteSendCommandError remote_result ;

  ZMapXMLHandlerStruct xml_handler ;



} ZMapWindowSelectStruct, *ZMapWindowSelect ;






/*! Data returned to the split window call. */
typedef struct
{
  ZMapWindow original_window;   /* We need to know where we came from... */
  GArray *split_patterns;

  FooCanvasItem *item;
  int window_index;             /* Important for stepping through the touched windows and split_patterns */
  gpointer other_data;
} ZMapWindowSplittingStruct, *ZMapWindowSplitting;



/*
 * THIS IS GOING BACK TO HOW I ORIGINALLY ENVISAGED THE CALLBACK SYSTEM WHICH WAS AS A COMMAND
 * FIELD FOLLOWED BY DIFFERENT INFO. STRUCTS...I.E. LIKE THE SIGNAL INTERFACE OF GTK ETC...
 *
 * Data returned by the "command" callback, note all command structs must start with the
 * CommandAny fields.
 *
 */

typedef enum
  {
    ZMAPWINDOW_CMD_INVALID,
    ZMAPWINDOW_CMD_GETFEATURES,
    ZMAPWINDOW_CMD_SHOWALIGN,
    ZMAPWINDOW_CMD_REVERSECOMPLEMENT
  } ZMapWindowCommandType ;


/* Controls mode of column compression: this removes or compresses columns to the width required
 * for the features within the whole sequence, the marked area or just the visible window. */
typedef enum
  {
    ZMAPWINDOW_COMPRESS_INVALID,
    ZMAPWINDOW_COMPRESS_VISIBLE,			    /* Compress for visible window only. */
    ZMAPWINDOW_COMPRESS_MARK,				    /* Compress for mark region. */
    ZMAPWINDOW_COMPRESS_ALL				    /* Compress for whole sequence. */
  } ZMapWindowCompressMode ;



typedef struct
{
  ZMapWindowCommandType cmd ;
} ZMapWindowCallbackCommandAnyStruct, *ZMapWindowCallbackCommandAny ;



/* Call an alignment display program for the given alignment feature. */
typedef enum
  {
    ZMAPWINDOW_ALIGNCMD_INVALID,
    ZMAPWINDOW_ALIGNCMD_NONE,
    ZMAPWINDOW_ALIGNCMD_FEATURES,
    ZMAPWINDOW_ALIGNCMD_SET,
    ZMAPWINDOW_ALIGNCMD_MULTISET,
    ZMAPWINDOW_ALIGNCMD_SEQ
  } ZMapWindowAlignSetType ;

typedef struct ZMapWindowCallbackCommandAlignStructName
{
  /* Common section. */
  ZMapWindowCommandType cmd ;

  ZMapFeatureBlock block ;

  /* Align specific section. */
  ZMapHomolType homol_type ;				    /* DNA or Peptide alignments. */

  int offset ;						    /* Offset for displaying coords with
							       different base. */

  int cursor_position ;					    /* Where the alignment tools "cursor" should be. */

  int window_start, window_end ;			    /* Start/end coords that should be initially
							       visible in alignment tool.  */

  int mark_start, mark_end ;				    /* Optional range for alignment viewing. */


  ZMapWindowAlignSetType homol_set ;			    /* What features to display. */


  GList *features ;					    /* Optional list of alignment features. */

  ZMapFeatureSet feature_set ;

  GList *source;                                     /* a list of featureset names */

} ZMapWindowCallbackCommandAlignStruct, *ZMapWindowCallbackCommandAlign ;


/* Call sources to get new features. */
typedef struct
{
  /* Common section. */
  ZMapWindowCommandType cmd ;

  ZMapFeatureBlock block ;				    /* Block for which features should be fetched. */
  GList *feature_set_ids ;				    /* List of names as quarks. */
  int start, end ;					    /* Range over which features should be fetched. */

} ZMapWindowCallbackCommandGetFeaturesStruct, *ZMapWindowCallbackGetFeatures ;



/* Reverse complement features. */
typedef struct
{
  /* Common section. */
  ZMapWindowCommandType cmd ;

  /* No extra data needed for rev. comp. */

} ZMapWindowCallbackCommandRevCompStruct, *ZMapWindowCallbackCommandRevComp ;







/* Callback functions that can be registered with ZMapWindow, functions are registered all in one.
 * go via the ZMapWindowCallbacksStruct. */
typedef void (*ZMapWindowCallbackFunc)(ZMapWindow window, void *caller_data, void *window_data) ;

typedef struct _ZMapWindowCallbacksStruct
{
  ZMapWindowCallbackFunc enter ;
  ZMapWindowCallbackFunc leave ;
  ZMapWindowCallbackFunc scroll ;
  ZMapWindowCallbackFunc focus ;
  ZMapWindowCallbackFunc select ;
  ZMapWindowCallbackFunc splitToPattern ;
  ZMapWindowCallbackFunc setZoomStatus ;
  ZMapWindowCallbackFunc visibilityChange ;
  ZMapWindowCallbackFunc command ;			    /* Request to exit given command. */
  ZMapWindowCallbackFunc drawn_data ;

  ZMapRemoteAppMakeRequestFunc remote_request_func ;

} ZMapWindowCallbacksStruct, *ZMapWindowCallbacks ;


typedef struct _ZMapWindowLongItemsStruct *ZMapWindowLongItems ;




void zMapWindowInit(ZMapWindowCallbacks callbacks) ;
ZMapWindow zMapWindowCreate(GtkWidget *parent_widget,
                            ZMapFeatureSequenceMap sequence, void *app_data,
                            GList *feature_set_names) ;
ZMapWindow zMapWindowCopy(GtkWidget *parent_widget, ZMapFeatureSequenceMap sequence,
			  void *app_data, ZMapWindow old,
			  ZMapFeatureContext features, GHashTable *all_styles, GHashTable *new_styles,
			  ZMapWindowLockType window_locking) ;

gboolean zMapWindowProcessRemoteRequest(ZMapWindow window,
					char *command_name, ZMapAppRemoteViewID view_id, char *request,
					ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data) ;


void zMapWindowBusyFull(ZMapWindow window, gboolean busy, const char *file, const char *func) ;
#ifdef __GNUC__
#define zMapWindowBusy(WINDOW, BUSY)         \
  zMapWindowBusyFull((WINDOW), (BUSY), __FILE__, (char *)__PRETTY_FUNCTION__)
#else
#define zMapWindowBusy(WINDOW, BUSY)         \
  zMapWindowBusyFull((WINDOW), (BUSY), __FILE__, NULL)
#endif

void zMapWindowDisplayData(ZMapWindow window, ZMapWindowState state,
			   ZMapFeatureContext current_features, ZMapFeatureContext new_features,
                     ZMapFeatureContextMap context_map,
                     GList *masked) ;
void zMapWindowUnDisplayData(ZMapWindow window,
                             ZMapFeatureContext current_features,
                             ZMapFeatureContext new_features);
void zMapWindowMove(ZMapWindow window, double start, double end) ;
void zMapWindowReset(ZMapWindow window) ;
void zMapWindowRedraw(ZMapWindow window) ;
void zMapWindowFeatureReset(ZMapWindow window, gboolean features_are_revcomped);
void zMapWindowFeatureRedraw(ZMapWindow window, ZMapFeatureContext feature_context,
			     gboolean reversed) ;
void zMapWindowZoom(ZMapWindow window, double zoom_factor) ;
ZMapWindowZoomStatus zMapWindowGetZoomStatus(ZMapWindow window) ;
double zMapWindowGetZoomFactor(ZMapWindow window);
double zMapWindowGetZoomMin(ZMapWindow window) ;
double zMapWindowGetZoomMax(ZMapWindow window) ;
double zMapWindowGetZoomMagnification(ZMapWindow window);
double zMapWindowGetZoomMagAsBases(ZMapWindow window) ;
double zMapWindowGetZoomMaxDNAInWrappedColumn(ZMapWindow window);

gboolean zMapWindowZoomToFeature(ZMapWindow window, ZMapFeature feature) ;
void zMapWindowZoomToWorldPosition(ZMapWindow window, gboolean border,
				   double rootx1, double rooty1,
                                   double rootx2, double rooty2);

void zmapWindowCoordPairToDisplay(ZMapWindow window,
				  int start_in, int end_in,
				  int *display_start_out, int *display_end_out) ;
gboolean zmapWindowGetCurrentSpan(ZMapWindow window,int *start,int *end);

gboolean zMapWindowGetMark(ZMapWindow window, int *start, int *end) ;
gboolean zMapWindowMarkGetSequenceSpan(ZMapWindow window, int *start, int *end) ;
void zmapWindowMarkPrint(ZMapWindow window, char *title) ;
gboolean zMapWindowMarkIsSet(ZMapWindow window);


void zmapWindowColumnBumpRange(FooCanvasItem *bump_item, ZMapStyleBumpMode bump_mode, ZMapWindowCompressMode compress_mode) ;
void zmapWindowFullReposition(ZMapWindow window) ;



gboolean zMapWindowGetDNAStatus(ZMapWindow window);
void zMapWindowStats(ZMapWindow window,GString *text) ;

void zMapWindow3FrameToggle(ZMapWindow window) ;
void zMapWindow3FrameSetMode(ZMapWindow window, ZMapWindow3FrameMode frame_mode) ;

/* should become internal to zmapwindow..... */
void zMapWindowToggleDNAProteinColumns(ZMapWindow window,
                                       GQuark align_id,   GQuark block_id,
                                       gboolean dna,      gboolean protein, gboolean trans,
                                       gboolean force_to, gboolean force);

void zMapWindowStateRecord(ZMapWindow window);
void zMapWindowBack(ZMapWindow window);
gboolean zMapWindowHasHistory(ZMapWindow window);
gboolean zMapWindowIsLocked(ZMapWindow window) ;
void zMapWindowSiblingWasRemoved(ZMapWindow window);	    /* For when a window in the same view
							       has a child removed */

PangoFontDescription *zMapWindowZoomGetFixedWidthFontInfo(ZMapWindow window,
                                                          double *width_out,
                                                          double *height_out);

void zMapWindowGetVisible(ZMapWindow window, double *top_out, double *bottom_out) ;
gboolean zMapWindowGetVisibleSeq(ZMapWindow window, FooCanvasItem *focus, int *top_out, int *bottom_out) ;
FooCanvasItem *zMapWindowFindFeatureItemByItem(ZMapWindow window, FooCanvasItem *item) ;

void zMapWindowColumnList(ZMapWindow window) ;
void zMapWindowColumnConfigure(ZMapWindow window, FooCanvasGroup *column_group) ;

gboolean zMapWindowExport(ZMapWindow window) ;
gboolean zMapWindowDump(ZMapWindow window) ;
gboolean zMapWindowPrint(ZMapWindow window) ;

/* Add, modify, draw, remove features from the canvas. */
FooCanvasItem *zMapWindowFeatureAdd(ZMapWindow window,
			      FooCanvasGroup *feature_group, ZMapFeature feature) ;
FooCanvasItem *zMapWindowFeatureSetAdd(ZMapWindow window,
                                       FooCanvasGroup *block_group,
                                       char *feature_set_name) ;
FooCanvasItem *zMapWindowFeatureReplace(ZMapWindow zmap_window,
					FooCanvasItem *curr_feature_item,
					ZMapFeature new_feature, gboolean destroy_orig_feature) ;
gboolean zMapWindowFeatureRemove(ZMapWindow zmap_window, FooCanvasItem *feature_item, ZMapFeature feature, gboolean destroy_feature) ;

gint zMapFeatureCmp(gconstpointer a, gconstpointer b);

gboolean zMapWindowGetMaskedColour(ZMapWindow window,GdkColor **border,GdkColor **fill);
gboolean zMapWindowGetFilteredColour(ZMapWindow window, GdkColor **fill);


void zMapWindowScrollToWindowPos(ZMapWindow window, int window_y_pos) ;
gboolean zMapWindowCurrWindowPos(ZMapWindow window,
				 double *x1_out, double *y1_out, double *x2_out, double *y2_out) ;
gboolean zMapWindowMaxWindowPos(ZMapWindow window,
				double *x1_out, double *y1_out, double *x2_out, double *y2_out) ;
gboolean zMapWindowScrollToItem(ZMapWindow window, FooCanvasItem *feature_item) ;

gboolean zMapWindowFeatureSelect(ZMapWindow window, ZMapFeature feature) ;

void zMapWindowHighlightFeature(ZMapWindow window, ZMapFeature feature) ;
void zMapWindowHighlightObject(ZMapWindow window, FooCanvasItem *feature,
			       gboolean replace_highlight_item, gboolean highlight_same_names) ;
void zMapWindowHighlightObjects(ZMapWindow window, ZMapFeatureContext context, gboolean multiple_select);

void zMapWindowDestroyLists(ZMapWindow window) ;
void zMapWindowUnlock(ZMapWindow window) ;
void zMapWindowMergeInFeatureSetNames(ZMapWindow window, GList *feature_set_names);

GList *zMapWindowGetSpawnedPIDList(ZMapWindow window);
void zMapWindowDestroy(ZMapWindow window) ;

void zMapWindowMenuAlignBlockSubMenus(ZMapWindow window,
                                      ZMapGUIMenuItem each_align,
                                      ZMapGUIMenuItem each_block,
                                      char *root,
                                      GArray **items_array_out);

gboolean zMapWindowXRemoteRegister(ZMapWindow window) ;

/* It's possible these are defunct now..... */
char *zMapWindowRemoteReceiveAccepts(ZMapWindow window);
void zMapWindowSetupXRemote(ZMapWindow window, GtkWidget *widget);




void zMapWindowUtilsSetClipboard(ZMapWindow window, char *text);

ZMapGuiNotebookChapter zMapWindowGetConfigChapter(ZMapWindow window, ZMapGuiNotebook parent);



/* window focus */

typedef struct _ZMapWindowFocusStruct *ZMapWindowFocus ;

typedef enum
{
      /* types of groups of features, used to index an array
       * NOTE: these are in order of priority
       */
      WINDOW_FOCUS_GROUP_FOCUS = 0,
      WINDOW_FOCUS_GROUP_EVIDENCE,
      WINDOW_FOCUS_GROUP_TEXT,

      /* NOTE above = focus items, below = global colours see BITMASK and FOCUSSED below */

	/* window focus has a cache of colours per window
	 * we can retrieve these by id which does note require out of scope data or headers
	 */
      WINDOW_FOCUS_GROUP_MASKED,	/* use to store colours but not set as a focus type */
      WINDOW_FOCUS_GROUP_FILTERED,
/* NOTE this must be the first blurred one */
#define N_FOCUS_GROUPS_FOCUS WINDOW_FOCUS_GROUP_MASKED

      N_FOCUS_GROUPS
} ZMapWindowFocusType;


extern int focus_group_mask[];	/* indexed by ZMapWindowFocusType */

#define WINDOW_FOCUS_GROUP_FOCUSSED 0x07	/* all the focus types */
#define WINDOW_FOCUS_GROUP_BLURRED	0xf8	/* all the blurred types */
#define WINDOW_FOCUS_GROUP_BITMASK	0xff	/* all the colour types, focussed and blurred */
#define WINDOW_FOCUS_DONT_USE	0xff00	/* see FEATURE_FOCUS in zmapWindowCanvasFeatureSet.c */
/* x-ref with zmapWindowCanvasFeatureset_I.h */

#define WINDOW_FOCUS_ID	0xffff0000

/* bitmap return values from the following function, _please_ don't enum them! */
#define WINDOW_FOCUS_CACHE_FILL 		1
#define WINDOW_FOCUS_CACHE_OUTLINE		2
#define WINDOW_FOCUS_CACHE_SELECTED		4
int zMapWindowFocusCacheGetSelectedColours(int id_flags,gulong *fill,gulong *outline);

void zMapWindowFocusCacheSetSelectedColours(ZMapWindow window);

gboolean zmapWindowFocusHasType(ZMapWindowFocus focus, ZMapWindowFocusType type);
gboolean zMapWindowFocusGetColour(ZMapWindow window,int mask, GdkColor *fill, GdkColor *border);


#endif /* !ZMAP_WINDOW_H */
