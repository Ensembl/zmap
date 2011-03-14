/*  File: zmapWindowRuler.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implements the scale bar shown for sequences.
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Jan 21 14:36 2008 (rds)
 * Created: Thu Mar  9 16:09:18 2006 (rds)
 * CVS info:   $Id: zmapWindowRuler.c,v 1.16 2011-03-14 11:35:18 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <zmapWindow_P.h>
#include <ZMap/zmapUtils.h>
#include <string.h>
#include <math.h>

#define DEFAULT_PANE_POSITION 100
#define ZMAP_SCALE_MINORS_PER_MAJOR 10
#define ZMAP_FORCE_FIVES TRUE
#define ZMAP_SCALE_BAR_GROUP_TYPE_KEY "scale_group_type"

/* For printing lots to the terminal and debugging
#define VERBOSE_1
#define VERBOSE_2
#define VERBOSE_3
*/

/* Privatise  */
typedef struct _ZMapWindowRulerCanvasStruct
{
  FooCanvas *canvas;            /* The Canvas */
  FooCanvasItem *scaleParent;  /* The group we draw into, without it we draw again */
  FooCanvasItem *horizon;
  GtkWidget *scrolled_window;   /* The scrolled window */

  int default_position;
  gboolean freeze, text_left;
  double line_height;
  gulong visibilityHandlerCB;

  PangoFont *font;
  PangoFontDescription *font_desc;
  int font_width;

  gboolean revcomped;
  int seq_start,seq_end;

  struct
  {
    double y1, y2;
    double pixels_per_unit_y;
    gboolean revcomped;
  }last_draw_coords;

  ZMapWindowRulerCanvasCallbackList callbacks; /* The callbacks we need to call sensible window functions... */

  guint display_forward_coords : 1;					    /* Has a coord display origin been set. */
} ZMapWindowRulerCanvasStruct;

/* Just a collection of ints, boring but makes it easier */
typedef struct _ZMapScaleBarStruct
{
  int base;                     /* One of 1 1e3 1e6 1e9 1e12 1e15 */
  int major;                    /* multiple of base */
  int minor;                    /* major / ZMAP_SCALE_MINORS_PER_MAJOR */
  char *unit;                   /* One of bp k M G T P */

  gboolean force_multiples_of_five;
  double zoom_factor;
  double projection_factor;

  int start;
  int end;
  int seq_start;
  int seq_end;

  int offset;           /* where to display rebased coordinates */

  gboolean display_forward_coords;	/* Has a coord display origin been set. */
  gboolean revcomped;
} ZMapScaleBarStruct, *ZMapScaleBar;

typedef enum
  {
    ZMAP_SCALE_BAR_GROUP_NONE,
    ZMAP_SCALE_BAR_GROUP_LEFT,
    ZMAP_SCALE_BAR_GROUP_RIGHT,
  } ZMapScaleBarGroupType;

static gboolean initialiseScale(ZMapScaleBar ruler_out,
                                double start, double end,
                                int seq_start, int seq_end,
                                double zoom, double line,
				gboolean display_forward_coords, gboolean revcomped,
                                double projection_factor);
static void drawScaleBar(ZMapScaleBar scaleBar,
                         FooCanvasGroup *group,
                         PangoFontDescription *font,
                         gboolean text_left);
static FooCanvasItem *rulerCanvasDrawScale(ZMapWindowRulerCanvas ruler,
                                           double start,
                                           double end);

static void positionLeftRight(FooCanvasGroup *left, FooCanvasGroup *right);
static void paneNotifyPositionCB(GObject *pane, GParamSpec *scroll, gpointer user_data);
static gboolean rulerVisibilityHandlerCB(GtkWidget *widget, GdkEventExpose *expose, gpointer user_data);
static gboolean rulerMaxVisibilityHandlerCB(GtkWidget *widget, GdkEventExpose *expose, gpointer user_data);
static void freeze_notify(ZMapWindowRulerCanvas ruler);
static void thaw_notify(ZMapWindowRulerCanvas ruler);


