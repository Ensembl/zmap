/*  File: zmapWindowFeaturesetItem.c
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
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
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jun  3 09:51 2009 (rds)
 * Created: Fri Jan 16 11:20:07 2009 (rds)
 *-------------------------------------------------------------------
 */

/* NOTE
 * this module implements the basic handling of featuresets as foo canvas items
 *
 * for basic features (the first type implemented) it runs about 3-5x faster than the foo canvas
 * but note that that does not include expose/ GDK
 * previous code can be used with 'foo=true' in the style

 _new code_
 deskpro17848[mh17]32: zmap --conf_file=ZMap_bins
 # /nfs/users/nfs_m/mh17/.ZMap/ZMap_bins	27/09/2011
 Draw featureset basic_1000: 999 features in 0.060 seconds
 Draw featureset scored: 9999 features in 0.347 seconds
 Draw featureset basic_10000: 9999 features in 0.324 seconds
 Draw featureset basic_100000: 99985 features in 1.729 seconds

 _old code_
 deskpro17848[mh17]33: zmap --conf_file=ZMap_bins
 # /nfs/users/nfs_m/mh17/.ZMap/ZMap_bins	27/09/2011
 Draw featureset basic_1000: 999 features in 0.165 seconds
 Draw featureset scored: 9999 features in 0.894 seconds
 Draw featureset basic_10000: 9999 features in 1.499 seconds
 Draw featureset basic_100000: 99985 features in 8.968 seconds


*/

#include <ZMap/zmap.h>



#include <math.h>
#include <string.h>
#include <ZMap/zmapUtilsLog.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapUtilsLog.h>
#include <ZMap/zmapWindow.h>

#include <zmapWindowCanvasItem.h>
#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasBasic.h>
#include <zmapWindowCanvasGlyph.h>
#include <zmapWindowCanvasAlignment.h>
#include <zmapWindowCanvasGraphItem.h>
#include <zmapWindowCanvasTranscript.h>
#include <zmapWindowCanvasAssembly.h>
#include <zmapWindowCanvasSequence.h>
#include <zmapWindowCanvasLocus.h>

#include <zmapWindowCanvasGraphics.h>


typedef gint (FeatureCmpFunc)(gconstpointer a, gconstpointer b) ;

static void zmap_window_featureset_item_item_class_init  (ZMapWindowFeaturesetItemClass featureset_class);
static void zmap_window_featureset_item_item_init        (ZMapWindowFeaturesetItem      item);

static void  zmap_window_featureset_item_item_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags);
static void  zmap_window_featureset_item_item_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags);
#define USE_FOO_POINT	1
static double  zmap_window_featureset_item_foo_point (FooCanvasItem *item, double x, double y, int cx, int cy, FooCanvasItem **actual_item);
#if !USE_FOO_POINT
static double  zmap_window_featureset_item_point (FooCanvasItem *item, double cx, double cy);
#endif
static void  zmap_window_featureset_item_item_bounds (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2);
static void  zmap_window_featureset_item_item_draw (FooCanvasItem *item, GdkDrawable *drawable, GdkEventExpose *expose);


static gboolean zmap_window_featureset_item_set_style(FooCanvasItem *item, ZMapFeatureTypeStyle style);
static void zmap_window_featureset_item_set_colour(ZMapWindowCanvasItem   thing,
						   FooCanvasItem         *interval,
						   ZMapFeature			feature,
						   ZMapFeatureSubPartSpan sub_feature,
						   ZMapStyleColourType    colour_type,
						   int colour_flags,
						   GdkColor              *default_fill,
						   GdkColor              *border);

static gboolean zmap_window_featureset_item_set_feature(FooCanvasItem *item, double x, double y);

static gboolean zmap_window_featureset_item_show_hide(FooCanvasItem *item, gboolean show);

static void zmap_window_featureset_item_item_destroy     (GObject *object);
static void zMapWindowCanvasFeaturesetPaintSet(ZMapWindowFeaturesetItem featureset,
					       GdkDrawable *drawable, GdkEventExpose *expose) ;

static double featurePoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
			   double item_x, double item_y, int cx, int cy,
			   double local_x, double local_y, double x_off) ;
static double graphicsPoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
			    double item_x, double item_y, int cx, int cy,
			    double local_x, double local_y, double x_off) ;

gint zMapFeatureNameCmp(gconstpointer a, gconstpointer b);
gint zMapFeatureFullCmp(gconstpointer a, gconstpointer b);
gint zMapFeatureCmp(gconstpointer a, gconstpointer b);


static FooCanvasItemClass *item_class_G = NULL;
static ZMapWindowFeaturesetItemClass featureset_class_G = NULL;

static void zmapWindowCanvasFeaturesetSetColours(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature);

static ZMapSkipList zmap_window_canvas_featureset_find_feature_index(ZMapWindowFeaturesetItem fi,ZMapFeature feature);
static ZMapWindowCanvasFeature zmap_window_canvas_featureset_find_feature(ZMapWindowFeaturesetItem fi,
									  ZMapFeature feature);
static ZMapSkipList zmap_window_canvas_featureset_find_feature_coords(FeatureCmpFunc compare_func,
								      ZMapWindowFeaturesetItem fi,
								      double y1, double y2) ;
void zmap_window_canvas_featureset_expose_feature(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs);







/* this is in parallel with the ZMapStyleMode enum */
/* beware, traditionally glyphs ended up as basic features */
/* This is a retro-fit ... i noticed that i'd defined a struct/feature type but not set it as orignally there was only one  */
static zmapWindowCanvasFeatureType feature_types[N_STYLE_MODE + 1] =
  {
    FEATURE_INVALID,		/* ZMAPSTYLE_MODE_INVALID */

	FEATURE_BASIC, 		/* ZMAPSTYLE_MODE_BASIC */
	FEATURE_ALIGN,		/* ZMAPSTYLE_MODE_ALIGNMENT */
	FEATURE_TRANSCRIPT,	/* ZMAPSTYLE_MODE_TRANSCRIPT */
	FEATURE_SEQUENCE,		/* ZMAPSTYLE_MODE_SEQUENCE */
	FEATURE_ASSEMBLY,		/* ZMAPSTYLE_MODE_ASSEMBLY_PATH */
	FEATURE_LOCUS,		/* ZMAPSTYLE_MODE_TEXT */
	FEATURE_GRAPH,		/* ZMAPSTYLE_MODE_GRAPH */
	FEATURE_GLYPH,		/* ZMAPSTYLE_MODE_GLYPH */

	FEATURE_GRAPHICS,		/* ZMAPSTYLE_MODE_PLAIN */	/* plain graphics, no features eg scale bar */

	FEATURE_INVALID		/* ZMAPSTYLE_MODE_META */
};

#if N_STYLE_MODE != FEATURE_N_TYPE
#error N_STYLE_MODE and FEATURE_N_TYPE differ
#endif


/* Tables of function pointers for individual feature types, only required ones have to be provided. */
static gpointer _featureset_set_init_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_prepare_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_set_paint_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_paint_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_flush_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_extent_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_free_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_zoom_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_point_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_add_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_subpart_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_colour_G[FEATURE_N_TYPE] = { 0 };




/* clip to expose region */
/* erm,,, clip to visible scroll region: else rectangles would get extra edges */
void zMap_draw_line(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset, gint cx1, gint cy1, gint cx2, gint cy2)
{
	/* for H or V lines we can clip easily */

	if(cy1 > featureset->clip_y2)
		return;
	if(cy2 < featureset->clip_y1)
		return;
#if 0
/*
	the problem is long vertical lines that wrap round
	we don't draw big horizontal ones
    */
	if(cx1 > featureset->clip_x2)
	  return;
	if(cx2 < featureset->clip_x1)
	  {
	    /* this will return if we expose part of a line that runs TR to BL
	     * we'd have to swap x1 and x2 round for the comparison to work
	     */
	    return;
	  }
#endif
	if(cx1 == cx2)	/* is vertical */
	  {
#if 0
	    /*
	      the problem is long vertical lines that wrap round
	      we don't draw big horizontal ones
	    */
	    if(cx1 < featureset->clip_x1)
	      cx1 = featureset->clip_x1;
	    if(cx2 > featureset->clip_x2)
	      cx2 = featureset->clip_x2;
#endif
	    if(cy1 < featureset->clip_y1)
	      cy1 = featureset->clip_y1;
	    if(cy2 > featureset->clip_y2)
	      cy2 = featureset->clip_y2 ;
	  }
	else
	  {
	    double dx = cx2 - cx1;
	    double dy = cy2 - cy1;

	    if(cy1 < featureset->clip_y1)
	      {
		/* need to round to get partial lines joining up neatly */
		cx1 += round(dx * (featureset->clip_y1 - cy1) / dy);
		cy1 = featureset->clip_y1;
	      }
	    if(cy2 > featureset->clip_y2)
	      {
		cx2 -= round(dx * (cy2 - featureset->clip_y2) / dy);
		cy2 = featureset->clip_y2;
	      }

	  }
	gdk_draw_line (drawable, featureset->gc, cx1, cy1, cx2, cy2);
}




/* clip to expose region */
/* erm,,, clip to visble scroll regipn: else rectangles would get extra edges */
/* NOTE rectangles may be drawn partially if they overlap the visible region
 * in which case a false outline will be draw outside the region
 */
void zMap_draw_rect(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset, gint cx1, gint cy1, gint cx2, gint cy2, gboolean fill)
{
  /* as our rectangles are all aligned to H and V we can clip easily */

  if(cy1 > featureset->clip_y2)
    return;
  if(cy2 < featureset->clip_y1)
    return;

  if(cx1 > featureset->clip_x2)
    return;
  if(cx2 < featureset->clip_x1)
    return;

  if(cx1 < featureset->clip_x1)
    cx1 = featureset->clip_x1;
  if(cy1 < featureset->clip_y1)
    cy1 = featureset->clip_y1;
  if(cx2 > featureset->clip_x2)
    cx2 = featureset->clip_x2;
  if(cy2 > featureset->clip_y2)
    cy2 = featureset->clip_y2;

  /* NOTE that the gdk_draw_rectangle interface is a bit esoteric
   * and it doesn't like rectangles that have no depth
   * read the docs to understand the coordinate calculations here
   */

  if(cy2 == cy1)
    {
      gdk_draw_line (drawable, featureset->gc, cx1, cy1, cx2, cy2);
    }
  else
    {
      if(fill)
	{
	  cx2++;
	  cy2++;
	}
      gdk_draw_rectangle (drawable, featureset->gc, fill, cx1, cy1, cx2 - cx1, cy2 - cy1);
    }


}




/* get all the pango stuff we need for a font on a drawable */
void zmapWindowCanvasFeaturesetInitPango(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasPango pango, char *family, int size, GdkColor *draw)
{
  GdkScreen *screen = gdk_drawable_get_screen (drawable);
  PangoFontDescription *desc;
  int width, height;

  if(pango->drawable && pango->drawable != drawable)
    zmapWindowCanvasFeaturesetFreePango(pango);

  if(!pango->renderer)
    {
      pango->renderer = gdk_pango_renderer_new (screen);
      pango->drawable = drawable;

      gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (pango->renderer), drawable);

      zMapAssert(featureset->gc);	/* should have been set by caller */
      gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (pango->renderer), featureset->gc);

      /* Create a PangoLayout, set the font and text */
      pango->context = gdk_pango_context_get_for_screen (screen);
      pango->layout = pango_layout_new (pango->context);

      //		font = findFixedWidthFontFamily(seq->pango.context);
      desc = pango_font_description_from_string (family);
#warning mod this function and call from both places
#if 0
      /* this must be identical to the one get by the ZoomControl */
      if(zMapGUIGetFixedWidthFont(view,
				  fixed_font_list, ZMAP_ZOOM_FONT_SIZE, PANGO_WEIGHT_NORMAL,
				  NULL,desc))
	{
	}
      else
	{
	  /* if this fails then we get a proportional font and someone will notice */
	  zmapLogWarning("Paint sequence cannot get fixed font","");
	}
#endif

      pango_font_description_set_size (desc,size * PANGO_SCALE);
      pango_layout_set_font_description (pango->layout, desc);
      pango_font_description_free (desc);

      pango_layout_set_text (pango->layout, "a", 1);		/* we need to get the size of one character */
      pango_layout_get_size (pango->layout, &width, &height);
      pango->text_height = height / PANGO_SCALE;
      pango->text_width = width / PANGO_SCALE;

      gdk_pango_renderer_set_override_color (GDK_PANGO_RENDERER (pango->renderer), PANGO_RENDER_PART_FOREGROUND, draw );
    }
}


void zmapWindowCanvasFeaturesetFreePango(ZMapWindowCanvasPango pango)
{
  if(pango->renderer)		/* free the pango renderer if allocated */
    {
      /* Clean up renderer, possiby this is not necessary */
      gdk_pango_renderer_set_override_color (GDK_PANGO_RENDERER (pango->renderer),
					     PANGO_RENDER_PART_FOREGROUND, NULL);
      gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (pango->renderer), NULL);
      gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (pango->renderer), NULL);

      /* free other objects we created */
      if(pango->layout)
	{
	  g_object_unref(pango->layout);
	  pango->layout = NULL;
	}

      if(pango->context)
	{
	  g_object_unref (pango->context);
	  pango->context = NULL;
	}

      g_object_unref(pango->renderer);
      pango->renderer = NULL;
      //		pango->drawable = NULL;
    }
}



/* define feature specific functions here */
/* only access via wrapper functions to allow type checking */

