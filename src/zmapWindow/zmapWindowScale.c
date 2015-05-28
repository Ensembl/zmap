/*  File: zmapWindowScale.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 * Description: Implements the scale bar shown for sequences.
 *              (simplified by mh17 and renamed zmapWindowScale to avoid
 *               confusion with the ruler)
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>
#include <math.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsFoo.h>
#include <zmapWindow_P.h>


#define DEFAULT_PANE_POSITION 100

/*
 * This is for my scale bar drawing experiments that are
 * being kept on the develop branch for the moment.
 */
/* #define SM23_EXPERIMENT 1 */

/* Privatise
 *
 * what, the NHS? if I were in charge...
 */
typedef struct _ZMapWindowScaleCanvasStruct
{
  FooCanvas *canvas;            /* The Canvas */
  FooCanvasItem *scaleParent;  /* The group we draw into, without it we draw again */
  FooCanvasItem *horizon;
  GtkWidget *scrolled_window;   /* The scrolled window */

  int default_position;
  gboolean freeze, text_left;

  PangoFont *font;
  PangoFontDescription *font_desc;
  int font_width;

  gboolean revcomped;
  int seq_start,seq_end;

  struct
  {
    double y1, y2;
    double x1, x2;
    double pixels_per_unit_y;
    gboolean revcomped;
  }last_draw_coords;

  ZMapWindowScaleCanvasCallbackList callbacks; /* The callbacks we need to call sensible window functions... */

  guint display_forward_coords : 1;                                            /* Has a coord display origin been set. */
} ZMapWindowScaleCanvasStruct;



typedef enum
  {
    ZMAP_SCALE_BAR_GROUP_NONE,
    ZMAP_SCALE_BAR_GROUP_LEFT,
    ZMAP_SCALE_BAR_GROUP_RIGHT,
  } ZMapScaleBarGroupType;

/*
 * This represents the information required to
 * create an optionally-labeled tick.
 */
typedef struct _TickStruct
  {
    int coord, width ;
    char *label ;
  } TickStruct, *Tick ;

static Tick createTick(int coord, int width, char * string) ;
static void destroyTick(gpointer tick) ;

static void paneNotifyPositionCB(GObject *pane, GParamSpec *scroll, gpointer user_data);


/* CODE... */

ZMapWindowScaleCanvas zmapWindowScaleCanvasCreate(ZMapWindowScaleCanvasCallbackList callbacks)
{
  ZMapWindowScaleCanvas ruler = NULL;

  ruler = g_new0(ZMapWindowScaleCanvasStruct, 1);

  /* Now we make the ruler canvas */
  ruler->canvas    = FOO_CANVAS(foo_canvas_new());
  foo_bug_set(ruler->canvas,"scale");

  ruler->callbacks = g_new0(ZMapWindowScaleCanvasCallbackListStruct, 1);
  ruler->callbacks->paneResize = callbacks->paneResize;
  ruler->callbacks->user_data  = callbacks->user_data;

  ruler->default_position = DEFAULT_PANE_POSITION;
  ruler->text_left        = TRUE; /* TRUE = put the text on the left! */

  ruler->last_draw_coords.y1 = ruler->last_draw_coords.y2 = 0.0;
  ruler->last_draw_coords.x1 = ruler->last_draw_coords.x2 = 0.0;

  return ruler;
}

