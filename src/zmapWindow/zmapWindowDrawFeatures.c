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
 * Last edited: Oct 13 21:47 2004 (edgrif)
 * Created: Thu Jul 29 10:45:00 2004 (rnc)
 * CVS info:   $Id: zmapWindowDrawFeatures.c,v 1.20 2004-10-14 08:43:54 edgrif Exp $
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
void zMapFeatureInit(ZMapFeatureCallbacks callbacks)
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


void zmapWindowDrawFeatures(ZMapWindow window, ZMapFeatureContext feature_context, GData *types)
{
  float          result;
  GtkWidget     *parent, *label, *vbox, *vscale, *frame;
  GError        *channel_error = NULL;
  double         wx;
  GtkAdjustment *adj;
  ZMapCanvasDataStruct *canvasData;
 
  if (feature_context)  /* Split an empty pane and you get here with no feature_context */
    {
      canvasData = g_object_get_data(G_OBJECT(window->canvas), "canvasData");
      canvasData->scaleBarOffset = 100.0;
      canvasData->types = types;
      canvasData->feature_context = feature_context;
      canvasData->channel = g_io_channel_new_file("features.out", "w", &channel_error);

      if (!canvasData->channel)
	printf("Unable to open output channel.  %d %s\n", channel_error->code, channel_error->message);


      /* make the sequence fit the default window */
      canvasData->seqLength =
	feature_context->sequence_to_parent.c2 - feature_context->sequence_to_parent.c1 + 1 ;

      adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolledWindow));

      window->zoom_factor = adj->page_size / canvasData->seqLength;

      zMapWindowZoom(window, 1.0);  /* already adjusted the zoom_factor, so 1.0 is right here. */

      /* convert page_size to world coordinates.  Ignore x dimension */
      foo_canvas_c2w(canvasData->canvas, 0, adj->page_size, &wx, &canvasData->height);

      if (canvasData->seqLength < 0)
	canvasData->seqLength *= -1.0;

      result = zmapDrawScale(window->canvas, window->scrolledWindow, 
			     canvasData->scaleBarOffset, 1.0,
			     feature_context->sequence_to_parent.c1, 
			     feature_context->sequence_to_parent.c2);

      canvasData->column_position = result + 100.0;  /* a bit right of where we drew the scale bar */
      canvasData->revColPos = result;                /* and left of it for reverse strand stuff */
      /* NB there's a flaw here: too many visible upstrand groups and we'll go left of the edge of
      ** the window. */

      if (feature_context)
	g_datalist_foreach(&(feature_context->feature_sets), zmapWindowProcessFeatureSet, canvasData);
    }

  if (channel_error)
    g_error_free(channel_error);

  g_io_channel_shutdown(canvasData->channel, TRUE, &channel_error);

  return;
}



