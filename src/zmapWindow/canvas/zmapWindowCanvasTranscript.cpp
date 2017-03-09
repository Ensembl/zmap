/*  File: zmapWindowCanvasTranscript.c
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
 * Description: Implements callback functions for FeaturesetItem
 *              transcript features.
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <math.h>
#include <string.h>

#include <ZMap/zmapFeature.hpp>
#include <zmapWindowCanvasDraw.hpp>
#include <zmapWindowCanvasFeatureset_I.hpp>
#include <zmapWindowCanvasFeature_I.hpp>
#include <zmapWindowCanvasTranscript_I.hpp>
#include <zmapWindowCanvasGlyph.hpp>

#define USE_DRAWING_BUG_HACK 1

static void transcriptPaintFeature(ZMapWindowFeaturesetItem featureset,
                                   ZMapWindowCanvasFeature feature,
                                   GdkDrawable *drawable, GdkEventExpose *expose) ;
static void transcriptPreZoom(ZMapWindowFeaturesetItem featureset) ;
static void transcriptZoomSet(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable) ;
static void transcriptFreeSet(ZMapWindowFeaturesetItem featureset) ;
static ZMapWindowCanvasFeature transcriptAddFeature(ZMapWindowFeaturesetItem featureset,
                                                    ZMapFeature feature, double y1, double y2) ;
static ZMapFeatureSubPart transcriptGetSubPart(FooCanvasItem *foo, ZMapFeature feature, double x, double y) ;
static void transcriptGetFeatureExtent(ZMapWindowCanvasFeature feature, ZMapSpan span, double *width) ;



/*
 *            Globals
 */

static ZMapWindowCanvasGlyph truncation_glyph_transcript_start_G = NULL ;
static ZMapWindowCanvasGlyph truncation_glyph_transcript_end_G = NULL ;




/*
 *                    External interface
 *
 * (Note some functions have internal linkage but form part of the external interface.)
 */

void zMapWindowCanvasTranscriptInit(void)
{
  gpointer funcs[FUNC_N_FUNC] = { NULL } ;
  gpointer feature_funcs[CANVAS_FEATURE_FUNC_N_FUNC] = { NULL };

  funcs[FUNC_PAINT] = (void *)transcriptPaintFeature ;
  funcs[FUNC_PRE_ZOOM] = (void *)transcriptPreZoom ;
  funcs[FUNC_ZOOM] = (void *)transcriptZoomSet ;
  funcs[FUNC_FREE] = (void *)transcriptFreeSet ;
  funcs[FUNC_ADD] = (void *)transcriptAddFeature ;

  zMapWindowCanvasFeatureSetSetFuncs(FEATURE_TRANSCRIPT, funcs, (size_t)0) ;

  feature_funcs[CANVAS_FEATURE_FUNC_EXTENT] = (void *)transcriptGetFeatureExtent ;
  feature_funcs[CANVAS_FEATURE_FUNC_SUBPART] = (void *)transcriptGetSubPart ;

  zMapWindowCanvasFeatureSetSize(FEATURE_TRANSCRIPT, feature_funcs, sizeof(zmapWindowCanvasTranscriptStruct)) ;

  return ;
}



