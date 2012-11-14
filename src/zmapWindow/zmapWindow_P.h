/*  Last edited: Jul 23 16:05 2012 (edgrif) */
/*  File: zmapWindow_P.h
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Defines internal interfaces/data structures of zMapWindow.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_P_H
#define ZMAP_WINDOW_P_H

#include <gtk/gtk.h>
#include <ZMap/zmapUtilsGUI.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapDraw.h>
#include <ZMap/zmapIO.h>
#include <ZMap/zmapWindow.h>
#include <zmapWindowMark_P.h>
#include <zmapWindowOverlays.h>
#include <zmapWindowContainerGroup.h>
#include <zmapWindowContainerUtils.h>


#include <zmapWindowCanvasFeatureset.h>


/* set a KNOWN initial size for the foo_canvas!
 * ... the same size as foo_canvas sets ...
 */
#define ZMAP_CANVAS_INIT_SIZE (100.0)


#define FEATURE_SIZE_REQUEST	0	// see item factory
#define USE_FACTORY	0

/*
 *  This section details data that we attach to the foocanvas items that represent
 *  contexts, aligns etc. Each data structure is accessed via a key given by the
 *  string in the hash define for each struct.
 */

/* Names for keys in G_OBJECTS */

#define ZMAP_WINDOW_POINTER       ZMAP_WINDOW_P_H "canvas_to_window"

#define ITEM_HIGHLIGHT_DATA  ZMAP_WINDOW_P_H "item_highlight_data"


/* All Align/Block/Column/Feature FooCanvas objects have their corresponding ZMapFeatureAny struct
 * attached to them via this key. */
#ifndef ITEM_FEATURE_DATA
#define ITEM_FEATURE_DATA         ZMAP_WINDOW_P_H "item_feature_data"
#endif /* !ITEM_FEATURE_DATA */


/* All Align/Block/Column/Feature FooCanvas containers have stats blocks attached to them
 * via this key, below structs show data collected for each. */
#define ITEM_FEATURE_STATS        ZMAP_WINDOW_P_H "item_feature_stats"


/* 3 frame mode flags and tests. */
typedef enum
  {
    DISPLAY_3FRAME_NONE     = 0x0U,			    /* No 3 frame at all. */
    DISPLAY_3FRAME_ON       = 0x1U,			    /* 3 frame display is on. */
    DISPLAY_3FRAME_TRANS    = 0x2U,			    /* Translations displayed when 3 frame on. */
    DISPLAY_3FRAME_TRANS_ON = 0x3U,
    DISPLAY_3FRAME_COLS     = 0x4U,			    /* All other cols displayed when 3 frame on. */
    DISPLAY_3FRAME_COLS_ON  = 0x5U,
    DISPLAY_3FRAME_ALL      = 0x6U,			    /* Everything on. */
    DISPLAY_3FRAME_ALL_ON   = 0x7U
  } Display3FrameMode ;

#define IS_3FRAME_TRANS(FRAME_MODE) \
  ((FRAME_MODE) & DISPLAY_3FRAME_TRANS)

#define IS_3FRAME_COLS(FRAME_MODE) \
  ((FRAME_MODE) & DISPLAY_3FRAME_COLS)

#define IS_3FRAME(FRAME_MODE) \
  ((FRAME_MODE) & DISPLAY_3FRAME_ON)




/* All feature stats structs must have these common fields at their start. */
typedef struct
{
  ZMapStyleMode feature_type ;

  GQuark style_id ;
  int num_children ;
  int num_items ;

} ZMapWindowStatsAnyStruct, *ZMapWindowStatsAny ;


typedef struct
{
  ZMapStyleMode feature_type ;

  ZMapFeatureTypeStyle style ;
  int features ;
  int items ;

} ZMapWindowStatsBasicStruct, *ZMapWindowStatsBasic ;


typedef struct
{
  ZMapStyleMode feature_type ;

  ZMapFeatureTypeStyle style ;
  int transcripts ;
  int items ;

  /* Transcript specific stats. */
  int exons ;
  int introns ;
  int cds ;

  int exon_boxes ;
  int intron_boxes ;
  int cds_boxes ;

} ZMapWindowStatsTranscriptStruct, *ZMapWindowStatsTranscript ;



/* Alignment stats. */
typedef struct
{
  ZMapStyleMode feature_type ;

  ZMapFeatureTypeStyle style ;
  int total_matches ;					    /* All matches drawn. */
  int items ;

  /* Alignment specific stats. */
  int ungapped_matches ;				    /* Ungapped matches drawn. */
  int gapped_matches ;					    /* Gapped matches drawn as gapped matches. */
  int not_perfect_gapped_matches ;			    /* Gapped matches drawn as ungapped because
							       failed quality check. */
  int total_boxes ;					    /* Total alignment boxes drawn. */
  int ungapped_boxes ;					    /* Ungapped boxes drawn. */
  int gapped_boxes ;					    /* Gapped boxes drawn. */
  int imperfect_boxes ;					    /* Gapped boxes _not_ drawn because of
							       quality check. */

} ZMapWindowStatsAlignStruct, *ZMapWindowStatsAlign ;


#define zmapWindowStatsAddBasic(STATS_PTR, FEATURE_PTR) \
  (ZMapWindowStatsBasic)zmapWindowStatsAddChild((STATS_PTR), (ZMapFeatureAny)(FEATURE_PTR))
#define zmapWindowStatsAddTranscript(STATS_PTR, FEATURE_PTR) \
  (ZMapWindowStatsTranscript)zmapWindowStatsAddChild((STATS_PTR), (ZMapFeatureAny)(FEATURE_PTR))
#define zmapWindowStatsAddAlign(STATS_PTR, FEATURE_PTR) \
  (ZMapWindowStatsAlign)zmapWindowStatsAddChild((STATS_PTR), (ZMapFeatureAny)(FEATURE_PTR))



/* the FtoIHash, search now returns these not the items */
/* We store ids with the group or item that represents them in the canvas.
 * May want to consider more efficient way of storing these than malloc... */
/* NOTE also used for user hidden items stack */
typedef struct
{
  FooCanvasItem *item ;					    /* could be group or item. */
  GHashTable *hash_table ;

  ZMapFeatureAny feature_any;
  	/* direct link to feature instead if via item */
  	/* need if we have composite items eg density plots */
} ID2CanvasStruct, *ID2Canvas ;


/* Callback for use in testing item hash objects to see if they fit a particular predicate. */
/* mh17: NOTE changed to take featre rather than item, the FToIHash now has both */
typedef gboolean (*ZMapWindowFToIPredFuncCB) (ZMapFeatureAny feature_any, gpointer user_data) ;

typedef struct
{
  gpointer search_function;	/* just a pointer to the function. _not_ usable as callback! */

  GQuark align_id;
  GQuark block_id;
  GQuark column_id;
  GQuark set_id;
  GQuark feature_id;

  char *strand_str;
  char *frame_str;

  ZMapWindowFToIPredFuncCB predicate_func;
  gpointer                 predicate_data;
  GDestroyNotify           predicate_free;
}ZMapWindowFToISetSearchDataStruct, *ZMapWindowFToISetSearchData;


/* Block data, this struct is attached to all FooCanvas block objects via ITEM_FEATURE_BLOCK_DATA key. */
#define ITEM_FEATURE_BLOCK_DATA     ZMAP_WINDOW_P_H "item_feature_block_data"


/* Feature set data, this struct is attached to all FooCanvas column objects via ITEM_FEATURE_SET_DATA key. */
#define ITEM_FEATURE_SET_DATA     ZMAP_WINDOW_P_H "item_feature_set_data"


/* Bump col data, this struct is attached to all extra bump col objects via ITEM_FEATURE_SET_DATA key. */
#define ITEM_FEATURE_BUMP_DATA     ZMAP_WINDOW_P_H "item_feature_bump_data"
typedef struct
{
  ZMapWindow window ;
  FooCanvasItem *first_item ;
  GQuark feature_id ;
  ZMapFeatureTypeStyle style ;
} ZMapWindowItemFeatureBumpDataStruct, *ZMapWindowItemFeatureBumpData ;


/* used by item factory for drawing features from one featureset */
typedef struct _zmapWindowFeatureStack
{
      ZMapFeatureContext context;
      ZMapFeatureAlignment align;
      ZMapFeatureBlock block;
      ZMapFeatureSet set;
      ZMapFeature feature;
      GQuark id;        /* used for density plots, set to zero */
      GQuark maps_to;	/* used by density plots for virtual featuresets */
      int set_index;	/* used by density plots for stagger */
      ZMapStrand strand;
      ZMapFrame frame;
	gboolean filter;	/* don't add to camvas if hidden */

	/* NOTE there's a struct defined privately in zmapWindowDrawFeatures.c that would be better having these
	 * as this is nominally related to the feature context not the camvas
	 * but as want to pass this data around eg to windowFeature.c it can't go there
	 * the idea here is to cache the column features group and column hash for each feature in the set
	 * instead of recalculating it for every feature
	 * these pointers also act as flags: they hold valid pointers if the frame/strand combination means draw the feature
	 */
#if ALL_FRAMES
// 3-frame gets drawn via 3 scans of the featureset
// it's a toss up whether this is fatser/slower than calculating where to send a feature out of 12 possible columns
// or to calculate only 3 and do it 3x
//      ZMapWindowContainerFeatures set_columns[N_FRAME * N_STRAND];	/* all possible strand and frame combinations */
//      GHashTable *col_hash[N_FRAME * N_STRAND];
#else
	// just handle the strand / frame conbo's is in the original code structre - set gets drawn 3x in 3 Frame mode
      ZMapWindowContainerFeatureSet set_column[N_STRAND_ALLOC];	/* all possible strand and frame combinations */
      ZMapWindowContainerFeatures set_features[N_STRAND_ALLOC];
	FooCanvasItem *col_featureset[N_STRAND_ALLOC];			/* the canvas featureset as allocated by the item factory */
	GHashTable *col_hash[N_STRAND_ALLOC];

#endif

} ZMapWindowFeatureStackStruct, *ZMapWindowFeatureStack;




