/*  File: zmapWindow.h
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Defines interface to code that creates/handles a
 *              window displaying genome data.
 *              
 * HISTORY:
 * Last edited: Jun 30 16:19 2005 (rds)
 * Created: Thu Jul 24 15:21:56 2003 (edgrif)
 * CVS info:   $Id: zmapWindow.h,v 1.39 2005-06-30 15:19:42 rds Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_H
#define ZMAP_WINDOW_H

#include <glib.h>
/* SHOULD CANVAS BE HERE...MAYBE, MAYBE NOT...... */
#include <libfoocanvas/libfoocanvas.h>

#include <ZMap/zmapSys.h>		       /* For callback funcs... */
#include <ZMap/zmapFeature.h>


/*! @addtogroup zmapwindow
 * @{
 *  */


/* Opaque type, represents an individual ZMap window. */
typedef struct _ZMapWindowStruct *ZMapWindow ;



/* indicates how far the zmap is zoomed, n.b. ZMAP_ZOOM_FIXED implies that the whole sequence
 * is displayed at the maximum zoom. */
typedef enum {ZMAP_ZOOM_INIT, ZMAP_ZOOM_MIN, ZMAP_ZOOM_MID, ZMAP_ZOOM_MAX,
	      ZMAP_ZOOM_FIXED} ZMapWindowZoomStatus ;

/* Should the original window and the new window be locked together for scrolling and zooming.
 * vertical means that the vertical scrollbars should be locked together, specifying vertical
 * or horizontal means locking of zoom as well. */
typedef enum {ZMAP_WINLOCK_NONE, ZMAP_WINLOCK_VERTICAL, ZMAP_WINLOCK_HORIZONTAL} ZMapWindowLockType ;



/* Data returned to the visibilityChange callback routine. */
typedef struct
{
  ZMapWindowZoomStatus zoom_status ;

  /* Top/bottom coords for section of sequence that can be scrolled currently in window. */
  double scrollable_top ;
  double scrollable_bot ;

} ZMapWindowVisibilityChangeStruct, *ZMapWindowVisibilityChange ;


/* Data returned to the visibilityChange callback routine. */
typedef struct
{
  char *text ;						    /* Describes selected item. */

  FooCanvasItem *item ;					    /* The feature selected, may be null
							       if a column was selected. */

} ZMapWindowSelectStruct, *ZMapWindowSelect ;


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
  ZMapWindowCallbackFunc setZoomStatus;
  ZMapWindowCallbackFunc visibilityChange ;
  ZMapWindowCallbackFunc destroy ;
} ZMapWindowCallbacksStruct, *ZMapWindowCallbacks ;



/*! Callback function for menu items. The id indicates which menu_item was selected
 *  resulting in the call to this callback. */
typedef void (*ZMapWindowMenuItemCallbackFunc)(int menu_item_id, gpointer callback_data) ;

/*!
 * Defines a menu item. */
typedef struct
{
  char *name ;						    /*!< Title string of menu item. */
  int id ;						    /*!< Number uniquely identifying this
							      menu item within a menu. */
  ZMapWindowMenuItemCallbackFunc callback_func ;	    /*!< Function to call when this item
							      is selected.  */
  gpointer callback_data ;				    /*!< Data to pass to callback function. */
} ZMapWindowMenuItemStruct, *ZMapWindowMenuItem ;


typedef struct _ZMapWindowFeatureQueryStruct
{
  char *name;                   /* Feature name */
  char *alignment;              /* Alignment */
  char *block;
  int query_start, query_end;   /* Query start and end */
  int target_start, target_end; /* Target start and end */
  ZMapFeatureType type;         /* Feature type */
  ZMapStrand strand;            /* Feature strand */
  char *style;                  /* Feature style */
  /* Whether these have been set */
  gboolean alignment_set;
  gboolean style_set;          
  gboolean strand_set;
} ZMapWindowFeatureQueryStruct, *ZMapWindowFeatureQuery;


/*! @} end of zmapwindow docs. */



void zMapWindowInit(ZMapWindowCallbacks callbacks) ;
ZMapWindow zMapWindowCreate(GtkWidget *parent_widget, char *sequence, void *app_data) ;
ZMapWindow zMapWindowCopy(GtkWidget *parent_widget, char *sequence, 
			  void *app_data, ZMapWindow old,
			  ZMapFeatureContext features, ZMapWindowLockType window_locking) ;
void zMapWindowDisplayData(ZMapWindow window,
			   ZMapFeatureContext current_features, ZMapFeatureContext new_features) ;
void zMapWindowZoom(ZMapWindow window, double zoom_factor) ;
void zMapWindowMove(ZMapWindow window, double start, double end) ;
void zMapWindowReset(ZMapWindow window) ;
GtkWidget *zMapWindowGetWidget(ZMapWindow window);

/* this looks like it needs some sorting out ! */
ZMapWindowZoomStatus zMapWindowGetZoomStatus(ZMapWindow window) ;
void zMapWindowSetZoomStatus(ZMapWindow window) ;
double zMapWindowGetZoomFactor(ZMapWindow window);
void zMapWindowSetZoomFactor(ZMapWindow window, double zoom_factor);
void zMapWindowSetMinZoom   (ZMapWindow window);

void zMapWindowGetVisible(ZMapWindow window, double *top_out, double *bottom_out) ;

FooCanvasItem *zMapWindowFindFeatureItemByName(ZMapWindow window, char *style,
					       ZMapFeatureType feature_type, char *feature_name,
					       ZMapStrand strand, int start, int end,
					       int query_start, int query_end) ;
FooCanvasItem *zMapWindowFindFeatureItemByQuery(ZMapWindow window, ZMapWindowFeatureQuery ft_q);

FooCanvasItem *zMapWindowFindFeatureItemByItem(ZMapWindow window, FooCanvasItem *item) ;

void zMapWindowScrollToWindowPos(ZMapWindow window, int window_y_pos) ;

gboolean zMapWindowScrollToItem(ZMapWindow window, FooCanvasItem *feature_item) ;

void zMapWindowHighlightObject(ZMapWindow window, FooCanvasItem *feature) ;


void zMapWindowDestroyLists(ZMapWindow window) ;

void zMapWindowUnlock(ZMapWindow window) ;

void zMapWindowDestroy(ZMapWindow window) ;

/* this may be better in a utils directory...not sure.... */
void zMapWindowMakeMenu(char *menu_title, ZMapWindowMenuItemStruct menu_items[],
			GdkEventButton *button_event) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* TEST SCAFFOLDING............... */
ZMapFeatureContext testGetGFF(void) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#endif /* !ZMAP_WINDOW_H */
