/*  File: zmapWindowFeaturesetItem.c
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
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
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
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
#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasItemFeatureSet_I.h>
#include <zmapWindowCanvasBasic.h>
#include <zmapWindowCanvasGlyph.h>
#include <zmapWindowCanvasAlignment.h>

static void zmap_window_featureset_item_item_class_init  (ZMapWindowFeaturesetItemClass featureset_class);
static void zmap_window_featureset_item_item_init        (ZMapWindowFeaturesetItem      item);

static void  zmap_window_featureset_item_item_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags);
static void  zmap_window_featureset_item_item_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags);
static double  zmap_window_featureset_item_item_point (FooCanvasItem *item, double x, double y, int cx, int cy, FooCanvasItem **actual_item);
static void  zmap_window_featureset_item_item_bounds (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2);
static void  zmap_window_featureset_item_item_draw (FooCanvasItem *item, GdkDrawable *drawable, GdkEventExpose *expose);
static void zmap_window_featureset_item_item_destroy     (GObject *object);

static ZMapWindowCanvasFeature zmapWindowCanvasFeatureAlloc(zmapWindowCanvasFeatureType type);
static void zmapWindowCanvasFeatureFree(gpointer thing);

static gint zMapFeatureNameCmp(gconstpointer a, gconstpointer b);
static gint zMapFeatureCmp(gconstpointer a, gconstpointer b);


static ZMapWindowFeaturesetItemClass featureset_class_G = NULL;
static FooCanvasItemClass *parent_class_G;

static void zmapWindowCanvasFeaturesetSetColours(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature);

static ZMapSkipList zmap_window_canvas_featureset_find_feature_index(ZMapWindowFeaturesetItem fi,ZMapFeature feature);
static ZMapWindowCanvasFeature zmap_window_canvas_featureset_find_feature(ZMapWindowFeaturesetItem fi,ZMapFeature feature);
void zmap_window_canvas_featureset_expose_feature(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs);






/* clip to expose region */
/* erm,,, clip to visble scroll regipn: else rectangles would get extra edges */
void zMap_draw_line(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset, gint cx1, gint cy1, gint cx2, gint cy2)
{
	/* for H or V lines we can clip easily */
#warning when transcripts are implemented we need to intersect the visible region

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


/* define feature specific functions here */
/* only access via wrapper functions to allow type checking */

/* paint one feature, all context needed is in the FeaturesetItem */
/* we need the expose regrion to clip at high zoom esp with peptide alignments */
static gpointer _featureset_paint_G[FEATURE_N_TYPE] = { 0 };
void zMapWindowCanvasFeaturesetPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable, GdkEventExpose *expose)
{
	void (*func) (ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable,  GdkEventExpose *expose);

	if(!feature || feature->type < 0 || feature->type >= FEATURE_N_TYPE)	/* chicken */
		return;

	func = _featureset_paint_G[feature->type];
	if(!func)
		return;

	func(featureset, feature, drawable, expose);
}


/* output any buffered paints: useful eg for poly-line */
/* paint function and flush must access data via FeaturesetItem */
static gpointer _featureset_flush_G[FEATURE_N_TYPE] = { 0 };
void zMapWindowCanvasFeaturesetPaintFlush(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable)
{
	void (*func) (ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable);

	if(!feature || feature->type < 0 || feature->type >= FEATURE_N_TYPE)
		return;

	func = _featureset_flush_G[feature->type];
	if(!func)
		return;

	func(featureset, feature, drawable);
}


