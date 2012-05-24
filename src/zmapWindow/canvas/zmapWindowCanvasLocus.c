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



void zMapWindowCanvasLocusPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable, GdkEventExpose *expose)
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
	x2 = x1 + feature->width;

}




/* de-overlap and hide features if necessary, NOTE that we never change the zoom, but we'll always get called once for display
 * need to be sure we get called if features are added or deleted
 * or if the selection of which to display changes
 */
static void zMapWindowCanvasLocusZoomSet(ZMapWindowFeaturesetItem featureset)
{
	ZMapSkipList sl;

	for(sl = zMapSkipListFirst(featureset->display_index); sl; sl = sl->next)
	{
		ZMapWindowCanvasLocus locus = (ZMapWindowCanvasLocus) sl->data;
	}
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

	zMapWindowCanvasFeatureSetSetFuncs(FEATURE_BASIC, funcs, sizeof(zmapWindowCanvasLocusStruct), sizeof(zmapWindowCanvasLocusSetStruct));
}

