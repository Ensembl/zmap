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
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <math.h>
#include <zmapWindowZoomControl_P.h>


static double getMinZoom(ZMapWindow window);
static void setZoomStatus(ZMapWindowZoomControl control);
static gboolean canZoomByFactor(ZMapWindowZoomControl control, double factor);
static ZMapWindowZoomControl controlFromWindow(ZMapWindow window);
#ifdef BLAH_BLAH_BLAH
static void printControl(ZMapWindowZoomControl control);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
static void textDimensionsOfFont(FooCanvasGroup *group,
                                 PangoFontDescription *font,
                                 double *width_out,
                                 double *height_out);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gboolean getFixedWidthFont(ZMapWindow window,
                                  PangoFont **font_out,
                                  GList *prefFamilies,
                                  gint points,
                                  PangoWeight weight);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


ZMAP_MAGIC_NEW(zoom_magic_G, );

/* =========================================================================== */
/*                                   PUBLIC                                    */
/* =========================================================================== */
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


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  bases = control->zF * control->pix2mmy ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  bases = control->pix2mmy  / control->zF ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("min/max/curr zoom: %g %g %g\tbases: %g\n",  control->minZF, control->maxZF, control->zF, bases) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return bases ;
}







#ifdef RDS_DONT_INCLUDE
PangoFont *zMapWindowGetFixedWidthFont(ZMapWindow window)
{
  ZMapWindowZoomControl control = NULL;
  control = controlFromWindow(window);
  return control->font;
}
#endif


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

/* =========================================================================== */
/*                                  PRIVATE                                    */
/* =========================================================================== */

