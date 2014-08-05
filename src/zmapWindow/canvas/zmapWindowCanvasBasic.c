/*  File: zmapWindowCanvasBasic.c
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
 *
 * Description: Implements callback functions for FeaturesetItem
 *              basic features (boxes).
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <string.h>

#include <ZMap/zmapFeature.h>
#include <ZMap/zmapStyle.h>
#include <zmapWindowCanvasDraw.h>
#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasGlyph_I.h>
#include <zmapWindowCanvasGlyph.h>

#define INCLUDE_TRUNCATION_GLYPHS_BASIC 1

/*
 * Glyphs to represent the trunction of a feature at the ZMap boundary. One for
 * truncation at the start and one for truncation at the end.
 */

static ZMapStyleGlyphShapeStruct truncation_shape_basic_instance_start =
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

static ZMapStyleGlyphShapeStruct truncation_shape_basic_instance_end =
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


static ZMapStyleGlyphShapeStruct * truncation_shape_basic_start = &truncation_shape_basic_instance_start ;
static ZMapStyleGlyphShapeStruct * truncation_shape_basic_end = &truncation_shape_basic_instance_end ;
static ZMapWindowCanvasGlyph truncation_glyph_basic_start = NULL ;
static ZMapWindowCanvasGlyph truncation_glyph_basic_end = NULL ;

/*
 * Function to draw a  basic feature.
 */
static void basicPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                              GdkDrawable *drawable, GdkEventExpose *expose)
{
  gulong fill,outline ;
  int colours_set = 0, fill_set = 0, outline_set = 0;
  double x1 = 0.0, x2 = 0.0, col_width = 0.0 ;
  gboolean draw_truncation_glyphs = TRUE,
    truncated_start = FALSE,
    truncated_end = FALSE ;
  zMapReturnIfFail(featureset && feature && feature->feature && drawable && expose ) ;
  ZMapFeatureTypeStyle style = *feature->feature->style;
  FooCanvasItem *foo = (FooCanvasItem *) featureset ;

  /* colours are not defined for the CanvasFeatureSet
   * as we can have several styles in a column
   * but they are cached by the calling function
   * and also the window focus code
   */
  colours_set = zMapWindowCanvasFeaturesetGetColours(featureset, feature, &fill, &outline) ;
  fill_set = colours_set & WINDOW_FOCUS_CACHE_FILL ;
  outline_set = colours_set & WINDOW_FOCUS_CACHE_OUTLINE ;

  if (fill_set && feature->feature->population)
    {
      if((zMapStyleGetScoreMode(style) == ZMAPSCORE_HEAT) || (zMapStyleGetScoreMode(style) == ZMAPSCORE_HEAT_WIDTH))
        {
          fill = (fill << 8) | 0xff;	/* convert back to RGBA */
          fill = foo_canvas_get_color_pixel(foo->canvas,
                                            zMapWindowCanvasFeatureGetHeatColour(0xffffffff,fill,feature->score));
        }
    }

  /*
   * Determine whether or not the feature needs to be truncated
   * at the start or end.
   */
  if (feature->feature->x1 < featureset->start)
    {
      truncated_start = TRUE ;
      feature->y1 = featureset->start ;
    }
  if (feature->feature->x2 > featureset->end)
    {
      truncated_end = TRUE ;
      feature->y2 = featureset->end ;
    }


  /*
   * Draw the basic box.
   */
  if (zMapWindowCanvasCalcHorizCoords(featureset, feature, &x1, &x2))
    {
      zMapCanvasFeaturesetDrawBoxMacro(featureset, x1, x2, feature->y1, feature->y2, drawable,
                                       fill_set, outline_set, fill, outline) ;
    }


#ifdef INCLUDE_TRUNCATION_GLYPHS_BASIC
  /*
   * Quick hack for features that are completely outside of the sequence
   * region. These should not really be passed in, but occasionally are
   * due to bugs on the otterlace side (Feb. 20th 2014).
   */
  if (    (feature->feature->x2 < featureset->start)
      ||  (feature->feature->x1 > featureset->end)  )
    {
      draw_truncation_glyphs = FALSE ;
    }

  if (draw_truncation_glyphs)
    {

      /*
       * Construct glyph object from shape given,
       * once and once only.
       */
      if (truncation_glyph_basic_start == NULL)
        {
          truncation_glyph_basic_start = g_new0(zmapWindowCanvasGlyphStruct, 1) ;
          truncation_glyph_basic_start->sub_feature = TRUE ;
          truncation_glyph_basic_start->shape = truncation_shape_basic_start ;
          truncation_glyph_basic_start->which = ZMAP_GLYPH_TRUNCATED_START ;
        }
      if (truncation_glyph_basic_end == NULL)
        {
          truncation_glyph_basic_end = g_new0(zmapWindowCanvasGlyphStruct, 1) ;
          truncation_glyph_basic_end->sub_feature = TRUE ;
          truncation_glyph_basic_end->shape = truncation_shape_basic_end ;
          truncation_glyph_basic_end->which = ZMAP_GLYPH_TRUNCATED_END ;
        }
      col_width = zMapStyleGetWidth(featureset->style) ;

      /*
       * Draw the truncation glyph subfeatures.
       */
      if (truncated_start)
        {
          zMapWindowCanvasGlyphDrawTruncationGlyph(foo, featureset, feature,
                                                   style, truncation_glyph_basic_start,
                                                   drawable, col_width) ;
        }
      if (truncated_end)
        {
          zMapWindowCanvasGlyphDrawTruncationGlyph(foo, featureset, feature,
                                                   style, truncation_glyph_basic_end,
                                                   drawable, col_width) ;
        }

    }
#endif


  return ;
}









void zMapWindowCanvasBasicInit(void)
{
  gpointer funcs[FUNC_N_FUNC] = { NULL } ;

  funcs[FUNC_PAINT] = basicPaintFeature ;

  zMapWindowCanvasFeatureSetSetFuncs(FEATURE_BASIC, funcs, 0, 0) ;

  return ;
}
