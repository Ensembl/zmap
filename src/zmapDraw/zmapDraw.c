/*  File: zmapDraw.c
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 * Last edited: Jul 14 15:43 2005 (rds)
 * Created: Wed Oct 20 09:19:16 2004 (edgrif)
 * CVS info:   $Id: zmapDraw.c,v 1.31 2005-07-14 15:24:49 rds Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <glib.h>
#include <ZMap/zmapDraw.h>
#include <math.h>

#define ZMAP_SCALE_MINORS_PER_MAJOR 10
#define ZMAP_FORCE_FIVES TRUE

/* Just a collection of ints, boring but makes it easier */
typedef struct _ZMapScaleBarStruct
{
  int base;                     /* One of 1 1e3 1e6 1e9 1e12 1e15 */
  int major;                    /* multiple of base */
  int minor;                    /* major / ZMAP_SCALE_MINORS_PER_MAJOR */
  char *unit;                   /* One of bp k M G T P */

  gboolean force_multiples_of_five;
  double zoom_factor;

  int start;
  int end;
} ZMapScaleBarStruct, *ZMapScaleBar;


static ZMapScaleBar createScaleBar_start_end_zoom_height(int start, int end, double zoom, double line);
static void drawScaleBar(ZMapScaleBar scaleBar, FooCanvasGroup *group);
static void destroyScaleBar(ZMapScaleBar scaleBar);


FooCanvasItem *zMapDisplayText(FooCanvasGroup *group, char *text, char *colour,
			       double x, double y)
{
  FooCanvasItem *item = NULL ;

  item = foo_canvas_item_new(group,
			     FOO_TYPE_CANVAS_TEXT,
			     "x", x, "y", y,
			     "text", text,
			     "fill_color", colour,
			     NULL);

  return item ;
}



/* Note that we don't specify "width_units" or "width_pixels" for the outline
 * here because by not doing so we get a one pixel wide outline that is
 * completely enclosed within the item. If you specify either width, you end
 * up with items that are larger than you expect because the outline is drawn
 * centred on the edge of the rectangle. */
FooCanvasItem *zMapDrawBox(FooCanvasItem *group, 
			   double x1, double y1, double x2, double y2, 
			   GdkColor *border_colour, GdkColor *fill_colour)
{
  FooCanvasItem *item = NULL ;
  int line_width = 1 ;

  item = foo_canvas_item_new(FOO_CANVAS_GROUP(group),
			     foo_canvas_rect_get_type(),
			     "x1", x1, "y1", y1,
			     "x2", x2, "y2", y2,
			     "outline_color_gdk", border_colour,
			     "fill_color_gdk", fill_colour,
			     NULL) ;

  return item;                                                                       
}


/* It may be good not to specify a width here as well (see zMapDrawBox) but I haven't
 * experimented yet. */
FooCanvasItem *zMapDrawLine(FooCanvasGroup *group, double x1, double y1, double x2, double y2, 
			    GdkColor *colour, double thickness)
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
			     "width_units", thickness,
			     NULL);
		    
  /* free the points array */
  foo_canvas_points_free(points) ;

  return item ;
}


/* It may be good not to specify a width here as well (see zMapDrawBox) but I haven't
 * experimented yet. */
FooCanvasItem *zMapDrawPolyLine(FooCanvasGroup *group, FooCanvasPoints *points,
				GdkColor *colour, double thickness)
{
  FooCanvasItem *item = NULL ;

  /* draw the line */
  item = foo_canvas_item_new(group,
			     foo_canvas_line_get_type(),
			     "points", points,
			     "fill_color_gdk", colour,
			     "width_units", thickness,
			     "join_style", GDK_JOIN_BEVEL,
			     "cap_style", GDK_CAP_BUTT,
			     NULL);
		    
  return item ;
}


                                                                                
/* This routine should not be enhanced or debugged (it does not draw the scale in a good
 * way, the units are often bizarre, e.g. 10001, 20001 etc.). In the future the scale
 * will be in a separate window. */