/* handle upstream edge effects converse of flush */
/* feature may be NULL to signify start of data */
void zMapWindowCanvasFeaturesetPaintPrepare(ZMapWindowFeaturesetItem featureset,
					    ZMapWindowCanvasFeature feature,
					    GdkDrawable *drawable, GdkEventExpose *expose)
{
  void (*func) (ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
		GdkDrawable *drawable, GdkEventExpose *expose) = NULL;

  if ((featureset->type > 0 && featureset->type < FEATURE_N_TYPE)
      && (func = _featureset_prepare_G[featureset->type]))
    func(featureset, feature, drawable, expose);

  return ;
}



/* paint one feature, all context needed is in the FeaturesetItem */
/* we need the expose region to clip at high zoom esp with peptide alignments */
void zMapWindowCanvasFeaturesetPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable, GdkEventExpose *expose)
{
  void (*func) (ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable, GdkEventExpose *expose) = NULL;


  /* NOTE we can have diff types of features in a column eg alignments and basic features in Repeats
   * if we use featureset->style then we get to call the wrong paint fucntion and crash
   * NOTE that if we use PaintPrepare and PaintFlush we require one type of feature only in the column
   * (more complex behavious has not been programmed; alignemnts and basic features don't use this)
   */
  if ((feature->type > 0 && feature->type < FEATURE_N_TYPE)
      && (func = _featureset_paint_G[feature->type]))
    func(featureset, feature, drawable, expose) ;

  return ;
}


/* output any buffered paints: useful eg for poly-line */
/* paint function and flush must access data via FeaturesetItem or globally in thier module */
/* feaature is the last feature painted */
void zMapWindowCanvasFeaturesetPaintFlush(ZMapWindowFeaturesetItem featureset,ZMapWindowCanvasFeature feature, GdkDrawable *drawable, GdkEventExpose *expose)
{
  void (*func) (ZMapWindowFeaturesetItem featureset,ZMapWindowCanvasFeature feature, GdkDrawable *drawable, GdkEventExpose *expose) = NULL;

  /* NOTE each feature type has it's own buffer if implemented
   * but  we expect only one type of feature and to handle mull features
   * (which we do to join up lines in gaps betweeen features)
   * we need to use the featureset type instead.
   * we could code this as featyreset iff feature is null
   * x-ref with PaintPrepare above
   */

#if 0
  if(feature &&
     (feature->type > 0 && feature->type < FEATURE_N_TYPE)
     && (func = _featureset_flush_G[feature->type]))
#else
  if ((featureset->type > 0 && featureset->type < FEATURE_N_TYPE)
      && (func = _featureset_flush_G[featureset->type]))
#endif
    func(featureset, feature, drawable, expose);

  return;
}


/* get the total sequence extent of a simple or complex feature */
/* used by bumping; we allow force to simple for optional but silly bump modes */
gboolean zMapWindowCanvasFeaturesetGetFeatureExtent(ZMapWindowCanvasFeature feature, gboolean complex, ZMapSpan span, double *width)
{
  void (*func) (ZMapWindowCanvasFeature feature, ZMapSpan span, double *width) = NULL;

  if(!feature || feature->type < 0 || feature->type >= FEATURE_N_TYPE)
    return FALSE;

  func = _featureset_extent_G[feature->type];

  if(!complex)		/* reset any compund feature extents */
    feature->y2 = feature->feature->x2;

  if(!func || !complex)
    {
      /* all features have an extent, if it's simple get it here */
      span->x1 = feature->feature->x1;	/* use real coord from the feature context */
      span->x2 = feature->feature->x2;
      *width = feature->width;
    }
  else
    {
      func(feature, span, width);
    }
  return TRUE;
}






/* interface design driven by exsiting (ZMapCanvasItem/Foo) application code
 *
 * for normal faatures we only set the colour flags
 * for sequence  features we actually supply colours
 */
void zmapWindowFeaturesetItemSetColour(FooCanvasItem         *interval,
				       ZMapFeature			feature,
				       ZMapFeatureSubPartSpan sub_feature,
				       ZMapStyleColourType    colour_type,
				       int colour_flags,
				       GdkColor              *default_fill,
				       GdkColor              *default_border)
{
  ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) interval;
  ZMapWindowCanvasFeature gs;

  void (*func) (FooCanvasItem         *interval,
		ZMapFeature			feature,
		ZMapFeatureSubPartSpan sub_feature,
		ZMapStyleColourType    colour_type,
		int colour_flags,
		GdkColor              *default_fill,
		GdkColor              *default_border);

  func = _featureset_colour_G[fi->type];
  gs = zmap_window_canvas_featureset_find_feature(fi,feature);
  if(!gs)
    return;

  if(func)
    {
      zmapWindowCanvasFeatureStruct dummy;

      func(interval, feature, sub_feature, colour_type, colour_flags, default_fill, default_border);

      dummy.width = gs->width;
      dummy.y1 = gs->y1;
      dummy.y2 = gs->y2;
      if(sub_feature && sub_feature->subpart != ZMAPFEATURE_SUBPART_INVALID)
	{
	  dummy.y1 = sub_feature->start;
	  dummy.y2 = sub_feature->end;
	}
      dummy.bump_offset = gs->bump_offset;

      zmap_window_canvas_featureset_expose_feature(fi, &dummy);
    }
  else
    {
      if(fi->highlight_sideways)	/* ie transcripts as composite features */
	{
	  while(gs->left)
	    gs = gs->left;
	}

      while(gs)
	{
	  /* set the focus flags (on or off) */
	  /* zmapWindowFocus.c maintans these and we set/clear then all at once */
	  /* NOTE some way up the call stack in zmapWindowFocus.c add_unique()
	   * colour flags has a window id set into it
	   */

	  /* these flags are very fiddly: handle with care */
	  gs->flags = ((gs->flags & ~FEATURE_FOCUS_MASK) | (colour_flags & FEATURE_FOCUS_MASK)) |
	    ((gs->flags & FEATURE_FOCUS_ID) | (colour_flags & FEATURE_FOCUS_ID)) |
	    (gs->flags & FEATURE_FOCUS_BLURRED);

	  zmap_window_canvas_featureset_expose_feature(fi, gs);

	  if(!fi->highlight_sideways)		/* only doing the selected one */
	    return;

	  gs = gs->right;
	}
    }
}




int zMapWindowCanvasFeaturesetAddFeature(ZMapWindowFeaturesetItem featureset,
					 ZMapFeature feature, double y1, double y2)
{
  int rc = 0 ;
  ZMapWindowCanvasFeature (*func)(ZMapWindowFeaturesetItem featureset, ZMapFeature feature, double y1, double y2) ;
  ZMapWindowCanvasFeature feat ;

  if (feature)
    {
      if ((func = _featureset_add_G[featureset->type]))
	{
	  feat = func(featureset, feature, y1, y2) ;
	}
      else
	{
	  if ((feat = zMapWindowFeaturesetAddFeature(featureset, feature, y1, y2)))
	    zMapWindowFeaturesetSetFeatureWidth(featureset, feat) ;
	}

      if (feat)
	{
	  featureset->last_added = feat ;
	  rc = 1 ;
	}
    }

  return rc ;
}


/* if a featureset has any allocated data free it */
void zMapWindowCanvasFeaturesetFree(ZMapWindowFeaturesetItem featureset)
{
  ZMapWindowFeatureFreeFunc func ;

  if ((featureset->type > 0 && featureset->type < FEATURE_N_TYPE)
      && (func = _featureset_free_G[featureset->type]))
    func(featureset) ;

  return;
}

/* return a span struct for the feature */
ZMapFeatureSubPartSpan zMapWindowCanvasFeaturesetGetSubPartSpan(FooCanvasItem *foo, ZMapFeature feature, double x,double y)
{
  ZMapFeatureSubPartSpan (*func) (FooCanvasItem *foo,ZMapFeature feature,double x,double y) = NULL;
  //	ZMapWindowFeaturesetItem featureset = (ZMapWindowFeaturesetItem) foo;


  if(feature->type > 0 && feature->type < FEATURE_N_TYPE)
    func = _featureset_subpart_G[feature->type];
  if(!func)
    return NULL;

  return func(foo, feature, x, y);
}



/*
 * do anything that needs doing on zoom
 * we have column specific things like are features visible
 * and feature specific things like recalculate gapped alignment display
 * NOTE this is also called on the first paint to create the index
 * it must create the index if it's not there and this may be done by class Zoom functions
 */
void zMapWindowCanvasFeaturesetZoom(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable)
{
  ZMapSkipList sl;
  long trigger = (long) zMapStyleGetSummarise(featureset->style);
  PixRect pix = NULL;

  if (featureset)
    {
      ZMapWindowFeatureItemZoomFunc func ;

      if ((featureset->type > 0 && featureset->type < FEATURE_N_TYPE)
	  && (func = _featureset_zoom_G[featureset->type]))
	{
	  /* zoom can (re)create the index eg if graphs density stuff gets re-binned */
	  func(featureset, drawable) ;
	}

      if(!featureset->display_index)
	zMapWindowCanvasFeaturesetIndex(featureset);


      /* column summarise: after creating the index work out which features are visible and hide the rest */
      /* set for basic features and alignments */
      /* this could be moved to the class functions but as it's used more generally not worth it */

      if(!featureset->bumped && !featureset->re_bin && trigger && featureset->n_features >= trigger)
	{
	  /*
	    on min zoom we have nominally 1000 pixels so if we have 1000 features we should get some overlap
	    depends on the sequence length, longer sequence means more overlap (and likely more features
	    but simple no of features is easy to work with.
	    if we used 100 features out of sheer exuberance
	    then the chance of wasted CPU is small as the data is small
	    at high zoom we still do it as overlap is still overlap (eg w/ high coverage BAM regions)
	  */

	  /* NOTE: for faster code just process features overlapping the visible scroll region */
	  /* however on current volumes (< 200k normally) it makes little difference */
	  for(sl = zMapSkipListFirst(featureset->display_index); sl; sl = sl->next)
	    {
	      ZMapWindowCanvasFeature feature = (ZMapWindowCanvasFeature) sl->data;

	      pix = zmapWindowCanvasFeaturesetSummarise(pix,featureset,feature);
	    }

	  /* clear up */
	  zmapWindowCanvasFeaturesetSummariseFree(featureset, pix);
	}
    }

  return;
}



/* paint set-level features, e.g. graph base lines etc. */
static void zMapWindowCanvasFeaturesetPaintSet(ZMapWindowFeaturesetItem featureset,
					       GdkDrawable *drawable, GdkEventExpose *expose)
{
  ZMapWindowFeatureItemSetPaintFunc func ;

  if ((featureset->type > 0 && featureset->type < FEATURE_N_TYPE)
      &&(func = _featureset_set_paint_G[featureset->type]))
    func(featureset, drawable, expose) ;

  return ;
}





/* each feature type defines its own functions */
/* if they inherit from another type then they must include that type's headers and call code directly */

void zMapWindowCanvasFeatureSetSetFuncs(int featuretype, gpointer *funcs, int feature_struct_size, int set_struct_size)
{

  _featureset_set_init_G[featuretype] = funcs[FUNC_SET_INIT];
  _featureset_prepare_G[featuretype] = funcs[FUNC_PREPARE];
  _featureset_set_paint_G[featuretype] = funcs[FUNC_SET_PAINT];
  _featureset_paint_G[featuretype] = funcs[FUNC_PAINT];
  _featureset_flush_G[featuretype] = funcs[FUNC_FLUSH];
  _featureset_extent_G[featuretype] = funcs[FUNC_EXTENT];
  _featureset_colour_G[featuretype]  = funcs[FUNC_COLOUR];
  _featureset_zoom_G[featuretype] = funcs[FUNC_ZOOM];
  _featureset_free_G[featuretype] = funcs[FUNC_FREE];
  _featureset_point_G[featuretype] = funcs[FUNC_POINT];
  _featureset_add_G[featuretype]     = funcs[FUNC_ADD];
  _featureset_subpart_G[featuretype] = funcs[FUNC_SUBPART];


  featureset_class_G->struct_size[featuretype] = feature_struct_size;
  featureset_class_G->set_struct_size[featuretype] = set_struct_size;

  return ;
}





/* fetch all the funcs we know about
 * there's no clean way to do this without everlasting overhead per feature
 * NB: canvas featuresets could get more than one type of feature
 * we know how many there are due to the zmapWindowCanvasFeatureType enum
 * so we just do them all, once, from the featureset class init
 * rather tediously that means we have to include headers for each type
 * In terms of OO 'classiness' we regard CanvasFeatureSet as the big daddy that has a few dependant children
 * they dont get to evolve unknown to thier parent.
 *
 * NOTE these functions could be called from the application, before starting the window/ camvas, which would allow extendablilty
 */
void featureset_init_funcs(void)
{
  /* set size of unspecifed features structs to just the base */
  featureset_class_G->struct_size[FEATURE_INVALID] = sizeof(zmapWindowCanvasFeatureStruct);

  zMapWindowCanvasBasicInit();		/* the order of these may be important */
  zMapWindowCanvasGlyphInit();
  zMapWindowCanvasAlignmentInit();
  zMapWindowCanvasGraphInit();
  zMapWindowCanvasTranscriptInit();
  zMapWindowCanvasAssemblyInit();
  zMapWindowCanvasSequenceInit();
  zMapWindowCanvasLocusInit();
  zMapWindowCanvasGraphicsInit();

  /* if you add a new one then update feature_types[N_STYLE_MODE] above */

  /* if not then graphics modules have to set thier size */
  /* this wastes a bit of memory but uses only one free list and may be safer */
  zMapAssert( sizeof(zmapWindowCanvasGraphicsStruct) <= sizeof(zmapWindowCanvasFeatureStruct));

  return ;
}



