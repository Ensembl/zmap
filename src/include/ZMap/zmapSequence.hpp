/*  File: zmapSequence.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: In the end I want to have a general set of sequence
 *              functions that will apply equally to dna and peptide.
 *              These functions will call functions in zmapDNA.c or
 *              zmapPeptide.c to do the actual work.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SEQMATCH_H
#define ZMAP_SEQMATCH_H

#include <ZMap/zmapFeature.hpp>



/* This all needs overhauling....it makes life complicated....we should have a union as
 * some fields are not needed for dna..... */

/*
 * Holds information about any sequence matches found by the match functions.
 * 
 * start/end are absolute sequence coords (1-based), length of match can
 * be derived from start/end.
 * 
 * match is a normal C string but can be NULL if the actual match is not required. */
typedef struct
{
  ZMapSequenceType match_type ;				    /* Match sequence is dna or peptide. */

  ZMapStrand strand ;
  ZMapFrame frame ;

  int ref_start ;
  int ref_end ;


  int start ;
  int end ;


  /* ???????????????????????????????? */
  int screen_start ;
  int screen_end ;

  char *match ;
} ZMapDNAMatchStruct, *ZMapDNAMatch ;



ZMapFrame zMapSequenceGetFrame(int position) ;
void zMapSequencePep2DNA(int *start_inout, int *end_inout, ZMapFrame frame) ;
void zMapSequenceDNA2Pep(int *start_inout, int *end_inout, ZMapFrame frame) ;



#endif /* ZMAP_SEQMATCH_H */
