/*  File: zmapWindowDrawFeatures.c
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk and
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *
 * Description: Draws genomic features in the data display window.
 *              
 * Exported functions: 
 * HISTORY:
 * Last edited: Nov 12 15:50 2004 (rnc)
 * Created: Thu Jul 29 10:45:00 2004 (rnc)
 * CVS info:   $Id: zmapWindowDrawFeatures.c,v 1.36 2004-11-12 16:03:26 rnc Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindow_P.h>
#include <ZMap/zmapDraw.h>
#include <ZMap/zmapUtils.h>


#define SCALEBAR_OFFSET 170.0
#define SCALEBAR_WIDTH   40.0
#define COLUMN_SPACING    5.0

/******************** function prototypes *************************************************/

static void     ProcessFeatureSet (GQuark key_id, gpointer data, gpointer user_data);
static void     ProcessFeature    (GQuark key_id, gpointer data, gpointer user_data);
static void     columnClickCB     (ZMapCanvasDataStruct *canvasData, ZMapFeatureSet feature_set);
static gboolean handleCanvasEvent (FooCanvasItem *item, GdkEventButton *event, gpointer data);
static gboolean freeObjectKeys    (GtkWidget *widget, gpointer data);
static void     freeFeatureItem   (gpointer data);
FooCanvasItem *createColumn(ZMapCanvasDataStruct *canvasData, FeatureKeys featureKeys,
			    gboolean forward, double position, gchar *type_name);

static void getTextDimensions(FooCanvasGroup *group, double *width_out, double *height_out) ;



/* These callback routines are static because they are set just once for the lifetime of the
 * process. */

/* Callbacks we make back to the level above us. */
static ZMapFeatureCallbacks feature_cbs_G = NULL ;



/************************ public functions ************************************************/

/* This routine must be called just once before any other windows routine, it is undefined
 * if the caller calls this routine more than once. The caller must supply all of the callback
 * routines.
 * 
 * Note that since this routine is called once per application we do not bother freeing it
 * via some kind of windows terminate routine. */
void zmapFeatureInit(ZMapFeatureCallbacks callbacks)
{
  zMapAssert(!feature_cbs_G) ;

  zMapAssert(callbacks && callbacks->click && callbacks->rightClick) ;

  feature_cbs_G = g_new0(ZMapFeatureCallbacksStruct, 1) ;

  feature_cbs_G->click      = callbacks->click;
  feature_cbs_G->rightClick = callbacks->rightClick;

  return ;
}


/** Callback to display feature details on the window.
 *  
 * Control passes up the hierarchy to zmapControl 
 * where a few details of the selected feature are 
 * displayed on the info_panel of the window. 
 */
gboolean zMapFeatureClickCB(ZMapCanvasDataStruct *canvasData, ZMapFeature feature)
{
  (*(feature_cbs_G->click))(canvasData->window, canvasData->window->app_data, feature);

  return FALSE;  /* FALSE allows any other connected function to be run. */
}


/* Drawing coordinates: PLEASE read this before you start messing about with anything...
 * 
 * It seems that item coordinates are _not_ specified in absolute world coordinates but
 * in fact in the coordinates of their parent group. This has a lot of knock on effects as
 * we get our feature coordinates in absolute world coords and wish to draw them as simply
 * as possible.
 * 
 * Currently we have these coordinate systems operating:
 * 
 * 
 * 0.0           0.0
 *  ^             ^
 *                           seq_start      seq_start    seq_start
 *                              ^              ^            ^
 * 
 * root          col           col           scroll        all
 * group        group(s)     bounding        region       features
 *                             box
 * 
 *                              v              v            v
 *                           seq_end        seq_end      seq_end
 *  v             v
 * inf           inf
 * 
 * 
 * 
 *  */