static void transcriptPaintFeature(ZMapWindowFeaturesetItem featureset,
                                   ZMapWindowCanvasFeature feature,
                                   GdkDrawable *drawable, GdkEventExpose *expose)
{
  gulong ufill = 0L, outline = 0L ;
  int colours_set = 0, fill_set = 0, outline_set = 0 ;
  int cx1, cy1, cx2, cy2, cy1_5, cx1_5;
  double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0, y1_cache = 0.0, y2_cache = 0.0, col_width = 0.0 ;
  FooCanvasItem *foo = NULL ;
  ZMapWindowCanvasTranscript tr = NULL ;
  ZMapFeature real_feature ;
  ZMapTranscript transcript = NULL ;
  ZMapFeatureTypeStyle style = NULL ;
  gboolean ignore_truncation_glyphs = FALSE,
    truncated_start = FALSE,
    truncated_end = FALSE ;

  zMapReturnIfFail(featureset && feature && feature->feature && drawable && expose) ;

  // Allocate colours for colinear lines if not allocated.
  if (!(featureset->colinear_colours))
    {
      featureset->colinear_colours = zMapCanvasDrawAllocColinearColours(gdk_gc_get_colormap(featureset->gc)) ;
    }

  foo = (FooCanvasItem *) featureset;
  tr = (ZMapWindowCanvasTranscript) feature;
  real_feature = feature->feature ;
  transcript = &feature->feature->feature.transcript;
  style = *feature->feature->style;

  /*
   * set up position.
   */
  zMapWindowCanvasCalcHorizCoords(featureset, feature, &x1, &x2) ;

  /*
   * Find y coordinates of the feature
   */
  y1_cache = y1 = feature->y1 ;
  y2_cache = y2 = feature->y2 ;

  /*
   * Sometimes if we are dealing with truncated features (e.g. transcript)
   * we might get a (sub)feature e.g. intron that is completely outside of the
   * view area. This test traps that possibility and thus avoids the ensuing
   * hilarity as a consequence of pixel based arithmetic when one attempts
   * to draw them.
   */
  if (y2 <= featureset->start || y1 >= featureset->end)
    {
      return ;
    }

  /*
   * Determine whether or not the feature needs to be truncated
   * at the start or end.
   */
  if (y1 < featureset->start)
    {
      truncated_start = TRUE ;
      feature->y1 = y1 = featureset->start ;
    }
  if (y2 > featureset->end)
    {
      truncated_end = TRUE ;
      feature->y2 = y2 = featureset->end ;
    }

  /* Quick hack for features that are completely outside of the sequence
   * region. These should not really be passed in, but occasionally are
   * due to bugs on the otterlace side (Feb. 20th 2014). We ignore truncation
   * glyphs completely for these cases.
   */
  if ((feature->feature->x2 < featureset->start) || (feature->feature->x1 > featureset->end))
    {
      ignore_truncation_glyphs = TRUE ;
    }

  /*
   * Set up non-cds colours
   */
  colours_set = zMapWindowCanvasFeaturesetGetColours(featureset, feature, &ufill, &outline);
  fill_set = colours_set & WINDOW_FOCUS_CACHE_FILL;
  outline_set = colours_set & WINDOW_FOCUS_CACHE_OUTLINE;

  /*
   * Drawing of truncation glyphs themselves.
   */
  if (!ignore_truncation_glyphs)
    {

      /*
       * Instantiate glyph objects.
       */
      if (truncation_glyph_transcript_start_G == NULL)
        {
          ZMapStyleGlyphShape start_shape, end_shape ;

          zMapWindowCanvasGlyphGetTruncationShapes(&start_shape, &end_shape) ;

          truncation_glyph_transcript_start_G = zMapWindowCanvasGlyphAlloc(start_shape, ZMAP_GLYPH_TRUNCATED_START,
                                                                           FALSE, TRUE) ;

          truncation_glyph_transcript_end_G = zMapWindowCanvasGlyphAlloc(end_shape, ZMAP_GLYPH_TRUNCATED_END,
                                                                         FALSE, TRUE) ;
        }

      /*
       * Draw glyphs at start and end if required.
       */
      col_width = zMapStyleGetWidth(featureset->style) ;

      if (truncated_start)
        {
          zMapWindowCanvasGlyphDrawGlyph(featureset, feature,
                                         style, truncation_glyph_transcript_start_G,
                                         drawable, col_width, 0.0) ;
        }
      if (truncated_end)
        {
          zMapWindowCanvasGlyphDrawGlyph(featureset, feature,
                                         style, truncation_glyph_transcript_end_G,
                                         drawable, col_width, 0.0) ;
        }

      if (truncated_start || truncated_end)
        featureset->draw_truncation_glyphs = TRUE ;

    }

  /*
   * Draw any UTR sections of an exon.
   */
  if (tr->sub_type == TRANSCRIPT_EXON)
    {
      /* utr at start ? */
      if(transcript->flags.cds && transcript->cds_start > y1 && transcript->cds_start < y2)
        {
          zMapCanvasFeaturesetDrawBoxMacro(featureset, x1, x2, y1, transcript->cds_start-1,
                                           drawable, fill_set, outline_set, ufill, outline) ;


          y1 = transcript->cds_start ;
        }

      /* utr at end ? */
      if(transcript->flags.cds && transcript->cds_end > y1 && transcript->cds_end < y2)
        {
          zMapCanvasFeaturesetDrawBoxMacro(featureset, x1, x2, transcript->cds_end + 1, y2,
                                           drawable, fill_set, outline_set, ufill, outline) ;
          y2 = transcript->cds_end ;
        }
    }


  /*
   * set up cds colours if in CDS
   */
  if (transcript->flags.cds && transcript->cds_start <= y1 && transcript->cds_end >= y2)
    {
      ZMapStyleColourType ct;
      GdkColor *gdk_fill = NULL, *gdk_outline = NULL;

      ct = ((feature->flags & WINDOW_FOCUS_GROUP_FOCUSSED)
            ? ZMAPSTYLE_COLOURTYPE_SELECTED : ZMAPSTYLE_COLOURTYPE_NORMAL) ;
      zMapStyleGetColours(*feature->feature->style,
                          STYLE_PROP_TRANSCRIPT_CDS_COLOURS, ct, &gdk_fill, NULL, &gdk_outline);

      if(gdk_fill)
        {
          gulong fill_colour = 0L ;

          fill_set = TRUE;
          fill_colour = zMap_gdk_color_to_rgba(gdk_fill);
          ufill = foo_canvas_get_color_pixel(foo->canvas, fill_colour);
        }
      else
        {
          /* Set fill_set = false so that the cds colour defaults to the background colour */
          fill_set = FALSE ;
        }

      if(gdk_outline)
        {
          gulong outline_colour = 0L ;

          outline_set = TRUE;
          outline_colour = zMap_gdk_color_to_rgba(gdk_outline);
          outline = foo_canvas_get_color_pixel(foo->canvas, outline_colour);
        }
      else
        {
          /* Don't set fill_set to false so that the border colour defaults to the
           * UTR border colour, if set */
        }

    }

  /*
   * Now draw either box for CDS of exon, or intron lines.
   */
  if (tr->sub_type == TRANSCRIPT_EXON)
    {
      int exon_index = tr->index - 1 ;
      GArray *exon_aligns ;

      // ok, here we need to detect whether this exon has gaps and act accordingly....   
      if (!zMapFeatureTranscriptHasAlignParts(real_feature)
          || !(exon_aligns = zMapFeatureTranscriptGetAlignParts(real_feature, exon_index)))
        {
          zMapCanvasFeaturesetDrawBoxMacro(featureset, x1, x2, y1, y2,
                                           drawable, fill_set, outline_set, ufill, outline) ;
        }
      else
        {
          if (!(tr->gapped))
            {
              gboolean is_forward = TRUE ;

              tr->gapped = zMapWindowCanvasAlignmentMakeGapped(featureset, feature,
                                                               y1, y2,
                                                               exon_aligns, is_forward) ;
            }

          zMapCanvasDrawBoxGapped(drawable,
                                  featureset->colinear_colours,
                                  fill_set, outline_set, ufill, outline,
                                  featureset, feature,
                                  x1, x2, tr->gapped) ;
        }
    }
  else if (outline_set)
    {
      if (tr->sub_type == TRANSCRIPT_INTRON)
        {
          GdkColor c;

          /* get item canvas coords in pixel coordinates */
          /* NOTE not quite sure why y1 is 1 out when y2 isn't */
          foo_canvas_w2c(foo->canvas, x1, feature->y1 - featureset->start + featureset->dy, &cx1, &cy1);
          foo_canvas_w2c(foo->canvas, x2, feature->y2 - featureset->start + featureset->dy + 1, &cx2, &cy2);

          cy1_5 = (cy1 + cy2) / 2 ;
          cx1_5 = (cx1 + cx2) / 2 ;

          c.pixel = outline;
          gdk_gc_set_foreground (featureset->gc, &c) ;

          /*
           * a hack to the intron lines
           */
#ifdef USE_DRAWING_BUG_HACK
          zMap_draw_line_hack(drawable, featureset, cx1_5, cy1, cx2, cy1_5) ;

          zMap_draw_line_hack(drawable, featureset, cx2, cy1_5+1, cx1_5, cy2) ;
#else
          zMap_draw_line(drawable, featureset, cx1_5, cy1, cx2, (featureset->draw_truncation_glyphs
                                                                 ? cy1_5+1 : cy1_5));
          zMap_draw_line(drawable, featureset, cx2, cy1_5, cx1_5, cy2);
#endif
        }
      else if(tr->sub_type == TRANSCRIPT_INTRON_START_NOT_FOUND)
        {
          GdkColor c;

          /* get item canvas coords in pixel coordinates */
          /* NOTE not quite sure why y1 is 1 out when y2 isn't */
          foo_canvas_w2c (foo->canvas, x1, feature->y1 - featureset->start + featureset->dy, &cx1, &cy1);
          foo_canvas_w2c (foo->canvas, x2, feature->y2 - featureset->start + featureset->dy + 1, &cx2, &cy2);

          cx1_5 = (cx1 + cx2) / 2;

          c.pixel = outline;
          gdk_gc_set_foreground (featureset->gc, &c);

          zMapDrawDashedLine(drawable, featureset, cx2, cy1, cx1_5, cy2);
        }
      else if(tr->sub_type == TRANSCRIPT_INTRON_END_NOT_FOUND)
        {
          GdkColor c;

          /* get item canvas coords in pixel coordinates */
          /* NOTE not quite sure why y1 is 1 out when y2 isn't */
          foo_canvas_w2c (foo->canvas, x1, feature->y1 - featureset->start + featureset->dy, &cx1, &cy1);
          foo_canvas_w2c (foo->canvas, x2, feature->y2 - featureset->start + featureset->dy + 1, &cx2, &cy2);

          cx1_5 = (cx1 + cx2) / 2;

          c.pixel = outline;
          gdk_gc_set_foreground (featureset->gc, &c);

          zMapDrawDashedLine(drawable, featureset, cx1_5, cy1, cx2, cy2);
        }
    }


  /* Highlight all splice positions if they exist, do this bumped or unbumped otherwise user
   * has to bump column to see common splices (See Splice_highlighting.html). */
  if (feature->splice_positions)
    {
      zMapCanvasFeaturesetDrawSpliceHighlights(featureset, feature, drawable, x1, x2) ;
    }


  /*
   * Reset to cached values.
   */
  feature->y1 = y1_cache ;
  feature->y2 = y2_cache ;

  return ;
}