/* parameters passed between the various functions drawing the features on the canvas, it's
 * simplest to cache the current stuff as we go otherwise the code becomes convoluted by having
 * to look up stuff all the time. */
typedef struct _ZMapCanvasDataStruct
{
  ZMapWindow window ;
  FooCanvas *canvas ;

  /* Records which alignment, block, set, type we are processing. */
  ZMapFeatureContext full_context ;
//  GHashTable *styles ;
  ZMapFeatureAlignment curr_alignment ;
  ZMapFeatureBlock curr_block ;
  ZMapFeatureSet curr_set ;

  /* THESE ARE REALLY NEEDED TO POSITION THE ALIGNMENTS FOR WHICH WE CURRENTLY HAVE NO
   * ORDERING/PLACEMENT MECHANISM.... */
  /* Records current positional information. */
//  double curr_x_offset ;
//  double curr_y_offset ;

  int feature_count;

  /* Records current canvas item groups, these are the direct parent groups of the display
   * types they contain, e.g. curr_root_group is the parent of the align */
  ZMapWindowContainerFeatures curr_root_group ;
  ZMapWindowContainerFeatures curr_align_group ;
  ZMapWindowContainerFeatures curr_block_group ;
#define curr_forward_group	curr_block_group
#define curr_reverse_group	curr_block_group

//  ZMapWindowContainerFeatures curr_forward_col ;
//  ZMapWindowContainerFeatures curr_reverse_col ;

  ZMapFeatureSet this_featureset_only;	/* when redrawing one featureset only */

  GHashTable *feature_hash ;

  GList *masked;


  /* frame specific display control. */
  gboolean    frame_mode_change ;
  ZMapFrame   current_frame;

  ZMapFeatureTypeStyle style;       /* for the column */

} ZMapCanvasDataStruct, *ZMapCanvasData ;



/* Item features are the canvas items that represent sequence features, they can be of various
 * types, in particular compound features such as transcripts require a parent, a bounding box
 * and children for features such as introns/exons. */

#define ITEM_FEATURE_ITEM_STYLE        ZMAP_WINDOW_P_H "item_feature_item_style"
#define ITEM_FEATURE_TYPE              ZMAP_WINDOW_P_H "item_feature_type"




/* Names of config stanzas. */
#define ZMAP_WINDOW_CONFIG "ZMapWindow"

#define DEFAULT_CURSOR "LEFT_PTR"
#define BUSY_CURSOR    "WATCH"


/* All settable from configuration file. */
#define ALIGN_SPACING        30.0
#define BLOCK_SPACING         5.0
#define STRAND_SPACING        7.0
#define COLUMN_SPACING        2.0
#define FEATURE_SPACING       1.0
#define FEATURE_LINE_WIDTH    0				    /* Special value meaning one pixel wide
							       lines with no aliasing. */

#define COLUMN_BACKGROUND_SPACING 2.0			    /* width of lines joining homol matches. */


/* Not settable, but could be */
#define ZMAP_WINDOW_FEATURE_ZOOM_BORDER 25


/* Min. distance in pixels user must move mouse button 1 to lasso, not settable but could be. */
#define ZMAP_WINDOW_MIN_LASSO 20.0



/* mh17: erm.... why aren't these #defines ?? */
enum
  {
//     ZMAP_WINDOW_TEXT_BORDER = 2, 			/* border above/below dna text.  */
// see zmapWindow.h
    ZMAP_WINDOW_STEP_INCREMENT = 10,                        /* scrollbar stepping increment */
    ZMAP_WINDOW_PAGE_INCREMENT = 600,                       /* scrollbar paging increment */
//    ZMAP_WINDOW_MAX_WINDOW = 30000			    /* Largest canvas window. */
// see zmapWindow.h
  } ;

enum
  {
    ZMAP_WINDOW_INTRON_POINTS = 3			    /* Number of points to draw an intron. */
  } ;

enum
  {
    ZMAP_WINDOW_GTK_BOX_SPACING = 5, /* Size in pixels for the GTK_BOX spacing between children */
    ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING = 10
  };

enum
  {
    ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH = 5
  };



/* Controls column configuration menu actions. */
typedef enum
  {
    ZMAPWINDOWCOLUMN_HIDE, ZMAPWINDOWCOLUMN_SHOW,
    ZMAPWINDOWCOLUMN_CONFIGURE, ZMAPWINDOWCOLUMN_CONFIGURE_ALL,
    ZMAPWINDOWCOLUMN_CONFIGURE_ALL_DEFERRED,
  } ZMapWindowColConfigureMode ;




/* Controls type of window list created. */
typedef enum
  {
    ZMAPWINDOWLIST_FEATURE_TREE,			    /* Show features as clickable "trees". */
    ZMAPWINDOWLIST_FEATURE_LIST,			    /* Show features as single lines. */
    ZMAPWINDOWLIST_DNA_LIST,				    /* Show dna matches as single lines. */
  } ZMapWindowListType ;


/* Controls what data is displayed in the feature list window. */

typedef enum
  {
    ZMAP_WINDOW_LIST_COL_NAME,				    /*!< feature name column  */
    ZMAP_WINDOW_LIST_COL_TYPE,				    /*!< feature type column  */
    ZMAP_WINDOW_LIST_COL_STRAND,			    /*!< feature strand  */
    ZMAP_WINDOW_LIST_COL_START,				    /*!< feature start */
    ZMAP_WINDOW_LIST_COL_END,				    /*!< feature end  */
    ZMAP_WINDOW_LIST_COL_QUERY_START,				    /*!< feature start */
    ZMAP_WINDOW_LIST_COL_QUERY_END,				    /*!< feature end  */
    ZMAP_WINDOW_LIST_COL_PHASE,				    /*!< feature phase  */
    ZMAP_WINDOW_LIST_COL_SCORE,				    /*!< feature score  */
    ZMAP_WINDOW_LIST_COL_FEATURE_TYPE,			    /*!< feature method  */
    ZMAP_WINDOW_LIST_COL_FEATURE,			    /*!< The feature !  */
    ZMAP_WINDOW_LIST_COL_SET_STRAND,			    /* The strand of the features parent set. */
    ZMAP_WINDOW_LIST_COL_SET_FRAME,			    /* The frame of the features parent set. */
    ZMAP_WINDOW_LIST_COL_SORT_TYPE,			    /*!< \                          */
    ZMAP_WINDOW_LIST_COL_SORT_STRAND,			    /*!< -- sort on integer columns */
    ZMAP_WINDOW_LIST_COL_SORT_PHASE,			    /*!< /                          */
    ZMAP_WINDOW_LIST_COL_NUMBER				    /*!< number of columns, must be last.  */
  } zmapWindowFeatureListColumn;

#define ZMAP_WINDOW_FEATURE_LIST_COL_NUMBER_KEY "column_number_data"



/* Controls what data is displayed in the dna list window. */
typedef enum
  {
    ZMAP_WINDOW_LIST_DNA_SCREEN_START,			    /* match screen start coord */
    ZMAP_WINDOW_LIST_DNA_SCREEN_END,			    /* match screen end coord */
    ZMAP_WINDOW_LIST_DNA_SCREEN_STRAND,			    /* match strand */
    ZMAP_WINDOW_LIST_DNA_SCREEN_FRAME,			    /* match frame */
    ZMAP_WINDOW_LIST_DNA_LENGTH,			    /* match length */
    ZMAP_WINDOW_LIST_DNA_MATCH,				    /* match */
    ZMAP_WINDOW_LIST_DNA_NOSHOW,			    /* cols after this are not displayed. */
    ZMAP_WINDOW_LIST_DNA_BLOCK = ZMAP_WINDOW_LIST_DNA_NOSHOW, /* block. */
    ZMAP_WINDOW_LIST_DNA_SEQTYPE,			    /* dna or peptide. */
    ZMAP_WINDOW_LIST_DNA_START,				    /* Actual match start coord. */
    ZMAP_WINDOW_LIST_DNA_END,				    /* Actual match end coord. */
    ZMAP_WINDOW_LIST_DNA_STRAND,			    /* Strand. */
    ZMAP_WINDOW_LIST_DNA_FRAME,				    /* frame. */
    ZMAP_WINDOW_LIST_DNA_NUMBER				    /* number of columns, must be last.  */
  } zmapWindowDNAListColumn ;

#define DNA_LIST_BLOCK_KEY "dna_list_block_data"


/* Used by menus to decide whether to display a "Show" or an "Export" button in dialogs. */
typedef enum
  {
    ZMAP_DIALOG_SHOW,
    ZMAP_DIALOG_EXPORT
  } ZMapWindowDialogType ;


/*
 *           Default colours.
 */

#define ZMAP_WINDOW_BACKGROUND_COLOUR "green"		    /* main canvas background */

#define ZMAP_WINDOW_BLOCK_BACKGROUND_COLOUR "white"	    /* main canvas background */

#define ZMAP_WINDOW_STRAND_DIVIDE_COLOUR "yellow"	    /* Marks boundary of forward/reverse strands. */

/* Colours for master alignment block (forward and reverse). */
#define ZMAP_WINDOW_MBLOCK_F_BG "white"
#define ZMAP_WINDOW_MBLOCK_R_BG "white"

