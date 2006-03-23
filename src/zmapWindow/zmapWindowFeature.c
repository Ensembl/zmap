/*  File: zmapWindowFeature.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2006
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Functions that manipulate displayed features, they
 *              encapsulate handling of the feature context, the
 *              displayed canvas items and the feature->item hash.
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Mar 23 16:18 2006 (edgrif)
 * Created: Mon Jan  9 10:25:40 2006 (edgrif)
 * CVS info:   $Id: zmapWindowFeature.c,v 1.12 2006-03-23 16:43:15 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <math.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapPeptide.h>
#include <ZMap/zmapGLibUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>



static FooCanvasItem *drawparentfeature(FooCanvasGroup *parent,
						  ZMapFeature feature,
						  double y_origin) ;
static FooCanvasItem *drawSimpleFeature(FooCanvasGroup *parent, ZMapFeature feature,
					double feature_offset,
					double x1, double y1, double x2, double y2,
					GdkColor *outline,
                                        GdkColor *foreground,
                                        GdkColor *background,
					ZMapWindow window) ;
static FooCanvasItem *drawTranscriptFeature(FooCanvasGroup *parent, ZMapFeature feature,
					    double feature_offset,
					    double x1, double feature_top,
					    double x2, double feature_bottom,
					    GdkColor *outline, 
                                            GdkColor *foreground,
                                            GdkColor *background,
					    ZMapWindow window) ;
static FooCanvasItem *drawAlignmentFeature(FooCanvasGroup *parent, ZMapFeature feature,
                                           double feature_offset,
                                           double x1, double feature_top,
                                           double x2, double feature_bottom,
                                           GdkColor *outline, 
                                           GdkColor *foreground,
                                           GdkColor *background,
                                           ZMapWindow window) ;
static FooCanvasItem *drawSequenceFeature(FooCanvasGroup *parent, ZMapFeature feature,
                                          double feature_offset,
                                          double feature_zero,      double feature_top,
                                          double feature_thickness, double feature_bottom,
                                          GdkColor *outline, GdkColor *background,
                                          ZMapWindow window) ;


static void makeItemMenu(GdkEventButton *button_event, ZMapWindow window,
			 FooCanvasItem *item) ;
static void itemMenuCB(int menu_item_id, gpointer callback_data) ;

static ZMapGUIMenuItem makeMenuFeatureOps(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data) ;


static void dna_redraw_callback(FooCanvasGroup *text_grp,
                                double zoom,
                                gpointer user_data);

static gboolean dnaItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;

static gboolean featureClipItemToDraw(ZMapFeature feature, 
                                      double *start_inout, 
                                      double *end_inout) ;
static void attachDataToItem(FooCanvasItem *feature_item, 
                             ZMapWindow window,
                             ZMapFeature feature,
                             ZMapWindowItemFeatureType type,
                             gpointer subFeatureData) ;

static void updateInfoGivenCoords(textGroupSelection select, 
                                  double worldCurrentX,
                                  double worldCurrentY) ;
static void pointerIsOverItem(gpointer data, gpointer user_data);

static textGroupSelection getTextGroupData(FooCanvasGroup *txtGroup,
                                           ZMapWindow window);
static ZMapDrawTextIterator getIterator(double win_min_coord, double win_max_coord,
                                        double offset_start,  double offset_end,
                                        double text_height, gboolean numbered) ;
static void destroyIterator(ZMapDrawTextIterator iterator) ;

static gboolean canvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static gboolean canvasItemDestroyCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;

static void pfetchEntry(ZMapWindow window, char *sequence_name) ;

static void removeFeatureLongItems(GList **long_items, FooCanvasItem *feature_item) ;
static void removeLongItemCB(gpointer data, gpointer user_data) ;

static gboolean makeFeatureEditWindow(ZMapWindow window, ZMapFeature feature) ;

static void printItem(gpointer data, gpointer user_data) ;






/* NOTE that we make some assumptions in this code including:
 * 
 * - the caller has found the correct context, alignment, block and set
 *   to put the feature in. Probably we will have to write some helper
 *   helper functions to do this if they have not already been written.
 *   There will need to map stuff like  "block spec" -> ZMapFeatureBlock etc.
 * 
 * 
 *  */


/* 
 * NOTE TO ROY: I've written the functions so that when you get the xml from James, you can
 * get the align/block/set information and then 
 * use the hash calls to get the foocanvasgroup that is the column you want to add/modify the feature
 * in. The you can just call these routines with the group and the feature.
 * 
 * This way if there are errors in the xml and you can't find the right align/block/set then
 * you can easily report back to lace what the error was....
 *  */




GList *zMapWindowFeatureAllStyles(ZMapWindow window)
{
  zMapAssert(window && window->feature_context);
  return window->feature_context->styles;
}





/* Add a new feature to the feature_set given by the set canvas item.
 * 
 * Returns the new canvas feature item or NULL if there is some problem, e.g. the feature already
 * exists in the feature_set.
 *  */
FooCanvasItem *zMapWindowFeatureAdd(ZMapWindow window,
				    FooCanvasGroup *feature_group, ZMapFeature feature)
{
  FooCanvasItem *new_feature = NULL ;
  ZMapFeatureSet feature_set ;

  zMapAssert(window && feature_group && feature && zMapFeatureIsValid((ZMapFeatureAny)feature)) ;


  feature_set = g_object_get_data(G_OBJECT(feature_group), ITEM_FEATURE_DATA) ;
  zMapAssert(feature_set) ;


  /* Check the feature does not already exist in the feature_set. */
  if (!zMapFeatureSetFindFeature(feature_set, feature))
    {
      /* Add it to the feature set. */
      if (zMapFeatureSetAddFeature(feature_set, feature))
	{
	  /* This function will add the new feature to the hash. */
	  new_feature = zmapWindowFeatureDraw(window, FOO_CANVAS_GROUP(feature_group), feature) ;
	}
    }

  return new_feature ;
}



/* THERE IS A PROBLEM HERE IN THAT WE REMOVE THE EXISTING FOOCANVAS ITEM FOR THE FEATURE
 * AND DRAW A NEW ONE, THIS WILL INVALIDATE ANY CODE THAT IS CACHING THE ITEM.
 * 
 * THE FIX IS TO KEEP THE TOP CANVAS ITEM FOR THE FEATURE AND MUNGE IT TO HOLD THE NEW
 * NEW FEATURE....BUT THIS IS _NOT_ SIMPLE.... */

/* Replace an existing feature in the feature context, this is the only way to modify
 * a feature currently.
 * 
 * Returns FALSE if the feature does not exist. */
FooCanvasItem *zMapWindowFeatureReplace(ZMapWindow zmap_window,
					FooCanvasItem *curr_feature_item, ZMapFeature new_feature)
{
  FooCanvasItem *replaced_feature = NULL ;
  ZMapFeatureSet feature_set ;
  ZMapFeature curr_feature ;

  zMapAssert(zmap_window && curr_feature_item
	     && new_feature && zMapFeatureIsValid((ZMapFeatureAny)new_feature)) ;

  curr_feature = g_object_get_data(G_OBJECT(curr_feature_item), ITEM_FEATURE_DATA) ;
  zMapAssert(curr_feature && zMapFeatureIsValid((ZMapFeatureAny)curr_feature)) ;

  feature_set = (ZMapFeatureSet)(curr_feature->parent) ;

  /* Check the feature exists in the feature_set. */
  if (zMapFeatureSetFindFeature(feature_set, curr_feature))
    {
      /* Remove it completely. */
      if (zMapWindowFeatureRemove(zmap_window, curr_feature_item))
	{
	  FooCanvasItem *set_item ;

	  /* Find the canvas group for this set and draw the feature into it. */
	  if ((set_item = zmapWindowFToIFindSetItem(zmap_window->context_to_item,
						    feature_set, new_feature->strand)))
	    {
	      replaced_feature = zMapWindowFeatureAdd(zmap_window,
						      FOO_CANVAS_GROUP(set_item), new_feature) ;
	    }
	}
    }

  return replaced_feature ;
}


