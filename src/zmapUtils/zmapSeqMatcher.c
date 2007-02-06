/*  File: zmapSeqMatcher.c
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
 * Description: Contains code to find DNA or peptide sequence matches.
 *              The code seems to have enough commonality that both
 *              peptide and DNA code should be in this one file.
 *
 * Exported functions: See ZMap/zmapSeqMatcher.h
 * HISTORY:
 * Last edited: Feb  6 10:13 2007 (edgrif)
 * Created: Tue Feb  6 09:26:51 2007 (edgrif)
 * CVS info:   $Id: zmapSeqMatcher.c,v 1.1 2007-02-06 10:21:58 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapSeqMatcher.h>
#include <ZMap/zmapDNA.h>


/*! @addtogroup zmaputils
 * @{
 *  */


GList *zMapSeqMatchFindAll(ZMapSeqMatchType match_type, char *target, char *query, int from, int length,
			   int max_errors, int max_Ns, gboolean return_matches)
{
  GList *sites = NULL ;
  int  n ;
  char *cp ;
  char *start, *end ;
  char *search_start, *search_end, *match ;
  char **match_ptr = NULL ;

  /* Return the actual match string ? */
  if (return_matches)
    match_ptr = &match ;

  /* rationalise coords.... */
  n = strlen(target) ;
  if (from < 0)
    from = 0 ;
  if (length < 0)
    length = 0 ;
  if (n > from + length)
    n = from + length ;
  if (from > n)
    from = n ;

  search_start = target + from ;
  search_end = search_start + length - 1 ;
  start = end = cp = search_start ;
  while (zMapDNAFindMatch(cp, search_end, query, max_errors, max_Ns, &start, &end, match_ptr))
    { 
      ZMapSeqMatch match ;

      /* Record this match. */
      match = g_new0(ZMapSeqMatchStruct, 1) ;
      match->start = start - target ;
      match->end = end - target ;
      if (return_matches)
	match->match = *match_ptr ;

      sites = g_list_append(sites, match) ;

      /* Move pointers on. */
      cp = end + 1 ;
    }

  return sites ;
}



/*! @} end of zmaputils docs. */





/* 
 *                     Internal routines. 
 */
