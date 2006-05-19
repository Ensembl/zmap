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
 * Last edited: May 19 17:02 2006 (edgrif)
 * Created: Mon Jan  9 10:25:40 2006 (edgrif)
 * CVS info:   $Id: zmapWindowFeature.c,v 1.28 2006-05-19 16:02:59 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <math.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapPeptide.h>
#include <ZMap/zmapGLibUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>


typedef struct 
{
  ZMapWindow window;
  FooCanvasGroup *column;
}processFeatureDataStruct, *processFeatureData;


/* So we can redraw a featureset */
static void drawEachFeature(GQuark key_id, gpointer data, gpointer user_data);

static FooCanvasItem *createParentGroup(FooCanvasGroup *parent,
                                        ZMapFeature feature,
                                        double y_origin) ;
static FooCanvasItem *drawSimpleFeature(FooCanvasGroup *parent, ZMapFeature feature,
					double feature_offset,
					double x1, double y1, double x2, double y2,
					GdkColor *outline,
                                        GdkColor *foreground,
                                        GdkColor *background,
					guint line_width,
					ZMapWindow window) ;
static FooCanvasItem *drawTranscriptFeature(FooCanvasGroup *parent, ZMapFeature feature,
					    double feature_offset,
					    double x1, double feature_top,
					    double x2, double feature_bottom,
					    GdkColor *outline, 
                                            GdkColor *foreground,
                                            GdkColor *background,
					    guint line_width,
					    ZMapWindow window) ;
static FooCanvasItem *drawAlignmentFeature(FooCanvasGroup *parent, ZMapFeature feature,
                                           double feature_offset,
                                           double x1, double feature_top,
                                           double x2, double feature_bottom,
                                           GdkColor *outline, 
                                           GdkColor *foreground,
                                           GdkColor *background,
					   guint line_width,
                                           ZMapWindow window) ;
static FooCanvasItem *drawDNA(FooCanvasGroup *parent, ZMapFeature feature,
                              double feature_offset,
                              double feature_zero,      double feature_top,
                              double feature_thickness, double feature_bottom,
                              GdkColor *outline, 
                              GdkColor *foreground,
                              GdkColor *background,
                              ZMapWindow window) ;
static FooCanvasItem *drawPep(FooCanvasGroup *parent, ZMapFeature feature,
                              double feature_offset,
                              double feature_zero,      double feature_top,
                              double feature_thickness, double feature_bottom,
                              GdkColor *outline, 
                              GdkColor *foreground,
                              GdkColor *background,
                              ZMapWindow window) ;
static void drawTextWrappedInColumn(FooCanvasItem *parent, char *text, 
                                    double feature_start, double feature_end,
                                    double feature_offset,
                                    GdkColor *fg, GdkColor *bg, GdkColor *out,
                                    double width, int bases_per_char, 
                                    GCallback eventCB, ZMapWindow window);

static ZMapGUIMenuItem makeMenuURL(int *start_index_inout,
				   ZMapGUIMenuItemCallbackFunc callback_func,
				   gpointer callback_data);
static void makeItemMenu(GdkEventButton *button_event, ZMapWindow window,
			 FooCanvasItem *item) ;
static ZMapGUIMenuItem makeMenuURL(int *start_index_inout,
				   ZMapGUIMenuItemCallbackFunc callback_func,
				   gpointer callback_data) ;
static void itemMenuCB(int menu_item_id, gpointer callback_data) ;

static ZMapGUIMenuItem makeMenuFeatureOps(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data) ;

static void dnaHandleZoomCB(FooCanvasItem *container,
                            double zoom,
                            ZMapWindow user_data);
static gboolean dnaItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;

static gboolean featureClipItemToDraw(ZMapFeature feature, 
                                      double *start_inout, 
                                      double *end_inout) ;
static void attachDataToItem(FooCanvasItem *feature_item, 
                             ZMapWindow window,
                             ZMapFeature feature,
                             ZMapWindowItemFeatureType type,
                             gpointer subFeatureData) ;

static ZMapDrawTextIterator zmapDrawTextIteratorBuild(double feature_start, double feature_end,
                                                      double feature_offset,
                                                      double scroll_start,  double scroll_end,
                                                      double text_width,    double text_height, 
                                                      char  *full_text,     int    bases_per_char,
                                                      gboolean numbered,
                                                      PangoFontDescription *font);

static void destroyIterator(ZMapDrawTextIterator iterator) ;

