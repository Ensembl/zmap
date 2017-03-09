/*  File: zmapWindowCanvasBasic.c
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
 * Description: Implements functions for FeaturesetItem representing
 *              basic features (i.e. boxes).
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <math.h>
#include <string.h>

#include <ZMap/zmapFeature.hpp>
#include <ZMap/zmapStyle.hpp>
#include <zmapWindowCanvasDraw.hpp>
#include <zmapWindowCanvasFeatureset_I.hpp>
#include <zmapWindowCanvasFeature_I.hpp>
#include <zmapWindowCanvasGlyph.hpp>




static void basicPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                              GdkDrawable *drawable, GdkEventExpose *expose) ;


/*
 *             Globals
 */

static ZMapWindowCanvasGlyph truncation_glyph_basic_start_G = NULL, truncation_glyph_basic_end_G = NULL ;





/*
 *             External routines
 *
 * (some of these have static linkage but form part of the external interface.)
 */


void zMapWindowCanvasBasicInit(void)
{
  gpointer funcs[FUNC_N_FUNC] = { NULL } ;
  gpointer feature_funcs[CANVAS_FEATURE_FUNC_N_FUNC] = { NULL };

  funcs[FUNC_PAINT] = (void *)basicPaintFeature ;

  zMapWindowCanvasFeatureSetSetFuncs(FEATURE_BASIC, funcs, (size_t)0) ;

  zMapWindowCanvasFeatureSetSize(FEATURE_BASIC, feature_funcs, sizeof(zmapWindowCanvasFeatureStruct)) ;


  return ;
}


/*
 * Function to draw a  basic feature.
 */
static void basicPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                              GdkDrawable *drawable, GdkEventExpose *expose)
{
  gulong ufill,outline ;
  int colours_set = 0, fill_set = 0, outline_set = 0 ;
  double x1 = 0.0, x2 = 0.0 ;
  gboolean draw_truncation_glyphs = TRUE, truncated_start = FALSE, truncated_end = FALSE ;
  zMapReturnIfFail(featureset && feature && feature->feature && drawable && expose ) ;
  ZMapFeatureTypeStyle style = *feature->feature->style ;
  FooCanvasItem *foo = (FooCanvasItem *) featureset ;

  /* colours are not defined for the CanvasFeatureSet
   * as we can have several styles in a column
   * but they are cached by the calling function
   * and also the window focus code
   */
  colours_set = zMapWindowCanvasFeaturesetGetColours(featureset, feature, &ufill, &outline) ;
  fill_set = colours_set & WINDOW_FOCUS_CACHE_FILL ;
  outline_set = colours_set & WINDOW_FOCUS_CACHE_OUTLINE ;

  if (fill_set && feature->feature->population)
    {
      if((zMapStyleGetScoreMode(style) == ZMAPSCORE_HEAT) || (zMapStyleGetScoreMode(style) == ZMAPSCORE_HEAT_WIDTH))
        {
          ufill = (ufill << 8) | 0xff;	/* convert back to RGBA */
          ufill = foo_canvas_get_color_pixel(foo->canvas,
                                            zMapWindowCanvasFeatureGetHeatColour(0xffffffff,ufill,feature->score));
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
                                       fill_set, outline_set, ufill, outline) ;
    }



  /* Highlight all splice positions if they exist, do this bumped or unbumped otherwise user
   * has to bump column to see common splices. (See Splice_highlighting.html) */
  if (feature->splice_positions)
    {
      zMapCanvasFeaturesetDrawSpliceHighlights(featureset, feature, drawable, x1, x2) ;
    }


  /*
   * Quick hack for features that are completely outside of the sequence
   * region. These should not really be passed in, but occasionally are
   * due to bugs on the otterlace side (Feb. 20th 2014).
   */
  if ((feature->feature->x2 < featureset->start) || (feature->feature->x1 > featureset->end))
    {
      draw_truncation_glyphs = FALSE ;
    }

  if (draw_truncation_glyphs)
    {
      double col_width ;

      /*
       * Construct glyph object from shape given,
       * once and once only.
       */
      if (truncation_glyph_basic_start_G == NULL)
        {
          ZMapStyleGlyphShape start_shape, end_shape ;

          zMapWindowCanvasGlyphGetTruncationShapes(&start_shape, &end_shape) ;

          truncation_glyph_basic_start_G = zMapWindowCanvasGlyphAlloc(start_shape, ZMAP_GLYPH_TRUNCATED_START,
                                                                      FALSE, TRUE) ;

          truncation_glyph_basic_end_G = zMapWindowCanvasGlyphAlloc(end_shape, ZMAP_GLYPH_TRUNCATED_END,
                                                                    FALSE, TRUE) ;
        }


      /*
       * Draw the truncation glyph subfeatures.
       */
      col_width = zMapStyleGetWidth(featureset->style) ;

      if (truncated_start)
        {
          zMapWindowCanvasGlyphDrawGlyph(featureset, feature,
                                         style, truncation_glyph_basic_start_G,
                                         drawable, col_width, 0.0) ;
        }
      if (truncated_end)
        {
          zMapWindowCanvasGlyphDrawGlyph(featureset, feature,
                                         style, truncation_glyph_basic_end_G,
                                         drawable, col_width, 0.0) ;
        }

    }

  return ;
}




/*
 *               Internal routines
 */
