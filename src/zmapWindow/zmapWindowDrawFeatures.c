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
 * Last edited: May  3 13:45 2005 (rnc)
 * Created: Thu Jul 29 10:45:00 2004 (rnc)
 * CVS info:   $Id: zmapWindowDrawFeatures.c,v 1.56 2005-05-03 14:29:10 rnc Exp $
 *-------------------------------------------------------------------
 */


#include <ZMap/zmapDraw.h>
#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>



/* Used to pass data to canvas item menu callbacks. */
typedef struct
{
  ZMapWindow window ;
  FooCanvasItem *item ;
} ItemMenuCBDataStruct, *ItemMenuCBData ;


static void makeItemMenu(GdkEventButton *button_event, ZMapWindow window,
			 FooCanvasItem *item) ;
static void itemMenuCB(int menu_item_id, gpointer callback_data) ;




/* these will go when scale is in separate window. */
#define SCALEBAR_OFFSET   0.0
#define SCALEBAR_WIDTH   50.0
#define COLUMN_SPACING    5.0



/* parameters passed between the various functions drawing the features on the canvas */
typedef struct _ZMapCanvasDataStruct
{
  ZMapWindow           window ;
  FooCanvas           *canvas ;

  ZMapWindowColumn column ;

  ZMapFeatureContext   full_context ;

  ZMapFeatureSet       feature_set;
  GQuark               context_key;

  ZMapFeatureTypeStyle type;

} ZMapCanvasDataStruct, *ZMapCanvasData ;




static gboolean canvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static gboolean canvasItemDestroyCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;

static void ProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data) ;
static void ProcessFeature(GQuark key_id, gpointer data, gpointer user_data) ;

static void getTextDimensions(FooCanvasGroup *group, double *width_out, double *height_out) ;

static void freeLongItem(gpointer data);
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
 */


/************************ external functions ************************************************/



/* THIS WILL HAVE TO CHANGE TO DRAWING ALIGNMENTS AND BLOCKS WITHIN THE ALIGNMENTS.... */
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
  double column_start ;
  double x1, y1, x2, y2 ;

  zMapAssert(window && full_context && diff_context && types) ;

  /* Must be reset each time because context will change as features get merged in. */
  window->feature_context = full_context ;

  /* Must be reset as well as types get merged in. */
  window->types = types ;

  window->seqLength
    = full_context->sequence_to_parent.c2 - full_context->sequence_to_parent.c1 + 1 ;

  window->seq_start = full_context->sequence_to_parent.c1 ;
  window->seq_end   = full_context->sequence_to_parent.c2 ;


  h_adj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;


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
  

  canvas_data.window = window;
  canvas_data.canvas = window->canvas;


  /* this routine requires that the scrolled region is set as well.... */
  foo_canvas_set_scroll_region(window->canvas,
			       0.0, window->seq_start, 
			       SCALEBAR_WIDTH, window->seq_end) ;

  window->scaleBarGroup = zmapDrawScale(window->canvas,
					0.0, window->zoom_factor,
					full_context->sequence_to_parent.c1,
					full_context->sequence_to_parent.c2,
					&(window->major_scale_units),
					&(window->minor_scale_units)) ;


  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(foo_canvas_root(window->canvas)), &x1, &y1, &x2, &y2) ;
  column_start = x2 + COLUMN_SPACING ;


  /* FUDGED CODE FOR NOW.....we want to deal with multiple alignments but zmapview etc. don't
   * deal with this at the moment so neither do we. The position of the alignment will be one
   * parameter we need to change as we draw alignments. */
  if (!window->alignments)
    {
      ZMapWindowAlignment alignment ;
      ZMapWindowAlignmentBlock block ;

      g_datalist_init(&window->alignments) ;

      alignment = zmapWindowAlignmentCreate(window->sequence, window,
					    foo_canvas_root(window->canvas), column_start) ;

      g_datalist_set_data_full(&window->alignments, window->sequence, alignment, destroyAlignment) ;


      g_datalist_init(&alignment->blocks) ;

      block = zmapWindowAlignmentAddBlock(alignment, "dummy", 0.0) ;

      g_datalist_set_data(&alignment->blocks, "dummy", block) ;

    }



  /* 
   *Draw all the features, so much in so few lines...sigh...
   */
  canvas_data.full_context = full_context ;
  g_datalist_foreach(&(diff_context->feature_sets), ProcessFeatureSet, &canvas_data) ;



  /* There may be a focus item if this routine is called as a result of splitting a window
   * or adding more features, make sure we scroll to the same point as we were
   * at in the previously. */
  if (window->focus_item)
    {
      zMapWindowScrollToItem(window, window->focus_item) ;
    }
  else
    {
      double scroll_x1, scroll_y1, scroll_x2, scroll_y2 ;

      foo_canvas_get_scroll_region(window->canvas, &scroll_x1, &scroll_y1, &scroll_x2, &scroll_y2) ;
      if (scroll_x1 == 0 && scroll_y1 == 0 && scroll_x2 == 0 && scroll_y2 == 0)
	foo_canvas_set_scroll_region(window->canvas,
				     0.0, window->seq_start,
				     h_adj->page_size, window->seq_end) ;
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Now hide/show any columns that have specific show/hide zoom factors set. */
  if (window->columns)
    zmapHideUnhideColumns(window) ;

  /* we need a hide columns call here..... */

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* We want the canvas to be the focus widget of its "window" otherwise keyboard input
   * (i.e. short cuts) will be delivered to some other widget. We do this here because
   * I think we may need a widget window to exist for this call to work. */
  gtk_widget_grab_focus(GTK_WIDGET(window->canvas)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  /* Expand the scroll region to include everything, note the hard-coded zero start, this is
   * because the actual visible window may not change when the scrolled region changes if its
   * still visible so we can end up with the visible window starting where the alignment box.
   * starts...should go away when scale moves into separate window. */  
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(foo_canvas_root(window->canvas)), &x1, &y1, &x2, &y2) ;
  foo_canvas_set_scroll_region(window->canvas,
			       0.0, window->seq_start, 
			       x2, window->seq_end) ;


  /* This routine requires that the scrolled region is valid.... */
  /* I'm not sure why this is here....can this routine be called when the features might
   * be too long........check this out....... */
  if (window->longItems)
    g_datalist_foreach(&(window->longItems), zmapWindowCropLongFeature, window);


  /* Expand the scroll region to include everything again as we need to include the scale bar. */  
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(foo_canvas_root(window->canvas)), &x1, &y1, &x2, &y2) ;
  foo_canvas_set_scroll_region(window->canvas,
			       0.0, window->seq_start, 
			       x2, window->seq_end) ;

  return ;
}






