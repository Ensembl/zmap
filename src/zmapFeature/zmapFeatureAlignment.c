/*  Last edited: Jul 13 15:40 2012 (edgrif) */
/*  File: zmapFeatureAlignment.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Functions to manipulate alignment features.
 *
 *              (EG: Note that the cigar/align code is taken from the acedb
 *              (www.acedb.org) implementation written by me.)
 *
 * Exported functions: See ZMap/zmapFeature.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <errno.h>
#include <string.h>

#include <zmapFeature_P.h>


/* We get align strings in several variants and then convert them into this common format. */
typedef struct
{
  char op ;
  int length ;
} AlignStrOpStruct, *AlignStrOp ;


typedef struct
{
  ZMapFeatureAlignFormat format ;

  GArray *align ;					    /* Of AlignStrOpStruct. */
} AlignStrCanonicalStruct, *AlignStrCanonical ;


static gboolean checkForPerfectAlign(GArray *gaps, unsigned int align_error) ;

static AlignStrCanonical alignStrMakeCanonical(char *match_str, ZMapFeatureAlignFormat align_format) ;
static void alignStrDestroyCanonical(AlignStrCanonical canon) ;
static gboolean alignStrCanon2Homol(AlignStrCanonical canon, ZMapStrand ref_strand, ZMapStrand match_strand,
				    int p_start, int p_end, int c_start, int c_end,
				    GArray **local_map_out) ;
#if NOT_USED
static gboolean alignStrVerifyStr(char *match_str, ZMapFeatureAlignFormat align_format) ;
static gboolean exonerateVerifyVulgar(char *match_str) ;
static gboolean exonerateVerifyCigar(char *match_str) ;
static gboolean ensemblVerifyCigar(char *match_str) ;
static gboolean bamVerifyCigar(char *match_str) ;
#endif
static gboolean exonerateCigar2Canon(char *match_str, AlignStrCanonical canon) ;
static gboolean exonerateVulgar2Canon(char *match_str, AlignStrCanonical canon) ;
static gboolean ensemblCigar2Canon(char *match_str, AlignStrCanonical canon) ;
static gboolean bamCigar2Canon(char *match_str, AlignStrCanonical canon) ;
static int cigarGetLength(char **cigar_str) ;
#if NOT_USED
static char *nextWord(char *str) ;
static gboolean gotoLastDigit(char **cp_inout) ;
#endif

static gboolean gotoLastSpace(char **cp_inout) ;




/*
 *               External functions.
 */


ZMAP_ENUM_AS_NAME_STRING_FUNC(zMapFeatureAlignFormat2ShortText, ZMapFeatureAlignFormat, ZMAP_ALIGN_GAP_FORMAT_LIST) ;



/* Adds homology data to a feature which may be empty or may already have partial features. */
gboolean zMapFeatureAddAlignmentData(ZMapFeature feature,
				     GQuark clone_id,
				     double percent_id,
				     int query_start, int query_end,
				     ZMapHomolType homol_type,
				     int query_length,
				     ZMapStrand query_strand,
				     ZMapPhase target_phase,
				     GArray *gaps, unsigned int align_error,
				     gboolean has_local_sequence, char *sequence)
				     /* NOTE has_local mean in ACEBD, sequence is from GFF */
{
  gboolean result = TRUE ;

  zMapAssert(feature && feature->type == ZMAPSTYLE_MODE_ALIGNMENT) ;

  if (clone_id)
    {
      feature->feature.homol.flags.has_clone_id = TRUE ;
      feature->feature.homol.clone_id = clone_id ;
    }

  if (percent_id)
    feature->feature.homol.percent_id = (float) percent_id ;

  feature->feature.homol.type = homol_type ;
  feature->feature.homol.strand = query_strand ;
  feature->feature.homol.target_phase = target_phase ;

  if (query_start > query_end)
    {
      int tmp;

      tmp = query_start ;
      query_start = query_end ;
      query_end = tmp ;
    }

  feature->feature.homol.y1 = query_start ;
  feature->feature.homol.y2 = query_end ;
  feature->feature.homol.length = query_length ;
  feature->feature.homol.flags.has_sequence = has_local_sequence ;
  feature->feature.homol.sequence = sequence ;


  if (gaps)
    {
      zMapFeatureSortGaps(gaps) ;

      feature->feature.homol.align = gaps ;

      feature->feature.homol.flags.perfect = checkForPerfectAlign(feature->feature.homol.align, align_error) ;
    }

  return result ;
}



