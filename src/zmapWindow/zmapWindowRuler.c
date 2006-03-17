/*  File: zmapWindowRuler.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2006
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Mar 17 17:04 2006 (rds)
 * Created: Thu Mar  9 16:09:18 2006 (rds)
 * CVS info:   $Id: zmapWindowRuler.c,v 1.3 2006-03-17 17:19:52 rds Exp $
 *-------------------------------------------------------------------
 */

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
  GtkWidget *scrolled_window;   /* The scrolled window */

  int default_position;
  gboolean freeze, text_left;
  double line_height;
  gulong visibilityHandlerCB;

  PangoFont *font;
  PangoFontDescription *font_desc;

  ZMapWindowRulerCanvasCallbackList callbacks; /* The callbacks we need to call sensible window functions... */

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

  int start;
  int end;
  double top;
  double bottom;
} ZMapScaleBarStruct, *ZMapScaleBar;

typedef enum
  {
    ZMAP_SCALE_BAR_GROUP_NONE,
    ZMAP_SCALE_BAR_GROUP_LEFT,
    ZMAP_SCALE_BAR_GROUP_RIGHT,
  } ZMapScaleBarGroupType;

static gboolean initialiseScale(ZMapScaleBar obj_out,
                                double start, double end, 
                                double zoom, double line);
static void drawScaleBar(ZMapScaleBar scaleBar, 
                         FooCanvasGroup *group, 
                         PangoFontDescription *font,
                         gboolean text_left);
static FooCanvasItem *rulerCanvasDrawScale(FooCanvas *canvas,
                                           PangoFontDescription *font,
                                           double start, 
                                           double end,
                                           double height,
                                           gboolean text_left);

static void positionLeftRight(FooCanvasGroup *left, FooCanvasGroup *right);
static void paneNotifyPositionCB(GObject *pane, GParamSpec *scroll, gpointer user_data);
static gboolean rulerVisibilityHandlerCB(GtkWidget *widget, GdkEventExpose *expose, gpointer user_data);
static gboolean rulerMaxVisibilityHandlerCB(GtkWidget *widget, GdkEventExpose *expose, gpointer user_data);
static void freeze_notify(ZMapWindowRulerCanvas obj);
static void thaw_notify(ZMapWindowRulerCanvas obj);


/* CODE... */

ZMapWindowRulerCanvas zmapWindowRulerCanvasCreate(ZMapWindowRulerCanvasCallbackList callbacks)
{
  ZMapWindowRulerCanvas obj = NULL;

  obj = g_new0(ZMapWindowRulerCanvasStruct, 1);

  /* Now we make the ruler canvas */
  obj->canvas = FOO_CANVAS(foo_canvas_new());
  obj->callbacks = g_new0(ZMapWindowRulerCanvasCallbackListStruct, 1);
  obj->callbacks->paneResize = callbacks->paneResize;
  obj->callbacks->user_data  = callbacks->user_data;

  obj->default_position = DEFAULT_PANE_POSITION;
  obj->text_left = FALSE;

  obj->visibilityHandlerCB = 0;

  return obj;
}

/* Packs the canvas in the supplied parent object... */
void zmapWindowRulerCanvasInit(ZMapWindowRulerCanvas obj,
                               GtkWidget *paned,
                               GtkAdjustment *vadjustment)
{
  GtkWidget *scrolled = NULL;

  obj->scrolled_window = scrolled = gtk_scrolled_window_new(NULL, NULL);

  zmapWindowRulerCanvasSetVAdjustment(obj, vadjustment);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW( scrolled ), 
                                 GTK_POLICY_ALWAYS, GTK_POLICY_NEVER);

  gtk_container_add(GTK_CONTAINER(scrolled), GTK_WIDGET(obj->canvas)) ;

  gtk_paned_pack1(GTK_PANED(paned), scrolled, FALSE, TRUE);

  /* ... I think we actually want to set to zero! until draw occurs ... */
  if(obj->callbacks->paneResize && obj->callbacks->user_data)
    (*(obj->callbacks->paneResize))(&(obj->default_position), obj->callbacks->user_data);

  g_object_connect(G_OBJECT(paned), 
                   "signal::notify::position", G_CALLBACK(paneNotifyPositionCB), (gpointer)obj,
                   NULL);

  if(!zMapGUIGetFixedWidthFont(GTK_WIDGET(paned), 
                               g_list_append(NULL, "Monospace"), 10, PANGO_WEIGHT_NORMAL,
                               &(obj->font), &(obj->font_desc)))
    printf("Couldn't get fixed width font\n");

  
