/*  File: zmapWindowCanvasAlignment.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2014: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description:
 *
 * implements callback functions for FeaturesetItem alignemnt features
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <string.h>

#include <ZMap/zmapFeature.h>
#include <ZMap/zmapUtilsLog.h>
#include <zmapWindowCanvasDraw.h>
#include <zmapWindowCanvasAlignment_I.h>
#include <zmapWindowCanvasGlyph_I.h>
#include <zmapWindowCanvasGlyph.h>

#define INCLUDE_TRUNCATION_GLYPHS_ALIGNMENT


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
static ZMapFeatureSubPartSpan zmapWindowCanvasAlignmentGetSubPartSpan(FooCanvasItem *foo,
								      ZMapFeature feature, double x, double y) ;
static double alignmentPoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
			     double item_x, double item_y, int cx, int cy,
			     double local_x, double local_y, double x_off) ;


static gboolean fragments_splice(char *fragment_a, char *fragment_b) ;
static gboolean is_nc_splice(ZMapFeature left,ZMapFeature right) ;
static void align_gap_free(AlignGap ag) ;
static AlignGap align_gap_alloc(void) ;
static AlignGap makeGapped(ZMapFeature feature, double offset, FooCanvasItem *foo, gboolean forward) ;



/*
 * Glyphs to represent the trunction of a feature at the ZMap boundary. One for
 * truncation at the start and one for truncation at the end.
 */

static ZMapStyleGlyphShapeStruct truncation_shape_alignment_instance_start =
{
  {
    0, 0,      5, -5,        0, -10,       -5, -5,      0, 0,          0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  },                                                                               /* length 32 coordinate array */
  5,                                                                               /* number of coordinates */
  10, 10,                                                                          /* width and height */
  0,                                                                               /* quark ID */
  GLYPH_DRAW_LINES                                                                 /* ZMapStyleGlyphDrawType; LINES == OUTLINE, POLYGON == filled */
}  ;

static ZMapStyleGlyphShapeStruct truncation_shape_alignment_instance_end =
{
  {
    0, 0,      -5, 5,        0, 10,       5, 5,      0, 0,          0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  },
  5,
  10, 10,
  0,
  GLYPH_DRAW_LINES
}  ;

static ZMapStyleGlyphShapeStruct * truncation_shape_alignment_start = &truncation_shape_alignment_instance_start ;
static ZMapStyleGlyphShapeStruct * truncation_shape_alignment_end = &truncation_shape_alignment_instance_end ;
static ZMapWindowCanvasGlyph truncation_glyph_alignment_start = NULL ;
static ZMapWindowCanvasGlyph truncation_glyph_alignment_end = NULL ;




/*
 *                   Globals
 */



/* optimise setting of colours, thes have to be GdkParsed and mapped to the canvas */
/* we has a flag to set these on the first draw operation which requires map and relaise of the
   canvas to have occured */
gboolean decoration_set_G = FALSE;
GdkColor colinear_gdk_G[COLINEARITY_N_TYPE];


GHashTable *align_glyph_G = NULL;


static AlignGap align_gap_free_G = NULL;

static long n_block_alloc = 0;
static long n_gap_alloc = 0;
static long n_gap_free = 0;





/*
 *                      External routines (static by exposed via func. ptrs.)
 */



void zMapWindowCanvasAlignmentInit(void)
{
  gpointer funcs[FUNC_N_FUNC] = { NULL };

  funcs[FUNC_SET_INIT] = alignmentColumnInit ;
  funcs[FUNC_PAINT]  = zMapWindowCanvasAlignmentPaintFeature;
  funcs[FUNC_EXTENT] = zMapWindowCanvasAlignmentGetFeatureExtent;
  funcs[FUNC_PRE_ZOOM] = zmapWindowCanvasAlignmentPreZoom ;
  funcs[FUNC_ZOOM]   = zMapWindowCanvasAlignmentZoomSet;
  funcs[FUNC_FREE]   = zMapWindowCanvasAlignmentFreeSet;
  funcs[FUNC_ADD]    = zMapWindowCanvasAlignmentAddFeature;
  funcs[FUNC_SUBPART] = zmapWindowCanvasAlignmentGetSubPartSpan;
  funcs[FUNC_POINT]   = alignmentPoint;

  zMapWindowCanvasFeatureSetSetFuncs(FEATURE_ALIGN, funcs, sizeof(zmapWindowCanvasAlignmentStruct), 0);

  return ;
}