void zmapWindowDrawFeatures(ZMapWindow window, ZMapFeatureContext feature_context, GData *types)
{
  char          *file = g_strdup_printf("features.out");
  GError        *channel_error = NULL;
  GtkAdjustment *v_adj, *h_adj ;
  ZMapCanvasDataStruct *canvasData;

 
  if (feature_context)  /* Split an empty pane and you get here with no feature_context */
    {
      double text_height, border_world ;

      canvasData = g_object_get_data(G_OBJECT(window->canvas), "canvasData");

      canvasData->scaleBarOffset  = SCALEBAR_OFFSET;
      canvasData->types           = types;
      canvasData->feature_context = feature_context;
      canvasData->seqLength
	= feature_context->sequence_to_parent.c2 - feature_context->sequence_to_parent.c1 + 1;
      canvasData->seq_start = feature_context->sequence_to_parent.c1 ;
      canvasData->seq_end   = feature_context->sequence_to_parent.c2 ;

      if (!(canvasData->channel = g_io_channel_new_file(file, "w", &channel_error)))
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "Cannot open output file: %s  %s\n", file, channel_error->message);
	  g_error_free(channel_error);
	}


      /* Set the zoom factor by default we start at min zoom, BUT note that if user has a
       * very short sequence it may be displayed at maximum zoom already. */
      /* page size seems to be one bigger than we think...to get the bottom of the sequence
       * shown properly we have to do the "- 1"...sigh... */
      v_adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolledWindow));
      h_adj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(window->scrolledWindow));


      getTextDimensions(foo_canvas_root(window->canvas),
			NULL, &text_height) ;

      /* Set border space for top/bottom of sequence. */
      canvasData->border_pixels = text_height * 2 ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* THIS "- 1" FIDDLE WILL NOT BE NECESSARY WHEN THE BOUNDARY STUFF IS DONE. */
      window->zoom_factor = (v_adj->page_size - 1) / canvasData->seqLength;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      window->zoom_factor = v_adj->page_size / canvasData->seqLength ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* I NEED TO FINISH THIS OFF BUT NEED TO DO A CHECK IN OF MY CODE.... */

      /* adjust zoom to allow for top/bottom borders... */
      border_world = canvasData->border_pixels / window->zoom_factor ;

      window->zoom_factor = v_adj->page_size / (canvasData->seqLength + (2.0 * border_world))  ;
							    /* Can't use c2w call because we
							       haven't set the canvas zoom yet. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      /* Record min/max zooms, the max zoom we want is large enough to hold the text required
       * to display a DNA base or other text + a bit. */
      canvasData->max_zoom = text_height + (double)(ZMAP_WINDOW_TEXT_BORDER * 2) ;

      window->zoom_status = ZMAP_ZOOM_MIN ;

      if (window->zoom_factor > canvasData->max_zoom)
	{
	  window->zoom_factor = canvasData->max_zoom ;
	  window->zoom_status = ZMAP_ZOOM_FIXED ;
	}

      canvasData->min_zoom = window->zoom_factor ;


      foo_canvas_set_pixels_per_unit_xy(window->canvas, 1.0, window->zoom_factor) ;


      window->page_increment = v_adj->page_size - 50 ;
      GTK_LAYOUT(window->canvas)->vadjustment->page_increment = window->page_increment ;
  

      /* Set root group to begin at 0, _vital_ for later feature placement. */
      window->group = foo_canvas_item_new(foo_canvas_root(window->canvas),
					  foo_canvas_group_get_type(),
					  "x", 0.0, "y", 0.0,
					  NULL) ;

      canvasData->scaleBarGroup = zmapDrawScale(canvasData->canvas, 
						canvasData->scaleBarOffset, window->zoom_factor,
						feature_context->sequence_to_parent.c1, 
						feature_context->sequence_to_parent.c2);

      canvasData->column_position = canvasData->scaleBarOffset + SCALEBAR_WIDTH + (2 * COLUMN_SPACING);
      canvasData->revColPos = canvasData->scaleBarOffset - 40;                
      /* NB there's a flaw here: too many visible upstrand groups and we'll go left of the edge of
      ** the window. Should really be keeping track and adjusting scroll region if necessary. */

      if (feature_context)
	g_datalist_foreach(&(feature_context->feature_sets), ProcessFeatureSet, canvasData);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Try here to get stuff set up....HAS TO GO AFTER ALL THE FEATURE DRAWING OTHERWISE
       * SCROLL REGION GETS RESET TO LIE AROUND FEATURES ONLY. */
      foo_canvas_set_scroll_region(window->canvas,
				   0.0,
				   (canvasData->seq_start - border_world),
				   h_adj->page_size,
				   (canvasData->seq_end + border_world)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      /* Try here to get stuff set up.... */
      foo_canvas_set_scroll_region(window->canvas,
				   0.0,
				   canvasData->seq_start,
				   h_adj->page_size,
				   canvasData->seq_end) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      zmapWindowPrintCanvas(window->canvas) ; 
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    }


  if (canvasData->channel)
    {
      if (!g_io_channel_shutdown(canvasData->channel, TRUE, &channel_error))
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "Error closing output file: %s  %s\n", file, channel_error->message);
	  g_error_free(channel_error);
	}
    }

  g_free(file);

  return;
}



