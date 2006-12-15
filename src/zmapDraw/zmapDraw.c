/*  File: zmapDraw.c
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Draw objects into the canvas, these may be unnecessary
 *              if they map closely enough to the foo_canvas calls.
 *              
 * Exported functions: See ZMap/zmapDraw.h
 *              
 * HISTORY:
 * Last edited: Dec 15 09:46 2006 (edgrif)
 * Created: Wed Oct 20 09:19:16 2004 (edgrif)
 * CVS info:   $Id: zmapDraw.c,v 1.57 2006-12-15 09:59:08 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <zmapDraw_P.h>


static void drawHighlightLinesInGroup(FooCanvasGroup *parent,
                                      GdkColor *outline,
                                      double offsetX1, double offsetY1,
                                      double offsetX2, double offsetY2,
                                      double minX,     double maxX,
                                      double dlength);
static void drawHighlightBackgroundInGroup(FooCanvasGroup *parent,
                                           GdkColor *background,
                                           double offsetX1, double offsetY1,
                                           double offsetX2, double offsetY2,
                                           double minX,     double maxX,
                                           double dlength);


/* bitmap for doing the overlays, leads to diagonal lines allowing an element of "transparency".
 * 
 * The only way to get rid of a bitmap (i.e. drawable) once allocated seems to be to do 
 * 
 *          g_object_unref(G_OBJECT(bitmap)) ;
 * 
 * the draw code doesn't need to do this so far.
 * 
 *  */
#define overlay_bitmap_width 16
#define overlay_bitmap_height 4
static char overlay_bitmap_bits[] =
  {
    0x11, 0x11,
    0x22, 0x22,
    0x44, 0x44,
    0x88, 0x88
  } ;






/*! @defgroup zmapdraw   zMapDraw: basic drawing operations: boxes, text etc.
 * @{
 * 
 * \brief  Drawing shapes, lines, text on the foocanvas.
 * 
 * zMapDraw routines encapsulate calls to draw items on to the foocanvas.
 *
 *  */





FooCanvasItem *zMapDisplayText(FooCanvasGroup *group, char *text, char *colour,
			       double x, double y)
{
  FooCanvasItem *item = NULL ;

  /* something to note here is that text sometimes produces strange sizes in its
   * containing group, I think this may be because we don't clip the text item to
   * be any particular size...but then again.... */
  item = foo_canvas_item_new(group,
			     FOO_TYPE_CANVAS_TEXT,
			     "x", x, "y", y,
			     "text", text,
                             "font", "Lucida Console",
			     "fill_color", colour,
                             "anchor",     GTK_ANCHOR_NW,
			     NULL);
  if(0)
    {
      PangoLayout *layout = NULL;
      PangoContext *context = NULL;
      PangoMatrix matrix = PANGO_MATRIX_INIT;
      
      layout  = FOO_CANVAS_TEXT(item)->layout;
      
      context = pango_layout_get_context(layout);
      
      pango_matrix_rotate(&matrix, 90.0);
      
      pango_context_set_matrix(context, &matrix);
      pango_layout_context_changed(layout);
    }

  return item ;
}

FooCanvasItem *zMapDrawHighlightableText(FooCanvasGroup *group,
                                         char *text_string,
                                         double x, double y,
                                         GdkColor *foreground, 
                                         GdkColor *highlight,
                                         FooCanvasItem **highlight_item_out)
{
  FooCanvasItem *text = NULL, *highlight_item = NULL;

  text = foo_canvas_item_new(group,
			     foo_canvas_text_get_type(),
			     "x",          x, 
                             "y",          y,
			     "text",       text_string,
                             "font",       "Lucida Console",
			     "fill_color", foreground,
                             "anchor",     GTK_ANCHOR_NW,
			     NULL);
  if(text)
    {
      double x1 = 0.0, y1 = 0.0, x2 = 0.0, y2 = 0.0;
      foo_canvas_item_get_bounds(text, &x1, &y1, &x2, &y2);
      if((highlight_item = zMapDrawBoxSolid(group, x1, y1, x2, y2, highlight)))
        {
          foo_canvas_item_lower(highlight_item, 1);
          foo_canvas_item_hide(highlight_item);
        }
    }

  if(highlight_item_out)
    *highlight_item_out = highlight_item;

  return text;
}


/* Note that we don't specify "width_units" or "width_pixels" for the outline
 * here because by not doing so we get a one pixel wide outline that is
 * completely enclosed within the item. If you specify either width, you end
 * up with items that are larger than you expect because the outline is drawn
 * centred on the edge of the rectangle. */
FooCanvasItem *zMapDrawBox(FooCanvasGroup *group, 
			   double x1, double y1, double x2, double y2, 
			   GdkColor *border_colour, GdkColor *fill_colour,
			   guint line_width)
{
  FooCanvasItem *item = NULL ;

  item = foo_canvas_item_new(FOO_CANVAS_GROUP(group),
			     foo_canvas_rect_get_type(),
			     "x1", x1, "y1", y1,
			     "x2", x2, "y2", y2,
			     "outline_color_gdk", border_colour,
			     "fill_color_gdk", fill_colour,
			     "width_pixels", line_width,
			     NULL) ;

  return item;                                                                       
}

/* As above but we do not set outline.... */
FooCanvasItem *zMapDrawBoxSolid(FooCanvasGroup *group, 
				double x1, double y1, double x2, double y2, 
				GdkColor *fill_colour)
{
  FooCanvasItem *item = NULL ;

  item = foo_canvas_item_new(FOO_CANVAS_GROUP(group),
			     foo_canvas_rect_get_type(),
			     "x1", x1, "y1", y1,
			     "x2", x2, "y2", y2,
			     "fill_color_gdk", fill_colour,
			     NULL) ;

  return item;                                                                       
}

/* Semi transparent box. */
FooCanvasItem *zMapDrawBoxOverlay(FooCanvasGroup *group, 
				  double x1, double y1, double x2, double y2, 
				  GdkColor *fill_colour)
{
  FooCanvasItem *item = NULL ;
  static GdkBitmap *overlay = NULL ;

  if (!overlay)
    overlay = gdk_bitmap_create_from_data(NULL, &overlay_bitmap_bits[0],
					  overlay_bitmap_width, overlay_bitmap_height) ;

  item = foo_canvas_item_new(FOO_CANVAS_GROUP(group),
			     foo_canvas_rect_get_type(),
			     "x1", x1, "y1", y1,
			     "x2", x2, "y2", y2,
			     "fill_color_gdk", fill_colour,
			     "fill_stipple", overlay,
			     NULL) ;

  return item;                                                                       
}


