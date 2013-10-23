/*  File: zmapWindowNav.c
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
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implements the separate scale/scrolling window widget which
 *              also holds loci which can be searched/filtered.
 *
 * Exported functions: See ZMap/zmapNavigator.h
 *
 *------------------------------------------------------------- - navigate->full_span.x1 + 1------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <string.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapUtilsGUI.h>
#include <ZMap/zmapUtils.h>
#include <zmapWindowNavigator_P.h>
#include <zmapWindowCanvasLocus.h>
#include <zmapWindowContainerFeatureSet_I.h>


/* Return the widget! */
#define NAVIGATOR_WIDGET(navigate) GTK_WIDGET(fetchCanvas(navigate))



typedef struct
{
  /* inputs */
  ZMapWindowNavigator navigate;
  ZMapFeatureContext  context;
  GHashTable              *styles;

  /* The current features in the recursion */
  ZMapFeatureAlignment current_align;
  ZMapFeatureBlock     current_block;
  ZMapFeatureSet       current_set;
  /* The current containers in the recursion */
  ZMapWindowContainerGroup container_block;
  ZMapWindowContainerGroup container_feature_set;
  double current;

} NavigateDrawStruct, *NavigateDraw;

typedef struct
{
  double start, end;
  ZMapStrand strand;
  ZMapFeature feature;
} LocusEntryStruct, *LocusEntry;





/* draw some features... */
static ZMapFeatureContextExecuteStatus drawContext(GQuark key,
                                                   gpointer data,
                                                   gpointer user_data,
                                                   char **err_out);
static void createColumnCB(gpointer data, gpointer user_data);

static void clampCoords(ZMapWindowNavigator navigate,
                        double *c1_inout, double *c2_inout);
static void clampScaled(ZMapWindowNavigator navigate,
                        double *s1_inout, double *s2_inout);
#if 0
static void clampWorld2Scaled(ZMapWindowNavigator navigate,
                              double *w1_inout, double *w2_inout);
#endif
static void updateLocatorDragger(ZMapWindowNavigator navigate, double button_y, double size);
static gboolean rootBGEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data);
//static gboolean columnBackgroundEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data);

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

static gboolean factoryItemHandler(FooCanvasItem       *new_item,
				   ZMapWindowFeatureStack     feature_stack,
				   gpointer             handler_data);



static GHashTable *zmapWindowNavigatorLDHCreate(void);
static LocusEntry zmapWindowNavigatorLDHFind(GHashTable *hash, GQuark key);
static LocusEntry zmapWindowNavigatorLDHInsert(GHashTable *hash,
                                               ZMapFeature feature);
static void zmapWindowNavigatorLDHDestroy(GHashTable **destroy);
static void destroyLocusEntry(gpointer data);



static void get_filter_list_up_to(GList **filter_out, int max);
static void available_locus_names_filter(GList **filter_out);
static void default_locus_names_filter(GList **filter_out);



static void navigatorRunSet(ZMapFeatureSet set, FooCanvasGroup *container, ZMapFrame frame,
			    ZMapWindowNavigator navigate);



/*
 *                     globals
 */



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
  ZMapWindowNavigator navigate = NULL ;
  FooCanvasGroup *root = NULL ;

  zMapAssert(FOO_IS_CANVAS(canvas_widget)) ;

  navigate = g_new0(ZMapWindowNavigatorStruct, 1) ;

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
  gdk_color_parse(LOCATOR_HIGHLIGHT, &(navigate->locator_highlight));

  navigate->locator_bwidth      = LOCATOR_LINE_WIDTH;
  navigate->locator_x_coords.x1 = 0.0;
  navigate->locator_x_coords.x2 = LOCATOR_LINE_WIDTH * 10.0;

  /* create the root container */
  navigate->canvas = FOO_CANVAS(canvas_widget);
  root   = FOO_CANVAS_GROUP(foo_canvas_root(navigate->canvas));


  g_object_set_data(G_OBJECT(navigate->canvas), ZMAP_WINDOW_POINTER, navigate->current_window) ;


  navigate->container_root = zmapWindowContainerGroupCreate(root, ZMAPCONTAINER_LEVEL_ROOT,
								   ROOT_CHILD_SPACING,
								   &(navigate->root_background), NULL);


  g_object_set_data(G_OBJECT(navigate->container_root), ZMAP_WINDOW_POINTER, navigate->current_window) ;



  /* add it to the hash. */
  zmapWindowFToIAddRoot(navigate->ftoi_hash, (FooCanvasGroup *)(navigate->container_root));


  g_signal_connect(GTK_OBJECT(navigate->canvas), "event", GTK_SIGNAL_FUNC(rootBGEventCB), (gpointer) navigate) ;


  default_locus_names_filter(&(navigate->hide_filter));

  available_locus_names_filter(&(navigate->available_filters));

      /* this is to allow other files in this module to access this module's data */
      /* yes really */
  zMapWindowNavigatorSetWindowNavigator(canvas_widget,navigate);

  return navigate ;
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

      canvas = fetchCanvas(navigate);

      g_signal_handler_disconnect(G_OBJECT(canvas), expose_id);

      navigate->draw_expose_handler_id = 0;
    }

  /* Keep pointers in step and recreate what was destroyed */
  if(navigate->container_align != NULL)
    {
      gtk_object_destroy(GTK_OBJECT(navigate->container_align));
      navigate->container_align = NULL;
    }

	/* these will have been destroyed with the align */
  navigate->locator = navigate->locator_drag = NULL;

  /* The hash contains invalid pointers so destroy and recreate. */
  zmapWindowFToIDestroy(navigate->ftoi_hash);
  navigate->ftoi_hash = zmapWindowFToICreate();


  zmapWindowNavigatorLDHDestroy(&(navigate->locus_display_hash));
  navigate->locus_display_hash = zmapWindowNavigatorLDHCreate();

  return ;
}

