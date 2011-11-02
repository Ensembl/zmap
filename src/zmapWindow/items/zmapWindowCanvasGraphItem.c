/*  File: zmapWindowCanvasGraphItem.c
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2011 Genome Research Ltd.
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

/* NOTE
 * originally implemnted as free standing canvas type and lter merged into CanvasFeatureset
 */

#include <ZMap/zmap.h>



#include <math.h>
#include <string.h>
#include <ZMap/zmapUtilsFoo.h>
#include <ZMap/zmapUtilsLog.h>
#include <zmapWindowBasicFeature.h>
#include <zmapWindowGraphDensityItem_I.h>





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



static guint32 gdk_color_to_rgba(GdkColor *color)
{
  guint32 rgba = 0;

  rgba = ((color->red & 0xff00) << 16  |
        (color->green & 0xff00) << 8 |
        (color->blue & 0xff00)       |
        0xff);

  return rgba;
}


/*
 * if we draw wiggle plots then instead of one GDK call per segment we cahce points and draw a big long poly-line
 *
 * ideally this would be implemented in the feaatureset struct as extenged for graphs
 * but we don't extend featuresets as the rest of the coed would be more complex.
 * instead we have a single cache here which works becaure we only ever display one column at a time.
 * and relies on ..PaintFlush() being called
 *
 * If we ever created threads to paint columns then this needs to be
 * moved into CanvasFeatureset and alloc'd for graphs and freed on CFS_destroy()
 */


#define N_POINTS	2000	/* will never run out as we only display one screen's worth */
static GdkPoint points[N_POINTS+2];		/* +2 for gaps between bins, inserting a line */
static int n_points = 0;


/* paint one feature, CanvasFeatureset handles focus highlight */
/* NOTE wiggle plots are drawn as poly-lines and get cached and painted when complete */
/* NOTE lines have to be drawn out of the box at the edges */
static void zMapWindowCanvasAlignmentPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable,  GdkEventExpose *expose)

{
      int cx1, cy1, cx2 = 0, cy2 = 0;		/* initialised for LINE mode */
      double i2w_dx, i2w_dy;
      double x1,y1,x2,y2;
      double width;
      gboolean draw_box = TRUE; 	/* else line */
      gulong fill,outline;
	int colours_set, fill_set, outline_set;

	switch(zMapStyleGraphMode(featureset->style))
	{
	case ZMAPSTYLE_GRAPH_LINE:
		draw_box = FALSE;
#if 0
		/* output poly-line segemnts via a series of points
		 * NOTE we start at the previous segment if any via the find algorithm
		 * so this caters for dangling lines above the start
		 * and we stop after the end so this caters for dangling lines below the end
		 */

// shome mistake?  (3x : look fwds for width
		feature->width = x2 = featureset->x_off + featureset->style->mode_data.graph.baseline + (width * feature->score) ;
//		feature->width = x2 =  featureset->style->mode_data.graph.baseline + (width * feature->score) ;
		y2 = (feature->y2 + feature->y1 + 1) / 2;
		foo_canvas_w2c (item->canvas, x2 + i2w_dx, y2 - featureset->start + i2w_dy, &cx2, &cy2);

		if(feature->y1 - last_gy > bases_per_pixel * 2)	/* && fill_gaps_between_bins) */
		{
			int cx,cy;

			if(last_gy < 0)	/* this is the first point */
			{
				last_gy = search.y1;
			}
			else
			{
				foo_canvas_w2c (item->canvas, featureset->x_off + featureset->style->mode_data.graph.baseline + i2w_dx, last_gy - featureset->start + i2w_dy, &cx, &cy);
				points[n_points].x = cx;
				points[n_points].y = cy;
				n_points++;
			}

			foo_canvas_w2c (item->canvas, featureset->x_off + featureset->style->mode_data.graph.baseline  + i2w_dx, feature->y1 - featureset->start + i2w_dy, &cx, &cy);
			points[n_points].x = cx;
			points[n_points].y = cy;
			n_points++;
		}

		points[n_points].x = cx2;
		points[n_points].y = cy2;
		last_gy = feature->y2;

		if(++n_points >= N_POINTS)
		{
			GdkColor c;

			/* NOTE there is a bug here in that the trailing segnemt does not always paint */
			/* kludged away by increasing N_POINTS to 2000 */
			c.pixel = featureset->outline_pixel;
			gdk_gc_set_foreground (featureset->gc, &c);

			gdk_draw_lines(drawable,featureset->gc,points,n_points);
			n_points = 0;
		}
#endif
		break;

	case ZMAPSTYLE_GRAPH_HEATMAP:
		/* colour between fill and outline according to score */
		x1 = featureset->x_off;
		feature->width = featureset->width;
		x2 = x1 + feature->width;

		outline_set = FALSE;
		fill_set = TRUE;
		fill = foo_canvas_get_color_pixel(item->canvas,
			get_heat_colour(featureset->fill_colour,featureset->outline_colour,feature->score));
		break;


	default:
	case ZMAPSTYLE_GRAPH_HISTOGRAM:
		x1 = featureset->x_off; //  + (width * zMapStyleBaseline(di->style)) ;
		feature->width = (width * gs->score) ;
		x2 = x1 + feature->width;

		/* If the baseline is not zero then we can end up with x2 being less than x1
		 * so we mst swap these round.
		 */
		if(x1 > x2)
		{
			double x = x1;
			x1 = x2;
			x2 = x;
		}

		colours_set = zMapWindowCanvasFeaturesetGetColours(featureset, feature, &fill, &outline);
		fill_set = colours_set & WINDOW_FOCUS_CACHE_FILL;
		outline_set = colours_set & WINDOW_FOCUS_CACHE_OUTLINE;

		break;
	}


	if(draw_box)
	{
		/* draw a box */

		/* colours are not defined for the CanvasFeatureSet
		* as we can have several styles in a column
		* but they are cached by the calling function
		* and also the window focus code
		*/

		zMapCanvasFeaturesetDrawBoxMacro(featureset, feature, x1,x2, drawable, expose, fill_set, outline_set, fill, outline);

	}

}


