/*  File: zmapDNA.c
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See ZMap/zmapDNA.h
 * HISTORY:
 * Last edited: Dec  7 16:39 2010 (edgrif)
 * Created: Fri Oct  6 11:41:38 2006 (edgrif)
 * CVS info:   $Id: zmapDNA.c,v 1.16 2010-12-07 16:40:01 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapSequence.h>
#include <ZMap/zmapDNA.h>


/* PLEASE READ THIS.....
 *
 * THIS FILE NEEDS EXPANDING TO HOLD CODE TO HANDLE DNA SEQUENCES, IT SHOULD IMPLEMENT
 * SOME COMMON FUNCTIONS WITH PEPTIDE.C IN THAT THEY SHOULD BOTH HAVE COMMON ELEMENTS IN THEIR
 * STRUCTS FOR NAME/LENGTH ETC SO THAT WE END UP WITH A "GENERAL" SEQUENCE OBJECT
 * THAT COULD BE CALLED FROM THE FASTA CODE ETC.....
 *
 *
 *  */



// mh17: moved DNA encoding here from zmapPeptide.c, seems more logical really

 /* I have added a '-' to ASCII position 45 for '-' which is used to pad incomplete
 * sequences. If you don't have this then on encountering a '-' the code will
 * insert a NULL char which terminates the sequence as C string !
 *
 * I've also added '-' to dnaDecodeChar[] which is populated in the function
 * that uses it....why it does that is an acedb mystery... */


char dnaEncodeChar[0x80] =
{
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '-', 0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,

  0,  A_,  B_,  C_,  D_,   0,   0,  G_,  H_,   0,   0,  K_,   0,  M_,  N_,   0,
  0,   0,  R_,  S_,  T_,  U_,  V_,  W_,  X_,  Y_,   0,   0,   0,   0,   0,   0,
  0,  A_,  B_,  C_,  D_,   0,   0,  G_,  H_,   0,   0,  K_,   0,  M_,  N_,   0,
  0,   0,  R_,  S_,  T_,  U_,  V_,  W_,  X_,  Y_,   0,   0,   0,   0,   0,   0,
} ;

