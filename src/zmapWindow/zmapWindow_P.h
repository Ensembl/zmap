/*  File: zmapWindow_P.h
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Defines internal interfaces/data structures of zMapWindow.
 *              
 * HISTORY:
 * Last edited: Oct 10 08:44 2005 (edgrif)
 * Created: Fri Aug  1 16:45:58 2003 (edgrif)
 * CVS info:   $Id: zmapWindow_P.h,v 1.82 2005-10-10 10:31:36 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_P_H
#define ZMAP_WINDOW_P_H

#include <gtk/gtk.h>
#include <ZMap/zmapDraw.h>
#include <ZMap/zmapWindow.h>
#include <ZMap/zmapFeature.h>
/* #include <libfoocanvas/libfoocanvas.h> */

/* Names of config stanzas. */
#define ZMAP_WINDOW_CONFIG "ZMapWindow"


/* default spacings...will be tailorable one day ? */
#define ALIGN_SPACING    30.0
#define STRAND_SPACING   20.0
#define COLUMN_SPACING    5.0



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

typedef enum { 
  ZMAP_WINDOW_LIST_COL_NAME,                /*!< feature name column  */
  ZMAP_WINDOW_LIST_COL_TYPE,                /*!< feature type column  */
  ZMAP_WINDOW_LIST_COL_STRAND,              /*!< feature strand  */
  ZMAP_WINDOW_LIST_COL_START,               /*!< feature start */
  ZMAP_WINDOW_LIST_COL_END,                 /*!< feature end  */
  ZMAP_WINDOW_LIST_COL_PHASE,               /*!< feature phase  */
  ZMAP_WINDOW_LIST_COL_SCORE,               /*!< feature score  */
  ZMAP_WINDOW_LIST_COL_FEATURE_TYPE,        /*!< feature method  */
  ZMAP_WINDOW_LIST_COL_FEATURE_ITEM,        /*!< foocanvas item  */
  ZMAP_WINDOW_LIST_COL_SORT_TYPE,           /*!< \                          */
  ZMAP_WINDOW_LIST_COL_SORT_STRAND,         /*!< -- sort on integer columns */
  ZMAP_WINDOW_LIST_COL_SORT_PHASE,          /*!< /                          */
  ZMAP_WINDOW_LIST_COL_NUMBER               /*!< number of columns  */
} zmapWindowFeatureListColumn;
#define ZMAP_WINDOW_FEATURE_LIST_COL_NUMBER_KEY "column_number_data"

/* Default colours. */

#define ZMAP_WINDOW_BACKGROUND_COLOUR "white"		    /* main canvas background */

/* Colours for master alignment block (forward and reverse). */
#define ZMAP_WINDOW_MBLOCK_F_BG "white"
#define ZMAP_WINDOW_MBLOCK_R_BG "light gray"

/* Colours for query alignment block (forward and reverse). */
#define ZMAP_WINDOW_QBLOCK_F_BG "light pink"
#define ZMAP_WINDOW_QBLOCK_R_BG "pink"

/* Default colours for features. */
#define ZMAP_WINDOW_ITEM_FILL_COLOUR "white"
#define ZMAP_WINDOW_ITEM_BORDER_COLOUR "black"

/* Item features are the canvas items that represent sequence features, they can be of various
 * types, in particular compound features such as transcripts require a parent, a bounding box
 * and children for features such as introns/exons. */
typedef enum
  {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    /* this should be redundant now...... */
    ITEM_ALIGN,
    ITEM_BLOCK,
    ITEM_SET,
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    ITEM_FEATURE_SIMPLE,				    /* Item is the whole feature. */
    ITEM_FEATURE_PARENT,				    /* Item is parent group of whole feature. */
    ITEM_FEATURE_CHILD,					    /* Item is child/subpart of feature. */
    ITEM_FEATURE_BOUNDING_BOX				    /* Item is invisible bounding box of
							       feature or subpart of feature.  */
  } ZMapWindowItemFeatureType ;

/* A bit field I found I needed to make it easier to calc what had been clamped. */
typedef enum
  {
    ZMAP_WINDOW_CLAMP_INIT  = 0,
    ZMAP_WINDOW_CLAMP_NONE  = (1 << 0),
    ZMAP_WINDOW_CLAMP_START = (1 << 1),
    ZMAP_WINDOW_CLAMP_END   = (1 << 2)
  } ZMapWindowClampType;


/* Probably will need to expand this to be a union as we come across more features that need
 * different information recording about them.
 * The intent of this structure is to hold information for items that are subparts of features,
 * e.g. exons, where it is tedious/error prone to recover the information from the feature and
 * canvas item. */
