/*  File: zmapWindowCanvasAlignment.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description:
 *
 * implements callback functions for FeaturesetItem alignemnt features
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <math.h>
#include <string.h>

#include <ZMap/zmapFeature.hpp>
#include <ZMap/zmapUtilsLog.hpp>
#include <zmapWindowCanvasDraw.hpp>
#include <zmapWindowCanvasFeature_I.hpp>
#include <zmapWindowCanvasAlignment_I.hpp>
#include <zmapWindowCanvasGlyph.hpp>




#define DEBUG_SPLICE 0


static void alignmentColumnInit(ZMapWindowFeaturesetItem featureset) ;
static void zMapWindowCanvasAlignmentPaintFeature(ZMapWindowFeaturesetItem featureset,
                                                  ZMapWindowCanvasFeature feature,
                                                  GdkDrawable *drawable, GdkEventExpose *expose) ;
static void zMapWindowCanvasAlignmentGetFeatureExtent(ZMapWindowCanvasFeature feature, ZMapSpan span, double *width) ;
static void zmapWindowCanvasAlignmentPreZoom(ZMapWindowFeaturesetItem featureset) ;
static void zMapWindowCanvasAlignmentZoomSet(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable) ;
static ZMapWindowCanvasFeature zMapWindowCanvasAlignmentAddFeature(ZMapWindowFeaturesetItem featureset,
   ZMapFeature feature, double y1, double y2) ;
static void zMapWindowCanvasAlignmentFreeSet(ZMapWindowFeaturesetItem featureset) ;
static ZMapFeatureSubPart zmapWindowCanvasAlignmentGetSubPart(FooCanvasItem *foo,
                                                              ZMapFeature feature, double x, double y) ;
static double alignmentPoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
                             double item_x, double item_y, int cx, int cy,
                             double local_x, double local_y, double x_off) ;

static gboolean hasNCSplices(ZMapFeature left, ZMapFeature right, gboolean *left_nc, gboolean *right_nc) ;

static ColinearityType getInterBlockColinearity(ZMapWindowFeaturesetItem featureset,
                                                ZMapWindowCanvasAlignment align, ZMapWindowCanvasAlignment next) ;
static ColinearityType getIntraBlockColinearity(ZMapFeature feature, ZMapAlignBlock block, ZMapAlignBlock next_block) ;

static void align_gap_free(AlignGap ag) ;
static AlignGap align_gap_alloc(void) ;
static AlignGap makeGapped(ZMapFeature feature, double offset, FooCanvasItem *foo,
                           int start, int end, GArray *align_gaps, gboolean is_forward) ;





/*
 *                   Globals
 */

static ZMapWindowCanvasGlyph truncation_glyph_alignment_start_G = NULL ;
static ZMapWindowCanvasGlyph truncation_glyph_alignment_end_G = NULL ;

static AlignGap align_gap_free_G = NULL;

static long n_block_alloc = 0;
static long n_gap_alloc = 0;
static long n_gap_free = 0;





/*
 *                      External routines (some are static but exposed via func. ptrs.)
 */


void zMapWindowCanvasAlignmentInit(void)
{
  gpointer funcs[FUNC_N_FUNC] = { NULL };
  gpointer feature_funcs[CANVAS_FEATURE_FUNC_N_FUNC] = { NULL };

  funcs[FUNC_SET_INIT] = (void *)alignmentColumnInit ;
  funcs[FUNC_PAINT]  = (void *)zMapWindowCanvasAlignmentPaintFeature;
  funcs[FUNC_PRE_ZOOM] = (void *)zmapWindowCanvasAlignmentPreZoom ;
  funcs[FUNC_ZOOM]   = (void *)zMapWindowCanvasAlignmentZoomSet;
  funcs[FUNC_FREE]   = (void *)zMapWindowCanvasAlignmentFreeSet;
  funcs[FUNC_ADD]    = (void *)zMapWindowCanvasAlignmentAddFeature;
  funcs[FUNC_POINT]   = (void *)alignmentPoint;

  zMapWindowCanvasFeatureSetSetFuncs(FEATURE_ALIGN, funcs, (size_t)0) ;


  feature_funcs[CANVAS_FEATURE_FUNC_EXTENT] = (void *)zMapWindowCanvasAlignmentGetFeatureExtent ;
  feature_funcs[CANVAS_FEATURE_FUNC_SUBPART] = (void *)zmapWindowCanvasAlignmentGetSubPart ;

  zMapWindowCanvasFeatureSetSize(FEATURE_ALIGN, feature_funcs, sizeof(zmapWindowCanvasAlignmentStruct)) ;

  return ;
}


/* Return all match blocks for the given alignment. Returns a GList containing all features
 * (ZMapFeature) that are linked to the given feature. */
GList* zMapWindowCanvasAlignmentGetAllMatchBlocks(FooCanvasItem *item)
{
  GList *result = NULL ;
  zMapReturnValIfFail(item, result) ;

  ZMapWindowFeaturesetItem featureset_item = (ZMapWindowFeaturesetItem)item ;
  ZMapWindowCanvasFeature canvas_feature = featureset_item->point_canvas_feature ;

  /* Find the first match block in the linked-list */
  while (canvas_feature && canvas_feature->left)
    canvas_feature = canvas_feature->left ;

  /* Now loop through the whole list and add them to the result */
  while (canvas_feature)
    {
      result = g_list_append(result, canvas_feature->feature) ;
      canvas_feature = canvas_feature->right ;
    }

  return result ;
}