/* CODE... */

ZMapWindowRulerCanvas zmapWindowRulerCanvasCreate(ZMapWindowRulerCanvasCallbackList callbacks)
{
  ZMapWindowRulerCanvas ruler = NULL;

  ruler = g_new0(ZMapWindowRulerCanvasStruct, 1);

  /* Now we make the ruler canvas */
  ruler->canvas    = FOO_CANVAS(foo_canvas_new());
  ruler->callbacks = g_new0(ZMapWindowRulerCanvasCallbackListStruct, 1);
  ruler->callbacks->paneResize = callbacks->paneResize;
  ruler->callbacks->user_data  = callbacks->user_data;

  ruler->default_position = DEFAULT_PANE_POSITION;
  ruler->text_left        = TRUE; /* TRUE = put the text on the left! */

  ruler->visibilityHandlerCB = 0;

  ruler->last_draw_coords.y1 = ruler->last_draw_coords.y2 = 0.0;

  return ruler;
}

/* Packs the canvas in the supplied parent object... */
void zmapWindowRulerCanvasInit(ZMapWindowRulerCanvas ruler,
                               GtkWidget *paned,
                               GtkAdjustment *vadjustment)
{
  GtkWidget *scrolled = NULL;

  ruler->scrolled_window = scrolled = gtk_scrolled_window_new(NULL, NULL);

  zmapWindowRulerCanvasSetVAdjustment(ruler, vadjustment);

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
                               g_list_append(NULL, "Monospace"), 10, PANGO_WEIGHT_NORMAL,
                               &(ruler->font), &(ruler->font_desc)))
    printf("Couldn't get fixed width font\n");
  else
    zMapGUIGetFontWidth(ruler->font, &(ruler->font_width));


  return ;
}

void zmapWindowRulerCanvasOpenAndMaximise(ZMapWindowRulerCanvas ruler)
{
  int open = 10;

  /* If there's one set, disconnect it */
  if(ruler->visibilityHandlerCB)
    g_signal_handler_disconnect(G_OBJECT(ruler->canvas), ruler->visibilityHandlerCB);

  /* Reconnect the maximising one... */
  ruler->visibilityHandlerCB =
    g_signal_connect(G_OBJECT(ruler->canvas),
                     "visibility-notify-event",
                     G_CALLBACK(rulerMaxVisibilityHandlerCB),
                     (gpointer)ruler);

  /* Now open it, which will result in the above getting called... */
  if(ruler->callbacks->paneResize &&
     ruler->callbacks->user_data)
    (*(ruler->callbacks->paneResize))(&open, ruler->callbacks->user_data);

  return ;
}

void zmapWindowRulerCanvasMaximise(ZMapWindowRulerCanvas ruler, double y1, double y2)
{
  double x2, max_x2,
    ix1 = 0.0,
    iy1 = y1,
    iy2 = y2;

  if(ruler->scaleParent)
    {
      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(ruler->scaleParent),
                                 NULL, NULL, &max_x2, NULL);
      if(iy1 == iy2 && iy1 == ix1)
        foo_canvas_get_scroll_region(FOO_CANVAS(ruler->canvas),
                                     NULL, &iy1, &x2, &iy2);
      else
        foo_canvas_get_scroll_region(FOO_CANVAS(ruler->canvas),
                                     NULL, NULL, &x2, NULL);
#ifdef VERBOSE_1
      printf("rulerCanvasMaximise: %f %f - %f %f %f %f\n", x2, max_x2, ix1, x2, iy1, iy2);
#endif /* VERBOSE_1 */
      foo_canvas_set_scroll_region(FOO_CANVAS(ruler->canvas),
                                   ix1, iy1, max_x2, iy2);

      if(max_x2 > 0.0)
        {
          ruler->default_position = max_x2;

          if(ruler->callbacks->paneResize &&
             ruler->callbacks->user_data)
            {
              int floored = (int)max_x2;
              freeze_notify(ruler);
              (*(ruler->callbacks->paneResize))(&floored, ruler->callbacks->user_data);
              thaw_notify(ruler);
            }
        }
    }

  return ;
}