/* Converts a sequence extent into a canvas extent.
 *
 *
 * sequence coords:           1  2  3  4  5  6  7  8
 *
 *                           |__|__|__|__|__|__|__|__|
 *
 *                           |                       |
 * canvas coords:           1.0                     9.0
 *
 * i.e. when we actually come to draw it we need to go one _past_ the sequence end
 * coord because our drawing needs to draw in the whole of the last base.
 *
 */
void zmapWindowFeaturesetS2Ccoords(double *start_inout, double *end_inout)
{
  zMapAssert(start_inout && end_inout && *start_inout <= *end_inout) ;

  *end_inout += 1 ;

  return ;
}



GType zMapWindowFeaturesetItemGetType(void)
{
  static GType group_type = 0 ;

  if (!group_type)
    {
      static const GTypeInfo group_info = {
	sizeof (zmapWindowFeaturesetItemClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) zmap_window_featureset_item_item_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (zmapWindowFeaturesetItem),
	0,              /* n_preallocs */
	(GInstanceInitFunc) zmap_window_featureset_item_item_init,
	NULL
      };

      group_type = g_type_register_static(zMapWindowCanvasItemGetType(),
					  ZMAP_WINDOW_FEATURESET_ITEM_NAME,
					  &group_info,
					  0) ;
    }

  return group_type;
}


/*
 * return a singleton column wide canvas item
 * just in case we wanted to overlay two or more line graphs we need to allow for more than one
 * graph per column, so we specify these by col_id (which includes strand) and featureset_id
 * we allow for stranded bins, which get fed in via the add function below
 * we also have to include the window to allow more than one -> not accessable so we use the canvas instead
 *
 * we also allow several featuresets to map into the same canvas item via a virtual featureset (quark)
 */
ZMapWindowCanvasItem zMapWindowCanvasItemFeaturesetGetFeaturesetItem(FooCanvasGroup *parent, GQuark id, int start,int end, ZMapFeatureTypeStyle style, ZMapStrand strand, ZMapFrame frame, int index)
{
  ZMapWindowFeaturesetItem featureset = NULL;
  zmapWindowCanvasFeatureType type;
  int stagger;
  ZMapWindowFeatureItemSetInitFunc func ;
  FooCanvasItem *foo  = NULL;

  /* class not intialised till we make an item in foo_canvas_item_new() below */
  if(featureset_class_G && featureset_class_G->featureset_items)
    foo = (FooCanvasItem *) g_hash_table_lookup( featureset_class_G->featureset_items, GUINT_TO_POINTER(id));

  if(foo)
    {
      return((ZMapWindowCanvasItem) foo);
    }
  else
    {
      foo = foo_canvas_item_new(parent, ZMAP_TYPE_WINDOW_FEATURESET_ITEM,
				//				 "x", 0.0,
				//				 "y", (double) start,
				NULL);

      featureset = (ZMapWindowFeaturesetItem) foo ;
      featureset->id = id;
      g_hash_table_insert(featureset_class_G->featureset_items,GUINT_TO_POINTER(id),(gpointer) foo);

      /* we record strand and frame for display colours
       * the features get added to the appropriate column depending on strand, frame
       * so we get the right featureset item foo item implicitly
       */
      featureset->strand = strand;
      featureset->frame = frame;
      featureset->style = style;

      featureset->overlap = TRUE;

      /* column type, style is a combination of all context featureset styles in the column */
      /* main use is for graph density items */
      featureset->type = type = feature_types[zMapStyleGetMode(featureset->style)];

      if(featureset_class_G->set_struct_size[type])
	featureset->opt = g_malloc0(featureset_class_G->set_struct_size[type]);

      /* Maybe these should be subsumed into the feature_set_init_G mechanism..... */
      /* mh17: via the set_init_G fucntion, already moved code from here for link_sideways */
      if(type == FEATURE_ALIGN)
	{
	  featureset->link_sideways = TRUE;
	}
      else if(type == FEATURE_TRANSCRIPT)
	{
	  featureset->highlight_sideways = TRUE;
	}

      else if(type == FEATURE_GRAPH && zMapStyleDensity(style))
	{
	  featureset->overlap = FALSE;
	  featureset->re_bin = TRUE;

	  /* this was originally invented for heatmaps & it would be better as generic, but that's another job */
	  stagger = zMapStyleDensityStagger(style);
	  featureset->set_index = index;
	  featureset->x_off = stagger * featureset->set_index;
	}

      featureset->width = zMapStyleGetWidth(featureset->style);

      /* width is in characters, need to get the sequence code to adjust this */
      if(zMapStyleGetMode(featureset->style) == ZMAPSTYLE_MODE_SEQUENCE)
	featureset->width *= 10;
      //  if(zMapStyleGetMode(featureset->style) == ZMAPSTYLE_MODE_TEXT)
      //    featureset->width *= 10;

      featureset->start = start;
      featureset->end = end;

      /* initialise zoom to prevent double index create on first draw (coverage graphs) */
      featureset->zoom = foo->canvas->pixels_per_unit_y;
      featureset->bases_per_pixel = 1.0 / featureset->zoom;

      /* feature type specific code. */
      if ((featureset->type > 0 && featureset->type < FEATURE_N_TYPE)
	  && (func = _featureset_set_init_G[featureset->type]))
	func(featureset) ;


      /* set our bounding box in canvas coordinates to be the whole column */
      foo_canvas_item_request_update (foo);
    }

  return ((ZMapWindowCanvasItem) foo);
}



/* scope issues.... */
void zMapWindowCanvasFeaturesetSetWidth(ZMapWindowFeaturesetItem featureset, double width)
{
  featureset->width = width;
  foo_canvas_item_request_update ((FooCanvasItem *) featureset);
}


/* this is a nest of incestuous functions that all do the same thing
 * but at least this way we keep the search criteria in one place not 20
 */
static ZMapSkipList zmap_window_canvas_featureset_find_feature_coords(FeatureCmpFunc compare_func,
								      ZMapWindowFeaturesetItem fi,
								      double y1, double y2)
{
  ZMapSkipList sl;
  zmapWindowCanvasFeatureStruct search;
  double extra = fi->longest;

  if (!compare_func)
    compare_func = zMapFeatureCmp;

  search.y1 = y1;
  search.y2 = y2;

  if(fi->overlap)
    {
      if(fi->bumped)
	extra =  fi->bump_overlap;

      /* glyphs are fixed size so expand/ contract according to zoom, fi->longest is in canvas pixel coordinates  */
      if(fi->style->mode == ZMAPSTYLE_MODE_GLYPH)
	foo_canvas_c2w(((FooCanvasItem *) fi)->canvas, 0, ceil(extra), NULL, &extra);

      search.y1 -= extra;

      // this is harmelss and otherwise prevents features overlapping the featureset being found
      //      if(search.y1 < fi->start)
      //		search.y1 = fi->start;
    }

  sl =  zMapSkipListFind(fi->display_index, compare_func, &search) ;
  //	if(sl->prev)
  //		sl = sl->prev;	/* in case of not exact match when rebinned... done by SkipListFind */

  return sl;
}


static ZMapSkipList zmap_window_canvas_featureset_find_feature_index(ZMapWindowFeaturesetItem fi,ZMapFeature feature)
{
  ZMapWindowCanvasFeature gs;
  ZMapSkipList sl;

  sl = zmap_window_canvas_featureset_find_feature_coords(NULL, fi, feature->x1, feature->x2);

  /* find the feature's feature struct but return the skip list */

  while(sl)
    {
      gs = sl->data;

      /* if we got rebinned then we need to find the bin surrounding the feature
	 if the feature is split bewteeen bins just choose one
      */
      if(gs->y1 > feature->x2)
	return NULL;

      /* don't we know we have a feature and item that both exist?
	 There must have been a reason for this w/ DensityItems */
#if 1 // NEVER_REBINNED
      if(gs->feature == feature)
#else
	/*
	 *	if we re-bin variable sized features then we could have features extending beyond bins
	 *	in which case a simple containment test will fail
	 *	so we have to test for overlap, which will also handle the simpler case.
	 *	bins have a real feature attached and this is fed in from the cursor code (point function)
	 *	this argument also applies to fixed size bins: we pro-rate overlaps when we calculate bins
	 *	according to pixel coordinates.
	 *	so we can have bin contains feature, feature contains bin, bin and feature overlap left or right. Yuk
	 */
	/* NOTE lookup exact feature may fail if rebinned */
	if(!((gs->y1 > feature->x2) || (gs->y2 < feature->x1)))
#endif
	  {
	    return sl;
	  }

      sl = sl->next;
    }
  return NULL;
}

static ZMapWindowCanvasFeature zmap_window_canvas_featureset_find_feature(ZMapWindowFeaturesetItem fi,ZMapFeature feature)
{
  ZMapSkipList sl;
  ZMapWindowCanvasFeature gs = NULL;

  if(fi->last_added && fi->last_added->feature == feature)
    {
      gs = fi->last_added;
    }
  else
    {
      sl = zmap_window_canvas_featureset_find_feature_index(fi,feature);

      if(sl)
	gs = (ZMapWindowCanvasFeature) sl->data;
    }

  return gs;
}


void zmap_window_canvas_featureset_expose_feature(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs)
{
  double i2w_dx,i2w_dy;
  int cx1,cx2,cy1,cy2;
  FooCanvasItem *foo = (FooCanvasItem *) fi;
  double x1;

  x1 = 0.0;
  if(fi->bumped)
    x1 += gs->bump_offset;

  /* trigger a repaint */
  /* get canvas coords */
  i2w_dx = i2w_dy = 0.0;
  foo_canvas_item_i2w (foo, &i2w_dx, &i2w_dy);

  /* get item canvas coords, following example from FOO_CANVAS_RE (used by graph items) */
  foo_canvas_w2c (foo->canvas, x1 + i2w_dx, gs->y1 - fi->start + i2w_dy, &cx1, &cy1);
  foo_canvas_w2c (foo->canvas, x1 + gs->width + i2w_dx, gs->y2 - fi->start + i2w_dy, &cx2, &cy2);

  /* need to expose + 1, plus for glyphs add on a bit: bodged to 8 pixels
   * really ought to work out max glyph size or rather have true feature extent
   * NOTE this is only currently used via OTF remove exisitng features
   */
  foo_canvas_request_redraw (foo->canvas, cx1, cy1-8, cx2 + 1, cy2 + 8);
}


void zMapWindowCanvasFeaturesetRedraw(ZMapWindowFeaturesetItem fi, double zoom)
{
  double i2w_dx,i2w_dy;
  int cx1,cx2,cy1,cy2;
  FooCanvasItem *foo = (FooCanvasItem *) fi;
  double x1;
  double width = fi->width;

  fi->zoom = zoom;	/* can set to 0 to trigger recalc of zoom data */
  x1 = 0.0;
  if(fi->bumped)
    width = fi->bump_width;

  /* trigger a repaint */
  /* get canvas coords */
  i2w_dx = i2w_dy = 0.0;
  foo_canvas_item_i2w (foo, &i2w_dx, &i2w_dy);

  /* get item canvas coords, following example from FOO_CANVAS_RE (used by graph items) */
  foo_canvas_w2c (foo->canvas, x1 + i2w_dx, i2w_dy, &cx1, &cy1);
  foo_canvas_w2c (foo->canvas, x1 + width + i2w_dx, fi->end - fi->start + i2w_dy, &cx2, &cy2);

  /* need to expose + 1, plus for glyphs add on a bit: bodged to 8 pixels
   * really ought to work out max glyph size or rather have true featrue extent
   * NOTE this is only currently used via OTF remove exisitng features
   */
  foo_canvas_request_redraw (foo->canvas, cx1, cy1, cx2 + 1, cy2 + 1);
}


/* NOTE this function interfaces to user show/hide via zmapWindow/unhideItemsCB() and zmapWindowFocus/hideFocusItemsCB()
 * and could do other things if appropriate if we include some other flags
 * current best guess is that this is not best practice: eg summarise is an internal function and does not need an external interface
 */
void zmapWindowFeaturesetItemShowHide(FooCanvasItem *foo, ZMapFeature feature, gboolean show, ZMapWindowCanvasFeaturesetHideType how)
{
  ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) foo;
  ZMapWindowCanvasFeature gs;

  //printf("show hide %s %d %d\n",g_quark_to_string(feature->original_id),show,how);

  gs = zmap_window_canvas_featureset_find_feature(fi,feature);
  if(!gs)
    return;

  if(fi->highlight_sideways)	/* ie transcripts as composite features */
    {
      while(gs->left)
	gs = gs->left;
    }

  while(gs)
    {
      if(show)
	{
	  /* due to calling code these flgs are not operated correctly, so always show */
	  gs->flags &= ~FEATURE_HIDDEN & ~FEATURE_HIDE_REASON;
	}
      else
	{
	  switch(how)
	    {
	    case ZMWCF_HIDE_USER:
	      gs->flags |= FEATURE_HIDDEN | FEATURE_USER_HIDE;
	      break;

	    case ZMWCF_HIDE_EXPAND:
	      /* NOTE as expanded features get deleted if unbumped we can be fairly slack not testing for other flags */
	      gs->flags |= FEATURE_HIDDEN | FEATURE_HIDE_EXPAND;
	      break;
	    default:
	      break;
	    }
	}
      //printf("gs->flags: %lx\n", gs->flags);
      zmap_window_canvas_featureset_expose_feature(fi, gs);

      if(!fi->highlight_sideways)		/* only doing the selected one */
	return;

      gs = gs->right;
    }
}


/*
 * return the foo canvas item under the lassoo, and a list of more features if included
 * we restrict the features to one column as previously multi-select was restricted to one column
 * the column is defined by the centre of the lassoo and we include features inside not overlapping
 * NOTE we implement this for CanvasFeatureset only, not old style foo
 * NOTE due to happenstance (sic) transcripts are selected if they overlap
 */