void zMapWindowNavigatorFocus(ZMapWindowNavigator navigate,
                              gboolean true_eq_focus)
{
  FooCanvasItem *root;

  root = FOO_CANVAS_ITEM(navigate->container_root);
//printf("nav focus %p %d\n", navigate, true_eq_focus);
  if(true_eq_focus)
    {
      foo_canvas_item_show(root);
    }
  else
    {
      foo_canvas_item_hide(root);
    }

  return ;
}

void zMapWindowNavigatorSetCurrentWindow(ZMapWindowNavigator navigate, ZMapWindow window)
{
  zMapAssert(navigate && window);

  /* We should be updating the locator here too.  In the case of
   * unlocked windows the locator becomes out of step on focusing
   * until the visibility change handler is called */

//printf("nav set window %p -> %p\n", navigate, window);
 /* mh17: should also twiddle revcomped? but as revcomp applies to the view
     in violation of MVC then all the windows will follow
  */
  navigate->current_window = window ;

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
				     GHashTable *styles)
{
  FooCanvas *canvas = NULL;
  NavigateDrawStruct draw_data = {NULL};

//printf("nav draw features %p -> %p\n",navigate, navigate->current_window);

  draw_data.navigate  = navigate;
  draw_data.context   = full_context;
  draw_data.styles    = styles;

  navigate->full_span.x1 = full_context->master_align->sequence_span.x1;
  navigate->full_span.x2 = full_context->master_align->sequence_span.x2 + 1.0;

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
      /* NOTE this can get called before we even drew in the first place */
  if(navigate && navigate->locus_featureset)
  {
	/* this will clear zoom and cause a de-overlap recalc with new filtered data */
	zMapWindowCanvasFeaturesetRedraw(navigate->locus_featureset, 0.0);

  }
  return ;
}

/* draw locator */

/* this resizes the widget too after having done some canvas update
 * we could calculate this stuff without waiting for the canvas....
 * the locator gets redrawn when the window zoom changes so this is called from there via zmapView->visibilityChangeCB()
 * and also from this module due to drag and drop
 */

void zMapWindowNavigatorDrawLocator(ZMapWindowNavigator navigate,
                                    double raw_top, double raw_bot)
{
  FooCanvasItem *root;
  GtkWidget *widget;
  zMapAssert(navigate);

  root  = (FooCanvasItem *) navigate->container_root;
  if(!root)
	  return;
  widget = GTK_WIDGET(root->canvas );

  /* Always set these... */
  navigate->locator_y_coords.x1 = raw_top;
  navigate->locator_y_coords.x2 = raw_bot;

  foo_canvas_update_now(navigate->canvas);

  navigate->width = root->x2 - root->x1 + 1;
  navigate->height = root->y2 - root->y1 + 1;
//printf("DrawLocator %f\n",navigate->width);

  zmapWindowNavigatorSizeRequest(widget, navigate->width, navigate->height,
      (double) navigate->full_span.x1,
      (double) navigate->full_span.x2);

  zmapWindowNavigatorFillWidget(widget);

  if(navigate->locator)
  {
	  zMapWindowCanvasFeaturesetSetSequence((ZMapWindowFeaturesetItem) navigate->locator, raw_top,raw_bot);
	  foo_canvas_item_request_update(navigate->locator);

	  /* redraw all to ensure shrinking locator is wiped */
	  foo_canvas_item_request_redraw(navigate->canvas->root);
//	  zMapWindowCanvasFeaturesetExpose((ZMapWindowFeaturesetItem) navigate->locator);
  }

//  if(navigate->draw_expose_handler_id == 0)	/* sets nav scroll region ?? */
//	zmapWindowFullReposition(navigate->container_root,TRUE, "draw locator");

  return ;
}


