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
 * Last edited: Sep  3 14:22 2004 (rnc)
 * Created: Thu Jul 29 10:45:00 2004 (rnc)
 * CVS info:   $Id: zmapWindowDrawFeatures.c,v 1.13 2004-09-03 13:23:52 rnc Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindow_P.h>
#include <ZMap/zmapDraw.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapWindowDrawFeatures.h>


static void zmapWindowProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data);
static void zmapWindowProcessFeature(GQuark key_id, gpointer data, gpointer user_data);
static gboolean featureClickCB(ParamStruct *params, ZMapFeature feature);
static gboolean handleCanvasEvent(GtkWidget *widget, GdkEventClient *event, gpointer data);

/* These callback routines are static because they are set just once for the lifetime of the
 * process. */

/* Callbacks we make back to the level above us. */
static ZMapFeatureCallbacks feature_cbs_G = NULL ;

static ParamStruct params;

typedef struct _Keys {
    ZMapFeatureSet feature_set;
    GQuark context_key;
    GQuark feature_key;
} KeyStruct, *Keys;


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
  double offset = 20.0;
  float result;
  double x1, x2, y1, y2;
  GtkWidget *parent, *label, *vbox, *vscale, *frame;
  FooCanvas *canvas = window->canvas;

  if (feature_context)  // Split an empty pane and you get here with no feature_context
    {
      foo_canvas_get_scroll_region(canvas, &x1, &y1, &x2, &y2);  // for display panel

      params.window = window;
      params.thisCanvas = canvas;
      params.height = (y2 - y1)*2.0; // arbitrarily increase the size for now
      params.length = feature_context->sequence_to_parent.c2 - feature_context->sequence_to_parent.c1;
      params.column_position = 20.0;
      params.types = types;
      params.feature_context = feature_context;

      printf("parent c2: %d, child p2 %d\n", feature_context->sequence_to_parent.c2,
	     feature_context->sequence_to_parent.p2);

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

      if (feature_context)
	g_datalist_foreach(&(feature_context->feature_sets), zmapWindowProcessFeatureSet, &params);
    }
  return;
}



/* Called for each feature set, it then calls a routine to draw each of its features.  */
static void zmapWindowProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
  ParamStruct *params = (ParamStruct*)user_data;

  FooCanvasItem *col_group;  // each feature_set represents a column
  double column_spacing = 40.0;


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
						"y", (double)10.0,
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
  double magFactor = params->height/params->length;
  const gchar *key = "feature";
  Keys keys;

  //  printf("type %d name %s\n", zMapFeature->type, zMapFeature->name);

  keys = g_new0(KeyStruct, 1);
  keys->feature_set = params->feature_set;
  keys->context_key = params->context_key;
  keys->feature_key = key_id;

  switch (zMapFeature->type)
    {
    case ZMAPFEATURE_BASIC: case ZMAPFEATURE_VARIATION:     /* type 0 is a basic, 5 is allele */
    case ZMAPFEATURE_BOUNDARY: case ZMAPFEATURE_SEQUENCE:   /* boundary is 6, sequence is 7 */

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
	  
	  g_object_set_data(G_OBJECT(object), key, keys);
	}
      break;

    case ZMAPFEATURE_HOMOL:     /* type 1 is a homol */

      if (zMapFeature->feature.homol.y2 > zMapFeature->feature.homol.y1)
	{	      
	  object = zmapDrawBox(FOO_CANVAS_ITEM(params->columnGroup), 0.0,
			       (zMapFeature->feature.homol.y1 * magFactor), 
			       params->thisType->width, 
			       (zMapFeature->feature.homol.y2 * magFactor),
			       &params->thisType->outline, 
			       &params->thisType->foreground);

	  g_signal_connect(GTK_OBJECT(object), "event",
			   GTK_SIGNAL_FUNC(handleCanvasEvent), params) ;

	  g_object_set_data(G_OBJECT(object), key, keys);
	}
      break;


    case ZMAPFEATURE_TRANSCRIPT:     /* type 4 is a transcript */
      feature_group = foo_canvas_item_new(foo_canvas_root(params->thisCanvas),
				  foo_canvas_group_get_type(),
				  "x", (double)params->column_position,
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

	      g_object_set_data(G_OBJECT(object), key, keys);
	    }
	}
      break;

    default:
      break;
    }

  
  return;
}


static gboolean handleCanvasEvent(GtkWidget *widget, GdkEventClient *event, gpointer data)
{
  ParamStruct *params = (ParamStruct*)data;
  ZMapFeatureContext feature_context;
  ZMapFeatureSet feature_set;
  ZMapFeature feature;
  const gchar *key = "feature";
  Keys keys;

  switch (event->type)
  {
  case GDK_BUTTON_PRESS:
    // retrieve the KeyStruct from the clicked object, obtain the feature_set from that and the
    // feature from that, using the two GQuarks, context_key and feature_key. Then call the 
    // click callback function, passing params and feature so the details of the clicked object
    // can be displayed in the info_panel.
    keys = g_object_get_data(G_OBJECT(widget), key);  
    feature_set = g_datalist_id_get_data(&(params->feature_context->feature_sets), keys->context_key);
    feature = g_datalist_id_get_data(&(feature_set->features), keys->feature_key); 
    featureClickCB(params, feature);                            
    return TRUE;

  default: return FALSE;
  }
  /* Event not handled; try parent item */
  return FALSE;

}



/****************** end of file ************************************/