/* Colours for query alignment block (forward and reverse). */
#define ZMAP_WINDOW_QBLOCK_F_BG "light pink"
#define ZMAP_WINDOW_QBLOCK_R_BG "pink"

/* Colour for highlighting a whole columns background. */
#define ZMAP_WINDOW_COLUMN_HIGHLIGHT "grey"
#define ZMAP_WINDOW_ITEM_HIGHLIGHT "dark grey"

/* Colours for highlighting 3 frame data */
#define ZMAP_WINDOW_FRAME_0 "red"
#define ZMAP_WINDOW_FRAME_1 "deep pink"
#define ZMAP_WINDOW_FRAME_2 "light pink"


/* Colour for "marked" item. */
#define ZMAP_WINDOW_ITEM_MARK "light blue"


/* Default colours for features. */
#define ZMAP_WINDOW_ITEM_FILL_COLOUR "white"
#define ZMAP_WINDOW_ITEM_BORDER_COLOUR "black"

/* default colours for related features */
#define ZMAP_WINDOW_ITEM_EVIDENCE_BORDER "black"
#define ZMAP_WINDOW_ITEM_EVIDENCE_FILL "yellow"


#define MH17_REVCOMP_DEBUG   0


/* Used to pass data to canvas item menu callbacks, whether columns or feature items. */
typedef struct
{
  double x, y ;						    /* Position of menu click. */

  gboolean item_cb ;					    /* TRUE => item callback,
							       FALSE => column callback. */

  ZMapWindow window ;

  /* this is sometimes a containerfeatureset
   * eg in zmapWindowDrawFeatures.c/columnMenuCB()
   */
  FooCanvasItem *item ;

  ZMapFeature feature;                    /* only used in item callbacks */
  ZMapFeatureSet feature_set ;            /* Only used in column callbacks... */
  GQuark req_id;					/* set to request if any */
  ZMapWindowContainerFeatureSet container_set;  /* we can get a/the featureset from this */
                                                /* be good to loose the featureset member */
                                                /* is this true: column contains mixed features */
  ZMapFeatureContextMap context_map ;	/* column to featureset mapping and other data. */


} ItemMenuCBDataStruct, *ItemMenuCBData ;




typedef void (*ZMapWindowScaleCanvasCallback)(gpointer data,
                                              gpointer user_data);

typedef struct _ZMapWindowScaleCanvasCallbackListStruct
{
  ZMapWindowScaleCanvasCallback paneResize;

  gpointer user_data;           /* Goes to all! */
} ZMapWindowScaleCanvasCallbackListStruct, *ZMapWindowScaleCanvasCallbackList;



typedef struct _ZMapWindowScaleCanvasStruct *ZMapWindowScaleCanvas;

typedef struct _ZMapWindowZoomControlStruct *ZMapWindowZoomControl ;



typedef struct _ZMapWindowFToIFactoryStruct *ZMapWindowFToIFactory ;

typedef struct _ZMapWindowStatsStruct *ZMapWindowStats ;


/* My intention is to gradually put all configuration data (spacing, borders, colours etc)
 * in this struct. */
typedef struct
{
  double align_spacing ;
  double block_spacing ;
#if USE_STRAND
  double strand_spacing ;
#endif
  double column_spacing ;
  double feature_spacing ;
  guint  feature_line_width ;				    /* n.b. line width is in pixels. */
} ZMapWindowConfigStruct, *ZMapWindowConfig ;




/* Represents a single sequence display window with its scrollbars, canvas and feature
 * display. */
typedef struct _ZMapWindowStruct
{

  ZMapWindowConfigStruct config ;			    /* Holds window configuration info. */


  /* this needs to go..... */
  ZMapWindowStats stats ;				    /* Holds statitics about no. of
							       features etc. */



  /* Widgets for displaying the data. */
  GtkWidget     *parent_widget ;
  GtkWidget     *toplevel ;
  GtkWidget     *scrolled_window ;
  GtkWidget     *pane;         /* points to toplevel */
  FooCanvas     *canvas ;				    /* where we paint the display */

  FooCanvasItem *rubberband;
  FooCanvasItem *horizon_guide_line;
  FooCanvasGroup *tooltip;
  FooCanvasItem  *mark_guide_line;

  gboolean xremote_client ;				    /* Is there a remote client ? */


  /* Handle cursor changes showing when zmap is busy. */
  GdkCursor *normal_cursor ;
  GdkCursor *busy_cursor ;
  GdkCursor *window_busy_cursor ;
  int cursor_busy_count ;


  ZMapWindowCallbacks caller_cbs ;			    /* table of callbacks registered by
							     * our caller. */

  /* We need to monitor changes to the size of the canvas window caused by user interactions
   * so we can adjust zoom and other controls appropriately. */
  gint window_width, window_height ;
  gint canvas_width, canvas_height ;


  /* Windows can be locked together in their zooming/scrolling. */
  gboolean locked_display ;				    /* Is this window locked ? */
  ZMapWindowLockType curr_locking ;			    /* Orientation of current locking. */
  GHashTable *sibling_locked_windows ;			    /* windows this window is locked with. */


  gboolean multi_select ;


  /* Some default colours, good for hiding boxes/lines. */
  GdkColor canvas_fill ;
  GdkColor canvas_border ;
  GdkColor canvas_background ;
  GdkColor align_background ;


  /* Detailed colours (NOTE...NEED MERGING WITH THE ABOVE.... */
  gboolean done_colours;
  GdkColor colour_root ;
  GdkColor colour_alignment ;
  GdkColor colour_block ;
  GdkColor colour_separator;
  GdkColor colour_mblock_for ;
  GdkColor colour_mblock_rev ;
  GdkColor colour_qblock_for ;
  GdkColor colour_qblock_rev ;
  GdkColor colour_mforward_col ;
  GdkColor colour_mreverse_col ;
  GdkColor colour_qforward_col ;
  GdkColor colour_qreverse_col ;

  GdkColor colour_frame_0 ;
  GdkColor colour_frame_1 ;
  GdkColor colour_frame_2 ;


  ZMapWindowZoomControl zoom;

  gboolean scroll_initialised;      /* have we ever set a scroll region?
                                     * used to control re-intialise eg on RevComp
                                     */

  /* Max canvas window size can either be set in pixels or in DNA bases, the latter overrides the
   * former. In either case the window cannot be more than 30,000 pixels (32k is the actual
   * X windows limit. */
  int canvas_maxwin_size ;
  int canvas_maxwin_bases ;


  /* I'm trying these out as using the sequence start/end is not correct for window stuff. */
  double min_coord ;					    /* min/max canvas coords */
  double max_coord ;

  double align_gap ;					    /* horizontal gap between alignments. */
  double column_gap ;					    /* horizontal gap between columns. */


  gboolean keep_empty_cols ;				    /* If TRUE then columns are displayed
							       even if they have no features. */


  GtkWidget     *text ;
  GdkAtom        zmap_atom ;
  void          *app_data ;
  gulong         exposeHandlerCB ;


  /* The length, start and end of the segment of sequence to be shown, there will be _no_
   * features outside of the start/end. */
  double         seqLength;
  ZMapFeatureSequenceMap sequence;  /* has name start and end */

  ZMapFeatureContext feature_context ;			    /* Currently displayed features. */

/*  GHashTable *read_only_styles ;				     Original styles list from server. */
/*  GHashTable *display_styles ;				     Styles used for current display. */

  ZMapFeatureContext strand_separator_context ; /* context to display non-feature context "features" with. */

  ZMapFeatureContextMap context_map ;      /* all the data for mapping featuresets styles and columns */

  GList *feature_set_names ;				    /* Gives names/order of columns to be displayed. */
/*  GHashTable *featureset_2_styles ;			     Links column names to the styles
							       for that column. */

/*  GHashTable *featureset_2_column ;        Mapping of a feature source to a column using ZMapGFFSet
                                           * NB: this contains data from ZMap config
                                           * sections [columns] [Column_description] _and_ ACEDB
                                           */
/*  GHashTable *columns;                     the display columns. These are not featuresets */

  GHashTable *context_to_item ;	      /* Links parts of a feature context to
							       the canvas groups/items that
							       represent those parts. */


  ZMapWindowContainerGroup feature_root_group ;	            /* The root of our features. (ZMapWindowContainerContext) */

#if USE_FACTORY
 ZMapWindowFToIFactory item_factory;
#endif

  /* Lists of dialog windows associated with this zmap window, these must be destroyed when
   * the zmap window is destroyed. */
  GPtrArray *feature_show_windows ;			    /* feature display windows. */

  GPtrArray *featureListWindows ;			    /* popup windows showing lists of
							       column features. */

  GPtrArray *search_windows ;				    /* popup search windows. */

  GPtrArray *dna_windows ;				    /* popup dna search windows. */

  GPtrArray *dnalist_windows ;				    /* popup showing list of dna match locations. */

  gpointer style_window;					/* set colour popup, one per window */
									/* is a struct defined in zmapWindowStyle,c */
  gboolean edit_styles;

  gboolean edittable_features ;				    /* FALSE means no features are edittable. */
  gboolean reuse_edit_window ;				    /* TRUE means reuse existing window
							       for new selected feature. */

  GtkWidget *col_config_window ;			    /* column configuration window. */


  ZMapWindowScaleCanvas ruler ;

  /* Holds focus items/column for the zmap. */
  ZMapWindowFocus focus ;

  /* Highlighting colours for items/columns. The item colour is used only if a select colour
   * was not specificed in the features style. */
  struct
  {
    unsigned int item : 1 ;
    unsigned int column : 1 ;
    unsigned int evidence : 1 ;
    unsigned int masked: 1;
    unsigned int filtered: 1;

  } highlights_set ;

  GdkColor colour_item_highlight ;
  GdkColor colour_column_highlight ;

  GdkColor colour_evidence_border ;
  GdkColor colour_evidence_fill ;

  GdkColor colour_masked_feature_fill ;  /* masked features normally not displayed but this is optional */
  GdkColor colour_masked_feature_border ;  /* if not set display as normal */

  GdkColor colour_filtered_column ; 	 /* to highlight the filter widget */

/* Holds the marked region or item. */
  ZMapWindowMark mark ;

  ZMapWindowStateQueue history ;

  ZMapWindowState state;	/* need to store this see revcomp, RT 229703 */

  /* We need to be able to find out if the user has done a revcomp for coordinate display
   * and other reasons. */
  gboolean revcomped_features ;

  /* The display_forward_coords flag controls whether coords are displayed
   * always as if for the original forward strand or for the whichever is the current forward
   * strand. origin is used to transform coords for revcomp if display_forward_coords == TRUE */
  gboolean display_forward_coords ;
/*  int origin ; */


  /* 3 frame display:
   * Use IS_3FRAME_XXXX macros to test these flags for whether 3 frame mode
   * is on and what is currently displayed. etc etc. */
  Display3FrameMode display_3_frame ;
  gboolean show_3_frame_reverse ;			  /* 3 frame displayed on reverse col ? */

  gboolean interrupt_expose;

} ZMapWindowStruct ;


