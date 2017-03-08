/*  File: zmapWindowCanvasDraw.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 *         Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implements common drawing functions.
 *
 * Exported functions: See zmapWindowCanvasDraw.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <limits.h>
#include <math.h>

#include <zmapWindowCanvasFeatureset_I.hpp>
#include <zmapWindowCanvasFeature_I.hpp>
#include <zmapWindowCanvasDraw.hpp>
#include <zmapWindowCanvasGlyph.hpp>



// Struct for holding colinear colours....
typedef struct ColinearColoursStructType
{
  GdkColor colinear_gdk[COLINEARITY_N_TYPE] ;

} ColinearColoursStruct ;



/* For highlighting of common splices across columns. */
typedef struct HighlightDataStructType
{
  ZMapWindowFeaturesetItem featureset ;
  ZMapWindowCanvasFeature feature ;
  GdkDrawable *drawable ;

  ZMapBoundaryType boundary_type ;                          /* 5' or 3' */
  double x1, x2 ;

} HighlightDataStruct, *HighlightData ;


static int drawLine(GdkDrawable *drawable, GdkGC *gc, ZMapWindowFeaturesetItem featureset,
                    gint cx1, gint cy1, gint cx2, gint cy2) ;
static void highlightSplice(gpointer data, gpointer user_data) ;




/*
 *                Globals
 */

static ZMapWindowCanvasGlyph match_junction_glyph_start_G = NULL, match_junction_glyph_end_G = NULL ;
static ZMapWindowCanvasGlyph non_match_junction_glyph_start_G = NULL, non_match_junction_glyph_end_G = NULL ;





/*
 *                External routines
 */



/* Calculates the left/right (x1, x2) coords of a feature. */
gboolean zMapWindowCanvasCalcHorizCoords(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                                         double *x1_out, double *x2_out)
{
  gboolean result = TRUE ;
  double x1, x2 ;

  zMapReturnValIfFailSafe((featureset && feature && x1_out && x2_out), FALSE) ;

  /* Centre features in column. */
  x1 = featureset->width / 2 - feature->width / 2 ;

  /* Move over if bumped. */
  if (featureset->bumped)
    x1 += feature->bump_offset ;

  /* Add column offset. */
  x1 += featureset->dx ;

  /* What's this offset ???? */
  x1 += featureset->x_off ;

  /* Set right coord. */
  x2 = x1 + feature->width ;

  *x1_out = x1 ;
  *x2_out = x2 ;

  return result ;
}


/*
 * This does the lower half of the plane only, i.e. y2>=y1. Mimics the behaviour of
 * original function zMap_draw_line().
 *
 * There is an extra-special zmap only optimisation in here; when we are drawing lines
 * for transcripts that are close to vertical, they are more efficiently drawn as
 * as a series of calls to the line drawing function, each of which draws a vertical
 * line only.
 */
