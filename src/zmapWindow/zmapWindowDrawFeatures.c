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
 * Last edited: Oct 20 14:11 2004 (edgrif)
 * Created: Thu Jul 29 10:45:00 2004 (rnc)
 * CVS info:   $Id: zmapWindowDrawFeatures.c,v 1.25 2004-10-20 13:13:03 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindow_P.h>
#include <ZMap/zmapDraw.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapWindowDrawFeatures.h>


#define MULT 0.875

/******************** function prototypes *************************************************/

static void     zmapWindowProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data);
static void     zmapWindowProcessFeature   (GQuark key_id, gpointer data, gpointer user_data);
static void     columnClickCB              (ZMapCanvasDataStruct *canvasData, ZMapFeatureSet feature_set);
static gboolean handleCanvasEvent          (GtkWidget *widget, GdkEventButton *event, gpointer data);
static gboolean freeObjectKeys             (GtkWidget *widget, gpointer data);

/* These callback routines are static because they are set just once for the lifetime of the
 * process. */

/* Callbacks we make back to the level above us. */
static ZMapFeatureCallbacks feature_cbs_G = NULL ;



/******************** start of function code **********************************************/

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



gboolean zMapFeatureClickCB(ZMapCanvasDataStruct *canvasData, ZMapFeature feature)
{

  /* call the function pointed to. This will cascade up the hierarchy
  ** to zmapControl.c where details of the object clicked on will be
  ** displayed in zmap->info_panel. */

  (*(feature_cbs_G->click))(canvasData->window, canvasData->window->app_data, feature);

  return FALSE;  /* FALSE allows any other connected function to be run. */
}



static void columnClickCB(ZMapCanvasDataStruct *canvasData, ZMapFeatureSet feature_set)
{
  (*(feature_cbs_G->rightClick))(canvasData, feature_set);

  return;
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
  float          result;
  GtkWidget     *parent, *label, *vbox, *vscale, *frame;
  GError        *channel_error = NULL;
  double         wx;
  GtkAdjustment *v_adj, *h_adj ;
  ZMapCanvasDataStruct *canvasData;
  double x, y;
 
  if (feature_context)  /* Split an empty pane and you get here with no feature_context */
    {
      canvasData = g_object_get_data(G_OBJECT(window->canvas), "canvasData");
      canvasData->scaleBarOffset = 100.0;
      canvasData->types = types;
      canvasData->feature_context = feature_context;
      canvasData->reduced = FALSE;
      canvasData->atLimit = FALSE;
      canvasData->seqLength
	= feature_context->sequence_to_parent.c2 - feature_context->sequence_to_parent.c1 + 1;
      canvasData->seq_start = feature_context->sequence_to_parent.c1 ;
      canvasData->seq_end = feature_context->sequence_to_parent.c2 ;

      if (!(canvasData->channel = g_io_channel_new_file("features.out", "w", &channel_error)))
	{
	  printf("Unable to open output channel.  %d %s\n", channel_error->code, channel_error->message);
	  g_error_free(channel_error);
	}

      /* make the sequence fit the default window */
      v_adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolledWindow));
      h_adj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(window->scrolledWindow));

      /* page size seems to be one bigger than we think...to get the bottom of the sequence
       * shown properly we have to do the "- 1"...sigh... */
      window->zoom_factor = (v_adj->page_size - 1) / canvasData->seqLength;

      foo_canvas_set_pixels_per_unit_xy(window->canvas, 1.0, window->zoom_factor) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      foo_canvas_c2w(window->canvas, v_adj->page_size, h_adj->page_size, &x, &y);
      printf("page_size (world) is %f\n", x);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      /* Set length of scroll region to the extent of the sequence data, width is set by default
       * to that of the scrolled window. */
      foo_canvas_set_scroll_region(window->canvas,
				   0.0, canvasData->seq_start,
				   h_adj->page_size, canvasData->seq_end) ;

      /* Set root group to begin at 0, _vital_ for later feature placement. */
      window->group = foo_canvas_item_new(foo_canvas_root(window->canvas),
					  foo_canvas_group_get_type(),
					  "x", 0.0, "y", 0.0,
					  NULL) ;

      result = zmapDrawScale(canvasData->canvas, window->scrolledWindow, 
			     canvasData->scaleBarOffset, 1.0,
			     feature_context->sequence_to_parent.c1, 
			     feature_context->sequence_to_parent.c2);

      canvasData->column_position = canvasData->scaleBarOffset + 110.0;
      canvasData->revColPos = result;                
      /* NB there's a flaw here: too many visible upstrand groups and we'll go left of the edge of
      ** the window. Should really be keeping track and adjusting scroll region if necessary. */

      if (feature_context)
	g_datalist_foreach(&(feature_context->feature_sets), zmapWindowProcessFeatureSet, canvasData);
    }


  if (canvasData->channel)
    {
      g_io_channel_shutdown(canvasData->channel, TRUE, &channel_error);
    }

  return;
}



