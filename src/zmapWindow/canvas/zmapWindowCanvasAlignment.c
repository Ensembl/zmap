/*  File: zmapWindowCanvasAlignment.c
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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

#include <ZMap/zmapUtilsLog.h>

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
#define DEBUG_SPLICE 0

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
#if DEBUG_SPLICE
zMapLogWarning("concensus splice = %d (%.3s, %.3s)",splice,fragment_a,fragment_b);
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

#if DEBUG_SPLICE
zMapLogWarning("splice %s",g_quark_to_string(left->unique_id));
#endif

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


static AlignGap align_gap_free_G = NULL;

static long n_block_alloc = 0;
static long n_gap_alloc = 0;
static long n_gap_free = 0;



void align_gap_free(AlignGap ag)
{
	ag->next = align_gap_free_G;
	align_gap_free_G = ag;

	n_gap_free++;
}

/* simple list structure, avioding extra malloc/free associated with GList code */
AlignGap align_gap_alloc(void)
{
	int i;
	AlignGap ag = NULL;

	if(!align_gap_free_G)
	{
		gpointer mem = g_malloc(sizeof(AlignGapStruct) * N_ALIGN_GAP_ALLOC);
		for(i = 0; i < N_ALIGN_GAP_ALLOC; i++)
		{
			align_gap_free(((AlignGap) mem) + i);
		}

		n_block_alloc++;
	}

	if(align_gap_free_G)
	{
		ag = (AlignGap) align_gap_free_G;
		align_gap_free_G = align_gap_free_G->next;
		memset((gpointer) ag, 0, sizeof(AlignGapStruct));

		n_gap_alloc++;
	}
	return ag;
}


/* NOTE the gaps array is not explicitly sorted, but is only provided by ACEDB and subsequent pipe scripts written by otterlace
 * and it is believed that the match blocks are always provided in start coordinate order (says Ed)
 * So it's a bit slack trusting an external program but ZMap has been doing that for a long time.
 * "roll on CIGAR strings" which prevent this kind of problem
 * NOTE that sorting a GArray might sort the data structures themselves, so schoolboy error kind of slow.
 * If they do need to be sorted then the place is in zmapGFF2Parser.c/loadGaps()
 *
 * sorting is interesting here as the display optimisation would produce a comppletely wrong picture if it was not done
 *
 * we draw boxes etc at the target coordinates ie on the genomic sequence
 *
 * NOTE for short reads we have an option to squash those that have the same gap
 * (they only have one gap ,except for a few pathological cases)
 * in this case the first and last blocks are a diff colour, so we flag this if the colour is visible and add another box not a line. Yuk
 *
 */
AlignGap make_gapped(ZMapFeature feature, double offset, FooCanvasItem *foo)
{
	int i;
	AlignGap ag;
	AlignGap last_box = NULL;
	AlignGap last_ag = NULL;
	AlignGap display_ag = NULL;
	GArray *gaps = feature->feature.homol.align;

	ZMapAlignBlock ab;
	int cy1,cy2,fy1;
	int n = gaps->len;
	gboolean edge;

	foo_canvas_w2c (foo->canvas, 0, feature->x1 + offset, NULL, &fy1);

	for(i = 0; i < n ;i++)
	{
		ab = &g_array_index(gaps, ZMapAlignBlockStruct, i);

		/* get pixel coords of block relative to feature y1, +1 to cover all of last base on cy2 */
		foo_canvas_w2c (foo->canvas, 0, ab->t1 + offset, NULL, &cy1);
		foo_canvas_w2c (foo->canvas, 0, ab->t2 + 1 + offset, NULL, &cy2);
		cy1 -= fy1;
		cy2 -= fy1;


		if(last_box)
		{
			if(i == 1 && (feature->flags.squashed_start))
			{
				last_box->edge = TRUE;
				if(last_box->y2 - last_box->y1 > 2)
					last_box = NULL;	/* force a new box if the colours are visible */
			}
			edge = FALSE;
			if(i == (n-1) && (feature->flags.squashed_end))
			{
				if(last_box && last_box->y2 - last_box->y1 > 2)
					last_box = NULL;	/* force a new box if the colours are visible */
				edge = TRUE;
			}
		}
		if(last_box)
		{
			if(last_box->y2 == cy1 && cy2 != cy1)
			{
				/* extend last box and add a line where last box ended */
				ag = align_gap_alloc();
				ag->y1 = last_box->y2;
				ag->y2 = last_box->y2;
				ag->type = GAP_HLINE;
				last_ag->next = ag;
				last_ag = ag;
				last_box->y2 = cy2;
			}

			else if(last_box->y2 < cy1 - 1)
			{
				/* visible gap between boxes: add a colinear line ? */
				ag = align_gap_alloc();
				ag->y1 = last_box->y2;
				ag->y2 = cy1;
				ag->type = GAP_VLINE;
				last_ag->next = ag;
				last_ag = ag;
			}

			if(last_box->y2 < cy1)
			{
				/* create new box if edges do not overlap */
				last_box = NULL;
			}
		}

		if(!last_box)
		{
			/* add a new box */
			ag = align_gap_alloc();
			ag->y1 = cy1;
			ag->y2 = cy2;
			ag->type = GAP_BOX;
			ag->edge = edge;

			if(last_ag)
				last_ag->next = ag;
			last_box = last_ag = ag;
			if(!display_ag)
				display_ag = ag;
		}

	}

	return display_ag;
}



