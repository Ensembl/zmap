/*  File: zmapWindowGraphItem.c
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
 * CVS info:   $Id: zmapWindowGlyphItem.c,v 1.14 2011-03-14 11:35:18 mh17 Exp $
 *-------------------------------------------------------------------
 */

 /* NOTE
  * this module takes a series of bins and draws then in various formats
  * (line, hiostogram, heatmap) but in each case the source data is the same
  * if 'density' mode is set then the source data is re-binned according to zoom level
  * it adds a single (extended) foo_canvas item to the canvas for each set of data
  * in a column.  It is possible to have several line graphs overlapping, but note that
  * heatmaps and histograms would not look as good.
  */

#include <ZMap/zmap.h>



#include <math.h>
#include <string.h>
#include <ZMap/zmapUtilsLog.h>
#include <zmapWindowBasicFeature.h>
#include <zmapWindowGraphDensityItem_I.h>


static void zmap_window_graph_density_item_class_init  (ZMapWindowGraphDensityItemClass density_class);
static void zmap_window_graph_density_item_init        (ZMapWindowGraphDensityItem      item);

static void  zmap_window_graph_density_item_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags);
static void  zmap_window_graph_density_item_map (FooCanvasItem *item);
static void  zmap_window_graph_density_item_realize (FooCanvasItem *item);
static void  zmap_window_graph_density_item_unmap (FooCanvasItem *item);
static void  zmap_window_graph_density_item_unrealize (FooCanvasItem *item);
static void  zmap_window_graph_density_item_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags);
static double  zmap_window_graph_density_item_point (FooCanvasItem *item, double x, double y, int cx, int cy, FooCanvasItem **actual_item);
static void  zmap_window_graph_density_item_bounds (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2);
static void  zmap_window_graph_density_item_draw (FooCanvasItem *item, GdkDrawable *drawable, GdkEventExpose *expose);
static void zmap_window_graph_density_item_destroy     (GObject *object);

static gint zmapGraphSegmentCmp(gconstpointer a, gconstpointer b);

static guint32 gdk_color_to_rgba(GdkColor *color);

static ZMapWindowGraphDensityItemClass density_class_G = NULL;
static FooCanvasItemClass *parent_class_G;



enum {
	PROP_0,
	PROP_X1,
	PROP_Y1,
	PROP_X2,
	PROP_Y2,
};