/* NEEDS TO GO......IS IT EVEN USED.... */
/* Used in our event communication.... */
#define ZMAP_ATOM  "ZMap_Atom"


/* Used to pass data via a "send event" to the canvas window. */
typedef struct
{
  ZMapWindow window ;
  void *data ;
} zmapWindowDataStruct, *zmapWindowData ;

typedef struct
{
  char    *location;
  char    *cookie_jar;
  char    *mode;
  int      port;
  gboolean full_record;
} PFetchUserPrefsStruct;

typedef struct
{
  ZMapFeatureSet       feature_set;
  ZMapFeatureTypeStyle feature_style;
} zmapWindowFeatureSetStyleStruct, *zmapWindowFeatureSetStyle;


/* Represents a feature display window. */
typedef struct ZMapWindowFeatureShowStruct_ *ZMapWindowFeatureShow ;



typedef struct _zmapWindowFeatureListCallbacksStruct
{
  GCallback columnClickedCB;
  GCallback rowActivatedCB;
  GtkTreeSelectionFunc selectionFuncCB;
} zmapWindowFeatureListCallbacksStruct, *zmapWindowFeatureListCallbacks;


typedef void (*ZMapWindowStyleTableCallback)(ZMapFeatureTypeStyle style, gpointer user_data) ;

typedef GHashTable * (*ZMapWindowListGetFToIHash)(gpointer user_data);
typedef GList * (*ZMapWindowListSearchHashFunc)(ZMapWindow widnow, GHashTable *hash_table, gpointer user_data);



ZMapWindowCallbacks zmapWindowGetCBs() ;

void zmapWindowZoom(ZMapWindow window, double zoom_factor, gboolean stay_centered);

GtkWidget *zmapWindowMakeMenuBar(ZMapWindow window) ;
GtkWidget *zmapWindowMakeButtons(ZMapWindow window) ;
GtkWidget *zmapWindowMakeFrame(ZMapWindow window) ;

void zmapWindowPrintCanvas(FooCanvas *canvas) ;
void zmapWindowPrintGroups(FooCanvas *canvas) ;
void zmapWindowPrintItem(FooCanvasGroup *item) ;
void zmapWindowPrintLocalCoords(char *msg_prefix, FooCanvasItem *item) ;

void zmapWindowShowItem(FooCanvasItem *item) ;


void zmapWindowListWindowCreate(ZMapWindow                   window,
				FooCanvasItem               *current_item,
				char                        *title,
				ZMapWindowListGetFToIHash    get_hash_func,
				gpointer                     get_hash_data,
				ZMapFeatureContextMap context_map,
				ZMapWindowListSearchHashFunc search_hash_func,
				gpointer                     search_hash_data,
				GDestroyNotify               search_hash_free,
				gboolean                     zoom_to_item);
void zmapWindowListWindow(ZMapWindow                   window,
			  FooCanvasItem               *current_item,
			  char                        *title,
			  ZMapWindowListGetFToIHash    get_hash_func,
			  gpointer                     get_hash_data,
			  ZMapFeatureContextMap context_map,
			  ZMapWindowListSearchHashFunc search_hash_func,
			  gpointer                     search_hash_data,
			  GDestroyNotify               search_hash_free,
			  gboolean                     zoom_to_item);
void zmapWindowListWindowReread(GtkWidget *window_list_widget) ;

void zmapWindowCreateSearchWindow(ZMapWindow zmapWindow,
				  ZMapWindowListGetFToIHash get_hash_func,
				  gpointer get_hash_data,
				  ZMapFeatureContextMap context_map,
				  FooCanvasItem *feature_item) ;
void zmapWindowCreateSequenceSearchWindow(ZMapWindow window, FooCanvasItem *feature_item,
					  ZMapSequenceType sequence_type) ;
void zmapWindowDNAListCreate(ZMapWindow zmapWindow, GList *dna_list,
			     char *ref_seq_name, char *match_sequence, char *match_details,
			     ZMapFeatureBlock block) ;
char *zmapWindowDNAChoose(ZMapWindow window, FooCanvasItem *feature_item, ZMapWindowDialogType dialog_type,
			  int *sequence_start_out, int *sequence_end_out) ;


void zmapWindowResetWidth(ZMapWindow window) ;

double zmapWindowCalcZoomFactor (ZMapWindow window);
void   zmapWindowSetPageIncr    (ZMapWindow window);


void zmapWindowDrawFeatures(ZMapWindow window,
			    ZMapFeatureContext current_context, ZMapFeatureContext new_context, GList *masked) ;
void zmapWindowreDrawContainerExecute(ZMapWindow                 window,
				      ZMapContainerUtilsExecFunc enter_cb,
				      gpointer                   enter_data);

gboolean zmapWindowDumpFile(ZMapWindow window, char *filename) ;


void zmapWindowDrawSetGroupBackground(ZMapWindowContainerGroup group, int start, int end, double width, gint layer, GdkColor *fill, GdkColor *border);
ZMapWindowContainerGroup zmapWindowContainerGroupCreateWithBackground(FooCanvasGroup        *parent,
							       ZMapContainerLevelType level,
							       double                 child_spacing,
							       GdkColor              *background_fill_colour,
							       GdkColor              *background_border_colour);


int zmapWindowCoordToDisplay(ZMapWindow window, int coord) ;
void zmapWindowCoordPairToDisplay(ZMapWindow window,
				  int start_in, int end_in,
				  int *display_start_out, int *display_end_out) ;
int zmapWindowCoordFromDisplay(ZMapWindow window, int coord) ;
int zmapWindowCoordFromOriginRaw(int start,int end, int coord, gboolean revcomped) ;
ZMapStrand zmapWindowStrandToDisplay(ZMapWindow window, ZMapStrand strand_in) ;
int zmapWindowWorldToSequenceForward(ZMapWindow window, int coord);

double zmapWindowExt(double start, double end) ;
void zmapWindowSeq2CanExt(double *start_inout, double *end_inout) ;
void zmapWindowExt2Zero(double *start_inout, double *end_inout) ;
void zmapWindowSeq2CanExtZero(double *start_inout, double *end_inout) ;
void zmapWindowSeq2CanOffset(double *start_inout, double *end_inout, double offset) ;

GQuark zMapWindowGetFeaturesetContainerID(ZMapWindow window,GQuark featureset_id);

FooCanvasItem *zmapWindowFToIFactoryRunSingle(GHashTable *ftoi_hash,
							    ZMapWindowContainerFeatureSet parent_container,
							    ZMapWindowContainerFeatures features_container,
							    FooCanvasItem * foo_featureset,
                                              ZMapWindowFeatureStack     feature_stack);

GHashTable *zmapWindowFToICreate(void) ;
gboolean zmapWindowFToIAddRoot(GHashTable *feature_to_context_hash, FooCanvasGroup *root_group) ;
gboolean zmapWindowFToIRemoveRoot(GHashTable *feature_to_context_hash) ;
gboolean zmapWindowFToIAddAlign(GHashTable *feature_to_context_hash,
				GQuark align_id, FooCanvasGroup *align_group) ;
gboolean zmapWindowFToIRemoveAlign(GHashTable *feature_to_context_hash,
				   GQuark align_id) ;
gboolean zmapWindowFToIAddBlock(GHashTable *feature_to_context_hash,
				GQuark align_id, GQuark block_id, FooCanvasGroup *block_group) ;
gboolean zmapWindowFToIRemoveBlock(GHashTable *feature_to_context_hash,
				   GQuark align_id, GQuark block_id) ;
gboolean zmapWindowFToIAddSet(GHashTable *feature_to_context_hash,
			      GQuark align_id, GQuark block_id, GQuark set_id,
			      ZMapStrand strand, ZMapFrame frame,
#if FEATURESET_AS_COLUMN
			      FooCanvasGroup *set_group) ;
#else
			      FooCanvasItem *set_item) ;
#endif

gboolean zmapWindowFToIRemoveSet(GHashTable *feature_to_context_hash,
				 GQuark align_id, GQuark block_id, GQuark set_id,
				 ZMapStrand strand, ZMapFrame frame, gboolean remove_features) ;