static void zMapWindowCanvasAlignmentPaintFeature(ZMapWindowFeaturesetItem featureset,
						  ZMapWindowCanvasFeature feature,
						  GdkDrawable *drawable, GdkEventExpose *expose)
{
  ZMapWindowCanvasAlignment align = (ZMapWindowCanvasAlignment) feature;
  int cx1, cy1, cx2, cy2;
  FooCanvasItem *foo = (FooCanvasItem *) featureset;
  ZMapFeatureTypeStyle homology = NULL,nc_splice = NULL;
  gulong fill,outline;
  int colours_set, fill_set, outline_set;
  gulong edge;
  gboolean edge_set;
  double mid_x;
  double x1,x2;

  /* if bumped we can have a very wide column: don't paint if not visible */
  /* display code assumes a narrow column w/ overlapping features and does not process x-coord */
  /* this makes a huge difference to display speed */

  mid_x = featureset->dx + (featureset->width / 2);
  if(featureset->bumped)
    mid_x += feature->bump_offset;
  x1 = mid_x - feature->width / 2;
  x2 = x1 + feature->width;
  foo_canvas_w2c (foo->canvas, x1, 0, &cx1, NULL);
  foo_canvas_w2c (foo->canvas, x2, 0, &cx2, NULL);

  if(cx2 < expose->area.x || cx1 > expose->area.x + expose->area.width)
    return;


  colours_set = zMapWindowCanvasFeaturesetGetColours(featureset, feature, &fill, &outline);
  fill_set = colours_set & WINDOW_FOCUS_CACHE_FILL;
  outline_set = colours_set & WINDOW_FOCUS_CACHE_OUTLINE;

  if(fill_set && feature->feature->population)
    {
      ZMapFeatureTypeStyle style = feature->feature->style;

      if((zMapStyleGetScoreMode(style) == ZMAPSCORE_HEAT) || (zMapStyleGetScoreMode(style) == ZMAPSCORE_HEAT_WIDTH))
	{
	  fill = (fill << 8) | 0xff;	/* convert back to RGBA */
	  fill = foo_canvas_get_color_pixel(foo->canvas,
					    zMapWindowCanvasFeatureGetHeatColour(0xffffffff,fill,feature->score));
	}

      if(zMapStyleIsSquash(style))		/* diff colours for first and last box */
	{
	  edge = 0x808080ff;
	  edge_set = TRUE;
#warning get alignment colour for edge
	}
    }

  if(  !(feature->feature->feature.homol.align)  ||
       (
	!zMapStyleIsAlwaysGapped(feature->feature->style) &&
	(!featureset->bumped ||
	 //			featureset->strand == ZMAPSTRAND_REVERSE ||
	 !zMapStyleIsShowGaps(feature->feature->style))
	)
       )
    {
      double x1,x2;

      /* draw a simple ungapped box */
      /* we don't draw gaps on reverse, annotators work on the fwd strand and revcomp if needs be */

      x1 = featureset->width / 2 - feature->width / 2;
      if(featureset->bumped)
	x1 += feature->bump_offset;

      x1 += featureset->dx;
      x2 = x1 + feature->width;

	/* must use feature coords here as the first alignemtn in the series gets expanded to pick up colinear lines
	 * if it's ungapped we'd draw a big box over the whole series
	 */
	zMapCanvasFeaturesetDrawBoxMacro(featureset,x1,x2, feature->feature->x1, feature->feature->x2, drawable, fill_set,outline_set,fill,outline);
	}
  else	/* draw gapped boxes */
    {
      AlignGap ag;
      GdkColor c;
      int cx1, cy1, cx2, cy2;


      /* create a list of things to draw at this zoom taking onto account bases per pixel */
      if(!align->gapped)
	{
	  align->gapped = make_gapped(feature->feature, featureset->dy - featureset->start, foo);
	}

      /* draw them */

      /* get item canvas coords.  gaps data is relative to feature y1 in pixel coordinates */
      foo_canvas_w2c (foo->canvas, x1, feature->feature->x1 - featureset->start + featureset->dy, &cx1, &cy1);
      foo_canvas_w2c (foo->canvas, x2, 0, &cx2, NULL);
      cy2 = cy1;

      for(ag = align->gapped; ag; ag = ag->next)
	{
	  int gy1, gy2, gx;

	  gy1 = cy1 + ag->y1;
	  gy2 = cy1 + ag->y2;

	  switch(ag->type)
	    {
	    case GAP_BOX:
	      if(fill_set && (!outline_set || (gy2 - gy1 > 1)))	/* fill will be visible */
		{
		  c.pixel = fill;
		  gdk_gc_set_foreground (featureset->gc, &c);
		  zMap_draw_rect(drawable, featureset, cx1, gy1, cx2, gy2, TRUE);
		}

	      if(outline_set)
		{
		  c.pixel = outline;
		  gdk_gc_set_foreground (featureset->gc, &c);
		  zMap_draw_rect(drawable, featureset, cx1, gy1, cx2, gy2, FALSE);
		}

	      break;

	    case GAP_HLINE:		/* x coords differ, y is the same */
	      if(!outline_set)	/* must be or else we are up the creek! */
		break;

	      c.pixel = outline;
	      gdk_gc_set_foreground (featureset->gc, &c);
	      zMap_draw_line(drawable, featureset, cx1, gy1, cx2, gy2);
	      break;

	    case GAP_VLINE:		/* y coords differ, x is the same */
	      if(!outline_set)	/* must be or else we are up the creek! */
		break;

	      gx = (int) mid_x;
	      c.pixel = outline;
	      gdk_gc_set_foreground (featureset->gc, &c);
	      zMap_draw_line(drawable, featureset, gx, gy1, gx, gy2);
	      break;
	    }
	}
    }

  if(featureset->bumped)		/* draw decorations */
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

      /* add some decorations */

      if(!align->bump_set)		/* set up glyph data once only */
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
	      /* design by guesswork: this is not well documented */
	      ZMapHomol homol = &feature->feature->feature.homol;
	      double score = homol->y1 - 1;
	      if(feature->feature->strand != homol->strand)
		score = homol->length - homol->y2;

	      if(score)
		align->glyph5 = zMapWindowCanvasGetGlyph(featureset, homology, feature->feature, 5, score);

#if DEBUG
	      char *sig = "";
	      if(align->glyph5)
		sig = g_quark_to_string(zMapWindowCanvasGlyphSignature(homology, feature->feature,5, score));

	      printf("%s glyph5 %f (%d %d, %d) %d/%d = %s\n", g_quark_to_string(feature->feature->original_id), score, 1, feature->feature->feature.homol.y1, feature->feature->feature.homol.y2, feature->feature->feature.homol.strand, feature->feature->strand, sig);
#endif
	    }

	  if(feature->right && nc_splice)	/* nc-splice ?? */
	    {
	      if(is_nc_splice(feature->feature,feature->right->feature))
		{
		  ZMapWindowCanvasAlignment next = (ZMapWindowCanvasAlignment) feature->right;

		  align->glyph3 = zMapWindowCanvasGetGlyph(featureset, nc_splice, feature->feature, 3, 0.0);
		  next->glyph5 = zMapWindowCanvasGetGlyph(featureset, nc_splice, next->feature.feature, 5, 0.0);
#if DEBUG_SPLICE
		  zMapLogWarning("set nc splice 3 %s -> %s",g_quark_to_string(feature->feature->unique_id),g_quark_to_string(align->glyph3->sig));
		  zMapLogWarning("set nc splice 5 %s -> %s",g_quark_to_string(feature->right->feature->unique_id),g_quark_to_string(next->glyph5->sig));
#endif
		}
	    }

	  if(!feature->right && homology)	/* bottom end homology incomplete ? */
	    {
	      /* design by guesswork: this is not well documented */
	      ZMapHomol homol = &feature->feature->feature.homol;
	      double score = homol->length - homol->y2;
	      if(feature->feature->strand != homol->strand)
		score = homol->y1 - 1;

	      if(score)
		align->glyph3 = zMapWindowCanvasGetGlyph(featureset, homology, feature->feature, 3, score);

#if DEBUG
	      char *sig = "";
	      if(align->glyph3)
		sig = g_quark_to_string(zMapWindowCanvasGlyphSignature(homology, feature->feature,3, score));

	      /printf("%s glyph3 %f (%d %d,%d) %d/%d = %s\n", g_quark_to_string(feature->feature->original_id), score, feature->feature->feature.homol.length, feature->feature->feature.homol.y1, feature->feature->feature.homol.y2, feature->feature->feature.homol.strand, feature->feature->strand, sig);
#endif
	    }

	  align->bump_set = TRUE;
	}

      if(!feature->left && !zMapStyleIsUnique(feature->feature->style))	/* first feature: draw colinear lines */
	{
	  ZMapWindowCanvasFeature feat;

	  for(feat = feature;feat->right; feat = feat->right)
	    {
	      ZMapWindowCanvasAlignment next = (ZMapWindowCanvasAlignment) feat->right;
	      GdkColor *colour;

	      /* get item canvas coords */
	      /* note we have to use real feature coords here as we have mangled the first feature's y2 */
	      foo_canvas_w2c (foo->canvas, mid_x, feat->feature->x2 - featureset->start + featureset->dy + 1, &cx1, &cy1);
	      foo_canvas_w2c (foo->canvas, mid_x, next->feature.y1 - featureset->start + featureset->dy, &cx2, &cy2);

	      if(cy2 < expose->area.y)
		continue;

	      colour = zmapWindowCanvasAlignmentGetFwdColinearColour((ZMapWindowCanvasAlignment) feat);

	      /* draw line between boxes, don't overlap the pixels */
	      gdk_gc_set_foreground (featureset->gc, colour);
	      zMap_draw_line(drawable, featureset, cx1, ++cy1, cx2, --cy2);

	      if(cy2 > expose->area.y + expose->area.height)
		break;
	    }
	}

