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
 * Last edited: Feb 25 16:13 2005 (edgrif)
 * Created: Thu Jul 29 10:45:00 2004 (rnc)
 * CVS info:   $Id: zmapWindowDrawFeatures.c,v 1.45 2005-02-25 16:45:44 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindow_P.h>
#include <ZMap/zmapDraw.h>
#include <ZMap/zmapUtils.h>






#define SCALEBAR_OFFSET   0.0
#define SCALEBAR_WIDTH   40.0
#define COLUMN_SPACING    5.0



/* parameters passed between the various functions drawing the features on the canvas */
typedef struct _ZMapCanvasDataStruct
{
  ZMapWindow           window ;
  FooCanvas           *canvas ;
  FooCanvasItem       *forward_column_group ;
  FooCanvasItem       *reverse_column_group ;
  double               forward_column_pos ;
  double               reverse_column_pos ;

  ZMapFeatureContext   full_context ;

  ZMapFeatureSet       feature_set;
  GQuark               context_key;
  ZMapFeatureTypeStyle thisType;

} ZMapCanvasDataStruct, *ZMapCanvasData ;





static gboolean canvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;



static void     showFeatureList     (ZMapWindow window, ZMapFeatureItem featureItem) ;
static void     ProcessFeatureSet (GQuark key_id, gpointer data, gpointer user_data) ;
static void     ProcessFeature    (GQuark key_id, gpointer data, gpointer user_data) ;
static void getTextDimensions(FooCanvasGroup *group, double *width_out, double *height_out) ;
static void freeFeatureItem  (gpointer data);
static void freeLongItem     (gpointer data);
static void storeLongItem(ZMapWindow window, FooCanvasItem *item, int start, int end,
			  GQuark key_id, char *name);

void destroyAlignment(gpointer data) ;



/* Drawing coordinates: PLEASE READ THIS BEFORE YOU START MESSING ABOUT WITH ANYTHING...
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


/************************ external functions ************************************************/



/* This stuff with featureClick is all messed up, it gets called from the list code when
 * really it should be just part of the callback system. */

/** Callback to display feature details on the window.
 *  
 * Control passes up the hierarchy to zmapControl 
 * where a few details of the selected feature are 
 * displayed on the info_panel of the window. 
 */
gboolean zMapWindowFeatureClickCB(ZMapWindow window, ZMapFeature feature)
{
  (*(window->caller_cbs->click))(window, window->app_data, feature);

  return FALSE;  /* FALSE allows any other connected function to be run. */
}



/* Draw features on the canvas, I think that this routine should _not_ get called unless 
 * there is actually data to draw.......
 * 
 * full_context contains all the features.
 * diff_context contains just the features from full_context that are newly added and therefore
 *              have not yet been drawn.
 * 
 * So NOTE that if no features have yet been drawn then  full_context == diff_context
 * 
 *  */
