/*  File: zmapWindowCanvasAlignment.c
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
#ifndef ZMAP_CANVAS_ALIGNMENT_I_H
#define ZMAP_CANVAS_ALIGNMENT_I_H

#include <ZMap/zmap.h>


#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasBasic.h>
#include <zmapWindowCanvasGlyph_I.h>
#include <zmapWindowCanvasAlignment.h>


typedef struct _AlignGapStruct
{
	int y1,y2;		/* in pixel coords from feature y1 */
	int type;
#define GAP_BOX	1
#define GAP_HLINE	2
#define GAP_VLINE	3
#define GAP_VLINE_INTRON 4
	gboolean edge;	/* for squashed short reads: edge blocks are diff colour */

	struct _AlignGapStruct *next;

} AlignGapStruct, *AlignGap;

#define N_ALIGN_GAP_ALLOC	1000



typedef struct _zmapWindowCanvasAlignmentStruct
{
	zmapWindowCanvasFeatureStruct feature;	/* all the common stuff */
	/* NOTE that we can have: alignment.feature->feature->feature.homol */

	AlignGap gapped;		/* boxes and lines to draw when bumped */

	/* stuff for displaying homology status derived from feature data when loading */

	/* we display one glyph max at each end
	 * so far these can be 2 kinds of homology incomplete or nc-splice, which cannot overlap
	 * however these are defined as sub styles and we can have more types in diff featuresets
	 * so we cache these in global hash table indexed by a glyph signature
	 * zero means don't display a glyph
	 */

	ZMapWindowCanvasGlyph glyph5;
	ZMapWindowCanvasGlyph glyph3;

	gboolean bump_set;	/* has homology and gaps data (lazy evaluation) */


} zmapWindowCanvasAlignmentStruct, *ZMapWindowCanvasAlignment;




#endif /* !ZMAP_CANVAS_ALIGNMENT_I_H */