/* given an alignment sub-feature return the colour or the colinearity line to the next sub-feature */
static GdkColor *zmapWindowCanvasAlignmentGetFwdColinearColour(ZMapWindowCanvasAlignment align)
{
  ZMapWindowCanvasAlignment next = (ZMapWindowCanvasAlignment) align->feature.right;
  int diff;
  int start2,end1;
  int threshold = (int) zMapStyleGetWithinAlignError(*align->feature.feature->style);
  ColinearityType ct = COLINEAR_PERFECT;
  ZMapHomol h1,h2;

  /* apparently this works thro revcomp:
   *
   * "When match is from reverse strand of homol then homol blocks are in reversed order
   * but coords are still _forwards_. Revcomping reverses order of homol blocks
   * but as before coords are still forwards."
   */
  if(align->feature.feature->strand == align->feature.feature->feature.homol.strand)
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
  diff = start2 - end1 - 1;
  if(diff < 0)
    diff = -diff;

  if(diff > threshold)
    {
      if(start2 < end1)
	ct = COLINEAR_NOT;
      else
	ct = COLINEAR_IMPERFECT;
    }

  return &colinear_gdk_G[ct];
}






static void zMapWindowCanvasAlignmentPaintFeature(ZMapWindowFeaturesetItem featureset,
                                                  ZMapWindowCanvasFeature feature,
                                                  GdkDrawable *drawable,
                                                  GdkEventExpose *expose)
{
  static const char *colours[] = { "black", "red", "orange", "green" };
  ZMapWindowCanvasAlignment align = (ZMapWindowCanvasAlignment)feature;
  FooCanvasItem *foo = (FooCanvasItem *) featureset;
  ZMapFeatureTypeStyle homology = NULL,
    nc_splice = NULL,
    style = NULL ;
  ZMapStyleScoreMode style_score_mode = ZMAPSCORE_INVALID ;
  gulong fill,outline;
  int cx1, cy1, cx2, cy2,
    colours_set, fill_set, outline_set,
    gy1, gy2, gx ;
  gulong edge;
  gboolean truncated_start = FALSE,
    truncated_end = FALSE,
    ignore_truncation_glyphs = FALSE ;
  double mid_x = 0.0,
    x1 = 0.0,
    x2 = 0.0,
    y1 = 0.0,
    y2 = 0.0,
    x1_cache = 0.0,
    x2_cache = 0.0,
    y1_cache = 0.0,
    y2_cache = 0.0,
    col_width = 0.0 ;

  /*
   * Error test.
   */
  zMapReturnIfFail(featureset && feature && drawable && expose ) ;

  /*
   * Find featureset style.
   */
  style = *feature->feature->style;

  /* if bumped we can have a very wide column: don't paint if not visible */
  /* display code assumes a narrow column w/ overlapping features and does not process x-coord */
  /* this makes a huge difference to display speed */

  /* Stuff this in here for now....needed in colinear lines.... */
  mid_x = featureset->dx + featureset->x_off + (featureset->width / 2);
  if(featureset->bumped)
    mid_x += feature->bump_offset;


  /*
   * Find horizontal coordinates of the feature in world coordinates.
   */
  zMapWindowCanvasCalcHorizCoords(featureset, feature, &x1, &x2) ;

  /*
   * Get some canvas coordinates.
   */
  foo_canvas_w2c (foo->canvas, x1, 0, &cx1, NULL);
  foo_canvas_w2c (foo->canvas, x2, 0, &cx2, NULL);

  if (cx2 < expose->area.x || cx1 > expose->area.x + expose->area.width)
    return;

#ifdef INCLUDE_TRUNCATION_GLYPHS_ALIGNMENT
  /*
   * Instantiate glyph objects.
   */
  if (truncation_glyph_alignment_start == NULL)
    {
      truncation_glyph_alignment_start = g_new0(zmapWindowCanvasGlyphStruct, 1) ;
      truncation_glyph_alignment_start->sub_feature = TRUE ;
      truncation_glyph_alignment_start->shape = truncation_shape_alignment_start ;
      truncation_glyph_alignment_start->which = ZMAP_GLYPH_TRUNCATED_START ;
    }
  if (truncation_glyph_alignment_end == NULL)
    {
      truncation_glyph_alignment_end = g_new0(zmapWindowCanvasGlyphStruct, 1) ;
      truncation_glyph_alignment_end->sub_feature = TRUE ;
      truncation_glyph_alignment_end->shape = truncation_shape_alignment_end ;
      truncation_glyph_alignment_end->which = ZMAP_GLYPH_TRUNCATED_END ;
    }
  col_width = zMapStyleGetWidth(featureset->style) ;
#endif


  /*
   * Find colours.
   */
  colours_set = zMapWindowCanvasFeaturesetGetColours(featureset, feature, &fill, &outline);
  fill_set = colours_set & WINDOW_FOCUS_CACHE_FILL;
  outline_set = colours_set & WINDOW_FOCUS_CACHE_OUTLINE;

  if (fill_set && feature->feature->population)
    {

      if ((zMapStyleGetScoreMode(style) == ZMAPSCORE_HEAT) || (zMapStyleGetScoreMode(style) == ZMAPSCORE_HEAT_WIDTH))
        {
          fill = (fill << 8) | 0xff;	/* convert back to RGBA */
          fill = foo_canvas_get_color_pixel(foo->canvas,
          zMapWindowCanvasFeatureGetHeatColour(0xffffffff,fill,feature->score));
        }

      if (zMapStyleIsSquash(style))		/* diff colours for first and last box */
        {
          edge = 0x808080ff;
        }
    }

    /*
     * If the feature is completely outside of the sequence-region then
     * we don't draw glyphs (otherwise we end up with a glyph on the edge of
     * the sequence-region, but no visible feature!).
     */
    if (    (feature->feature->x2 < featureset->start)
        ||  (feature->feature->x1 > featureset->end)  )
      {
        ignore_truncation_glyphs = TRUE ;
      }


#ifdef INCLUDE_TRUNCATION_GLYPHS_ALIGNMENT
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
        if (truncated_start)
          {
            zMapWindowCanvasGlyphDrawTruncationGlyph(foo, featureset, feature,
                                                     style, truncation_glyph_alignment_start,
                                                     drawable, col_width) ;
          }
        if (truncated_end)
          {
            zMapWindowCanvasGlyphDrawTruncationGlyph(foo, featureset, feature,
                                                     style, truncation_glyph_alignment_end,
                                                     drawable, col_width) ;
          }

        if (truncated_start || truncated_end)
          featureset->draw_truncation_glyphs = TRUE ;

      }
