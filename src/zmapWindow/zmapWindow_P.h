/*  File: zmapWindow_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Defines internal interfaces/data structures of zMapWindow.
 *              
 * HISTORY:
 * Last edited: Sep 27 17:09 2007 (edgrif)
 * Created: Fri Aug  1 16:45:58 2003 (edgrif)
 * CVS info:   $Id: zmapWindow_P.h,v 1.195 2007-10-04 10:01:51 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_P_H
#define ZMAP_WINDOW_P_H

#include <gtk/gtk.h>
#include <ZMap/zmapUtilsGUI.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapDraw.h>
#include <ZMap/zmapWindow.h>
#include <zmapWindowOverlays.h>
#include <zmapWindowTextPositioner.h>

/* 
 *  This section details data that we attacht to the foocanvas items that represent
 *  contexts, aligns etc. Each data structure is accessed via a key given by the
 *  string in the hash define for each struct.
 */


/* Names for keys in G_OBJECTS */


#define ZMAP_WINDOW_POINTER       ZMAP_WINDOW_P_H "canvas_to_window"

#define ITEM_HIGHLIGHT_DATA  ZMAP_WINDOW_P_H "item_highlight_data"


/* All Align/Block/Column/Feature FooCanvas objects have their corresponding ZMapFeatureAny struct
 * attached to them via this key. */
#define ITEM_FEATURE_DATA         ZMAP_WINDOW_P_H "item_feature_data"


/* Feature set data, this struct is attached to all FooCanvas column objects via ITEM_FEATURE_SET_DATA key. */
#define ITEM_FEATURE_SET_DATA     ZMAP_WINDOW_P_H "item_feature_set_data"
typedef struct
{
  ZMapWindow window ;
  ZMapStrand strand ;
  ZMapFrame frame ;
  ZMapFeatureTypeStyle style ;
  GHashTable *style_table ;

  /* We keep the features sorted by position and size so we can cursor through them... */
  gboolean sorted ;
  
  /* Features hidden by user, should stay hidden. */
  GQueue *user_hidden_stack ;

  /* These fields are used for some of the more exotic column bumping. */
  gboolean hidden_bump_features ;			    /* Features were hidden because they
							       are out of the marked range. */

  GList *extra_items ;					    /* Match backgrounds etc. */

  GList *gaps_added_items ;				    /* List of features where gap data was added. */

} ZMapWindowItemFeatureSetDataStruct, *ZMapWindowItemFeatureSetData ;



/* Bump col data, this struct is attached to all extra bump col objects via ITEM_FEATURE_SET_DATA key. */
#define ITEM_FEATURE_BUMP_DATA     ZMAP_WINDOW_P_H "item_feature_bump_data"
typedef struct
{
  ZMapWindow window ;
  FooCanvasItem *first_item ;
  GQuark feature_id ;
  ZMapFeatureTypeStyle style ;
} ZMapWindowItemFeatureBumpDataStruct, *ZMapWindowItemFeatureBumpData ;




/* Item features are the canvas items that represent sequence features, they can be of various
 * types, in particular compound features such as transcripts require a parent, a bounding box
 * and children for features such as introns/exons. */

#define ITEM_FEATURE_ITEM_STYLE        ZMAP_WINDOW_P_H "item_feature_item_style"
#define ITEM_FEATURE_TYPE              ZMAP_WINDOW_P_H "item_feature_type"
#define ITEM_SUBFEATURE_DATA           ZMAP_WINDOW_P_H "item_subfeature_data"

#define ITEM_FEATURE_TEXT_DATA         ZMAP_WINDOW_P_H "item_feature_text_data"

typedef enum
  {
    ITEM_FEATURE_INVALID,

    /* Item is a group of features. */
    ITEM_FEATURE_GROUP,					    /* Parent group for features. */
    ITEM_FEATURE_GROUP_BACKGROUND,			    /* background for group. */

    /* Item is the whole feature. */
    ITEM_FEATURE_SIMPLE,

    /* Item is a compound feature composed of subparts, e.g. exons, introns etc. */
    ITEM_FEATURE_PARENT,				    /* Item is parent group of compound feature. */
    ITEM_FEATURE_CHILD,					    /* Item is child/subpart of feature. */
    ITEM_FEATURE_BOUNDING_BOX				    /* Item is invisible bounding box of
							       feature or subpart of feature.  */
  } ZMapWindowItemFeatureType ;



/* Names of config stanzas. */
#define ZMAP_WINDOW_CONFIG "ZMapWindow"


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

