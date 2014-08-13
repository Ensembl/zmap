/*  File: zmapFeatureAlignment.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2014: Genome Research Ltd.
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
static gboolean alignProperties2Format(ZMapFeatureAlignFormat *align_format,
                                       AlignStrCanonicalProperties properties) ;
static gboolean homol2AlignStrCanonical(AlignStrCanonical *canon_out, ZMapFeature feature) ;
static gboolean parse_cigar_general(const char * const str,
                                    AlignStrCanonical canon,
                                    AlignStrCanonicalProperties properties,
                                    GError **error) ;

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
static gboolean exonerateVulgar2Canon(char *match_str, AlignStrCanonical canon) ;
static gboolean ensemblCigar2Canon(char *match_str, AlignStrCanonical canon) ;
static gboolean bamCigar2Canon(char *match_str, AlignStrCanonical canon) ;

static int cigarGetLength(char **cigar_str) ;

static gboolean alignStrCanonical2ExonerateCigar(char ** align_str, AlignStrCanonical canon) ;
static gboolean alignStrCanonical2EnsemblCigar(char ** align_str, AlignStrCanonical canon) ;
static gboolean alignStrCanonical2BamCigar(char ** align_str, AlignStrCanonical canon) ;
static gboolean alignStrCanonical2ExonerateVulgar(char ** align_str, AlignStrCanonical canon) ;

static gboolean alignStrCanonicalSubstituteBamCigar(AlignStrCanonical canon) ;
static gboolean alignStrCanonicalSubstituteEnsemblCigar(AlignStrCanonical canon) ;
static gboolean alignStrCanonicalSubstituteExonerateCigar(AlignStrCanonical canon) ;
static gboolean alignStrCanonicalSubstituteExonerateVulgar(AlignStrCanonical canon) ;

#if NOT_USED
static char *nextWord(char *str) ;
static gboolean gotoLastDigit(char **cp_inout) ;
#endif

static gboolean gotoLastSpace(char **cp_inout) ;

/*
 * These arrays are the allowed operators for various of the format types.
 */
static const char *operators_cigar_bam       = "MIDN" ;
static const char *operators_cigar_exonerate = "MIDFR" ;
static const char *operators_gffv3_gap       = "MIDFR" ;

/*
 * Allowed operators for internal "canonical" representation.
 */
static const char *operators_canonical = "MINDG";


/*
 *               External functions.
 */


ZMAP_ENUM_TO_SHORT_TEXT_FUNC(zMapFeatureAlignFormat2ShortText, ZMapFeatureAlignFormat, ZMAP_ALIGN_GAP_FORMAT_LIST) ;



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
        /* properties->bDigitsLeft = FALSE ;
        properties->bMayOmit1   = FALSE ;
        properties->bSpaces     = FALSE ;
        result = TRUE ; */
        break ;
      case ZMAPALIGN_FORMAT_CIGAR_BAM:
        properties->bDigitsLeft = TRUE ;
        properties->bMayOmit1   = TRUE ;
        properties->bSpaces     = FALSE ;
        result = TRUE ;
        break ;
      case ZMAPALIGN_FORMAT_VULGAR_EXONERATE:
      default:
        break ;
    }

  return result ;
}

/*
 * Convert a Canonical properties to align format. Unfortunately, this is not 1:1 mapping,
 * so the choice to favour gffv3 Gap over exonerate is made.
 */
