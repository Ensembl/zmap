/*  File: zmapWindowAlignment.c
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Feb 25 16:30 2005 (edgrif)
 * Created: Thu Feb 24 11:19:23 2005 (edgrif)
 * CVS info:   $Id: zmapWindowAlignment.c,v 1.1 2005-02-25 16:46:07 edgrif Exp $
 *-------------------------------------------------------------------
 */


#include <zmapWindow_P.h>


static ZMapWindowColumn findColumn(GPtrArray *col_array, char *col_name, gboolean forward_strand) ;
static ZMapWindowColumn createColumn(ZMapWindowAlignment alignment, gboolean forward, 
				     double position, gchar *type_name, ZMapFeatureTypeStyle type) ;

static gboolean canvasColumnEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static gboolean canvasBoundingBoxEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;




/* In the end we will need position information here.... */
ZMapWindowAlignment zmapWindowAlignmentCreate(char *align_name, ZMapWindow window,
					      FooCanvasGroup *parent_group)
{
  ZMapWindowAlignment alignment ;

  alignment = g_new0(ZMapWindowAlignmentStruct, 1) ;

  alignment->alignment_id = g_quark_from_string(align_name) ;

  alignment->window = window ;

  alignment->alignment_group = foo_canvas_item_new(parent_group,
						   foo_canvas_group_get_type(),
						   "x", 0.0,
						   "y", 0.0,
						   NULL) ;

  alignment->col_gap = 5.0 ;				    /* should be user settable... */

  alignment->columns = g_ptr_array_new() ;

  return alignment ;
}