#ifdef RDS_DONT_INCLUDE
  g_object_connect(G_OBJECT(obj->canvas),
                   "signal::expose-event", G_CALLBACK(rulerVisibilityHandlerCB), (gpointer)obj,
                   NULL);

  gtk_widget_modify_bg(GTK_WIDGET(obj->canvas), 
                       GTK_STATE_NORMAL, &(window->canvas_background)) ;
#endif

  return ;
}

void zmapWindowRulerCanvasOpenAndMaximise(ZMapWindowRulerCanvas obj)
{
  int open = 10;

  /* If there's one set, disconnect it */
  if(obj->visibilityHandlerCB)
    g_signal_handler_disconnect(G_OBJECT(obj->canvas), obj->visibilityHandlerCB);

  /* Reconnect the maximising one... */
  obj->visibilityHandlerCB = 
    g_signal_connect(G_OBJECT(obj->canvas),
                     "visibility-notify-event", 
                     G_CALLBACK(rulerMaxVisibilityHandlerCB), 
                     (gpointer)obj);
  
  if(obj->callbacks->paneResize &&
     obj->callbacks->user_data)
    (*(obj->callbacks->paneResize))(&open, obj->callbacks->user_data);

  return ;
}

void zmapWindowRulerCanvasMaximise(ZMapWindowRulerCanvas obj, double y1, double y2)
{
  double x2, max_x2,
    ix1 = 0.0,
    iy1 = y1, 
    iy2 = y2;

  if(obj->scaleParent)
    { 
      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(obj->scaleParent),
                                 NULL, NULL, &max_x2, NULL);
      if(iy1 == iy2 && iy1 == ix1)
        foo_canvas_get_scroll_region(FOO_CANVAS(obj->canvas),
                                     NULL, &iy1, &x2, &iy2);
      else
        foo_canvas_get_scroll_region(FOO_CANVAS(obj->canvas),
                                     NULL, NULL, &x2, NULL);
#ifdef VERBOSE_1        
      printf("rulerCanvasMaximise: %f %f - %f %f %f %f\n", x2, max_x2, ix1, x2, iy1, iy2);
#endif /* VERBOSE_1 */
      foo_canvas_set_scroll_region(FOO_CANVAS(obj->canvas),
                                   ix1, iy1, max_x2, iy2);
      
      if(max_x2 != 0.0 && 
         obj->callbacks->paneResize &&
         obj->callbacks->user_data)
        {
          int floored = (int)max_x2;
          freeze_notify(obj);
          (*(obj->callbacks->paneResize))(&floored, obj->callbacks->user_data);
          thaw_notify(obj);
        }
    }

  return ;
}


/* I don't like the dependence on window here! */
void zmapWindowRulerCanvasDraw(ZMapWindowRulerCanvas obj, double start, double end, gboolean force)
{
  double at_least = 0.0;
  zMapAssert(obj && obj->canvas);

  if(force && obj->scaleParent)
    gtk_object_destroy(GTK_OBJECT(obj->scaleParent)); /* Remove the current one */

  if((force && obj->line_height >= at_least) || 
     (!force && !obj->scaleParent && obj->line_height >= at_least))
    {
#ifdef VERBOSE_2
      printf("Draw: drawing call\n");
#endif /* VERBOSE_2 */
      obj->scaleParent = rulerCanvasDrawScale(obj->canvas, 
                                              obj->font_desc,
                                              start,
                                              end,
                                              obj->line_height,
                                              obj->text_left);
      
      /* We either need to do this check or 
       * double test = 0.0;
       * foo_canvas_item_get_bounds((FOO_CANVAS_ITEM(obj->scaleParent)), NULL, NULL, &test, NULL);
       * if(test == 0.0)
       *   g_signal_connect(...);
       */
      if(obj->visibilityHandlerCB == 0 &&
         !((FOO_CANVAS_ITEM(obj->scaleParent))->object.flags & (FOO_CANVAS_ITEM_REALIZED | FOO_CANVAS_ITEM_MAPPED)))
        obj->visibilityHandlerCB = 
          g_signal_connect(G_OBJECT(obj->canvas),
                           "visibility-notify-event", 
                           G_CALLBACK(rulerVisibilityHandlerCB), 
                           (gpointer)obj);
    }
  else if( obj->line_height < at_least)
    printf("Draw: line_height '%f' too small!\n", obj->line_height);
  else
    printf("Draw: a useless call\n");

  return ;
}