GType zMapWindowGraphDensityItemGetType(void)
{
  static GType group_type = 0 ;

  if (!group_type)
    {
      static const GTypeInfo group_info = {
      sizeof (zmapWindowGraphDensityItemClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) zmap_window_graph_density_item_class_init,
      NULL,           /* class_finalize */
      NULL,           /* class_data */
      sizeof (zmapWindowGraphDensityItem),
      0,              /* n_preallocs */
      (GInstanceInitFunc) zmap_window_graph_density_item_init,
      NULL
      };

      group_type = g_type_register_static(foo_canvas_item_get_type(),
                                ZMAP_WINDOW_GRAPH_DENSITY_ITEM_NAME,
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
ZMapWindowCanvasItem zMapWindowGraphDensityItemGetDensityItem(FooCanvasGroup *parent, GQuark id, int start,int end, ZMapFeatureTypeStyle style, ZMapStrand strand, ZMapFrame frame, int index)
{
      ZMapWindowGraphDensityItem di = NULL;
      FooCanvasItem *foo  = NULL,*interval;
	GdkColor *fill = NULL,*draw = NULL, *outline = NULL;


            /* class not intialised till we make an item in foo_canvas_item_new() below */
      if(density_class_G && density_class_G->density_items)
            foo = (FooCanvasItem *) g_hash_table_lookup( density_class_G->density_items, GUINT_TO_POINTER(id));
      if(foo)
      {
            return((ZMapWindowCanvasItem) foo);
      }
      else
      {
      	int stagger;

            /* need a wrapper to get ZWCI with a foo_density inside it */
            foo = foo_canvas_item_new(parent, ZMAP_TYPE_WINDOW_GRAPH_ITEM, NULL);

            interval = foo_canvas_item_new((FooCanvasGroup *) foo, ZMAP_TYPE_WINDOW_GRAPH_DENSITY, NULL);

            di = (ZMapWindowGraphDensityItem) interval;
            di->id = id;
            g_hash_table_insert(density_class_G->density_items,GUINT_TO_POINTER(id),(gpointer) foo);
//printf("make density item %s\n",g_quark_to_string(di->id));

		di->re_bin = style->mode_data.graph.density;

		/* we record strand and frame for display colours
		 * the features get added to the approriate column depending on strand, frame
		 * so we get the rights density item foo item implicitly
		 */
		di->strand = strand;
		di->frame = frame;

		di->style = style;

		di->width = 1;
  		di->width_pixels = TRUE;
  		di->start = start;
  		di->end = end;

            stagger = zMapStyleDensityStagger(style);
            di->index = index;
            di->x_off = stagger * di->index;

		/* set our bounding box in canvas coordinates to be the whole column */
		foo_canvas_item_request_update (interval);

		zmapWindowCanvasItemGetColours(style, strand, frame, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill, &draw, &outline, NULL, NULL);

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
      }
      return ((ZMapWindowCanvasItem) foo);
}


void zmapWindowGraphDensityItemSetColour(ZMapWindowCanvasItem   item,
						      FooCanvasItem         *interval,
						      ZMapFeature			feature,
						      ZMapFeatureSubPartSpan sub_feature,
						      ZMapStyleColourType    colour_type,
							int colour_flags,
						      GdkColor              *default_fill,
                                          GdkColor              *default_border)
{
	ZMapWindowGraphDensityItem di = (ZMapWindowGraphDensityItem) interval;
	ZMapWindowCanvasGraphSegment gs;
	ZMapSkipList sl;
	ZMapWindowCanvasGraphSegmentStruct search;


		/* set focus colour once only */
	if(colour_flags)
	{
		GdkColor *draw = NULL, *fill = NULL, *outline = NULL;

		/* fill and border are defaults set for the whole window but are currently not used
		   instead we have to get the selected colours from the style
		   but if someone defined the defaults they'd get used
		 */

      	zmapWindowCanvasItemGetColours(di->style, di->strand, di->frame, colour_type, &fill, &draw, &outline, default_fill, default_border);

#warning ideally we could set these colours on item init but the code driving this assumes differently
		if(!di->selected_fill_set && fill)
		{
			di->selected_fill_colour = gdk_color_to_rgba(fill);
			di->selected_fill_pixel = foo_canvas_get_color_pixel(interval->canvas, di->selected_fill_colour);
			di->selected_fill_set = TRUE;
		}
		if(!di->selected_outline_set && outline)
		{
			di->selected_outline_colour = gdk_color_to_rgba(outline);
			di->selected_outline_pixel = foo_canvas_get_color_pixel(interval->canvas, di->selected_outline_colour);
			di->selected_outline_set = TRUE;
		}
	}

		/* find the feature's graph segment struct */
	search.y1 = (int) feature->x1;
	search.y2 = (int) feature->x2;

	sl =  zMapSkipListFind(di->display_index, zmapGraphSegmentCmp, &search);
//	if(sl->prev)
//		sl = sl->prev;	/* in case of not exact match when rebinned */

	while(sl)
	{
		gs = sl->data;
//printf("focus find %s sl = %f,%f\n",g_quark_to_string(di->id),gs->y1,gs->y2);

		/* if we got rebinned then we need to find the bin surrounding the feature
		   if the feature is split bewteeen bins just choose one
		 */
		if(gs->y1 > feature->x2)
			break;

#if NEVER_REBINNED
		if(gs->feature == feature)
#else
/*
 *	if we re-bin variable sized features then we could have features extending beyond bins
 * 	in which case a simple containment test will fail
 * 	so we have to test for overlap, which will also handle the simpler case.
 *	bins have a real feature attached and this is fed in from the cursor code (point function)
 * 	this argument also applies to fixed size bins: we pro-rate overlaps when we calculate bins
 * 	according to pixel coordinates.
 *	so we can have bin contains feature, feature contains bin, bin and feature overlap left or right. Yuk
 */
		if(!((gs->y1 > feature->x2) || (gs->y2 < feature->x1)))
#endif
		{
			double i2w_dx,i2w_dy;
			int cx1,cx2,cy1,cy2;
			FooCanvasItem *foo = (FooCanvasItem *) interval;

			/* set the focus flags (on or off) */
			gs->flags = colour_flags;

			/* trigger a repaint */
			/* get canvas coords */
			i2w_dx = i2w_dy = 0.0;
			foo_canvas_item_i2w (foo, &i2w_dx, &i2w_dy);

			/* get item canvas coords, following example from FOO_CANVAS_RE (used by graph items) */
			foo_canvas_w2c (foo->canvas, 0.0 + i2w_dx, gs->y1 - di->start + i2w_dy, &cx1, &cy1);
      		foo_canvas_w2c (foo->canvas, zMapStyleGetWidth(di->style) + i2w_dx, gs->y2 - di->start + i2w_dy, &cx2, &cy2);

			foo_canvas_request_redraw (foo->canvas, cx1, cy1, cx2, cy2);
//printf("focus found %s sl = %f,%f\n",g_quark_to_string(di->id),gs->y1,gs->y2);

			return;
		}

		sl = sl->next;
	}
}


gboolean zMapWindowGraphDensityItemSetStyle(ZMapWindowGraphDensityItem di, ZMapFeatureTypeStyle style)
{
	GdkColor *draw = NULL, *fill = NULL, *outline = NULL;
	FooCanvasItem *foo = (FooCanvasItem *) di;

	if(di->display_index && zMapStyleDensityMinBin(di->style) != zMapStyleDensityMinBin(style))
	{
		zMapSkipListDestroy(di->display_index,
			di->source_used ? NULL : zmapWindowCanvasGraphSegmentFree);
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



/* NOTE this extends FooCanvasItem not ZMapWindowCanvasItem */
static void zmap_window_graph_density_item_class_init(ZMapWindowGraphDensityItemClass density_class)
{
  GObjectClass *gobject_class ;
  FooCanvasItemClass *item_class;

  density_class_G = density_class;
  density_class_G->density_items = g_hash_table_new(NULL,NULL);

  parent_class_G = gtk_type_class (foo_canvas_item_get_type ());

  gobject_class = (GObjectClass *) density_class;

  item_class = (FooCanvasItemClass *) density_class;

  gobject_class->dispose = zmap_window_graph_density_item_destroy;

  item_class->update = zmap_window_graph_density_item_update;
  item_class->map = zmap_window_graph_density_item_map;
  item_class->realize = zmap_window_graph_density_item_realize;
  item_class->unmap = zmap_window_graph_density_item_unmap;
  item_class->unrealize = zmap_window_graph_density_item_unrealize;
  item_class->bounds = zmap_window_graph_density_item_bounds;
  item_class->point  = zmap_window_graph_density_item_point;
  item_class->draw   = zmap_window_graph_density_item_draw;

//  zmapWindowItemStatsInit(&(canvas_class->stats), ZMAP_TYPE_WINDOW_GRAPH_DENSITY) ;

  return ;
}



static void zmap_window_graph_density_item_init (ZMapWindowGraphDensityItem graph)
{
  return ;
}

static void zmap_window_graph_density_item_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags)
{

      ZMapWindowGraphDensityItem di = (ZMapWindowGraphDensityItem) item;
      int cx1,cy1,cx2,cy2;
      double x1,x2,y1,y2;

	if (parent_class_G->update)
		(* parent_class_G->update) (item, i2w_dx, i2w_dy, flags);

	// cribbed from FooCanvasRE; this sets the canvas coords in the foo item
	x1 = i2w_dx + di->x_off;
	x2 = x1 + zMapStyleGetWidth(di->style);
	y1 = i2w_dy;
	y2 = y1 + di->end - di->start;

	foo_canvas_w2c (item->canvas, x1, y1, &cx1, &cy1);
	foo_canvas_w2c (item->canvas, x2, y2, &cx2, &cy2);

	item->x1 = cx1;
	item->y1 = cy1;
	item->x2 = cx2+1;
	item->y2 = cy2+1;

}

/* these should not be needed */
static void  zmap_window_graph_density_item_map (FooCanvasItem *item)
{
  if (parent_class_G->map)
    (* parent_class_G->map) (item);
}

static void  zmap_window_graph_density_item_realize (FooCanvasItem *item)
{
  if (parent_class_G->realize)
    (* parent_class_G->realize) (item);
}

static void  zmap_window_graph_density_item_unmap (FooCanvasItem *item)
{
  if (parent_class_G->unmap)
    (* parent_class_G->unmap) (item);
}

static void  zmap_window_graph_density_item_unrealize (FooCanvasItem *item)
{
  if (parent_class_G->unrealize)
    (* parent_class_G->unrealize) (item);
}


/* how far are we from the cursor? */
/* can't return foo canvas item for the feature as they are not in the canvas,
 * so return the density foo item adjusted to point at the nearest feature */

 /* by a process of guesswork x,y are world coordinates and cx,cy are canvas (i think) */
double  zmap_window_graph_density_item_point (FooCanvasItem *item, double x, double y, int cx, int cy, FooCanvasItem **actual_item)
{
	double best = 0.0;
	ZMapWindowGraphDensityItem di = (ZMapWindowGraphDensityItem) item;
	ZMapWindowCanvasGraphSegment gs;
	ZMapSkipList sl;
	ZMapWindowCanvasGraphSegmentStruct search;
//      double dist = 0.0;
      double wx; //,wy;
      double dx,dy;
      int n_test = 3,i;

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

	/* NOTE there is a flake in world coords at low zoom */

	search.y1 = (int) y - item->canvas->close_enough;	/* NOTE close_enough is zero */
	search.y2 = (int) y + item->canvas->close_enough;

	sl =  zMapSkipListFind(di->display_index, zmapGraphSegmentCmp, &search);
	if(!sl)
		return(0.0);

	for(i = 0;i < n_test;i++)
	{
		gs = (ZMapWindowCanvasGraphSegment) sl->data;


		/* good for boxes, lines are more tricky */

		if(gs->y1 <= y && gs->y2 >= y)	/* overlaps cursor */
		{
//			printf("found graph segment %d/%d at y %f-%f (%f)\n",i, n_test, gs->y1, gs->y2, y);
			wx = item->x1 + gs->width;
			if(wx >= x && item->x1 <= x)	/* item contains cursor */
			{
//			      ZMapWindowCanvasItem parent;	/* containing graph feature */

				best = 0.0;
				*actual_item = item;

				di->point_feature = gs->feature;
//				parent = (ZMapWindowCanvasItem) item->parent;
//				parent->feature = gs->feature;

//				printf("	found graph segment %s at x %f-%f (%f)\n", g_quark_to_string(di->id),item->x1,gs->width,y);
			}
		}
	}
      return best;
}

void  zmap_window_graph_density_item_bounds (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
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

/* find an RGB pixel value between a and b */
/* NOTE foo canvas and GDK have got in a tangle with pixel values and we go round in circle to do this
 * but i acted dumb and followed the procedures (approx) used elsewhere
 */
gulong get_heat_colour(gulong a, gulong b, double score)
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



/* create a new list of binned data derived from the real stuff */
/* bins are coerced to be at least one pixel across */
/* bin values are set as the max (ie most extreme, can be -ve) pro-rated value within the range */
/* this is to highlight peaks without creating silly wide graphs */
/* we are aiming for the same graph at lower resolution */
/* this point is most acute for heatmaps that cannot be overlapped sensibly */
/* try not to split big bins into smaller ones, there's no min size in BP, but the source data imposes a limit */

GList *density_calc_bins(ZMapWindowGraphDensityItem di)
{
	double start,end;
	int seq_range;
	int n_bins;
	int bases_per_bin;
	int bin_start,bin_end;
	ZMapWindowCanvasGraphSegment src_gs = NULL;	/* source */
	ZMapWindowCanvasGraphSegment bin_gs;	/* re-binned */
	GList *src,*dest;
	double score;
	int min_bin = zMapStyleDensityMinBin(di->style);	/* min pixels per bin */
	gboolean fixed = zMapStyleDensityFixed(di->style);

	if(!min_bin)
		min_bin = 4;

	start = di->start;
	end   = di->end;

	seq_range = (int) (end - start + 1);
	n_bins = (int) (seq_range * di->zoom / min_bin) ;
	bases_per_bin = seq_range / n_bins;
	if(bases_per_bin < 1)	/* at high zoom we get many pixels per base */
		bases_per_bin = 1;

	di->source_used = FALSE;

//printf("calc density item %s = %d\n",g_quark_to_string(di->id),g_list_length(di->source_bins));
//printf("bases per bin = %d,fixed = %d\n",bases_per_bin,fixed);

	for(bin_start = start,src = di->source_bins,dest = NULL; bin_start < end && src; bin_start = bin_end + 1)
	{
		bin_end = bin_start + bases_per_bin - 1;		/* end can equal start */

	  	bin_gs = zmapWindowCanvasGraphSegmentAlloc();

		bin_gs->y1 = bin_start;
  		bin_gs->y2 = bin_end;
  		bin_gs->score = 0.0;
//printf("bin: %d,%d\n",bin_start,bin_end);
  		for(;src; src = src->next)
  		{
			src_gs = (ZMapWindowCanvasGraphSegment) src->data;
//printf("src: %f,%f\n",src_gs->y1,src_gs->y2);
			if(src_gs->y2 < bin_start)
				continue;
			if(src_gs->y1 > bin_end)
			{
				/* jump fwds to next data at high zoom */
				if(src_gs->y1 > bin_end + bases_per_bin)
				{
					bin_end = src_gs->y1;
					/* bias to even boundaries */
					bin_end -= (bin_end - bin_start + 1) % bases_per_bin;
					bin_end--; 	/* as it gets ioncremented by the loop */
//printf("jump fwds to %d\n",bin_end);
				}
				break;
			}

			if(fixed && bin_gs->feature && src_gs->y2 > bin_end + bases_per_bin)
			{
				/* short bin(s) ahead of big bin: ouput bin less than min size
				 * to avoid swallowing up details in even bigger bin.
				 * worst case is 2x no of bins
				 */
				bin_gs->y2 = bin_end = src_gs->y1 - 1;
				break;
			}

			if(!bin_gs->feature)
			{
				if(!fixed)
				{
					bin_gs->y1 = src_gs->y1;		/* limit dest bin to extent of src */
//printf("set src back to %f\n",bin_gs->y1);
				}
		  		bin_gs->feature = src_gs->feature;
		  	}
			score = src_gs->score;

				/* this implicitly pro-rates any partial overlap of source bins */
			if(fabs(score) > fabs(bin_gs->score))
			{
				bin_gs->score = score;
				bin_gs->feature = src_gs->feature;		/* can only have one... choose the biggest */
			}

			if(src_gs->y2 > bin_end)
			{
				/* src bin is bigger than dest bin: make dest bigger */
				if(!fixed)
				{
					bin_gs->y2 = bin_end = src_gs->y2;
//printf("loose jump to %d\n",bin_end);
				}
				else if(src_gs->y2 > bin_end + bases_per_bin)
				{
//printf("fixed jump: %d, %d\n",(bin_end - bin_start + 1),(bin_end - bin_start + 1) % bases_per_bin);
					bin_end = src_gs->y2;	/* bias to the next one before the end */
									/* else we start overlapping the next src bin */
//					bin_end -= (bin_end - bin_start + 1) % bases_per_bin;
//					bin_end--;		/* as it gets incremented by the loop */
					bin_gs->y2 = bin_end;
//printf("fixed jump to %d\n",bin_end);
				}
				break;
			}
  		}
		if(bin_gs->score > 0)
		{
			if(!fixed && src_gs->y2 < bin_gs->y2)	/* limit dest bin to extent of src */
			{
				bin_gs->y2 = src_gs->y2;
			}
//printf("add: %f,%f\n", bin_gs->y1,bin_gs->y2);
  			dest = g_list_prepend(dest,(gpointer) bin_gs);
  		}

	}

	/* we could have been artisitic and constructed dest backwards */
	dest = g_list_reverse(dest);
	return(dest);
}




void  zmap_window_graph_density_item_draw (FooCanvasItem *item, GdkDrawable *drawable, GdkEventExpose *expose)
{
	ZMapSkipList sl;
	ZMapWindowCanvasGraphSegmentStruct search;
	ZMapWindowCanvasGraphSegment gs;
      int cx1, cy1, cx2 = 0, cy2 = 0;		/* initialised for LINE mode */
      double i2w_dx, i2w_dy;
      double x1,y1,x2,y2;
      double width;
      gulong fill_pixel, outline_pixel;
      gboolean draw_fill = FALSE, draw_outline = FALSE;
      gboolean draw_box = TRUE; 	/* else line */
#define N_POINTS	2000	/* will never run out as we only display one screen;s worth */
      GdkPoint points[N_POINTS+2];		/* +2 for gaps between bins, inserting a line */
      int n_points = 0;

      ZMapWindowGraphDensityItem di = (ZMapWindowGraphDensityItem) item;

		/* if in density mode we re-calc bins on zoom */
		/* no need to patch into zoom code as zoom has to re-paint */
	if(di->re_bin && di->zoom != item->canvas->pixels_per_unit_y)
	{
		if(di->display_index)
		{
			zMapSkipListDestroy(di->display_index,
				di->source_used ? NULL : zmapWindowCanvasGraphSegmentFree);
		}
		di->display_index = NULL;
		di->zoom = item->canvas->pixels_per_unit_y;
	}

      if(!di->display_index)
      {
      	GList *source_bins;


      	if(!di->source_sorted)
      	{
      		di->source_bins = g_list_sort(di->source_bins,zmapGraphSegmentCmp);
      		di->source_sorted = TRUE;
      		di->n_source_bins = g_list_length(di->source_bins);
      	}

      	source_bins = di->source_bins;
      	di->source_used = TRUE;
      	if(di->re_bin)
      	{
      		source_bins = density_calc_bins(di);
      	}

      	di->display_index = zMapSkipListCreate(source_bins, zmapGraphSegmentCmp);

		/* get max overlap if needed */
#warning overlapped graphs not implemented, need to offset by max_len

      }
      if(!di->display_index)
      	return; 		/* could be an empty column or a mistake */

      /* paint all the data in the exposed area */

	if(!di->gc && (item->object.flags & FOO_CANVAS_ITEM_REALIZED))
		di->gc = gdk_gc_new (item->canvas->layout.bin_window);
	if(!di->gc)
		return;		/* got a draw before realize ?? */

      width = zMapStyleGetWidth(di->style) - 1;		/* off by 1 error! width = #pixels not end-start */

	/*
	 *	get the exposed area
	 *	find the top (and bottom?) items
	 *	clip the extremes and paint all
	 */

	i2w_dx = i2w_dy = 0.0;		/* this gets the offset of the parent of this item */
      foo_canvas_item_i2w (item, &i2w_dx, &i2w_dy);

	foo_canvas_c2w(item->canvas,0,floor(expose->area.y),NULL,&y1);
	foo_canvas_c2w(item->canvas,0,ceil(expose->area.y + expose->area.height),NULL,&y2);

	search.y1 = (int) y1;
	search.y2 = (int) y2;
//printf("expose %d-%d\n",(int) search.y1,(int) search.y2);

	if(di->overlap)
	{
		zMapLogWarning("GraphDensityItem overlap not coded","");
#warning overlapped graphs not implemented, need to offset by max_len
	}

	sl =  zMapSkipListFind(di->display_index, zmapGraphSegmentCmp, &search);

	if(!sl)
		return;	/* if the index is not NULL then we nust have a leaf node */

	/* need to get items that overlap the top of the expose */
	while(sl->prev)
	{
		gs = (ZMapWindowCanvasGraphSegment) sl->data;
		if(gs->y2 < search.y1)
			break;
		sl = sl->prev;
	}
	if(zMapStyleGraphMode(di->style) == ZMAPSTYLE_GRAPH_LINE)
	{
		/* need to draw one more line ase these extend to the previous */
		if(sl->prev)
			sl = sl->prev;
	}


	/* NOTE assuming no overlap, maybe can restructure this code when implementing */
//	if(!di->overlap)
	{
		double last_gy = -1.0;	/* edge of prev bin */
		double bases_per_pixel;

		bases_per_pixel = 1.0 / item->canvas->pixels_per_unit_y;

		/* we have already found the first matching or previous item */

		for(;sl;sl = sl->next)
		{
			gs = (ZMapWindowCanvasGraphSegment) sl->data;
//printf("found %d-%d\n",(int) gs->y1,(int) gs->y2);

			if(zMapStyleGraphMode(di->style) != ZMAPSTYLE_GRAPH_LINE)
			{
				if(gs->y1 > search.y2)
					break;	/* finished */
				if(gs->y2 < search.y1)
					continue;
			}

			/*
			   NOTE need to sort out container positioning to make this work
			   di covers its container exactly, but is it offset??
			   by analogy w/ old style ZMapWindowCanvasItems we should display
			   'intervals' as item relative
			*/

			/* clip this one (GDK does that? or is it X?) and paint */
//printf("paint %d-%d\n",(int) gs->y1,(int) gs->y2);

			switch(zMapStyleGraphMode(di->style))
			{
			case ZMAPSTYLE_GRAPH_LINE:
				draw_box = FALSE;

				/* output poly-line segemnts via a series of points
				 *NOTE we start at the previous segment if any via the find algorithm
				 * so this caters for dangling lines above the start
				 * and we stop after the end so this caters for dangling lines below the end
				 */

		      	gs->width = x2 = di->x_off + di->style->mode_data.graph.baseline + (width * gs->score) ;
				y2 = (gs->y2 + gs->y1 + 1) / 2;
	      		foo_canvas_w2c (item->canvas, x2 + i2w_dx, y2 - di->start + i2w_dy, &cx2, &cy2);

	      		if(gs->y1 - last_gy > bases_per_pixel * 2)	/* && fill_gaps_between_bins) */
				{
					int cx,cy;

					if(last_gy < 0)	/* this is the first point */
					{
						last_gy = search.y1;
					}
					else
					{
			      		foo_canvas_w2c (item->canvas, di->x_off + di->style->mode_data.graph.baseline + i2w_dx, last_gy - di->start + i2w_dy, &cx, &cy);
						points[n_points].x = cx;
						points[n_points].y = cy;
						n_points++;
					}

		      		foo_canvas_w2c (item->canvas, di->x_off + di->style->mode_data.graph.baseline  + i2w_dx, gs->y1 - di->start + i2w_dy, &cx, &cy);
					points[n_points].x = cx;
					points[n_points].y = cy;
					n_points++;
				}

				points[n_points].x = cx2;
				points[n_points].y = cy2;
				last_gy = gs->y2;

				if(++n_points >= N_POINTS)
				{
					GdkColor c;

					/* NOTE there is a bug here in that the trailing segnemt does not always paint */
					/* kludged away by increasing N_POINTS to 2000 */
					c.pixel = di->outline_pixel;
					gdk_gc_set_foreground (di->gc, &c);

					gdk_draw_lines(drawable,di->gc,points,n_points);
					n_points = 0;
				}
				break;

			case ZMAPSTYLE_GRAPH_HEATMAP:
				/* colour between fill and outline according to score */
				x1 = di->x_off;
				gs->width = x2 = x1 + width;
				draw_fill = TRUE;
				fill_pixel = foo_canvas_get_color_pixel(item->canvas,
					get_heat_colour(di->fill_colour,di->outline_colour,gs->score));
				break;

			default:
			case ZMAPSTYLE_GRAPH_HISTOGRAM:
				x1 = di->x_off; //  + (width * zMapStyleBaseline(di->style)) ;
		      	gs->width = x2 = x1 + (width * gs->score) ;

  			      /* If the baseline is not zero then we can end up with x2 being less than x1 so
  			         swop them for drawing, perhaps the drawing code should take care of this. */
			      if (x1 > x2)
   				{
 			           double tmp = x1;
			           x1 = x2 ;
			           x2 = tmp ;
			      }

				if (di->fill_set)
				{
					draw_fill = TRUE;
					fill_pixel = di->fill_pixel;
				}
				if (di->outline_set)
				{
					draw_outline = TRUE;
					outline_pixel = di->outline_pixel;
				}

				if(gs->flags & GS_FOCUS_MASK)		/* get highlit colour */
				{
					fill_pixel = di->selected_fill_pixel;
					outline_pixel = di->selected_outline_pixel;
				}
				break;
			}


			if(draw_box)
			{
				GdkColor c;

				/* get item canvas coords, following example from FOO_CANVAS_RE (used by graph items) */
				foo_canvas_w2c (item->canvas, x1 + i2w_dx, gs->y1 - di->start + i2w_dy, &cx1, &cy1);
      			foo_canvas_w2c (item->canvas, x2 + i2w_dx, gs->y2 - di->start + i2w_dy + 1, &cx2, &cy2);
      						/* + 1 to draw to the end of the last base */
//if(gs->flags & GS_FOCUS_MASK)
//	printf("gdk   at %d,%d - %d,%d (from %f,%f) (%f,%f, %f)\n",cy1,cx1,cy2,cx2,x1,x2,i2w_dx,i2w_dy,di->x_off);

				if (draw_fill)
				{
					/* we have pre-calculated pixel colours */
					c.pixel = fill_pixel;
					gdk_gc_set_foreground (di->gc, &c);

					gdk_draw_rectangle (drawable,
						di->gc,
						TRUE,
						cx1, cy1,
						cx2 - cx1,
						cy2 - cy1);
				}
				if (draw_outline)
				{
					c.pixel = di->outline_pixel;
					gdk_gc_set_foreground (di->gc, &c);

					gdk_draw_rectangle (drawable,
					di->gc,
					FALSE,
					cx1,
					cy1,
					cx2 - cx1,
					cy2 - cy1);
				}
			}

			if(gs->y1 > search.y2)
				break;	/* got last line point: finished */
		}

		if(n_points > 1)
		{
			GdkColor c;

			c.pixel = di->outline_pixel;
			gdk_gc_set_foreground (di->gc, &c);

			gdk_draw_lines(drawable,di->gc,points,n_points);
		}
	}
}


ZMapWindowCanvasGraphSegment zmapWindowCanvasGraphSegmentAlloc(void)
{
      GList *l;
      ZMapWindowCanvasGraphSegment gs;

      if(!density_class_G->graph_segment_free_list)
      {
            int i;
            gs = g_new(ZMapWindowCanvasGraphSegmentStruct,N_GS_ALLOC);

            for(i = 0;i < N_GS_ALLOC;i++)
                  zmapWindowCanvasGraphSegmentFree((gpointer) gs++);
      }
      zMapAssert(density_class_G->graph_segment_free_list);

      l = density_class_G->graph_segment_free_list;
      gs = (ZMapWindowCanvasGraphSegment) l->data;
      density_class_G->graph_segment_free_list = g_list_delete_link(density_class_G->graph_segment_free_list,l);

      /* these can get re-allocated so must zero */
      memset((gpointer) gs,0,sizeof(ZMapWindowCanvasGraphSegmentStruct));
      return(gs);
}


/* need to be a ZMapSkipListFreeFunc for use as a callback */
void zmapWindowCanvasGraphSegmentFree(gpointer thing)
{
	ZMapWindowCanvasGraphSegment gs = (ZMapWindowCanvasGraphSegment) thing;

      density_class_G->graph_segment_free_list =
            g_list_prepend(density_class_G->graph_segment_free_list, (gpointer) gs);
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

static gint zmapGraphSegmentCmp(gconstpointer a, gconstpointer b)
{
	ZMapWindowCanvasGraphSegment gsa = (ZMapWindowCanvasGraphSegment) a;
	ZMapWindowCanvasGraphSegment gsb = (ZMapWindowCanvasGraphSegment) b;

	if(gsa->y1 < gsb->y1)
		return(-1);
	if(gsa->y1 > gsb->y1)
		return(1);
	if(gsa->y2 < gsb->y2)
		return(-1);
	if(gsa->y2 > gsb->y2)
		return(1);
	return(0);
}

/* no return, as all this data in internal to the column density item
 * bins get squashed according to zoom
 */
void zMapWindowGraphDensityAddItem(FooCanvasItem *foo, ZMapFeature feature, double dx, double y1, double y2)
{
  ZMapWindowCanvasGraphSegment gs;
  ZMapFeatureTypeStyle style = feature->style;
  ZMapWindowGraphDensityItem density_item;

  zMapAssert(style);

  if(!dx)		/* we handle zeroes as we can't see them, for coverage data we should not even have them */
  	return;

  gs = zmapWindowCanvasGraphSegmentAlloc();
  gs->feature = feature;

  density_item = (ZMapWindowGraphDensityItem) foo;

  gs->y1 = y1;
  gs->y2 = y2;
  gs->score = dx;			/* is between (-1.0) 0.0 and 1.0 */


  /* even if they come in order we still have to sort them to be sure so just add to the front */
  density_item->source_bins = g_list_prepend(density_item->source_bins,gs);
  density_item->source_sorted = FALSE;

  /* add to the display bins if index already created */
  if(density_item->display_index)
  {
  	if(density_item->re_bin)
  	{
  		/* need to recalc bins */
  		/* quick fix FTM, de-calc which requires a re-calc on display */
  		zMapSkipListDestroy(density_item->display_index,
  			density_item->source_used? NULL : zmapWindowCanvasGraphSegmentFree);
  		density_item->display_index = NULL;
  	}
  	else
  	{
  		density_item->display_index =
  			zMapSkipListAdd(density_item->display_index, zmapGraphSegmentCmp, gs);
  	}
  }
}



static void zmap_window_graph_density_item_destroy     (GObject *object)
{

  ZMapWindowGraphDensityItem density;
  /* mh17 NOTE: this function gets called twice for every object via a very very tall stack */
  /* no idea why, but this is all harmless here if we make sure to test if pointers are valid */
  /* what's more interesting is why an object has to be killed twice */

//  printf("zmap_window_graph_density_item_destroy %p\n",object);
  g_return_if_fail(ZMAP_IS_WINDOW_GRAPH_DENSITY_ITEM(object));

  density = ZMAP_WINDOW_GRAPH_DENSITY_ITEM(object);

  if(density->display_index)
  {
  	zMapSkipListDestroy(density->display_index,
  		density->source_used? NULL : zmapWindowCanvasGraphSegmentFree);
  	density->display_index = NULL;
  }
  	/* removing it the second time will fail gracefully */
  g_hash_table_remove(density_class_G->density_items,GUINT_TO_POINTER(density->id));

//  printf("removing %s\n",g_quark_to_string(density->id));


  if (GTK_OBJECT_CLASS (parent_class_G)->destroy)
	(* GTK_OBJECT_CLASS (parent_class_G)->destroy) (GTK_OBJECT(object));

  return ;
}



