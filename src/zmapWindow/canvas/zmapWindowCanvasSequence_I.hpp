/*  File: zmapWindowCanvasSequence_I.h
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