void zMapDrawBoxChangeSize(FooCanvasItem *box, 
			   double x1, double y1, double x2, double y2)
{

  foo_canvas_item_set(box,
		      "x1", x1,
		      "y1", y1,
		      "x2", x2,
		      "y2", y2,
		      NULL) ;

  return ;
}




/*!
 * Draws scaleless glyphs, they show up at any zoom which is what we want.
 *
 * @param group        Parent foocanvas group to contain the new item.
 * @param x            X coordinate of item in group coords.
 * @param y            Y coordinate of item in group coords.
 * @param glyph_type   shape to be drawn
 * @param colour       colour shape is to be drawn in.
 * @param width        overall width of glyph.
 * @param line_width   line width of glyph.
 *
 * @return Returns a pointer to the new canvas item representing the glyph.
 *  */
FooCanvasItem *zMapDrawGlyph(FooCanvasGroup *group, double x, double y,
			     ZMapDrawGlyphType glyph_type,
			     GdkColor *colour, double width, guint line_width)
{
  FooCanvasItem *item = NULL ;
  enum {MAX_POINTS = 20} ;
  double glyph_points[MAX_POINTS] ;
  FooCanvasPoints glyph = {NULL} ;


  zMapAssert(FOO_IS_CANVAS_GROUP(group)
	     && glyph_type != ZMAPDRAW_GLYPH_INVALID && colour && width > 0 && line_width >= 0) ;

  switch (glyph_type)
    {
    case ZMAPDRAW_GLYPH_LINE:
      {
	glyph_points[0] = 0.0 ;
	glyph_points[1] = 0.0 ;
	glyph_points[2] = 0.0 + 5.0 ;
	glyph_points[3] = 0.0 ;

	glyph.num_points = 2 ;

	break ;
      }
    case ZMAPDRAW_GLYPH_ARROW:
      {
	double arrow_width = width ;
	double arrow_height = width * 0.25 ;

	glyph_points[0] = 0.0 + arrow_width ;
	glyph_points[1] = 0.0 ;
	glyph_points[2] = 0.0 ;
	glyph_points[3] = 0.0 ;
	glyph_points[4] = 0.0 + arrow_height ;
	glyph_points[5] = 0.0 - arrow_height ;
	glyph_points[6] = 0.0 + arrow_height ;
	glyph_points[7] = 0.0 + arrow_height ;
	glyph_points[8] = 0.0 ;
	glyph_points[9] = 0.0 ;

	glyph.num_points = 5 ;

	break ;
      }

    case ZMAPDRAW_GLYPH_DOWN_BRACKET:
    case ZMAPDRAW_GLYPH_UP_BRACKET:
      {
	double bracket_height = 10.0 ; ;

	if (glyph_type == ZMAPDRAW_GLYPH_UP_BRACKET)
	  bracket_height *= -1.0 ;

	glyph_points[0] = 0.0 ;
	glyph_points[1] = 0.0 ;
	glyph_points[2] = width ;
	glyph_points[3] = 0.0 ;
	glyph_points[4] = width ;
	glyph_points[5] = bracket_height ;

	glyph.num_points = 3 ;

	break ;
      }


    default:
      zMapAssertNotReached() ;
      break ;
    }

  glyph.coords = glyph_points ;
  glyph.ref_count = 1 ;					    /* Make sure canvas does not try to free them. */


  /* draw the line */
  item = foo_canvas_item_new(group,
			     foo_canvas_line_glyph_get_type(),
			     "x", x, "y", y,
			     "points", &glyph,
			     "fill_color_gdk", colour,
			     "width_pixels", line_width,
			     "join_style", GDK_JOIN_BEVEL,
			     "cap_style", GDK_CAP_BUTT,
			     NULL);


		    
  return item ;
}





/* dimension is either a line width when form translates to creating a
 * line.  It can refer to a position though, see utr form.  We might
 * need another one, but if any more are required a rewrite/alternate
 * function might well be better
 */
