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
 * Last edited: Nov 12 14:42 2004 (edgrif)
 * Created: Fri Aug  1 16:45:58 2003 (edgrif)
 * CVS info:   $Id: zmapWindow_P.h,v 1.31 2004-11-12 14:45:57 edgrif Exp $
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
    ZMAP_WINDOW_MAX_WINDOW = 30000,			    /* Largest canvas window. */
    ZMAP_WINDOW_TEXT_BORDER = 2				    /* border above/below dna text. */
  } ;


typedef struct _ZMapWindowStruct
{
  gchar *sequence ;

  /* Widgets for displaying the data. */
  GtkWidget *parent_widget ;
  GtkWidget *toplevel ;


  GtkWidget   *scrolledWindow;
  FooCanvas   *canvas;					    /* where we paint the display */
  FooCanvasItem *group;
  GtkWidget   *combo;
  int          basesPerLine;
  InvarCoord   centre;
  int          graphHeight;
  int          dragBox, scrollBox;
  GPtrArray    cols;
  GArray       *box2seg, *box2col;

  int          DNAwidth;

  double       zoom_factor ;
  ZMapWindowZoomStatus zoom_status ;			    /* For short sequences that are
							       displayed at max. zoom initially. */

  int          canvas_maxwin_size ;			    /* 30,000 is the maximum (default). */

  int          step_increment;
  int          page_increment;

  GtkWidget *text ;

  GdkAtom zmap_atom ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapVoidIntCallbackFunc app_routine ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  void *app_data ;


  /* Is this even used ??????? */
  InvarCoord      origin; /* that base which is VisibleCoord 1 */

  GPtrArray *featureListWindows;

  GData     *featureItems;            /*!< enables unambiguous link between features and canvas items. */

} ZMapWindowStruct ;


typedef struct _ZMapFeatureItemStruct {   /*!< keyed on feature->id, gives access to canvas item */
  ZMapFeatureSet feature_set;
  FooCanvasItem *canvasItem;
} ZMapFeatureItemStruct, *ZMapFeatureItem;


typedef struct
{
  ZMapWindow window ;
  void *data ;						    /* void for now, union later ?? */
  GData *types;                         
} zmapWindowDataStruct, *zmapWindowData ;


/* used in handleCanvasEvent to obtain the actual feature that's been clicked on */
typedef struct _FeatureKeys {
    ZMapFeatureSet feature_set;
    GQuark feature_key;
} FeatureKeyStruct, *FeatureKeys;


/* Used in our event communication.... */
#define ZMAP_ATOM  "ZMap_Atom"

typedef struct _ZMapColStruct
{
  FooCanvasItem       *item;
  gboolean             forward;
  ZMapFeatureTypeStyle type;
  gchar               *type_name;
} ZMapColStruct, *ZMapCol;


/* parameters passed between the various functions processing the features on the canvas */
/* I wanta to group the members logically, but I don't think I'm there yet. */
typedef struct _ZMapCanvasDataStruct
{
  ZMapWindow           window;
  FooCanvas           *canvas;
  FooCanvasItem       *columnGroup;
  FooCanvasItem       *revColGroup;         /* a group for reverse strand features */
  double               column_position;
  double               revColPos;           /* column position on reverse strand */

  double min_zoom ;					    /* min/max allowable zoom. */
  double max_zoom ;


  GData               *types;
  ZMapFeatureTypeStyle thisType;

  ZMapFeatureContext   feature_context;
  ZMapFeatureSet       feature_set;
  GQuark               context_key;
  FooCanvasItem       *feature_group;       /* the group this feature was drawn in */
  ZMapFeature          feature;

  GArray              *columns;             /* keep track of canvas columns */

  ZMapFeatureTypeStyle focusType;
  FooCanvasItem       *focusFeature;

  GIOChannel          *channel;

  /* The length, start and end of the segment of sequence to be shown, there will be _no_
   * features outside of the start/end. */
  double               seqLength;
  double               seq_start ;
  double               seq_end ;

  int border_pixels ;					    /* top/bottom border to sequence. */

  FooCanvasItem       *scaleBarGroup;
  double               scaleBarOffset;
  double               x;                   /* x coord of a column the user clicked */
  gboolean             reduced;             /* keep track of scroll region reduction */	
  gboolean             atLimit;			     
} ZMapCanvasDataStruct;


/* the function to be ultimately called when the user clicks on a canvas item. */
typedef gboolean (*ZMapFeatureCallbackFunc)(ZMapCanvasDataStruct *canvasData, ZMapFeatureSet feature_set);


/* Set of callback routines that allow the caller to be notified when events happen
 * to a feature. */
typedef struct _ZMapFeatureCallbacksStruct
{
  ZMapWindowFeatureCallbackFunc click ;
  ZMapFeatureCallbackFunc       rightClick;
} ZMapFeatureCallbacksStruct, *ZMapFeatureCallbacks ;



GtkWidget *zmapWindowMakeMenuBar(ZMapWindow window) ;
GtkWidget *zmapWindowMakeButtons(ZMapWindow window) ;
GtkWidget *zmapWindowMakeFrame(ZMapWindow window) ;



void zmapWindowHighlightObject(FooCanvasItem *feature, ZMapCanvasDataStruct *canvasData) ;

void zmapFeatureInit(ZMapFeatureCallbacks callbacks) ;

void zmapWindowPrintCanvas(FooCanvas *canvas) ;

void zMapWindowCreateListWindow(ZMapCanvasDataStruct *canvasData, ZMapFeatureSet feature_set);
gboolean zMapFeatureClickCB  (ZMapCanvasDataStruct *canvasData, ZMapFeature feature);
FooCanvasItem *zmapDrawScale(FooCanvas *canvas,
			     double offset, double zoom_factor, 
			     int start, int end);


#endif /* !ZMAP_WINDOW_P_H */