static gboolean canvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static gboolean canvasItemDestroyCB(FooCanvasItem *item, gpointer data) ;

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

  return window->feature_context->styles ;
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

  feature = g_object_get_data(G_OBJECT(feature_item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature && zMapFeatureIsValid((ZMapFeatureAny)feature)) ;

  feature_set = (ZMapFeatureSet)(feature->parent) ;

  /* Need to delete the feature from the feature set and from the hash and destroy the
   * canvas item....NOTE this is very order dependent. */
  if (zmapWindowFToIRemoveFeature(zmap_window->context_to_item, feature)
      && zMapFeatureSetRemoveFeature(feature_set, feature))
    {
      /* destroy the canvas item... */
      gtk_object_destroy(GTK_OBJECT(feature_item)) ;
      
      result = TRUE ;
    }

  return result ;
}


/* Encapulates the rules about which strand a feature will be drawn on.
 * 
 * For ZMap this amounts to:
 *                            strand specific     not strand specific
 *       features strand
 * 
 *       ZMAPSTRAND_NONE            +                     +
 *       ZMAPSTRAND_FORWARD         +                     +
 *       ZMAPSTRAND_REVERSE         -                     +
 * 
 * Some features (e.g. repeats) are derived from alignments that match to the
 * forward or reverse strand and hence the feature can be said to have strand
 * in the alignment sense but for the actual feature (the repeat) the strand
 * is irrelevant.
 * 
 *  */