void zmapWindowDrawFeatures(ZMapWindow window,
			    ZMapFeatureContext full_context, ZMapFeatureContext diff_context,
			    GData *types)
{
  GtkAdjustment *h_adj;
  double zoom_factor ;
  ZMapCanvasDataStruct canvas_data = {NULL} ;		    /* Rest of struct gets set to zero. */


  zMapAssert(window && full_context && diff_context && types) ;



  /* FUDGED CODE FOR NOW.....we want to deal with multiple alignments but zmapview etc. don't
   * deal with this at the moment so neither do we. */
  if (!window->alignments)
    {
      ZMapWindowAlignment alignment ;


      g_datalist_init(&window->alignments) ;


      alignment = zmapWindowAlignmentCreate(window->sequence, window, foo_canvas_root(window->canvas)) ;
      g_datalist_set_data_full(&window->alignments, window->sequence, alignment, destroyAlignment) ;
    }


  window->scaleBarOffset  = SCALEBAR_OFFSET;
  window->types           = types;

  window->seqLength
    = full_context->sequence_to_parent.c2 - full_context->sequence_to_parent.c1 + 1 ;

  window->seq_start = full_context->sequence_to_parent.c1 ;
  window->seq_end   = full_context->sequence_to_parent.c2 ;


  h_adj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(window->scrolledWindow)) ;


  /* Need text dimensions to set maximum zoom. */
  getTextDimensions(foo_canvas_root(window->canvas), NULL, &window->text_height) ;

  /* Set border space for top/bottom of sequence. */
  window->border_pixels = window->text_height * 2 ;


  /* Set the zoom factor. By default we start at min zoom, BUT note that if user has a
   * very short sequence it may be displayed at maximum zoom already. */

  /* Zoom_factor: calculate it locally so we can set zoom_status
   * properly if the whole of the sequence is visible and we're
   * already at max zoom.  Then check to see if there's already
   * a value in the ZMapWindow and only if not, use the new one. 
   * So far, if window->zoom_factor and min_zoom are zero we 
   * assume this is the first window, not a splitting one. */
  zoom_factor = zmapWindowCalcZoomFactor(window);


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
  if (window->max_zoom == 0.0)
    window->max_zoom = window->text_height + (double)(ZMAP_WINDOW_TEXT_BORDER * 2) ;

  if (window->zoom_status == ZMAP_ZOOM_INIT)
    window->zoom_status = ZMAP_ZOOM_MIN ;

  if (zoom_factor > window->max_zoom)
    {
      zoom_factor = window->max_zoom ;
      window->zoom_status = ZMAP_ZOOM_FIXED ;
    }

  /* if the screen is being split, use the previous zoom_factor
   * so the data is displayed the same size */
  if (window->zoom_factor == 0.0)
    window->zoom_factor = zoom_factor;


  if (window->min_zoom == 0.0)
    window->min_zoom = window->zoom_factor ;


  foo_canvas_set_pixels_per_unit_xy(window->canvas, 1.0, window->zoom_factor) ;


  /* OH DEAR, THIS IS HORRIBLE.......... */
  zmapWindowSetPageIncr(window);
  

  /* WHY DID YOU COMMENT THIS OUT ROB ? */
  /* Set root group to begin at 0, _vital_ for later feature placement. */
  /*      window->group = foo_canvas_item_new(foo_canvas_root(window->canvas),
	  foo_canvas_group_get_type(),
	  "x", 0.0, "y", 0.0,
	  NULL) ;
	  never used */



  canvas_data.window = window;
  canvas_data.canvas = window->canvas;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  WE NEED AN OVERALL GROUP HERE............
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  canvas_data.forward_column_pos = window->scaleBarOffset + (SCALEBAR_WIDTH + (2 * COLUMN_SPACING)) ;
  canvas_data.reverse_column_pos = window->scaleBarOffset - (2 * COLUMN_SPACING) ;



  /* Draw all the features, so much in so few lines...sigh... */
  canvas_data.full_context = full_context ;
  g_datalist_foreach(&(diff_context->feature_sets), ProcessFeatureSet, &canvas_data) ;



  /* If we're splitting a window, scroll to the same point as we were
   * at in the previous window. */
  if (window->focusQuark)
    zMapWindowScrollToItem(window, window->typeName, window->focusQuark);
  else
    {
      double scroll_x1, scroll_y1, scroll_x2, scroll_y2 ;

      foo_canvas_get_scroll_region(window->canvas, &scroll_x1, &scroll_y1, &scroll_x2, &scroll_y2) ;
      if (scroll_x1 == 0 && scroll_y1 == 0 && scroll_x2 == 0 && scroll_y2 == 0)
	foo_canvas_set_scroll_region(window->canvas,
				     0.0, window->seq_start,
				     h_adj->page_size, window->seq_end) ;
    }


  window->scaleBarGroup = zmapDrawScale(window->canvas,
					window->scaleBarOffset, window->zoom_factor,
					full_context->sequence_to_parent.c1,
					full_context->sequence_to_parent.c2,
					&(window->major_scale_units),
					&(window->minor_scale_units)) ;


  /* I'm not sure why this is here....can this routine be called when the features might
   * be too long........check this out....... */
  if (window->longItems)
    g_datalist_foreach(&(window->longItems), zmapWindowCropLongFeature, window);




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Now hide/show any columns that have specific show/hide zoom factors set. */
  if (window->columns)
    zmapHideUnhideColumns(window) ;

  /* we need a hide columns call here..... */

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapWindowPrintCanvas(window->canvas) ; 
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      
  /* We want the canvas to be the focus widget of its "window" otherwise keyboard input
   * (i.e. short cuts) will be delivered to some other widget. We do this here because
   * I think we may need a widget window to exist for this call to work. */
  gtk_widget_grab_focus(GTK_WIDGET(window->canvas)) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  {
    FooCanvasItem *item ;

    item = FOO_CANVAS_ITEM(foo_canvas_root(window->canvas)) ;

    printf("got it\n") ;
  }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
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
  gtk_widget_show_all(GTK_WIDGET(window->canvas)) ;

  return ;
}





