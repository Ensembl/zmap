/*  File: zmapWindowNav.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Apr 23 09:27 2009 (rds)
 * Created: Wed Sep  6 11:22:24 2006 (rds)
 * CVS info:   $Id: zmapWindowNavigator.c,v 1.50 2009-04-23 08:51:30 rds Exp $
 *-------------------------------------------------------------------
 */

#include <math.h>
#include <string.h>
#include <ZMap/zmapUtilsGUI.h>
#include <ZMap/zmapUtils.h>
#include <zmapWindowNavigator_P.h>
#ifdef RDS_WITH_STIPPLE
#include <ZMap/zmapNavigatorStippleG.xbm> /* bitmap... */
#endif

/* Return the widget! */
#define NAVIGATOR_WIDGET(navigate) GTK_WIDGET(fetchCanvas(navigate))

typedef struct
{
  /* inputs */
  ZMapWindowNavigator navigate;
  ZMapFeatureContext  context;
  GData              *styles;

  /* The current features in the recursion */
  ZMapFeatureAlignment current_align;
  ZMapFeatureBlock     current_block;
  ZMapFeatureSet       current_set;
  /* The current containers in the recursion */
  FooCanvasGroup     *container_block;
  FooCanvasGroup     *container_strand;
  FooCanvasGroup     *container_feature_set;
  double current;               

} NavigateDrawStruct, *NavigateDraw;

typedef struct
{
  double y1;
  double y2;
  FooCanvasItem *item;
}NavigatorLeadingStruct, *NavigatorLeading;

typedef struct
{
  double offset ;
} PositionColumnStruct, *PositionColumn ;

typedef struct
{
  double start, end;
  ZMapStrand strand;
  ZMapFeature feature;
} LocusEntryStruct, *LocusEntry;


/* We need this because the locator is drawn as a foo_canvas_rect with
 * a transparent background, that will not receive events! Therefore as
 * a work around we set up a handler on the root background and test 
 * whether we're within the bounds of the locator. */
typedef struct
{
  ZMapWindowNavigator navigate;
  gboolean locator_click;
  double   click_correction;
}TransparencyEventStruct, *TransparencyEvent;

typedef struct
{
  ZMapWindowNavigator navigate;
  FooCanvasItem *item;
  ZMapWindowTextPositioner positioner;
  double wheight;
}RepositionTextDataStruct, *RepositionTextData;

typedef struct
{
  ZMapWindowNavigator navigate;
  GdkColor highlight_colour;
  double x1, x2;
  double top;
  double bot;
}NavigatorLocatorStruct, *NavigatorLocator;

static void repositionText(ZMapWindowNavigator navigate);

/* draw some features... */
static ZMapFeatureContextExecuteStatus drawContext(GQuark key, 
                                                   gpointer data, 
                                                   gpointer user_data,
                                                   char **err_out);
static void createColumnCB(gpointer data, gpointer user_data);

static void clampCoords(ZMapWindowNavigator navigate, 
                        double pre_scale, double post_scale, 
                        double *c1_inout, double *c2_inout);
static void clampScaled(ZMapWindowNavigator navigate, 
                        double *s1_inout, double *s2_inout);
static void clampWorld2Scaled(ZMapWindowNavigator navigate, 
                              double *w1_inout, double *w2_inout);
static void setupLocatorGroup(ZMapWindowNavigator navigate);
static void updateLocatorDragger(ZMapWindowNavigator navigate, double button_y, double size);
static gboolean rootBGEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data);
static gboolean columnBackgroundEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data);
static void positioningCB(FooCanvasGroup *container, FooCanvasPoints *points, 
                          ZMapContainerLevelType level, gpointer user_data);

static FooCanvas *fetchCanvas(ZMapWindowNavigator navigate);

static gboolean initialiseScaleIfNotExists(ZMapFeatureBlock block);
static gboolean drawScaleRequired(NavigateDraw draw_data);
static void drawScale(NavigateDraw draw_data);
static void navigateDrawFunc(NavigateDraw nav_draw, GtkWidget *widget);
static void expose_handler_disconn_cb(gpointer user_data, GClosure *unused);
static gboolean nav_draw_expose_handler(GtkWidget *widget,
					GdkEventExpose *expose, 
					gpointer user_data);

static gboolean navCanvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data);
static gboolean navCanvasItemDestroyCB(FooCanvasItem *feature_item, gpointer data);

static void factoryItemHandler(FooCanvasItem            *new_item,
                               ZMapWindowItemFeatureType new_item_type,
                               ZMapFeature               full_feature,
                               ZMapWindowItemFeature     sub_feature,
                               double                    new_item_y1,
                               double                    new_item_y2,
                               gpointer                  handler_data);

static gboolean factoryFeatureSizeReq(ZMapFeature feature, 
                                      double *limits_array, 
                                      double *points_array_inout, 
                                      gpointer handler_data);

static void printCoordsInfo(ZMapWindowNavigator navigate, char *message, double start, double end);
static void customiseFactory(ZMapWindowNavigator navigator);

static GHashTable *zmapWindowNavigatorLDHCreate(void);
static LocusEntry zmapWindowNavigatorLDHFind(GHashTable *hash, GQuark key);
static LocusEntry zmapWindowNavigatorLDHInsert(GHashTable *hash, 
                                               ZMapFeature feature, 
                                               double start, double end);
static void zmapWindowNavigatorLDHDestroy(GHashTable **destroy);
static void destroyLocusEntry(gpointer data);

static void get_filter_list_up_to(GList **filter_out, int max);
static void available_locus_names_filter(GList **filter_out);
static void default_locus_names_filter(GList **filter_out);
static gint strcmp_list_find(gconstpointer list_data, gconstpointer user_data);

static void highlight_columns_cb(FooCanvasGroup *container, FooCanvasPoints *points, 
				 ZMapContainerLevelType level, gpointer user_data);
static void locator_highlight_column_areas(ZMapWindowNavigator navigate,
					   double x1, double x2,
					   double raw_top, double raw_bot);
static void real_focus_navigate(ZMapWindowNavigator navigate);
static gboolean nav_focus_expose_handler(GtkWidget *widget, GdkEventExpose *expose, gpointer user_data);
static void real_draw_locator(ZMapWindowNavigator navigate);
static gboolean nav_locator_expose_handler(GtkWidget *widget, GdkEventExpose *expose, gpointer user_data);

static ZMapFeatureTypeStyle getPredefinedStyleByName(char *style_name);

static void set_data_destroy(gpointer user_data);

/* ------------------- */
static gboolean locator_debug_G = FALSE;

/*
 * Mail from Jon:
 * If you eliminate MGC:, SK:, MIT:, GD:, WU: then I think the
 * problem will be largely solved. However, I myself would also
 * get rid of (i.e. not display) ENSEST... but keep
 * ENSG... since the former seems to often mask HAVANA objects.
 * Mail from Jane:
 * I'd like to see CCDS:
 */
/* Keep the enum and array in step please... */
enum
  {
    LOCUS_NAMES_MGC,
    LOCUS_NAMES_SK,
    LOCUS_NAMES_MIT,
    LOCUS_NAMES_GD,
    LOCUS_NAMES_WU,
    LOCUS_NAMES_INT,
    LOCUS_NAMES_KO,
    LOCUS_NAMES_ERI,
    LOCUS_NAMES_JGI,
    LOCUS_NAMES_OLD_MIT,
    LOCUS_NAMES_MPI,
    LOCUS_NAMES_RI,
    LOCUS_NAMES_GC,
    LOCUS_NAMES_BCM,
    LOCUS_NAMES_C22,

    /* Before this one the loci names are hidden by default. */
    LOCUS_NAMES_SEPARATOR,
    /* After they will be shown... */

    LOCUS_NAMES_ENSEST,
    LOCUS_NAMES_ENSG,
    LOCUS_NAMES_CCDS,

    LOCUS_NAMES_LENGTH
  };

static char *locus_names_filter_G[] = {
  "MGC:",			/* LOCUS_NAMES_MGC */
  "SK:",
  "MIT:",
  "GD:",
  "WU:",
  "INT:",
  "KO:",
  "ERI:",
  "JGI:",
  "OLD_MIT:",
  "MPI:",
  "RI:",
  "GC:",
  "BCM:",
  "C22:",
  "-empty-",			/* LOCUS_NAMES_SEPARATOR */
  "ENSEST",
  "ENSG",
  "CCDS:",
  "-empty-"			/* LOCUS_NAMES_LENGTH */
};

