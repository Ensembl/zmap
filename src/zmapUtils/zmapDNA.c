/*  File: zmapDNA.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Oct 11 10:31 2006 (edgrif)
 * Created: Fri Oct  6 11:41:38 2006 (edgrif)
 * CVS info:   $Id: zmapDNA.c,v 1.2 2006-11-08 09:24:42 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapDNA.h>


/* Checks that dna is a valid string (a,c,t,g,n). */
gboolean zMapDNAValidate(char *dna)
{
  gboolean valid = FALSE ;

  if (dna && *dna)
    {
      char *base ;

      base = dna ;
      valid = TRUE ;
      while (*base)
	{
	  if (*base != 'a' && *base != 'c' && *base != 'g' && *base != 't' && *base != 'n')
	    {
	      valid = FALSE ;
	      break ;
	    }
	  else
	    base++ ;
	}
    }

  return valid ;
}




/* This code is modified from acedb code (www.acedb.org)
 * 
 * Given target and query DNA sequences find the query sequence in 
 * the target.
 * 
 * Returns TRUE if query sequence found and records position of first significant match
 * in position_out, FALSE otherwise.
 * 
 * In addition if match_str is not NULL a pointer to an allocated string which is the match
 * is returned. N.B. this could potentially use up a lot of memory !
 * 
 *  */
gboolean zMapDNAFindMatch(char *cp, char *end, char *tp, int maxError, int maxN,
			  char **start_out, char **end_out, char **match_str)
{
  gboolean result = FALSE ;
  char *c=cp, *t=tp, *cs=cp ;
  int  i = maxError, j = maxN ;
  char *start ;

  zMapAssert(cp && *cp && tp && *tp) ;

  start = cp ;
  while (c <= end)
    {
      if (!*t)
	{
	  result = TRUE ;
	  break ;
	}
      else if (*c == 'n' && --j < 0)
	{
	  t = tp ;
	  c = ++cs ;
	  i = maxError ;
	  j = maxN ;

	  start = c ;					    /* reset start.... */
	}
      else if (!(*t++ == *c++) && (--i < 0))
	{
	  t = tp ;
	  c = ++cs ;
	  i = maxError ;
	  j = maxN ;

	  start = c ;					    /* reset start. */
	}
    }

  if (result || !*t)
    {
      result = TRUE ;
      *start_out = start ;
      *end_out = c - 1 ;				    /* We've gone past the last match so
							       reset. */
      if (match_str)
	*match_str = g_strndup(*start_out, *end_out - *start_out + 1) ;
    }

  return result ;
}


GList *zMapDNAFindAllMatches(char *dna, char *query, int from, int length,
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
  n = strlen(dna) ;
  if (from < 0)
    from = 0 ;
  if (length < 0)
    length = 0 ;
  if (n > from + length)
    n = from + length ;
  if (from > n)
    from = n ;

  search_start = dna + from ;
  search_end = search_start + length - 1 ;
  start = end = cp = search_start ;
  while (zMapDNAFindMatch(cp, search_end, query, max_errors, max_Ns, &start, &end, match_ptr))
    { 
      ZMapDNAMatch match ;

      /* Record this match. */
      match = g_new0(ZMapDNAMatchStruct, 1) ;
      match->start = start - dna ;
      match->end = end - dna ;
      if (return_matches)
	match->match = *match_ptr ;

      sites = g_list_append(sites, match) ;

      /* Move pointers on. */
      cp = end + 1 ;
    }

  return sites ;
}

