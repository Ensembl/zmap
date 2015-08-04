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
static void destroyTick(Tick tick) ;

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
  int open = zmapWindowScaleCanvasGetWidth(ruler) + 1;

  /* Now open it, which will result in the above getting called... */
  if(ruler->callbacks->paneResize &&
     ruler->callbacks->user_data)
    (*(ruler->callbacks->paneResize))(&open, ruler->callbacks->user_data);

  return ;
}

/* I don't like the dependence on window here! */
/* MH17 NOTE this is called from zmapWindow.c */
/* MH17 NOTE start and end are the foo canvas scroll region */
gboolean zmapWindowScaleCanvasDraw(ZMapWindowScaleCanvas ruler, int scroll_start, int scroll_end, int seq_start, int seq_end,
                                   int true_start, int true_end,
                                   ZMapWindowDisplayCoordinates display_coordinates,
                                   gboolean force_redraw )
{
  gboolean drawn = FALSE;
  double zoom_factor = 0.0;
  double width = 0.0;
  gboolean zoomed = FALSE;

  if (!ruler || !ruler->canvas)
    return drawn ;

  if(   force_redraw
     || ruler->last_draw_coords.y1 != scroll_start
     || ruler->last_draw_coords.y2 != scroll_end
     || (ruler->last_draw_coords.revcomped != ruler->revcomped)
     || (ruler->last_draw_coords.pixels_per_unit_y != ruler->canvas->pixels_per_unit_y))
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
                                               "y", (double) seq_start,
                                               NULL) ;

      zoom_factor = ruler->canvas->pixels_per_unit_y;

      if(scroll_start > seq_start || scroll_end < seq_end)
        zoomed = TRUE;

      width = zmapWindowDrawScaleBar(ruler->scrolled_window,
                                     FOO_CANVAS_GROUP(ruler->scaleParent),
                                     scroll_start, scroll_end,
                                     seq_start, seq_end,
                                     true_start, true_end,
                                     zoom_factor, ruler->revcomped, zoomed,
                                     display_coordinates) ;

      drawn = TRUE;

      ruler->last_draw_coords.y1 = scroll_start;
      ruler->last_draw_coords.y2 = scroll_end;
      ruler->last_draw_coords.x1 = 0;
      ruler->last_draw_coords.x2 = width;
      ruler->last_draw_coords.pixels_per_unit_y = ruler->canvas->pixels_per_unit_y;
      ruler->last_draw_coords.revcomped = ruler->revcomped;

    }

  return drawn;
}


double zmapWindowScaleCanvasGetWidth(ZMapWindowScaleCanvas ruler)
{
  return ruler->last_draw_coords.x2;
}


void zmapWindowScaleCanvasSetScroll(ZMapWindowScaleCanvas ruler, double x1, double y1, double x2, double y2)
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


double zmapWindowDrawScaleBar(GtkWidget *canvas_scrolled_window,
                              FooCanvasGroup *group,
                              int scroll_start, int scroll_end,
                              int seq_start, int seq_end,
                              int true_start, int true_end,
                              double zoom_factor, gboolean revcomped, gboolean zoomed,
                              ZMapWindowDisplayCoordinates display_coordinates)
{
  double scale_width = 70.0;
  PangoFontDescription *font_desc;
  double font_width,font_height;
  int text_height;
  GdkColor black,grey,*colour;
  double tick_width;
  int scroll_len;
  int seq_len;         /* biggest slice coord to display */

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
  /* sprintf(buf,"scalebar%c%p", revcomped ? '-' : '+', ((FooCanvasItem *) group)->canvas); */
  sprintf(buf, "scalebar%p", ((FooCanvasItem*) group)->canvas) ;
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
  GList *tick_list = NULL,
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
      switch (display_coordinates)
        {
          case ZMAP_WINDOW_DISPLAY_SLICE:
            coord_used = coord_slice ;
            break ;
          case ZMAP_WINDOW_DISPLAY_CHROM:
            coord_used = coord_chrom ;
            break ;
          case ZMAP_WINDOW_DISPLAY_OTHER:
          case ZMAP_WINDOW_DISPLAY_INVALID:
            coord_used = 0 ;
            break;
          default:
            break ;
        }

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
              if (tick_label)
                str_length_max = (str_length = strlen(tick_label)) > str_length_max ? str_length : str_length_max ;
            }

          tick_object = createTick(coord_draw, tick_width_use, tick_label) ;
          tick_list = g_list_prepend(tick_list, (gpointer) tick_object) ;
        }
    }
  /*
   * This can only be done once the string labels have been inspected in order
   * that we know what the longest one will be.
   */
  text_max = (str_length_max+1.0+(revcomped ? 1.0 : 0.0))*font_width ;
  scale_width = text_max + tick_width_number;
  zMapWindowCanvasFeaturesetSetWidth(featureset, scale_width);

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
                                      NULL, &black, NULL) ;

          /* draw label */
          if (tick_object->label)
            {
               zMapWindowFeaturesetAddGraphics(featureset, FEATURE_TEXT,
                                      0, coord_draw_at,
                                      text_max, coord_draw_at,
                                      NULL, &black, tick_object->label) ;
            }

          /* draw vertical line from start of interval to first tick position */
          //if (first_tick)
          //  {
              //zMapWindowFeaturesetAddGraphics(featureset, FEATURE_LINE,
              //                            scale_width-1.0, (double)interval_start,
              //                            scale_width-1.0, coord_draw_at,
              //                            NULL, &black, NULL ) ;
              //first_tick = FALSE ;
          //  }
          //else /* draw vertical line between the previous tick and this one */
          //  {
              //zMapWindowFeaturesetAddGraphics(featureset, FEATURE_LINE,
              //                            scale_width-1.0, coord_draw_at_last,
              //                            scale_width-1.0, coord_draw_at,
              //                            NULL, &black, NULL ) ;
          //  }

          coord_draw_at_last = coord_draw_at ;
          destroyTick(tick_object) ;
        }
    }
  /* draw vertical line between the last tick position and the end of the interval */
  //zMapWindowFeaturesetAddGraphics(featureset, FEATURE_LINE,
  //                            scale_width-1.0, coord_draw_at,
  //                            scale_width-1.0, (double)interval_end,
  //                            NULL, &black, NULL ) ;
  /*
   * Instead of many small lines, we just do one long one; massively
   * reduces the amount of memory required.
   */
  zMapWindowFeaturesetAddGraphics(featureset, FEATURE_LINE,
                                  scale_width-1.0, (double)interval_start,
                                  scale_width-1.0, (double)interval_end,
                                  NULL, &black, NULL ) ;

  if (tick_list)
    g_list_free(tick_list) ;

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

static void destroyTick(Tick tick)
{
  if (tick)
    {
      //if (tick->label)
      //  g_free((void*) tick->label) ;
      g_free((void*) tick) ;
    }
}