static void zMapWindowCanvasAlignmentPaintFeature(ZMapWindowFeaturesetItem featureset,
                                                  ZMapWindowCanvasFeature feature,
                                                  GdkDrawable *drawable,
                                                  GdkEventExpose *expose)
{
  ZMapWindowCanvasAlignment align = (ZMapWindowCanvasAlignment)feature ;
  FooCanvasItem *foo = (FooCanvasItem *)featureset ;
  ZMapFeatureTypeStyle homology = NULL,
    nc_splice = NULL,
    style = NULL ;
  gulong ufill,outline;
  int cx1, cy1, cx2, cy2,
    colours_set, fill_set, outline_set ;
  gboolean truncated_start = FALSE,
    truncated_end = FALSE,
    ignore_truncation_glyphs = FALSE ;
  double mid_x = 0.0,
    x1 = 0.0,
    x2 = 0.0,
    x1_cache = 0.0,
    x2_cache = 0.0,
    y1_cache = 0.0,
    y2_cache = 0.0 ;


  zMapReturnIfFail(featureset && feature && feature->feature && drawable && expose) ;


  // Allocate colours for colinear lines if not allocated.
  if (!(featureset->colinear_colours))
    {
      featureset->colinear_colours = zMapCanvasDrawAllocColinearColours(gdk_gc_get_colormap(featureset->gc)) ;
    }


  /* Find featureset style. */
  style = *(feature->feature->style) ;

  /* if bumped we can have a very wide column: don't paint if not visible */
  /* display code assumes a narrow column w/ overlapping features and does not process x-coord */
  /* this makes a huge difference to display speed */

  /* Stuff this in here for now....needed in colinear lines.... */
  mid_x = featureset->dx + featureset->x_off + (featureset->width / 2);
  if (featureset->bumped)
    mid_x += feature->bump_offset;


  /* Find horizontal coordinates of the feature in world coordinates. */
  zMapWindowCanvasCalcHorizCoords(featureset, feature, &x1, &x2) ;

  /* Get some canvas coordinates. */
  foo_canvas_w2c(foo->canvas, x1, 0, &cx1, NULL);
  foo_canvas_w2c(foo->canvas, x2, 0, &cx2, NULL);


  /* ouch...can't we do this earlier.....rather than return here..... */
  if (cx2 < expose->area.x || cx1 > expose->area.x + expose->area.width)
    return;

  /*
   * Instantiate glyph objects.
   */
  if (truncation_glyph_alignment_start_G == NULL)
    {
      ZMapStyleGlyphShape start_shape, end_shape ;

      zMapWindowCanvasGlyphGetTruncationShapes(&start_shape, &end_shape) ;

      truncation_glyph_alignment_start_G = zMapWindowCanvasGlyphAlloc(start_shape, ZMAP_GLYPH_TRUNCATED_START,
                                                                      FALSE, TRUE) ;

      truncation_glyph_alignment_end_G = zMapWindowCanvasGlyphAlloc(end_shape, ZMAP_GLYPH_TRUNCATED_END,
                                                                    FALSE, TRUE) ;
    }



  /*
   * Find colours.
   */
  colours_set = zMapWindowCanvasFeaturesetGetColours(featureset, feature, &ufill, &outline);
  fill_set = colours_set & WINDOW_FOCUS_CACHE_FILL;
  outline_set = colours_set & WINDOW_FOCUS_CACHE_OUTLINE;

  if (fill_set && feature->feature->population)
    {

      if ((zMapStyleGetScoreMode(style) == ZMAPSCORE_HEAT) || (zMapStyleGetScoreMode(style) == ZMAPSCORE_HEAT_WIDTH))
        {
          ufill = (ufill << 8) | 0xff;/* convert back to RGBA */
          ufill = foo_canvas_get_color_pixel(foo->canvas,
                                            zMapWindowCanvasFeatureGetHeatColour(0xffffffff,ufill,feature->score));
        }

    }

  /*
   * If the feature is completely outside of the sequence-region then
   * we don't draw glyphs (otherwise we end up with a glyph on the edge of
   * the sequence-region, but no visible feature!).
   */
  if ((feature->feature->x2 < featureset->start)
      || (feature->feature->x1 > featureset->end)  )
    {
      ignore_truncation_glyphs = TRUE ;
    }



  /*
   * Store feature coordintes.
   */
  x1_cache = feature->feature->x1 ;
  x2_cache = feature->feature->x2 ;
  y1_cache = feature->y1 ;
  y2_cache = feature->y2 ;

  /*
   * Determine whether or not the feature needs to be truncated
   * at the start or end.
   */
  if (feature->feature->x1 < featureset->start)
    {
      truncated_start = TRUE ;
      feature->y1 = feature->feature->x1 = featureset->start ;
    }
  if (feature->feature->x2 > featureset->end)
    {
      truncated_end = TRUE ;
      feature->y2 = feature->feature->x2 = featureset->end ;
    }

  if (!ignore_truncation_glyphs)
    {
      double col_width ;

      col_width = zMapStyleGetWidth(featureset->style) ;

      if (truncated_start)
        {
          zMapWindowCanvasGlyphDrawGlyph(featureset, feature,
                                         style, truncation_glyph_alignment_start_G,
                                         drawable, col_width, 0.0) ;
        }
      if (truncated_end)
        {
          zMapWindowCanvasGlyphDrawGlyph(featureset, feature,
                                         style, truncation_glyph_alignment_end_G,
                                         drawable, col_width, 0.0) ;
        }

      if (truncated_start || truncated_end)
        featureset->draw_truncation_glyphs = TRUE ;

    }


  /*
   * Draw the alignment boxes.
   */
  if (!feature->feature->feature.homol.align
      || (!zMapStyleIsAlwaysGapped(*feature->feature->style)
          && (!featureset->bumped || !zMapStyleIsShowGaps(*feature->feature->style)))
      )
    {
      /*
       * draw a simple ungapped box
       * we don't draw gaps on reverse, annotators work on the fwd strand and revcomp if needs be
       * must use feature coords here as the first alignment in the series gets expanded to pick up colinear lines
       * if it's ungapped we'd draw a big box over the whole series
       */
      zMapCanvasFeaturesetDrawBoxMacro(featureset, x1, x2, feature->feature->x1, feature->feature->x2,
                                       drawable, fill_set, outline_set, ufill, outline) ;
    }
  else
    {
      /*
       * Note: we only enter this section on bump event with gapped item present in the featureset
       */

      /* Draw full gapped alignment boxes, colinear lines etc etc. */

      /*
       * create a list of things to draw at this zoom taking onto account bases per pixel
       *
       * Note that: The function makeGapped() returns data in canvas pixel coordinates
       * _relative_to_ the start of the feature, that is the quantity feature->feature->x1.
       * Hence the rather arcane looking arithmetic on coordinates in the section below
       * where these are modified for cases that are truncated.
       */
      if (!align->gapped)
        {
          ZMapHomol homol = &feature->feature->feature.homol;
          gboolean is_forward = (feature->feature->strand == homol->strand);

          if (homol->strand == ZMAPSTRAND_REVERSE)
            is_forward = !is_forward;

          align->gapped = zMapWindowCanvasAlignmentMakeGapped(featureset, feature,
                                                              feature->feature->x1,
                                                              feature->feature->x2,
                                                              homol->align, is_forward) ;
        }

      zMapCanvasDrawBoxGapped(drawable,
                              featureset->colinear_colours,
                              fill_set, outline_set, ufill, outline,
                              featureset, feature,
                              x1, x2, align->gapped) ;
    }


  /* Highlight all splice positions if they exist, do this bumped or unbumped otherwise user
   * has to bump column to see common splices. (See Splice_highlighting.html) */
  if (feature->splice_positions)
    {
      zMapCanvasFeaturesetDrawSpliceHighlights(featureset, feature, drawable, x1, x2) ;
    }


  /*
   * Reset to cached values before attempting anything else.
   */
  feature->feature->x1 = x1_cache ;
  feature->feature->x2 = x2_cache ;
  feature->y1 = y1_cache ;
  feature->y2 = y2_cache ;


  /*
   * draw decorations
   */
  if (featureset->bumped)
    {
      /* add some decorations */
      if (!align->bump_set)/* set up glyph data once only */
        {
          ZMapFeatureTypeStyle style = *feature->feature->style ;

          /* NOTE this code assumes the styles have been set up with:
           * 'glyph-strand=flip-y'
           * which will handle reverse strand indication */

          /* set the glyph pointers if applicable */
          homology = style->sub_style[ZMAPSTYLE_SUB_FEATURE_HOMOLOGY];
          nc_splice = style->sub_style[ZMAPSTYLE_SUB_FEATURE_NON_CONCENCUS_SPLICE];

          /* find and/or allocate glyph structs */
          if (!feature->left && homology)/* top end homology incomplete ? */
            {
              /* design by guesswork: this is not well documented */
              ZMapHomol homol = &feature->feature->feature.homol;
              double score = homol->y1 - 1;

              if (feature->feature->strand != homol->strand)
                score = homol->length - homol->y2;

              if (score)
                align->glyph5 = zMapWindowCanvasGetGlyph(featureset,
                                                         homology,
                                                         feature->feature->strand,
                                                         ZMAP_GLYPH_FIVEPRIME, score);
            }

          /* Draw any non-canonical splices markers, can only be done if we have the DNA. */
          if (feature->right && nc_splice && zMapFeatureDNAExists(feature->feature))
            {
              gboolean left_nc = FALSE, right_nc = FALSE ;

              if (hasNCSplices(feature->feature, feature->right->feature, &left_nc, &right_nc))
                {
                  ZMapWindowCanvasAlignment next = (ZMapWindowCanvasAlignment) feature->right ;

                  if (left_nc)
                    align->glyph3 = zMapWindowCanvasGetGlyph(featureset,
                                                             nc_splice,
                                                             feature->feature->strand,
                                                             ZMAP_GLYPH_THREEPRIME, 0.0) ;

                  if (right_nc)
                    next->glyph5 = zMapWindowCanvasGetGlyph(featureset,
                                                            nc_splice,
                                                            next->feature.feature->strand,
                                                            ZMAP_GLYPH_FIVEPRIME, 0.0) ;
                }
            }

          if (!feature->right && homology)/* bottom end homology incomplete ? */
            {
              /* design by guesswork: this is not well documented */
              ZMapHomol homol = &feature->feature->feature.homol;
              double score = homol->length - homol->y2;

              if (feature->feature->strand != homol->strand)
                score = homol->y1 - 1;

              if (score)
                align->glyph3 = zMapWindowCanvasGetGlyph(featureset,
                                                         homology, feature->feature->strand,
                                                         ZMAP_GLYPH_THREEPRIME, score);
            }

          align->bump_set = TRUE;
        }


      /* first feature: draw colinear lines */
      if (!feature->left && !zMapStyleIsUnique(*feature->feature->style))
        {
          ZMapWindowCanvasFeature feat;

          for (feat = feature ; feat->right ; feat = feat->right)
            {
              ZMapWindowCanvasAlignment next = (ZMapWindowCanvasAlignment) feat->right ;
              GdkColor *colour;

              /* get item canvas coords */
              /* note we have to use real feature coords here as we have mangled the first feature's y2 */
              foo_canvas_w2c(foo->canvas,
                             mid_x, feat->feature->x2 - featureset->start + featureset->dy + 1,
                             &cx1, &cy1);

              foo_canvas_w2c(foo->canvas,
                             mid_x, next->feature.y1 - featureset->start + featureset->dy,
                             &cx2, &cy2);

              if (cy2 < expose->area.y)
                continue;

              ColinearityType colinearity ;

              colinearity = getInterBlockColinearity(featureset,
                                                     (ZMapWindowCanvasAlignment)feat, (ZMapWindowCanvasAlignment)(feat->right)) ;

              colour = zMapCanvasDrawGetColinearGdkColor(ZMAP_WINDOW_FEATURESET_ITEM(featureset)->colinear_colours, colinearity) ;

              gdk_gc_set_foreground(featureset->gc, colour) ;

              /* draw line between boxes, don't overlap the pixels */

              /* The bit about clobbering the "first feature's y2" above seems to mean the first
               * line is drawn one pixel too long at the top. I hate doing this but I haven't
               * worked out what is going on  here yet !!! */
              if (!feature->left)
                cy1++ ;

              zMap_draw_line(drawable, featureset, cx1, cy1, cx2, cy2) ;

              if (cy2 > expose->area.y + expose->area.height)
                break;
            } /* for (feat = feature ; feat->right ; feat = feat->right) */
        }


      /* all features: add glyphs if present */
      if (align->glyph5)
        zMapWindowCanvasGlyphPaintSubFeature(featureset, feature, align->glyph5, drawable) ;
      if (align->glyph3)
        zMapWindowCanvasGlyphPaintSubFeature(featureset, feature, align->glyph3, drawable) ;
    }

  return ;
}