void zMapWindowCanvasFeaturesetPaintFlush(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable)
{

	if(n_points > 1)
	{
		GdkColor c;

		c.pixel = featureset->outline_pixel;
		gdk_gc_set_foreground (featureset->gc, &c);

		gdk_draw_lines(drawable,featureset->gc,points,n_points);
	}

}

/*
 * if we are displaying a gapped alignment, recalculate this data
 * do this by freeing the existing data, new stuff will be added by the paint fucntion
 */
static void zMapWindowCanvasGraphZoomSet(ZMapWindowFeaturesetItem featureset)
{
	if(featureset->rebin)		/* if it's a density item we always re-bin */
	{
		if(featureset->display_index)
		{
			zMapSkipListDestroy(featureset->display_index, zmapWindowCanvasFeatureFree);
			featureset->display = NULL;
			featureset->display_index = NULL;
		}

		featureset->display = density_calc_bins(featureset);
	}
	/* else we just paint features as they are */


	if(!featureset->display_index)
  	      zMapWindowCanvasFeaturesetIndex(featureset);	/* will index display not features if display is set */
}



void zMapWindowCanvasGraphInit(void)
{
	gpointer funcs[FUNC_N_FUNC] = { NULL };

	funcs[FUNC_PAINT]  = zMapWindowCanvasAlignmentPaintFeature;
	funcs[FUNC_EXTENT] = zMapWindowCanvasAlignmentGetFeatureExtent;
	funcs[FUNC_ZOOM]   = zMapWindowCanvasGraphZoomSet;

	zMapWindowCanvasFeatureSetSetFuncs(FEATURE_GRAPH, funcs, 0);
}



/* for bump_style */
gboolean zMapWindowGraphDensityItemSetStyle(ZMapWindowFeaturesetItem di, ZMapFeatureTypeStyle style)
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





/* create a new list of binned data derived from the real stuff */
/* bins are coerced to be at least one pixel across */
/* bin values are set as the max (ie most extreme, can be -ve) pro-rated value within the range */
/* this is to highlight peaks without creating silly wide graphs */
/* we are aiming for the same graph at lower resolution */
/* this point is most acute for heatmaps that cannot be overlapped sensibly */
/* try not to split big bins into smaller ones, there's no min size in BP, but the source data imposes a limit */

GList *density_calc_bins(ZMapWindowFeaturesetItem di)
{
	double start,end;
	int seq_range;
	int n_bins;
	int bases_per_bin;
	int bin_start,bin_end;
	zmapWindowCanvasFeature src_gs = NULL;	/* source */
	zmapWindowCanvasFeaturet bin_gs;	/* re-binned */
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

//printf("calc density item %s = %d\n",g_quark_to_string(di->id),g_list_length(di->source_bins));
//printf("bases per bin = %d,fixed = %d\n",bases_per_bin,fixed);

	for(bin_start = start,src = di->features,dest = NULL; bin_start < end && src; bin_start = bin_end + 1)
	{
		bin_end = bin_start + bases_per_bin - 1;		/* end can equal start */

	  	bin_gs = zmapWindowCanvasFeatureAlloc(di->type);

		bin_gs->y1 = bin_start;
  		bin_gs->y2 = bin_end;
  		bin_gs->score = 0.0;
//printf("bin: %d,%d\n",bin_start,bin_end);
  		for(;src; src = src->next)
  		{
			src_gs = (ZMapWindowCanvasFeature) src->data;
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












