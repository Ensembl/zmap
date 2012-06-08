/*  File: zmapWindowCanvasLocus.c
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
 *-------------------------------------------------------------------
 * ZMap is free software; you can refeaturesetstribute it and/or
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
 * implements callback functions for FeaturesetItem locus features
 * locus features are single line (word?) things (the feature name with maybe a line for decoration)
 * unlike old style ZMWCI i called these locus not text as they are not generic text features
 * but instead a specific sequence region with a name
 *
 * of old there was talk of creating diff features externally for loci (i hear)
 * but here we simply have created a feature from the name and coords of the original, if it has a locus id
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>





#include <math.h>
#include <string.h>
#include <ZMap/zmapFeature.h>
#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasLocus_I.h>


static void zmapWindowCanvasLocusGetPango(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasLocusSet lset)
{
	/* lazy evaluation of pango renderer */

	if(lset && !lset->pango.renderer)
	{
		GdkColor *draw;

		zMapStyleGetColours(featureset->style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, NULL, &draw, NULL);

		zmapWindowCanvasFeaturesetInitPango(drawable, featureset, &lset->pango, ZMAP_ZOOM_FONT_FAMILY, ZMAP_ZOOM_FONT_SIZE, draw);
	}
}



void zMapWindowCanvasLocusPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable, GdkEventExpose *expose)
{
	double x1,x2;

	FooCanvasItem *foo = (FooCanvasItem *) featureset;
	ZMapWindowCanvasLocus locus = (ZMapWindowCanvasLocus) feature;
	ZMapWindowCanvasLocusSet lset = (ZMapWindowCanvasLocusSet) featureset->opt;
	char *text;
	int len;
     	int cx1, cy1, cx2, cy2;

	zmapWindowCanvasLocusGetPango(drawable, featureset, lset);

	x1 = featureset->dx;
	x2 = x1 + locus->x_off;

	text = (char *) g_quark_to_string(feature->feature->original_id);
	len = strlen(text);
	pango_layout_set_text (lset->pango.layout, text, len);

		/* need to get pixel coordinates for pango */
// (dy = start plus block offset)
//	foo_canvas_w2c (foo->canvas, x1, locus->ylocus - featureset->start + featureset->dy, &cx1, &cy1);
//	foo_canvas_w2c (foo->canvas, x2, locus->ytext - featureset->start + featureset->dy, &cx2, &cy2);
	foo_canvas_w2c (foo->canvas, x1, locus->ylocus, &cx1, &cy1);
	foo_canvas_w2c (foo->canvas, x2, locus->ytext, &cx2, &cy2);

	zMap_draw_line(drawable, featureset, cx1, cy1, cx2, cy2);

	cy2 -= lset->pango.text_height / 2;		/* centre text on line */
	/* NOTE this is equivalent to using feature->y1 and converting to canvas coords */
	pango_renderer_draw_layout (lset->pango.renderer, lset->pango.layout,  cx2 * PANGO_SCALE , cy2 * PANGO_SCALE);
}



/* using world coords shift locus names around on the canvas so they don't overlap
 * the theory is that we have arranged for there to be enough space
 * but if not they will still overlap
 *
 * unlike the previous version this is less optimal, but generally tends to put the names nearer the loci
 * so it looks better with the downsdie of some overlap it it's a bit crowded.
 * we get groups overlapping, could do zebra stripe colour coding....
 */


