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
 * Last edited: Sep 13 13:57 2004 (rnc)
 * Created: Thu Jul 29 10:45:00 2004 (rnc)
 * CVS info:   $Id: zmapWindowDrawFeatures.c,v 1.14 2004-09-13 13:31:44 rnc Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindow_P.h>
#include <ZMap/zmapDraw.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapWindowDrawFeatures.h>


// parameters passed between the various functions processing the features to be drawn on the canvas
typedef struct _ParamStruct
{
  ZMapWindow           window;
  FooCanvas           *thisCanvas;
  FooCanvasItem       *columnGroup;
  double               height;
  double               length;
  double               column_position;
  GData               *types;
  ZMapFeatureTypeStyle thisType;
  ZMapFeature          feature;
  ZMapFeatureContext   feature_context;
  ZMapFeatureSet       feature_set;
  GQuark               context_key;
  GIOChannel          *channel;
} ParamStruct;

static ParamStruct params;


// the function to be ultimately called when the user clicks on a canvas item.
typedef void (*ZMapFeatureCallbackFunc)(ParamStruct *params);

/******************** function prototypes *************************************************/

static void     zmapWindowProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data);
static void     zmapWindowProcessFeature   (GQuark key_id, gpointer data, gpointer user_data);
static gboolean featureClickCB             (ParamStruct *params, ZMapFeature feature);
static gboolean handleCanvasEvent          (GtkWidget *widget, GdkEventClient *event, gpointer data);
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

  zMapAssert(callbacks && callbacks->click) ;

  feature_cbs_G = g_new0(ZMapFeatureCallbacksStruct, 1) ;

  feature_cbs_G->click = callbacks->click;

  return ;
}



static gboolean featureClickCB(ParamStruct *params, ZMapFeature feature)
{

  /* call the function pointed to. This will cascade up the hierarchy
  ** to zmapControl.c where details of the object clicked on will be
  ** displayed in zmap->info_panel. */

  (*(feature_cbs_G->click))(params->window, params->window->app_data, feature);

  return FALSE;  // FALSE allows any other connected function to be run.
}



void zmapWindowDrawFeatures(ZMapWindow window, ZMapFeatureContext feature_context, GData *types)
{
  double offset = 100.0;
  float result;
  double x1, x2, y1, y2;
  GtkWidget *parent, *label, *vbox, *vscale, *frame;
  FooCanvas *canvas = window->canvas;
  GError    *channel_error = NULL;

  if (feature_context)  // Split an empty pane and you get here with no feature_context
    {
      foo_canvas_get_scroll_region(canvas, &x1, &y1, &x2, &y2);  // for display panel

      params.window = window;
      params.thisCanvas = canvas;
      params.height = (y2 - y1)*2.0; // arbitrarily increase the size for now
      params.length = feature_context->sequence_to_parent.c2 - feature_context->sequence_to_parent.c1;
      params.types = types;
      params.feature_context = feature_context;
      params.channel = g_io_channel_new_file("features.out", "w", &channel_error);

      if (!params.channel)
	printf("Unable to open output channel.  %d %s\n", channel_error->code, channel_error->message);

      if (params.length < 0) params.length *= -1.0;

      if (y2 >= y1)
	{
	  if ((feature_context->sequence_to_parent.c2 * params.height/params.length) > y2)
	    y2 = (feature_context->sequence_to_parent.c2 * params.height/params.length) + 100;
	}
      else
	{
	  if ((feature_context->sequence_to_parent.c1 * params.height/params.length) > y2)
	    y2 = (feature_context->sequence_to_parent.c1 * params.height/params.length) + 100;
	}

      // adjust the canvas to suit the sequence we're displaying
      foo_canvas_set_scroll_region(canvas, x1, y1, x2, y2);

      result = zmapDrawScale(canvas, offset, 
			     feature_context->sequence_to_parent.c1, 
			     feature_context->sequence_to_parent.c2);

      params.column_position = result + 100;  // a bit to the right of where we drew the scale bar

      if (feature_context)
	g_datalist_foreach(&(feature_context->feature_sets), zmapWindowProcessFeatureSet, &params);
    }

  if (channel_error)
    g_error_free(channel_error);

  g_io_channel_shutdown(params.channel, TRUE, &channel_error);
  return;
}