/************************ internal functions **************************************/



/* Called for each feature set, it then calls a routine to draw each of its features.  */
static void ProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
  ZMapCanvasData canvas_data  = (ZMapCanvasData)user_data ;
  GString *source ;
  char *source_name ;
  ZMapWindowColumn column ;

  /* this is hacked for now as we only have one alignment.... */
  ZMapWindowAlignment alignment ;



  /* this lowercasing should be in a function...... */
  source = g_string_new(feature_set->source) ;
  source = g_string_ascii_down(source) ;		    /* does it in place */

  canvas_data->thisType = (ZMapFeatureTypeStyle)g_datalist_get_data(&(canvas_data->window->types),
								    source->str) ;
  zMapAssert(canvas_data->thisType) ;
  source_name = source->str ;

  g_string_free(source, FALSE) ;


  canvas_data->feature_set = feature_set ;


  /* context_key is used when a user clicks a canvas item.  The callback function needs to know
   * which feature is associated with the clicked object, so we use the GQuarks to look it up.*/
  canvas_data->context_key = key_id ;


  /* NOTE THAT WE ARE GOING TO CHANGE THE COLUMN STUFF TO HAVE FORWARD/REVERSE STRAND FEATURES
   * IN THE SAME COLUMN AND SWITCHABLE.... */
  alignment = (ZMapWindowAlignment)g_datalist_get_data(&(canvas_data->window->alignments),
						       canvas_data->window->sequence) ;

  /* This is hacked up a bit, it shouldn't really need to return the column group but it will
   * do for now....... */
  canvas_data->forward_column_group = canvas_data->reverse_column_group = 
    zmapWindowAlignmentAddColumn(alignment, source_name,
				 canvas_data->forward_column_pos, canvas_data->thisType) ;


  canvas_data->forward_column_pos += (canvas_data->thisType->width + COLUMN_SPACING) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* If the column doesn't already exist we need to find a new one. */
  if (!(column = findColumn(canvas_data->window->columns, source_name, TRUE)))
    {
      column = createColumn(canvas_data, TRUE, 
			    canvas_data->forward_column_pos, source_name) ;

      g_ptr_array_add(canvas_data->window->columns, column) ;

      canvas_data->forward_column_pos += (canvas_data->thisType->width + COLUMN_SPACING) ;
    }

  canvas_data->forward_column_group = column->group ;

  if (canvas_data->thisType->showUpStrand)
    {
      if (!(column = findColumn(canvas_data->window->columns, source_name, FALSE)))
	{
	  canvas_data->reverse_column_pos -= canvas_data->thisType->width ;

	  column = createColumn(canvas_data, TRUE, 
				canvas_data->reverse_column_pos, source_name) ;

	  canvas_data->reverse_column_pos -= COLUMN_SPACING ;

	  g_ptr_array_add(canvas_data->window->columns, column) ;
	}

      canvas_data->reverse_column_group = column->group ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  
  /* Here we should be querying the size of the alignment group and using that to set the
   * scrolled region...i.e. do it dynamically... */

  /* Expand the scroll region _horizontally_ to include the latest forward strand column. */
  foo_canvas_set_scroll_region(canvas_data->canvas,
			       canvas_data->reverse_column_pos,
			       canvas_data->window->seq_start, 
			       canvas_data->forward_column_pos, 
			       canvas_data->window->seq_end) ;


  /* Now draw all the features in the column. */
  g_datalist_foreach(&(feature_set->features), ProcessFeature, canvas_data) ;


  /* Free source_name ???? */

  return ;
}