/* get sequence extent of compound alignment (for bumping) */
/* NB: canvas coords could overlap due to sub-pixel base pairs
 * which could give incorrect de-overlapping
 * that would be revealed on Zoom
 */
/*
 * we adjust the extent of the first CanvasAlignment to cover tham all
 * as the first one draws all the colinear lines
 */
static void zMapWindowCanvasAlignmentGetFeatureExtent(ZMapWindowCanvasFeature feature, ZMapSpan span, double *width)
{
  ZMapWindowCanvasFeature first = feature;

  zMapReturnIfFail(feature && feature->feature && width ) ;

  *width = feature->width;

  /* if not joining up same name features they don't need to go in the same column */
  if(!zMapStyleIsUnique(*feature->feature->style))
    {
      while(first->left)
        {
          first = first->left;
          if (first->width > *width)
            *width = first->width;
        }

      while(feature->right)
        {
          feature = feature->right;
          if (feature->width > *width)
            *width = feature->width;
        }

      first->y2 = feature->y2;
    }

  span->x1 = first->y1;
  span->x2 = first->y2;

  return ;
}


static void alignmentColumnInit(ZMapWindowFeaturesetItem featureset)
{
  zMapWindowCanvasGlyphSetColData(featureset) ;

  return ;
}


static void zmapWindowCanvasAlignmentPreZoom(ZMapWindowFeaturesetItem featureset)
{
  /* Need to call routine to trigger calculate on zoom for text here... */
  zMapWindowCanvasAlignmentZoomSet(featureset, NULL) ;

  return ;
}