/* This function NEEDS Seq2CanExt output coords i.e. seqstart -> seqend + 1 */
FooCanvasItem *zMapDrawScale(FooCanvas *canvas,
			     double zoom_factor, 
			     int start, int end)
{
  FooCanvasItem *group = NULL ;
  int width = 0 ;
  GdkColor black, white, yellow ;
  double x1, y1, x2, y2, height;
  ZMapScaleBar scaleBar = NULL;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* If the scrolled_region has been cropped, we need to crop the scalebar too */
  /* This shouldn't be needed!!! we should just draw from the start, we know what the zoom is!! */
  foo_canvas_get_scroll_region(canvas, &x1, &y1, &x2, &y2);
  if (start < y1)
      start = y1;
  if (end > y2)
      end = y2;
#endif

  group = foo_canvas_item_new(foo_canvas_root(canvas),
			      foo_canvas_group_get_type(),
			      "x", 0.0,
			      "y", 0.0,
			      NULL) ;

  zMapDrawGetTextDimensions(FOO_CANVAS_GROUP(group), NULL, &height);

  scaleBar = createScaleBar_start_end_zoom_height(start, end, zoom_factor, height);

  drawScaleBar(scaleBar, FOO_CANVAS_GROUP(group));

  destroyScaleBar(scaleBar);

  return group ;
}

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
                      "x1", MINVAL(origin_x, current_x),
                      "y1", MINVAL(origin_y, current_y),
                      "x2", MAXVAL(origin_x, current_x),
                      "y2", MAXVAL(origin_y, current_y),
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

  foo_canvas_get_scroll_region(line->canvas, &x1, &y1, &x2, &y2);

  /* allocate a new points array */
  points = foo_canvas_points_new(2) ;
				                                            
  /* fill out the points */
  points->coords[0] = x1;
  points->coords[1] = current_y ;
  points->coords[2] = x2;
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

/* Find out the text size for a group. */
void zMapDrawGetTextDimensions(FooCanvasGroup *group, double *width_out, double *height_out)
{
  double width = -1.0, height = -1.0 ;
  FooCanvasItem *item ;

  item = foo_canvas_item_new(group,
			     FOO_TYPE_CANVAS_TEXT,
			     "x", -400.0, "y", 0.0, "text", "dummy",
			     NULL);

  g_object_get(GTK_OBJECT(item),
	       "FooCanvasText::text_width", &width,
	       "FooCanvasText::text_height", &height,
	       NULL) ;

  gtk_object_destroy(GTK_OBJECT(item));

  if (width_out)
    *width_out = width ;
  if (height_out)
    *height_out = height ;

  return ;
}

int zMapDrawBorderSize(FooCanvasGroup *group)
{
  int size = 0;
  double height, zoom;

  zMapDrawGetTextDimensions(group, NULL, &height);

  g_object_get(GTK_OBJECT(group),
	       "FooCanvas::pixels_per_unit", &zoom,
	       NULL) ;

  size = ceil(height / zoom) + 1;

  return size;
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
			     FOO_TYPE_CANVAS_TEXT,
			     "x", 0.0, "y", 0.0,
			     "text", "",
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
  double x1, x2, y1, y2, height, n;
  int width;
  n     = 7.0;
  width = strlen(text);
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
                      "x1", x1 - 2,
                      "y1", y1,
                      "x2", x2 + 2,
                      "y2", y2,
                      NULL);
  /* Here we want about 70% of the height, so we get a space between
   * line and box. We have a problem about the cursor though... (x
   * axis and positioning the tt far enough right). n * width does
   * not work as it make the tt "jump" when text changes length!
   * Just using x centers the text on the cursor.
   */
  foo_canvas_item_set(FOO_CANVAS_ITEM(tooltip),
                      "x", x,
                      "y", y - (height * 0.7),
                      NULL);
  foo_canvas_item_raise_to_top(FOO_CANVAS_ITEM(tooltip));
  foo_canvas_item_show(FOO_CANVAS_ITEM(tooltip));
  
  return ;
}
/* ========================================================================== */
/* INTERNAL */
/* ========================================================================== */