/* Called for each individual feature. */
static void ProcessFeature(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ; 
  ZMapCanvasData canvas_data  = (ZMapCanvasDataStruct*)user_data ;
  ZMapSpan zMapSpan, prevSpan ;
  int i ;
  FooCanvasItem *columnGroup, *feature_group, *object = NULL, *box = NULL ;
  float line_width  = 1.0 ;
  ZMapFeatureItem feature_item = g_new0(ZMapFeatureItemStruct, 1) ;
  double position ;
  GdkColor black ;
  GdkColor white ;


  gdk_color_parse("black", &black) ;
  gdk_color_parse("white", &white) ;

  /* decide whether this feature lives on the up or down strand */
  if (feature->strand == ZMAPSTRAND_DOWN || feature->strand == ZMAPSTRAND_NONE)
    {
      position    = canvas_data->forward_column_pos;
      columnGroup = canvas_data->forward_column_group;
    }
  else
    {
      position    = canvas_data->reverse_column_pos;
      columnGroup = canvas_data->reverse_column_group;
    }

  feature_item->feature_set = canvas_data->feature_set;
  feature_item->feature_key = key_id;
  feature_item->strand      = feature->strand;

  if (feature->strand == ZMAPSTRAND_DOWN
      || feature->strand == ZMAPSTRAND_NONE
      || (feature->strand == ZMAPSTRAND_UP && canvas_data->thisType->showUpStrand == TRUE))
    {
      switch (feature->type)
	{
	case ZMAPFEATURE_BASIC: case ZMAPFEATURE_HOMOL:
	case ZMAPFEATURE_VARIATION:
	case ZMAPFEATURE_BOUNDARY: case ZMAPFEATURE_SEQUENCE:

	  object = zmapDrawBox(FOO_CANVAS_ITEM(columnGroup), 0.0,
			       feature->x1,
			       canvas_data->thisType->width, 
			       feature->x2,
			       &canvas_data->thisType->outline,
			       &canvas_data->thisType->foreground);

	  g_object_set_data(G_OBJECT(object), "feature", feature_item);

	  g_signal_connect(GTK_OBJECT(object), "event",
			   GTK_SIGNAL_FUNC(canvasItemEventCB), canvas_data->window) ;

	  /* any object longer than about 1.5k will need to be cropped as we zoom in to avoid
	   * going over the 30k XWindows limit, so we store them in ZMapWindow->longItem struct. */
	  if ((feature->x2 - feature->x1)
	      > (ZMAP_WINDOW_MAX_WINDOW / canvas_data->window->text_height))
	    storeLongItem(canvas_data->window, object, feature->x1, feature->x2, key_id,
			  feature->name);

	  break;

	case ZMAPFEATURE_TRANSCRIPT:
	  {
	    /* Note that for transcripts the boxes and lines are contained in a canvas group
	     * and that therefore their coords are relative to the start of the group which
	     * is the start of the transcript, i.e. feature->x1. */

	    feature_group = foo_canvas_item_new(FOO_CANVAS_GROUP(columnGroup),
						foo_canvas_group_get_type(),
						"x", (double)0.0,
						"y", (double)feature->x1,
						NULL);
      
	    /* first we draw the introns, then the exons.  Introns will have an invisible
	     * box around them to allow the user to click there and get a reaction. */
	    for (i = 1; i < feature->feature.transcript.exons->len; i++)  
	      {
		zMapSpan = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i);
		prevSpan = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i-1);
		if (zMapSpan->x1 > prevSpan->x2)
		  {
		    /*		    box = zmapDrawBox(feature_group, 
				      0.0, 
				      (prevSpan->x2 - feature->x1),
				      canvas_data->thisType->width, 
				      (zMapSpan->x1 - feature->x1), 
				      &black, &white); 

		    g_object_set_data(G_OBJECT(box), "feature", feature_item);

		    g_signal_connect(G_OBJECT(box), "event",
				     G_CALLBACK(canvasItemEventCB), canvas_data->window) ;
		    */
		    zmapDrawLine(FOO_CANVAS_GROUP(feature_group), 
				 canvas_data->thisType->width/2, 
				 (prevSpan->x2 - feature->x1),
				 canvas_data->thisType->width  ,
				 (((prevSpan->x2 + zMapSpan->x1)/2) - feature->x1),
				 &canvas_data->thisType->foreground, line_width);
			       
		    zmapDrawLine(FOO_CANVAS_GROUP(feature_group), 
				 canvas_data->thisType->width  , 
				 (((prevSpan->x2 + zMapSpan->x1)/2) - feature->x1),
				 canvas_data->thisType->width/2, 
				 (zMapSpan->x1 - feature->x1), 
				 &canvas_data->thisType->foreground, line_width);
		  }
	      }
	  
	    for (i = 0; i < feature->feature.transcript.exons->len; i++)
	      {
		zMapSpan = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i);

		box = zmapDrawBox(feature_group, 
				  0.0,
				  (zMapSpan->x1 - feature->x1), 
				  canvas_data->thisType->width,
				  (zMapSpan->x2 - feature->x1), 
				  &canvas_data->thisType->outline, 
				  &canvas_data->thisType->foreground);

		g_object_set_data(G_OBJECT(box), "feature", feature_item);

		g_signal_connect(GTK_OBJECT(box), "event",
				 GTK_SIGNAL_FUNC(canvasItemEventCB), canvas_data->window) ;


		/* THIS CODE SHOULD BE LOOKING TO SEE IF THE ITEM IS LONGER THAN THE CANVAS WINDOW
		 * LENGTH AS THAT IS THE KEY LENGTH, NOT THIS BUILT IN CONSTANT.... */
		/* any object longer than about 1.5k will need to be cropped as we zoom in to avoid
		 * going over the 30k XWindows limit, so we store them in ZMapWindow->longItem struct. */
		if ((zMapSpan->x2 - zMapSpan->x1)
		    > (ZMAP_WINDOW_MAX_WINDOW / canvas_data->window->text_height))
		  storeLongItem(canvas_data->window, box, 
				(zMapSpan->x2 - feature->x1), 
				(zMapSpan->x2 - feature->x1),
				key_id, feature->name);

		if (!object)
		  object = box;
	      }
	  }
	  break;

	default:
	  printf("Not displaying %s type %d\n", feature->name, feature->type);
	  break;
	}

      if (object)
	{
	  /* connect the object to its feature_item, add that into the GData in the ZMapWindow. */
	  feature_item->canvasItem = object;
	  g_datalist_id_set_data_full(&(canvas_data->window->featureItems),
				      key_id, feature_item, freeFeatureItem) ;
 	}
    }

  return ;
}



