/*  File: zmapWindowScale.c
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
#define ZMAP_SCALE_MINORS_PER_MAJOR 10
#define ZMAP_FORCE_FIVES TRUE
#define ZMAP_SCALE_BAR_GROUP_TYPE_KEY "scale_group_type"

/* For printing lots to the terminal and debugging
#define VERBOSE_1
#define VERBOSE_2
#define VERBOSE_3
*/

#define debug printf
#define SCALE_DEBUG	0
#define FOO_SCALE_DEBUG	0


/* Privatise  */
typedef struct _ZMapWindowScaleCanvasStruct
{
  FooCanvas *canvas;            /* The Canvas */
  FooCanvasItem *scaleParent;  /* The group we draw into, without it we draw again */
  FooCanvasItem *horizon;
  GtkWidget *scrolled_window;   /* The scrolled window */

  int default_position;
  gboolean freeze, text_left;
  //  double line_height;
#if ZOOM_SCROLL
  gulong visibilityHandlerCB;
#endif

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

  guint display_forward_coords : 1;					    /* Has a coord display origin been set. */
} ZMapWindowScaleCanvasStruct;



typedef enum
  {
    ZMAP_SCALE_BAR_GROUP_NONE,
    ZMAP_SCALE_BAR_GROUP_LEFT,
    ZMAP_SCALE_BAR_GROUP_RIGHT,
  } ZMapScaleBarGroupType;





//static void positionLeftRight(FooCanvasGroup *left, FooCanvasGroup *right);
static void paneNotifyPositionCB(GObject *pane, GParamSpec *scroll, gpointer user_data);
//static gboolean rulerVisibilityHandlerCB(GtkWidget *widget, GdkEventExpose *expose, gpointer user_data);
#if ZOOM_SCROLL
static gboolean rulerMaxVisibilityHandlerCB(GtkWidget *widget, GdkEventExpose *expose, gpointer user_data);

static void freeze_notify(ZMapWindowScaleCanvas ruler);
static void thaw_notify(ZMapWindowScaleCanvas ruler);
#endif



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

#if ZOOM_SCROLL
  ruler->visibilityHandlerCB = 0;
#endif

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

#ifdef RDS_DONT_DO_THIS_IT_CAUSES_FLICKER
  /* ... I think we actually want to set to zero! until draw occurs ... */
  if(ruler->callbacks->paneResize && ruler->callbacks->user_data)
    (*(ruler->callbacks->paneResize))(&(ruler->default_position), ruler->callbacks->user_data);
#endif

  g_object_connect(G_OBJECT(paned),
                   "signal::notify::position", G_CALLBACK(paneNotifyPositionCB), (gpointer)ruler,
                   NULL);

  if(!zMapGUIGetFixedWidthFont(GTK_WIDGET(paned),
                               g_list_append(NULL, ZMAP_ZOOM_FONT_FAMILY), ZMAP_ZOOM_FONT_SIZE, PANGO_WEIGHT_NORMAL,
                               &(ruler->font), &(ruler->font_desc)))
    printf("Couldn't get fixed width font\n");
  else
    zMapGUIGetFontWidth(ruler->font, &(ruler->font_width));


  return ;
}

void zmapWindowScaleCanvasOpenAndMaximise(ZMapWindowScaleCanvas ruler)
{
  int open = zMapWindowScaleCanvasGetWidth(ruler) + 1;

#if ZOOM_SCROLL
  /* If there's one set, disconnect it */
  if(ruler->visibilityHandlerCB)
    g_signal_handler_disconnect(G_OBJECT(ruler->canvas), ruler->visibilityHandlerCB);

  /* Reconnect the maximising one... */
  ruler->visibilityHandlerCB =
    g_signal_connect(G_OBJECT(ruler->canvas),
                     "visibility-notify-event",
                     G_CALLBACK(rulerMaxVisibilityHandlerCB),
                     (gpointer)ruler);
#endif

  /* Now open it, which will result in the above getting called... */
  if(ruler->callbacks->paneResize &&
     ruler->callbacks->user_data)
    (*(ruler->callbacks->paneResize))(&open, ruler->callbacks->user_data);

  return ;
}