void zMap_draw_line_hack(GdkDrawable* drawable, ZMapWindowFeaturesetItem featureset,
                         gint x1, gint y1, gint x2, gint y2 )
{
  static const float fOne = (float)1.0,
                     fZero = (float)0.0,
                     fHalf = (float)0.5 ;
  int x = 0, y = 0, xdelta = 0, ydelta = 0, x1p = 0, y1p = 0;
  float m = fZero, e = fZero ;
  GdkGC *context = NULL ;
  void (*draw_points)(GdkDrawable*, GdkGC*, GdkPoint*, gint) = NULL ;
  void (*draw_segments)(GdkDrawable*, GdkGC*, GdkSegment*, gint) = NULL ;
  GdkPoint point = {0} ;
  GdkSegment segment = {0} ;

  /* error tests */
  zMapReturnIfFail(GDK_IS_DRAWABLE (drawable));
  zMapReturnIfFail(featureset) ;
  context = featureset->gc ;
  zMapReturnIfFail(GDK_IS_GC (context));

  /* now on with the rest of the drawing */
  xdelta = x2 - x1 ;
  ydelta = y2 - y1 ;

  if (ydelta < 0)
    return ;

  /* clipping - lifted from the original implementation */
  if(y1 > featureset->clip_y2)
    return ;
  if(y2 < featureset->clip_y1)
    return ;
  if(x1 == x2)
    {
      if(y1 < featureset->clip_y1)
        y1 = featureset->clip_y1;
      if(y2 > featureset->clip_y2)
        y2 = featureset->clip_y2 ;
    }
  else
    {
      float dx = xdelta ;
      float dy = ydelta ;
      if(y1 < featureset->clip_y1)
        {
          x1 += roundf(dx * (featureset->clip_y1 - y1) / dy);
          y1 = featureset->clip_y1;
        }
      if(y2 > featureset->clip_y2)
        {
          x2 -= roundf(dx * (y2 - featureset->clip_y2) / dy);
          y2 = featureset->clip_y2;
        }
    }

  /* vertical or horizontal cases */
  if (!xdelta)
    {
      if (!ydelta)
        return ;
      gdk_draw_line (drawable, context, x1, y1, x2, y2) ;
      return ;
    }
  else if (!ydelta)
    {
      if (!xdelta)
        return ;
      gdk_draw_line (drawable, context, x1, y1, x2, y2) ;
      return ;
    }

  draw_points = GDK_DRAWABLE_GET_CLASS(drawable)->draw_points ;
  if (!draw_points)
    return ;
  draw_segments = GDK_DRAWABLE_GET_CLASS(drawable)->draw_segments ;
  if (!draw_segments)
    return ;

  /* lines at 45 degrees */
  if (abs(xdelta) == abs(ydelta))
    {
      if (xdelta > 0)
        {
          for (x=x1,y=y1; x<=x2; ++x, ++y)
            {
              point.x = x;
              point.y = y;
              draw_points(drawable, context, &point, 1);
            }
        }
      else
        {
          for (x=x1,y=y1; x>=x2; --x, ++y)
            {
              point.x = x;
              point.y = y;
              draw_points(drawable, context, &point, 1);
            }
        }

      return ;
    }

  /* all other cases */
  m = (float)ydelta / (float)xdelta ;
  if ((fZero<m) && (m<fOne))
    {
      for (x=x1,y=y1; x<=x2; ++x)
        {
          point.x = x;
          point.y = y;
          draw_points(drawable, context, &point, 1);
          if ((e+m) < fHalf)
            {
              e += m ;
            }
          else
            {
              e += m - fOne ;
              ++y ;
            }
        }
    }
  else if (m > fOne)
    {
      m = fOne/m ;
      x1p=x1;
      y1p=y1;
      for (x=x1,y=y1; y<=y2; ++y)
        {
          /*point.x = x;
          point.y = y;
          draw_points(drawable, context, &point, 1);*/
          if ((e+m) < fHalf)
            {
              e += m ;
            }
          else
            {
              e += m - fOne ;
              ++x ;

              segment.x1 = x1p;
              segment.y1 = y1p;
              segment.x2 = x1p;
              segment.y2 = y;
              draw_segments(drawable, context, &segment, 1) ;
              x1p=x;
              y1p=y+1;
            }
        }
      segment.x1 = x1p;
      segment.y1 = y1p;
      segment.x2 = x1p;
      segment.y2 = y-1;
      draw_segments(drawable, context, &segment, 1) ;
    }
  else if (m < -fOne)
    {
      m = -fOne/m ;
      x1p=x1;
      y1p=y1;
      for (x=x1,y=y1; y<=y2; ++y)
        {
          /*point.x = x;
          point.y = y;
          draw_points(drawable, context, &point, 1);*/
          if ((e+m) < fHalf)
            {
              e += m ;
            }
          else
            {
              e += m - fOne;
              --x ;

              segment.x1 = x1p;
              segment.y1 = y1p;
              segment.x2 = x1p;
              segment.y2 = y;
              draw_segments(drawable, context, &segment, 1) ;
              x1p=x;
              y1p=y+1;
            }
        }
      segment.x1 = x1p;
      segment.y1 = y1p;
      segment.x2 = x1p;
      segment.y2 = y-1;
      draw_segments(drawable, context, &segment, 1) ;
    }
  else if ((-fOne<m) && (m<0.0))
    {
      m = -m ;
      for (x=x1,y=y1; x>=x2; --x)
        {
          point.x = x;
          point.y = y;
          draw_points(drawable, context, &point, 1);
          if ((e+m) < fHalf)
            {
              e += m ;
            }
          else
            {
              e += m - fOne ;
              ++y ;
            }
        }
    }
}