/* Called for each feature set, it then calls a routine to draw each of its features.  */
static void zmapWindowProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
  ParamStruct *params = (ParamStruct*)user_data;

  FooCanvasItem *col_group;  // each feature_set represents a column
  double column_spacing = 20.0;


  // NB for each column we create a new canvas group, with the initial y coordinate set to
  // 10.0 to just drop it a teeny bit from the top of the window.  All items live in the group.
  // Note that this y coord adjustment is directly linked to the the manipulation of 
  // params->height performed in zmapWindowDrawFeatures.  Change them together or not at all.
  params->thisType = (ZMapFeatureTypeStyle)g_datalist_get_data(&(params->types), feature_set->source) ;

  // context_key is used when a user clicks a canvas item.  The callback function needs to know
  // which feature is associated with the clicked object, so we use the GQuarks to look it up.
  params->feature_set = feature_set; 
  params->context_key = key_id;

  if (!params->thisType) // ie no method for this source
      zMapLogWarning("No ZMapType (aka method) found for source: %s", feature_set->source);
  else
    {
      params->column_position += column_spacing;
  
      params->columnGroup = foo_canvas_item_new(foo_canvas_root(params->thisCanvas),
						foo_canvas_group_get_type(),
						"x", (double)params->column_position,
						"y", (double)5.0,
						NULL);

      g_datalist_foreach(&(feature_set->features), zmapWindowProcessFeature, params) ;
    }
  return ;
}



/* ROB, what I have done may mess up your group structs...e.g. how does the group id get
 * into this routine ? Needs to be added to your params struct maybe ? I didn't try and sort
 * this out as I didn't want to mess stuff up */

