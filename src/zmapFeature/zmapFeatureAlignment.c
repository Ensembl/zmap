/*  File: zmapFeatureAlignment.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
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
#include <ZMap/zmapGFFStringUtils.h>

#include <errno.h>
#include <string.h>
#include <ctype.h>

#include <zmapFeature_P.h>

/*
 * We get align strings in several variants and then convert them
 * into this common format.
 *
 * Operator-length pair. Character to represent the operator and a
 * positive integer for the length.
 */
typedef struct _AlignStrOpStruct
{
  char op ;
  int length ;
} AlignStrOpStruct, *AlignStrOp ;

/*
 * Note that the possible values of ZMapFeatureAlignFormat are:
 *
 * ZMAPALIGN_FORMAT_INVALID
 * ZMAPALIGN_FORMAT_CIGAR_EXONERATE
 * ZMAPALIGN_FORMAT_CIGAR_ENSEMBL
 * ZMAPALIGN_FORMAT_CIGAR_BAM
 * ZMAPALIGN_FORMAT_GAP_GFF3
 * ZMAPALIGN_FORMAT_VULGAR_EXONERATE
 * ZMAPALIGN_FORMAT_GAPS_ACEDB
 *
 */

/*
 * An object of this type represents the whole of the alignment data
 * for the whole feature. We have a format (one of the enum type given
 * above), an array of AlignStOpStruct objects and an integer value
 * for the number of operators.
 */
typedef struct AlignStrCanonicalStruct_
{
  ZMapFeatureAlignFormat format ;

  GArray *align ;    /* Of AlignStrOpStruct. */
  int num_operators ;
} AlignStrCanonicalStruct, *AlignStrCanonical ;

/*
 * Represents the various options for text representation of
 * alignment data.
 *
 * bDigitsLeft             digits are on left of operators otherwise on right
 * bMayOmit1               length 1 operators may omit the number
 * bSpaces                 one space required between operator-number pairs,
 *                         otherwise no spaces are allowed
 */
typedef struct AlignStrCanonicalPropertiesStruct_
{
  unsigned int bDigitsLeft : 1 ;
  unsigned int bMayOmit1   : 1 ;
  unsigned int bSpaces     : 1 ;
} AlignStrCanonicalPropertiesStruct, *AlignStrCanonicalProperties ;


static gboolean checkForPerfectAlign(GArray *gaps, unsigned int align_error) ;

static AlignStrCanonical alignStrCanonicalCreate(ZMapFeatureAlignFormat align_format) ;
static void alignStrCanonicalDestroy(AlignStrCanonical *canon) ;
static AlignStrOp alignStrCanonicalGetOperator(AlignStrCanonical canon, int i) ;
static char alignStrCanonicalGetOperatorChar(AlignStrCanonical canon, int i) ;
static gboolean alignStrCanonicalAddOperator(AlignStrCanonical canon, const AlignStrOpStruct * const op) ;
static gboolean alignStrCanonicalRemoveOperator(AlignStrCanonical canon, int i) ;
static AlignStrCanonical alignStrMakeCanonical(char *match_str, ZMapFeatureAlignFormat align_format) ;

static GArray * alignStrOpArrayCreate() ;
static void alignStrOpArrayDestroy(GArray * const align_array) ;

static gboolean alignStrCanon2Homol(AlignStrCanonical canon,
                                    ZMapStrand ref_strand, ZMapStrand match_strand,
                                    int p_start, int p_end, int c_start, int c_end,
                                    GArray **local_map_out) ;
static gboolean alignFormat2Properties(ZMapFeatureAlignFormat align_format,
                                       AlignStrCanonicalProperties properties) ;
static AlignStrCanonical homol2AlignStrCanonical(ZMapFeature feature, ZMapFeatureAlignFormat align_format) ;
static gboolean alignStrCanonical2string(char ** p_string,
                                         AlignStrCanonicalProperties properties,
                                         AlignStrCanonical canon) ;
static gboolean parse_cigar_general(const char * const str,
                                    AlignStrCanonical canon,
                                    AlignStrCanonicalProperties properties,
                                    GError **error) ;
static gboolean parse_get_op_data(int i,
                                  char * const sBuff,
                                  size_t buffer_size,
                                  const char ** pArray,
                                  int nOp,
                                  const char * const target,
                                  gboolean bLeft) ;
static char * parse_is_valid_number(char *sBuff, size_t iLen, int opt) ;
static int parse_is_valid_op_data(char * sBuff,
                                  gboolean bMayOmit1,
                                  gboolean bLeft,
                                  gboolean bSpaces,
                                  gboolean bEnd,
                                  gboolean bFirst) ;
static gboolean parse_canon_valid(AlignStrCanonical canon, GError **error) ;

/*
 * (sm23) I've put in the new code for this within defines in order that
 * it can be reverted easily if anyone notices a problem.
 */
#define USE_NEW_CIGAR_PARSING_CODE 1

#if NOT_USED
static gboolean alignStrVerifyStr(char *match_str, ZMapFeatureAlignFormat align_format) ;
static gboolean exonerateVerifyVulgar(char *match_str) ;
static gboolean exonerateVerifyCigar(char *match_str) ;
static gboolean ensemblVerifyCigar(char *match_str) ;
static gboolean bamVerifyCigar(char *match_str) ;
#endif

static gboolean exonerateCigar2Canon(char *match_str, AlignStrCanonical canon) ;
static gboolean gffv3Gap2Canon(char *match_str, AlignStrCanonical canon) ;
static gboolean exonerateVulgar2Canon(char *match_str, AlignStrCanonical canon) ;
static gboolean ensemblCigar2Canon(char *match_str, AlignStrCanonical canon) ;
static gboolean bamCigar2Canon(char *match_str, AlignStrCanonical canon) ;

#if NOT_USED
static int cigarGetLength(char **cigar_str) ;
#endif

static gboolean alignStrCanonicalSubstituteBamCigar(AlignStrCanonical canon) ;
static gboolean alignStrCanonicalSubstituteEnsemblCigar(AlignStrCanonical canon) ;
static gboolean alignStrCanonicalSubstituteExonerateCigar(AlignStrCanonical canon) ;
static gboolean alignStrCanonicalSubstituteGFFv3Gap(AlignStrCanonical canon) ;

#if NOT_USED
static gboolean alignStrCanonicalSubstituteExonerateVulgar(AlignStrCanonical canon) ;
static char *nextWord(char *str) ;
static gboolean gotoLastDigit(char **cp_inout) ;
static gboolean gotoLastSpace(char **cp_inout) ;
#endif

/*
 * These arrays are the allowed operators for various of the format types.
 *
 * Notes:
 *
 * (a) According to www.sequenceontology.org gffv3 Gap attribute is the same as
 *     exonerate cigar. We use the same code for both here.
 * (b) We do not handle frameshift operators.
 */
static const char *operators_cigar_bam       = "MIDN" ;
static const char *operators_cigar_exonerate = "MID" ;
static const char *operators_gffv3_gap       = "MID" ;
static const char *operators_cigar_ensembl   = "MID" ;

/*
 * Allowed operators for internal "canonical" representation.
 */
#if NOT_USED
static const char *operators_canonical = "MINDG";
#endif


/*
 *               External functions.
 */


ZMAP_ENUM_TO_SHORT_TEXT_FUNC(zMapFeatureAlignFormat2ShortText, ZMapFeatureAlignFormat, ZMAP_ALIGN_GAP_FORMAT_LIST) ;