enum
  {
    ZMAP_WINDOW_TEXT_BORDER = 2,			    /* border above/below dna text. */
    ZMAP_WINDOW_STEP_INCREMENT = 10,                        /* scrollbar stepping increment */
    ZMAP_WINDOW_PAGE_INCREMENT = 600,                       /* scrollbar paging increment */
    ZMAP_WINDOW_MAX_WINDOW = 30000			    /* Largest canvas window. */
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
    ZMAPWWINDOWCOLUMN_HIDE, ZMAPWWINDOWCOLUMN_SHOW,
    ZMAPWWINDOWCOLUMN_CONFIGURE, ZMAPWWINDOWCOLUMN_CONFIGURE_ALL
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

#define ZMAP_WINDOW_BACKGROUND_COLOUR "white"		    /* main canvas background */

#define ZMAP_WINDOW_STRAND_DIVIDE_COLOUR "yellow"	    /* Marks boundary of forward/reverse
							       strands. */

/* Colours for master alignment block (forward and reverse). */
#define ZMAP_WINDOW_MBLOCK_F_BG "white"
#define ZMAP_WINDOW_MBLOCK_R_BG "white"

/* Colours for query alignment block (forward and reverse). */
#define ZMAP_WINDOW_QBLOCK_F_BG "light pink"
#define ZMAP_WINDOW_QBLOCK_R_BG "pink"

/* Colour for highlighting a whole columns background. */
#define ZMAP_WINDOW_COLUMN_HIGHLIGHT "grey"

/* Colours for highlighting 3 frame data */
#define ZMAP_WINDOW_FRAME_0 "red"
#define ZMAP_WINDOW_FRAME_1 "deep pink"
#define ZMAP_WINDOW_FRAME_2 "light pink"


/* Colour for "marked" item. */
#define ZMAP_WINDOW_ITEM_MARK "light blue"


/* Default colours for features. */
#define ZMAP_WINDOW_ITEM_FILL_COLOUR "white"
#define ZMAP_WINDOW_ITEM_BORDER_COLOUR "black"


/* Colours for matches to indicate degrees of colinearity. */
#define ZMAP_WINDOW_MATCH_PERFECT       "green"
#define ZMAP_WINDOW_MATCH_COLINEAR      "orange"
#define ZMAP_WINDOW_MATCH_NOTCOLINEAR   "red"


/* Used to pass data to canvas item menu callbacks, whether columns or feature items. */
typedef struct
{
  gboolean item_cb ;					    /* TRUE => item callback,
							       FALSE => column callback. */

  ZMapWindow window ;
  FooCanvasItem *item ;

  ZMapFeatureSet feature_set ;				    /* Only used in column callbacks... */
} ItemMenuCBDataStruct, *ItemMenuCBData ;



/* Probably will need to expand this to be a union as we come across more features that need
 * different information recording about them.
 * The intent of this structure is to hold information for items that are subparts of features,
 * e.g. exons, where it is tedious/error prone to recover the information from the feature and
 * canvas item. */
typedef struct
{

  /* I'm not completely sure this is a good idea but somehow we do need to be able to find out
   * whether something is in intron/exon or whatever.... */
  ZMapFeatureSubpartType subpart ;			    /* Exon, Intron etc. */

  int start, end ;					    /* start/end of subpart in sequence coords. */

  int query_start, query_end ;				    /* For alignments. */

  FooCanvasItem *twin_item ;				    /* Some features need to be drawn with
							       two canvas items, an example is
							       introns which need an invisible
							       bounding box for sensible user
							       interaction. */

} ZMapWindowItemFeatureStruct, *ZMapWindowItemFeature ;



typedef void (*ZMapWindowRulerCanvasCallback)(gpointer data,
                                              gpointer user_data);

typedef struct _ZMapWindowRulerCanvasCallbackListStruct
{
  ZMapWindowRulerCanvasCallback paneResize;
  
  gpointer user_data;           /* Goes to all! */
} ZMapWindowRulerCanvasCallbackListStruct, *ZMapWindowRulerCanvasCallbackList;



typedef struct _ZMapWindowRulerCanvasStruct *ZMapWindowRulerCanvas;

typedef struct _ZMapWindowZoomControlStruct *ZMapWindowZoomControl ;

typedef struct _ZMapWindowFocusStruct *ZMapWindowFocus ;

typedef struct _ZMapWindowMarkStruct *ZMapWindowMark ;

typedef struct _ZMapWindowLongItemsStruct *ZMapWindowLongItems ;

typedef struct _ZMapWindowFToIFactoryStruct *ZMapWindowFToIFactory;


/* My intention is to gradually put all configuration data (spacing, borders, colours etc)
 * in this struct. */
typedef struct
{
  double align_spacing ;
  double block_spacing ;
  double strand_spacing ;
  double column_spacing ;
  double feature_spacing ;
  guint  feature_line_width ;				    /* n.b. line width is in pixels. */
} ZMapWindowConfigStruct, *ZMapWindowConfig ;


/* Stats struct, useful for debugging but also for general information about a window. */
typedef struct
{



  /* Alignment stats. */
  int total_matches ;					    /* All matches drawn. */
  int gapped_matches ;					    /* Gapped matches drawn as gapped matches. */
  int not_perfect_gapped_matches ;			    /* Gapped matches drawn as ungapped because
							       failed quality check. */
  int ungapped_matches ;				    /* Ungapped matches drawn. */

  int total_boxes ;					    /* Total alignment boxes drawn. */
  int gapped_boxes ;					    /* Gapped boxes drawn. */
  int ungapped_boxes ;					    /* Ungapped boxes drawn. */
  int imperfect_boxes ;					    /* Gapped boxes _not_ drawn because of
							       quality check. */

} ZMapWindowStatsStruct, *ZMapWindowStats ;



/* Represents a single sequence display window with its scrollbars, canvas and feature
 * display. */
typedef struct _ZMapWindowStruct
{
  gchar *sequence ;					    /* Should remove this... */

  ZMapWindowConfigStruct config ;			    /* Holds window configuration info. */

  ZMapWindowStatsStruct stats ;				    /* Holds statitics about no. of features etc. */

  /* Widgets for displaying the data. */
  GtkWidget     *parent_widget ;
  GtkWidget     *toplevel ;
  GtkWidget     *scrolled_window ;
  GtkWidget     *pane;         /* points to toplevel */
  FooCanvas     *canvas ;				    /* where we paint the display */

  FooCanvasItem *rubberband;
  FooCanvasItem *horizon_guide_line;
  FooCanvasGroup *tooltip;

  int cursor_busy_count ;				    /* Used to turn cursor on/off... */


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


  /* Some default colours, good for hiding boxes/lines. */
  GdkColor canvas_fill ;
  GdkColor canvas_border ;
  GdkColor canvas_background ;
  GdkColor align_background ;


  /* Detailed colours (NOTE...NEED MERGING WITH THE ABOVE.... */
  gboolean done_colours ;
  GdkColor colour_root ;
  GdkColor colour_alignment ;
  GdkColor colour_block ;
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
  double         seq_start ;
  double         seq_end ;

  ZMapFeatureContext feature_context ;			    /* Currently displayed features. */

  GList *feature_set_names ;				    /* Gives names/order of columns to be displayed. */

  GHashTable *context_to_item ;				    /* Links parts of a feature context to
							       the canvas groups/items that
							       represent those parts. */


  FooCanvasGroup *feature_root_group ;			    /* the root of our features. */

  /* The stupid foocanvas can generate graphics items that are greater than the X Windows 32k
   * limit, so we have to keep a list of the canvas items that can generate graphics greater
   * than this limit as we zoom in and crop them ourselves. */
  ZMapWindowLongItems long_items ;				    
  ZMapWindowFToIFactory item_factory;

  /* Lists of dialog windows associated with this zmap window, these must be destroyed when
   * the zmap window is destroyed. */
  GPtrArray *feature_show_windows ;			    /* feature display windows. */

  GPtrArray *featureListWindows ;			    /* popup windows showing lists of
							       column features. */

  GPtrArray *search_windows ;				    /* popup search windows. */

  GPtrArray *dna_windows ;				    /* popup dna search windows. */

  GPtrArray *dnalist_windows ;				    /* popup showing list of dna match locations. */

  gboolean edittable_features ;				    /* FALSE means no features are edittable. */
  gboolean reuse_edit_window ;				    /* TRUE means reuse existing window
							       for new selected feature. */

  GtkWidget *col_config_window ;			    /* column configuration window. */


  ZMapWindowRulerCanvas ruler ;


  /* Holds focus items/column for the zmap. */
  ZMapWindowFocus focus ;


  /* Highlighting colours for items/columns. The item colour is used only if a select colour
   * was not specificed in the features style. */
  struct
  {
    unsigned int item : 1 ;
    unsigned int column : 1 ;
  } highlights_set ;
  GdkColor colour_item_highlight ;
  GdkColor colour_column_highlight ;


  /* Holds the marked region or item. */
  ZMapWindowMark mark ;

  /* We need to be able to find out if the user has done a revcomp for coordinate display
   * and other reasons, the display_forward_coords flag controls whether coords are displayed
   * always as if for the original forward strand or for the whichever is the current forward
   * strand. origin is used to transform coords for revcomp if display_forward_coords == TRUE */
  gboolean revcomped_features ;
  gboolean display_forward_coords ;
  int origin ;


  /* Are the "3 frame" columns displayed currently ? If show_3_frame_reverse == TRUE then
   * they are displayed on forward and reverse strands. */
  gboolean display_3_frame ;
  gboolean show_3_frame_reverse ;

  gboolean interrupt_expose;
} ZMapWindowStruct ;


/* Used in our event communication.... */
#define ZMAP_ATOM  "ZMap_Atom"


/* Used to pass data via a "send event" to the canvas window. */
typedef struct
{
  ZMapWindow window ;
  void *data ;
} zmapWindowDataStruct, *zmapWindowData ;


/* Represents a feature display window. */
typedef struct ZMapWindowFeatureShowStruct_ *ZMapWindowFeatureShow ;



typedef struct _zmapWindowFeatureListCallbacksStruct
{
  GCallback columnClickedCB;
  GCallback rowActivatedCB;
  GtkTreeSelectionFunc selectionFuncCB;
} zmapWindowFeatureListCallbacksStruct, *zmapWindowFeatureListCallbacks;


typedef struct _ZMapWindowFocusItemAreaStruct
{
  FooCanvasItem  *focus_item;
  gboolean        highlighted;
  gpointer        associated;
} ZMapWindowFocusItemAreaStruct,  *ZMapWindowFocusItemArea;

typedef void (*ZMapWindowStyleTableCallback)(ZMapFeatureTypeStyle style, gpointer user_data) ;


/* Callback for use in testing item hash objects to see if they fit a particular predicate. */
typedef gboolean (*ZMapWindowFToIPredFuncCB)(FooCanvasItem *canvas_item, gpointer user_data) ;

/* Handler to set stuff after an item has been drawn. */
typedef void (*ZMapWindowFeaturePostItemDrawHandler)(FooCanvasItem            *new_item, 
                                                     ZMapWindowItemFeatureType new_item_type,
                                                     ZMapFeature               full_feature,
                                                     ZMapWindowItemFeature     sub_feature,
                                                     double                    new_item_y1,
                                                     double                    new_item_y2,
                                                     gpointer                  handler_data);


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

void zmapWindowListWindowCreate(ZMapWindow zmapWindow, 
				GList *itemList,
				char *title,
				FooCanvasItem *currentItem, gboolean zoom_to_item) ;
void zmapWindowListWindowReread(GtkWidget *window_list_widget) ;

void zmapWindowCreateSearchWindow(ZMapWindow zmapWindow, FooCanvasItem *feature_item) ;
void zmapWindowCreateSequenceSearchWindow(ZMapWindow window, FooCanvasItem *feature_item,
					  ZMapSequenceType sequence_type) ;
void zmapWindowDNAListCreate(ZMapWindow zmapWindow, GList *dna_list, char *title, ZMapFeatureBlock block) ;
char *zmapWindowDNAChoose(ZMapWindow window, FooCanvasItem *feature_item, ZMapWindowDialogType dialog_type) ;

void zmapWindowFullReposition(ZMapWindow window) ;
void zmapWindowResetWidth(ZMapWindow window) ;

double zmapWindowCalcZoomFactor (ZMapWindow window);
void   zmapWindowSetPageIncr    (ZMapWindow window);

ZMapWindowLongItems zmapWindowLongItemCreate(double max_zoom) ;
void zmapWindowLongItemPushInterruption(ZMapWindowLongItems long_item);
void zmapWindowLongItemPopInterruption(ZMapWindowLongItems long_item);
gulong zmapWindowLongItemsInitialiseExpose(ZMapWindowLongItems long_item, FooCanvas *canvas);
void zmapWindowLongItemSetMaxZoom(ZMapWindowLongItems long_item, double max_zoom) ;
void zmapWindowLongItemCheck(ZMapWindowLongItems long_item, FooCanvasItem *item, double start, double end) ;
gboolean zmapWindowLongItemCoords(ZMapWindowLongItems long_items, FooCanvasItem *item,
				  double *start_out, double *end_out) ;
void zmapWindowLongItemCrop(ZMapWindowLongItems long_items, double x1, double y1, double x2, double y2) ;
gboolean zmapWindowLongItemRemove(ZMapWindowLongItems long_item, FooCanvasItem *item) ;
void zmapWindowLongItemPrint(ZMapWindowLongItems long_items) ;
void zmapWindowLongItemFree(ZMapWindowLongItems long_items) ;
void zmapWindowLongItemDestroy(ZMapWindowLongItems long_item) ;

void zmapWindowDrawFeatures(ZMapWindow window, 
			    ZMapFeatureContext current_context, ZMapFeatureContext new_context) ;

gboolean zmapWindowDumpFile(ZMapWindow window, char *filename) ;


int zmapWindowCoordToDisplay(ZMapWindow window, int coord) ;
int zmapWindowCoordFromDisplay(ZMapWindow window, int coord) ;
int zmapWindowCoordFromOriginRaw(int origin, int start) ;

double zmapWindowExt(double start, double end) ;
void zmapWindowSeq2CanExt(double *start_inout, double *end_inout) ;
void zmapWindowExt2Zero(double *start_inout, double *end_inout) ;
void zmapWindowSeq2CanExtZero(double *start_inout, double *end_inout) ;
void zmapWindowSeq2CanOffset(double *start_inout, double *end_inout, double offset) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
void zmapHideUnhideColumns(ZMapWindow window) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



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
			      FooCanvasGroup *set_group) ;