/* Remove an existing feature from the displayed feature context.
 * 
 * Returns FALSE if the feature does not exist. */
gboolean zMapWindowFeatureRemove(ZMapWindow zmap_window, FooCanvasItem *feature_item)
{
  gboolean result = FALSE ;
  ZMapFeature feature ;
  ZMapFeatureSet feature_set ;


  /* Check to see if there is an entry in long items for this feature.... */
  removeFeatureLongItems(&(zmap_window->long_items), feature_item) ;

  zmapWindowItemRemoveFocusItem(zmap_window, feature_item);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (zmapWindowLongItemRemove(&(menu_data->window->long_items), menu_data->item))
    printf("found a long item\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




  feature = g_object_get_data(G_OBJECT(feature_item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature && zMapFeatureIsValid((ZMapFeatureAny)feature)) ;

  feature_set = (ZMapFeatureSet)(feature->parent) ;

  /* Need to delete the feature from the feature set and from the hash and destroy the
   * canvas item....NOTE this is very order dependent. */
  if (zmapWindowFToIRemoveFeature(zmap_window->context_to_item,
				  feature->parent->parent->parent->unique_id,
				  feature->parent->parent->unique_id,
				  feature->parent->unique_id,
				  feature->strand,
				  feature->unique_id)
      && zMapFeatureSetRemoveFeature(feature_set, feature))
    {
      /* destroy the canvas item... */
      gtk_object_destroy(GTK_OBJECT(feature_item)) ;
      
      result = TRUE ;
    }

  return result ;
}




/* Called to draw each individual feature. */

FooCanvasItem *zmapWindowFeatureDraw(ZMapWindow window, FooCanvasGroup *set_group, ZMapFeature feature)
{
  FooCanvasItem *new_feature = NULL ;
  ZMapFeatureTypeStyle style = feature->style ;
  ZMapFeatureContext context ;
  ZMapFeatureAlignment alignment ;
  ZMapFeatureBlock block ;
  ZMapFeatureSet set ;
  FooCanvasGroup *column_parent, *column_group ;
  GQuark column_id ;
  FooCanvasItem *top_feature_item = NULL ;
  double feature_offset;
#ifdef RDS_DONT_INCLUDE_UNUSED
  GdkColor *background ;
#endif
  double start_x, end_x ;


  /* THIS MAY HAVE TO CHANGE SO WE DRAW EVERYTHING, BUT HIDE SOME COLS....OR MAYBE NOT....
   * IT ALL DEPENDS HOW WE DO REVCOMP.... */

  /* Users will often not want to see what is on the reverse strand, style specifies what should
   * be shown. */
  if (style->strand_specific
      && (feature->strand == ZMAPSTRAND_REVERSE && style->show_rev_strand == FALSE))
    {
      return NULL ;
    }



  set       = (ZMapFeatureSet)zMapFeatureGetParentGroup((ZMapFeatureAny)feature, ZMAPFEATURE_STRUCT_FEATURESET) ;
  block     = (ZMapFeatureBlock)zMapFeatureGetParentGroup((ZMapFeatureAny)set, ZMAPFEATURE_STRUCT_BLOCK) ;
  alignment = (ZMapFeatureAlignment)zMapFeatureGetParentGroup((ZMapFeatureAny)block, ZMAPFEATURE_STRUCT_ALIGN) ;
  context   = (ZMapFeatureContext)zMapFeatureGetParentGroup((ZMapFeatureAny)alignment, ZMAPFEATURE_STRUCT_CONTEXT) ;

  /* Retrieve the parent col. group/id. */
  column_group = zmapWindowContainerGetFeatures(set_group) ;

  if (feature->strand == ZMAPSTRAND_FORWARD || feature->strand == ZMAPSTRAND_NONE)
    {
      column_id = zmapWindowFToIMakeSetID(set->unique_id, ZMAPSTRAND_FORWARD) ;
    }
  else
    {
      column_id = zmapWindowFToIMakeSetID(set->unique_id, ZMAPSTRAND_REVERSE) ;
    }


  /* Start/end of feature within alignment block.
   * Feature position on screen is determined the relative offset of the features coordinates within
   * its align block added to the alignment block _query_ coords. You can't just use the
   * features own coordinates as these will be its coordinates in its original sequence. */

#ifdef RDS_DONT_INCLUDE
  feature_offset = FOO_CANVAS_GROUP(column_group)->ypos;
#endif
  feature_offset = block->block_to_sequence.q1 ;


  /* Note: for object to _span_ "width" units, we start at zero and end at "width". */
  start_x = 0.0 ;
  end_x = style->width ;


  switch (feature->type)
    {
    case ZMAPFEATURE_BASIC:
      {
	top_feature_item = drawSimpleFeature(column_group, feature,
					     feature_offset,
					     start_x, feature->x1, end_x, feature->x2,
					     &style->outline,
                                             &style->foreground,
					     &style->background,
					     window) ;
	break ;
      }
    case ZMAPFEATURE_ALIGNMENT:
      {
	zmapWindowGetPosFromScore(style, feature->score, &start_x, &end_x) ;

        if(style->align_gaps)
          top_feature_item = drawAlignmentFeature(column_group, feature,
                                                  feature_offset,
                                                  start_x, feature->x1, end_x, feature->x2,
                                                  &style->outline,
                                                  &style->foreground,
                                                  &style->background,
                                                  window);
        else
          top_feature_item = drawSimpleFeature(column_group, feature,
                                               feature_offset,
                                               start_x, feature->x1, end_x, feature->x2,
                                               &style->outline,
                                               &style->foreground,
                                               &style->background,
                                               window) ;
        break;
      }
    case ZMAPFEATURE_RAW_SEQUENCE:
      {
	/* Surely this must be true.... */
	zMapAssert(feature->feature.sequence.sequence) ;

	/* Flag our parent container that it has children that need redrawing on zoom. */
	column_parent = zmapWindowContainerGetParent(FOO_CANVAS_ITEM(column_group)) ;
	zmapWindowContainerSetChildRedrawRequired(column_parent, TRUE) ;

	top_feature_item = drawSequenceFeature(column_group, feature,
                                               feature_offset,
                                               start_x, feature->x1, end_x, feature->x2,
                                               &style->outline,
                                               &style->background,
                                               window) ;

        break ;
      }
    case ZMAPFEATURE_TRANSCRIPT:
      {
	top_feature_item = drawTranscriptFeature(column_group, feature,
						 feature_offset,
						 start_x, 0.0, end_x, 0.0,
						 &style->outline,
                                                 &style->foreground,
						 &style->background,
						 window) ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	zmapWindowPrintGroup(FOO_CANVAS_GROUP(feature_group)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	break ;
      }
    default:
      zMapLogFatal("Feature %s is of unknown type: %d\n", 
		   (char *)g_quark_to_string(feature->original_id), feature->type) ;
      break;
    }


  /* If we created an object then set up a hash that allows us to go unambiguously from
   * (feature, style) -> feature_item. */
  if (top_feature_item)
    {
      gboolean status ;
      
      status = zmapWindowFToIAddFeature(window->context_to_item,
					alignment->unique_id,
					block->unique_id, 
					column_id,
					feature->unique_id, top_feature_item) ;
      zMapAssert(status) ;

      new_feature = top_feature_item ;
    }

  return new_feature ;
}



static void printItem(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ZMapWindowItemFeatureType type ;
  

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;

  if (type == ITEM_FEATURE_SIMPLE)
    printf("found feature simple\n") ;


  return ;
}



/* the column_txt_grp need to exist before calling this! */
void zmapWindowColumnWriteDNA(ZMapWindow window, FooCanvasGroup *column_txt_grp)
{
  ZMapFeatureContext full_context = NULL;
  ZMapDrawTextIterator iterator   = NULL;
  PangoFontDescription *dna_font = NULL;
  double text_height = 0.0;
  double x1, x2, y1, y2;
  int i = 0;

  x1 = x2 = y1 = y2 = 0.0;
  full_context = window->feature_context;

  zmapWindowScrollRegionTool(window, &x1, &y1, &x2, &y2);

  /* should this be copied? */
  dna_font = zMapWindowGetFixedWidthFontDescription(window);
  /* zMapGUIGetFixedWidthFont(, &dna_font); */

  zmapWindowGetBorderSize(window, &text_height);

  iterator = getIterator(window->min_coord, window->max_coord,
                         y1, y2, text_height, FALSE);
  
  for(i = 0; i < iterator->rows; i++)
    {
      FooCanvasItem *item = NULL;
      iterator->iteration = i;

      if((item = zMapDrawRowOfText(column_txt_grp, 
                                   dna_font, 
                                   full_context->sequence->sequence, 
                                   iterator)))
        g_signal_connect(G_OBJECT(item), "event",
                         G_CALLBACK(dnaItemEventCB), (gpointer)window);      
    }

  destroyIterator(iterator);

  return ;
}





/* 
 *                       Internal functions.
 */



/* LONG ITEMS SHOULD BE INCORPORATED IN THIS LOT.... */
/* some drawing functions, may want these to be externally visible later... */


static FooCanvasItem *drawparentfeature(FooCanvasGroup *parent,
					ZMapFeature feature,
					double y_origin)
{
  FooCanvasItem *grp = NULL;
  /* this group is the parent of the entire transcript. */
  grp = foo_canvas_item_new(FOO_CANVAS_GROUP(parent),
                            foo_canvas_group_get_type(),
                            "y", y_origin,
                            NULL) ;
  grp = FOO_CANVAS_ITEM(grp) ;

  /* we probably need to set this I think, it means we will go to the group when we
   * navigate or whatever.... */
  g_object_set_data(G_OBJECT(grp), ITEM_FEATURE_TYPE,
                    GINT_TO_POINTER(ITEM_FEATURE_PARENT)) ;
  g_object_set_data(G_OBJECT(grp), ITEM_FEATURE_DATA, feature) ;

  return grp ;
}


static FooCanvasItem *drawSimpleFeature(FooCanvasGroup *parent, ZMapFeature feature,
					double feature_offset,
					double x1, double y1, double x2, double y2,
					GdkColor *outline, 
                                        GdkColor *foreground,
                                        GdkColor *background,
					ZMapWindow window)
{
  FooCanvasItem *feature_item ;

  if(featureClipItemToDraw(feature, &y1, &y2))
    return NULL ;

  zmapWindowSeq2CanOffset(&y1, &y2, feature_offset) ;
  feature_item = zMapDrawBox(FOO_CANVAS_ITEM(parent),
                             x1, y1, x2, y2,
                             outline, background) ;
  attachDataToItem(feature_item, window, feature, ITEM_FEATURE_SIMPLE, NULL);
  zmapWindowLongItemCheck(window, feature_item, y1, y2) ;

  return feature_item ;
}


static FooCanvasItem *drawSequenceFeature(FooCanvasGroup *parent, ZMapFeature feature,
                                          double feature_offset,
                                          double feature_zero,      double feature_top,
                                          double feature_thickness, double feature_bottom,
                                          GdkColor *outline, GdkColor *background,
                                          ZMapWindow window)
{
  FooCanvasItem *text_feature ;

  /* I WOULD HAVE THOUGHT THAT WE NEEDED TO BE MAKING MORE USE OF COLOURS ETC HERE.... */


  /* Note that the sequence item is different in that there is a single group child for the
   * entire dna rows of text.... */

  text_feature = foo_canvas_item_new(parent,
				     foo_canvas_group_get_type(),
				     NULL) ;

  g_object_set_data(G_OBJECT(text_feature), CONTAINER_REDRAW_CALLBACK,
		    dna_redraw_callback) ;
  g_object_set_data(G_OBJECT(text_feature), CONTAINER_REDRAW_DATA,
		    (gpointer)window) ;

  /* This sets an event handler which would need to be merged with the below.... */
  zmapWindowColumnWriteDNA(window, FOO_CANVAS_GROUP(text_feature)) ;


  /* TRY THIS...EVENT HANDLER MAY INTERFERE THOUGH...SO MAY NEED TO REMOVE EVENT HANDLERS... */
  attachDataToItem(text_feature, window, feature, ITEM_FEATURE_SIMPLE, NULL) ;

  return text_feature ;
}


static FooCanvasItem *drawAlignmentFeature(FooCanvasGroup *parent, ZMapFeature feature,
                                           double feature_offset,
                                           double x1, double feature_top,
                                           double x2, double feature_bottom,
                                           GdkColor *outline, 
                                           GdkColor *foreground,
                                           GdkColor *background,
                                           ZMapWindow window) 
{
  FooCanvasItem *feature_item ;
  double offset ;
  int i ;

  if (!feature->feature.homol.align)
    feature_item = drawSimpleFeature(parent, feature, feature_offset,
                                     x1, feature_top, x2, feature_bottom,
                                     outline, foreground, background, window) ;
  else if(feature->feature.homol.align)
    {
      float line_width  = 1.5 ;
      double feature_start, feature_end;
      FooCanvasItem *lastBoxWeDrew   = NULL;
      FooCanvasGroup *feature_group  = NULL;
      ZMapAlignBlock prev_align_span = NULL;

      feature_start = feature->x1;
      feature_end   = feature->x2; /* I want this function to find these values */
      zmapWindowSeq2CanOffset(&feature_start, &feature_end, feature_offset) ;
      feature_item  = drawparentfeature(parent, feature, feature_start);
      feature_group = FOO_CANVAS_GROUP(feature_item);

      /* Calculate total offset for for subparts of the feature. */
      offset = feature_start + feature_offset ;

      for (i = 0 ; i < feature->feature.homol.align->len ; i++)  
        {
          ZMapAlignBlock align_span;
          FooCanvasItem *align_box, *gap_line, *gap_line_box;
          double top, bottom, dummy = 0.0;
          ZMapWindowItemFeature box_data, gap_data, align_data ;
          
          align_span = &g_array_index(feature->feature.homol.align, ZMapAlignBlockStruct, i) ;   
          top    = align_span->t1;
          bottom = align_span->t2;

          if(featureClipItemToDraw(feature, &top, &bottom))
            continue;

          align_data = g_new0(ZMapWindowItemFeatureStruct, 1) ;
	  align_data->subpart = ZMAPFEATURE_SUBPART_MATCH ;
          align_data->start = align_span->t1 ;
          align_data->end   = align_span->t2 ;

          zmapWindowSeq2CanOffset(&top, &bottom, offset) ;

          if(prev_align_span)
            {
              int q_indel, t_indel;
              t_indel = align_span->t1 - prev_align_span->t2;
              /* For query the current and previous can be reversed
               * wrt target which is _always_ consecutive. */
              if((align_span->t_strand == ZMAPSTRAND_REVERSE && 
                  align_span->q_strand == ZMAPSTRAND_FORWARD) || 
                 (align_span->t_strand == ZMAPSTRAND_FORWARD &&
                  align_span->q_strand == ZMAPSTRAND_REVERSE))
                q_indel = prev_align_span->q1 - align_span->q2;
              else
                q_indel = align_span->q1 - prev_align_span->q2;
              
#ifdef RDS_THESE_NEED_A_LITTLE_BIT_OF_CHECKING
              /* I'm not 100% sure on some of this. */
                {
                  if(align_span->q_strand == ZMAPSTRAND_REVERSE)
                    printf("QR TF Find %s\n", g_quark_to_string(feature->unique_id));
                  else
                    printf("QF TR Find %s\n", g_quark_to_string(feature->unique_id));
                }
                {               /* Especially REVERSE REVERSE. */
                  if(align_span->t_strand == ZMAPSTRAND_REVERSE &&
                     align_span->q_strand == ZMAPSTRAND_REVERSE)
                    printf("QR TR Find %s\n", g_quark_to_string(feature->unique_id));
                }
#endif /* RDS_THESE_NEED_A_LITTLE_BIT_OF_CHECKING */

              if(q_indel >= t_indel) /* insertion in query, expand align and annotate the insertion */
                {
                  zMapDrawAnnotatePolygon(lastBoxWeDrew,
                                          ZMAP_ANNOTATE_EXTEND_ALIGN, 
                                          NULL,
                                          foreground,
                                          bottom,
                                          feature->strand);
                  /* we need to check here that the item hasn't been
                   * expanded to be too long! This will involve
                   * updating the rather than inserting as the check
                   * code currently does */
                  /* Really I want to display the correct width in the target here.
                   * Not sure on it's effect re the cap/join style.... 
                   * t_indel may always be 1, but possible that this is not the case, 
                   * code allows it not to be.
                   */
                }
              else
                {
                  lastBoxWeDrew = 
                    align_box = zMapDrawSSPolygon(feature_item, ZMAP_POLYGON_SQUARE,
                                                  x1, x2,
                                                  top, bottom,
                                                  outline, background,
                                                  feature->strand);
                  zmapWindowLongItemCheck(window, align_box, top, bottom);              
                  attachDataToItem(align_box, window, feature, ITEM_FEATURE_CHILD, align_data);

                  box_data        = g_new0(ZMapWindowItemFeatureStruct, 1) ;
                  box_data->start = align_span->t1 ;
                  box_data->end   = prev_align_span->t2 ;

                  gap_data        = g_new0(ZMapWindowItemFeatureStruct, 1) ;
		  gap_data->subpart = ZMAPFEATURE_SUBPART_GAP ;
                  gap_data->start = align_span->t1 ;
                  gap_data->end   = prev_align_span->t2 ;

                  bottom = prev_align_span->t2;
                  zmapWindowSeq2CanOffset(&dummy, &bottom, offset);
                  gap_line_box = zMapDrawSSPolygon(feature_item, ZMAP_POLYGON_SQUARE,
                                                   x1, x2,
                                                   bottom, top,
                                                   NULL,NULL,
                                                   feature->strand);
                  attachDataToItem(gap_line_box, window, feature, ITEM_FEATURE_BOUNDING_BOX, box_data);
                  zmapWindowLongItemCheck(window, gap_line_box, bottom, top);              

                  gap_line = zMapDrawAnnotatePolygon(gap_line_box,
                                                     ZMAP_ANNOTATE_GAP, 
                                                     NULL,
                                                     background,
                                                     line_width,
                                                     feature->strand);
                  attachDataToItem(gap_line, window, feature, ITEM_FEATURE_CHILD, gap_data);
                  zmapWindowLongItemCheck(window, gap_line, bottom, top);
                  foo_canvas_item_lower(gap_line, 2);
                  gap_data->twin_item = gap_line_box;
                  box_data->twin_item = gap_line;
                }
            }
          else
            {
              /* Draw the align match box */
              lastBoxWeDrew =
                align_box = zMapDrawSSPolygon(feature_item, ZMAP_POLYGON_SQUARE,
                                              x1, x2,
                                              top, bottom,
                                              outline, background,
                                              feature->strand);
              zmapWindowLongItemCheck(window, align_box, top, bottom) ;
              attachDataToItem(align_box, window, feature, ITEM_FEATURE_CHILD, align_data);
              /* Finished with the align match */

            }

          prev_align_span = align_span;
        }

    }
    
  return feature_item ;
}


	/* Note that for transcripts the boxes and lines are contained in a canvas group
	 * and that therefore their coords are relative to the start of the group which
	 * is the start of the transcript, i.e. feature->x1. */

static FooCanvasItem *drawTranscriptFeature(FooCanvasGroup *parent, ZMapFeature feature,
					    double feature_offset,
					    double x1, double feature_top,
					    double x2, double feature_bottom,
					    GdkColor *outline, 
                                            GdkColor *foreground,
                                            GdkColor *background,
					    ZMapWindow window)
{
  FooCanvasItem *feature_item ;
  int i ;
  double offset ;

  /* If there are no exons/introns then just draw a simple box. Can happen for putative
   * transcripts or perhaps where user has a single exon object but does not give an exon,
   * only an overall extent. */
  if (!feature->feature.transcript.introns && !feature->feature.transcript.exons)
    {
      feature_item = drawSimpleFeature(parent, feature, feature_offset,
				       x1, feature->x1, x2, feature->x2,
				       outline, foreground, background, window) ;
    }
  else
    {
      double feature_start, feature_end;
      double cds_start = 0.0, cds_end = 0.0; /* cds_start < cds_end */
      GdkColor *cds_bg = background;
      FooCanvasGroup *feature_group ;

      feature_start = feature->x1;
      feature_end   = feature->x2; /* I want this function to find these values */
      
      zmapWindowSeq2CanOffset(&feature_start, &feature_end, feature_offset) ;

      feature_item = drawparentfeature(parent, feature, feature_start);
      feature_group = FOO_CANVAS_GROUP(feature_item);

      /* Calculate total offset for subparts of the feature. */
      /* We could look @ foo_canavs_group->ypos, but locks us 2 the vertical! */
      offset = feature_start + feature_offset ;

      /* Set up the cds coords if there is one. */
      if(feature->feature.transcript.flags.cds)
        {
          cds_start = feature->feature.transcript.cds_start ;
          cds_end   = feature->feature.transcript.cds_end ;
          zMapAssert(cds_start < cds_end);


	  /* I think this is correct...???? */
	  if(!featureClipItemToDraw(feature, &cds_start, &cds_end))
            zmapWindowSeq2CanOffset(&cds_start, &cds_end, offset) ;
        }
      
      /* first we draw the introns, then the exons.  Introns will have an invisible
       * box around them to allow the user to click there and get a reaction.
       * It's a bit hacky but the bounding box has the same coords stored on it as
       * the intron...this needs tidying up because its error prone, better if they
       * had a surrounding group.....e.g. what happens when we free ?? */
      if (feature->feature.transcript.introns)
	{
          float line_width = 1.5 ;

	  for (i = 0 ; i < feature->feature.transcript.introns->len ; i++)  
	    {
	      FooCanvasItem *intron_box, *intron_line ;
	      double left, right, top, bottom ;
	      ZMapWindowItemFeature box_data, intron_data ;
	      ZMapSpan intron_span ;

	      intron_span = &g_array_index(feature->feature.transcript.introns, ZMapSpanStruct, i) ;

	      left   = x1;
	      right  = x2 - line_width ;
              top    = intron_span->x1;
              bottom = intron_span->x2;

              if(featureClipItemToDraw(feature, &top, &bottom))
                continue ;

              box_data        = g_new0(ZMapWindowItemFeatureStruct, 1) ;
              box_data->start = intron_span->x1;
              box_data->end   = intron_span->x2;

              intron_data        = g_new0(ZMapWindowItemFeatureStruct, 1) ;
	      intron_data->subpart = ZMAPFEATURE_SUBPART_INTRON ;
              intron_data->start = intron_span->x1;
              intron_data->end   = intron_span->x2;

              zmapWindowSeq2CanOffset(&top, &bottom, offset);
              intron_box = zMapDrawSSPolygon(feature_item, 
                                             ZMAP_POLYGON_SQUARE, 
                                             left, right,
                                             top, bottom,
                                             NULL, NULL,
                                             feature->strand);
              
              attachDataToItem(intron_box, window, feature, ITEM_FEATURE_BOUNDING_BOX, box_data);
              zmapWindowLongItemCheck(window, intron_box, top, bottom);

              if(!feature->feature.transcript.flags.cds || 
                 (feature->feature.transcript.flags.cds &&
                 (cds_start > bottom)))
                cds_bg = background;  /* probably a reverse & cds_end thing here too! */
              else
                cds_bg = foreground;
              
              intron_line = zMapDrawAnnotatePolygon(intron_box,
                                                    ZMAP_ANNOTATE_INTRON, 
                                                    NULL,
                                                    cds_bg,
                                                    line_width,
                                                    feature->strand);
              attachDataToItem(intron_line, window, feature, ITEM_FEATURE_CHILD, intron_data);
              zmapWindowLongItemCheck(window, intron_line, top, bottom);              
                  
              intron_data->twin_item = intron_box;
              box_data->twin_item = intron_line;
              
            }
	}


      if (feature->feature.transcript.exons)
	{
	  for (i = 0; i < feature->feature.transcript.exons->len; i++)
	    {
	      ZMapSpan exon_span ;
	      FooCanvasItem *exon_box ;
	      ZMapWindowItemFeature exon_data ;
	      double top, bottom ;

	      exon_span = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i);

              top    = exon_span->x1;
              bottom = exon_span->x2;

              if(featureClipItemToDraw(feature, &top, &bottom))
                continue;

              zmapWindowSeq2CanOffset(&top, &bottom, offset) ;


              exon_data        = g_new0(ZMapWindowItemFeatureStruct, 1) ;
	      exon_data->subpart = ZMAPFEATURE_SUBPART_EXON ;
              exon_data->start = exon_span->x1;
	      exon_data->end   = exon_span->x2;

              if(!feature->feature.transcript.flags.cds
                 || (feature->feature.transcript.flags.cds &&
		     (cds_start > bottom)))
                cds_bg = background;  /* probably a reverse & cds_end thing here too! */
              else
                cds_bg = foreground;

              exon_box = zMapDrawSSPolygon(feature_item, 
                                           ZMAP_POLYGON_POINTING, 
                                           x1, x2,
                                           top, bottom,
                                           outline, cds_bg,
                                           feature->strand);
                
              attachDataToItem(exon_box, window, feature, ITEM_FEATURE_CHILD, exon_data);
              zmapWindowLongItemCheck(window, exon_box, top, bottom);

              if (feature->feature.transcript.flags.cds)
		{
		  FooCanvasItem *non_cds_box = NULL ;
		  int non_start, non_end ;

		  /* Watch out for the conditions on the start/end here... */
		  if ((cds_start > top) && (cds_start <= bottom))
		    {
		      non_cds_box = zMapDrawAnnotatePolygon(exon_box,
							(feature->strand == ZMAPSTRAND_REVERSE ? 
							 ZMAP_ANNOTATE_UTR_LAST
							 : ZMAP_ANNOTATE_UTR_FIRST),
							outline,
							background,
							cds_start,
							feature->strand) ;

		      if (non_cds_box)
			{
			  non_start = top ;
			  non_end = top + (cds_start - top) - 1 ;

			  exon_data        = g_new0(ZMapWindowItemFeatureStruct, 1) ;
			  exon_data->subpart = ZMAPFEATURE_SUBPART_EXON ;
			  exon_data->start = non_start + offset ;
			  exon_data->end   = non_end + offset ;

			  attachDataToItem(non_cds_box, window, feature, ITEM_FEATURE_CHILD, exon_data);
			  zmapWindowLongItemCheck(window, non_cds_box, non_start, non_end) ;
			}
		    }
		  if ((cds_end >= top) && (cds_end < bottom))
		    {
		      non_cds_box = zMapDrawAnnotatePolygon(exon_box,
							(feature->strand == ZMAPSTRAND_REVERSE ? 
							 ZMAP_ANNOTATE_UTR_FIRST
							 : ZMAP_ANNOTATE_UTR_LAST),
							outline,
							background,
							cds_end,
							feature->strand) ;

		      if (non_cds_box)
			{
			  non_start = bottom - (bottom - cds_end) + 1 ;
			  non_end = bottom ;

			  exon_data = g_new0(ZMapWindowItemFeatureStruct, 1) ;
			  exon_data->subpart = ZMAPFEATURE_SUBPART_EXON ;
			  exon_data->start = non_start + offset ;
			  exon_data->end   = non_end + offset - 1 ; /* - 1 because non_end
								       calculated from bottom
								       which covers the last base. */

			  attachDataToItem(non_cds_box, window, feature, ITEM_FEATURE_CHILD, exon_data);
			  zmapWindowLongItemCheck(window, non_cds_box, non_start, non_end);
			}
		    }
		}

            }
	}


    }

  return feature_item ;
}



static gboolean featureClipItemToDraw(ZMapFeature feature, 
                                      double *start_inout, 
                                      double *end_inout)
{
  gboolean out_of_block  = FALSE; /* most will be within the block bounds */
  double block_start, block_end; /* coords of the block */
  int start_end_crossing = 0;
  ZMapFeatureBlock block = NULL;
  ZMapFeatureAny any     = NULL;

  zMapAssert(feature && start_inout && end_inout);

  any   = feature->parent->parent;
  zMapAssert(any && any->struct_type == ZMAPFEATURE_STRUCT_BLOCK);
  block = (ZMapFeatureBlock)any;

  /* Get block start and end */
  block_start = block->block_to_sequence.q1;
  block_end   = block->block_to_sequence.q2;

  /* shift according to how we cross, like this for ease of debugging, not speed */
  start_end_crossing |= ((*start_inout < block_start) << 1);
  start_end_crossing |= ((*end_inout   > block_end)   << 2);
  start_end_crossing |= ((*start_inout > block_end)   << 3);
  start_end_crossing |= ((*end_inout   < block_start) << 4);

  /* Now check whether we cross! */
  if(start_end_crossing & 8 || 
     start_end_crossing & 16) /* everything is out of range don't display! */
    out_of_block = TRUE;        

  if(start_end_crossing & 2)
    *start_inout = block_start;
  if(start_end_crossing & 4)
    *end_inout = block_end;


  return out_of_block ;
}

static void attachDataToItem(FooCanvasItem *feature_item, 
                             ZMapWindow window,
                             ZMapFeature feature,
                             ZMapWindowItemFeatureType type,
                             gpointer subFeatureData)
{
  zMapAssert(feature_item && window && feature);

  g_object_set_data(G_OBJECT(feature_item), ITEM_FEATURE_TYPE,
                    GINT_TO_POINTER(type)) ;
  g_object_set_data(G_OBJECT(feature_item), ITEM_FEATURE_DATA, feature) ;
  if(subFeatureData != NULL)
    g_object_set_data(G_OBJECT(feature_item), ITEM_SUBFEATURE_DATA,
                      subFeatureData);

  g_signal_connect(GTK_OBJECT(feature_item), "event",
                   GTK_SIGNAL_FUNC(canvasItemEventCB), window) ;
  g_signal_connect(GTK_OBJECT(feature_item), "destroy",
                   GTK_SIGNAL_FUNC(canvasItemDestroyCB), window) ;

  return ;
}



/* I'm not completely happy with all the hash removing/adding stuff....if we don't remember to
 * do this properly then it will cause us to crash.... */
static void dna_redraw_callback(FooCanvasGroup *text_grp,
                                double zoom,
                                gpointer user_data)
{
  ZMapWindow window       = (ZMapWindow)user_data;
  FooCanvasItem *grp_item = FOO_CANVAS_ITEM(text_grp);

  if(zmapWindowItemIsVisible(grp_item)) /* No need to redraw if we're not visible! */
    {
      textGroupSelection select = NULL;
#ifdef RDS_DONT_INCLUDE
      FooCanvasItem *txt_item = NULL;
      ZMapDrawTextRowData   trd = NULL;
#endif
      FooCanvasGroup *container ;
      ZMapFeature feature ;
      gboolean status ;
      GQuark column_id ;


      /* Grab this so we can reattach it to new text group. */
      feature = (ZMapFeature)g_object_get_data(G_OBJECT(text_grp), ITEM_FEATURE_DATA) ;
      zMapAssert(feature) ;


      status = zmapWindowFToIRemoveFeature(window->context_to_item,
					   feature->parent->parent->parent->unique_id,
					   feature->parent->parent->unique_id,
					   feature->parent->unique_id,
					   feature->strand,
					   feature->unique_id) ;
      zMapAssert(status) ;



      container = zmapWindowContainerGetParent(grp_item->parent) ;

      /* We should hide these so they don't look odd */
      select = getTextGroupData(text_grp, window);
      foo_canvas_item_hide(FOO_CANVAS_ITEM(select->tooltip));
      foo_canvas_item_hide(FOO_CANVAS_ITEM(select->highlight));

      /* gets rid of text_grp !!!!... so we must create it again.... */
      zmapWindowContainerPurge(FOO_CANVAS_GROUP(grp_item->parent)) ;

      text_grp = FOO_CANVAS_GROUP(foo_canvas_item_new(zmapWindowContainerGetFeatures(container),
						      foo_canvas_group_get_type(),
						      NULL)) ;
      g_object_set_data(G_OBJECT(text_grp), CONTAINER_REDRAW_CALLBACK,
			dna_redraw_callback) ;
      g_object_set_data(G_OBJECT(text_grp), CONTAINER_REDRAW_DATA,
			(gpointer)window) ;

      /* This sets an event handler which would need to be merged with the below.... */
      zmapWindowColumnWriteDNA(window, text_grp);


      /* TRY THIS...EVENT HANDLER MAY INTERFERE THOUGH...SO MAY NEED TO REMOVE EVENT HANDLERS... */
      attachDataToItem(FOO_CANVAS_ITEM(text_grp), window, feature, ITEM_FEATURE_SIMPLE, NULL) ;



      /* AGGGGGGGGGGGGGGGHHHHHHHHHHHHHHHHHHH horrible...sort this out....should be automatic. */
      if (feature->strand == ZMAPSTRAND_FORWARD || feature->strand == ZMAPSTRAND_NONE)
	{
	  column_id = zmapWindowFToIMakeSetID(feature->parent->unique_id, ZMAPSTRAND_FORWARD) ;
	}
      else
	{
	  column_id = zmapWindowFToIMakeSetID(feature->parent->unique_id, ZMAPSTRAND_REVERSE) ;
	}

      status = zmapWindowFToIAddFeature(window->context_to_item,
					feature->parent->parent->parent->unique_id,
					feature->parent->parent->unique_id,
					column_id,
					feature->unique_id,
					FOO_CANVAS_ITEM(text_grp)) ;
      zMapAssert(status) ;



#ifdef RDS_DONT_INCLUDE
      /* We could probably redraw the highlight here! 
       * Not sure this is good. Although it sort of works
       * it's not superb and I think I prefer not doing so */
      if(text_grp->item_list &&
         (txt_item = FOO_CANVAS_ITEM(text_grp->item_list->data)) && 
         (trd = zMapDrawGetTextItemData(txt_item)))
        zMapDrawHighlightTextRegion(select->highlight,
                                    select->seqFirstIdx, 
                                    select->seqLastIdx, 
                                    txt_item);
#endif
    }
  
  return ;
}




/* Callback for destroy of items... */
static gboolean canvasItemDestroyCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;			    /* Make sure any other callbacks also
							       get run. */
  ZMapWindowItemFeatureType item_feature_type ;
#ifdef RDS_DONT_INCLUDE
  ZMapWindow  window = (ZMapWindowStruct*)data ;
#endif

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;

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

	item_data = g_object_get_data(G_OBJECT(item), ITEM_SUBFEATURE_DATA) ;
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



/* Callback for any events that happen on individual canvas items.
 * 
 * NOTE that we can use a static for the double click detection because the
 * GUI runs in a single thread and because the user cannot double click on
 * more than one window at a time....;-)
 *  */
static gboolean canvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;
  ZMapWindow window = (ZMapWindowStruct*)data ;
  ZMapFeature feature ;
  static guint32 last_but_press = 0 ;			    /* Used for double clicks... */


  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
      {
	GdkEventButton *but_event = (GdkEventButton *)event ;
	ZMapWindowItemFeatureType item_feature_type ;
	FooCanvasItem *real_item ;

	item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							      ITEM_FEATURE_TYPE)) ;
	zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE
		   || item_feature_type == ITEM_FEATURE_CHILD
		   || item_feature_type == ITEM_FEATURE_BOUNDING_BOX) ;

	/* If its a bounding box then we don't want the that to influence highlighting.. */
	if (item_feature_type == ITEM_FEATURE_BOUNDING_BOX)
	  real_item = zMapWindowFindFeatureItemByItem(window, item) ;
	else
	  real_item = item ;

	/* Retrieve the feature item info from the canvas item. */
	feature = (ZMapFeature)g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);  
	zMapAssert(feature) ;

	if (but_event->type == GDK_BUTTON_PRESS)
	  {
	    /* Double clicks occur within 250 milliseconds so we ignore the second button
	     * press generated by clicks but catch the button2 press (see below). */
	    if (but_event->time - last_but_press > 250)
	      {
		/* Button 1 and 3 are handled, 2 is left for a general handler which could be
		 * the root handler. */
		if (but_event->button == 1 || but_event->button == 3)
		  {
		    /* Pass information about the object clicked on back to the application. */
		    zMapWindowUpdateInfoPanel(window, feature, real_item) ;
		
		    if (but_event->button == 3)
		      {
			/* Pop up an item menu. */
			makeItemMenu(but_event, window, real_item) ;
		      }
		  }
	      }

	    last_but_press = but_event->time ;

	    event_handled = TRUE ;
	  }
	else
	  {
	    /* Handle second click of a double click. */
	    if (but_event->button == 1)
	      {
		gboolean result ;
		
		result = makeFeatureEditWindow(window, feature) ;
		zMapAssert(result) ;			    /* v. bad news if we can't find this item. */

		event_handled = TRUE ;
	      }
	  }
	break ;
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