/************************ internal functions **************************************/



/* Called for each feature set, it then calls a routine to draw each of its features.  */
static void ProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
  ZMapCanvasData canvas_data  = (ZMapCanvasData)user_data ;
  ZMapWindow window = canvas_data->window ;
  GQuark type_quark ;
  char *type_name ;
  ZMapWindowColumn column ;
  double x1, y1, x2, y2 ;

  /* this is hacked for now as we only have one alignment.... */
  ZMapWindowAlignment alignment ;
  ZMapWindowAlignmentBlock block ;


  /* Each column is known by its type/style name. */
  type_quark = feature_set->unique_id ;


  type_name = (char *)g_quark_to_string(type_quark) ;

  /* SHOULDN'T type_quark == key_id ????? CHECK THAT THIS IS SO.... */

  canvas_data->type = (ZMapFeatureTypeStyle)g_datalist_id_get_data(&(canvas_data->window->types),
								   type_quark) ;
  zMapAssert(canvas_data->type) ;

  canvas_data->feature_set = feature_set ;


  /* Add a hash table for this columns features. */
  zmapWindowFToIAddSet(window->feature_to_item, type_quark) ;


  /* context_key is used when a user clicks a canvas item.  The callback function needs to know
   * which feature is associated with the clicked object, so we use the GQuarks to look it up.*/
  canvas_data->context_key = key_id ;


  alignment = (ZMapWindowAlignment)g_datalist_get_data(&(canvas_data->window->alignments),
						       canvas_data->window->sequence) ;


  /* Hack this for now to simply get a block..... */
  block = (ZMapWindowAlignmentBlock)g_datalist_get_data(&(alignment->blocks),
							"dummy") ;


  canvas_data->column = zmapWindowAlignmentAddColumn(block, type_quark, canvas_data->type) ;



  /* Now draw all the features in the column. */
  g_datalist_foreach(&(feature_set->features), ProcessFeature, canvas_data) ;


  return ;
}