void zmapWindowRulerCanvasZoom(ZMapWindowRulerCanvas obj, double x, double y)
{

  zmapWindowRulerCanvasSetPixelsPerUnit(obj, x, y);

  return ;
}

void zmapWindowRulerCanvasSetVAdjustment(ZMapWindowRulerCanvas obj, GtkAdjustment *vadjustment)
{

  gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(obj->scrolled_window), vadjustment) ;

  return ;
}

void zmapWindowRulerCanvasSetPixelsPerUnit(ZMapWindowRulerCanvas obj, double x, double y)
{
  zMapAssert(obj && obj->canvas);

  foo_canvas_set_pixels_per_unit_xy(obj->canvas, x, y) ;

  return ;
}

void zmapWindowRulerCanvasSetLineHeight(ZMapWindowRulerCanvas obj,
                                        double line_height)
{
  zMapAssert(obj);
#ifdef VERBOSE_3
  printf("setLineHeight: setting line_height = %f\n", line_height);
#endif
  obj->line_height = line_height;

  return ;
}



/* INTERNALS */
static void paneNotifyPositionCB(GObject *pane, GParamSpec *scroll, gpointer user_data)
{
  ZMapWindowRulerCanvas obj = (ZMapWindowRulerCanvas)user_data;
  FooCanvasItem *scale = NULL;
  gboolean cancel = FALSE;

#ifdef VERBOSE_1
  printf("notifyPos: enter notify for %s.\n", g_param_spec_get_name(scroll));
#endif /* VERBOSE_1 */

  if(obj->scaleParent && (scale = FOO_CANVAS_ITEM(obj->scaleParent)) != NULL)
    cancel = TRUE;

  /* Reduce the processing this needs to do if we're frozen.  I can't
   * get g_object_freeze_notify(G_OBJECT(pane)) to work... 
   */
  if(cancel)
    cancel = !obj->freeze;

  if(cancel)
    {
      int position, 
        max = obj->default_position;
      double x2 = 100.0;

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
              obj->callbacks->paneResize && 
              obj->callbacks->user_data)
        (*(obj->callbacks->paneResize))(&max, obj->callbacks->user_data);
      else if(position == 0 && obj->visibilityHandlerCB == 0)
        obj->visibilityHandlerCB = 
          g_signal_connect(G_OBJECT(obj->canvas),
                           "visibility-notify-event", 
                           G_CALLBACK(rulerVisibilityHandlerCB), 
                           (gpointer)obj);
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
  ZMapWindowRulerCanvas obj = (ZMapWindowRulerCanvas)user_data;
  gboolean handled  = FALSE; 

#ifdef VERBOSE_1
  printf("rulerVisibilityHandlerCB: enter\n");
#endif /* VERBOSE_1 */
  if(obj->scaleParent)
    {
      GList *list = NULL;
      FooCanvasGroup *left = NULL, *right = NULL;
#ifdef VERBOSE_1
      printf("rulerVisibilityHandlerCB: got a scale bar\n");
#endif /* VERBOSE_1 */
      list = FOO_CANVAS_GROUP(obj->scaleParent)->item_list;
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
          g_signal_handler_disconnect(G_OBJECT(widget), obj->visibilityHandlerCB);
          obj->visibilityHandlerCB = 0; /* Make sure we reset this! */
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
  ZMapWindowRulerCanvas obj = (ZMapWindowRulerCanvas)user_data;
  gboolean handled = FALSE;

  handled = rulerVisibilityHandlerCB(widget, expose, user_data);

  zmapWindowRulerCanvasMaximise(obj, 0.0, 0.0);

  return handled;
}


static void freeze_notify(ZMapWindowRulerCanvas obj)
{
  obj->freeze = TRUE;
  return ;
}
static void thaw_notify(ZMapWindowRulerCanvas obj)
{
  obj->freeze = FALSE;
  return ;
}


/* =================================================== */
static FooCanvasItem *rulerCanvasDrawScale(FooCanvas *canvas,
                                           PangoFontDescription *font,
                                           double start, 
                                           double end,
                                           double height,
                                           gboolean text_left)
{
  FooCanvasItem *group = NULL ;
  ZMapScaleBarStruct scaleBar = { 0 };
  double zoom_factor = 0.0;

  group = foo_canvas_item_new(foo_canvas_root(canvas),
			      foo_canvas_group_get_type(),
			      "x", 0.0,
			      "y", 0.0,
			      NULL) ;

  zoom_factor = FOO_CANVAS(canvas)->pixels_per_unit_y;
  height      = height / zoom_factor;

  if(initialiseScale(&scaleBar, start, end, zoom_factor, height))
    drawScaleBar(&scaleBar, FOO_CANVAS_GROUP(group), font, text_left);

  if(scaleBar.unit)
    g_free(scaleBar.unit);

  return group ;
}

static gboolean initialiseScale(ZMapScaleBar obj_out,
                                double start, 
                                double end, 
                                double zoom, 
                                double line)
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

  obj_out->start  = (int)start;
  obj_out->top    = start;
  obj_out->end    = (int)end;
  obj_out->bottom = end;
  obj_out->force_multiples_of_five = ZMAP_FORCE_FIVES;
  obj_out->zoom_factor = zoom;

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
  diff          = obj_out->end - obj_out->start + 1;
  basesPerPixel = diff / (diff * obj_out->zoom_factor);

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
  obj_out->base = majorSize;

  tmp = ceil((basesBetween / majorSize));

  /* This isn't very elegant, and is kind of a reverse of the previous
   * logic used */
  if(obj_out->force_multiples_of_five == TRUE)
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

  obj_out->base  = majorUnits[unitIndex];
  obj_out->major = majorSize;
  obj_out->unit  = g_strdup( majorAlpha[unitIndex] );

  if(obj_out->major >= ZMAP_SCALE_MINORS_PER_MAJOR)
    obj_out->minor = majorSize / ZMAP_SCALE_MINORS_PER_MAJOR;
  else
    obj_out->minor = 1;

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

  /* < rather than <= to avoid final minor as end is seq_end + 1 */
  for( ; i < scaleBar->end; i+=scaleBar->minor, n++)
    {
      /* More conditionals than I intended here... */
      char *digitUnit = NULL;
      double i_d = (double)i ;				    /* Save a lot of casting... */


      if (n % ZMAP_SCALE_MINORS_PER_MAJOR) /* Minors */
        {
          if (i < scaleBar->start)
            zMapDrawLine(lines, 
                         scale_min, (double)scaleBar->start, 
                         scale_line, (double)scaleBar->start, 
                         &black, 1.0) ;
          else if (i == scaleBar->start && n < 5)
            {                   /* n < 5 to stop overlap of digitUnit at some zooms */
              digitUnit = g_strdup_printf("%d", scaleBar->start);
              zMapDrawLine(lines, scale_min, i_d, scale_line, i_d, &black, 1.0) ;
            }
          else
            zMapDrawLine(lines, scale_min, i_d, scale_line, i_d, &black, 1.0) ;
        }
      else                      /* Majors */
        {
          if(i && i >= scaleBar->start)
            {
              zMapDrawLine(lines, scale_maj, i_d, scale_line, i_d, &black, 1.0);
              digitUnit = g_strdup_printf("%d%s", 
                                          (i / scaleBar->base), 
                                          scaleBar->unit);
            }
          else
            {
              zMapDrawLine(lines, 
                           scale_min, (double)scaleBar->start, 
                           scale_line, (double)scaleBar->start, 
                           &black, 1.0);
              digitUnit = g_strdup_printf("%d", scaleBar->start);
            }
        }

      /* =========================================================== */
      if(digitUnit)
        {
          FooCanvasItem *item = NULL;
          double x = 0.0;

          if(text_left)
            {
              width = strlen(digitUnit);
              x = ((scale_maj) - (5.0 * (double)width));
            }

          item = foo_canvas_item_new(text,
                                     foo_canvas_text_get_type(),
                                     "x",          x,
                                     "y",          (i_d < (double)scaleBar->start ? (double)scaleBar->start : i_d),
                                     "text",       digitUnit,
                                     "font_desc",  font,
                                     "fill_color", "black",
                                     "anchor",     (text_left ? GTK_ANCHOR_CENTER : GTK_ANCHOR_W ),
                                     NULL);
          g_free(digitUnit);
        }

    }


  /* draw the vertical line of the scalebar. */
    /* allocate a new points array */
  points = foo_canvas_points_new(2) ;
				                                            
  /* fill out the points */
  points->coords[0] = scale_line ;
  points->coords[1] = scaleBar->top ;
  points->coords[2] = scale_line ;
  points->coords[3] = scaleBar->bottom ;

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




