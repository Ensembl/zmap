/*  File: zmapWindowUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 * Last edited: Jan 20 17:03 2005 (edgrif)
 * Created: Thu Jan 20 14:43:12 2005 (edgrif)
 * CVS info:   $Id: zmapWindowUtils.c,v 1.1 2005-01-24 13:22:19 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>





static void checkScrollRegion(ZMapWindow window, double start, double end) ;



void zMapWindowGetVisible(ZMapWindow window, double *top_out, double *bottom_out)
{
  double scroll_x1, scroll_y1, scroll_x2, scroll_y2 ;

  foo_canvas_get_scroll_region(window->canvas, &scroll_x1, &scroll_y1, &scroll_x2, &scroll_y2) ;

  *top_out = scroll_y1 ;
  *bottom_out = scroll_y2 ;

  return ;
}



/** \Brief Scroll to the selected item.
 * 
 * If necessary, recalculate the scroll region, then scroll to the item
 * and highlight it.
 */
gboolean zMapWindowScrollToItem(ZMapWindow window, gchar *type, GQuark feature_id) 
{
  int cx, cy, height;
  double x1, y1, x2, y2;
  ZMapFeatureItem featureItem = NULL;
  ZMapFeature feature;
  ZMapFeatureTypeStyle thisType;
  gboolean result;
  G_CONST_RETURN gchar *quarkString;
  GtkWidget *topWindow;

  if (!(quarkString = g_quark_to_string(feature_id)))
    {
      zMapLogWarning("Quark %d, of type %s, not a valid quark\n", feature_id, type) ;
      result = FALSE;
    }
  else
    {
      if ((featureItem = (ZMapFeatureItem)g_datalist_id_get_data(&(window->featureItems), feature_id)))
	{
	  feature = (ZMapFeature)g_datalist_id_get_data(&(featureItem->feature_set->features),
							featureItem->feature_key) ;
	  zMapAssert(feature) ;
	  
	  thisType = (ZMapFeatureTypeStyle)g_datalist_get_data(&(window->types), type);
	  zMapAssert(thisType);
	  
	  topWindow = gtk_widget_get_toplevel(GTK_WIDGET(window->canvas));
	  gtk_widget_show_all(topWindow);


	  /* featureItem holds canvasItem and ptr to the feature_set containing the feature. */
	  featureItem = g_datalist_id_get_data(&(window->featureItems), feature_id);
	  feature = g_datalist_id_get_data(&(featureItem->feature_set->features), feature_id);


	  /* May need to move scroll region if object is outside it. */
	  checkScrollRegion(window, feature->x1, feature->x2) ;


	  /* scroll up or down to user's selection. */
	  foo_canvas_item_get_bounds(featureItem->canvasItem, &x1, &y1, &x2, &y2); /* world coords */
	  
	  if (y1 <= 0.0)    /* we might be dealing with a multi-box item, eg transcript */
	    {
	      double px1, py1, px2, py2;
	      
	      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(featureItem->canvasItem->parent),
					 &px1, &py1, &px2, &py2); 
	      if (py1 > 0.0)
		y1 = py1;
	    }
	  
	  /* Note that because we zoom asymmetrically, we only convert the y coord 
	   * to canvas coordinates, leaving the x as is.  */
	  foo_canvas_w2c(window->canvas, 0.0, y1, &cx, &cy); 

	  height = GTK_WIDGET(window->canvas)->allocation.height;
	  foo_canvas_scroll_to(window->canvas, (int)x1, cy - height/3);             /* canvas pixels */
	  
	  foo_canvas_item_raise_to_top(featureItem->canvasItem);
	  
	  /* highlight the item */
	  zmapWindowHighlightObject(featureItem->canvasItem, window, thisType);
	  
	  zMapWindowFeatureClickCB(window, feature) ;	    /* show feature details on info_panel  */
	  
	  window->focusFeature = featureItem->canvasItem;
	  window->focusType = thisType;

	  result =  TRUE;
	}

    }  /* else silently ignore the fact we've not found it. */

  return result;
}



/** \Brief Recalculate the scroll region.
 *
 * If the selected feature is outside the current scroll region, recalculate
 * the region to be the same size but with the selecte feature in the middle.
 */
static void checkScrollRegion(ZMapWindow window, double start, double end)
{
  double x1, y1, x2, y2 ;


  /* NOTE THAT THIS ROUTINE NEEDS TO CALL THE VISIBILITY CHANGE CALLBACK IF WE MOVE
   * THE SCROLL REGION TO MAKE SURE THAT ZMAPCONTROL UPDATES ITS SCROLLBARS.... */

  foo_canvas_get_scroll_region(window->canvas, &x1, &y1, &x2, &y2);  /* world coords */
  if ( start < y1 || end > y2)
    {
      ZMapWindowVisibilityChangeStruct vis_change ;
      int top, bot ;
      double height ;


      height = y2 - y1;

      y1 = start - (height / 2.0) ;

      if (y1 < window->seq_start)
	y1 = window->seq_start;

      y2 = y1 + height;

      /* this shouldn't happen */
      if (y2 > window->seq_end)
	y2 = window->seq_end ;


      foo_canvas_set_scroll_region(window->canvas, x1, y1, x2, y2);


      /* UGH, I'M NOT SURE I LIKE THE LOOK OF ALL THIS INT CONVERSION STUFF.... */

      /* redraw the scale bar */
      top = (int)y1;                   /* zmapDrawScale expects integer coordinates */
      bot = (int)y2;
      gtk_object_destroy(GTK_OBJECT(window->scaleBarGroup));
      window->scaleBarGroup = zmapDrawScale(window->canvas, 
					    window->scaleBarOffset, 
					    window->zoom_factor,
					    top, bot,
					    &(window->major_scale_units),
					    &(window->minor_scale_units)) ;

      /* agh, this seems to be here because we move the scroll region...we need a function
       * to do this all....... */
      if (window->longItems)
	g_datalist_foreach(&(window->longItems), zmapWindowCropLongFeature, window);




      /* Call the visibility change callback to notify our caller that our zoom/position has
       * changed. */
      vis_change.zoom_status = window->zoom_status ;
      vis_change.scrollable_top = y1 ;
      vis_change.scrollable_bot = y2 ;
      (*(window->caller_cbs->visibilityChange))(window, window->app_data, (void *)&vis_change) ;

    }


  return ;
}