void zmapWindowHighlightObject(FooCanvasItem *feature, ZMapCanvasDataStruct *canvasData)
{                                               
  GdkColor foreground;


  /* if any other feature is currently in focus, revert it to its std colours */
  if (canvasData->focusFeature)
    foo_canvas_item_set(FOO_CANVAS_ITEM(canvasData->focusFeature),
 			"fill_color_gdk", &canvasData->focusType->foreground,
			NULL);

  /* set foreground GdkColor to be the inverse of current settings */

  foreground.red   = (65535 - canvasData->thisType->foreground.red  );
  foreground.green = (65535 - canvasData->thisType->foreground.green);
  foreground.blue  = (65535 - canvasData->thisType->foreground.blue );

  foo_canvas_item_set(feature,
		      "fill_color_gdk", &foreground,
		      NULL);
 
  /* store these settings and reshow the feature */
  canvasData->focusFeature = feature;
  canvasData->focusType    = canvasData->thisType;
  gtk_widget_show_all(GTK_WIDGET(canvasData->canvas)); 

  return;
}


/************************ internal functions **************************************/

/* Called for each feature set, it then calls a routine to draw each of its features.  */
static void ProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
  ZMapCanvasDataStruct *canvasData = (ZMapCanvasDataStruct*)user_data;

  FeatureKeys    featureKeys;
  GString       *source = g_string_new(feature_set->source);

  source = g_string_ascii_down(source); /* does it in place */

  canvasData->thisType = (ZMapFeatureTypeStyle)g_datalist_get_data(&(canvasData->types), source->str);
  zMapAssert(canvasData->thisType);

  /* context_key is used when a user clicks a canvas item.  The callback function needs to know
  ** which feature is associated with the clicked object, so we use the GQuarks to look it up.*/
  canvasData->feature_set = feature_set; 
  canvasData->context_key = key_id;

  featureKeys = g_new0(FeatureKeyStruct, 1);
  featureKeys->feature_set = feature_set;
  featureKeys->feature_key = 0;

  canvasData->columnGroup = createColumn(canvasData, featureKeys, TRUE, 
					 canvasData->column_position, source->str);
  canvasData->column_position += (canvasData->thisType->width + COLUMN_SPACING);

  if (canvasData->thisType->showUpStrand)
    {
      canvasData->revColGroup = createColumn(canvasData, featureKeys, FALSE, 
					     canvasData->revColPos, source->str);
      canvasData->revColPos -= (canvasData->thisType->width + COLUMN_SPACING);
    }
  

  /* Expand the scroll region _horizontally_ to include the latest column. */
  foo_canvas_set_scroll_region(canvasData->window->canvas, 1.0, canvasData->seq_start, 
			       canvasData->column_position + 20, 
			       canvasData->seq_end) ;
  
  g_datalist_foreach(&(feature_set->features), ProcessFeature, canvasData) ;

  return ;
}



/* createColumn
 */
