/*  File: zmapWindowCanvasTranscript_I.h
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description:
 *
 * implements callback functions for FeaturesetItem transcript features
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CANVAS_TRANSCRIPT_I_H
#define ZMAP_CANVAS_TRANSCRIPT_I_H

#include <ZMap/zmap.hpp>


#include <zmapWindowCanvasFeatureset_I.hpp>
#include <zmapWindowCanvasTranscript.hpp>

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


// Is there one of these per exon/intron or per transcript ????
// looks like one per exon/intron.....
//   
typedef struct _zmapWindowCanvasTranscriptStruct
{
  zmapWindowCanvasFeatureStruct feature;	/* all the common stuff */

  // ??? what was this used for ??   
  int index;		/* of intron or exon */

  ZMapWindowCanvasTranscriptSubType sub_type;
  /* can tell if exon has CDS/  UTR from feature->feature struct */

  // Try this here for gapped exons....   
  AlignGap gapped ;                                         /* boxes and lines to draw when bumped */


} zmapWindowCanvasTranscriptStruct, *ZMapWindowCanvasTranscript;



#endif /* !ZMAP_CANVAS_TRANSCRIPT_I_H */