static gboolean dnaItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE;
  FooCanvasGroup *txtGroup = NULL;
  textGroupSelection txtSelect = NULL;

  txtGroup  = FOO_CANVAS_GROUP(item->parent) ;

  txtSelect = getTextGroupData(txtGroup, (ZMapWindow)data); /* This will initialise for us if it's not there. */
  zMapAssert(txtSelect);

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
      {
        event_handled   = TRUE;
        if(event->type == GDK_BUTTON_RELEASE)
          {
#define DONTGRABPOINTER
#ifndef DONTGRABPOINTER
            foo_canvas_item_ungrab(item, event->button.time);
#endif 
            txtSelect->originItemListMember = NULL;
            foo_canvas_item_hide(FOO_CANVAS_ITEM(txtSelect->tooltip));
          }
        else
          {
            GdkCursor *xterm;
            GList *list;
            xterm = gdk_cursor_new (GDK_XTERM);

#ifndef DONTGRABPOINTER
            foo_canvas_item_grab(item, 
                                 GDK_POINTER_MOTION_MASK |  
                                 GDK_BUTTON_RELEASE_MASK | 
                                 GDK_BUTTON_MOTION_MASK,
                                 xterm,
                                 event->button.time);
#endif
            gdk_cursor_destroy(xterm);
            list = g_list_first(txtGroup->item_list);
            while(list)
              {
                /* pointers equal */
                if(item != list->data)
                  list = list->next;
                else
                  {
                    txtSelect->originItemListMember = list;
                    list = NULL;
                  }
              }
            txtSelect->seqFirstIdx = txtSelect->seqLastIdx = -1;
            updateInfoGivenCoords(txtSelect, event->button.x, event->button.y);
          }
      }
      break;
    case GDK_MOTION_NOTIFY:
      {
        if(txtSelect->originItemListMember)
          {
            ZMapDrawTextRowData trd = NULL;
            if((trd = zMapDrawGetTextItemData(item)))
              {
                updateInfoGivenCoords(txtSelect, event->motion.x, event->motion.y);
                zMapDrawHighlightTextRegion(txtSelect->highlight,
                                            txtSelect->seqFirstIdx, 
                                            txtSelect->seqLastIdx,
                                            item);
                event_handled = TRUE;
              }
          }
      }    
      break;
    case GDK_LEAVE_NOTIFY:
      {
        foo_canvas_item_hide(FOO_CANVAS_ITEM( txtSelect->tooltip ));
      }
    case GDK_ENTER_NOTIFY:
      {
        
      }
      break;
    default:
      {
	/* By default we _don't_handle events. */
	event_handled = FALSE ;
	break ;
      }
    }

  return event_handled ;
}