GList *deOverlap(GList *visible,int n_loci, double text_h, double start, double end)
{
	LocusGroup group_span = NULL;
	GList *groups = NULL;
	int n_groups = 0;
	GList *l;
	double needed = 0.0, spare;
//	double overlap;
//	double group_sep;
//	double item_sep;
	double cur_y;

	visible = g_list_sort(visible, zMapFeatureCmp);

		/* arrange into groups of overlapping loci (not names).
		 * NOTE the groups do not overlap at all,but the locus names may
		 */
	for(l = visible;l; l = l->next)
	{
		ZMapWindowCanvasLocus locus = (ZMapWindowCanvasLocus) l->data;

		if(!group_span || locus->ytext > group_span->y2)
			/* simple comparison assuming data is in order of start coord */
		{
			/* does not overlap: start a new group */
			/* first save the span */

			group_span = g_new0(LocusGroupStruct,1);
			groups = g_list_append(groups,group_span);	/* append: keep in order */

			n_groups++;
			group_span->id = n_groups;

			/* store feature coords for sorting into groups */
			/* we create groups of loci that overlap as locus features
			* this is the only way to have stable groups as text itens
			* can overlap differently according to the window height
			* and NOTE it has more relevance biologically
			*/
			group_span->y1 = locus->feature.feature->x1;
			group_span->y2 = locus->feature.feature->x2;
		}

		locus->group = group_span;

		if(locus->feature.feature->x2 > group_span->y2)
			group_span->y2 = locus->feature.feature->x2;

		group_span->height += text_h;
//printf("locus %s in group %d %.1f,%.1f (%.1f)\n",g_quark_to_string(locus->feature.feature->original_id), group_span->id, group_span->y1, group_span->y2, group_span->height);

		needed += text_h;
	}

	spare = end - start - needed;

		/* de-overlap names in each group, extending downwards but leaving groups in the same place */
		/* change group span into text extent not locus extent */
	for(l = visible, group_span = NULL;l; l = l->next)
	{
		ZMapWindowCanvasLocus locus = (ZMapWindowCanvasLocus) l->data;

		if(group_span == locus->group)
		{
/* may need this space */
//			if(locus->ytext < cur_y)
				locus->ytext = cur_y;
		}
		else
		{
			group_span = locus->group;
			group_span->y1 = locus->ytext;
		}
		cur_y = locus->ytext + text_h;
		group_span->y2 = cur_y;
		group_span->height = group_span->y2 - group_span->y1;
//printf("locus %s/%d @ %.1f(%.1f)\n",g_quark_to_string(locus->feature.feature->original_id), group_span->id, locus->ytext, group_span->height);
	}

		/* de-overlap each group moving upwards  */
	for(l = g_list_last(groups), cur_y = end; l; l = l->prev)
	{
		group_span = (LocusGroup) l->data;

		if(cur_y - group_span->y2 > spare)
		{
//			group_span->y1 += spare;
		}

		/* as we extended loci downwards we can shift upwards, there will be space */
		if(group_span->y2 > cur_y)
		{
			double shift = group_span->y2 - cur_y;

			if(shift > 0.0)		/* we overlap the end or the following group */
			{
				group_span->y1 -= shift;
				group_span->y2 -= shift;
			}

			shift = start - group_span->y1;	/* but in case there is no spare space force to be on screen */
			if(shift > 0.0)
			{
				group_span->y1 += shift;
				group_span->y2 += shift;
			}

		}
		cur_y = group_span->y1;
//printf("group %d @ %.1f\n",group_span->id,cur_y);
	}

	/* now move the loci to where the groups are */
	/* and set the feature extent so they paint */
	for(l = visible, group_span = NULL;l; l = l->next)
	{
		ZMapWindowCanvasLocus locus = (ZMapWindowCanvasLocus) l->data;

		if(group_span != locus->group)
		{
			group_span = locus->group;
			cur_y = group_span->y1;
		}

		locus->ytext = cur_y;
//printf("locus %s/%d @ %.1f\n",g_quark_to_string(locus->feature.feature->original_id), group_span->id, cur_y);

		locus->feature.y1 = cur_y - text_h / 2;
		locus->feature.y2 = cur_y + text_h / 2;

		if(locus->ylocus < locus->feature.y1)
			locus->feature.y1 = locus->ylocus;
		if(locus->ylocus > locus->feature.y2)
			locus->feature.y2 = locus->ylocus;

		cur_y += text_h;

	}

	if(groups)
		g_list_free(groups);

	return visible;
}



/* de-overlap and hide features if necessary,
 * NOTE that we never change the zoom,
 * but we'll always get called once for display (NOTE before creating the index)
 * need to be sure we get called if features are added or deleted
 * or if the selection of which to display changes
 * or the window is resized
 */
