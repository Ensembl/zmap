/*  File: zmapWindowRuler.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implements the scale bar shown for sequences.
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <zmapWindow_P.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsFoo.h>
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

#if 0
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
#endif

typedef enum
  {
    ZMAP_SCALE_BAR_GROUP_NONE,
    ZMAP_SCALE_BAR_GROUP_LEFT,
    ZMAP_SCALE_BAR_GROUP_RIGHT,
  } ZMapScaleBarGroupType;

#if 0
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
#endif

void zMapWindowDrawScaleBar(FooCanvasGroup *group, double scroll_start, double scroll_end,
	int seq_start, int seq_end, double zoom_factor, gboolean revcomped, double projection_factor);


static FooCanvasItem *rulerCanvasDrawScale(ZMapWindowRulerCanvas ruler,
                                           double start,
                                           double end,
                                          int seq_start, int seq_end);

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
/* MH17 NOTE this is called from zmapWindow.c */
/* MH17 NOTE start and end are the foo canvas scroll region */
gboolean zmapWindowRulerCanvasDraw(ZMapWindowRulerCanvas ruler, double start, double end,int seq_start,int seq_end, gboolean force)
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

      ruler->scaleParent = rulerCanvasDrawScale(ruler, start, end, seq_start, seq_end);

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

/* MH17 NOTE this is called from zmapWindowNavigator.c */
void zmapWindowRulerGroupDraw(FooCanvasGroup *parent, double project_at, gboolean revcomped, double start, double end)
{
  FooCanvas *canvas = NULL;
  double zoom_factor = 0.0;

  canvas = FOO_CANVAS(FOO_CANVAS_ITEM(parent)->canvas);

  zoom_factor = FOO_CANVAS(canvas)->pixels_per_unit_y;

  zMapWindowDrawScaleBar(parent, start, end, (int) start,(int) end, zoom_factor, revcomped, project_at);

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
                                           double end,
                                           int seq_start, int seq_end)
{
  FooCanvasItem *group = NULL ;
//  ZMapScaleBarStruct scaleBar = { 0 };
  double zoom_factor = 0.0;
  double height;

  group = foo_canvas_item_new(foo_canvas_root(ruler->canvas),
			      foo_canvas_group_get_type(),
			      "x", 0.0,
			      "y", 0.0,
			      NULL) ;

  zoom_factor = FOO_CANVAS(ruler->canvas)->pixels_per_unit_y;
  height      = ruler->line_height / zoom_factor;

	zMapWindowDrawScaleBar(FOO_CANVAS_GROUP(group), start, end, seq_start, seq_end,
		zoom_factor, ruler->revcomped, 1.0);

  return group ;
}


