/*  File: zmapWindowCanvasGraphItem.c
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2012 Genome Research Ltd.
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
 * originally implemented as free standing canvas type and lter merged into CanvasFeatureset
 */

#include <ZMap/zmap.h>



#include <math.h>
#include <string.h>
#include <ZMap/zmapUtilsFoo.h>
#include <ZMap/zmapUtilsLog.h>
#include <ZMap/zmapFeature.h>
#include <zmapWindowCanvasGraphItem_I.h>


static GList *density_calc_bins(ZMapWindowFeaturesetItem di);

gboolean zMapWindowGraphDensityItemSetStyle(ZMapWindowFeaturesetItem di, ZMapFeatureTypeStyle style);
void zMapWindowCanvasGraphPaintFlush(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable, GdkEventExpose *expose);



/*
 * if we draw wiggle plots then instead of one GDK call per segment we cahce points and draw a big long poly-line
 *
 * ideally this would be implemented in the featureset struct as extended for graphs
 * but we don't extend featuresets as the rest of the code would be more complex.
 * instead we have a single cache here which works becaure we only ever display one column at a time.
 * and relies on ..PaintFlush() being called
 *
 * If we ever created threads to paint columns then this needs to be
 * moved into CanvasFeatureset and alloc'd for graphs and freed on CFS_destroy()
 *
 * NOTE currently we do use callbacks but these are necessarily serialised by the Foo Canvas
 */


#define N_POINTS	2000	/* will never run out as we only display one screen's worth */
static GdkPoint points[N_POINTS+4];		/* +2 for gaps between bins, inserting a line, plus allow for end of data trailing lines */
static int n_points = 0;
double last_gy = -1.0;



void zMapWindowCanvasGraphPaintPrepare(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable, GdkEventExpose *expose)
{
      int cx2 = 0, cy2 = 0;		/* initialised for LINE mode */
      double x2,y2;
	FooCanvasItem *item = (FooCanvasItem *) featureset;

	if(zMapStyleGraphMode(featureset->style) != ZMAPSTYLE_GRAPH_LINE)
		return;

	n_points = 0;

	if(feature)
			/* add first point to join up to */
	{
		/* this is probably a paint in the middle of the window */
		/* and the line has been drawn previously, we need to paint over it exactly */
		x2 = featureset->style->mode_data.graph.baseline + feature->width - 1;
		y2 = (feature->y2 + feature->y1 + 1) / 2;
		foo_canvas_w2c (item->canvas, x2 + featureset->dx + featureset->x_off, y2 - featureset->start + featureset->dy, &cx2, &cy2);

		points[n_points].x = cx2;
		points[n_points].y = cy2;
		n_points++;
		last_gy = feature->y2;
	}
	else
	{
		/* trigger vertical bracket at the start */
		last_gy = featureset->start;
	}
}

/* paint one feature, CanvasFeatureset handles focus highlight */
/* NOTE wiggle plots are drawn as poly-lines and get cached and painted when complete */
/* NOTE lines have to be drawn out of the box at the edges */


static void zMapWindowCanvasGraphPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable, GdkEventExpose *expose)

{
      int cx2 = 0, cy2 = 0;		/* initialised for LINE mode */
      double x1,x2,y2;
      gboolean draw_box = TRUE; 	/* else line */
      gulong fill,outline;
	int colours_set, fill_set, outline_set;
	FooCanvasItem *item = (FooCanvasItem *) featureset;

	switch(zMapStyleGraphMode(featureset->style))
	{
	case ZMAPSTYLE_GRAPH_LINE:
		draw_box = FALSE;

		/* output poly-line segemnts via a series of points
		 * NOTE we start at the previous segment if any via the calling display code
		 * so this caters for dangling lines above the start
		 * and we stop after the end so this caters for dangling lines below the end
		 */

		/* get width, y1, y2 and middle
		 * our point is middle,width
		 * and we join to the previous point if it's close in y
		 * or back to baseline if y1 > prev.y2 in pixels, and then join with a vertical (add two points)
		 */

		if((feature->y1 - last_gy > featureset->bases_per_pixel * 2))	/* && fill_gaps_between_bins) */
		{
			int cx,cy;

			/* draw from last point back to base: add baseline point at previous feature y2 */
			foo_canvas_w2c (item->canvas, featureset->style->mode_data.graph.baseline + featureset->dx + featureset->x_off, last_gy - featureset->start + featureset->dy, &cx, &cy);

			points[n_points].x = cx;
			points[n_points].y = cy;
			n_points++;

			/* add point at current feature y1, implies vertical if there's a previous point */
			foo_canvas_w2c (item->canvas, featureset->style->mode_data.graph.baseline  + featureset->dx + featureset->x_off, feature->y1 - featureset->start + featureset->dy, &cx, &cy);

			points[n_points].x = cx;
			points[n_points].y = cy;
			n_points++;
		}


		x2 = featureset->style->mode_data.graph.baseline + feature->width - 1;
		y2 = (feature->y2 + feature->y1 + 1) / 2;
		foo_canvas_w2c (item->canvas, x2 + featureset->dx + featureset->x_off, y2 - featureset->start + featureset->dy, &cx2, &cy2);

		points[n_points].x = cx2;
		points[n_points].y = cy2;
		last_gy = feature->y2;

		if(++n_points >= N_POINTS)
		{
			zMapWindowCanvasGraphPaintFlush(featureset, feature, drawable, expose);
		}

		break;

	case ZMAPSTYLE_GRAPH_HEATMAP:
		/* colour between fill and outline according to score */
		x1 = featureset->dx + featureset->x_off;
		x2 = x1 + featureset->width - 1;

		outline_set = FALSE;
		fill_set = TRUE;
		fill = foo_canvas_get_color_pixel(item->canvas,
			zMapWindowCanvasFeatureGetHeatColour(featureset->fill_colour,featureset->outline_colour,feature->score));
		break;


	default:
	case ZMAPSTYLE_GRAPH_HISTOGRAM:
		x1 = featureset->dx + featureset->x_off; //  + (width * zMapStyleBaseline(di->style)) ;
		x2 = x1 + feature->width - 1;

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
		zMapCanvasFeaturesetDrawBoxMacro(featureset,x1,x2, feature->y1, feature->y2, drawable, fill_set,outline_set,fill,outline);
	}
}