FooCanvasItem *zMapDrawAnnotatePolygon(FooCanvasItem *polygon, 
                                       ZMapAnnotateForm form,
                                       GdkColor *border,
                                       GdkColor *fill,
                                       double dimension, /* we might need another one */
				       guint line_width,
                                       int zmapStrand)
{
  FooCanvasItem   *item  = NULL;
  FooCanvasPoints *final = NULL, *tmp   = NULL;
  GType annItemType = 0; /* So we can draw a line or polygon with points */
  double x1, x2, y1, y2;
  int i;
  zMapAssert( FOO_IS_CANVAS_POLYGON(polygon) );

  g_object_get(G_OBJECT(polygon),
               "points", &tmp,
               NULL);
  zMapAssert(tmp);

  x1 = x2 = tmp->coords[0];
  y1 = y2 = tmp->coords[1];

  /* we need to find the bounds of the polygon.
   * foo_canvas_item_get_bounds could do this, but we already have the
   * info and saving a number of function calls here. */
  for(i = 0; i < tmp->num_points * 2; i++)
    {
      if(i % 2)                 /* odd = y */
        {
          y1 = MIN(y1, tmp->coords[i]);
          y2 = MAX(y2, tmp->coords[i]);
        }
      else                      /* even = x*/
        {
          x1 = MIN(x1, tmp->coords[i]);
          x2 = MAX(x2, tmp->coords[i]);
        }
    }

  switch(form)
    {
    case ZMAP_ANNOTATE_INTRON:
      {
        double y_halfway = 0.0;
        /* Here we want halfway Y between bow + stern and bow bow and stern stern points */
        y_halfway = y1 + ((y2 - y1) / 2);
        annItemType = foo_canvas_line_get_type();

        final = foo_canvas_points_new(3); /* We need three points, this is defined somewhere */
        final->coords[0] = tmp->coords[2];
        final->coords[1] = tmp->coords[3];
        final->coords[2] = x2;
        final->coords[3] = y_halfway;
        final->coords[4] = tmp->coords[8];
        final->coords[5] = tmp->coords[9];
      }
      break;
    case ZMAP_ANNOTATE_GAP:
      annItemType = foo_canvas_line_get_type();
      final = foo_canvas_points_new(2);
      final->coords[0] = tmp->coords[2];
      final->coords[1] = y1;
      final->coords[2] = tmp->coords[2];
      final->coords[3] = y2;
      break;
    case ZMAP_ANNOTATE_UTR_LAST:
      annItemType = foo_canvas_polygon_get_type(); /* We'll make a polygon */
      final = foo_canvas_points_new((int)(tmp->num_points));

      memcpy(&(final->coords[0]),
             &(tmp->coords[0]),
             ((tmp->num_points * 2) * sizeof(double))) ;
      /* Now we only need to change some */
      final->coords[7]  = dimension;
      final->coords[9]  = dimension;
      final->coords[11] = dimension;
      final->coords[11] = dimension;
      tmp->coords[1]  = dimension;
      tmp->coords[3]  = dimension;
      tmp->coords[5]  = dimension;
      tmp->coords[13] = dimension;

      foo_canvas_item_set(polygon, "points", tmp, NULL);
      break;
    case ZMAP_ANNOTATE_UTR_FIRST:
      annItemType = foo_canvas_polygon_get_type(); /* We'll make a polygon */
      final = foo_canvas_points_new((int)(tmp->num_points));
      /* we change the stern here! */

      memcpy(&(final->coords[0]),
             &(tmp->coords[0]),
             ((tmp->num_points * 2) * sizeof(double))) ;
      /* Now we only need to change some */
      final->coords[1]  = dimension;
      final->coords[3]  = dimension;
      final->coords[5]  = dimension;
      final->coords[13] = dimension;
      tmp->coords[7]  = dimension;
      tmp->coords[9]  = dimension;
      tmp->coords[11] = dimension;
      tmp->coords[11] = dimension;

      foo_canvas_item_set(polygon, "points", tmp, NULL);
      break;
    case ZMAP_ANNOTATE_EXTEND_ALIGN:
      {
        double diff = dimension - y2;
        annItemType = foo_canvas_line_get_type(); /* We draw a line at the original place */
        final = foo_canvas_points_new(2); /* just a line */
        final->coords[0] = x1 + 1;
        final->coords[1] = y2;
        final->coords[2] = x2 - 1; /* cap style issues */
        final->coords[3] = y2;
        switch(zmapStrand)
          {
          case ZMAPSTRAND_REVERSE:
            tmp->coords[7]  += diff;
            tmp->coords[9]  += diff;
            tmp->coords[11] += diff;
            break;
          case ZMAPSTRAND_FORWARD:
          default:
            tmp->coords[1] += diff;
            tmp->coords[3] += diff;
            tmp->coords[5] += diff;
            tmp->coords[13] += diff;
            break;
          }
        foo_canvas_item_set(polygon, "points", tmp, NULL);
        dimension = 1.0;
      }
      break;
    default:
      annItemType = foo_canvas_text_get_type();
      final = foo_canvas_points_new(2); /* just a line */      
      switch(zmapStrand)
        {
        case ZMAPSTRAND_REVERSE:
          break;
        case ZMAPSTRAND_FORWARD:
        default:
          break;
        }
      break;
    }

  zMapAssert(final != NULL && annItemType);

  if(annItemType == foo_canvas_line_get_type())
    item = foo_canvas_item_new(FOO_CANVAS_GROUP(polygon->parent),
                               annItemType,
                               "points", final,
                               "fill_color_gdk", fill,
                               "width_pixels", line_width,
                               "join_style", GDK_JOIN_BEVEL,
                               "cap_style", GDK_CAP_BUTT,
                               NULL) ;
  else if(annItemType == foo_canvas_polygon_get_type())
    item = foo_canvas_item_new(FOO_CANVAS_GROUP(polygon->parent),
                               annItemType,
                               "points", final,
                               "outline_color_gdk", border,
                               "fill_color_gdk", fill,
                               "width_pixels", line_width,
                               NULL) ;

  return item ;
}

/* zMapDrawSSPolygon = draw a Strand Sensitive Polygon */
/* We don't close the polygon, the foocanvas does that for us.
 * this means that we end up with POINT_MAX points!
 */

/* A Feature has a length and a width.  So we draw from feature length
 * start to feature length end and the feature ends up feature width A
 * to feature width B wide.  Whether this is vertical or horizontal
 * _shouldn't_ really matter. */