/* Called for each feature set, it then calls a routine to draw each of its features.  */
static void zmapWindowProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
  ZMapCanvasDataStruct *canvasData = (ZMapCanvasDataStruct*)user_data;

  FooCanvasItem *col_group;  /* each feature_set represents a column */
  double         column_spacing = 5.0;
  const gchar   *itemDataKey = "feature_set";
  FeatureKeys    featureKeys;
  FooCanvasItem *boundingBox;
  double         x1, y1, x2, y2;
  GdkColor       white;
  GdkColor       pink;


  gdk_color_parse("white", &white);
  gdk_color_parse("pink", &pink);

  canvasData->thisType = (ZMapFeatureTypeStyle)g_datalist_get_data(&(canvasData->types),
								   feature_set->source) ;

  /* context_key is used when a user clicks a canvas item.  The callback function needs to know
  ** which feature is associated with the clicked object, so we use the GQuarks to look it up.*/
  canvasData->feature_set = feature_set; 
  canvasData->context_key = key_id;

  featureKeys = g_new0(FeatureKeyStruct, 1);
  featureKeys->feature_set = canvasData->feature_set;
  featureKeys->context_key = canvasData->context_key;
  featureKeys->feature_key = 0;

  if (!canvasData->thisType) /* ie no method for this source */
      zMapLogWarning("No ZMapType (aka method) found for source: %s", feature_set->source);
  else
    {
      /* There should be a function here to replace all this duplicated code..... */

      /* It is _vital_ that the column group starts at 0.0 so that the feature drawing
       * code can just directly draw the features in their natural coords. */
      canvasData->columnGroup = foo_canvas_item_new(foo_canvas_root(canvasData->canvas),
						    foo_canvas_group_get_type(),
						    "x", canvasData->column_position,
						    "y", 0.0,
						    NULL);

      canvasData->column_position += (canvasData->thisType->width + column_spacing);

      /* Each column is entirely covered by a bounding box over the range of the sequence,
       * this means we can get events (e.g. button clicks) from anywhere within the column. */
      boundingBox = foo_canvas_item_new(FOO_CANVAS_GROUP(canvasData->columnGroup),
					foo_canvas_rect_get_type(),
					"x1", 0.0,
					"y1", canvasData->seq_start,
					"x2", (double)canvasData->thisType->width,
					"y2", canvasData->seq_end,
					"fill_color_gdk", &white,
					NULL);

      g_signal_connect(G_OBJECT(boundingBox), "event",
		       G_CALLBACK(handleCanvasEvent), canvasData) ;

      itemDataKey = "feature_set";
      g_object_set_data(G_OBJECT(boundingBox), itemDataKey, featureKeys);
      itemDataKey = "canvasData";
      g_object_set_data(G_OBJECT(boundingBox), itemDataKey, canvasData);
	  
      if (boundingBox)
	g_signal_connect(G_OBJECT(boundingBox), "destroy",
			 G_CALLBACK(freeObjectKeys), featureKeys);

      feature_set->forCol = canvasData->columnGroup;
      if (canvasData->window->zoom_factor < canvasData->thisType->min_mag)
      	foo_canvas_item_hide(FOO_CANVAS_ITEM(feature_set->forCol));
        

      /* if (showUpStrand) then create a reverse strand group as well */
      if (canvasData->thisType->showUpStrand)
	{
	  canvasData->revColGroup = foo_canvas_item_new(foo_canvas_root(canvasData->canvas),
						    foo_canvas_group_get_type(),
						    "x", canvasData->revColPos,
						    "y", canvasData->seq_start,
						    NULL);
	  canvasData->revColPos -= (canvasData->thisType->width + column_spacing);

	  boundingBox = foo_canvas_item_new(FOO_CANVAS_GROUP(canvasData->revColGroup),
					    foo_canvas_rect_get_type(),
					    "x1", 0.0,
					    "y1", canvasData->seq_start,
					    "x2", canvasData->thisType->width,
					    "y2", canvasData->seq_end,
					    "fill_color_gdk", &white,
					    NULL);

	  g_signal_connect(G_OBJECT(boundingBox), "event",
			   G_CALLBACK(handleCanvasEvent), canvasData) ;

	  itemDataKey = "feature_set";
	  g_object_set_data(G_OBJECT(boundingBox), itemDataKey, featureKeys);
	  itemDataKey = "canvasData";
	  g_object_set_data(G_OBJECT(boundingBox), itemDataKey, canvasData);
	  
	  if (boundingBox)
	    g_signal_connect(G_OBJECT(boundingBox), "destroy",
			     G_CALLBACK(freeObjectKeys), featureKeys);

	  /* store a pointer to the group in the feature_set to enable hide/unhide zooming */
	  feature_set->revCol = canvasData->revColGroup;

	  /* hide the column if it's below the specified threshold */
	  if (canvasData->window->zoom_factor < canvasData->thisType->min_mag)
	    foo_canvas_item_hide(FOO_CANVAS_ITEM(feature_set->revCol));
	  
	}

      /* Expand the scroll region _horizontally_ to include the latest column. */
      foo_canvas_set_scroll_region(canvasData->window->canvas, 1.0, canvasData->seq_start, 
				   canvasData->column_position + 20, 
				   canvasData->seq_end) ;

      g_datalist_foreach(&(feature_set->features), zmapWindowProcessFeature, canvasData) ;
    }

  return ;
}