/* create */
ZMapWindowNavigator zMapWindowNavigatorCreate(GtkWidget *canvas_widget)
{
  ZMapWindowNavigator navigate = NULL;

  zMapAssert(FOO_IS_CANVAS(canvas_widget));

  if((navigate = g_new0(ZMapWindowNavigatorStruct, 1)))
    {
      FooCanvas *canvas = NULL;
      FooCanvasGroup *root = NULL;

      navigate->ftoi_hash = zmapWindowFToICreate();

      navigate->locus_display_hash = zmapWindowNavigatorLDHCreate();
      navigate->locus_id = g_quark_from_string("locus");

      if(USE_BACKGROUNDS)
        {
          gdk_color_parse(ROOT_BACKGROUND,   &(navigate->root_background));
          gdk_color_parse(ALIGN_BACKGROUND,  &(navigate->align_background));
          gdk_color_parse(BLOCK_BACKGROUND,  &(navigate->block_background));
          gdk_color_parse(STRAND_BACKGROUND, &(navigate->strand_background));
          gdk_color_parse(COLUMN_BACKGROUND, &(navigate->column_background));
        }
      else
        {
          GdkColor *canvas_color = NULL;
          int gdkcolor_size = 0;
          gdkcolor_size = sizeof(GdkColor);
          canvas_color  = &(gtk_widget_get_style(canvas_widget)->bg[GTK_STATE_NORMAL]);
          memcpy(&(navigate->root_background), 
                 canvas_color,
                 gdkcolor_size);
          memcpy(&(navigate->align_background), 
                 canvas_color,
                 gdkcolor_size);
          memcpy(&(navigate->block_background), 
                 canvas_color,
                 gdkcolor_size);
          memcpy(&(navigate->strand_background), 
                 canvas_color,
                 gdkcolor_size);
          memcpy(&(navigate->column_background), 
                 canvas_color,
                 gdkcolor_size);
        }

      gdk_color_parse(LOCATOR_BORDER,    &(navigate->locator_border_gdk));
      gdk_color_parse(LOCATOR_DRAG,      &(navigate->locator_drag_gdk));
      gdk_color_parse(LOCATOR_FILL,      &(navigate->locator_fill_gdk));
#ifdef RDS_WITH_STIPPLE
      navigate->locator_stipple = gdk_bitmap_create_from_data(NULL, &zmapNavigatorStippleG_bits[0],
                                                              zmapNavigatorStippleG_width,
                                                              zmapNavigatorStippleG_height);
#endif
      /* create the root container */
      canvas = FOO_CANVAS(canvas_widget);
      root   = FOO_CANVAS_GROUP(foo_canvas_root(canvas));
      navigate->container_root = zmapWindowContainerCreate(root, ZMAPCONTAINER_LEVEL_ROOT,
                                                           ROOT_CHILD_SPACING, 
                                                           &(navigate->root_background), NULL, NULL);
      /* add it to the hash. */
      zmapWindowFToIAddRoot(navigate->ftoi_hash, navigate->container_root);
      /* lower to bottom so that everything else works... */
      foo_canvas_item_lower_to_bottom(FOO_CANVAS_ITEM(navigate->container_root));

      navigate->scaling_factor = 0.0;
      navigate->locator_bwidth = LOCATOR_LINE_WIDTH;
      navigate->locator_x1     = 0.0;
      navigate->locator_x2     = LOCATOR_LINE_WIDTH * 10.0;

      customiseFactory(navigate);

      default_locus_names_filter(&(navigate->hide_filter));

      available_locus_names_filter(&(navigate->available_filters));
    }

  zMapAssert(navigate);

  return navigate;
}


void zMapWindowNavigatorSetStrand(ZMapWindowNavigator navigate, gboolean revcomped)
{
  navigate->is_reversed = revcomped;
  return ;
}

void zMapWindowNavigatorReset(ZMapWindowNavigator navigate)
{
  gulong expose_id = 0;

  if((expose_id = navigate->draw_expose_handler_id) != 0)
    {
      FooCanvas *canvas;

      canvas       = fetchCanvas(navigate);

      g_signal_handler_disconnect(G_OBJECT(canvas), expose_id);

      navigate->draw_expose_handler_id = 0;
    }

  if((expose_id = navigate->focus_expose_handler_id) != 0)
    {
      FooCanvas *canvas;

      canvas       = fetchCanvas(navigate);

      g_signal_handler_disconnect(G_OBJECT(canvas), expose_id);

      navigate->focus_expose_handler_id = 0;
    }
  
  if((expose_id = navigate->locator_expose_handler_id) != 0)
    {
      FooCanvas *canvas;

      canvas       = fetchCanvas(navigate);

      g_signal_handler_disconnect(G_OBJECT(canvas), expose_id);

      navigate->locator_expose_handler_id = 0;
    }

  zmapWindowContainerPurge(zmapWindowContainerGetFeatures( navigate->container_root ));

  /* Keep pointers in step and recreate what was destroyed */
  navigate->container_align = NULL;

  navigate->locator_group = NULL;
  navigate->locator_drag  =
    navigate->locator     = NULL;

  /* The hash contains invalid pointers so destroy and recreate. */
  zmapWindowFToIDestroy(navigate->ftoi_hash);
  navigate->ftoi_hash = zmapWindowFToICreate();

  zmapWindowNavigatorLDHDestroy(&(navigate->locus_display_hash));
  navigate->locus_display_hash = zmapWindowNavigatorLDHCreate();

  zmapWindowFToIFactoryClose(navigate->item_factory);
  customiseFactory(navigate);
  
  return ;
}

void zMapWindowNavigatorFocus(ZMapWindowNavigator navigate, 
                              gboolean raise_to_top,
                              double *x1_inout, double *y1_inout, 
                              double *x2_inout, double *y2_inout)
{
  FooCanvasItem *root;
  FooCanvasItem *root_bg;
  double x1, x2, y1, y2;

  root = FOO_CANVAS_ITEM(navigate->container_root);

  if(navigate->locator == NULL)
    {
      /* We don't want to continue... */
      raise_to_top = FALSE;
      x1_inout = x2_inout = y1_inout = y2_inout = NULL;
    }

  if(x1_inout && x2_inout && y1_inout && y2_inout)
    {
      FooCanvasItem *root_features;
      double y_min = 0.0, y_max = (double)NAVIGATOR_SIZE;

      foo_canvas_item_set(root, "x", 0.0, NULL);

      root_features = FOO_CANVAS_ITEM(zmapWindowContainerGetFeatures(navigate->container_root));

      foo_canvas_item_get_bounds(root_features, &x1, &y1, &x2, &y2);

      if(x1 < *x1_inout)
        *x1_inout = x1;
      if(y1 < *y1_inout && y1 >= 0.0)
        *y1_inout = y1;
      else
	*y1_inout = y_min;

      if(x2 > *x2_inout)
        *x2_inout = x2;
      if(y2 > *y2_inout && y2 <= y_max)
        *y2_inout = y2;
      else
	*y2_inout = y_max;
      
      root_bg = zmapWindowContainerGetBackground(navigate->container_root);

      foo_canvas_item_set(root, "x", -1000.0, NULL);
      
      foo_canvas_item_set(root_bg, 
                          "x1", *x1_inout, "x2", *x2_inout,
                          "y1", *y1_inout, "y2", *y2_inout,
                          NULL);
    }

  if(raise_to_top)
    {
      foo_canvas_item_set(root, "x", 0.0, NULL);

      if(navigate->locator_group)
	foo_canvas_item_set(FOO_CANVAS_ITEM(navigate->locator_group), "x", 0.0, NULL);

      foo_canvas_item_raise_to_top(root);

      if(!GTK_WIDGET_MAPPED(GTK_WIDGET(root->canvas)))
	{
	  gulong expose_id = 0; 

	  if((expose_id = navigate->focus_expose_handler_id) == 0)
	    {
	      expose_id = g_signal_connect_data(G_OBJECT(root->canvas), "expose_event",
						G_CALLBACK(nav_focus_expose_handler), (gpointer)navigate,
						NULL, G_CONNECT_AFTER) ;
	      
	      navigate->focus_expose_handler_id = expose_id;
	    }
	  /* else { let the current expose handler do it... } */
	}  
      else
	{
	  gulong expose_id;
	  
	  if((expose_id = navigate->focus_expose_handler_id) == 0)
	    {
	      real_focus_navigate(navigate);
	    }
	  /* else { let the current expose handler do it... } */
	}
    }

  return ;
}

void zMapWindowNavigatorSetCurrentWindow(ZMapWindowNavigator navigate, ZMapWindow window)
{
  zMapAssert(navigate && window);

  /* We should be updating the locator here too.  In the case of
   * unlocked windows the locator becomes out of step on focusing
   * until the visibility change handler is called */

  navigate->current_window = window;

  return ;
}

void zMapWindowNavigatorMergeInFeatureSetNames(ZMapWindowNavigator navigate, 
                                               GList *navigator_sets)
{
  /* This needs to do more that just concat!! ha, it'll break something down the line ... column ordering at least */
  navigator_sets = g_list_copy(navigator_sets);
  navigate->feature_set_names = g_list_concat(navigate->feature_set_names, navigator_sets);

  return ;
}

