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
 * Last edited: Nov 29 10:19 2004 (rnc)
 * Created: Thu Jul 29 10:45:00 2004 (rnc)
 * CVS info:   $Id: zmapWindowDrawFeatures.c,v 1.39 2004-11-29 13:56:09 rnc Exp $
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
static void     columnClickCB     (ZMapWindow window, ZMapFeatureItem featureItem);
static gboolean handleCanvasEvent (FooCanvasItem *item, GdkEventButton *event, gpointer data);
static FooCanvasItem *createColumn(ZMapCanvasDataStruct *canvasData, gboolean forward, 
				   double position, gchar *type_name);

static void getTextDimensions(FooCanvasGroup *group, double *width_out, double *height_out) ;
static void freeFeatureItem(gpointer data);


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
gboolean zMapFeatureClickCB(ZMapWindow window, ZMapFeature feature)
{
  (*(feature_cbs_G->click))(window, window->app_data, feature);

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
  GtkAdjustment *h_adj ;
  ZMapCanvasData canvasData;

 
  if (feature_context)  /* Split an empty pane and you get here with no feature_context */
    {
      double border_world ;

      canvasData = g_new0(ZMapCanvasDataStruct, 1) ;
      canvasData->window = window;
      canvasData->canvas = window->canvas;

      g_object_set_data(G_OBJECT(window->canvas), "canvasData", canvasData);

      window->scaleBarOffset  = SCALEBAR_OFFSET;
      window->types           = types;
      canvasData->feature_context = feature_context;
      window->seqLength
	= feature_context->sequence_to_parent.c2 - feature_context->sequence_to_parent.c1 + 1;
      window->seq_start = feature_context->sequence_to_parent.c1 ;
      window->seq_end   = feature_context->sequence_to_parent.c2 ;

      
      gtk_widget_show_all (window->parent_widget);

      /* Set the zoom factor. By default we start at min zoom, BUT note that if user has a
       * very short sequence it may be displayed at maximum zoom already. */
      h_adj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(window->scrolledWindow));

      getTextDimensions(foo_canvas_root(window->canvas),
			NULL, &window->text_height) ;

      /* Set border space for top/bottom of sequence. */
      window->border_pixels = window->text_height * 2 ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* THIS "- 1" FIDDLE WILL NOT BE NECESSARY WHEN THE BOUNDARY STUFF IS DONE. */
      window->zoom_factor = (v_adj->page_size - 1) / window->seqLength;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      /* if the screen is being split, use the previous zoom_factor
       * so the data is displayed the same size */
      if (window->zoom_factor == 0.0)
	window->zoom_factor = zmapWindowCalcZoomFactor(window);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* I NEED TO FINISH THIS OFF BUT NEED TO DO A CHECK IN OF MY CODE.... */

      /* adjust zoom to allow for top/bottom borders... */
      border_world = window->border_pixels / window->zoom_factor ;

      window->zoom_factor = v_adj->page_size / (window->seqLength + (2.0 * border_world))  ;
							    /* Can't use c2w call because we
							       haven't set the canvas zoom yet. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      /* Record min/max zooms, the max zoom we want is large enough to hold the text required
       * to display a DNA base or other text + a bit. */
      zmapWindowCalcMaxZoom(window);

      if (window->zoom_status == ZMAP_ZOOM_INIT)  /* ie 0.0 */
	window->zoom_status = ZMAP_ZOOM_MIN ;

      if (window->zoom_factor > window->max_zoom)
	{
	  window->zoom_factor = window->max_zoom ;
	  window->zoom_status = ZMAP_ZOOM_FIXED ;
	}


      zMapAssert(window->zoom_factor);
      if (window->min_zoom == 0.0)
	window->min_zoom = window->zoom_factor ;

      foo_canvas_set_pixels_per_unit_xy(window->canvas, 1.0, window->zoom_factor) ;


      GTK_LAYOUT(window->canvas)->vadjustment->page_increment = GTK_WIDGET(window->canvas)->allocation.height - 50;
  

      /* Set root group to begin at 0, _vital_ for later feature placement. */
      /*      window->group = foo_canvas_item_new(foo_canvas_root(window->canvas),
					  foo_canvas_group_get_type(),
					  "x", 0.0, "y", 0.0,
					  NULL) ;
      never used */
      window->scaleBarGroup = zmapDrawScale(window->canvas, 
						window->scaleBarOffset, window->zoom_factor,
						feature_context->sequence_to_parent.c1, 
						feature_context->sequence_to_parent.c2);

      canvasData->forward_column_pos = window->scaleBarOffset + SCALEBAR_WIDTH + (2 * COLUMN_SPACING);
      canvasData->reverse_column_pos = window->scaleBarOffset - 40;                
      /* NB there's a flaw here: too many visible upstrand groups and we'll go left of the edge of
      ** the window. Should really be keeping track and adjusting scroll region if necessary. */

      if (feature_context)
	g_datalist_foreach(&(feature_context->feature_sets), ProcessFeatureSet, canvasData);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Try here to get stuff set up....HAS TO GO AFTER ALL THE FEATURE DRAWING OTHERWISE
       * SCROLL REGION GETS RESET TO LIE AROUND FEATURES ONLY. */
      foo_canvas_set_scroll_region(window->canvas,
				   0.0,
				   (window->seq_start - border_world),
				   h_adj->page_size,
				   (window->seq_end + border_world)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      /* Try here to get stuff set up.... */
      foo_canvas_set_scroll_region(window->canvas,
				   0.0,
				   window->seq_start,
				   h_adj->page_size,
				   window->seq_end) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      zmapWindowPrintCanvas(window->canvas) ; 
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    }

  g_free(canvasData);

  return;
}