FooCanvasItem *createColumn(ZMapCanvasDataStruct *canvasData,
			    FeatureKeys featureKeys,
			    gboolean forward,
			    double position,
			    gchar *type_name)
{
  GdkColor       white;
  FooCanvasItem *group, *boundingBox;
  ZMapColStruct  column;
  double         min_mag;

  gdk_color_parse("white", &white);

  group = foo_canvas_item_new(foo_canvas_root(canvasData->canvas),
			      foo_canvas_group_get_type(),
			      "x", position,
			      "y", 0.0,
			      NULL);
  
  boundingBox = foo_canvas_item_new(FOO_CANVAS_GROUP(group),
				    foo_canvas_rect_get_type(),
				    "x1", 0.0,
				    "y1", canvasData->seq_start,
				    "x2", (double)canvasData->thisType->width,
				    "y2", canvasData->seq_end,
				    "fill_color_gdk", &white,
				    NULL);

  g_signal_connect(G_OBJECT(boundingBox), "event",
		   G_CALLBACK(handleCanvasEvent), canvasData) ;

  g_object_set_data(G_OBJECT(boundingBox), "feature", featureKeys);
  g_object_set_data(G_OBJECT(boundingBox), "canvasData", canvasData);
  
  g_signal_connect(G_OBJECT(group), "destroy",
		   G_CALLBACK(freeObjectKeys), featureKeys);

  /* store a pointer to the column to enable hide/unhide while zooming */
  column.type = canvasData->thisType;
  column.type_name = type_name;

  column.item = group;
  column.forward = forward;
  canvasData->columns = g_array_append_val(canvasData->columns, column);
  
  /* Decide whether or not this column should be visible */
  /* thisType->min_mag is in bases per line, but window->zoom_factor is pixels per base */
  min_mag = (canvasData->thisType->min_mag ? canvasData->max_zoom/canvasData->thisType->min_mag : 0.0);
  
  if (canvasData->window->zoom_factor < min_mag)
    foo_canvas_item_hide(FOO_CANVAS_ITEM(group));
  
  return group;
}