/* draw features */
void zMapWindowNavigatorDrawFeatures(ZMapWindowNavigator navigate, 
                                     ZMapFeatureContext full_context,
				     GData *styles)
{
  FooCanvas *canvas = NULL;
  NavigateDrawStruct draw_data = {NULL};

  draw_data.navigate  = navigate;
  draw_data.context   = full_context;
  draw_data.styles    = styles;

  navigate->full_span.x1 = full_context->sequence_to_parent.c1;
  navigate->full_span.x2 = full_context->sequence_to_parent.c2 + 1.0;

  navigate->scaling_factor = NAVIGATOR_SIZE / (navigate->full_span.x2 - navigate->full_span.x1 + 1.0);

  canvas = fetchCanvas(navigate);

  if(!GTK_WIDGET_MAPPED(GTK_WIDGET(canvas)))
    {
      gulong expose_id = 0;

      NavigateDraw draw_data_cpy = g_new0(NavigateDrawStruct, 1);

      memcpy(draw_data_cpy, &draw_data, sizeof(NavigateDrawStruct));

      if((expose_id = navigate->draw_expose_handler_id) == 0)
	{
	  expose_id = g_signal_connect_data(G_OBJECT(canvas), "expose_event",
					    G_CALLBACK(nav_draw_expose_handler), (gpointer)draw_data_cpy,
					    (GClosureNotify)(expose_handler_disconn_cb), 0) ;

	  navigate->draw_expose_handler_id = expose_id;
	}
      /* else { let the current expose handler do it... } */
    }
  else
    {
      gulong expose_id;

      if((expose_id = navigate->draw_expose_handler_id) == 0)
	{
	  navigateDrawFunc(&draw_data, GTK_WIDGET(canvas));
	}
      /* else { let the current expose handler do it... } */
    }

  return ;
}

void zmapWindowNavigatorLocusRedraw(ZMapWindowNavigator navigate)
{

  repositionText(navigate);

  return ;
}

/* draw locator */
void zMapWindowNavigatorDrawLocator(ZMapWindowNavigator navigate,
                                    double raw_top, double raw_bot)
{
  FooCanvas *canvas = NULL;

  zMapAssert(navigate);

  /* Always set these... */
  navigate->locator_span.x1 = raw_top;
  navigate->locator_span.x2 = raw_bot;

  canvas = fetchCanvas(navigate);

  if(!GTK_WIDGET_MAPPED(GTK_WIDGET(canvas)))
    {
      gulong expose_id = 0; 

      if((expose_id = navigate->locator_expose_handler_id) == 0)
	{
	  expose_id = g_signal_connect_data(G_OBJECT(canvas), "expose_event",
					    G_CALLBACK(nav_locator_expose_handler), (gpointer)navigate,
					    NULL, G_CONNECT_AFTER);
	  
	  navigate->locator_expose_handler_id = expose_id;
	}
      /* else { let the current expose handler do it... } */
    }
  else
    {
      gulong expose_id;

      if((expose_id = navigate->locator_expose_handler_id) == 0)
	{
	  real_draw_locator(navigate);
	}
      /* else { let the current expose handler do it... } */
    }

  return ;
}

void zmapWindowNavigatorPositioning(ZMapWindowNavigator navigate)
{
  /* comments origianlly in navigateDrawFunc.... */

  /* Some of the next code needs to be encoded in a function passed to
   * this call to container execute. */
  zmapWindowContainerExecuteFull(navigate->container_align, 
                                 ZMAPCONTAINER_LEVEL_FEATURESET,
                                 NULL, NULL, positioningCB, navigate, TRUE);
  /* ***************************** i.e. here ^^^^, ^^^^ */

  return ;
}

/* destroy */
void zMapWindowNavigatorDestroy(ZMapWindowNavigator navigate)
{

  gtk_object_destroy(GTK_OBJECT(navigate->container_root)); // remove our item.

  zmapWindowFToIDestroy(navigate->ftoi_hash);
  zmapWindowFToIFactoryClose(navigate->item_factory);
  zmapWindowNavigatorLDHDestroy(&(navigate->locus_display_hash));

  g_free(navigate);             /* possibly not enough ... */

  return ;
}


/* INTERNAL */

static void positioningCB(FooCanvasGroup *container, FooCanvasPoints *points, 
                          ZMapContainerLevelType level, gpointer user_data)
{
  ZMapWindowNavigator navigate = (ZMapWindowNavigator)user_data;
  double init_y1, init_y2, init_size;
  double rx1, rx2, width_x;

  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_ALIGN:
      {
        GtkWidget *widget = NULL;

        if(navigate->locator_span.x1 == navigate->locator_span.x2)
          {
            init_y1 = (double)(navigate->full_span.x1);
            init_y2 = (double)(navigate->full_span.x2);
          }
        else
          {
            init_y1 = (double)(navigate->locator_span.x1);  
            init_y2 = (double)(navigate->locator_span.x2);  
          }

        init_size = init_y2 - init_y1;
        width_x   = (double)(navigate->locator_bwidth);
        
        rx1 = points->coords[0];
        rx2 = points->coords[2];

	if(navigate->locator)
	  {
	    double ix1, ix2, dummy_y;
	    ix1 = rx1;
	    ix2 = rx2;
	    dummy_y = 0.0;

	    foo_canvas_item_w2i(navigate->locator, &ix1, &dummy_y);
	    foo_canvas_item_w2i(navigate->locator, &ix2, &dummy_y);

	    if((ix1 + width_x) < (ix2 - width_x))
	      {
		navigate->locator_x1 = ix1 + width_x;
		navigate->locator_x2 = ix2 - width_x;
	      }
	    else
	      {
		navigate->locator_x1 = ix1 - 0.5;
		navigate->locator_x2 = ix1 + 0.5;
	      }

	    zMapWindowNavigatorDrawLocator(navigate, init_y1, init_y2);
	    
	    widget = NAVIGATOR_WIDGET(navigate);
	    
	    zmapWindowNavigatorSizeRequest(widget, rx2 - rx1 + 1.0, init_size);
	    
	    zmapWindowNavigatorFillWidget(widget);
	  }
      }
      break;
    case ZMAPCONTAINER_LEVEL_ROOT:
    case ZMAPCONTAINER_LEVEL_BLOCK:
    case ZMAPCONTAINER_LEVEL_FEATURESET:
    case ZMAPCONTAINER_LEVEL_STRAND:
    default:
      break;
    }

  return ;
}

static FooCanvas *fetchCanvas(ZMapWindowNavigator navigate)
{
  FooCanvas *canvas = NULL;
  FooCanvasGroup *g = NULL;

  g = navigate->container_root;

  zMapAssert(g);

  canvas = FOO_CANVAS(FOO_CANVAS_ITEM(g)->canvas);

  return canvas;
}

static void expose_handler_disconn_cb(gpointer user_data, GClosure *unused)
{
  g_free(user_data);
  return ;
}

static gboolean nav_draw_expose_handler(GtkWidget *widget, GdkEventExpose *expose, gpointer user_data)
{
  NavigateDraw draw_data = (NavigateDraw)user_data;
  gulong expose_id;

  expose_id = draw_data->navigate->draw_expose_handler_id;
  
  if(draw_data->navigate->current_window)
    {
      g_signal_handler_block(G_OBJECT(widget), expose_id);

      g_signal_handler_disconnect(G_OBJECT(widget), expose_id);

      /* zero this before any other code. */
      draw_data->navigate->draw_expose_handler_id = 0;
      /* navigateDrawFunc can end up in nav_focus|locator_expose_handler */
      navigateDrawFunc(draw_data, widget);
    }

  return FALSE;                 /* lets others run. */
}

static void locus_gh_func(gpointer hash_key, gpointer hash_value, gpointer user_data)
{
  RepositionTextData data = (RepositionTextData)user_data;
  ZMapFeature feature = NULL;
  FooCanvasItem *item = NULL;
  LocusEntry locus_data = (LocusEntry)hash_value;
  double text_height, start, end, mid, draw_here, dummy_x = 0.0;
  double iy1, iy2, wy1, wy2;
  int cx = 0, cy1, cy2;

  feature = locus_data->feature;
  start   = locus_data->start;
  end     = locus_data->end;

  if((item = zmapWindowFToIFindFeatureItem(data->navigate->ftoi_hash,
                                           locus_data->strand, ZMAPFRAME_NONE, 
                                           feature)))
    {
      FooCanvasItem *line_item;
      GList *hide_list;
      double x1, x2, y1, y2;

      text_height = data->wheight;
      mid         = start + ((end - start + 1.0) / 2.0);
      draw_here   = mid - (text_height / 2.0);
      
      iy1 = wy1 = start;
      iy2 = wy2 = start + text_height;
      
      foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2);
      
      /* move to the start of the locus... */
      foo_canvas_item_move(item, 0.0, start - y1);
      
      foo_canvas_item_get_bounds(item, &x1, &(iy1), &x2, &(iy2));
      
      wy1 = iy1;
      wy2 = iy2;
      
      foo_canvas_item_i2w(item, &dummy_x, &(wy1));
      foo_canvas_item_i2w(item, &dummy_x, &(wy2));
      
      foo_canvas_w2c(item->canvas, dummy_x, wy1, &cx, &(cy1));
      foo_canvas_w2c(item->canvas, dummy_x, wy2, &cx, &(cy2));

      /* This is where we hide/show text matching the filter */
      foo_canvas_item_show(item); /* _always_ show the item */

      /* If this a redraw, there may be aline item.  If there
       * is. Destroy it.  It'll get recreated... */

      if((line_item = g_object_get_data(G_OBJECT(item), ZMAPWINDOWTEXT_ITEM_TO_LINE)))
	{
	  gboolean move_back_to_zero = TRUE;

	  gtk_object_destroy(GTK_OBJECT(line_item));
	  line_item = NULL;
	  g_object_set_data(G_OBJECT(item), ZMAPWINDOWTEXT_ITEM_TO_LINE, line_item);

	  if(move_back_to_zero)
	    {
	      double x1, x2, y1, y2;
	      foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2);
	      x1 = 0 - x1;
	      foo_canvas_item_move(item, x1, 0.0);
	    }
	}

      if((hide_list = data->navigate->hide_filter))
	{
	  GList *match;
	  char *text = NULL;
	  
	  g_object_get(G_OBJECT(item),
		       "text", &text,
		       NULL);

	  if(text && (match = g_list_find_custom(hide_list, text, strcmp_list_find)))
	    {
	      foo_canvas_item_hide(item);
	    }
	  else
	    zmapWindowTextPositionerAddItem(data->positioner, item);

	  if(text)
	    g_free(text);
	}
      else
	zmapWindowTextPositionerAddItem(data->positioner, item);
    }
  
  return ;
}

