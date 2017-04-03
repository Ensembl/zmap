/*  File: zmapWindowCanvasGraphics.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
 * implements callback functions for FeaturesetItem (plain graphics with no features)
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>


/* NOTE
 * as we use 'lines' mainly for decoration rather than features there may not be a real feature attached
 */


#include <math.h>
#include <string.h>
#include <ZMap/zmapFeature.hpp>
#include <zmapWindowCanvasDraw.hpp>
#include <zmapWindowCanvasFeatureset_I.hpp>
#include <zmapWindowCanvasFeature_I.hpp>
#include <zmapWindowCanvasGraphics.hpp>



void linePaint(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable, GdkEventExpose *expose)
{
  int cx1, cy1, cx2, cy2;
  FooCanvasItem *foo = NULL ;
  ZMapWindowCanvasGraphics feat = NULL ;
  GdkColor c;

  zMapReturnIfFail(featureset && feature );

  foo = (FooCanvasItem *) featureset ;
  feat = (ZMapWindowCanvasGraphics) feature ;

  c.pixel = feat->outline_val;
  gdk_gc_set_foreground (featureset->gc, &c);

  foo_canvas_w2c(foo->canvas, feat->x1 + featureset->dx, feat->y1 - featureset->start + featureset->dy,
                 &cx1, &cy1);
  foo_canvas_w2c(foo->canvas, feat->x2 + featureset->dx, feat->y2 - featureset->start + featureset->dy,
                 &cx2, &cy2);

  zMap_draw_line(drawable, featureset, cx1, cy1, cx2, cy2);

  return ;
}





static void zmapWindowCanvasGraphicsGetPango(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset)
{
  /* lazy evaluation of pango renderer */
  ZMapWindowCanvasPango pango = NULL ;

  zMapReturnIfFail(featureset && featureset->opt ) ;

  pango = (ZMapWindowCanvasPango) featureset->opt;

  if(pango->drawable && pango->drawable != drawable)
    zmapWindowCanvasFeaturesetFreePango(pango);

  if(!pango->renderer)
    {
      GdkColor *draw;

      zMapStyleGetColours(featureset->style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, NULL, NULL, &draw );

      zmapWindowCanvasFeaturesetInitPango(drawable, featureset, pango, ZMAP_ZOOM_FONT_FAMILY, ZMAP_ZOOM_FONT_SIZE, draw);
    }
}




void textPaint(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable, GdkEventExpose *expose)
{
  FooCanvasItem *foo = NULL ;
  ZMapWindowCanvasGraphics gfx = NULL ;
  int len;
  int cx,cy;

  zMapReturnIfFail(featureset && feature ) ;

  foo = (FooCanvasItem *) featureset ;
  gfx = (ZMapWindowCanvasGraphics) feature;

  ZMapWindowCanvasPango pango = (ZMapWindowCanvasPango) featureset->opt;
  if(!pango)
    return;

  zmapWindowCanvasGraphicsGetPango(drawable, featureset);

  if(!gfx->text || !*gfx->text)
    return;

  len = strlen(gfx->text);
  pango_layout_set_text (pango->layout, gfx->text, len);

  foo_canvas_w2c (foo->canvas, gfx->x1 + featureset->dx, (gfx->y1 + gfx->y2) / 2 - featureset->start + featureset->dy, &cx, &cy);
  cy -= pango->text_height / 2;                /* centre text on line */

  pango_renderer_draw_layout (pango->renderer, pango->layout,  cx * PANGO_SCALE , cy * PANGO_SCALE);
}



void graphicsZoomSet(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable)
{
  /* if longest item < text height increase that to match */
  ZMapWindowCanvasPango pango = NULL;
  double text_h;
  GList *l;

  zMapReturnIfFail(featureset);
  pango = (ZMapWindowCanvasPango) featureset->opt;

  if(!pango)
    return;

  zmapWindowCanvasGraphicsGetPango(drawable, featureset);

  text_h = pango->text_height / featureset->zoom;        /* can't use foo_canvas_c2w as it does a scroll offset */

  /* strickly speaking we could get away with text/2 as we centre the text on a y coordinate
   * that would be enough to make the lookup work
   */
  if(featureset->longest < text_h)
    featureset->longest = text_h;

  /* resize all the text items so they get painted properly */
  /* NOTE these structs are ref'd but he index it it exists */
  for(l = featureset->features; l ; l = l->next)
    {
      ZMapWindowCanvasGraphics gfx = (ZMapWindowCanvasGraphics ) l->data;
      double mid;

      if(gfx->type == FEATURE_TEXT)
        {
          mid = (gfx->y1 +gfx->y2 ) / 2;
          gfx->y1 = mid - text_h / 2;
          gfx->y2 = gfx->y1 + text_h;
        }
    }
}



void zMapWindowCanvasGraphicsInit(void)
{
  gpointer funcs[FUNC_N_FUNC] = { NULL };
  gpointer feature_funcs[CANVAS_FEATURE_FUNC_N_FUNC] = { NULL };

  funcs[FUNC_ZOOM] = (void *)graphicsZoomSet;
  zMapWindowCanvasFeatureSetSetFuncs(FEATURE_GRAPHICS, funcs, sizeof(zmapWindowCanvasPangoStruct));
  zMapWindowCanvasFeatureSetSize(FEATURE_GRAPHICS, feature_funcs, sizeof(zmapWindowCanvasFeatureStruct)) ;

  funcs[FUNC_ZOOM] = NULL;        /* not needed by the others but no harm done  if it's left in?? */

  funcs[FUNC_PAINT] = (void *)linePaint;
  zMapWindowCanvasFeatureSetSetFuncs(FEATURE_LINE, funcs, (size_t) 0) ;
  zMapWindowCanvasFeatureSetSize(FEATURE_LINE, feature_funcs, sizeof(zmapWindowCanvasFeatureStruct)) ;

  funcs[FUNC_PAINT] = (void *)textPaint;
  zMapWindowCanvasFeatureSetSetFuncs(FEATURE_TEXT, funcs, (size_t) 0);
  zMapWindowCanvasFeatureSetSize(FEATURE_TEXT, feature_funcs, sizeof(zmapWindowCanvasFeatureStruct)) ;

  return ;
}

