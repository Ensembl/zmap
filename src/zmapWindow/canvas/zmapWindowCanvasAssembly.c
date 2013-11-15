/*  File: zmapWindowCanvasAssembly.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
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
 * implements callback functions for FeaturesetItem assembly features
 * NOTE originally assemebly feature has display option to shade overlaps and show the actual path chosen
 * this was never immplemented in ZMapWindowCanvasItems and this mopdule is initially a copy of CanvasBasic
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>





#include <math.h>
#include <string.h>
#include <ZMap/zmapFeature.h>
#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasAssembly_I.h>


/* not static as we want to use this in alignments */
void zMapWindowCanvasAssemblyPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable, GdkEventExpose *expose)
{
	gulong fill,outline;
	int colours_set, fill_set, outline_set;
	double x1,x2;

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
	if(featureset->bumped)
		x1 += feature->bump_offset;

	x1 += featureset->dx;
	x2 = x1 + feature->width - 1;

	zMapCanvasFeaturesetDrawBoxMacro(featureset,x1,x2, feature->y1, feature->y2, drawable, fill_set,outline_set,fill,outline);
}



void zMapWindowCanvasAssemblyInit(void)
{
	gpointer funcs[FUNC_N_FUNC] = { NULL };

	funcs[FUNC_PAINT] = zMapWindowCanvasAssemblyPaintFeature;

	zMapWindowCanvasFeatureSetSetFuncs(FEATURE_ASSEMBLY, funcs, 0, 0);
}