static void zmapWindowProcessFeature(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature zMapFeature = (ZMapFeature)data ; 
  ParamStruct *params = (ParamStruct*)user_data;

  ZMapSpan zMapSpan, prevSpan;
  ZMapAlignBlock *zMapAlign, *prevAlign;
  int i, j;
  FooCanvasItem *feature_group, *object;
  int x1, x2, y0, y1, y2;
  float middle, line_width = 1.0;
  double magFactor = (params->height / params->length) * 0.98;
  const gchar *itemDataKey = "feature";
  FeatureKeys featureKeys;
  GString *buf = g_string_new("new");
  GError *channel_error = NULL;
  gsize bytes_written;


  g_string_printf(buf, "%s %d %d", 
		  zMapFeature->name,
		  zMapFeature->x1,
		  zMapFeature->x2);

  featureKeys = g_new0(FeatureKeyStruct, 1);
  featureKeys->feature_set = params->feature_set;
  featureKeys->context_key = params->context_key;
  featureKeys->feature_key = key_id;


  switch (zMapFeature->type)
    {
    case ZMAPFEATURE_BASIC: case ZMAPFEATURE_VARIATION:     /* type 0 is a basic, 5 is allele */
    case ZMAPFEATURE_BOUNDARY: case ZMAPFEATURE_SEQUENCE:   /* boundary is 6, sequence is 7 */
      g_string_append_printf(buf, "\n");

      if (zMapFeature->x2 > zMapFeature->x1)
	{
	  object = zmapDrawBox(FOO_CANVAS_ITEM(params->columnGroup), 0.0,
			       (zMapFeature->x1 * magFactor), 
			       params->thisType->width, 
			       (zMapFeature->x2 * magFactor),
			       &params->thisType->outline,
			       &params->thisType->foreground);

	  g_signal_connect(GTK_OBJECT(object), "event",
			   GTK_SIGNAL_FUNC(handleCanvasEvent), params) ;
	  
	  g_object_set_data(G_OBJECT(object), itemDataKey, featureKeys);
	}
      break;

    case ZMAPFEATURE_HOMOL:     /* type 1 is a homol */
      g_string_append_printf(buf, "\n");

      if (zMapFeature->x2 > zMapFeature->x1)
	{	      
	  object = zmapDrawBox(FOO_CANVAS_ITEM(params->columnGroup), 0.0,
			       (zMapFeature->x1 * magFactor), 
			       params->thisType->width, 
			       (zMapFeature->x2 * magFactor), 
			       &params->thisType->outline, 
			       &params->thisType->foreground);

	  g_signal_connect(GTK_OBJECT(object), "event",
			   GTK_SIGNAL_FUNC(handleCanvasEvent), params) ;

	  g_object_set_data(G_OBJECT(object), itemDataKey, featureKeys);
	}
      break;


    case ZMAPFEATURE_TRANSCRIPT:     /* type 4 is a transcript */
      feature_group = foo_canvas_item_new(FOO_CANVAS_GROUP(params->columnGroup),
					  foo_canvas_group_get_type(),
					  "x", (double)0.0,
					  "y", (double)zMapFeature->x1 * magFactor,
					  NULL);

      for (j = 1; j < zMapFeature->feature.transcript.exons->len; j++)
	{
	  zMapSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, j);
	  prevSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, j-1);
	  middle = (((prevSpan->x2 + zMapSpan->x1)/2) - zMapFeature->x1) * magFactor;
	  y0 = (prevSpan->x1 - zMapFeature->x1) * magFactor; // y coord of preceding exon
	  y1 = (prevSpan->x2 - zMapFeature->x1) * magFactor;
	  y2 = (zMapSpan->x1 - zMapFeature->x1) * magFactor;

	  // skip this intron if its preceding exon is too small to draw. 
	  // Note this is a kludge which will meet its come-uppance during zooming
	  // because zooming doesn't revisit this block of code, so tiny exons will
	  // appear when zoomed, but the intervening introns will be lost forever.

	  if (y2 > y1 && y0+2 <= y1)
	    {
	      zmapDrawLine(FOO_CANVAS_GROUP(feature_group), params->thisType->width/2, y1,
			   params->thisType->width, middle, 
			   &params->thisType->foreground, line_width);
	      zmapDrawLine(FOO_CANVAS_GROUP(feature_group), params->thisType->width, middle, 
			   params->thisType->width/2, y2, 
			   &params->thisType->foreground, line_width);
	    }
	}	  
      for (j = 0; j < zMapFeature->feature.transcript.exons->len; j++)
	{

	  zMapSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, j);
	      
	  g_string_append_printf(buf, " Transcript: %d %d\n", zMapSpan->x1, zMapSpan->x2);
	  if (zMapSpan->x2 > zMapSpan->x1)
	    {
	      if (j == 0)
		zmapDisplayText(FOO_CANVAS_GROUP(feature_group), zMapFeature->name, 
				"brown",40.0, (zMapSpan->x1 - zMapFeature->x1) * magFactor); 
	      object = zmapDrawBox(feature_group, 0.0,
				   ((zMapSpan->x1 - zMapFeature->x1) * magFactor), 
				   params->thisType->width,
				   ((zMapSpan->x2 - zMapFeature->x1) * magFactor),
				   &params->thisType->outline, 
				   &params->thisType->foreground);

	      g_signal_connect(GTK_OBJECT(object), "event",
			       GTK_SIGNAL_FUNC(handleCanvasEvent), params) ;

	      g_object_set_data(G_OBJECT(object), itemDataKey, featureKeys);
	    }
	}
      break;

    default:
      printf("Not displaying %s type %s\n", zMapFeature->name, zMapFeature->type);
      break;
    }

  g_io_channel_write_chars(params->channel, buf->str, -1, &bytes_written, &channel_error);
  g_string_free(buf, TRUE);
  if (channel_error)
    g_error_free(channel_error);

  if (object)
    g_signal_connect(GTK_OBJECT(object), "destroy",
		     GTK_SIGNAL_FUNC(freeObjectKeys), featureKeys);
  
  return;
}


static gboolean handleCanvasEvent(GtkWidget *widget, GdkEventClient *event, gpointer data)
{
  ParamStruct *params = (ParamStruct*)data;
  ZMapFeatureContext feature_context;
  ZMapFeatureSet feature_set;
  ZMapFeature feature;
  const gchar *itemDataKey = "feature";
  FeatureKeys featureKeys;
  gboolean result = FALSE;  // if not a BUTTON_PRESS, leave for any other event handlers.

  if (event->type == GDK_BUTTON_PRESS)
  {
    // retrieve the FeatureKeyStruct from the clicked object, obtain the feature_set from that and the
    // feature from that, using the two GQuarks, context_key and feature_key. Then call the 
    // click callback function, passing params and feature so the details of the clicked object
    // can be displayed in the info_panel.
    featureKeys = g_object_get_data(G_OBJECT(widget), itemDataKey);  
    feature_set = g_datalist_id_get_data(&(params->feature_context->feature_sets), featureKeys->context_key);
    feature = g_datalist_id_get_data(&(feature_set->features), featureKeys->feature_key); 
    featureClickCB(params, feature);                            
    result = TRUE;
  }

  return result;

}


static gboolean freeObjectKeys(GtkWidget *widget, gpointer data)
{
  FeatureKeys featureKeys = (FeatureKeyStruct*)data;

  g_free(featureKeys);

  return FALSE;
}

/****************** end of file ************************************/
