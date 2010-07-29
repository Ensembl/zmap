/*  File: zmapSequence.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: In the end I want to have a general set of sequence
 *              functions that will apply equally to dna and peptide.
 *              These functions will call functions in zmapDNA.c or
 *              zmapPeptide.c to do the actual work.
 *
 * HISTORY:
 * Last edited: Jul  7 10:21 2010 (edgrif)
 * Created: Thu Sep 27 10:37:20 2007 (edgrif)
 * CVS info:   $Id: zmapSequence.h,v 1.3 2010-07-29 09:13:10 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SEQMATCH_H
#define ZMAP_SEQMATCH_H

#include <ZMap/zmapFeature.h>


/*
 * Holds information about any sequence matches found by the match functions.
 * 
 * start/end are absolute sequence coords (1-based), length of match can
 * be derived from start/end.
 * 
 * match is a normal C string but can be NULL if the actual match is not required. */
typedef struct
{
  ZMapSequenceType match_type ;
  ZMapStrand strand ;
  ZMapFrame frame ;
  int start ;
  int end ;
  int screen_start ;
  int screen_end ;
  char *match ;
} ZMapDNAMatchStruct, *ZMapDNAMatch ;



ZMapFrame zMapSequenceGetFrame(int position) ;
void zMapSequencePep2DNA(int *start_inout, int *end_inout, ZMapFrame frame) ;
void zMapSequenceDNA2Pep(int *start_inout, int *end_inout, ZMapFrame frame) ;



#endif /* ZMAP_SEQMATCH_H */