void zmapWindowHighlightObject(FooCanvasItem *feature, ZMapWindow window, ZMapFeatureTypeStyle thisType)
{                                               
  GdkColor foreground;


  /* if any other feature is currently in focus, revert it to its std colours */
  if (window->focusFeature)
    foo_canvas_item_set(FOO_CANVAS_ITEM(window->focusFeature),
 			"fill_color_gdk", &window->focusType->foreground,
			NULL);

  /* set foreground GdkColor to be the inverse of current settings */

  foreground.red   = (65535 - thisType->foreground.red  );
  foreground.green = (65535 - thisType->foreground.green);
  foreground.blue  = (65535 - thisType->foreground.blue );

  foo_canvas_item_set(feature,
		      "fill_color_gdk", &foreground,
		      NULL);
 
  /* store these settings and reshow the feature */
  window->focusFeature = feature;
  window->focusType    = thisType;
  gtk_widget_show_all(GTK_WIDGET(window->canvas)); 

  return;
}


/************************ internal functions **************************************/


/* Called for each feature set, it then calls a routine to draw each of its features.  */
static void ProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet  feature_set = (ZMapFeatureSet)data ;
  ZMapCanvasData  canvasData  = (ZMapCanvasDataStruct*)user_data;
  GString        *source      = g_string_new(feature_set->source);

  source = g_string_ascii_down(source); /* does it in place */
  canvasData->thisType = (ZMapFeatureTypeStyle)g_datalist_get_data(&(canvasData->window->types), source->str);
  g_string_free(source, FALSE);
  zMapAssert(canvasData->thisType);

  /* context_key is used when a user clicks a canvas item.  The callback function needs to know
  ** which feature is associated with the clicked object, so we use the GQuarks to look it up.*/
  canvasData->feature_set = feature_set; 
  canvasData->context_key = key_id;

  canvasData->forward_column_group = createColumn(canvasData, TRUE, 
					 canvasData->forward_column_pos, source->str);
  canvasData->forward_column_pos += (canvasData->thisType->width + COLUMN_SPACING);

  if (canvasData->thisType->showUpStrand)
    {
      canvasData->reverse_column_group = createColumn(canvasData, FALSE, 
					     canvasData->reverse_column_pos, source->str);
      canvasData->reverse_column_pos -= (canvasData->thisType->width + COLUMN_SPACING);
    }
  

  /* Expand the scroll region _horizontally_ to include the latest forward strand column. */
  foo_canvas_set_scroll_region(canvasData->canvas, 1.0, canvasData->window->seq_start, 
			       canvasData->forward_column_pos + 20, 
			       canvasData->window->seq_end) ;
  
  g_datalist_foreach(&(feature_set->features), ProcessFeature, canvasData) ;

  return ;
}



/* createColumn
 */