static void ProcessFeature(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature           zMapFeature = (ZMapFeature)data ; 
  ZMapCanvasDataStruct *canvasData = (ZMapCanvasDataStruct*)user_data;

  ZMapSpan              zMapSpan, prevSpan;
  int                   i;
  FooCanvasItem        *columnGroup, *feature_group, *object = NULL;
  float                 line_width = 1.0;
  const gchar          *itemDataKey = "feature";
  FeatureKeys           featureKeys;
  GString              *buf ;
  GError               *channel_error = NULL;
  gsize                 bytes_written;
  double                position;


  /* set up the primary details for output */
  buf = g_string_new("new") ;

  g_string_printf(buf, "%s %d %d", 
		  zMapFeature->name,
		  zMapFeature->x1,
		  zMapFeature->x2);

  featureKeys = g_new0(FeatureKeyStruct, 1);
  featureKeys->feature_set = canvasData->feature_set;
  featureKeys->feature_key = key_id;


  /* decide whether this feature lives on the up or down strand */
  if (zMapFeature->strand == ZMAPSTRAND_DOWN || zMapFeature->strand == ZMAPSTRAND_NONE)
    {
      position = canvasData->column_position;
      columnGroup = canvasData->columnGroup;
    }
  else
    {
      position = canvasData->revColPos;
      columnGroup = canvasData->revColGroup;
    }


  if (zMapFeature->strand == ZMAPSTRAND_DOWN
      || zMapFeature->strand == ZMAPSTRAND_NONE
      || (zMapFeature->strand == ZMAPSTRAND_UP && canvasData->thisType->showUpStrand == TRUE))
    {
      switch (zMapFeature->type)
	{
	case ZMAPFEATURE_BASIC: case ZMAPFEATURE_HOMOL:
	case ZMAPFEATURE_VARIATION:
	case ZMAPFEATURE_BOUNDARY: case ZMAPFEATURE_SEQUENCE:

	  g_string_append_printf(buf, "\n");

	  canvasData->feature_group = columnGroup;
	  
	  object = zmapDrawBox(FOO_CANVAS_ITEM(columnGroup), 0.0,
			       zMapFeature->x1,
			       canvasData->thisType->width, 
			       zMapFeature->x2,
			       &canvasData->thisType->outline,
			       &canvasData->thisType->foreground);

	  g_signal_connect(G_OBJECT(object), "event",
			   G_CALLBACK(handleCanvasEvent), canvasData) ;
	  
	  itemDataKey = "feature";
	  g_object_set_data(G_OBJECT(object), itemDataKey, featureKeys);
	  itemDataKey = "canvasData";
	  g_object_set_data(G_OBJECT(object), itemDataKey, canvasData);

	  break;

	case ZMAPFEATURE_TRANSCRIPT:
	  {
	    /* Note that for transcripts the boxes and lines are contained in a canvas group
	     * and that therefore their coords are relative to the start of the group which
	     * is the start of the transcript, i.e. zMapFeature->x1. */

	    feature_group = foo_canvas_item_new(FOO_CANVAS_GROUP(columnGroup),
						foo_canvas_group_get_type(),
						"x", (double)0.0,
						"y", (double)zMapFeature->x1,
						NULL);
      
	    canvasData->feature_group = feature_group;
	  
	    for (i = 1; i < zMapFeature->feature.transcript.exons->len; i++)
	      {
		zMapSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, i);
		prevSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, i-1);
		if (zMapSpan->x1 > prevSpan->x2)
		  {
		    zmapDrawLine(FOO_CANVAS_GROUP(feature_group), 
				 canvasData->thisType->width/2, 
				 (prevSpan->x2 - zMapFeature->x1),
				 canvasData->thisType->width  ,
				 (((prevSpan->x2 + zMapSpan->x1)/2) - zMapFeature->x1),
				 &canvasData->thisType->foreground, line_width);
			       
		    zmapDrawLine(FOO_CANVAS_GROUP(feature_group), 
				 canvasData->thisType->width  , 
				 (((prevSpan->x2 + zMapSpan->x1)/2) - zMapFeature->x1),
				 canvasData->thisType->width/2, 
				 (zMapSpan->x1 - zMapFeature->x1), 
				 &canvasData->thisType->foreground, line_width);
		  }
	      }
	  
	    for (i = 0; i < zMapFeature->feature.transcript.exons->len; i++)
	      {
		zMapSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, i);

		object = zmapDrawBox(feature_group, 
				     0.0,
				     (zMapSpan->x1 - zMapFeature->x1), 
				     canvasData->thisType->width,
				     (zMapSpan->x2 - zMapFeature->x1), 
				     &canvasData->thisType->outline, 
				     &canvasData->thisType->foreground);

		g_signal_connect(G_OBJECT(object), "event",
				 G_CALLBACK(handleCanvasEvent), canvasData) ;
		  
		itemDataKey = "feature";
		g_object_set_data(G_OBJECT(object), itemDataKey, featureKeys);
		itemDataKey = "canvasData";
		g_object_set_data(G_OBJECT(object), itemDataKey, canvasData);

		/* add transcript details for output */
		g_string_append_printf(buf, " Transcript: %d %d\n", zMapSpan->x1, zMapSpan->x2);
	      }
	    break;
	  }
	default:
	  printf("Not displaying %s type %d\n", zMapFeature->name, zMapFeature->type);
	  break;
	}

      /* print the feature details to the output file */
      if (canvasData->channel)
	{
	  g_io_channel_write_chars(canvasData->channel, buf->str, -1, &bytes_written, &channel_error);
	  
	  if (channel_error)
	    {
	      zMapShowMsg(ZMAP_MSG_WARNING, "Error writing to output file: %30s :%s\n", buf->str, channel_error->message);
	      g_error_free(channel_error);
	    }
	}
    }

  g_string_free(buf, TRUE);

  if (object)
    {
      ZMapFeatureItem  featureItem = g_new0(ZMapFeatureItemStruct, 1);

      featureItem->feature_set = canvasData->feature_set;
      featureItem->canvasItem = object;
      g_datalist_id_set_data_full(&(canvasData->window->featureItems), key_id, featureItem, freeFeatureItem);

      g_signal_connect(G_OBJECT(object), "destroy",
		       G_CALLBACK(freeObjectKeys), featureKeys);
    }

  return ;
}

/** Destroy function for featureItem.
 *
 * Called by g_datalist_clear 
 */