/* clip to expose region */
/* erm,,, clip to visible scroll region: else rectangles would get extra edges */
int zMap_draw_line(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset,
                   gint cx1, gint cy1, gint cx2, gint cy2)
{
  zMapReturnValIfFail(featureset, 0);

  /* for H or V lines we can clip easily */

  if(cy1 > featureset->clip_y2)
    return 0;
  if(cy2 < featureset->clip_y1)
    return 0;


#if 0
  /*
    the problem is long vertical lines that wrap round
    we don't draw big horizontal ones
  */
  if(cx1 > featureset->clip_x2)
    return;
  if(cx2 < featureset->clip_x1)
    {
      /* this will return if we expose part of a line that runs TR to BL
       * we'd have to swap x1 and x2 round for the comparison to work
       */
      return;
    }
#endif


  if(cx1 == cx2)        /* is vertical */
    {
#if 0
      /*
        the problem is long vertical lines that wrap round
        we don't draw big horizontal ones
      */
      if(cx1 < featureset->clip_x1)
        cx1 = featureset->clip_x1;
      if(cx2 > featureset->clip_x2)
        cx2 = featureset->clip_x2;
#endif

      if(cy1 < featureset->clip_y1)
        cy1 = featureset->clip_y1;
      if(cy2 > featureset->clip_y2)
        cy2 = featureset->clip_y2 ;
    }
  else
    {
      double dx = cx2 - cx1;
      double dy = cy2 - cy1;

      if(cy1 < featureset->clip_y1)
        {
          /* need to round to get partial lines joining up neatly */
          cx1 += round(dx * (featureset->clip_y1 - cy1) / dy);
          cy1 = featureset->clip_y1;
        }
      if(cy2 > featureset->clip_y2)
        {
          cx2 -= round(dx * (cy2 - featureset->clip_y2) / dy);
          cy2 = featureset->clip_y2;
        }

    }

  gdk_draw_line (drawable, featureset->gc, cx1, cy1, cx2, cy2) ;

  return 1;
}


/* draw thick lines (for intra-block gaps....) */
int zMapDrawThickLine(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset,
                      gint cx1, gint cy1, gint cx2, gint cy2)
{
  int ret = 0 ;

  zMapReturnValIfFail(featureset && drawable, ret);

  gdk_gc_set_line_attributes(featureset->gc, 5, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER) ;

  ret = zMap_draw_line(drawable, featureset, cx1, cy1, cx2, cy2);

  gdk_gc_set_line_attributes(featureset->gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER) ;

  return ret;
}


/* Draw a bog standard default dashed line instead of a solid one. */
int zMapDrawDashedLine(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset,
                       gint cx1, gint cy1, gint cx2, gint cy2)
{
  int ret = 0 ;

  zMapReturnValIfFail(featureset && drawable, 0) ;

  gdk_gc_set_line_attributes(featureset->gc, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER) ;

  ret = zMap_draw_line(drawable, featureset, cx1, cy1, cx2, cy2) ;

  gdk_gc_set_line_attributes(featureset->gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER) ;

  return ret;
}

/* Draw a dotted line, tedious because we have to query current GC values, set values to draw
 * dotted, draw and then reset to previous values. */
