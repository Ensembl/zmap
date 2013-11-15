/*  File: zmapWindowZoomControl.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Functions to control zooming in and out, zoom min is
 *              defined as when the sequence occupies the window
 *              visible to the user, zoom max is when the sequence
 *              is zoomed in to show just one DNA base per line.
 *
 * Exported functions: See ZMap/zmapWindow.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>

#include <zmapWindowZoomControl_P.h>


static double getMinZoom(ZMapWindow window, ZMapWindowZoomControl control) ;
static void setZoomStatus(ZMapWindowZoomControl control);
static gboolean canZoomByFactor(ZMapWindowZoomControl control, double factor);
static ZMapWindowZoomControl controlFromWindow(ZMapWindow window);
static void textDimensionsOfFont(FooCanvasGroup *group,
                                 PangoFontDescription *font,
                                 double *width_out,
                                 double *height_out);
#ifdef BLAH_BLAH_BLAH
static void printControl(ZMapWindowZoomControl control);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* 
 *                Globals
 */

ZMAP_MAGIC_NEW(zoom_magic_G, );





/*
 *                External public routines
 */



ZMapWindowZoomStatus zMapWindowGetZoomStatus(ZMapWindow window)
{
  ZMapWindowZoomStatus status   = ZMAP_ZOOM_INIT;
  ZMapWindowZoomControl control = NULL;

  control = controlFromWindow(window);

  status  = control->status;

  return status;
}

double zMapWindowGetZoomMaxDNAInWrappedColumn(ZMapWindow window)
{
  ZMapWindowZoomControl control = NULL;
  double zoom_factor = 0.0, max_chars_per_row = 0.0;

  control = controlFromWindow(window);

  /* possibly requiring floor as we can't draw half chars  */
  max_chars_per_row = ZMAP_ZOOM_MAX_TEXT_COLUMN_WIDTH / control->textWidth;

  /* zoom_factor = control->maxZF / max_chars_per_row; */
  zoom_factor = control->maxZF / floor(max_chars_per_row - (ZMAP_WINDOW_TEXT_BORDER * 2.0));
  /* This is a bit of a fudge ATM, mainly as the correct   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
   * calculation results in dots still being shown.  This is because of a problem with my
   * zMapDrawRowOfText function. https://rt.sanger.ac.uk/rt/Ticket/Display.html?id=1888
   * **************************************************************************************/
  return zoom_factor;
}

double zMapWindowGetZoomFactor(ZMapWindow window)
{
  ZMapWindowZoomControl control = NULL;

  control = controlFromWindow(window);

  return control->zF;
}


double zMapWindowGetZoomMin(ZMapWindow window)
{
  ZMapWindowZoomControl control = NULL;

  control = controlFromWindow(window);

  return control->minZF ;
}


double zMapWindowGetZoomMax(ZMapWindow window)
{
  ZMapWindowZoomControl control = NULL;

  control = controlFromWindow(window);

  return control->maxZF ;
}


double zMapWindowGetZoomMagnification(ZMapWindow window)
{
  ZMapWindowZoomControl control = NULL ;
  double mag ;

  control = controlFromWindow(window) ;

  /* Not sure what this actually needs to be calculating,
   * but a ratio might be nice rather than just random numbers */
  mag = control->maxZF / control->zF ;

  return mag ;
}


double zMapWindowGetZoomMagAsBases(ZMapWindow window)
{
  ZMapWindowZoomControl control = NULL ;
  double bases ;

  control = controlFromWindow(window) ;

  bases = control->pix2mmy  / control->zF ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("min/max/curr zoom: %g %g %g\tbases: %g\n",  control->minZF, control->maxZF, control->zF, bases) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return bases ;
}


