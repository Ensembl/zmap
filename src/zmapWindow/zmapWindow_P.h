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
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Oct 20 14:10 2004 (edgrif)
 * Created: Fri Aug  1 16:45:58 2003 (edgrif)
 * CVS info:   $Id: zmapWindow_P.h,v 1.23 2004-10-20 13:13:03 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_P_H
#define ZMAP_WINDOW_P_H

#include <gtk/gtk.h>
#include <ZMap/zmapWindow.h>


/* Test scaffoling */
#include <ZMap/zmapFeature.h>

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
  int          step_increment;

  GtkWidget *text ;

  GdkAtom zmap_atom ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapVoidIntCallbackFunc app_routine ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  void *app_data ;


  /* Is this even used ??????? */
  InvarCoord      origin; /* that base which is VisibleCoord 1 */

  GtkWidget *featureListWindow;


} ZMapWindowStruct ;


typedef struct
{
  ZMapWindow window ;
  void *data ;						    /* void for now, union later ?? */
  GData *types;                         
} zmapWindowDataStruct, *zmapWindowData ;


/* used in handleCanvasEvent to obtain the actual feature that's been clicked on */
typedef struct _FeatureKeys {
    ZMapFeatureSet feature_set;
    GQuark context_key;
    GQuark feature_key;
} FeatureKeyStruct, *FeatureKeys;


/* Used in our event communication.... */
#define ZMAP_ATOM  "ZMap_Atom"


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

  GData               *types;
  ZMapFeatureTypeStyle thisType;

  ZMapFeatureContext   feature_context;
  ZMapFeatureSet       feature_set;
  GQuark               context_key;
  FooCanvasItem       *feature_group;       /* the group this feature was drawn in */
  ZMapFeature          feature;

  ZMapFeatureTypeStyle focusType;
  FooCanvasItem       *focusFeature;

  GIOChannel          *channel;

  /* The length, start and end of the segment of sequence to be shown, there will be _no_
   * features outside of the start/end. */
  double               seqLength;
  double               seq_start ;
  double               seq_end ;

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



void zmapFeatureInit(ZMapFeatureCallbacks callbacks) ;


#endif /* !ZMAP_WINDOW_P_H */