#if DEBUG_SPLICE
      zMapLogWarning("paint glyphs %s %p,%p",g_quark_to_string(feature->feature->unique_id),align->glyph5,align->glyph3);
#endif
      /* all features: add glyphs if present */
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
 * we adjust the extent of the first CanvasAlignment to cover tham all
 * as the first one draws all the colinear lines
 */
static void zMapWindowCanvasAlignmentGetFeatureExtent(ZMapWindowCanvasFeature feature, ZMapSpan span, double *width)
{
	ZMapWindowCanvasFeature first = feature;

	*width = feature->width;

	if(!zMapStyleIsUnique(feature->feature->style))
		/* if not joining up same name features they don't need to go in the same column */
	{
		while(first->left)
		{
			first = first->left;
			if(first->width > *width)
			*width = first->width;
		}

		while(feature->right)
		{
			feature = feature->right;
			if(feature->width > *width)
			*width = feature->width;
		}

		first->y2 = feature->y2;
	}

	span->x1 = first->y1;
	span->x2 = first->y2;
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


/*
 * if we are displaying a gapped alignment, recalculate this data
 * do this by freeing the existing data, new stuff will be added by the paint function
 *
 * NOTE ref to FreeSet() below
 */
static void zMapWindowCanvasAlignmentZoomSet(ZMapWindowFeaturesetItem featureset)
{
	ZMapSkipList sl;

	if(featureset->link_sideways && !featureset->linked_sideways)
		zmap_window_featureset_item_link_sideways(featureset);


	/* NOTE display index will be null on first call */

		/* feature specific eg bumped gapped alignments - adjust gaps display */
	for(sl = zMapSkipListFirst(featureset->display_index); sl; sl = sl->next)
	{
		ZMapWindowCanvasAlignment align = (ZMapWindowCanvasAlignment) sl->data;
		AlignGap ag, del;

		if(align->feature.type != FEATURE_ALIGN)	/* we could have other types in the col due to mis-config */
			continue;

		  /* delete the old, these get created on paint */
		for(ag = align->gapped; ag;)
		{
			del = ag;
			ag = ag->next;
			align_gap_free(del);
		}
		align->gapped = NULL;
	}

//printf("alignment zoom: %ld %ld %ld\n",n_block_alloc, n_gap_alloc, n_gap_free);
}



static void zMapWindowCanvasAlignmentAddFeature(ZMapWindowFeaturesetItem featureset, ZMapFeature feature, double y1, double y2)
{
	ZMapWindowCanvasFeature feat = zMapWindowFeaturesetAddFeature(featureset, feature, y1, y2);

	zMapWindowFeaturesetSetFeatureWidth(featureset, feat);

  	if(feat->feature->feature.homol.flags.masked)
  		feat->flags |= focus_group_mask[WINDOW_FOCUS_GROUP_MASKED];

	/* NOTE if we configure styles to not load these into the canvas we don't get here */
	if(feature->flags.collapsed || feature->flags.squashed || feature->flags.joined)
	  feat->flags |= FEATURE_HIDE_COMPOSITE | FEATURE_HIDDEN;

	/* NOTE we may not have an index so this flag must be unset seperately */
	/* eg on OTF w/ delete existing selected */
	if(featureset->link_sideways)
		featureset->linked_sideways = FALSE;

}


static void zMapWindowCanvasAlignmentFreeSet(ZMapWindowFeaturesetItem featureset)
{
	/* frees gapped data _and does not alloc any more_ */
	zMapWindowCanvasAlignmentZoomSet(featureset);
}


static ZMapFeatureSubPartSpan zmapWindowCanvasAlignmentGetSubPartSpan(FooCanvasItem *foo,ZMapFeature feature,double x,double y)
{
	static ZMapFeatureSubPartSpanStruct sub_part;
	ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) foo;

	ZMapAlignBlock ab;
	int i;

	/* find the gap or match if we are bumped */
	if(!fi->bumped)
		return NULL;
	if(!feature->feature.homol.align)	/* is un-gapped */
		return NULL;

#if 0
	if(!y)	/* interface to legacy code they uses G_OBJECT_DATA */
	{
		ZMapWindowFeaturesetItem featureset = (ZMapWindowFeaturesetItem) foo;

		if(!featureset->point_canvas_feature)
			return NULL;
		sub_part.start = !featureset->point_canvas_feature->y1;
		sub_part.end   = !featureset->point_canvas_feature->y2;
		sub_part.subpart = ZMAPFEATURE_SUBPART_MATCH;
		sub_part.index = 1;
		return &sub_part;
	}
#endif

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
			sub_part.end = ab->t1;
			sub_part.subpart = ZMAPFEATURE_SUBPART_GAP;
			return &sub_part;
		}
	}
	return NULL;
}