static ZMapScaleBar createScaleBar_start_end_zoom_height(int start, int end, double zoom, double line)
{
  ZMapScaleBar scaleBar = NULL;
  int majorUnits[]      = {1   , 1000, 1000000, 1000000000, 0};
  char *majorAlpha[]    = {"", " k" , " M"    , " G"};
  int unitIndex         = 0;
  int speed_factor      = 4;
  int *iter;
  double basesPerPixel;
  int minorsPerMajor = 10;

  int majorSize, minorSize, diff, modulus;
  int majorCount, i, tmp;
  int absolute_min, lineheight;
  int basesBetween;
  double maxMajorCount;
  
  scaleBar = g_new0(ZMapScaleBarStruct, 1);
  scaleBar->start = start;
  scaleBar->end   = end;
  scaleBar->force_multiples_of_five = ZMAP_FORCE_FIVES;
  scaleBar->zoom_factor = zoom;

  /* line * zoom is constant @ 14 on my machine, 
   * simply increasing this decreases the number of majors (makes it faster),
   * hence inclusion of 'speed_factor'. May want to refine what looks good
   * 2 or 4 are reasonable, while 10 is way OTT!
   * 1 gives precision issues when drawing the mnor ticks. At reasonable zoom
   * it gives about 1 pixel between minor ticks and this sometimes equates to 
   * two pixels (precision) which looks odd.
   */
  if(speed_factor >= 1)
    lineheight = ceil(line * zoom * speed_factor); 
  else
    lineheight = ceil(line * zoom); 

  /* Abosulte minimum of (ZMAP_SCALE_MINORS_PER_MAJOR * 2) + 1
   * Explain: pixel width for each line, plus one to see it + 1 for good luck!
   * Require 1 * text line height + 1 so they're not merged
   * 
   */

  diff          = scaleBar->end - scaleBar->start + 1;
  basesPerPixel = diff / (diff * scaleBar->zoom_factor);

  lineheight++;
  absolute_min = (ZMAP_SCALE_MINORS_PER_MAJOR * 2) + 1;
  basesBetween = floor((lineheight >= absolute_min ?
                        lineheight : absolute_min) * basesPerPixel);
  /* Now we know we can put a major tick every basesBetween pixels */

  for(i = 0, iter = majorUnits; *iter != 0; iter++, i++){
    int mod;
    mod = basesBetween % majorUnits[i];

    if(mod && mod != basesBetween)
      unitIndex = i;
    else if (basesBetween > majorUnits[i] && !mod)
      unitIndex = i;
  }

  /* Now we think we know what the major should be */
  majorSize  = majorUnits[unitIndex];
  scaleBar->base = majorSize;

  tmp = ceil((basesBetween / majorSize));

  /* This isn't very elegant, and is kind of a reverse of the previous
   * logic used */
  if(scaleBar->force_multiples_of_five == TRUE)
    {
      if(tmp <= 5)
        majorSize = 5  * majorSize;
      else if(tmp <= 10)
        majorSize = 10 * majorSize;
      else if (tmp <= 50)
        majorSize = 50 * majorSize;
      else if (tmp <= 100)
        majorSize = 100 * majorSize;
      else if (tmp <= 500)
        majorSize = 500 * majorSize;
      else if (tmp <= 1000)
        {
          majorSize = 1000 * majorSize;
          unitIndex++;
        }
    }
  else
    {
      majorSize = tmp * majorSize;
    }

  scaleBar->base  = majorUnits[unitIndex];
  scaleBar->major = majorSize;
  scaleBar->unit  = g_strdup( majorAlpha[unitIndex] );

  if(scaleBar->major >= ZMAP_SCALE_MINORS_PER_MAJOR)
    scaleBar->minor = majorSize / ZMAP_SCALE_MINORS_PER_MAJOR;
  else
    scaleBar->minor = 1;

  return scaleBar;
}