static void repositionText(ZMapWindowNavigator navigate)
{
  RepositionTextDataStruct repos_data = {NULL};

  if(navigate->locus_display_hash)
    {
      FooCanvas *canvas = NULL;

      canvas = fetchCanvas(navigate);

      repos_data.navigate = navigate;
      repos_data.positioner = zmapWindowTextPositionerCreate(navigate->full_span.x1 * navigate->scaling_factor, 
                                                             navigate->full_span.x2 * navigate->scaling_factor);

      zmapWindowNavigatorTextSize(GTK_WIDGET(canvas), 
                                  NULL, &(repos_data.wheight));

      g_hash_table_foreach(navigate->locus_display_hash,
                           locus_gh_func,
                           &repos_data);

      zmapWindowTextPositionerUnOverlap(repos_data.positioner, TRUE);
      zmapWindowTextPositionerDestroy(repos_data.positioner);
    }
  
  return ;
}

static void navigateDrawFunc(NavigateDraw nav_draw, GtkWidget *widget)
{
  ZMapWindowNavigator navigate = nav_draw->navigate;

  /* We need this! */
  zMapAssert(navigate->current_window);

  setupLocatorGroup(navigate);

  /* Everything to get a context drawn, raised to top and visible. */
  zMapFeatureContextExecuteComplete((ZMapFeatureAny)(nav_draw->context), 
                                    ZMAPFEATURE_STRUCT_FEATURE, 
                                    drawContext, 
                                    NULL, nav_draw);

  repositionText(navigate);
  
  zmapWindowNavigatorPositioning(navigate);
  
  zmapWindowNavigatorFillWidget(widget);

  return ;
}


static ZMapFeatureContextExecuteStatus drawContext(GQuark key_id, 
                                                   gpointer data, 
                                                   gpointer user_data,
                                                   char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapFeatureBlock     feature_block = NULL;
  ZMapFeatureSet       feature_set   = NULL;
  ZMapFeatureStructType feature_type = ZMAPFEATURE_STRUCT_INVALID;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  NavigateDraw draw_data = (NavigateDraw)user_data;
  ZMapWindowNavigator navigate = NULL;
  gboolean hash_status = FALSE;

  feature_type = feature_any->struct_type;

  navigate = draw_data->navigate;

  switch(feature_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
        FooCanvasGroup *container = NULL;

        draw_data->current_align = (ZMapFeatureAlignment)feature_any;
        container = zmapWindowContainerGetFeatures(navigate->container_root);
        if(!navigate->container_align)
          {
            navigate->container_align = zmapWindowContainerCreate(container, ZMAPCONTAINER_LEVEL_ALIGN,
                                                                  ALIGN_CHILD_SPACING, 
                                                                  &(navigate->align_background), NULL, NULL);
	    g_object_set_data(G_OBJECT(navigate->container_align), ITEM_FEATURE_STATS, 
			      zmapWindowStatsCreate((ZMapFeatureAny)draw_data->current_align)) ;
            hash_status = zmapWindowFToIAddAlign(navigate->ftoi_hash, key_id, navigate->container_align);
            zMapAssert(hash_status);
          }
      }
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
        FooCanvasGroup *features = NULL;
        double block_start, block_end;

        draw_data->current_block = feature_block = (ZMapFeatureBlock)feature_any;

        block_start = feature_block->block_to_sequence.q1;
        block_end   = feature_block->block_to_sequence.q2;

        //draw_data->navigate->scaling_factor = NAVIGATOR_SIZE / (block_end - block_start + 1.0);
        
        /* create the block and add the item to the hash */
        features    = zmapWindowContainerGetFeatures(draw_data->navigate->container_align);
        draw_data->container_block = zmapWindowContainerCreate(features, ZMAPCONTAINER_LEVEL_BLOCK,
                                                               BLOCK_CHILD_SPACING, 
                                                               &(navigate->block_background), NULL, NULL);
	g_object_set_data(G_OBJECT(draw_data->container_block), ITEM_FEATURE_STATS, 
			  zmapWindowStatsCreate((ZMapFeatureAny)draw_data->current_block)) ;
        hash_status = zmapWindowFToIAddBlock(navigate->ftoi_hash, draw_data->current_align->unique_id,
                                             key_id, draw_data->container_block);
        zMapAssert(hash_status);

        /* we're only displaying one strand... create it ... */
        features = zmapWindowContainerGetFeatures(draw_data->container_block);
        /* The strand container doesn't get added to the hash! */
        draw_data->container_strand = zmapWindowContainerCreate(features, ZMAPCONTAINER_LEVEL_STRAND,
                                                                STRAND_CHILD_SPACING, 
                                                                &(navigate->strand_background), NULL, NULL);

        /* create a column per set ... */
        initialiseScaleIfNotExists(draw_data->current_block);
        g_list_foreach(draw_data->navigate->feature_set_names, createColumnCB, (gpointer)draw_data);
        if(drawScaleRequired(draw_data))
          drawScale(draw_data);
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        FooCanvasItem *item = NULL;
        feature_set = (ZMapFeatureSet)feature_any;

        draw_data->current_set = feature_set;

        status = ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND;

        if((item = zmapWindowFToIFindSetItem(draw_data->navigate->ftoi_hash, feature_set, 
                                             ZMAPSTRAND_NONE, ZMAPFRAME_NONE)))
          {
            FooCanvasGroup *container_feature_set = NULL;
	    ZMapFeatureTypeStyle navigator_version, context_copy, context_version;
	    ZMapStyleOverlapMode overlap_mode;

	    context_version = zmapWindowContainerGetStyle(FOO_CANVAS_GROUP(item));

	    if((navigator_version = getPredefinedStyleByName(zMapStyleGetName(context_version))))
	      {
		context_copy = zMapFeatureStyleCopy(navigator_version);
		zMapStyleMerge(context_version, navigator_version);
	      }

            container_feature_set = FOO_CANVAS_GROUP(item);
            zmapWindowFToIFactoryRunSet(draw_data->navigate->item_factory, 
                                        feature_set, 
                                        container_feature_set, ZMAPFRAME_NONE);

	    if ((overlap_mode = zMapStyleGetOverlapMode(context_version)) != ZMAPOVERLAP_COMPLETE)
	      {
		zmapWindowContainerSortFeatures(container_feature_set, ZMAPCONTAINER_VERTICAL);

		zmapWindowColumnBumpRange(item, overlap_mode, ZMAPWINDOW_COMPRESS_ALL) ;
	      }

	    if(navigator_version)
	      {
		zMapStyleMerge(context_version, context_copy);
		zMapFeatureTypeDestroy(context_copy);
	      }
          }
        else
          {
            /* do not draw, the vast majority, of the feature context. */
          }
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      /* FeatureSet draws it all ;) */
      break;
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      status = ZMAP_CONTEXT_EXEC_STATUS_ERROR;
      zMapAssertNotReached();
      break;
    }

  return status;
}

static gboolean initialiseScaleIfNotExists(ZMapFeatureBlock block)
{
  ZMapFeatureSet scale;
  char *scale_id = ZMAP_FIXED_STYLE_SCALE_NAME;
  gboolean got_initialised = FALSE;

  if(!(scale = zMapFeatureBlockGetSetByID(block, g_quark_from_string(scale_id))))
    {
      scale = zMapFeatureSetCreate(scale_id, NULL);

      zMapFeatureBlockAddFeatureSet(block, scale);

      got_initialised = TRUE;
    }

  return got_initialised;
}

static gboolean drawScaleRequired(NavigateDraw draw_data)
{
  FooCanvasItem *item;
  gboolean required = FALSE;
  GQuark scale_id;

  scale_id = g_quark_from_string(ZMAP_FIXED_STYLE_SCALE_NAME);

  if((item = zmapWindowFToIFindItemFull(draw_data->navigate->ftoi_hash,
                                        draw_data->current_align->unique_id,
                                        draw_data->current_block->unique_id,
                                        scale_id, ZMAPSTRAND_NONE, ZMAPFRAME_NONE, 0)))
    {
      /* if block size changes then the scale will start breaking... */
      required = !(zmapWindowContainerHasFeatures(FOO_CANVAS_GROUP(item)));
    }

  return required;
}