/* Default function to check if the given x,y coord is within a feature, this
 * function assumes the feature is box-like. */
static double alignmentPoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
			   double item_x, double item_y, int cx, int cy,
			   double local_x, double local_y, double x_off)
{
  double best = 1.0e36 ;
  double can_start, can_end ;

  /* Get feature extent on display. */
  /* NOTE cannot use feature coords as transcript exons all point to the same feature */
  /* alignments have to implement a special fucntion to handle bumped features - the first exon gets expanded to cover the whole */
  /* when we get upgraded to vulgar strings these can be like transcripts... except that there's a performance problem due to volume */
  /* perhaps better to add  extra display/ search coords to ZMapWindowCancasFeature ?? */
  can_start = gs->feature->x1 ;
  can_end = gs->feature->x2 ;
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



void zMapWindowCanvasAlignmentInit(void)
{
	gpointer funcs[FUNC_N_FUNC] = { NULL };

	funcs[FUNC_PAINT]  = zMapWindowCanvasAlignmentPaintFeature;
	funcs[FUNC_EXTENT] = zMapWindowCanvasAlignmentGetFeatureExtent;
	funcs[FUNC_ZOOM]   = zMapWindowCanvasAlignmentZoomSet;
	funcs[FUNC_FREE]   = zMapWindowCanvasAlignmentFreeSet;

	funcs[FUNC_ADD]    = zMapWindowCanvasAlignmentAddFeature;

	funcs[FUNC_SUBPART] = zmapWindowCanvasAlignmentGetSubPartSpan;

	funcs[FUNC_POINT]   = alignmentPoint;

	zMapWindowCanvasFeatureSetSetFuncs(FEATURE_ALIGN, funcs, sizeof(zmapWindowCanvasAlignmentStruct));
}