typedef struct
{
  int start, end ;					    /* start/end of feature in sequence coords. */
  FooCanvasItem *twin_item ;				    /* Some features need to be drawn with
							       two canvas items, an example is
							       introns which need an invisible
							       bounding box for sensible user
							       interaction. */
} ZMapWindowItemFeatureStruct, *ZMapWindowItemFeature ;


/* Bump type for columns. */
typedef enum
  {
    ZMAP_WINDOW_BUMP_NONE,				    /* i.e. reset to no bumping. */
    ZMAP_WINDOW_BUMP_SIMPLE,				    /* only for testing really... */
    ZMAP_WINDOW_BUMP_POSITION,				    /* bump on y position */
    ZMAP_WINDOW_BUMP_NAME				    /* bump on feature name */
  } ZMapWindowBumpType ;



typedef struct _ZMapWindowZoomControlStruct *ZMapWindowZoomControl ;

/* Represents a single sequence display window with its scrollbars, canvas and feature
 * display. */
typedef struct _ZMapWindowStruct
{
  gchar *sequence ;

  /* Widgets for displaying the data. */
  GtkWidget     *parent_widget ;
  GtkWidget     *toplevel ;
  GtkWidget     *scrolled_window ;			    /* points to toplevel */
  FooCanvas     *canvas ;				    /* where we paint the display */
  FooCanvas     *ruler_canvas ;				    /* where we paint the display */

  FooCanvasItem *rubberband;
  FooCanvasItem *horizon_guide_line;
  FooCanvasGroup *tooltip;

  ZMapWindowCallbacks caller_cbs ;			    /* table of callbacks registered by
							     * our caller. */

  gint width, height ;					    /* Used by a size allocate callback to
							       monitor canvas size changes and
							       remap the minimum size of the canvas if
							       needed. */
  
  /* Windows can be locked together in their zooming/scrolling. */
  gboolean locked_display ;				    /* Is this window locked ? */
  ZMapWindowLockType curr_locking ;			    /* Orientation of current locking. */
  GHashTable *sibling_locked_windows ;			    /* windows this window is locked with. */

  /* Some default colours, good for hiding boxes/lines. */
  GdkColor canvas_fill ;
  GdkColor canvas_border ;
  GdkColor canvas_background ;
  GdkColor align_background ;

  ZMapWindowZoomControl zoom;

  int            canvas_maxwin_size ;			    /* 30,000 is the maximum (default). */

  /* I'm trying these out as using the sequence start/end is not correct for window stuff. */
  double min_coord ;					    /* min/max canvas coords */
  double max_coord ;

  double align_gap ;					    /* horizontal gap between alignments. */
  double column_gap ;					    /* horizontal gap between columns. */


  GtkWidget     *text ;
  GdkAtom        zmap_atom ;
  void          *app_data ;
  gulong         exposeHandlerCB ;


  ZMapFeatureContext feature_context ;			    /* Currently displayed features. */

  GHashTable *context_to_item ;				    /* Links parts of a feature context to
							       the canvas groups/items that
							       represent those parts. */

  GPtrArray     *featureListWindows ;			    /* popup windows showing lists of
							       column features. */

  /* The stupid foocanvas can generate graphics items that are greater than the X Windows 32k
   * limit, so we have to keep a list of the canvas items that can generate graphics greater
   * than this limit as we zoom in and crop them ourselves. */
  GList         *long_items ;				    

  /* This all needs to move and for scale to be in a separate window..... */
  FooCanvasItem *scaleBarGroup;           /* canvas item in which we build the scalebar */

  /* The length, start and end of the segment of sequence to be shown, there will be _no_
   * features outside of the start/end. */
  double         seqLength;
  double         seq_start ;
  double         seq_end ;

  FooCanvasItem       *focus_item ;			    /* the item which has focus */

  /* THIS FIELD IS TEMPORARY UNTIL ALL THE SCALE/RULER IS SORTED OUT, DO NOT USE... */
  double alignment_start ;

} ZMapWindowStruct ;


/* We use this to hold info. about canvas items that will exceed the X Windows 32k limit
 * for graphical objects and hence may need to be clipped. */
typedef struct _ZMapWindowLongItemStruct
{
  FooCanvasItem *item ;

  union
  {
    struct {
      double         start ;
      double         end ;
    } box ;
    FooCanvasPoints *points ;
  } pos ;
} ZMapWindowLongItemStruct, *ZMapWindowLongItem ;



/* Used in our event communication.... */
#define ZMAP_ATOM  "ZMap_Atom"


/* Used to pass data via a "send event" to the canvas window. */
typedef struct
{
  ZMapWindow window ;
  void *data ;
} zmapWindowDataStruct, *zmapWindowData ;