GHashTable *zmapWindowFToIGetSetHash(GHashTable *feature_context_to_item,
				  GQuark align_id, GQuark block_id,
				  GQuark set_id, ZMapStrand set_strand, ZMapFrame set_frame);
gboolean zmapWindowFToIAddSetFeature(GHashTable *set,	GQuark feature_id, FooCanvasItem *feature_item, ZMapFeature feature);
gboolean zmapWindowFToIAddFeature(GHashTable *feature_to_context_hash,
				  GQuark align_id, GQuark block_id,
				  GQuark set_id, ZMapStrand set_strand, ZMapFrame set_frame,
				  GQuark feature_id, FooCanvasItem *feature_item, ZMapFeature feature) ;
gboolean zmapWindowFToIRemoveFeature(GHashTable *feature_to_context_hash,
				     ZMapStrand set_strand, ZMapFrame set_frame, ZMapFeature feature) ;
ID2Canvas zmapWindowFToIFindID2CFull(ZMapWindow window, GHashTable *feature_context_to_item,
					  GQuark align_id, GQuark block_id,
					  GQuark set_id,
					  ZMapStrand set_strand, ZMapFrame set_frame,
					  GQuark feature_id);
FooCanvasItem *zmapWindowFToIFindItemFull(ZMapWindow window,GHashTable *feature_to_context_hash,
					  GQuark align_id, GQuark block_id, GQuark set_id,
					  ZMapStrand strand, ZMapFrame frame,
					  GQuark feature_id) ;
FooCanvasItem *zmapWindowFToIFindItemColumn(ZMapWindow window, GHashTable *feature_context_to_item,
					  GQuark align_id, GQuark block_id,
					  GQuark set_id,
					  ZMapStrand set_strand, ZMapFrame set_frame);
GList *zmapWindowFToIFindItemSetFull(ZMapWindow window,GHashTable *feature_to_context_hash,
				     GQuark align_id, GQuark block_id, GQuark column_id, GQuark set_id,
				     char *strand_spec, char *frame_spec,
				     GQuark feature_id,
				     ZMapWindowFToIPredFuncCB pred_func, gpointer user_data) ;
GList *zmapWindowFToIFindSameNameItems(ZMapWindow window,GHashTable *feature_to_context_hash,
				       char *set_strand, char *set_frame, ZMapFeature feature) ;

ZMapWindowFToISetSearchData zmapWindowFToISetSearchCreateFull(gpointer    search_function,
							      ZMapFeature feature,
							      GQuark      align_id,
							      GQuark      block_id,
							      GQuark column_id,
							      GQuark      set_id,
							      GQuark      feature_id,
							      char       *strand_str,
							      char       *frame_str,
							      ZMapWindowFToIPredFuncCB predicate_func,
							      gpointer                 predicate_data,
							      GDestroyNotify           predicate_free);
ZMapWindowFToISetSearchData zmapWindowFToISetSearchCreate(gpointer    search_function,
							  ZMapFeature feature,
							  GQuark      align_id,
							  GQuark      block_id,
							  GQuark      column_id,
							  GQuark      set_id,
							  GQuark      feature_id,
							  char       *strand_str,
							  char       *frame_str);
GList *zmapWindowFToISetSearchPerform(ZMapWindow window,GHashTable *feature_to_context_hash,
				      ZMapWindowFToISetSearchData search_data);
void zmapWindowFToISetSearchDestroy(ZMapWindowFToISetSearchData search_data);

FooCanvasItem *zmapWindowFToIFindSetItem(ZMapWindow window,GHashTable *feature_to_context_hash,
					 ZMapFeatureSet feature_set,
					 ZMapStrand strand, ZMapFrame frame) ;
FooCanvasItem *zmapWindowFToIFindFeatureItem(ZMapWindow window,GHashTable *feature_to_context_hash,
                               ZMapStrand set_strand, ZMapFrame set_frame, ZMapFeature feature) ;
FooCanvasItem *zmapWindowFToIFindItemChild(ZMapWindow window,GHashTable *feature_to_context_hash,
					   ZMapStrand set_strand, ZMapFrame set_frame,
					   ZMapFeature feature, int child_start, int child_end) ;
FooCanvasItem *zMapWindowFindFeatureItemByItem(ZMapWindow window, FooCanvasItem *item) ;
void zmapWindowFToIDestroy(GHashTable *feature_to_item_hash) ;

#if USE_FACTORY
void zmapWindowFeatureFactoryInit(ZMapWindow window);
void zmapWindowFToIFactoryClose(ZMapWindowFToIFactory factory) ;
#endif

void zmapWindowZoomToItem(ZMapWindow window, FooCanvasItem *item) ;
void zmapWindowZoomToItems(ZMapWindow window, GList *items) ;
void zmapWindowZoomToWorldPosition(ZMapWindow window, gboolean border,
				   double rootx1, double rooty1, double rootx2, double rooty2) ;
void zmapWindowGetMaxBoundsItems(ZMapWindow window, GList *items,
				 double *rootx1, double *rooty1, double *rootx2, double *rooty2) ;

gboolean zmapWindowItemIsCompound(FooCanvasItem *item) ;
FooCanvasItem *zmapWindowItemGetTrueItem(FooCanvasItem *item) ;
FooCanvasItem *zmapWindowItemGetNthChild(FooCanvasGroup *compound_item, int child_index) ;
FooCanvasGroup *zmapWindowItemGetParentContainer(FooCanvasItem *feature_item) ;

FooCanvasItem *zmapWindowItemGetDNATextItem(ZMapWindow window, FooCanvasItem *item);
FooCanvasGroup *zmapWindowItemGetTranslationColumnFromBlock(ZMapWindow window, ZMapFeatureBlock block);

void zmapWindowHighlightSequenceItems(ZMapWindow window, FooCanvasItem *item) ;
void zmapWindowHighlightSequenceRegion(ZMapWindow window, ZMapFeatureBlock block,
				       ZMapSequenceType seq_type, ZMapFrame frame, int start, int end,
				       gboolean centre_on_region) ;
void zmapWindowItemHighlightDNARegion(ZMapWindow window, gboolean item_highlight, gboolean sub_feature,
				      FooCanvasItem *any_item, ZMapFrame required_frame,
				      ZMapSequenceType coords_type, int region_start, int region_end) ;
void zmapWindowItemUnHighlightDNA(ZMapWindow window, FooCanvasItem *item) ;
void zmapWindowItemHighlightTranslationRegions(ZMapWindow window, gboolean item_highlight, gboolean sub_feature,
					       FooCanvasItem *item,
					       ZMapFrame required_frame,
					       ZMapSequenceType coords_type, int region_start, int region_end) ;
void zmapWindowItemUnHighlightTranslations(ZMapWindow window, FooCanvasItem *item) ;
void zmapWindowItemHighlightShowTranslationRegion(ZMapWindow window, gboolean item_highlight, gboolean sub_feature,
						  FooCanvasItem *item,
						  ZMapFrame required_frame,
						  ZMapSequenceType coords_type,
						  int region_start, int region_end) ;
void zmapWindowItemUnHighlightShowTranslations(ZMapWindow window, FooCanvasItem *item) ;



void zmapWindowCallBlixem(ZMapWindow window, FooCanvasItem *item,
			  ZMapWindowAlignSetType requested_homol_set,
			  ZMapFeatureSet feature_set, GList *source,
			  double x_pos, double y_pos) ;

ZMapFeatureBlock zmapWindowItemGetFeatureBlock(FooCanvasItem *item) ;
ZMapFeature zmapWindowItemGetFeature(FooCanvasItem *item) ;
ZMapFeatureAny zmapWindowItemGetFeatureAny(FooCanvasItem *item) ;
ZMapFeatureAny zmapWindowItemGetFeatureAnyType(FooCanvasItem *item, ZMapFeatureStructType expected_type) ;

FooCanvasItem *zmapWindowItemGetShowTranslationColumn(ZMapWindow window, FooCanvasItem *item) ;
void zmapWindowItemShowTranslation(ZMapWindow window, FooCanvasItem *feature_to_translate) ;
void zmapWindowItemShowTranslationRemove(ZMapWindow window, FooCanvasItem *feature_item);

ZMapFeatureTypeStyle zmapWindowItemGetStyle(FooCanvasItem *feature_item) ;
void zmapWindowRaiseItem(FooCanvasItem *item) ;
GList *zmapWindowItemSortByPostion(GList *feature_item_list) ;
void zmapWindowSortCanvasItems(FooCanvasGroup *group) ;
FooCanvasGroup *zmapWindowFeatureItemsMakeGroup(ZMapWindow window, GList *feature_items) ;
gboolean zmapWindowItemGetStrandFrame(FooCanvasItem *item, ZMapStrand *set_strand, ZMapFrame *set_frame) ;
void zmapWindowPrintItemCoords(FooCanvasItem *item) ;
char *zmapWindowItemCoordsText(FooCanvasItem *item) ;

gboolean zmapWindowWorld2SeqCoords(ZMapWindow window, FooCanvasItem *foo,
				   double wx1, double wy1, double wx2, double wy2,
				   FooCanvasGroup **block_grp_out, int *y1_out, int *y2_out) ;
gboolean zmapWindowItem2SeqCoords(FooCanvasItem *item, int *y1, int *y2) ;

void zmapWindowItemGetVisibleWorld(ZMapWindow window, double *wx1, double *wy1, double *wx2, double *wy2);

/* These should be in a separate file...and merged into foocanvas.... */
void my_foo_canvas_item_w2i(FooCanvasItem *item, double *x, double *y) ;
void my_foo_canvas_item_i2w(FooCanvasItem *item, double *x, double *y) ;
void my_foo_canvas_item_goto(FooCanvasItem *item, double *x, double *y) ;
void my_foo_canvas_item_get_world_bounds(FooCanvasItem *item,
					 double *rootx1_out, double *rooty1_out,
					 double *rootx2_out, double *rooty2_out) ;