static void zmapWindowProcessFeature(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature           zMapFeature = (ZMapFeature)data ; 
  ZMapCanvasDataStruct *canvasData = (ZMapCanvasDataStruct*)user_data;

  ZMapSpan              zMapSpan, prevSpan;
  ZMapAlignBlock       *zMapAlign, *prevAlign;
  int                   i, j;
  FooCanvasItem        *columnGroup, *feature_group, *object = NULL;
  int                   x1, x2;
  float                 line_width = 1.0;
  const gchar          *itemDataKey = "feature";
  FeatureKeys           featureKeys;
  GString              *buf ;
  GError               *channel_error = NULL;
  gsize                 bytes_written;
  double                dummy, y1, y2, middle;





  /* set up the primary details for output */
  buf = g_string_new("new") ;

  g_string_printf(buf, "%s %d %d", 
		  zMapFeature->name,
		  zMapFeature->x1,
		  zMapFeature->x2);

  featureKeys = g_new0(FeatureKeyStruct, 1);
  featureKeys->feature_set = canvasData->feature_set;
  featureKeys->context_key = canvasData->context_key;
  featureKeys->feature_key = key_id;

  /* decide whether this feature lives on the up or down strand */
  if (zMapFeature->strand == ZMAPSTRAND_DOWN || zMapFeature->strand == ZMAPSTRAND_NONE)
      columnGroup = canvasData->columnGroup;
  else
    columnGroup = canvasData->revColGroup;


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
	  itemDataKey = "feature_set";
	  g_object_set_data(G_OBJECT(object), itemDataKey, featureKeys);
	  
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
	  
	    for (j = 1; j < zMapFeature->feature.transcript.exons->len; j++)
	      {
		zMapSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, j);
		prevSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, j-1);
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
	  
	    for (j = 0; j < zMapFeature->feature.transcript.exons->len; j++)
	      {
		zMapSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, j);

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
    }

  /* print the feature details to the output file */
  if (canvasData->channel)
    {
      g_io_channel_write_chars(canvasData->channel, buf->str, -1, &bytes_written, &channel_error);

      if (channel_error)				    /* um...report error ? */
	g_error_free(channel_error);
    }

  g_string_free(buf, TRUE);

  if (object)
    g_signal_connect(G_OBJECT(object), "destroy",
		     G_CALLBACK(freeObjectKeys), featureKeys);

  return ;
}


