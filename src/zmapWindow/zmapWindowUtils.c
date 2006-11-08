/*  File: zmapWindowUtils.c
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Utility functions for the zMapWindow code.
 *              
 * Exported functions: See ZMap/zmapWindow.h
 * HISTORY:
 * Last edited: Oct 11 09:29 2006 (rds)
 * Created: Thu Jan 20 14:43:12 2005 (edgrif)
 * CVS info:   $Id: zmapWindowUtils.c,v 1.35 2006-11-08 09:25:32 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <math.h>
#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>



/* Struct for style table callbacks. */
typedef struct
{
  ZMapWindowStyleTableCallback func ;
  gpointer data ;
} ZMapWindowStyleTableDataStruct, *ZMapWindowStyleTableData ;




static void styleDestroyCB(gpointer data) ;
static void styleTableHashCB(gpointer key, gpointer value, gpointer user_data) ;




/* A couple of simple coord calculation routines, if these prove too expensive they
 * can be replaced with macros. */


/* Users sometimes want to see coords from a different origin e.g. perhaps as though for the
 * forward strand even though they have revcomped the sequence. */
int zmapWindowCoordFromOrigin(ZMapWindow window, int start)
{
  int new_start ;

  new_start = start - window->origin + 1 ;

  return new_start ;
}

/* Use if you have no window.... */
int zmapWindowCoordFromOriginRaw(int origin, int start)
{
  int new_start ;

  new_start = start - origin + 1 ;

  return new_start ;
}




/* This is the basic length calculation, obvious, but the "+ 1" is constantly overlooked. */
double zmapWindowExt(double start, double end)
{
  double extent ;

  zMapAssert(start <= end) ;

  extent = end - start + 1 ;

  return extent ;
}

/* Converts a sequence extent into a canvas extent.
 *
 * Less obvious as it covers the following slightly subtle problem:
 * 
 * sequence coords:           1  2  3  4  5  6  7  8                                         
 *                                                                                           
 * canvas coords:            |__|__|__|__|__|__|__|__|                                       
 *                                                                                           
 *                           |                       |                                       
 *                          1.0                     9.0                                      
 *                                                                                           
 * i.e. when we actually come to draw it we need to go one _past_ the sequence end           
 * coord because our drawing needs to draw in the whole of the last base.                    
 * 
 */
void zmapWindowSeq2CanExt(double *start_inout, double *end_inout)
{
  zMapAssert(start_inout && end_inout && *start_inout <= *end_inout) ;

  *end_inout = *end_inout + 1 ;

  return ;
}

/* Converts a start/end pair of coords into a zero based pair of coords.
 *
 * For quite a lot of the canvas group stuff we need to take two coords defining a range
 * in some kind of parent system and convert that range into the same range but starting at zero,
 * e.g.  range  3 -> 6  becomes  0 -> 3  */
void zmapWindowExt2Zero(double *start_inout, double *end_inout)
{
  zMapAssert(start_inout && end_inout && *start_inout <= *end_inout) ;

  *end_inout = *end_inout - *start_inout ;		    /* do this first before zeroing start ! */

  *start_inout = 0.0 ;

  return ;
}


/* Converts a sequence extent into a zero based canvas extent.
 *
 * Combines zmapWindowSeq2CanExt() and zmapWindowExt2Zero(), used in positioning
 * groups/features a lot because in the canvas item coords are relative to their parent group
 * and hence zero-based. */
void zmapWindowSeq2CanExtZero(double *start_inout, double *end_inout)
{
  zMapAssert(start_inout && end_inout && *start_inout <= *end_inout) ;

  *end_inout = *end_inout - *start_inout ;		    /* do this first before zeroing start ! */

  *start_inout = 0.0 ;

  *end_inout = *end_inout + 1 ;

  return ;
}


/* NOTE: offset may not be quite what you think, the routine recalculates */
void zmapWindowSeq2CanOffset(double *start_inout, double *end_inout, double offset)
{
  zMapAssert(start_inout && end_inout && *start_inout <= *end_inout) ;

  *start_inout -= offset ;

  *end_inout -= offset ;

  *end_inout += 1 ;

  return ;
}


ZMapGUIClampType zmapWindowClampedAtStartEnd(ZMapWindow window, double *top_inout, double *bot_inout)
{
  ZMapGUIClampType clamp_type = ZMAPGUI_CLAMP_INIT;

  clamp_type = zMapGUICoordsClampToLimits(window->min_coord, window->max_coord, 
                                          top_inout, bot_inout);

  return clamp_type;                 /* ! */

#ifdef RDS_DONT_INCLUDE
  double top, bot;

  top = *top_inout;
  bot = *bot_inout;

  if (top < window->min_coord)
    {
      top    = window->min_coord ;
      clamp |= ZMAP_WINDOW_CLAMP_START;
    }
  else if (floor(top) == window->min_coord)
    clamp |= ZMAP_WINDOW_CLAMP_START;

  if (bot > window->max_coord)
    {
      bot    = window->max_coord ;
      clamp |= ZMAP_WINDOW_CLAMP_END;
    }
  else if (ceil(bot) == window->max_coord)
    clamp |= ZMAP_WINDOW_CLAMP_END;
    
  *top_inout = top;
  *bot_inout = bot;

  return clamp;  
#endif
}

/* Clamps the span within the length of the sequence,
 * possibly shifting the span to keep it the same size. */
