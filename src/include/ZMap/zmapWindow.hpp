/*  File: zmapWindow.h
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

#include <ZMap/zmapUtilsGUI.hpp>
#include <ZMap/zmapFeature.hpp>
#include <ZMap/zmapFeatureLoadDisplay.hpp>
#include <ZMap/zmapAppRemote.hpp>
#include <ZMap/zmapXMLHandler.hpp>


#define SCRATCH_FEATURE_NAME "temp_feature"


/*! Opaque type, represents an individual ZMap window. */
typedef struct ZMapWindowStructType *ZMapWindow ;



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

/*
 * Coordinates to display to the user in the window.
 */
typedef enum
  {
    ZMAP_WINDOW_DISPLAY_INVALID,
    ZMAP_WINDOW_DISPLAY_SLICE,
    ZMAP_WINDOW_DISPLAY_CHROM,
    ZMAP_WINDOW_DISPLAY_OTHER
  } ZMapWindowDisplayCoordinates ;

/* Controls display of 3 frame translations and/or 3 frame columns when 3 frame mode is selected. */
typedef enum
  {
    ZMAP_WINDOW_3FRAME_OFF,                                 /* No 3 frame display. */
    ZMAP_WINDOW_3FRAME_TRANS,                               /* 3 frame translation display. */
    ZMAP_WINDOW_3FRAME_COLS,                                /* 3 frame cols display. */
    ZMAP_WINDOW_3FRAME_ALL                                  /* 3 frame cols and translation display. */
  } ZMapWindow3FrameMode ;

/* These flags are stored in an array in the ZMapView but the array is also passed to the window
 * level so they are accessible there */
typedef enum
  {
    ZMAPFLAG_REVCOMPED_FEATURES,         /* True if the user has done a revcomp */
    ZMAPFLAG_HIGHLIGHT_FILTERED_COLUMNS, /* True if filtered columns should be highlighted */
    ZMAPFLAG_FEATURES_NEED_SAVING,       /* True if there are new features that have not been saved */
    ZMAPFLAG_SCRATCH_NEEDS_SAVING,       /* True if changes have been made in the scratch column
                                          * that have not been "saved" to a real featureset */
    ZMAPFLAG_CONFIG_NEEDS_SAVING,        /* True if there are unsaved changes to prefs */
    ZMAPFLAG_STYLES_NEED_SAVING,         /* True if there are unsaved changes to styles */
    ZMAPFLAG_COLUMNS_NEED_SAVING,        /* True if there are unsaved changes to the columns order */
    ZMAPFLAG_CHANGED_FEATURESET_STYLE,   /* True if featureset-style relationships have changed */

    ZMAPFLAG_ENABLE_ANNOTATION,          /* True if we should enable editing via the annotation column */
    ZMAPFLAG_ENABLE_ANNOTATION_INIT,     /* False until the enable-annotation flag has been initialised */

    ZMAPFLAG_NUM_FLAGS                   /* Must be last in list */
  } ZMapFlag;

/* These int values are stored in an array in the ZMapView but the array is also passed to the
 * window level so that they are accessible there. */
typedef enum
  {
    ZMAPINT_SCRATCH_ATTRIBUTE_FEATURE,   /* Quark to use as feature name for scratch feature */
    ZMAPINT_SCRATCH_ATTRIBUTE_FEATURESET,/* Quark to use as featureset name for scratch feature */

    ZMAPINT_NUM_VALUES
  } ZMapIntValue;


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


typedef int (*ZMapWindowFilterCallbackFunc)(gpointer filter, double value, gboolean highlight_filtered_columns) ;

typedef struct
{
  double min_val, max_val;                                           /* for the spin button */
  double value;
  ZMapWindow window;
  gpointer featureset;                                      /* where to filter, is a ZMapWindowCanvasFeatureset... scope :-( */
  gpointer column;                                          /* where to filter, is a ZMapWindowCanvasFeaturesetItem... scope :-( */

  ZMapWindowFilterCallbackFunc func;                        /* function to do it */

  int n_filtered;                                           /* how many we hid */
  gboolean enable;

  /* be good to implement this oe day, currently we look it up from config
   * every time someone click the filter button */
  GdkColor fill_col;                                            /* background for widget and also filtered column */

} ZMapWindowFilterStruct, *ZMapWindowFilter;




/* Selection/pasting of features, data returned to the focus callback routine and so on. */