FooCanvasItem *zMapDrawSSPolygon(FooCanvasItem *grp, ZMapPolygonForm form,
                                 double fwidthA, double fwidthB, 
                                 double fstart, double fend, 
                                 GdkColor *border, GdkColor *fill,
				 guint line_width,
				 int zmapStrand)
{
  FooCanvasItem   *item   = NULL;
  FooCanvasPoints *points = NULL;
  int i = 0, strand = 0;
  double x1, x2, y1, y2;
  /* For now */
  x1 = fwidthA; x2 = fwidthB; y1 = fstart; y2 = fend;
  
  /* I want to do the following here! */
#ifdef NOT_JUST_YET________________________________________________
  zmapWindowSeq2CanOffset(&x1, &x2, (FOO_CANVAS_GROUP(grp)->xpos) + 1);
  zmapWindowSeq2CanOffset(&y1, &y2, (FOO_CANVAS_GROUP(grp)->ypos) + 1);
#endif

  points = foo_canvas_points_new(POINT_MAX - 1);

  switch(zmapStrand)
    {
    case ZMAPSTRAND_REVERSE:
      strand = -1;
      break;
    case ZMAPSTRAND_FORWARD:
    case ZMAPSTRAND_NONE:
    default:
      strand = 1;
      break;
    }

  zMapAssert(strand == -1 || strand == 1);

  for(i = BOW_PORT; i < POINT_MAX; i++)
    {
      int x_coord, y_coord, curr_p;
      curr_p  = (i + VERTICAL) * strand;
      x_coord = (i - 1) * 2;    /* these are just indices */
      y_coord = x_coord + 1;
      /* This is really long I know, but ... faster than function
       * calls and probably more terse than many functions! Unless
       * some commonality is apparent. */
      switch(curr_p)
        {
          /* FORWARD POINTS */
        case POINT_BOW_PORT_FORWARD:
          points->coords[x_coord] = x2;
          switch(form)
            {
            case ZMAP_POLYGON_DIAMOND:
            case ZMAP_POLYGON_DOUBLE_POINTING:
            case ZMAP_POLYGON_POINTING:
            case ZMAP_POLYGON_TRIANGLE_PORT:
            case ZMAP_POLYGON_TRAPEZOID_PORT:
              points->coords[y_coord] = y2 - ZMAP_EXON_POINT_SIZE;          
              break;
            case ZMAP_POLYGON_SQUARE: /* etc... */
            default:
              points->coords[y_coord] = y2;          
              break;
            }
          break;
        case POINT_BOW_BOW_FORWARD:
          points->coords[y_coord] = y2;
          switch(form)
            {
            case ZMAP_POLYGON_TRIANGLE_PORT:
            case ZMAP_POLYGON_TRAPEZOID_PORT:
              points->coords[x_coord] = x1;
              break;
            case ZMAP_POLYGON_TRIANGLE_STAR:
            case ZMAP_POLYGON_TRAPEZOID_STAR:
              points->coords[x_coord] = x2;
              break;
            case ZMAP_POLYGON_SQUARE:
            case ZMAP_POLYGON_POINTING:
            case ZMAP_POLYGON_DOUBLE_POINTING:
            case ZMAP_POLYGON_DIAMOND:
            default:
              points->coords[x_coord] = x1 + ((x2 - x1) / 2);
              break;
            }
          break;
        case POINT_BOW_STARBOARD_FORWARD:
          points->coords[x_coord] = x1;
          switch(form)
            {
            case ZMAP_POLYGON_DIAMOND:
            case ZMAP_POLYGON_DOUBLE_POINTING:
            case ZMAP_POLYGON_POINTING:
            case ZMAP_POLYGON_TRIANGLE_STAR:
            case ZMAP_POLYGON_TRAPEZOID_STAR:
              points->coords[y_coord] = y2 - ZMAP_EXON_POINT_SIZE;
              break;
            case ZMAP_POLYGON_SQUARE:
            default:
              points->coords[y_coord] = y2;
              break;
            }
          break;
        case POINT_STERN_STARBOARD_FORWARD:
          points->coords[x_coord] = x1;
          switch(form)
            {
            case ZMAP_POLYGON_TRAPEZOID_STAR:
            case ZMAP_POLYGON_TRIANGLE_STAR:
              points->coords[y_coord] = y1 + ZMAP_EXON_POINT_SIZE;
              break;
            default:
              points->coords[y_coord] = y1;
              break;
            }
          break;
        case POINT_STERN_STERN_FORWARD:
          switch(form)
            {
            case ZMAP_POLYGON_DIAMOND:
              points->coords[x_coord] = x1 + ((x2 - x1) / 2);
              points->coords[y_coord] = y1 - ZMAP_EXON_POINT_SIZE;
              break;
            case ZMAP_POLYGON_DOUBLE_POINTING:
              points->coords[x_coord] = x1 + ((x2 - x1) / 2);
              points->coords[y_coord] = y1 + ZMAP_EXON_POINT_SIZE;
              break;
            case ZMAP_POLYGON_TRIANGLE_PORT:
            case ZMAP_POLYGON_TRAPEZOID_PORT:
              points->coords[x_coord] = x1;
              points->coords[y_coord] = y1;
              break;
            case ZMAP_POLYGON_TRIANGLE_STAR:
            case ZMAP_POLYGON_TRAPEZOID_STAR:
              points->coords[x_coord] = x2;
              points->coords[y_coord] = y1;
              break;
            case ZMAP_POLYGON_SQUARE:
            case ZMAP_POLYGON_POINTING:
            default:
              points->coords[x_coord] = x1 + ((x2 - x1) / 2);
              points->coords[y_coord] = y1;
              break;
            }
          break;
        case POINT_STERN_PORT_FORWARD:
          points->coords[x_coord] = x2;
          switch(form)
            {
            case ZMAP_POLYGON_TRAPEZOID_PORT:
            case ZMAP_POLYGON_TRIANGLE_PORT:
              points->coords[y_coord] = y1 + ZMAP_EXON_POINT_SIZE;
              break;
            default:
              points->coords[y_coord] = y1;
              break;
            }
          break;
          /* REVERSE POINTS */
        case POINT_BOW_PORT_REVERSE:
          points->coords[x_coord] = x1;
          switch(form)
            {
            case ZMAP_POLYGON_DIAMOND:
            case ZMAP_POLYGON_DOUBLE_POINTING:
            case ZMAP_POLYGON_POINTING:
            case ZMAP_POLYGON_TRIANGLE_PORT:
            case ZMAP_POLYGON_TRAPEZOID_PORT:
              points->coords[y_coord] = y1 + ZMAP_EXON_POINT_SIZE;
              break;
            case ZMAP_POLYGON_SQUARE:
            default:
              points->coords[y_coord] = y1;
              break;
            }
          break;
        case POINT_BOW_BOW_REVERSE:
          points->coords[y_coord] = y1;
          switch(form)
            {
            case ZMAP_POLYGON_TRIANGLE_PORT:
            case ZMAP_POLYGON_TRAPEZOID_PORT:
              points->coords[x_coord] = x2;
              break;
            case ZMAP_POLYGON_TRIANGLE_STAR:
            case ZMAP_POLYGON_TRAPEZOID_STAR:
              points->coords[x_coord] = x1;
              break;
            case ZMAP_POLYGON_SQUARE:
            case ZMAP_POLYGON_POINTING:
            case ZMAP_POLYGON_DOUBLE_POINTING:
            case ZMAP_POLYGON_DIAMOND:
            default:
              points->coords[x_coord] = x1 + ((x2 - x1) / 2);
              break;
            }
          break;
        case POINT_BOW_STARBOARD_REVERSE:
          points->coords[x_coord] = x2;
          switch(form)
            {
            case ZMAP_POLYGON_DIAMOND:
            case ZMAP_POLYGON_DOUBLE_POINTING:
            case ZMAP_POLYGON_POINTING:
            case ZMAP_POLYGON_TRIANGLE_STAR:
            case ZMAP_POLYGON_TRAPEZOID_STAR:
              points->coords[y_coord] = y1 + ZMAP_EXON_POINT_SIZE;
              break;
            case ZMAP_POLYGON_SQUARE:
            default:
              points->coords[y_coord] = y1;
              break;
            }
          break;
        case POINT_STERN_STARBOARD_REVERSE:
          points->coords[x_coord] = x2;
          switch(form)
            {
            case ZMAP_POLYGON_TRAPEZOID_STAR:
            case ZMAP_POLYGON_TRIANGLE_STAR:
              points->coords[y_coord] = y2 - ZMAP_EXON_POINT_SIZE;
              break;
            default:
              points->coords[y_coord] = y2;
              break;
            }
          break;
        case POINT_STERN_STERN_REVERSE:
          switch(form)
            {
            case ZMAP_POLYGON_DIAMOND:
              points->coords[x_coord] = x1 + ((x2 - x1) / 2);
              points->coords[y_coord] = y2 + ZMAP_EXON_POINT_SIZE;
              break;
            case ZMAP_POLYGON_DOUBLE_POINTING:
              points->coords[x_coord] = x1 + ((x2 - x1) / 2);
              points->coords[y_coord] = y2 - ZMAP_EXON_POINT_SIZE;
              break;
            case ZMAP_POLYGON_TRIANGLE_PORT:
            case ZMAP_POLYGON_TRAPEZOID_PORT:
              points->coords[x_coord] = x2;
              points->coords[y_coord] = y2;
              break;
            case ZMAP_POLYGON_TRIANGLE_STAR:
            case ZMAP_POLYGON_TRAPEZOID_STAR:
              points->coords[x_coord] = x1;
              points->coords[y_coord] = y2;
              break;
            case ZMAP_POLYGON_SQUARE:
            case ZMAP_POLYGON_POINTING:
            default:
              points->coords[x_coord] = x1 + ((x2 - x1) / 2);
              points->coords[y_coord] = y2;
              break;
            }
          break;
        case POINT_STERN_PORT_REVERSE:
          points->coords[x_coord] = x1;
          switch(form)
            {
            case ZMAP_POLYGON_TRAPEZOID_PORT:
            case ZMAP_POLYGON_TRIANGLE_PORT:
              points->coords[y_coord] = y2 - ZMAP_EXON_POINT_SIZE;
              break;
            default:
              points->coords[y_coord] = y2;
              break;
            }
          break;
          /* Unknown and defaults are errors */
        case POINT_UNKNOWN_FORWARD:
        default:
          zMapAssert("Error: Unknown point type." == 0);
          break;
          
        }
    }

  item = foo_canvas_item_new(FOO_CANVAS_GROUP(grp),
			     foo_canvas_polygon_get_type(),
                             "points", points,
			     "outline_color_gdk", border,
			     "fill_color_gdk", fill,
			     "width_pixels", line_width,
			     NULL) ;
  /* We Should be doing the long item check here! but we don't get access to that :( */
  /* Mainly cos we know the longest distance here and don't want to pass it elsewhere */
  foo_canvas_points_free(points);

#ifdef NOT_JUST_YET________________________________________________
  *fwidthA = x1; *fwidthB = x2; *fstart = y1; *fend = y2;
#endif

  return item;
}


