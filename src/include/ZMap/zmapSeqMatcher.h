/*  File: zmapSeqMatcher.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * Description: Functions to find DNA and peptide matches.
 *
 * HISTORY:
 * Last edited: Feb  6 09:50 2007 (edgrif)
 * Created: Tue Feb  6 09:30:29 2007 (edgrif)
 * CVS info:   $Id: zmapSeqMatcher.h,v 1.1 2007-02-06 10:21:23 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SEQMATCH_H
#define ZMAP_SEQMATCH_H

#include <glib.h>


/*! @addtogroup zmaputils
 * @{
 *  */


/*!
 * The type of match, so far only dna to dna and peptide to dna are supported. */
typedef enum {ZMAPSEQMATCH_INVALID, ZMAPSEQMATCH_DNA_TO_DNA, ZMAPSEQMATCH_PEP_2_DNA} ZMapSeqMatchType ;


/*!
 * Holds information about any sequence matches found by the match functions.
 * 
 * start/end are absolute sequence coords (1-based), length of match can
 * be derived from start/end.
 * 
 * match is a normal C string but can be NULL if the actual match is not required. */
typedef struct
{
  char *match ;
  int start ;
  int end ;
  int screen_start ;
  int screen_end ;
} ZMapSeqMatchStruct, *ZMapSeqMatch ;


/*! @} end of zmaputils docs. */



GList *zMapSeqMatchFindAll(ZMapSeqMatchType match_type, char *target, char *query, int from, int length,
			   int max_errors, int max_Ns, gboolean return_matches) ;



#endif /* ZMAP_SEQMATCH_H */