/* Returns TRUE if alignment is gapped, FALSE otherwise. NOTE that sometimes
 * we are passed data for an alignment feature which must represent a gapped
 * alignment but we are not passed the gap data. This tests all this. */
gboolean zMapFeatureAlignmentIsGapped(ZMapFeature feature)
{
  gboolean result = FALSE ;

  zMapAssert(zMapFeatureIsValidFull((ZMapFeatureAny) feature, ZMAPFEATURE_STRUCT_FEATURE)) ;

  if (feature->type == ZMAPSTYLE_MODE_ALIGNMENT)
    {
      int ref_length, match_length ;

      ref_length = (feature->x2 - feature->x1) + 1 ;

      match_length = (feature->feature.homol.y2 - feature->feature.homol.y1) + 1 ;
      if (feature->feature.homol.type != ZMAPHOMOL_N_HOMOL)
	match_length *= 3 ;

      if (ref_length != match_length)
	result = TRUE ;
    }

  return result ;
}




/* Constructs a gaps array from an alignment string such as CIGAR, VULGAR etc.
 * Returns TRUE on success with the gaps array returned in gaps_out. The array
 * should be free'd with g_array_free when finished with. */
gboolean zMapFeatureAlignmentString2Gaps(ZMapFeatureAlignFormat align_format,
					 ZMapStrand ref_strand, int ref_start, int ref_end,
					 ZMapStrand match_strand, int match_start, int match_end,
					 char *align_string, GArray **gaps_out)
{
  gboolean result = FALSE ;
  AlignStrCanonical canon = NULL ;
  GArray *gaps = NULL ;

 if (!(canon = alignStrMakeCanonical(align_string, align_format)))
    {
      result = FALSE ;
      zMapLogWarning("Cannot convert alignment string to canonical format: %s", align_string) ;
    }
  else if (!(result = (alignStrCanon2Homol(canon, ref_strand, match_strand,
					   ref_start, ref_end,
					   match_start, match_end,
					   &gaps))))
    {
      zMapLogWarning("Cannot convert alignment string to gaps array: s", align_string) ;
    }

  if (!result)
    {
      if (canon)
	alignStrDestroyCanonical(canon) ;
    }
  else
    {
      *gaps_out = gaps ;
    }

  return result ;
}







/*
 *               Internal functions.
 */


/* Returns TRUE if the target blocks match coords are within align_error bases of each other, if
 * there are less than two blocks then FALSE is returned.
 *
 * Sometimes, for reasons I don't understand, it's possible to have two abutting matches, i.e. they
 * should really be one continuous match. It may be that this happens at a clone boundary, I don't
 * try to correct this because really it's a data entry problem.
 *
 *  */
static gboolean checkForPerfectAlign(GArray *gaps, unsigned int align_error)
{
  gboolean perfect_align = FALSE ;
  int i ;
  ZMapAlignBlock align, last_align ;

  if (gaps->len > 1)
    {
      perfect_align = TRUE ;
      last_align = &g_array_index(gaps, ZMapAlignBlockStruct, 0) ;

      for (i = 1 ; i < gaps->len ; i++)
	{
	  int prev_end, curr_start ;

	  align = &g_array_index(gaps, ZMapAlignBlockStruct, i) ;


	  /* The gaps array gets sorted by target coords, this can have the effect of reversing
	     the order of the query coords if the match is to the reverse strand of the target sequence. */
	  if (align->q2 < last_align->q1)
	    {
	      prev_end = align->q2 ;
	      curr_start = last_align->q1 ;
	    }
	  else
	    {
	      prev_end = last_align->q2 ;
	      curr_start = align->q1 ;
	    }


	  /* The "- 1" is because the default align_error is zero, i.e. zero _missing_ bases,
	     which is true when sub alignment follows on directly from the next. */
	  if ((curr_start - prev_end - 1) <= align_error)
	    {
	      last_align = align ;
	    }
	  else
	    {
	      perfect_align = FALSE ;
	      break ;
	    }
	}
    }

  return perfect_align ;
}



/* Takes a match string and format and converts that string into a canonical form
 * for further processing. */
