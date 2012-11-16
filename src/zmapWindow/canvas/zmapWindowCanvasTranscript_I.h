
/*  File: zmapWindowCanvasTranscript_I.h
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
 * implements callback functions for FeaturesetItem transcript features
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>


#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasTranscript.h>

/*
---------------------------------------------------------------------------------
	*** handling start and end not found ***

	there is probably a lot of code that assumed we have neat & tidy exons with intron between
	for truncated transcripts (ie that overlap the clone boundaries) we just have part of the whole
	and no trailing introns.
	There is a separate status of start/end not found which doesn not imply truncatiion due to session limits
	and this is handled with exactly the same flags.

	to display these as dotted lines we insert intron CanvasFeatureset items at either end of the transcript
	but DO NOT ALTER THE TRANSCRIPT FEATURE in any way.

	the transcript is displayed as a series of exons and introns linked sideways and we just insert extra introns at the ends,
	but note that these do not correspond to data in the transcript feature's GArrays.

	These are indedxed as -1 or n+1

	the overall extent of the transcript is expanded to match

	If we have diff styles for transcript_trunc and start-not-found-but-not-truncated (we do) then we display dotted lines
	in the different colours as specified and uses wil be able to tell the difference.
---------------------------------------------------------------------------------
*/

typedef enum
{
	TRANSCRIPT_INVALID,
	TRANSCRIPT_EXON,
	TRANSCRIPT_INTRON,
	TRANSCRIPT_INTRON_START_NOT_FOUND,		/* canvas featureset items not in the feature */
	TRANSCRIPT_INTRON_END_NOT_FOUND

} ZMapWindowCanvasTranscriptSubType;


typedef struct _zmapWindowCanvasTranscriptStruct
{
	zmapWindowCanvasFeatureStruct feature;	/* all the common stuff */

	int index;		/* of intron or exon */

	ZMapWindowCanvasTranscriptSubType sub_type;
	/* can tell if exon has CDS/  UTR from feature->feature struct */

} zmapWindowCanvasTranscriptStruct, *ZMapWindowCanvasTranscript;