static FooCanvasItem *createColumn(ZMapCanvasData canvasData,
				   gboolean forward,
				   double position,
				   gchar *type_name)
{
  GdkColor       white;
  FooCanvasItem *group, *boundingBox;
  ZMapCol        column = g_new0(ZMapColStruct, 1); /* destroyed with ZMapWindow */
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
				    "y1", canvasData->window->seq_start,
				    "x2", (double)canvasData->thisType->width,
				    "y2", canvasData->window->seq_end,
				    "fill_color_gdk", &white,
				    NULL);

  /* store a pointer to the column to enable hide/unhide while zooming */
  column->type = canvasData->thisType;
  column->type_name = type_name;
  column->column = group;
  column->forward = forward;

  g_ptr_array_add(canvasData->window->columns, column);
  
  /* Decide whether or not this column should be visible */
  /* thisType->min_mag is in bases per line, but window->zoom_factor is pixels per base */
  min_mag = (canvasData->thisType->min_mag ? 
	                 canvasData->window->max_zoom/canvasData->thisType->min_mag : 0.0);
  
  if (canvasData->window->zoom_factor < min_mag)
    foo_canvas_item_hide(FOO_CANVAS_ITEM(group));
  
  return group;
}




static void ProcessFeature(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature      zMapFeature = (ZMapFeature)data ; 
  ZMapCanvasData   canvasData  = (ZMapCanvasDataStruct*)user_data;
  ZMapSpan         zMapSpan, prevSpan;
  int              i;
  FooCanvasItem   *columnGroup, *feature_group, *object = NULL, *box = NULL;
  float            line_width  = 1.0;
  ZMapFeatureItem  featureItem = g_new0(ZMapFeatureItemStruct, 1);
  double           position;

  GdkColor black;
  GdkColor white;

  gdk_color_parse("black", &black);
  gdk_color_parse("white", &white);


  /* decide whether this feature lives on the up or down strand */
  if (zMapFeature->strand == ZMAPSTRAND_DOWN || zMapFeature->strand == ZMAPSTRAND_NONE)
    {
      position    = canvasData->forward_column_pos;
      columnGroup = canvasData->forward_column_group;
    }
  else
    {
      position    = canvasData->reverse_column_pos;
      columnGroup = canvasData->reverse_column_group;
    }

  featureItem->feature_set = canvasData->feature_set;
  featureItem->feature_key = key_id;
  featureItem->strand      = zMapFeature->strand;

  if (zMapFeature->strand == ZMAPSTRAND_DOWN
      || zMapFeature->strand == ZMAPSTRAND_NONE
      || (zMapFeature->strand == ZMAPSTRAND_UP && canvasData->thisType->showUpStrand == TRUE))
    {
      switch (zMapFeature->type)
	{
	case ZMAPFEATURE_BASIC: case ZMAPFEATURE_HOMOL:
	case ZMAPFEATURE_VARIATION:
	case ZMAPFEATURE_BOUNDARY: case ZMAPFEATURE_SEQUENCE:

	  object = zmapDrawBox(FOO_CANVAS_ITEM(columnGroup), 0.0,
			       zMapFeature->x1,
			       canvasData->thisType->width, 
			       zMapFeature->x2,
			       &canvasData->thisType->outline,
			       &canvasData->thisType->foreground);

	  g_object_set_data(G_OBJECT(object), "feature", featureItem);
	  
	  g_signal_connect(G_OBJECT(object), "event",
			   G_CALLBACK(handleCanvasEvent), canvasData->window) ;
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
      
	    /* first we draw the introns, then the exons.  Introns have an invisible
	     * box around them to allow the user to click there and get a reaction. */
	    for (i = 1; i < zMapFeature->feature.transcript.exons->len; i++)  
	      {
		zMapSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, i);
		prevSpan = &g_array_index(zMapFeature->feature.transcript.exons, ZMapSpanStruct, i-1);
		if (zMapSpan->x1 > prevSpan->x2)
		  {
		    /*		    box = zmapDrawBox(feature_group, 
				      0.0, 
				      (prevSpan->x2 - zMapFeature->x1),
				      canvasData->thisType->width, 
				      (zMapSpan->x1 - zMapFeature->x1), 
				      &black, &white); 

		    g_object_set_data(G_OBJECT(box), "feature", featureItem);

		    g_signal_connect(G_OBJECT(box), "event",
				     G_CALLBACK(handleCanvasEvent), canvasData->window) ;
		    */
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

		box = zmapDrawBox(feature_group, 
				  0.0,
				  (zMapSpan->x1 - zMapFeature->x1), 
				  canvasData->thisType->width,
				  (zMapSpan->x2 - zMapFeature->x1), 
				  &canvasData->thisType->outline, 
				  &canvasData->thisType->foreground);

		g_object_set_data(G_OBJECT(box), "feature", featureItem);
		g_signal_connect(G_OBJECT(box), "event",
				 G_CALLBACK(handleCanvasEvent), canvasData->window) ;
		if (!object)
		  object = box;
	      }
	  }
	  break;

	default:
	  printf("Not displaying %s type %d\n", zMapFeature->name, zMapFeature->type);
	  break;
	}

      if (object)
	{
	  /* connect the object to its featureItem, add that into the GData in the ZMapWindow,
	   * connect any event that object emits to handleCanvasEvent().  */
	  featureItem->canvasItem = object;
	  g_datalist_id_set_data_full(&(canvasData->window->featureItems), key_id, featureItem, freeFeatureItem);
 	}
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
  ZMapWindow  window = (ZMapWindowStruct*)data;
  ZMapFeature feature;
  ZMapFeatureItem featureItem;
  gboolean result = FALSE;  /* if not a BUTTON_PRESS, leave for any other event handlers. */
  GString     *source;
  ZMapFeatureTypeStyle thisType;

  /* For now, we go on BUTTON_RELEASE since, after zooming, the canvas never emits
  ** a BUTTON_PRESS event.  Maybe when we find out why, we'll change this.  Or maybe not. */
  if (event->type == GDK_BUTTON_RELEASE)  /* GDK_BUTTON_PRESS is 4, RELEASE 7 */
  {
    /* retrieve the FeatureItemStruct from the clicked object, obtain the feature_set from that and the
    ** feature from that, using the GQuark feature_key. Then call the 
    ** click callback function, passing the ZMapWindow and feature so the details of the clicked object
    ** can be displayed in the info_panel. */

    featureItem = g_object_get_data(G_OBJECT(item), "feature");  
    zMapAssert(featureItem);

    if (featureItem->feature_key)
      {
	feature = g_datalist_id_get_data(&(featureItem->feature_set->features), featureItem->feature_key); 
	zMapFeatureClickCB(window, feature);   

	/* lowercase the source as all stored types are lowercase */                         
	source = g_string_new(featureItem->feature_set->source);
	source = g_string_ascii_down(source);
	thisType = (ZMapFeatureTypeStyle)g_datalist_get_data(&(window->types),source->str);
	g_string_free(source, FALSE); 
	zMapAssert(thisType);
									
	foo_canvas_item_raise_to_top(item);

  	zmapWindowHighlightObject(item, window, thisType);

	result = TRUE;
      }

    /* if this was a right-click, display a list of the features in this column so the user
    ** can select one and have the display scroll to it. */
    if (event->button == 3)
      {
	if (featureItem)
	  {
	    /* display selectable list of features */
	    columnClickCB(window, featureItem);
	    result = TRUE;
	  }
      }
  }


  return result;
}



static void columnClickCB(ZMapWindow window, ZMapFeatureItem featureItem)
{
  (*(feature_cbs_G->rightClick))(window, featureItem);

  return;
}



/* Find out the text size for a group. */
static void getTextDimensions(FooCanvasGroup *group, double *width_out, double *height_out)
{
  double width = -1.0, height = -1.0 ;
  FooCanvasItem *item ;

  item = foo_canvas_item_new(group,
			     FOO_TYPE_CANVAS_TEXT,
			     "x", -400.0, "y", 0.0, "text", "dummy",
			     NULL);

  g_object_get(GTK_OBJECT(item),
	       "FooCanvasText::text_width", &width,
	       "FooCanvasText::text_height", &height,
	       NULL) ;

  gtk_object_destroy(GTK_OBJECT(item));

  if (width_out)
    *width_out = width ;
  if (height_out)
    *height_out = height ;

  return ;
}




/****************** end of file ************************************/