/* get the total sequence extent of a simple or complex feature */
/* used by bumping; we allow force to simple for optional but silly bump modes */
static gpointer _featureset_extent_G[FEATURE_N_TYPE] = { 0 };
gboolean zMapWindowCanvasFeaturesetGetFeatureExtent(ZMapWindowCanvasFeature feature, gboolean complex, ZMapSpan span, double *width)
{
	void (*func) (ZMapWindowCanvasFeature feature, ZMapSpan span, double *width);

	if(!feature || feature->type < 0 || feature->type >= FEATURE_N_TYPE)
		return FALSE;

	func = _featureset_extent_G[feature->type];

	if(!complex)		/* reset any compund feature extents */
	{
		feature->y2 = feature->feature->x2;
	}

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


#if CANVAS_FEATURESET_LINK_FEATURE
/* link same name features in a feature type specific way */
/* must accept NULL as meaning clear local data */
static gpointer _featureset_link_G[FEATURE_N_TYPE] = { 0 };
int zMapWindowCanvasFeaturesetLinkFeature(ZMapWindowCanvasFeature feature)
{
	int (*func) (ZMapWindowCanvasFeature feature);

	if(!feature || feature->type < 0 || feature->type >= FEATURE_N_TYPE)
		return FALSE;

	func = _featureset_link_G[feature->type];
	if(!func)
		return FALSE;

	return func(feature);
}
#endif





/*
 * do anything that needs doing on zoom
 * we have column specific things like are features visible
 * and feature specific things like recalculate gapped alignment display
 */
static gpointer _featureset_zoom_G[FEATURE_N_TYPE] = { 0 };
void zMapWindowCanvasFeaturesetZoom(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable)
{
	int (*func) (ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature);
	ZMapSkipList sl;
	long trigger = (long) zMapStyleGetSummarise(featureset->style);
	PixRect pix = NULL;

	if(!featureset)
		return;

		/* featureset specific */
	if(!featureset->bumped && trigger && featureset->n_features >= trigger)
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
		for(sl = zMapSkipListFirst(featureset->display_index); sl; sl = sl->next)
		{
			ZMapWindowCanvasFeature feature = (ZMapWindowCanvasFeature) sl->data;

			pix = zmapWindowCanvasFeaturesetSummarise(pix,featureset,feature);
		}

		/* clear up */
		zmapWindowCanvasFeaturesetSummariseFree(featureset, pix);
	}


		/* feature specific eg bumped gapped alignments - adjust gaps display or graph /re-bin */
	for(sl = zMapSkipListFirst(featureset->display_index); sl; sl = sl->next)
	{
		ZMapWindowCanvasFeature feature = (ZMapWindowCanvasFeature) sl->data;	/* base struct of all features */

		if(feature->type < 0 || feature->type >= FEATURE_N_TYPE)
			continue;

		func = _featureset_zoom_G[feature->type];
		if(!func)
			continue;

		func(featureset,feature);
	}


	return;
}


/* each feature type defines its own functions */
/* if they inherit from another type then they must include that type's headers and call code directly */

void zMapWindowCanvasFeatureSetSetFuncs(int featuretype,gpointer *funcs, int struct_size)
{
	_featureset_paint_G[featuretype] = funcs[FUNC_PAINT];
	_featureset_flush_G[featuretype] = funcs[FUNC_FLUSH];
	_featureset_extent_G[featuretype] = funcs[FUNC_EXTENT];
	_featureset_zoom_G[featuretype] = funcs[FUNC_ZOOM];
#if CANVAS_FEATURESET_LINK_FEATURE
	_featureset_link_G[featuretype] = funcs[FUNC_LINK];
#endif
	featureset_class_G->struct_size[featuretype] = struct_size;
}


/* fetch all the funcs we know about
 * there's no clean way to do this without everlasting overhead per feature
 * NB: canvas featuresets could get more than one type of feature
 * we know how many there are due to the zmapWindowCanvasFeatureType enum
 * so we just do them all, once, from the featureset class init
 * rather tediously that means we have to include headers for each type
 * In terms of OO 'classiness' we regard CanvasFeatureSet as the big daddy that has a few dependant children
 * they dont get to evolve unknown to thier parent.
 */