static void zMapWindowCanvasLocusZoomSet(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable)
{
	GList *l;
	double len, width, f_width = featureset->width;
	char *text;
	int n_loci = 0;
	GList *visible = NULL;
	double span = featureset->end - featureset->start + 1.0;
	double text_h;
//	FooCanvasItem *foo = (FooCanvasItem *) featureset;

	ZMapWindowCanvasLocusSet lset = (ZMapWindowCanvasLocusSet) featureset->opt;
//printf("locus zoom\n");
	zmapWindowCanvasLocusGetPango(drawable, featureset, lset);

	featureset->width = 0.0;

	for(l = featureset->features; l ; l = l->next)		/* (we get called before the index is created) */
	{
//		ZMapWindowCanvasLocus locus = (ZMapWindowCanvasLocus) sl->data;
		ZMapWindowCanvasLocus locus = (ZMapWindowCanvasLocus) l->data;

		locus->ylocus = locus->feature.feature->x1;
		locus->ytext = locus->feature.feature->x1;
		locus->x_off = ZMAP_LOCUS_LINE_WIDTH;

		text = (char *) g_quark_to_string(locus->feature.feature->original_id);
		len = strlen(text);

#if 0
not implemented yet!
		if(filtered(text))
		{
			feature->flags |= FEATURE_HIDDEN | FEATURE_HIDE_FILTER;
		}
		else
#endif
		{
			/* expand the column if needed */
			width = locus->x_off + len * lset->pango.text_width;
			if(width > featureset->width)
				featureset->width = width;
			n_loci++;
			visible = g_list_prepend(visible,(gpointer) locus);
//printf("visible locus %s\n",g_quark_to_string(locus->feature.feature->original_id));
		}
	}
	/* do more filtering if there are too many loci */

	text_h = lset->pango.text_height / featureset->zoom;	/* can't use foo_canvas_c2w as it does a scroll offset */

	if(n_loci * text_h > span)
	{
//		zMapWarning("Need to filter some more %d %.1f %.1f",n_loci,text_h, span);
	}

	if(f_width != featureset->width)
		zMapWindowCanvasFeaturesetRequestReposition((FooCanvasItem *) featureset);


	/* de-overlap the loci left over
	 * this will move features around and expand their extents
	 * after this fucntion the featureset code will create the index
	 */
	if(featureset->display_index)		/* make sure it gets re-created */
      {
		zMapSkipListDestroy(featureset->display_index, NULL);
		featureset->display_index = NULL;
      }

	if(n_loci > 1)
		visible = deOverlap(visible, n_loci, text_h, featureset->start, featureset->end);

	g_list_free(visible);
}



static void zMapWindowCanvasLocusFreeSet(ZMapWindowFeaturesetItem featureset)
{
	ZMapWindowCanvasLocusSet lset = (ZMapWindowCanvasLocusSet) featureset->opt;

	if(lset)
		zmapWindowCanvasFeaturesetFreePango(&lset->pango);
}


static void zMapWindowCanvasLocusAddFeature(ZMapWindowFeaturesetItem featureset, ZMapFeature feature, double y1, double y2)
{
	(void) zMapWindowFeaturesetAddFeature(featureset, feature, y1, y2);

	featureset->zoom = 0.0;		/* force recalc of de-overlap */
						/* force display at all */
//	printf("added locus feature %s\n",g_quark_to_string(feature->original_id));
}



void zMapWindowCanvasLocusInit(void)
{
	gpointer funcs[FUNC_N_FUNC] = { NULL };

	funcs[FUNC_PAINT] = zMapWindowCanvasLocusPaintFeature;
	funcs[FUNC_ZOOM]   = zMapWindowCanvasLocusZoomSet;
	funcs[FUNC_FREE]   = zMapWindowCanvasLocusFreeSet;
	funcs[FUNC_ADD]   = zMapWindowCanvasLocusAddFeature;

	zMapWindowCanvasFeatureSetSetFuncs(FEATURE_LOCUS, funcs, sizeof(zmapWindowCanvasLocusStruct), sizeof(zmapWindowCanvasLocusSetStruct));
}