void my_foo_canvas_world_bounds_to_item(FooCanvasItem *item,
					double *rootx1_inout, double *rooty1_inout,
					double *rootx2_inout, double *rooty2_inout) ;
void my_foo_canvas_item_lower_to(FooCanvasItem *item, int position) ;


void zmapWindowPrintW2I(FooCanvasItem *item, char *text, double x1, double y1) ;
void zmapWindowPrintI2W(FooCanvasItem *item, char *text, double x1, double y1) ;

void zmapWindowGetScrollRegion(ZMapWindow window,
			       double *x1_inout, double *y1_inout,
			       double *x2_inout, double *y2_inout);
void zmapWindowSetScrollRegion(ZMapWindow window,
			       double *x1_inout, double *y1_inout,
			       double *x2_inout, double *y2_inout,char *where);
ZMapGUIClampType zmapWindowClampSpan(ZMapWindow window,
                                     double *top_inout,
                                     double *bot_inout) ;
ZMapGUIClampType zmapWindowClampedAtStartEnd(ZMapWindow window,
                                             double *top_inout,
                                             double *bot_inout) ;

void zMapWindowMoveItem(ZMapWindow window, ZMapFeature origFeature,
			ZMapFeature modFeature,  FooCanvasItem *item);
void zMapWindowMoveSubFeatures(ZMapWindow window,
			       ZMapFeature originalFeature,
			       ZMapFeature modifiedFeature,
			       GArray *origArray, GArray *modArray,
			       gboolean isExon);

void zmapWindowUpdateInfoPanel(ZMapWindow window, ZMapFeature feature,
			       GList *feature_list, FooCanvasItem *item,  ZMapFeatureSubPartSpan sub_feature,
			       int sub_item_dna_start, int sub_item_dna_end,
			       int sub_item_coords_start, int sub_item_coords_end,
			       char *alternative_clipboard_text,
			       gboolean replace_highlight_item, gboolean highlight_same_names, gboolean sub_part) ;

void zmapWindowDrawZoom(ZMapWindow window) ;
void zmapWindowDrawManageWindowWidth(ZMapWindow window);

void zmapWindowColumnConfigure(ZMapWindow window, FooCanvasGroup *column_group,
			       ZMapWindowColConfigureMode configure_mode) ;
void zmapWindowColumnConfigureDestroy(ZMapWindow window) ;

void zmapWindowColumnsCompress(FooCanvasItem *column_item, ZMapWindow window, ZMapWindowCompressMode compress_mode) ;
void zmapWindowContainerBlockAddCompressedColumn(ZMapWindowContainerBlock block_data, FooCanvasGroup *container) ;
gboolean zmapWindowContainerBlockIsCompressedColumn(ZMapWindowContainerBlock block_data) ;
GList *zmapWindowContainerBlockRemoveCompressedColumns(ZMapWindowContainerBlock block_data) ;

void zmapWindowColumnBump(FooCanvasItem *bump_item, ZMapStyleBumpMode bump_mode) ;
void zmapWindowContainerBlockAddBumpedColumn(ZMapWindowContainerBlock block_data, FooCanvasGroup *container) ;
gboolean zmapWindowContainerBlockIsBumpedColumn(ZMapWindowContainerBlock block_data) ;
GList *zmapWindowContainerBlockRemoveBumpedColumns(ZMapWindowContainerBlock block_data) ;

void zmapWindowColumnBumpAllInitial(FooCanvasItem *column_item);
void zmapWindowColumnUnbumpAll(FooCanvasItem *column_item);
void zmapWindowColumnWriteDNA(ZMapWindow window, FooCanvasGroup *column_parent);
void zmapWindowColumnHide(FooCanvasGroup *column_group) ;
void zmapWindowColumnShow(FooCanvasGroup *column_group) ;
gboolean zmapWindowColumnIsVisible(ZMapWindow window, FooCanvasGroup *col_group) ;

gboolean zmapWindowColumnBoundingBoxEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data);

gboolean zmapWindowColumnIs3frameVisible(ZMapWindow window, FooCanvasGroup *col_group) ;
gboolean zmapWindowColumnIs3frameDisplayed(ZMapWindow window, FooCanvasGroup *col_group) ;
void zmapWindowDrawRemove3FrameFeatures(ZMapWindow window) ;
void zmapWindowDraw3FrameFeatures(ZMapWindow window) ;

gboolean zmapWindowColumnIsMagVisible(ZMapWindow window, FooCanvasGroup *col_group) ;
void zmapWindowColumnSetMagState(ZMapWindow window, FooCanvasGroup *col_group) ;
void zmapMakeColumnMenu(GdkEventButton *button_event, ZMapWindow window, FooCanvasItem *item,
			ZMapWindowContainerFeatureSet container, ZMapFeatureTypeStyle style) ;

gboolean zmapWindowGetColumnVisibility(ZMapWindow window,FooCanvasGroup *column_group);

void zmapWindowColumnSetState(ZMapWindow window, FooCanvasGroup *column_group,
			      ZMapStyleColumnDisplayState new_col_state, gboolean redraw_if_required) ;
void zmapWindowGetPosFromScore(ZMapFeatureTypeStyle style, double score,
			       double *curr_x1_inout, double *curr_x2_out) ;
void zmapWindowFreeWindowArray(GPtrArray **window_array_inout, gboolean free_array) ;
ZMapWindowFeatureShow zmapWindowFeatureShowCreate(ZMapWindow zmapWindow, FooCanvasItem *item) ;
ZMapWindowFeatureShow zmapWindowFeatureShow(ZMapWindow zmapWindow, FooCanvasItem *item) ;

GList *zmapWindowFeatureGetEvidence(ZMapWindow window,ZMapFeature feature);

/* summarise busy column by not displaying invisible features */
gboolean zmapWindowContainerSummariseIsItemVisible(ZMapWindow window, double dx1,double dy1,double dx2, double dy2);
void zMapWindowContainerSummariseClear(ZMapWindow window,ZMapFeatureSet fset);
gboolean zMapWindowContainerSummarise(ZMapWindow window,ZMapFeatureTypeStyle style);
GList *zMapWindowContainerSummariseSortFeatureSet(ZMapFeatureSet fset);

ZMapFeatureTypeStyle zMapWindowContainerFeatureSetGetStyle(ZMapWindowContainerFeatureSet container);
gboolean zMapWindowContainerFeatureSetSetBumpMode(ZMapWindowContainerFeatureSet container_set, ZMapStyleBumpMode bump_mode);

GtkTreeModel *zmapWindowFeatureListCreateStore(ZMapWindowListType list_type) ;
GtkWidget    *zmapWindowFeatureListCreateView(ZMapWindowListType list_type, GtkTreeModel *treeModel,
                                              GtkCellRenderer *renderer,
                                              zmapWindowFeatureListCallbacks callbacks,
                                              gpointer user_data);
void zmapWindowFeatureListPopulateStoreList(GtkTreeModel *treeModel, ZMapWindowListType list_type,
                                            GList *list, gpointer user_data);
void zmapWindowFeatureListRereadStoreList(GtkTreeView *tree_view, ZMapWindow window) ;
gint zmapWindowFeatureListCountSelected(GtkTreeSelection *selection);
gint zmapWindowFeatureListGetColNumberFromTVC(GtkTreeViewColumn *col);


void zmapMakeItemMenu(GdkEventButton *button_event, ZMapWindow window, FooCanvasItem *item) ;