/* Create the Controller... */
ZMapWindowZoomControl zmapWindowZoomControlCreate(ZMapWindow window)
{
  ZMapWindowZoomControl num_cruncher = NULL;
#ifdef RDS_DONT_INCLUDE_UNUSED
  double text_height;
  int x_windows_limit = (2 >> 15) - 1;
  int user_set_limit  = (2 >> 15) - 1;  /* possibly a parameter later?!? */
  int max_window_size = 0;
#endif
  num_cruncher = g_new0(ZMapWindowZoomControlStruct, 1);
  num_cruncher->magic = zoom_magic_G ;

  /* We need to set up / have already set up our font we draw on the canvas with */
  if(!zMapGUIGetFixedWidthFont(GTK_WIDGET(window->toplevel),
                               g_list_append(NULL, ZMAP_ZOOM_FONT_FAMILY),
                               ZMAP_ZOOM_FONT_SIZE, PANGO_WEIGHT_NORMAL,
                               &(num_cruncher->font), &(num_cruncher->font_desc)))
    printf("zoom failed to get fixed width font \n");

  /* Make sure this is the 1:1 text_height 14 on my machine*/
  foo_canvas_set_pixels_per_unit_xy(window->canvas, 1.0, 1.0);
  textDimensionsOfFont(foo_canvas_root(window->canvas),
                       num_cruncher->font_desc,
                       &(num_cruncher->textWidth),
                       &(num_cruncher->textHeight)) ;

  num_cruncher->maxZF   = num_cruncher->textHeight + (double)(ZMAP_WINDOW_TEXT_BORDER);
  num_cruncher->border  = num_cruncher->maxZF; /* This should _NOT_ be changed */
  num_cruncher->status  = ZMAP_ZOOM_INIT;

  //  num_cruncher->max_window_size = (user_set_limit ? user_set_limit : x_windows_limit)
  //  - ((num_cruncher->border * 2) * num_cruncher->maxZF);


  {
    GdkScreen *screen ;
    double width_pixels, height_pixels, width_mm, height_mm ;

    screen = gdk_screen_get_default() ;

    width_pixels = (double)gdk_screen_get_width(screen) ;
    height_pixels = (double)gdk_screen_get_height(screen) ;
    width_mm = (double)gdk_screen_get_width_mm(screen) ;
    height_mm = (double)gdk_screen_get_height_mm(screen) ;

    num_cruncher->pix2mmy = height_pixels / height_mm ;
    num_cruncher->pix2mmx = width_pixels / width_mm ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    printf("stop here\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  }


  return num_cruncher;
}

/* Because we need to create and then set it up later */
void zmapWindowZoomControlInitialise(ZMapWindow window)
{
  ZMapWindowZoomControl control;

  control = controlFromWindow(window);

  control->minZF = getMinZoom(window);
 if(control->status == ZMAP_ZOOM_INIT)
    control->zF    = control->minZF;

  /* Account for SHORT sequences (shorter than canvas height) */
  setZoomStatus(control); /* This alone probably doesn't do this!! */

  return ;
}

/* Should be called when either canvas height(size-allocate) or seq
   length change. Does virtually the same as initialise... */
void zmapWindowZoomControlHandleResize(ZMapWindow window)
{
  ZMapWindowZoomControl control;
  double min;

  control = controlFromWindow(window);

  min = getMinZoom(window);

  /* Always need to reset this. */
  control->minZF = min;

  if(min > control->zF)
    zMapWindowZoom(window, min / control->zF);

  setZoomStatus(control);

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
  zMapAssert(max_pixels && def && pointA && pointB);

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
#ifdef NOW_IN_UTILS_RDS
void zmapWindowZoomControlClampSpan(ZMapWindow window, double *top_inout, double *bot_inout)
{
  ZMapWindowZoomControl control;
  double top, bot;

  top = *top_inout;
  bot = *bot_inout;
  control = controlFromWindow(window);

  if (top < window->sequence->start)
    {
      if ((bot = bot + (window->sequence->start - top)) > window->sequence->end)
        bot = window->sequence->end ;
      top = window->sequence->start ;
    }
  else if (bot > window->sequence->end)
    {
      if ((top = top - (bot - window->sequence->end)) < window->sequence->start)
        top = window->sequence->start ;
      bot = window->sequence->end ;
    }

  *top_inout = top;
  *bot_inout = bot;

  return ;
}
#endif   /* NOW_IN_UTILS_RDS  */

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

#ifdef BLAH_BLAH_BLAH
void zmapWindowDebugWindowCopy(ZMapWindow window)
{
  ZMapWindowZoomControl control;
  double x1, x2, y1, y2;
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(foo_canvas_root(FOO_CANVAS(window->canvas))),
                             &x1, &y1,
                             &x2, &y2);
  control = controlFromWindow(window);
  printf("Window:\n"
         " min_coord %f\n"
         " max_coord %f\n"
         " seqLength %f\n"
         " x1 %f, y1 %f, x2 %f, y2 %f\n",
         window->min_coord,
         window->max_coord,
         window->seqLength,
         x1, y1, x2, y2
         );
  printControl(control);
  return ;
}
#endif /* BLAH_BLAH_BLAH */

/* =========================================================================== */
/*                                  INTERNAL                                   */
/* =========================================================================== */

/* Exactly what it says */
static double getMinZoom(ZMapWindow window)
{
  ZMapWindowZoomControl control;
  double canvas_height, border_height, zf;

  control = controlFromWindow(window);
  zMapAssert(control != NULL
             && window->seqLength != 0
             );

  /* These heights are in pixels */
  border_height = control->border * 2.0;
  canvas_height = GTK_WIDGET(window->canvas)->allocation.height;

  zMapAssert(canvas_height > 0);
  /* zMapAssert(canvas_height >= border_height); */
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
  zMapAssert(control && ZMAP_MAGIC_IS_VALID(zoom_magic_G, control->magic)) ;

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

  zMapAssert(control && ZMAP_MAGIC_IS_VALID(zoom_magic_G, control->magic)) ;

  if (control->status == ZMAP_ZOOM_FIXED
      || (factor > 1.0 && control->status == ZMAP_ZOOM_MAX)
      || (factor < 1.0 && control->status == ZMAP_ZOOM_MIN))
    {
      can_zoom = FALSE;
    }
  return can_zoom;
}

/* This is here incase window struct changes name of control; */
static ZMapWindowZoomControl controlFromWindow(ZMapWindow window)
{
  ZMapWindowZoomControl control = NULL;

  zMapAssert(window && window->zoom);

  control = window->zoom;

  zMapAssert(control && ZMAP_MAGIC_IS_VALID(zoom_magic_G, control->magic)) ;

  return control;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gboolean getFixedWidthFont(ZMapWindow window,
                                  PangoFont **font_out,
                                  GList *prefFamilies,
                                  gint points,
                                  PangoWeight weight)
{
  PangoFontFamily **families;
  PangoFontFamily *match_family = NULL;
  PangoContext *context         = NULL;
  gint n_families, i;
  PangoFont *font = NULL;
  gboolean found  = FALSE,
    havePreferred = FALSE;

  context = gtk_widget_get_pango_context(GTK_WIDGET (window->toplevel));

  pango_context_list_families(context,
                              &families, &n_families);

  for (i=0; i<n_families; i++)
    {
      const gchar *name = pango_font_family_get_name (families[i]);
      if(!havePreferred && (found = TRUE))
        {                       /* pango_font_family_is_monospace(families[i]) */
          GList *pref = g_list_first(prefFamilies);
          //printf("Found monospace family %s\n", name);
          while(pref)
            {
              if(!g_ascii_strcasecmp(name, (char *)pref->data))
                havePreferred = TRUE;
              pref = g_list_next(pref);
            }
          match_family = families[i];
        }
    }


  if(font_out && found)
    {
      PangoFontDescription *desc = NULL;
      const gchar *name = pango_font_family_get_name (match_family);

      desc = pango_font_description_from_string(name);
      pango_font_description_set_family(desc, name);
      pango_font_description_set_size  (desc, points * PANGO_SCALE);
      pango_font_description_set_weight(desc, weight);
      font = pango_context_load_font(context, desc);

      zMapAssert(font);
      *font_out = font;
    }

  return found;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



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
  zMapAssert(control && ZMAP_MAGIC_IS_VALID(zoom_magic_G, control->magic)) ;

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


#ifdef WINDOW_STUFF_________________________

typedef struct ZMapWindowZoomControlWindowStruct_
{
  ZMapWindow zmapwindow;
  GtkLayout *layout;
} ZMapWindowZoomControlWindowStruct, *ZMapWindowZoomControlWindow;

static void destroyZoomWindowCB(GtkWidget *window, gpointer user_data);
void zmapWindowZoomControlWindowCreate(ZMapWindow zmapwindow);

void zmapWindowZoomControlWindowCreate(ZMapWindow zmapwindow)
{
  ZMapWindowZoomControlWindow win = NULL;
  GtkWidget *window, *subFrame, *darea, *image;
  GdkPixbuf *thumb1, *thumb2;

  win = g_new0(ZMapWindowZoomControlWindowStruct, 1);
  win->zmapwindow = zmapwindow;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title(GTK_WINDOW(window), "view browser");
  gtk_window_set_default_size(GTK_WINDOW(window), 215, 175);
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
  gtk_container_border_width(GTK_CONTAINER(window), 5);

  /* And a destroy function */
  g_signal_connect(GTK_OBJECT(window), "destroy",
                   GTK_SIGNAL_FUNC(destroyZoomWindowCB), window);

  /* Start drawing things in it. */
  subFrame = gtk_frame_new(NULL);
  gtk_container_add(GTK_CONTAINER(window), subFrame);

  {
    GdkPixbuf *copy, *scaled;
    GtkWidget *image;
    GtkLayout *layout;
    int src_x = 0, src_y = 0, dest_x = 0, dest_y = 0, width, height;
    double dx1, dx2, dy1, dy2;
    foo_canvas_get_scroll_region(FOO_CANVAS(zmapwindow->canvas), &dx1, &dy1, &dx2, &dy2);
    foo_canvas_w2c(FOO_CANVAS(zmapwindow->canvas), dx2, dy2, &width, &height);
    width  = zmapwindow->canvas->layout.hadjustment->upper;
    height = zmapwindow->canvas->layout.vadjustment->upper;

    layout = GTK_LAYOUT(&(zmapwindow->canvas->layout));
    gdk_window_raise(layout->bin_window);
    if((copy = gdk_pixbuf_get_from_drawable(NULL, GDK_DRAWABLE(layout->bin_window),
                                            NULL, src_x, src_y, dest_x, dest_y,
                                            width, height)) != NULL)
      {
        /* This will almost certainly need to be in an expose handler! */
        scaled = gdk_pixbuf_scale_simple(copy,
                                         ZMAP_WINDOW_BROWSER_WIDTH, ZMAP_WINDOW_BROWSER_HEIGHT,
                                         GDK_INTERP_BILINEAR);
        /* we should now have it scaled ready for drawing into  */
        image =  gtk_image_new_from_pixbuf(scaled);
        gtk_container_add(GTK_CONTAINER(subFrame), image);
      }
    else
      {
        darea = gtk_drawing_area_new();
        gtk_widget_modify_bg(GTK_WIDGET(darea), GTK_STATE_NORMAL, &(zmapwindow->canvas_background)) ;
        gtk_container_add(GTK_CONTAINER(subFrame), darea);
      }
  }

  //
  //gtk_image_new(); // possible alternative
  //gtk_widget_set_size_request(darea, 200, 160);
  // we don't need to redraw the pixmap, just the navigation box corners.
  // we will need a list of pixmaps though, one per view.
  //  g_signal_connect (G_OBJECT (darea), "expose_event",
  //                G_CALLBACK (redraw_after_canvas_change__), NULL);

  //

  /* Now show everything. */
  gtk_widget_show_all(window) ;

  return ;
}

static void destroyZoomWindowCB(GtkWidget *window, gpointer user_data)
{

  gtk_widget_destroy(GTK_WIDGET(window));

  return ;
}



#endif