static void drawScale(NavigateDraw draw_data)
{
  FooCanvasGroup *features    = NULL;
  FooCanvasItem *item = NULL;
  
  GQuark scale_id = 0;
  int min, max, origin;

  /* HACK...  */
  scale_id = g_quark_from_string(ZMAP_FIXED_STYLE_SCALE_NAME);
  /* less of a hack ... */
  if((item = zmapWindowFToIFindItemFull(draw_data->navigate->ftoi_hash,
                                        draw_data->current_align->unique_id,
                                        draw_data->current_block->unique_id,
                                        scale_id, ZMAPSTRAND_NONE, ZMAPFRAME_NONE, 0)))
    {
      FooCanvasGroup *scale_group = NULL;
      
      scale_group = FOO_CANVAS_GROUP(item);

      features = zmapWindowContainerGetFeatures(scale_group);

      min = draw_data->context->sequence_to_parent.c1;
      max = draw_data->context->sequence_to_parent.c2;

      /* Not sure this is the correct logic, but there seems to be
       * something wrong with the is_reversed flag, either never set
       * correctly, or just semantics */
      if(draw_data->navigate->is_reversed)
        origin = max + 2;
      else
        origin = min;

      zmapWindowRulerGroupDraw(features, draw_data->navigate->scaling_factor,
                               (double)origin, (double)min, (double)max);
    }

  return ;
}

static void set_data_destroy(gpointer user_data)
{
  ZMapWindowItemFeatureSetData set_data = (ZMapWindowItemFeatureSetData)user_data;

  zmapWindowItemFeatureSetDestroy(set_data);

  return ;
}

/* data is a GQuark, user_data is a NavigateDraw */
static void createColumnCB(gpointer data, gpointer user_data)
{
  GQuark set_id = GPOINTER_TO_UINT(data);
  NavigateDraw draw_data = (NavigateDraw)user_data;
  ZMapWindowItemFeatureSetData set_data = NULL;
  FooCanvasGroup *features = NULL;
  FooCanvasItem  *container_background = NULL;
  ZMapFeatureTypeStyle style = NULL;
  gboolean status = FALSE;

  /* assuming set name == style name !!! */
  if((style = zMapFindStyle(draw_data->styles, set_id)) &&
     (draw_data->current_set = zMapFeatureBlockGetSetByID(draw_data->current_block, set_id)))
    {
      GList *style_list = NULL;

      zMapAssert(draw_data->current_set);

      features = zmapWindowContainerGetFeatures(draw_data->container_strand);
      draw_data->container_feature_set = zmapWindowContainerCreate(features, ZMAPCONTAINER_LEVEL_FEATURESET,
                                                                   SET_CHILD_SPACING, 
                                                                   &(draw_data->navigate->column_background), NULL, NULL);

      g_object_set_data(G_OBJECT(draw_data->container_feature_set), ITEM_FEATURE_STATS, 
			zmapWindowStatsCreate((ZMapFeatureAny)draw_data->current_set)) ;

      status = zmapWindowFToIAddSet(draw_data->navigate->ftoi_hash, 
                                    draw_data->current_align->unique_id,
                                    draw_data->current_block->unique_id,
                                    set_id, ZMAPSTRAND_NONE, ZMAPFRAME_NONE,
                                    draw_data->container_feature_set);
      zMapAssert(status);

      style    = zMapFeatureStyleCopy(style);

      style_list = zmapWindowFeatureSetStyles(draw_data->navigate->current_window,
					      draw_data->styles, set_id);

      if(style_list)		/* This should be tested earlier! i.e. We shouldn't be creating the column. */
	{
	  set_data = zmapWindowItemFeatureSetCreate(draw_data->navigate->current_window,
						    draw_data->container_feature_set,
						    set_id, 0, style_list,
						    ZMAPSTRAND_FORWARD, ZMAPFRAME_NONE);

	  zmapWindowItemFeatureSetAttachFeatureSet(set_data, draw_data->current_set);
	  
	  g_list_free(style_list);
	}

      zmapWindowContainerSetVisibility(draw_data->container_feature_set, TRUE);

      container_background = zmapWindowContainerGetBackground(draw_data->container_feature_set);

      zmapWindowContainerSetBackgroundSize(draw_data->container_feature_set, 
					   draw_data->current_block->block_to_sequence.t2 * draw_data->navigate->scaling_factor);

      /* scale doesn't need this. */
      if(set_id != g_quark_from_string(ZMAP_FIXED_STYLE_SCALE_NAME))
        g_signal_connect(G_OBJECT(container_background), "event",
                         G_CALLBACK(columnBackgroundEventCB), 
                         (gpointer)draw_data->navigate);
    }
  else if(!style)
    printf("Failed to find style with id %s\n", g_quark_to_string(set_id));
  else
    printf("Failed to find ftset with id %s\n", g_quark_to_string(set_id));

  return ;
}

static void setupLocatorGroup(ZMapWindowNavigator navigate)
{
  FooCanvasGroup *locator_grp  = NULL;
  FooCanvasItem  *locator      = NULL;
  FooCanvasItem  *locator_drag = NULL;
  TransparencyEvent transp_data = NULL;
  double init_y1 = 0.0, init_y2 = 0.0;
  double init_x1 = 0.0, init_x2 = 0.0;

  init_x1 = navigate->locator_x1;
  init_x2 = navigate->locator_x2;

  if(!(locator_grp = navigate->locator_group))
    {
      FooCanvasGroup *root_features = NULL;
      root_features = zmapWindowContainerGetFeatures(navigate->container_root);

      locator_grp = navigate->locator_group = 
        FOO_CANVAS_GROUP(foo_canvas_item_new(root_features,
                                             foo_canvas_group_get_type(),
                                             "x", 0.0,
                                             "y", 0.0,
                                             NULL));
    }

  if(!(locator = navigate->locator))
    {
      locator = navigate->locator = 
        foo_canvas_item_new(FOO_CANVAS_GROUP(locator_grp),
                            foo_canvas_rect_get_type(),
                            "x1", 0.0,
                            "y1", init_y1,
                            "x2", 100.0,
                            "y2", init_y2,
                            "outline_color_gdk", &(navigate->locator_border_gdk),
                            "fill_color_gdk",    (GdkColor *)(NULL),
#ifdef RDS_WITH_STIPPLE
                            //"fill_stipple",      navigate->locator_stipple,
#endif
                            //"fill_color_gdk",    &(navigate->locator_fill_gdk),
                            "width_pixels",      navigate->locator_bwidth,
                            NULL);

      //foo_canvas_item_lower_to_bottom(locator);
    }

  if(!(locator_drag = navigate->locator_drag))
    {
      locator_drag = navigate->locator_drag = 
        foo_canvas_item_new(FOO_CANVAS_GROUP(locator_grp),
                            foo_canvas_rect_get_type(),
                            "x1", init_x1,
                            "y1", init_y1,
                            "x2", init_x2,
                            "y2", init_y2,
                            "outline_color_gdk", &(navigate->locator_drag_gdk),
                            "fill_color_gdk",    (GdkColor *)(NULL),
                            "width_pixels",      navigate->locator_bwidth,
                            NULL);
      
      foo_canvas_item_hide(FOO_CANVAS_ITEM(locator_drag));
    }

  if((transp_data =  g_new0(TransparencyEventStruct, 1)))
    {
      FooCanvasItem *root_bg = NULL;

      root_bg = zmapWindowContainerGetBackground(navigate->container_root);

      transp_data->navigate      = navigate;
      transp_data->locator_click = FALSE;
      transp_data->click_correction = 0.0;

      g_signal_connect(GTK_OBJECT(root_bg), "event",
                       GTK_SIGNAL_FUNC(rootBGEventCB), transp_data);

    }

  return ;
}

static void clampCoords(ZMapWindowNavigator navigate, 
                        double pre_scale, double post_scale, 
                        double *c1_inout, double *c2_inout)
{
  double top, bot;
  double min, max;
  
  min = (double)(navigate->full_span.x1) * pre_scale;
  max = (double)(navigate->full_span.x2) * pre_scale;

  top = *c1_inout;
  bot = *c2_inout;

  zMapGUICoordsClampSpanWithLimits(min, max, &top, &bot);

  *c1_inout = top * post_scale;
  *c2_inout = bot * post_scale;

  return ;
}


static void clampScaled(ZMapWindowNavigator navigate, double *s1_inout, double *s2_inout)
{
  clampCoords(navigate, 
              navigate->scaling_factor, 
              1.0,
              s1_inout,
              s2_inout);

  return ;
}

static void clampWorld2Scaled(ZMapWindowNavigator navigate, double *w1_inout, double *w2_inout)
{
  clampCoords(navigate, 
              1.0,
              navigate->scaling_factor, 
              w1_inout,
              w2_inout);
  return ;
}