ZMapStrand zmapWindowFeatureStrand(ZMapFeature feature)
{
  ZMapStrand strand = ZMAPSTRAND_FORWARD ;
  ZMapFeatureTypeStyle style = feature->style ;


  if (!(style->strand_specific)
      || (feature->strand == ZMAPSTRAND_FORWARD || feature->strand == ZMAPSTRAND_NONE))
    strand = ZMAPSTRAND_FORWARD ;
  else
    strand = ZMAPSTRAND_REVERSE ;

    return strand ;
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
  FooCanvasGroup *column_group ;
  FooCanvasItem *top_feature_item = NULL ;
  GdkColor *b, *f, *o;
  guint line_width ;
  double feature_offset;
  double start_x, end_x ;


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


  /* Start/end of feature within alignment block.
   * Feature position on screen is determined the relative offset of the features coordinates within
   * its align block added to the alignment block _query_ coords. You can't just use the
   * features own coordinates as these will be its coordinates in its original sequence. */

  feature_offset = block->block_to_sequence.q1 ;

  /* Note: for object to _span_ "width" units, we start at zero and end at "width". */
  start_x = 0.0 ;
  end_x   = style->width ;

  zMapFeatureTypeGetColours(style, &b, &f, &o);

  line_width = window->config.feature_line_width ;


  switch (feature->type)
    {
    case ZMAPFEATURE_BASIC:
      {
	top_feature_item = drawSimpleFeature(column_group, feature,
					     feature_offset,
					     start_x, feature->x1, end_x, feature->x2,
					     o, f, b,
					     line_width,
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
                                                  o, f, b,
						  line_width,
                                                  window);
        else
          top_feature_item = drawSimpleFeature(column_group, feature,
                                               feature_offset,
                                               start_x, feature->x1, end_x, feature->x2,
                                               o, f, b,
					       line_width,
                                               window) ;
        break;
      }
    case ZMAPFEATURE_RAW_SEQUENCE:
      {
        ZMapFeatureBlock block = NULL;
        block = (ZMapFeatureBlock)(feature->parent->parent);

	/* Surely this must be true.... */
        zMapAssert( block && block->sequence.sequence );

	top_feature_item = drawDNA(column_group, feature,
                                   feature_offset,
                                   start_x, feature->x1, end_x, feature->x2,
                                   o, f, b,
                                   window) ;

        break ;
      }
    case ZMAPFEATURE_PEP_SEQUENCE:
      {
        ZMapFeatureTypeStyle pep_style = NULL;
        top_feature_item = drawPep(column_group, feature,
                                   feature_offset,
                                   start_x, feature->x1, end_x, feature->x2,
                                   o, f, b,
                                   window);
        pep_style = zmapWindowContainerGetStyle(set_group) ;
        pep_style->bump_width = 300.0;
        break;
      }
    case ZMAPFEATURE_TRANSCRIPT:
      {
	top_feature_item = drawTranscriptFeature(column_group, feature,
						 feature_offset,
						 start_x, 0.0, end_x, 0.0,
						 o, f, b,
						 line_width,
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
					set->unique_id,
					feature->unique_id, top_feature_item) ;
      zMapAssert(status) ;

      new_feature = top_feature_item ;
    }

  return new_feature ;
}


/* This is not a great function...a bit dumb really...but hard to make smarter as caller
 * must make a number of decisions which we can't.... */
char *zmapWindowFeatureSetDescription(GQuark feature_set_id, ZMapFeatureTypeStyle style)
{
  char *description =  NULL ;

  zMapAssert(feature_set_id && style) ;

  description = g_strdup_printf("%s  :  %s%s%s", (char *)g_quark_to_string(feature_set_id),
				style->description ? "\"" : "",
				style->description ? style->description : "",
				style->description ? "\"" : "") ;

  return description ;
}





/* 
 *                       Internal functions.
 */


static void printItem(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ZMapWindowItemFeatureType type ;
  

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;

  if (type == ITEM_FEATURE_SIMPLE)
    printf("found feature simple\n") ;


  return ;
}





/* LONG ITEMS SHOULD BE INCORPORATED IN THIS LOT.... */
/* some drawing functions, may want these to be externally visible later... */


static FooCanvasItem *createParentGroup(FooCanvasGroup *parent,
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
					guint line_width,
					ZMapWindow window)
{
  FooCanvasItem *feature_item ;

  if(featureClipItemToDraw(feature, &y1, &y2))
    return NULL ;

  zmapWindowSeq2CanOffset(&y1, &y2, feature_offset) ;
  feature_item = zMapDrawBox(FOO_CANVAS_ITEM(parent),
                             x1, y1, x2, y2,
                             outline, background, 0.0) ;

  attachDataToItem(feature_item, window, feature, ITEM_FEATURE_SIMPLE, NULL);

  zmapWindowLongItemCheck(window, feature_item, y1, y2) ;

  return feature_item ;
}



static FooCanvasItem *drawAlignmentFeature(FooCanvasGroup *parent, ZMapFeature feature,
                                           double feature_offset,
                                           double x1, double feature_top,
                                           double x2, double feature_bottom,
                                           GdkColor *outline, 
                                           GdkColor *foreground,
                                           GdkColor *background,
					   guint line_width,
                                           ZMapWindow window) 
{
  FooCanvasItem *feature_item ;
  double offset ;
  int i ;

  if (!feature->feature.homol.align)
    feature_item = drawSimpleFeature(parent, feature, feature_offset,
                                     x1, feature_top, x2, feature_bottom,
                                     outline, foreground, background, line_width, window) ;
  else if(feature->feature.homol.align)
    {
      double feature_start, feature_end;
      FooCanvasItem *lastBoxWeDrew   = NULL;
      FooCanvasGroup *feature_group  = NULL;
      ZMapAlignBlock prev_align_span = NULL;
      double dimension = 1.5 ;

      feature_start = feature->x1;
      feature_end   = feature->x2; /* I want this function to find these values */
      zmapWindowSeq2CanOffset(&feature_start, &feature_end, feature_offset) ;
      feature_item  = createParentGroup(parent, feature, feature_start);
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
					  line_width,
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
						  line_width,
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
						   line_width,
                                                   feature->strand);
                  attachDataToItem(gap_line_box, window, feature, ITEM_FEATURE_BOUNDING_BOX, box_data);
                  zmapWindowLongItemCheck(window, gap_line_box, bottom, top);              

                  gap_line = zMapDrawAnnotatePolygon(gap_line_box,
                                                     ZMAP_ANNOTATE_GAP, 
                                                     NULL,
                                                     background,
						     dimension,
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
					      line_width,
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
					    guint line_width,
					    ZMapWindow window)
{
  FooCanvasItem *feature_item ;
  GdkColor *intron_fill = NULL;
  int i ;
  double offset ;

  if((intron_fill = background) == NULL)
    intron_fill   = outline;
  
  /* If there are no exons/introns then just draw a simple box. Can happen for putative
   * transcripts or perhaps where user has a single exon object but does not give an exon,
   * only an overall extent. */
  if (!feature->feature.transcript.introns && !feature->feature.transcript.exons)
    {
      feature_item = drawSimpleFeature(parent, feature, feature_offset,
				       x1, feature->x1, x2, feature->x2,
				       outline, foreground, background, line_width, window) ;
    }
  else
    {
      double feature_start, feature_end;
      double cds_start = 0.0, cds_end = 0.0; /* cds_start < cds_end */
      FooCanvasGroup *feature_group ;
      gboolean has_cds = FALSE;

      feature_start = feature->x1;
      feature_end   = feature->x2; /* I want this function to find these values */
      
      zmapWindowSeq2CanOffset(&feature_start, &feature_end, feature_offset) ;

      feature_item  = createParentGroup(parent, feature, feature_start);
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
          has_cds   = TRUE;

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
          GdkColor *line_fill = intron_fill;
          double dimension = 1.5 ;

	  for (i = 0 ; i < feature->feature.transcript.introns->len ; i++)  
	    {
	      FooCanvasItem *intron_box, *intron_line ;
	      double left, right, top, bottom ;
	      ZMapWindowItemFeature box_data, intron_data ;
	      ZMapSpan intron_span ;

	      intron_span = &g_array_index(feature->feature.transcript.introns, ZMapSpanStruct, i) ;

	      left   = x1;
	      right  = x2 - (double)line_width ;
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
					     line_width,
                                             feature->strand);
              
              attachDataToItem(intron_box, window, feature, ITEM_FEATURE_BOUNDING_BOX, box_data);
              zmapWindowLongItemCheck(window, intron_box, top, bottom);

              /* IF we are part of CDS we need to set intron_fill to foreground */
              /* introns will be completely contained */
#ifdef RDS_FMAP_DOESNT_DO_THIS
              if(has_cds && 
                 ((cds_start < top) && (cds_end >= bottom)) )
                line_fill = foreground;
              else
                line_fill = intron_fill;
#endif
              intron_line = zMapDrawAnnotatePolygon(intron_box,
                                                    ZMAP_ANNOTATE_INTRON, 
                                                    NULL,
                                                    line_fill,
						    dimension,
                                                    line_width,
                                                    feature->strand);
              attachDataToItem(intron_line, window, feature, ITEM_FEATURE_CHILD, intron_data);
              zmapWindowLongItemCheck(window, intron_line, top, bottom);              
                  
              intron_data->twin_item = intron_box;
              box_data->twin_item    = intron_line;
              
            }

	}


      if (feature->feature.transcript.exons)
	{
          GdkColor *exon_fill = background, 
            *exon_outline = outline;
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

              /* if cds boundary is within this exon we use the
               * foreground colour, else it's the background. 
               * If background is null though outline will need
               * to be set to what background would be and 
               * background should then = NULL */
              if(has_cds && ((cds_start > bottom) || (cds_end < top)))
                {
                  if((exon_fill = background) == NULL)
                    exon_outline = outline;
                }
              else if(has_cds)
                { 
                  if((exon_fill = background) == NULL)
                    exon_outline = foreground;
                  else
                    exon_fill = foreground;
                }
              else if((exon_fill = background) == NULL)
                {
                  exon_outline = outline;
                }

              exon_box = zMapDrawSSPolygon(feature_item, 
                                           ZMAP_POLYGON_POINTING, 
                                           x1, x2,
                                           top, bottom,
                                           exon_outline, 
                                           exon_fill,
					   line_width,
                                           feature->strand);
                
              attachDataToItem(exon_box, window, feature, ITEM_FEATURE_CHILD, exon_data);
              zmapWindowLongItemCheck(window, exon_box, top, bottom);

              if (has_cds)
		{
		  FooCanvasItem *utr_box = NULL ;
		  int non_start, non_end ;

		  /* Watch out for the conditions on the start/end here... */
		  if ((cds_start > top) && (cds_start <= bottom))
		    {
		      utr_box = zMapDrawAnnotatePolygon(exon_box,
							(feature->strand == ZMAPSTRAND_REVERSE ? 
							 ZMAP_ANNOTATE_UTR_LAST
							 : ZMAP_ANNOTATE_UTR_FIRST),
							outline,
							background,
							cds_start,
                                                        line_width,
							feature->strand) ;

		      if (utr_box)
			{
			  non_start = top ;
			  non_end = top + (cds_start - top) - 1 ;

			  exon_data        = g_new0(ZMapWindowItemFeatureStruct, 1) ;
			  exon_data->subpart = ZMAPFEATURE_SUBPART_EXON ;
			  exon_data->start = non_start + offset ;
			  exon_data->end   = non_end + offset ;

			  attachDataToItem(utr_box, window, feature, ITEM_FEATURE_CHILD, exon_data);
			  zmapWindowLongItemCheck(window, utr_box, non_start, non_end) ;
			}
		    }
		  if ((cds_end >= top) && (cds_end < bottom))
		    {
		      utr_box = zMapDrawAnnotatePolygon(exon_box,
							(feature->strand == ZMAPSTRAND_REVERSE ? 
							 ZMAP_ANNOTATE_UTR_FIRST
							 : ZMAP_ANNOTATE_UTR_LAST),
                                                        outline,
                                                        background,
                                                        cds_end,
                                                        line_width,
                                                        feature->strand) ;

		      if (utr_box)
			{
			  non_start = bottom - (bottom - cds_end) + 1 ;
			  non_end = bottom ;

			  exon_data = g_new0(ZMapWindowItemFeatureStruct, 1) ;
			  exon_data->subpart = ZMAPFEATURE_SUBPART_EXON ;
			  exon_data->start = non_start + offset ;
			  exon_data->end   = non_end + offset - 1 ; /* - 1 because non_end
								       calculated from bottom
								       which covers the last base. */

			  attachDataToItem(utr_box, window, feature, ITEM_FEATURE_CHILD, exon_data);
			  zmapWindowLongItemCheck(window, utr_box, non_start, non_end);
			}
		    }
		}

            }
	}


    }

  return feature_item ;
}