/* It may be good not to specify a width here as well (see zMapDrawBox) but I haven't
 * experimented yet. */
FooCanvasItem *zMapDrawLine(FooCanvasGroup *group, double x1, double y1, double x2, double y2, 
			    GdkColor *colour, guint line_width)
{
  FooCanvasItem *item = NULL ;
  FooCanvasPoints *points ;

  /* allocate a new points array */
  points = foo_canvas_points_new(2) ;
				                                            
  /* fill out the points */
  points->coords[0] = x1 ;
  points->coords[1] = y1 ;
  points->coords[2] = x2 ;
  points->coords[3] = y2 ;

  /* draw the line */
  item = foo_canvas_item_new(group,
			     foo_canvas_line_get_type(),
			     "points", points,
			     "fill_color_gdk", colour,
			     "width_pixels", line_width,
			     NULL);
		    
  /* free the points array */
  foo_canvas_points_free(points) ;

  return item ;
}


/* It may be good not to specify a width here as well (see zMapDrawBox) but I haven't
 * experimented yet. */
FooCanvasItem *zMapDrawPolyLine(FooCanvasGroup *group, FooCanvasPoints *points,
				GdkColor *colour, guint line_width)
{
  FooCanvasItem *item = NULL ;

  /* draw the line */
  item = foo_canvas_item_new(group,
			     foo_canvas_line_get_type(),
			     "points", points,
			     "fill_color_gdk", colour,
			     "width_pixels", line_width,
			     "join_style", GDK_JOIN_BEVEL,
			     "cap_style", GDK_CAP_BUTT,
			     NULL);
		    
  return item ;
}


/* WE PROBABLY SHOULDN'T USE THE WIDTH PIXELS BELOW AS IT RESULTS IN SLOWER DRAWING, TRY
 * REMOVING AT SOME TIME.... */

/* We use the canvas, get the root group and create it there so we can
 * raise to top and always see it. It'll hide behind widgets though,
 * but we don't have any of those.
 */
FooCanvasItem *zMapDrawRubberbandCreate(FooCanvas *canvas)
{
  FooCanvasItem *rubberband;
  rubberband = foo_canvas_item_new (foo_canvas_root(FOO_CANVAS(canvas)),
                             foo_canvas_rect_get_type (),
                             "outline_color", "black",
                             "width_pixels", 1,
                             NULL);
  return rubberband;
}

void zMapDrawRubberbandResize(FooCanvasItem *band, 
                              double origin_x, double origin_y, 
                              double current_x, double current_y
                              )
{
  foo_canvas_item_hide(band);
  foo_canvas_item_set(band,
                      "x1", MIN(origin_x, current_x),
                      "y1", MIN(origin_y, current_y),
                      "x2", MAX(origin_x, current_x),
                      "y2", MAX(origin_y, current_y),
                      NULL);
  foo_canvas_item_show(band);
  foo_canvas_item_raise_to_top(band);
  return ;
}

FooCanvasItem *zMapDrawHorizonCreate(FooCanvas *canvas)
{
  FooCanvasItem *line;

  line = foo_canvas_item_new (foo_canvas_root(FOO_CANVAS(canvas)),
                             foo_canvas_line_get_type (),
                             "fill_color", "black",
                             "width_pixels", 1,
                             NULL);
  return line;
}

void zMapDrawHorizonReposition(FooCanvasItem *line, double current_y)
{
  FooCanvasPoints *points;
  double x1, x2, y1, y2;
  double width;

  foo_canvas_get_scroll_region(line->canvas, &x1, &y1, &x2, &y2);
  /* A little unsure if we ever need to use the x2 from above 
   * and can depend on the width below as we won't see the line 
   * past the edge of the widget. horizontal scrolling?? */
  width = GTK_WIDGET(line->canvas)->allocation.width;

  /* allocate a new points array */
  points = foo_canvas_points_new(2) ;
				                                            
  /* fill out the points */
  points->coords[0] = x1;
  points->coords[1] = current_y ;
  points->coords[2] = MAX(x2, width);
  points->coords[3] = current_y ;

  foo_canvas_item_hide(line);
  foo_canvas_item_set(line,
                      "points", points,
                      NULL);
  foo_canvas_item_show(line);
  foo_canvas_item_raise_to_top(line);
  foo_canvas_points_free(points) ;
  return ;
}