/* a rewrite */
/* we need :
start and end seq coord
foo group to draw into
window height in pixels
start and end window coords?
text height (font)
*/
/* we draw the scale on the right and text to the left */
void zMapWindowDrawScaleBar(FooCanvasGroup *group, double scroll_start, double scroll_end, int seq_start, int seq_end, double zoom_factor, gboolean revcomped, double projection_factor)
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
{
	int seq_len;	/* # bases in the scroll region */
	int tick;
	int tick_coord;
	int top, level;	/* we go many levels deep */
	int n_pixels;	/* cannot be more than 30k due to foo */
	int gap;		/* pixels between ticks */
	int digit;		/* which tick out of 10 */
	int digits;		/* how many digits in fractional part */
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
	FooCanvasGroup *lines = 0, *text = 0;
	char *units[] = { "","","k","M","G" };
	int i;
	int scr_start;
	int scr_end;

	if(revcomped)
	{
		/* scroll coords are seq coords reflected in seq_end, we need these as -1 based -ve
		 * typically as we never know seq_end these appear to be 1-based
		 * but they are not eg  if we select a subsequence or zoom in
		 * this is a complication introduced by applying revcomp to the model (feature context)
		 * rather than the view (zmapWindow)
		 * here scr_start,end are being set to the scroll limits of the coords we want to display
		 */

		/* get fwd strand coords by reflecting in seq_end then
		 * make these negative
		 * then add seq_start to make them -1 based
		 * they are already swapped (end <-> start)
		 */
		scr_start = -(seq_end - ((int) scroll_start) + 1) + seq_start - 2;
		scr_end = -(seq_end - ((int) scroll_end) + 1) + seq_start - 2;
//printf("revcomp: seq = %d,%d scroll = %f,%f, scr = %d,%d\n", seq_start,seq_end,scroll_start,scroll_end,scr_start,scr_end);
	}
	else
	{
		/* get 1-based coordinates for our subsequence */
		scr_start = ((int) scroll_start) - seq_start + 1;
		scr_end = ((int) scroll_end) - seq_start + 1;
//printf("forward: seq = %d,%d scroll = %f,%f, scr = %d,%d\n",seq_start,seq_end,scroll_start,scroll_end,scr_start,scr_end);
	}
	seq_len = scr_end - scr_start + 1;

	if(seq_len < 10)	/* we get called on start up w/ no sequence */
		return;

	lines = FOO_CANVAS_GROUP(foo_canvas_item_new(group,
            foo_canvas_group_get_type(),
            NULL));
  	text  = FOO_CANVAS_GROUP(foo_canvas_item_new(group,
            foo_canvas_group_get_type(),
            NULL));

	gdk_color_parse("black", &black) ;
	gdk_color_parse("grey", &grey) ;

	zMapFoocanvasGetTextDimensions(((FooCanvasItem *) group)->canvas, &font_desc, &font_width, &font_height);
	text_height = (int) (font_height);

	if(!font_height)
	{
		font_height = 14.0;	/* don't just give up ! */
		font_width = 8.0;
		text_height = 14;
		zMapLogWarning("DrawScale get font size failed","");
	}

	/* get the highest order ticks: this need to use sequence not scroll region */
	for(tick = 1,n_levels = 1,i = seq_len; i > 9; i /= 10, tick *= 10,n_levels++)
		continue;

	/* get the number of pixels: this used the scroll region */

	n_pixels = (int) (seq_len * zoom_factor * projection_factor);

	for(n_hide = n_levels,gap = n_pixels * tick / seq_len;gap;gap /= 10,n_hide--)
		continue;

	/* do we assume there are enough pixels for the highest order ticks?
	 * no: if they shrink the window so that you can't read it then don't display
	 */

	gap = (int) (((double) n_pixels) * tick / seq_len );	/* (double) or else would get overflow on big sequences */

//printf("scale bar: %d bases (%d-%d) @(%d,%d) was @(%f,%f) tick = %d, first = %d,levels= %d,%d, %d pixels font %d, zoom %f\n",
//seq_len, seq_start, seq_end, scr_start,scr_end, scroll_start, scroll_end,
//tick,(scr_start / tick) * tick, n_levels,n_hide, n_pixels, text_height,zoom_factor);

	/* we need a lot of levels to cope with large sequences zoomed in a lot */
	/* to get down to single base level */
	for(level = n_levels,top = 1;level >= 0 && tick > 0;level--, tick /= 10, gap /= 10, top = 0)
	{
		if(!gap)
			break;

		if(top)
		{
			base = 1;
			digits = 1;
			for(i = 1;base <= tick && i < 5;i++,base *= 1000, digits += 2)
				continue;

			unit = units[--i];
			base /= 1000;
			digits -= 2;
		}
//printf("level %d gap = %d, tick = %d, base = %d, digits = %d\n",level,gap,tick,base,digits);


		for(tick_coord = (scr_start / tick) * tick;;tick_coord += tick)
		{
			if(tick_coord < scr_start)
				continue;
			if(tick_coord >= scr_end)
				break;


			digit =  (tick_coord / tick) % 10;
			if(top || digit)			/* don't draw ticks over higher order ticks */
			{
				nudge = (digit == 5 || digit == -5);
				if((nudge && gap > 1) || gap > 2)	/* don't crowd ticks together */
				{
					/* draw the tick, with a number if appropriate */

					int draw_at = tick_coord;

					if(revcomped)
					{
						/* tick coord is # bases off the end due to revcomp mixture */
						/* this is the scr_start calc from above reversed */
						/* but we draw 1 bp backwards */
						draw_at = tick_coord + seq_end - seq_start + 2 + 1;
					}
					else
					{
						draw_at = tick_coord + seq_start - 1;
					}

					label[0] = 0;
					canvas_coord = (double) draw_at;
//if(top) printf("coord: %d %d (%d,%d) = %d\n", tick_coord, digit, seq_start,seq_end,draw_at);

					canvas_coord *= projection_factor;

					tick_width = 4.0 * (level - n_hide);
					colour = &black;
					if(nudge)
					{
						tick_width += 3.0;
						if(gap < 3)
							colour = &grey;
					}
					else if(gap < 5)
					{
						colour = &grey;
					}

					zMapDrawLine(lines, scale_width - tick_width,  canvas_coord,
						scale_width, canvas_coord, colour, 1.0) ;

					if((gap > text_height * 2))
					{
	      				FooCanvasItem *item = NULL;
						double x = 0.0;
						int num,frac;
						char *sign = "";
						double offset = revcomped ? -0.5 : 0.5;

						num = tick_coord / base;
						if(num < 0)
							num = -num;
						frac = tick_coord % base;
						if(frac < 0)
							frac = -frac;
						if(tick_coord < 0)
							sign = "-";

						if(frac)
						{
							/* this ought to be easier but we get to cope with
							 * leading and trailing zeroes and -ve zeroes
							 * and then the % operator is a bit flaky w/ -ve numbers
							 */
							char *p;

							p = label + sprintf(label,"%s%d.%0*d",sign,num,digits,frac);
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

						item = foo_canvas_item_new(text,
							foo_canvas_text_get_type(),
							"x",          x,
							"y",          canvas_coord + offset,	/* display between ticks at high zoom */
							"text",       label,
							"font_desc",  font_desc,
							"fill_color", "black",
							// anchor centre means middle of text item is at coordinate?
							"anchor",     GTK_ANCHOR_W,
							NULL);
					}

//if(top) printf("tick @ %d %f (%d, %d) level %d: %s\n",tick_coord,canvas_coord,base,tick,level,label);
				}
			}
		}
	}

	/* one vertical line */
	zMapDrawLine(lines, scale_width, scroll_start * projection_factor,
		scale_width, scroll_end  * projection_factor, &black, 1.0) ;

//  if(text_left)
    {
      g_object_set_data(G_OBJECT(text),
                        ZMAP_SCALE_BAR_GROUP_TYPE_KEY,
                        GINT_TO_POINTER(ZMAP_SCALE_BAR_GROUP_LEFT));
      g_object_set_data(G_OBJECT(lines),
                        ZMAP_SCALE_BAR_GROUP_TYPE_KEY,
                        GINT_TO_POINTER(ZMAP_SCALE_BAR_GROUP_RIGHT));
      positionLeftRight(text, lines);
    }

}