static AlignStrCanonical alignStrMakeCanonical(char *match_str, ZMapFeatureAlignFormat align_format)
{
  AlignStrCanonical canon = NULL ;
  gboolean result = FALSE ;

  canon = g_new0(AlignStrCanonicalStruct, 1) ;
  canon->format = align_format ;
  canon->align = g_array_new(FALSE, TRUE, sizeof(AlignStrOpStruct)) ;

  switch (align_format)
    {
    case ZMAPALIGN_FORMAT_CIGAR_EXONERATE:
      result = exonerateCigar2Canon(match_str, canon) ;
      break ;
    case ZMAPALIGN_FORMAT_CIGAR_ENSEMBL:
      result = ensemblCigar2Canon(match_str, canon) ;
      break ;
    case ZMAPALIGN_FORMAT_CIGAR_BAM:
      result = bamCigar2Canon(match_str, canon) ;
      break ;
    case ZMAPALIGN_FORMAT_VULGAR_EXONERATE:
      result = exonerateVulgar2Canon(match_str, canon) ;
      break ;
    default:
      zMapAssertNotReached() ;
      break ;
    }

  if (!result)
    {
      alignStrDestroyCanonical(canon) ;
      canon = NULL ;
    }

  return canon ;
}


static void alignStrDestroyCanonical(AlignStrCanonical canon)
{
  g_array_free(canon->align, TRUE) ;
  g_free(canon) ;

  return ;
}




/* Convert the canonical "string" to a zmap align array, note that we do this blindly
 * assuming that everything is ok because we have verified the string....
 *
 * Note that this routine does not support vulgar unless the vulgar string only contains
 * M, I and G (instead of D).
 *
 *  */
static gboolean alignStrCanon2Homol(AlignStrCanonical canon, ZMapStrand ref_strand, ZMapStrand match_strand,
				    int p_start, int p_end, int c_start, int c_end,
				    GArray **local_map_out)
{
  gboolean result = TRUE ;
  int curr_ref, curr_match ;
  int i, j ;
  GArray *local_map = NULL ;
  GArray *align = canon->align ;
  AlignBlockBoundaryType boundary_type = ALIGN_BLOCK_BOUNDARY_EDGE ;
  ZMapAlignBlock prev_gap = NULL ;

  /* is there a call to do this in feature code ??? or is the array passed in...check gff..... */
  local_map = g_array_sized_new(FALSE, FALSE, sizeof(ZMapAlignBlockStruct), 10) ;


  if (ref_strand == ZMAPSTRAND_FORWARD)
    curr_ref = p_start ;
  else
    curr_ref = p_end ;

  if (match_strand == ZMAPSTRAND_FORWARD)
    curr_match = c_start ;
  else
    curr_match = c_end ;


  for (i = 0, j = 0 ; i < align->len ; i++)
    {
      /* If you alter this code be sure to notice the += and -= which alter curr_ref and curr_match. */
      AlignStrOp op ;
      int curr_length ;
      ZMapAlignBlockStruct gap = {0} ;

      op = &(g_array_index(align, AlignStrOpStruct, i)) ;

      curr_length = op->length ;

      switch (op->op)
	{
        case 'N':                                           /* Intron */
	  {
            if (ref_strand == ZMAPSTRAND_FORWARD)
	      curr_ref += curr_length ;
	    else
	      curr_ref -= curr_length ;

            boundary_type = ALIGN_BLOCK_BOUNDARY_INTRON ;	    
	    break ;
	  }
	case 'D':					    /* Deletion in reference sequence. */
	case 'G':
	  {
	    if (ref_strand == ZMAPSTRAND_FORWARD)
	      curr_ref += curr_length ;
	    else
	      curr_ref -= curr_length ;

            boundary_type = ALIGN_BLOCK_BOUNDARY_DELETION ;
	    break ;
	  }
	case 'I':					    /* Insertion from match sequence. */
	  {
	    if (match_strand == ZMAPSTRAND_FORWARD)
	      curr_match += curr_length ;
	    else
	      curr_match -= curr_length ;

            boundary_type = ALIGN_BLOCK_BOUNDARY_MATCH ; /* it is shown butted up to the previous align block */
	    break ;
	  }
	case 'M':					    /* Match. */
	  {
	    gap.t_strand = ref_strand ;
	    gap.q_strand = match_strand ;
            gap.start_boundary = boundary_type ;
            gap.end_boundary = ALIGN_BLOCK_BOUNDARY_EDGE ; /* this may get updated as we parse subsequent operators */

	    if (ref_strand == ZMAPSTRAND_FORWARD)
	      {
		gap.t1 = curr_ref ;
		gap.t2 = (curr_ref += curr_length) - 1 ;
	      }
	    else
	      {
		gap.t2 = curr_ref ;
		gap.t1 = (curr_ref -= curr_length) + 1 ;
	      }

	    gap.q1 = curr_match ;
	    if (match_strand == ZMAPSTRAND_FORWARD)
	      gap.q2 = (curr_match += curr_length) - 1 ;
	    else
	      gap.q2 = (curr_match -= curr_length) + 1 ;

	    local_map = g_array_append_val(local_map, gap) ;

	    j++ ;					    /* increment for next gap element. */

            boundary_type = ALIGN_BLOCK_BOUNDARY_MATCH ;
	    break ;
	  }
	default:
	  {
	    zMapAssertNotReached() ;

	    break ;
	  }
	}

      if (prev_gap)
        prev_gap->end_boundary = boundary_type ;

      if (op->op == 'M')
        prev_gap = &g_array_index(local_map, ZMapAlignBlockStruct, local_map->len - 1) ;
      else
        prev_gap = NULL ;
    }

  *local_map_out = local_map ;

  return result ;
}



