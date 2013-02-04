/*  File: zmapWindowCanvasTranscript.c
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 * implements callback functions for FeaturesetItem transcript features
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <string.h>
#include <ZMap/zmapFeature.h>
#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasTranscript_I.h>



static void zMapWindowCanvasTranscriptPaintFeature(ZMapWindowFeaturesetItem featureset,
						   ZMapWindowCanvasFeature feature,
						   GdkDrawable *drawable, GdkEventExpose *expose)
{
  gulong fill,outline;
  int colours_set, fill_set, outline_set;
  double x1,x2;
  double y1,y2;
  ZMapWindowCanvasTranscript tr = (ZMapWindowCanvasTranscript) feature;
  FooCanvasItem *foo = (FooCanvasItem *) featureset;
  ZMapTranscript transcript = &feature->feature->feature.transcript;

  /* draw a box */

  /* colours are not defined for the CanvasFeatureSet
   * as we can have several styles in a column
   * but they are cached by the calling function
   * and also the window focus code
   * however, as we have CDS colours diff from non-CDS and possibly diff style in one column
   * this is likely ineffective, but as the number of features is small we don't care so much
   */

  colours_set = zMapWindowCanvasFeaturesetGetColours(featureset, feature, &fill, &outline);		/* non cds colours */
  fill_set = colours_set & WINDOW_FOCUS_CACHE_FILL;
  outline_set = colours_set & WINDOW_FOCUS_CACHE_OUTLINE;

  x1 = featureset->width / 2 - feature->width / 2;	/* NOTE works for exons not introns */
  if(featureset->bumped)
    x1 += feature->bump_offset;

  x1 += featureset->dx;
  x2 = x1 + feature->width - 1;
  y1 = feature->y1;
  y2 = feature->y2;

  if(tr->sub_type == TRANSCRIPT_EXON)
    {
      if(transcript->flags.cds && transcript->cds_start > y1 && transcript->cds_start < y2)

	{
	  /* draw a utr box at the start of the exon */
	  zMapCanvasFeaturesetDrawBoxMacro(featureset, x1, x2, y1, transcript->cds_start-1, drawable, fill_set,outline_set,fill,outline);
	  y1 = transcript->cds_start;
	}
      if(transcript->flags.cds && transcript->cds_end > y1 && transcript->cds_end < y2)
	{
	  /* draw a utr box at the end of the exon */
	  zMapCanvasFeaturesetDrawBoxMacro(featureset, x1, x2, transcript->cds_end + 1, y2, drawable, fill_set,outline_set,fill,outline);
	  y2 = transcript->cds_end;
	}
    }

  /* get cds colours if in CDS */
  if(transcript->flags.cds && transcript->cds_start <= y1 && transcript->cds_end >= y2)
    {
      ZMapStyleColourType ct;
      GdkColor *gdk_fill = NULL, *gdk_outline = NULL;

      ct = feature->flags & WINDOW_FOCUS_GROUP_FOCUSSED ? ZMAPSTYLE_COLOURTYPE_SELECTED : ZMAPSTYLE_COLOURTYPE_NORMAL;
      zMapStyleGetColours(*feature->feature->style, STYLE_PROP_TRANSCRIPT_CDS_COLOURS, ct, &gdk_fill, NULL, &gdk_outline);

      if(gdk_fill)
	{
	  gulong fill_colour;

	  fill_set = TRUE;
	  fill_colour = zMap_gdk_color_to_rgba(gdk_fill);
	  fill = foo_canvas_get_color_pixel(foo->canvas, fill_colour);
	}
      if(gdk_outline)
	{
	  gulong outline_colour;

	  outline_set = TRUE;
	  outline_colour = zMap_gdk_color_to_rgba(gdk_outline);
	  outline = foo_canvas_get_color_pixel(foo->canvas, outline_colour);
	}
    }


  if(tr->sub_type == TRANSCRIPT_EXON)
    {
      zMapCanvasFeaturesetDrawBoxMacro(featureset, x1, x2, y1, y2, drawable, fill_set,outline_set,fill,outline);
    }
  else if (outline_set)
    {
      if(tr->sub_type == TRANSCRIPT_INTRON)
	{
	  GdkColor c;
	  int cx1, cy1, cx2, cy2, cy1_5, cx1_5;

	  /* get item canvas coords in pixel coordinates */
	  /* NOTE not quite sure why y1 is 1 out when y2 isn't */
	  foo_canvas_w2c (foo->canvas, x1, feature->y1 - featureset->start + featureset->dy, &cx1, &cy1);
	  foo_canvas_w2c (foo->canvas, x2, feature->y2 - featureset->start + featureset->dy + 1, &cx2, &cy2);
	  cy1_5 = (cy1 + cy2) / 2;
	  cx1_5 = (cx1 + cx2) / 2;
	  c.pixel = outline;
	  gdk_gc_set_foreground (featureset->gc, &c);
	  zMap_draw_line(drawable, featureset, cx1_5, cy1, cx2, cy1_5);
	  zMap_draw_line(drawable, featureset, cx2, cy1_5, cx1_5, cy2);
	}
      else if(tr->sub_type == TRANSCRIPT_INTRON_START_NOT_FOUND)
	{
	  GdkColor c;
	  int cx1, cy1, cx2, cy2, cx1_5;

	  /* get item canvas coords in pixel coordinates */
	  /* NOTE not quite sure why y1 is 1 out when y2 isn't */
	  foo_canvas_w2c (foo->canvas, x1, feature->y1 - featureset->start + featureset->dy, &cx1, &cy1);
	  foo_canvas_w2c (foo->canvas, x2, feature->y2 - featureset->start + featureset->dy + 1, &cx2, &cy2);
	  cx1_5 = (cx1 + cx2) / 2;
	  c.pixel = outline;
	  gdk_gc_set_foreground (featureset->gc, &c);
	  zMap_draw_broken_line(drawable, featureset, cx2, cy1, cx1_5, cy2);
	}
      else if(tr->sub_type == TRANSCRIPT_INTRON_END_NOT_FOUND)
	{
	  GdkColor c;
	  int cx1, cy1, cx2, cy2, cx1_5;

	  /* get item canvas coords in pixel coordinates */
	  /* NOTE not quite sure why y1 is 1 out when y2 isn't */
	  foo_canvas_w2c (foo->canvas, x1, feature->y1 - featureset->start + featureset->dy, &cx1, &cy1);
	  foo_canvas_w2c (foo->canvas, x2, feature->y2 - featureset->start + featureset->dy + 1, &cx2, &cy2);
	  cx1_5 = (cx1 + cx2) / 2;
	  c.pixel = outline;
	  gdk_gc_set_foreground (featureset->gc, &c);
	  zMap_draw_broken_line(drawable, featureset, cx1_5, cy1, cx2, cy2);
	}
    }
}