static void drawTextWrappedInColumn(FooCanvasItem *parent, char *text, 
                                    double feature_start, double feature_end,
                                    double feature_offset,
                                    GdkColor *fg, GdkColor *bg, GdkColor *out,
                                    double width, int bases_per_char,
                                    GCallback eventCB, ZMapWindow window)
{
  ZMapDrawTextIterator iterator   = NULL;
  PangoFontDescription *font = NULL;
  double txt_height, txt_width;
  double x1, x2, y1, y2;
  int i;

  x1 = x2 = y1 = y2 = 0.0;
  /* Get the current scroll region to find exactly which sub part of
   * the text to display */
  zmapWindowScrollRegionTool(window, &x1, &y1, &x2, &y2);

  /* should this be copied? */
  font = zMapWindowZoomGetFixedWidthFontInfo(window, &txt_width, &txt_height);

  zmapWindowGetBorderSize(window, &txt_height);

  iterator = zmapDrawTextIteratorBuild(feature_start, feature_end,
                                       feature_offset,
                                       y1, y2, 
                                       txt_width, txt_height, 
                                       text, bases_per_char, 
                                       FALSE, font);

  iterator->foreground = fg;
  iterator->background = bg;
  iterator->outline    = out;

#ifdef RDS_THESE_NEED_A_LITTLE_BIT_OF_CHECKING
  printf("drawTextWrappedInColumn (%x): start=%f, end=%f, y1=%f, y2=%f\n",
         window, feature_start, feature_end, y1, y2);
#endif

  for(i = 0; i < iterator->rows; i++)
    {
      FooCanvasItem *item = NULL;
      iterator->iteration = i;

      if((item = zMapDrawRowOfText(FOO_CANVAS_GROUP(parent), 
                                   font, 
                                   text, 
                                   iterator)) && eventCB != NULL)
        {
          g_signal_connect(G_OBJECT(item), "event",
                           G_CALLBACK(eventCB), (gpointer)window);
        }
    }
  
  destroyIterator(iterator);
  
  return ;
}
 