#if ZOOM_SCROLL
void zmapWindowScaleCanvasMaximise(ZMapWindowScaleCanvas ruler, double y1, double y2)
{
  double x2, max_x2,
    ix1 = 0.0,
    iy1 = y1,
    iy2 = y2;

  if(ruler->scaleParent)
    {
      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(ruler->scaleParent),
                                 NULL, NULL, &max_x2, NULL);
      if(!max_x2)
	max_x2 = ruler->last_draw_coords.x2 + 1;
      /* somehow max_x2 is always 0, bit we'd like to maximise only if they have-nt shrunk it */

      if(iy1 == iy2 && iy1 == ix1)
        foo_canvas_get_scroll_region(FOO_CANVAS(ruler->canvas),
                                     NULL, &iy1, &x2, &iy2);
      else
        foo_canvas_get_scroll_region(FOO_CANVAS(ruler->canvas),
                                     NULL, NULL, &x2, NULL);
#if SCALE_DEBUG
      debug("scaleCanvas set scroll: %1.f %1.f - %1.f %1.f %1.f %1.f\n", x2, max_x2, ix1, ruler->last_draw_coords.x2, iy1, iy2);
#endif /* VERBOSE_1 */

      /* NOTE this canvas has nothing except the scale bar */
      foo_canvas_set_scroll_region(FOO_CANVAS(ruler->canvas),
                                   ix1, iy1, max_x2, iy2);

      if(max_x2 > 0.0)
        {
          ruler->default_position = max_x2;

          if(ruler->callbacks->paneResize &&
             ruler->callbacks->user_data)
            {
              int floored = (int) max_x2;
              freeze_notify(ruler);
              (*(ruler->callbacks->paneResize))(&floored, ruler->callbacks->user_data);
              thaw_notify(ruler);
            }
        }
    }

  return ;
}
#endif

#if FOO_SCALE_DEBUG
extern gpointer scale_thing;
#endif

/* I don't like the dependence on window here! */
/* MH17 NOTE this is called from zmapWindow.c */
/* MH17 NOTE start and end are the foo canvas scroll region */
gboolean zmapWindowScaleCanvasDraw(ZMapWindowScaleCanvas ruler, int start, int end,int seq_start,int seq_end)
{
  gboolean drawn = FALSE;
  double zoom_factor = 0.0;
  double width = 0.0;
  gboolean zoomed = FALSE;

  zMapAssert(ruler && ruler->canvas);

  if(ruler->last_draw_coords.y1 != start || ruler->last_draw_coords.y2 != end ||
     (ruler->last_draw_coords.revcomped != ruler->revcomped) ||
     //     (ruler->display_forward_coords && ruler->last_draw_coords.origin != ruler->origin) ||
     (ruler->last_draw_coords.pixels_per_unit_y != ruler->canvas->pixels_per_unit_y))
    {
      /* We need to remove the current item */
      if(ruler->scaleParent)
	{
	  gtk_object_destroy(GTK_OBJECT(ruler->scaleParent));
	  ruler->scaleParent = ruler->horizon = NULL;
	}

#if SCALE_DEBUG
      printf("scaleCanvasDraw %d %d %d %d\n", start, end, seq_start, seq_end);
#endif

      ruler->scaleParent = foo_canvas_item_new(foo_canvas_root(ruler->canvas),
					       foo_canvas_group_get_type(),
					       "x", 0.0,
					       "y", (double) seq_start,	/* simulate the canvas environment of a context group @ seq coords */
					       /* can we have a group at seq coords at high zoom?- would wrap round so we use start which is the scroll region */
					       NULL) ;

      zoom_factor = ruler->canvas->pixels_per_unit_y;

      if(start > seq_start || end < seq_end)
	zoomed = TRUE;

      width = zMapWindowDrawScaleBar(FOO_CANVAS_GROUP(ruler->scaleParent), start, end, seq_start, seq_end, zoom_factor, ruler->revcomped, zoomed);

      drawn = TRUE;

      ruler->last_draw_coords.y1 = start;
      ruler->last_draw_coords.y2 = end;
      ruler->last_draw_coords.x1 = 0;
      ruler->last_draw_coords.x2 = width;
      ruler->last_draw_coords.pixels_per_unit_y = ruler->canvas->pixels_per_unit_y;
      ruler->last_draw_coords.revcomped = ruler->revcomped;

#if 0	// this doesn't work, no idea why
	/* somehow wihtout this we get an expose 1 pixel wide, prob due to the style not speciying width
	 * canvasfeaturesets hide the width of features and thier width is onyl taken into account during and update
	 * the foo canvas queue extra exposes on update but somehow this isn't working */
      {
	int x1,x2,y1,y2;
	foo_canvas_w2c(ruler->canvas,0, start,&x1,&y1);
	foo_canvas_w2c(ruler->canvas, width, end, &x2, &y2);
	foo_canvas_request_redraw(ruler->canvas, x1, y1, x2, y2);
      }
#endif
    }

#if FOO_SCALE_DEBUG
  scale_thing = ruler->canvas;
#endif

  return drawn;
}