#endif

  /*
   * Draw the alignment boxes.
   */
  if (   !feature->feature->feature.homol.align
      || (!zMapStyleIsAlwaysGapped(*feature->feature->style) && (!featureset->bumped || !zMapStyleIsShowGaps(*feature->feature->style))   )
     )
    {

      /*
       * draw a simple ungapped box
       * we don't draw gaps on reverse, annotators work on the fwd strand and revcomp if needs be
       * must use feature coords here as the first alignment in the series gets expanded to pick up colinear lines
       * if it's ungapped we'd draw a big box over the whole series
       */
      zMapCanvasFeaturesetDrawBoxMacro(featureset, x1, x2, feature->feature->x1, feature->feature->x2,
                                       drawable, fill_set, outline_set, fill, outline) ;


    }
  else
    {

      /*
       * Note: we only enter this section on bump event with gapped item present in the featureset
       */

      /* Draw full gapped alignment boxes, colinear lines etc etc. */
      AlignGap ag;
      GdkColor c;


      /*
       * create a list of things to draw at this zoom taking onto account bases per pixel
       *
       * Note that: The function makeGapped() returns data in canvas pixel coordinates
       * _relative_to_ the start of the feature, that is the quantity feature->feature->x1.
       * Hence the rather arcane looking arithmetic on coordinates in the section below
       * where these are modified for cases that are truncated.
       */
      if(!align->gapped)
        {
          ZMapHomol homol = &feature->feature->feature.homol;
          gboolean forward = (feature->feature->strand == homol->strand);

          if (homol->strand == ZMAPSTRAND_REVERSE)
            forward = !forward;

          align->gapped = makeGapped(feature->feature, featureset->dy - featureset->start, foo, forward);

#ifdef INCLUDE_TRUNCATION_GLYPHS_ALIGNMENT
          for (ag = align->gapped ; ag ; ag = ag->next )
            {
              /* treat start of box */
              ag->y1 = ag->y1 < 0 ? 0 : ag->y1 ;
              ag->y2 = ag->y2 < 0 ? 0 : ag->y2 ;

              /* treat end of box */
              foo_canvas_w2c(foo->canvas, 0, feature->feature->x1, NULL, &cy1) ;
              foo_canvas_w2c(foo->canvas, 0, feature->feature->x2, NULL, &cy2) ;
              cy2 -= cy1 ;
              ag->y1 = ag->y1 > cy2 ? cy2 : ag->y1 ;
              ag->y2 = ag->y2 > cy2 ? cy2 : ag->y2 ;
            }
#endif
        }

      /* draw them */

      /* get item canvas coords.  gaps data is relative to feature y1 in pixel coordinates */
      foo_canvas_w2c(foo->canvas, x1, feature->feature->x1 - featureset->start + featureset->dy, &cx1, &cy1) ;
      foo_canvas_w2c(foo->canvas, x2, 0, &cx2, NULL) ;
      cy2 = cy1 ;

      for (ag = align->gapped ; ag ; ag = ag->next)
        {

          gy1 = cy1 + ag->y1;
          gy2 = cy1 + ag->y2;

          switch(ag->type)
            {
              case GAP_BOX:
                {

                  /* Can't use generalised draw call here because these are already canvas coords. */
                  if (fill_set && (!outline_set || (gy2 - gy1 > 1)))	/* fill will be visible */
                    {
                      c.pixel = fill;
                      gdk_gc_set_foreground(featureset->gc, &c) ;
                      zMap_draw_rect(drawable, featureset, cx1, gy1, cx2, gy2, TRUE) ;
                    }

                  if (outline_set)
                    {
                      c.pixel = outline;
                      gdk_gc_set_foreground (featureset->gc, &c);
                      zMap_draw_rect(drawable, featureset, cx1, gy1, cx2, gy2, FALSE);
                    }

                  break;
                }

              case GAP_HLINE:		/* x coords differ, y is the same */
                {
                  if (!outline_set)	/* must be or else we are up the creek! */
                    break;

                  c.pixel = outline;
                  gdk_gc_set_foreground (featureset->gc, &c);
                  zMap_draw_line(drawable, featureset, cx1, gy1, cx2 - 1, gy2);	/* GDK foible */
                  break;
                }
              case GAP_VLINE_INTRON:      /* fall through */
              case GAP_VLINE:		/* y coords differ, x is the same */
                if(!outline_set)	/* must be or else we are up the creek! */
                  break;

                gx = (cx1 + cx2) / 2;
                c.pixel = outline;
                gdk_gc_set_foreground (featureset->gc, &c);

                if (ag->type == GAP_VLINE_INTRON)
                  zMap_draw_broken_line(drawable, featureset, gx, gy1, gx, gy2);
                else
                  zMap_draw_line(drawable, featureset, gx, gy1, gx, gy2);
                break;

            } /* switch statement */

        } /* for (ag = align->gapped ; ag ; ag = ag->next) */
    }