void featureset_init_funcs(void)
{
		/* set size of iunspecifed features structs to just the base */
	featureset_class_G->struct_size[FEATURE_INVALID] = sizeof(zmapWindowCanvasFeatureStruct);

	zMapWindowCanvasBasicInit();		/* the order of these may be important */
	zMapWindowCanvasGlyphInit();
	zMapWindowCanvasAlignmentInit();

	/* if you add a new one then update feature_types[N_STYLE_MODE] below */
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

      group_type = g_type_register_static(foo_canvas_item_get_type(),
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
 * we also allow several featuresets to map into the same canvas item via a vitueal featureset (quark)
 */
ZMapWindowCanvasItem zMapWindowFeaturesetItemGetFeaturesetItem(FooCanvasGroup *parent, GQuark id, int start,int end, ZMapFeatureTypeStyle style, ZMapStrand strand, ZMapFrame frame, int index)
{
      ZMapWindowFeaturesetItem di = NULL;
      FooCanvasItem *foo  = NULL,*interval;

            /* class not intialised till we make an item in foo_canvas_item_new() below */
      if(featureset_class_G && featureset_class_G->featureset_items)
            foo = (FooCanvasItem *) g_hash_table_lookup( featureset_class_G->featureset_items, GUINT_TO_POINTER(id));
      if(foo)
      {
            return((ZMapWindowCanvasItem) foo);
      }
      else
      {
            /* need a wrapper to get ZWCI with a foo_featureset inside it */
            foo = foo_canvas_item_new(parent, ZMAP_TYPE_WINDOW_CANVAS_FEATURESET_ITEM, NULL);

            interval = foo_canvas_item_new((FooCanvasGroup *) foo, ZMAP_TYPE_WINDOW_FEATURESET_ITEM, NULL);

            di = (ZMapWindowFeaturesetItem) interval;
            di->id = id;
            g_hash_table_insert(featureset_class_G->featureset_items,GUINT_TO_POINTER(id),(gpointer) foo);
zMapLogWarning("make featureset item %s",g_quark_to_string(di->id));


		/* we record strand and frame for display colours
		 * the features get added to the appropriate column depending on strand, frame
		 * so we get the right featureset item foo item implicitly
		 */
		di->strand = strand;
		di->frame = frame;
		di->style = style;

		di->overlap = TRUE;
		if(zMapStyleGetMode(style) == ZMAPSTYLE_MODE_GRAPH && zMapStyleDensity(style))
			di->overlap = FALSE;

		di->width = zMapStyleGetWidth(di->style);
  		di->start = start;
  		di->end = end;

		/* set our bounding box in canvas coordinates to be the whole column */
		foo_canvas_item_request_update (interval);
      }
      return ((ZMapWindowCanvasItem) foo);
}




/* this is a nest of insxestuous functions that all do the same thing
 * but at least this way we keep the search criteria in one place not 20
 */
static ZMapSkipList zmap_window_canvas_featureset_find_feature_coords(ZMapWindowFeaturesetItem fi, double y1, double y2)
{
	ZMapSkipList sl;
	zmapWindowCanvasFeatureStruct search;

	search.y1 = y1;
	search.y2 = y2;

	if(fi->overlap)
	{
		if(fi->bumped)
		  	search.y1 -= fi->bump_overlap;
		else
			search.y1 -= fi->longest;

		if(search.y1 < fi->start)
			search.y1 = fi->start;
	}

	sl =  zMapSkipListFind(fi->display_index, zMapFeatureCmp, &search);
//	if(sl->prev)
//		sl = sl->prev;	/* in case of not exact match when rebinned... done by SkipListFind */

	return sl;
}


static ZMapSkipList zmap_window_canvas_featureset_find_feature_index(ZMapWindowFeaturesetItem fi,ZMapFeature feature)
{
	ZMapWindowCanvasFeature gs;
	ZMapSkipList sl;

	sl = zmap_window_canvas_featureset_find_feature_coords(fi,feature->x1,feature->x2);

	/* find the feature's feature struct but return the skip list */

	while(sl)
	{
		gs = sl->data;

		/* if we got rebinned then we need to find the bin surrounding the feature
		   if the feature is split bewteeen bins just choose one
		 */
		if(gs->y1 > feature->x2)
			return NULL;

/* don't we know we have a feature and item that both exist? There must have been a reason for this w/ DensityItems */
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
#warning lookup exact feature may fail if rebinned
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
	ZMapSkipList sl = zmap_window_canvas_featureset_find_feature_index(fi,feature);
	ZMapWindowCanvasFeature gs = NULL;

	if(sl)
		gs = (ZMapWindowCanvasFeature) sl->data;

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
		 * really ought to work out max glyph size or rather have true featrue extent
		 * NOTE this is only currently used via OTF remove exisitng features
		 */
	foo_canvas_request_redraw (foo->canvas, cx1, cy1-8, cx2 + 1, cy2 + 8);
}



/* interface design driven by exsiting application code */
void zmapWindowFeaturesetItemSetColour(ZMapWindowCanvasItem   item,
						      FooCanvasItem         *interval,
						      ZMapFeature			feature,
						      ZMapFeatureSubPartSpan sub_feature,
						      ZMapStyleColourType    colour_type,
							int colour_flags,
						      GdkColor              *default_fill,
                                          GdkColor              *default_border)
{
	ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) interval;
	ZMapWindowCanvasFeature gs;

	gs = zmap_window_canvas_featureset_find_feature(fi,feature);

	if(gs)
	{
		/* set the focus flags (on or off) */
		/* zmapWindowFocus.c maintans these and we set/clear then all at once */
		/* NOTE some way up the call stack in zmapWindowFocus.c add_unique()
		 * colour flags has a window id set into it
		 */

		gs->flags = (gs->flags & ~FEATURE_FOCUS_BITMAP) | (colour_flags & FEATURE_FOCUS_BITMAP);
		gs->flags = (gs->flags & ~FEATURE_FOCUS_ID) | (colour_flags & FEATURE_FOCUS_ID);
		zmap_window_canvas_featureset_expose_feature(fi, gs);
		return;
	}
}


/* NOTE this function interfaces to user show/hide via zmapWindow/unhideItemsCB() and zmapWindowFocus/hideFocusItemsCB()
 * and could do other things if appropriate if we include some other flags
 * current best guess is that this is not best practice: eg summarise is an internal function and does not need an external interface
 */
void zmapWindowFeaturesetItemShowHide(FooCanvasItem *foo, ZMapFeature feature, gboolean show)
{
	ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) foo;
	ZMapWindowCanvasFeature gs;

	gs = zmap_window_canvas_featureset_find_feature(fi,feature);

	if(gs)
	{
		if(show)
			gs->flags &= ~FEATURE_HIDDEN & ~FEATURE_HIDE_REASON;
		else
			gs->flags |= FEATURE_HIDDEN | FEATURE_USER_HIDE;

		zmap_window_canvas_featureset_expose_feature(fi, gs);
	}
}


#if 0
// for bump_style only

gboolean zMapWindowFeaturesetItemSetStyle(ZMapWindowFeaturesetItem di, ZMapFeatureTypeStyle style)
{
	GdkColor *draw = NULL, *fill = NULL, *outline = NULL;
	FooCanvasItem *foo = (FooCanvasItem *) di;

	if(di->display_index && zMapStyleDensityMinBin(di->style) != zMapStyleDensityMinBin(style))
	{
		zMapSkipListDestroy(di->display_index,
			di->source_used ? NULL : zmapWindowCanvasFeatureFree);
		di->display_index = NULL;
	}

	di->style = style;		/* includes col width */
	di->x_off = zMapStyleDensityStagger(style) * di->index;

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

	foo_canvas_item_request_update ((FooCanvasItem *) di);
	return TRUE;
}
#endif




/* NOTE this extends FooCanvasItem not ZMapWindowCanvasItem */
static void zmap_window_featureset_item_item_class_init(ZMapWindowFeaturesetItemClass featureset_class)
{
  GObjectClass *gobject_class ;
  FooCanvasItemClass *item_class;

  featureset_class_G = featureset_class;
  featureset_class_G->featureset_items = g_hash_table_new(NULL,NULL);

  parent_class_G = gtk_type_class (foo_canvas_item_get_type ());

  gobject_class = (GObjectClass *) featureset_class;

  item_class = (FooCanvasItemClass *) featureset_class;

  gobject_class->dispose = zmap_window_featureset_item_item_destroy;

  item_class->update = zmap_window_featureset_item_item_update;
  item_class->bounds = zmap_window_featureset_item_item_bounds;
  item_class->point  = zmap_window_featureset_item_item_point;
  item_class->draw   = zmap_window_featureset_item_item_draw;

//  zmapWindowItemStatsInit(&(canvas_class->stats), ZMAP_TYPE_WINDOW_FEATURESET_ITEM) ;

  featureset_init_funcs();

  return ;
}


/* replaces a g_object_get_data() call and returns a static data struct */
/* only used by the status bar if you click on a sub-feature */
ZMapFeatureSubPartSpan zMapWindowCanvasFeaturesetGetSubPartSpan(FooCanvasItem *foo,ZMapFeature feature,double x,double y)
{
	static ZMapFeatureSubPartSpanStruct sub_part;
	ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) foo;

	switch(feature->type)
	{
	case ZMAPSTYLE_MODE_ALIGNMENT:
		{
			ZMapAlignBlock ab;
			int i;

			/* find the gap or match if we are bumped */
			if(!fi->bumped)
				return NULL;
			if(!feature->feature.homol.align)	/* is un-gapped */
				return NULL;

			/* get sequence coords for x,y,  well y at least */
			/* AFAICS y is a world coordinate as the caller runs it through foo_w2c() */


			/* we refer to the actual feature gaps data not the display data
			* as that may be compressed and does not contain sequence info
			* return the type index and target start and end
			*/
			for(i = 0; i < feature->feature.homol.align->len;i++)
			{
				ab = &g_array_index(feature->feature.homol.align, ZMapAlignBlockStruct, i);
				/* in the original foo based code
				 * match n corresponds to the match block indexed from 1
				 * gap n corresponds to the following match block
				 */

				sub_part.index = i + 1;
				sub_part.start = ab->t1;
				sub_part.end = ab->t2;

				if(y >= ab->t1 && y <= ab->t2)
				{
					sub_part.subpart = ZMAPFEATURE_SUBPART_MATCH;
					return &sub_part;
				}
				if(y < ab->t1)
				{
					sub_part.start = sub_part.end + 1;
					sub_part.end = ab->t1 -1;
					sub_part.subpart = ZMAPFEATURE_SUBPART_GAP;
					return &sub_part;
				}
			}
		}
		break;

	default:
		break;
	}
	return NULL;
}