int zMapDrawDottedLine(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset,
                       gint cx1, gint cy1, gint cx2, gint cy2)
{
  int ret = 0 ;
  static GdkGC *copy_gc = NULL ;
  enum {DOTTED_LIST_LEN = 2, OFFSET = 4, DOT = 4, SPACE = 8} ;
  gint8 dotted[DOTTED_LIST_LEN] = {DOT, SPACE} ;

  zMapReturnValIfFail(featureset && drawable, 0) ;

  // Cache the gc for dotted lines, too expensive to remake every function call.
  if (!copy_gc)
    copy_gc = gdk_gc_new(drawable) ;

  // we do the setting each time because featureset->gc may be different each time.
  gdk_gc_copy(copy_gc, featureset->gc) ;

  gdk_gc_set_line_attributes(copy_gc, OFFSET, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER) ;

  gdk_gc_set_dashes(copy_gc, DOT, dotted, DOTTED_LIST_LEN) ;

  ret = drawLine(drawable, copy_gc, featureset, cx1, cy1, cx2, cy2) ;

  return ret ;
}


/* NOTE rectangles may be drawn partially if they overlap the visible region
 * in which case a false outline will be draw outside the region
 */
int zMap_draw_rect(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset,
                   gint cx1, gint cy1, gint cx2, gint cy2, gboolean fill_flag)
{
  int result = 0 ;

  zMapReturnValIfFail(featureset && drawable, result) ;

  /* as our rectangles are all aligned to H and V we can clip easily */

  /* First check whether the coords overlap the clip rect at all */
  if(cy1 > featureset->clip_y2)
    return 0;
  if(cy2 < featureset->clip_y1)
    return 0;

  if(cx1 > featureset->clip_x2)
    return 0;
  if(cx2 < featureset->clip_x1)
    return 0;

  /* Clip the coords */
  if(cx1 < featureset->clip_x1)
    cx1 = featureset->clip_x1;
  if(cy1 < featureset->clip_y1)
    cy1 = featureset->clip_y1;
  if(cx2 > featureset->clip_x2)
    cx2 = featureset->clip_x2;
  if(cy2 > featureset->clip_y2)
    cy2 = featureset->clip_y2;



  if (cy2 == cy1 || cx1 == cx2)
    {
      gdk_draw_line (drawable, featureset->gc, cx1, cy1, cx2, cy2);
      result = 1;
    }

  /* This needs removing once I've made this function static....it shouldn't happen... */
  else if (cy2 < cy1 || cx2 < cx1)
    {
      /* We expect the second coord to be greater than the first so
       * if we get here it's an error. */
      zMapWarning("Program error: Tried to draw a rectangle with negative width/height (width=%d, height=%d)",
                  cx2 - cx1, cy2 - cy1) ;
    }

  else
    {
      /* Fill rectangles get drawn by X one smaller in each dimension so correct for this. */
      if (fill_flag)
	{
	  cx2++ ;
	  cy2++ ;
	}

      gdk_draw_rectangle(drawable, featureset->gc, fill_flag, cx1, cy1, cx2 - cx1, cy2 - cy1) ;

      result = 1 ;
    }

  return result ;
}


/* Draw rectangles: takes care of resizing for filled vs. outline rectangles, rectangles
 * that are so small they are drawn as lines and so on.
 *
 * One point to note is that I'm not too happy with the world -> pixel conversion, there
 * is inaccuracy caused by rounding or maybe it's just the coordinate reporting function
 * of zmap that is broken. Anyhow we end up subtracting a pixel off the length of the
 * rectangle to try make things right....duh.....
 */