FooCanvasGroup *zMapDrawToolTipCreate(FooCanvas *canvas)
{
  FooCanvasGroup *tooltip;
  FooCanvasItem *box, *tip;
  GdkColor border, bgcolor;
  /* Parse colours */
  gdk_color_parse("#000000", &border);
  gdk_color_parse("#e2e2de", &bgcolor);
  /* tooltip group */
  tooltip = 
    FOO_CANVAS_GROUP(foo_canvas_item_new(foo_canvas_root(FOO_CANVAS(canvas)),
                                         foo_canvas_group_get_type(),
                                         "x", 0.0,
                                         "y", 0.0,
                                         NULL));
  /* Create the item for the background/outline of the tip */
  box = foo_canvas_item_new(tooltip,
			     foo_canvas_rect_get_type(),
			     "x1", 0.0, "y1", 0.0,
			     "x2", 1.0, "y2", 1.0,
			     "outline_color_gdk", &border,
			     "fill_color_gdk", &bgcolor,
			     NULL) ;
  g_object_set_data(G_OBJECT(tooltip), "tooltip_box", box);
  /* Create the item for the text of the tip */
  tip = foo_canvas_item_new(tooltip,
                            foo_canvas_text_get_type(),
                            "x", 0.0, "y", 0.0,
                            "text", "",
                            "font", "Lucida Console",
                            "fill_color", "black",
                            NULL);
  g_object_set_data(G_OBJECT(tooltip), "tooltip_tip", tip);

  /* Hide the naked tooltip. */
  foo_canvas_item_hide(FOO_CANVAS_ITEM(tooltip));

  return tooltip;
}

void zMapDrawToolTipSetPosition(FooCanvasGroup *tooltip, double x, double y, char *text)
{
  FooCanvasItem *box, *tip;
  double x1, x2, y1, y2, height;
  int width;
  int extra = 4;

  width = strlen(text);
  /* Need to modify x slightly to avoid the cursor (16 pixels) */
  x    += 16.00 + extra + (width * 2.5); /* half width * 5.0 (text width)*/
  foo_canvas_item_hide(FOO_CANVAS_ITEM(tooltip));

  box = FOO_CANVAS_ITEM( g_object_get_data(G_OBJECT(tooltip), "tooltip_box") );
  tip = FOO_CANVAS_ITEM( g_object_get_data(G_OBJECT(tooltip), "tooltip_tip") );

  g_object_get(GTK_OBJECT(tip),
	       "FooCanvasText::text_height", &height,
	       NULL) ;

  foo_canvas_item_set(tip,
                      "text", text,
                      NULL);
  /* This seems to hold tighter to x than y so we add and subtract a bit */
  foo_canvas_item_get_bounds(tip, &x1, &y1, &x2, &y2);  
  foo_canvas_item_set(box,
                      "x1", x1 - extra,
                      "y1", y1,
                      "x2", x2 + extra,
                      "y2", y2,
                      NULL);
  /* Here we want about 70% of the height, so we get a space between
   * line and box. We need to work out above or below, based on the
   * current canvas position.
   */
  {
    double boxSize;
    int cx, cy, cBoxSize, vValue;
    FooCanvas *canvas;

    canvas  = FOO_CANVAS_ITEM(tooltip)->canvas;
    boxSize = height * 0.7;
    foo_canvas_w2c(canvas, x, y + (boxSize * 2.0), NULL, &cBoxSize);
    foo_canvas_w2c(canvas, x, y, &cx, &cy);
    cBoxSize -= cy;
    vValue    = canvas->layout.vadjustment->value;

    if((cy - vValue) > cBoxSize)
      y -= boxSize;
    else
      y += boxSize;
  }
  foo_canvas_item_set(FOO_CANVAS_ITEM(tooltip),
                      "x", x,
                      "y", y,
                      NULL);
  foo_canvas_item_raise_to_top(FOO_CANVAS_ITEM(tooltip));
  foo_canvas_item_show(FOO_CANVAS_ITEM(tooltip));
  
  return ;
}

/* *grp must be an empty group or one containing 8 line items at the
 * beginning.
 *
 * To make a multicolour highlight box just #define ZMAP_DRAW_HIGHLIGHT_MULTICOLOR
 */
//#define ZMAP_DRAW_HIGHLIGHT_MULTICOLOR
void zMapDrawHighlightTextRegion(FooCanvasGroup *grp,
                                 double firstX, double firstY, /* obvious! x = row, y = col */
                                 double lastX,  double lastY,
                                 FooCanvasItem *srcItem)
{
  double minX, maxX;
  ZMapDrawTextRowData trd = NULL;
  GdkColor default_bg_color, default_mq_color, *draw_color;
  ZMapDrawTextHighlightStyle 
    highlight_style = ZMAP_TEXT_HIGHLIGHT_BACKGROUND;

  zMapAssert(srcItem && (trd = zMapDrawGetTextItemData(srcItem)));

  gdk_color_parse("pink",  &default_bg_color);
  gdk_color_parse("black", &default_mq_color);

  minX     = 0.0;
  maxX     = MAX(trd->row_width, trd->char_width * trd->seq_truncated_idx);

  foo_canvas_item_hide(FOO_CANVAS_ITEM(grp));

  if(trd->highlight_style == ZMAP_TEXT_HIGHLIGHT_BACKGROUND)
    {
      if((draw_color = trd->background) == NULL)
        draw_color = &default_bg_color;
      drawHighlightBackgroundInGroup(grp,    draw_color, 
                                     firstX, firstY,
                                     lastX,  lastY, 
                                     minX,   maxX, 
                                     trd->char_height);
    }
  else if(trd->highlight_style == ZMAP_TEXT_HIGHLIGHT_MARQUEE)
    {
      if((draw_color = trd->outline) == NULL)
        draw_color = &default_mq_color;
      drawHighlightLinesInGroup(grp,    draw_color,
                                firstX, firstY, 
                                lastX,  lastY, 
                                minX,   maxX, 
                                trd->row_height);
    }
  else
    {
      if(highlight_style == ZMAP_TEXT_HIGHLIGHT_MARQUEE)
        drawHighlightLinesInGroup(grp,    &default_mq_color,
                                  firstX, firstY,
                                  lastX,  lastY,
                                  minX,   maxX, 
                                  trd->row_height);
      else if (highlight_style == ZMAP_TEXT_HIGHLIGHT_BACKGROUND)
        drawHighlightBackgroundInGroup(grp,    &default_bg_color,
                                       firstX, firstY,
                                       lastX,  lastY,
                                       minX,   maxX, 
                                       trd->row_height);
    }

  foo_canvas_item_lower_to_bottom(FOO_CANVAS_ITEM(grp));
  foo_canvas_item_show(FOO_CANVAS_ITEM(grp));

  return ;
}


/*! @} end of zmapdraw docs. */




/*
 *  ------------------- Internal functions -------------------
 */




