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
  * for basic features (the first type implemented it runs about 3-5x faster than the foo canvas
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

static void zmap_window_featureset_item_item_class_init  (ZMapWindowFeaturesetItemClass featureset_class);
static void zmap_window_featureset_item_item_init        (ZMapWindowFeaturesetItem      item);

static void  zmap_window_featureset_item_item_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags);
static void  zmap_window_featureset_item_item_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags);
static double  zmap_window_featureset_item_item_point (FooCanvasItem *item, double x, double y, int cx, int cy, FooCanvasItem **actual_item);
static void  zmap_window_featureset_item_item_bounds (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2);
static void  zmap_window_featureset_item_item_draw (FooCanvasItem *item, GdkDrawable *drawable, GdkEventExpose *expose);
static void zmap_window_featureset_item_item_destroy     (GObject *object);

static gint zMapFeatureCmp(gconstpointer a, gconstpointer b);


static ZMapWindowFeaturesetItemClass featureset_class_G = NULL;
static FooCanvasItemClass *parent_class_G;

static void zmapWindowCanvasFeaturesetSetColours(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature);

static ZMapSkipList zmap_window_canvas_featureset_find_feature_index(ZMapWindowFeaturesetItem fi,ZMapFeature feature);
static ZMapWindowCanvasFeature zmap_window_canvas_featureset_find_feature(ZMapWindowFeaturesetItem fi,ZMapFeature feature);
void zmap_window_canvas_featureset_expose_feature(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs);


/* define feature specific functions here */
/* only access via wrapper functions to allow type checking */

/* paint one feature, all context needed is in the FeaturesetItem */
static gpointer _featureset_paint_G[FEATURE_N_TYPE] = { 0 };
static void zMapWindowCanvasFeaturesetPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable)
{
	static void (*func) (ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable);

	if(!feature || feature->type < 0 || feature->type >= FEATURE_N_TYPE)	/* chicken */
		return;

	func = _featureset_paint_G[feature->type];
	if(!func)
		return;

	func(featureset, feature, drawable);
}


/* output any buffered paints: useful eg for poly-line */
/* paint function and flush must access data via FeaturesetItem */
static gpointer _featureset_flush_G[FEATURE_N_TYPE] = { 0 };
static void zMapWindowCanvasFeaturesetPaintFlush(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable)
{
	static void (*func) (ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable);

	if(!feature || feature->type < 0 || feature->type >= FEATURE_N_TYPE)
		return;

	func = _featureset_flush_G[feature->type];
	if(!func)
		return;

	func(featureset, feature, drawable);
}



/* each feature type defines its own functions */
/* if they inherit from another type then they must include that type's headers and call code directly */

void zMapWindowCanvasFeatureSetSetFuncs(int featuretype,gpointer *funcs)
{
	_featureset_paint_G[featuretype] = funcs[FUNC_PAINT];
	_featureset_flush_G[featuretype] = funcs[FUNC_FLUSH];
}


/* fetch all the funcs we know about
 * there's no clean way to do this without everlasting overhead per feature
 * NB: canvas featuresets could get more than one type of feature
 * we know how many there are due to the zmapWindowCanvasFeatureType enum
 * so we just do them all, once, from the featureset class init
 * rather tediously that means we have to include headers for each type
 * In terms of OO 'classiness' we reagrd CanvasFeatureSet as the big daddy that has a few dependant children
 * they dont get to evolve unknown to thier parent.
 */