static gboolean alignProperties2Format(ZMapFeatureAlignFormat *align_format_out,
                                       AlignStrCanonicalProperties properties)
{
  gboolean result = FALSE ;
  ZMapFeatureAlignFormat align_format = ZMAPALIGN_FORMAT_INVALID;

  zMapReturnValIfFail(align_format_out && properties, result) ;

  /*
   * Go through various cases explicitly
   */
  if (!properties->bDigitsLeft && !properties->bMayOmit1 && properties->bSpaces)
    {
      align_format = ZMAPALIGN_FORMAT_GAP_GFF3 ;
      result = TRUE ;
    }
  else if (properties->bDigitsLeft && !properties->bMayOmit1 && !properties->bSpaces)
    {
      align_format = ZMAPALIGN_FORMAT_CIGAR_BAM ;
      result = TRUE ;
    }
  else
    {

    }

  if (result)
    *align_format_out = align_format ;

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
  new_size = factor * *p_buffer_size ;
  p_new = (char const **) g_new0(char*, new_size) ;
  memset(p_new, '\0', new_size) ;

  /*
   * Copy old array into new one
   */
  for (i=0; i<*p_buffer_size; ++i)
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
    data_found = FALSE,
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
          data_found = parse_get_op_data(i, sBuff, num_buffer_size, pArray, iOperators, str, bDigitsLeft ) ;
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
  GArray *align_temp = NULL ;
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

  sprintf(sTest01, "GT33M13I52M10I1980MII123LXK") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  bResult = alignStrCanonicalSubstituteBamCigar(canon) ;
  bResult = parse_remove_invalid_operators(canon, operators_cigar_bam, &num_removed) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, " 33M13I52M10I1980MII") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  bResult = parse_remove_invalid_operators(canon, operators_cigar_bam, &num_removed) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "33 M13I52M10I1980MII") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "3M13I52M10I 1980MII ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "33M13I52$10I1980MII") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "=33M13I52M10I1980MII") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  /* settings 2 */
  properties.bDigitsLeft = FALSE ;
  properties.bMayOmit1 = TRUE ;
  properties.bSpaces = FALSE ;

  sprintf(sTest01, "M3I13M52I10M1980II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M3 I13M52I10M1980II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, " M3I13M52I10M1980II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M 3I13M52I10M1980II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M3I13M52I10M1980II1 ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M3I13M52I10 M1980II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M3I13M52&10M1980II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M3I13M52I10M1980I*") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  /* settings 3 */
  properties.bDigitsLeft = TRUE ;
  properties.bMayOmit1 = TRUE ;
  properties.bSpaces = TRUE ;

  sprintf(sTest01, "33M 13I 52M 10I 1980M I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, " 33M 13I 52M 10I 1980M I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "33M13I 52M 10I 1980M I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "33M 13I 52M 10I 1980MI I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "33M 13I 52M 10I 1980M II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "33M 13I 52M 10I 1980M I I1") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "33M 13I 52 10I 1980M I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "33M 13I 52M 10I 1980M I=I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  /* settings 4 */
  properties.bDigitsLeft = FALSE ;
  properties.bMayOmit1 = TRUE ;
  properties.bSpaces = TRUE;
  sprintf(sTest01, "M33 I13 M52 I10 M1980 I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M33 I13 M52 I10 M1980 I I ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M33 I13 M52 I10 M1980 II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M33 I13 M52I10 M1980 I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M33 II13 M52 I10 M1980 I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M33 I13 M52 I10M1980 I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M33 I13 M52 I10 M1980 I ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  /* settings 5 (cigar_bam) */
  properties.bDigitsLeft = TRUE ;
  properties.bMayOmit1 = FALSE ;
  properties.bSpaces = FALSE ;

  sprintf(sTest01, "33M13I52M10I1980M1I13I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "33M13I52M10I1980M1I13I ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M13I52M10I1980M1I13I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "33M13I52M10I1980M1II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "33M13I52 M10I1980M1I13I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "33M13I52M 10I1980M1I13I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, " 33M13I52M10I1980M1I13I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "33M13I52M10I1980M1I13I ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  /* settings 6 */
  properties.bDigitsLeft = FALSE ;
  properties.bMayOmit1 = FALSE ;
  properties.bSpaces = FALSE ;

  sprintf(sTest01, "M3I13M52I10M1980I1I18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M3I13M52I10M1980I1I18 ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M3 I13M52I10M1980I1I18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, " M3I13M52I10M1980I1I18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "MI13M52I10M1980I1I18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M3I13M52I10M1980I1I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M3I13M52I10M1980II18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M3I13M52 I10M1980I1I18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M3I13M52I10M19&80I1I18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  /* settings 7 */
  properties.bDigitsLeft = TRUE ;
  properties.bMayOmit1 = FALSE ;
  properties.bSpaces = TRUE ;

  sprintf(sTest01, "3M 13I 52M 10I 1980M 1I 200I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M 13I 52M 10I 1980M 1I 200I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "3M13I 52M 10I 1980M 1I 200I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "3M 13I 52M 10I 1980M 1I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "3M 13I 52M 10I 1980M 1I 200I ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "3M 13I M 10I 1980M 1I 200I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, " 3M 13I 52M 10I 1980M 1I 200I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "3M 13I 52M && 10I 1980M 1I 200I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  /* settings 8 */
  properties.bDigitsLeft = FALSE ;
  properties.bMayOmit1 = FALSE ;
  properties.bSpaces = TRUE ;

  sprintf(sTest01, "M31 I13 M52 I10 M1980 I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M31I13 M52 I10 M1980 I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M31 I13 M52 10 M1980 I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M31 I13 M52 I10 M1980 I3 I32 ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, " M31 I13 M52 I10 M1980 I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M31 I13 M52 I10 M I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "31 I13 M52 I10 M1980 I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  sprintf(sTest01, "M31 I13 M52 I&10 M1980 I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parse_cigar_general(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;

  if (sTest01)
    g_free(sTest01) ;
}
#undef TEST_STRING_MAX








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
  char *align_str = NULL ;
  AlignStrCanonical canon = NULL ;

  /*
   * Test feature type and other errors
   */
  zMapReturnValIfFail(feature && (feature->mode == ZMAPSTYLE_MODE_ALIGNMENT), result) ;

  /*
   * Homol to canonical
   */
  result = homol2AlignStrCanonical(&canon, feature) ;

  /*
   * Convert the canonical to the appropriate string format
   */
  if (result)
    {

      switch (align_format)
        {
          case ZMAPALIGN_FORMAT_GAP_GFF3:
          case ZMAPALIGN_FORMAT_CIGAR_EXONERATE:
            result = alignStrCanonical2ExonerateCigar(&align_str, canon) ;
            break ;
          case ZMAPALIGN_FORMAT_CIGAR_ENSEMBL:
            result = alignStrCanonical2EnsemblCigar(&align_str, canon) ;
            break ;
          case ZMAPALIGN_FORMAT_CIGAR_BAM:
            result = alignStrCanonical2BamCigar(&align_str, canon) ;
            break ;
          case ZMAPALIGN_FORMAT_VULGAR_EXONERATE:
            result = alignStrCanonical2ExonerateVulgar(&align_str, canon) ;
            break ;
          default:
            zMapWarnIfReached() ;
            break ;
        }

      if (result)
        {
          *p_string_out = align_str ;
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
      zMapLogWarning("Cannot convert alignment string to align array: s", align_string) ;
    }

  if (!result)
    {
      if (canon)
        alignStrCanonicalDestroy(&canon) ;
    }
  else
    {
      *align_out = align ;
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








/* Takes a match string and format and converts that string into a canonical form
 * for further processing. */
static AlignStrCanonical alignStrMakeCanonical(char *match_str, ZMapFeatureAlignFormat align_format)
{
  AlignStrCanonical canon = NULL ;
  gboolean result = FALSE ;

  zMapReturnValIfFail(match_str && *match_str, canon) ;

  canon = alignStrCanonicalCreate(align_format) ;

  switch (align_format)
    {
    case ZMAPALIGN_FORMAT_GAP_GFF3:
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

  if (!result)
    {
      alignStrCanonicalDestroy(&canon) ;
      canon = NULL ;
    }

  return canon ;
}



/* Convert the canonical "string" to a zmap align array, note that we do this blindly
 * assuming that everything is ok because we have verified the string....
 *
 * Note that this routine does not support vulgar unless the vulgar string only contains
 * M, I and G (instead of D).
 *
 *  */
static gboolean alignStrCanon2Homol(AlignStrCanonical canon,
                                    ZMapStrand ref_strand, ZMapStrand match_strand,
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
      AlignStrOp op = NULL ;
      int curr_length = 0 ;
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

            local_map = g_array_append_val(local_map, gap) ;

            j++ ;    /* increment for next gap element. */

                    boundary_type = ALIGN_BLOCK_BOUNDARY_MATCH ;
            break ;
          }
        default:
          {
            zMapWarning("Unrecognized operator '%c' in align string\n", op->op) ;
            zMapWarnIfReached() ;

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
 * Converts an internal align array into the "canonical" string representation.
 * Opposite operation to the function alignStrCanon2Homol() above.
 */
static gboolean homol2AlignStrCanonical(AlignStrCanonical *canon_out, ZMapFeature feature)
{
  static const char cM = 'M',
                    cI = 'I',
                    cD = 'D' ;
  ZMapAlignBlock block = NULL ;
  gboolean result = FALSE ;
  GArray* align = NULL ;
  int i = 0, length = 0 ;

  zMapReturnValIfFail(  canon_out
                     && feature
                     && feature->mode == ZMAPSTYLE_MODE_ALIGNMENT
                     && feature->feature.homol.align,
                     result) ;
  align = feature->feature.homol.align ;
  length = align->len ;

  for (i=0; i<length; ++i)
    {
      block = &(g_array_index(align, ZMapAlignBlockStruct, i)) ;



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
 * Convert AlignStrCanonical to exonerate cigar string.
 */
static gboolean alignStrCanonical2ExonerateCigar(char ** p_align_str, AlignStrCanonical canon)
{
  gboolean result = FALSE ;
  char* temp_string ;

  temp_string = g_strdup_printf("%s", "Mi Dj F R123123 etc") ;

  return result ;
}

/*
 * Convert AlignStrCanonical to ensembl cigar string.
 *              NOT IMPLEMENTED
 */
static gboolean alignStrCanonical2EnsemblCigar(char ** p_align_str, AlignStrCanonical canon)
{
  gboolean result = FALSE ;

  return result ;
}

/*
 * Convert AlignStrCanonical to bam cigar string.
 *              NOT IMPLEMENTED
 */
static gboolean alignStrCanonical2BamCigar(char ** p_align_str, AlignStrCanonical canon)
{
  gboolean result = FALSE ;

  return result ;
}

/*
 * Convert AlignStrCanonical to exonerate vulgar string.
 *              NOT IMPLEMENTED
 */
static gboolean alignStrCanonical2ExonerateVulgar(char ** p_align_str, AlignStrCanonical canon)
{
  gboolean result = FALSE ;

  return result ;
}



/* Blindly converts, assumes match_str is a valid exonerate cigar string. */
static gboolean exonerateCigar2Canon(char *match_str, AlignStrCanonical canon)
{
  gboolean result = FALSE ;
  char *cp = NULL ;

  zMapReturnValIfFail(canon &&
                      canon->align &&
                      (canon->format==ZMAPALIGN_FORMAT_CIGAR_EXONERATE) &&
                      (canon->num_operators==0),                           result ) ;

  result = TRUE ;

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


/* Blindly converts, assumes match_str is a valid ensembl cigar string. */
static gboolean ensemblCigar2Canon(char *match_str, AlignStrCanonical canon)
{
  gboolean result = FALSE ;
  char *cp = NULL ;

  zMapReturnValIfFail(canon &&
                      canon->align &&
                      (canon->format==ZMAPALIGN_FORMAT_CIGAR_ENSEMBL) &&
                      (canon->num_operators == 0),                       result) ;

  result = TRUE ;

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
  char *cp = NULL;

  zMapReturnValIfFail(canon &&
                      canon->align &&
                      (canon->format==ZMAPALIGN_FORMAT_CIGAR_BAM) &&
                      (canon->num_operators==0),                     result) ;

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
      int num_removed = 0 ;
      GError *error = NULL ;
      AlignStrCanonicalPropertiesStruct properties ;
      alignFormat2Properties(canon->format, &properties) ;
      if ((result = parse_cigar_general(match_str, canon, &properties, &error)))
        {
          canon_edited = alignStrCanonicalSubstituteBamCigar(canon) ;
          canon_edited = parse_remove_invalid_operators(canon, operators_cigar_bam, &num_removed) ;
        }
      else if (error)
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

      cp-- ;    /* position on last digit. */

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

      cp-- ;    /* position on last space. */

      *cp_inout = cp ;
      result = TRUE ;
    }


  return result ;
}


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

static gboolean alignStrCanonicalSubstituteExonerateVulgar(AlignStrCanonical canon)
{
  gboolean result = FALSE ;
  zMapReturnValIfFail(canon && canon->format == ZMAPALIGN_FORMAT_VULGAR_EXONERATE, result) ;
  return result ;
}