static gboolean handleCanvasEvent(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  ZMapCanvasDataStruct *canvasData;
  ZMapFeatureContext feature_context;
  ZMapFeatureSet feature_set;
  ZMapFeature feature;
  const gchar *itemDataKey = "feature";
  FeatureKeys featureKeys;
  gboolean result = FALSE;  /* if not a BUTTON_PRESS, leave for any other event handlers. */

  if (event->type == GDK_BUTTON_PRESS)  /* GDK_BUTTON_PRESS is 4 */
  {
    /* retrieve the FeatureKeyStruct from the clicked object, obtain the feature_set from that and the
    ** feature from that, using the two GQuarks, context_key and feature_key. Then call the 
    ** click callback function, passing canvasData and feature so the details of the clicked object
    ** can be displayed in the info_panel. */
    featureKeys = g_object_get_data(G_OBJECT(widget), itemDataKey);  
    itemDataKey = "canvasData";
    canvasData      = g_object_get_data(G_OBJECT(widget), itemDataKey);  

    if (featureKeys && featureKeys->feature_key)
      {
	feature_set = g_datalist_id_get_data(&(canvasData->feature_context->feature_sets),
					     featureKeys->context_key);
	feature = g_datalist_id_get_data(&(feature_set->features), featureKeys->feature_key); 
	zMapFeatureClickCB(canvasData, feature);                            

	canvasData->thisType = (ZMapFeatureTypeStyle)g_datalist_get_data(&(canvasData->types), 
									 feature_set->source) ;
  	zmapHighlightObject(FOO_CANVAS_ITEM(widget), canvasData);

	result = TRUE;
      }

    /* if this was a right-click, display a list of the features in this column so the user
    ** can select one and have the display scroll to it. */
    if (event->button == 3)
      {
	double wy;

	foo_canvas_item_i2w(FOO_CANVAS_ITEM(widget), &canvasData->x, &wy);

	if (featureKeys)
	  {
	    feature_set = g_datalist_id_get_data(&(canvasData->feature_context->feature_sets),
						 featureKeys->context_key);
	    canvasData->feature = feature;
	    columnClickCB(canvasData, feature_set);  /* display selectable list of features */
	    result = TRUE;
	  }
      }
  }


  return result;
}


static gboolean freeObjectKeys(GtkWidget *widget, gpointer data)
{
  FeatureKeys featureKeys = (FeatureKeyStruct*)data;

  g_free(featureKeys);

  return FALSE;
}


void zmapHighlightObject(FooCanvasItem *feature, ZMapCanvasDataStruct *canvasData)
{                                               
  GdkColor outline;
  GdkColor foreground;


  /* if any other feature is currently in focus, revert it to its std colours */
  if (canvasData->focusFeature)
    foo_canvas_item_set(FOO_CANVAS_ITEM(canvasData->focusFeature),
			"outline_color_gdk", &canvasData->focusType->outline,
 			"fill_color_gdk"   , &canvasData->focusType->foreground,
			NULL);

  /* set outline and foreground GdkColors to be the inverse of current settings */
  outline.red      = (65535 - canvasData->thisType->outline.red  );
  outline.green    = (65535 - canvasData->thisType->outline.green);
  outline.blue     = (65535 - canvasData->thisType->outline.blue );
  
  foreground.red   = (65535 - canvasData->thisType->foreground.red  );
  foreground.green = (65535 - canvasData->thisType->foreground.green);
  foreground.blue  = (65535 - canvasData->thisType->foreground.blue );

  foo_canvas_item_set(feature,
		      "outline_color_gdk", &outline,
		      "fill_color_gdk"   , &foreground,
		      NULL);
 
  /* store these settings and reshow the feature */
  canvasData->focusFeature = feature;
  canvasData->focusType = canvasData->thisType;
  gtk_widget_show_all(GTK_WIDGET(canvasData->canvas)); 

  return;
}

/****************** end of file ************************************/