/* Packs the canvas in the supplied parent object... */
void zmapWindowScaleCanvasInit(ZMapWindowScaleCanvas ruler,
                               GtkWidget *paned,
                               GtkAdjustment *vadjustment)
{
  GtkWidget *scrolled = NULL;

  ruler->scrolled_window = scrolled = gtk_scrolled_window_new(NULL, NULL);

  zmapWindowScaleCanvasSetVAdjustment(ruler, vadjustment);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW( scrolled ),
                                 GTK_POLICY_ALWAYS, GTK_POLICY_NEVER);

  gtk_widget_set_sensitive(GTK_SCROLLED_WINDOW(scrolled)->hscrollbar, TRUE);
  gtk_widget_set_name(GTK_WIDGET(GTK_SCROLLED_WINDOW(scrolled)->hscrollbar), "zmap-ruler-hscrollbar");

  gtk_container_add(GTK_CONTAINER(scrolled), GTK_WIDGET(ruler->canvas)) ;

  gtk_paned_pack1(GTK_PANED(paned), scrolled, FALSE, TRUE);

  g_object_connect(G_OBJECT(paned),
                   "signal::notify::position", G_CALLBACK(paneNotifyPositionCB), (gpointer)ruler,
                   NULL);

  if(!zMapGUIGetFixedWidthFont(GTK_WIDGET(paned),
                               g_list_append(NULL, (void *)ZMAP_ZOOM_FONT_FAMILY), ZMAP_ZOOM_FONT_SIZE, PANGO_WEIGHT_NORMAL,
                               &(ruler->font), &(ruler->font_desc)))
    zMapLogWarning("%s", "Couldn't get fixed width font\n") ;
  else
    zMapGUIGetFontWidth(ruler->font, &(ruler->font_width)) ;


  return ;
}


GtkWidget *zmapWindowScaleCanvasGetScrolledWindow(ZMapWindowScaleCanvas ruler)
{
  GtkWidget *scolled_window = NULL ;

  scolled_window = ruler->scrolled_window ;

  return scolled_window ;
}




void zmapWindowScaleCanvasOpenAndMaximise(ZMapWindowScaleCanvas ruler)
{
  int open = zMapWindowScaleCanvasGetWidth(ruler) + 1;

  /* Now open it, which will result in the above getting called... */
  if(ruler->callbacks->paneResize &&
     ruler->callbacks->user_data)
    (*(ruler->callbacks->paneResize))(&open, ruler->callbacks->user_data);

  return ;
}

/* I don't like the dependence on window here! */
/* MH17 NOTE this is called from zmapWindow.c */
/* MH17 NOTE start and end are the foo canvas scroll region */
gboolean zmapWindowScaleCanvasDraw(ZMapWindowScaleCanvas ruler, int start, int end,int seq_start,int seq_end,
                                   int true_start, int true_end)
{
  gboolean drawn = FALSE;
  double zoom_factor = 0.0;
  double width = 0.0;
  gboolean zoomed = FALSE;

  if (!ruler || !ruler->canvas)
    return drawn ;

  if(ruler->last_draw_coords.y1 != start || ruler->last_draw_coords.y2 != end ||
     (ruler->last_draw_coords.revcomped != ruler->revcomped) ||
     (ruler->last_draw_coords.pixels_per_unit_y != ruler->canvas->pixels_per_unit_y))
    {
      /* We need to remove the current item */
      if(ruler->scaleParent)
        {
          gtk_object_destroy(GTK_OBJECT(ruler->scaleParent));
          ruler->scaleParent = ruler->horizon = NULL;
        }

      ruler->scaleParent = foo_canvas_item_new(foo_canvas_root(ruler->canvas),
                                               foo_canvas_group_get_type(),
                                               "x", 0.0,
                                               "y", (double) seq_start,        /* simulate the canvas environment of a context group @ seq coords */
                                               /* can we have a group at seq coords at high zoom?- would wrap round so we use start which is the scroll region */
                                               NULL) ;

      zoom_factor = ruler->canvas->pixels_per_unit_y;

      if(start > seq_start || end < seq_end)
        zoomed = TRUE;

      width = zMapWindowDrawScaleBar(ruler->scrolled_window,
                                     FOO_CANVAS_GROUP(ruler->scaleParent),
                                     start, end, seq_start, seq_end,
                                     true_start, true_end,
                                     zoom_factor, ruler->revcomped, zoomed) ;

      drawn = TRUE;

      ruler->last_draw_coords.y1 = start;
      ruler->last_draw_coords.y2 = end;
      ruler->last_draw_coords.x1 = 0;
      ruler->last_draw_coords.x2 = width;
      ruler->last_draw_coords.pixels_per_unit_y = ruler->canvas->pixels_per_unit_y;
      ruler->last_draw_coords.revcomped = ruler->revcomped;

    }

  return drawn;
}


