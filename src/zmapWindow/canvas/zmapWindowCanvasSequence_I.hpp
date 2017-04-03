/*  File: zmapWindowCanvasSequence_I.h
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
 * implements callback functions for FeaturesetItem sequence features
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CANVAS_SEQUENCE_I_H
#define ZMAP_CANVAS_SEQUENCE_I_H

#include <ZMap/zmap.hpp>

#include <zmapWindowCanvasFeatureset_I.hpp>
#include <zmapWindowCanvasSequence.hpp>



#define MAX_SEQ_TEXT	32	/* it's nominally 20 i think */



typedef enum
  {
    SEQUENCE_INVALID,
    SEQUENCE_DNA,
    SEQUENCE_PEPTIDE

  } ZMapWindowCanvasSequenceType;


typedef struct
{
  long start,end;
  gulong colour;
  ZMapFeatureSubPartType type;

} zmapSequenceHighlightStruct, *ZMapSequenceHighlight;


typedef struct zmapWindowCanvasSequenceStructType
{
  zmapWindowCanvasFeatureStruct feature;	/* all the common stuff */

  /* this->feature->feature.sequence has the useful info, see zmapFeature.h/ZMapSequenceStruct_ */

  gulong background;
  GList *highlight;					    /* of ZMapSequenceHighlight */

  char *text;						    /* a buffer for one line */
  int n_text;

  /* first coord, normally equals featureset start but for show translation in zmap not so */
  long start;
  long end;

  long row_size;					    /* no of bases/ residues between rows */
  long row_disp;					    /* no to display in each row */
  long n_bases;						    /* actual bases excluding ... */
  long spacing;						    /* between rows */
  long offset;						    /* to centre rows in spacing */
  const char *truncated;                                    /* show ... if we run out of space */
  int factor;						    /* for dna or peptide */

  gboolean background_set;


} zmapWindowCanvasSequenceStruct, *ZMapWindowCanvasSequence;



#endif /* !ZMAP_CANVAS_SEQUENCE_I_H */