double zMapWindowScaleCanvasGetWidth(ZMapWindowScaleCanvas ruler)
{
  return ruler->last_draw_coords.x2;
}


void zMapWindowScaleCanvasSetScroll(ZMapWindowScaleCanvas ruler, double x1, double y1, double x2, double y2)
{
  //printf("scale set scroll %f %f %f %f\n", x1, y1, x2, y2);

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

#if NOT_USED
void zmapWindowScaleCanvasZoom(ZMapWindowScaleCanvas ruler, double x, double y)
{

  zmapWindowScaleCanvasSetPixelsPerUnit(ruler, x, y);

  return ;
}
#endif


void zmapWindowScaleCanvasSetRevComped(ZMapWindowScaleCanvas ruler, gboolean revcomped)
{
  zMapAssert(ruler) ;

  ruler->display_forward_coords = TRUE ;
  ruler->revcomped = revcomped;

  return ;
}


#if NOT_USED
/* set seq start, end to handle high zoom level with non 1-based first display coord */
void zmapWindowScaleCanvasSetSpan(ZMapWindowScaleCanvas ruler, int start,int end)
{
  zMapAssert(ruler) ;

  ruler->seq_start = start;
  ruler->seq_end = end;
}
#endif


void zmapWindowScaleCanvasSetVAdjustment(ZMapWindowScaleCanvas ruler, GtkAdjustment *vadjustment)
{

  gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(ruler->scrolled_window), vadjustment) ;

  return ;
}

void zmapWindowScaleCanvasSetPixelsPerUnit(ZMapWindowScaleCanvas ruler, double x, double y)
{
  zMapAssert(ruler && ruler->canvas);

  foo_canvas_set_pixels_per_unit_xy(ruler->canvas, x, y) ;

  return ;
}

#if NOT_USED
void zmapWindowScaleCanvasSetLineHeight(ZMapWindowScaleCanvas ruler,
                                        double line_height)
{
  zMapAssert(ruler);
#ifdef VERBOSE_3
  printf("setLineHeight: setting line_height = %f\n", line_height);
#endif
  ruler->line_height = line_height;

  return ;
}
#endif


#if NOT_USED

void zmapWindowScaleCanvasRepositionHorizon(ZMapWindowScaleCanvas ruler,
                                            double y_position)
{
  /*  double x1, x2; */
  FooCanvasPoints *points = NULL;

  points = foo_canvas_points_new(2);
  points->coords[0] = 0.0;
  points->coords[1] = y_position;
  points->coords[2] = ruler->default_position - 2.0;
  points->coords[3] = y_position;

  if(!ruler->horizon)
    {
      ruler->horizon = foo_canvas_item_new(FOO_CANVAS_GROUP(ruler->scaleParent),
					   foo_canvas_line_get_type(),
					   "fill_color", "red",
					   "width_pixels", 1,
					   "cap_style", GDK_CAP_NOT_LAST,
					   NULL);
      foo_canvas_item_lower_to_bottom(ruler->horizon);
    }

  foo_canvas_item_set(ruler->horizon,
                      "points", points,
                      NULL);
  foo_canvas_item_show(ruler->horizon);
  return ;
}