FooCanvasItem *zmapWindowAlignmentAddColumn(ZMapWindowAlignment alignment, char *source_name,
					    double position, ZMapFeatureTypeStyle type)
{
  FooCanvasItem *col_group ;
  ZMapWindowColumn column ;

  /* when we come to add a column we should be able to assume that we can just position it
   * past the last column.... */

  /* If the column doesn't already exist we need to find a new one. */
  if (!(column = findColumn(alignment->columns, source_name, TRUE)))
    {
      column = createColumn(alignment, TRUE, position, source_name, type) ;

      g_ptr_array_add(alignment->columns, column) ;
    }


  col_group = column->column_group ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  canvas_data->forward_column_group = column->group ;

  if (canvas_data->thisType->showUpStrand)
    {
      if (!(column = findColumn(canvas_data->window->columns, source_name, FALSE)))
	{
	  canvas_data->reverse_column_pos -= canvas_data->thisType->width ;

	  column = createColumn(alignment, TRUE, 
				canvas_data->reverse_column_pos, source_name) ;

	  canvas_data->reverse_column_pos -= COLUMN_SPACING ;

	  g_ptr_array_add(canvas_data->window->columns, column) ;
	}

      canvas_data->reverse_column_group = column->group ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  return col_group ;
}



void zmapWindowAlignmentHideUnhideColumns(ZMapWindowAlignment alignment)
{
  int i ;
  ZMapWindowColumn column ;
  double min_mag ;

  for ( i = 0 ; i < alignment->columns->len ; i++ )
    {
      column = g_ptr_array_index(alignment->columns, i) ;

      /* type->min_mag is in bases per line, but window->zoom_factor is pixels per base */
      min_mag = (column->type->min_mag ? alignment->window->max_zoom/column->type->min_mag : 0.0) ;

      if (alignment->window->zoom_factor > min_mag)
	{
	  if (column->forward)
	    foo_canvas_item_show(column->forward_group);
	  else if (column->type->showUpStrand)
	    foo_canvas_item_show(column->reverse_group);
	}
      else
	foo_canvas_item_hide(column->column_group) ;
    }

  return ;
}



/* Returns NULL always so you can have this function "automatically" reset your
 * pointer to the alignment when you destroy it. */
ZMapWindowAlignment zmapWindowAlignmentDestroy(ZMapWindowAlignment alignment)
{


  if (alignment->columns)
    {
      /* Should we first call a foreach routine to free off the canvas obj. etc. ?? */
      
      g_ptr_array_free(alignment->columns, TRUE) ;
    }


  g_free(alignment) ;

  return NULL ;
}



/* Find the column with the given name and strand, returns NULL if not found. */
static ZMapWindowColumn findColumn(GPtrArray *col_array, char *col_name, gboolean forward_strand)
{
  ZMapWindowColumn column = NULL ;
  int i ;
  
  for (i = 0 ; i < col_array->len ; i++)
    {
      ZMapWindowColumn next_column = g_ptr_array_index(col_array, i) ;

      if (strcmp(next_column->type_name, col_name) == 0 && next_column->forward == forward_strand)
	{
	  column = next_column ;
	  break ;
	}
    }

  return column ;
}



/* create Column */
static ZMapWindowColumn createColumn(ZMapWindowAlignment alignment,
				     gboolean forward, double position,
				     gchar *type_name, ZMapFeatureTypeStyle type)
{
  ZMapWindowColumn column ;
  GdkColor white ;
  FooCanvasItem *group, *boundingBox ;
  double min_mag ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gdk_color_parse("white", &white) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  gdk_color_parse("red", &white) ;


  printf("Col: %s  is at:  %f\n", type_name, position) ;

  /* We seem to need the bounding box to get a container that spans the whole column
   * to enable us to get events on a per column basis. */

  group = foo_canvas_item_new(FOO_CANVAS_GROUP(alignment->alignment_group),
			      foo_canvas_group_get_type(),
			      "x", position,
			      "y", 0.0,
			      NULL) ;

  g_signal_connect(GTK_OBJECT(group), "event",
		   GTK_SIGNAL_FUNC(canvasColumnEventCB), alignment->window) ;


  position = 0.0 ;

  boundingBox = foo_canvas_item_new(FOO_CANVAS_GROUP(group),
				    foo_canvas_rect_get_type(),
				    "x1", position,
				    "y1", alignment->window->seq_start,
				    "x2", type->width,
				    "y2", alignment->window->seq_end,
				    "fill_color_gdk", &white,
				    NULL) ;

  g_signal_connect(GTK_OBJECT(boundingBox), "event",
		   GTK_SIGNAL_FUNC(canvasBoundingBoxEventCB), alignment->window) ;


  /* store a pointer to the column to enable hide/unhide while zooming */
  column = g_new0(ZMapWindowColumnStruct, 1) ;		    /* destroyed with ZMapWindow */
  column->type = type ;
  column->type_name = type_name ;

  column->column_group = group ;

  column->forward = forward ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* can be done later.... */

  /* Decide whether or not this column should be visible */
  /* thisType->min_mag is in bases per line, but window->zoom_factor is pixels per base */
  min_mag = (canvas_data->thisType->min_mag ? 
	                 canvas_data->window->max_zoom/canvas_data->thisType->min_mag : 0.0);

  
  if (canvas_data->window->zoom_factor < min_mag)
    foo_canvas_item_hide(FOO_CANVAS_ITEM(group)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  
  return column ;
}


static gboolean canvasColumnEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;
  ZMapWindow  window = (ZMapWindowStruct*)data;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("in COLUMN canvas event handler\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  if (event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton *but_event = (GdkEventButton *)event ;

      printf("in COLUMN group event handler - CLICK\n") ;


      /* retrieve the FeatureItemStruct from the clicked object, obtain the feature_set from that and the
       * feature from that, using the GQuark feature_key. Then call the 
       * click callback function, passing the ZMapWindow and feature so the details of the clicked object
       * can be displayed in the info_panel. */

      /* THERE APPEARS TO BE NO CODE HERE.... */

      event_handled = FALSE ;
    } 
  else if (event->type == GDK_BUTTON_RELEASE)
    {
      GdkEventButton *but_event = (GdkEventButton *)event ;

      /* retrieve the FeatureItemStruct from the clicked object, obtain the feature_set from that and the
       * feature from that, using the GQuark feature_key. Then call the 
       * click callback function, passing the ZMapWindow and feature so the details of the clicked object
       * can be displayed in the info_panel. */

      event_handled = FALSE ;
    } 
  else if (event->type == GDK_KEY_PRESS)
    {
      event_handled = FALSE ;
    }

  return event_handled ;
}

static gboolean canvasBoundingBoxEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;
  ZMapWindow  window = (ZMapWindowStruct*)data;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("in COLUMN canvas event handler\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  if (event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton *but_event = (GdkEventButton *)event ;

      printf("in COLUMN BOUNDING BOX canvas event handler - CLICK\n") ;


      /* retrieve the FeatureItemStruct from the clicked object, obtain the feature_set from that and the
       * feature from that, using the GQuark feature_key. Then call the 
       * click callback function, passing the ZMapWindow and feature so the details of the clicked object
       * can be displayed in the info_panel. */

      /* THERE APPEARS TO BE NO CODE HERE.... */

      event_handled = FALSE ;
    } 
  else if (event->type == GDK_BUTTON_RELEASE)
    {
      GdkEventButton *but_event = (GdkEventButton *)event ;

      /* retrieve the FeatureItemStruct from the clicked object, obtain the feature_set from that and the
       * feature from that, using the GQuark feature_key. Then call the 
       * click callback function, passing the ZMapWindow and feature so the details of the clicked object
       * can be displayed in the info_panel. */

      event_handled = FALSE ;
    } 
  else if (event->type == GDK_KEY_PRESS)
    {
      event_handled = FALSE ;
    }

  return event_handled ;
}