/* destroy */
void zMapWindowNavigatorDestroy(ZMapWindowNavigator navigate)
{

  gtk_object_destroy(GTK_OBJECT(navigate->container_root)); // remove our item.

  zmapWindowFToIDestroy(navigate->ftoi_hash);

  zmapWindowNavigatorLDHDestroy(&(navigate->locus_display_hash));

  g_free(navigate);             /* possibly not enough ... */

  return ;
}




/*
 *                     Internal routines
 */



static FooCanvas *fetchCanvas(ZMapWindowNavigator navigate)
{
  FooCanvas *canvas = NULL;
  FooCanvasGroup *g = NULL;

  g = (FooCanvasGroup *)(navigate->container_root);

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



static void navigateDrawFunc(NavigateDraw nav_draw, GtkWidget *widget)
{
  ZMapWindowNavigator navigate = nav_draw->navigate;
  FooCanvasItem *root;

  /* We need this! */
  zMapAssert(navigate->current_window);

#if SIZE_INVALID
  zmapWindowNavigatorSizeRequest(widget, navigate->width, navigate->height,
      (double) navigate->full_span.x1,
      (double) navigate->full_span.x2);

  zmapWindowNavigatorFillWidget(widget);
#endif
  foo_canvas_busy(navigate->canvas, TRUE);

  /* Everything to get a context drawn, raised to top and visible. */
  zMapFeatureContextExecuteComplete((ZMapFeatureAny)(nav_draw->context),
                                    ZMAPFEATURE_STRUCT_FEATURE,
                                    drawContext,
                                    NULL, nav_draw);

  foo_canvas_item_request_update(navigate->canvas->root);
  foo_canvas_update_now(navigate->canvas);

  root  = (FooCanvasItem *) navigate->container_root;

  navigate->width = root->x2 - root->x1 + 1;
  navigate->height = root->y2 - root->y1 + 1;

//printf("draw func root: %f %f %f %f\n",root->x1,root->y1,navigate->width,navigate->height);

	/* these next two calls may not actually be useful due to race conditions */
  zmapWindowNavigatorSizeRequest(widget, navigate->width, navigate->height,
      (double) navigate->full_span.x1,
      (double) navigate->full_span.x2);

  zmapWindowNavigatorFillWidget(widget);

  /* bodge to trigger a good refresh (see RT 303016) */
  zMapWindowNavigatorDrawLocator(navigate, navigate->locator_y_coords.x1, navigate->locator_y_coords.x2);


// commenting this back in gives a navigator pane that's too narrow: very odd
// don-t: zmapWindowFullReposition(navigate->container_root,TRUE, "nav draw");

  foo_canvas_busy(navigate->canvas, FALSE);

  return ;
}




static ZMapFeatureContextExecuteStatus drawContext(GQuark key_id,
                                                   gpointer data,
                                                   gpointer user_data,
                                                   char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  NavigateDraw draw_data = (NavigateDraw)user_data;
  ZMapFeatureBlock     feature_block = NULL;
  ZMapFeatureSet       feature_set   = NULL;
  ZMapFeatureLevelType feature_type = ZMAPFEATURE_STRUCT_INVALID;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  ZMapWindowNavigator navigate = NULL;
  gboolean hash_status = FALSE;

  feature_type = feature_any->struct_type;

  navigate = draw_data->navigate;

  switch(feature_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      {
	FooCanvasItem *root = (FooCanvasItem *) navigate->container_root;
	double x,y;

	zmapWindowContainerAttachFeatureAny(navigate->container_root, feature_any);

	foo_canvas_set_scroll_region(root->canvas, 0.0, navigate->full_span.x1, 1.0, navigate->full_span.x2);

	/* Set root group to start where sequence starts... */
	y = navigate->full_span.x1 ;
	x = 0.0;
	foo_canvas_item_w2i(root, &x, &y) ;

	foo_canvas_item_set(root, "y", y, NULL) ;

      }
      break;

    case ZMAPFEATURE_STRUCT_ALIGN:
      {
	ZMapWindowContainerFeatures container_features;

        draw_data->current_align = (ZMapFeatureAlignment)feature_any;
        container_features = zmapWindowContainerGetFeatures(navigate->container_root);
        if(!navigate->container_align)
          {
            navigate->container_align = zmapWindowContainerGroupCreate((FooCanvasGroup *) container_features, ZMAPCONTAINER_LEVEL_ALIGN,
								       ALIGN_CHILD_SPACING,
								       &(navigate->align_background), NULL);

	    g_object_set_data(G_OBJECT(navigate->container_align), ZMAP_WINDOW_POINTER, navigate->current_window) ;

#if 0
	    container_group_add_highlight_area_item(navigate, navigate->container_align);
#endif

	    /* mh17: has to add thsi to avoid an assert in FtoIAddAlign() */
	    zmapWindowContainerAlignmentAugment((ZMapWindowContainerAlignment) navigate->container_align,
						(ZMapFeatureAlignment) feature_any);

	    //		zmapWindowDrawSetGroupBackground(navigate->container_align, 0, 1, 1.0, ZMAP_CANVAS_LAYER_ALIGN_BACKGROUND, NULL, NULL);

            hash_status = zmapWindowFToIAddAlign(navigate->ftoi_hash, key_id, (FooCanvasGroup *)(navigate->container_align));

            zMapAssert(hash_status);
          }
      }
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
        ZMapWindowContainerFeatures features = NULL;
        int block_start, block_end;

        draw_data->current_block = feature_block = (ZMapFeatureBlock)feature_any;

        block_start = feature_block->block_to_sequence.block.x1;
        block_end   = feature_block->block_to_sequence.block.x2;

        /* create the block and add the item to the hash */
        features    = zmapWindowContainerGetFeatures(draw_data->navigate->container_align);
        navigate->container_block = draw_data->container_block = zmapWindowContainerGroupCreate((FooCanvasGroup *) features, ZMAPCONTAINER_LEVEL_BLOCK,
												BLOCK_CHILD_SPACING,
												&(navigate->block_background), NULL);

	g_object_set_data(G_OBJECT(draw_data->container_block),
			  ZMAP_WINDOW_POINTER, draw_data->navigate->current_window) ;

#if 0
	container_group_add_highlight_area_item(navigate, draw_data->container_block);
#endif
	g_object_set_data(G_OBJECT(draw_data->container_block), ITEM_FEATURE_STATS,
			  zmapWindowStatsCreate((ZMapFeatureAny)draw_data->current_block)) ;


	zmapWindowContainerBlockAugment((ZMapWindowContainerBlock)draw_data->container_block,
					(ZMapFeatureBlock) feature_any) ;

	//	zmapWindowDrawSetGroupBackground(draw_data->container_block, 0, 1, 1.0, ZMAP_CANVAS_LAYER_BLOCK_BACKGROUND, NULL, NULL);

        hash_status = zmapWindowFToIAddBlock(navigate->ftoi_hash, draw_data->current_align->unique_id,
                                             key_id, (FooCanvasGroup *)(draw_data->container_block));
        zMapAssert(hash_status);

	navigate->locator = zmapWindowDrawSetGroupBackground(navigate->current_window, navigate->container_block,
							     block_start, block_end, 1.0,
							     ZMAP_CANVAS_LAYER_NAV_LOCATOR,
							     &navigate->locator_fill_gdk, &navigate->locator_border_gdk);

	navigate->locator_drag = zmapWindowDrawSetGroupBackground(navigate->current_window, navigate->container_block,
								  block_start, block_end, 1.0,
								  ZMAP_CANVAS_LAYER_NAV_LOCATOR_DRAG,
								  NULL, &navigate->locator_drag_gdk);
	foo_canvas_item_hide(navigate->locator_drag);

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
	ZMapFeatureSetDesc gffset;
        feature_set = (ZMapFeatureSet)feature_any;
	GQuark col_id = feature_set->unique_id;

        gffset = g_hash_table_lookup(draw_data->navigate->current_window->context_map->featureset_2_column,
				     GUINT_TO_POINTER(feature_set->unique_id));

	if (gffset)
	  col_id = gffset->column_id;

        draw_data->current_set = feature_set;

        status = ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND;

	if ((item = zMapFindCanvasColumn(draw_data->navigate->container_root,
					 draw_data->current_align->unique_id,
					 draw_data->current_block->unique_id,
					 col_id,
					 ZMAPSTRAND_FORWARD, ZMAPFRAME_NONE)))
          {
	    ZMapWindowContainerFeatureSet container_feature_set;
            FooCanvasGroup *group_feature_set;
	    ZMapStyleBumpMode bump_mode;

            group_feature_set = FOO_CANVAS_GROUP(item);
	    container_feature_set = (ZMapWindowContainerFeatureSet)item;

            navigatorRunSet(feature_set, group_feature_set, ZMAPFRAME_NONE, draw_data->navigate) ;

	    if ((bump_mode = zmapWindowContainerFeatureSetGetBumpMode(container_feature_set)) != ZMAPBUMP_UNBUMP)
	      zmapWindowColumnBumpRange(item, bump_mode, ZMAPWINDOW_COMPRESS_ALL) ;
	    //printf("nav draw context %p -> %p %s %s\n", draw_data->navigate, draw_data->navigate->current_window, g_quark_to_string(feature_set->unique_id), g_quark_to_string(col_id));
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

  item = zmapWindowFToIFindItemFull(draw_data->navigate->current_window,
				    draw_data->navigate->ftoi_hash,
				    draw_data->current_align->unique_id,
				    draw_data->current_block->unique_id,
				    scale_id, ZMAPSTRAND_FORWARD, ZMAPFRAME_NONE, 0);
  required = !item;

  return required;
}

static void drawScale(NavigateDraw draw_data)
{
  FooCanvasItem *item = NULL;

  GQuark scale_id = 0;
  int min, max;
  double zoom_factor = 0.0;


  /* HACK...  */
  scale_id = g_quark_from_string(ZMAP_FIXED_STYLE_SCALE_NAME);

  /* less of a hack ... */
  if ((item = zMapFindCanvasColumn(draw_data->navigate->container_root,
				   draw_data->current_align->unique_id,
				   draw_data->current_block->unique_id,
				   scale_id,	/* is same as column id */
				   ZMAPSTRAND_FORWARD, ZMAPFRAME_NONE)))
    {
      FooCanvasGroup *scale_group = NULL;
      FooCanvasGroup *features    = NULL;

      scale_group = FOO_CANVAS_GROUP(item);

      features = (FooCanvasGroup *)zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)scale_group);

      min = draw_data->context->master_align->sequence_span.x1;
      max = draw_data->context->master_align->sequence_span.x2;
      zoom_factor = item->canvas->pixels_per_unit_y;

      zMapWindowDrawScaleBar(draw_data->navigate->current_window->scrolled_window,
			     features, min, max, min, max, zoom_factor, draw_data->navigate->is_reversed, FALSE);
    }

  return ;
}




/* data is a GQuark, user_data is a NavigateDraw */
static void createColumnCB(gpointer data, gpointer user_data)
{
  GQuark set_id = GPOINTER_TO_UINT(data);
  NavigateDraw draw_data = (NavigateDraw)user_data;
  ZMapWindowContainerFeatures features;
  ZMapFeatureTypeStyle style;
  GQuark set_unique_id;

  /* We need the mapping stuff so navigator can use windowsearch calls and other stuff. */

  /* for the navigator styles are hard coded?? and there's no featureset_2_column mapping ?
     style = zMapWindowGetColumnStyle(draw_data->navigate->current_window,set_id);
  */

  /* mh17: need to canonicalise the set name to find the style */
  set_unique_id = zMapStyleCreateID((char *) g_quark_to_string(set_id)) ;

  style = zMapFindStyle(draw_data->styles, set_unique_id) ;

  draw_data->current_set = zMapFeatureBlockGetSetByID(draw_data->current_block, set_unique_id);


  if(!style)
    {
      zMapLogWarning("Failed to find style for navigator featureset '%s'", g_quark_to_string(set_id));
    }
  else if(draw_data->current_set)
    {
      ZMapFeatureSetDesc gffset;
      GQuark col_id = set_unique_id;
      ZMapWindowContainerFeatureSet container_set;

      features = zmapWindowContainerGetFeatures(ZMAP_CONTAINER_GROUP(draw_data->container_block));
      draw_data->container_feature_set = zmapWindowContainerGroupCreate((FooCanvasGroup *) features,
									ZMAPCONTAINER_LEVEL_FEATURESET,
									SET_CHILD_SPACING,
									&(draw_data->navigate->column_background),
									NULL);

      g_object_set_data(G_OBJECT(draw_data->container_feature_set),
			ZMAP_WINDOW_POINTER, draw_data->navigate->current_window) ;

      gffset = g_hash_table_lookup(draw_data->navigate->current_window->context_map->featureset_2_column,
				   GUINT_TO_POINTER(set_unique_id));
      if(gffset)
	col_id = gffset->column_id;

      container_set = (ZMapWindowContainerFeatureSet)draw_data->container_feature_set;

      zmapWindowContainerFeatureSetAugment(container_set,
					   draw_data->navigate->current_window,
					   draw_data->current_align->unique_id,
					   draw_data->current_block->unique_id,
					   col_id, col_id, style,
					   ZMAPSTRAND_FORWARD, ZMAPFRAME_NONE);

      zmapWindowContainerAttachFeatureAny(draw_data->container_feature_set, (ZMapFeatureAny) draw_data->current_set);

      zmapWindowContainerSetVisibility(FOO_CANVAS_GROUP(draw_data->container_feature_set), TRUE);

      //	zmapWindowDrawSetGroupBackground(draw_data->container_feature_set, 0, 1, 1.0, ZMAP_CANVAS_LAYER_COL_BACKGROUND,  NULL, NULL);
    }
#if 0
  // we'll get this pointless error if the set has not loaded yet
  else
    {
      GList *l;
      char *x = "";

      printf("Failed to find navigator featureset '%s'\n", g_quark_to_string(set_id));

      zMap_g_hash_table_get_keys(&l,draw_data->current_block->feature_sets);
      for(;l;l = l->next)
	{
	  char *y = (char *) g_quark_to_string(GPOINTER_TO_UINT(l->data));
	  x = g_strconcat(x," ",y,NULL);
	}
      printf("available are: %s\n",x);
    }
#endif

  return ;
}


static void clampCoords(ZMapWindowNavigator navigate,
                        double *c1_inout, double *c2_inout)
{
  double top, bot;
  double min, max;

  min = (double)(navigate->full_span.x1);
  max = (double)(navigate->full_span.x2);

  top = *c1_inout;
  bot = *c2_inout;

  zMapGUICoordsClampSpanWithLimits(min, max, &top, &bot);

  *c1_inout = top;
  *c2_inout = bot;

  return ;
}


static void clampScaled(ZMapWindowNavigator navigate, double *s1_inout, double *s2_inout)
{
  clampCoords(navigate, s1_inout, s2_inout);

  return ;
}

#if 0
static void clampWorld2Scaled(ZMapWindowNavigator navigate, double *w1_inout, double *w2_inout)
{
  clampCoords(navigate, w1_inout, w2_inout);
  return ;
}
#endif

static void updateLocatorDragger(ZMapWindowNavigator navigate, double button_y, double size)
{
  double a, b;

  if(!navigate->locator_drag)
    return;

  a = button_y;
  b = button_y + size;

  clampScaled(navigate, &a, &b);

  /* need to expose where we moved from */
  zMapWindowCanvasFeaturesetExpose((ZMapWindowFeaturesetItem) navigate->locator_drag);


  zMapWindowCanvasFeaturesetSetSequence((ZMapWindowFeaturesetItem) navigate->locator_drag, a, b);
  foo_canvas_item_request_update(navigate->locator_drag);

  // this fails to draw the top line if dragging upwards
  //  foo_canvas_item_request_redraw(navigate->locator_drag);

  // this works but is a bodge
  //  foo_canvas_item_request_redraw(navigate->canvas->root);

  /* need to expose where we moved to */
  zMapWindowCanvasFeaturesetExpose((ZMapWindowFeaturesetItem) navigate->locator_drag);


  return ;
}

static gboolean rootBGEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  ZMapWindowNavigator navigate = (ZMapWindowNavigator) data;
  gboolean event_handled = FALSE;
  double locator_y1, locator_y2;
  double drag_y1, drag_y2;
  double y_coord;
  double locator_size = navigate->locator_y_coords.x2 - navigate->locator_y_coords.x1;

  if(!navigate->locator || !navigate->locator_drag)
    return FALSE;

  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(navigate->locator), NULL, &locator_y1, NULL, &locator_y2);
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(navigate->locator_drag), NULL, &drag_y1, NULL, &drag_y2);

  switch(event->type)
    {
    case GDK_BUTTON_PRESS:
      {
        GdkEventButton *button = (GdkEventButton *)event;

        if(button->button  == 1)
          {
            gboolean within_fuzzy = FALSE;
            int fuzzy_dist = 6;

            if( (button->y > locator_y1) && (button->y < locator_y2) )
              within_fuzzy = TRUE;
            else if(fuzzy_dist > 1 && fuzzy_dist < 10)
              {
                if(locator_y2 - locator_y1 <= fuzzy_dist)
                  {
                    fuzzy_dist /= 2;
                    if(button->y < (locator_y1 + fuzzy_dist) && button->y > (locator_y1 - fuzzy_dist))
                      within_fuzzy = TRUE;
                  }
              }

            if(within_fuzzy == TRUE)
              {

                foo_canvas_item_show(navigate->locator_drag);

                navigate->click_correction = button->y - locator_y1;
		foo_canvas_c2w(navigate->canvas, 0, locator_y1, NULL, &y_coord);

                updateLocatorDragger(navigate, y_coord, locator_size);

		navigate->locator_click = event_handled = TRUE;
              }
          }
      }
      break;

    case GDK_MOTION_NOTIFY:
      {
        GdkEventButton *button = (GdkEventButton *)event;

        if(navigate->locator_click == TRUE)
          {
            y_coord = (double) button->y - navigate->click_correction;
	    foo_canvas_c2w(navigate->canvas, 0, y_coord, NULL, &y_coord);

            updateLocatorDragger(navigate, y_coord, locator_size);

            event_handled = TRUE;
          }
      }
      break;

    case GDK_BUTTON_RELEASE:
      {
        GdkEventButton *button = (GdkEventButton *)event;

        if(navigate->locator_click == TRUE)
          {
            foo_canvas_item_hide(navigate->locator_drag);

            if(button->y != (locator_y1 + navigate->click_correction))	// why bother testing?
	      {
		y_coord = (double) button->y - navigate->click_correction;
		foo_canvas_c2w(navigate->canvas, 0, y_coord, NULL, &y_coord);


#if 0
		this will be called by zMapWindowMove()
		  zMapWindowNavigatorDrawLocator(navigate, y_coord, y_coord + locator_size);
#endif

#if RUN_AROUND
		/*
		 * this follows an obscure callback meander that ends up calling zMapWindowMove()
		 * with a window parameter stored elsewhere
		 * we could just call zMapWindowMove(navigate->current_window) from here
		 * but instead jump into zmapNavigatorWidget.c which calls class_data->callbacks.valueCB()
		 * which equates to canvas_value_cb() in zmapControlNavigator.c
		 * which then calls a ZMapNavigator (NOTE: not the same as ZMapWindowNavigator) callback
		 * (set up by zMapNavigatorSetWindowCallback() in zmapControlWindowMakeFrame() in zmapControlWindowFrame.c)
		 * which then calls zMapWindowMove() with that mysterious window pointer
		 */
		zmapWindowNavigatorValueChanged(NAVIGATOR_WIDGET(navigate), y_coord, y_coord + locator_size);
#else
		zMapWindowMove(navigate->current_window, y_coord, y_coord + locator_size);
#endif
	      }

            navigate->locator_click = FALSE;

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

  feature_any = zmapWindowItemGetFeatureAny(item);

  menu_title  = zMapFeatureName(feature_any);

  if((menu_data = g_new0(NavigateMenuCBDataStruct, 1)))
    {
      ZMapWindowContainerFeatureSet container = NULL;
      ZMapStyleBumpMode bump_mode = ZMAPBUMP_UNBUMP;
      menu_data->item     = item;
      menu_data->navigate = (ZMapWindowNavigator)data;



      if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
        {
          menu_data->item_cb  = TRUE;
          /* This is the variant filter... Shouldn't be in the menu code too! */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          if (feature->feature.transcript.locus_id != 0)
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

            {
              menu_sets = g_list_append(menu_sets, zmapWindowNavigatorMakeMenuLocusOps(NULL, NULL, menu_data));
              menu_sets = g_list_append(menu_sets, zmapWindowNavigatorMakeMenuLocusColumnOps(NULL, NULL, menu_data));
              menu_sets = g_list_append(menu_sets, separator);
            }

	  container = (ZMapWindowContainerFeatureSet)zmapWindowContainerCanvasItemGetContainer(item);
        }
      else
        {
          /* get set_data->style */

	  if (ZMAP_IS_CONTAINER_FEATURESET(item))
	    {
	      container = ZMAP_CONTAINER_FEATURESET(item);

	      if(container->unique_id == menu_data->navigate->locus_id)
		{
		  menu_sets = g_list_append(menu_sets, zmapWindowNavigatorMakeMenuLocusColumnOps(NULL, NULL, menu_data));
		  menu_sets = g_list_append(menu_sets, separator);
		}
	    }

          menu_data->item_cb  = FALSE;
        }

      if (bumping_works && container)
        {
	  bump_mode = zmapWindowContainerFeatureSetGetBumpMode(container);

          menu_sets = g_list_append(menu_sets, zmapWindowNavigatorMakeMenuBump(NULL, NULL, menu_data, bump_mode));
          menu_sets = g_list_append(menu_sets, separator);
        }

      menu_sets = g_list_append(menu_sets, zmapWindowNavigatorMakeMenuColumnOps(NULL, NULL, menu_data));

    }

  zMapGUIMakeMenu(menu_title, menu_sets, button);

  return ;
}

#if 0
static gboolean columnBackgroundEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
#ifdef __DATA_UNUSED__
  ZMapWindowNavigator navigate = (ZMapWindowNavigator)data;
#endif /* __DATA_UNUSED__ */

  gboolean event_handled = FALSE;

  switch(event->type)
    {
    case GDK_BUTTON_PRESS:
      {
        GdkEventButton *button = (GdkEventButton *)event;

        if(button->button == 3)
          {
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
#endif

static gboolean navCanvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE;
  ZMapWindowNavigator navigate = (ZMapWindowNavigator)data;
  ZMapFeature feature = NULL;
  static guint32 last_but_press = 0 ;			    /* Used for double clicks... */

  switch(event->type)
    {
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
      {
	GdkEventButton *button = (GdkEventButton *)event ;

	/* freeze the selected feature from the point function */
	zMapWindowCanvasItemSetFeature((ZMapWindowCanvasItem) item, button->x, button->y);

	/* Retrieve the feature item info from the canvas item. */
        feature = zmapWindowItemGetFeature(item);
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
		    event_handled = TRUE ;
                  }
              }
            last_but_press = button->time ;
          }
        else
          {
            if (button->button == 1  /* && feature->feature.transcript.locus_id != 0) */
		&& feature->type == ZMAPSTYLE_MODE_TEXT)
              {
		/*used to get a transcript feature? no idea how, but the locus feature is a text item */
                zmapWindowNavigatorGoToLocusExtents(navigate, item);

		foo_canvas_item_hide(navigate->locator_drag);	/* stupid GTK give a press and 2-press, so i'm depressed */

		navigate->locator_click = FALSE;	/* can get mixed signals ??, end up moving somewhere else ?? */
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

  return FALSE;
}

static gboolean factoryItemHandler(FooCanvasItem       *new_item,
				   ZMapWindowFeatureStack     feature_stack,
				   gpointer             handler_data)
{
  g_signal_connect(GTK_OBJECT(new_item), "event",
                   GTK_SIGNAL_FUNC(navCanvasItemEventCB), handler_data);
  g_signal_connect(GTK_OBJECT(new_item), "destroy",
                   GTK_SIGNAL_FUNC(navCanvasItemDestroyCB), handler_data);

  zMapWindowCanvasItemSetConnected((ZMapWindowCanvasItem) new_item, TRUE);

  return TRUE;
}





static gboolean variantFeature(ZMapFeature feature, ZMapWindowNavigator navigate)
{
  gboolean outside = FALSE;

  if(feature && navigate->locus_id == feature->parent->unique_id)
    {
      if(zmapWindowNavigatorLDHFind(navigate->locus_display_hash,
				    feature->original_id))
        {
          /* we only ever draw the first one of these. */
          outside = TRUE;
        }
      else if(zmapWindowNavigatorLDHInsert(navigate->locus_display_hash, feature))
        {
          outside = FALSE;
        }
      else
        {
          zMapAssertNotReached();
        }
    }


  return outside;
}








/* mini package for the locus_display_hash ... */
/*
 * \brief Create a hash for the locus display.
 *
 * Hash is keyed on GUINT_TO_POINTER(feature->original_id)
 * Contains LocusEntry items.
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
                                               ZMapFeature feature)
{
  LocusEntry hash_entry = NULL;

  if((hash_entry = g_new0(LocusEntryStruct, 1)))
    {
      hash_entry->start   = feature->x1;		/* these are never used */
      hash_entry->end     = feature->x2;
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



void navigatorRunSet(  ZMapFeatureSet set,
		       FooCanvasGroup *container,
		       ZMapFrame frame,
		       ZMapWindowNavigator navigate)
{
  ZMapWindowFeatureStackStruct feature_stack = { NULL };
  GList *l;
  ZMapWindowContainerFeatures features = zmapWindowContainerGetFeatures((ZMapWindowContainerGroup) container) ;
  ZMapWindowFeaturesetItem locus_featureset = NULL;

  feature_stack.feature = NULL;
  zmapGetFeatureStack(&feature_stack,set,NULL,frame);

  zMap_g_hash_table_get_data(&l, set->features);

  for(; l ; l = l->next)
    {
      ZMapFeature feature = (ZMapFeature)l->data;
      FooCanvasItem *foo = NULL;

      /* filter on frame! */
      if((frame != ZMAPFRAME_NONE) && frame  != zmapWindowFeatureFrame(feature))
	continue;

      feature_stack.feature = feature;

      /* this stuff is just not used in the navigator..... esp given that container is a given */
      if(*(feature->style))	/* chicken */
	{
	  if(zMapStyleIsStrandSpecific(*feature->style))
	    feature_stack.strand = zmapWindowFeatureStrand(NULL,feature);
	  if(zMapStyleIsFrameSpecific(*feature->style))
	    feature_stack.frame = zmapWindowFeatureFrame(feature);
	}

      //	if(zMapStyleGetMode(*feature->style) == ZMAPSTYLE_MODE_TEXT)	variant feature handles this
      {
	if(variantFeature(feature, navigate))
	  continue;
      }

      foo = zmapWindowFToIFactoryRunSingle(navigate->ftoi_hash, (ZMapWindowContainerFeatureSet) container, features, foo, &feature_stack);

      if(!zMapWindowCanvasItemIsConnected((ZMapWindowCanvasItem) foo))
	factoryItemHandler (foo, &feature_stack, (gpointer) navigate);

      if(zMapStyleGetMode(*feature->style) == ZMAPSTYLE_MODE_TEXT && !locus_featureset && foo)
	{
	  locus_featureset = (ZMapWindowFeaturesetItem) foo;
	  zMapWindowCanvasLocusSetFilter(locus_featureset, navigate->hide_filter);

	  navigate->locus_featureset = locus_featureset;
	}

    }

  return ;
}