gboolean zMapCanvasFeaturesetDrawBoxMacro(ZMapWindowFeaturesetItem featureset,
                                          double x1, double x2, double y1, double y2,
                                          GdkDrawable *drawable,
                                          gboolean fill_set, gboolean outline_set, gulong ufill, gulong outline)
{
  gboolean result = FALSE ;
  FooCanvasItem *item = (FooCanvasItem *)featureset ;
  GdkColor c ;
  int cx1, cy1, cx2, cy2 ;

  /* Cannot test ufill or outline except maybe for max value. */
  zMapReturnValIfFailSafe(featureset, FALSE) ;
  zMapReturnValIfFailSafe((x1 >= 0 && x1 <= x2 && y1 >= 0 && y1 <= y2), FALSE) ;
  zMapReturnValIfFailSafe(drawable, FALSE) ;
  zMapReturnValIfFailSafe((fill_set || outline_set), FALSE) ;

  /* get item canvas coords, following example from FOO_CANVAS_RE (used by graph items) */
  /* NOTE CanvasFeature coords are the extent including decorations so we get coords from the feature */
  foo_canvas_w2c(item->canvas, x1, (y1 - featureset->start + featureset->dy), &cx1, &cy1) ;
  foo_canvas_w2c(item->canvas, x2, (y2 - featureset->start + featureset->dy) + 1, &cx2, &cy2) ;
                                                            /* + 1 to draw to the end of the last base */

  /* as our rectangles are all aligned to H and V we can clip easily */


  /* First check whether the coords overlap the clip rect at all */
  if (!(cy1 > featureset->clip_y2 || cy2 < featureset->clip_y1
        || cx1 > featureset->clip_x2 || cx2 < featureset->clip_x1))
    {
      /* NOTE...I'M NOT THAT HAPPY ABOUT THIS AS FEATURES MAY END UP LOOKING AS THOUGH THEY
       * ARE SHORTER THAN THEY REALLY ARE...CHECK THIS....e.g. A TRANSCRIPT AT THE EDGE OF ZMAP. */

      /* Do any clipping required. */
      if(cx1 < featureset->clip_x1)
        cx1 = featureset->clip_x1 ;
      if(cy1 < featureset->clip_y1)
        cy1 = featureset->clip_y1 ;
      if(cx2 > featureset->clip_x2)
        cx2 = featureset->clip_x2 ;
      if(cy2 > featureset->clip_y2)
        cy2 = featureset->clip_y2 ;

      if (cx1 == cx2 || cy1 == cy2)
        {
          /* Just draw a line.....only need to draw ufill if there's no outline. */
          if (outline_set)
            c.pixel = outline ;
          else
            c.pixel = ufill ;

          gdk_gc_set_foreground(featureset->gc, &c) ;
          gdk_draw_line(drawable, featureset->gc, cx1, cy1, cx2, cy2) ;
        }
      else
        {
          /* Need to draw a rectangle, possibly with ufill and outline.
           *
           * NOTE: outline rectangles are drawn _one_ bigger than filled rectangles
           * in both x & y by gdk_draw_rectangle() so we need to allow for that.
           */
          int x_width, y_width ;

          x_width = cx2 - cx1 ;
          y_width = cy2 - cy1 ;

          if (!outline_set)
            {
              /* No outline so need to enlarge ufill as gdk ufill draws one pixel short. */
              cx2++ ;
              cy2++ ;

              c.pixel = ufill ;

              gdk_gc_set_foreground(featureset->gc, &c) ;
              gdk_draw_rectangle(drawable, featureset->gc, TRUE, cx1, cy1, x_width, y_width) ;
            }
          else
            {
              /* Draw the outline. */
              c.pixel = outline ;

              gdk_gc_set_foreground(featureset->gc, &c) ;
              gdk_draw_rectangle(drawable, featureset->gc, FALSE, cx1, cy1, x_width, y_width) ;

              /* Draw the ufill if there is one _and_ it will be visible on the screen. */
              if (fill_set && ((cx2 - cx1 > 1) && (cy2 - cy1 > 1)))
                {
                  /* start one pixel in so as not to draw on outline (sets width one less as well). */
                  cx1++ ;
                  cy1++ ;
                  x_width-- ;
                  y_width-- ;

                  c.pixel = ufill ;

                  gdk_gc_set_foreground(featureset->gc, &c) ;
                  gdk_draw_rectangle(drawable, featureset->gc, TRUE, cx1, cy1, x_width, y_width) ;
                }
            }
        }

      result = TRUE ;
    }

  return result ;
}