/* Not sure this is right, copy with pango_font_description_copy and merge??? */
PangoFontDescription *zMapWindowZoomGetFixedWidthFontInfo(ZMapWindow window,
                                                          double *width_out,
                                                          double *height_out)
{
  ZMapWindowZoomControl control = NULL;
  control = controlFromWindow(window);

  if(control->status != ZMAP_ZOOM_INIT)
    {
      double ppux, ppuy, tmp;        /* pixels per unit */
      ppux = FOO_CANVAS(window->canvas)->pixels_per_unit_x;
      ppuy = FOO_CANVAS(window->canvas)->pixels_per_unit_y;

      if(width_out)
        {
          tmp = control->textWidth / ppux;
          *width_out = tmp;
        }
      if(height_out)
        {
          tmp = control->textHeight / ppuy;
          *height_out = tmp;
        }
    }

  return control->font_desc;
}




/*
 *               External package routines
 */


/* Create the Controller...
 * there must be a known fixed width font in order to be able to calculate the zoom factor.
 */
ZMapWindowZoomControl zmapWindowZoomControlCreate(ZMapWindow window)
{
  ZMapWindowZoomControl num_cruncher = NULL ;
  PangoFont *tmp_font = NULL ;
  PangoFontDescription *tmp_font_desc = NULL ;


  if(!zMapGUIGetFixedWidthFont(GTK_WIDGET(window->toplevel),
			       g_list_append(NULL, ZMAP_ZOOM_FONT_FAMILY),
			       ZMAP_ZOOM_FONT_SIZE, PANGO_WEIGHT_NORMAL,
			       &(tmp_font), &(tmp_font_desc)))
    {
      zMapLogCritical("%s", "Zoom Control Create failed to get fixed width font for zoom calculation"
		      " so zmap display is not possible.") ;
    }
  else
    {
      GdkScreen *screen ;
      double width_pixels, height_pixels, width_mm, height_mm ;

      num_cruncher = g_new0(ZMapWindowZoomControlStruct, 1);
      num_cruncher->magic = zoom_magic_G ;

      num_cruncher->font = tmp_font ;
      num_cruncher->font_desc = tmp_font_desc ;

      /* Make sure this is the 1:1 text_height 14 on my machine*/
      zmapWindowSetPixelxy(window, 1.0, 1.0);

      textDimensionsOfFont(foo_canvas_root(window->canvas),
			   num_cruncher->font_desc,
			   &(num_cruncher->textWidth),
			   &(num_cruncher->textHeight)) ;

      num_cruncher->maxZF   = num_cruncher->textHeight + (double)(ZMAP_WINDOW_TEXT_BORDER);
      num_cruncher->border  = num_cruncher->maxZF; /* This should _NOT_ be changed */
      num_cruncher->status  = ZMAP_ZOOM_INIT;

      //  num_cruncher->max_window_size = (user_set_limit ? user_set_limit : x_windows_limit)
      //  - ((num_cruncher->border * 2) * num_cruncher->maxZF);


      screen = gdk_screen_get_default() ;

      width_pixels = (double)gdk_screen_get_width(screen) ;
      height_pixels = (double)gdk_screen_get_height(screen) ;
      width_mm = (double)gdk_screen_get_width_mm(screen) ;
      height_mm = (double)gdk_screen_get_height_mm(screen) ;

      num_cruncher->pix2mmy = height_pixels / height_mm ;
      num_cruncher->pix2mmx = width_pixels / width_mm ;
    }

  return num_cruncher ;
}

/* Because we need to create and then set it up later */
void zmapWindowZoomControlInitialise(ZMapWindow window)
{
  ZMapWindowZoomControl control;

  control = controlFromWindow(window);

  control->minZF = getMinZoom(window, control) ;

  if (control->status == ZMAP_ZOOM_INIT)
    control->zF = control->minZF;

  /* Account for SHORT sequences (shorter than canvas height) */
  setZoomStatus(control); /* This alone probably doesn't do this!! */

  return ;
}

/* Should be called when either canvas height(size-allocate) or seq
 * length change. Does virtually the same as initialise... */
void zmapWindowZoomControlHandleResize(ZMapWindow window)
{
  ZMapWindowZoomControl control;
  double min;

  control = controlFromWindow(window);

  min = getMinZoom(window, control) ;

  /* Always need to reset this. */
  control->minZF = min;

  if (min != control->zF)
    {
      /* Try this here.....clunky hack....otherwise zMapWindowZoom() may not change
       * due to status erroneously being at min or max.... */
      control->status = ZMAP_ZOOM_MID ;

      zMapWindowZoom(window, min / control->zF);
    }

  setZoomStatus(control) ;

  return ;
}