/* I don't like the dependence on window here! */
gboolean zmapWindowRulerCanvasDraw(ZMapWindowRulerCanvas ruler, double start, double end, gboolean force)
{
  gboolean drawn = FALSE;
  zMapAssert(ruler && ruler->canvas);

  if(force || ruler->last_draw_coords.y1 != start || ruler->last_draw_coords.y2 != end ||
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

      ruler->scaleParent = rulerCanvasDrawScale(ruler, start, end);

      /* We either need to do this check or
       * double test = 0.0;
       * foo_canvas_item_get_bounds((FOO_CANVAS_ITEM(ruler->scaleParent)), NULL, NULL, &test, NULL);
       * if(test == 0.0)
       *   g_signal_connect(...);
       */
      if(ruler->visibilityHandlerCB != 0)
	{
	  g_signal_handler_disconnect(G_OBJECT(ruler->canvas), ruler->visibilityHandlerCB);
	  ruler->visibilityHandlerCB = 0;
	}

      if(ruler->visibilityHandlerCB == 0 &&
	 !((FOO_CANVAS_ITEM(ruler->scaleParent))->object.flags & (FOO_CANVAS_ITEM_REALIZED | FOO_CANVAS_ITEM_MAPPED)))
	{
	  ruler->visibilityHandlerCB = g_signal_connect(G_OBJECT(ruler->canvas),
							"visibility-notify-event",
							G_CALLBACK(rulerVisibilityHandlerCB),
							(gpointer)ruler);
	}
      drawn = TRUE;
    }

  ruler->last_draw_coords.y1 = start;
  ruler->last_draw_coords.y2 = end;
  ruler->last_draw_coords.pixels_per_unit_y = ruler->canvas->pixels_per_unit_y;
  ruler->last_draw_coords.revcomped = ruler->revcomped;

  return drawn;
}

void zmapWindowRulerCanvasZoom(ZMapWindowRulerCanvas ruler, double x, double y)
{

  zmapWindowRulerCanvasSetPixelsPerUnit(ruler, x, y);

  return ;
}



void zmapWindowRulerCanvasSetRevComped(ZMapWindowRulerCanvas ruler, gboolean revcomped)
{
  zMapAssert(ruler) ;

  ruler->display_forward_coords = TRUE ;
  ruler->revcomped = revcomped;

  return ;
}


/* set seq start, end to handle high zoom level with non 1-based first display coord */
void zmapWindowRulerCanvasSetSpan(ZMapWindowRulerCanvas ruler, int start,int end)
{
  zMapAssert(ruler) ;

  ruler->seq_start = start;
  ruler->seq_end = end;
}



void zmapWindowRulerCanvasSetVAdjustment(ZMapWindowRulerCanvas ruler, GtkAdjustment *vadjustment)
{

  gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(ruler->scrolled_window), vadjustment) ;

  return ;
}

void zmapWindowRulerCanvasSetPixelsPerUnit(ZMapWindowRulerCanvas ruler, double x, double y)
{
  zMapAssert(ruler && ruler->canvas);

  foo_canvas_set_pixels_per_unit_xy(ruler->canvas, x, y) ;

  return ;
}

void zmapWindowRulerCanvasSetLineHeight(ZMapWindowRulerCanvas ruler,
                                        double line_height)
{
  zMapAssert(ruler);
#ifdef VERBOSE_3
  printf("setLineHeight: setting line_height = %f\n", line_height);
#endif
  ruler->line_height = line_height;

  return ;
}