static FooCanvasItem *drawPep(FooCanvasGroup *parent, ZMapFeature feature,
                              double feature_offset,
                              double x1, double y1, double x2, double y2,
                              GdkColor *outline, 
                              GdkColor *foreground,
                              GdkColor *background,
                              ZMapWindow window)
{
  double feature_start, feature_end, new_x = 0.0;
  FooCanvasItem  *feature_parent = NULL;
  FooCanvasItem  *prev_trans     = NULL;
  FooCanvasGroup *column_parent  = NULL;
  ZMapWindowItemHighlighter hlght= NULL;
  gpointer callback              = NULL;

  feature_start  = feature->x1;
  feature_end    = feature->x2;

  zmapWindowSeq2CanOffset(&feature_start, &feature_end, feature_offset) ;

  /* bump the feature BEFORE drawing */
  if(parent->item_list_end && (prev_trans = FOO_CANVAS_ITEM(parent->item_list_end->data)))
    foo_canvas_item_get_bounds(prev_trans, NULL, NULL, &new_x, NULL);
  
  feature_parent = createParentGroup(parent, feature, feature_start);

  new_x += COLUMN_SPACING;
  my_foo_canvas_item_goto(feature_parent, &new_x, NULL);

  if(!featureClipItemToDraw(feature, &feature_start, &feature_end))
    {
      column_parent = zmapWindowContainerGetParent(FOO_CANVAS_ITEM(parent)) ;

      zmapWindowContainerSetZoomEventHandler(column_parent, dnaHandleZoomCB, window);

      if((hlght = zmapWindowItemTextHighlightCreateData(window, 
                                                        FOO_CANVAS_GROUP(feature_parent))))
        {
          zmapWindowItemTextHighlightSetFullText(hlght, feature->text, FALSE);
          callback = G_CALLBACK(dnaItemEventCB);
        }

      drawTextWrappedInColumn(feature_parent, feature->text,
                              feature_start, feature_end, feature_offset,
                              foreground, background, outline,
                              300.0, 3, callback, window);
      
#ifdef RDS_THESE_NEED_A_LITTLE_BIT_OF_CHECKING
      /* TRY THIS...EVENT HANDLER MAY INTERFERE THOUGH...SO MAY NEED TO REMOVE EVENT HANDLERS... */
      attachDataToItem(feature_parent, window, feature, ITEM_FEATURE_SIMPLE, NULL) ;
      
#endif

    }

  return feature_parent;
}


