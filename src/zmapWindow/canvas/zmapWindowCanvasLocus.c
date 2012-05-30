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
 * locus features are single line (word?) thing (the feature name with maybe a line for decoration
 * unlike old style ZMWCI i called these locus not text as they are not generic text features
 * but instead a specific sequence region with a name
 *
 * of old there was talk of creating diff features externally for loci (i hear)
 * but here we simply have created a feature form the name and coords of the original, if it has a locus id
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
	gulong fill,outline;
	int colours_set, fill_set, outline_set;
	double x1,x2;

	FooCanvasItem *foo = (FooCanvasItem *) featureset;
	ZMapWindowCanvasLocus locus = (ZMapWindowCanvasLocus) feature;
	ZMapWindowCanvasLocusSet lset = (ZMapWindowCanvasLocusSet) featureset->opt;
	char *text;
	int len, width;
     	int cx1, cy1, cx2, cy2;

	zmapWindowCanvasLocusGetPango(drawable, featureset, lset);

	x1 = featureset->dx;
	x2 = x1 + locus->x_off;

	text = (char *) g_quark_to_string(feature->feature->original_id);
	len = strlen(text);
	pango_layout_set_text (lset->pango.layout, text, len);

		/* need to get pixel coordinates for pango */
	foo_canvas_w2c (foo->canvas, x1, locus->y1 + featureset->dy, &cx1, &cy1);
	foo_canvas_w2c (foo->canvas, x2, locus->y2 + featureset->dy, &cx2, &cy2);

	zMap_draw_line(drawable, featureset, cx1, cy1, cx2, cy2);

	cy2 -= lset->pango.text_height / 2;		/* centre text on line */
	pango_renderer_draw_layout (lset->pango.renderer, lset->pango.layout,  cx2 * PANGO_SCALE , cy2 * PANGO_SCALE);
}




/* de-overlap and hide features if necessary,
 * NOTE that we never change the zoom,
 * but we'll always get called once for display (NOTE before creating the index)
 * need to be sure we get called if features are added or deleted
 * or if the selection of which to display changes
 */
static void zMapWindowCanvasLocusZoomSet(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable)
{
	ZMapSkipList sl;
	GList *l;
	double len, width, f_width = featureset->width;
	char *text;

	ZMapWindowCanvasLocusSet lset = (ZMapWindowCanvasLocusSet) featureset->opt;

	zmapWindowCanvasLocusGetPango(drawable, featureset, lset);

	featureset->width = 0.0;

//	for(sl = zMapSkipListFirst(featureset->display_index); sl; sl = sl->next)
	for(l = featureset->features; l ; l = l->next)
	{
//		ZMapWindowCanvasLocus locus = (ZMapWindowCanvasLocus) sl->data;
		ZMapWindowCanvasLocus locus = (ZMapWindowCanvasLocus) l->data;

		locus->y1 = locus->feature.feature->x1;
		locus->y2 = locus->feature.feature->x1;
		locus->x_off = ZMAP_LOCUS_LINE_WIDTH;

		/* expand the column if needed */
		text = (char *) g_quark_to_string(locus->feature.feature->original_id);
		len = strlen(text);

		width = locus->x_off + len * lset->pango.text_width;
		if(width > featureset->width)
			featureset->width = width;
	}

	if(f_width != featureset->width)
		zMapWindowCanvasFeaturesetRequestReposition((FooCanvasItem *) featureset);

}



static void zMapWindowCanvasLocusFreeSet(ZMapWindowFeaturesetItem featureset)
{
	ZMapWindowCanvasLocusSet lset = (ZMapWindowCanvasLocusSet) featureset->opt;

	if(lset)
		zmapWindowCanvasFeaturesetFreePango(&lset->pango);
}




void zMapWindowCanvasLocusInit(void)
{
	gpointer funcs[FUNC_N_FUNC] = { NULL };

	funcs[FUNC_PAINT] = zMapWindowCanvasLocusPaintFeature;
	funcs[FUNC_ZOOM]   = zMapWindowCanvasLocusZoomSet;
	funcs[FUNC_FREE]   = zMapWindowCanvasLocusFreeSet;

	zMapWindowCanvasFeatureSetSetFuncs(FEATURE_LOCUS, funcs, sizeof(zmapWindowCanvasLocusStruct), sizeof(zmapWindowCanvasLocusSetStruct));
}