gboolean zmapWindowFToIRemoveSet(GHashTable *feature_to_context_hash,
				 GQuark align_id, GQuark block_id, GQuark set_id,
				 ZMapStrand strand, ZMapFrame frame) ;
gboolean zmapWindowFToIAddFeature(GHashTable *feature_to_context_hash,
				  GQuark align_id, GQuark block_id,
				  GQuark set_id, ZMapStrand set_strand, ZMapFrame set_frame,
				  GQuark feature_id, FooCanvasItem *feature_item) ;
gboolean zmapWindowFToIRemoveFeature(GHashTable *feature_to_context_hash,
				     ZMapStrand set_strand, ZMapFrame set_frame, ZMapFeature feature) ;
FooCanvasItem *zmapWindowFToIFindItemFull(GHashTable *feature_to_context_hash,
					  GQuark align_id, GQuark block_id, GQuark set_id,
					  ZMapStrand strand, ZMapFrame frame,
					  GQuark feature_id) ;
GList *zmapWindowFToIFindItemSetFull(GHashTable *feature_to_context_hash,
				     GQuark align_id, GQuark block_id, GQuark set_id,
				     char *strand_spec, char *frame_spec,
				     GQuark feature_id,
				     ZMapWindowFToIPredFuncCB pred_func, gpointer user_data) ; 
