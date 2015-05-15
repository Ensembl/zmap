/*  File: zmapWindowCanvasTranscript.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 * Description: Implements callback functions for FeaturesetItem
 *              transcript features.
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <string.h>

#include <ZMap/zmapFeature.h>
#include <zmapWindowCanvasDraw.h>
#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasFeature_I.h>
#include <zmapWindowCanvasTranscript_I.h>
#include <zmapWindowCanvasGlyph.h>



static void transcriptPaintFeature(ZMapWindowFeaturesetItem featureset,
                                   ZMapWindowCanvasFeature feature,
                                   GdkDrawable *drawable, GdkEventExpose *expose) ;
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

  funcs[FUNC_PAINT] = (void *)transcriptPaintFeature;
  funcs[FUNC_ADD]   = (void *)transcriptAddFeature;

  zMapWindowCanvasFeatureSetSetFuncs(FEATURE_TRANSCRIPT, funcs, (size_t)0) ;

  feature_funcs[CANVAS_FEATURE_FUNC_EXTENT] = (void *)transcriptGetFeatureExtent ;
  feature_funcs[CANVAS_FEATURE_FUNC_SUBPART] = (void *)transcriptGetSubPart;
  zMapWindowCanvasFeatureSetSize(FEATURE_TRANSCRIPT, feature_funcs, sizeof(zmapWindowCanvasTranscriptStruct)) ;

  return ;
}



static void transcriptPaintFeature(ZMapWindowFeaturesetItem featureset,
                                   ZMapWindowCanvasFeature feature,
                                   GdkDrawable *drawable, GdkEventExpose *expose)
{
  gulong fill = 0L, outline = 0L ;
  int colours_set = 0, fill_set = 0, outline_set = 0 ;
  double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0, y1_cache = 0.0, y2_cache = 0.0, col_width = 0.0 ;
  ZMapWindowCanvasTranscript tr = NULL ;
  FooCanvasItem *foo = NULL ;
  ZMapTranscript transcript = NULL ;
  ZMapFeatureTypeStyle style = NULL ;
  gboolean ignore_truncation_glyphs = FALSE,
    truncated_start = FALSE,
    truncated_end = FALSE ;

  zMapReturnIfFail(featureset && feature && feature->feature && drawable && expose ) ;

  foo = (FooCanvasItem *) featureset;
  tr = (ZMapWindowCanvasTranscript) feature;
  transcript = &feature->feature->feature.transcript;
  style = *feature->feature->style;

  /* draw a box */

  /* colours are not defined for the CanvasFeatureSet
   * as we can have several styles in a column
   * but they are cached by the calling function
   * and also the window focus code
   * however, as we have CDS colours diff from non-CDS and possibly diff style in one column
   * this is likely ineffective, but as the number of features is small we don't care so much
   */

  /*
   * Set up non-cds colours
   */
  colours_set = zMapWindowCanvasFeaturesetGetColours(featureset, feature, &fill, &outline);
  fill_set = colours_set & WINDOW_FOCUS_CACHE_FILL;
  outline_set = colours_set & WINDOW_FOCUS_CACHE_OUTLINE;


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
                                           drawable, fill_set, outline_set, fill, outline) ;


          y1 = transcript->cds_start ;
        }

      /* utr at end ? */
      if(transcript->flags.cds && transcript->cds_end > y1 && transcript->cds_end < y2)
        {
          zMapCanvasFeaturesetDrawBoxMacro(featureset, x1, x2, transcript->cds_end + 1, y2,
                                           drawable, fill_set, outline_set, fill, outline) ;
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
          fill = foo_canvas_get_color_pixel(foo->canvas, fill_colour);
        }

      if(gdk_outline)
        {
          gulong outline_colour = 0L ;

          outline_set = TRUE;
          outline_colour = zMap_gdk_color_to_rgba(gdk_outline);
          outline = foo_canvas_get_color_pixel(foo->canvas, outline_colour);
        }

    }

  /*
   * Now draw either box for CDS of exon, or intron lines.
   */
  int cx1, cy1, cx2, cy2, cy1_5, cx1_5;
  if(tr->sub_type == TRANSCRIPT_EXON)
    {
      zMapCanvasFeaturesetDrawBoxMacro(featureset, x1, x2, y1, y2, drawable, fill_set,outline_set,fill,outline) ;
    }
  else if (outline_set)
    {
      if(tr->sub_type == TRANSCRIPT_INTRON)
        {
          GdkColor c;

          /* get item canvas coords in pixel coordinates */
          /* NOTE not quite sure why y1 is 1 out when y2 isn't */
          foo_canvas_w2c(foo->canvas, x1, feature->y1 - featureset->start + featureset->dy, &cx1, &cy1);
          foo_canvas_w2c(foo->canvas, x2, feature->y2 - featureset->start + featureset->dy + 1, &cx2, &cy2);

          cy1_5 = (cy1 + cy2) / 2;
          cx1_5 = (cx1 + cx2) / 2;

          c.pixel = outline;
          gdk_gc_set_foreground (featureset->gc, &c);

          /*
           * sm23 21st Feb. 2014.
           *
           * This modification to the y coordinate is required when truncation glyphs are drawn.
           * It is a hack to get around what looks like a bug in some part of the drawing code
           * or perhas the canvas itself.
           */
          zMap_draw_line(drawable, featureset, cx1_5, cy1, cx2, (featureset->draw_truncation_glyphs
                                                                 ? cy1_5+1 : cy1_5));
          zMap_draw_line(drawable, featureset, cx2, cy1_5, cx1_5, cy2);
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

          zMap_draw_broken_line(drawable, featureset, cx2, cy1, cx1_5, cy2);
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

          zMap_draw_broken_line(drawable, featureset, cx1_5, cy1, cx2, cy2);
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




static ZMapFeatureSubPart transcriptGetSubPart(FooCanvasItem *foo,
                                               ZMapFeature feature, double x, double y)
{
  ZMapFeatureSubPart sub_part = NULL ;

  /* the interface to this is via zMapWindowCanvasItemGetInterval(), so here we have to look up the feature again */
  /*! \todo #warning revisit this when canvas items are simplified */
  /* and then we find the transcript data in the feature context which has a list of exons and introms */
  /* or we could find the exons/introns in the canvas and process those */

  ZMapSpan exon,intron ;
  int ni = 0, ne = 0, i ;
  GArray *introns,*exons ;
  ZMapTranscript tr = NULL ;

  zMapReturnValIfFail(feature, sub_part ) ;

  tr = &feature->feature.transcript;

  introns = tr->introns;
  exons = tr->exons;
  if(introns)
    ni = introns->len;
  if(exons)
    ne = exons->len;

  /* NOTE: is we have truncated introns then we will not return a sub part as they are not in the feature */

  for(i = 0 ; i < ne ; i++)
    {
      exon = &g_array_index(exons, ZMapSpanStruct, i) ;

      if (exon->x1 <= y && exon->x2 >= y)
        {
          sub_part = g_new0(ZMapFeatureSubPartStruct, 1) ;

          if (feature->strand == ZMAPSTRAND_FORWARD)
            sub_part->index = i + 1 ;
          else
            sub_part->index = (ne - i) ;

          sub_part->start = exon->x1;
          sub_part->end = exon->x2;
          sub_part->subpart = ZMAPFEATURE_SUBPART_EXON;

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

          return sub_part ;
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

              sub_part->start = intron->x1;
              sub_part->end = intron->x2;
              sub_part->subpart = ZMAPFEATURE_SUBPART_INTRON;

              if(tr->flags.cds && tr->cds_start <= y && tr->cds_end >= y)
                sub_part->subpart = ZMAPFEATURE_SUBPART_INTRON_CDS;

              return sub_part;
            }
        }
    }

  return NULL;
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