static void updateLocatorDragger(ZMapWindowNavigator navigate, double button_y, double size)
{
  double a, b;

  a = button_y;
  b = button_y + size - 1.0;
  
  clampScaled(navigate, &a, &b);
  
  if(locator_debug_G)
    printCoordsInfo(navigate, "updateLocatorDragger", a, b);
  
  foo_canvas_item_set(FOO_CANVAS_ITEM(navigate->locator_drag),
		      "y1", a,
		      "y2", b,
		      NULL);

  return ;
}

static gboolean rootBGEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  TransparencyEvent transp_data = (TransparencyEvent)data;
  ZMapWindowNavigator navigate = NULL;
  gboolean event_handled = FALSE;

  navigate = transp_data->navigate;

  switch(event->type)
    {
    case GDK_BUTTON_PRESS:
      {
        GdkEventButton *button = (GdkEventButton *)event;
        zMapAssert(navigate->locator_drag);

        if(button->button  == 1)
          {
            double locator_y1, locator_y2, locator_size, y_coord, x = 0.0;
            gboolean within_fuzzy = FALSE;
            int fuzzy_dist = 6;

            foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(navigate->locator), NULL, &locator_y1, NULL, &locator_y2);

            y_coord = (double)button->y;

            if( (y_coord > locator_y1) && (y_coord < locator_y2) )
              within_fuzzy = TRUE;
            else if(fuzzy_dist > 1 && fuzzy_dist < 10)
              {
                int cx, cy1, cy2, cbut;
                FooCanvas *canvas = FOO_CANVAS(FOO_CANVAS_ITEM(navigate->locator)->canvas);
                foo_canvas_w2c(canvas, x, locator_y1, &cx, &cy1);
                foo_canvas_w2c(canvas, x, locator_y2, &cx, &cy2);
                foo_canvas_w2c(canvas, x, y_coord,    &cx, &cbut);

                if(cy2 - cy1 <= fuzzy_dist)
                  {
                    fuzzy_dist /= 2;
                    cy1 -= fuzzy_dist;
                    cy2 += fuzzy_dist;
                    if(cbut < cy2 && cbut > cy1)
                      within_fuzzy = TRUE;
                  }
              }

            if(within_fuzzy == TRUE)
              {
                transp_data->click_correction = y_coord - locator_y1;

                locator_y1  += LOCATOR_LINE_WIDTH / 2.0;
                locator_y2  -= LOCATOR_LINE_WIDTH / 2.0;

                locator_size = locator_y2 - locator_y1 + 1.0;
           
                foo_canvas_item_show(navigate->locator_drag);
                foo_canvas_item_lower_to_bottom(navigate->locator_drag);
                
                updateLocatorDragger(navigate, y_coord - transp_data->click_correction, locator_size);
            
                event_handled = transp_data->locator_click = TRUE;
              }
          }
      }
      break;
    case GDK_BUTTON_RELEASE:
      {
        double locator_y1, locator_y2, origin_y1, origin_y2, dummy;
        zMapAssert(navigate->locator_drag);
        
        if(transp_data->locator_click == TRUE)
          {
            foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(navigate->locator_drag), NULL, &locator_y1, NULL, &locator_y2);
            foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(navigate->locator), NULL, &origin_y1, NULL, &origin_y2);

            locator_y1 += LOCATOR_LINE_WIDTH / 2.0;
            locator_y2 -= LOCATOR_LINE_WIDTH / 2.0;

            origin_y1 += LOCATOR_LINE_WIDTH / 2.0;
            origin_y2 -= LOCATOR_LINE_WIDTH / 2.0;
            
            foo_canvas_item_i2w(FOO_CANVAS_ITEM(navigate->locator_drag), &dummy, &locator_y1);
            foo_canvas_item_i2w(FOO_CANVAS_ITEM(navigate->locator_drag), &dummy, &locator_y2);

            foo_canvas_item_i2w(FOO_CANVAS_ITEM(navigate->locator), &dummy, &origin_y1);
            foo_canvas_item_i2w(FOO_CANVAS_ITEM(navigate->locator), &dummy, &origin_y2);

            locator_y1 /= navigate->scaling_factor;
            locator_y2 /= navigate->scaling_factor;

            origin_y1 /= navigate->scaling_factor;
            origin_y2 /= navigate->scaling_factor;
            
            foo_canvas_item_hide(navigate->locator_drag);
            
            zMapWindowNavigatorDrawLocator(navigate, locator_y1, locator_y2);

            if(locator_y1 != origin_y1 && locator_y2 != origin_y2)
              zmapWindowNavigatorValueChanged(NAVIGATOR_WIDGET(navigate), locator_y1, locator_y2);
            
            transp_data->locator_click = FALSE;
            
            event_handled = TRUE;
          }
      }
      break;
    case GDK_MOTION_NOTIFY:
      {
        double button_y = 0.0, locator_y1, 
          locator_y2, locator_size = 0.0;
        zMapAssert(navigate->locator_drag);
        
        if(transp_data->locator_click == TRUE)
          {
            button_y = (double)event->button.y - transp_data->click_correction;

            foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(navigate->locator), NULL, &locator_y1, NULL, &locator_y2);
            
            locator_y1 += LOCATOR_LINE_WIDTH / 2.0;
            locator_y2 -= LOCATOR_LINE_WIDTH / 2.0;

            locator_size = locator_y2 - locator_y1 + 1.0;

            updateLocatorDragger(navigate, button_y, locator_size);
            
            event_handled = TRUE;
          }
      }
      break;
    default:
      {
        /* By default we _don't_handle events. */
        event_handled = FALSE ;
        break ;
      }
      
    }
  
  return event_handled;
}



static void makeMenuFromCanvasItem(GdkEventButton *button, FooCanvasItem *item, gpointer data)
{
  static ZMapGUIMenuItemStruct separator[] =
    {
      {ZMAPGUI_MENU_SEPARATOR, NULL, 0, NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;
  NavigateMenuCBData menu_data = NULL;
  ZMapFeatureAny feature_any = NULL;
  GList *menu_sets = NULL;
  char *menu_title = NULL;
  gboolean bumping_works = TRUE;

  feature_any = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);

  menu_title  = zMapFeatureName(feature_any);

  if((menu_data = g_new0(NavigateMenuCBDataStruct, 1)))
    {
      ZMapStyleOverlapMode bump_mode = ZMAPOVERLAP_COMPLETE;
      menu_data->item     = item;
      menu_data->navigate = (ZMapWindowNavigator)data;

      if(feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
        {
	  ZMapFeatureTypeStyle style;
          ZMapFeature feature = (ZMapFeature)feature_any;

	  style = zmapWindowItemGetStyle(item) ;
          menu_data->item_cb  = TRUE;
          /* This is the variant filter... Shouldn't be in the menu code too! */

          if(feature->locus_id != 0)
            {
              menu_sets = g_list_append(menu_sets, zmapWindowNavigatorMakeMenuLocusOps(NULL, NULL, menu_data));
              menu_sets = g_list_append(menu_sets, zmapWindowNavigatorMakeMenuLocusColumnOps(NULL, NULL, menu_data));
              menu_sets = g_list_append(menu_sets, separator);
            }

	  bump_mode = zMapStyleGetOverlapMode(style);
        }
      else
        {
          /* get set_data->style */
          ZMapWindowItemFeatureSetData set_data ;

          set_data = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_SET_DATA) ;
          zMapAssert(set_data) ;

	  if(set_data->unique_id == menu_data->navigate->locus_id)
	    {
              menu_sets = g_list_append(menu_sets, zmapWindowNavigatorMakeMenuLocusColumnOps(NULL, NULL, menu_data));
              menu_sets = g_list_append(menu_sets, separator);
	    }

	  bump_mode = zmapWindowItemFeatureSetGetOverlapMode(set_data);
          menu_data->item_cb  = FALSE;
        }

      if(bumping_works)
        {
          menu_sets = g_list_append(menu_sets, zmapWindowNavigatorMakeMenuBump(NULL, NULL, menu_data, bump_mode));
          menu_sets = g_list_append(menu_sets, separator);
        }

      menu_sets = g_list_append(menu_sets, zmapWindowNavigatorMakeMenuColumnOps(NULL, NULL, menu_data));

    }

  zMapGUIMakeMenu(menu_title, menu_sets, button);

  return ;
}

static gboolean columnBackgroundEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE;
  //ZMapWindowNavigator navigate = (ZMapWindowNavigator)data;

  switch(event->type)
    {
    case GDK_BUTTON_PRESS:
      {
        GdkEventButton *button = (GdkEventButton *)event;
        if(button->button == 3)
          {
            item = FOO_CANVAS_ITEM(zmapWindowContainerGetParent(item));
            makeMenuFromCanvasItem(button, item, data);
            event_handled = TRUE;
          }
      }
      break;
    default:
      break;
    }

  return event_handled;
}