GList *zmapWindowFToIFindSameNameItems(GHashTable *feature_to_context_hash,
				       char *set_strand, char *set_frame, ZMapFeature feature) ;
FooCanvasItem *zmapWindowFToIFindSetItem(GHashTable *feature_to_context_hash,
					 ZMapFeatureSet feature_set,
					 ZMapStrand strand, ZMapFrame frame) ;
FooCanvasItem *zmapWindowFToIFindFeatureItem(GHashTable *feature_to_context_hash, ZMapStrand set_strand,
					     ZMapFrame set_frame, ZMapFeature feature) ;
FooCanvasItem *zmapWindowFToIFindItemChild(GHashTable *feature_to_context_hash,
					   ZMapStrand set_strand, ZMapFrame set_frame,
					   ZMapFeature feature, int child_start, int child_end) ;
FooCanvasItem *zMapWindowFindFeatureItemByItem(ZMapWindow window, FooCanvasItem *item) ;
void zmapWindowFToIDestroy(GHashTable *feature_to_item_hash) ;


void zmapWindowFeatureFactoryInit(ZMapWindow window);
void zmapWindowFToIFactoryClose(ZMapWindowFToIFactory factory) ;

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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
gboolean zmapWindowItemIsGetSize(FooCanvasItem *item) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