void zmapWindowScaleCanvasHideHorizon(ZMapWindowScaleCanvas ruler)
{
  if(ruler->horizon)
    foo_canvas_item_hide(ruler->horizon);
  return ;
}

#endif



/* INTERNALS */
static void paneNotifyPositionCB(GObject *pane, GParamSpec *scroll, gpointer user_data)
{
  ZMapWindowScaleCanvas ruler = (ZMapWindowScaleCanvas)user_data;
  FooCanvasItem *scale = NULL;
  gboolean cancel = FALSE;

#ifdef VERBOSE_1
  debug("notifyPos: enter notify for %s.\n", g_param_spec_get_name(scroll));
#endif /* VERBOSE_1 */

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
#if 0
      else if(position == 0 && ruler->visibilityHandlerCB == 0)
        ruler->visibilityHandlerCB =
          g_signal_connect(G_OBJECT(ruler->canvas),
                           "visibility-notify-event",
                           G_CALLBACK(rulerVisibilityHandlerCB),
                           (gpointer)ruler);
#endif
    }

 leave:
#ifdef VERBOSE_1
  debug("notifyPos: leave\n");
#endif /* VERBOSE_1 */
  return ;
}



#if ZOOM_SCROLL
/* And a version which WILL maximise after calling the other one. */
static gboolean rulerMaxVisibilityHandlerCB(GtkWidget *widget, GdkEventExpose *expose, gpointer user_data)
{
  ZMapWindowScaleCanvas ruler = (ZMapWindowScaleCanvas)user_data;
  gboolean handled = FALSE;

  zmapWindowScaleCanvasMaximise(ruler, 0.0, 0.0);

  return handled;
}
#endif


#if ZOOM_SCROLL

static void freeze_notify(ZMapWindowScaleCanvas ruler)
{
  ruler->freeze = TRUE;
  return ;
}
static void thaw_notify(ZMapWindowScaleCanvas ruler)
{
  ruler->freeze = FALSE;
  return ;
}