/* Feature info/coord display, selection, paste data. */

/* Coord frame for coordinate display, exporting, pasting (coords are held internally in their
 * natural frame). */
typedef enum
  {
    ZMAPWINDOW_COORD_ONE_BASED,                             /* Convert coords to one-based. */
    ZMAPWINDOW_COORD_NATURAL                                /* Leave coords as they are, e.g. chromosome. */
  } ZMapWindowCoordFrame ;

/* request a particular format for pasting of feature coords.... */
typedef enum
  {
    /* For otterlace paste all feature subparts (e.g. exons), in one-based corods
     * with feature name, lengths:
     *                              "LA16c-366D1.2-001"    28897 29136 (240)
     *                              "LA16c-366D1.2-001"    31731 32851 (1121)
     *                              etc
     */
    ZMAPWINDOW_PASTE_FORMAT_OTTERLACE,

    /* Show in universal browser chromsome coord format (chromosome and coords):
                                    "16:610422-615528"
    */
    ZMAPWINDOW_PASTE_FORMAT_BROWSER,

    /* As ZMAPWINDOW_PASTE_FORMAT_BROWSER but with a "CHR" prefix: "CHR16:610422-615528" */
    ZMAPWINDOW_PASTE_FORMAT_BROWSER_CHR
  } ZMapWindowPasteStyleType ;


/* Which part of feature should be pasted. */
typedef enum
  {
    ZMAPWINDOW_PASTE_TYPE_SELECTED,                         /* paste whatever is selected (must == 0).  */
    ZMAPWINDOW_PASTE_TYPE_ALLSUBPARTS,                      /* paste all subparts of feature. */
    ZMAPWINDOW_PASTE_TYPE_SUBPART,                          /* paste single subpart clicked on. */
    ZMAPWINDOW_PASTE_TYPE_EXTENT                            /* paste extent of feature. */
  } ZMapWindowPasteFeatureType ;


/* Use to specify how to paste, display, export features. */
typedef struct ZMapWindowDisplayStyleStructName
{
  ZMapWindowCoordFrame coord_frame ;
  ZMapWindowPasteStyleType paste_style ;
  ZMapWindowPasteFeatureType paste_feature ;
} ZMapWindowDisplayStyleStruct, *ZMapWindowDisplayStyle ;








typedef enum {ZMAPWINDOW_SELECT_SINGLE, ZMAPWINDOW_SELECT_DOUBLE} ZMapWindowSelectType ;


typedef struct
{
  ZMapWindowSelectType type;                                /* SINGLE or DOUBLE */

  FooCanvasItem *highlight_item ;                           /* The feature selected to be highlighted, may be null
                                                               if a column was selected. */

  GList *feature_list;                                      /* for lassoo multiple select */

  gboolean replace_highlight_item ;                         /* TRUE means highlight item replaces
                                                               existing highlighted item, FALSE
                                                               means its added to the list of
                                                               highlighted items. */

  gboolean highlight_same_names ;                           /* TRUE means highlight all other
                                                               features with the same name in the
                                                               same feature set. */

  gboolean highlight_sub_part ;                             /* if selecting part of a feature w/
                                                               the control key */
  ZMapFeatureSubPart sub_part ;                             /* subpart to be highlighted. */


  ZMapFeatureDescStruct feature_desc ;                      /* Text descriptions of selected feature. */


  char *secondary_text ;                                    /* Simple string description used for paste to
                                                               clipboard, format is one of ZMapWindowPasteStyleType */


  ZMapWindowFilterStruct filter;

} ZMapWindowSelectStruct, *ZMapWindowSelect ;


typedef struct
{
  ZMapFeature feature ;
  ZMapFeatureSet feature_set ;
  GError *error ;
} ZMapWindowMergeNewFeatureStruct, *ZMapWindowMergeNewFeature ;





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
 * THIS NEEDS REDOING SO THE CALLER OF WINDOW KNOWS NOTHING ABOUT THE ACTUAL FUNCTION
 * TO BE CALLED OR ANYTHING ABOUT THE ARGS, IT NEEDS GENERALISING....
 *
 * THERE SHOULD BE A SINGLE STRUCT WITH A CALLBACK AND A USER POINTER AND THAT'S IT.
 *
 * ALL OF THESE STRUCTS SHOULD BE MOVED INTERNALLY TO ZMAPWINDOW AND BE USED THERE.
 *
 */