void zMapWindowCanvasGraphPaintFlush(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable, GdkEventExpose *expose)
{
	gboolean is_line = (zMapStyleGraphMode(featureset->style) == ZMAPSTYLE_GRAPH_LINE);

	if(!is_line)
		return;

	if(n_points < 1)		/* no features, draw baseline */
	{
		zmapWindowCanvasFeatureStruct dummy = { 0 };
		FooCanvasItem *foo = (FooCanvasItem *) featureset;

		foo_canvas_c2w (foo->canvas, 0, featureset->clip_y1, NULL, &dummy.y2);
		dummy.y1 = dummy.y2 - 1.0;;
		dummy.width = 0.0;

		zMapWindowCanvasGraphPaintFeature(featureset, &dummy, drawable, expose );
		n_points -= 1;	/* remove this point and leave the downstream vertical */
	}

	if(n_points >= 1)
	{
		GdkColor c;

		if(!feature)	/* draw back to baseline from last point and add trailing line */

		{
			zmapWindowCanvasFeatureStruct dummy = { 0 };
			FooCanvasItem *foo = (FooCanvasItem *) featureset;

			foo_canvas_c2w (foo->canvas, 0, featureset->clip_y2, NULL, &dummy.y1);
			dummy.y1 += featureset->bases_per_pixel * 2;
			dummy.y2 = dummy.y1 + 1.0;
			dummy.width = 0.0;

			zMapWindowCanvasGraphPaintFeature(featureset, &dummy, drawable, expose );
			n_points -= 1;	/* remove this point and leave the downstream vertical */
		}

		c.pixel = featureset->outline_pixel;
		gdk_gc_set_foreground (featureset->gc, &c);

		gdk_draw_lines(drawable, featureset->gc, points, n_points);	/* these are already clipped to the visible scroll region */

		if(feature)		/* interim flush: add last point at start */
		{
			n_points--;
			points[0].x = points[n_points].x;
			points[0].y = points[n_points].y;
			n_points = 1;
		}
		else
		{
			n_points = 0;
		}
	}
}

/*
 * recalculate the bins if the zoom level changes
 */
static void zMapWindowCanvasGraphZoomSet(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable)
{
	if(featureset->re_bin)		/* if it's a density item we always re-bin */
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

	funcs[FUNC_PREPARE]= zMapWindowCanvasGraphPaintPrepare;
	funcs[FUNC_PAINT]  = zMapWindowCanvasGraphPaintFeature;
	funcs[FUNC_FLUSH]  = zMapWindowCanvasGraphPaintFlush;
	funcs[FUNC_ZOOM]   = zMapWindowCanvasGraphZoomSet;

	zMapWindowCanvasFeatureSetSetFuncs(FEATURE_GRAPH, funcs, 0, 0);
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
	ZMapWindowCanvasFeature src_gs = NULL;	/* source */
	ZMapWindowCanvasFeature bin_gs;	/* re-binned */
	GList *src,*dest;
	double score;
	int min_bin = zMapStyleDensityMinBin(di->style);	/* min pixels per bin */
	gboolean fixed = zMapStyleDensityFixed(di->style);

	if(!di->features_sorted)
		di->features = g_list_sort(di->features,zMapFeatureCmp);
	di->features_sorted = TRUE;

	if(!min_bin)
		min_bin = 4;

	start = di->start;
	end   = di->end;

	seq_range = (int) (end - start + 1);
	n_bins = (int) (seq_range * di->zoom / min_bin) ;
	bases_per_bin = seq_range / n_bins;
	if(bases_per_bin < 1)	/* at high zoom we get many pixels per base */
		bases_per_bin = 1;

//printf("calc density item %s = %d\n",g_quark_to_string(di->id),g_list_length(di->features));
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
//printf("src: %f,%f, %f\n",src_gs->y1,src_gs->y2,src_gs->score);
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
//printf("score set to %f\n",score);
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
			bin_gs->score = zMapWindowCanvasFeatureGetNormalisedScore(di->style, bin_gs->feature->score);
			bin_gs->width = di->width;
			if(di->style->mode_data.graph.mode != ZMAPSTYLE_GRAPH_HEATMAP)
				bin_gs->width = di->width * bin_gs->score;

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