void featureset_init_funcs(void)
{
	zMapWindowCanvasBasicInit();
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
	GdkColor *fill = NULL,*draw = NULL, *outline = NULL;


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
//printf("make featureset item %s\n",g_quark_to_string(di->id));


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
  		di->width_pixels = TRUE;
  		di->start = start;
  		di->end = end;

		/* set our bounding box in canvas coordinates to be the whole column */
		foo_canvas_item_request_update (interval);

		zmapWindowCanvasItemGetColours(style, strand, frame, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill, &draw, &outline, NULL, NULL);

		if(fill)
		{
			di->fill_set = TRUE;
			di->fill_colour = zMap_gdk_color_to_rgba(fill);
			di->fill_pixel = foo_canvas_get_color_pixel(foo->canvas, di->fill_colour);
		}
		if(outline)
		{
			di->outline_set = TRUE;
			di->outline_colour = zMap_gdk_color_to_rgba(outline);
			di->outline_pixel = foo_canvas_get_color_pixel(foo->canvas, di->outline_colour);
		}
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

	foo_canvas_request_redraw (foo->canvas, cx1, cy1, cx2, cy2);
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
		gs->flags = (gs->flags & ~FEATURE_FOCUS_MASK) | (colour_flags & FEATURE_FOCUS_MASK);
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
			gs->flags &= ~FEATURE_HIDDEN & ~FEATURE_USER_HIDE;
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
	x1 = i2w_dx;
	x2 = x1 + (di->bumped? di->bump_width : di->width);

	y1 = i2w_dy;
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
double  zmap_window_featureset_item_item_point (FooCanvasItem *item, double x, double y, int cx, int cy, FooCanvasItem **actual_item)
{
	double best = 0.0;
	ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) item;
	ZMapWindowCanvasFeature gs;
	ZMapSkipList sl;
//      double dist = 0.0;
      double wx; //,wy;
      double dx,dy;
      double y1,y2;

      /*
       * need to scan internal list and apply close enough rules
       */

       /* zmapSkipListFind();		 gets exact match to start coord or item before
	   if any feature overlaps choose that
	   (assuming non overlapping features)
	   else choose nearest of next and previous
        */
      *actual_item = NULL;

	dx = dy = 0.0;
      foo_canvas_item_i2w (item, &dx, &dy);

	x += dx;
	y += dy;

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

		if(gs->y1 > y2)
			break;

		if(gs->y1 <= y && gs->y2 >= y)	/* overlaps cursor */
		{
			wx = item->x1;
			if(fi->bumped)
				wx += gs->bump_offset;

			if(x >= wx && x < wx + gs->width)	/* item contains cursor */
			{
				best = 0.0;
				*actual_item = item;

				fi->point_feature = gs->feature;
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



void  zmap_window_featureset_item_item_draw (FooCanvasItem *item, GdkDrawable *drawable, GdkEventExpose *expose)
{
	ZMapSkipList sl;
	ZMapWindowCanvasFeature feat;
      double y1,y2;
      double width;
      GList *highlight = NULL;	/* must paint selected on top ie last */

      ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) item;

	/* check zoom level and recalculate: was done here for density items */

      if(!fi->display_index)
      {
		fi->features = g_list_sort(fi->features,zMapFeatureCmp);
		fi->n_features = g_list_length(fi->features);
      	fi->display_index = zMapSkipListCreate(fi->features, zMapFeatureCmp);
      }
      if(!fi->display_index)
      	return; 		/* could be an empty column or a mistake */

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

//printf("expose %d-%d, dx,y = %f,%f\n",(int) search.y1,(int) search.y2,fi->dx,fi->dy);

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
			continue;

		if((feat->flags & FEATURE_HIDDEN))
			continue;
		/*
		NOTE need to sort out container positioning to make this work
		di covers its container exactly, but is it offset??
		by analogy w/ old style ZMapWindowCanvasItems we should display
		'intervals' as item relative
		*/

		/* clip this one (GDK does that? or is it X?) and paint */
//printf("paint %d-%d\n",(int) feat->y1,(int) feat->y2);


		if((feat->flags & FEATURE_FOCUS_MASK))
		{
			highlight = g_list_prepend(highlight, feat);
			continue;
		}

		/* set style colours if they changed */
		zmapWindowCanvasFeaturesetSetColours(fi,feat);

		// call the paint function for the feature
		zMapWindowCanvasFeaturesetPaintFeature(fi,feat,drawable);
	}

	/* flush out any stored data (eg if we are drawing polylines) */
	zMapWindowCanvasFeaturesetPaintFlush(fi,feat,drawable);


	/* now paint the focus features on top, clear style to force colours lookup */
	for(fi->featurestyle = NULL;highlight;highlight = highlight->next)
	{
		feat = (ZMapWindowCanvasFeature) highlight->data;

		zmapWindowCanvasFeaturesetSetColours(fi,feat);
		zMapWindowCanvasFeaturesetPaintFeature(fi,feat,drawable);
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
	 * so we reset the style befoer doing focus
	 * other kond of focus do not have colours set in the style
	 * so mixed focus types are not a problem
	 */

/*	if(!(feat->flags & FEATURE_FOCUS_MASK) &&		 NOTE selected may use normal outline */

	if((fi->featurestyle != feat->feature->style))	/* diff style: set colour from style */
	{
		/* cache style for a single featureset
		* if we have one source featureset in the column then this works fastest (eg good for trembl/ swissprot)
		* if we have several (eg BAM rep1, rep2, etc,  Repeatmasker then we get to switch around frequently
		* but hopefully it's faster than not caching regardless
		* beware of enum ZMapWindowFocusType not being a bit-mask
		*/

		GdkColor *fill = NULL,*draw = NULL, *outline = NULL;

		fi->featurestyle = feat->feature->style;

		ct = feat->flags & WINDOW_FOCUS_GROUP_ALL ? ZMAPSTYLE_COLOURTYPE_SELECTED : ZMAPSTYLE_COLOURTYPE_NORMAL;

		zmapWindowCanvasItemGetColours(fi->featurestyle, fi->strand, fi->frame, ct , &fill, &draw, &outline, NULL, NULL);

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




ZMapWindowCanvasFeature zmapWindowCanvasFeatureAlloc(void)
{
      GList *l;
      ZMapWindowCanvasFeature feat;

      if(!featureset_class_G->feature_free_list)
      {
            int i;
            feat = g_new(zmapWindowCanvasFeatureStruct,N_FEAT_ALLOC);

            for(i = 0;i < N_FEAT_ALLOC;i++)
                  zmapWindowCanvasFeatureFree((gpointer) feat++);
      }
      zMapAssert(featureset_class_G->feature_free_list);

      l = featureset_class_G->feature_free_list;
      feat = (ZMapWindowCanvasFeature) l->data;
      featureset_class_G->feature_free_list = g_list_delete_link(featureset_class_G->feature_free_list,l);

      /* these can get re-allocated so must zero */
      memset((gpointer) feat,0,sizeof(zmapWindowCanvasFeatureStruct));
      return(feat);
}


/* need to be a ZMapSkipListFreeFunc for use as a callback */
void zmapWindowCanvasFeatureFree(gpointer thing)
{
	ZMapWindowCanvasFeature feat = (ZMapWindowCanvasFeature) thing;

      featureset_class_G->feature_free_list =
            g_list_prepend(featureset_class_G->feature_free_list, (gpointer) feat);
}







static gint zMapFeatureCmp(gconstpointer a, gconstpointer b)
{
	ZMapWindowCanvasFeature feata = (ZMapWindowCanvasFeature) a;
	ZMapWindowCanvasFeature featb = (ZMapWindowCanvasFeature) b;


	if(feata->y1 < featb->y1)
		return(-1);
	if(feata->y1 > featb->y1)
		return(1);
	if(feata->y2 < featb->y2)
		return(-1);
	if(feata->y2 > featb->y2)
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



/* no return, as all this data in internal to the column featureset item
 * bins get squashed according to zoom
 */
void zMapWindowFeaturesetAddItem(FooCanvasItem *foo, ZMapFeature feature, double y1, double y2)
{
  ZMapWindowCanvasFeature feat;
  ZMapFeatureTypeStyle style = feature->style;
  ZMapWindowFeaturesetItem featureset_item;

  zMapAssert(style);

  feat = zmapWindowCanvasFeatureAlloc();
  feat->feature = feature;

  featureset_item = (ZMapWindowFeaturesetItem) foo;

  feat->y1 = y1;
  feat->y2 = y2;

  if(feature->flags.has_score)
	feat->width = zmap_window_featureset_item_set_width_from_score(style, featureset_item->width, feature->score);
  else
  	feat->width = featureset_item->width;

  if(y2 - y1 > featureset_item->longest)
	featureset_item->longest = y2 - y1;

	  /* even if they come in order we still have to sort them to be sure so just add to the front */
  featureset_item->features = g_list_prepend(featureset_item->features,feat);
  featureset_item->n_features++;

  /* add to the display bins if index already created */
  if(featureset_item->display_index)
  {
  	/* have to re-sort... NB SkipListAdd() not exactly well tested, so be dumb */
  	/* it's very rare that we add features anyway */
  	/* although we could have several featuresets being loaded into one column */
  	/* whereby this may be more efficient ? */
  	if(1)
  	{
  		/* need to recalc bins */
  		/* quick fix FTM, de-calc which requires a re-calc on display */
  		zMapSkipListDestroy(featureset_item->display_index, zmapWindowCanvasFeatureFree);
  		featureset_item->display_index = NULL;
  	}
  	else
  	{
  		featureset_item->display_index =
  			zMapSkipListAdd(featureset_item->display_index, zMapFeatureCmp, feat);
  	}
  }
}



static void zmap_window_featureset_item_item_destroy     (GObject *object)
{

  ZMapWindowFeaturesetItem featureset_item;
  /* mh17 NOTE: this function gets called twice for every object via a very very tall stack */
  /* no idea why, but this is all harmless here if we make sure to test if pointers are valid */
  /* what's more interesting is why an object has to be killed twice */

//  printf("zmap_window_featureset_item_item_destroy %p\n",object);
  g_return_if_fail(ZMAP_IS_WINDOW_FEATURESET_ITEM(object));

  featureset_item = ZMAP_WINDOW_FEATURESET_ITEM(object);

  if(featureset_item->display_index)
  {
  	zMapSkipListDestroy(featureset_item->display_index, zmapWindowCanvasFeatureFree);
  	featureset_item->display_index = NULL;
  }

  	/* removing it the second time will fail gracefully */
  g_hash_table_remove(featureset_class_G->featureset_items,GUINT_TO_POINTER(featureset_item->id));

//  printf("removing %s\n",g_quark_to_string(featureset->id));


  if (GTK_OBJECT_CLASS (parent_class_G)->destroy)
	(* GTK_OBJECT_CLASS (parent_class_G)->destroy) (GTK_OBJECT(object));

  return ;
}