static ZMapWindowCanvasFeature zMapWindowCanvasTranscriptAddFeature(ZMapWindowFeaturesetItem featureset, ZMapFeature feature, double y1, double y2)
{
	int ni = 0, ne = 0, i;
	GArray *introns,*exons;
	ZMapWindowCanvasFeature feat;
	ZMapWindowCanvasTranscript tr = NULL;
	double fy1,fy2;
	ZMapSpan exon,intron;


	introns = feature->feature.transcript.introns;
	exons = feature->feature.transcript.exons;
	if(introns)
		ni = introns->len;
	if(exons)
		ne = exons->len;

#if USE_DOTTED_LINES
	if(feature->feature.transcript.flags.start_not_found)	/* add dotted line fading away into the distance */
	{
		double trunc_len = zMapStyleGetTruncatedIntronLength(*feature->style);
		if(!trunc_len)
			trunc_len = y1 - featureset->start;

		fy2 = y1;
		fy1 = fy2 - trunc_len;

		feat = zMapWindowFeaturesetAddFeature(featureset, feature, fy1, fy2);
		feat->width = featureset->width;

		tr = (ZMapWindowCanvasTranscript) feat;
		tr->sub_type = TRANSCRIPT_INTRON_START_NOT_FOUND;
		tr->index = -1;
	}
#endif

	for(i = 0; i < ne; i++)
	{
		exon = &g_array_index(exons,ZMapSpanStruct,i);
		fy1 = y1 - feature->x1 + exon->x1;
		fy2 = y1 - feature->x1 + exon->x2;

		feat = zMapWindowFeaturesetAddFeature(featureset, feature, fy1, fy2);
		feat->width = featureset->width;

		if(tr)
		{
			tr->feature.right = feat;
			feat->left = &tr->feature;
		}

		tr = (ZMapWindowCanvasTranscript) feat;
		tr->sub_type = TRANSCRIPT_EXON;
		tr->index = i;

		if(i < ni)
		{
			intron = &g_array_index(introns,ZMapSpanStruct,i);
			fy1 = y1 - feature->x1 + intron->x1;
			fy2 = y1 - feature->x1 + intron->x2;

			feat = zMapWindowFeaturesetAddFeature(featureset, feature, fy1,fy2);
			feat->width = featureset->width;

			tr->feature.right = feat;
			feat->left = &tr->feature;

			tr = (ZMapWindowCanvasTranscript) feat;
			tr->sub_type = TRANSCRIPT_INTRON;
			tr->index = i;
		}
	}

#if USE_DOTTED_LINES
	if(feature->feature.transcript.flags.end_not_found)	/* add dotted line fading away into the distance */
	{
		double trunc_len = zMapStyleGetTruncatedIntronLength(*feature->style);
		if(!trunc_len)
			trunc_len =  featureset->end - y2;

		fy1 = y2;
		fy2 = fy1 + trunc_len;

		feat = zMapWindowFeaturesetAddFeature(featureset, feature, fy1, fy2);
		feat->width = featureset->width;

		tr->feature.right = feat;
		feat->left = &tr->feature;

		tr = (ZMapWindowCanvasTranscript) feat;
		tr->sub_type = TRANSCRIPT_INTRON_END_NOT_FOUND;
		tr->index = i;
	}
#endif

	return feat;
}