double zMapWindowScaleCanvasGetWidth(ZMapWindowScaleCanvas ruler)
{
  return ruler->last_draw_coords.x2;
}


void zMapWindowScaleCanvasSetScroll(ZMapWindowScaleCanvas ruler, double x1, double y1, double x2, double y2)
{
  /* NOTE this canvas has nothing except the scale bar */
  foo_canvas_set_scroll_region(FOO_CANVAS(ruler->canvas), x1, y1, x2, y2);

  /* we can't rely on scroll to queue an expose */
  {
    int ix1, ix2, iy1, iy2;
    foo_canvas_w2c(ruler->canvas, x1, y1,&ix1,&iy1);
    foo_canvas_w2c(ruler->canvas, x2, y2, &ix2, &iy2);
    foo_canvas_request_redraw(ruler->canvas, ix1, iy1, ix2, iy2);
  }

}

void zmapWindowScaleCanvasSetRevComped(ZMapWindowScaleCanvas ruler, gboolean revcomped)
{
  if (!ruler)
    return ;

  ruler->display_forward_coords = TRUE ;
  ruler->revcomped = revcomped;

  return ;
}

void zmapWindowScaleCanvasSetVAdjustment(ZMapWindowScaleCanvas ruler, GtkAdjustment *vadjustment)
{

  gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(ruler->scrolled_window), vadjustment) ;

  return ;
}

void zmapWindowScaleCanvasSetPixelsPerUnit(ZMapWindowScaleCanvas ruler, double x, double y)
{
  if (!ruler || !ruler->canvas)
    return ;

  foo_canvas_set_pixels_per_unit_xy(ruler->canvas, x, y) ;

  return ;
}


/* INTERNALS */
static void paneNotifyPositionCB(GObject *pane, GParamSpec *scroll, gpointer user_data)
{
  ZMapWindowScaleCanvas ruler = (ZMapWindowScaleCanvas)user_data;
  FooCanvasItem *scale = NULL;
  gboolean cancel = FALSE;

  if(ruler->scaleParent && (scale = FOO_CANVAS_ITEM(ruler->scaleParent)) != NULL)
    cancel = TRUE;

  /* Reduce the processing this needs to do if we're frozen.  I can't
   * get g_object_freeze_notify(G_OBJECT(pane)) to work...
   */
  if(cancel)
    cancel = !ruler->freeze;

  if(cancel)
    {
      int position,
        max = ruler->default_position;
      double x2 = 0.0;

      foo_canvas_item_get_bounds(scale,
                                 NULL, NULL, &x2, NULL);
      /* get bounds returns 0.0 if item is not mapped.
       * This happens when handle position == 0. */
      if(x2 != 0.0)
        max = (int)x2;

      g_object_get(G_OBJECT(pane),
                   "position", &position,
                   NULL);

      if(position == max)
        goto leave;
      else if(position > max &&
              ruler->callbacks->paneResize &&
              ruler->callbacks->user_data)
        (*(ruler->callbacks->paneResize))(&max, ruler->callbacks->user_data);
    }

 leave:
  return ;
}


/*
 * This function is to convert the integer base coordinate (which we imagine
 * is either a 1-based slice coordinate or a chromosome coordinate) into
 * a string to be written into the ruler. The function returns a newly allocated
 * string. For revcomped data we stick a '-' sign at the start; simply a convention.
 *
 * The maximum number of significant digits to be printed after the decimal
 * point is 6 (this is the default, but it's in the format strings anyway).
 */
static char * number_to_text(int num, int revcomped)
{
  static const char *format_for = "%.6g%c",
    *format_rev = "-%.6g%c" ;
  static const int npowers = 4,
    powers[] = {1000000000,
                   1000000,
                      1000,
                         1 } ;
  static const char unit_chars[] = {'G', 'M', 'k', '\0'} ;
  char *result = NULL ;
  int i = 0, j = 0 ;

  if (num)
    {
      for (i=0; i<npowers ; ++i)
        if ((j = num/powers[i]))
          break ;
      if (revcomped)
        result = g_strdup_printf(format_rev, (double)num/(double)powers[i], unit_chars[i]) ;
      else
        result = g_strdup_printf(format_for, (double)num/(double)powers[i], unit_chars[i]) ;
    }
  else
    {
      result = g_strdup_printf("0") ;
    }

  return result ;
}


