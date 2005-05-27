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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 *
 * Description: Defines internal interfaces/data structures of zMapWindow.
 *              
 * HISTORY:
 * Last edited: May 26 15:58 2005 (rnc)
 * Created: Fri Aug  1 16:45:58 2003 (edgrif)
 * CVS info:   $Id: zmapWindow_P.h,v 1.51 2005-05-27 08:51:39 rnc Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_P_H
#define ZMAP_WINDOW_P_H

#include <gtk/gtk.h>
#include <ZMap/zmapWindow.h>
#include <ZMap/zmapFeature.h>


/* Names of config stanzas. */
#define ZMAP_WINDOW_CONFIG "ZMapWindow"
#define ZMAP_BLIXEM_CONFIG "blixem"




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
 */


enum
  {
    ZMAP_WINDOW_TEXT_BORDER = 2,			    /* border above/below dna text. */
    ZMAP_WINDOW_STEP_INCREMENT = 10,                        /* scrollbar stepping increment */
    ZMAP_WINDOW_PAGE_INCREMENT = 600,                       /* scrollbar paging increment */
    ZMAP_WINDOW_MAX_WINDOW = 30000			    /* Largest canvas window. */
  } ;


/* Default colours. */
#define ZMAP_WINDOW_ITEM_FILL_COLOUR "white"
#define ZMAP_WINDOW_ITEM_BORDER_COLOUR "black"
#define ZMAP_WINDOW_BACKGROUND_COLOUR "white"


/* Represents all blocks for a single alignment, if the alignment is ungapped it
 * will contain just one block. */
typedef struct ZMapWindowAlignmentStructName
{
  GQuark alignment_id ;

  ZMapWindow window ;					    /* our parent window. */

  /* We may need to have a feature context in here.... */


  FooCanvasItem *alignment_group ;			    /* Group containing all columns for
							       this alignment. */

  GData *blocks ;					    /* All blocks in the alignment. */

  double col_gap ;					    /* space between columns. */

} ZMapWindowAlignmentStruct, *ZMapWindowAlignment ;


/* Represents all the columns of features for one contiguous alignment block.
 * There may be multiple discontinuous blocks displayed for a single alignment. */
typedef struct ZMapWindowAlignmentBlockStructName
{
  GQuark block_id ;					    /* Needed ???? */

  ZMapWindowAlignment parent ;				    /* Our parent alignment. */

  FooCanvasItem *block_group ;				    /* Group containing all columns for
							       this alignment. */

  GPtrArray *columns ;					    /* All ZMapWindowColumn's in the alignment. */

} ZMapWindowAlignmentBlockStruct, *ZMapWindowAlignmentBlock ;




/* Represents a column on the display. A column contains features that all have the
 * same type/style, e.g. "confirmed" transcripts. The column contains both the forward
 * and reverse strand features. */
typedef struct
{
  GQuark type_name ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  GQuark key_id ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  ZMapWindowAlignmentBlock parent_block ;		    /* Our Alignment block parent. */

  FooCanvasItem *column_group ;				    /* The whole column. */
  FooCanvasItem *forward_group, *reverse_group ;	    /* The forward and reverse groups. */

  gboolean forward ;
  ZMapFeatureTypeStyle type ;

  double strand_gap ;					    /* space between strands. */

} ZMapWindowColumnStruct, *ZMapWindowColumn ;



/* Item features are the canvas items that represent sequence features, they can be of various
 * types, in particular compound features such as transcripts require a parent, a bounding box
 * and children for features such as introns/exons. */
typedef enum
  {
    ITEM_FEATURE_SIMPLE,				    /* Item is the whole feature. */
    ITEM_FEATURE_PARENT,				    /* Item is parent group of whole feature. */
    ITEM_FEATURE_CHILD,					    /* Item is child/subpart of feature. */
    ITEM_FEATURE_BOUNDING_BOX				    /* Item is invisible bounding box of
							       feature or subpart of feature.  */
  } ZMapWindowItemFeatureType ;


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

  double         zoom_factor ;
  ZMapWindowZoomStatus zoom_status ;   /* For short sequences that are displayed at max. zoom initially. */
  double         min_zoom ;				    /* min/max allowable zoom. */
  double         max_zoom ;
  int            canvas_maxwin_size ;			    /* 30,000 is the maximum (default). */
  int            border_pixels ;			    /* top/bottom border to sequence. */
  double         text_height;                               /* used to calculate min/max zoom */

  GtkWidget     *text ;
  GdkAtom        zmap_atom ;
  void          *app_data ;
  gulong         exposeHandlerCB ;


  /* The relationship between context and alignment will need refining when we do multiple
   * alignments properly. */
  ZMapFeatureContext feature_context ;			    /* Currently displayed features. */

  GData         *alignments ;

  GData         *types ;


  GHashTable *feature_to_item ;				    /* Links a feature to the canvas item
							       that displays it. */

  GPtrArray     *featureListWindows ;			    /* popup windows showing lists of
							       column features. */

  GData         *longItems ;				    /* set of features >30k long, need to be
							       cropped as we zoom in. */


  /* This all needs to move and for scale to be in a separate window..... */
  FooCanvasItem *scaleBarGroup;           /* canvas item in which we build the scalebar */
  double         scaleBarOffset;
  int major_scale_units, minor_scale_units ;		    /* Major/minor tick marks on scale. */


  /* The length, start and end of the segment of sequence to be shown, there will be _no_
   * features outside of the start/end. */
  double         seqLength;
  double         seq_start ;
  double         seq_end ;

  FooCanvasItem       *focus_item ;			    /* the item which has focus */

} ZMapWindowStruct ;



