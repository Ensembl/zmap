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
#include <zmapWindowCanvasAlignment_I.h>


/* optimise setting of colours, thes have to be GdkParsed and mapped to the canvas */
/* we has a flag to set these on the first draw operation which requires map and relaise of the canvas to have occured */


gboolean decoration_set_G = FALSE;
GdkColor colinear_gdk_G[COLINEARITY_N_TYPE];


GHashTable *align_glyph_G = NULL;



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




/*
James says:

These are the rules I implemented in the ExonCanvas for flagging good splice sites:

1.)  Canonical splice donor sites are:

    Exon|Intron
        |gt
       g|gc

2.)  Canonical splice acceptor site is:

    Intron|Exon
        ag|
*/

/*
These can be configured in/out via styles:
sub-features=non-concensus-splice:nc-splice-glyph
*/

static gboolean fragments_splice(char *fragment_a, char *fragment_b)
{
  gboolean splice = FALSE;

    // NB: DNA always reaches us as lower case, see zmapUtils/zmapDNA.c
  if(!fragment_a || !fragment_b)
    return(splice);

      /* we have fwd strand bases (not reverse complemented)
       * but need to test both directions as splicing is not strand sensitive...
       * in-line rev comp makes me go cross-eyed 8-(
       */

  if(!g_ascii_strncasecmp(fragment_b, "AG",2))
    {
      if(!g_ascii_strncasecmp(fragment_a+1,"GT",2) || !g_ascii_strncasecmp(fragment_a,"GGC",3))
           splice = TRUE;
    }
  else if(!g_ascii_strncasecmp(fragment_a+1, "CT",2))
    {
      if(!g_ascii_strncasecmp(fragment_b,"AC",2) || !g_ascii_strncasecmp(fragment_b,"GCC",3))
           splice = TRUE;
    }
#if 0
zMapLogWarning("nc splice = %d (%.3s, %.3s)",splice,fragment_a,fragment_b);
#endif
  return splice;
}



gboolean is_nc_splice(ZMapFeature left,ZMapFeature right)
{
      char *ldna, *rdna;
      gboolean ret = FALSE;

      /* MH17 NOTE
       *
       * for reverse strand we need to splice backwards
       * logically we could have a mixed series
       * we do get duplicate series and these can be reversed
       * we assume any reversal is a series break
       * and do not attempt to splice inverted exons
       */

          // 3' end of exon: get 1 base  + 2 from intron
      ldna = zMapFeatureGetDNA((ZMapFeatureAny)left,
                         left->x2,
                         left->x2 + 2,
                         0); //prev_reversed);

            // 5' end of exon: get 2 bases from intron
            // NB if reversed we need 3 bases
      rdna = zMapFeatureGetDNA((ZMapFeatureAny)right,
                         right->x1 - 2,
                         right->x1,
                         0); //curr_reversed);

	if(!fragments_splice(ldna,rdna))
		ret = TRUE;

	if(ldna) g_free(ldna);
	if(rdna) g_free(rdna);

	return ret;
}



static void zMapWindowCanvasAlignmentPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable)
{
	ZMapWindowCanvasAlignment align = (ZMapWindowCanvasAlignment) feature;
	int cx1, cy1, cx2, cy2;
	FooCanvasItem *foo = (FooCanvasItem *) featureset;
	ZMapFeatureTypeStyle homology = NULL,nc_splice = NULL;
	gulong fill,outline;
	int colours_set, fill_set, outline_set;

	colours_set = zMapWindowCanvasFeaturesetGetColours(featureset, feature, &fill, &outline);
	fill_set = colours_set & WINDOW_FOCUS_CACHE_FILL;
	outline_set = colours_set & WINDOW_FOCUS_CACHE_OUTLINE;

	zMapCanvasFeaturesetDrawBoxMacro(featureset,feature,drawable,fill_set,outline_set,fill,outline);
	/* the above is once again identical to zMapWindowCanvasAlignmentPaintFeature() and we could call that instead */

	if(featureset->bumped)
	{
		if(!decoration_set_G)
		{
			char *colours[] = { "black", "red", "orange", "green" };
			int i;
			GdkColor colour;
			gulong pixel;
			FooCanvasItem *foo = (FooCanvasItem *) featureset;

			/* cache the colours for colinear lines */
			for(i = 1; i < COLINEARITY_N_TYPE; i++)
			{
				gdk_color_parse(colours[i],&colour);
				pixel = zMap_gdk_color_to_rgba(&colour);
				colinear_gdk_G[i].pixel = foo_canvas_get_color_pixel(foo->canvas,pixel);
			}

			decoration_set_G = TRUE;
		}

		/* add some gaps */

		/* add some decorations */

		if(!align->bump_set)
		{
			/* NOTE this code assumes the styles have been set up with:
			 * 	'glyph-strand=flip-y'
			 * which will handle reverse strand indication */

			/* set the glyph pointers if applicable */
			homology = feature->feature->style->sub_style[ZMAPSTYLE_SUB_FEATURE_HOMOLOGY];
			nc_splice = feature->feature->style->sub_style[ZMAPSTYLE_SUB_FEATURE_NON_CONCENCUS_SPLICE];

			/* find and/or allocate glyph structs */
			if(!feature->left && homology)	/* top end homology incomplete ? */
			{
				double score = feature->feature->feature.homol.y1 - 1;

				if(score)
					align->glyph5 = zMapWindowCanvasGetGlyph(featureset, homology, feature->feature, 5, score);
			}

			if(feature->right && nc_splice)	/* nc-splice ?? */
			{
				if(is_nc_splice(feature->feature,feature->right->feature))
				{
					ZMapWindowCanvasAlignment next = (ZMapWindowCanvasAlignment) feature->right;

					align->glyph3 = zMapWindowCanvasGetGlyph(featureset, nc_splice, feature->feature, 3, 0.0);
					next->glyph5 = zMapWindowCanvasGetGlyph(featureset, nc_splice, next->feature.feature, 5, 0.0);
				}
			}

			if(!feature->right && homology)	/* bottom end homology incomplete ? */
			{
				double score = feature->feature->feature.homol.length - feature->feature->feature.homol.y2;

				if(score)
					align->glyph3 = zMapWindowCanvasGetGlyph(featureset, homology, feature->feature, 3, score);
			}

//zMapLogWarning("feature glyph %f,%f %s, %s", feature->y1,feature->y2,g_quark_to_string(align->glyph5->shape->id),g_quark_to_string(align->glyph3->shape->id));

			align->bump_set = TRUE;
		}

		if(!feature->left)	/* first feature: draw colinear lines */
		{
			ZMapWindowCanvasFeature	feat;
			double mid_x;

			mid_x = featureset->dx + (featureset->width / 2) + feature->bump_offset;

			for(feat = feature;feat->right; feat = feat->right)
			{
				ZMapWindowCanvasAlignment next = (ZMapWindowCanvasAlignment) feat->right;
				GdkColor *colour;

				colour = zmapWindowCanvasAlignmentGetFwdColinearColour((ZMapWindowCanvasAlignment) feat);

				/* get item canvas coords */
				/* note we have to use real feature coords here as we have mangle the first feature's y2 */
				foo_canvas_w2c (foo->canvas, mid_x, feat->feature->x2 - featureset->start + featureset->dy + 1, &cx1, &cy1);
				foo_canvas_w2c (foo->canvas, mid_x, next->feature.y1 - featureset->start + featureset->dy, &cx2, &cy2);

				/* draw line between boxes, don't overlap the pixels */
				gdk_gc_set_foreground (featureset->gc, colour);
				gdk_draw_line (drawable, featureset->gc, cx1, ++cy1, cx2, --cy2);
			}
		}

		zMapWindowCanvasGlyphPaintSubFeature(featureset, feature, align->glyph5, drawable);
		zMapWindowCanvasGlyphPaintSubFeature(featureset, feature, align->glyph3, drawable);
	}
}




/* get sequence extent of compound alignment (for bumping) */
/* NB: canvas coords could overlap due to sub-pixel base pairs
 * which could give incorrect de-overlapping
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