static FooCanvasItem *drawDNA(FooCanvasGroup *parent, ZMapFeature feature,
                              double feature_offset,
                              double x1, double y1, double x2, double y2,
                              GdkColor *outline, 
                              GdkColor *foreground,
                              GdkColor *background,
                              ZMapWindow window)
{
  double feature_start, feature_end;
  FooCanvasItem  *feature_parent = NULL;
  FooCanvasGroup *column_parent  = NULL;
  ZMapFeatureBlock feature_block = NULL;
  ZMapFeatureSet   feature_set   = NULL;
  ZMapWindowItemHighlighter hlght= NULL;
  gpointer callback              = NULL;

  feature_start  = feature->x1;
  feature_end    = feature->x2;

  zmapWindowSeq2CanOffset(&feature_start, &feature_end, feature_offset) ;

  feature_parent = createParentGroup(parent, feature, feature_start);

  feature_set    = (ZMapFeatureSet)zMapFeatureGetParentGroup((ZMapFeatureAny)feature, 
                                                             ZMAPFEATURE_STRUCT_FEATURESET);
  feature_block  = (ZMapFeatureBlock)zMapFeatureGetParentGroup((ZMapFeatureAny)feature_set, 
                                                               ZMAPFEATURE_STRUCT_BLOCK);

  /* -------------------------------------------------
   * outline = highlight outline colour...
   * background = hightlight background colour...
   * foreground = text colour... 
   * -------------------------------------------------
   */
  /* what is parent's CONTAINER_TYPE_KEY and what is it's CONTAINER_DATA->level */

  column_parent = zmapWindowContainerGetParent(FOO_CANVAS_ITEM(parent)) ;

  zmapWindowContainerSetZoomEventHandler(column_parent, dnaHandleZoomCB, window);

  if((hlght = zmapWindowItemTextHighlightCreateData(window, 
                                                    FOO_CANVAS_GROUP(feature_parent))))
    {
      zmapWindowItemTextHighlightSetFullText(hlght, feature_block->sequence.sequence, FALSE);
      callback = G_CALLBACK(dnaItemEventCB);
    }

  drawTextWrappedInColumn(feature_parent, feature_block->sequence.sequence,
                          feature->x1, feature->x2, feature_offset,
                          foreground, background, outline,
                          300.0, 1, callback, window);

  /* TRY THIS...EVENT HANDLER MAY INTERFERE THOUGH...SO MAY NEED TO REMOVE EVENT HANDLERS... */
  attachDataToItem(feature_parent, window, feature, ITEM_FEATURE_SIMPLE, NULL) ;
  
  return feature_parent;
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

static void drawEachFeature(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature feature  = (ZMapFeature)data ; 
  processFeatureData d = (processFeatureData)user_data;

  zmapWindowFeatureDraw(d->window, d->column, feature);

  return ;
}

static void dnaHandleZoomCB(FooCanvasItem *container,
                            double zoom,
                            ZMapWindow user_data)
{
  ZMapWindow window = (ZMapWindow)user_data;
  FooCanvasGroup *container_features ;
  ZMapFeatureSet feature_set ;

  if(zmapWindowItemIsVisible(container)) /* No need to redraw if we're not visible! */
    {
      processFeatureDataStruct drawHandlerData = {NULL};

      feature_set = (ZMapFeatureSet)g_object_get_data(G_OBJECT(container), ITEM_FEATURE_DATA) ;
      /* Check it's actually a featureset */
      zMapAssert(feature_set && feature_set->struct_type == ZMAPFEATURE_STRUCT_FEATURESET) ;

      container_features = FOO_CANVAS_GROUP(zmapWindowContainerGetFeatures(FOO_CANVAS_GROUP(container))) ;

      zmapWindowContainerPurge(container_features) ;

      drawHandlerData.window = window;
      drawHandlerData.column = FOO_CANVAS_GROUP(container);

      g_datalist_foreach(&(feature_set->features), drawEachFeature, &drawHandlerData);

      zmapWindowContainerSetBackgroundSize(FOO_CANVAS_GROUP(container), 0.0);
    }
  
  return ;
}




/* Callback for destroy of items... */
static gboolean canvasItemDestroyCB(FooCanvasItem *item, gpointer data)
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

  /*  printf("canvasItemEventCB (%x): enter\n", item); */

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
  ZMapWindowItemHighlighter highlighObj = NULL;

  /*  printf("dnaItemEventCB (%x): enter... parent %x\n", item, item->parent);  */

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
      {
        GdkEventButton *button = (GdkEventButton *)event;

        if(event->type == GDK_BUTTON_RELEASE)
          {
#define DONTGRABPOINTER
#ifndef DONTGRABPOINTER
            foo_canvas_item_ungrab(item, button->time);
#endif 
            highlighObj = zmapWindowItemTextHighlightRetrieve(FOO_CANVAS_GROUP(item->parent));
            zMapAssert(highlighObj);

            zmapWindowItemTextHighlightFinish(highlighObj);
            event_handled = TRUE;
          }
        else if(button->button == 1)
          {
#ifndef DONTGRABPOINTER
            GdkCursor *xterm;
            xterm = gdk_cursor_new (GDK_XTERM);

            foo_canvas_item_grab(item, 
                                 GDK_POINTER_MOTION_MASK |  
                                 GDK_BUTTON_RELEASE_MASK | 
                                 GDK_BUTTON_MOTION_MASK,
                                 xterm,
                                 event->button.time);
            gdk_cursor_destroy(xterm);
#endif

            if((highlighObj = zmapWindowItemTextHighlightRetrieve(FOO_CANVAS_GROUP(item->parent)))
               && zmapWindowItemTextHighlightBegin(highlighObj, FOO_CANVAS_GROUP(item->parent), item))
              zmapWindowItemTextHighlightUpdateCoords(highlighObj, 
                                                      event->button.x, 
                                                      event->button.y);
            event_handled = TRUE;
          }
        else if(button->button == 3)
          {
            unsigned int firstIdx, lastIdx;
            highlighObj = zmapWindowItemTextHighlightRetrieve(FOO_CANVAS_GROUP(item->parent));
            if(zmapWindowItemTextHighlightGetIndices(highlighObj, &firstIdx, &lastIdx))
              {
                //printf("dnaItemEventCB (%x): dna from %d to %d\n", item, firstIdx, lastIdx);
                event_handled = TRUE;
              }
          }
      }
      break;
    case GDK_MOTION_NOTIFY:
      {
        ZMapDrawTextRowData trd = NULL;
        
        highlighObj = zmapWindowItemTextHighlightRetrieve(FOO_CANVAS_GROUP(item->parent));
        zMapAssert(highlighObj);
        
        if(zmapWindowItemTextHighlightValidForMotion(highlighObj)
           && (trd = zMapDrawGetTextItemData(item)))
          {
            zmapWindowItemTextHighlightUpdateCoords(highlighObj, 
                                                    event->motion.x,
                                                    event->motion.y);
            zmapWindowItemTextHighlightDraw(highlighObj, item);
            event_handled = TRUE;
          }    
      }
      break;
    case GDK_LEAVE_NOTIFY:
      {
	event_handled = FALSE ;
        break;
      }
    case GDK_ENTER_NOTIFY:
      {
	event_handled = FALSE ;
        break;
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


static void fuzzyTableDimensions(double region_range, double trunc_col, 
                                 double chars_per_base, int *row, int *col,
                                 double *height_inout, int *rcminusmod_out)
{
  double orig_height, final_height, fl_height, cl_height;
  double temp_rows, temp_cols, text_height, tmp_mod = 0.0;
  gboolean row_precedence = FALSE;

  region_range--;
  region_range *= chars_per_base;

  orig_height = text_height = *height_inout * chars_per_base;
  temp_rows   = region_range / orig_height     ;
  fl_height   = region_range / floor(temp_rows);
  cl_height   = region_range / ceil(temp_rows) ;

  if(((region_range / temp_rows) > trunc_col))
    row_precedence = TRUE;
  
  if(row_precedence)            /* dot dot dot */
    {
      /* printf("row_precdence\n"); */
      if(cl_height < fl_height && ((fl_height - cl_height) < (orig_height / 8)))
        final_height = cl_height;
      else
        final_height = fl_height;

      temp_rows = floor(region_range / final_height);
      temp_cols = floor(final_height);
    }
  else                          /* column_precedence */
    {
      double tmp_rws, tmp_len, sos;
      /* printf("col_precdence\n"); */

      if((sos = orig_height - 0.5) <= floor(orig_height) && sos > 0.0)
        temp_cols = final_height = floor(orig_height);
      else
        temp_cols = final_height = ceil(orig_height);

      /*
      printf(" we're going to miss %f bases at the end if using %f cols\n", 
             (tmp_mod = fmod(region_range, final_height)), final_height);
      */

      if(final_height != 0.0 && tmp_mod != 0.0)
        {
          tmp_len = region_range + (final_height - fmod(region_range, final_height));
          tmp_rws = tmp_len / final_height;
          
          final_height = (region_range + 1) / tmp_rws;
          temp_rows    = tmp_rws;

          /* 
          printf("so modifying... tmp_len=%f, tmp_rws=%f, final_height=%f\n", 
                 tmp_len, tmp_rws, final_height);
          */
        }
      else if(final_height != 0.0)
        temp_rows = floor(region_range / final_height);

    }

  /* sanity */
  if(temp_rows > region_range)
    temp_rows = region_range;

#ifdef RDS_DONT_INCLUDE
  printf("rows=%f, height=%f, range=%f, would have been min rows=%f & height=%f, or orignal rows=%f & height=%f\n", 
         temp_rows, final_height, region_range,
         region_range / fl_height, fl_height,
         region_range / orig_height, orig_height);
#endif

  if(height_inout)
    *height_inout = text_height = final_height / chars_per_base;
  if(row)
    *row = (int)temp_rows;
  if(col)
    *col = (int)temp_cols;
  if(rcminusmod_out)
    *rcminusmod_out = (int)(temp_rows * temp_cols - tmp_mod);

  return ;
}


static ZMapDrawTextIterator zmapDrawTextIteratorBuild(double feature_start, double feature_end,
                                                      double feature_offset,
                                                      double scroll_start,  double scroll_end,
                                                      double text_width,    double text_height, 
                                                      char  *full_text,     int    bases_per_char,
                                                      gboolean numbered,
                                                      PangoFontDescription *font)
{
  int tmp;
  double column_width = 300.0, chars_per_base, feature_first_char, feature_first_base;
  ZMapDrawTextIterator iterator   = g_new0(ZMapDrawTextIteratorStruct, 1);

  column_width  /= bases_per_char;
  chars_per_base = 1.0 / bases_per_char;

  iterator->seq_start      = feature_start;
  iterator->seq_end        = feature_end;
  iterator->offset_start   = floor(scroll_start - feature_offset);

  feature_first_base  = floor(scroll_start) - feature_start;
  tmp                 = (int)feature_first_base % (int)bases_per_char;
  feature_first_char  = (feature_first_base - tmp) / bases_per_char;

  /* iterator->offset_start += tmp; */ /* Keep this in step with the bit above */

  iterator->x            = 0.0;
  iterator->y            = 0.0;

  iterator->char_width   = text_width;

  iterator->truncate_at  = floor(column_width / text_width);

  iterator->char_height = text_height;

  fuzzyTableDimensions((ceil(scroll_end) - floor(scroll_start) + 1.0), 
                       (double)iterator->truncate_at, 
                       chars_per_base,
                       &(iterator->rows), 
                       &(iterator->cols), 
                       &(iterator->char_height),
                       &(iterator->lastPopulatedCell));

  /* Not certain this column calculation is correct (chars_per_base bit) */
  if(iterator->cols > iterator->truncate_at)
    {
      iterator->truncated = TRUE;
      /* Just make the cols smaller and the number of rows bigger */
      iterator->cols *= chars_per_base;
    }
  else
    {
      iterator->truncate_at  = iterator->cols;
    }


  iterator->row_text   = g_string_sized_new(iterator->truncate_at);
  iterator->wrap_text  = full_text;
  iterator->wrap_text += iterator->index_start = (int)feature_first_char;

  iterator->row_data   = g_new0(ZMapDrawTextRowDataStruct, iterator->rows);

  return iterator;
}


static void destroyIterator(ZMapDrawTextIterator iterator)
{
  zMapAssert(iterator);
  g_string_free(iterator->row_text, TRUE);
  iterator->row_data = NULL;
  g_free(iterator);

  return ;
}






/* Build the menu for a feature item. */
static void makeItemMenu(GdkEventButton *button_event, ZMapWindow window, FooCanvasItem *item)
{
  char *menu_title ;
  GList *menu_sets = NULL ;
  ItemMenuCBData menu_data ;
  ZMapFeature feature ;

  /* Some parts of the menu are feature type specific so retrieve the feature item info
   * from the canvas item. */
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;

  menu_title = zMapFeatureName((ZMapFeatureAny)feature) ;

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
	ZMapStrand strand ;

	strand = zmapWindowFeatureStrand(feature) ;

        list = zmapWindowFToIFindItemSetFull(menu_data->window->context_to_item, 
					     feature->parent->parent->parent->unique_id,
					     feature->parent->parent->unique_id,
					     feature->parent->unique_id,
					     zMapFeatureStrand2Str(strand),
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
      {"Show This Feature",           2, itemMenuCB, NULL},
      {"List All Column Features",      1, itemMenuCB, NULL},
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
  ZMapStrand strand ;

  strand = zmapWindowFeatureStrand(feature) ;

  if ((item = zmapWindowFToIFindItemFull(window->context_to_item, 
					 feature->parent->parent->parent->unique_id,
					 feature->parent->parent->unique_id,
					 feature->parent->unique_id,
					 strand,
					 feature->unique_id)))
    {
      zmapWindowEditorCreate(window, item) ;
      result = TRUE ;
    }

  return result ;
}
