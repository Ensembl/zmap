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
 * Last edited: Mar  8 10:40 2005 (edgrif)
 * Created: Fri Aug  1 16:45:58 2003 (edgrif)
 * CVS info:   $Id: zmapWindow_P.h,v 1.43 2005-03-08 15:33:04 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_P_H
#define ZMAP_WINDOW_P_H

#include <gtk/gtk.h>
#include <ZMap/zmapWindow.h>
#include <ZMap/zmapFeature.h>


/* This is the name of the window config stanza. */
#define ZMAP_WINDOW_CONFIG "ZMapWindow"



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




/* Represents a column on the display. A column contains features that all have the
 * same type/style, e.g. "confirmed" transcripts. The column contains both the forward
 * and reverse strand features. */
typedef struct
{
  /* which one to use here ?? */
  gchar *type_name ;
  GQuark key_id ;

  FooCanvasItem *column_group ;				    /* The whole column. */
  FooCanvasItem *forward_group, *reverse_group ;	    /* The forward and reverse groups. */

  gboolean forward ;
  ZMapFeatureTypeStyle type ;

  double strand_gap ;					    /* space between strands. */

} ZMapWindowColumnStruct, *ZMapWindowColumn ;



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



/* Represents all blocks for a single alignment, if the alignment is ungapped it
 * will contain just one block. */
typedef struct ZMapWindowAlignmentStructName
{
  GQuark alignment_id ;

  ZMapWindow window ;					    /* our parent window. */

  FooCanvasItem *alignment_group ;			    /* Group containing all columns for
							       this alignment. */

  GData *blocks ;					    /* All blocks in the alignment. */

  double col_gap ;					    /* space between columns. */

} ZMapWindowAlignmentStruct, *ZMapWindowAlignment ;



/* Represents a single sequence display window with its scrollbars, canvas and feature
 * display. */
typedef struct _ZMapWindowStruct
{
  gchar *sequence ;

  /* Widgets for displaying the data. */
  GtkWidget     *parent_widget ;
  GtkWidget     *toplevel ;
  GtkWidget     *scrolledWindow;                            /* points to toplevel */
  FooCanvas     *canvas;				    /* where we paint the display */

  ZMapWindowCallbacks caller_cbs ;			    /* table of callbacks registered by
							     * our caller. */


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


  GData         *alignments ;

  GData         *types ;

  GPtrArray     *featureListWindows ;

  GData         *featureItems ;            /*!< enables unambiguous link between features and canvas items. */

  GData         *longItems ;               /*!< features >30k long need to be cropped as we zoom in. */


  /* This all needs to move and for scale to be in a separate window..... */
  FooCanvasItem *scaleBarGroup;           /* canvas item in which we build the scalebar */
  double         scaleBarOffset;
  int major_scale_units, minor_scale_units ;		    /* Major/minor tick marks on scale. */


  /* The length, start and end of the segment of sequence to be shown, there will be _no_
   * features outside of the start/end. */
  double         seqLength;
  double         seq_start ;
  double         seq_end ;


  ZMapFeatureTypeStyle focusType;         /* type of the item which has focus */
  gchar               *typeName;          
  FooCanvasItem       *focusFeature;      /* the item which has focus */
  GQuark               focusQuark;        /* its GQuark */
  int            DNAwidth;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapVoidIntCallbackFunc app_routine ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

} ZMapWindowStruct ;



typedef struct _ZMapWindowLongItemStruct
{
  char          *name;
  double         start;
  double         end;
  FooCanvasItem *canvasItem;
} ZMapWindowLongItemStruct, *ZMapWindowLongItem;



/*!< keyed on feature->id, gives access to canvas item */
typedef struct _ZMapFeatureItemStruct
{							    
  ZMapFeatureSet feature_set;
  GQuark         feature_key;
  FooCanvasItem *canvasItem;
  ZMapStrand     strand;
} ZMapFeatureItemStruct, *ZMapFeatureItem;



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

void zmapWindowHighlightObject(FooCanvasItem *feature,
			       ZMapWindow window, ZMapFeatureTypeStyle thisType);

void zmapWindowPrintCanvas(FooCanvas *canvas) ;
void zmapWindowPrintGroups(FooCanvas *canvas) ;
void zmapWindowPrintItem(FooCanvasGroup *item) ;


void     zMapWindowCreateListWindow(ZMapWindow window, ZMapFeatureItem featureItem);
gboolean zMapWindowFeatureClickCB(ZMapWindow window, ZMapFeature feature);

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

ZMapWindowColumn zmapWindowAlignmentAddColumn(ZMapWindowAlignmentBlock block, char *source_name,
					      ZMapFeatureTypeStyle type) ;
FooCanvasItem *zmapWindowAlignmentGetColumn(ZMapWindowColumn column_group, ZMapStrand strand) ;
void zmapWindowAlignmentHideUnhideColumns(ZMapWindowAlignmentBlock block) ;

ZMapWindowAlignment zmapWindowAlignmentDestroy(ZMapWindowAlignment alignment) ;


#endif /* !ZMAP_WINDOW_P_H */