static void freeFeatureItem(gpointer data)
{
  ZMapFeatureItem featureItem = (ZMapFeatureItemStruct*)data;

  g_free(featureItem);
  return;
}


static gboolean handleCanvasEvent(FooCanvasItem *item, GdkEventButton *event, gpointer data)
{
  ZMapCanvasDataStruct *canvasData;
  ZMapFeature feature;
  const gchar *itemDataKey;
  FeatureKeys featureKeys;
  gboolean result = FALSE;  /* if not a BUTTON_PRESS, leave for any other event handlers. */
  GString     *source, *source_lower;
  double       x1, y1, x2, y2;

  /* For now, we go on BUTTON_RELEASE since, after zooming, the canvas never emits
  ** a BUTTON_PRESS event.  Maybe when we find out why, we'll change this.  Or maybe not. */
  if (event->type == GDK_BUTTON_RELEASE)  /* GDK_BUTTON_PRESS is 4, RELEASE 7 */
  {
    /* retrieve the FeatureKeyStruct from the clicked object, obtain the feature_set from that and the
    ** feature from that, using the GQuark feature_key. Then call the 
    ** click callback function, passing canvasData and feature so the details of the clicked object
    ** can be displayed in the info_panel. */
    itemDataKey = "feature";
    featureKeys = g_object_get_data(G_OBJECT(item), itemDataKey);  
    itemDataKey = "canvasData";
    canvasData      = g_object_get_data(G_OBJECT(item), itemDataKey);  

    if (featureKeys && featureKeys->feature_key)
      {
	feature = g_datalist_id_get_data(&(featureKeys->feature_set->features), featureKeys->feature_key); 
	zMapFeatureClickCB(canvasData, feature);   

	/* lowercase the source as all stored types are lowercase */                         
	source = g_string_new(featureKeys->feature_set->source);
	source_lower = g_string_ascii_down(source);
	canvasData->thisType = (ZMapFeatureTypeStyle)g_datalist_get_data(&(canvasData->types),source_lower->str); 
	zMapAssert(canvasData->thisType);
									
	foo_canvas_item_raise_to_top(item);

  	zmapWindowHighlightObject(item, canvasData);

	result = TRUE;
      }

    /* if this was a right-click, display a list of the features in this column so the user
    ** can select one and have the display scroll to it. */
    if (event->button == 3)
      {
	if (featureKeys)
	  {
	    /* store the x coordinate of the clicked item so we can tell
	     * whether it's up or down strand. */
	    foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2);
	    foo_canvas_item_i2w(item, &x1, &y1);
	    canvasData->x = x1;
	    canvasData->feature_set = featureKeys->feature_set;
	    columnClickCB(canvasData, featureKeys->feature_set);  /* display selectable list of features */
	    result = TRUE;
	  }
      }
  }


  return result;
}



static void columnClickCB(ZMapCanvasDataStruct *canvasData, ZMapFeatureSet feature_set)
{
 (*(feature_cbs_G->rightClick))(canvasData, feature_set);

  return;
}


static gboolean freeObjectKeys(GtkWidget *widget, gpointer data)
{
  FeatureKeys featureKeys = (FeatureKeyStruct*)data;

  g_free(featureKeys);

  return FALSE;
}


/* Find out the text size for a group. */
static void getTextDimensions(FooCanvasGroup *group, double *width_out, double *height_out)
{
  double width = -1.0, height = -1.0 ;
  FooCanvasItem *item ;

  /* THIS IS A HACK, THE TEXT IS OFF THE SCREEN TO THE LEFT BECAUSE I DON'T KNOW HOW TO DELETE A
   * CANVAS ITEM.....FIX THIS WHEN I FIND OUT.... */
  item = foo_canvas_item_new(group,
			     FOO_TYPE_CANVAS_TEXT,
			     "x", -400.0, "y", 0.0, "text", "dummy",
			     NULL);

  g_object_get(GTK_OBJECT(item),
	       "FooCanvasText::text_width", &width,
	       "FooCanvasText::text_height", &height,
	       NULL) ;

  if (width_out)
    *width_out = width ;
  if (height_out)
    *height_out = height ;

  return ;
}




/****************** end of file ************************************/