static gboolean navCanvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE;
  ZMapWindowNavigator navigate = (ZMapWindowNavigator)data;
  ZMapWindowItemFeatureType item_feature_type ;
  ZMapFeature feature = NULL;
  static guint32 last_but_press = 0 ;			    /* Used for double clicks... */

  switch(event->type)
    {
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
      {
	GdkEventButton *button = (GdkEventButton *)event ;
        
        item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
                                                              ITEM_FEATURE_TYPE)) ;
        zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE
                   || item_feature_type == ITEM_FEATURE_CHILD
                   || item_feature_type == ITEM_FEATURE_BOUNDING_BOX) ;
        
        /* Retrieve the feature item info from the canvas item. */
        feature = (ZMapFeature)g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);  
        zMapAssert(feature) ;

        if(button->type == GDK_BUTTON_PRESS)
          {
            if (button->time - last_but_press > 250)
              {
                if(button->button == 1)
                  {
                    /* ignore ATM */
                  }
                else if(button->button == 3)
                  {
                    /* make item menu */
                    makeMenuFromCanvasItem(button, item, navigate);
                  }
              }
            last_but_press = button->time ;
            
	    event_handled = TRUE ;
          }
        else
          {
            if(button->button == 1 && feature->locus_id != 0)
              {
                zmapWindowNavigatorGoToLocusExtents(navigate, item);
                event_handled = TRUE;
              }
          }
      }
      break;
    default:
      break;
    }

  return event_handled;
}

static gboolean navCanvasItemDestroyCB(FooCanvasItem *feature_item, gpointer data)
{
  //ZMapWindowNavigator navigate = (ZMapWindowNavigator)data;
  if(locator_debug_G)
    printf("item got destroyed\n");

  return FALSE;
}

static void factoryItemHandler(FooCanvasItem            *new_item,
                               ZMapWindowItemFeatureType new_item_type,
                               ZMapFeature               full_feature,
                               ZMapWindowItemFeature     sub_feature,
                               double                    new_item_y1,
                               double                    new_item_y2,
                               gpointer                  handler_data)
{
  g_signal_connect(GTK_OBJECT(new_item), "event", 
                   GTK_SIGNAL_FUNC(navCanvasItemEventCB), handler_data);
  g_signal_connect(GTK_OBJECT(new_item), "destroy",
                   GTK_SIGNAL_FUNC(navCanvasItemDestroyCB), handler_data);
  return ;
}

static gboolean factoryFeatureSizeReq(ZMapFeature feature, 
                                      double *limits_array, 
                                      double *points_array_inout, 
                                      gpointer handler_data)
{
  ZMapWindowNavigator navigate = (ZMapWindowNavigator)handler_data;
  gboolean outside = FALSE;
  double scale_factor = 1.0;    /* get from handler_data! */
  double *x1_inout = &(points_array_inout[1]);
  double *x2_inout = &(points_array_inout[3]);
  int start_end_crossing = 0;
  double block_start, block_end;
  LocusEntry hash_entry = NULL;

  scale_factor = navigate->scaling_factor;

  block_start  = limits_array[1] * scale_factor;
  block_end    = limits_array[3] * scale_factor;
  
  *x1_inout = *x1_inout * scale_factor;
  *x2_inout = *x2_inout * scale_factor;

  if(navigate->locus_id == feature->parent->unique_id)
    {
      points_array_inout[0] += 20;
      points_array_inout[2] += 20;

      if((hash_entry = zmapWindowNavigatorLDHFind(navigate->locus_display_hash, 
                                                  feature->original_id)))
        {
          /* we only ever draw the first one of these. */
          outside = TRUE;
        }
      else if((hash_entry = zmapWindowNavigatorLDHInsert(navigate->locus_display_hash, feature,
                                                         *x1_inout, *x2_inout)))
        {
          outside = FALSE;
        }
      else
        {
          zMapAssertNotReached();
        }
    }
    
  /* shift according to how we cross, like this for ease of debugging, not speed */
  start_end_crossing |= ((*x1_inout < block_start) << 1);
  start_end_crossing |= ((*x2_inout > block_end)   << 2);
  start_end_crossing |= ((*x1_inout > block_end)   << 3);
  start_end_crossing |= ((*x2_inout < block_start) << 4);
  
  /* Now check whether we cross! */
  if(start_end_crossing & 8 || 
     start_end_crossing & 16) /* everything is out of range don't display! */
    outside = TRUE;
  
  if(start_end_crossing & 2)
    *x1_inout = block_start;
  if(start_end_crossing & 4)
    *x2_inout = block_end;
  
  if(hash_entry)
    {
      /* extend the values in the hash list */
      if(hash_entry->start > *x1_inout)
        hash_entry->start  = *x1_inout;
      if(hash_entry->end < *x2_inout)
        hash_entry->end  = *x2_inout;
    }

  return outside;
}


static void printCoordsInfo(ZMapWindowNavigator navigate, char *message, double start, double end)
{
  double sf = navigate->scaling_factor;
  double top, bot;

  top = start / sf;
  bot = end   / sf;

  printf("%s: [nav] %f -> %f (%f) = [wrld] %f -> %f (%f)\n", 
         message, 
         start, end, end - start + 1.0,
         top, bot, bot - top + 1.0);

  return ;
}

static void customiseFactory(ZMapWindowNavigator navigate)
{
  ZMapWindowFToIFactoryProductionTeamStruct factory_helpers = {NULL};
  
  /* create a factory and set up */ 
  navigate->item_factory = zmapWindowFToIFactoryOpen(navigate->ftoi_hash, NULL);
  factory_helpers.feature_size_request = factoryFeatureSizeReq;
  factory_helpers.item_created         = factoryItemHandler;
  zmapWindowFToIFactorySetup(navigate->item_factory, 1, /* line_width hardcoded for now. */
                             &factory_helpers, (gpointer)navigate);

  return ;
}



/* mini package for the locus_display_hash ... */
/*
 * \brief Create a hash for the locus display.
 * 
 * Hash is keyed on GUINT_TO_POINTER(feature->unique_id)
 */
static GHashTable *zmapWindowNavigatorLDHCreate(void)
{
  GHashTable *hash = NULL;
  
  hash = g_hash_table_new_full(NULL, NULL, NULL, destroyLocusEntry);

  return hash;
}

/* 
 * \brief finds the entry.
 */
static LocusEntry zmapWindowNavigatorLDHFind(GHashTable *hash, GQuark key)
{
  LocusEntry hash_entry = NULL;

  hash_entry = g_hash_table_lookup(hash, GUINT_TO_POINTER(key));

  return hash_entry;
}
/* 
 * \brief creates a new entry from feature, start and end (scaled) and returns it.
 */
static LocusEntry zmapWindowNavigatorLDHInsert(GHashTable *hash, 
                                               ZMapFeature feature, 
                                               double start, double end)
{
  LocusEntry hash_entry = NULL;

  if((hash_entry = g_new0(LocusEntryStruct, 1)))
    {
      hash_entry->start   = start;
      hash_entry->end     = end;
      hash_entry->strand  = feature->strand;
      hash_entry->feature = feature; /* So we can find the item */
      g_hash_table_insert(hash, 
                          GUINT_TO_POINTER(feature->original_id), 
                          (gpointer)hash_entry);
    }
  else
    zMapAssertNotReached();

  return hash_entry;
}
/* 
 * \brief Destroy the hash **! NULL's for you.
 */
static void zmapWindowNavigatorLDHDestroy(GHashTable **destroy_me)
{
  zMapAssert(destroy_me && *destroy_me);

  g_hash_table_destroy(*destroy_me);

  *destroy_me = NULL;

  return ;
}

static void destroyLocusEntry(gpointer data)
{
  LocusEntry locus_entry = (LocusEntry)data;

  zMapAssert(data);

  locus_entry->feature = NULL;
  
  g_free(locus_entry);

  return ;
}


static void get_filter_list_up_to(GList **filter_out, int max)
{
  if(max <= LOCUS_NAMES_LENGTH)
    {
      int i;
      char **names_ptr = locus_names_filter_G;
      GList *filter = NULL;

      for(i = 0; i < LOCUS_NAMES_LENGTH && names_ptr; i++, names_ptr++)
	{
	  if(i < max && i != LOCUS_NAMES_SEPARATOR)
	    {
	      filter = g_list_append(filter, *names_ptr);
	    }
	}
      if(filter_out)
	*filter_out = filter;
    }

  return ;
}

static void available_locus_names_filter(GList **filter_out)
{
  GList *filter = NULL;

  if(filter_out)
    {
      get_filter_list_up_to(&filter, LOCUS_NAMES_LENGTH);
      *filter_out = filter;
    }

  return ;
}

static void default_locus_names_filter(GList **filter_out)
{
  GList *filter = NULL;

  if(filter_out)
    {
      get_filter_list_up_to(&filter, LOCUS_NAMES_SEPARATOR);
      *filter_out = filter;
    }

  return ;
}

static gint strcmp_list_find(gconstpointer list_data, gconstpointer user_data)
{
  gint result = -1;

  result = strncmp(list_data, user_data, strlen(list_data));

  return result;
}