static ZMapFeatureSubPartSpan zmapWindowCanvasTranscriptGetSubPartSpan (FooCanvasItem *foo, ZMapFeature feature, double x,double y)
{
	static ZMapFeatureSubPartSpanStruct sub_part;

	/* the interface to this is via zMapWindowCanvasItemGetInterval(), so here we have to look up the feature again */
#warning revisit this when canvas items are simplified
	/* and then we find the transcript data in the feature context which has a list of exons and introms */
	/* or we could find the exons/introns in the canvas and process those */

	ZMapSpan exon,intron;
	int ni = 0, ne = 0, i;
	GArray *introns,*exons;
	ZMapTranscript tr = &feature->feature.transcript;

#if 0
	if(!y)	/* interface to legacy code they uses G_OBJECT_DATA */
	{
		ZMapWindowFeaturesetItem featureset = (ZMapWindowFeaturesetItem) foo;

		if(!featureset->point_canvas_feature)
			return NULL;

		sub_part.start = featureset->point_canvas_feature->y1;
		sub_part.end   = featureset->point_canvas_feature->y2;
		sub_part.subpart = ZMAPFEATURE_SUBPART_EXON;
		/* work out which one it really is... */
		for(sub_part.index = 1, wcf = featureset->point_canvas_feature; wcf->left; wcf = wcf->left)
		{
			if()
				sub_part.index++;
		}

		return &sub_part;
	}
#endif

	introns = tr->introns;
	exons = tr->exons;
	if(introns)
		ni = introns->len;
	if(exons)
		ne = exons->len;

	/* NOTE: is we have truncated introns then we will not return a sub part as they are not in the feature */

	for(i = 0; i < ne; i++)
	{
		exon = &g_array_index(exons,ZMapSpanStruct,i);
		if(exon->x1 <= y && exon->x2 >= y)
		{
			sub_part.index = i + 1;
			sub_part.start = exon->x1;
			sub_part.end = exon->x2;
			sub_part.subpart = ZMAPFEATURE_SUBPART_EXON;
			if(tr->flags.cds)
			{
				if(tr->cds_start <= y && tr->cds_end >= y)
				{
					/* cursor in CDS but could have UTR in this exon */
					sub_part.subpart = ZMAPFEATURE_SUBPART_EXON_CDS;

					if(sub_part.start < tr->cds_start)
						sub_part.start = tr->cds_start;
					if(sub_part.end > tr->cds_end)		/* these coordinates are inclusive according to EG */
						sub_part.end = tr->cds_end;
				}
				/* we have to handle both ends :-(   |----UTR-----|--------CDS--------|-----UTR------| */
				else if(y >= tr->cds_end)
				{
					/* cursor not in CDS but could have some in this exon */
					if(sub_part.start <= tr->cds_end)
						sub_part.start = tr->cds_end + 1;
				}
				else if(y <= tr->cds_start)
				{
					if(sub_part.end >= tr->cds_start)
						sub_part.end = tr->cds_start - 1;
				}
			}
			return &sub_part;
		}

		if(i < ni)
		{
			intron = &g_array_index(introns,ZMapSpanStruct,i);
			if(intron->x1 <= y && intron->x2 >= y)
			{
				sub_part.index = i + 1;
				sub_part.start = intron->x1;
				sub_part.end = intron->x2;
				sub_part.subpart = ZMAPFEATURE_SUBPART_INTRON;
				if(tr->flags.cds && tr->cds_start <= y && tr->cds_end >= y)
					sub_part.subpart = ZMAPFEATURE_SUBPART_INTRON_CDS;
				return &sub_part;
			}
		}
	}

	return NULL;
}


/* get sequence extent of compound alignment (for bumping) */
/* NB: canvas coords could overlap due to sub-pixel base pairs
 * which could give incorrect de-overlapping
 * that would be revealed on zoom
 */
static void zMapWindowCanvasTranscriptGetFeatureExtent(ZMapWindowCanvasFeature feature, ZMapSpan span, double *width)
{
	ZMapWindowCanvasFeature first = feature;

	*width = feature->width;

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

	span->x1 = first->y1;
	span->x2 = feature->y2;
}



void zMapWindowCanvasTranscriptInit(void)
{
	gpointer funcs[FUNC_N_FUNC] = { NULL };

	funcs[FUNC_PAINT] = zMapWindowCanvasTranscriptPaintFeature;
	funcs[FUNC_ADD]   = zMapWindowCanvasTranscriptAddFeature;
	funcs[FUNC_SUBPART] = zmapWindowCanvasTranscriptGetSubPartSpan;
	funcs[FUNC_EXTENT] = zMapWindowCanvasTranscriptGetFeatureExtent;

	zMapWindowCanvasFeatureSetSetFuncs(FEATURE_TRANSCRIPT, funcs, sizeof(zmapWindowCanvasTranscriptStruct), 0);
}