/* Called for each individual feature. */
static void ProcessFeature(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ; 
  ZMapCanvasData canvas_data  = (ZMapCanvasDataStruct*)user_data ;
  ZMapWindow window = canvas_data->window ;
  int i ;
  FooCanvasItem *column_group, *top_feature_item = NULL ;


  double x = 0.0, y = 0.0 ;				    /* for testing... */


  /* Users will often not want to see what is on the reverse strand. */
  if (feature->strand == ZMAPSTRAND_REVERSE && canvas_data->type->showUpStrand == FALSE)
    return ;

  column_group = zmapWindowAlignmentGetColumn(canvas_data->column, feature->strand) ;

  switch (feature->type)
    {
    case ZMAPFEATURE_BASIC: case ZMAPFEATURE_HOMOL:
    case ZMAPFEATURE_VARIATION:
    case ZMAPFEATURE_BOUNDARY: case ZMAPFEATURE_SEQUENCE:
      {
	/* NOTE THAT ONCE WE SUPPORT HOMOLOGIES MORE FULLY WE WILL WANT SEPARATE BOXES
	 * FOR BLOCKS WITHIN A SINGLE ALIGNMENT, this will mean a change to "item_feature_type"
	 * in line with how its done for transcripts. */

	top_feature_item = zMapDrawBox(FOO_CANVAS_ITEM(column_group), 0.0,
			     feature->x1,
			     canvas_data->type->width, 
			     feature->x2,
			     &canvas_data->type->outline,
			     &canvas_data->type->foreground) ;
	
	g_object_set_data(G_OBJECT(top_feature_item), "feature", feature) ;
	g_object_set_data(G_OBJECT(top_feature_item), "item_feature_type",
			  GINT_TO_POINTER(ITEM_FEATURE_SIMPLE)) ;
	g_signal_connect(GTK_OBJECT(top_feature_item), "event",
			 GTK_SIGNAL_FUNC(canvasItemEventCB), window) ;
	g_signal_connect(GTK_OBJECT(top_feature_item), "destroy",
			 GTK_SIGNAL_FUNC(canvasItemDestroyCB), window) ;


	/* Is this correct, I'm not sure if this doesn't need a more dynamic approach of
	 * recacalculation each time we zoom... */

	/* any object longer than about 1.5k will need to be cropped as we zoom in to avoid
	 * going over the 30k XWindows limit, so we store them in ZMapWindow->longItem struct. */
	if ((feature->x2 - feature->x1)
	    > (ZMAP_WINDOW_MAX_WINDOW / window->text_height))
	  storeLongItem(window, top_feature_item, feature->x1, feature->x2, key_id,
			(char *)g_quark_to_string(feature->original_id)) ;

	break ;
      }
    case ZMAPFEATURE_TRANSCRIPT:
      {
	/* Note that for transcripts the boxes and lines are contained in a canvas group
	 * and that therefore their coords are relative to the start of the group which
	 * is the start of the transcript, i.e. feature->x1. */
	FooCanvasPoints *points ;
	FooCanvasItem *feature_group ;

	/* allocate a points array for drawing the intron lines, we need three points to draw the
	 * two lines that make the familiar  /\  shape between exons. */
	points = foo_canvas_points_new(3) ;


	/* this group is the parent of the entire transcript. */
	feature_group = foo_canvas_item_new(FOO_CANVAS_GROUP(column_group),
					    foo_canvas_group_get_type(),
					    "x", 0.0,
					    "y", (double)feature->x1,
					    NULL);
      
	top_feature_item = FOO_CANVAS_ITEM(feature_group) ;

	/* we probably need to set this I think, it means we will go to the group when we
	 * navigate or whatever.... */
	g_object_set_data(G_OBJECT(top_feature_item), "feature", feature) ;
	g_object_set_data(G_OBJECT(top_feature_item), "item_feature_type",
			  GINT_TO_POINTER(ITEM_FEATURE_PARENT)) ;


	/* first we draw the introns, then the exons.  Introns will have an invisible
	 * box around them to allow the user to click there and get a reaction.
	 * It's a bit hacky but the bounding box has the same coords stored on it as
	 * the intron...this needs tidying up because its error prone, better if they
	 * had a surrounding group.....e.g. what happens when we free ?? */
	if (feature->feature.transcript.introns)
	  {
	    float line_width  = 1.5 ;

	    for (i = 0 ; i < feature->feature.transcript.introns->len ; i++)  
	      {
		ZMapSpan intron_span ;
		FooCanvasItem *intron_box, *intron_line ;
		double left, right, top, middle, bottom ;
		ZMapWindowItemFeature box_data, intron_data ;

		intron_span = &g_array_index(feature->feature.transcript.introns, ZMapSpanStruct, i) ;

		box_data = g_new0(ZMapWindowItemFeatureStruct, 1) ;
		box_data->start = intron_span->x1 ;
		box_data->end = intron_span->x2 ;

		/* Need to remember that group coords start at zero, need to encapsulate this
		 * in some kind of macro/function that uses the group coords etc. to set positions. */
		left = canvas_data->type->width / 2 ;
		right = canvas_data->type->width ;
		top = intron_span->x1 - feature->x1 ;
		bottom = intron_span->x2 - feature->x1 ;
		middle = top + ((bottom - top + 1) / 2) ;

		intron_box = zMapDrawBox(feature_group,
					 0.0, top,
					 right, bottom,
					 &(window->canvas_background),
					 &(window->canvas_background)) ;

		g_object_set_data(G_OBJECT(intron_box), "feature", feature) ;
		g_object_set_data(G_OBJECT(intron_box), "item_feature_type",
				  GINT_TO_POINTER(ITEM_FEATURE_BOUNDING_BOX)) ;
		g_object_set_data(G_OBJECT(intron_box), "item_feature_data",
				  box_data) ;
		g_signal_connect(GTK_OBJECT(intron_box), "event",
				 GTK_SIGNAL_FUNC(canvasItemEventCB), window) ;
		g_signal_connect(GTK_OBJECT(intron_box), "destroy",
				 GTK_SIGNAL_FUNC(canvasItemDestroyCB), window) ;


		intron_data = g_new0(ZMapWindowItemFeatureStruct, 1) ;
		intron_data->start = intron_span->x1 ;
		intron_data->end = intron_span->x2 ;

		/* fill out the points */
		points->coords[0] = left ;
		points->coords[1] = top ;
		points->coords[2] = right ;
		points->coords[3] = middle ;
		points->coords[4] = left ;
		points->coords[5] = bottom ;

		intron_line = zMapDrawPolyLine(FOO_CANVAS_GROUP(feature_group),
					       points,
					       &canvas_data->type->foreground, line_width) ;

		g_object_set_data(G_OBJECT(intron_line), "feature", feature) ;
		g_object_set_data(G_OBJECT(intron_line), "item_feature_type",
				  GINT_TO_POINTER(ITEM_FEATURE_CHILD)) ;
		g_object_set_data(G_OBJECT(intron_line), "item_feature_data",
				  intron_data) ;
		g_signal_connect(GTK_OBJECT(intron_line), "event",
				 GTK_SIGNAL_FUNC(canvasItemEventCB), window) ;
		g_signal_connect(GTK_OBJECT(intron_line), "destroy",
				 GTK_SIGNAL_FUNC(canvasItemDestroyCB), window) ;


		/* Now we can get at either twinned item from the other item. */
		box_data->twin_item = intron_line ;
		intron_data->twin_item = intron_box ;
	      }
	  }


	/* We assume there are exons....I think we should check this actually.... */
	for (i = 0; i < feature->feature.transcript.exons->len; i++)
	  {
	    ZMapSpan exon_span ;
	    FooCanvasItem *exon_box ;
	    ZMapWindowItemFeature feature_data ;

	    exon_span = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i);

	    feature_data = g_new0(ZMapWindowItemFeatureStruct, 1) ;
	    feature_data->start = exon_span->x1 ;
	    feature_data->end = exon_span->x2 ;

	    exon_box = zMapDrawBox(feature_group, 
				   0.0,
				   (exon_span->x1 - feature->x1), 
				   canvas_data->type->width,
				   (exon_span->x2 - feature->x1), 
				   &canvas_data->type->outline, 
				   &canvas_data->type->foreground) ;

	    g_object_set_data(G_OBJECT(exon_box), "feature", feature) ;
	    g_object_set_data(G_OBJECT(exon_box), "item_feature_type",
			      GINT_TO_POINTER(ITEM_FEATURE_CHILD)) ;
	    g_object_set_data(G_OBJECT(exon_box), "item_feature_data",
			      feature_data) ;
	    g_signal_connect(GTK_OBJECT(exon_box), "event",
			     GTK_SIGNAL_FUNC(canvasItemEventCB), window) ;
	    g_signal_connect(GTK_OBJECT(exon_box), "destroy",
			     GTK_SIGNAL_FUNC(canvasItemDestroyCB), window) ;


	    /* THIS CODE SHOULD BE LOOKING TO SEE IF THE ITEM IS LONGER THAN THE CANVAS WINDOW
	     * LENGTH AS THAT IS THE KEY LENGTH, NOT THIS BUILT IN CONSTANT.... */
	    /* any object longer than about 1.5k will need to be cropped as we zoom in to avoid
	     * going over the 30k XWindows limit, so we store them in ZMapWindow->longItem struct. */
	    if ((exon_span->x2 - exon_span->x1)
		> (ZMAP_WINDOW_MAX_WINDOW / window->text_height))
	      storeLongItem(window, exon_box, 
			    (exon_span->x2 - feature->x1), 
			    (exon_span->x2 - feature->x1),
			    key_id, (char *)g_quark_to_string(feature->original_id)) ;
	  }



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	zmapWindowPrintGroup(FOO_CANVAS_GROUP(feature_group)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	/* tidy up. */
	foo_canvas_points_free(points) ;

	break ;
      }
    default:
      zMapLogWarning("Feature %s is of unknown type: %d\n", 
		     (char *)g_quark_to_string(feature->original_id), feature->type) ;
      break;
    }


  /* If we created an object then set up a hash that allows us to go unambiguously from
   * (feature, style) -> feature_item. */
  if (top_feature_item)
    {
      zmapWindowFToIAddFeature(window->feature_to_item, feature->style,
			       feature->unique_id, top_feature_item) ;
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



/* Destroy function for longItem.
 *
 * Called by g_datalist_clear
 */
static void freeLongItem(gpointer data)
{
  ZMapWindowLongItem longItem = (ZMapWindowLongItemStruct*)data;
                                                                                                                                                  
  g_free(longItem);

  return;
}


/* Callback for destroy of items... */
static gboolean canvasItemDestroyCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;			    /* Make sure any other callbacks also
							       get run. */
  ZMapWindow  window = (ZMapWindowStruct*)data ;
  ZMapWindowItemFeatureType item_feature_type ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "item_feature_type")) ;

  switch (item_feature_type)
    {
    case ITEM_FEATURE_SIMPLE:
    case ITEM_FEATURE_PARENT:
      {
	/* Nothing to do... */

	break ;
      }
    case ITEM_FEATURE_BOUNDING_BOX:
    case ITEM_FEATURE_CHILD:
      {
	ZMapWindowItemFeature item_data ;

	item_data = g_object_get_data(G_OBJECT(item), "item_feature_data") ;
	g_free(item_data) ;
	break ;
      }
    default:
      {
	zMapLogFatal("Coding error, unrecognised ZMapWindowItemFeatureType: %d", item_feature_type) ;
	break ;
      }
    }

  return event_handled ;
}