#if NOT_USED
/* Takes a match string and format and verifies that the string is valid
 * for that format. */
static gboolean alignStrVerifyStr(char *match_str, ZMapFeatureAlignFormat align_format)
{

  /* We should be using some kind of state machine here, setting a model and
   * then just chonking through it......
   *  */

  gboolean result = FALSE ;

  switch (align_format)
    {
    case ZMAPALIGN_FORMAT_CIGAR_EXONERATE:
      result = exonerateVerifyCigar(match_str) ;
      break ;
    case ZMAPALIGN_FORMAT_CIGAR_ENSEMBL:
      result = ensemblVerifyCigar(match_str) ;
      break ;
    case ZMAPALIGN_FORMAT_CIGAR_BAM:
      result = bamVerifyCigar(match_str) ;
      break ;
    case ZMAPALIGN_FORMAT_VULGAR_EXONERATE:
      result = exonerateVerifyVulgar(match_str) ;
      break ;
    default:
      zMapAssertNotReached() ;
      break ;
    }

  return result ;
}




/* Format of an exonerate vulgar string is:
 *
 * "operator number number" repeated as necessary. Operators are D, I or M.
 *
 * A regexp (without dealing with greedy matches) would be something like:
 *
 *                           (([MCGN53ISF]) ([0-9]+) ([0-9]+))*
 *
 *  */
static gboolean exonerateVerifyVulgar(char *match_str)
{
  gboolean result = TRUE ;
  typedef enum {STATE_OP, STATE_SPACE_OP,
		STATE_NUM1, STATE_SPACE_NUM1, STATE_NUM2, STATE_SPACE_NUM2} VulgarStates ;
  char *cp = match_str ;
  VulgarStates state ;

  state = STATE_OP ;
  do
    {
      switch (state)
	{
	case STATE_OP:
	  if (*cp == 'M' || *cp == 'C' || *cp == 'G'
	      || *cp == 'N' || *cp == '5' || *cp == '3'
	      || *cp == 'I' || *cp == 'S' || *cp == 'F')
	    state = STATE_SPACE_OP ;
	  else
	    result = FALSE ;
	  break ;
	case STATE_SPACE_OP:
	  if (gotoLastSpace(&cp))
	    state = STATE_NUM1 ;
	  else
	    result = FALSE ;
	  break ;
	case STATE_NUM1:
	  if (gotoLastDigit(&cp))
	    state = STATE_SPACE_NUM1 ;
	  else
	    result = FALSE ;
	  break ;
	case STATE_SPACE_NUM1:
	  if (gotoLastSpace(&cp))
	    state = STATE_NUM2 ;
	  else
	    result = FALSE ;
	  break ;
	case STATE_NUM2:
	  if (gotoLastDigit(&cp))
	    state = STATE_SPACE_NUM2 ;
	  else
	    result = FALSE ;
	  break ;
	case STATE_SPACE_NUM2:
	  if (gotoLastSpace(&cp))
	    state = STATE_OP ;
	  else
	    result = FALSE ;
	  break ;
	}

      if (result)
	cp++ ;
      else
	break ;

    } while (*cp) ;

  return result ;
}