static void drawHighlightBackgroundInGroup(FooCanvasGroup *parent,
                                           GdkColor *background,
                                           double offsetX1, double offsetY1,
                                           double offsetX2, double offsetY2,
                                           double minX,     double maxX,
                                           double dlength)
{
  FooCanvasPoints *points = NULL;
  GList *rects = NULL;
  GdkColor color;
  int i;
  double x1, y1, x2, y2;

  double ydiff = offsetY2 - offsetY1 + 1.0;

  rects    = parent->item_list;
  points   = foo_canvas_points_new(2);

  color = *background;
#ifdef RDS_DEBUG
  printf("drawHighlightBackgroundInGroup (%x): y1=%f, y2=%f, y2-y1=%f, dlength=%f\n", parent, offsetY1, offsetY2, ydiff, dlength);
#endif
  for(i = 0; i < 3; i++)
    {
      switch(i)
        {
        case 0:                   /* FIRST ROW */
          x1 = points->coords[0] = MIN(offsetX1, maxX);
          y1 = points->coords[1] = offsetY1;
          x2 = points->coords[2] = (ydiff  >= dlength ? maxX : offsetX2);
          //x2 = points->coords[2] = (offsetY2 - offsetY1 >= dlength ? maxX : MIN(offsetX2, maxX)); // 20060329
          y2 = points->coords[3] = offsetY1 + dlength;
#ifdef ZMAP_DRAW_HIGHLIGHT_MULTICOLOR
          gdk_color_parse("gold", &color);
#endif
          break;
        case 1:                   /* LARGE BLOCK */
          /* This shows up a good bug in the foocanvas
           * Without the MIN/MAX on calculating y1 and y2 the foocanvas doesn't invalidate 
           * the area correctly. Therefore we need to keep the sense of the y coords correct.
           */
          x1 = points->coords[0] = (ydiff >= dlength ? minX : offsetX1);
          y1 = points->coords[1] = MIN((offsetY1 + dlength), offsetY2);
          x2 = points->coords[2] = (ydiff >= dlength ? maxX : MIN(offsetX2, maxX));
          y2 = points->coords[3] = MAX((offsetY1 + dlength), offsetY2);
#ifdef ZMAP_DRAW_HIGHLIGHT_MULTICOLOR
          gdk_color_parse("red", &color);
#endif
          break;
        case 2:                   /* LAST ROW */
          x1 = points->coords[0] = (ydiff  >= dlength ? minX : offsetX1);
          y1 = points->coords[1] = offsetY2;
          x2 = points->coords[2] = MIN(offsetX2, maxX);
          y2 = points->coords[3] = offsetY2 + dlength;
#ifdef ZMAP_DRAW_HIGHLIGHT_MULTICOLOR
          gdk_color_parse("blue", &color);
#endif
          break;
        default:
          zMapAssertNotReached();
          break;
        }

      //      printf("drawHighlightBackgroundInGroup (%x): %d %f %f %f %f\n", 
      //             parent, i, x1, y1, x2, y2);
#ifdef RDS_DEBUG
      printf("drawHighlightBackgroundInGroup (%x): %d %f %f %f %f\n", 
             parent, i, 
             points->coords[0], 
             points->coords[1], 
             points->coords[2], 
             points->coords[3]);
#endif

      if(!rects)
        foo_canvas_item_new(parent,
                            foo_canvas_rect_get_type(),
                            "x1", points->coords[0],
                            "x2", points->coords[2],
                            "y1", points->coords[1],
                            "y2", points->coords[3],
                            "fill_color_gdk", &color,
                            "outline_color_gdk", NULL,
                            "width_units", 1.0,
                            NULL);
      else
        {
          if(FOO_IS_CANVAS_RECT(FOO_CANVAS_ITEM(rects->data)))
            foo_canvas_item_set(FOO_CANVAS_ITEM(rects->data),
                                "x1", points->coords[0],
                                "x2", points->coords[2],
                                "y1", points->coords[1],
                                "y2", points->coords[3],
                                NULL);
          rects = rects->next;
        }        

    }
  
  foo_canvas_points_free(points);

  return ;
}

static void drawHighlightLinesInGroup(FooCanvasGroup *parent,
                                      GdkColor *outline,
                                      double offsetX1, double offsetY1,
                                      double offsetX2, double offsetY2,
                                      double minX,     double maxX,
                                      double dlength)
{
  FooCanvasPoints *points = NULL;
  GList *lines = NULL;
  GdkColor color;
  int i;

  lines    = parent->item_list;
  points   = foo_canvas_points_new(2);

  color = *outline;

  for(i = 0; i < REGION_LAST_LINE; i++)
    {
      switch(i){
      case REGION_ROW_LEFT:
#ifdef ZMAP_DRAW_HIGHLIGHT_MULTICOLOR
        gdk_color_parse("red", &color);
#endif
        points->coords[0] = MIN(offsetX1, maxX);
        points->coords[1] = offsetY1;
        points->coords[2] = MIN(offsetX1, maxX) ;
        points->coords[3] = offsetY1 + dlength;
        break;
      case REGION_ROW_TOP:
#ifdef ZMAP_DRAW_HIGHLIGHT_MULTICOLOR
        gdk_color_parse("orange", &color);
#endif
        points->coords[0] = MIN(offsetX1, maxX);
        points->coords[1] = offsetY1;
        points->coords[2] = (offsetY2 - offsetY1  >= dlength ? maxX : MIN(offsetX2, maxX));
        points->coords[3] = offsetY1;
        break;  
      case REGION_ROW_RIGHT:
#ifdef ZMAP_DRAW_HIGHLIGHT_MULTICOLOR
        gdk_color_parse("gold", &color);
#endif
        points->coords[0] = MIN(offsetX2, maxX);
        points->coords[1] = offsetY2;
        points->coords[2] = MIN(offsetX2, maxX);
        points->coords[3] = offsetY2 + dlength;
        break;
      case REGION_ROW_BOTTOM:
#ifdef ZMAP_DRAW_HIGHLIGHT_MULTICOLOR
        gdk_color_parse("green", &color);
#endif
        points->coords[0] = MIN(offsetX2, maxX);
        points->coords[1] = offsetY2 + dlength;
        points->coords[2] = (offsetY2 - offsetY1  >= dlength ? minX : offsetX1);
        points->coords[3] = offsetY2 + dlength;
        break;
      case REGION_ROW_EXTENSION_LEFT:
#ifdef ZMAP_DRAW_HIGHLIGHT_MULTICOLOR
        gdk_color_parse("blue", &color);
#endif
        points->coords[0] = (offsetY2 - offsetY1  >= dlength ? minX : offsetX1);
        points->coords[1] = offsetY1 + dlength;
        points->coords[2] = (offsetY2 - offsetY1  >= dlength ? minX : offsetX1);
        points->coords[3] = offsetY2 + dlength;
        break;
      case REGION_ROW_EXTENSION_RIGHT:
#ifdef ZMAP_DRAW_HIGHLIGHT_MULTICOLOR
        gdk_color_parse("grey", &color);
#endif
        points->coords[0] = (offsetY2 - offsetY1  >= dlength ? maxX : offsetX2);
        points->coords[1] = offsetY1;
        points->coords[2] = (offsetY2 - offsetY1  >= dlength ? maxX : offsetX2);
        points->coords[3] = offsetY2;
        break;
      case REGION_ROW_JOIN_TOP:
#ifdef ZMAP_DRAW_HIGHLIGHT_MULTICOLOR
        gdk_color_parse("brown", &color);
#endif
        points->coords[0] = MIN(offsetX1, maxX);
        points->coords[1] = offsetY1 + dlength;
        points->coords[2] = (offsetY2 - offsetY1  >= dlength ? minX : offsetX1);
        points->coords[3] = offsetY1 + dlength;
        break;
      case REGION_ROW_JOIN_BOTTOM:
#ifdef ZMAP_DRAW_HIGHLIGHT_MULTICOLOR
        gdk_color_parse("pink", &color);
#endif
        points->coords[0] = MIN(offsetX2, maxX);
        points->coords[1] = offsetY2;
        points->coords[2] = (offsetY2 - offsetY1 >= dlength ? maxX : MIN(offsetX2, maxX));
        points->coords[3] = offsetY2;
        break;
      default:
        zMapAssertNotReached();
        break;
      }
      
      if(!lines)
        foo_canvas_item_new(parent,
                            foo_canvas_line_get_type(),
                            "points", points,
                            "line_style", GDK_LINE_ON_OFF_DASH,
                            "fill_color_gdk", &color,
                            "width_units", 1.0,
                            NULL);
      else
        {
          if(FOO_IS_CANVAS_LINE(FOO_CANVAS_ITEM(lines->data)))
            foo_canvas_item_set(FOO_CANVAS_ITEM(lines->data),
                                "points", points,
                                NULL);
          lines = lines->next;
        }        
    }
  foo_canvas_points_free(points);

  return ;
}