/*
 * if we are displaying a gapped alignment, recalculate this data
 * do this by freeing the existing data, new stuff will be added by the paint function
 *
 * NOTE ref to FreeSet() below -> drawable may be NULL
 */
static void zMapWindowCanvasAlignmentZoomSet(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable)
{
  ZMapSkipList sl;

  zMapReturnIfFail(featureset) ;

  /* NOTE display index will be null on first call */

  /* feature specific eg bumped gapped alignments - adjust gaps display */
  for(sl = zMapSkipListFirst(featureset->display_index); sl; sl = sl->next)
    {
      ZMapWindowCanvasAlignment align = (ZMapWindowCanvasAlignment) sl->data;
      AlignGap ag, del;

      if (align->feature.type != FEATURE_ALIGN)/* we could have other types in the col due to mis-config */
        continue;

      /* delete the old, these get created on paint */
      for(ag = align->gapped; ag;)
        {
          del = ag;
          ag = ag->next;
          align_gap_free(del);
        }
      align->gapped = NULL;
    }

  //printf("alignment zoom: %ld %ld %ld\n",n_block_alloc, n_gap_alloc, n_gap_free);
}



static ZMapWindowCanvasFeature zMapWindowCanvasAlignmentAddFeature(ZMapWindowFeaturesetItem featureset,
                                                                   ZMapFeature feature, double y1, double y2)
{
  ZMapWindowCanvasFeature feat = NULL ;

  zMapReturnValIfFail(featureset && feature, feat) ;

  feat = zMapWindowFeaturesetAddFeature(featureset, feature, y1, y2);

  zMapWindowFeaturesetSetFeatureWidth(featureset, feat);

  if (feat->feature->feature.homol.flags.masked)
    feat->flags |= focus_group_mask[WINDOW_FOCUS_GROUP_MASKED];

#if MH17_DO_HIDE
  /* NOTE if we configure styles to not load these into the canvas we don't get here */
  /* however if the user selects 'Expand feature' then we do */
  /* so we can comment out this line and remove a showhide call from zmapWindowFeatureExpand()
   * _follow this link if you revert this code_
   */
  if (feature->flags.collapsed || feature->flags.squashed || feature->flags.joined)
    feat->flags |= FEATURE_HIDE_COMPOSITE | FEATURE_HIDDEN;
#endif

  /* NOTE we may not have an index so this flag must be unset seperately */
  /* eg on OTF w/ delete existing selected */
  if (featureset->link_sideways)
    featureset->linked_sideways = FALSE;

  return feat;
}