#ifdef INCLUDE_TRUNCATION_GLYPHS_ALIGNMENT
  /*
   * Reset to cached values before attempting anything else.
   */
  feature->feature->x1 = x1_cache ;
  feature->feature->x2 = x2_cache ;
  feature->y1 = y1_cache ;
  feature->y2 = y2_cache ;
#endif


  /*
   * draw decorations
   */
  if (featureset->bumped)
    {
      if(!decoration_set_G)
        {
          int i;
          GdkColor colour;
          gulong pixel;
          FooCanvasItem *foo = (FooCanvasItem *) featureset;

          /* cache the colours for colinear lines */
          for(i = 1; i < COLINEARITY_N_TYPE; i++)
          {
            gdk_color_parse(colours[i],&colour);
            pixel = zMap_gdk_color_to_rgba(&colour);
            colinear_gdk_G[i].pixel = foo_canvas_get_color_pixel(foo->canvas,pixel);
          }

          decoration_set_G = TRUE;
        }

      /* add some decorations */
      if(!align->bump_set)		/* set up glyph data once only */
        {
          align->glyph3 = align->glyph5 = NULL ;

          /* NOTE this code assumes the styles have been set up with:
          * 	'glyph-strand=flip-y'
          * which will handle reverse strand indication */
          ZMapFeatureTypeStyle style = *feature->feature->style;

          /* set the glyph pointers if applicable */
          homology = style->sub_style[ZMAPSTYLE_SUB_FEATURE_HOMOLOGY];
          nc_splice = style->sub_style[ZMAPSTYLE_SUB_FEATURE_NON_CONCENCUS_SPLICE];

          /* find and/or allocate glyph structs */
          if(!feature->left && homology)	/* top end homology incomplete ? */
            {
              /* design by guesswork: this is not well documented */
              ZMapHomol homol = &feature->feature->feature.homol;
              double score = homol->y1 - 1;
              if(feature->feature->strand != homol->strand)
              score = homol->length - homol->y2;

              if(score)
                align->glyph5 = zMapWindowCanvasGetGlyph(featureset, homology, feature->feature, 5, score);

            }

          if(feature->right && nc_splice)	/* nc-splice ?? */
            {
              if(is_nc_splice(feature->feature,feature->right->feature))
              {
                ZMapWindowCanvasAlignment next = (ZMapWindowCanvasAlignment) feature->right;

                align->glyph3 = zMapWindowCanvasGetGlyph(featureset, nc_splice, feature->feature, 3, 0.0);
                next->glyph5 = zMapWindowCanvasGetGlyph(featureset, nc_splice, next->feature.feature, 5, 0.0);
              }
            }

          if(!feature->right && homology)	/* bottom end homology incomplete ? */
            {
              /* design by guesswork: this is not well documented */
              ZMapHomol homol = &feature->feature->feature.homol;
              double score = homol->length - homol->y2;
              if(feature->feature->strand != homol->strand)
                score = homol->y1 - 1;

              if(score)
                align->glyph3 = zMapWindowCanvasGetGlyph(featureset, homology, feature->feature, 3, score);
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

              if(cy2 < expose->area.y)
                continue;

              colour = zmapWindowCanvasAlignmentGetFwdColinearColour((ZMapWindowCanvasAlignment) feat);

              /* draw line between boxes, don't overlap the pixels */
              gdk_gc_set_foreground(featureset->gc, colour);


              /* The bit about clobbering the "first feature's y2" above seems to mean the first
               * line is drawn one pixel too long at the top. I hate doing this but I haven't
               * worked out what is going on  here yet !!! */
              if (!feature->left)
                cy1++ ;


              zMap_draw_line(drawable, featureset, cx1, cy1, cx2, cy2) ;

              if(cy2 > expose->area.y + expose->area.height)
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

  *width = feature->width;

  if(!zMapStyleIsUnique(*feature->feature->style))
    /* if not joining up same name features they don't need to go in the same column */
    {
      while(first->left)
	{
	  first = first->left;
	  if(first->width > *width)
	    *width = first->width;
	}

      while(feature->right)
	{
	  feature = feature->right;
	  if(feature->width > *width)
	    *width = feature->width;
	}

      first->y2 = feature->y2;
    }

  span->x1 = first->y1;
  span->x2 = first->y2;
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

  /* NOTE display index will be null on first call */

  /* feature specific eg bumped gapped alignments - adjust gaps display */
  for(sl = zMapSkipListFirst(featureset->display_index); sl; sl = sl->next)
    {
      ZMapWindowCanvasAlignment align = (ZMapWindowCanvasAlignment) sl->data;
      AlignGap ag, del;

      if(align->feature.type != FEATURE_ALIGN)	/* we could have other types in the col due to mis-config */
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
  ZMapWindowCanvasFeature feat = zMapWindowFeaturesetAddFeature(featureset, feature, y1, y2);

  zMapWindowFeaturesetSetFeatureWidth(featureset, feat);

  if(feat->feature->feature.homol.flags.masked)
    feat->flags |= focus_group_mask[WINDOW_FOCUS_GROUP_MASKED];

#if MH17_DO_HIDE
  /* NOTE if we configure styles to not load these into the canvas we don't get here */
  /* however if the user selects 'Expand feature' then we do */
  /* so we can comment out this line and remove a showhide call from zmapWindowFeatureExpand()
   * _follow this link if you revert this code_
   */
  if(feature->flags.collapsed || feature->flags.squashed || feature->flags.joined)
    feat->flags |= FEATURE_HIDE_COMPOSITE | FEATURE_HIDDEN;
#endif

  /* NOTE we may not have an index so this flag must be unset seperately */
  /* eg on OTF w/ delete existing selected */
  if(featureset->link_sideways)
    featureset->linked_sideways = FALSE;

  return feat;
}


static void zMapWindowCanvasAlignmentFreeSet(ZMapWindowFeaturesetItem featureset)
{
  /* frees gapped data _and does not alloc any more_ */
  zMapWindowCanvasAlignmentZoomSet(featureset,NULL);
}


/* Returns a newly-allocated ZMapFeatureSubPartSpan, which the caller must free with g_free,
 * or NULL. */
static ZMapFeatureSubPartSpan zmapWindowCanvasAlignmentGetSubPartSpan(FooCanvasItem *foo,
								      ZMapFeature feature, double x, double y)
{
  ZMapFeatureSubPartSpan sub_part = NULL ;
  ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) foo ;
  ZMapAlignBlock ab, prev_ab ;
  int i, match_num, gap_num ;

  /* find the gap or match if we are bumped */
  if(!fi->bumped)
    return NULL;
  if(!feature->feature.homol.align)	/* is un-gapped */
    return NULL;

  /* get sequence coords for x,y,  well y at least */
  /* AFAICS y is a world coordinate as the caller runs it through foo_w2c() */

  /* we refer to the actual feature gaps data not the display data
   * as that may be compressed and does not contain sequence info
   * return the type index and target start and end
   */

  match_num = feature->feature.homol.align->len ;
  gap_num = match_num - 1 ;
  prev_ab = NULL ;
  for (i = 0 ; !sub_part && i < feature->feature.homol.align->len ; i++)
    {
      /* in the original foo based code
       * match n corresponds to the match block indexed from 1
       * gap n corresponds to the following match block
       */
      double start, end ;

      ab = &g_array_index(feature->feature.homol.align, ZMapAlignBlockStruct, i) ;

      /* Matches.... */
      start = ab->t1 ;
      end = ab->t2 + 1 ;                                    /* full extent of gap is end + 1. */
      if (y >= start && y < end)
	{
          sub_part = g_malloc0(sizeof *sub_part) ;

          if (feature->strand == ZMAPSTRAND_FORWARD)
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
              sub_part = g_malloc0(sizeof *sub_part) ;

              if (feature->strand == ZMAPSTRAND_FORWARD)
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


/* Default function to check if the given x,y coord is within a feature, this
 * function assumes the feature is box-like. */
static double alignmentPoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
			     double item_x, double item_y, int cx, int cy,
			     double local_x, double local_y, double x_off)
{
  double best = 1.0e36 ;
  double can_start, can_end ;

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


  if (can_start <= local_y && can_end >= local_y)			    /* overlaps cursor */
    {
      double wx ;
      double left, right ;

      wx = x_off; // - (gs->width / 2) ;

      if (fi->bumped)
	wx += gs->bump_offset ;

      /* get coords within one pixel */
      left = wx - 1 ;					    /* X coords are on fixed zoom, allow one pixel grace */
      right = wx + gs->width + 1 ;

      if (local_x > left && local_x < right)			    /* item contains cursor */
	{
	  best = 0.0;
	}
    }

  return best ;
}






/*
 *                            Internal routines
 */



/*
  James says:

  These are the rules I implemented in the ExonCanvas for flagging good splice sites:

  1.)  Canonical splice donor sites are:

  Exon|Intron
  |gt
  g|gc

  2.)  Canonical splice acceptor site is:

  Intron|Exon
  ag|
*/

/*
  These can be configured in/out via styles:
  sub-features=non-concensus-splice:nc-splice-glyph
*/
#define DEBUG_SPLICE 0

static gboolean fragments_splice(char *fragment_a, char *fragment_b)
{
  gboolean splice = FALSE;

  // NB: DNA always reaches us as lower case, see zmapUtils/zmapDNA.c

  if(!fragment_a || !fragment_b)
    return(splice);

  /* we have fwd strand bases (not reverse complemented)
   * but need to test both directions as splicing is not strand sensitive...
   * in-line rev comp makes me go cross-eyed 8-(
   */

  if(!g_ascii_strncasecmp(fragment_b, "AG",2))
    {
      if(!g_ascii_strncasecmp(fragment_a+1,"GT",2) || !g_ascii_strncasecmp(fragment_a,"GGC",3))
	splice = TRUE;
    }
  else if(!g_ascii_strncasecmp(fragment_a+1, "CT",2))
    {
      if(!g_ascii_strncasecmp(fragment_b,"AC",2) || !g_ascii_strncasecmp(fragment_b,"GCC",3))
	splice = TRUE;
    }
#if DEBUG_SPLICE
  zMapLogWarning("concensus splice = %d (%.3s, %.3s)",splice,fragment_a,fragment_b);
#endif

  return splice;
}


static gboolean is_nc_splice(ZMapFeature left,ZMapFeature right)
{
  char *ldna, *rdna;
  gboolean ret = FALSE;

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

  // 3' end of exon: get 1 base  + 2 from intron
  ldna = zMapFeatureGetDNA((ZMapFeatureAny)left,
			   left->x2,
			   left->x2 + 2,
			   0); //prev_reversed);

  // 5' end of exon: get 2 bases from intron
  // NB if reversed we need 3 bases
  rdna = zMapFeatureGetDNA((ZMapFeatureAny)right,
			   right->x1 - 2,
			   right->x1,
			   0); //curr_reversed);

  if(!fragments_splice(ldna,rdna))
    ret = TRUE;

  if(ldna) g_free(ldna);
  if(rdna) g_free(rdna);

  return ret;
}


static void align_gap_free(AlignGap ag)
{
  ag->next = align_gap_free_G;
  align_gap_free_G = ag;

  n_gap_free++;
}

/* simple list structure, avioding extra malloc/free associated with GList code */
static AlignGap align_gap_alloc(void)
{
  int i;
  AlignGap ag = NULL;

  if(!align_gap_free_G)
    {
      gpointer mem = g_malloc(sizeof(AlignGapStruct) * N_ALIGN_GAP_ALLOC);
      for(i = 0; i < N_ALIGN_GAP_ALLOC; i++)
	{
	  align_gap_free(((AlignGap) mem) + i);
	}

      n_block_alloc++;
    }

  if(align_gap_free_G)
    {
      ag = (AlignGap) align_gap_free_G;
      align_gap_free_G = align_gap_free_G->next;
      memset((gpointer) ag, 0, sizeof(AlignGapStruct));

      n_gap_alloc++;
    }
  return ag;
}


/* NOTE the gaps array is not explicitly sorted, but is only provided by ACEDB and subsequent pipe
 * scripts written by otterlace and it is believed that the match blocks are always provided in
 * start coordinate order (says Ed)
 *
 * So it's a bit slack trusting an external program but ZMap has been doing that for a long time.
 * "roll on CIGAR strings" which prevent this kind of problem
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
static AlignGap makeGapped(ZMapFeature feature, double offset, FooCanvasItem *foo, gboolean forward)
{
  int i;
  AlignGap ag;
  AlignGap last_box = NULL;
  AlignGap last_ag = NULL;
  AlignGap display_ag = NULL;
  GArray *gaps ;
  ZMapAlignBlock ab;
  int cy1,cy2,fy1;
  int n ;
  gboolean edge;


  foo_canvas_w2c(foo->canvas, 0, feature->x1 + offset, NULL, &fy1);

  gaps = feature->feature.homol.align ;
  n = gaps->len ;
  for (i = 0; i < n ;i++)
    {
      ab = &g_array_index(gaps, ZMapAlignBlockStruct, i);

      /* get pixel coords of block relative to feature y1, +1 to cover all of last base on cy2 */
      foo_canvas_w2c (foo->canvas, 0, ab->t1 + offset, NULL, &cy1);
      foo_canvas_w2c (foo->canvas, 0, ab->t2 + 1 + offset, NULL, &cy2);
      cy1 -= fy1;
      cy2 -= fy1;


      if (last_box)
	{
	  if (i == 1 && (feature->flags.squashed_start))
	    {
	      last_box->edge = TRUE;
	      if(last_box->y2 - last_box->y1 > 2)
		last_box = NULL;	/* force a new box if the colours are visible */
	    }

	  edge = FALSE;

	  if(i == (n-1) && (feature->flags.squashed_end))
	    {
	      if(last_box && last_box->y2 - last_box->y1 > 2)
		last_box = NULL;	/* force a new box if the colours are visible */
	      edge = TRUE;
	    }
	}

      if(last_box)
	{
	  if(last_box->y2 == cy1 && cy2 != cy1)
	    {
	      /* extend last box and add a line where last box ended */
	      ag = align_gap_alloc();
	      ag->y1 = last_box->y2;
	      ag->y2 = last_box->y2;
	      ag->type = GAP_HLINE;
	      last_ag->next = ag;
	      last_ag = ag;
	      last_box->y2 = cy2;
	    }

	  else if(last_box->y2 < cy1 - 1)
	    {
	      /* visible gap between boxes: add a colinear line */
	      AlignBlockBoundaryType boundary_type = (forward ? ab->start_boundary : ab->end_boundary);
	      ag = align_gap_alloc();
	      ag->y1 = last_box->y2;
	      ag->y2 = cy1;
	      ag->type = (boundary_type == ALIGN_BLOCK_BOUNDARY_INTRON ? GAP_VLINE_INTRON : GAP_VLINE);
	      last_ag->next = ag;
	      last_ag = ag;
	    }

	  if(last_box->y2 < cy1)
	    {
	      /* create new box if edges do not overlap */
	      last_box = NULL;
	    }
	}

      if(!last_box)
	{
	  /* add a new box */
	  ag = align_gap_alloc();
	  ag->y1 = cy1;
	  ag->y2 = cy2;
	  ag->type = GAP_BOX;
	  ag->edge = edge;

	  if(last_ag)
	    last_ag->next = ag;
	  last_box = last_ag = ag;
	  if(!display_ag)
	    display_ag = ag;
	}

    }

  return display_ag;
}