/*
 * coordinates are canvas not world, we have to compare with foo bounds for each canvasfeatureset
 */
GList *zMapWindowFeaturesetItemFindFeatures(FooCanvasItem **item, double y1, double y2, double x1, double x2)
{
  ZMapWindowFeaturesetItem fset;
  ZMapSkipList sl;
  GList *feature_list = NULL;
  FooCanvasItem *foo = NULL;

  *item = NULL;

  double mid_x = (x1 + x2) / 2;
  GList *l, *lx;

  zMap_g_hash_table_get_data(&lx,featureset_class_G->featureset_items);

  for(l = lx;l ;l = l->next)
    {
      foo = (FooCanvasItem *) l->data;

      if (!(foo->object.flags & FOO_CANVAS_ITEM_VISIBLE))
	continue;

      if(foo->x1 < mid_x && foo->x2 > mid_x)
	break;
    }

  if(lx)
    g_list_free(lx);

  if(!l || !foo)
    return(NULL);

  fset = (ZMapWindowFeaturesetItem) foo;

  sl = zmap_window_canvas_featureset_find_feature_coords(NULL, fset, y1, y2);

  for(;sl ; sl = sl->next)
    {
      ZMapWindowCanvasFeature gs;
      gs = sl->data;

      if(gs->flags & FEATURE_HIDDEN)	/* we are setting focus on visible features ! */
	continue;

      if(gs->y1 > y2)
	break;

      /* reject overlaps as we can add to the selection but not subtract from it */
      if(gs->y1 < y1)
	continue;
      if(gs->y2 > y2)
	continue;

      if(fset->bumped)
	{
	  double x = fset->dx + gs->bump_offset;
	  if(x < x1)
	    continue;
	  x += gs->width;

	  if(x > x2)
	    continue;
	}
      /* else just match */

      if(!feature_list)
	{
	  *item = (FooCanvasItem *) fset;
	  zMapWindowCanvasItemSetFeaturePointer((ZMapWindowCanvasItem) *item, gs->feature);

	  /* rather boringly these could get revived later and overwrite the canvas item feature ?? */
	  /* NOTE probably not, the bug was a missing * in the line above */
	  fset->point_feature = gs->feature;
	  fset->point_canvas_feature = gs;
	}
      //     else	// why? item has the first one and feature list is the others if present
      // mh17: always include the first in the list to filter duplicates eg transcript exons
      {
	feature_list = zMap_g_list_append_unique(feature_list, gs->feature);
      }
    }
  return feature_list;
}




static guint32 gdk_color_to_rgba(GdkColor *color)
{
  guint32 rgba = 0;

  rgba = ((color->red & 0xff00) << 16  |
	  (color->green & 0xff00) << 8 |
	  (color->blue & 0xff00)       |
	  0xff);

  return rgba;
}


/* cut and paste from former graph density code */
gboolean zMapWindowFeaturesetItemSetStyle(ZMapWindowFeaturesetItem di, ZMapFeatureTypeStyle style)
{
  GdkColor *draw = NULL, *fill = NULL, *outline = NULL;
  FooCanvasItem *foo = (FooCanvasItem *) di;
  gboolean re_index = FALSE;
  GList  *features;

//  di->zoom = 0.0;		// trigger recalc


  if(zMapStyleGetMode(di->style) == ZMAPSTYLE_MODE_GRAPH && di->re_bin && zMapStyleDensityMinBin(di->style) != zMapStyleDensityMinBin(style))
    re_index = TRUE;

  if(di->display_index && re_index)
    {
      zMapSkipListDestroy(di->display_index, NULL);
      di->display_index = NULL;

      if(di->display)	/* was re-binned */
	{
	  for(features = di->display; features; features = g_list_delete_link(features,features))
	    {
	      ZMapWindowCanvasFeature feat = (ZMapWindowCanvasFeature) features->data;
	      zmapWindowCanvasFeatureFree(feat);
	    }
	  di->display = NULL;
	}
    }

  di->style = style;		/* includes col width */
  di->width = style->width;
  di->x_off = zMapStyleDensityStagger(style) * di->set_index;

  /* need to set colours */
  zmapWindowCanvasItemGetColours(style, di->strand, di->frame, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill, &draw, &outline, NULL, NULL);

  if(fill)
    {
      di->fill_set = TRUE;
      di->fill_colour = gdk_color_to_rgba(fill);
      di->fill_pixel = foo_canvas_get_color_pixel(foo->canvas, di->fill_colour);
    }
  if(outline)
    {
      di->outline_set = TRUE;
      di->outline_colour = gdk_color_to_rgba(outline);
      di->outline_pixel = foo_canvas_get_color_pixel(foo->canvas, di->outline_colour);
    }

  foo_canvas_item_request_update ((FooCanvasItem *)di);

  return TRUE;
}





/* NOTE this extends FooCanvasItem via ZMapWindowCanvasItem */
static void zmap_window_featureset_item_item_class_init(ZMapWindowFeaturesetItemClass featureset_class)
{
  GObjectClass *gobject_class ;
  FooCanvasItemClass *item_class;
  ZMapWindowCanvasItemClass canvas_class;

  featureset_class_G = featureset_class;
  featureset_class_G->featureset_items = g_hash_table_new(NULL,NULL);

  gobject_class = (GObjectClass *) featureset_class;
  item_class = (FooCanvasItemClass *) featureset_class;
  item_class_G = gtk_type_class(FOO_TYPE_CANVAS_ITEM);
  canvas_class = (ZMapWindowCanvasItemClass) featureset_class;

  gobject_class->dispose = zmap_window_featureset_item_item_destroy;

  item_class->update = zmap_window_featureset_item_item_update;
  item_class->bounds = zmap_window_featureset_item_item_bounds;
#if USE_FOO_POINT
  item_class->point  = zmap_window_featureset_item_foo_point;
#endif

  item_class->draw   = zmap_window_featureset_item_item_draw;


  canvas_class->set_colour = zmap_window_featureset_item_set_colour;
  canvas_class->set_feature = zmap_window_featureset_item_set_feature;
  canvas_class->set_style = zmap_window_featureset_item_set_style;
  canvas_class->showhide = zmap_window_featureset_item_show_hide;

  //  zmapWindowItemStatsInit(&(canvas_class->stats), ZMAP_TYPE_WINDOW_FEATURESET_ITEM) ;

  featureset_init_funcs();

  return ;
}


/* record the current feature found by cursor movement which continues moving as we run more code using the feature */
static gboolean zmap_window_featureset_item_set_feature(FooCanvasItem *item, double x, double y)
{
  if (g_type_is_a(G_OBJECT_TYPE(item), ZMAP_TYPE_WINDOW_FEATURESET_ITEM))
    {
      ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) item;
#if MOUSE_DEBUG
      zMapLogWarning("set feature %p",fi->point_feature);
#endif

#if !USE_FOO_POINT
      zmap_window_featureset_item_point(item, x, y);
#endif

      if(fi->point_feature)
	{
	  fi->__parent__.feature = fi->point_feature;
	  return TRUE;
	}
    }
  return FALSE;
}


/* redisplay the column using an alternate style */
static gboolean zmap_window_featureset_item_set_style(FooCanvasItem *item, ZMapFeatureTypeStyle style)
{
  if (g_type_is_a(G_OBJECT_TYPE(item), ZMAP_TYPE_WINDOW_FEATURESET_ITEM))
    {
      ZMapWindowFeaturesetItem di = (ZMapWindowFeaturesetItem) item;
      zMapWindowFeaturesetItemSetStyle(di,style);
    }
  return FALSE;
}



static gboolean zmap_window_featureset_item_show_hide(FooCanvasItem *item, gboolean show)
{
  if (g_type_is_a(G_OBJECT_TYPE(item), ZMAP_TYPE_WINDOW_FEATURESET_ITEM))
    {
      ZMapWindowFeaturesetItem di = (ZMapWindowFeaturesetItem) item;
      ZMapWindowCanvasItem canvas_item = (ZMapWindowCanvasItem) &(di->__parent__);
      /* find the feature struct and set a flag */
#warning this should be a class function
      zmapWindowFeaturesetItemShowHide(item,canvas_item->feature,show, ZMWCF_HIDE_USER);
    }
  return FALSE;
}


static void zmap_window_featureset_item_set_colour(ZMapWindowCanvasItem   item,
						   FooCanvasItem         *interval,
						   ZMapFeature			feature,
						   ZMapFeatureSubPartSpan sub_feature,
						   ZMapStyleColourType    colour_type,
						   int colour_flags,
						   GdkColor              *fill,
						   GdkColor              *border)
{
  if (g_type_is_a(G_OBJECT_TYPE(interval), ZMAP_TYPE_WINDOW_FEATURESET_ITEM))
    {
#warning this should be a class function
      zmapWindowFeaturesetItemSetColour(interval,feature,sub_feature,colour_type,colour_flags,fill,border);
    }

}



/* Called for each new featureset (== column ??). */
static void zmap_window_featureset_item_item_init(ZMapWindowFeaturesetItem featureset)
{

  return ;
}



static void zmap_window_featureset_item_item_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags)
{

  ZMapWindowFeaturesetItem di = (ZMapWindowFeaturesetItem) item;
  int cx1,cy1,cx2,cy2;
  double x1,x2,y1,y2;

  if(item_class_G->update)
    item_class_G->update(item, i2w_dx, i2w_dy, flags);

  //printf("update %s width = %.1f\n",g_quark_to_string(di->id),di->width);
  // cribbed from FooCanvasRE; this sets the canvas coords in the foo item
  /* x_off is needed for staggered graphs, is currently 0 for all other types */
  di->dx = x1 = i2w_dx + di->x_off;
  x2 = x1 + (di->bumped? di->bump_width : di->width);

  di->dy = y1 = i2w_dy;
  y2 = y1 + di->end - di->start;

  foo_canvas_w2c (item->canvas, x1, y1, &cx1, &cy1);
  foo_canvas_w2c (item->canvas, x2, y2, &cx2, &cy2);

  item->x1 = cx1;
  item->y1 = cy1;
  item->x2 = cx2+1;
  item->y2 = cy2+1;
}



#if USE_FOO_POINT

/* how far are we from the cursor? */
/* can't return foo canvas item for the feature as they are not in the canvas,
 * so return the featureset foo item adjusted to point at the nearest feature */

/* by a process of guesswork x,y are world coordinates and cx,cy are canvas (i think) */
/* No: x,y are parent item local coordinates ie offset within the group
 * we have a ZMapCanvasItem group with no offset, so we need to adjust by the x,ypos of that group
 */
double  zmap_window_featureset_item_foo_point(FooCanvasItem *item,
					      double item_x, double item_y, int cx, int cy,
					      FooCanvasItem **actual_item)
{
  double best = 1.0e36 ;				    /* Default value from foocanvas code. */
  ZMapWindowFeatureItemPointFunc point_func = NULL;
  ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem)item;
  ZMapWindowCanvasFeature gs;
  ZMapSkipList sl;
  double local_x, local_y ;
  double y1,y2;
  //      FooCanvasGroup *group;
  double x_off;
  static double save_best = 1.0e36, save_x = 0.0, save_y = 0.0 ;
  //int debug = fi->type >= FEATURE_GRAPHICS ;

  int n = 0;
  /*
   * need to scan internal list and apply close enough rules
   */

  /* zmapSkipListFind();		 gets exact match to start coord or item before
     if any feature overlaps choose that
     (assuming non overlapping features)
     else choose nearest of next and previous
  */
  *actual_item = NULL;


  /* optimise repeat calls: the foo canvas does 6 calls for a click event (3 down 3 up)
   * and if we are zoomed into a bumped peptide alignment column that means looking at a lot of features
   * each one goes through about 12 layers of canvas containers first but that's another issue
   * then if we move the lassoo that gets silly (button down: calls point())
   */
  //if(debug)
  //	zMapLogWarning("point: %.1f,%.1f %.1f %.1f\n", item_x, item_y, fi->start, fi->dy);

  if(fi->point_canvas_feature && item_x == save_x && item_y == save_y)
    {
      fi->point_feature = fi->point_canvas_feature->feature;
      *actual_item = item;
      best = save_best ;
    }
  else
    {
      save_x = item_x;
      save_y = item_y;
      fi->point_canvas_feature = NULL;

      best = fi->end - fi->start + 1;

#warning need to check these coordinate calculations
      local_x = item_x + fi->dx;
      local_y = item_y + fi->dy;
#warning this code is in the wrong place, review when containers rationalised

      y1 = local_y - item->canvas->close_enough;
      y2 = local_y + item->canvas->close_enough;


      /* This all seems a bit hokey...who says the glyphs are in the middle of the column ? */
      /* NOTE histgrams are hooked onto the LHS, but we can click on the row and still get the feature */
#warning change this to use featurex1 and x2 coords
      /* NOTE warning even better if we express point() fucntion in pixel coordinates only */
      x_off = fi->dx + fi->x_off + fi->width / 2;

      /* NOTE there is a flake in world coords at low zoom */
      /* NOTE close_enough is zero */
      sl = zmap_window_canvas_featureset_find_feature_coords(zMapFeatureFullCmp, fi, y1 , y2) ;

      //printf("point %s	%f,%f %d,%d: %p\n",g_quark_to_string(fi->id),x,y,cx,cy,sl);
      if (!sl)
	return best ;

      for (; sl ; sl = sl->next)
	{
	  gs = (ZMapWindowCanvasFeature) sl->data;
	  double this_one;

	  // printf("y1,2: %.1f %.1f,   gs: %s %lx %f %f\n",y1,y2, g_quark_to_string(gs->feature->unique_id), gs->flags, gs->y1,gs->y2);

	  n++;
	  if(gs->flags & FEATURE_HIDDEN)
	    continue;

	  // mh17: if best is 1e36 this is silly:
	  //	  if (gs->y1 > y2  + best)
	  if(gs->y1 > y2)		/* y2 has close_enough factored in */
	    break;

	  /* check for feature type specific point code, otherwise default to standard point func. */
	  point_func = NULL;

  	  if (gs->type > 0 && gs->type < FEATURE_N_TYPE)
	    point_func = _featureset_point_G[gs->type] ;
	  if (!point_func)
	    point_func = gs->type < FEATURE_GRAPHICS ? featurePoint : graphicsPoint;

	  if ((this_one = point_func(fi, gs, item_x, item_y, cx, cy, local_x, local_y, x_off)) < best)
	    {
	      fi->point_feature = gs->feature;
	      *actual_item = item;
	      //printf("overlaps x\n");
#warning this could concievably cause a memory fault if we freed point_canvas_feature but that seems unlikely if we don-t nove the cursor
	      fi->point_canvas_feature = gs;
	      best = this_one;

	      if(!best)	/* can't get better */
		{
		  /* and if we don't quit we will look at every other feature,
		   * pointlessly, although that makes no difference to the user
		   */
		  break;
		}
	    }
	}
    }


  /* experiment: this prevents the delay: 	*actual_item = NULL;  best = 1e36; */