void zmapWindowRulerCanvasRepositionHorizon(ZMapWindowRulerCanvas ruler,
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

void zmapWindowRulerCanvasHideHorizon(ZMapWindowRulerCanvas ruler)
{
  if(ruler->horizon)
    foo_canvas_item_hide(ruler->horizon);
  return ;
}

void zmapWindowRulerGroupDraw(FooCanvasGroup *parent, double project_at, gboolean revcomped, double start, double end)
{
  FooCanvas *canvas = NULL;
  ZMapScaleBarStruct scaleBar = {0};
  PangoFontDescription *font_desc = NULL;
  PangoFont *font = NULL;
  double font_width  = 0.0;
  double line_height = 0.0;
  double zoom_factor = 0.0;

  canvas = FOO_CANVAS(FOO_CANVAS_ITEM(parent)->canvas);

  zoom_factor = FOO_CANVAS(canvas)->pixels_per_unit_y;

  if(!zMapGUIGetFixedWidthFont(GTK_WIDGET(canvas),
                               g_list_append(NULL, "Monospace"), 10, PANGO_WEIGHT_NORMAL,
                               &font, &font_desc))
    printf("Couldn't get fixed width font\n");
  else
    {
      FooCanvasItem *item ;
      PangoLayout *playout;
      int iwidth = -1, iheight = -1;

      item = foo_canvas_item_new(parent,
                                 FOO_TYPE_CANVAS_TEXT,
                                 "x",         0.0,
                                 "y",         0.0,
                                 "text",      "X",
                                 "font_desc", font_desc,
                                 NULL);
      playout = FOO_CANVAS_TEXT(item)->layout;

      pango_layout_get_pixel_size( playout , &iwidth, &iheight );

      font_width  = (double)iwidth;
      line_height = (double)iheight;

      gtk_object_destroy(GTK_OBJECT(item));
    }

  //line_height      = line_height / zoom_factor;

  if(initialiseScale(&scaleBar, start, end, (int) start, (int) end, zoom_factor, line_height, TRUE, revcomped, project_at))
    drawScaleBar(&scaleBar, FOO_CANVAS_GROUP(parent), font_desc, TRUE);


  return ;
}


/* INTERNALS */
static void paneNotifyPositionCB(GObject *pane, GParamSpec *scroll, gpointer user_data)
{
  ZMapWindowRulerCanvas ruler = (ZMapWindowRulerCanvas)user_data;
  FooCanvasItem *scale = NULL;
  gboolean cancel = FALSE;

#ifdef VERBOSE_1
  printf("notifyPos: enter notify for %s.\n", g_param_spec_get_name(scroll));
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
      else if(position == 0 && ruler->visibilityHandlerCB == 0)
        ruler->visibilityHandlerCB =
          g_signal_connect(G_OBJECT(ruler->canvas),
                           "visibility-notify-event",
                           G_CALLBACK(rulerVisibilityHandlerCB),
                           (gpointer)ruler);
    }

 leave:
#ifdef VERBOSE_1
  printf("notifyPos: leave\n");
#endif /* VERBOSE_1 */
  return ;
}

static void positionLeftRight(FooCanvasGroup *left, FooCanvasGroup *right)
{
  double x2 = 0.0;

  my_foo_canvas_item_goto(FOO_CANVAS_ITEM(left), &x2, NULL);

  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(left),
                             NULL, NULL, &x2, NULL);
  my_foo_canvas_item_goto(FOO_CANVAS_ITEM(right), &x2, NULL);

  return ;
}