typedef struct _zmapWindowEditorDataStruct *ZMapWindowEditor;

typedef struct _zmapWindowFeatureListCallbacksStruct
{
  GCallback columnClickedCB;
  GCallback rowActivatedCB;
  GtkTreeSelectionFunc selectionFuncCB;
} zmapWindowFeatureListCallbacksStruct, *zmapWindowFeatureListCallbacks;


GtkWidget *zmapWindowMakeMenuBar(ZMapWindow window) ;
GtkWidget *zmapWindowMakeButtons(ZMapWindow window) ;
GtkWidget *zmapWindowMakeFrame(ZMapWindow window) ;

void zmapWindowPrintCanvas(FooCanvas *canvas) ;
void zmapWindowPrintGroups(FooCanvas *canvas) ;
void zmapWindowPrintItem(FooCanvasGroup *item) ;
void zmapWindowPrintLocalCoords(char *msg_prefix, FooCanvasItem *item) ;

void zmapWindowShowItem(FooCanvasItem *item) ;

//void zmapWindowListWindowCreate(ZMapWindow window, FooCanvasItem *item, ZMapStrand strandMask) ;
void zmapWindowListWindowCreate(ZMapWindow zmapWindow, 
                                GList *itemList,
                                char *title,
                                FooCanvasItem *currentItem);

void zmapWindowCreateSearchWindow(ZMapWindow zmapWindow, ZMapFeatureAny feature_any) ;

double zmapWindowCalcZoomFactor (ZMapWindow window);
void   zmapWindowSetPageIncr    (ZMapWindow window);

void zmapWindowLongItemCheck(ZMapWindow window, FooCanvasItem *item, double start, double end) ;
void zmapWindowLongItemCrop(ZMapWindow window) ;
void zmapWindowLongItemFree(GList *long_items) ;

void zmapWindowDrawFeatures(ZMapWindow window, 
			    ZMapFeatureContext current_context, ZMapFeatureContext new_context) ;


double zmapWindowExt(double start, double end) ;
void zmapWindowSeq2CanExt(double *start_inout, double *end_inout) ;
void zmapWindowExt2Zero(double *start_inout, double *end_inout) ;
void zmapWindowSeq2CanExtZero(double *start_inout, double *end_inout) ;
void zmapWindowSeq2CanOffset(double *start_inout, double *end_inout, double offset) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
void zmapHideUnhideColumns(ZMapWindow window) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


GHashTable *zmapWindowFToICreate(void) ;
GQuark zmapWindowFToIMakeSetID(GQuark set_id, ZMapStrand strand) ;
gboolean zmapWindowFToIAddAlign(GHashTable *feature_to_context_hash,
				GQuark align_id,
				FooCanvasGroup *align_group) ;
gboolean zmapWindowFToIAddBlock(GHashTable *feature_to_context_hash,
				GQuark align_id, GQuark block_id,
				FooCanvasGroup *block_group) ;
gboolean zmapWindowFToIAddSet(GHashTable *feature_to_context_hash,
			      GQuark align_id, GQuark block_id, GQuark set_id,
			      ZMapStrand strand,
			      FooCanvasGroup *set_group) ;
gboolean zmapWindowFToIAddFeature(GHashTable *feature_to_context_hash,
				  GQuark align_id, GQuark block_id, GQuark set_id,
				  GQuark feature_id, FooCanvasItem *feature_item) ;
FooCanvasItem *zmapWindowFToIFindItemFull(GHashTable *feature_to_context_hash,
					  GQuark align_id, GQuark block_id, GQuark set_id,
					  ZMapStrand strand,
					  GQuark feature_id) ;
GList *zmapWindowFToIFindItemSetFull(GHashTable *feature_to_context_hash,
				     GQuark align_id, GQuark block_id, GQuark set_id,
				     ZMapStrand strand,
				     GQuark feature_id) ;
FooCanvasItem *zmapWindowFToIFindSetItem(GHashTable *feature_to_context_hash,
					 ZMapFeatureSet feature_set, ZMapStrand strand) ;
FooCanvasItem *zmapWindowFToIFindFeatureItem(GHashTable *feature_to_context_hash, ZMapFeature feature) ;
FooCanvasItem *zmapWindowFToIFindItemChild(GHashTable *feature_to_context_hash, ZMapFeature feature,
					   int child_start, int child_end) ;
void zmapWindowFToIDestroy(GHashTable *feature_to_item_hash) ;

void zmapWindowPrintItemCoords(FooCanvasItem *item) ;