/* Format of an exonerate cigar string is:
 *
 * "operator number" repeated as necessary. Operators are D, I or M.
 *
 * A regexp (without dealing with greedy matches) would be something like:
 *
 *                           (([DIM]) ([0-9]+))*
 *
 *  */
static gboolean exonerateVerifyCigar(char *match_str)
{
  gboolean result = TRUE ;
  typedef enum {STATE_OP, STATE_SPACE_OP, STATE_NUM, STATE_SPACE_NUM} CigarStates ;
  char *cp = match_str ;
  CigarStates state ;

  state = STATE_OP ;
  do
    {
      switch (state)
	{
	case STATE_OP:
	  if (*cp == 'M' || *cp == 'D' || *cp == 'I')
	    state = STATE_SPACE_OP ;
	  else
	    result = FALSE ;
	  break ;
	case STATE_SPACE_OP:
	  if (gotoLastSpace(&cp))
	    state = STATE_NUM ;
	  else
	    result = FALSE ;
	  break ;
	case STATE_NUM:
	  if (gotoLastDigit(&cp))
	    state = STATE_SPACE_NUM ;
	  else
	    result = FALSE ;
	  break ;
	case STATE_SPACE_NUM:
	  if (gotoLastSpace(&cp))
	    state = STATE_OP ;
	  else
	    result = FALSE ;
	  break ;
	}

      if (result)
	cp++ ;
      else
	break ;

    } while (*cp) ;

  return result ;
}



/* Format of an ensembl cigar string is:
 *
 * "[optional number]operator" repeated as necessary. Operators are D, I or M.
 *
 * A regexp (without dealing with greedy matches) would be something like:
 *
 *                           (([0-9]+)?([DIM]))*
 *
 *  */
static gboolean ensemblVerifyCigar(char *match_str)
{
  gboolean result = TRUE ;
  typedef enum {STATE_OP, STATE_NUM} CigarStates ;
  char *cp = match_str ;
  CigarStates state ;

  state = STATE_NUM ;
  do
    {
      switch (state)
	{
	case STATE_NUM:
	  if (!gotoLastDigit(&cp))
	    cp-- ;

	  state = STATE_OP ;
	  break ;
	case STATE_OP:
	  if (*cp == 'M' || *cp == 'D' || *cp == 'I')
	    state = STATE_NUM ;
	  else
	    result = FALSE ;
	  break ;
	}

      if (result)
	cp++ ;
      else
	break ;

    } while (*cp) ;

  return result ;
}


/* Format of an BAM cigar string (the ones we handle) is:
 *
 * "[optional number]operator" repeated as necessary. Operators are N, M, D, I, X, S, P.
 * We don't handle H (hard-clipping)
 *
 * A regexp (without dealing with greedy matches) would be something like:
 *
 *                           (([0-9]+)([NMDIXSP]))*
 *
 *  */
static gboolean bamVerifyCigar(char *match_str)
{
  gboolean result = TRUE ;
  typedef enum {STATE_OP, STATE_NUM} CigarStates ;
  char *cp = match_str ;
  CigarStates state ;

  state = STATE_NUM ;
  do
    {
      switch (state)
	{
	case STATE_NUM:
	  if (!gotoLastDigit(&cp))
	    cp-- ;

	  state = STATE_OP ;
	  break ;
	case STATE_OP:
	  if (*cp == 'M' || *cp == 'N' || *cp == 'D' || *cp == 'I' || *cp == 'X' || *cp == 'S' || *cp == 'P')
	    state = STATE_NUM ;
	  else
	    result = FALSE ;
	  break ;
	}

      if (result)
	cp++ ;
      else
	break ;

    } while (*cp) ;

  return result ;
}

#endif

/* Blindly converts, assumes match_str is a valid exonerate cigar string. */
static gboolean exonerateCigar2Canon(char *match_str, AlignStrCanonical canon)
{
  gboolean result = TRUE ;
  char *cp ;

  cp = match_str ;
  do
    {
      AlignStrOpStruct op = {'\0'} ;

      op.op = *cp ;
      cp++ ;
      gotoLastSpace(&cp) ;
      cp++ ;

      op.length = cigarGetLength(&cp) ;			    /* N.B. moves cp on as needed. */

      gotoLastSpace(&cp) ;
      if (*cp == ' ')
	cp ++ ;

      canon->align = g_array_append_val(canon->align, op) ;
    } while (*cp) ;

  return result ;
}