typedef struct _ZMapWindowLongItemStruct
{
  char          *name;
  double         start;
  double         end;
  FooCanvasItem *canvasItem;
} ZMapWindowLongItemStruct, *ZMapWindowLongItem;




/* Used in our event communication.... */
#define ZMAP_ATOM  "ZMap_Atom"


/* Used to pass data via a "send event" to the canvas window. */
typedef struct
{
  ZMapWindow window ;
  void *data ;
} zmapWindowDataStruct, *zmapWindowData ;



GtkWidget *zmapWindowMakeMenuBar(ZMapWindow window) ;
GtkWidget *zmapWindowMakeButtons(ZMapWindow window) ;
GtkWidget *zmapWindowMakeFrame(ZMapWindow window) ;

void zmapWindowPrintCanvas(FooCanvas *canvas) ;
void zmapWindowPrintGroups(FooCanvas *canvas) ;
void zmapWindowPrintItem(FooCanvasGroup *item) ;

void zmapWindowShowItem(FooCanvasItem *item) ;

void     zMapWindowCreateListWindow(ZMapWindow window, FooCanvasItem *item) ;

FooCanvasItem *zmapDrawScale(FooCanvas *canvas, double offset, double zoom_factor, int start, int end,
			     int *major_units_out, int *minor_units_out);
double zmapWindowCalcZoomFactor (ZMapWindow window);
void   zmapWindowSetPageIncr    (ZMapWindow window);
void   zmapWindowCropLongFeature(GQuark quark, gpointer data, gpointer user_data);

void zmapWindowDrawFeatures(ZMapWindow window, 
			    ZMapFeatureContext current_context, ZMapFeatureContext new_context,
			    GData *types) ;

void zmapHideUnhideColumns(ZMapWindow window) ;

ZMapWindowAlignment zmapWindowAlignmentCreate(char *align_name, ZMapWindow window,
					      FooCanvasGroup *parent_group, double position) ;
ZMapWindowAlignmentBlock zmapWindowAlignmentAddBlock(ZMapWindowAlignment alignment,
						     char *block_id, double position) ;
ZMapWindowColumn zmapWindowAlignmentAddColumn(ZMapWindowAlignmentBlock block, GQuark type_name,
					      ZMapFeatureTypeStyle type) ;
FooCanvasItem *zmapWindowAlignmentGetColumn(ZMapWindowColumn column_group, ZMapStrand strand) ;
void zmapWindowAlignmentHideUnhideColumns(ZMapWindowAlignmentBlock block) ;
ZMapWindowAlignment zmapWindowAlignmentDestroy(ZMapWindowAlignment alignment) ;


GHashTable *zmapWindowFToICreate(void) ;
void zmapWindowFToIAddSet(GHashTable *feature_to_item_hash, GQuark set_id) ;
void zmapWindowFToIAddFeature(GHashTable *feature_to_item_hash, GQuark set_id,
			      GQuark feature_id, FooCanvasItem *item) ;
FooCanvasItem *zmapWindowFToIFindItem(GHashTable *feature_to_item_hash, GQuark set_id,
				      GQuark feature_id) ;
FooCanvasItem *zmapWindowFToIFindItemChild(GHashTable *feature_to_item_hash, GQuark set_id,
					   GQuark feature_id, int child_start, int child_end) ;
void zmapWindowFToIDestroy(GHashTable *feature_to_item_hash) ;


void zmapWindowPrintItemCoords(FooCanvasItem *item) ;
void my_foo_canvas_item_w2i (FooCanvasItem *item, double *x, double *y) ;
void my_foo_canvas_item_i2w (FooCanvasItem *item, double *x, double *y) ;

void zmapWindowCallExternal(ZMapWindow window, FooCanvasItem *item, gboolean oneType);



#endif /* !ZMAP_WINDOW_P_H */