static gboolean rulerVisibilityHandlerCB(GtkWidget *widget, GdkEventExpose *expose, gpointer user_data)
{
  ZMapWindowRulerCanvas ruler = (ZMapWindowRulerCanvas)user_data;
  gboolean handled  = FALSE;

#ifdef VERBOSE_1
  printf("rulerVisibilityHandlerCB: enter\n");
#endif /* VERBOSE_1 */
  if(ruler->scaleParent)
    {
      GList *list = NULL;
      FooCanvasGroup *left = NULL, *right = NULL;

#ifdef VERBOSE_1
      printf("rulerVisibilityHandlerCB: got a scale bar\n");
#endif /* VERBOSE_1 */

      list = FOO_CANVAS_GROUP(ruler->scaleParent)->item_list;
      while(list)
        {
          ZMapScaleBarGroupType type = ZMAP_SCALE_BAR_GROUP_NONE;
          type = GPOINTER_TO_INT( g_object_get_data(G_OBJECT(list->data), ZMAP_SCALE_BAR_GROUP_TYPE_KEY) );
          switch(type)
            {
            case ZMAP_SCALE_BAR_GROUP_LEFT:
              left = FOO_CANVAS_GROUP(list->data);
              break;
            case ZMAP_SCALE_BAR_GROUP_RIGHT:
              right = FOO_CANVAS_GROUP(list->data);
              break;
            default:
              break;
            }
          list = list->next;
        }
      if(left && right)
        {
          positionLeftRight(left, right);
          g_signal_handler_disconnect(G_OBJECT(widget), ruler->visibilityHandlerCB);
          ruler->visibilityHandlerCB = 0; /* Make sure we reset this! */
        }
    }
#ifdef VERBOSE_1
  printf("rulerVisibilityHandlerCB: leave, handled = %s\n", (handled ? "TRUE" : "FALSE"));
#endif /* VERBOSE_1 */

  return handled;
}

/* And a version which WILL maximise after calling the other one. */
static gboolean rulerMaxVisibilityHandlerCB(GtkWidget *widget, GdkEventExpose *expose, gpointer user_data)
{
  ZMapWindowRulerCanvas ruler = (ZMapWindowRulerCanvas)user_data;
  gboolean handled = FALSE;

  handled = rulerVisibilityHandlerCB(widget, expose, user_data);

  zmapWindowRulerCanvasMaximise(ruler, 0.0, 0.0);

  return handled;
}


static void freeze_notify(ZMapWindowRulerCanvas ruler)
{
  ruler->freeze = TRUE;
  return ;
}
static void thaw_notify(ZMapWindowRulerCanvas ruler)
{
  ruler->freeze = FALSE;
  return ;
}


/* =================================================== */
static FooCanvasItem *rulerCanvasDrawScale(ZMapWindowRulerCanvas ruler,
                                           double start,
                                           double end)
{
  FooCanvasItem *group = NULL ;
  ZMapScaleBarStruct scaleBar = { 0 };
  double zoom_factor = 0.0;
  double height;

  group = foo_canvas_item_new(foo_canvas_root(ruler->canvas),
			      foo_canvas_group_get_type(),
			      "x", 0.0,
			      "y", 0.0,
			      NULL) ;

  zoom_factor = FOO_CANVAS(ruler->canvas)->pixels_per_unit_y;
  height      = ruler->line_height / zoom_factor;

  if(initialiseScale(&scaleBar, start, end, ruler->seq_start, ruler->seq_end, zoom_factor, height, ruler->display_forward_coords, ruler->revcomped, 1.0))
    drawScaleBar(&scaleBar, FOO_CANVAS_GROUP(group), ruler->font_desc, ruler->text_left);

  if(scaleBar.unit)
    g_free(scaleBar.unit);

  return group ;
}