static void storeLongItem(ZMapWindow window, FooCanvasItem *item, int start, int end, GQuark key_id, char *name)
{
  ZMapWindowLongItem  longItem = g_new0(ZMapWindowLongItemStruct, 1);
  
  longItem->name        = name;
  longItem->canvasItem  = item;
  longItem->start       = start;
  longItem->end         = end;  
  g_datalist_id_set_data_full(&(window->longItems), key_id, longItem, freeLongItem);

  return;
}




/** Destroy function for feature_item.
 *
 * Called by g_datalist_clear
 */
static void freeFeatureItem(gpointer data)
{
  ZMapFeatureItem feature_item = (ZMapFeatureItemStruct*)data;
                                                                                                                                                  
  g_free(feature_item);

  return;
}
                                                                                                                                                  
                                                                                                                                                  

/** Destroy function for longItem.
 *
 * Called by g_datalist_clear
 */
static void freeLongItem(gpointer data)
{
  ZMapWindowLongItem longItem = (ZMapWindowLongItemStruct*)data;
                                                                                                                                                  
  g_free(longItem);

  return;
}
                                                                                                                                                  
                                                                                                                                                  

static gboolean canvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;
  ZMapWindow  window = (ZMapWindowStruct*)data;
  ZMapFeature feature;
  ZMapFeatureItem feature_item;
  GString     *source;
  ZMapFeatureTypeStyle thisType;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("ITEM canvas event handler\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  if (event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton *but_event = (GdkEventButton *)event ;


      printf("ITEM event handler - CLICK\n") ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      {
	double x = item->x1, y = item->y1 ;
	double x1, y1, x2, y2 ;

	x = y = x1 = y1 = x2 = y2 = 0 ;

	foo_canvas_item_i2w(item, &x, &y) ;




	foo_canvas_item_get_bounds (item, &x1, &y1, &x2, &y2) ;


	printf("got it\n") ;
      }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





      /* retrieve the FeatureItemStruct from the clicked object, obtain the feature_set from that and the
       * feature from that, using the GQuark feature_key. Then call the 
       * click callback function, passing the ZMapWindow and feature so the details of the clicked object
       * can be displayed in the info_panel. */
      feature_item = g_object_get_data(G_OBJECT(item), "feature");  
      zMapAssert(feature_item);

      zMapAssert(feature_item->feature_key) ;

      /* SURELY WE CAN NEVER NOT HAVE A KEY !  SHOULD CHECK THE LOGIC HERE !  EG */
      if (feature_item->feature_key)
	{
	  feature = g_datalist_id_get_data(&(feature_item->feature_set->features), feature_item->feature_key); 
	  zMapWindowFeatureClickCB(window, feature);   

	  /* lowercase the source as all stored types are lowercase */                         
	  source = g_string_new(feature_item->feature_set->source);
	  source = g_string_ascii_down(source);
	  thisType = (ZMapFeatureTypeStyle)g_datalist_get_data(&(window->types),source->str);
	  zMapAssert(thisType);
									
	  foo_canvas_item_raise_to_top(item);

	  zmapWindowHighlightObject(item, window, thisType);
	  window->focusQuark = feature_item->feature_key;
	  window->typeName   = source->str;

	  g_string_free(source, FALSE); 

	  event_handled = FALSE ;
	}

      /* if this was a right-click, display a list of the features in this column so the user
       * can select one and have the display scroll to it. */
      if (but_event->button == 3)
	{
	  if (feature_item)
	    {
	      /* display selectable list of features */
	      showFeatureList(window, feature_item);
	      event_handled = FALSE ;
	    }
	}

      event_handled = FALSE ;
    } 
  else if (event->type == GDK_BUTTON_RELEASE)
    {
      event_handled = FALSE ;
    }
  else if (event->type == GDK_KEY_PRESS)
    {
      event_handled = FALSE ;
    }

  return event_handled ;
}



static void showFeatureList(ZMapWindow window, ZMapFeatureItem feature_item)
{
  /* Actually this is not really correct, in the end it will go back to our caller who may
   * then call this zmapwindow call.... */

  /* user selects new feature in this column */
  zMapWindowCreateListWindow(window, feature_item) ;

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



/* GDestroyNotify() function, called when an alignment GData item is destroyed. */
void destroyAlignment(gpointer data)
{
  ZMapWindowAlignment alignment = (ZMapWindowAlignment)data ;

  zmapWindowAlignmentDestroy(alignment) ;

  return ;
}




/****************** end of file ************************************/