typedef enum
  {
    ZMAPWINDOW_CMD_INVALID,
    ZMAPWINDOW_CMD_GETFEATURES,
    ZMAPWINDOW_CMD_SHOWALIGN,
    ZMAPWINDOW_CMD_REVERSECOMPLEMENT,
    ZMAPWINDOW_CMD_COLFILTER,
    ZMAPWINDOW_CMD_COPYTOSCRATCH,
    ZMAPWINDOW_CMD_SETCDSSTART,
    ZMAPWINDOW_CMD_SETCDSEND,
    ZMAPWINDOW_CMD_SETCDSRANGE,
    ZMAPWINDOW_CMD_DELETEFROMSCRATCH,
    ZMAPWINDOW_CMD_CLEARSCRATCH,
    ZMAPWINDOW_CMD_UNDOSCRATCH,
    ZMAPWINDOW_CMD_REDOSCRATCH,
    ZMAPWINDOW_CMD_GETEVIDENCE
  } ZMapWindowCommandType ;


/* Controls mode of column compression: this removes or compresses columns to the width required
 * for the features within the whole sequence, the marked area or just the visible window. */
typedef enum
  {
    ZMAPWINDOW_COMPRESS_INVALID,
    ZMAPWINDOW_COMPRESS_VISIBLE,                            /* Compress for visible window only. */
    ZMAPWINDOW_COMPRESS_MARK,                               /* Compress for mark region. */
    ZMAPWINDOW_COMPRESS_ALL                                 /* Compress for whole sequence. */
  } ZMapWindowCompressMode ;



typedef struct
{
  ZMapWindowCommandType cmd ;

  gboolean result ;

} ZMapWindowCallbackCommandAnyStruct, *ZMapWindowCallbackCommandAny ;



/* Call an alignment display program for the given alignment feature. */
typedef enum
  {
    ZMAPWINDOW_ALIGNCMD_INVALID,
    ZMAPWINDOW_ALIGNCMD_NONE,       /* column with no aligns. */
    ZMAPWINDOW_ALIGNCMD_FEATURES,   /* all matches for selected features in this column. */
    ZMAPWINDOW_ALIGNCMD_EXPANDED,   /* selected features expanded into hidden underlying data */
    ZMAPWINDOW_ALIGNCMD_SET,        /* all matches for all features in this column. */
    ZMAPWINDOW_ALIGNCMD_MULTISET,   /* all matches for all features in the list of columns in the blixem config file. */
    ZMAPWINDOW_ALIGNCMD_SEQ         /* a coverage column: find the real data column */
  } ZMapWindowAlignSetType ;

typedef struct ZMapWindowCallbackCommandAlignStructName
{
  /* Common section. */
  ZMapWindowCommandType cmd ;
  gboolean result ;



  ZMapFeatureBlock block ;

  /* Align specific section. */
  ZMapHomolType homol_type ;                                /* DNA or Peptide alignments. */

  int offset ;                                              /* Offset for displaying coords with
                                                               different base. */

  int cursor_position ;                                     /* Where the alignment tools "cursor" should be. */

  int window_start, window_end ;                            /* Start/end coords that should be initially
                                                               visible in alignment tool.  */

  int mark_start, mark_end ;                                /* Optional range for alignment viewing. */


  ZMapWindowAlignSetType homol_set ;                        /* What features to display. */


  GList *features ;                                         /* Optional list of alignment features. */

  ZMapFeatureSet feature_set ;

  GList *source;                                     /* a list of featureset names */

  gboolean isSeq;

} ZMapWindowCallbackCommandAlignStruct, *ZMapWindowCallbackCommandAlign ;


/* Call sources to get new features. */
typedef struct
{
  /* Common section. */
  ZMapWindowCommandType cmd ;
  gboolean result ;


  ZMapFeatureBlock block ;                                  /* Block for which features should be fetched. */
  GList *feature_set_ids ;                                  /* List of names as quarks. */
  int start, end ;                                          /* Range over which features should be fetched. */

} ZMapWindowCallbackCommandGetFeaturesStruct, *ZMapWindowCallbackGetFeatures ;



/* Reverse complement features. */
typedef struct
{
  /* Common section. */
  ZMapWindowCommandType cmd ;
  gboolean result ;

  /* No extra data needed for rev. comp. */

} ZMapWindowCallbackCommandRevCompStruct, *ZMapWindowCallbackCommandRevComp ;



