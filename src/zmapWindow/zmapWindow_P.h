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
 * Last edited: Dec 16 15:27 2004 (edgrif)
 * Created: Fri Aug  1 16:45:58 2003 (edgrif)
 * CVS info:   $Id: zmapWindow_P.h,v 1.37 2004-12-20 10:59:50 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_P_H
#define ZMAP_WINDOW_P_H

#include <gtk/gtk.h>
#include <ZMap/zmapWindow.h>


/* Test scaffoling */
#include <ZMap/zmapFeature.h>


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#define PIXELS_PER_BASE 20.0   /* arbitrary text size to limit zooming in.  Must be tied
			       ** in to actual text size dynamically some time soon. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* This is the name of the window config stanza. */
#define ZMAP_WINDOW_CONFIG "ZMapWindow"

enum
  {
    ZMAP_WINDOW_TEXT_BORDER = 2,			    /* border above/below dna text. */
    ZMAP_WINDOW_STEP_INCREMENT = 10,                        /* scrollbar stepping increment */
    ZMAP_WINDOW_PAGE_INCREMENT = 600,                       /* scrollbar paging increment */
    ZMAP_WINDOW_MAX_WINDOW = 30000			    /* Largest canvas window. */
  } ;




typedef struct _ZMapWindowStruct
{
  gchar *sequence ;

  /* Widgets for displaying the data. */
  GtkWidget     *parent_widget ;
  GtkWidget     *toplevel ;
  GtkWidget     *scrolledWindow;                            /* points to toplevel */
  FooCanvas     *canvas;				    /* where we paint the display */

  double         zoom_factor ;
  ZMapWindowZoomStatus zoom_status ;   /* For short sequences that are displayed at max. zoom initially. */
  double         min_zoom ;				    /* min/max allowable zoom. */
  double         max_zoom ;
  int            canvas_maxwin_size ;			    /* 30,000 is the maximum (default). */
  int            border_pixels ;			    /* top/bottom border to sequence. */
  double         text_height;                               /* used to calculate min/max zoom */
  double         current_x;                                /* for scrolling a split window to */
  double         current_y;                                /* the same place as its sister. */
  double         scroll_x1;                                
  double         scroll_y1;                                
  double         scroll_x2;                                
  double         scroll_y2;                                

  GtkWidget     *text ;
  GdkAtom        zmap_atom ;
  void          *app_data ;
  gulong         realizeHandlerCB;

  GData         *types;
  GPtrArray     *featureListWindows;
  GData         *featureItems;            /*!< enables unambiguous link between features and canvas items. */
  GData         *longItems;               /*!< features >30k long need to be cropped as we zoom in. */
  GPtrArray     *columns;                 /* keep track of canvas columns */

  FooCanvasItem *scaleBarGroup;           /* canvas item in which we build the scalebar */
  double         scaleBarOffset;

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



typedef struct _ZMapWindowLongItemStruct {
  char          *name;
  double         start;
  double         end;
  FooCanvasItem *canvasItem;
} ZMapWindowLongItemStruct, *ZMapWindowLongItem;


typedef struct _ZMapFeatureItemStruct {   /*!< keyed on feature->id, gives access to canvas item */
  ZMapFeatureSet feature_set;
  GQuark         feature_key;
  FooCanvasItem *canvasItem;
  ZMapStrand     strand;
} ZMapFeatureItemStruct, *ZMapFeatureItem;


typedef struct
{
  ZMapWindow window ;
  void *data ;						    /* void for now, union later ?? */
  GData *types;                         
} zmapWindowDataStruct, *zmapWindowData ;



/* Used in our event communication.... */
#define ZMAP_ATOM  "ZMap_Atom"

typedef struct _ZMapColStruct
{
  FooCanvasItem       *column;
  gboolean             forward;
  ZMapFeatureTypeStyle type;
  gchar               *type_name;
} ZMapColStruct, *ZMapCol;


/* parameters passed between the various functions drawing the features on the canvas */
typedef struct _ZMapCanvasDataStruct
{
  ZMapWindow           window;
  FooCanvas           *canvas;
  FooCanvasItem       *forward_column_group;
  FooCanvasItem       *reverse_column_group; 
  double               forward_column_pos;
  double               reverse_column_pos;  
  ZMapFeatureContext   feature_context;
  ZMapFeatureSet       feature_set;
  GQuark               context_key;
  ZMapFeatureTypeStyle thisType;

} ZMapCanvasDataStruct, *ZMapCanvasData;


/* the function to be ultimately called when the user clicks on a canvas item. */
typedef gboolean (*ZMapFeatureCallbackFunc)(ZMapWindow window, ZMapFeatureItem featureItem);


/* Set of callback routines that allow the caller to be notified when events happen
 * to a feature. */
typedef struct _ZMapFeatureCallbacksStruct
{
  ZMapWindowCallbackFunc click ;
  ZMapFeatureCallbackFunc       rightClick;
} ZMapFeatureCallbacksStruct, *ZMapFeatureCallbacks ;



GtkWidget *zmapWindowMakeMenuBar(ZMapWindow window) ;
GtkWidget *zmapWindowMakeButtons(ZMapWindow window) ;
GtkWidget *zmapWindowMakeFrame(ZMapWindow window) ;



void zmapWindowHighlightObject(FooCanvasItem *feature, ZMapWindow window, ZMapFeatureTypeStyle thisType);

void zmapFeatureInit(ZMapFeatureCallbacks callbacks) ;

void zmapWindowPrintCanvas(FooCanvas *canvas) ;

void     zMapWindowCreateListWindow(ZMapWindow window, ZMapFeatureItem featureItem);
gboolean zMapWindowFeatureClickCB(ZMapWindow window, ZMapFeature feature);
FooCanvasItem *zmapDrawScale    (FooCanvas *canvas, double offset, double zoom_factor, int start, int end);
double zmapWindowCalcZoomFactor (ZMapWindow window);
void   zmapWindowSetPageIncr    (ZMapWindow window);
void   zmapWindowCropLongFeature(GQuark quark, gpointer data, gpointer user_data);


#endif /* !ZMAP_WINDOW_P_H */
