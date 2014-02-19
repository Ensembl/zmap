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

/*
 * A glyph to represent the trunction of a feature at the ZMap boundary. This one just draws a
 * square with the diagonal oriented in the direction of the feature.
 */
static ZMapStyleGlyphShapeStruct truncation_shape_basic_instance01 =
{
  {
    5, 0,      0, 5,        -5, 0,       0, -5,      5, 0,          0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  },                                                                               /* length 32 coordinate array */
  5,                                                                               /* number of coordinates */
  10, 10,                                                                          /* width and height */
  0,                                                                               /* quark ID */
  GLYPH_DRAW_LINES                                                                 /* ZMapStyleGlyphDrawType; LINES == OUTLINE, POLYGON == filled */
}  ;

static ZMapStyleGlyphShapeStruct * truncation_shape_basic01 = &truncation_shape_basic_instance01 ;
static ZMapWindowCanvasGlyph truncation_glyph_basic = NULL ;

/* draw a box */
static void basicPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                              GdkDrawable *drawable, GdkEventExpose *expose)
{
  gulong fill,outline ;
  int colours_set = 0, fill_set = 0, outline_set = 0;
  double x1 = 0.0, x2 = 0.0, col_width = 0.0 ;
  gboolean truncated_start = FALSE,
    truncated_end = FALSE ;
  zMapReturnIfFail(featureset && feature && drawable && expose ) ;
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

  /*
   * Construct glyph object from shape given,
   * once and once only.
   */
#ifdef INCLUDE_TRUNCATION_GLYPHS
  if (truncation_glyph_basic == NULL)
    {
      truncation_glyph_basic = g_new0(zmapWindowCanvasGlyphStruct, 1) ;
      truncation_glyph_basic->sub_feature = TRUE ;
      truncation_glyph_basic->shape = truncation_shape_basic01 ;
    }
  col_width = zMapStyleGetWidth(featureset->style) ;
  zmap_window_canvas_set_glyph(foo, truncation_glyph_basic, style, feature->feature, col_width, feature->score ) ;

  /*
   * Draw the truncation glyph subfeatures.
   */
  if (truncated_start)
    {
      truncation_glyph_basic->which = ZMAP_GLYPH_TRUNCATED_START ;
      zMapWindowCanvasGlyphPaintSubFeature(featureset, feature, truncation_glyph_basic, drawable) ;
    }
  if (truncated_end)
    {
      truncation_glyph_basic->which = ZMAP_GLYPH_TRUNCATED_END ;
      zMapWindowCanvasGlyphPaintSubFeature(featureset, feature, truncation_glyph_basic, drawable) ;
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