/* Callback for any events that happen on individual canvas items. */
static gboolean canvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;
  ZMapWindow  window = (ZMapWindowStruct*)data ;
  ZMapFeature feature ;
  ZMapFeatureTypeStyle type ;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      {
	GdkEventButton *but_event = (GdkEventButton *)event ;
	ZMapWindowItemFeatureType item_feature_type ;
	FooCanvasItem *real_item ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	printf("ITEM event handler - CLICK\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	/* Retrieve the feature item info from the canvas item. */
	feature = g_object_get_data(G_OBJECT(item), "feature");  
	zMapAssert(feature) ;


	item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							      "item_feature_type")) ;

	/* If its a bounding box then we don't want the that to influence highlighting.. */
	if (item_feature_type == ITEM_FEATURE_BOUNDING_BOX)
	  real_item = zMapWindowFindFeatureItemByItem(window, item) ;
	else
	  real_item = item ;


	/* Button 1 and 3 are handled, 2 is left for a general handler which could be
	 * the root handler. */
	if (but_event->button == 1 || but_event->button == 3)
	  {
	    /* Highlight the object the user clicked on and pass information about
	     * about it back to layer above. */
	    ZMapWindowSelectStruct select = {NULL} ;
	    double x = 0.0, y = 0.0 ;
	    
	    /* Pass information about the object clicked on back to the application. */
	    select.text = g_strdup_printf("%s   %s   %d   %d   %s   %s", 
					  (char *)g_quark_to_string(feature->original_id),
					  zmapFeatureLookUpEnum(feature->strand, STRAND_ENUM),
					  feature->x1,
					  feature->x2,
					  zmapFeatureLookUpEnum(feature->type, TYPE_ENUM),
					  (char *)g_quark_to_string(feature->style)) ;
	    
	    select.item = real_item ;
	    
	    (*(window->caller_cbs->select))(window, window->app_data, (void *)&select) ;
	    
	    g_free(select.text) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	    /* I'm not sure what to do here, actually the callback above ends up highlighting
	     * so there is no need to here....perhaps we should leave it to the caller to
	     * do any highlighting ??? */
	    zMapWindowHighlightObject(window, real_item) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	    if (but_event->button == 3)
	      {
		/* Pop up an item menu. */
		makeItemMenu(but_event, window, real_item) ;
	      }

	    event_handled = TRUE ;
	  }
      }
    default:
      {
	/* By default we _don't_handle events. */
	event_handled = FALSE ;

	break ;
      }
    }

  return event_handled ;
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