static void transcriptPreZoom(ZMapWindowFeaturesetItem featureset)
{
  /* Need to call routine to trigger calculate on zoom for text here... */
  transcriptZoomSet(featureset, NULL) ;

  return ;
}


/*
 * if we are displaying a gapped alignment, recalculate this data
 * do this by freeing the existing data, new stuff will be added by the paint function
 *
 * NOTE ref to FreeSet() below -> drawable may be NULL
 */
static void transcriptZoomSet(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable)
{
  ZMapSkipList sl;

  zMapReturnIfFail(featureset) ;

  /* NOTE display index will be null on first call */

  /* feature specific eg bumped gapped alignments - adjust gaps display */
  for(sl = zMapSkipListFirst(featureset->display_index); sl; sl = sl->next)
    {
      ZMapWindowCanvasTranscript transcript = (ZMapWindowCanvasTranscript)(sl->data) ;
      AlignGap ag, del;

      if (transcript->feature.type != FEATURE_TRANSCRIPT)/* we could have other types in the col due to mis-config */
        continue;

      /* delete the old, these get created on paint */
      for(ag = transcript->gapped ; ag ; )
        {
          del = ag ;
          ag = ag->next ;

          zMapWindowCanvasAlignmentFreeGapped(del) ;
        }

      transcript->gapped = NULL ;
    }

  //printf("alignment zoom: %ld %ld %ld\n",n_block_alloc, n_gap_alloc, n_gap_free);

  return ;
}