/* Called for each feature set, it then calls a routine to draw each of its features.  */
static void zmapWindowProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
  ZMapCanvasDataStruct *canvasData = (ZMapCanvasDataStruct*)user_data;

  FooCanvasItem *col_group;  /* each feature_set represents a column */
  double         column_spacing = 20.0;
  const gchar   *itemDataKey = "feature_set";
  FeatureKeys    featureKeys;
  FooCanvasItem *boundingBox;
  double         wx, wy;
  GdkColor       white;


  gdk_color_parse("white", &white);

  /* NB for each column we create a new canvas group, with the initial y coordinate set to
  ** 5.0 (#define BORDER 5.0 in zmapDraw.h) to just drop it a teeny bit from the top of the
  ** window.  All items live in the group.  
  ** Note that this y coord adjustment is directly linked to the the manipulation of 
  ** canvasData->height performed in zmapWindowDrawFeatures.  Change them together or not at all. */
  canvasData->thisType = (ZMapFeatureTypeStyle)g_datalist_get_data(&(canvasData->types), feature_set->source) ;

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
      /* calculate top position in world coords; ignore x dimension */
      foo_canvas_c2w(canvasData->canvas, 0, BORDER, &wx, &wy);

      canvasData->column_position += column_spacing;

      canvasData->columnGroup = foo_canvas_item_new(foo_canvas_root(canvasData->canvas),
						foo_canvas_group_get_type(),
						"x", (double)canvasData->column_position,
						"y", (double)wy,
						NULL);


      boundingBox = foo_canvas_item_new(FOO_CANVAS_GROUP(canvasData->columnGroup),
					foo_canvas_rect_get_type(),
					"x1", (double)0.0,
					"y1", (double)0.0,
					"x2", (double)canvasData->thisType->width,
					"y2", (double)canvasData->height - (2 * wy),
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
						    "x", (double)canvasData->revColPos,
						    "y", (double)wy,
						    NULL);
	  boundingBox = foo_canvas_item_new(FOO_CANVAS_GROUP(canvasData->revColGroup),
					    foo_canvas_rect_get_type(),
					    "x1", (double)0.0,
					    "y1", (double)0.0,
					    "x2", (double)canvasData->thisType->width,
					    "y2", (double)canvasData->height - (2 * wy),
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

	  canvasData->revColPos -= column_spacing;

	  /* store a pointer to the group in the feature_set to enable hide/unhide zooming */
	  feature_set->revCol = canvasData->revColGroup;

	  /* hide the column if it's below the specified threshold */
	  if (canvasData->window->zoom_factor < canvasData->thisType->min_mag)
	    foo_canvas_item_hide(FOO_CANVAS_ITEM(feature_set->revCol));
	  
	}

      foo_canvas_set_scroll_region(canvasData->window->canvas, 1.0, -5.0, 
				   canvasData->column_position + 20, 
				   canvasData->seqLength + 5.0);

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
  GString              *buf = g_string_new("new");
  GError               *channel_error = NULL;
  gsize                 bytes_written;
  double                dummy, y1, y2, middle;

  /* set up the primary details for output */
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
	case ZMAPFEATURE_BASIC: case ZMAPFEATURE_VARIATION:     /* type 0 is a basic, 5 is allele */
	case ZMAPFEATURE_BOUNDARY: case ZMAPFEATURE_SEQUENCE:   /* boundary is 6, sequence is 7 */

	  g_string_append_printf(buf, "\n");

	  canvasData->feature_group = columnGroup;
	  
	  /* calculate position in world coords */
	  foo_canvas_c2w(canvasData->canvas, 
			 0,
			 (zMapFeature->x1 + BORDER) * canvasData->window->zoom_factor * MULT, 
			 &dummy, &y1);
	  foo_canvas_c2w(canvasData->canvas, 
			 0,
			 (zMapFeature->x2 + BORDER) * canvasData->window->zoom_factor * MULT, 
			 &dummy, &y2);

	  object = zmapDrawBox(FOO_CANVAS_ITEM(columnGroup), 0.0,
			       y1,
			       canvasData->thisType->width, 
			       y2,
			       &canvasData->thisType->outline,
			       &canvasData->thisType->foreground);
	

	  g_signal_connect(G_OBJECT(object), "event",
			   G_CALLBACK(handleCanvasEvent), canvasData) ;
	  
	  itemDataKey = "feature";
	  g_object_set_data(G_OBJECT(object), itemDataKey, featureKeys);
	  itemDataKey = "canvasData";
	  g_object_set_data(G_OBJECT(object), itemDataKey, canvasData);
	  
	  break;

	case ZMAPFEATURE_HOMOL:     /* type 1 is a homol */

	  g_string_append_printf(buf, "\n");

	  canvasData->feature_group = columnGroup;
	  
	  /* calculate position in world coords */
	  foo_canvas_c2w(canvasData->canvas, 
			 0,
			 (zMapFeature->x1 + BORDER) * canvasData->window->zoom_factor * MULT, 
			 &dummy, &y1);

	  foo_canvas_c2w(canvasData->canvas, 
			 0,
			 (zMapFeature->x2 + BORDER) * canvasData->window->zoom_factor * MULT, 
			 &dummy, &y2);

	  object = zmapDrawBox(FOO_CANVAS_ITEM(columnGroup), 0.0,
			       y1,
			       canvasData->thisType->width, 
			       y2,
			       &canvasData->thisType->outline, 
			       &canvasData->thisType->foreground);


	  g_signal_connect(G_OBJECT(object), "event",
			   G_CALLBACK(handleCanvasEvent), canvasData) ;

	  itemDataKey = "feature";
	  g_object_set_data(G_OBJECT(object), itemDataKey, featureKeys);
	  itemDataKey = "canvasData";
	  g_object_set_data(G_OBJECT(object), itemDataKey, canvasData);

	  break;


	case ZMAPFEATURE_TRANSCRIPT:     /* type 4 is a transcript */
	  foo_canvas_c2w(canvasData->canvas, 
			 0.0,
			 (zMapFeature->x1 + BORDER) * canvasData->window->zoom_factor * MULT, 
			 &dummy, &y1);

	  feature_group = foo_canvas_item_new(FOO_CANVAS_GROUP(columnGroup),
					      foo_canvas_group_get_type(),
					      "x", (double)0.0,
					      "y", (double)y1,
					      NULL);
      
	  canvasData->feature_group = feature_group;
	  
	  for (j = 1; j < zMapFeature->feature.transcript.exons->len; j++)
	    {
	      zMapSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, j);
	      prevSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, j-1);
	      //middle = (((prevSpan->x2 + zMapSpan->x1)/2) - zMapFeature->x1) * canvasData->window->zoom_factor;
	      //y0 = (prevSpan->x1 - zMapFeature->x1) * canvasData->window->zoom_factor; /* y coord of preceding
	      //								    exon */							    
	      //y1 = (prevSpan->x2 - zMapFeature->x1) * canvasData->window->zoom_factor;
	      //y2 = (zMapSpan->x1 - zMapFeature->x1) * canvasData->window->zoom_factor;

	      /* calculate position in world coords */
	      foo_canvas_c2w(canvasData->canvas, 
			     0.0,
			(((prevSpan->x2 + zMapSpan->x1)/2 - zMapFeature->x1) + BORDER) * canvasData->window->zoom_factor * MULT, 
			     &dummy, &middle);

	      foo_canvas_c2w(canvasData->canvas, 
			     0.0,
			     (prevSpan->x2 - zMapFeature->x1 + BORDER) * canvasData->window->zoom_factor * MULT, 
			     &dummy, &y1);

	      foo_canvas_c2w(canvasData->canvas, 
			     0.0,
			     (zMapSpan->x1 - zMapFeature->x1 + BORDER) * canvasData->window->zoom_factor * MULT, 
			     &dummy, &y2);

	      if (y2 > y1)
		{
		  zmapDrawLine(FOO_CANVAS_GROUP(feature_group), 
			       canvasData->thisType->width/2, y1,
			       canvasData->thisType->width  , middle, 
			       &canvasData->thisType->foreground, line_width);

		  zmapDrawLine(FOO_CANVAS_GROUP(feature_group), 
			       canvasData->thisType->width  , middle, 
			       canvasData->thisType->width/2, y2, 
			       &canvasData->thisType->foreground, line_width);
		}
	    }
	  for (j = 0; j < zMapFeature->feature.transcript.exons->len; j++)
	    {
	      zMapSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, j);
	      /* calculate position in world coords */
	      foo_canvas_c2w(canvasData->canvas, 
			     0.0,
			     (zMapSpan->x1 - zMapFeature->x1 + BORDER) * canvasData->window->zoom_factor * MULT, 
			     &dummy, &y1);
	      
	      foo_canvas_c2w(canvasData->canvas, 
			     0.0,
			     (zMapSpan->x2 - zMapFeature->x1 + BORDER) * canvasData->window->zoom_factor * MULT, 
			     &dummy, &y2);
	      
	      /*	      if (j == 0)
		zmapDisplayText(FOO_CANVAS_GROUP(feature_group), zMapFeature->name, 
				"brown", 40.0, y1);
	      */
	      object = zmapDrawBox(feature_group, 0.0,
				   y1,
				   canvasData->thisType->width,
				   y2,
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

	default:
	  printf("Not displaying %s type %d\n", zMapFeature->name, zMapFeature->type);
	  break;
	}
    }
  /* print the feature details to the output file */
  g_io_channel_write_chars(canvasData->channel, buf->str, -1, &bytes_written, &channel_error);
  g_string_free(buf, TRUE);
  if (channel_error)
    g_error_free(channel_error);

  if (object)
    g_signal_connect(G_OBJECT(object), "destroy",
		     G_CALLBACK(freeObjectKeys), featureKeys);
  return;
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
	feature_set = g_datalist_id_get_data(&(canvasData->feature_context->feature_sets), featureKeys->context_key);
	feature = g_datalist_id_get_data(&(feature_set->features), featureKeys->feature_key); 
	zMapFeatureClickCB(canvasData, feature);                            

	canvasData->thisType = (ZMapFeatureTypeStyle)g_datalist_get_data(&(canvasData->types), 
									 feature_set->source) ;
  	zmapHighlightObject(FOO_CANVAS_ITEM(widget), canvasData);

	result = TRUE;
      }

    if (event->button == 3)
      {
	double wy;

	foo_canvas_item_i2w(FOO_CANVAS_ITEM(widget), &canvasData->x, &wy);

	itemDataKey = "feature_set";
	featureKeys = g_object_get_data(G_OBJECT(widget), itemDataKey);  

	if (featureKeys)
	  {
	    feature_set = g_datalist_id_get_data(&(canvasData->feature_context->feature_sets), featureKeys->context_key);
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
			"outline_color_gdk", &canvasData->thisType->outline,
 			"fill_color_gdk"   , &canvasData->thisType->foreground,
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