FooCanvasItem *zmapWindowItemGetDNAItem(ZMapWindow window, FooCanvasItem *item);
void zmapWindowItemHighlightDNARegion(ZMapWindow window, FooCanvasItem *any_item, int region_start, int region_end);
FooCanvasGroup *zmapWindowItemGetTranslationColumnFromBlock(ZMapWindow window, ZMapFeatureBlock block);
FooCanvasItem *zmapWindowItemGetTranslationItemFromItem(ZMapWindow window, FooCanvasItem *item);
void zmapWindowItemHighlightTranslationRegion(ZMapWindow window, FooCanvasItem *item, 
					      ZMapFrame required_frame,
					      int region_start, int region_end) ;

ZMapFeatureTypeStyle zmapWindowItemGetStyle(FooCanvasItem *feature_item) ;
void zmapWindowRaiseItem(FooCanvasItem *item) ;
GList *zmapWindowItemSortByPostion(GList *feature_item_list) ;
void zmapWindowSortCanvasItems(FooCanvasGroup *group) ;
FooCanvasGroup *zmapWindowFeatureItemsMakeGroup(ZMapWindow window, GList *feature_items) ;
gboolean zmapWindowItemGetStrandFrame(FooCanvasItem *item, ZMapStrand *set_strand, ZMapFrame *set_frame) ;
void zmapWindowPrintItemCoords(FooCanvasItem *item) ;