ZMapGUIMenuItem zmapWindowMakeMenuSearchListOps(int *start_index_inout,
						ZMapGUIMenuItemCallbackFunc callback_func,
						gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuBlixemBAM(int *start_index_inout,
					    ZMapGUIMenuItemCallbackFunc callback_func,
					    gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuRequestBAM(int *start_index_inout,
					     ZMapGUIMenuItemCallbackFunc callback_func,
					     gpointer callback_data) ;

ZMapGUIMenuItem zmapWindowMakeMenuBump(int *start_index_inout,
				       ZMapGUIMenuItemCallbackFunc callback_func,
				       gpointer callback_data, ZMapStyleBumpMode curr_bump_mode) ;

ZMapGUIMenuItem zmapWindowMakeMenuShowTop(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuShowFeature(int *start_index_inout,
					      ZMapGUIMenuItemCallbackFunc callback_func,
					      gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuDNAFeatureAny(int *start_index_inout,
						ZMapGUIMenuItemCallbackFunc callback_func,
						gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuDNAFeatureAnyFile(int *start_index_inout,
						    ZMapGUIMenuItemCallbackFunc callback_func,
						    gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuDNATranscript(int *start_index_inout,
						ZMapGUIMenuItemCallbackFunc callback_func,
						gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuDNATranscriptFile(int *start_index_inout,
						    ZMapGUIMenuItemCallbackFunc callback_func,
						    gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuPeptide(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuPeptideFile(int *start_index_inout,
					      ZMapGUIMenuItemCallbackFunc callback_func,
					      gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuExportOps(int *start_index_inout,
					    ZMapGUIMenuItemCallbackFunc callback_func,
					    gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuDeveloperOps(int *start_index_inout,
					       ZMapGUIMenuItemCallbackFunc callback_func,
					       gpointer callback_data) ;

ZMapGUIMenuItem zmapWindowMakeMenuBlixTop(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuBlixCommon(int *start_index_inout,
					     ZMapGUIMenuItemCallbackFunc callback_func,
					     gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuBlixColCommon(int *start_index_inout,
						ZMapGUIMenuItemCallbackFunc callback_func,
						gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuNonHomolFeature(int *start_index_inout,
						  ZMapGUIMenuItemCallbackFunc callback_func,
						  gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuDNAHomol(int *start_index_inout,
					   ZMapGUIMenuItemCallbackFunc callback_func,
					   gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuDNAHomolFeature(int *start_index_inout,
						  ZMapGUIMenuItemCallbackFunc callback_func,
						  gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuProteinHomol(int *start_index_inout,
					       ZMapGUIMenuItemCallbackFunc callback_func,
					       gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuProteinHomolFeature(int *start_index_inout,
						      ZMapGUIMenuItemCallbackFunc callback_func,
						      gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuTranscriptTools(int *start_index_inout,
                                                  ZMapGUIMenuItemCallbackFunc callback_func,
                                                  gpointer callback_data);
ZMapGUIMenuItem zmapWindowMakeMenuMarkExportOps(int *start_index_inout,
						ZMapGUIMenuItemCallbackFunc callback_func,
						gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuFeatureOps(int *start_index_inout,
					     ZMapGUIMenuItemCallbackFunc callback_func,
					     gpointer callback_data) ;

void zmapWindowFeatureExpand(ZMapWindow window, FooCanvasItem *foo,
			     ZMapFeature feature, ZMapWindowContainerFeatureSet container_set) ;
void zmapWindowFeatureContract(ZMapWindow window, FooCanvasItem *foo,
			       ZMapFeature feature, ZMapWindowContainerFeatureSet container_set) ;

ZMapXRemoteSendCommandError zmapWindowUpdateXRemoteData(ZMapWindow window,
							ZMapFeatureAny feature_any,
							char *action,
							FooCanvasItem *real_item);
ZMapXRemoteSendCommandError zmapWindowUpdateXRemoteDataFull(ZMapWindow window, ZMapFeatureAny feature_any,
							    char *action, FooCanvasItem *real_item,
							    ZMapXMLObjTagFunctions start_handlers,
							    ZMapXMLObjTagFunctions end_handlers,
							    gpointer handler_data) ;

void zmapWindowShowStyleDialog( ItemMenuCBData menu_data );
void zmapWindowMenuSetStyleCB(int menu_item_id, gpointer callback_data);
gboolean zmapWindowSetStyleFeatureset(ZMapWindow window, FooCanvasItem *foo, ZMapFeature feature);
void zmapStyleWindowDestroy(ZMapWindow window);


/* ================= in zmapWindowZoomControl.c ========================= */
ZMapWindowZoomControl zmapWindowZoomControlCreate(ZMapWindow window) ;
void zmapWindowZoomControlInitialise(ZMapWindow window) ;
gboolean zmapWindowZoomControlZoomByFactor(ZMapWindow window, double factor);
void zmapWindowZoomControlHandleResize(ZMapWindow window);
double zmapWindowZoomControlLimitSpan(ZMapWindow window, double y1, double y2) ;
void zmapWindowZoomControlCopyTo(ZMapWindowZoomControl orig, ZMapWindowZoomControl new) ;
void zmapWindowZoomControlGetScrollRegion(ZMapWindow window,
                                          double *x1_out, double *y1_out,
                                          double *x2_out, double *y2_out);


void zmapWindowBusyInternal(ZMapWindow window,  gboolean external_call,
			    gboolean busy, const char *file, const char *func) ;
#ifdef __GNUC__
#define zmapWindowBusy(WINDOW, BUSY)         \
  zmapWindowBusyInternal((WINDOW), FALSE, (BUSY), __FILE__, (char *)__PRETTY_FUNCTION__)
#else
#define zmapWindowBusy(WINDOW, BUSY)         \
  zmapWindowBusyInternal((WINDOW),  FALSE, (BUSY), __FILE__, NULL)
#endif


void zmapGetFeatureStack(ZMapWindowFeatureStack feature_stack,ZMapFeatureSet feature_set, ZMapFeature feature, ZMapFrame frame);


ZMapStrand zmapWindowFeatureStrand(ZMapWindow window, ZMapFeature feature) ;
ZMapFrame zmapWindowFeatureFrame(ZMapFeature feature) ;

FooCanvasItem *zmapWindowFeatureDraw(ZMapWindow window, ZMapFeatureTypeStyle style,
				     	ZMapWindowContainerFeatureSet set_group,
				      ZMapWindowContainerFeatures set_features,
					FooCanvasItem *foo_featureset,
					ZMapWindowFeatureStack feature_stack) ;

void zmapWindowRedrawFeatureSet(ZMapWindow window, ZMapFeatureSet featureset);

char *zmapWindowFeatureSetDescription(ZMapFeatureSet feature_set) ;
char *zmapWindowFeatureSourceDescription(ZMapFeature feature) ;
void zmapWindowFeatureGetSetTxt(ZMapFeature feature, char **set_name_out, char **set_description_out) ;
void zmapWindowFeatureGetSourceTxt(ZMapFeature feature, char **source_name_out, char **source_description_out) ;
char *zmapWindowFeatureDescription(ZMapFeature feature) ;

char *zmapWindowFeatureTranscriptFASTA(ZMapFeature feature, gboolean spliced, gboolean cds_only);
/*
void zmapWindowzoomControlClampSpan(ZMapWindow window, double *top_inout, double *bot_inout) ;
*/
void zmapWindowDebugWindowCopy(ZMapWindow window);
void zmapWindowGetBorderSize(ZMapWindow window, double *border);
/* End of zmapWindowZoomControl.c functions */

double zMapWindowDrawScaleBar(FooCanvasGroup *group, int scroll_start, int scroll_end,
	int seq_start, int seq_end, double zoom_factor, gboolean revcomped, gboolean zoomed);

gboolean zmapWindowItemIsVisible(FooCanvasItem *item) ;
gboolean zmapWindowItemIsShown(FooCanvasItem *item) ;
void zmapWindowItemCentreOnItem(ZMapWindow window, FooCanvasItem *item,
                                gboolean alterScrollRegionSize,
                                double boundaryAroundItem) ;
void zmapWindowItemCentreOnItemSubPart(ZMapWindow window, FooCanvasItem *item,
				       gboolean alterScrollRegionSize,
				       double boundaryAroundItem,
				       double sub_start, double sub_end) ;

gboolean zmapWindowItemIsOnScreen(ZMapWindow window, FooCanvasItem *item, gboolean completely) ;
void zmapWindowScrollToItem(ZMapWindow window, FooCanvasItem *item) ;




/*!-------------------------------------------------------------------!
 *| The following functions maintain the list of focus items          |
 *| (window->focusItemSet). Just to explain this list should be the   |
 *| highlighted items (rev-video) on the canvas.  This seems          |
 *| simple. We also need to know which item is the current or most    |
 *| recent item.  In order to support this I'm assuming that the first|
 *| item in the list is the most recent.  I call this the "hot" item, |
 *| like a hot potato.                                                |
 *!-------------------------------------------------------------------!*/
// also used for evidence highlight lists

ZMapWindowFocus zmapWindowFocusCreate(ZMapWindow window) ;
void zmapWindowFocusAddItemType(ZMapWindowFocus focus, FooCanvasItem *item,ZMapFeature feature, ZMapWindowFocusType type);
#define zmapWindowFocusAddItem(focus, item_list, feature) \
      zmapWindowFocusAddItemType(focus, item_list, feature, WINDOW_FOCUS_GROUP_FOCUS);
void zmapWindowFocusAddItemsType(ZMapWindowFocus focus, GList *list, FooCanvasItem *hot,ZMapWindowFocusType type);
#define zmapWindowFocusAddItems(focus, item_list, hot) \
      zmapWindowFocusAddItemsType(focus, item_list, hot, WINDOW_FOCUS_GROUP_FOCUS);
void zmapWindowFocusForEachFocusItemType(ZMapWindowFocus focus, ZMapWindowFocusType type, GFunc callback, gpointer user_data) ;
void zmapWindowFocusResetType(ZMapWindowFocus focus, ZMapWindowFocusType type);
void zmapWindowFocusReset(ZMapWindowFocus focus) ;
void zmapWindowFocusRemoveFocusItemType(ZMapWindowFocus focus,
					FooCanvasItem *item, ZMapWindowFocusType type, gboolean unhighlight);

#define zmapWindowFocusRemoveFocusItem(focus, item) \
  zmapWindowFocusRemoveFocusItemType(focus, item, WINDOW_FOCUS_GROUP_FOCUS, TRUE)
#define zmapWindowFocusRemoveOnlyFocusItem(focus, item) \
  zmapWindowFocusRemoveFocusItemType(focus, item, WINDOW_FOCUS_GROUP_FOCUS, FALSE)


void zmapWindowFocusSetHotItem(ZMapWindowFocus focus, FooCanvasItem *item, ZMapFeature feature) ;
FooCanvasItem *zmapWindowFocusGetHotItem(ZMapWindowFocus focus) ;
GList *zmapWindowFocusGetFocusItemsType(ZMapWindowFocus focus, ZMapWindowFocusType type) ;
#define zmapWindowFocusGetFocusItems(focus) \
    zmapWindowFocusGetFocusItemsType(focus, WINDOW_FOCUS_GROUP_FOCUS)
gboolean zmapWindowFocusIsItemInHotColumn(ZMapWindowFocus focus, FooCanvasItem *item) ;
void zmapWindowFocusSetHotColumn(ZMapWindowFocus focus, FooCanvasGroup *column, FooCanvasItem *item) ;
FooCanvasGroup *zmapWindowFocusGetHotColumn(ZMapWindowFocus focus) ;
void zmapWindowFocusDestroy(ZMapWindowFocus focus) ;

void zmapWindowFocusMaskOverlay(ZMapWindowFocus focus, FooCanvasItem *item, GdkColor *highlight);
void zmapWindowFocusAddOverlayManager(ZMapWindowFocus focus, ZMapWindowOverlay overlay);
void zmapWindowFocusRemoveOverlayManager(ZMapWindowFocus focus, ZMapWindowOverlay overlay);
void zmapWindowFocusClearOverlayManagers(ZMapWindowFocus focus);


void zmapWindowFocusHideFocusItems(ZMapWindowFocus focus, GList **hidden_items);
void zmapWindowFocusRehighlightFocusItems(ZMapWindowFocus focus, ZMapWindow window);
void zmapWindowFocusHighlightFocusItems(ZMapWindowFocus focus, ZMapWindow window);
void zmapWindowFocusUnhighlightFocusItems(ZMapWindowFocus focus, ZMapWindow window);

void zmapWindowReFocusHighlights(ZMapWindow window);


GList *zmapWindowItemListToFeatureList(GList *item_list);
GList *zmapWindowItemListToFeatureListExpanded(GList *item_list, int expand);
int zmapWindowItemListStartCoord(GList *item_list);


void zmapWindowHighlightObject(ZMapWindow window, FooCanvasItem *item,
			       gboolean replace_highlight_item, gboolean highlight_same_names, gboolean sub_part) ;
void zmapWindowFocusHighlightHotColumn(ZMapWindowFocus focus) ;
void zmapWindowFocusUnHighlightHotColumn(ZMapWindowFocus focus) ;

void zmapWindowHighlightFocusItems(ZMapWindow window) ;
void zmapWindowUnHighlightFocusItems(ZMapWindow window) ;

void zmapWindowMarkItem(ZMapWindow window, FooCanvasItem *item, gboolean mark) ;

#ifdef RDS_BREAKING_STUFF
ZMapWindowItemHighlighter zmapWindowItemTextHighlightCreateData(FooCanvasGroup *group);
ZMapWindowItemHighlighter zmapWindowItemTextHighlightRetrieve(FooCanvasGroup *group);
gboolean zmapWindowItemTextHighlightGetIndices(ZMapWindowItemHighlighter select_control,
                                               int *firstIdx, int *lastIdx);
void zmapWindowItemTextHighlightFinish(ZMapWindowItemHighlighter select_control);
gboolean zmapWindowItemTextHighlightValidForMotion(ZMapWindowItemHighlighter select_control);
void zmapWindowItemTextHighlightDraw(ZMapWindowItemHighlighter select_control,
                                     FooCanvasItem *item_receiving_event);
gboolean zmapWindowItemTextHighlightBegin(ZMapWindowItemHighlighter select_control,
                                          FooCanvasGroup *group_under_control,
                                          FooCanvasItem *item_receiving_event);
void zmapWindowItemTextHighlightUpdateCoords(ZMapWindowItemHighlighter select_control,
                                             double event_x_coord,
                                             double event_y_coord);
void zmapWindowItemTextHighlightRegion(ZMapWindowItemHighlighter select_control,
                                       FooCanvasItem *feature_parent,
                                       int firstIdx, int lastIdx);
void zmapWindowItemTextHighlightSetFullText(ZMapWindowItemHighlighter select_control,
                                            char *text_string, gboolean copy_string);
char *zmapWindowItemTextHighlightGetFullText(ZMapWindowItemHighlighter select_control);
void zmapWindowItemTextHighlightReset(ZMapWindowItemHighlighter select_control);
#endif
gboolean zmapWindowCreateSetColumns(ZMapWindow window,
                                    ZMapWindowContainerFeatures forward_strand_group,
                                    ZMapWindowContainerFeatures reverse_strand_group,
                                    ZMapFeatureBlock block,
                                    ZMapFeatureSet feature_set,
                                    ZMapFrame frame,
                                    FooCanvasGroup **forward_col_out,
                                    FooCanvasGroup **reverse_col_out,
				    FooCanvasGroup **separator_col_out);
int zmapWindowDrawFeatureSet(ZMapWindow window,
//			      GHashTable *styles,
                              ZMapFeatureSet feature_set,
                              FooCanvasGroup *forward_col,
                              FooCanvasGroup *reverse_col,
                              ZMapFrame frame, gboolean frame_mode_change) ;

void zMapWindowDrawContext(ZMapCanvasData     canvas_data,
			      ZMapFeatureContext full_context,
			      ZMapFeatureContext diff_context,
			      GList *masked);

void zmapWindowRemoveEmptyColumns(ZMapWindow window,
				  FooCanvasGroup *forward_group, FooCanvasGroup *reverse_group) ;
gboolean zmapWindowRemoveIfEmptyCol(FooCanvasGroup **col_group) ;

void zmapWindowDrawSeparatorFeatures(ZMapWindow           window,
				     ZMapFeatureBlock     block,
				     ZMapFeatureSet       feature_set,
				     ZMapFeatureTypeStyle style);

gboolean zmapWindowUpdateStyles(ZMapWindow window, GHashTable *read_only_styles, GHashTable *display_styles) ;
gboolean zmapWindowGetMarkedSequenceRangeFwd(ZMapWindow       window,
					     ZMapFeatureBlock block,
					     int *start, int *end);

GHashTable *zmapWindowStyleTableCreate(void) ;
ZMapFeatureTypeStyle zmapWindowStyleTableAddCopy(GHashTable *style_table, ZMapFeatureTypeStyle new_style) ;
ZMapFeatureTypeStyle zmapWindowStyleTableFind(GHashTable *style_table, GQuark style_id) ;
void zmapWindowStyleTableForEach(GHashTable *style_table,
				 ZMapWindowStyleTableCallback app_func, gpointer app_data) ;
void zmapWindowStyleTableDestroy(GHashTable *style_table) ;

ZMapFeatureColumn zMapWindowGetColumn(ZMapFeatureContextMap map,GQuark col_id);
ZMapFeatureColumn zMapWindowGetSetColumn(ZMapFeatureContextMap map,GQuark set_id);
ZMapFeatureTypeStyle zMapWindowGetSetColumnStyle(ZMapWindow window,GQuark set_id);
ZMapFeatureTypeStyle zMapWindowGetColumnStyle(ZMapWindow window,GQuark col_id);

GList *zmapWindowFeatureColumnStyles(ZMapFeatureContextMap map, GQuark column_id);
//GList *zmapWindowFeatureSetStyles(ZMapWindow window, GHashTable *all_styles, GQuark feature_set_id);



/* Ruler Functions */
ZMapWindowScaleCanvas zmapWindowScaleCanvasCreate(ZMapWindowScaleCanvasCallbackList callbacks);
void zmapWindowScaleCanvasInit(ZMapWindowScaleCanvas obj,
                               GtkWidget *paned,
                               GtkAdjustment *vadjustment);
void zmapWindowScaleCanvasMaximise(ZMapWindowScaleCanvas obj, double y1, double y2);
void zmapWindowScaleCanvasOpenAndMaximise(ZMapWindowScaleCanvas obj);
void zmapWindowScaleCanvasSetRevComped(ZMapWindowScaleCanvas obj, gboolean revcomped) ;
void zmapWindowScaleCanvasSetSpan(ZMapWindowScaleCanvas ruler, int start,int end);
gboolean zmapWindowScaleCanvasDraw(ZMapWindowScaleCanvas obj, int x, int y,int seq_start, int seq_end);
void zmapWindowScaleCanvasSetVAdjustment(ZMapWindowScaleCanvas obj, GtkAdjustment *vadjustment);
void zmapWindowScaleCanvasSetPixelsPerUnit(ZMapWindowScaleCanvas obj, double x, double y);
void zmapWindowScaleCanvasSetLineHeight(ZMapWindowScaleCanvas obj, double border);
//void zmapWindowScaleGroupDraw(FooCanvasGroup *parent, gboolean revcomped,double start, double end, double canvas_offset);

/* Stats functions. */
ZMapWindowStats zmapWindowStatsCreate(ZMapFeatureAny feature_any ) ;
ZMapWindowStatsAny zmapWindowStatsAddChild(ZMapWindowStats stats, ZMapFeatureAny feature_any) ;
void zmapWindowStatsReset(ZMapWindowStats stats) ;
void zmapWindowStatsPrint(GString *text, ZMapWindowStats stats) ;
void zmapWindowStatsDestroy(ZMapWindowStats stats) ;



void zmapWindowShowStyle(ZMapFeatureTypeStyle style) ;

char *zmapWindowGetDialogText(ZMapWindowDialogType dialog_type) ;
void zmapWindowToggleMark(ZMapWindow window, gboolean whole_feature);

void zmapWindowColOrderColumns(ZMapWindow window);
void zmapWindowColOrderPositionColumns(ZMapWindow window);

void zmapWindowFullReposition(ZMapWindowContainerGroup root) ;



void zmapWindowItemDebugItemToString(GString *string, FooCanvasItem *item);

void zmapWindowPfetchEntry(ZMapWindow window, char *sequence_name) ;

gboolean zmapWindowGetPFetchUserPrefs(char *config_file, PFetchUserPrefsStruct *pfetch);

void zmapWindowFetchData(ZMapWindow window, ZMapFeatureBlock block, GList *column_name_list, gboolean use_mark,gboolean is_column);

void zmapWindowStateRevCompRegion(ZMapWindow window, double *a, double *b);

void zmapWindowStateRevCompRegion(ZMapWindow window, double *a, double *b);

#endif /* !ZMAP_WINDOW_P_H */