#if MOUSE_DEBUG

  { char *x = "";
    extern int n_item_pick; // foo canvas debug

    if(fi->point_feature) x = (char *) g_quark_to_string(fi->point_feature->unique_id);

    zMapLogWarning("point tried %d/ %d features (%.1f,%.1f) @ %s (picked = %d)\n",
		   n,fi->n_features, item_x, item_y, x, n_item_pick);
  }
#endif

  return best;
}
#else

/* how far are we from the cursor? */
/* can't return foo canvas item for the feature as they are not in the canvas,
 * so return the featureset foo item adjusted to point at the nearest feature */
/* NOTE as the canvas featureset covers the whole column as signify 'no feature' by not setting actual_item */

/* by a process of guesswork x,y are world coordinates and cx,cy are canvas (i think) */
/* No: x,y are parent item local coordinates ie offset within the group
 * we have a ZMapCanvasItem group with no offset, so we need to adjust by the x,ypos of that group
 */


double  zmap_window_featureset_item_foo_point(FooCanvasItem *item,
					      double item_x, double item_y, int cx, int cy,
					      FooCanvasItem **actual_item)
{
  double best = zmap_window_featureset_item_point(item,cx,cy);
  ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem)item;

  if(fi->point_feature)
    *actual_item = item;
  return best;
}



/* cx an cy are from a button event and will be canvas coords */
double  zmap_window_featureset_item_point(FooCanvasItem *item, double cx, double cy)
{
  double best = 1.0e36 ;				    /* Default value from foocanvas code. */
  ZMapWindowFeatureItemPointFunc point_func ;
  ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem)item;
  ZMapWindowCanvasFeature gs;
  ZMapSkipList sl;
  double item_x, item_y ;
  double local_x, local_y ;
  double y1,y2;
  double x_off;
  static double save_best = 1.0e36, save_x = 0.0, save_y = 0.0 ;

  /* check for feature type specific point code, otherwise default to standard point func. */
  if (fi->type > 0 && fi->type < FEATURE_N_TYPE)
    point_func = _featureset_point_G[fi->type] ;

  if (!point_func)
    point_func = defaultPoint ;


  /*
   * need to scan internal list and apply close enough rules
   */

  /* zmapSkipListFind();		 gets exact match to start coord or item before
     if any feature overlaps choose that
     (assuming non overlapping features)
     else choose nearest of next and previous
  */

  fi->point_feature = NULL;

  /* optimise repeat calls: the foo canvas does 6 calls for a click event (3 down 3 up)
   * and if we are zoomed into a bumped peptide alignment column that means looking at a lot of features
   * each one goes through about 12 layers of canvas containers first but that's another issue
   * then if we move the lassoo that gets silly (button down: calls point())
   */

  if(fi->point_canvas_feature && cx == save_x && cy == save_y)
    {
      fi->point_feature = fi->point_canvas_feature->feature;
      best = save_best ;
    }
  else
    {
      save_x = cx;
      save_y = cy;

      /* get the seq coords from canvas, converse of zMapCanvasFeaturesetDrawBoxMacro */
      /* by analogy with the prior foo canvas function above local coords are sequence, item is offset within the foo grouip
       * which is in the opposite sense to what i'd expect
       * NOTE item_x,y are never used and could be removed quite easily
       */
      foo_canvas_c2w(item->canvas, cx, cy, &local_x, &local_y);
      item_x = local_x - fi->dx;
      item_y = local_y + fi->start - fi->dy;


      fi->point_canvas_feature = NULL;

      best = fi->end - fi->start + 1;

      y1 = local_y - item->canvas->close_enough;
      y2 = local_y + item->canvas->close_enough;


      /* This all seems a bit hokey...who says the glyphs are in the middle of the column ? */
      /* NOTE histograms are hooked onto the LHS, but we can click on the row and still get the feature */
#warning change this to use feature x1 and x2 coords
      /* NOTE warning even better if we express point() function in pixel coordinates only */
      x_off = fi->dx + fi->x_off + fi->width / 2;

      /* NOTE there is a flake in world coords at low zoom */
      /* NOTE close_enough is zero */
      sl = zmap_window_canvas_featureset_find_feature_coords(zMapFeatureFullCmp, fi, y1 , y2) ;

      //printf("point %s	%f,%f %d,%d: %p\n",g_quark_to_string(fi->id),x,y,cx,cy,sl);
      if (!sl)
	return best ;

      for (; sl ; sl = sl->next)
	{
	  gs = (ZMapWindowCanvasFeature) sl->data;
	  double this_one;

	  // printf("y1,2: %.1f %.1f,   gs: %s %lx %f %f\n",y1,y2, g_quark_to_string(gs->feature->unique_id), gs->flags, gs->y1,gs->y2);

	  if(gs->flags & FEATURE_HIDDEN)
	    continue;

	  // mh17: if best is 1e36 this is silly:
	  //	  if (gs->y1 > y2  + best)
	  if(gs->y1 > y2)		/* y2 has close_enough factored in */
	    break;

	  if ((this_one = point_func(fi, gs, item_x, item_y, cx, cy, local_x, local_y, x_off)) < best)
	    {
	      fi->point_feature = gs->feature;
	      //printf("overlaps x\n");
#warning this could concievably cause a memory fault if we freed point_canvas_feature but that seems unlikely if we don-t nove the cursor
	      fi->point_canvas_feature = gs;
	      best = this_one;
	    }
	}
    }

  return best;
}

#endif

/* Default function to check if the given x,y coord is within a feature, this
 * function assumes the feature is box-like. */
static double featurePoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
			   double item_x, double item_y, int cx, int cy,
			   double local_x, double local_y, double x_off)
{
  double best = 1.0e36 ;
  double can_start, can_end ;

  /* Get feature extent on display. */
  /* NOTE cannot use feature coords as transcript exons all point to the same feature */
  /* alignments have to implement a special fucntion to handle bumped features - the first exon gets expanded to cover the whole */
  /* when we get upgraded to vulgar strings these can be like transcripts... except that there's a performance problem due to volume */
  /* perhaps better to add  extra display/ search coords to ZMapWindowCanvasFeature ?? */
  can_start = gs->y1; 	//feature->x1 ;
  can_end = gs->y2;	//feature->x2 ;
  zmapWindowFeaturesetS2Ccoords(&can_start, &can_end) ;


  if (can_start <= local_y && can_end >= local_y)			    /* overlaps cursor */
    {
      double wx ;
      double left, right ;

      wx = x_off - (gs->width / 2) ;

      if (fi->bumped)
	wx += gs->bump_offset ;

      /* get coords within one pixel */
      left = wx - 1 ;					    /* X coords are on fixed zoom, allow one pixel grace */
      right = wx + gs->width + 1 ;

      if (local_x > left && local_x < right)			    /* item contains cursor */
	{
	  best = 0.0;
	}
    }

  return best ;
}

/* Default function to check if the given x,y coord is within a feature, this
 * function assumes the feature is box-like. */
static double graphicsPoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
			    double item_x, double item_y, int cx, int cy,
			    double local_x, double local_y, double x_off)
{
  double best = 1.0e36 ;
  double can_start, can_end ;
  ZMapWindowCanvasGraphics gfx = (ZMapWindowCanvasGraphics) gs;

  /* Get feature extent on display. */
  /* NOTE cannot use feature coords as transcript exons all point to the same feature */
  /* alignments have to implement a special fucntion to handle bumped features - the first exon gets expanded to cover the whole */
  /* when we get upgraded to vulgar strings these can be like transcripts... except that there's a performance problem due to volume */
  /* perhaps better to add  extra display/ search coords to ZMapWindowCancasFeature ?? */
  can_start = gs->y1; 	//feature->x1 ;
  can_end = gs->y2;	//feature->x2 ;
  zmapWindowFeaturesetS2Ccoords(&can_start, &can_end) ;


  if (can_start <= local_y && can_end >= local_y)			    /* overlaps cursor */
    {
      double left, right ;

      /* get coords within one pixel */
      left = gfx->x1 - 1 ;					    /* X coords are on fixed zoom, allow one pixel grace */
      right = gfx->x2 + 1 ;

      if (local_x > left && local_x < right)			    /* item contains cursor */
	{
	  best = 0.0;
	}
    }

  return best ;
}




void  zmap_window_featureset_item_item_bounds (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
  double minx,miny,maxx,maxy;
  FooCanvasGroup *group;

  minx = item->x1;
  miny = item->y1;
  maxx = item->x2;
  maxy = item->y2;

  /* Make the bounds be relative to our parent's coordinate system */
  if (item->parent)
    {
      group = FOO_CANVAS_GROUP (item->parent);

      minx += group->xpos;
      miny += group->ypos;
      maxx += group->xpos;
      maxy += group->ypos;
    }

  *x1 = minx;
  *y1 = miny;
  *x2 = maxx;
  *y2 = maxy;
}



/* a slightly ad-hoc function
 * really the feature context should specify complex features
 * but for historical reasons alignments come disconnected
 * and we have to join them up by inference (same name)
 * we may have several context featuresets but by convention all these have features with different names
 * do we have to do the same w/ transcripts? watch this space:
 *
 */
#warning revisit this when transcripts are implemented: call from zmapView code
void zmap_window_featureset_item_link_sideways(ZMapWindowFeaturesetItem fi)
{
  GList *l;
  ZMapWindowCanvasFeature left,right;		/* feat -ures */
  GQuark name = 0;

  /* we use the featureset features list which sits there in parallel with the skip list (display index) */
  /* sort by name and start coord */
  /* link same name features with ascending query start coord */

  /* if we ever need to link by something other than same name
   * then we can define a feature type specific sort function
   * and revive the zMapWindowCanvasFeaturesetLinkFeature() fucntion
   */

  fi->features = g_list_sort(fi->features,zMapFeatureNameCmp);
  fi->features_sorted = FALSE;

  for(l = fi->features;l;l = l->next)
    {
      right = (ZMapWindowCanvasFeature) l->data;
      right->left = right->right = NULL;		/* we can re-calculate so must zero */

      if(name == right->feature->original_id)
	{
	  right->left = left;
	  left->right = right;
	}
      left = right;
      name = left->feature->original_id;

    }

  fi->linked_sideways = TRUE;
}



void zMapWindowCanvasFeaturesetIndex(ZMapWindowFeaturesetItem fi)
{
  GList *features;

  /*
   * this call has to be here as zMapWindowCanvasFeaturesetIndex() is called from bump, which can happen before we get a paint
   * i tried to move it into alignments (it's a bodge to cope with the data being shredded before we get it)
   */
  if(fi->link_sideways && !fi->linked_sideways)
    zmap_window_featureset_item_link_sideways(fi);

  features = fi->display;		/* NOTE: is always sorted */

  if(!fi->features_sorted)
    {
      //printf("sort index\n");
      fi->features = g_list_sort(fi->features,zMapFeatureCmp);
      fi->features_sorted = TRUE;
    }
  if(!features)				/* was not pre-processed */
    features = fi->features;

  fi->display_index = zMapSkipListCreate(features, NULL);
}