ZMapDrawTextRowData zMapDrawGetTextItemData(FooCanvasItem *item)
{
  ZMapDrawTextRowData trd = NULL;

  if(item && FOO_IS_CANVAS_TEXT(item))
    trd = (ZMapDrawTextRowData)g_object_get_data(G_OBJECT(item),
                                                 ZMAP_DRAW_TEXT_ROW_DATA_KEY);

  return trd;
}

FooCanvasItem *zMapDrawRowOfText(FooCanvasGroup *group,
                                 PangoFontDescription *fixed_font,
                                 char *fullText, 
                                 ZMapDrawTextIterator iterator)
{
  FooCanvasItem *item = NULL;
  char *errText = "ERROR";
  ZMapDrawTextRowData curr_data = NULL;
  int char_count_inc, char_count_ex, curr_idx, iteration, offset;
  gboolean good = TRUE;

  iteration = iterator->iteration;

  curr_data = (ZMapDrawTextRowData)(iterator->row_data);
  curr_data+= iteration;

  iterator->y  = iteration * iterator->char_height;
  iterator->y += offset = iterator->offset_start;

  g_string_truncate(iterator->row_text, 0);

  fullText     = iterator->wrap_text;

  char_count_ex = iterator->truncate_at;

  /* Protect from overflowing the text */
  if((curr_idx = iteration * iterator->cols) <= (iterator->lastPopulatedCell))
    fullText  += curr_idx;
  else if(iterator->lastPopulatedCell - curr_idx < iterator->truncate_at)
    {
      fullText  += curr_idx;
      char_count_ex = iterator->lastPopulatedCell - curr_idx;
    }
  else
    {
      fullText      = errText;
      char_count_ex = strlen(errText);
      good          = FALSE;
    }

  g_string_truncate(iterator->row_text, 0);
  char_count_inc = char_count_ex;

  if(fullText && *fullText)
    {
      g_string_append_len(iterator->row_text, fullText, char_count_ex);

      if(iterator->truncated)
        {
          char_count_inc = char_count_ex + 3;
          g_string_append(iterator->row_text, "...");
        }
    }

  if(iterator->row_text->str)
    {
#ifdef RDS_DONT_INCLUDE_UNUSED
      g_string_append_printf(iterator->row_text, " %d", iteration);
#endif

      item = foo_canvas_item_new(group,
                                 foo_canvas_text_get_type(),
                                 "x",          iterator->x,
                                 "y",          iterator->y,
                                 "text",       iterator->row_text->str,
                                 "anchor",     GTK_ANCHOR_NW,
                                 "font_desc",  fixed_font,
                                 "fill_color_gdk", iterator->foreground,
                                 NULL);

      /* Now we know most of what we need to store in curr_data */
      curr_data->curr_first_index  = iterator->offset_start;
      curr_data->seq_index_start   = curr_idx + iterator->index_start;
      curr_data->seq_index_end     = curr_data->seq_index_start + iterator->cols;
      curr_data->seq_truncated_idx = iterator->truncate_at;
      curr_data->chars_on_screen   = char_count_inc;
      curr_data->chars_drawn       = char_count_ex;
      curr_data->char_width        = iterator->char_width;
      curr_data->char_height       = iterator->char_height;
      curr_data->background        = iterator->background;
      curr_data->outline           = iterator->outline;
      curr_data->highlight_style   = (iterator->background 
                                      ? ZMAP_TEXT_HIGHLIGHT_BACKGROUND 
                                      : ZMAP_TEXT_HIGHLIGHT_MARQUEE);

      foo_canvas_item_get_bounds(item, 
                                 &(iterator->x1), 
                                 &(iterator->y1), 
                                 &(iterator->x2), 
                                 &(iterator->y2));
      curr_data->row_height = iterator->y2 - iterator->y1;
      curr_data->row_width  = iterator->x2 - iterator->x1;

      g_object_set_data_full(G_OBJECT(item), 
                             ZMAP_DRAW_TEXT_ROW_DATA_KEY, 
                             curr_data,
                             NULL); /* this needs setting, but this is an element of an array??? */
#ifdef RDS_DONT_INCLUDE_UNUSED
      foo_canvas_item_set(item,
                          "clip",        TRUE,
                          "clip_width",  iterator->x2 - iterator->x1,
                          "clip_height", iterator->y2 - iterator->y1,
                          NULL
                          );
#endif 
      foo_canvas_item_raise_to_top(item);

    }

  return item;
}




/* ========================================================================== */
/* INTERNAL */
/* ========================================================================== */

