/*  File: zmapWindowCanvasAlignment.c
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
 *-------------------------------------------------------------------
 * ZMap is free software; you can refeaturesetstribute it and/or
 * mofeaturesetfy it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is featuresetstributed in the hope that it will be useful,
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
 * implements callback functions for FeaturesetItem alignemnt features
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>





#include <math.h>
#include <string.h>
#include <ZMap/zmapFeature.h>
#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasBasic_I.h>
#include <zmapWindowCanvasAlignment_I.h>


/* optimise setting of colours, thes have to be GdkParsed and mapped to the canvas */
/* we has a flag to set these on the first draw operation which requires map and relaise of the canvas to have occured */


gboolean colinear_gdk_set_G = FALSE;
GdkColor colinear_gdk_G[COLINEARITY_N_TYPE];


/* given an alignment sub-feature return the colour or the colinearity line to the next sub-feature */
static GdkColor *zmapWindowCanvasAlignmentGetFwdColinearColour(ZMapWindowCanvasAlignment align)
{
	ZMapWindowCanvasAlignment next = (ZMapWindowCanvasAlignment) align->feature.right;
	int diff;
	int start2,end1;
	int threshold = (int) zMapStyleGetWithinAlignError(align->feature.feature->style);
	ColinearityType ct = COLINEAR_PERFECT;
	ZMapHomol h1,h2;

	/* apparently this works thro revcomp:
	 *
       * "When match is from reverse strand of homol then homol blocks are in reversed order
       * but coords are still _forwards_. Revcomping reverses order of homol blocks
       * but as before coords are still forwards."
	 */
	if(align->feature.feature->strand == align->feature.feature->feature.homol.strand)
	{
		h1 = &align->feature.feature->feature.homol;
		h2 = &next->feature.feature->feature.homol;
	}
	else
	{
		h2 = &align->feature.feature->feature.homol;
		h1 = &next->feature.feature->feature.homol;
	}
	end1 = h1->y2;
	start2 = h2->y1;
	diff = start2 - end1 - 1;
	if(diff < 0)
		diff = -diff;

	if(diff > threshold)
	{
		if(start2 < end1)
			ct = COLINEAR_NOT;
		else
			ct = COLINEAR_IMPERFECT;
	}

	return &colinear_gdk_G[ct];
}



static void zMapWindowCanvasAlignmentPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable)
{
	ZMapWindowCanvasAlignment align = (ZMapWindowCanvasAlignment) feature;
	int cx1, cy1, cx2, cy2;
	double mid_x;
	FooCanvasItem *foo = (FooCanvasItem *) featureset;

	/* better to cut and paste this to avoid repeating calculations if bumped*/
	zMapWindowCanvasBasicPaintFeature(featureset, feature, drawable);


	if(featureset->bumped)
	{

		if(!colinear_gdk_set_G)
		{
			char *colours[] = { "black", "red", "orange", "green" };
			int i;
			GdkColor colour;
			gulong pixel;
			FooCanvasItem *foo = (FooCanvasItem *) featureset;

			for(i = 1; i < COLINEARITY_N_TYPE; i++)
			{
				gdk_color_parse(colours[i],&colour);
				pixel = zMap_gdk_color_to_rgba(&colour);
				colinear_gdk_G[i].pixel = foo_canvas_get_color_pixel(foo->canvas,pixel);
			}
			colinear_gdk_set_G = TRUE;
		}

		/* add some gaps */

		/* add some decorations */
		mid_x = featureset->dx + (featureset->width / 2) + feature->bump_offset;

		for(;feature->right; feature = feature->right)
		{
			ZMapWindowCanvasAlignment next = (ZMapWindowCanvasAlignment) feature->right;
			GdkColor *colour;

			align = (ZMapWindowCanvasAlignment) feature;
			colour = zmapWindowCanvasAlignmentGetFwdColinearColour(align);

			/* get item canvas coords */
			/* note we have to use real feature coords here as we have mangle the first feature's y2 */
			foo_canvas_w2c (foo->canvas, mid_x, feature->feature->x2 - featureset->start + featureset->dy, &cx1, &cy1);
			foo_canvas_w2c (foo->canvas, mid_x, next->feature.y1 - featureset->start + featureset->dy + 1, &cx2, &cy2);

			/* draw line between boxes, don't overlap the pixels */
			gdk_gc_set_foreground (featureset->gc, colour);
			gdk_draw_line (drawable, featureset->gc, cx1, ++cy1, cx2, --cy2);
		}
	}
}

/* get sequence extent of compound alignment (for bumping)*/
/* NB: canvas coords could overlap due to sub-pixel base pairs
 *which could give incorrect de-overlapping
 * that would be revealed on Zoom
 */
/*
 * we adjust the extent fo the forst CanvasAlignment to cover tham all
 * as the first one draws all the colinear lines
 */

static void zMapWindowCanvasAlignmentGetFeatureExtent(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, ZMapSpan span)
{
	ZMapWindowCanvasFeature first = feature;
	double extra;

	while(first->left)
		first = first->left;

	while(feature->right)
		feature = feature->right;

	extra = feature->y2 - first->y2;
	if(extra > featureset->bump_extra_overlap)
		featureset->bump_extra_overlap = extra;

	first->y2 = feature->y2;

	span->x1 = first->y1;
	span->x2 = first->y2;
}





void zMapWindowCanvasAlignmentInit(void)
{
	gpointer funcs[FUNC_N_FUNC] = { NULL };

	funcs[FUNC_PAINT]  = zMapWindowCanvasAlignmentPaintFeature;
	funcs[FUNC_EXTENT] = zMapWindowCanvasAlignmentGetFeatureExtent;
#if CANVAS_FEATURESET_LINK_FEATURE
	funcs[FUNC_LINK]   = zMapWindowCanvasAlignmentLinkFeature;
#endif

	zMapWindowCanvasFeatureSetSetFuncs(FEATURE_ALIGN, funcs, sizeof(zmapWindowCanvasAlignmentStruct));

}



#if CANVAS_FEATURESET_LINK_FEATURE

/* set up same name links by proxy and add homology data to CanvasAlignment struct */

int zMapWindowCanvasAlignmentLinkFeature(ZMapWindowCanvasFeature feature)
{
	static ZMapFeature prev_feat;
	static GQuark name = 0;
	static double start, end;
	ZMapFeature feat;
//	ZMapHomol homol;
	gboolean link = FALSE;

	if(!feature)
	{
		start = end = 0;
		name = 0;
		prev_feat = NULL;
		return FALSE;
	}

	zMapAssert(feature->type == FEATURE_ALIGN);

	/* link by same name
	 * homologies can get confusing...
	 * we get alignments from exonerate the get chopped into pieces
	 * and we can sort these into target (genomic) start coord order
	 * the query coords do not always form an ascendign series
	 * ESTs tend to be OK (almost all green), proteins are often not (red)
	 */
	/* so we link by name only */

	feat = feature->feature;
//	homol  = &feat->feature.homol;

	if(name == feat->original_id && feat->strand == prev_feat->strand)
	{
		link = TRUE;
	}
	prev_feat = feat;
	name = feat->original_id;
	start = homol->y1;
	end = homol->y2;

	/* leave homology and gaps data till we need it */

	return link;
}
#endif