static void drawScaleBar(ZMapScaleBar scaleBar, FooCanvasGroup *group)
{
  int i, n, width = 0;
  GdkColor black, white, yellow ;
  double scale_left, scale_right, scale_mid;
  FooCanvasPoints *points ;

  scale_left  = 60.0;
  scale_right = scale_left + 10.0;
  scale_mid   = scale_left + ((scale_right - scale_left) / 2);

  gdk_color_parse("black", &black) ;
  gdk_color_parse("white", &white) ;
  gdk_color_parse("yellow", &yellow) ;

  n = ceil( (scaleBar->start % 
             (scaleBar->major < ZMAP_SCALE_MINORS_PER_MAJOR 
              ? ZMAP_SCALE_MINORS_PER_MAJOR 
              : scaleBar->major
              )
             ) / scaleBar->minor
            );
  i = (scaleBar->start - (scaleBar->start % scaleBar->minor));

  /* What I want to do here rather than worry about width of
   * characters is to have two groups/columns. One for the
   * numbers/units and one for the line/marks, but I haven't worked
   * out how best to right align the numbers/units without relying on
   * width.  Until then this works.
   */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("%d to %d\n", scaleBar->start, scaleBar->end);
#endif
  /* < rather than <= to avoid final minor as end is seq_end + 1 */
  for( ; i < scaleBar->end; i+=scaleBar->minor, n++)
    {
      /* More conditionals than I intended here... */
      char *digitUnit = NULL;
      if(n % ZMAP_SCALE_MINORS_PER_MAJOR) /* Minors */
        {
          if( i < scaleBar->start )
            zMapDrawLine(FOO_CANVAS_GROUP(group), 
                         scale_mid, scaleBar->start, 
                         scale_right, scaleBar->start, 
                         &black, 1.0) ;
          else if( i == scaleBar->start && n < 5)
            {                   /* n < 5 to stop overlap of digitUnit at some zooms */
              digitUnit = g_strdup_printf("%d", scaleBar->start);
              zMapDrawLine(FOO_CANVAS_GROUP(group), scale_mid, i, scale_right, i, &black, 1.0) ;
            }
          else
            zMapDrawLine(FOO_CANVAS_GROUP(group), scale_mid, i, scale_right, i, &black, 1.0) ;
        }
      else                      /* Majors */
        {
          if(i && i >= scaleBar->start)
            {
              zMapDrawLine(FOO_CANVAS_GROUP(group), scale_left, i, scale_right, i, &black, 1.0);
              digitUnit = g_strdup_printf("%d%s", 
                                          (i / scaleBar->base), 
                                          scaleBar->unit);
            }
          else
            {
              zMapDrawLine(FOO_CANVAS_GROUP(group), 
                           scale_mid, scaleBar->start, 
                           scale_right, scaleBar->start, 
                           &black, 1.0);
              digitUnit = g_strdup_printf("%d", scaleBar->start);
            }
        }
      /* =========================================================== */
      if(digitUnit)
        {
          width = strlen(digitUnit);
          zMapDisplayText(FOO_CANVAS_GROUP(group), 
                          digitUnit, "black", 
                          ((scale_left) - (5.0 * width)), 
                          (i < scaleBar->start ? scaleBar->start : i)); 
          g_free(digitUnit);
        }
    }

  /* draw the vertical line of the scalebar. */
    /* allocate a new points array */
  points = foo_canvas_points_new(2) ;
				                                            
  /* fill out the points */
  points->coords[0] = scale_right ;
  points->coords[1] = scaleBar->start ;
  points->coords[2] = scale_right ;
  points->coords[3] = scaleBar->end ;

  /* draw the line, unfortunately we need to use GDK_CAP_PROJECTING here to make it work */
  foo_canvas_item_new(group,
                      foo_canvas_line_get_type(),
                      "points", points,
                      "fill_color_gdk", &black,
                      "width_units", 1.0,
                      "cap_style", GDK_CAP_PROJECTING,
                      NULL);
		    
  /* free the points array */
  foo_canvas_points_free(points) ;

  return ;
}

static void destroyScaleBar(ZMapScaleBar scaleBar)
{
  if(scaleBar->unit)
    g_free(scaleBar->unit);

  g_free(scaleBar);

  return ;
}