gboolean zmapWindowWorld2SeqCoords(ZMapWindow window,
				   double wx1, double wy1, double wx2, double wy2,
				   FooCanvasGroup **block_grp_out, int *y1_out, int *y2_out) ;
gboolean zmapWindowItem2SeqCoords(FooCanvasItem *item, int *y1, int *y2) ;

void zmapWindowItemGetVisibleCanvas(ZMapWindow window, 
                                    double *wx1, double *wy1,
                                    double *wx2, double *wy2);

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
void my_foo_canvas_item_get_long_bounds(ZMapWindowLongItems long_items, FooCanvasItem *item,
					double *x1_out, double *y1_out,
					double *x2_out, double *y2_out) ;


void zmapWindowPrintW2I(FooCanvasItem *item, char *text, double x1, double y1) ;
void zmapWindowPrintI2W(FooCanvasItem *item, char *text, double x1, double y1) ;

void zmapWindowScrollRegionTool(ZMapWindow window,
                                double *x1_inout, double *y1_inout,
                                double *x2_inout, double *y2_inout);
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

void zMapWindowUpdateInfoPanel(ZMapWindow window, ZMapFeature feature,
			       FooCanvasItem *item, FooCanvasItem *highlight_item,
			       gboolean replace_highlight_item, gboolean highlight_same_names) ;

void zmapWindowDrawZoom(ZMapWindow window) ;

void zmapWindowColumnConfigure(ZMapWindow window, FooCanvasGroup *column_group,
			       ZMapWindowColConfigureMode configure_mode) ;
void zmapWindowColumnConfigureDestroy(ZMapWindow window) ;

void zmapWindowColumnBump(FooCanvasItem *bump_item, ZMapStyleOverlapMode bump_mode) ;

void zmapWindowColumnWriteDNA(ZMapWindow window,
                              FooCanvasGroup *column_parent);

void zmapWindowColumnSetMagState(ZMapWindow window, FooCanvasGroup *col_group) ;
void zmapWindowColumnHide(FooCanvasGroup *column_group) ;
void zmapWindowColumnShow(FooCanvasGroup *column_group) ;
void zmapMakeColumnMenu(GdkEventButton *button_event, ZMapWindow window, FooCanvasItem *item,
			ZMapFeatureSet feature_set, ZMapFeatureTypeStyle style) ;


void zmapWindowToggleColumnInMultipleBlocks(ZMapWindow window, char *name,
                                            GQuark align_id, GQuark block_id, 
                                            gboolean force_to, gboolean force);


void zmapWindowGetPosFromScore(ZMapFeatureTypeStyle style, double score,
			       double *curr_x1_inout, double *curr_x2_out) ;

void zmapWindowFreeWindowArray(GPtrArray **window_array_inout, gboolean free_array) ;