typedef void (*ZMapWindowGetEvidenceCB)(GList *evidence, gpointer user_data) ;

typedef struct ZMapWindowCallbackCommandScratchStructName
{
  /* Common section. */
  ZMapWindowCommandType cmd ;
  gboolean result ;

  ZMapFeatureBlock block ;

  /* Scratch specific section. */
  GList *features;      /* clicked feature(s) */
  long seq_start;       /* sequence coordinate that was clicked (sequence features only) */
  long seq_end;         /* sequence coordinate that was clicked */
  ZMapFeatureSubPart subpart; /* the subpart to use, if applicable */
  gboolean use_subfeature; /* if true, use the clicked subfeature; otherwise use the entire
                              feature */

  ZMapWindowGetEvidenceCB evidence_cb; /* callback to call for get-evidence */
  gpointer evidence_cb_data;
} ZMapWindowCallbackCommandScratchStruct, *ZMapWindowCallbackCommandScratch ;







/* Callback functions that can be registered with ZMapWindow, functions are registered all in one.
 * go via the ZMapWindowCallbacksStruct. */
typedef void (*ZMapWindowCallbackFunc)(ZMapWindow window, void *caller_data, void *window_data) ;
typedef gboolean (*ZMapWindowBoolCallbackFunc)(ZMapWindow window, void *caller_data, void *window_data) ;
typedef void (*ZMapWindowLoadCallbackFunc)(ZMapWindow window,
                                           void *caller_data, gpointer load_cb_data, void *window_data) ;


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
  ZMapWindowCallbackFunc command ;                          /* Request to exit given command. */
  ZMapWindowLoadCallbackFunc drawn_data ;
  ZMapWindowBoolCallbackFunc merge_new_feature ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* WHAT..... */
  ZMapRemoteAppMakeRequestFunc remote_request_func ;
  ZMapRemoteAppMakeRequestFunc remote_request_func_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  ZMapRemoteAppMakeRequestFunc remote_request_func ;
  gpointer remote_request_func_data ;

} ZMapWindowCallbacksStruct, *ZMapWindowCallbacks ;




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
  WINDOW_FOCUS_GROUP_MASKED,        /* use to store colours but not set as a focus type */
  WINDOW_FOCUS_GROUP_FILTERED,

  /* NOTE this must be the first blurred one */
#define N_FOCUS_GROUPS_FOCUS WINDOW_FOCUS_GROUP_MASKED

  N_FOCUS_GROUPS
} ZMapWindowFocusType;



/* all these comments imply that this stuff should NOT be in the public header.... */

#define WINDOW_FOCUS_GROUP_FOCUSSED 0x07        /* all the focus types */
#define WINDOW_FOCUS_GROUP_BLURRED      0xf8    /* all the blurred types */
#define WINDOW_FOCUS_GROUP_BITMASK      0xff    /* all the colour types, focussed and blurred */
#define WINDOW_FOCUS_DONT_USE   0xff00  /* see FEATURE_FOCUS in zmapWindowCanvasFeatureSet.c */
/* x-ref with zmapWindowCanvasFeatureset_I.h */

#define WINDOW_FOCUS_ID 0xffff0000

/* bitmap return values from the following function, _please_ don't enum them! */
#define WINDOW_FOCUS_CACHE_FILL                 1
#define WINDOW_FOCUS_CACHE_OUTLINE              2
#define WINDOW_FOCUS_CACHE_SELECTED             4


/* Does this need to be here ??????? */
extern int focus_group_mask[];  /* indexed by ZMapWindowFocusType */




void zMapWindowInit(ZMapWindowCallbacks callbacks) ;
ZMapWindow zMapWindowCreate(GtkWidget *parent_widget,
                            ZMapFeatureSequenceMap sequence, void *app_data,
                            GList *feature_set_names, gboolean *flags, int *int_values) ;
ZMapWindow zMapWindowCopy(GtkWidget *parent_widget, ZMapFeatureSequenceMap sequence,
                          void *app_data, ZMapWindow old,
                          ZMapFeatureContext features, GHashTable *all_styles, GHashTable *new_styles,
                          ZMapWindowLockType window_locking) ;

void zMapWindowBusyFull(ZMapWindow window, gboolean busy, const char *file, const char *func) ;