/* draw the scale bar: called from navigator and window */

/* a rewrite */
/* we need :
   start and end seq coord
   foo group to draw into
   window height in pixelsforce
   start and end window coords?
   text height (font)
*/
/* we draw the scale on the right and text to the left */
/*
 * scroll start and end are as displayed (NB navigator always has whole sequence)
 * seq start and end are whole sequence in fwd strand coodinates
 * NOTE the canvas is set (elsewhere) with a scroll region of fwd strand chromosome coord
 * and we have to calculate 1 based fwd and revcomp'd coordinates for where to draw the ticks
 *
 * NOTE the locus display in the navigator assumes the same mapping so setting the scroll region
 * here would probably break something
 * all this makes the zmapWindowUtils functions not much use as we have a mixed set of coordinate systems
 */

/*
 * NOTE (18 July 12)
 * navigator and scale now work as the 'normal' canvas, the scale bar is in a foo group with y set to seq coords
 * seq_start and end are as revcomp-d (always +ve), scroll region is exactly as in the main window
 * canvas coords are always seq_start based and +ve (but scroll region can stray into -ve for the border)
 * however, the scroll reqion as passsed here is clamped to sequence coordinates
 *
 * for revcomp we have to reverse back and express as -ve to get the ticks in the right place (-1 based)
 * the code is very fiddly
 *
 */
/* NOTE (24 July 2012)
 * perhaps it would have been simpler to stay with the two columns (text and ticks) instead of calculating text size...
 * but regardless of that they have to be drawn together in the x axis
 */