#endif




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
double zMapWindowDrawScaleBar(FooCanvasGroup *group, int scroll_start, int scroll_end, int seq_start, int seq_end, double zoom_factor, gboolean revcomped, gboolean zoomed)
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
{
  int seq_len;	/* # bases in the scroll region */
  int tick;
  int tick_coord;
  int top, level;	/* we go many levels deep */
  int n_pixels;	/* cannot be more than 30k due to foo */
  int gap;		/* pixels between ticks */
  int digit;		/* which tick out of 10 */
#if SCALE_DEBUG
  int digits;		/* how many digits in fractional part */
#endif
  int nudge;		/* tick is 5th? make bigger */
  char label [32];	/* only need ~8 but there you go */
  char *unit;
  int base;
  PangoFontDescription *font_desc;
  double font_width,font_height;
  int text_height;
  double canvas_coord;
  GdkColor black,grey,*colour;
  double scale_width = 70.0;
  double tick_width;
  int n_levels;	/* number of levels needed for BP resolution */
  int n_hide; 	/* number we can't see at current zoom level */
  int n_text_hide;	/* text we can't display due to overlap */
  char *units[] = { "","","k","M","G" };
  int i;
  int tick_start;
  int tick_end;
  int draw_at;
  int scroll_len;
  int seq_max;	/* biggest slice coord to display */

  ZMapWindowFeaturesetItem featureset;
  GQuark fid;
  char buf[32];
  double text_max,tick_max;	/* max width of each */
  int tick_inc = 4.0;		/* diff in size per level */
  double prev;
  int first;

  static ZMapFeatureTypeStyle scale_style = NULL;
#warning move this to predefined styles code in featureTyoes.c
  /* temporarily create pre-defined style here (needed by Canvasfeatureset)
   * we don't actually use the colours here, maybe we should */
  if(!scale_style)
    {
      scale_style = zMapStyleCreate("scale", "scale bar");

      g_object_set(G_OBJECT(scale_style),
		   ZMAPSTYLE_PROPERTY_MODE, ZMAPSTYLE_MODE_PLAIN,
		   ZMAPSTYLE_PROPERTY_COLOURS, "normal fill white; normal border black",
		   NULL);
      zMapStyleSetDisplayable(scale_style, TRUE);	/* (always drawable) */
    }

#if SCALE_DEBUG
  debug("draw scale %p\n",((FooCanvasItem *) group)->canvas);
#endif

  /* get unique id. and create.... if windows get destroyed then so should the featuresets */
  sprintf(buf,"scalebar%c%p", revcomped ? '-' : '+', ((FooCanvasItem *) group)->canvas);
  fid = g_quark_from_string(buf);

  /* allocate featureset for whole sequence even if zoomed, if the feeatureset does not get re-created on zoom then the extent will be ok */
  featureset = (ZMapWindowFeaturesetItem) zMapWindowCanvasItemFeaturesetGetFeaturesetItem(group, fid, seq_start, seq_end, scale_style, ZMAPSTRAND_NONE, ZMAPFRAME_0, 0, 0);

  seq_len = seq_end - seq_start + 1;
  if(seq_len < 10)	/* we get called on start up w/ no sequence */
    return 0;


  scroll_len = scroll_end - scroll_start + 1;

  gdk_color_parse("black", &black) ;
  gdk_color_parse("grey", &grey) ;

  zMapFoocanvasGetTextDimensions(((FooCanvasItem *) group)->canvas, &font_desc, &font_width, &font_height);
  text_height = (int) (font_height);

  if(!font_height)
    {
      font_height = 14.0;	/* don't just give up ! */
      font_width = 8.0;
      text_height = 14;
      //		zMapLogWarning("DrawScale get font size failed","");
#warning fix the font size to work centrally
    }

  /* get the highest order ticks:  sequence and scroll are in chromosome coordinates but we display slice coordinates.... */
  /* highest order tick is the one displayed not the one in the whole sequence */
  {
    int s = scroll_start - seq_start + 1, e = scroll_end - seq_start + 1;

    if(revcomped)	/* we are counting backwards from the end in slice coordinates */
      {
	int x = s;

	s = scroll_end - e + 1;
	e = scroll_end - x + 1;
      }

    for(tick = 1,n_levels = 0; s != e;tick *= 10, s /= 10, e /= 10, n_levels++)
      {
#if SCALE_DEBUG
	debug("units: %d %d = %d/%d\n", s, e, tick, n_levels);
#endif
	continue;
      }
    tick /= 10;
  }

  /* get the number of pixels: this uses the scroll region */

  n_pixels = (int) (scroll_len * zoom_factor);

  for(n_hide = n_levels,gap = n_pixels * tick / scroll_len; n_hide && gap; gap /= 10,n_hide--)
    continue;


  tick_max = tick_inc * (n_levels - n_hide);	/* max tick width */
#if SCALE_DEBUG
  debug("hide = %d\n", n_hide);
#endif

  /* choose units */
  base = 1;
#if SCALE_DEBUG
  digits = 1;
#endif
  for(i = 1;base <= tick && i < 5;i++,base *= 1000
#if SCALE_DEBUG
	, digits += 2
#endif
      )
    continue;
  unit = units[--i];
  base /= 1000;

#if SCALE_DEBUG
  digits -= 2;
  debug("levels, hide = %d %d %d %d (%s)\n", n_levels,n_hide, base, digits, unit);
#endif

  /* work out space needed for labels */

  for(n_text_hide = n_levels,gap = n_pixels * tick / scroll_len; n_text_hide && gap > text_height * 2; gap /= 10,n_text_hide--)
    continue;

  for(i = 1; n_text_hide-- && i < base; i *= 10)		/* get max resolution of ticks */
    continue;

  seq_max = scroll_end;
  if(revcomped)
    seq_max = seq_end  - scroll_start + 1;
  for(text_max = 1; i < seq_max; text_max++, i *= 10)	/* get number of digits needed */

    continue;
  text_max--;

  if(*unit)		/* for decimal point and suffix */
    text_max += 2;

  if(revcomped)	/* for - sign */
    text_max++;

  text_max *= font_width;


  scale_width = text_max + tick_max;
  zMapWindowCanvasFeaturesetSetWidth(featureset, scale_width);
  {
    FooCanvasItem *foo = (FooCanvasItem *) group;
    foo_canvas_item_request_update(foo);
  }
#if SCALE_DEBUG
  printf("text width: %.1f %.1f %d %d %s\n",font_width, text_max, i, base, unit);
  printf("tick width: %.1f %.1f\n",tick_max, scale_width);
#endif


  /* do we assume there are enough pixels for the highest order ticks?
   * no: if they shrink the window so that you can't read it then don't display
   */

  gap = (int) (((double) n_pixels) * tick / scroll_len );	/* (double) or else would get overflow on big sequences */

#if SCALE_DEBUG
  debug("scale bar: %d bases (%d-%d) @(%d,%d)  tick = %d, levels= %d,%d, %d pixels font %d, zoom %f, rev %d\n",
	seq_len, seq_start, seq_end,  scroll_start, scroll_end,
	tick, n_levels,n_hide, n_pixels, text_height, zoom_factor, revcomped);
#endif


  /* we need a lot of levels to cope with large sequences zoomed in a lot */
  /* to get down to single base level */
  for(level = n_levels,top = 1;level >= 0 && tick > 0;level--, tick /= 10, gap /= 10, top = 0)
    {
      if(!gap)
	break;

      //debug("level %d gap = %d, tick = %d, base = %d, digits = %d\n",level,gap,tick,base,digits);
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
	}			/* NOTE tick_coord is 0 based, it is added to seq_start which is not */
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
#if SCALE_DEBUG
      debug("tick start,end = %d %d\n",tick_start, tick_end);
#endif
      for(tick_coord = tick_start; tick_coord <= tick_end ;tick_coord += tick)
	{
	  int slice_coord = tick_coord;

	  if(tick_coord + seq_start < scroll_start)
	    continue;
	  if(tick_coord + seq_start > scroll_end + 1)
	    break;

	  if(revcomped)
	    slice_coord = seq_end - tick_coord; 	/* -1 based coord off the sequence end */

	  digit =  (slice_coord / tick) % 10;

	  if(top || digit)			/* don't draw ticks over higher order ticks */
	    {
	      nudge = (digit == 5 || digit == -5);
	      if((nudge && gap > 1) || gap > 2)	/* don't crowd ticks together */
		{
		  /* draw the tick, with a number if appropriate */

		  if(revcomped)
		    draw_at = tick_coord + seq_start + 1;
		  else
		    draw_at = tick_coord + seq_start - 1;

		  label[0] = 0;
		  canvas_coord = (double) draw_at;
#if SCALE_DEBUG
		  if(top) debug("coord: %.1f %d (%d,%d) = %d\n", canvas_coord, digit, seq_start,seq_end, tick_coord);
#endif
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
		      //	      				FooCanvasItem *item = NULL;
		      //						double x = 0.0;
		      int num,frac;
		      char *sign = "";
		      double offset = revcomped ? -0.5 : 0.5;
		      ZMapWindowCanvasGraphics gfx;

		      num = slice_coord / base;
		      //						if(num < 0)
		      //							num = -num;
		      frac = slice_coord % base;
		      //						if(frac < 0)
		      //							frac = -frac;
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
                          //p = label + sprintf(label,"%s%d.%0*d",sign,num,digits,frac);
                          float val = (float)slice_coord / (float)base;
                          p = label + sprintf(label,"%s%0*f",sign, digits, val);

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

#if SCALE_DEBUG
		  if(top) debug("tick @ %d %f (%d, %d) level %d: %s\n",tick_coord,canvas_coord,base,tick,level,label);
#endif
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
      draw_at = tick_coord - 1 ;		/* extra -1 to cover first base which is between canvas 0 and 1 */

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
#if SCALE_DEBUG
      else
	debug("lines start at %.1f\n",canvas_coord);
#endif
      first = 0;
      prev = canvas_coord;

      if(tick_coord >= scroll_end + 1)
	break;
    }
#if SCALE_DEBUG
  debug("lines stop at %.1f (%d %d)\n",canvas_coord, tick_coord, scroll_end);
#endif

  return scale_width;
}



