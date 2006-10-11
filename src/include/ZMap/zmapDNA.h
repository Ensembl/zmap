/*  File: zmapDNA.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2006
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: DNA manipulation functions.
 *
 * HISTORY:
 * Last edited: Oct 11 10:32 2006 (edgrif)
 * Created: Fri Oct  6 14:26:08 2006 (edgrif)
 * CVS info:   $Id: zmapDNA.h,v 1.1 2006-10-11 11:36:16 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>


/* Holds information about any DNA matches found by the match functions.
 * 
 * start/end are absolute sequence coords (1-based), length of match can
 * be derived from start/end.
 * match which is a normal C string but can be NULL for performance reasons. */
typedef struct
{
  char *match ;
  int start ;
  int end ;
} ZMapDNAMatchStruct, *ZMapDNAMatch ;


gboolean zMapDNAValidate(char *dna) ;
gboolean zMapDNAFindMatch(char *cp, char *end, char *tp, int maxError, int maxN,
			  char **start_out, char **end_out, char **match_str) ;
GList *zMapDNAFindAllMatches(char *dna, char *query, int from, int length,
			     int max_errors, int max_Ns, gboolean return_matches) ;