/* Adds homology data to a feature which may be empty or may already have partial features. */
gboolean zMapFeatureAddAlignmentData(ZMapFeature feature,
     GQuark clone_id,
     double percent_id,
     int query_start, int query_end,
     ZMapHomolType homol_type,
     int query_length,
     ZMapStrand query_strand,
     ZMapPhase target_phase,
     GArray *align, unsigned int align_error,
     gboolean has_local_sequence, char *sequence)
     /* NOTE has_local mean in ACEBD, sequence is from GFF */
{
  gboolean result = FALSE ;

  zMapReturnValIfFail(feature && (feature->mode == ZMAPSTYLE_MODE_ALIGNMENT), result);

  result = TRUE ;

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


  if (align)
    {
      zMapFeatureSortGaps(align) ;

      feature->feature.homol.align = align ;

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

  if (!zMapFeatureIsValidFull((ZMapFeatureAny) feature, ZMAPFEATURE_STRUCT_FEATURE))
    return result ;

  if (feature->mode == ZMAPSTYLE_MODE_ALIGNMENT)
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


/*
 * This function takes a feature as argument, and converts it's homol data into
 * a string of the specified format. This is done in two stages:
 *
 * (1) First convert the feature data into the canonical form above (AlignStrCanonical), then
 * (2) Convert the canonical form into the appropriate string for output.
 */
gboolean zMapFeatureAlignmentGetAlignmentString(ZMapFeature feature,
                                                ZMapFeatureAlignFormat align_format,
                                                char **p_string_out)
{
  gboolean result = FALSE ;
  char *temp_string = NULL ;
  AlignStrCanonical canon = NULL ;
  AlignStrCanonicalPropertiesStruct properties ;

  /*
   * Test feature type and other errors
   */
  zMapReturnValIfFail(feature &&
                      (feature->mode == ZMAPSTYLE_MODE_ALIGNMENT) &&
                      p_string_out, result) ;

  /*
   * Homol to canonical
   */
  result = (canon = homol2AlignStrCanonical(feature, align_format)) ? TRUE : FALSE ;

  /*
   * Get string properties for this alignment type.
   */
  if (alignFormat2Properties(canon->format, &properties))
    {

      /*
       * This function loops over operators and converts to string representation
       */
      if ((result = alignStrCanonical2string(&temp_string, &properties, canon)))
        {
          if (temp_string)
            {
              *p_string_out = temp_string ;
              result = TRUE ;
            }
        }
      else
        {
          if (temp_string)
            g_free(temp_string) ;
        }
    }

  if (canon)
    {
      alignStrCanonicalDestroy(&canon) ;
    }

  return result ;
}




/*
 * Constructs a gaps array from an alignment string such as CIGAR, VULGAR etc.
 * Returns TRUE on success with the gaps array returned in gaps_out. The array
 * should be free'd with g_array_free when finished with.
 */
gboolean zMapFeatureAlignmentString2Gaps(ZMapFeatureAlignFormat align_format,
                                         ZMapStrand ref_strand, int ref_start, int ref_end,
                                         ZMapStrand match_strand, int match_start, int match_end,
                                         char *align_string, GArray **align_out)
{
  gboolean result = FALSE ;
  AlignStrCanonical canon = NULL ;
  GArray *align = NULL ;

  if (!(canon = alignStrMakeCanonical(align_string, align_format)))
    {
      result = FALSE ;
      zMapLogWarning("Cannot convert alignment string to canonical format: %s", align_string) ;
    }
  else if (!(result = (alignStrCanon2Homol(canon, ref_strand, match_strand,
                                           ref_start, ref_end,
                                           match_start, match_end,
                                           &align))))
    {
      zMapLogWarning("Cannot convert canonical format to align array: s", align_string) ;
    }

  /*
   * The canonical object should be destroyed even if
   * the above conversions succeed.
   */
  if (canon)
    alignStrCanonicalDestroy(&canon) ;

  if (result)
    {
      *align_out = align ;
    }
  else
    {
      *align_out = NULL ;
    }

  return result ;
}




/*
 *            Package routines.
 */

/* Do any of boundaries match the start/end of the alignment or any of the
 * gapped blocks (if there are any) within the alignment.Return a list of ZMapSpanStruct with those
 * that do.
 *
 * Note in the code below that we can avoid unnecessary comparisons because
 * both boundaries and the exons in the transcript are sorted into ascending
 * sequence position.
 * Note also that a zero value boundary means "no boundary".
 */
GList *zmapFeatureAlignmentHasMatchingBoundaries(ZMapFeature feature, GList *boundaries)
{
  GList *matching_boundaries = NULL ;
  int slop ;

  zMapReturnValIfFail(zMapFeatureIsValid((ZMapFeatureAny)feature), FALSE) ;

  slop = zMapStyleSpliceHighlightTolerance(*(feature->style)) ;

  if (!(feature->feature.homol.align))
    {
      matching_boundaries = zmapFeatureCoordsListMatch(feature, slop, boundaries) ;
    }
  else
    {
      int slop ;
      GArray *gaps_array = feature->feature.homol.align ;
      GList *curr ;
      int i ;

      slop = zMapStyleSpliceHighlightTolerance(*(feature->style)) ;

      curr = boundaries ;
      i = 0 ;
      while (curr)
        {
          ZMapSpan curr_boundary = (ZMapSpan)(curr->data) ;

          if ((curr_boundary->x1) && feature->x2 < curr_boundary->x1)
            {
              /* Feature is before current splice. */
              ;
            }
          else if ((curr_boundary->x2) && feature->x1 > curr_boundary->x2)
            {
              /* curr splice is beyond feature so stop compares. */
              break ;
            }
          else
            {
              /* Compare blocks. */
              while (i < gaps_array->len)
                {
                  ZMapAlignBlock block ;

                  block = &(g_array_index(gaps_array, ZMapAlignBlockStruct, i)) ;

                  if ((curr_boundary->x1) && block->t2 < curr_boundary->x1)
                    {
                      /* Block is before current splice. */
                      ;
                    }
                  else if ((curr_boundary->x2) && block->t1 > curr_boundary->x2)
                    {
                      /* curr splice is beyond feature so stop compares. */
                      break ;
                    }
                  else
                    {
                      int match_boundary_start = 0, match_boundary_end = 0 ;

                      if (zmapFeatureCoordsMatch(slop, curr_boundary->x1, curr_boundary->x2,
                                                 block->t1, block->t2,
                                                 &match_boundary_start, &match_boundary_end))
                        {
                          ZMapSpan match_boundary ;

                          match_boundary = g_new0(ZMapSpanStruct, 1) ;

                          if (match_boundary_start)
                            match_boundary->x1 = match_boundary_start ;
                          if (match_boundary_end)
                            match_boundary->x2 = match_boundary_end ;

                          matching_boundaries = g_list_append(matching_boundaries, match_boundary) ;
                        }
                    }

                  i++ ;
                }
            }

          curr = g_list_next(curr) ;
        }
    }

  return matching_boundaries ;
}





/*
 *            Internal routines.
 */


/*
 * Convert a format to the appropriate properties flags.
 */
static gboolean alignFormat2Properties(ZMapFeatureAlignFormat align_format,
                                       AlignStrCanonicalProperties properties)
{
  gboolean result = FALSE ;

  zMapReturnValIfFail( (align_format != ZMAPALIGN_FORMAT_INVALID) &&
                       properties,
                                       result ) ;
  properties->bDigitsLeft = FALSE ;
  properties->bMayOmit1   = FALSE ;
  properties->bSpaces     = FALSE ;

  switch (align_format)
    {
      case ZMAPALIGN_FORMAT_GAP_GFF3:
      case ZMAPALIGN_FORMAT_CIGAR_EXONERATE:
        properties->bDigitsLeft = FALSE ;
        properties->bMayOmit1   = FALSE ;
        properties->bSpaces     = TRUE ;
        result = TRUE ;
        break ;
      case ZMAPALIGN_FORMAT_CIGAR_ENSEMBL:
        properties->bDigitsLeft = TRUE ;
        properties->bMayOmit1   = TRUE ;
        properties->bSpaces     = FALSE ;
        result = TRUE ;
        break ;
      case ZMAPALIGN_FORMAT_CIGAR_BAM:
        properties->bDigitsLeft = TRUE ;
        properties->bMayOmit1   = FALSE ;
        properties->bSpaces     = FALSE ;
        result = TRUE ;
        break ;
      case ZMAPALIGN_FORMAT_VULGAR_EXONERATE:
        break ;
      default:
        break ;
    }

  return result ;
}


/*
 * Return the group of characters associated with a given operator.
 * Arguments are:
 *
 * i            index of operator desired [0, nOp-1] as usual
 * sBuff        buffer to copy them into
 * pArray       positions of the operators in the target string
 * nOp          number of operators
 * target       original target string
 * bLeft        if TRUE we look to the left of operator i, otherwise right
 *              ie
 *
 * Returns TRUE if the operation succeeded.
 *
 * Note: We may return an empty buffer.
 */
static gboolean parse_get_op_data(int i,
                                  char * const sBuff,
                                  size_t buffer_size,
                                  const char ** pArray,
                                  int nOp,
                                  const char * const target,
                                  gboolean bLeft)
{
  gboolean result = FALSE ;
  const char *pStart = NULL,
    *pEnd = NULL ;
  int j = 0 ;

  if ((i>=0) && (i<nOp))
    {

      if (bLeft)
        {
          if (i == 0)
            pStart = target ;
          else
            pStart = pArray[i-1]+1;
         pEnd = pArray[i];
        }
      else
        {
          pStart = pArray[i]+1;

          if (i < nOp-1)
            pEnd = pArray[i+1];
          else
            pEnd = target + strlen(target) ;
        }

      /* copy data */
      while ((pStart<pEnd) && (j<buffer_size))
        {
          sBuff[j] = *pStart ;
          ++pStart ;
          ++j ;
        }
      sBuff[j] = '\0' ;
      result = TRUE ;


    }

  return result ;
}

/*
 * Accepts a string and determines if it is a valid number from a cigar string.
 * It must have length at least unity, and there are three options:
 * opt == 0       all characters must be digits
 * opt == 1       single space at start, all other characters must be digits
 * opt == 2       single space at end, all other characters must be digits
 * return value is pointer to the start of the digits
 */
static char * parse_is_valid_number(char *sBuff, size_t iLen, int opt)
{
  static const char cSpace = ' ' ;
  char * num_pos = NULL, *result = NULL ;
  gboolean bDigits = TRUE ;
  int nDigits = 0 ;

  if (iLen)
    {
      if (opt == 0)
        {
          /* all characters must be digits */
          num_pos = sBuff ;
          while (*num_pos && (bDigits = isdigit(*num_pos)))
            {
              ++nDigits ;
              ++num_pos ;
            }
          if (bDigits && nDigits==iLen)
            result = sBuff ;
        }
      else if (opt == 1)
        {
          /* single space at start, all other characters must be digits */
          if (cSpace == *sBuff)
            {
              num_pos = sBuff+1;
              while (*num_pos && (bDigits = isdigit(*num_pos)))
                {
                  ++nDigits ;
                  ++num_pos ;
                }
              if (bDigits && nDigits==iLen-1)
                result = sBuff+1 ;
            }
        }
      else if (opt == 2)
        {
          /* single space at end, all other characters must be digits */
          if (cSpace == *(sBuff+iLen-1))
            {
              num_pos = sBuff ;
              while (*num_pos && cSpace!=*num_pos && (bDigits = isdigit(*num_pos)))
                {
                  ++nDigits ;
                  ++num_pos ;
                }
              if (bDigits && nDigits==iLen-1)
                result = sBuff ;
            }
        }
    }

  return result ;
}

/*
 * Inpects the buffer to determine if it contains a valid piece of numerical data
 * for the size of an operator.
 *
 * Arguments are:
 * sBuff                             buffer (may be empty)
 * bMayOmit1                         TRUE if ommitting "1" from operator size is OK
 * bLeft                             TRUE if the number was to the left of the operator
 * bSpaces                           TRUE if the op/number pairs are space seperated
 *
 * First perform some tests to see if the data item was valid according to criteria
 * described below, then parse the numerical value and return it.
 *
 */
static int parse_is_valid_op_data(char * sBuff,
                                  gboolean bMayOmit1,
                                  gboolean bLeft,
                                  gboolean bSpaces,
                                  gboolean bEnd,
                                  gboolean bFirst)
{
  static const char cSpace = ' ' ;
  char *num_pos = NULL ;
  int length = 0, num_opt = 0 ;
  size_t iLen = strlen(sBuff) ;

  /* Tests:
   *
   * (1) If iLen of the string is zero, and omit1 is true,
   *     then length = 1
   * (2) If iLen is 1 and we only have a space, and bSpace is true
   *     and bOmit1 is true then we have length = 1,
   * (3) If iLen is 1 and we have a digit and bSpace is false
         then parse for length
   * (4) If iLen is > 1 and bSpaces is true then we must have
   *     one space at the start or end according to bLeft
   * (5) If iLen is > 1 and bSpaces is false, then there must be
   *     no spaces,
   *  then parse for number
   */

  if (iLen == 0)
    {
      if (!bSpaces)
        {
          if (bMayOmit1)
            length = 1 ;
        }
      else
        {
          if (bMayOmit1 && bEnd)
            {
              if (bLeft && bFirst)
                length = 1 ;
              else if (!bLeft && !bFirst)
                length = 1 ;
            }
        }
    }
  else if (iLen == 1)
    {
      if (sBuff[0] == cSpace)
        {
          if (bSpaces && bMayOmit1)
            length = 1 ;
        }
      else if (isdigit(sBuff[0]))
        {
          if (!bSpaces)
            length = atoi(sBuff) ;
          else
            {
              if (bEnd)
                {
                  if ((bLeft && bFirst) || (!bLeft && !bFirst))
                    length = atoi(sBuff) ;
                }
            }
        }
    }
  else
    {
      if (bSpaces)
        {
          if (bLeft)
            {
              /* there must be one space at the start and all other characters must be digits */
              if (bEnd && bFirst)
                num_opt = 0 ;
              else
                num_opt = 1 ;
              num_pos = parse_is_valid_number(sBuff, iLen, num_opt) ;
            }
          else
            {
              /* there must be one space at the end and all other characters must be digits */
              if (bEnd && !bFirst)
                num_opt = 0 ;
              else
                num_opt = 2 ;
              num_pos = parse_is_valid_number(sBuff, iLen, num_opt) ;
            }
        }
      else
        {
          /* all characters must be digits */
          num_opt = 0 ;
          num_pos = parse_is_valid_number(sBuff, iLen, num_opt) ;
        }
      if (num_pos)
        length = atoi(num_pos) ;
    }

  return length ;
}


/*
 * Allocate the operator parsing array
 */
char const ** parse_allocate_operator_buffer(size_t operator_buffer_size)
{
  char const ** pArray = NULL ;
  pArray = (char const **) g_new0(char*, operator_buffer_size) ;
  if (pArray)
    memset(pArray, '\0', operator_buffer_size) ;
  return pArray ;
}

/*
 * Free the operator parsing array
 */
void parse_free_operator_array(char const ***p_array)
{
  if (*p_array)
    g_free(*p_array) ;
}

/*
 * This is to resize the operator buffer, by the factor given.
 */
void parse_resize_operator_buffer(char const ***p_array, size_t *p_buffer_size)
{
  static const size_t factor = 2 ;
  size_t i = 0 ;
  char const ** p_new = NULL, ** p_old = NULL ;
  size_t current_size = 0, new_size = 0 ;

  zMapReturnIfFail(p_array && *p_array && p_buffer_size && *p_buffer_size) ;
  current_size = *p_buffer_size ;
  p_old = *p_array ;

  /*
   * Create new array and initialise to zeros
   */
  new_size = factor * current_size ;
  p_new = (char const **) g_new0(char*, new_size) ;
  memset(p_new, '\0', new_size) ;

  /*
   * Copy old array into new one
   */
  for (i=0; i<current_size; ++i)
    {
      p_new[i] = p_old[i] ;
    }

  /*
   * Free old one
   */
  g_free(p_old) ;

  /*
   * Copy new data back
   */
  *p_array = p_new ;
  *p_buffer_size = new_size ;

  return ;
}


/*
 * This is a test function that I'm using to look at formats - at time of writing,
 * the exonerate cigar parser is incorrect.
 *
 * Arguments are:
 * char *str                    string to operate upon
 * gboolean bDigitsLeft         if TRUE, digits must be before operators
 *                              if FALSE, digits must be after operators
 * bMayOmit1                    is is OK to omit a number, will be assumed to be 1, otherwise
 *                              numbers must always be included
 * bSpaces                      operator/number pairs are seperated by one space, otherwise
 *                              may not be seperated by anything
 */
#define NUM_OPERATOR_START 10
#define NUM_DIGITS_LIMIT 32
static gboolean parse_cigar_general(const char * const str,
                                    AlignStrCanonical canon,
                                    AlignStrCanonicalProperties properties,
                                    GError **error)
{
  gboolean result = FALSE,
    bEnd = FALSE,
    bFirst = FALSE ;
  size_t iLength = 0,
    num_buffer_size = NUM_DIGITS_LIMIT,
    operator_buffer_size = NUM_OPERATOR_START ;
  const char *sTarget = NULL ;
  int i = 0, iOperators = 0 ;
  char * sBuff = NULL ;
  char const ** pArray = NULL ;
  char cStart = '\0',
    cEnd = '\0' ;
  gboolean bDigitsLeft = FALSE,
    bMayOmit1 = FALSE,
    bSpaces = FALSE ;
  AlignStrOp pAlignStrOp = NULL ;

  zMapReturnValIfFail(str && *str &&
                      canon && canon->align && (canon->num_operators == 0)
                      && error && properties, result) ;

  bDigitsLeft   = properties->bDigitsLeft ;
  bMayOmit1     = properties->bMayOmit1 ;
  bSpaces       = properties->bSpaces ;
  sTarget = str ;
  cStart = *str ;
  iLength = strlen(str) ;
  cEnd = str[iLength-1] ;
  result = TRUE ;

  /*
   * We do not allow anything other than operator character or
   * digit at the start or end, regardless of other options.
   */
  if (!isalnum(cStart) && result)
    {
      *error = g_error_new(g_quark_from_string(ZMAP_CIGAR_PARSE_ERROR), 0,
                           "target string = '%s', has non-operator or non-digit character at start", str) ;
      result = FALSE ;
    }
  if (!isalnum(cEnd) && result)
    {
      *error = g_error_new(g_quark_from_string(ZMAP_CIGAR_PARSE_ERROR), 0,
                           "target string = '%s', has non-operator and non-digit character at end", str) ;
      result = FALSE ;
    }

  /*
   * If bDigitsLeft, there can be no digits at the end of the string,
   * and if !bDigitsLeft there can be no digits at the start of the string.
   */
  if (bDigitsLeft && result)
    {
      if (!isalpha(cEnd))
        {
          *error = g_error_new(g_quark_from_string(ZMAP_CIGAR_PARSE_ERROR), 0,
                               "target string = '%s', has at non-operator character at end", str) ;
          result = FALSE ;
        }
    }
  else if (!bDigitsLeft && result)
    {
      if (!isalpha(cStart))
        {
          *error = g_error_new(g_quark_from_string(ZMAP_CIGAR_PARSE_ERROR), 0,
                               "target string = '%s', has non-operator character at start", str) ;
          result = FALSE ;
        }
    }

  /*
   * Firstly find the operators and their positions.
   */
  if (result)
    {

      sBuff = (char*) g_new0(char, num_buffer_size) ;
      pArray = parse_allocate_operator_buffer(operator_buffer_size) ;

      /*
       * Operators are defined by one character only each. This step finds their
       * positions in the input string.
       */
      while (*sTarget)
        {
          if (isalpha(*sTarget) )
            {
              pArray[iOperators] = sTarget ;
              iOperators++ ;

              if (iOperators == operator_buffer_size)
                parse_resize_operator_buffer(&pArray, &operator_buffer_size) ;
            }
          ++sTarget ;
        }

    }


  /*
   * Determine type of operators and add to the canon object.
   */
  if (result)
    {

      /*
       * Loop over the operators and determine their type.
       */
      for (i=0; i<iOperators; ++i)
        {
          AlignStrOpStruct oAlignStrOp ;
          oAlignStrOp.op = *pArray[i] ;
          oAlignStrOp.length = 0 ;

          if (!alignStrCanonicalAddOperator(canon, &oAlignStrOp) )
            {
              result = FALSE ;
            }
        }
    }


  /*
   * Extract data from string and parse.
   */
  if (result)
    {

      /*
       * Now inspect various bits of the string to extract digits associated with the operators.
       */
      for (i=0; i<iOperators; ++i)
        {
          bEnd = FALSE ;
          if ((i==0) || (i==(iOperators-1)))
            bEnd = TRUE ;
          bFirst = !(gboolean)i;

          /*
           * This should be put into a function wrapping the AlignStrCanonical
           */
          pAlignStrOp = alignStrCanonicalGetOperator(canon, i) ;

          /*
           * Get data between operators and parse for number
           */
          sBuff[0] = '\0' ;
          parse_get_op_data(i, sBuff, num_buffer_size, pArray, iOperators, str, bDigitsLeft ) ;
          pAlignStrOp->length =
            parse_is_valid_op_data(sBuff, bMayOmit1, bDigitsLeft, bSpaces, bEnd, bFirst) ;

        }

      /*
       * Now run over all operators and check that
       * (1) We have at least one operator, and
       * (2) All operators are alphabetic characters and op->length != 0.
       */
      result = iOperators ? TRUE : FALSE ;
      for (i=0; i<iOperators; ++i)
        {
          pAlignStrOp = alignStrCanonicalGetOperator(canon, i) ;
          if (!isalpha(pAlignStrOp->op) || !pAlignStrOp->length)
            {
              *error = g_error_new(g_quark_from_string(ZMAP_CIGAR_PARSE_ERROR), 0,
                        "target string = '%s', op_index = %i", str, i) ;
              result = FALSE ;
              break ;
            }
        }

    } /* if (result) */

  if (sBuff)
    g_free(sBuff) ;
  if (pArray)
    g_free(pArray) ;

  return result ;
}
#undef NUM_OPERATOR_LIMIT
#undef NUM_DIGITS_LIMIT


/*
 * This function runs over the canonical form and determines if the operators are present
 * in the acceptable operators list also passed in. If any are found that are not, then
 * they are removed. If any are removed, TRUE is returned, otherwise FALSE. The number
 * removed is also output.
 */
static gboolean parse_remove_invalid_operators(AlignStrCanonical canon,
                                               const char * const operators,
                                               int * const p_num_removed)
{
  gboolean result = FALSE ;
  int i = 0, num_removed = 0 ;
  AlignStrOp op = NULL ;

  zMapReturnValIfFail(canon &&
                      canon->align &&
                      (canon->format!=ZMAPALIGN_FORMAT_INVALID) &&
                      strlen(operators),          result) ;

  for (i=0; i<canon->num_operators; )
    {
      op = alignStrCanonicalGetOperator(canon, i) ;
      if (!strchr(operators, op->op))
        {
          alignStrCanonicalRemoveOperator(canon, i) ;
          ++num_removed ;
          result = TRUE ;
        }
      else
        {
          ++i ;
        }
    }

  *p_num_removed = num_removed ;

  return result ;
}

/*
 * Perform any other tests that are not covered by any other
 * functions involved in cigar parsing.
 *
 * Currently these are:
 *
 * (a) Check that we have at least one operator.
 * (b) We must always have an odd number of operators.
 * (c) Check that the first and last operators are M for all
 *     cases of ZMapFeatureAlignFormat.
 *
 */
static gboolean parse_canon_valid(AlignStrCanonical canon, GError **error)
{
  gboolean result = FALSE ;
  char cOp = '\0' ;
  static const char cM = 'M' ;
  zMapReturnValIfFail(canon &&
                      (canon->format!=ZMAPALIGN_FORMAT_INVALID), result ) ;
  result = TRUE ;

  /*
   * Check that we have at least one operator.
   */
  if (canon->num_operators <= 0)
    {
      result = FALSE ;
    }

  /*
   * Check that we have an odd number of operators.
   */
  if (result)
    {
      if ((canon->num_operators % 2) == 0)
        {
          /* gb10: We get a lot of cigar strings from ensembl that return even numbers of
           * operators, either because there are two M operators next to each other (sounds like
           * an error but is something we can understand) or there is a D and an I next to each
           * other (sounds like it could be valid but ZMap doesn't handle these very well).
           * I've made it still succeed here but set an error message. */
          /*result = FALSE ;*/

          g_set_error(error, g_quark_from_string(ZMAP_CIGAR_PARSE_ERROR), 0,
                      "Alignment string has an even number of operators") ;
        }
    }

  /*
   * Check first and last operators are 'M'
   */
  if (result)
    {
      if (canon->num_operators == 0)
        {
          result = FALSE ;
        }
      else if (canon->num_operators >= 1)
        {
          cOp = alignStrCanonicalGetOperatorChar(canon, 0) ;
          if (cOp != cM )
            result = FALSE ;
          if (result && canon->num_operators > 1)
            {
              cOp = alignStrCanonicalGetOperatorChar(canon, canon->num_operators-1) ;
              if (cOp != cM)
                result = FALSE ;
            }
        }
    }

  return result ;
}

/*
 * Test of parsing various kinds of cigar strings...
 */
#define TEST_STRING_MAX 1024
static void parseTestFunction01()
{
  char *sTest01 = NULL ;
  int num_removed = 0 ;
  gboolean bResult = FALSE ;
  GError *error = NULL ;
  AlignStrCanonicalPropertiesStruct properties ;
  AlignStrCanonical canon = NULL ;
  ZMapFeatureAlignFormat align_format = ZMAPALIGN_FORMAT_CIGAR_BAM ;

  sTest01 = (char*) g_new0(char, TEST_STRING_MAX) ;

  /*
   * First one of each group should pass, all others should fail.
   * 31/07/2014 Current version of functions passes all these tests.
   */

  /* settings 1 */
  properties.bDigitsLeft = TRUE ;
  properties.bMayOmit1 = TRUE ;
  properties.bSpaces = FALSE ;

  sprintf(sTest01, "33M13I52M10I1980M") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  bResult = parse_remove_invalid_operators(canon, operators_cigar_bam, &num_removed) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, " 33M13I52M10I1980MII") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  bResult = parse_remove_invalid_operators(canon, operators_cigar_bam, &num_removed) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33 M13I52M10I1980MII") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "3M13I52M10I 1980MII ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M13I52$10I1980MII") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "=33M13I52M10I1980MII") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  /* settings 2 */
  properties.bDigitsLeft = FALSE ;
  properties.bMayOmit1 = TRUE ;
  properties.bSpaces = FALSE ;

  sprintf(sTest01, "M3I13M52I10M1980II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3 I13M52I10M1980II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, " M3I13M52I10M1980II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M 3I13M52I10M1980II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3I13M52I10M1980II1 ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3I13M52I10 M1980II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3I13M52&10M1980II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3I13M52I10M1980I*") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  /* settings 3 */
  properties.bDigitsLeft = TRUE ;
  properties.bMayOmit1 = TRUE ;
  properties.bSpaces = TRUE ;

  sprintf(sTest01, "33M 13I 52M 10I 1980M I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, " 33M 13I 52M 10I 1980M I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M13I 52M 10I 1980M I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M 13I 52M 10I 1980MI I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M 13I 52M 10I 1980M II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M 13I 52M 10I 1980M I I1") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M 13I 52 10I 1980M I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M 13I 52M 10I 1980M I=I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  /* settings 4 */
  properties.bDigitsLeft = FALSE ;
  properties.bMayOmit1 = TRUE ;
  properties.bSpaces = TRUE;
  sprintf(sTest01, "M33 I13 M52 I10 M1980 I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M33 I13 M52 I10 M1980 I I ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M33 I13 M52 I10 M1980 II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M33 I13 M52I10 M1980 I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M33 II13 M52 I10 M1980 I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M33 I13 M52 I10M1980 I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M33 I13 M52 I10 M1980 I ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  /* settings 5 (cigar_bam) */
  properties.bDigitsLeft = TRUE ;
  properties.bMayOmit1 = FALSE ;
  properties.bSpaces = FALSE ;

  sprintf(sTest01, "33M13I52M10I1980M1I13I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M13I52M10I1980M1I13I ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M13I52M10I1980M1I13I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M13I52M10I1980M1II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M13I52 M10I1980M1I13I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M13I52M 10I1980M1I13I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, " 33M13I52M10I1980M1I13I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M13I52M10I1980M1I13I ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  /* settings 6 */
  properties.bDigitsLeft = FALSE ;
  properties.bMayOmit1 = FALSE ;
  properties.bSpaces = FALSE ;

  sprintf(sTest01, "M3I13M52I10M1980I1I18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3I13M52I10M1980I1I18 ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3 I13M52I10M1980I1I18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, " M3I13M52I10M1980I1I18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "MI13M52I10M1980I1I18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3I13M52I10M1980I1I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3I13M52I10M1980II18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3I13M52 I10M1980I1I18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3I13M52I10M19&80I1I18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  /* settings 7 */
  properties.bDigitsLeft = TRUE ;
  properties.bMayOmit1 = FALSE ;
  properties.bSpaces = TRUE ;

  sprintf(sTest01, "3M 13I 52M 10I 1980M 1I 200I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M 13I 52M 10I 1980M 1I 200I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "3M13I 52M 10I 1980M 1I 200I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "3M 13I 52M 10I 1980M 1I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "3M 13I 52M 10I 1980M 1I 200I ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "3M 13I M 10I 1980M 1I 200I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, " 3M 13I 52M 10I 1980M 1I 200I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "3M 13I 52M && 10I 1980M 1I 200I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  /* settings 8 */
  properties.bDigitsLeft = FALSE ;
  properties.bMayOmit1 = FALSE ;
  properties.bSpaces = TRUE ;

  sprintf(sTest01, "M31 I13 M52 I10 M1980 I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M31I13 M52 I10 M1980 I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M31 I13 M52 10 M1980 I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M31 I13 M52 I10 M1980 I3 I32 ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, " M31 I13 M52 I10 M1980 I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M31 I13 M52 I10 M I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "31 I13 M52 I10 M1980 I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M31 I13 M52 I&10 M1980 I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  if (sTest01)
    g_free(sTest01) ;
}
#undef TEST_STRING_MAX


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


/*
 * Create an AlignStrCanonical object.
 */
static AlignStrCanonical alignStrCanonicalCreate(ZMapFeatureAlignFormat align_format)
{
  AlignStrCanonical canon = NULL ;
  zMapReturnValIfFail(align_format != ZMAPALIGN_FORMAT_INVALID, canon) ;

  canon = g_new0(AlignStrCanonicalStruct, 1) ;
  if (canon)
    {
      canon->format = align_format ;
      canon->align = alignStrOpArrayCreate() ;
      canon->num_operators = 0 ;
    }

  return canon ;
}


/*
 * Destroy an AlignStrCanonical object.
 */
static void alignStrCanonicalDestroy(AlignStrCanonical *canon)
{
  if (canon && *canon)
    {
      alignStrOpArrayDestroy((*canon)->align) ;
      g_free(*canon) ;
      *canon = NULL ;
    }

  return ;
}


/*
 * Create an align array.
 */
static GArray * alignStrOpArrayCreate()
{
  return g_array_new(FALSE, TRUE, sizeof(AlignStrOpStruct)) ;
}

/*
 * Destroy an align array.
 */
static void alignStrOpArrayDestroy(GArray * const align_array)
{
  if (align_array)
    g_array_free(align_array, TRUE) ;
}

/*
 * Return a pointer to the -ith operator in the canonical form.
 */
static AlignStrOp alignStrCanonicalGetOperator(AlignStrCanonical canon, int i)
{
  AlignStrOp op = NULL ;
  if (canon && canon->align && i>= 0 && i<canon->num_operators)
    {
       op = &(g_array_index(canon->align, AlignStrOpStruct, i)) ;
    }
  return op ;
}

/*
 * Return the character associated with the i-th operator.
 */
static char alignStrCanonicalGetOperatorChar(AlignStrCanonical canon, int i)
{
  char cOp = '\0' ;
  AlignStrOp op = alignStrCanonicalGetOperator(canon, i) ;
  if (op)
    {
      cOp = op->op ;
    }
  return cOp ;
}

/*
 * Add an operator to the end of the canonical
 */
static gboolean alignStrCanonicalAddOperator(AlignStrCanonical canon, const AlignStrOpStruct * const op)
{
  gboolean result = FALSE ;
  if (canon && canon->align && op )
    {
      canon->align = g_array_append_val(canon->align, *op) ;
      ++canon->num_operators ;
      result = TRUE ;
    }
  return result ;
}

/*
 * Remove the i-th operator from the canonical form
 */
static gboolean alignStrCanonicalRemoveOperator(AlignStrCanonical canon, int i)
{
  gboolean result = FALSE ;
  if (canon && canon->align && i>= 0 && i<canon->num_operators)
    {
      canon->align = g_array_remove_index(canon->align, i) ;
      --canon->num_operators ;
      result = TRUE ;
    }
  return result ;
}








/*
 * Takes a match string and format and converts that string into a canonical form
 * for further processing.
 */
static AlignStrCanonical alignStrMakeCanonical(char *match_str, ZMapFeatureAlignFormat align_format)
{
  AlignStrCanonical canon = NULL ;
  gboolean result = FALSE ;

  zMapReturnValIfFail(match_str && *match_str, canon) ;

  canon = alignStrCanonicalCreate(align_format) ;

  switch (align_format)
    {
    case ZMAPALIGN_FORMAT_GAP_GFF3:
      result = gffv3Gap2Canon(match_str, canon) ;
      break ;
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
      zMapWarnIfReached() ;
      break ;
    }

  /*
   * final test of validity
   */
  GError *error = NULL;
  result = parse_canon_valid(canon, &error) ;

  if (error)
    {
      zMapLogWarning("Error processing alignment string '%s': %s", match_str, error->message);
      g_error_free(error);
      error = NULL;
    }

  if (!result)
    {
      alignStrCanonicalDestroy(&canon) ;
      canon = NULL ;
    }

  return canon ;
}



/*
 * Convert the canonical "string" to a zmap align array, note that we do this blindly
 * assuming that everything is ok because we have verified the string....
 *
 * Note that this routine does not support vulgar unless the vulgar string only contains
 * M, I and G (instead of D).
 *
 *
 */
static gboolean alignStrCanon2Homol(AlignStrCanonical canon,
                                    ZMapStrand ref_strand, ZMapStrand match_strand,
                                    int p_start, int p_end, int c_start, int c_end,
                                    GArray **local_map_out)
{
  gboolean result = TRUE ;
  int curr_ref, curr_match ;
  int i, j ;
  GArray *local_map = NULL,
    *align = NULL ;
  AlignBlockBoundaryType boundary_type = ALIGN_BLOCK_BOUNDARY_EDGE ;
  ZMapAlignBlock prev_gap = NULL ;
  AlignStrOpStruct *op = NULL ;

  zMapReturnValIfFail(canon, FALSE ) ;
  align = canon->align ;

  /* is there a call to do this in feature code ??? (sm23) there is now... */
  local_map = zMapAlignBlockArrayCreate() ;

  if (ref_strand == ZMAPSTRAND_FORWARD)
    curr_ref = p_start ;
  else
    curr_ref = p_end ;

  if (match_strand == ZMAPSTRAND_FORWARD)
    curr_match = c_start ;
  else
    curr_match = c_end ;


  for (i = 0, j = 0 ; i < align->len ; ++i)
    {
      /* If you alter this code be sure to notice the += and -= which alter curr_ref and curr_match. */
      int curr_length = 0 ;
      ZMapAlignBlockStruct gap = {0} ;

      op = alignStrCanonicalGetOperator(canon, i) ;

      if (op)
        {
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
              case 'D':    /* Deletion in reference sequence. */
              case 'G':
                {
                  if (ref_strand == ZMAPSTRAND_FORWARD)
                    curr_ref += curr_length ;
                  else
                    curr_ref -= curr_length ;

                  boundary_type = ALIGN_BLOCK_BOUNDARY_DELETION ;
                  break ;
                }
              case 'I':    /* Insertion from match sequence. */
                {
                  if (match_strand == ZMAPSTRAND_FORWARD)
                    curr_match += curr_length ;
                  else
                    curr_match -= curr_length ;

                  boundary_type = ALIGN_BLOCK_BOUNDARY_MATCH ; /* it is shown butted up to the previous align block */
                  break ;
                }
              case 'M':    /* Match. */
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

                  zMapAlignBlockAddBlock(&local_map, &gap) ;
                  ++j ;    /* increment for next gap element. */

                  boundary_type = ALIGN_BLOCK_BOUNDARY_MATCH ;
                  break ;
                }
              default:
                {
                  zMapWarning("Unrecognized operator '%c' in align string\n", op->op) ;
                  zMapWarnIfReached() ;

                  break ;
                }
            } /* switch (op->op) */

          if (prev_gap)
            prev_gap->end_boundary = boundary_type ;

          if (op->op == 'M')
            prev_gap = zMapAlignBlockArrayGetBlock(local_map, local_map->len-1) ;
          else
            prev_gap = NULL ;

        } /* if (op) */

    } /* for (i = 0, j = 0 ; i < align->len ; ++i) */

  if (local_map && local_map->len == 1)
    {
      /* If there is only one match block then it's ungapped and we must set the
       * gaps array to null (the code relies on this being null to indicate ungapped) */
      g_array_free(local_map, TRUE);
      local_map = NULL;
    }

  *local_map_out = local_map ;

  return result ;
}




/*
 * Converts an internal align array into the AlignStrCanonical representation.
 * We only handle MID operators. As if that's not bad enough.
 */
static AlignStrCanonical homol2AlignStrCanonical(ZMapFeature feature, ZMapFeatureAlignFormat align_format)
{
  typedef enum {TYPE_NONE, TYPE_I, TYPE_D} OperatorCase ;
  static const char cM = 'M',
                    cI = 'I',
                    cD = 'D' ;
  ZMapAlignBlock this_block = NULL, last_block = NULL ;
  AlignStrCanonical canon = NULL ;
  GArray* align = NULL ;
  int i = 0, length = 0 ;

  zMapReturnValIfFail(  feature &&
                        feature->mode == ZMAPSTYLE_MODE_ALIGNMENT &&
                        align_format != ZMAPALIGN_FORMAT_INVALID, canon) ;
  align = feature->feature.homol.align ;
  canon = alignStrCanonicalCreate(align_format) ;

  if (align != NULL )
    length = align->len ;

  if (length == 0)
    {
      /*
       * Assume there is only one operator that covers the whole object and
       * is taken to be <n>M where n = end-start-1.
       */
      AlignStrOpStruct op ;
      op.op = cM ;
      op.length = feature->feature.homol.y2 - feature->feature.homol.y1 + 1 ;

      alignStrCanonicalAddOperator(canon, &op) ;
    }
  else
    {

      /*
       * Loop over align blocks.
       */
      for (i=0; i<length; ++i)
        {
          /*
           * Identify the current block.
           */
          this_block = zMapAlignBlockArrayGetBlock(align, i) ;

          /*
           * Create I or D to represent whatever is between last_block and
           * this_block.
           */
          if (i>0)
            {
              if (last_block && this_block)
                {
                  AlignStrOpStruct op ;

                  if ((last_block->q2+1 == this_block->q1) && (last_block->t2+1 < this_block->t1))
                    {
                      /* this is a D operator */
                      op.op = cD ;
                      op.length = this_block->t1-last_block->t2-1 ;
                      if (op.length >= 1)
                        alignStrCanonicalAddOperator(canon, &op) ;
                    }
                  else if ((last_block->q2+1 < this_block->q1) && (last_block->t2+1 == this_block->t1))
                    {
                      /* this is a I operator */
                      op.op = cI ;
                      op.length = this_block->q1-last_block->q2-1 ;
                      if (op.length >= 1)
                        alignStrCanonicalAddOperator(canon, &op) ;
                    }

                }
            }

          /*
           * Create M operator for current block and add to canon.
           */
          AlignStrOpStruct op ;
          op.op = cM ;
          op.length = this_block->q2 - this_block->q1 + 1 ;
          if (op.length >= 1)
            alignStrCanonicalAddOperator(canon, &op) ;

          /*
           * End of iteration
           */
          last_block = this_block ;
        }

    }

  /*
   *
   */

  return canon ;
}


/*
 * This function converts the canon item to a string but does nothing else.
 */
static gboolean alignStrCanonical2string(char ** p_string,
                                         AlignStrCanonicalProperties properties,
                                         AlignStrCanonical canon)
{
  gboolean result = FALSE ;
  int i = 0 ;
  AlignStrOp op = NULL ;
  char *temp_string = NULL, *buff = NULL ;
  gboolean bDigitsLeft = FALSE,
    bSpaces = FALSE ;

  zMapReturnValIfFail(p_string &&
                      canon &&
                      canon->align &&
                      (canon->format!=ZMAPALIGN_FORMAT_INVALID) &&
                      properties,  result) ;

  bDigitsLeft = properties->bDigitsLeft ;
  bSpaces = properties->bSpaces ;

  if (canon->num_operators)
    {
      op = alignStrCanonicalGetOperator(canon, 0) ;
      temp_string = bDigitsLeft ?
        g_strdup_printf("i%c", op->length) : g_strdup_printf("%c%i", op->op, op->length) ;

      for (i=1; i<canon->num_operators; ++i)
        {
          op = alignStrCanonicalGetOperator(canon, i) ;
          buff = temp_string ;
          if (bSpaces)
            temp_string = bDigitsLeft ?
              g_strdup_printf("%s %i%c", buff, op->length, op->op) : g_strdup_printf("%s %c%i", buff, op->op, op->length);
          else
            temp_string = bDigitsLeft ?
              g_strdup_printf("%s%i%c", buff, op->length, op->op) : g_strdup_printf("%s%c%i", buff, op->op, op->length);
          g_free(buff) ;
        }

      *p_string = temp_string ;
      result = TRUE ;
    }

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
      zMapWarnIfReached() ;
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

/*
 *
 */
static gboolean gffv3Gap2Canon(char *match_str, AlignStrCanonical canon)
{
  gboolean result = FALSE, canon_edited = FALSE ;
  int num_removed = 0 ;
  GError *error = NULL ;
  AlignStrCanonicalPropertiesStruct properties ;

  zMapReturnValIfFail(canon &&
                      canon->align &&
                      (canon->format==ZMAPALIGN_FORMAT_GAP_GFF3) &&
                      (canon->num_operators==0),                           result ) ;

  result = TRUE ;

#ifndef USE_NEW_CIGAR_PARSING_CODE

#else

  alignFormat2Properties(canon->format, &properties) ;
  if ((result = parse_cigar_general(match_str, canon, &properties, &error)))
    {
      canon_edited = alignStrCanonicalSubstituteGFFv3Gap(canon) ;
      canon_edited = parse_remove_invalid_operators(canon, operators_gffv3_gap, &num_removed) ;
    }
  if (error)
    {
      g_error_free(error) ;
    }

#endif

  return result ;
}

/*
 *
 */
static gboolean exonerateCigar2Canon(char *match_str, AlignStrCanonical canon)
{
  gboolean result = FALSE, canon_edited = FALSE ;
  int num_removed = 0 ;
  GError *error = NULL ;
  AlignStrCanonicalPropertiesStruct properties ;

  zMapReturnValIfFail(canon &&
                      canon->align &&
                      (canon->format==ZMAPALIGN_FORMAT_CIGAR_EXONERATE) &&
                      (canon->num_operators==0),                           result ) ;

  result = TRUE ;

#ifndef USE_NEW_CIGAR_PARSING_CODE

  /*
   * (sm23) This old stuff is a load of toss and quite clearly has never worked.
   */
  cp = match_str ;
  do
    {
      AlignStrOpStruct op = {'\0'} ;

      op.op = *cp ;
      cp++ ;
      gotoLastSpace(&cp) ;
      cp++ ;

      op.length = cigarGetLength(&cp) ;    /* N.B. moves cp on as needed. */

      gotoLastSpace(&cp) ;
      if (*cp == ' ')
        cp ++ ;

      canon->align = g_array_append_val(canon->align, op) ;
    } while (*cp) ;

#else

  alignFormat2Properties(canon->format, &properties) ;
  if ((result = parse_cigar_general(match_str, canon, &properties, &error)))
    {
      canon_edited = alignStrCanonicalSubstituteExonerateCigar(canon) ;
      canon_edited = parse_remove_invalid_operators(canon, operators_cigar_exonerate, &num_removed) ;
    }
  if (error)
    {
      g_error_free(error) ;
    }

#endif

  return result ;
}


/* Not straight forward to implement, wait and see what we need.... */
static gboolean exonerateVulgar2Canon(char *match_str, AlignStrCanonical canon)
{
  gboolean result = FALSE ;

  zMapReturnValIfFail(canon &&
                      canon->align &&
                      (canon->format==ZMAPALIGN_FORMAT_VULGAR_EXONERATE) &&
                      (canon->num_operators==0),                           result) ;

  return result ;
}


/*
 * Blindly converts, assumes match_str is a valid ensembl cigar string.
 */
static gboolean ensemblCigar2Canon(char *match_str, AlignStrCanonical canon)
{
  gboolean result = FALSE, canon_edited = FALSE ;
  int num_removed = 0 ;
  GError *error = NULL ;
  AlignStrCanonicalPropertiesStruct properties ;

  zMapReturnValIfFail(canon &&
                      canon->align &&
                      (canon->format==ZMAPALIGN_FORMAT_CIGAR_ENSEMBL) &&
                      (canon->num_operators == 0),                       result) ;

  result = TRUE ;

#ifndef USE_NEW_CIGAR_PARSING_CODE

  cp = match_str ;
  do
    {
      AlignStrOpStruct op = {'\0'} ;

      if (g_ascii_isdigit(*cp))
        op.length = cigarGetLength(&cp) ;    /* N.B. moves cp on as needed. */
      else
        op.length = 1 ;

      /* EnsEMBL CIGAR interchanges 'D' and 'I' */
      op.op =
        (*cp == 'D') ? 'I' :
        (*cp == 'I') ? 'D' :
        *cp;

      canon->align = g_array_append_val(canon->align, op) ;

      cp++ ;
    } while (*cp) ;

#else

  alignFormat2Properties(canon->format, &properties) ;
  if ((result = parse_cigar_general(match_str, canon, &properties, &error)))
    {
      canon_edited = alignStrCanonicalSubstituteEnsemblCigar(canon) ;
      canon_edited = parse_remove_invalid_operators(canon, operators_cigar_ensembl, &num_removed) ;
    }
  if (error)
    {
      g_error_free(error) ;
    }

#endif

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
  gboolean result = FALSE, canon_edited = FALSE ;
  int num_removed = 0 ;
  GError *error = NULL ;
  AlignStrCanonicalPropertiesStruct properties ;

  zMapReturnValIfFail(canon &&
                      canon->align &&
                      (canon->format==ZMAPALIGN_FORMAT_CIGAR_BAM) &&
                      (canon->num_operators==0),                     result) ;

  result = TRUE ;

  parseTestFunction01() ;

#ifndef USE_NEW_CIGAR_PARSING_CODE

  cp = match_str ;
  do
    {
      AlignStrOpStruct op = {'\0'} ;

      if (g_ascii_isdigit(*cp))
        op.length = cigarGetLength(&cp) ;    /* N.B. moves cp on as needed. */
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
          ++canon->num_operators ;
        }

      cp++ ;
    } while (*cp) ;

#else

  /*
   * Same thing using new approach
   */
  alignFormat2Properties(canon->format, &properties) ;
  if ((result = parse_cigar_general(match_str, canon, &properties, &error)))
    {
      canon_edited = alignStrCanonicalSubstituteBamCigar(canon) ;
      canon_edited = parse_remove_invalid_operators(canon, operators_cigar_bam, &num_removed) ;
    }
  if (error)
    {
      /*
       * This is a parse error, which could be passed to the caller, but at the moment
       * there is no facility for handling it, so will be destroyed here.
       */
      g_error_free(error) ;
    }

#endif

  return result ;
}



/* Returns -1 if string could not be converted to number. */
#if NOT_USED
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
#endif


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

      cp-- ;    /* position on last digit. */

      *cp_inout = cp ;
      result = TRUE ;
    }


  return result ;
}



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

      cp-- ;    /* position on last space. */

      *cp_inout = cp ;
      result = TRUE ;
    }


  return result ;
}

#endif



/*
 * These functions substitute some operators from one type to another.
 * For example
 *
 * (a) In cigar_bam we replace X with M
 * (b) In cigar_ensembl replace D with I and I with D
 *
 * Return TRUE if one or more substitution is made, FALSE otherwise.
 */
static gboolean alignStrCanonicalSubstituteBamCigar(AlignStrCanonical canon)
{
  int i = 0 ;
  AlignStrOp align_op = NULL ;
  gboolean result = FALSE ;
  zMapReturnValIfFail(canon && canon->format == ZMAPALIGN_FORMAT_CIGAR_BAM, result ) ;

  for (i=0; i<canon->num_operators; ++i)
    {
      align_op = alignStrCanonicalGetOperator(canon, i) ;
      if (align_op->op == 'X')
        {
          align_op->op = 'M' ;
          result = TRUE ;
        }
    }
  return result ;
}

static gboolean alignStrCanonicalSubstituteEnsemblCigar(AlignStrCanonical canon)
{
  int i = 0 ;
  AlignStrOp align_op = NULL ;
  gboolean result = FALSE ;
  zMapReturnValIfFail(canon && canon->format == ZMAPALIGN_FORMAT_CIGAR_ENSEMBL, result) ;

  for (i=0; i<canon->num_operators; ++i)
    {
      align_op = alignStrCanonicalGetOperator(canon, i) ;
      if (align_op->op == 'D')
        {
          align_op->op = 'I' ;
          result = TRUE ;
        }
      else if (align_op->op == 'I')
        {
          align_op->op = 'D' ;
          result = TRUE ;
        }
    }

  return result ;
}

static gboolean alignStrCanonicalSubstituteExonerateCigar(AlignStrCanonical canon)
{
  gboolean result = FALSE ;
  zMapReturnValIfFail(canon && canon->format == ZMAPALIGN_FORMAT_CIGAR_EXONERATE, result) ;
  return result ;
}


static gboolean alignStrCanonicalSubstituteGFFv3Gap(AlignStrCanonical canon)
{
  gboolean result = FALSE ;
  zMapReturnValIfFail(canon && canon->format == ZMAPALIGN_FORMAT_GAP_GFF3, result) ;
  return result ;
}

#ifdef NOT_USED
static gboolean alignStrCanonicalSubstituteExonerateVulgar(AlignStrCanonical canon)
{
  gboolean result = FALSE ;
  zMapReturnValIfFail(canon && canon->format == ZMAPALIGN_FORMAT_VULGAR_EXONERATE, result) ;
  return result ;
}
#endif