static void zMapWindowCanvasAlignmentFreeSet(ZMapWindowFeaturesetItem featureset)
{
  /* frees gapped data _and does not alloc any more_ */
  zMapWindowCanvasAlignmentZoomSet(featureset,NULL);
}



AlignGap zMapWindowCanvasAlignmentMakeGapped(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                                             int start, int end,
                                             GArray *align_gaps,
                                             gboolean is_forward)
{
  AlignGap gapped = NULL ;
  FooCanvasItem *foo = (FooCanvasItem *)featureset ;
  AlignGap ag ;
  int cy1, cy2 ;

  gapped = makeGapped(feature->feature, featureset->dy - featureset->start, foo,
                      start, end, align_gaps, is_forward) ;

  for (ag = gapped ; ag ; ag = ag->next )
    {
      /* treat start of box */
      ag->y1 = ag->y1 < 0 ? 0 : ag->y1 ;
      ag->y2 = ag->y2 < 0 ? 0 : ag->y2 ;

      /* treat end of box */
      foo_canvas_w2c(foo->canvas, 0, start, NULL, &cy1) ;
      foo_canvas_w2c(foo->canvas, 0, end + 1, NULL, &cy2) ;

      cy2 -= cy1 ;
      ag->y1 = ag->y1 > cy2 ? cy2 : ag->y1 ;
      ag->y2 = ag->y2 > cy2 ? cy2 : ag->y2 ;
    }

  return gapped ;
}


void zMapWindowCanvasAlignmentFreeGapped(AlignGap ag)
{
  align_gap_free(ag) ;

  return ;
}



ZMapFeatureSubPart zMapWindowCanvasGetGappedSubPart(GArray *aligns_array, ZMapStrand strand, double y)
{
  ZMapFeatureSubPart sub_part = NULL ;
  ZMapAlignBlock ab, prev_ab ;
  int match_num = 0, gap_num = 0 ;


  match_num = aligns_array->len ;
  gap_num = match_num - 1 ;
  prev_ab = NULL ;
  for (guint i = 0 ; !sub_part && i < aligns_array->len ; i++)
    {
      /* in the original foo based code
       * match n corresponds to the match block indexed from 1
       * gap n corresponds to the following match block
       */
      double start, end ;

      ab = &g_array_index(aligns_array, ZMapAlignBlockStruct, i) ;

      /* Matches.... */
      start = ab->t1 ;
      end = ab->t2 + 1 ;                                    /* full extent of gap is end + 1. */
      if (y >= start && y < end)
        {
          sub_part = (ZMapFeatureSubPart)g_malloc0(sizeof *sub_part) ;

          if (strand == ZMAPSTRAND_FORWARD)
            sub_part->index = i + 1 ;
          else
            sub_part->index = (match_num - i) ;

          sub_part->start = ab->t1 ;
          sub_part->end = ab->t2 ;
          sub_part->subpart = ZMAPFEATURE_SUBPART_MATCH ;

          break ;
        }

      /* Gaps.... */
      if (prev_ab)
        {
          start = prev_ab->t2 + 1 ;
          end = ab->t1 ;

          if (y >= start && y < end)
            {
              sub_part = (ZMapFeatureSubPart)g_malloc0(sizeof *sub_part) ;

              if (strand == ZMAPSTRAND_FORWARD)
                sub_part->index = i ;
              else
                sub_part->index = (gap_num - i) + 1 ;

              sub_part->start = start ;
              sub_part->end = end - 1 ;
              sub_part->subpart = ZMAPFEATURE_SUBPART_GAP ;
            }
        }

      prev_ab = ab ;
    }


  return sub_part ;
}