/* This just does the maths and sets zoom factor and status
 * (Might need to work with setZoomStatus??? also ZMAP_ZOOM_FIXED)
 */
gboolean zmapWindowZoomControlZoomByFactor(ZMapWindow window, double factor)
{
  ZMapWindowZoomControl control;
  gboolean did_zoom = FALSE;
  double zoom;

  control = controlFromWindow(window);

  if(canZoomByFactor(control, factor))
    {
      /* Calculate the zoom. */
      zoom = control->zF * factor ;
      if (zoom <= control->minZF)
          control->zF = control->minZF ;
      else if (zoom >= control->maxZF)
          control->zF = control->maxZF ;
      else
          control->zF = zoom ;

      setZoomStatus(control);

      did_zoom = TRUE;
    }

  return did_zoom;
}

static void centeringLimitSpan2MaxOrDefaultAtZoom(double  max_pixels,
                                                  double  def,
                                                  double  zoom,
                                                  double *pointA,
                                                  double *pointB)
{
  double max_span, seq_span, new_span, canv_span, start, end, halfway;
  if (!max_pixels || !def || !pointA || !pointB)
    return ;

  start     = *pointA;
  end       = *pointB;
  max_span  = max_pixels;
  seq_span  = def * zoom;
  new_span  = end - start + 1;
  halfway   = start + (new_span / 2);
  new_span *= zoom;

  new_span  = (new_span >= max_span ?
               max_span : (seq_span > max_span ?
                           max_span
                           : seq_span)) ;

  canv_span = new_span / zoom;

  /* Need to centre on the new zoom, quick bit of math */

  *pointA = (halfway - (canv_span / 2));
  *pointB = (halfway + (canv_span / 2));

  return ;
}

void zmapWindowZoomControlGetScrollRegion(ZMapWindow window,
                                          double *x1_out, double *y1_out,
                                          double *x2_out, double *y2_out)
{
  ZMapWindowZoomControl control;
  double x1, x2, y1, y2;

  control = controlFromWindow(window);
  x1 = x2 = y1 = y2 = 0.0;
  foo_canvas_get_scroll_region(window->canvas, &x1, &y1, &x2, &y2);
  //zmapWindowScrollRegionTool(window, &x1, &y1, &x2, &y2);

  if(x1_out && x2_out)
    {
      *x1_out = x1;
      *x2_out = x2;
      centeringLimitSpan2MaxOrDefaultAtZoom((double)(window->canvas_maxwin_size),
                                            (x2 - x1 + 1),
                                            1.0,
                                            x1_out, x2_out);
    }
  if(y1_out && y2_out)
    {
      *y1_out = y1;
      *y2_out = y2;
      centeringLimitSpan2MaxOrDefaultAtZoom((double)(window->canvas_maxwin_size),
                                            window->seqLength,
                                            control->zF,
                                            y1_out, y2_out);
      zmapWindowClampSpan(window, y1_out, y2_out);
    }

  return ;
}
double zmapWindowZoomControlLimitSpan(ZMapWindow window,
                                      double y1_inout, double y2_inout)
{
  ZMapWindowZoomControl control;
  double max_span, seq_span, new_span, canv_span;
  /* double x1, x2; */
  double y1, y2;

  control   = controlFromWindow(window);


  if(y1_inout && y2_inout)
    {
      y1        = y1_inout;
      y2        = y2_inout;
      max_span  = (double)(window->canvas_maxwin_size);
      seq_span  = window->seqLength * control->zF;

      new_span  = y2 - y1 + 1 ;
      new_span *= control->zF ;

      new_span  = (new_span >= max_span ?
                   max_span : (seq_span > max_span ?
                               max_span
                               : seq_span)) ;

      canv_span = new_span / control->zF;
    }

  return canv_span;
}


void zmapWindowZoomControlCopyTo(ZMapWindowZoomControl orig, ZMapWindowZoomControl new)
{
  new->zF     = orig->zF;
  setZoomStatus(new);
  return ;
}