/* 1<<4 = 16, not big enough for the 45th element in dnaDecodeString() */
char dnaDecodeChar[1<<6] = { 0 };

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Copied from acedb code.....We'll need this some time I guess..... */
static void dnaDecodeString(char *cp)
{
  dnaDecodeChar[A_] = 'a';
  dnaDecodeChar[T_] = 't';
  dnaDecodeChar[G_] = 'g';
  dnaDecodeChar[C_] = 'c';

  dnaDecodeChar[R_] = 'r';
  dnaDecodeChar[Y_] = 'y';
  dnaDecodeChar[M_] = 'm';
  dnaDecodeChar[K_] = 'k';
  dnaDecodeChar[S_] = 's';
  dnaDecodeChar[W_] = 'w';

  dnaDecodeChar[H_] = 'h';
  dnaDecodeChar[B_] = 'b';
  dnaDecodeChar[V_] = 'v';
  dnaDecodeChar[D_] = 'd';

  dnaDecodeChar[N_] = 'n';
//  dnaDecodeChar[X_] = 'x';  // X_ is defined as N_

  dnaDecodeChar[45] = '-';                          /* Needed for padded sequences. */

  --cp;
  while(*++cp)
    {
      *cp = dnaDecodeChar[((int)*cp)];
    }

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* This is not ideal, the acedb code works on encoded strings so we have to convert the
 * dna to this format before translating it. */
void dnaEncodeString(char *cp)
{
  --cp ;
  while(*++cp)
    {
      *cp = dnaEncodeChar[((int)*cp) & 0x7f] ;
    }

  return ;
}





/* Takes a dna string and lower cases it inplace. */
gboolean zMapDNACanonical(char *dna)
{
  gboolean result = TRUE ;				    /* Nothing to fail currently. */
  char *base ;

  zMapAssert(dna && *dna) ;

  base = dna ;
  while (*base)
    {
      *base = g_ascii_tolower(*base) ;
      base++ ;
    }

  return result ;
}


/* Checks that dna is a valid string (a,c,t,g,n), NOTE no upper case...... */
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
	  if (*base != 'a' && *base != 'c' && *base != 'g' && *base != 't' && *base != 'u'
	      && *base != 'm' && *base != 'r' && *base != 'w' && *base != 's' && *base != 'y'
	      && *base != 'k' && *base != 'v' && *base != 'h' && *base != 'd' && *base != 'b'
	      && *base != 'n' && *base != 'x' && *base != '-' && *base != '*' && *base != '.')
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
 * tp is the query (with degenerate codes) and cp is the DNA to search (with atgcnx)
 */
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
      else if ((*c == 'n' || *c == 'x') && --j < 0)
	{
	  t = tp ;
	  c = ++cs ;
	  i = maxError ;
	  j = maxN ;

	  start = c ;					    /* reset start.... */
	}
      else
        {
          gboolean match  = FALSE;

          char base = dnaEncodeChar[(int) *c++ & 0x7f];

            /* does this cry out for an automaton or what?
             * this runs inefficiently as a naive algorithm but as it's one match
             *  against one shortish DNA sequence it completes in a person feasable time
             */
          match = (*t++ & base);

          if (!match && (--i < 0))
            {
              t = tp ;
              c = ++cs ;
              i = maxError ;
              j = maxN ;

              start = c ;                                 /* reset start. */
            }
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




/* Looks for dna matches on either or both strand (if strand == ZMAPSTRAND_NONE it does both). */
GList *zMapDNAFindAllMatches(char *dna, char *query, ZMapStrand strand, int from, int length,
			     int max_errors, int max_Ns, gboolean return_matches)
{
  GList *sites = NULL ;
  int  n ;
  char *cp ;
  char *start, *end ;
  char *search_start, *search_end, *match ;
  char **match_ptr = NULL ;
  char * tx_query;

  tx_query = g_strdup(query);
  dnaEncodeString(tx_query);

  /* Return the actual match string ? */
  if (return_matches)
    match_ptr = &match ;

  /* rationalise coords, they are always given in terms of the forward strand.... */
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

  if (strand == ZMAPSTRAND_NONE || strand == ZMAPSTRAND_FORWARD)
    {
      /* cp < search_end for when the last match is @ the end of target seq...
       * e.g.
       *  Query:               ATG
       * Target: ATGGCGGATTAGCAATG
       */
      while (cp < search_end && zMapDNAFindMatch(cp, search_end, tx_query, max_errors, max_Ns, &start, &end, match_ptr))
	{
	  ZMapDNAMatch match ;

	  /* Record this match. */
	  match = g_new0(ZMapDNAMatchStruct, 1) ;
	  match->match_type = ZMAPSEQUENCE_DNA ;
	  match->strand = ZMAPSTRAND_FORWARD ;
	  match->start = start - dna ;
	  match->end = end - dna ;
	  match->frame = zMapSequenceGetFrame(match->start + 1) ;

	  if (return_matches)
	    match->match = *match_ptr ;

	  sites = g_list_append(sites, match) ;

	  /* Move pointers on. */
	  cp = end + 1 ;
	}
    }

  if (strand == ZMAPSTRAND_NONE || strand == ZMAPSTRAND_REVERSE)
    {
      char *revcomp_dna ;
      int offset ;

      offset = search_start - dna ;

      /* Make a revcomp'd copy of the forward strand section. */
      revcomp_dna = g_memdup(search_start, length) ;
      zMapDNAReverseComplement(revcomp_dna, length) ;

      /* Set the pointers and search. */
      search_start = revcomp_dna ;
      search_end = search_start + length - 1 ;
      start = end = cp = search_start ;

      /* cp < search_end for when the last match is @ the end of target seq...
       * e.g.
       *  Query:               ATG
       * Target: ATGGCGGATTAGCAATG
       */

      while (cp < search_end && zMapDNAFindMatch(cp, search_end, tx_query, max_errors, max_Ns, &start, &end, match_ptr))
	{
	  ZMapDNAMatch match ;
	  int tmp ;

	  /* Record this match. */
	  match = g_new0(ZMapDNAMatchStruct, 1) ;
	  match->match_type = ZMAPSEQUENCE_DNA ;
	  match->strand = ZMAPSTRAND_REVERSE ;
	  match->start = (length - (start - revcomp_dna)) + offset - 1 ;
	  match->end = (length - (end - revcomp_dna)) + offset - 1 ;
	  tmp = match->start ;
	  match->start = match->end ;
	  match->end = tmp ;
	  match->frame = zMapSequenceGetFrame(match->start + 1) ;
	  if (return_matches)
	    match->match = *match_ptr ;

	  sites = g_list_append(sites, match) ;

	  /* Move pointers on. */
	  cp = end + 1 ;
	}

      g_free(revcomp_dna) ;
    }

  g_free(tx_query);

  return sites ;
}





/* Reverse complement the DNA. Probably this is about as good as one can do....
 *
 * It works by reading in towards the middle and at each position, reading
 * the base, complementing it and then putting it back at the mirror position,
 * i.e. the whole thing is done in place. (if there is a middle base it is done
 * twice)
 *  */
void zMapDNAReverseComplement(char *sequence_in, int length_in)
{
  unsigned char *sequence = (unsigned char *)sequence_in ;  /* Stop compiler moaning. */
  static  unsigned char rev[256] = {'\0'} ;
  unsigned char *s_ptr, *e_ptr ;
  size_t length ;
  int i ;

  /* could be done at compile time for max efficiency but not portable (EBCDIC ??). */
  /* IUPAC */
  rev['a'] = 't' ;
  rev['c'] = 'g' ;
  rev['g'] = 'c' ;
  rev['t'] = 'a' ;

  rev['m'] = 'k' ;
  rev['r'] = 'y' ;
  rev['w'] = 'w' ;
  rev['s'] = 's' ;
  rev['y'] = 'r' ;
  rev['k'] = 'm' ;
  rev['v'] = 'b' ;
  rev['h'] = 'd' ;
  rev['d'] = 'h' ;
  rev['b'] = 'v' ;

  rev['n'] = 'n' ;
  rev['x'] = 'x' ;      // NCBI

  /* Other common symbols. */
  rev['-'] = '-' ;
  rev['.'] = '.' ;
  rev['*'] = '*' ;


  length = (size_t)length_in ;

  s_ptr = sequence ;
  e_ptr = sequence + (length - 1) ;
  for (i = 0 ;
       i < (length + 1) / 2 ;
       s_ptr++, e_ptr--, i++)
    {
      char s, e ;

      s = rev[*s_ptr] ;
      e = rev[*e_ptr] ;

      *s_ptr = e ;
      *e_ptr = s ;
    }

  return ;
}