/* Returns a newly-allocated ZMapFeatureSubPartSpan, which the caller must free with g_free,
 * or NULL. */
ZMapFeatureSubPart zmapWindowCanvasAlignmentGetSubPart(FooCanvasItem *foo,
                                                       ZMapFeature feature, double x, double y)
{
  ZMapFeatureSubPart sub_part = NULL ;
  ZMapWindowFeaturesetItem fi = NULL ;

  zMapReturnValIfFail(foo && feature, NULL) ;

  fi = (ZMapWindowFeaturesetItem)foo ;

  if (!fi->bumped)
    {
      // gaps/matches only displayed if bumped so only return them if bumped.

      sub_part = NULL ;
    }
  else  if (!feature->feature.homol.align)
    {
      // Ungapped match so nothing to return.
     
      sub_part = NULL ;
    }
  else
    {
      /* get sequence coords for x,y,  well y at least */
      /* AFAICS y is a world coordinate as the caller runs it through foo_w2c() */

      /* we refer to the actual feature gaps data not the display data
       * as that may be compressed and does not contain sequence info
       * return the type index and target start and end
       */
      sub_part = zMapWindowCanvasGetGappedSubPart(feature->feature.homol.align, feature->strand, y) ;
    }

  return sub_part ;
}


/* Default function to check if the given x,y coord is within a feature, this
 * function assumes the feature is box-like. */
static double alignmentPoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
                             double item_x, double item_y, int cx, int cy,
                             double local_x, double local_y, double x_off)
{
  double best = 1.0e36 ;
  double can_start = 0.0, can_end = 0.0;

  zMapReturnValIfFail(gs && gs->feature, best ) ;

  /* Get feature extent on display. */
  /* NOTE cannot use feature coords as transcript exons all point to the same feature */
  /* alignments have to implement a special function to handle bumped features
   *  - the first exon gets expanded to cover the whole */
  /* when we get upgraded to vulgar strings these can be like transcripts...
   * except that there's a performance problem due to volume */
  /* perhaps better to add  extra display/ search coords to ZMapWindowCancasFeature ?? */


  can_start = gs->feature->x1 ;
  can_end = gs->feature->x2 ;
  zmapWindowFeaturesetS2Ccoords(&can_start, &can_end) ;


  if (can_start <= local_y && can_end >= local_y)    /* overlaps cursor */
    {
      double wx ;
      double left, right ;

      wx = x_off; // - (gs->width / 2) ;

      if (fi->bumped)
        wx += gs->bump_offset ;

      /* get coords within one pixel */
      left = wx - 1 ;    /* X coords are on fixed zoom, allow one pixel grace */
      right = wx + gs->width + 1 ;

      if (local_x > left && local_x < right)    /* item contains cursor */
        {
          best = 0.0;
        }
    }

  return best ;
}






/*
 *                            Internal routines
 */



/* Function returns TRUE if either the 3' splice of feature left or the 5' splice of
 * feature right are non-canonical. Which splices are non-canonical are returned in
 * left_nc and right_nc. Returns FALSE if both splices are canonical.
 *
 * These are the canonical splices:
 *
 *        Exon|Intron..................Intron|Exon
 *            |gt                          ag|
 *           g|gc
 *
 *
 * These can be configured in/out via styles:
 * sub-features=non-concensus-splice:nc-splice-glyph
 *
 * Notes
 *
 * We need to test splices according to the strand of the feature, for reverse
 * strand rather than reverse the whole dna string we just reverse the string we
 * test with. The strings are:
 *
 *  five_prime_splice == test one base from 3' end of exon and next 2 bases from intron
 * three_prime_splice == test 2 bases from intron before next exon
 *
 * We always get three bases for each so we can test in either direction.
 *
 * Code assumes the two match features are on same strand.....
 *
 *  */