void zmapWindowGetBorderSize(ZMapWindow window, double *border)
{
  ZMapWindowZoomControl control = NULL;
  double b = 0.0;

  control = controlFromWindow(window);

  if(control->status != ZMAP_ZOOM_INIT)
    b = control->border / control->zF;

  *border = b;

  return ;
}



/*
 *                             Internal routines 
 */


/* This is here in case window struct changes name of control */
static ZMapWindowZoomControl controlFromWindow(ZMapWindow window)
{
  ZMapWindowZoomControl control = NULL;

  if (!window || !window->zoom)
    return control ;

  control = window->zoom;

  return control;
}



/* Exactly what it says */
static double getMinZoom(ZMapWindow window, ZMapWindowZoomControl control)
{
  double canvas_height, border_height, zf = 0.0;

  /* These heights are in pixels */
  border_height = control->border * 2.0;
  canvas_height = GTK_WIDGET(window->canvas)->allocation.height;

  if (canvas_height <= 0)
    return zf ;
  /* rearrangement of
   *              canvas height
   * zf = ------------------------------
   *      seqLength + (text_height / zf)
   * could do with replacing allocation.height with adjuster->page_size
   */
  /*     canvas_height    two borders      length of sequence */
  zf = ( canvas_height - border_height ) / (window->seqLength + 1);

  return zf;
}


/* Private: from initialise and other public only?
 * Public: ????
 */
static void setZoomStatus(ZMapWindowZoomControl control)
{
  /* This needs to handle ZMAP_ZOOM_FIXED too!! */
  if (control->minZF >= control->maxZF)
    {
      control->status = ZMAP_ZOOM_FIXED ;
      control->zF     = control->maxZF ;
    }
  else if (control->zF <= control->minZF)
    control->status = ZMAP_ZOOM_MIN ;
  else if (control->zF >= control->maxZF)
    control->status = ZMAP_ZOOM_MAX ;
  else
    control->status = ZMAP_ZOOM_MID ;

  return ;
}

/* No point zooming if we can't */
static gboolean canZoomByFactor(ZMapWindowZoomControl control, double factor)
{
  gboolean can_zoom = TRUE;

  if (control->status == ZMAP_ZOOM_FIXED
      || (factor > 1.0 && control->status == ZMAP_ZOOM_MAX)
      || (factor < 1.0 && control->status == ZMAP_ZOOM_MIN))
    {
      can_zoom = FALSE;
    }
  return can_zoom;
}


/* Find out the text size for a group. */
static void textDimensionsOfFont(FooCanvasGroup *group,
                                 PangoFontDescription *font,
                                 double *width_out,
                                 double *height_out)
{
  double width = -1.0, height = -1.0 ;
  int iwidth = -1, iheight = -1;
  FooCanvasItem *item ;
  PangoLayout *playout;

  item = foo_canvas_item_new(group,
			     FOO_TYPE_CANVAS_TEXT,
			     "x",         0.0,
                             "y",         0.0,
                             "text",      "X",
                             "font_desc", font,
			     NULL);
  playout = FOO_CANVAS_TEXT(item)->layout;

  g_object_get(GTK_OBJECT(item),
	       "FooCanvasText::text_width", &width,
	       "FooCanvasText::text_height", &height,
	       NULL) ;

  pango_layout_get_pixel_size( playout , &iwidth, &iheight );

  width = (double)iwidth;
  height = (double)iheight;

  gtk_object_destroy(GTK_OBJECT(item));

  if (width_out)
    *width_out = width ;
  if (height_out)
    *height_out = height ;

  return ;
}


#ifdef BLAH_BLAH_BLAH
static void printControl(ZMapWindowZoomControl control)
{
  if (!control || !ZMAP_MAGIC_IS_VALID(zoom_magic_G, control->magic)) 
    return ;

  printf("Control:\n"
         " factor %f\n"
         " min %f\n"
         " max %f\n"
         " status %d\n",
         control->zF,
         control->minZF,
         control->maxZF,
         control->status
         );
  return ;
}
#endif /* BLAH_BLAH_BLAH */