double zMapWindowDrawScaleBar(GtkWidget *canvas_scrolled_window,
                              FooCanvasGroup *group,
                              int scroll_start, int scroll_end,
                              int seq_start, int seq_end,
                              int true_start, int true_end,
                              double zoom_factor, gboolean revcomped, gboolean zoomed)
{
  static const size_t label_size = 32 ;
  char label [label_size];        /* only need ~8 but there you go */
  double scale_width = 70.0;
  int seq_len;        /* # bases in the scroll region */
  int tick;
  int tick_coord;
  int top, level;        /* we go many levels deep */
  int n_pixels;        /* cannot be more than 30k due to foo */
  int gap;                /* pixels between ticks */
  int digit;                /* which tick out of 10 */
  int nudge;                /* tick is 5th? make bigger */
  int base;
  PangoFontDescription *font_desc;
  double font_width,font_height;
  int text_height;
  double canvas_coord;
  GdkColor black,grey,*colour;
  double tick_width;
  int n_levels;        /* number of levels needed for BP resolution */
  int n_hide;         /* number we can't see at current zoom level */
  int n_text_hide;        /* text we can't display due to overlap */
  const char *unit;
  const char *units[] = { "","","k","M","G" };
  int i;
  int tick_start;
  int tick_end;
  int draw_at;
  int scroll_len;
  int seq_max;        /* biggest slice coord to display */

  ZMapWindowFeaturesetItem featureset;
  GQuark fid;
  char buf[32];
  double text_max,tick_max;        /* max width of each */
  int tick_inc = 4.0;                /* diff in size per level */
  double prev;
  int first;
  static ZMapFeatureTypeStyle scale_style = NULL;
  GtkAdjustment *v_adjust ;


  /*! \todo #warning move this to predefined styles code in featureTyoes.c */
  /* temporarily create pre-defined style here (needed by Canvasfeatureset)
   * we don't actually use the colours here, maybe we should */
  if(!scale_style)
    {
      scale_style = zMapStyleCreate("scale", "scale bar");

      g_object_set(G_OBJECT(scale_style),
                   ZMAPSTYLE_PROPERTY_MODE, ZMAPSTYLE_MODE_PLAIN,
                   ZMAPSTYLE_PROPERTY_COLOURS, "normal fill white; normal border black",
                   NULL);
      zMapStyleSetDisplayable(scale_style, TRUE);        /* (always drawable) */
    }

  /* get unique id. and create.... if windows get destroyed then so should the featuresets */
  sprintf(buf,"scalebar%c%p", revcomped ? '-' : '+', ((FooCanvasItem *) group)->canvas);
  fid = g_quark_from_string(buf);


  /* allocate featureset for whole sequence even if zoomed, if the feeatureset does not get
   * re-created on zoom then the extent will be ok */
  v_adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(canvas_scrolled_window)) ;

  featureset = (ZMapWindowFeaturesetItem)zMapWindowCanvasItemFeaturesetGetFeaturesetItem(group, fid,
                                                                                         v_adjust,
                                                                                         seq_start, seq_end,
                                                                                         scale_style,
                                                                                         ZMAPSTRAND_NONE, ZMAPFRAME_0,
                                                                                         0, 0);

  seq_len = seq_end - seq_start + 1;
  if(seq_len < 10)        /* we get called on start up w/ no sequence */
    return 0;


  scroll_len = scroll_end - scroll_start + 1;

  gdk_color_parse("black", &black) ;
  gdk_color_parse("grey", &grey) ;

  zMapFoocanvasGetTextDimensions(((FooCanvasItem *) group)->canvas, &font_desc, &font_width, &font_height);
  text_height = (int) (font_height);

  if(!font_height)
    {
      font_height = 14.0;        /* don't just give up ! */
      font_width = 8.0;
      text_height = 14;
    }

  /* get the highest order ticks:  sequence and scroll are in chromosome coordinates but we
   * display slice coordinates.... */
  /* highest order tick is the one displayed not the one in the whole sequence */
  {
    int s = scroll_start - seq_start + 1, e = scroll_end - seq_start + 1;

    if(revcomped)        /* we are counting backwards from the end in slice coordinates */
      {
        int x = s;

        s = scroll_end - e + 1;
        e = scroll_end - x + 1;
      }

    for(tick = 1,n_levels = 0; s != e;tick *= 10, s /= 10, e /= 10, n_levels++)
      {
        continue;
      }
    tick /= 10;
  }

  /* get the number of pixels: this uses the scroll region */

  n_pixels = (int) (scroll_len * zoom_factor);

  for(n_hide = n_levels,gap = n_pixels * tick / scroll_len; n_hide && gap; gap /= 10,n_hide--)
    continue;


  tick_max = tick_inc * (n_levels - n_hide);        /* max tick width */

  /* choose units */
  base = 1;
  for(i = 1;base <= tick && i < 5;i++,base *= 1000)
    continue;
  unit = units[--i];
  base /= 1000;
  /* work out space needed for labels */

  for(n_text_hide = n_levels, gap = n_pixels * tick / scroll_len ;
      n_text_hide && gap > text_height * 2 ;
      gap /= 10,n_text_hide--)
    continue ;

  for(i = 1; n_text_hide-- && i < base; i *= 10)                /* get max resolution of ticks */
    continue;

  seq_max = scroll_end;
  if(revcomped)
    seq_max = seq_end  - scroll_start + 1;
  for(text_max = 1; i < seq_max; text_max++, i *= 10)        /* get number of digits needed */

    continue;
  text_max--;

  if(*unit)                /* for decimal point and suffix */
    text_max += 2;

  if(revcomped)        /* for - sign */
    text_max++;

  text_max *= font_width;


  scale_width = text_max + tick_max;
  zMapWindowCanvasFeaturesetSetWidth(featureset, scale_width);
  {
    FooCanvasItem *foo = (FooCanvasItem *) group;

    foo_canvas_item_request_update(foo);
  }

  /* do we assume there are enough pixels for the highest order ticks?
   * no: if they shrink the window so that you can't read it then don't display
   */

  gap = (int) (((double) n_pixels) * tick / scroll_len );        /* (double) or else would get overflow on big sequences */