void  zmap_window_featureset_item_item_draw (FooCanvasItem *item, GdkDrawable *drawable, GdkEventExpose *expose)
{
  ZMapSkipList sl;
  ZMapWindowCanvasFeature feat = NULL;
  double y1,y2;
  double width;
  GList *highlight = NULL;	/* must paint selected on top ie last */
  gboolean is_line;
  //gboolean debug = FALSE;

  ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) item;


  /* get visible scroll region in gdk coordinates to clip features that overlap and possibly extend beyond actual scroll
   * this avoids artifacts due to wrap round
   * NOTE we cannot calc this post zoom as we get scroll afterwards
   * except possibly if we combine the zoom and scroll operation
   * but this code cannot assume that
   */
  {
    GdkRegion *region;
    GdkRectangle rect;

    region = gdk_drawable_get_visible_region(drawable);
    gdk_region_get_clipbox ((const GdkRegion *) region, &rect);
    gdk_region_destroy(region);

    fi->clip_x1 = rect.x - 1;
    fi->clip_y1 = rect.y - 1;
    fi->clip_x2 = rect.x + rect.width + 1;
    fi->clip_y2 = rect.y + rect.height + 1;
  }

  if(!fi->gc && (item->object.flags & FOO_CANVAS_ITEM_REALIZED))
    fi->gc = gdk_gc_new (item->canvas->layout.bin_window);
  if(!fi->gc)
    return;		/* got a draw before realize ?? */

  /* check zoom level and recalculate */
  /* NOTE this also creates the index if needed */
  if(!fi->display_index || fi->zoom != item->canvas->pixels_per_unit_y)
    {
      fi->zoom = item->canvas->pixels_per_unit_y;
      fi->bases_per_pixel = 1.0 / fi->zoom;
      zMapWindowCanvasFeaturesetZoom(fi, drawable) ;
    }

  if(!fi->display_index)
    return; 		/* could be an empty column or a mistake */

  /* paint all the data in the exposed area */

  //  width = zMapStyleGetWidth(fi->style) - 1;		/* off by 1 error! width = #pixels not end-start */
  width = fi->width;

  /*
   *	get the exposed area
   *	find the top (and bottom?) items
   *	clip the extremes and paint all
   */

  fi->dx = fi->dy = 0.0;		/* this gets the offset of the parent of this item */
  foo_canvas_item_i2w (item, &fi->dx, &fi->dy);
  /* ref to zMapCanvasFeaturesetDrawBoxMacro to see how seq coords map to world coords and then canvas coords */

  foo_canvas_c2w(item->canvas,0,floor(expose->area.y - 1),NULL,&y1);
  foo_canvas_c2w(item->canvas,0,ceil(expose->area.y + expose->area.height + 1),NULL,&y2);

#if 0
  if(fi->type >= FEATURE_GRAPHICS)
    printf("expose %p %s %.1f,%.1f (%d %d, %d %d)\n", item->canvas, g_quark_to_string(fi->id), y1, y2, fi->clip_x1, fi->clip_y1, fi->clip_x2, fi->clip_y2);
#endif

  /* ok...this looks like the place to do feature specific painting..... */
  zMapWindowCanvasFeaturesetPaintSet(fi, drawable, expose) ;


  //if(zMapStyleDisplayInSeparator(fi->style)) debug = TRUE;

  sl = zmap_window_canvas_featureset_find_feature_coords(NULL, fi, y1, y2);
//if(debug) printf("draw %s	%f,%f: %p\n",g_quark_to_string(fi->id),y1,y2,sl);

  if(!sl)
    return;

  /* we have already found the first matching or previous item */
  /* get the previous one to handle wiggle plots that must go off screen */
  is_line = (zMapStyleGetMode(fi->style) == ZMAPSTYLE_MODE_GRAPH && fi->style->mode_data.graph.mode == ZMAPSTYLE_GRAPH_LINE);

  if(is_line)
    {
      feat = sl->prev ? (ZMapWindowCanvasFeature) sl->prev->data : NULL;
      zMapWindowCanvasFeaturesetPaintPrepare(fi,feat,drawable,expose);
    }

  for(fi->featurestyle = NULL;sl;sl = sl->next)
    {
      feat = (ZMapWindowCanvasFeature) sl->data;

//      if(debug && feat->feature) printf("feat: %s %lx %f %f\n",g_quark_to_string(feat->feature->unique_id), feat->flags, feat->y1,feat->y2);
      if(!is_line && feat->y1 > y2)		/* for lines we have to do one more */
	break;	/* finished */

      if(feat->y2 < y1)
	{
	  /* if bumped and complex then the first feature does the join up lines */
	  if(!fi->bumped || feat->left)
	    continue;
	}

      if (feat->type < FEATURE_GRAPHICS && (feat->flags & FEATURE_HIDDEN))
	continue;

      /* when bumped we can have a sequence wide 'bump_overlap
       * which means we could try to paint all the features
       * which would be slow
       * so we need to test again for the expose region and not call gdk
       * for alignments the first feature in a set has the colinear lines and we clip in that paint fucntion too
       */
      /* erm... already did that */

      /*
	NOTE need to sort out container positioning to make this work
	di covers its container exactly, but is it offset??
	by analogy w/ old style ZMapWindowCanvasItems we should display
	'intervals' as item relative
      */

      /* we don't display focus on lines */
      if (feat->type < FEATURE_GRAPHICS && !is_line && (feat->flags & FEATURE_FOCUS_MASK))
	{
	  highlight = g_list_prepend(highlight, feat);
	  continue;
	}

      /* clip this one (GDK does that? or is it X?) and paint */
      //if(debug) printf("paint %d-%d\n",(int) feat->y1,(int) feat->y2);

      /* set style colours if they changed */
      if(feat->type < FEATURE_GRAPHICS)
	zmapWindowCanvasFeaturesetSetColours(fi,feat);

      // call the paint function for the feature
      zMapWindowCanvasFeaturesetPaintFeature(fi,feat,drawable,expose);

      if(feat->y1 > y2)		/* for lines we have to do one more */
	break;	/* finished */
    }

  /* flush out any stored data (eg if we are drawing polylines) */
  zMapWindowCanvasFeaturesetPaintFlush(fi, sl ? feat : NULL, drawable, expose);

  if(!is_line && highlight)
    {
      // NOTE type will be < FEATURE_GRAPHICS for all items in the list

      highlight = g_list_reverse(highlight);	/* preserve normal display ordering */

      /* now paint the focus features on top, clear style to force colours lookup */
      for(fi->featurestyle = NULL;highlight;highlight = highlight->next)
	{
	  feat = (ZMapWindowCanvasFeature) highlight->data;

	  zmapWindowCanvasFeaturesetSetColours(fi,feat);
	  zMapWindowCanvasFeaturesetPaintFeature(fi,feat,drawable,expose);
	}
      zMapWindowCanvasFeaturesetPaintFlush(fi, feat ,drawable, expose);
    }

#if MOUSE_DEBUG
  zMapLogWarning("expose completes","");
#endif
}



void zmapWindowCanvasFeaturesetSetColours(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature feat)
{
  FooCanvasItem *item = (FooCanvasItem *) fi;
  ZMapStyleColourType ct;

  /* NOTE carefully balanced code:
   * we do all blurred features then all focussed ones
   * if the style changes then we look up the colours
   * so we reset the style before doing focus
   * other kinds of focus eg evidence do not have colours set in the style
   * so mixed focus types are not a problem
   */

  if((fi->featurestyle != feat->feature->style) || !(fi->frame && zMapStyleIsFrameSpecific(feat->feature->style)))
    /* diff style: set colour from style */
    {
      /* cache style for a single featureset
       * if we have one source featureset in the column then this works fastest (eg good for trembl/ swissprot)
       * if we have several (eg BAM rep1, rep2, etc,  Repeatmasker then we get to switch around frequently
       * but hopefully it's faster than not caching regardless
       * beware of enum ZMapWindowFocusType not being a bit-mask
       */

      GdkColor *fill = NULL,*draw = NULL, *outline = NULL;
      ZMapFrame frame;
      ZMapStrand strand;

      fi->featurestyle = feat->feature->style;

      /* eg for glyphs these get mixed up in one column so have to set for the feature not featureset */
      frame = zMapFeatureFrame(feat->feature);
      strand = feat->feature->strand;

      ct = feat->flags & WINDOW_FOCUS_GROUP_FOCUSSED ? ZMAPSTYLE_COLOURTYPE_SELECTED : ZMAPSTYLE_COLOURTYPE_NORMAL;

      zmapWindowCanvasItemGetColours(fi->featurestyle, strand, frame, ct , &fill, &draw, &outline, NULL, NULL);

      /* can cache these in the feature? or style?*/

      fi->fill_set = FALSE;
      if(fill)
	{
	  fi->fill_set = TRUE;
	  fi->fill_colour = zMap_gdk_color_to_rgba(fill);
	  fi->fill_pixel = foo_canvas_get_color_pixel(item->canvas, fi->fill_colour);
	}
      fi->outline_set = FALSE;
      if(outline)
	{
	  fi->outline_set = TRUE;
	  fi->outline_colour = zMap_gdk_color_to_rgba(outline);
	  fi->outline_pixel = foo_canvas_get_color_pixel(item->canvas, fi->outline_colour);
	}

    }
}


/* called by item drawing code, we cache style colours hoping it will run faster */
/* see also zmap_window_canvas_alignment_get_colours() */
int zMapWindowCanvasFeaturesetGetColours(ZMapWindowFeaturesetItem featureset,
					 ZMapWindowCanvasFeature feature,
					 gulong *fill_pixel, gulong *outline_pixel)
{
  int ret = 0;

  ret = zMapWindowFocusCacheGetSelectedColours(feature->flags, fill_pixel, outline_pixel);

  if(!(ret & WINDOW_FOCUS_CACHE_FILL))
    {
      if (featureset->fill_set)
	{
	  *fill_pixel = featureset->fill_pixel;
	  ret |= WINDOW_FOCUS_CACHE_FILL;
	}
    }

  if(!(ret & WINDOW_FOCUS_CACHE_OUTLINE))
    {
      if (featureset->outline_set)
	{
	  *outline_pixel = featureset->outline_pixel;
	  ret |= WINDOW_FOCUS_CACHE_OUTLINE;
	}
    }

  return ret;
}


/* show / hide all masked features in the CanvasFeatureset
 * this just means setting a flag
 *
 * unless of course we are re-masking and have to delete items if no colour is defined
 * in which case we destroy the index to force a rebuild: slow but not run very often
 * original foo code actually deleted the features, we will just flag them away.
 * feature homol flags displayed means 'is in foo'
 * so we could have to post process the featureset->features list
 * to delete then and then rebuild the index (skip list)
 * if the colour is not defined we should not have the show/hide menu item
 */
void zMapWindowCanvasFeaturesetShowHideMasked(FooCanvasItem *foo, gboolean show, gboolean set_colour)
{
  ZMapWindowFeaturesetItem featureset = (ZMapWindowFeaturesetItem) foo;
  ZMapSkipList sl;
  gboolean delete = FALSE;
  int has_colours;

  has_colours = zMapWindowFocusCacheGetSelectedColours(WINDOW_FOCUS_GROUP_MASKED, NULL, NULL);
  delete = !has_colours;

  for(sl = zMapSkipListFirst(featureset->display_index); sl; sl = sl->next)
    {
      ZMapWindowCanvasFeature feature = (ZMapWindowCanvasFeature) sl->data;	/* base struct of all features */

      if(feature->type == FEATURE_ALIGN && feature->feature->feature.homol.flags.masked)
	{
	  if(set_colour)      /* called on masking by another featureset */
	    {
	      feature->flags |= focus_group_mask[WINDOW_FOCUS_GROUP_MASKED];
	    }

	  if(set_colour && delete)
	    {
	      feature->feature->feature.homol.flags.displayed = FALSE;
	      feature->flags |= FEATURE_MASK_HIDE | FEATURE_HIDDEN;
	    }
	  else if(show)
	    {
	      feature->flags &= ~FEATURE_MASK_HIDE;
	      /* this could get complicated combined with summarise */
	      if(!(feature->flags & FEATURE_HIDE_REASON))
		{
		  feature->flags &= ~FEATURE_HIDDEN;
		  //					feature->feature.homol.flags.displayed = TRUE;	/* legacy, should net be needed */
		}
	    }
	  else
	    {
	      feature->flags |= FEATURE_MASK_HIDE | FEATURE_HIDDEN;
	      //				feature->feature.homol.flags.displayed = FALSE;	/* legacy, should net be needed */
	    }
	}
    }
#if MH17_NOT_IMPLEMENTED
  if(delete)
    {
      scan featureset->features, delete masked features with homol.flags/displayed == FALSE
	destroy the index to force a rebuild
	}
#endif
}


static long n_block_alloc = 0;
static long n_feature_alloc = 0;
static long n_feature_free = 0;


/* allocate a free list for an unknown structure */
ZMapWindowCanvasFeature zmapWindowCanvasFeatureAlloc(zmapWindowCanvasFeatureType type)
{
  GList *l;
  gpointer mem;
  int size;
  ZMapWindowCanvasFeature feat;
  zmapWindowCanvasFeatureType ftype = type;

  if(type <= FEATURE_INVALID || type >= FEATURE_N_TYPE)
    {
      /* there was a crash where type and ftype were bth rubbigh and different
       * adding this if 'cured' it
       * look like stack corruption ??
       * but how???
       */
      //	  int x = 0;	/* try to make totalview work */
      zMapAssert("bad feature type in alloc");
    }

  size = featureset_class_G->struct_size[type];
  if(!size)
    {
      type = FEATURE_INVALID;		/* catch all for simple features (one common free list) */
      size = sizeof(zmapWindowCanvasFeatureStruct);
    }

  if(!featureset_class_G->feature_free_list[type])
    {
      int i;
      mem = g_malloc(size * N_FEAT_ALLOC);
      zMapAssert(mem);

      for(i = 0;i < N_FEAT_ALLOC;i++, mem += size)
      	{
	  feat = (ZMapWindowCanvasFeature) mem;
	  feat->type = type;
	  feat->feature = NULL;
	  zmapWindowCanvasFeatureFree((gpointer) mem);
	}
      n_block_alloc++;
    }
  zMapAssert(featureset_class_G->feature_free_list[type]);

  l = featureset_class_G->feature_free_list[type];
  feat = (ZMapWindowCanvasFeature) l->data;
  featureset_class_G->feature_free_list[type] = g_list_delete_link(featureset_class_G->feature_free_list[type],l);

  /* these can get re-allocated so must zero */
  memset((gpointer) feat,0,size);

  feat->type = ftype;

  n_feature_alloc++;
  return(feat);
}