static void transcriptFreeSet(ZMapWindowFeaturesetItem featureset)
{
  /* frees gapped data _and does not alloc any more_ */
  transcriptZoomSet(featureset, NULL) ;

  return ;
}


static ZMapWindowCanvasFeature transcriptAddFeature(ZMapWindowFeaturesetItem featureset,
                                                    ZMapFeature feature, double y1, double y2)
{
  ZMapWindowCanvasFeature feat = NULL ;
  int ni = 0, ne = 0, i;
  GArray *introns,*exons;
  ZMapWindowCanvasTranscript tr = NULL;
  double fy1,fy2;
  ZMapSpan exon,intron;

  zMapReturnValIfFail(featureset && feature, feat) ;

  if ((exons = feature->feature.transcript.exons))
    {
      ne = exons->len ;

      if ((introns = feature->feature.transcript.introns))
        ni = introns->len ;

#if USE_DOTTED_LINES
      if(feature->feature.transcript.flags.start_not_found)        /* add dotted line fading away into the distance */
        {
          double trunc_len = zMapStyleGetTruncatedIntronLength(*feature->style);
          if(!trunc_len)
            trunc_len = y1 - featureset->start;

          fy2 = y1;
          fy1 = fy2 - trunc_len;

          feat = zMapWindowFeaturesetAddFeature(featureset, feature, fy1, fy2);
          feat->width = featureset->width;

          tr = (ZMapWindowCanvasTranscript) feat;
          tr->sub_type = TRANSCRIPT_INTRON_START_NOT_FOUND;
          tr->index = -1;
        }
#endif

      for(i = 0; i < ne; i++)
        {
          exon = &g_array_index(exons,ZMapSpanStruct,i);
          fy1 = y1 - feature->x1 + exon->x1;
          fy2 = y1 - feature->x1 + exon->x2;

          feat = zMapWindowFeaturesetAddFeature(featureset, feature, fy1, fy2);
          feat->width = featureset->width;

          if(tr)
            {
              tr->feature.right = feat;
              feat->left = &tr->feature;
            }

          tr = (ZMapWindowCanvasTranscript)feat ;
          tr->sub_type = TRANSCRIPT_EXON ;

          if (feature->strand == ZMAPSTRAND_FORWARD)
            tr->index = i + 1 ;
          else
            tr->index = (ne - i) ;

          if (i < ni)
            {
              intron = &g_array_index(introns, ZMapSpanStruct, i) ;

              fy1 = y1 - feature->x1 + intron->x1 ;
              fy2 = y1 - feature->x1 + intron->x2 ;

              feat = zMapWindowFeaturesetAddFeature(featureset, feature, fy1,fy2) ;
              feat->width = featureset->width ;

              tr->feature.right = feat ;
              feat->left = &tr->feature ;

              tr = (ZMapWindowCanvasTranscript)feat ;
              tr->sub_type = TRANSCRIPT_INTRON ;

              /* should this be corrected for strand...find out where it's used.... */
              if (feature->strand == ZMAPSTRAND_FORWARD)
                tr->index = i + 1 ;
              else
                tr->index = (ni - i) ;
            }
        }

#if USE_DOTTED_LINES
      if(feature->feature.transcript.flags.end_not_found)        /* add dotted line fading away into the distance */
        {
          double trunc_len = zMapStyleGetTruncatedIntronLength(*feature->style);
          if(!trunc_len)
            trunc_len =  featureset->end - y2;

          fy1 = y2;
          fy2 = fy1 + trunc_len;

          feat = zMapWindowFeaturesetAddFeature(featureset, feature, fy1, fy2);
          feat->width = featureset->width;

          tr->feature.right = feat;
          feat->left = &tr->feature;

          tr = (ZMapWindowCanvasTranscript) feat;
          tr->sub_type = TRANSCRIPT_INTRON_END_NOT_FOUND;
          tr->index = i;
        }
#endif
    }


  return feat ;
}