ZMapWindowFeatureShow zmapWindowFeatureShowCreate(ZMapWindow zmapWindow, FooCanvasItem *item) ; 
ZMapWindowFeatureShow zmapWindowFeatureShow(ZMapWindow zmapWindow, FooCanvasItem *item) ;


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
ZMapGUIMenuItem zmapWindowMakeMenuBump(int *start_index_inout,
				       ZMapGUIMenuItemCallbackFunc callback_func,
				       gpointer callback_data, ZMapStyleOverlapMode curr_overlap_mode) ;
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
ZMapGUIMenuItem zmapWindowMakeMenuDumpOps(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuDNAHomol(int *start_index_inout,
					   ZMapGUIMenuItemCallbackFunc callback_func,
					   gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuProteinHomol(int *start_index_inout,
					       ZMapGUIMenuItemCallbackFunc callback_func,
					       gpointer callback_data) ;
ZMapGUIMenuItem zmapWindowMakeMenuTranscriptTools(int *start_index_inout,
                                                  ZMapGUIMenuItemCallbackFunc callback_func,
                                                  gpointer callback_data);

gboolean zmapWindowUpdateXRemoteData(ZMapWindow window, 
                                     ZMapFeatureAny feature_any, 
                                     char *action,
                                     FooCanvasItem *real_item);
gboolean zmapWindowUpdateXRemoteDataFull(ZMapWindow window, ZMapFeatureAny feature_any,
					 char *action, FooCanvasItem *real_item,
					 ZMapXMLObjTagFunctions start_handlers,
					 ZMapXMLObjTagFunctions end_handlers,
					 gpointer handler_data) ;

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


ZMapStrand zmapWindowFeatureStrand(ZMapFeature feature) ;
ZMapFrame zmapWindowFeatureFrame(ZMapFeature feature) ;

FooCanvasItem *zmapWindowFeatureDraw(ZMapWindow window, FooCanvasGroup *set_group, ZMapFeature feature) ;
FooCanvasItem *zmapWindowFeatureDrawScaled(ZMapWindow window, 
                                           FooCanvasGroup *set_group, 
                                           ZMapFeature feature,
                                           double scale_factor,
                                           ZMapWindowFeaturePostItemDrawHandler handler);


char *zmapWindowFeatureSetDescription(GQuark feature_set_id, ZMapFeatureTypeStyle style) ;

void zmapWindowFeatureHighlightDNA(ZMapWindow window, ZMapFeature Feature, FooCanvasItem *item);
/* 
void zmapWindowzoomControlClampSpan(ZMapWindow window, double *top_inout, double *bot_inout) ;
*/
void zmapWindowDebugWindowCopy(ZMapWindow window);
void zmapWindowGetBorderSize(ZMapWindow window, double *border);
/* End of zmapWindowZoomControl.c functions */


void zmapWindowDrawScaleBar(ZMapWindow window, double start, double end) ;

/*!-------------------------------------------------------------------!
 *| Checks to see if the item really is visible.  In order to do this |
 *| all the item's parent groups need to be examined.                 |  
 *!-------------------------------------------------------------------!*/
gboolean zmapWindowItemIsVisible(FooCanvasItem *item) ;
/*!-------------------------------------------------------------------!
 *| Checks to see if the item is shown.  An item may still not be     |
 *| visible as any one of its parents might be hidden. If this        |
 *| definitive answer is required, use zmapWindowItemIsVisible        |
 *| instead.                                                          |
 *!-------------------------------------------------------------------!*/
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

ZMapWindowMark zmapWindowMarkCreate(ZMapWindow window) ;
gboolean zmapWindowMarkIsSet(ZMapWindowMark mark) ;
void zmapWindowMarkReset(ZMapWindowMark mark) ;
void zmapWindowMarkSetColour(ZMapWindowMark mark, char *colour) ;
GdkColor *zmapWindowMarkGetColour(ZMapWindowMark mark) ;
void zmapWindowMarkSetItem(ZMapWindowMark mark, FooCanvasItem *item) ;
FooCanvasItem *zmapWindowMarkGetItem(ZMapWindowMark mark) ;
gboolean zmapWindowMarkSetWorldRange(ZMapWindowMark mark,
				     double world_x1, double world_y1, double world_x2, double world_y2) ;
gboolean zmapWindowMarkGetWorldRange(ZMapWindowMark mark,
				     double *world_x1, double *world_y1,
				     double *world_x2, double *world_y2) ;
gboolean zmapWindowMarkGetSequenceRange(ZMapWindowMark mark, int *start, int *end) ;
void zmapWindowMarkDestroy(ZMapWindowMark mark) ;



/*!-------------------------------------------------------------------!
 *| The following functions maintain the list of focus items          |
 *| (window->focusItemSet). Just to explain this list should be the   |
 *| highlighted items (rev-video) on the canvas.  This seems          |
 *| simple. We also need to know which item is the current or most    |
 *| recent item.  In order to support this I'm assuming that the first|
 *| item in the list is the most recent.  I call this the "hot" item, |
 *| like a hot potato.                                                |
 *!-------------------------------------------------------------------!*/

ZMapWindowFocus zmapWindowFocusCreate(void) ;
void zmapWindowFocusAddItem(ZMapWindowFocus focus, FooCanvasItem *item);
void zmapWindowFocusAddItems(ZMapWindowFocus focus, GList *item_list);
void zmapWindowFocusForEachFocusItem(ZMapWindowFocus focus, GFunc callback, gpointer user_data) ;
void zmapWindowFocusReset(ZMapWindowFocus focus) ;
void zmapWindowFocusRemoveFocusItem(ZMapWindowFocus focus, FooCanvasItem *item);
void zmapWindowFocusSetHotItem(ZMapWindowFocus focus, FooCanvasItem *item, gboolean remove_hot_item) ;
FooCanvasItem *zmapWindowFocusGetHotItem(ZMapWindowFocus focus) ;
GList *zmapWindowFocusGetFocusItems(ZMapWindowFocus focus) ;
gboolean zmapWindowFocusIsItemInHotColumn(ZMapWindowFocus focus, FooCanvasItem *item) ;
void zmapWindowFocusSetHotColumn(ZMapWindowFocus focus, FooCanvasGroup *column) ;
FooCanvasGroup *zmapWindowFocusGetHotColumn(ZMapWindowFocus focus) ;
void zmapWindowFocusDestroy(ZMapWindowFocus focus) ;

void zmapWindowFocusMaskOverlay(ZMapWindowFocus focus, FooCanvasItem *item, GdkColor *highlight);
void zmapWindowFocusAddOverlayManager(ZMapWindowFocus focus, ZMapWindowOverlay overlay);
void zmapWindowFocusClearOverlayManagers(ZMapWindowFocus focus);


ZMapWindowFocusItemArea zmapWindowFocusItemAreaCreate(FooCanvasItem *item);
void zmapWindowFocusItemAreaDestroy(ZMapWindowFocusItemArea item_area);

void zmapWindowReFocusHighlights(ZMapWindow window);


GList *zmapWindowItemListToFeatureList(GList *item_list);

void zmapWindowHighlightObject(ZMapWindow window, FooCanvasItem *item,
			       gboolean replace_highlight_item, gboolean highlight_same_names) ;
void zmapHighlightColumn(ZMapWindow window, FooCanvasGroup *column) ;
void zmapUnHighlightColumn(ZMapWindow window, FooCanvasGroup *column) ;

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
                                    FooCanvasGroup *forward_strand_group, 
                                    FooCanvasGroup *reverse_strand_group,
                                    ZMapFeatureBlock block, 
                                    ZMapFeatureSet feature_set,
                                    ZMapFrame frame,
                                    FooCanvasGroup **forward_col_out, 
                                    FooCanvasGroup **reverse_col_out);
void zmapWindowDrawFeatureSet(ZMapWindow window, 
                              ZMapFeatureSet feature_set,
                              FooCanvasGroup *forward_col, 
                              FooCanvasGroup *reverse_col,
                              ZMapFrame frame) ;
void zmapWindowRemoveEmptyColumns(ZMapWindow window,
				  FooCanvasGroup *forward_group, FooCanvasGroup *reverse_group) ;
gboolean zmapWindowRemoveIfEmptyCol(FooCanvasGroup **col_group) ;

GHashTable *zmapWindowStyleTableCreate(void) ;
gboolean zmapWindowStyleTableAdd(GHashTable *style_table, ZMapFeatureTypeStyle new_style) ;
ZMapFeatureTypeStyle zmapWindowStyleTableFind(GHashTable *style_table, GQuark style_id) ;
void zmapWindowStyleTableForEach(GHashTable *style_table,
				 ZMapWindowStyleTableCallback app_func, gpointer app_data) ;
void zmapWindowStyleTableDestroy(GHashTable *style_table) ;

/* Ruler Functions */
ZMapWindowRulerCanvas zmapWindowRulerCanvasCreate(ZMapWindowRulerCanvasCallbackList callbacks);
void zmapWindowRulerCanvasInit(ZMapWindowRulerCanvas obj,
                               GtkWidget *paned,
                               GtkAdjustment *vadjustment);
void zmapWindowRulerCanvasMaximise(ZMapWindowRulerCanvas obj, double y1, double y2);
void zmapWindowRulerCanvasOpenAndMaximise(ZMapWindowRulerCanvas obj);
void zmapWindowRulerCanvasSetOrigin(ZMapWindowRulerCanvas obj, int origin) ;
void zmapWindowRulerCanvasDraw(ZMapWindowRulerCanvas obj, double x, double y, gboolean force);
void zmapWindowRulerCanvasSetVAdjustment(ZMapWindowRulerCanvas obj, GtkAdjustment *vadjustment);
void zmapWindowRulerCanvasSetPixelsPerUnit(ZMapWindowRulerCanvas obj, double x, double y);
void zmapWindowRulerCanvasSetLineHeight(ZMapWindowRulerCanvas obj,
                                        double border);

void zmapWindowRulerGroupDraw(FooCanvasGroup *parent, double project_at, 
                              double origin, double start, double end);

/* End Ruler Functions */


void zmapWindowStatsReset(ZMapWindowStats stats) ;
void zmapWindowStatsPrint(ZMapWindowStats stats) ;

ZMapWindowItemFeatureSetData zmapWindowItemFeatureSetCreate(ZMapWindow window,
                                                            ZMapFeatureTypeStyle style,
                                                            ZMapStrand strand,
                                                            ZMapFrame frame);
void zmapWindowItemFeatureSetDestroy(ZMapWindowItemFeatureSetData item_feature_set);


char *zmapWindowGetDialogText(ZMapWindowDialogType dialog_type) ;

void zmapWindowColOrderColumns(ZMapWindow window);
void zmapWindowColOrderPositionColumns(ZMapWindow window);

void zmapWindowContextExplorerCreate(ZMapWindow window, ZMapFeatureAny feature_any);

#endif /* !ZMAP_WINDOW_P_H */