static gboolean initialiseScale(ZMapScaleBar scale_out,
                                double start,
                                double end,
                                int seq_start, int seq_end,
                                double zoom,
                                double line,
				gboolean display_forward_coords,gboolean revcomped,
                                double projection_factor)
{
  gboolean good = TRUE;
  int majorUnits[]      = {1   , 1000, 1000000, 1000000000, 0};
  char *majorAlpha[]    = {"", " k" , " M"    , " G"};
  int unitIndex         = 0;
  int speed_factor      = 4;
  int *iter;
  double basesPerPixel, absolute_min, lineheight, dtmp;
  int majorSize, diff ;
  int i, tmp;
  int basesBetween;

  absolute_min = lineheight = basesPerPixel = dtmp = 0.0;

  scale_out->start  = (int)start;
  scale_out->end    = (int)end;
  scale_out->seq_start = seq_start;
  scale_out->seq_end = seq_end;
  scale_out->force_multiples_of_five = ZMAP_FORCE_FIVES;
  scale_out->zoom_factor             = zoom * projection_factor;
  scale_out->display_forward_coords  = display_forward_coords ;
  scale_out->revcomped               = revcomped ;
  scale_out->projection_factor       = projection_factor;

  /* line * zoom is constant @ 14 on my machine,
   * simply increasing this decreases the number of majors (makes it faster),
   * hence inclusion of 'speed_factor'. May want to refine what looks good
   * 2 or 4 are reasonable, while 10 is way OTT!
   * 1 gives precision issues when drawing the mnor ticks. At reasonable zoom
   * it gives about 1 pixel between minor ticks and this sometimes equates to
   * two pixels (precision) which looks odd.
   */
  if(speed_factor >= 1)
    dtmp = line * zoom * speed_factor;
  else
    dtmp = line * zoom;

  lineheight = ceil(dtmp);

  /* Abosulte minimum of (ZMAP_SCALE_MINORS_PER_MAJOR * 2) + 1
   * Explain: pixel width for each line, plus one to see it + 1 for good luck!
   * Require 1 * text line height + 1 so they're not merged
   *
   */
  diff          = scale_out->end - scale_out->start + 1;
  basesPerPixel = diff / (diff * scale_out->zoom_factor);

  lineheight  += 1.0;
  absolute_min = (double)((ZMAP_SCALE_MINORS_PER_MAJOR * 2) + 1);
  basesBetween = (int)floor((lineheight >= absolute_min ?
                             lineheight : absolute_min) * basesPerPixel);
  /* Now we know we can put a major tick every basesBetween pixels */
  /*  printf("diff %d, bpp %f\n", diff, basesPerPixel); */

  for(i = 0, iter = majorUnits; *iter != 0; iter++, i++){
    int mod;
    mod = basesBetween % majorUnits[i];

    if(mod && mod != basesBetween)
      unitIndex = i;
    else if (basesBetween > majorUnits[i] && !mod)
      unitIndex = i;
  }
  /*  */
  /* Now we think we know what the major should be */
  majorSize  = majorUnits[unitIndex];
  scale_out->base = majorSize;

  tmp = ceil((basesBetween / majorSize));

  /* This isn't very elegant, and is kind of a reverse of the previous
   * logic used */
  if(scale_out->force_multiples_of_five == TRUE)
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

  scale_out->base  = majorUnits[unitIndex];
  scale_out->major = majorSize;
  scale_out->unit  = g_strdup( majorAlpha[unitIndex] );

  if(scale_out->major >= ZMAP_SCALE_MINORS_PER_MAJOR)
    scale_out->minor = majorSize / ZMAP_SCALE_MINORS_PER_MAJOR;
  else
    scale_out->minor = 1;

  return good;
}