/* need to be a ZMapSkipListFreeFunc for use as a callback */
void zmapWindowCanvasFeatureFree(gpointer thing)
{
  ZMapWindowCanvasFeature feat = (ZMapWindowCanvasFeature) thing;
  zmapWindowCanvasFeatureType type = feat->type;

  if(!featureset_class_G->struct_size[type])
    type = FEATURE_INVALID;		/* catch all for simple features */

  featureset_class_G->feature_free_list[type] =
    g_list_prepend(featureset_class_G->feature_free_list[type], thing);

  n_feature_free++;
}




/* sort by name and the start coord to link same name features */
/* note that ultimately we are interested in query coords in a zmapHomol struct
 * but only after getting alignments in order on the genomic sequence
 */
gint zMapFeatureNameCmp(gconstpointer a, gconstpointer b)
{
  ZMapWindowCanvasFeature feata = (ZMapWindowCanvasFeature) a;
  ZMapWindowCanvasFeature featb = (ZMapWindowCanvasFeature) b;

  if(!featb)
    {
      if(!feata)
	return(0);
      return(1);
    }
  if(!feata)
    return(-1);

  if(feata->feature->strand < featb->feature->strand)
    return(-1);
  if(feata->feature->strand > featb->feature->strand)
    return(1);

  if(feata->feature->original_id < featb->feature->original_id)
    return(-1);
  if(feata->feature->original_id > featb->feature->original_id)
    return(1);

  return(zMapFeatureCmp(a,b));
}



/* Fuller version of zMapFeatureCmp() which handles special glyph code where
 * positions to be compared can be greater than the feature coords.
 *
 * NOTE that featb is a 'dummy' just used for coords.
 *  */
gint zMapFeatureFullCmp(gconstpointer a, gconstpointer b)
{
  ZMapWindowCanvasFeature feata = (ZMapWindowCanvasFeature) a ;
  ZMapWindowCanvasFeature featb = (ZMapWindowCanvasFeature) b ;

  /* we can get NULLs due to GLib being silly */
  /* this code is pedantic, but I prefer stable sorting */
  if(!featb)
    {
      if(!feata)
	return(0);
      return(1);
    }
  else if(!feata)
    {
      return(-1);
    }
  else
    {
      ZMapFeature feature = feata->feature ;
      int real_start, real_end ;

      real_start = feata->y1 ;
      real_end = feata->y2 ;

      if (feata->type == FEATURE_GLYPH && feature->flags.has_boundary)
	{
	  if (feature->boundary_type == ZMAPBOUNDARY_5_SPLICE)
	    {
	      real_start = feata->y1 + 0.5 ;
	      real_end = feata->y2 + 2 ;
	    }
	  else if (feature->boundary_type == ZMAPBOUNDARY_3_SPLICE)
	    {
	      real_start = feata->y1 - 2 ;
	      real_end = feata->y2 - 0.5 ;
	    }


	  /* Look for dummy to be completey inside.... */
	  if (real_start <= featb->y1 && real_end >= featb->y2)
	    return(0);

	  if(real_start < featb->y1)
	    return(-1);
	  if(real_start > featb->y1)
	    return(1);
	  if(real_end > featb->y2)
	    return(-1);
	  if(real_end < featb->y2)
	    return(1);
	}
      else
	{
	  if(real_start < featb->y1)
	    return(-1);
	  if(real_start > featb->y1)
	    return(1);

	  if(real_end > featb->y2)
	    return(-1);

	  if(real_end < featb->y2)
	    return(1);
	}
    }

  return(0);
}



/* sort by genomic coordinate for display purposes */
/* start coord then end coord reversed, mainly for summarise fucntion */
/* also used by collapse code and locus de-overlap  */
gint zMapFeatureCmp(gconstpointer a, gconstpointer b)
{
  ZMapWindowCanvasFeature feata = (ZMapWindowCanvasFeature) a;
  ZMapWindowCanvasFeature featb = (ZMapWindowCanvasFeature) b;

  /* we can get NULLs due to GLib being silly */
  /* this code is pedantic, but I prefer stable sorting */
  if(!featb)
    {
      if(!feata)
	return(0);
      return(1);
    }
  if(!feata)
    return(-1);

  if(feata->y1 < featb->y1)
    return(-1);
  if(feata->y1 > featb->y1)
    return(1);

  if(feata->y2 > featb->y2)
    return(-1);

  if(feata->y2 < featb->y2)
    return(1);
  return(0);
}


/*
 * look up a feature
 * < 1 if feature is before b
 * > 1 if feature is after b
 * 0 if it overlaps
 */
gint zMapFeatureFind(gconstpointer a, gconstpointer b)
{
  ZMapWindowCanvasFeature feata = (ZMapWindowCanvasFeature) a;
  ZMapWindowCanvasFeature featb = (ZMapWindowCanvasFeature) b;

  /* we can get NULLs due to GLib being silly */
  /* this code is pedantic, but I prefer stable sorting */
  if(!featb)
    {
      if(!feata)
	return(0);
      return(1);
    }
  if(!feata)
    return(-1);

  if(feata->y1 < featb->y1)
    return(-1);
  if(feata->y1 > featb->y1)
    return(1);

  if(feata->y2 > featb->y2)
    return(-1);

  if(feata->y2 < featb->y2)
    return(1);
  return(0);
}


int get_heat_rgb(int a,int b,double score)
{
  int val = b - a;

  val = a + (int) (val * score);
  if(val < 0)
    val = 0;
  if(val > 0xff)
    val = 0xff;
  return(val);
}

/* find an RGBA pixel value between a and b */
/* NOTE foo canvas and GDK have got in a tangle with pixel values and we go round in circle to do this
 * but i acted dumb and followed the procedures (approx) used elsewhere
 */
gulong zMapWindowCanvasFeatureGetHeatColour(gulong a, gulong b, double score)
{
  int ar,ag,ab;
  int br,bg,bb;
  gulong colour;

  a >>= 8;		/* discard alpha */
  ab = a & 0xff; a >>= 8;
  ag = a & 0xff; a >>= 8;
  ar = a & 0xff; a >>= 8;

  b >>= 8;
  bb = b & 0xff; b >>= 8;
  bg = b & 0xff; b >>= 8;
  br = b & 0xff; b >>= 8;

  colour = 0xff;
  colour |= get_heat_rgb(ab,bb,score) << 8;
  colour |= get_heat_rgb(ag,bg,score) << 16;
  colour |= get_heat_rgb(ar,br,score) << 24;

  return(colour);
}





/* cribbed from zmapWindowGetPosFromScore(() which is called 3x from Item Factory and will eventaully get removed */
double zMapWindowCanvasFeatureGetWidthFromScore(ZMapFeatureTypeStyle style, double width, double score)
{
  double dx = 0.0 ;
  double numerator, denominator ;
  double max_score, min_score ;

  min_score = zMapStyleGetMinScore(style) ;
  max_score = zMapStyleGetMaxScore(style) ;

  /* We only do stuff if there are both min and max scores to work with.... */
  if (max_score && min_score)
    {
      numerator = score - min_score ;
      denominator = max_score - min_score ;

      if (denominator == 0)				    /* catch div by zero */
	{
	  if (numerator <= 0)
	    dx = 0.25 ;
	  else if (numerator > 0)
	    dx = 1 ;
	}
      else
	{
	  dx = 0.25 + (0.75 * (numerator / denominator)) ;
	}

      if (dx < 0.25)
	dx = 0.25 ;
      else if (dx > 1)
	dx = 1 ;

      width *= dx;
    }

  return width;
}


/* used by graph data ... which recalulates bins */
/* normal features just have width set from feature score */
double zMapWindowCanvasFeatureGetNormalisedScore(ZMapFeatureTypeStyle style, double score)
{
  double numerator, denominator, dx ;
  double max_score, min_score ;

  min_score = zMapStyleGetMinScore(style) ;
  max_score = zMapStyleGetMaxScore(style) ;

  numerator = score - min_score ;
  denominator = max_score - min_score ;

  if(numerator < 0)			/* coverage and histgrams do not have -ve values */
    numerator = 0;
  if(denominator < 0)		/* dumb but wise, could conceivably be mis-configured and not checked */
    denominator = 0;

  if(style->mode_data.graph.scale == ZMAPSTYLE_GRAPH_SCALE_LOG)
    {
      numerator++;	/* as log(1) is zero we need to bodge values of 1 to distingish from zero */
      /* and as log(0) is big -ve number bias zero to come out as zero */

      numerator = log(numerator);
      denominator = log(denominator) ;
    }

  if (denominator == 0)                         /* catch div by zero */
    {
      if (numerator < 0)
	dx = 0 ;
      else if (numerator > 0)
	dx = 1 ;
    }
  else
    {
      dx = numerator / denominator ;
      if (dx < 0)
	dx = 0 ;
      if (dx > 1)
	dx = 1 ;
    }
  return(dx);
}

double zMapWindowCanvasFeaturesetGetFilterValue(FooCanvasItem *foo)
{
  ZMapWindowFeaturesetItem featureset_item;

  featureset_item = (ZMapWindowFeaturesetItem) foo;
  return featureset_item->filter_value ;
}

int zMapWindowCanvasFeaturesetGetFilterCount(FooCanvasItem *foo)
{
  ZMapWindowFeaturesetItem featureset_item;

  featureset_item = (ZMapWindowFeaturesetItem) foo;
  return featureset_item->n_filtered ;
}

int zMapWindowCanvasFeaturesetFilter(gpointer gfilter, double value)
{
  ZMapWindowFilter filter	= (ZMapWindowFilter) gfilter;
  ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) filter->featureset;
  ZMapSkipList sl;
  int was = fi->n_filtered;
  double score;

  fi->filter_value = value;
  fi->n_filtered = 0;

  for(sl = zMapSkipListFirst(fi->display_index); sl; sl = sl->next)
    {
      ZMapWindowCanvasFeature feature = (ZMapWindowCanvasFeature) sl->data;	/* base struct of all features */
      ZMapWindowCanvasFeature f;

      if(feature->left)		/* we do joined up alignments */
	continue;

      if(! feature->feature->flags.has_score)
	continue;


      /* get score for whole series of alignments */
      for(f = feature, score = 0.0; f; f = f->right)
	{
	  double feature_score = f->feature->score;
	  /* NOTE feature->score is normalised, feature->feature->score is what we filter by */

	  if(zMapStyleGetScoreMode(f->feature->style) == ZMAPSCORE_PERCENT)
	    feature_score = f->feature->feature.homol.percent_id;

	  if(feature_score > score)
	    score = feature_score;
	}

      /* set flags for whole series based on max score: filter is all below value */
      for(f = feature; f; f = f->right)
	{
	  if(score < value)
	    {
	      f->flags |= FEATURE_HIDE_FILTER | FEATURE_HIDDEN;
	      fi->n_filtered++;

	      /* NOTE many of these may be FEATURE_SUMMARISED which is not operative during bump
	       * so setting HIDDEN here must be done for the filtered features only
	       * Hmmm... not quite sure of the mechanism, but putting this if
	       * outside the brackets give a glitch on set column focus when bumped (features not painted)
	       * .... summarised features (FEATURE_SUMMARISED set but not HIDDEN bue to bumping) set to hidden when bumped
	       */
	    }
	  else
	    {
	      /* reset in case score went down */

	      f->flags &= ~FEATURE_HIDE_FILTER;
	      if(!(f->flags & FEATURE_HIDE_REASON))
		f->flags &= ~FEATURE_HIDDEN;
	      else if(fi->bumped && (f->flags & FEATURE_HIDE_REASON) == FEATURE_SUMMARISED)
		f->flags &= ~FEATURE_HIDDEN;
	    }
	}
    }

  if(fi->n_filtered != was)
    {
      /* trigger a re-calc if summarised to ensure the picture is pixel perfect
       * NOTE if bumped we don;t calculate so no creeping inefficiency here
       */
      fi->zoom = 0;


      if(fi->bumped)
	{
	  ZMapWindowCompressMode compress_mode;

	  if (zMapWindowMarkIsSet(filter->window))
	    compress_mode = ZMAPWINDOW_COMPRESS_MARK ;
	  else
	    compress_mode = ZMAPWINDOW_COMPRESS_ALL ;

	  zmapWindowColumnBumpRange(filter->column, ZMAPBUMP_INVALID, ZMAPWINDOW_COMPRESS_INVALID);

	  /* dissapointing: we only need to reposition columns to the right of this one */

	  zmapWindowFullReposition(filter->window) ;
	}
      else
	{
	  zMapWindowCanvasFeaturesetRedraw(fi, fi->zoom);
	}
    }

  return(fi->n_filtered);
}


void zMapWindowFeaturesetSetFeatureWidth(ZMapWindowFeaturesetItem featureset_item, ZMapWindowCanvasFeature feat)
{
  ZMapFeature feature = feat->feature;
  ZMapFeatureTypeStyle style = feature->style;

  feat->width = featureset_item->width;

  if(feature->population)	/* collapsed duplicated features, takes precedence over score */
    {
      double score = (double) feature->population;

      feat->score = zMapWindowCanvasFeatureGetNormalisedScore(style, score);

      if ((zMapStyleGetScoreMode(style) == ZMAPSCORE_WIDTH) || (zMapStyleGetScoreMode(style) == ZMAPSCORE_HEAT_WIDTH))
	feat->width = zMapWindowCanvasFeatureGetWidthFromScore(style, featureset_item->width, score);
    }
  else if(feature->flags.has_score)
    {
      if(featureset_item->style->mode == ZMAPSTYLE_MODE_GRAPH)
	{
	  feat->score = zMapWindowCanvasFeatureGetNormalisedScore(style, feature->score);
	  feat->width = featureset_item->width * feat->score;
	}
      else
	{
	  if ((zMapStyleGetScoreMode(style) == ZMAPSCORE_WIDTH && feature->flags.has_score))
	    feat->width = zMapWindowCanvasFeatureGetWidthFromScore(style, featureset_item->width, feature->score);
	  else if(zMapStyleGetScoreMode(style) == ZMAPSCORE_PERCENT)
	    feat->width = zMapWindowCanvasFeatureGetWidthFromScore(style, featureset_item->width, feature->feature.homol.percent_id);
	}
    }
}


