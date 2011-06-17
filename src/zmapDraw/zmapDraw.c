/*  File: zmapDraw.c
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Draw objects into the canvas, these may be unnecessary
 *              if they map closely enough to the foo_canvas calls.
 *
 * Exported functions: See ZMap/zmapDraw.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <zmapDraw_P.h>

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

#define make_clickable_bmp_width 16
#define make_clickable_bmp_height 4
static char make_clickable_bmp_bits[] =
  {
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00
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


FooCanvasItem *zMapDrawBox(FooCanvasGroup *group,
			   double x1, double y1, double x2, double y2,
			   GdkColor *border_colour, GdkColor *fill_colour,
			   guint line_width)
{
  FooCanvasItem *item = NULL ;

  item = zMapDrawBoxFull(group, FOO_CANVAS_GROUP_TOP,
			 x1, y1, x2, y2,
			 border_colour, fill_colour,
			 line_width) ;

  return item;
}




/* Note that we don't specify "width_units" or "width_pixels" for the outline
 * here because by not doing so we get a one pixel wide outline that is
 * completely enclosed within the item. If you specify either width, you end
 * up with items that are larger than you expect because the outline is drawn
 * centred on the edge of the rectangle. */
FooCanvasItem *zMapDrawBoxFull(FooCanvasGroup *group, FooCanvasGroupPosition position,
			       double x1, double y1, double x2, double y2,
			       GdkColor *border_colour, GdkColor *fill_colour,
			       guint line_width)
{
  FooCanvasItem *item = NULL ;
  static GdkBitmap *make_clickable_bmp = NULL ;

  zMapAssert(FOO_IS_CANVAS_GROUP(group)) ;

  if (fill_colour == NULL)
    {
      /* If fill_colour is NULL then a simple box is not clickable.
       * Events are never received on it. */
      /* Adding an empty stipple and using the border colour sorts
       * this out.  If the border colour is NULL too then we can't
       * help it, but you wouldn't see the box anyway! */
      if (!make_clickable_bmp)
        make_clickable_bmp = gdk_bitmap_create_from_data(NULL, &make_clickable_bmp_bits[0],
                                                         make_clickable_bmp_width, make_clickable_bmp_height) ;

      item = foo_canvas_item_new_position(FOO_CANVAS_GROUP(group),
					  foo_canvas_rect_get_type(), position,
					  "x1", x1, "y1", y1,
					  "x2", x2, "y2", y2,
					  "outline_color_gdk", border_colour,
					  "fill_stipple", make_clickable_bmp,
					  "fill_color_gdk", border_colour,
					  "width_pixels", line_width,
					  NULL) ;
    }
  else
      item = foo_canvas_item_new_position(FOO_CANVAS_GROUP(group),
					  foo_canvas_rect_get_type(), position,
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


FooCanvasItem *zMapDrawLine(FooCanvasGroup *group, double x1, double y1, double x2, double y2,
			    GdkColor *colour, guint line_width)
{
  FooCanvasItem *item = NULL ;

  item = zMapDrawLineFull(group, FOO_CANVAS_GROUP_TOP,
			  x1, y1, x2, y2,
			  colour, line_width) ;

  return item ;
}

/* It may be good not to specify a width here as well (see zMapDrawBox) but I haven't
 * experimented yet. */
FooCanvasItem *zMapDrawLineFull(FooCanvasGroup *group, FooCanvasGroupPosition position,
				double x1, double y1, double x2, double y2,
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

  item = foo_canvas_item_new_position(group,
				      foo_canvas_line_get_type(),
				      position,
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
  double input_x, input_y;
  double x_correction;
  gboolean keep_within_window = TRUE;
  int width;
  int extra = 4;

  input_x = x; input_y = y;

  width = strlen(text);
  /* Need to modify x slightly to avoid the cursor (16 pixels) */
  x_correction = 16.00 + extra + (width * 2.5); /* half width * 5.0 (text width)*/
  x += x_correction;

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

  if(keep_within_window)
    {
      FooCanvas *canvas;
      GtkAdjustment *hadjust, *vadjust;
      double tip_width, tip_height;
      double window_x, window_y;
      double window_min_x, window_max_x;
      double window_min_y, window_max_y;
      int hadj_min, hadj_max;
      int vadj_min, vadj_max;
      int cx, cy, tip_canvas_height;

      tip_width  = (x2 - x1 + 1.0) + (2.0 * extra);
      tip_height = (y2 - y1 + 1.0);

      canvas   = FOO_CANVAS_ITEM(tooltip)->canvas;

      hadjust  = canvas->layout.hadjustment;
      hadj_min = hadjust->value;
      hadj_max = hadjust->value + hadjust->page_size;

      vadjust  = canvas->layout.vadjustment;
      vadj_min = vadjust->value;
      vadj_max = vadjust->value + vadjust->page_size;

      foo_canvas_world_to_window(canvas, input_x, input_y, &window_x, &window_y);

      foo_canvas_world_to_window(canvas,
				 (double)hadj_min, (double)vadj_min,
				 &window_min_x, &window_min_y);

      foo_canvas_world_to_window(canvas,
				 (double)hadj_max, (double)vadj_max,
				 &window_max_x, &window_max_y);

      if(window_x < window_min_x)
	x = window_min_x + x_correction;
      else if(window_x + tip_width > window_max_x)
	x = window_max_x - x_correction;


      /* Here we want about 70% of the height, so we get a space between
       * line and box. We need to work out above or below, based on the
       * current canvas position.
       */
      foo_canvas_w2c(canvas, x, y + (tip_height * 1.4), NULL, &tip_canvas_height);
      foo_canvas_w2c(canvas, x, y, &cx, &cy);
      tip_canvas_height -= cy;

      if((cy - vadj_min) > tip_canvas_height)
	y -= tip_height;
      else
	y += tip_height;
    }

  foo_canvas_item_set(FOO_CANVAS_ITEM(tooltip),
                      "x", x,
                      "y", y,
                      NULL);
  foo_canvas_item_raise_to_top(FOO_CANVAS_ITEM(tooltip));
  foo_canvas_item_show(FOO_CANVAS_ITEM(tooltip));

  return ;
}

#ifdef RDS_DONT_INCLUDE
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
#endif

FooCanvasItem *zMapDrawTextWithFont(FooCanvasGroup *parent,
                                    char *some_text,
                                    PangoFontDescription *font_desc,
                                    double x, double y,
                                    GdkColor *text_color)
{
  FooCanvasItem *text_item = NULL;

  text_item = foo_canvas_item_new(parent,
                                  foo_canvas_text_get_type(),
                                  "x",              x,
                                  "y",              y,
                                  "text",           some_text,
                                  "anchor",         GTK_ANCHOR_NW,
                                  "font_desc",      font_desc,
                                  "fill_color_gdk", text_color,
                                  NULL);
#ifdef RAISE_TO_TOP_CAN_BE_SLOW
  foo_canvas_item_raise_to_top(text_item);
#endif /* RAISE_TO_TOP_CAN_BE_SLOW */

  return text_item;
}


/*! @} end of zmapdraw docs. */




/*
 *  ------------------- Internal functions -------------------
 */