#ifndef SM23_EXPERIMENT


  /* we need a lot of levels to cope with large sequences zoomed in a lot */
  /* to get down to single base level */
  for(level = n_levels,top = 1;level >= 0 && tick > 0;level--, tick /= 10, gap /= 10, top = 0)
    {
      if(!gap)
        break;

      if(revcomped)
        {
          if(zoomed)
            {
              tick_start = (int) scroll_start - seq_start + 1;
              tick_start -= tick - (seq_len - tick_start) % tick;
              tick_end = tick_start + scroll_len + tick;
            }
          else
            {
              tick_start = seq_len % tick ;
              tick_end =  seq_len;
            }
        }                        /* NOTE tick_coord is 0 based, it is added to seq_start which is not */
      else
        {
          if(zoomed)
            {
              tick_start = (int) scroll_start - seq_start;
              tick_start -= tick_start % tick;
              tick_end = tick_start + scroll_len + tick;
            }
          else
            {
              tick_start = 0;
              tick_end =  seq_len;
            }
        }

      for(tick_coord = tick_start; tick_coord <= tick_end ;tick_coord += tick)
        {
          int slice_coord = tick_coord;

          if(tick_coord + seq_start < scroll_start)
            continue;
          if(tick_coord + seq_start > scroll_end + 1)
            break;

          if(revcomped)
            slice_coord = seq_end - tick_coord;         /* -1 based coord off the sequence end */

          digit =  (slice_coord / tick) % 10;

          if(top || digit)                        /* don't draw ticks over higher order ticks */
            {
              nudge = (digit == 5 || digit == -5);
              if((nudge && gap > 1) || gap > 2)        /* don't crowd ticks together */
                {
                  /* draw the tick, with a number if appropriate */

                  if(revcomped)
                    draw_at = tick_coord + seq_start + 1;
                  else
                    draw_at = tick_coord + seq_start - 1;

                  label[0] = 0;
                  canvas_coord = (double) draw_at;

                  tick_width = tick_inc * (level - n_hide);
                  colour = &black;
                  if(nudge)
                    {
                      tick_width += tick_inc * 0.75;
                      if(gap < 3)
                        colour = &grey;
                    }
                  else if(gap < 5)
                    {
                      colour = &grey;
                    }

                  zMapWindowFeaturesetAddGraphics(featureset, FEATURE_LINE,
                                                  scale_width - tick_width, canvas_coord,
                                                  scale_width, canvas_coord,
                                                  NULL, colour, NULL);

                  if((slice_coord && gap > text_height * 2))
                    {
                      int num,frac;
                      const char *sign = "";
                      double offset = revcomped ? -0.5 : 0.5;
                      ZMapWindowCanvasGraphics gfx;

                      num = slice_coord / base;
                      frac = slice_coord % base;
                      if(revcomped)
                        sign = "-";

                      if(frac)
                        {
                          /* this ought to be easier but we get to cope with
                           * leading and trailing zeroes and -ve zeroes
                           * and then the % operator is a bit flaky w/ -ve numbers
                           */
                          char *p;
                          int f = tick;
                          int digits;

                          for(digits = 1; f < base ; f *= 10)
                            digits++;

                          /* gb10: the original printf which treats the fraction
                           * separately doesn't always work (RT333321) so I've
                           * changed it to use a decimal instead. Not sure if there
                           * are drawbacks with this that the original code intended
                           * to avoid... */
                          float val = (float)slice_coord / (float)base;
                          p = label + sprintf(label,"%s%.*f",sign, digits, val);

                          while(*--p == '0')
                            continue;
                          if(*p != '.')
                            p++;
                          strcpy(p,unit);
                        }
                      else
                        {
                          sprintf(label,"%s%d%s",sign,num,unit);
                        }

                      gfx = zMapWindowFeaturesetAddGraphics(featureset, FEATURE_TEXT,
                                                            0, canvas_coord + offset - 1,
                                                            /* text gets centred on a line, size varies with zoom as it's a fixed size font */
                                                            text_max, canvas_coord + offset + 1,
                                                            NULL, &black, g_strdup(label));
                    }
                }
            }
        }
    }

  /* add a few shortish vertical lines down the left
   * instead of one big one that would mean searching the whole index for small exposes
   * NOTE these lines are drawn at 1-bases visible slice/ scroll region coords#
   * NOTE tick_start and tick_end are as set by the highest res ticks above
   */

  for(first = 1, tick = scroll_len / 20,tick_coord = scroll_start; ;tick_coord += tick)
    {
      draw_at = tick_coord - 1 ;                /* extra -1 to cover first base which is between canvas 0 and 1 */

      if(draw_at < scroll_start)
        draw_at = scroll_start;
      if(draw_at > scroll_end + 1)
        draw_at = scroll_end + 1;

      canvas_coord = (double) draw_at;

      if(!first)
        {
          zMapWindowFeaturesetAddGraphics(featureset, FEATURE_LINE,
                                          scale_width, prev,
                                          scale_width, canvas_coord,
                                          NULL, &black, NULL);
        }

      first = 0;
      prev = canvas_coord;

      if(tick_coord >= scroll_end + 1)
        break;
    }