static void zmap_window_featureset_item_item_init (ZMapWindowFeaturesetItem featureset)
{
  return ;
}

static void zmap_window_featureset_item_item_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags)
{

      ZMapWindowFeaturesetItem di = (ZMapWindowFeaturesetItem) item;
      int cx1,cy1,cx2,cy2;
      double x1,x2,y1,y2;

	if (parent_class_G->update)
		(* parent_class_G->update) (item, i2w_dx, i2w_dy, flags);

	// cribbed from FooCanvasRE; this sets the canvas coords in the foo item
	di->dx = x1 = i2w_dx;
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



/* how far are we from the cursor? */
/* can't return foo canvas item for the feature as they are not in the canvas,
 * so return the featureset foo item adjusted to point at the nearest feature */

 /* by a process of guesswork x,y are world coordinates and cx,cy are canvas (i think) */
 /* No: x,y are parent item local coordinates ie offset within the group
  * we have a ZMapCanvasItem group with no offset, s we nee to adjust by the x,ypos of that group
  */
double  zmap_window_featureset_item_item_point (FooCanvasItem *item, double x, double y, int cx, int cy, FooCanvasItem **actual_item)
{
	ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) item;
	ZMapWindowCanvasFeature gs;
	ZMapSkipList sl;
      double wx; //,wy;
//      double dx,dy;
      double y1,y2;
//      FooCanvasGroup *group;

      /* optimise repeat calls: the foo canvas does 6 calls for a click event (3 down 3 up)
       * and if we are zoomed into a bumped peptide alignemnt column that means looking at a lot of features
       * each one goes through about 12 layers of canvas containers first but that's another issue
       * then if we move the lassoo that gets silly (button down: calls point())
       */
      static double save_x,save_y;
      static ZMapWindowCanvasFeature save_gs = NULL;
	static double best = 0.0;

      /*
       * need to scan internal list and apply close enough rules
       */

       /* zmapSkipListFind();		 gets exact match to start coord or item before
	   if any feature overlaps choose that
	   (assuming non overlapping features)
	   else choose nearest of next and previous
        */
      *actual_item = NULL;

      if(save_gs && x == save_x && y == save_y)
      {
      	fi->point_feature = save_gs->feature;
      	*actual_item = item;

      	return best;
      }

	save_x = x;
	save_y = y;
      save_gs = NULL;

	x += fi->dx;
	y += fi->dy;
#warning this code is in the wrong place, review when containers rationalised
/*
 * The foo canvas calls point() with group relative coordinates ad to the
 * world coordinates ie our feature coordinates we have to add the containign group's x,ypos
 * this should be accounted for by fi->dx,y as that's derived from a _i2w call
 * but it doesn't work. So this is a bodge to make the cursor work. There must be another
 * mistake/ bodge somewhere requiring this.
 * but we need to get the column containing our ZmapCanvasItem
 * or is that the container that conatind the features that contains the list of conatiners that contain out ZmapCanvasItem?
 * regardless this has no effect.

 	group = FOO_CANVAS_GROUP(item->parent);
 	group = FOO_CANVAS_GROUP((FOO_CANVAS_ITEM(group)->parent));
 	x += group->xpos;
 	y += group->ypos;
 */
	y1 = y - item->canvas->close_enough;
	y2 = y + item->canvas->close_enough;


	/* NOTE there is a flake in world coords at low zoom */
	/* NOTE close_enough is zero */
	sl = zmap_window_canvas_featureset_find_feature_coords(fi, y1 , y2);

	if(!sl)
		return(0.0);

	for(; sl ; sl = sl->next)
	{
		gs = (ZMapWindowCanvasFeature) sl->data;

		if(gs->flags & FEATURE_HIDDEN)
			continue;

		if(gs->y1 > y2)
			break;

		if(gs->feature->x1 <= y && gs->feature->x2 >= y)	/* overlaps cursor */
		{
			wx = fi->dx;
			if(fi->bumped)
				wx += gs->bump_offset;

			if(x >= wx && x < wx + gs->width)	/* item contains cursor */
			{
				best = 0.0;
				fi->point_feature = gs->feature;
				*actual_item = item;

#warning this could concievably cause a memory fault if we freed save_gs but that seems unlikely if we don-t nove the cursor
      			save_gs = gs;
				break;			/* just find any one */
			}
		}
	}
      return best;
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
void zmap_window_featureset_item_link_sideways(ZMapWindowFeaturesetItem fi)
{
	GList *l;
	ZMapWindowCanvasFeature left,right;		/* feat -ures */
#if !CANVAS_FEATURESET_LINK_FEATURE
	GQuark name = 0;
#endif

	/* we use the featureset features list which sits there in parallel with the skip list (display index) */
	/* sort by name and start coord */
	/* link same name features with ascending query start coord */

	/* if we ever need to link by something other than same name
	 * then we can define a feature type specific sort function
	 * and revive the zMapWindowCanvasFeaturesetLinkFeature() fucntion
	 */
	fi->features = g_list_sort(fi->features,zMapFeatureNameCmp);

#if CANVAS_FEATURESET_LINK_FEATURE
	zMapWindowCanvasFeaturesetLinkFeature(NULL);	/* clear out static data */
#endif

	for(l = fi->features;l;l = l->next)
	{
		right = (ZMapWindowCanvasFeature) l->data;
		right->left = right->right = NULL;		/* we can re-calculate so must zero */

zMapAssert(right != left);

#if CANVAS_FEATURESET_LINK_FEATURE
		if(zMapWindowCanvasFeaturesetLinkFeature(right))
#else
		if(name == right->feature->original_id)
#endif
		{
			right->left = left;
			left->right = right;
		}
		left = right;
#if !CANVAS_FEATURESET_LINK_FEATURE
		name = left->feature->original_id;
#endif

	}

	fi->linked_sideways = TRUE;
}


void zMapWindowCanvasFeaturesetIndex(ZMapWindowFeaturesetItem fi)
{
	if(fi->link_sideways && !fi->linked_sideways)
		zmap_window_featureset_item_link_sideways(fi);

    fi->features = g_list_sort(fi->features,zMapFeatureCmp);
    fi->display_index = zMapSkipListCreate(fi->features, NULL);
}


void  zmap_window_featureset_item_item_draw (FooCanvasItem *item, GdkDrawable *drawable, GdkEventExpose *expose)
{
	ZMapSkipList sl;
	ZMapWindowCanvasFeature feat;
      double y1,y2;
      double width;
      GList *highlight = NULL;	/* must paint selected on top ie last */

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
		fi->clip_x2 =rect.x + rect.width + 1;
		fi->clip_y2 = rect.y + rect.height + 1;
	}


      if(!fi->display_index)
	      zMapWindowCanvasFeaturesetIndex(fi);

	if(!fi->display_index)
      	return; 		/* could be an empty column or a mistake */

	/* check zoom level and recalculate */
	if(fi->zoom != item->canvas->pixels_per_unit_y)
	{
		fi->zoom = item->canvas->pixels_per_unit_y;
		fi->bases_per_pixel = 1.0 / fi->zoom;
		zMapWindowCanvasFeaturesetZoom(fi, drawable);
	}


      /* paint all the data in the exposed area */

	if(!fi->gc && (item->object.flags & FOO_CANVAS_ITEM_REALIZED))
		fi->gc = gdk_gc_new (item->canvas->layout.bin_window);
	if(!fi->gc)
		return;		/* got a draw before realize ?? */

      width = zMapStyleGetWidth(fi->style) - 1;		/* off by 1 error! width = #pixels not end-start */

	/*
	 *	get the exposed area
	 *	find the top (and bottom?) items
	 *	clip the extremes and paint all
	 */

	fi->dx = fi->dy = 0.0;		/* this gets the offset of the parent of this item */
      foo_canvas_item_i2w (item, &fi->dx, &fi->dy);

	foo_canvas_c2w(item->canvas,0,floor(expose->area.y),NULL,&y1);
	foo_canvas_c2w(item->canvas,0,ceil(expose->area.y + expose->area.height),NULL,&y2);

//printf("expose %s %d-%d, dx,y = %f,%f\n",g_quark_to_string(fi->id),(int) search.y1,(int) search.y2,fi->dx,fi->dy);

	sl = zmap_window_canvas_featureset_find_feature_coords(fi, y1, y2);
	if(!sl)
		return;

		/* we have already found the first matching or previous item */

	for(fi->featurestyle = NULL;sl;sl = sl->next)
	{
		feat = (ZMapWindowCanvasFeature) sl->data;
//printf("found %d-%d\n",(int) feat->y1,(int) feat->y2);

		if(feat->y1 > y2)
			break;	/* finished */
		if(feat->y2 < y1)
		{
		    /* if bumped and complex then the first feature does the join up lines */
		    if(!fi->bumped || feat->left)
			continue;
		}

		if((feat->flags & FEATURE_HIDDEN))
		{
			continue;
		}
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

		if((feat->flags & FEATURE_FOCUS_MASK))
		{
			highlight = g_list_prepend(highlight, feat);
			continue;
		}

		/* clip this one (GDK does that? or is it X?) and paint */
//printf("paint %d-%d\n",(int) feat->y1,(int) feat->y2);

		/* set style colours if they changed */
		zmapWindowCanvasFeaturesetSetColours(fi,feat);

		// call the paint function for the feature
		zMapWindowCanvasFeaturesetPaintFeature(fi,feat,drawable,expose);
	}

	/* flush out any stored data (eg if we are drawing polylines) */
	zMapWindowCanvasFeaturesetPaintFlush(fi,feat,drawable);


	highlight = g_list_reverse(highlight);	/* preserve normal display ordering */

	/* now paint the focus features on top, clear style to force colours lookup */
	for(fi->featurestyle = NULL;highlight;highlight = highlight->next)
	{
		feat = (ZMapWindowCanvasFeature) highlight->data;

		zmapWindowCanvasFeaturesetSetColours(fi,feat);
		zMapWindowCanvasFeaturesetPaintFeature(fi,feat,drawable,expose);
	}
	zMapWindowCanvasFeaturesetPaintFlush(fi,feat,drawable);
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
int zMapWindowCanvasFeaturesetGetColours(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, gulong *fill_pixel,gulong *outline_pixel)
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
 * so we sould have to post process the featureset->features list
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



/* allocate a free list for an unknown structure */
static ZMapWindowCanvasFeature zmapWindowCanvasFeatureAlloc(zmapWindowCanvasFeatureType type)
{
      GList *l;
      gpointer mem;
      int size;
      ZMapWindowCanvasFeature feat;

	size = featureset_class_G->struct_size[type];
	if(!size)
	{
		type = FEATURE_INVALID;		/* catch all for simple features */
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
      }
      zMapAssert(featureset_class_G->feature_free_list[type]);

      l = featureset_class_G->feature_free_list[type];
      feat = (ZMapWindowCanvasFeature) l->data;
      featureset_class_G->feature_free_list[type] = g_list_delete_link(featureset_class_G->feature_free_list[type],l);

      /* these can get re-allocated so must zero */
      memset((gpointer) feat,0,size);

      feat->type = type;

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
}




/* sort by name and the start coord to link same name features */
/* note that ultimately we are interested in query coords in a zmapHomol struct
 * but only after getting alignments in order on the genomic sequence
 */
static gint zMapFeatureNameCmp(gconstpointer a, gconstpointer b)
{
	ZMapWindowCanvasFeature feata = (ZMapWindowCanvasFeature) a;
	ZMapWindowCanvasFeature featb = (ZMapWindowCanvasFeature) b;

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



/* sort by genomic coordinate for display purposes */
/* start coord then end coord reversed, mainly for summarise fucntion */
static gint zMapFeatureCmp(gconstpointer a, gconstpointer b)
{
	ZMapWindowCanvasFeature feata = (ZMapWindowCanvasFeature) a;
	ZMapWindowCanvasFeature featb = (ZMapWindowCanvasFeature) b;

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




/* cribbed from zmapWindowGetPosFromScore(() which is called 3x from Item Factory and will eventaully get removed */
double zmap_window_featureset_item_set_width_from_score(ZMapFeatureTypeStyle style, double width, double score)
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




/* this is in parallel with the ZMapStyleMode enum */
/* beware, traditionally glyphs ended up as basic features */
/* This is a retro-fit ... i noticed that i'd defien a struct/feature type but not set it
 * as orignally there was only one (blush) -> is that brown bag coding?? */
static zmapWindowCanvasFeatureType feature_types[N_STYLE_MODE] =
{
	FEATURE_INVALID,		/* ZMAPSTYLE_MODE_INVALID */
	FEATURE_BASIC, 		/* ZMAPSTYLE_MODE_BASIC */
	FEATURE_ALIGN,		/* ZMAPSTYLE_MODE_ALIGNMENT */
	FEATURE_INVALID,		/* ZMAPSTYLE_MODE_TRANSCRIPT */
	FEATURE_INVALID,		/* ZMAPSTYLE_MODE_SEQUENCE */
	FEATURE_INVALID,		/* ZMAPSTYLE_MODE_ASSEMBLY_PATH */
	FEATURE_INVALID,		/* ZMAPSTYLE_MODE_TEXT */
	FEATURE_INVALID,		/* ZMAPSTYLE_MODE_GRAPH */
	FEATURE_GLYPH,		/* ZMAPSTYLE_MODE_GLYPH */
	FEATURE_INVALID		/* ZMAPSTYLE_MODE_META */
};

/* no return, as all this data in internal to the column featureset item */
void zMapWindowFeaturesetAddFeature(FooCanvasItem *foo, ZMapFeature feature, double y1, double y2)
{
  ZMapWindowCanvasFeature feat;
  ZMapFeatureTypeStyle style = feature->style;
  ZMapWindowFeaturesetItem featureset_item;
  zmapWindowCanvasFeatureType type = FEATURE_INVALID;

  if(style)
  	type = feature_types[zMapStyleGetMode(feature->style)];
  if(type == FEATURE_INVALID)		/* no style or feature type not implemented */
  	return;

  featureset_item = (ZMapWindowFeaturesetItem) foo;

  feat = zmapWindowCanvasFeatureAlloc(type);
  feat->feature = feature;
  feat->type = type;
  if(type == FEATURE_ALIGN)
  {
  	featureset_item->link_sideways = TRUE;

  	if(feat->feature->feature.homol.flags.masked)
  		feat->flags |= focus_group_mask[WINDOW_FOCUS_GROUP_MASKED];
  }

  feat->y1 = y1;
  feat->y2 = y2;

	/* NOTE maybe this is not generic enough eg for glyphs, graphs */
  if ((zMapStyleGetScoreMode(style) == ZMAPSCORE_WIDTH && feature->flags.has_score))
	  feat->width = zmap_window_featureset_item_set_width_from_score(style, featureset_item->width, feature->score);
  else if(zMapStyleGetScoreMode(style) == ZMAPSCORE_PERCENT)
	  feat->width = zmap_window_featureset_item_set_width_from_score(style, featureset_item->width, feature->feature.homol.percent_id);
  else
  	feat->width = featureset_item->width;

  if(y2 - y1 > featureset_item->longest)
	featureset_item->longest = y2 - y1;

	  /* even if they come in order we still have to sort them to be sure so just add to the front */
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
#else
  	{
  		featureset_item->display_index =
  			zMapSkipListAdd(featureset_item->display_index, zMapFeatureCmp, feat);
#warning need to fix linked_sideways
  	}
#endif
  }
  /* NOTE we may not have an index so this flag must be unset seperately */
  /* eg on OTF w/ delete existing selected */
  featureset_item->linked_sideways = FALSE;
}



/*
	delete feature from the features list and trash the index
	returns no of features left

	rather tediously we can't get the feature via the index as that will give us the feature not the list node pointing to it.
	so to delete a whole featureset we could have a quadratic search time unless we delete in order
	but from OTF if we delete old ones we do this via a small hash table
	we don't delete elsewhere, execpt for legacy gapped alignments, so this works ok by fluke

	this function's a bit horrid: when we find the feature to delete we have to look it up in the index to repaint
	we really need a coplumn refresh

	we really need to rethink this: deleting was not considered
*/
int zMapWindowFeaturesetItemRemoveFeature(FooCanvasItem *foo, ZMapFeature feature)
{
	ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) foo;
	GList *l;
	ZMapWindowCanvasFeature feat;

	for(l = fi->features;l;l = l->next)
	{
		feat = (ZMapWindowCanvasFeature) l->data;
		if(feat->feature == feature)
		{
zMapLogWarning("OTF CFS delete %s of %d",g_quark_to_string(feature->unique_id),fi->n_features);
			ZMapWindowCanvasFeature gs = zmap_window_canvas_featureset_find_feature(fi,feature);
			if(gs)
			{
zMapLogWarning("OTF CFS expose %s",g_quark_to_string(feature->unique_id));
				zmap_window_canvas_featureset_expose_feature(fi, gs);
			}

			zmapWindowCanvasFeatureFree(feat);
			fi->features = g_list_delete_link(fi->features,l);
			fi->n_features--;
			break;
		}
	}

	if(fi->display_index)
	{
		/* need to recalc bins */
		/* quick fix FTM, de-calc which requires a re-calc on display */
		zMapSkipListDestroy(fi->display_index, NULL);
		fi->display_index = NULL;
	}
	/* NOTE we may not have an index so this flag must be unset seperately */
	fi->linked_sideways = FALSE;

	return fi->n_features;
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

zMapLogWarning("OTF CFS destroy %s",g_quark_to_string(featureset_item->id));
  if(featureset_item->display_index)
  {
  	zMapSkipListDestroy(featureset_item->display_index, NULL);
	featureset_item->display_index = NULL;
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

//  printf("removing %s\n",g_quark_to_string(featureset->id));


  if (GTK_OBJECT_CLASS (parent_class_G)->destroy)
	(* GTK_OBJECT_CLASS (parent_class_G)->destroy) (GTK_OBJECT(object));

  return ;
}



