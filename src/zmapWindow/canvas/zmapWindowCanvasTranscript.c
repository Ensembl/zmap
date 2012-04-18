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



static void zMapWindowCanvasTranscriptPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable, GdkEventExpose *expose)
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
	 */

	colours_set = zMapWindowCanvasFeaturesetGetColours(featureset, feature, &fill, &outline);
	fill_set = colours_set & WINDOW_FOCUS_CACHE_FILL;
	outline_set = colours_set & WINDOW_FOCUS_CACHE_OUTLINE;

	x1 = featureset->width / 2 - feature->width / 2;
	feature->feature_offset = x1;

	if(featureset->bumped)
		x1 += feature->bump_offset;

	x1 += featureset->dx;
	x2 = x1 + feature->width;
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
		zMapCanvasFeaturesetDrawBoxMacro(featureset, x1, x2, y1, y2, drawable, fill_set,outline_set,fill,outline);
	}
	else if (outline_set && tr->sub_type == TRANSCRIPT_INTRON)
	{
		GdkColor c;
      	int cx1, cy1, cx2, cy2, cy1_5, cx1_5;
			/* get item canvas coords in pixel coordinates */
		foo_canvas_w2c (foo->canvas, x1, feature->y1 - featureset->start + featureset->dy, &cx1, &cy1);
		foo_canvas_w2c (foo->canvas, x2, feature->y2 - featureset->start + featureset->dy, &cx2, &cy2);
		cy1_5 = (cy1 + cy2) / 2;
		cx1_5 = (cx1 + cx2) / 2;
		c.pixel = outline;
		gdk_gc_set_foreground (featureset->gc, &c);
		zMap_draw_line(drawable, featureset, cx1_5, cy1, cx2, cy1_5);
		zMap_draw_line(drawable, featureset, cx2, cy1_5, cx1_5, cy2);
	}
}


#if 0
	    /* cds will default to normal colours if its own colours are not set. */
	    zMapStyleGetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
				&xon_fill, &xon_draw, &xon_border);

	if ((sub_feature->subpart == ZMAPFEATURE_SUBPART_EXON_CDS) &&
	    !(zMapStyleGetColours(style, STYLE_PROP_TRANSCRIPT_CDS_COLOURS, colour_type,
				  &xon_fill, &xon_draw, &xon_border)))
#endif


static void zMapWindowCanvasTranscriptAddFeature(ZMapWindowFeaturesetItem featureset, ZMapFeature feature, double y1, double y2)
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

			feat = zMapWindowFeaturesetAddFeature(featureset, feature, fy1, fy2);
			feat->width = featureset->width;

			tr->feature.right = feat;
			feat->left = &tr->feature;

			tr = (ZMapWindowCanvasTranscript) feat;
			tr->sub_type = TRANSCRIPT_INTRON;
			tr->index = i;
		}
	}
}



#if 0
zmapWindowFeature.c
	handleButton() ->
      sub_item = zMapWindowCanvasItemGetInterval(canvas_item, but_event->x, but_event->y, &sub_feature);

	(eventually)

  	if(ZMAP_IS_WINDOW_FEATURESET_ITEM(matching_interval))
  	{
  		/* returns a static dara structure */
  		*sub_feature_out =
  			zMapWindowCanvasFeaturesetGetSubPartSpan(matching_interval, zMapWindowCanvasItemGetFeature(item) ,x,y);
  	}
  	else
  		*sub_feature_out = g_object_get_data(G_OBJECT(matching_interval), ITEM_SUBFEATURE_DATA);

#endif



void zMapWindowCanvasTranscriptInit(void)
{
	gpointer funcs[FUNC_N_FUNC] = { NULL };

	funcs[FUNC_PAINT] = zMapWindowCanvasTranscriptPaintFeature;
	funcs[FUNC_ADD]   = zMapWindowCanvasTranscriptAddFeature;

	zMapWindowCanvasFeatureSetSetFuncs(FEATURE_TRANSCRIPT, funcs, sizeof(zmapWindowCanvasTranscriptStruct));
}