#else

  /*
   * The information required about the resolution of the display
   * comes from the quantity
   *   zoom_factor = ruler->canvas->pixels_per_unit_y
   * in the caller. Other things we need to know are whether or
   * not the display is revcomped, the true start and end in chromosome
   * coordinates of the region displayed, and whether to display slice
   * or chromosome coordinates to the user.
   */
  static const int interval_max = 1000000000,  /* 10^9 */
    pixels_limit = 3 ,
    tick_width_default = 3,
    tick_width_number = 7,
    tick_width_special = 5 ;
  gboolean first_tick = TRUE ;
  int interval_length = 0,
    interval_start = 0,
    interval_end = 0,
    pixels_per_interval = 0,
    coord_draw = 0,
    coord_slice = 0,
    coord_chrom = 0,
    coord_used = 0,
    coord_draw_step_size = 0,
    tick_interval = 0,
    numbering_interval = 0,
    special_interval = 0,
    str_length = 0,
    str_length_max = 0,
    tick_width_use = 0 ;
  char *tick_label = NULL ;
  double coord_draw_at = 0.0,
    coord_draw_at_last = 0.0 ;
  Tick tick_object = NULL ;
  GSList *tick_list = NULL,
    *list = NULL ;
  interval_start = (scroll_start > seq_start ? scroll_start : seq_start) ;  /* max at start of interval */
  interval_end = (scroll_end < seq_end ? scroll_end : seq_end) ;            /* min at end of interval   */
  interval_length = interval_end - interval_start + 1 ;

  /*
   * Find the spacing in base pairs of the tick_interval, and
   * numbering_interval.
   */
  for (tick_interval = 1; tick_interval<interval_max ; tick_interval*=10 )
    {
      pixels_per_interval = (int) (zoom_factor * tick_interval );
      if (pixels_per_interval > pixels_limit)
        break ;
    }
  numbering_interval = 10 * tick_interval ;
  special_interval = numbering_interval/2 ;

  /*
   * This loop checks coordinate values to determine where to draw
   * ticks and what the label associated with them (if any) should
   * be.
   *
   * We are not checking every value of the coordinate; note the usage
   * of the variable "coord_draw_step_size" below.
   */
  coord_draw_step_size = 1 ;
  for (coord_draw=interval_start; coord_draw<=interval_end; coord_draw+=coord_draw_step_size)
    {

      if (revcomped)
        {
          coord_slice = (seq_end-seq_start+1) - (coord_draw-1) ;
          coord_chrom = (seq_end-seq_start+1) - coord_draw + true_start ;
        }
      else
        {
          coord_slice = coord_draw - seq_start + 1 ;
          coord_chrom = coord_draw ;
        }

      /*
       * Choose the coordinate that will be used to draw as labels and
       * that is also used to decide where the ticks and labels are drawn
       * in the parent canvas.
       */
      coord_used = coord_slice ;

      if (!(coord_used%tick_interval))
        {
          tick_label = NULL ;
          tick_width_use = tick_width_default ;
          coord_draw_step_size = tick_interval ;

          if (!(coord_used%special_interval))
            {
              tick_width_use = tick_width_special ;
            }

          if (!(coord_used%numbering_interval))
            {
              tick_label = number_to_text(coord_used, revcomped) ;
              tick_width_use = tick_width_number ;
              str_length_max = (str_length = strlen(tick_label)) > str_length_max ? str_length : str_length_max ;
            }

          tick_object = createTick(coord_draw, tick_width_use, tick_label) ;
          tick_list = g_slist_prepend(tick_list, (gpointer) tick_object) ;
        }
    }

  /*
   * This can only be done once the string labels have been inspected in order
   * that we know what the longest one will be.
   */
  text_max = (str_length_max+1.0+(revcomped ? 1.0 : 0.0))*font_width ;
  scale_width = text_max + tick_width_number;
  zMapWindowCanvasFeaturesetSetWidth(featureset, scale_width);
  {
    FooCanvasItem *foo = (FooCanvasItem *) group;

    foo_canvas_item_request_update(foo);
  }


  /*
   * Now we can actually draw the things, destroy the ticks as we go along
   * and then free the list itself. This loop also draws the vertical lines
   * on the RHS between ticks.
   */
  for (list=tick_list; list!=NULL; list=list->next)
    {
      tick_object = (Tick) list->data ;

      if (tick_object)
        {
          tick_width = (double)tick_object->width ;
          coord_draw_at = (double)tick_object->coord ;

          /* draw tick */
          zMapWindowFeaturesetAddGraphics(featureset, FEATURE_LINE,
                                      scale_width-tick_width-1.0, coord_draw_at,
                                      scale_width-1.0, coord_draw_at,
                                      NULL, &black, NULL);

          /* draw label */
          if (tick_object->label)
            {
               zMapWindowFeaturesetAddGraphics(featureset, FEATURE_TEXT,
                                      0, coord_draw_at,
                                      text_max, coord_draw_at,
                                      NULL, &black, tick_object->label);
            }

          /* draw vertical line from start of interval to first tick position */
          if (first_tick)
            {
              zMapWindowFeaturesetAddGraphics(featureset, FEATURE_LINE,
                                          scale_width-1.0, (double)interval_start,
                                          scale_width-1.0, coord_draw_at,
                                          NULL, &black, NULL ) ;
              first_tick = FALSE ;
            }
          else /* draw vertical line between the previous tick and this one */
            {
              zMapWindowFeaturesetAddGraphics(featureset, FEATURE_LINE,
                                          scale_width-1.0, coord_draw_at_last,
                                          scale_width-1.0, coord_draw_at,
                                          NULL, &black, NULL ) ;
            }

          coord_draw_at_last = coord_draw_at ;
          destroyTick(tick_object) ;
        }
    }
  /* draw vertical line between the last tick position and the end of the interval */
  zMapWindowFeaturesetAddGraphics(featureset, FEATURE_LINE,
                              scale_width-1.0, coord_draw_at,
                              scale_width-1.0, (double)interval_end,
                              NULL, &black, NULL ) ;

  if (tick_list)
    g_slist_free(tick_list) ;

#endif

  return scale_width;
}


/*
 * This creates a tick object.
 *
 * Note that when the object is created, the argument string pointer
 * is copied, we do not create a new string and when it is destroyed,
 * the string is not deleted.
 *
 * This is because of the usage of the label when the function
 * zMapWindowFeaturesetAddGraphics() is called; the original usage
 * appears to require a new instance of the string for every one.
 */
static Tick createTick(int coord, int width, char * string)
{
  Tick tick = NULL ;

  tick = (Tick) g_new0(TickStruct, 1) ;
  if (tick)
    {
      tick->coord = coord ;
      tick->width = width ;
      tick->label = string ;
    }

  return tick ;
}

static void destroyTick(gpointer tick)
{
  if (tick)
    g_free((void*) tick) ;
}