/* the interface to this is via zMapWindowCanvasItemGetInterval(), so here we have to look up the feature again */
/*! \todo #warning revisit this when canvas items are simplified */
/* and then we find the transcript data in the feature context which has a list of exons and introns */
/* or we could find the exons/introns in the canvas and process those */
static ZMapFeatureSubPart transcriptGetSubPart(FooCanvasItem *foo, ZMapFeature feature, double x, double y)
{
  ZMapFeatureSubPart sub_part = NULL ;
  ZMapSpan exon,intron ;
  int ni = 0, ne = 0, i ;
  GArray *introns,*exons ;
  ZMapTranscript tr = NULL ;

  zMapReturnValIfFail(feature, NULL) ;

  tr = &feature->feature.transcript ;

  introns = tr->introns;
  exons = tr->exons;
  if(introns)
    ni = introns->len;
  if(exons)
    ne = exons->len;

  /* NOTE: if we have truncated introns then we will not return a sub part as they are not in the feature */

  for (i = 0 ; i < ne ; i++)
    {
      exon = &g_array_index(exons, ZMapSpanStruct, i) ;

      if (exon->x1 <= y && exon->x2 >= y)
        {
          GArray *exon_aligns ;

          sub_part = g_new0(ZMapFeatureSubPartStruct, 1) ;

          if (feature->strand == ZMAPSTRAND_FORWARD)
            sub_part->index = i + 1 ;
          else
            sub_part->index = (ne - i) ;

          sub_part->subpart = ZMAPFEATURE_SUBPART_EXON ;

          if (zMapFeatureTranscriptHasAlignParts(feature)
              && (exon_aligns = zMapFeatureTranscriptGetAlignParts(feature, i)))
            {
              // Gapped exon from align program, we assume it's all CDS for now.
              sub_part = zMapWindowCanvasGetGappedSubPart(exon_aligns, feature->strand, y) ;
            }
          else
            {
              // non-gapped, "traditional" exon, check for UTR, CDS etc....   

              sub_part->start = exon->x1;
              sub_part->end = exon->x2;

              /* we have to handle both ends :-(   |----UTR-----|--------CDS--------|-----UTR------| */
              if (tr->flags.cds)
                {
                  if(tr->cds_start <= y && tr->cds_end >= y)
                    {
                      /* cursor in CDS but could have UTR in this exon */
                      sub_part->subpart = ZMAPFEATURE_SUBPART_EXON_CDS;

                      if(sub_part->start < tr->cds_start)
                        {
                          sub_part->start = tr->cds_start;
                        }

                      if(sub_part->end > tr->cds_end)
                        {
                          sub_part->end = tr->cds_end ;
                        }
                    }
                  else if (y >= tr->cds_end)
                    {
                      /* cursor not in CDS but could have some in this exon */
                      if (sub_part->start <= tr->cds_end)
                        {
                          sub_part->start = tr->cds_end + 1;
                        }
                    }
                  else if (y <= tr->cds_start)
                    {
                      if (sub_part->end >= tr->cds_start)
                        {
                          sub_part->end = tr->cds_start - 1;
                        }
                    }
                }

            }

          break ;
        }

      if (i < ni)
        {
          intron = &g_array_index(introns,ZMapSpanStruct,i) ;

          if (intron->x1 <= y && intron->x2 >= y)
            {
              sub_part = g_new0(ZMapFeatureSubPartStruct, 1) ;

              if (feature->strand == ZMAPSTRAND_FORWARD)
                sub_part->index = i + 1 ;
              else
                sub_part->index = (ni - i) ;

              sub_part->start = intron->x1 ;
              sub_part->end = intron->x2 ;
              sub_part->subpart = ZMAPFEATURE_SUBPART_INTRON ;

              if(tr->flags.cds && tr->cds_start <= y && tr->cds_end >= y)
                sub_part->subpart = ZMAPFEATURE_SUBPART_INTRON_CDS ;

              break ;
            }
        }
    }


  return sub_part ;
}


/* get sequence extent of compound alignment (for bumping) */
/* NB: canvas coords could overlap due to sub-pixel base pairs
 * which could give incorrect de-overlapping
 * that would be revealed on zoom
 */
static void transcriptGetFeatureExtent(ZMapWindowCanvasFeature feature, ZMapSpan span, double *width)
{
  ZMapWindowCanvasFeature first = feature;

  zMapReturnIfFail(feature && width) ;

  first = feature;

  *width = feature->width;

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

  span->x1 = first->y1;
  span->x2 = feature->y2;

  return ;
}