static void makeItemMenu(GdkEventButton *button_event, ZMapWindow window, FooCanvasItem *item)
{
  char *menu_title = "Item menu" ;
  ZMapWindowMenuItemStruct menu[] =
    {
      {"Show Feature List", 1, itemMenuCB},
      {"Dummy",             2, itemMenuCB},
      {NULL,                0, NULL}
    } ;
  ZMapWindowMenuItem menu_item ;
  ItemMenuCBData menu_data ;

  menu_data = g_new0(ItemMenuCBDataStruct, 1) ;
  menu_data->window = window ;
  menu_data->item = item ;

  menu_item = menu ;
  while (menu_item->name != NULL)
    {
      menu_item->callback_data = menu_data ;
      menu_item++ ;
    }

  zMapWindowMakeMenu(menu_title, menu, button_event) ;

  return ;
}


static void itemMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;

  switch (menu_item_id)
    {
    case 1:
      {
	/* display a list of the features in this column so the user
	 * can select one and have the display scroll to it. */
	zMapWindowCreateListWindow(menu_data->window, menu_data->item) ;
	break ;
      }
    case 2:
      {
	printf("pillock\n") ;
	break ;
      }
    default:
      {
	zMapAssert("Coding error, unrecognised menu item number.") ; /* exits... */
	break ;
      }
    }

  g_free(menu_data) ;

  return ;
}