static textGroupSelection getTextGroupData(FooCanvasGroup *txtGroup, ZMapWindow window)
{
  textGroupSelection txtSelect = NULL;

  if(!(txtSelect = (textGroupSelection)g_object_get_data(G_OBJECT(txtGroup), "HIGHLIGHT_INFO")))
    {
      txtSelect  = (textGroupSelection)g_new0(textGroupSelectionStruct, 1);
      txtSelect->tooltip = zMapDrawToolTipCreate(FOO_CANVAS_ITEM(txtGroup)->canvas);
      foo_canvas_item_hide(FOO_CANVAS_ITEM( txtSelect->tooltip ));

      txtSelect->highlight = FOO_CANVAS_GROUP(foo_canvas_item_new(txtGroup,
                                                                  foo_canvas_group_get_type(),
                                                                  NULL));
      txtSelect->window = window;
      g_object_set_data(G_OBJECT(txtGroup), "HIGHLIGHT_INFO", (gpointer)txtSelect);

    }

  return txtSelect ;
}



ZMapDrawTextIterator getIterator(double win_min_coord, double win_max_coord,
                                 double offset_start,  double offset_end,
                                 double text_height, gboolean numbered)
{
  int tmp;
  ZMapDrawTextIterator iterator   = g_new0(ZMapDrawTextIteratorStruct, 1);

  iterator->seq_start      = win_min_coord;
  iterator->seq_end        = win_max_coord - 1.0;
  iterator->offset_start   = floor(offset_start - win_min_coord);
  iterator->offset_end     = ceil( offset_end   - win_min_coord);
  iterator->shownSeqLength = ceil( offset_end - offset_start + 1.0);

  iterator->x            = ZMAP_WINDOW_TEXT_BORDER;
  iterator->y            = 0.0;

  iterator->n_bases      = ceil(text_height);

  iterator->length2draw  = iterator->shownSeqLength;
  /* checks for trailing bases */
  iterator->length2draw -= (tmp = iterator->length2draw % iterator->n_bases);

  iterator->rows         = iterator->length2draw / iterator->n_bases;
  iterator->cols         = iterator->n_bases;
  
  if((iterator->numbered = numbered))
    {
      double dtmp;
      int i = 0;
      dtmp = (double)iterator->length2draw;
      while(++i && ((dtmp = dtmp / 10) > 1.0)){ /* Get the int's string length for the format. */ }  
      iterator->format = g_strdup_printf("%%%dd: %%s...", i);
    }
  else
    iterator->format = "%s...";
  
  return iterator;
}