void zmapWindowFeaturesetAddToIndex(ZMapWindowFeaturesetItem featureset_item, ZMapWindowCanvasFeature feat)
{
  /* even if they come in order we still have to sort them to be sure so just add to the front */
  /* NOTE we asign the from pointer here: not just more efficient if we have eg 60k features but essential to prepend */
  //  feat->from =
  // only for feature not graphics, form odes not work anyway

  featureset_item->features = g_list_prepend(featureset_item->features,feat);
  featureset_item->n_features++;

  //  printf("add item %s/%s @%p: %ld/%d\n",g_quark_to_string(featureset_item->id),g_quark_to_string(feature->unique_id),feature, featureset_item->n_features, g_list_length(featureset_item->features));

  /* add to the display bins if index already created */
  if(featureset_item->display_index)
    {
      /* have to re-sort... NB SkipListAdd() not exactly well tested, so be dumb */
      /* it's very rare that we add features anyway */
      /* although we could have several featuresets being loaded into one column */
      /* whereby this may be more efficient ? */
#if 1
      {
	/* need to recalc bins */
	/* quick fix FTM, de-calc which requires a re-calc on display */
	zMapSkipListDestroy(featureset_item->display_index, NULL);
	featureset_item->display_index = NULL;
      }
    }
  /* must set this independantly as empty columns with no index get flagged as sorted */
  featureset_item->features_sorted = FALSE;
  //  featureset_item->zoom = 0.0;	/* trigger a recalc */
#else
  // untested code
  {
    /* NOTE when this is implemented properly then it might be best to add to a linked list if there's no index created
     * as that might be more effecient,  or maybe it won't be.
     * i'd guess that creating a skip list from a sorted list is faster
     * Worth considering and also timing this,
     */
    featureset_item->display_index =
      zMapSkipListAdd(featureset_item->display_index, zMapFeatureCmp, feat);
	/* NOTE need to fix linked_sideways */
  }
}
#endif
}

ZMapWindowCanvasFeature zMapWindowFeaturesetAddFeature(ZMapWindowFeaturesetItem featureset_item, ZMapFeature feature, double y1, double y2)
{
  ZMapWindowCanvasFeature feat;
  ZMapFeatureTypeStyle style = feature->style;
  zmapWindowCanvasFeatureType type = FEATURE_INVALID;

  zMapAssert(zMapFeatureIsValid((ZMapFeatureAny) feature));

  if(style)
    type = feature_types[zMapStyleGetMode(feature->style)];
  if(type == FEATURE_INVALID)		/* no style or feature type not implemented */
    return NULL;

  feat = zmapWindowCanvasFeatureAlloc(type);

  feat->feature = feature;
  feat->type = type;

  feat->y1 = y1;
  feat->y2 = y2;

  if(y2 - y1 > featureset_item->longest)
    featureset_item->longest = y2 - y1;

  zmapWindowFeaturesetAddToIndex(featureset_item, feat);

  return feat;
}


ZMapWindowCanvasGraphics zMapWindowFeaturesetAddGraphics(ZMapWindowFeaturesetItem featureset_item, zmapWindowCanvasFeatureType type, double x1, double y1, double x2, double y2, GdkColor *fill, GdkColor *outline, char *text)
{
  ZMapWindowCanvasGraphics feat;
  gulong fill_pixel= 0, outline_pixel = 0;
  FooCanvasItem *foo = (FooCanvasItem *) featureset_item;

  if (type == FEATURE_INVALID || type < FEATURE_GRAPHICS)
    return NULL;

  feat = (ZMapWindowCanvasGraphics) zmapWindowCanvasFeatureAlloc(type);

  feat->type = type;

  if(x1 > x2) {  double x = x1; x1 = x2; x2 = x; }	/* boring.... */
  if(y1 > y2) {  double x = y1; y1 = y2; y2 = x; }

  feat->y1 = y1;
  feat->y2 = y2;

  feat->x1 = x1;
  feat->x2 = x2;

  feat->text = text;

  if(fill)
    {
      fill_pixel = zMap_gdk_color_to_rgba(fill);
      feat->fill = foo_canvas_get_color_pixel(foo->canvas, fill_pixel);
      feat->flags |= WCG_FILL_SET;
    }
  if(outline)
    {
      outline_pixel = zMap_gdk_color_to_rgba(outline);
      feat->outline = foo_canvas_get_color_pixel(foo->canvas, outline_pixel);
      feat->flags |= WCG_OUTLINE_SET;
    }

  if(y2 - y1 > featureset_item->longest)
    featureset_item->longest = y2 - y1 + 1;

  zmapWindowFeaturesetAddToIndex(featureset_item, (ZMapWindowCanvasFeature) feat);

  return feat;
}



int zMapWindowFeaturesetRemoveGraphics(ZMapWindowFeaturesetItem featureset_item, ZMapWindowCanvasGraphics feat)
{
#warning zMapWindowFeaturesetRemoveGraphics not implemented
  /* is this needed? yes: diff struct */

#if 0
  /* copy from remove feature */


  /* not strictly necessary to re-sort as the order is the same
   * but we avoid the index becoming degenerate by doing this
   * better to implement zmapSkipListRemove() properly
   */
  if(fi->display_index)
    {
      /* need to recalc bins */
      /* quick fix FTM, de-calc which requires a re-calc on display */
      zMapSkipListDestroy(fi->display_index, NULL);
      fi->display_index = NULL;
      /* is still sorted if it was before */
    }

  return fi->n_features;
#else
  return 0;
#endif
}



/*
  delete feature from the features list and trash the index
  returns no of features left

  rather tediously we can't get the feature via the index as that will give us the feature not the list node pointing to it.
  so to delete a whole featureset we could have a quadratic search time unless we delete in order
  but from OTF if we delete old ones we do this via a small hash table
  we don't delete elsewhere, execpt for legacy gapped alignments, so this works ok by fluke

  this function's a bit horrid: when we find the feature to delete we have to look it up in the index to repaint
  we really need a column refresh

  we really need to rethink this: deleting was not considered
*/
#warning need to revist this code and make it more efficient
// ideas:
// use a skip list exclusively ??
// use features list for loading, convert to skip list and remove features
// can add new features via list and add to skip list (extract skip list, add to features , sort and re-create skip list

/* NOTE Here we improve the efficiency of deleting a feature with some rubbish code
 * we add a pointer to a feature's list mode in the fi->features list
 * which allows us to delete that feature from the list without searching for it
 * this is building rubbish on top of rubbish: really we should loose the features list
 * and _only_ store references to features in the skip list
 * there's reasons why not:
 * - haven't implemented add/delete in zmapSkipList.c, which requires fiddly code to prevent degeneracy.
 *   (Initially not implemented as not used, but now needed).
 * - graph density items recalculate bins and have a seperate list of features that get indexed -> we should do this using another skip list
 * - sometimes we may wish to sort differently, having a diff list to sort helps... we can use glib functions.
 * So to implement this quickly i added an extra pointer in the CanvasFeature struct.
 *
 * unfortunately glib sorts by creating new list nodes (i infer) so that the from pointer is invalid
 *
 */
int zMapWindowFeaturesetItemRemoveFeature(FooCanvasItem *foo, ZMapFeature feature)
{
  ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) foo;


#if 1 // ORIGINAL_SLOW_VERSION
  GList *l;
  ZMapWindowCanvasFeature feat;

  for(l = fi->features;l;)
    {
      GList *del;

      feat = (ZMapWindowCanvasFeature) l->data;

      if(feat->feature == feature)
	{
	  /* NOTE the features list and display index both point to the same structs */

	  zmap_window_canvas_featureset_expose_feature(fi, feat);

	  zmapWindowCanvasFeatureFree(feat);
	  del = l;
	  l = l->next;
	  fi->features = g_list_delete_link(fi->features,del);
	  fi->n_features--;

	  if(fi->link_sideways)	/* we'll get calls for each sub-feature */
	    break;
	  /* else have to go through the whole list; fortunately transcripts are low volume */
	}
      else
	{
	  l = l->next;
	}
    }

  /* NOTE we may not have an index so this flag must be unset seperately */
  fi->linked_sideways = FALSE;  /* See code below: this was slack */


#else

  ZMapWindowCanvasFeature gs = zmap_window_canvas_featureset_find_feature(fi,feature);

  if(gs)
    {
      GList *link = gs->from;
      /* NOTE search for ->from if you revive this */
      /* glib  g_list_sort() invalidates these adresses -> preserves the data but not the list elements */

      zMapAssert(link);
      zmap_window_canvas_featureset_expose_feature(fi, gs);


      //      if(fi->linked_sideways)
      {
	if(gs->left)
	  gs->left->right = gs->right;
	if(gs->right)
	  gs->right->left = gs->left;
      }

      zmapWindowCanvasFeatureFree(gs);
      fi->features = g_list_delete_link(fi->features,link);
      fi->n_features--;
    }
#endif

  /* not strictly necessary to re-sort as the order is the same
   * but we avoid the index becoming degenerate by doing this
   * better to implement zmapSkipListRemove() properly
   */
  if(fi->display_index)
    {
      /* need to recalc bins */
      /* quick fix FTM, de-calc which requires a re-calc on display */
      zMapSkipListDestroy(fi->display_index, NULL);
      fi->display_index = NULL;
      /* is still sorted if it was before */
    }

  return fi->n_features;
}


#warning make this into a foo canvas item class func
/* get the bounds of the current feature which has been set by the caller */
void zMapWindowCanvasFeaturesetGetFeatureBounds(FooCanvasItem *foo, double *rootx1, double *rooty1, double *rootx2, double *rooty2)
{
  ZMapWindowCanvasItem item = (ZMapWindowCanvasItem) foo;
  ZMapWindowFeaturesetItem fi;

  fi = (ZMapWindowFeaturesetItem) foo;

  if(rootx1)
    *rootx1 = fi->dx;
  if(rootx2)
    *rootx2 = fi->dx + fi->width;
  if(rooty1)
    *rooty1 = item->feature->x1;
  if(rooty2)
    *rooty2 = item->feature->x2;
}



static void zmap_window_featureset_item_item_destroy     (GObject *object)
{

  ZMapWindowFeaturesetItem featureset_item;
  GList *features;
  ZMapWindowCanvasFeature feat;

  /* mh17 NOTE: this function gets called twice for every object via a very very tall stack */
  /* no idea why, but this is all harmless here if we make sure to test if pointers are valid */
  /* what's more interesting is why an object has to be killed twice */


  //  printf("zmap_window_featureset_item_item_destroy %p\n",object);

  g_return_if_fail(ZMAP_IS_WINDOW_FEATURESET_ITEM(object));

  featureset_item = ZMAP_WINDOW_FEATURESET_ITEM(object);

  if(featureset_item->display_index)
    {
      zMapSkipListDestroy(featureset_item->display_index, NULL);
      featureset_item->display_index = NULL;
      featureset_item->features_sorted = FALSE;
    }
  if(featureset_item->display)	/* was re-binned */
    {
      for(features = featureset_item->display; features; features = g_list_delete_link(features,features))
	{
	  feat = (ZMapWindowCanvasFeature) features->data;
	  zmapWindowCanvasFeatureFree(feat);
	}
      featureset_item->display = NULL;
    }

  if(featureset_item->features)
    {
      /* free items separately from the index as conceivably we may not have an index */
      for(features = featureset_item->features; features; features = g_list_delete_link(features,features))
	{
	  feat = (ZMapWindowCanvasFeature) features->data;
	  zmapWindowCanvasFeatureFree(feat);
	}
      featureset_item->features = NULL;
      featureset_item->n_features = 0;
    }


  /* removing it the second time will fail gracefully */
  g_hash_table_remove(featureset_class_G->featureset_items,GUINT_TO_POINTER(featureset_item->id));

  //printf("features %s: %ld %ld %ld,\n",g_quark_to_string(featureset_item->id), n_block_alloc, n_feature_alloc, n_feature_free);

  zMapWindowCanvasFeaturesetFree(featureset_item);	/* must tidy opt */

  if(featureset_item->opt)
    {
      g_free(featureset_item->opt);
      featureset_item->opt = NULL;
    }


  if(featureset_item->gc)
    {
      g_object_unref(featureset_item->gc);
      featureset_item->gc = NULL;
    }

  {
    GtkObjectClass *gobj_class = (GtkObjectClass *) featureset_class_G;

    if(gobj_class->destroy)
      gobj_class->destroy (GTK_OBJECT(object));
  }

  return ;
}






#include <zmapWindowContainerGroup.h>
#include <zmapWindowContainerUtils.h>
/* to resize ourselves and reposition stuff to the right we have to resize the root */
/* There's a lot of container code that labouriously trundles up the tree, but each canvas item knows the root. so let's use that */
/* hmmmm... you need to call special invocations that set properties that then set flags... yet another run-around. */

void zMapWindowCanvasFeaturesetRequestReposition(FooCanvasItem *foo)
{
  /* container and item code is separate despite all of them having parent pointers */
  foo = (FooCanvasItem *) zmapWindowContainerCanvasItemGetContainer(foo);

  /* this finds the root slowly and sets a flag, slowly */
  zmapWindowContainerRequestReposition((ZMapWindowContainerGroup) foo);

  return ;
}