static gboolean hasNCSplices(ZMapFeature left, ZMapFeature right, gboolean *left_nc, gboolean *right_nc)
{
  gboolean result = FALSE ;
  char *ldna = NULL, *rdna = NULL;
  gboolean revcomp = FALSE ;

  zMapReturnValIfFail(left_nc && right_nc, result);


  /* OH DEAR...THIS SHOULD NOT HAPPEN...SORTING FOR BUMP SHOULD STOP THIS...edgrif */
  /* MH17 NOTE
   *
   * for reverse strand we need to splice backwards
   * logically we could have a mixed series
   * we do get duplicate series and these can be reversed
   * we assume any reversal is a series break
   * and do not attempt to splice inverted exons
   */

#if DEBUG_SPLICE
  zMapLogWarning("splice %s",g_quark_to_string(left->unique_id));
#endif

  // 3' end of exon: get 1 base of last exon  + 2 from intron
  ldna = zMapFeatureGetDNA((ZMapFeatureAny)left, left->x2, left->x2 + 2, revcomp) ;

  // 5' end of exon: get 2 bases from intron + 1 base of next exon
  rdna = zMapFeatureGetDNA((ZMapFeatureAny)right, right->x1 - 2, right->x1, revcomp) ;

  if (ldna && rdna)
    {
      char *five_prime_splice, *three_prime_splice ;

      *left_nc = *right_nc = FALSE ;

      if (left->strand == ZMAPSTRAND_FORWARD)
        {
          five_prime_splice = ldna ;
          three_prime_splice = rdna ;

          if (g_ascii_strncasecmp(five_prime_splice + 1, "GT", 2) != 0
              && g_ascii_strncasecmp(five_prime_splice, "GGC", 3) != 0)
            *left_nc = TRUE ;

          if (g_ascii_strncasecmp(three_prime_splice, "AG", 2) != 0)
            *right_nc = TRUE ;
        }
      else
        {
          five_prime_splice = rdna ;
          three_prime_splice = ldna ;

          if (g_ascii_strncasecmp(five_prime_splice, "AC", 2) == 0
              || g_ascii_strncasecmp(five_prime_splice, "GCC", 3) == 0)
            *right_nc = TRUE ;

          if (g_ascii_strncasecmp(three_prime_splice + 1, "CT", 2) == 0)
            *left_nc = TRUE ;
        }

      if (*left_nc || *right_nc)
        result = TRUE ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
        {
          char *base_str ;

          base_str = g_strdup_printf("Splice is %s...%s\t%c|%c%c...%c%c|%c",
                                     (*left_nc ? "Non-Canon" : "Canon    "),
                                     (*right_nc ? "Non-Canon" : "Canon    "),
                                     *ldna, *(ldna + 1), *(ldna + 2),
                                     *rdna, *(rdna + 1), *(rdna + 2)) ;

          zMapDebugPrintf("%s", base_str) ;
          zMapLogMessage("%s", base_str) ;

          g_free(base_str) ;
        }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  if (ldna)
    g_free(ldna) ;
  if (rdna)
    g_free(rdna) ;

  return result ;
}







/* simple list structure, avioding extra malloc/free associated with GList code */
static AlignGap align_gap_alloc(void)
{
  int i;
  AlignGap ag = NULL;

  if (!align_gap_free_G)
    {
      gpointer mem = g_malloc(sizeof(AlignGapStruct) * N_ALIGN_GAP_ALLOC);
      for(i = 0; i < N_ALIGN_GAP_ALLOC; i++)
        {
          align_gap_free(((AlignGap) mem) + i);
        }

      n_block_alloc++;
    }

  if (align_gap_free_G)
    {
      ag = (AlignGap) align_gap_free_G;
      align_gap_free_G = align_gap_free_G->next;
      memset((gpointer) ag, 0, sizeof(AlignGapStruct));

      n_gap_alloc++;
    }

  return ag;
}

static void align_gap_free(AlignGap ag)
{
  ag->next = align_gap_free_G;
  align_gap_free_G = ag;

  n_gap_free++;
}





/* NOTE the gaps array is not explicitly sorted, but is only provided by ACEDB and subsequent pipe
 * scripts written by otterlace and it is believed that the match blocks are always provided in
 * start coordinate order (says Ed)
 *
 * So it's a bit slack trusting an external program but ZMap has been doing that for a long time.
 * "roll on CIGAR strings" which prevent this kind of problem
 *
 * EG - Actually acedb arrays are always sorted and now most of our data is CIGAR-derived so we should
 * be ok.
 * 
 * 
 * NOTE that sorting a GArray might sort the data structures themselves, so schoolboy error kind
 * of slow.  If they do need to be sorted then the place is in zmapGFF2Parser.c/loadGaps() Sorting
 * is interesting here as the display optimisation would produce a completely wrong picture if it
 * was not done
 *
 * we draw boxes etc at the target coordinates ie on the genomic sequence
 *
 * NOTE for short reads we have an option to squash those that have the same gap (they only have
 * one gap ,except for a few pathological cases) in this case the first and last blocks are a diff
 * colour, so we flag this if the colour is visible and add another box not a line. Yuk
 *
 */
static AlignGap makeGapped(ZMapFeature feature, double offset, FooCanvasItem *foo,
                           int start, int end,
                           GArray *align_gaps, gboolean is_forward)
{
  AlignGap display_ag = NULL ;
  int fy1 ;
  GArray *gaps ;
  int n ;
  int i ;
  ZMapAlignBlock last_ab ;
  AlignGap last_box, last_ag ;

  zMapReturnValIfFail(foo && feature, NULL) ;

  foo_canvas_w2c(foo->canvas, 0, start + offset, NULL, &fy1);

  gaps = align_gaps ;
  n = align_gaps->len ;

  for (i = 0, last_ab = NULL, last_box = NULL, last_ag = NULL ; i < n ; i++)
    {
      AlignGap ag = NULL ;
      ZMapAlignBlock ab ;
      int cy1, cy2 ;
      gboolean edge ;

      ab = &g_array_index(gaps, ZMapAlignBlockStruct, i) ;

      /* get pixel coords of block relative to feature y1, +1 to cover all of last base on cy2 */
      foo_canvas_w2c(foo->canvas, 0, ab->t1 + offset, NULL, &cy1);
      foo_canvas_w2c(foo->canvas, 0, ab->t2 + 1 + offset, NULL, &cy2);
      cy1 -= fy1;
      cy2 -= fy1;


      if (last_box)
        {
          if (i == 1 && (feature->flags.squashed_start))
            {
              last_box->edge = TRUE;

              if (last_box->y2 - last_box->y1 > 2)
                last_box = NULL ;                           /* force a new box if the colours are visible */
            }

          edge = FALSE;

          if (i == (n-1) && (feature->flags.squashed_end))
            {
              if (last_box && last_box->y2 - last_box->y1 > 2)
                last_box = NULL ;                           /* force a new box if the colours are visible */

              edge = TRUE;
            }
        }

      if (last_box)
        {
          if (last_box->y2 == cy1 && cy2 != cy1)
            {
              /* extend last box and add a line where last box ended */
              ag = align_gap_alloc();
              last_ag->next = ag;

              ag->y1 = last_box->y2;
              ag->y2 = last_box->y2;
              ag->type = GAP_HLINE;

              last_ag = ag;
              last_box->y2 = cy2;
            }
          else if (last_box->y2 < cy1 - 1)
            {
              /* visible gap between boxes: add a colinear line */
              AlignBlockBoundaryType boundary_type = (is_forward ? ab->start_boundary : ab->end_boundary) ;

              ag = align_gap_alloc() ;
              last_ag->next = ag ;

              ag->y1 = last_box->y2 ;
              ag->y2 = cy1 ;

              ag->type = (boundary_type == ALIGN_BLOCK_BOUNDARY_INTRON ? GAP_VLINE_INTRON : GAP_VLINE) ;

              if (ag->type == GAP_VLINE_INTRON)
                {
                  ag->colinearity = getIntraBlockColinearity(feature, last_ab, ab) ;
                }

              last_ag = ag ;
            }

          if (last_box->y2 < cy1)
            {
              /* create new box if edges do not overlap */
              last_box = NULL;
            }
        }

      if (!last_box)
        {
          /* add a new box */
          ag = align_gap_alloc();
          ag->y1 = cy1;
          ag->y2 = cy2;
          ag->type = GAP_BOX;
          ag->edge = edge;

          if (last_ag)
            last_ag->next = ag;

          last_box = last_ag = ag;

          if (!display_ag)
            display_ag = ag;
        }

      last_ab = ab ;
    }

  return display_ag ;
}



// Where an alignment has multiple separate matches to a sequence this function will calculate the
// colinearity of adjacent matches given the align and next parameters.
//
// Note that when match is from opposite strand of homol then homol matches are in reversed order
// but coords are still _forwards_. Revcomping reverses order of homol matches but as before
// coords are still forwards.
//
static ColinearityType getInterBlockColinearity(ZMapWindowFeaturesetItem featureset,
                                                ZMapWindowCanvasAlignment align, ZMapWindowCanvasAlignment next)
{
  ColinearityType ct = COLINEAR_INVALID ;
  int start2 = 0, end1 = 0;
  int threshold = 0 ;
  ZMapHomol h1,h2;

  /* apparently this works thro revcomp:
   *
   * "When match is from reverse strand of homol then homol blocks are in reversed order
   * but coords are still _forwards_. Revcomping reverses order of homol blocks
   * but as before coords are still forwards."
   */
  if (align->feature.feature->strand == align->feature.feature->feature.homol.strand)
    {
      h1 = &align->feature.feature->feature.homol;
      h2 = &next->feature.feature->feature.homol;
    }
  else
    {
      h2 = &align->feature.feature->feature.homol;
      h1 = &next->feature.feature->feature.homol;
    }

  end1 = h1->y2;
  start2 = h2->y1;

  threshold = (int)zMapStyleGetWithinAlignError(*align->feature.feature->style) ;

  ct = zMapCanvasDrawGetColinearity(end1, start2, threshold) ;

  return ct ;
}


// Where an alignment has internal gaps this function will calculate the colinearity of adjacent
// blocks given the the block and next_block parameters.
//
// Note that when match is from opposite strand of homol then align blocks are in reversed order
// but coords are still _forwards_. Revcomping reverses order of align blocks but as before
// coords are still forwards.
//
static ColinearityType getIntraBlockColinearity(ZMapFeature feature, ZMapAlignBlock block, ZMapAlignBlock next_block)
{
  ColinearityType ct = COLINEAR_INVALID ;
  bool align_same_strand ;
  int threshold ;
  int prev_end, curr_start ;

  if (feature->strand == feature->feature.homol.strand)
    align_same_strand = true ;
  else
    align_same_strand = false ;

  threshold = (int)zMapStyleGetWithinAlignError(*(feature->style)) ;

  if (align_same_strand)
    {
      prev_end = block->q2 ;
      curr_start = next_block->q1 ;
    }
  else
    {
      prev_end = next_block->q2 ;
      curr_start = block->q1 ;
    }
                  
  ct = zMapCanvasDrawGetColinearity(prev_end, curr_start, threshold) ;

  return ct ;
}