/* I'M TRYING THESE TWO FUNCTIONS BECAUSE I DON'T LIKE THE BIT WHERE IT GOES TO THE ITEMS
 * PARENT IMMEDIATELY....WHAT IF THE ITEM IS ALREADY A GROUP ?????? */

/**
 * foo_canvas_item_w2i:
 * @item: A canvas item.
 * @x: X coordinate to convert (input/output value).
 * @y: Y coordinate to convert (input/output value).
 *
 * Converts a coordinate pair from world coordinates to item-relative
 * coordinates.
 **/
void my_foo_canvas_item_w2i (FooCanvasItem *item, double *x, double *y)
{
	g_return_if_fail (FOO_IS_CANVAS_ITEM (item));
	g_return_if_fail (x != NULL);
	g_return_if_fail (y != NULL);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	item = item->parent;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	if (!FOO_IS_CANVAS_GROUP (item))
	  item = item->parent;

	while (item) {
		if (FOO_IS_CANVAS_GROUP (item)) {
			*x -= FOO_CANVAS_GROUP (item)->xpos;
			*y -= FOO_CANVAS_GROUP (item)->ypos;
		}

		item = item->parent;
	}
}


/**
 * foo_canvas_item_i2w:
 * @item: A canvas item.
 * @x: X coordinate to convert (input/output value).
 * @y: Y coordinate to convert (input/output value).
 *
 * Converts a coordinate pair from item-relative coordinates to world
 * coordinates.
 **/
void my_foo_canvas_item_i2w (FooCanvasItem *item, double *x, double *y)
{
	g_return_if_fail (FOO_IS_CANVAS_ITEM (item));
	g_return_if_fail (x != NULL);
	g_return_if_fail (y != NULL);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	item = item->parent;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	if (!FOO_IS_CANVAS_GROUP (item))
	  item = item->parent;

	while (item) {
		if (FOO_IS_CANVAS_GROUP (item)) {
			*x += FOO_CANVAS_GROUP (item)->xpos;
			*y += FOO_CANVAS_GROUP (item)->ypos;
		}

		item = item->parent;
	}
}








/****************** end of file ************************************/