gboolean zMapWindowProcessRemoteRequest(ZMapWindow window,
                                        char *command_name, char *request,
                                        ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data) ;

gboolean zMapWindowExecuteCommand(ZMapWindow window, ZMapWindowCallbackCommandAny cmd_any, char **err_msg_out) ;

void zMapWindowSetCursor(ZMapWindow window, GdkCursor *cursor) ;

void zMapWindowDisplayData(ZMapWindow window, ZMapWindowState state,
                           ZMapFeatureContext current_features, ZMapFeatureContext new_features,
                           ZMapFeatureContextMap context_map,
                           GList *masked,
                           ZMapFeature highlight_feature,  gboolean splice_highlight,
                           gpointer loaded_cb_user_data) ;
void zMapWindowUnDisplayData(ZMapWindow window,
                             ZMapFeatureContext current_features,
                             ZMapFeatureContext new_features);
void zMapWindowUnDisplaySearchFeatureSets(ZMapWindow window,
                             ZMapFeatureContext current_features,
                             ZMapFeatureContext new_features);
void zMapWindowMove(ZMapWindow window, double start, double end) ;
void zMapWindowReset(ZMapWindow window) ;
void zMapWindowRedraw(ZMapWindow window) ;
void zMapWindowFeatureSaveState(ZMapWindow window, gboolean features_are_revcomped);
void zMapWindowFeatureReset(ZMapWindow window, gboolean features_are_revcomped);
void zMapWindowFeatureRedraw(ZMapWindow window, ZMapFeatureContext feature_context,
                             gboolean reversed) ;
void zMapWindowZoom(ZMapWindow window, double zoom_factor) ;
gboolean zMapWindowZoomFromClipboard(ZMapWindow window) ;
ZMapWindowZoomStatus zMapWindowGetZoomStatus(ZMapWindow window) ;
double zMapWindowGetZoomFactor(ZMapWindow window);
double zMapWindowGetZoomMin(ZMapWindow window) ;
double zMapWindowGetZoomMax(ZMapWindow window) ;
double zMapWindowGetZoomMagnification(ZMapWindow window);
double zMapWindowGetZoomMagAsBases(ZMapWindow window) ;
double zMapWindowGetZoomMaxDNAInWrappedColumn(ZMapWindow window);
gboolean zMapWindowItemGetSeqCoord(FooCanvasItem *item,
                                   gboolean set, double x, double y, long *seq_start, long *seq_end);
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

void zmapWindowColumnBumpRange(FooCanvasItem *bump_item,
                               ZMapStyleBumpMode bump_mode, ZMapWindowCompressMode compress_mode) ;

void zMapWindowRequestReposition(FooCanvasItem *foo);


gboolean zMapWindowGetDNAStatus(ZMapWindow window);
void zMapWindowStats(ZMapWindow window,GString *text) ;

void zMapWindow3FrameToggle(ZMapWindow window) ;
void zMapWindow3FrameSetMode(ZMapWindow window, ZMapWindow3FrameMode frame_mode) ;

/* should become internal to zmapwindow..... */
void zMapWindowToggleDNAProteinColumns(ZMapWindow window,
                                       GQuark align_id,   GQuark block_id,
                                       gboolean dna,      gboolean protein, gboolean trans,
                                       gboolean force_to, gboolean force);
void zMapWindowToggleScratchColumn(ZMapWindow window,
                                   GQuark align_id,   GQuark block_id,
                                   gboolean force_to, gboolean force);