gboolean zMapCanvasDrawBoxGapped(GdkDrawable *drawable,
                                 ZMapCanvasDrawColinearColours colinear_colours,
                                 int fill_set, int outline_set,
                                 gulong ufill, gulong outline,
                                 ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                                 double x1, double x2,
                                 AlignGap gapped)
{
  gboolean result = FALSE ;
  FooCanvasItem *foo = (FooCanvasItem *)featureset ;

  /* Draw full gapped alignment boxes, colinear lines etc etc. */
  AlignGap ag;
  GdkColor c;
  int cx1, cy1, cx2 ;
  int gx, gy1, gy2 ;



  /*
   * create a list of things to draw at this zoom taking onto account bases per pixel
   *
   * Note that: The function makeGapped() returns data in canvas pixel coordinates
   * _relative_to_ the start of the feature, that is the quantity feature->feature->x1.
   * Hence the rather arcane looking arithmetic on coordinates in the section below
   * where these are modified for cases that are truncated.
   */


  /* draw them */

  /* get item canvas coords.  gaps data is relative to feature y1 in pixel coordinates */
  foo_canvas_w2c(foo->canvas, x1, feature->feature->x1 - featureset->start + featureset->dy, &cx1, &cy1) ;
  foo_canvas_w2c(foo->canvas, x2, 0, &cx2, NULL) ;

  for (ag = gapped ; ag ; ag = ag->next)
    {

      gy1 = cy1 + ag->y1;
      gy2 = cy1 + ag->y2;

      switch(ag->type)
        {
        case GAP_BOX:
          {

            /* Can't use generalised draw call here because these are already canvas coords. */
            if (fill_set && (!outline_set || (gy2 - gy1 > 1)))/* ufill will be visible */
              {
                c.pixel = ufill;
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

        case GAP_HLINE:/* x coords differ, y is the same */
          {
            if (!outline_set)/* must be or else we are up the creek! */
              break;

            c.pixel = outline;
            gdk_gc_set_foreground (featureset->gc, &c);
            zMap_draw_line(drawable, featureset, cx1, gy1, cx2 - 1, gy2);/* GDK foible */
            break;
          }


        case GAP_VLINE:
        case GAP_VLINE_INTRON:
        case GAP_VLINE_UNSEQUENCED:
        case GAP_VLINE_MISSING:
          {
            if (!outline_set)/* must be or else we are up the creek! */
              break;

            gx = (cx1 + cx2) / 2;
            c.pixel = outline;
            gdk_gc_set_foreground (featureset->gc, &c);

            switch(ag->type)
              {
              case GAP_VLINE:
                zMapDrawThickLine(drawable, featureset, gx, gy1, gx, gy2);
                break ;

              case GAP_VLINE_INTRON:

                if (ag->colinearity)
                  {
                    GdkColor *colour ;

                    colour = zMapCanvasDrawGetColinearGdkColor(colinear_colours, ag->colinearity) ;
                    gdk_gc_set_foreground(featureset->gc, colour) ;

                     /* line is drawn one pixel too long at the top. I hate doing this but I haven't
                     * worked out what is going on  here yet !!! */
                    if (!feature->left)
                      gy1++ ;
                  }

                /* draw line between boxes, don't overlap the pixels */
                zMap_draw_line(drawable, featureset, gx, gy1, gx, gy2) ;

                break ;

              case GAP_VLINE_MISSING:
                zMapDrawDashedLine(drawable, featureset, gx, gy1, gx, gy2) ;
                break ;

              case GAP_VLINE_UNSEQUENCED:
                zMapDrawDottedLine(drawable, featureset, gx, gy1, gx, gy2) ;
                break ;

              default:
                zMapWarnIfReached() ;
                break ;
              }

            break;
          }

        default:
          {
            zMapWarnIfReached() ;
            break ;
          }

        } /* switch statement */

    } /* for (ag = align->gapped ; ag ; ag = ag->next) */


  return result ;
}



// Colinear colours package..could be generalised to do other colours too.....
//

// Init the colours package, the application needs to hang on to this for all drawing to a
// particular widget.
//
// Colours are a per-colourmap resource and the returned object give colinear colours for only the
// original colourmap.
//
ZMapCanvasDrawColinearColours zMapCanvasDrawAllocColinearColours(GdkColormap *colour_map)
{
  ZMapCanvasDrawColinearColours colinear_colours = NULL ;
  const char *colours[] = { "black", "red", "orange", "green" } ;

  zMapReturnValIfFail((colour_map), NULL) ;

  colinear_colours = g_new0(ColinearColoursStruct, 1) ;

  /* cache the colours for colinear lines */
  for (int i = 1 ; i < COLINEARITY_N_TYPE ; i++)
    {
      GdkColor *colour;

      colour = &(colinear_colours->colinear_gdk[i]) ;

      gdk_color_parse(colours[i], colour) ;

      if (!gdk_colormap_alloc_color(colour_map, colour, FALSE, FALSE))
        {
          zMapLogWarning("Could not allocate colinear line colour: %s", colours[i]) ;

          zMapCanvasDrawFreeColinearColours(colinear_colours) ;
          colinear_colours = NULL ;

          break ;
        }
    }

  return colinear_colours ;
}


// Given a end/start/threshold return the type of colinearity for the gap
// represented by the  end -> start  coords.
ColinearityType zMapCanvasDrawGetColinearity(int end_1, int start_2, int threshold)
{
  ColinearityType ct = COLINEAR_PERFECT ;
  int diff = 0 ;

  diff = (start_2 - end_1) - 1 ;
  if (diff < 0)
    diff = -diff ;

  if (diff > threshold)
    {
      if (start_2 < end_1)
        ct = COLINEAR_NOT ;
      else
        ct = COLINEAR_IMPERFECT ;
    }

  return ct ;
}


// Given a ColinearityType return the corresponding GdkColor struct.
// 
GdkColor *zMapCanvasDrawGetColinearGdkColor(ZMapCanvasDrawColinearColours colinear_colours, ColinearityType ct)
{
  GdkColor *colinear_colour = NULL ;

  colinear_colour = &(colinear_colours->colinear_gdk[ct]) ;

  return colinear_colour ;
}


void zMapCanvasDrawFreeColinearColours(ZMapCanvasDrawColinearColours colinear_colours)
{
  zMapReturnIfFail((colinear_colours)) ;

  g_free(colinear_colours) ;

  return ;
}







/* Given the featureset, drawable etc., display all the splices for the given feature.
 * (See Splice_highlighting.html)
 */
void zMapCanvasFeaturesetDrawSpliceHighlights(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                                              GdkDrawable *drawable, double x1, double x2)
{
  HighlightDataStruct highlight_data ;

  highlight_data.featureset = featureset ;
  highlight_data.feature = feature ;
  highlight_data.drawable = drawable ;
  highlight_data.x1 = x1 ;
  highlight_data.x2 = x2 ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zMapWindowCanvasFeaturesetGetSpliceColour(featureset, &(highlight_data.splice_pixel)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /*
   * Construct glyph object from shape given,
   * once and once only.
   */
  if (match_junction_glyph_start_G == NULL)
    {
      /* this all quite hacky until I sort out the glyphs properly..... */
      gulong border_pixel = 0, fill_pixel = 0 ;
      ZMapStyleGlyphShape start_shape, end_shape ;


      zMapWindowCanvasGlyphGetJunctionShapes(&start_shape, &end_shape) ;

      match_junction_glyph_start_G = zMapWindowCanvasGlyphAlloc(start_shape, ZMAP_GLYPH_JUNCTION_START, TRUE, TRUE) ;

      match_junction_glyph_end_G = zMapWindowCanvasGlyphAlloc(end_shape, ZMAP_GLYPH_JUNCTION_END, TRUE, TRUE) ;

      non_match_junction_glyph_start_G = zMapWindowCanvasGlyphAlloc(start_shape, ZMAP_GLYPH_JUNCTION_START, TRUE, TRUE) ;

      non_match_junction_glyph_end_G = zMapWindowCanvasGlyphAlloc(end_shape, ZMAP_GLYPH_JUNCTION_END, TRUE, TRUE) ;

      if (zMapWindowCanvasFeaturesetGetSpliceColour(featureset, TRUE, &border_pixel, &fill_pixel))
        {
          zMapWindowCanvasGlyphSetColours(match_junction_glyph_start_G, border_pixel, fill_pixel) ;

          zMapWindowCanvasGlyphSetColours(match_junction_glyph_end_G, border_pixel, fill_pixel) ;
        }

      if (zMapWindowCanvasFeaturesetGetSpliceColour(featureset, FALSE, &border_pixel, &fill_pixel))
        {
          zMapWindowCanvasGlyphSetColours(non_match_junction_glyph_start_G, border_pixel, fill_pixel) ;

          zMapWindowCanvasGlyphSetColours(non_match_junction_glyph_end_G, border_pixel, fill_pixel) ;
        }
    }

  g_list_foreach(feature->splice_positions, highlightSplice, &highlight_data) ;

  return ;
}





/*
 *             Internal routines
 */


/* clip to expose region */
/* erm,,, clip to visible scroll region: else rectangles would get extra edges */
static int drawLine(GdkDrawable *drawable, GdkGC *gc, ZMapWindowFeaturesetItem featureset,
                    gint cx1, gint cy1, gint cx2, gint cy2)
{
  /* for H or V lines we can clip easily */

  if(cy1 > featureset->clip_y2)
    return 0;
  if(cy2 < featureset->clip_y1)
    return 0;


#if 0
  /*
    the problem is long vertical lines that wrap round
    we don't draw big horizontal ones
  */
  if(cx1 > featureset->clip_x2)
    return;
  if(cx2 < featureset->clip_x1)
    {
      /* this will return if we expose part of a line that runs TR to BL
       * we'd have to swap x1 and x2 round for the comparison to work
       */
      return;
    }
#endif


  if(cx1 == cx2)        /* is vertical */
    {
#if 0
      /*
        the problem is long vertical lines that wrap round
        we don't draw big horizontal ones
      */
      if(cx1 < featureset->clip_x1)
        cx1 = featureset->clip_x1;
      if(cx2 > featureset->clip_x2)
        cx2 = featureset->clip_x2;
#endif

      if(cy1 < featureset->clip_y1)
        cy1 = featureset->clip_y1;
      if(cy2 > featureset->clip_y2)
        cy2 = featureset->clip_y2 ;
    }
  else
    {
      double dx = cx2 - cx1;
      double dy = cy2 - cy1;

      if(cy1 < featureset->clip_y1)
        {
          /* need to round to get partial lines joining up neatly */
          cx1 += round(dx * (featureset->clip_y1 - cy1) / dy);
          cy1 = featureset->clip_y1;
        }
      if(cy2 > featureset->clip_y2)
        {
          cx2 -= round(dx * (cy2 - featureset->clip_y2) / dy);
          cy2 = featureset->clip_y2;
        }

    }

  gdk_draw_line (drawable, gc, cx1, cy1, cx2, cy2) ;

  return 1;
}



static void highlightSplice(gpointer data, gpointer user_data)
{
  ZMapSplicePosition splice_pos = (ZMapSplicePosition)data ;
  HighlightData highlight_data = (HighlightData)user_data ;
  double col_width ;


  /*
   * Draw the truncation glyph subfeatures.
   */
  ZMapFeatureTypeStyle style = *(highlight_data->feature->feature->style) ;

  ZMapWindowCanvasGlyph five_start, three_end ;

  col_width = zMapStyleGetWidth(highlight_data->featureset->style) ;


  if (splice_pos->match)
    {
      five_start = match_junction_glyph_start_G ;
      three_end = match_junction_glyph_end_G ;
    }
  else
    {
      five_start = non_match_junction_glyph_start_G ;
      three_end = non_match_junction_glyph_end_G ;
    }


  /* HACKED POSITIONS BECAUSE THEY HAVE ALREADY BEEN OFFSET.....IN THE SPLICE POS.... */
  if (splice_pos->boundary_type == ZMAPBOUNDARY_5_SPLICE)

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    zMapWindowCanvasGlyphDrawGlyph(highlight_data->featureset, highlight_data->feature,
                                   style, five_start,
                                   highlight_data->drawable, col_width, splice_pos->end + 1) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    zMapWindowCanvasGlyphDrawGlyph(highlight_data->featureset, highlight_data->feature,
                                   style, five_start,
                                   highlight_data->drawable, col_width, splice_pos->start) ;

  else
    zMapWindowCanvasGlyphDrawGlyph(highlight_data->featureset, highlight_data->feature,
                                   style, three_end,
                                   highlight_data->drawable, col_width, splice_pos->end + 1) ;


  return ;
}