static ZMapFeatureTypeStyle getPredefinedStyleByName(char *style_name)
{
  GQuark requested_id;
  ZMapFeatureTypeStyle *curr, result = NULL;
  static ZMapFeatureTypeStyle predefined[] = {
    NULL,			/* genomic_canonical */
    NULL		       	/* END VALUE */
  };
  curr = &predefined[0];
  
  if((*curr == NULL))
    {
      /* initialise */
      *curr = zMapStyleCreate("genomic_canonical", "Genomic Canonical");
      g_object_set(G_OBJECT(*curr),
		   "overlap_mode", ZMAPOVERLAP_OSCILLATE,
		   NULL);
      curr++;
    }

  curr = &predefined[0] ;

  requested_id = zMapStyleCreateID(style_name);

  while ((curr != NULL) && *curr != NULL)
    {
      GQuark predefined_id;
      g_object_get(G_OBJECT(*curr),
		   "unique-id", &predefined_id,
		   NULL);
      if(requested_id == predefined_id)
	{
	  result = *curr;
	  break;
	}

      curr++ ;
    }

  return result;
}


static void highlight_columns_cb(FooCanvasGroup *container, FooCanvasPoints *points, 
				 ZMapContainerLevelType level, gpointer user_data)
{
  NavigatorLocator nav_data = (NavigatorLocator)user_data;
  FooCanvasGroup *container_underlay;
  FooCanvasGroup *container_overlay;
  FooCanvasGroup *container_features;

  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_ALIGN:
    case ZMAPCONTAINER_LEVEL_BLOCK:
    case ZMAPCONTAINER_LEVEL_STRAND:
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      container_underlay = zmapWindowContainerGetUnderlays(container);
      container_features = zmapWindowContainerGetFeatures(container);
      container_overlay  = zmapWindowContainerGetOverlays(container);

      foo_canvas_item_set(FOO_CANVAS_ITEM(container_underlay),
			  "x", 0.0, "y", 0.0, NULL);
      foo_canvas_item_set(FOO_CANVAS_ITEM(container_features),
			  "x", 0.0, "y", 0.0, NULL);

      if(container_overlay && container_overlay->item_list)
	{
	  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(container),
				     &(points->coords[0]), NULL,
				     &(points->coords[2]), NULL);
	  points->coords[0] -= container->xpos;
	  points->coords[2] -= container->xpos;
	}
      else
	{
	  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(container_features),
				     &(points->coords[0]), NULL,
				     &(points->coords[2]), NULL);
	}

      if(container_underlay->item_list == NULL)
	foo_canvas_item_new(container_underlay,
			    foo_canvas_rect_get_type(),
			    "x1", points->coords[0],
			    "x2", points->coords[2],
			    "y1", nav_data->top,
			    "y2", nav_data->bot,
			    "fill_color_gdk", &(nav_data->highlight_colour),
			    "width_pixels", 0,
			    NULL);
      else if(container_underlay->item_list == container_underlay->item_list_end)
	foo_canvas_item_set(FOO_CANVAS_ITEM(container_underlay->item_list->data),
			    "x1", points->coords[0],
			    "x2", points->coords[2],
			    "y1", nav_data->top,
			    "y2", nav_data->bot,
			    "fill_color_gdk", &(nav_data->highlight_colour),
			    "width_pixels", 0,
			    NULL);
      break;
    default:
      zMapAssertNotReached();
      break;
    }

  return ;
}

static void locator_highlight_column_areas(ZMapWindowNavigator navigate,
					   double x1, double x2,
					   double raw_top, double raw_bot)
{
  NavigatorLocatorStruct tmp_data = {};
  tmp_data.navigate = navigate;
  tmp_data.top      = raw_top;
  tmp_data.bot      = raw_bot;
  tmp_data.x1       = x1;
  tmp_data.x2       = x2 - 1.0;

  gdk_color_parse("white", &(tmp_data.highlight_colour));

  zmapWindowContainerExecuteFull(navigate->container_align, 
                                 ZMAPCONTAINER_LEVEL_FEATURESET,
                                 NULL, NULL, highlight_columns_cb, 
				 &tmp_data, FALSE);  

  return ;
}

static void foo_canvas_item_print(FooCanvasItem *item, GString *data)
{
  if(FOO_IS_CANVAS_GROUP(item))
    {
      g_string_append_c(data,'\t');
      g_list_foreach(FOO_CANVAS_GROUP(item)->item_list, (GFunc)foo_canvas_item_print, data);
      g_string_truncate(data, data->len - 1);
    }
  else
    {
      double ix1, iy1, ix2, iy2;
      double wx1, wy1, wx2, wy2;
      foo_canvas_item_get_bounds(item, &ix1, &iy1, &ix2, &iy2);
      wx1 = ix1;
      wx2 = ix2;
      wy1 = iy1;
      wy2 = iy2;
      foo_canvas_item_i2w(item, &wx1, &wy1);
      foo_canvas_item_i2w(item, &wx2, &wy2);
      printf("%s - %f,%f -> %f,%f (%f,%f -> %f,%f)\n", data->str,
	     ix1, iy1, ix2, iy2,
	     wx1, wy1, wx2, wy2);
    }
  return ;
}

static void real_focus_navigate(ZMapWindowNavigator navigate)
{
  if(navigate->container_align)
    {
      double x1, x2, y1, y2;
      
      x1 = y1 = x2 = y2 = 0.0;

      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(navigate->container_align), 
				 &x1, &y1, &x2, &y2);

      if(locator_debug_G)
	{
	  GString *string = g_string_sized_new(128);
	  foo_canvas_item_print(FOO_CANVAS_ITEM(navigate->locator_group), string);
	  g_string_free(string, TRUE);
	}

      zmapWindowNavigatorSizeRequest(NAVIGATOR_WIDGET(navigate), x2 - x1 + 1, y2 - y1 + 1);
      
      zmapWindowNavigatorFillWidget(NAVIGATOR_WIDGET(navigate));
    }

  return ;
}

static gboolean nav_focus_expose_handler(GtkWidget *widget, GdkEventExpose *expose, gpointer user_data)
{
  ZMapWindowNavigator navigate = (ZMapWindowNavigator)user_data;
  gulong expose_id;
  gboolean handled = FALSE;

  if(navigate->draw_expose_handler_id)
    {
      g_signal_emit(G_OBJECT(widget), navigate->draw_expose_handler_id,
		    g_quark_from_string("expose_event"), &handled);
      zMapAssert(navigate->draw_expose_handler_id == 0);
    }

  expose_id = navigate->focus_expose_handler_id;

  g_signal_handler_block(G_OBJECT(widget), expose_id);

  g_signal_handler_disconnect(G_OBJECT(widget), expose_id);      

  /* zero this before any other code. */
  navigate->focus_expose_handler_id = 0;

  real_focus_navigate(navigate);
  
  return handled;		/* FALSE to let others run... */
}

static void real_draw_locator(ZMapWindowNavigator navigate)
{
  double x1, y1, x2, y2;
  
  x1 = navigate->locator_x1; 
  x2 = navigate->locator_x2;
  
  y1 = navigate->locator_span.x1;
  y2 = navigate->locator_span.x2;
  
  clampWorld2Scaled(navigate, &y1, &y2);
  
  if(!navigate->locator)
    setupLocatorGroup(navigate);

  foo_canvas_item_set(FOO_CANVAS_ITEM(navigate->locator),
		      "x1", 0.0,
		      "y1", y1,
		      "x2", x2,
		      "y2", y2,
		      NULL);

  if(locator_debug_G)
    {
      GString *string = g_string_sized_new(128);
      foo_canvas_item_print(FOO_CANVAS_ITEM(navigate->locator_group), string);
      g_string_free(string, TRUE);
    }

  foo_canvas_item_show(FOO_CANVAS_ITEM(navigate->locator));

  foo_canvas_item_raise_to_top(FOO_CANVAS_ITEM(navigate->locator));

  foo_canvas_item_set(FOO_CANVAS_ITEM(navigate->locator_drag),
		      "x1", 0.0,
		      "y1", y1,
		      "x2", x2,
		      "y2", y2,
		      NULL);
  
  locator_highlight_column_areas(navigate, x1, x2, y1, y2);
  
  if(locator_debug_G)
    printCoordsInfo(navigate, "nav_locator_expose_handler", y1, y2);
  
  foo_canvas_item_show(FOO_CANVAS_ITEM(navigate->locator_group));
  foo_canvas_item_raise_to_top(FOO_CANVAS_ITEM(navigate->locator_group));

  return ;
}

static gboolean nav_locator_expose_handler(GtkWidget *widget, GdkEventExpose *expose, gpointer user_data)
{
  ZMapWindowNavigator navigate = (ZMapWindowNavigator)user_data;
  gulong expose_id;
  gboolean handled = FALSE;

  if(navigate->draw_expose_handler_id)
    {
      g_signal_emit(G_OBJECT(widget), navigate->draw_expose_handler_id,
		    g_quark_from_string("expose_event"), &handled);
      zMapAssert(navigate->draw_expose_handler_id == 0);
    }

  expose_id = navigate->locator_expose_handler_id;

  g_signal_handler_block(G_OBJECT(widget), expose_id);

  g_signal_handler_disconnect(G_OBJECT(widget), expose_id);      
  /* zero this before any other code. */
  navigate->locator_expose_handler_id = 0;

  real_draw_locator(navigate);

  return handled;
}