void my_foo_canvas_item_w2i(FooCanvasItem *item, double *x, double *y) ;
void my_foo_canvas_item_i2w(FooCanvasItem *item, double *x, double *y) ;
void my_foo_canvas_item_goto(FooCanvasItem *item, double *x, double *y) ;

void zmapWindowPrintW2I(FooCanvasItem *item, char *text, double x1, double y1) ;
void zmapWindowPrintI2W(FooCanvasItem *item, char *text, double x1, double y1) ;

gboolean zmapWindowCallBlixem(ZMapWindow window, FooCanvasItem *item, gboolean oneType);

/* Window Editor functions */
ZMapWindowEditor zmapWindowEditorCreate(ZMapWindow zmapWindow, FooCanvasItem *item); 
void zmapWindowEditorDraw(ZMapWindowEditor editor);
/* End of Window Editor functions */

void zmapWindow_set_scroll_region(ZMapWindow window, double y1a, double y2a);

ZMapWindowClampType zmapWindowClampSpan(ZMapWindow window, 
                                        double *top_inout, 
                                        double *bot_inout) ;
ZMapWindowClampType zmapWindowClampStartEnd(ZMapWindow window, 
                                            double *top_inout, 
                                            double *bot_inout) ;

void zmapWindowMaximiseRectangle(FooCanvasItem *rectangle) ;

void zMapWindowMoveItem(ZMapWindow window, ZMapFeature origFeature,
			ZMapFeature modFeature,  FooCanvasItem *item);

void zMapWindowMoveSubFeatures(ZMapWindow window, 
			       ZMapFeature originalFeature, 
			       ZMapFeature modifiedFeature,
			       GArray *origArray, GArray *modArray,
			       gboolean isExon);

void zMapWindowUpdateInfoPanel(ZMapWindow window, ZMapFeature feature, FooCanvasItem *item);


void zmapWindowColumnBump(FooCanvasGroup *column_group, ZMapWindowBumpType bump_type) ;
void zmapWindowColumnReposition(FooCanvasGroup *column_group) ;

FooCanvasGroup *zmapWindowContainerCreate(FooCanvasGroup *parent,
					  GdkColor *background_fill_colour,
					  GdkColor *background_border_colour) ;
FooCanvasGroup *zmapWindowContainerGetSuperGroup(FooCanvasGroup *container_parent) ;
FooCanvasGroup *zmapWindowContainerGetParent(FooCanvasItem *any_container_child) ;
FooCanvasGroup *zmapWindowContainerGetFeatures(FooCanvasGroup *container_parent) ;
FooCanvasItem *zmapWindowContainerGetBackground(FooCanvasGroup *container_parent) ;
void zmapWindowContainerSetBackgroundSize(FooCanvasGroup *container_parent, double y_extent) ;
void zmapWindowContainerPrint(FooCanvasGroup *container_parent) ;


void zmapWindowCanvasGroupChildSort(FooCanvasGroup *group_inout) ;


GtkTreeModel *zmapWindowFeatureListCreateStore(gboolean use_tree_store);
GtkWidget    *zmapWindowFeatureListCreateView(GtkTreeModel *treeModel,
                                              GtkCellRenderer *renderer,
                                              zmapWindowFeatureListCallbacks callbacks,
                                              gpointer user_data);
void zmapWindowFeatureListPopulateStoreList(GtkTreeModel *treeModel,
                                            GList *list);
gint zmapWindowFeatureListCountSelected(GtkTreeSelection *selection);
gint zmapWindowFeatureListGetColNumberFromTVC(GtkTreeViewColumn *col);

/* ================= in zmapWindowZoomControl.c ========================= */
ZMapWindowZoomControl zmapWindowZoomControlCreate(ZMapWindow window) ;
void zmapWindowZoomControlInitialise(ZMapWindow window) ;
gboolean zmapWindowZoomControlZoomByFactor(ZMapWindow window, double factor);
void zmapWindowZoomControlHandleResize(ZMapWindow window);
double zmapWindowZoomControlLimitSpan(ZMapWindow window, double y1, double y2) ;
void zmapWindowZoomControlCopyTo(ZMapWindowZoomControl orig, ZMapWindowZoomControl new) ;


/* 
void zmapWindowzoomControlClampSpan(ZMapWindow window, double *top_inout, double *bot_inout) ;
*/
void zmapWindowDebugWindowCopy(ZMapWindow window);
void zmapWindowGetBorderSize(ZMapWindow window, double *border);
/* End of zmapWindowZoomControl.c functions */


void zmapWindowDrawScaleBar(ZMapWindow window, double start, double end) ;


gboolean zmapWindowItemIsVisible(FooCanvasItem *item) ;


#endif /* !ZMAP_WINDOW_P_H */