static void drawScaleBar(ZMapScaleBar scaleBar,
                         FooCanvasGroup *group,
                         PangoFontDescription *font,
                         gboolean text_left)
{
  int i, n, width = 0;
  GdkColor black, white, yellow ;
  double scale_maj, scale_line, scale_min;
  FooCanvasPoints *points ;
  FooCanvasGroup *lines = NULL, *text = NULL;
  double scale_over = 70.0;
  GString *digitUnitStr = NULL;
  /* 20 should be enough for anyone */
  digitUnitStr = g_string_sized_new(20);

  lines = FOO_CANVAS_GROUP(foo_canvas_item_new(group,
            foo_canvas_group_get_type(),
            NULL));
  text  = FOO_CANVAS_GROUP(foo_canvas_item_new(group,
            foo_canvas_group_get_type(),
            NULL));

  if(text_left)
    {
      scale_line = scale_over;
      scale_maj  = scale_over - 10.0;
      scale_min  = scale_over - 5.0;
    }
  else
    {
      scale_over = 0.0;
      scale_line = scale_over;
      scale_min  = scale_over + 5.0;
      scale_maj  = scale_line + 10.0;
    }

  gdk_color_parse("black", &black) ;
  gdk_color_parse("white", &white) ;
  gdk_color_parse("yellow", &yellow) ;

#if !ODD_CHOICE_OF_START
  n = ceil( (scaleBar->start %
             (scaleBar->major < ZMAP_SCALE_MINORS_PER_MAJOR
              ? ZMAP_SCALE_MINORS_PER_MAJOR
              : scaleBar->major
              )
             ) / scaleBar->minor
            );

  i = (scaleBar->start - (scaleBar->start % scaleBar->minor));
#else
      /* as the coords we display are 1-based and the coords we iterate through and draw at are different
       * we need to adjust the major/minor to appear at sensible places in display coords
       */
  {
  int y1 = zmapWindowCoordFromOriginRaw(scaleBar->seq_start,scaleBar->seq_end,scaleBar->start,scaleBar->revcomped);

  if(y1 < 0)      /* ie revcomped */
  {
      n = (-(y1) % scaleBar->major) / scaleBar->minor;      /* get no of minors till first major */
      i = y1 - scaleBar->minor + ((-y1) % scaleBar->minor); /* start on first minor */
  }
  else
  {
      n = (y1 % scaleBar->major) / scaleBar->minor;         /* get no of minors till first major */
      i = y1 - (y1 % scaleBar->minor);                      /* start on first minor */
  }
printf("i,n,y1 = %d %d %d (%d %d %d)\n",i,n,y1,scaleBar->start,scaleBar->minor,scaleBar->major);
  }
#endif

  /* What I want to do here rather than worry about width of
   * characters is to have two groups/columns. One for the
   * numbers/units and one for the line/marks, but I haven't worked
   * out how best to right align the numbers/units without relying on
   * width.  Until then this works.
   */

  /* < rather than <= to avoid final minor as end is seq_end + 1 */
  for( ; i < scaleBar->end; i+=scaleBar->minor, n++)
    {
      /* More conditionals than I intended here... */
      /* char *digitUnit = NULL; */

            /* Save a lot of casting, typing and multiplication... */
            /* offset to place the ruler in the same place as the features */
      double i_d = (double) (i)  * scaleBar->projection_factor ;
      int scale_pos ;
      double scale_left = scale_min;

      if (i < scaleBar->start)
        {
          i_d = (double) (scaleBar->start) * scaleBar->projection_factor;
        }

      if (n % ZMAP_SCALE_MINORS_PER_MAJOR) /* Minors */
        {
            if (i == scaleBar->start && n < 5)
            {                   /* n < 5 to stop overlap of digitUnit at some zooms */

	      if (scaleBar->display_forward_coords)
		  scale_pos = zmapWindowCoordFromOriginRaw(scaleBar->seq_start, scaleBar->seq_end, scaleBar->start, scaleBar->revcomped) ;

	      else
		  scale_pos = scaleBar->start ;
            g_string_printf(digitUnitStr, "%d", scale_pos);
            }
        }
      else                      /* Majors */
        {
          if (i && i >= scaleBar->start)
            {
              scale_left = scale_maj;

	      if (scaleBar->display_forward_coords)
		{
              scale_pos = zmapWindowCoordFromOriginRaw(scaleBar->seq_start, scaleBar->seq_end, i, scaleBar->revcomped) ;
		  scale_pos = (scale_pos / scaleBar->base) ;
		}
	      else
		scale_pos = (i / scaleBar->base) ;

              g_string_printf(digitUnitStr,
                              "%d%s",
                              scale_pos,
                              scaleBar->unit) ;
            }
          else
            {
	      /* I guess this means "do the first one".... */
	      if (scaleBar->display_forward_coords)
		{
              scale_pos = zmapWindowCoordFromOriginRaw(scaleBar->seq_start, scaleBar->seq_end, scaleBar->start, scaleBar->revcomped) ;

		  if (abs(scale_pos) > 1)
		    {
		      scale_pos = (scale_pos / scaleBar->base) ;

		      g_string_printf(digitUnitStr,
				      "%d%s",
				      scale_pos,
				      scaleBar->unit);
		    }
		  else
		    g_string_printf(digitUnitStr, "%d", scale_pos);
		}
	      else
		{
		  scale_pos = scaleBar->start ;
		  g_string_printf(digitUnitStr, "%d", scale_pos);
		}
            }
        }

      /* =========================================================== */

      zMapDrawLine(lines, scale_left,  i_d, scale_line, i_d, &black, 1.0) ;

      if(digitUnitStr->len > 0)
        {
          FooCanvasItem *item = NULL;
          double x = 0.0;

          if(text_left)
            {
              /* width = strlen(digitUnit); */
              width = digitUnitStr->len;
              x = ((scale_maj) - (5.0 * (double)width));
            }

          item = foo_canvas_item_new(text,
                                     foo_canvas_text_get_type(),
                                     "x",          x,
                                     "y",          i_d,
                                     "text",       digitUnitStr->str,
                                     "font_desc",  font,
                                     "fill_color", "black",
                                     "anchor",     (text_left ? GTK_ANCHOR_CENTER : GTK_ANCHOR_W ),
                                     NULL);
          g_string_truncate(digitUnitStr, 0);
          /* g_free(digitUnit); */
        }

    }

  g_string_free(digitUnitStr, TRUE);

  /* draw the vertical line of the scalebar. */
    /* allocate a new points array */
  points = foo_canvas_points_new(2) ;

  /* fill out the points */
  points->coords[0] = scale_line ;
  points->coords[1] = scaleBar->start * scaleBar->projection_factor;
  points->coords[2] = scale_line ;
  points->coords[3] = scaleBar->end * scaleBar->projection_factor;


//zMapLogWarning("scale extent %f %f",points->coords[1],points->coords[3]);

  /* draw the line, unfortunately we need to use GDK_CAP_PROJECTING here to make it work */
  foo_canvas_item_new(lines,
                      foo_canvas_line_get_type(),
                      "points", points,
                      "fill_color_gdk", &black,
                      "width_units", 1.0,
                      "cap_style", GDK_CAP_PROJECTING,
                      NULL);

  /* free the points array */
  foo_canvas_points_free(points) ;

  /* Arrange groups to remove overlapping */
  if(text_left)
    {
      g_object_set_data(G_OBJECT(text),
                        ZMAP_SCALE_BAR_GROUP_TYPE_KEY,
                        GINT_TO_POINTER(ZMAP_SCALE_BAR_GROUP_LEFT));
      g_object_set_data(G_OBJECT(lines),
                        ZMAP_SCALE_BAR_GROUP_TYPE_KEY,
                        GINT_TO_POINTER(ZMAP_SCALE_BAR_GROUP_RIGHT));
      positionLeftRight(text, lines);
    }
  else
    {
      g_object_set_data(G_OBJECT(lines),
                        ZMAP_SCALE_BAR_GROUP_TYPE_KEY,
                        GINT_TO_POINTER(ZMAP_SCALE_BAR_GROUP_LEFT));
      g_object_set_data(G_OBJECT(text),
                        ZMAP_SCALE_BAR_GROUP_TYPE_KEY,
                        GINT_TO_POINTER(ZMAP_SCALE_BAR_GROUP_RIGHT));
      positionLeftRight(lines, text);

    }

  return ;
}




