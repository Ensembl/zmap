/*  File: zmapWindowCanvasBasic.c
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
 * implements callback functions for FeaturesetItem basic features
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>





#include <math.h>
#include <string.h>
#include <ZMap/zmapFeature.h>
#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasBasic_I.h>


/* not static as we want to use this in alignments */
void zMapWindowCanvasBasicPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable)
{
	gulong fill,outline;
	GdkColor c;
	FooCanvasItem *item = (FooCanvasItem *) featureset;
      int cx1, cy1, cx2, cy2;
	int colours_set, fill_set, outline_set;

	double x1,x2;

	/* draw a box */

	/* colours are not defined for the CanvasFeatureSet
	 * as we can have several styles in a column
	 * but they are cached by the calling function
	 */

	x1 = featureset->width / 2 - feature->width / 2;
	if(featureset->bumped)
		x1 += feature->bump_offset;

	x1 += featureset->dx;
	x2 = x1 + feature->width;

		/* get item canvas coords, following example from FOO_CANVAS_RE (used by graph items) */
		/* NOTE CanvasFeature coords are the extent including decorations so we get coords from the feature */
	foo_canvas_w2c (item->canvas, x1, feature->feature->x1 - featureset->start + featureset->dy, &cx1, &cy1);
	foo_canvas_w2c (item->canvas, x2, feature->feature->x2 - featureset->start + featureset->dy + 1, &cx2, &cy2);
      						/* + 1 to draw to the end of the last base */

		/* we have pre-calculated pixel colours */
	colours_set = zMapWindowCanvasFeaturesetGetColours(featureset, feature, &fill, &outline);
	fill_set = colours_set & WINDOW_FOCUS_CACHE_FILL;
	outline_set = colours_set & WINDOW_FOCUS_CACHE_FILL;

		/* NOTE that the gdk_draw_rectangle interface is a bit esoteric
		 * and it doesn't like rectangles that have no depth
		 */

	if(fill_set && (!outline_set || (cy2 - cy1 > 1)))	/* fill will be visible */
	{
		c.pixel = fill;
		gdk_gc_set_foreground (featureset->gc, &c);
		gdk_draw_rectangle (drawable, featureset->gc,TRUE, cx1, cy1, cx2 - cx1 + 1, cy2 - cy1 + 1);
	}

	if(outline_set)
	{
		c.pixel = outline;
		gdk_gc_set_foreground (featureset->gc, &c);
		if(cy2 == cy1)
			gdk_draw_line (drawable, featureset->gc, cx1, cy1, cx2, cy2);
		else
			gdk_draw_rectangle (drawable, featureset->gc,FALSE, cx1, cy1, cx2 - cx1, cy2 - cy1);
	}
}



void zMapWindowCanvasBasicInit(void)
{
	gpointer funcs[FUNC_N_FUNC] = { NULL };

	funcs[FUNC_PAINT] = zMapWindowCanvasBasicPaintFeature;

	zMapWindowCanvasFeatureSetSetFuncs(FEATURE_BASIC, funcs, 0);
}