static void destroyIterator(ZMapDrawTextIterator iterator)
{
  zMapAssert(iterator);
  if(iterator->numbered && iterator->format)
    g_free(iterator->format);
  g_free(iterator);

  return ;
}






/* Build the menu for a feature item. */
static void makeItemMenu(GdkEventButton *button_event, ZMapWindow window, FooCanvasItem *item)
{
  char *menu_title = "Item menu" ;
  GList *menu_sets = NULL ;
  ItemMenuCBData menu_data ;
  ZMapFeature feature ;
#ifdef RDS_DONT_INCLUDE
  ZMapGUIMenuItem menu_item ;
#endif

  /* Some parts of the menu are feature type specific so retrieve the feature item info
   * from the canvas item. */
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;

  /* Call back stuff.... */
  menu_data = g_new0(ItemMenuCBDataStruct, 1) ;
  menu_data->item_cb = TRUE ;
  menu_data->window = window ;
  menu_data->item = item ;

  /* Make up the menu. */
  menu_sets = g_list_append(menu_sets, makeMenuFeatureOps(NULL, NULL, menu_data)) ;

  if (feature->url)
    menu_sets = g_list_append(menu_sets, makeMenuURL(NULL, NULL, menu_data)) ;


  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
    {
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDNATranscript(NULL, NULL, menu_data)) ;
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDNATranscriptFile(NULL, NULL, menu_data)) ;
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuPeptide(NULL, NULL, menu_data)) ;
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuPeptideFile(NULL, NULL, menu_data)) ;
    }
  else
    {
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDNAFeatureAny(NULL, NULL, menu_data)) ;
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDNAFeatureAnyFile(NULL, NULL, menu_data)) ;
    }

  if (feature->type == ZMAPFEATURE_ALIGNMENT)
    {
      if (feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
	menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuProteinHomol(NULL, NULL, menu_data)) ;
      else
	menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDNAHomol(NULL, NULL, menu_data)) ;
    }

  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuBump(NULL, NULL, menu_data)) ;

  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDumpOps(NULL, NULL, menu_data)) ;

  zMapGUIMakeMenu(menu_title, menu_sets, button_event) ;

  return ;
}