void zMapWindowStateRecord(ZMapWindow window);
void zMapWindowBack(ZMapWindow window);
gboolean zMapWindowHasHistory(ZMapWindow window);
gboolean zMapWindowIsLocked(ZMapWindow window) ;
void zMapWindowSiblingWasRemoved(ZMapWindow window);        /* For when a window in the same view
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
gboolean zMapWindowFeatureRemove(ZMapWindow zmap_window,
                                 FooCanvasItem *feature_item, ZMapFeature feature, gboolean destroy_feature) ;

gboolean zMapWindowGetMaskedColour(ZMapWindow window,GdkColor **border,GdkColor **fill_col);
gboolean zMapWindowGetFilteredColour(ZMapWindow window, GdkColor **fill_col);


void zMapWindowScrollToWindowPos(ZMapWindow window, int window_y_pos) ;
gboolean zMapWindowCurrWindowPos(ZMapWindow window,
                                 double *x1_out, double *y1_out, double *x2_out, double *y2_out) ;
gboolean zMapWindowMaxWindowPos(ZMapWindow window,
                                double *x1_out, double *y1_out, double *x2_out, double *y2_out) ;
gboolean zMapWindowScrollToItem(ZMapWindow window, FooCanvasItem *feature_item) ;

gboolean zMapWindowFeatureSelect(ZMapWindow window, ZMapFeature feature) ;

void zMapWindowHighlightFeature(ZMapWindow window,
                                ZMapFeature feature, gboolean highlight_same_names, gboolean replace_flag);
gboolean zMapWindowUnhighlightFeature(ZMapWindow window, ZMapFeature feature) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
void zMapWindowHighlightObject(ZMapWindow window, FooCanvasItem *feature,
                               gboolean replace_highlight_item, gboolean highlight_same_names, gboolean sub_part) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
void zMapWindowHighlightObject(ZMapWindow window, FooCanvasItem *feature,
                               gboolean replace_highlight_item, gboolean highlight_same_names,
                               ZMapFeatureSubPart sub_part) ;

void zMapWindowHighlightObjects(ZMapWindow window, ZMapFeatureContext context, gboolean multiple_select);

void zmapWindowHighlightSequenceItem(ZMapWindow window, FooCanvasItem *item, int start, int end, int flanking);

char *zMapWindowGetHotColumnName(ZMapWindow window) ;

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


/* It's possible these are defunct now..... */
gboolean zMapWindowXRemoteRegister(ZMapWindow window) ;
char *zMapWindowRemoteReceiveAccepts(ZMapWindow window);
void zMapWindowSetupXRemote(ZMapWindow window, GtkWidget *widget);

char *zMapWindowGetSelectionText(ZMapWindow window, ZMapWindowDisplayStyle display_style) ;

ZMapGuiNotebookChapter zMapWindowGetConfigChapter(ZMapWindow window, ZMapGuiNotebook parent);

int zMapWindowFocusCacheGetSelectedColours(int id_flags,gulong *fill_col, gulong *outline);

void zMapWindowFocusCacheSetSelectedColours(ZMapWindow window);

gboolean zmapWindowFocusHasType(ZMapWindowFocus focus, ZMapWindowFocusType type);
gboolean zMapWindowFocusGetColour(ZMapWindow window,int mask, GdkColor *fill_col, GdkColor *border);

void zMapWindowUpdateColumnBackground(ZMapWindow window, ZMapFeatureSet feature_set, gboolean highlight_column_background);

GList* zMapWindowCanvasAlignmentGetAllMatchBlocks(FooCanvasItem *item) ;


gboolean zMapWindowExportFeatureSets(ZMapWindow window,
                                     GList* featuresets, gboolean marked_region, char **filepath_inout, GError **error) ;
gboolean zMapWindowExportFeatures(ZMapWindow window,
                                  gboolean all_features, gboolean marked_region, ZMapFeatureAny feature_in, char **filepath_inout, GError **error) ;
gboolean zMapWindowExportFASTA(ZMapWindow window, ZMapFeatureAny feature_in, GError **error) ;
gboolean zMapWindowExportContext(ZMapWindow window, GError **error) ;

void zMapWindowColumnHide(ZMapWindow window, GQuark column_id) ;
void zMapWindowColumnShow(ZMapWindow window, GQuark column_id) ;
double zMapWindowGetDisplayOrigin(ZMapWindow window) ;
void zMapWindowSetDisplayOrigin(ZMapWindow window, double origin) ;
void zMapWindowShowStyleDialog(ZMapWindow window, ZMapFeatureTypeStyle style, GQuark new_style_name) ;

/*
 * Set whether we are to display slice or chromosome coordinates.
 */
void zMapWindowToggleDisplayCoordinates(ZMapWindow window) ;
void zMapWindowSetDisplayCoordinatesSlice(ZMapWindow window) ;
void zMapWindowSetDisplayCoordinatesChrom(ZMapWindow window) ;
void zMapWindowSetDisplayCoordinates(ZMapWindow window, ZMapWindowDisplayCoordinates display_coordinates) ;
ZMapWindowDisplayCoordinates zMapWindowGetDisplayCoordinates(ZMapWindow window) ;

#endif /* !ZMAP_WINDOW_H */