ZMapGUIClampType zmapWindowClampSpan(ZMapWindow window, double *top_inout, double *bot_inout)
{
  ZMapGUIClampType clamp = ZMAPGUI_CLAMP_INIT;

  clamp = zMapGUICoordsClampSpanWithLimits(window->min_coord, 
                                           window->max_coord, 
                                           top_inout, bot_inout);

#ifdef RDS_DONT_INCLUDE
  double top, bot;
  top = *top_inout;
  bot = *bot_inout;

  if (top < window->min_coord)
    {
      if ((bot = bot + (window->min_coord - top)) > window->max_coord)
        {
          bot    = window->max_coord ;
          clamp |= ZMAP_WINDOW_CLAMP_END;
        }
      clamp |= ZMAP_WINDOW_CLAMP_START;
      top    = window->min_coord ;
    }
  else if (bot > window->max_coord)
    {
      if ((top = top - (bot - window->max_coord)) < window->min_coord)
        {
          clamp |= ZMAP_WINDOW_CLAMP_START;
          top    = window->min_coord ;
        }
      clamp |= ZMAP_WINDOW_CLAMP_END;
      bot    = window->max_coord ;
    }

  *top_inout = top;
  *bot_inout = bot;
#endif
  return clamp;
}


/* We have some arrays of additional windows that are popped up as a result of user interaction
 * with the zmap, these need to cleaned up when the zmap goes away.
 * 
 * If window_array points to NULL then function just returns. */
void zmapWindowFreeWindowArray(GPtrArray **window_array_inout, gboolean free_array)
{
  GPtrArray *window_array ;

  zMapAssert(window_array_inout) ;

  if ((window_array = *window_array_inout))
    {
      /* Slightly tricky here, when the list widget gets destroyed it removes itself
       * from the array so the array shrinks by one, hence we just continue to access
       * the first element until all elements are gone. */
      while (window_array->len)
	{
	  GtkWidget *widget ;

	  widget = g_ptr_array_index(window_array, 0) ;

	  gtk_widget_destroy(widget) ;
	}

      if (free_array)
	{
	  g_ptr_array_free(window_array, FALSE) ;
	  *window_array_inout = NULL ;
	}
    }

  return ;
}






void zMapWindowGetVisible(ZMapWindow window, double *top_out, double *bottom_out)
{
  double scroll_x1, scroll_y1, scroll_x2, scroll_y2 ;

  foo_canvas_get_scroll_region(window->canvas, &scroll_x1, &scroll_y1, &scroll_x2, &scroll_y2) ;

  *top_out = scroll_y1 ;
  *bottom_out = scroll_y2 ;

  return ;
}



/* Set of functions for managing hash tables of styles for features. Each column in zmap has
 * a hash table of styles for all the features it displays. */

GHashTable *zmapWindowStyleTableCreate(void)
{
  GHashTable *style_table = NULL ;

  style_table = g_hash_table_new_full(NULL, NULL, NULL, styleDestroyCB) ;

  return style_table ;
}


/* Adds a style to the list of feature styles using the styles unique_id as the key.
 * NOTE that if the style is already there it is not added as this would overwrite
 * the existing style. */
gboolean zmapWindowStyleTableAdd(GHashTable *style_table, ZMapFeatureTypeStyle new_style)
{
  gboolean result = FALSE ;
  ZMapFeatureTypeStyle style ;

  zMapAssert(style_table && new_style) ;

  if (!(style = g_hash_table_lookup(style_table, GINT_TO_POINTER(new_style->unique_id))))
    {
      g_hash_table_insert(style_table, GINT_TO_POINTER(new_style->unique_id), new_style) ;

      result = TRUE ;
    }

  return result ;
}


/* Finds a feature style using the styles unique_id. */
ZMapFeatureTypeStyle zmapWindowStyleTableFind(GHashTable *style_table, GQuark style_id)
{
  ZMapFeatureTypeStyle style ;

  zMapAssert(style_table && style_id) ;

  style = g_hash_table_lookup(style_table, GINT_TO_POINTER(style_id)) ;

  return style ;
}


/* Calls the app_func for each of the styles in the table. */
void zmapWindowStyleTableForEach(GHashTable *style_table,
				 ZMapWindowStyleTableCallback app_func, gpointer app_data)
{
  ZMapWindowStyleTableDataStruct table_data ;

  zMapAssert(style_table && app_func && app_data) ;

  table_data.func = app_func ;
  table_data.data = app_data ;

  /* Call the users style processing function. */
  g_hash_table_foreach(style_table, styleTableHashCB, &table_data) ;


  return ;
}

/* Destroys the style table. */
void zmapWindowStyleTableDestroy(GHashTable *style_table)
{
  zMapAssert(style_table) ;

  g_hash_table_destroy(style_table) ;

  return ;
}



/* 
 *                  Internal routines.
 */


/* Called when a style is destroyed and the styles hash of feature_styles is destroyed. */
static void styleDestroyCB(gpointer data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;

  zMapFeatureTypeDestroy(style) ;

  return ;
}



/* A GHFunc() callback, used by zmapWindowStyleTableForEach() to call the applications
 * style callback. */
static void styleTableHashCB(gpointer key, gpointer value, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)value ;
  ZMapWindowStyleTableData style_data = (ZMapWindowStyleTableData)user_data ;

  /* Call the apps function passing in the current style and the apps data. */
  (style_data->func)(style, style_data->data) ;

  return ;
}
