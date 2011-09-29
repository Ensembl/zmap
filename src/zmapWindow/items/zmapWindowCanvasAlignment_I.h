
/*  File: zmapWindowCanvasAlignment.c
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
 *-------------------------------------------------------------------
 * ZMap is free software; you can redistribute it and/or
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
 * implements callback functions for FeaturesetItem alignment features
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>


#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasAlignment.h>


typedef struct _zmapWindowCanvasAlignmentStruct
{
	zmapWindowCanvasFeatureStruct feature;	/* all the common stuff */
	/* NOTE that we can have: alignment.feature->feature->feature.homol */


	/* stuff for displaying homology status derived from feature data when loading */

	gboolean bump_set;	/* has homology and gaps data (lazy evaluation */

	/* we display one glyph max at each end */
	/* so far these can be 2 kinds of homology incomplete or nc-splice, which cannot overlap */

	char glyph5;
	char glyph3;

	/* we display traffic light lines from left link to right */
	/* NOTE for rev strand fwds may be descending query coords */
	/* as this is for display only we only need to know the colour */
	ColinearityType colinear_fwds_pixel;	/* defined in zmapFeature.h */


} zmapWindowCanvasAlignmentStruct, *ZMapWindowCanvasAlignment;



