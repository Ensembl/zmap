/*  File: zmapWindowCanvasDraw.c
 *  Author: ed (edgrif@sanger.ac.uk)
 *  Copyright (c) 2014: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implements common drawing functions.
 *
 * Exported functions: See zmapWindowCanvasDraw.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>

#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasDraw.h>




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



/* clip to expose region */
/* erm,,, clip to visible scroll region: else rectangles would get extra edges */
int zMap_draw_line(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset, gint cx1, gint cy1, gint cx2, gint cy2)
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


  if(cx1 == cx2)	/* is vertical */
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

  gdk_draw_line (drawable, featureset->gc, cx1, cy1, cx2, cy2);

  return 1;
}


/* these are less common than solid lines */
int zMap_draw_broken_line(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset, gint cx1, gint cy1, gint cx2, gint cy2)
{
  int ret;

  gdk_gc_set_line_attributes(featureset->gc, 1, GDK_LINE_ON_OFF_DASH,GDK_CAP_BUTT, GDK_JOIN_MITER);

  ret = zMap_draw_line(drawable, featureset, cx1, cy1, cx2, cy2);

  gdk_gc_set_line_attributes(featureset->gc, 1, GDK_LINE_SOLID,GDK_CAP_BUTT, GDK_JOIN_MITER);

  return ret;
}


/* NOTE rectangles may be drawn partially if they overlap the visible region
 * in which case a false outline will be draw outside the region
 */
int zMap_draw_rect(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset,
                   gint cx1, gint cy1, gint cx2, gint cy2, gboolean fill)
{
  int result = 0 ;

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
      /* Fill rectangles get drawn one smaller in each dimension so correct for this. */
      if (fill)
	{
	  cx2++ ;
	  cy2++ ;
	}

      gdk_draw_rectangle(drawable, featureset->gc, fill, cx1, cy1, cx2 - cx1, cy2 - cy1) ;

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
                                          gboolean fill_set, gboolean outline_set, gulong fill, gulong outline)
{
  gboolean result = FALSE ;
  FooCanvasItem *item = (FooCanvasItem *)featureset ;
  GdkColor c ;
  int cx1, cy1, cx2, cy2 ;

  /* Cannot test fill or outline except maybe for max value. */
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
          /* Just draw a line.....only need to draw fill if there's no outline. */
          if (outline_set)
            c.pixel = outline ;
          else
            c.pixel = fill ;

          gdk_gc_set_foreground(featureset->gc, &c) ;
          gdk_draw_line (drawable, featureset->gc, cx1, cy1, cx2, cy2) ;
        }
      else
        {
          /* Need to draw a rectangle, possibly with fill and outline.
           * 
           * NOTE: outline rectangles are drawn _one_ bigger than filled rectangles
           * in both x & y by gdk_draw_rectangle() so we need to allow for that.
           */
          int x_width, y_width ;

          x_width = cx2 - cx1 ;
          y_width = cy2 - cy1 ;

          if (!outline_set)
            {
              /* No outline so need to enlarge fill as gdk fill draws one pixel short. */
              cx2++ ;
              cy2++ ;

              c.pixel = fill ;

              gdk_gc_set_foreground(featureset->gc, &c) ;
              gdk_draw_rectangle(drawable, featureset->gc, TRUE, cx1, cy1, x_width, y_width) ;
            }
          else
            {
              /* Draw the outline. */
              c.pixel = outline ;

              gdk_gc_set_foreground(featureset->gc, &c) ;
              gdk_draw_rectangle(drawable, featureset->gc, FALSE, cx1, cy1, x_width, y_width) ;

              /* Draw the fill if there is one _and_ it will be visible on the screen. */
              if (fill_set && ((cx2 - cx1 > 1) && (cy2 - cy1 > 1)))
                {
                  /* start one pixel in so as not to draw on outline (sets width one less as well). */
                  cx1++ ;
                  cy1++ ;
                  x_width-- ;
                  y_width-- ;

                  c.pixel = fill ;

                  gdk_gc_set_foreground(featureset->gc, &c) ;
                  gdk_draw_rectangle(drawable, featureset->gc, TRUE, cx1, cy1, x_width, y_width) ;
                }
            }
        }

      result = TRUE ;
    }

  return result ;
}