/* Not straight forward to implement, wait and see what we need.... */
static gboolean exonerateVulgar2Canon(char *match_str, AlignStrCanonical canon)
{
  gboolean result = FALSE ;


  return result ;
}


/* Blindly converts, assumes match_str is a valid ensembl cigar string. */
static gboolean ensemblCigar2Canon(char *match_str, AlignStrCanonical canon)
{
  gboolean result = TRUE ;
  char *cp ;

  cp = match_str ;
  do
    {
      AlignStrOpStruct op = {'\0'} ;

      if (g_ascii_isdigit(*cp))
	op.length = cigarGetLength(&cp) ;		    /* N.B. moves cp on as needed. */
      else
	op.length = 1 ;

      op.op = *cp ;

      canon->align = g_array_append_val(canon->align, op) ;

      cp++ ;
    } while (*cp) ;

  return result ;
}



/* Blindly converts, assumes match_str is a valid BAM cigar string.
 * Currently we convert:
 * 
 * X -> M
 * P -> ignored
 * S -> ignored
 * H -> not handled
 * 
 */
static gboolean bamCigar2Canon(char *match_str, AlignStrCanonical canon)
{
  gboolean result = TRUE ;
  char *cp ;

  cp = match_str ;
  do
    {
      AlignStrOpStruct op = {'\0'} ;

      if (g_ascii_isdigit(*cp))
	op.length = cigarGetLength(&cp) ;		    /* N.B. moves cp on as needed. */
      else
	op.length = 1 ;

      if (*cp == 'H')
        {
          /* We don't handle hard-clipping */
          result = FALSE ;
          break ;
        }
      else if (*cp == 'P' || *cp == 'S')
        {
          /* Padding and soft-clipping: should be fine to ignore these */
        }
      else 
        {
          if (*cp == 'X') /* Mismatch. Treat it like a match. */
            op.op = 'M' ;
          else            /* Everything else we handle */
            op.op = *cp ;

          canon->align = g_array_append_val(canon->align, op) ;
        }
      
      cp++ ;
    } while (*cp) ;

  return result ;
}



/* Returns -1 if string could not be converted to number. */
static int cigarGetLength(char **cigar_str)
{
  int length = -1 ;
  char *old_cp = *cigar_str, *new_cp = NULL ;

  errno = 0 ;
  length = strtol(old_cp, &new_cp, 10) ;

  if (!errno)
    {
      if (new_cp == old_cp)
	length = 1 ;

      *cigar_str = new_cp ;
    }

  return length ;
}


#if NOT_USED
/* Put in utils somewhere ????
 *
 * Whether str is on a word or on space, moves to the _next_ word.
 * Returns NULL if there is no next word or at end of string.
 *
 *  */
static char *nextWord(char *str)
{
  char *word = NULL ;

  if (str && *str)
    {
      char *cp = str ;

      while (*cp)
	{
	  if (*cp == ' ')
	    {
	      break ;
	    }

	  cp++ ;
	}

      while (*cp)
	{
	  if (*cp != ' ')
	    {
	      word = cp ;
	      break ;
	    }

	  cp++ ;
	}
    }

  return word ;
}



/* If looking at a character in a string which is a number move to the last digit of that number. */
static gboolean gotoLastDigit(char **cp_inout)
{
  gboolean result = FALSE ;
  char *cp = *cp_inout ;

  if (g_ascii_isdigit(*cp))
    {
      cp++ ;

      while (*cp && g_ascii_isdigit(*cp))
	cp++ ;

      cp-- ;					    /* position on last digit. */

      *cp_inout = cp ;
      result = TRUE ;
    }


  return result ;
}

#endif


/* If looking at a character in a string which is a space move to the last space of those spaces.  */
static gboolean gotoLastSpace(char **cp_inout)
{
  gboolean result = FALSE ;
  char *cp = *cp_inout ;

  if (*cp == ' ')
    {
      cp++ ;

      while (*cp && *cp == ' ')
	cp++ ;

      cp-- ;					    /* position on last space. */

      *cp_inout = cp ;
      result = TRUE ;
    }


  return result ;
}