static void itemMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapFeature feature ;

  /* Retrieve the feature item info from the canvas item. */
  feature = g_object_get_data(G_OBJECT(menu_data->item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;

  switch (menu_item_id)
    {
    case 1:
      {
        GList *list = NULL;

        list = zmapWindowFToIFindItemSetFull(menu_data->window->context_to_item, 
					     feature->parent->parent->parent->unique_id,
					     feature->parent->parent->unique_id,
					     feature->parent->unique_id,
					     zMapFeatureStrand2Str(feature->strand),
					     g_quark_from_string("*")) ;

        zmapWindowListWindowCreate(menu_data->window, list, 
                                   (char *)g_quark_to_string(feature->parent->original_id), 
                                   menu_data->item) ;
	break ;
      }
    case 2:
      {
	gboolean result ;
	    
	result = makeFeatureEditWindow(menu_data->window, feature) ;
	zMapAssert(result) ;				    /* v. bad news if we can't find this item. */

	break ;
      }
    case 3:
      zmapWindowCreateSearchWindow(menu_data->window, (ZMapFeatureAny)feature) ;
      break ;
    case 4:
      pfetchEntry(menu_data->window, (char *)g_quark_to_string(feature->original_id)) ;
      break ;

    case 6:
      {
	gboolean result ;
	GError *error = NULL ;

	if (!(result = zMapLaunchWebBrowser(feature->url, &error)))
	  {
	    zMapWarning("Error: %s\n", error->message) ;

	    g_error_free(error) ;
	  }
	break ;
      }

    default:
      zMapAssertNotReached() ;				    /* exits... */
      break ;
    }

  g_free(menu_data) ;

  return ;
}



static void updateInfoGivenCoords(textGroupSelection select, 
                                  double currentX,
                                  double currentY) /* These are WORLD coords */
{
  GList *listStart = NULL;
  FooCanvasItem *origin;
  ZMapGListDirection forward = ZMAP_GLIST_FORWARD;

  select->buttonCurrentX = currentX;
  select->buttonCurrentY = currentY;
  select->need_update    = TRUE;
  listStart              = select->originItemListMember;

  /* Now work out where we are with custom g_list_foreach */
  
  /* First we need to know which way to go */
  origin = (FooCanvasItem *)(listStart->data);
  if(origin)
    {
      double x1, x2, y1, y2, halfy;
      foo_canvas_item_get_bounds(origin, &x1, &y1, &x2, &y2);
      foo_canvas_item_i2w(origin, &x1, &y2);
      foo_canvas_item_i2w(origin, &x2, &y2);
      halfy = ((y2 - y1) / 2) + y1;
      if(select->buttonCurrentY < halfy)
        forward = ZMAP_GLIST_REVERSE;
      else if(select->buttonCurrentY > halfy)
        forward = ZMAP_GLIST_FORWARD;
      else
        forward = ZMAP_GLIST_FORWARD;
    }

  if(select->need_update)
    zMap_g_list_foreach_directional(listStart, pointerIsOverItem, 
                                    (gpointer)select, forward);

  return ;
}



/* This is in the general menu and needs to be handled separately perhaps as the index is a global
 * one shared amongst all general menu functions...
 * NOTE HOW THE MENUS ARE DECLARED STATIC IN THE VARIOUS ROUTINES TO MAKE SURE THEY STAY
 * AROUND...OTHERWISE WE WILL HAVE TO KEEP ALLOCATING/DEALLOCATING THEM.....
 */
static ZMapGUIMenuItem makeMenuFeatureOps(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {"Show Feature",           2, itemMenuCB, NULL},
      {"All Column Features",      1, itemMenuCB, NULL},
      {"Feature Search Window",  3, itemMenuCB, NULL},
      {"Pfetch this feature",    4, itemMenuCB, NULL},
      {NULL,                     0, NULL,       NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


static ZMapGUIMenuItem makeMenuURL(int *start_index_inout,
				   ZMapGUIMenuItemCallbackFunc callback_func,
				   gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {"URL",                    6, itemMenuCB, NULL},
      {NULL,                     0, NULL,       NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}




static void pointerIsOverItem(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data;
  textGroupSelection select = (textGroupSelection)user_data;
  ZMapDrawTextRowData trd = NULL;
  double currentY = 0.0;
  currentY = select->buttonCurrentY - select->window->min_coord;

  if(select->need_update && (trd = zMapDrawGetTextItemData(item)))
    {
      if(currentY > trd->rowOffset &&
         currentY < trd->rowOffset + trd->fullStrLength)
        {
          /* Now what do we need to do? */
          /* update the values of seqFirstIdx && seqLastIdx */
          int currentIdx, iMinIdx, iMaxIdx;
          double x1, x2, y1, y2, char_width, clmpX;

          foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2);
          foo_canvas_item_i2w(item, &x1, &y1);
          foo_canvas_item_i2w(item, &x2, &y2);

          /* clamp inside the item! (and effectively the sequence) */
          clmpX = (select->buttonCurrentX > x1 ? 
                   select->buttonCurrentX < x2 ? select->buttonCurrentX :
                   x2 : x1);

          char_width  = trd->columnWidth / trd->drawnStrLength;
          currentIdx  = floor((clmpX - x1) / char_width); /* we're zero based so we need to be able to select the first base. revisit if it's an issue */
          currentIdx += trd->rowOffset;

          if(select->seqLastIdx  == -1 && 
             select->seqFirstIdx == select->seqLastIdx)
            select->seqFirstIdx = 
              select->seqLastIdx = 
              select->originIdx = currentIdx;

          /* clamp to the original index */
          iMinIdx = (select->seqFirstIdx < select->originIdx ? select->originIdx : select->seqFirstIdx);
          iMaxIdx = (select->seqLastIdx  > select->originIdx ? select->originIdx : select->seqLastIdx);
          
          /* resize to current index */
          select->seqFirstIdx = MIN(currentIdx, iMinIdx);
          select->seqLastIdx  = MAX(currentIdx, iMaxIdx);

          zMapDrawToolTipSetPosition(select->tooltip, 
                                     x1 - 200,
                                     trd->rowOffset - (y2 - y1), 
                                     g_strdup_printf("%d - %d", select->seqFirstIdx + 1, select->seqLastIdx + 1));

          select->need_update = FALSE;
        }
    }

  return ;
}




static void pfetchEntry(ZMapWindow window, char *sequence_name)
{
  char *error_prefix = "PFetch failed:" ;
  gboolean result ;
  gchar *command_line ;
  gchar *standard_output = NULL, *standard_error = NULL ;
  gint exit_status = 0 ;
  GError *error = NULL ;

  command_line = g_strdup_printf("pfetch -F '%s'", sequence_name) ;

  if (!(result = g_spawn_command_line_sync(command_line,
					   &standard_output, &standard_error,
					   &exit_status, &error)))
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "%s  %s", error_prefix, error->message) ;

      g_error_free(error) ;
    }
  else if (!*standard_output)
    {
      /* some versions of pfetch erroneously return nothing if they fail to find an entry. */

      zMapShowMsg(ZMAP_MSG_WARNING, "%s  %s", error_prefix, "no output returned !") ;
    }
  else
    {
      *(standard_output + ((int)(strlen(standard_output)) - 1)) = '\0' ;	/* Get rid of annoying newline */

      if (g_ascii_strcasecmp(standard_output, "no match") == 0)
	{
	  zMapShowMsg(ZMAP_MSG_INFORMATION, "%s  %s", error_prefix, standard_output) ;
	}
      else
	{
	  char *title ;
	  
	  title = g_strdup_printf("pfetch: \"%s\"", sequence_name) ;

	  zMapGUIShowText(title, standard_output, FALSE) ;

	  g_free(title) ;
	}
    }

  /* Clear up, note that we have to do this because currently they are returned as buffers
   * from g_strings (not documented in the interface) and so should always be freed. I include
   * the conditional freeing in case the implementation changes anytime. */
  if (standard_output)
    g_free(standard_output) ;
  if (standard_error)
    g_free(standard_error) ;


  g_free(command_line) ;

  return ;
}


/* We may wish to make this public some time but as the rest of the long item calls for features
 * are in this file then there is no need for it to be exposed.
 * 
 * Deals with removing all the items contained in a compound object from the longitem list.
 * 
 *  */
static void removeFeatureLongItems(GList **long_items, FooCanvasItem *feature_item)
{
  ZMapWindowItemFeatureType type ;

  zMapAssert(long_items && feature_item) ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(feature_item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(type == ITEM_FEATURE_SIMPLE
	     || type == ITEM_FEATURE_PARENT || type == ITEM_FEATURE_CHILD) ;

  /* If item is part of a compound featuret then get the parent. */
  if (type == ITEM_FEATURE_CHILD)
    feature_item = feature_item->parent ;

  /* Test for long items according to whether feature is simple or compound. */
  if (type == ITEM_FEATURE_SIMPLE)
    {
      zmapWindowLongItemRemove(long_items, feature_item) ;  /* Ignore boolean result. */
    }
  else
    {
      FooCanvasGroup *feature_group = FOO_CANVAS_GROUP(feature_item) ;

      g_list_foreach(feature_group->item_list, removeLongItemCB, long_items) ;
    }

  return ;
}


/* GFunc callback for removing long items. */
static void removeLongItemCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  GList **long_items = (GList **)user_data ;

  zmapWindowLongItemRemove(long_items, item) ;		    /* Ignore boolean result. */

  return ;
}



/* Returns TRUE if item found, FALSE otherwise.  */
static gboolean makeFeatureEditWindow(ZMapWindow window, ZMapFeature feature)
{
  gboolean result = FALSE ;
  FooCanvasItem *item ;

  if ((item = zmapWindowFToIFindItemFull(window->context_to_item, 
					 feature->parent->parent->parent->unique_id,
					 feature->parent->parent->unique_id,
					 feature->parent->unique_id,
					 feature->strand,
					 feature->unique_id)))
    {
      zmapWindowEditorCreate(window, item) ;
      result = TRUE ;
    }

  return result ;
}
