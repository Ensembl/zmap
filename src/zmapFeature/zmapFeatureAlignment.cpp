/*  File: zmapFeatureAlignment.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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

#include <glib.h>

#include <errno.h>
#include <string.h>
#include <ctype.h>

#include <ZMap/zmap.hpp>
#include <ZMap/zmapGFFStringUtils.hpp>

#include <zmapFeature_P.hpp>


// Align string parsing errors.
//   
#define ZMAP_ALIGNSTRING_PARSE_ERROR_DOMAIN "ZMAP_ALIGNSTRING_PARSE_ERROR_DOMAIN"

// A bit of a hack, need to test for this message so we can report this error
// but not fail on it as we have a lot of cigar strings of this type.
#define ALIGN_EVEN_OPERATORS "Alignment string has an even number of operators"


// Used to do overall parsing of a vulgar string.   
enum class VulgarParseState {START, IN_EXON, IN_INTRON, END} ;

// Used for "start"/"end" ops for vulgar string
#define VULGAR_NO_OP '\0'

#define BAD_OP_FORMAT "parsing failed in state %s because encountered bad op: \"%c\""

#define BAD_TRANSITION_FORMAT "parsing failed in state %s because previous -> current op transition" \
" should not happen: \"%c\" -> \"%c\""


/* WE SHOULD FILL THIS IN CORRECTLY FOR ENSEMBL CIGAR (I.E. REFERENCE LENGTH),
   EXONERATE CIGAR (I.E. MATCH LENGTH) ETC ETC...THEN THE INDIVIDUAL ROUTINES
   SHOULD USE THE RIGHT FIELD......
*/
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

  int first_length ;
  int second_length ;

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
 * bTwoNumbers         for each operator lengths for both query and target sequence are given.
 */
typedef struct AlignStrCanonicalPropertiesStruct_
{
  unsigned int bDigitsLeft     : 1 ;
  unsigned int bMayOmit1       : 1 ;
  unsigned int bSpaces         : 1 ;
  unsigned int bTwoNumbers     : 1 ;
} AlignStrCanonicalPropertiesStruct, *AlignStrCanonicalProperties ;


static gboolean checkForPerfectAlign(GArray *gaps, unsigned int align_error) ;

static AlignStrCanonical alignStrCanonicalCreate(ZMapFeatureAlignFormat align_format) ;
static void alignStrCanonicalDestroy(AlignStrCanonical *canon) ;
static AlignStrOp alignStrCanonicalGetOperator(AlignStrCanonical canon, int i) ;
static char alignStrCanonicalGetOperatorChar(AlignStrCanonical canon, int i) ;
static gboolean alignStrCanonicalAddOperator(AlignStrCanonical canon, const AlignStrOpStruct *const op) ;
static gboolean alignStrCanonicalRemoveOperator(AlignStrCanonical canon, int i) ;
static AlignStrCanonical alignStrMakeCanonical(const char *match_str, ZMapFeatureAlignFormat align_format) ;

static GArray *blockArrayCreate() ;
static ZMapAlignBlock blockArrayGetBlock(GArray *const, int index) ;
static gboolean blockAddBlock(GArray**, const ZMapAlignBlockStruct *const) ;

static GArray *alignStrOpArrayCreate() ;
static void alignStrOpArrayDestroy(GArray * const align_array) ;

static gboolean alignStrCigarCanon2Homol(AlignStrCanonical canon,
                                    ZMapStrand ref_strand, ZMapStrand match_strand,
                                    int p_start, int p_end, int c_start, int c_end,
                                    GArray **local_map_out) ;
static gboolean alignStrVulgarCanon2Exons(AlignStrCanonical canon,
                                    ZMapStrand ref_strand, ZMapStrand match_strand,
                                    int p_start, int p_end, int c_start, int c_end,
                                    GArray **exons_out, GArray **introns_out, GArray **exon_aligns_out,
                                    int *cds_start_out, int *cds_end_out,
                                    char **err_string) ;
static gboolean alignFormat2Properties(ZMapFeatureAlignFormat align_format,
                                       AlignStrCanonicalProperties properties) ;
static AlignStrCanonical homol2AlignStrCanonical(ZMapFeature feature, ZMapFeatureAlignFormat align_format) ;
static gboolean alignStrCanonical2string(char **p_string,
                                         AlignStrCanonicalProperties properties,
                                         AlignStrCanonical canon) ;

static gboolean parseCigarGeneral(const char *const str,
                                  AlignStrCanonical canon,
                                  AlignStrCanonicalProperties properties,
                                  GError **error) ;
static gboolean parseGetOpData(int i,
                               char *const sBuff,
                               size_t buffer_size,
                               const char **pArray,
                               int nOp,
                               const char * const target,
                               gboolean bLeft) ;
static bool parseIsValidNumber(char *sBuff, size_t iLen, int opt,
                               char **first_num_str_out, char **second_num_str_out) ;
static bool parseIsValidOpData(char *sBuff,
                               gboolean bMayOmit1, gboolean bLeft, gboolean bSpaces,
                               gboolean bEnd, gboolean bFirst,
                               gboolean bTwoNumbers,
                               int *first_num_out, int *second_num_out) ;
static bool parse_canon_valid(AlignStrCanonical canon, GError **error) ;
static gboolean exonerateCigar2Canon(const char *match_str, AlignStrCanonical canon) ;
static gboolean gffv3Gap2Canon(const char *match_str, AlignStrCanonical canon) ;
static gboolean exonerateVulgar2Canon(const char *match_str, AlignStrCanonical canon) ;
static gboolean ensemblCigar2Canon(const char *match_str, AlignStrCanonical canon) ;
static gboolean bamCigar2Canon(const char *match_str, AlignStrCanonical canon) ;
static gboolean alignStrCanonicalSubstituteBamCigar(AlignStrCanonical canon) ;
static gboolean alignStrCanonicalSubstituteEnsemblCigar(AlignStrCanonical canon) ;
static gboolean alignStrCanonicalSubstituteExonerateCigar(AlignStrCanonical canon) ;
static gboolean alignStrCanonicalSubstituteGFFv3Gap(AlignStrCanonical canon) ;

static void freeAligns(GArray *aligns);
static void incrementCoords(ZMapStrand ref_strand, int *curr_ref_inout, int ref_length,
                            ZMapStrand match_strand, int *curr_match_inout, int match_length) ;
static void calcCoords(ZMapStrand strand, int curr, int length, int *start, int *end) ;
static void addNewAlign(GArray *local_map,
                        ZMapStrand ref_strand, int curr_ref, int ref_length,
                        ZMapStrand match_strand, int curr_match, int match_length,
                        AlignBlockBoundaryType start_boundary, AlignBlockBoundaryType end_boundary) ;
static void extendAlignCoords(ZMapAlignBlock sub_align,
                              ZMapStrand ref_strand, int curr_ref, int ref_length,
                              ZMapStrand match_strand, int curr_match, int match_length) ;
static void setAlignBoundaries(ZMapAlignBlock sub_align,
                               AlignBlockBoundaryType start_boundary, AlignBlockBoundaryType end_boundary) ;
static void extendCoords(ZMapStrand strand, int curr, int length, int *start, int *end) ;



/*
 * These arrays are the allowed operators for various of the format types.
 *
 * Notes:
 *
 * (a) According to www.sequenceontology.org gffv3 Gap attribute is the same as
 *     exonerate cigar. We use the same code for both here.
 * (b) We do not handle frameshift operators.
 */
static const char *operators_cigar_exonerate = "MID" ;
static const char *operators_cigar_ensembl   = "MID" ;
static const char *operators_gffv3_gap       = "MID" ;
static const char *operators_cigar_bam       = "MIDN" ;




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
ZMAP_ENUM_TO_SHORT_TEXT_FUNC(zMapFeatureAlignBoundary2ShortText, AlignBlockBoundaryType,
                             ZMAP_ALIGN_BLOCK_BOUNDARY_LIST) ;


// This should be used everywhere but I expect is not...in theory there should only be an align
// array IF it's len is greater than zero !
bool zMapFeatureAlignmentHasGaps(ZMapFeature feature)
{
  bool result = false ;

  if (feature->feature.homol.align && feature->feature.homol.align->len > 0)
    result = true ;

  return result ;
}


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
 * Constructs a gaps array from an alignment string such as CIGAR, GAPS etc.,
 * this string _MUST_ represent a single HSP, the function cannot be used for
 * strings that represent gapped alignments such as a VULGAR string, instead
 * use zMapFeatureAlignmentString2ExonsGaps().
 * 
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
  else if (!(result = (alignStrCigarCanon2Homol(canon, ref_strand, match_strand,
                                                ref_start, ref_end,
                                                match_start, match_end,
                                                &align))))
    {
      zMapLogWarning("Cannot convert canonical format to align array: \"%s\"", align_string) ;
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


/* Constructs an exons array and also an array of aligns arrays for each exon
 * from a VULGAR string (no other alignment strings are currently supported).
 * 
 * Returns TRUE on success with the exons array returned in exons_out and the
 * aligns array returned in aligns_out. Both arrays should be free'd with 
 * g_array_free when finished with.
 */
gboolean zMapFeatureAlignmentString2ExonsGaps(ZMapFeatureAlignFormat align_format,
                                              ZMapStrand ref_strand, int ref_start, int ref_end,
                                              ZMapStrand match_strand, int match_start, int match_end,
                                              const char *align_string,
                                              GArray **exons_out, GArray **introns_out, GArray **exon_aligns_out)
{
  gboolean result = FALSE ;
  AlignStrCanonical canon = NULL ;
  GArray *exons = NULL, *introns = NULL, *exon_aligns = NULL ;
  int cds_start = 0, cds_end = 0 ;
  char *err_msg = NULL ;


  // Need validation of the vulgar string here.....
  if (!(canon = alignStrMakeCanonical(align_string, align_format)))
    {
      result = FALSE ;

      zMapLogWarning("Cannot convert alignment string to canonical format: %s", align_string) ;
    }

  if (!(result = alignStrVulgarCanon2Exons(canon,
                                     ref_strand, match_strand,
                                     ref_start, ref_end, match_start, match_end,
                                     &exons, &introns, &exon_aligns,
                                     &cds_start, &cds_end,
                                     &err_msg)))
    {
      zMapLogWarning("Cannot convert align string \"%s\" to exons/introns/aligns, error was: %s",
                     align_string, err_msg) ;

      g_free(err_msg) ;
    }


  /*
   * The canonical object should be destroyed even if
   * the above conversions succeed.
   */
  if (canon)
    alignStrCanonicalDestroy(&canon) ;


  if (result)
    {
      *exons_out = exons ;
      *introns_out = introns ;
      *exon_aligns_out = exon_aligns ;
    }


  return result ;
}


// some debug stuff......for the above.....
void zMapFeatureAlignmentPrintExonsAligns(GArray *exons, GArray *introns, GArray *exon_aligns)
{
  guint i, j ;


  for (i = 0 ; i < exons->len ; i++)
    {
      ZMapSpan exon, intron ;      

      exon = &(g_array_index(exons, ZMapSpanStruct, i)) ;
      zMapDebugPrintf("E(%d) %d -> %d", i, exon->x1, exon->x2) ;
      
      if (i < introns->len)
        {
          intron = &(g_array_index(introns, ZMapSpanStruct, i)) ;
          zMapDebugPrintf("I(%d) %d -> %d", i, intron->x1, intron->x2) ;
        }
    }


  for (i = 0, j = 0 ; i < exon_aligns->len ; i++)
    {
      GArray *local_aligns = (GArray *)(g_array_index(exon_aligns, GArray *, i)) ;

      if (local_aligns->len == 0)
        {
          zMapDebugPrintf("E(%d) ungapped", i) ;
        }
      else
        {
          for (j = 0 ; j < local_aligns->len ; j++)
            {
              ZMapAlignBlock block = &(g_array_index(local_aligns, ZMapAlignBlockStruct, j)) ;

              zMapDebugPrintf("E(%d)/A(%d) (%s, %s) %d, %d -> %d, %d",
                              i, j, 
                              zMapFeatureAlignBoundary2ShortText(block->start_boundary),
                              zMapFeatureAlignBoundary2ShortText(block->end_boundary),
                              block->q1, block->q2,  block->t1, block->t2) ;
            }
        }
    }

  return ;
}







/*
 *            Package routines.
 */

void zmapFeatureAlignmentCopyFeature(ZMapFeature orig_feature, ZMapFeature new_feature)
{
  ZMapAlignBlockStruct align ;

  if (orig_feature->feature.homol.align != NULL && orig_feature->feature.homol.align->len > (guint)0)
    {
      unsigned int i ;

      new_feature->feature.homol.align = g_array_sized_new(FALSE, TRUE,
                                                           sizeof(ZMapAlignBlockStruct),
                                                           orig_feature->feature.homol.align->len) ;

      for (i = 0; i < orig_feature->feature.homol.align->len; i++)
        {
          align = g_array_index(orig_feature->feature.homol.align, ZMapAlignBlockStruct, i) ;

          new_feature->feature.homol.align = g_array_append_val(new_feature->feature.homol.align, align) ;
        }
    }

  return ;
}


void zmapFeatureAlignmentDestroyFeature(ZMapFeature feature)
{
  if (feature->feature.homol.align)
    g_array_free(feature->feature.homol.align, TRUE) ;

  return ;
}


ZMapFeaturePartsList zmapFeatureAlignmentSubPartsGet(ZMapFeature feature, ZMapFeatureSubPartType requested_bounds)
{
  ZMapFeaturePartsList subparts = NULL ;
  GArray *gaps_array ;

  zMapReturnValIfFail((feature->mode == ZMAPSTYLE_MODE_ALIGNMENT), NULL) ;

  zMapReturnValIfFailSafe((feature->feature.homol.align), NULL) ;

  gaps_array = feature->feature.homol.align ;
  subparts = g_new0(ZMapFeaturePartsListStruct, 1) ;

  if (requested_bounds == ZMAPFEATURE_SUBPART_MATCH)
    {
      int i ;

      for (i = 0 ; i < (int)gaps_array->len ; i++)
        {
          ZMapAlignBlock block = NULL ;
          ZMapFeatureSubPart span ;

          block = &(g_array_index(gaps_array, ZMapAlignBlockStruct, i)) ;
          span = zMapFeatureSubPartCreate(ZMAPFEATURE_SUBPART_MATCH, i, block->t1, block->t2) ;
          subparts->parts = g_list_append(subparts->parts, span) ;

          if (i == 0)
            subparts->min_val = block->t1 ;
          else if (i == ((int)gaps_array->len - 1))
            subparts->max_val = block->t2 ;
        }
    }
  else if (requested_bounds == ZMAPFEATURE_SUBPART_GAP)
    {
      int i ;

      for (i = 0 ; i < ((int)gaps_array->len - 1) ; i++)
        {
          ZMapAlignBlock block1 = NULL, block2 = NULL ;
          ZMapFeatureSubPart span ;

          block1 = &(g_array_index(gaps_array, ZMapAlignBlockStruct, i)) ;
          block2 = &(g_array_index(gaps_array, ZMapAlignBlockStruct, (i + 1))) ;
          span = zMapFeatureSubPartCreate(ZMAPFEATURE_SUBPART_GAP, i, (block1->t2 + 1), (block2->t1 - 1)) ;
          subparts->parts = g_list_append(subparts->parts, span) ;

          if (i == 0)
            subparts->min_val = (block1->t2 + 1) ;
          else if (i == ((int)gaps_array->len - 2))
            subparts->max_val = (block2->t1 - 1) ;
        }
    }
  else if (requested_bounds == ZMAPFEATURE_SUBPART_FEATURE)
    {
      subparts = zmapFeatureBasicSubPartsGet(feature, requested_bounds) ;
    }

  return subparts ;
}






/* Do any of boundaries match the start/end of the alignment or any of the
 * gapped blocks (if there are any) within the alignment.
 * Return a list of ZMapFeatureSubPart's of those that do.
 *
 * Note in the code below that we can avoid unnecessary comparisons because
 * both boundaries and the exons in the transcript are sorted into ascending
 * sequence position.
 * Note also that a zero value boundary means "no boundary".
 */
gboolean zmapFeatureAlignmentMatchingBoundaries(ZMapFeature feature,
                                                ZMapFeatureSubPartType part_type, gboolean exact_match, int slop,
                                                ZMapFeaturePartsList boundaries,
                                                ZMapFeaturePartsList *matching_boundaries_out,
                                                ZMapFeaturePartsList *non_matching_boundaries_out)
{
  gboolean result = FALSE ;


  zMapReturnValIfFail(zMapFeatureIsValid((ZMapFeatureAny)feature), FALSE) ;
  zMapReturnValIfFail((part_type == ZMAPFEATURE_SUBPART_MATCH || part_type == ZMAPFEATURE_SUBPART_GAP
                       || part_type == ZMAPFEATURE_SUBPART_FEATURE), FALSE) ;

  if (!(feature->feature.homol.align) && part_type == ZMAPFEATURE_SUBPART_FEATURE)
    {
      result = zmapFeatureMatchingBoundaries(feature, exact_match, slop,
                                             boundaries,
                                             matching_boundaries_out, non_matching_boundaries_out) ;
    }
  else
    {
      int slop ;
      GArray *gaps_array = feature->feature.homol.align ;
      GList *curr ;
      int i ;
      gboolean error = FALSE ;
      ZMapFeaturePartsList matching_boundaries ;

      matching_boundaries = zMapFeaturePartsListCreate((ZMapFeatureFreePartFunc)zMapFeatureBoundaryMatchDestroy) ;

      slop = zMapStyleFilterTolerance(*(feature->style)) ;

      curr = boundaries->parts ;
      i = 0 ;
      while (curr)
        {
          ZMapFeatureSubPart curr_boundary = (ZMapFeatureSubPart)(curr->data) ;

          if ((curr_boundary->start) && feature->x2 < curr_boundary->start)
            {
              /* Feature is before current splice. */
              ;
            }
          else if ((curr_boundary->end) && feature->x1 > curr_boundary->end)
            {
              /* curr splice is beyond feature so stop compares. */
              break ;
            }
          else
            {
              int index = 0 ;

              /* Compare blocks. */
              while (i < (int)gaps_array->len)
                {
                  ZMapAlignBlock block ;

                  block = &(g_array_index(gaps_array, ZMapAlignBlockStruct, i)) ;

                  if ((curr_boundary->start) && block->t2 < curr_boundary->start)
                    {
                      /* Block is before current splice. */
                      ;
                    }
                  else if ((curr_boundary->end) && block->t1 > curr_boundary->end)
                    {
                      /* curr splice is beyond feature so stop compares. */
                      break ;
                    }
                  else
                    {
                      int match_boundary_start = 0, match_boundary_end = 0 ;

                      /* Duh....don't seem to have implemented gaps/parts matching properly,
                       * the gaps align is not done.... */

                      if (zmapFeatureCoordsMatch(slop, TRUE, curr_boundary->start, curr_boundary->end,
                                                 block->t1, block->t2, &match_boundary_start, &match_boundary_end))
                        {
                          ZMapFeatureSubPart match_boundary ;

                          match_boundary = zMapFeatureSubPartCreate(curr_boundary->subpart, index,
                                                                    block->t1, block->t2) ;

                          if (zMapFeaturePartsListAdd(matching_boundaries, (ZMapFeaturePart)match_boundary))
                            {
                              index++ ;
                            }
                          else
                            {
                              error = TRUE ;

                              break ;
                            }
                        }
                    }

                  i++ ;
                }
            }

          curr = g_list_next(curr) ;
        }

      /* It's possible to have a feature sit wholly in a gap (or match if checking gap matches)
       * and hence for there to be no error but no match either, hence the subpart test. */
      if (error || !(matching_boundaries->parts))
        {
          zMapFeaturePartsListDestroy(matching_boundaries) ;
          matching_boundaries = NULL ;
        }
      else
        {
          if (matching_boundaries_out)
            *matching_boundaries_out = matching_boundaries ;

          result = TRUE ;
        }
    }

  return result ;
}





/*
 *            Internal routines.
 */



/*
 * Create a GArray to store elements of type ZMapAlignBlockStruct
 */
static GArray* blockArrayCreate()
{
  GArray* align_block_array = NULL ;
  align_block_array = g_array_sized_new(FALSE, FALSE, sizeof(ZMapAlignBlockStruct), 10) ;
  return align_block_array ;
}


/*
 *
 */
static ZMapAlignBlock blockArrayGetBlock(GArray*const align_block_array, int index)
{
  ZMapAlignBlock block = NULL ;
  if (align_block_array && index>=0)
    {
      block = &g_array_index(align_block_array, ZMapAlignBlockStruct, index) ;
    }
  return block ;
}

static gboolean blockAddBlock(GArray** p_align_block_array, const ZMapAlignBlockStruct * const block)
{
  gboolean result = FALSE ;
  GArray *align_block_array = NULL ;
  if (p_align_block_array && (align_block_array = *p_align_block_array) && block)
    {
      align_block_array = g_array_append_val(align_block_array, *block) ;
      *p_align_block_array = align_block_array ;
      result = TRUE ;
    }
  return result ;
}



/*
 * Convert a format to the appropriate properties flags.
 */
static gboolean alignFormat2Properties(ZMapFeatureAlignFormat align_format,
                                       AlignStrCanonicalProperties properties)
{
  gboolean result = FALSE ;

  zMapReturnValIfFail((align_format != ZMAPALIGN_FORMAT_INVALID) && properties, FALSE) ;

  properties->bDigitsLeft = FALSE ;
  properties->bMayOmit1   = FALSE ;
  properties->bSpaces     = FALSE ;
  properties->bTwoNumbers = FALSE ;

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
        properties->bSpaces     = TRUE ;
        properties->bTwoNumbers = TRUE ;
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
 * 
 *
 * Returns TRUE if the operation succeeded.
 *
 * Note: We may return an empty buffer.
 */
static gboolean parseGetOpData(int i,
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
  size_t j = 0 ;

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
      while ((pStart<pEnd) && (j < buffer_size))
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


/* Accepts a string and determines if it is a valid number from a cigar string.
 * It must have length at least unity, and there are three options:
 * opt == 0       all characters must be digits
 * opt == 1       single space at start, all other characters must be digits
 * opt == 2       single space at end, all other characters must be digits

 * VULGAR.....
 * opt == 3       space, number, space, number, space

 * return value is pointer to the start of the digits
 */
static bool parseIsValidNumber(char *sBuff, size_t iLen, int opt,
                               char **first_num_str_out, char **second_num_str_out)
{
  bool result = FALSE ;
  static const char cSpace = ' ' ;
  char *num_pos = NULL ;
  char *first_num = NULL, *second_num = NULL ;
  gboolean bDigits = TRUE ;
  size_t nDigits = 0 ;

  // um....why are we calling it if we have ilen == 0.....   
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
            {
              first_num = sBuff ;
              result = true ;
            }
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
                {
                  first_num = sBuff+1 ;
                  result = true ;
                }
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
                {
                  first_num = sBuff ;
                  result = true ;
                }
            }
        }
      else if (opt == 3)
        {
          /* space, digits, space, digits, space e.g. " 23 42 ", we could do more checking here.... */
          if (cSpace == *sBuff)
            {
              num_pos = sBuff + 1 ;

              while (*num_pos && *num_pos != ' ' && (bDigits = isdigit(*num_pos)))
                {
                  ++nDigits ;
                  ++num_pos ;
                }

              if (bDigits && nDigits && *num_pos == ' ')
                first_num = sBuff + 1 ;

              if (first_num)
                {
                  sBuff = num_pos ;
                  num_pos++ ;                               // Move over space
                  nDigits = 0 ;

                  while (*num_pos && *num_pos != ' ' && (bDigits = isdigit(*num_pos)))
                    {
                      ++nDigits ;
                      ++num_pos ;
                    }

                  if (bDigits && nDigits && (*num_pos == ' ' || !*num_pos))
                    second_num = sBuff + 1 ;
                }

              if (first_num && second_num)
                result = true ;
            }
        }

      if (result)
        {
          *first_num_str_out = first_num ;
          *second_num_str_out = second_num ;
        }
    }


  return result ;
}



/* Inpects the buffer to determine if it contains a valid piece of numerical data
 * for the size of an operator.
 *
 * Arguments are:
 * sBuff                             buffer (may be empty)
 * bMayOmit1                         TRUE if ommitting "1" from operator size is OK
 * bLeft                             TRUE if the number was to the left of the operator
 * bSpaces                           TRUE if the op/number pairs are space seperated
 * bTwoNumbers                   TRUE if both query and target lengths are given
 *
 * First perform some tests to see if the data item was valid according to criteria
 * described below, then parse the numerical value and return it.
 *
 * REWRITING THIS TO RETURN A BOOLEAN AND MAYBE A MATCH AND A REFERENCE SEQUENCE LENGTH
 *
 * NOTE, if the function fails then first_num_out and second_num_out are not altered.
 */
static bool parseIsValidOpData(char *sBuff,
                               gboolean bMayOmit1,
                               gboolean bLeft,
                               gboolean bSpaces,
                               gboolean bEnd,
                               gboolean bFirst,
                               gboolean bTwoNumbers,
                               int *first_num_out, int *second_num_out)
{
  bool result = false ;
  bool found_nums = false ;
  static const char cSpace = ' ' ;
  char *first_num_str = NULL, *second_num_str = NULL ;
  int first_num = 0, second_num = 0 ;
  int num_opt = 0 ;
  size_t iLen ;

  iLen = strlen(sBuff) ;

  /* Tests:
   *
   * (1) If iLen of the string is zero, and omit1 is true,
   *     then reference_length = 1
   * (2) If iLen is 1 and we only have a space, and bSpace is true
   *     and bOmit1 is true then we have reference_length = 1,
   * (3) If iLen is 1 and we have a digit and bSpace is false
         then parse for reference_length
   * (4) If iLen is > 1 and bSpaces is true then we must have
   *     one space at the start or end according to bLeft
   * (5) If iLen is > 1 and bSpaces is false, then there must be
   *     no spaces,
   *  then parse for number
   * (6) If iLen is > 1 and bSpaces is true and bTwoNumbers is true
   *     then we must have one space at the start, a number, a second space,
   *     a second number. We return the FIRST number.
   */

  if (iLen == 0)
    {
      if (!bSpaces)
        {
          if (bMayOmit1)
            {
              first_num = 1 ;
              found_nums = true ;
            }
        }
      else
        {
          if (bMayOmit1 && bEnd)
            {
              if (bLeft && bFirst)
                {
                  first_num = 1 ;
                  found_nums = true ;
                }
              else if (!bLeft && !bFirst)
                {
                  first_num = 1 ;
                  found_nums = true ;
                }
            }
        }
    }
  else if (iLen == 1)
    {
      if (sBuff[0] == cSpace)
        {
          if (bSpaces && bMayOmit1)
            {
              first_num = 1 ;
              found_nums = true ;
            }
        }
      else if (isdigit(sBuff[0]))
        {
          if (!bSpaces)
            {
              first_num = atoi(sBuff) ;
              found_nums = true ;
            }
          else
            {
              if (bEnd)
                {
                  if ((bLeft && bFirst) || (!bLeft && !bFirst))
                    {
                      first_num = atoi(sBuff) ;
                      found_nums = true ;
                    }
                 }
            }
        }
    }
  else
    {
      if (bSpaces)
        {
          if (!bTwoNumbers)
            {
              if (bLeft)
                {
                  /* there must be one space at the start and all other characters must be digits */
                  if (bEnd && bFirst)
                    num_opt = 0 ;
                  else
                    num_opt = 1 ;

                  found_nums = parseIsValidNumber(sBuff, iLen, num_opt, &first_num_str, &second_num_str) ;
                }
              else
                {
                  /* there must be one space at the end and all other characters must be digits */
                  if (bEnd && !bFirst)
                    num_opt = 0 ;
                  else
                    num_opt = 2 ;

                  found_nums = parseIsValidNumber(sBuff, iLen, num_opt, &first_num_str, &second_num_str) ;
                }
            }
          else
            {
              // Currently we only do VULGAR and this is !bLeft so we don't test for that option.   
              num_opt = 3 ;

              found_nums = parseIsValidNumber(sBuff, iLen, num_opt, &first_num_str, &second_num_str) ;
            }
        }
      else
        {
          /* all characters must be digits */
          num_opt = 0 ;
          found_nums = parseIsValidNumber(sBuff, iLen, num_opt, &first_num_str, &second_num_str) ;
        }

      if (found_nums)
        {
          first_num = atoi(first_num_str) ;
          if (second_num_str)
            second_num = atoi(second_num_str) ;
        }
    }

  if (found_nums)
    {
      *first_num_out = first_num ;
      *second_num_out = second_num ;

      result = true ;
    }

  return result ;
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


// THESE HEADER COMMENTS SEEM COMPLETELY OUT OF DATE ??   
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
static gboolean parseCigarGeneral(const char * const str,
                                    AlignStrCanonical canon,
                                    AlignStrCanonicalProperties properties,
                                    GError **error)
{
  gboolean result = FALSE,
    bEnd = FALSE,
    bFirst = FALSE ;
  size_t iLength = 0,
    num_buffer_size = NUM_DIGITS_LIMIT,
    operator_buffer_size = NUM_OPERATOR_START,
    iOperators = 0 ;
  const char *sTarget = NULL ;
  size_t i = 0 ;
  char * sBuff = NULL ;
  char const ** pArray = NULL ;
  char cStart = '\0',
    cEnd = '\0' ;
  gboolean bDigitsLeft = FALSE, bMayOmit1 = FALSE, bSpaces = FALSE, bTwoNumbers = FALSE ;
  AlignStrOp pAlignStrOp = NULL ;

  zMapReturnValIfFail(str && *str &&
                      canon && canon->align && (canon->num_operators == 0)
                      && error && properties, result) ;

  bDigitsLeft   = properties->bDigitsLeft ;
  bMayOmit1     = properties->bMayOmit1 ;
  bSpaces       = properties->bSpaces ;
  bTwoNumbers = properties->bTwoNumbers ;
  
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
      *error = g_error_new(g_quark_from_string(ZMAP_ALIGNSTRING_PARSE_ERROR_DOMAIN), 0,
                           "target string = '%s', has non-operator or non-digit character at start", str) ;
      result = FALSE ;
    }
  if (!isalnum(cEnd) && result)
    {
      *error = g_error_new(g_quark_from_string(ZMAP_ALIGNSTRING_PARSE_ERROR_DOMAIN), 0,
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
          *error = g_error_new(g_quark_from_string(ZMAP_ALIGNSTRING_PARSE_ERROR_DOMAIN), 0,
                               "target string = '%s', has at non-operator character at end", str) ;
          result = FALSE ;
        }
    }
  else if (!bDigitsLeft && result)
    {
      if (!isalpha(cStart))
        {
          *error = g_error_new(g_quark_from_string(ZMAP_ALIGNSTRING_PARSE_ERROR_DOMAIN), 0,
                               "target string = '%s', has non-operator character at start", str) ;
          result = FALSE ;
        }
    }

  /*
   * Firstly find the operators and their positions.
   */
  if (result)
    {
      sBuff = (char*)g_new0(char, num_buffer_size) ;
      pArray = parse_allocate_operator_buffer(operator_buffer_size) ;


      if (bTwoNumbers)
        {
          enum ParseState {OP, FIRST_NUM, SECOND_NUM} ;
          ParseState state ;

          // operators are VULGAR format: 'op num num'
          state = OP ;
          while (*sTarget)
            {
              switch (state)
                {
                case OP:
                  {
                    if (isalnum(*sTarget))
                      {
                        pArray[iOperators] = sTarget ;
                        iOperators++ ;

                        if (iOperators == operator_buffer_size)
                          parse_resize_operator_buffer(&pArray, &operator_buffer_size) ;
                      }

                    state = FIRST_NUM ;

                    break ;
                  }
                case FIRST_NUM:
                  {
                    if (isdigit(*sTarget))
                      {
                        while (isdigit(*sTarget))
                          ++sTarget ;

                        state = SECOND_NUM ;
                      }
                    break ;
                  }
                case SECOND_NUM:
                  {
                    if (isdigit(*sTarget))
                      {
                        while (isdigit(*sTarget))
                          ++sTarget ;

                        state = OP ;
                      }
                    break ;
                  }
                }

              if (*sTarget)
                ++sTarget ;
            }
        }
      else
        {
          /*
           * Operators are defined by one character only each. This step finds their
           * positions in the input string.
           */
          while (*sTarget)
            {
              if (isalpha(*sTarget))
                {
                  pArray[iOperators] = sTarget ;
                  iOperators++ ;

                  if (iOperators == operator_buffer_size)
                    parse_resize_operator_buffer(&pArray, &operator_buffer_size) ;
                }

              if (*sTarget)
                ++sTarget ;
            }
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
          oAlignStrOp.first_length = oAlignStrOp.second_length = 0 ;

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
          int first_num = 0, second_num = 0 ;
          

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
          parseGetOpData(i, sBuff, num_buffer_size, pArray, iOperators, str, bDigitsLeft ) ;

          if ((result = parseIsValidOpData(sBuff, bMayOmit1, bDigitsLeft, bSpaces,
                                           bEnd, bFirst, bTwoNumbers, &first_num, &second_num)))
            {
              pAlignStrOp->first_length = first_num ;
              pAlignStrOp->second_length = second_num ;
            }
          
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
          if (!isalpha(pAlignStrOp->op) && (!pAlignStrOp->first_length && !pAlignStrOp->second_length))
            {
              *error = g_error_new(g_quark_from_string(ZMAP_ALIGNSTRING_PARSE_ERROR_DOMAIN), 0,
                        "target string = '%s', op_index = %i", str, (int)i) ;
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
 * (b) We must always have an odd number of operators
 * (c) Check that the first and last operators are M for all cases of ZMapFeatureAlignFormat.
 *
 */
static bool parse_canon_valid(AlignStrCanonical canon, GError **error)
{
  bool result = true ;
  char cOp = '\0' ;
  static const char cM = 'M' ;
  const char *err_str ;

  /*
   * Check that we have at least one operator.
   */
  if (canon->num_operators <= 0)
    {
      err_str = "Alignment string has no operators" ;
      result = false ;
    }

  /*
   * Check that we have an odd number of operators.
   */
  if (result)
    {
      if ((canon->num_operators % 2) == 0)
        {
          err_str = ALIGN_EVEN_OPERATORS ;
          result = false ;
        }
    }

  /*
   * Check first and last operators are 'M'
   */
  if (result)
    {
      if (canon->num_operators > 0)
        {
          cOp = alignStrCanonicalGetOperatorChar(canon, 0) ;
          if (cOp != cM)
            {
              err_str = "Alignment string first operator is not M" ;
              result = false ;
            }

          if (result && canon->num_operators > 1)
            {
              cOp = alignStrCanonicalGetOperatorChar(canon, canon->num_operators-1) ;
              if (cOp != cM)
                {
                  err_str = "Alignment string last operator is not M" ;
                  result = false ;
                }
            }
        }
    }

  if (!result)
    g_set_error(error, g_quark_from_string(ZMAP_ALIGNSTRING_PARSE_ERROR_DOMAIN), 0, "%s", err_str) ;

  return result ;
}

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

  if ((int)gaps->len > 1)
    {
      perfect_align = TRUE ;
      last_align = &g_array_index(gaps, ZMapAlignBlockStruct, 0) ;

      for (i = 1 ; i < (int)gaps->len ; i++)
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
          if ((curr_start - prev_end - 1) <= (int)align_error)
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

  if (canon && canon->align && i >= 0 && i < canon->num_operators)
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
static AlignStrCanonical alignStrMakeCanonical(const char *match_str, ZMapFeatureAlignFormat align_format)
{
  AlignStrCanonical canon = NULL ;
  gboolean result = FALSE ;

  zMapReturnValIfFailSafe(match_str && *match_str, canon) ;

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
  if (result)
    {
      GError *error = NULL ;

      if (!parse_canon_valid(canon, &error))
        {
          // log all errors.   
          zMapLogWarning("Error processing alignment string '%s': %s", match_str, error->message) ;

          /* gb10: We get a lot of cigar strings from ensembl that return even numbers of
           * operators, either because there are two M operators next to each other (sounds like
           * an error but is something we can understand) or there is a D and an I next to each
           * other (in theory not valid as the CIGAR string should terminate at a double gap).
           *
           * Therefore we only return NULL if the error is _NOT_ this kind of error. */
          if (g_ascii_strcasecmp(error->message, ALIGN_EVEN_OPERATORS) != 0)
            {
              alignStrCanonicalDestroy(&canon) ;
              canon = NULL ;
            }

          g_error_free(error) ;
          error = NULL ;
        }
    }

  return canon ;
}


// WE SHOULD HAVE A SEPARATE ROUTINE FOR HANDLING BAM CIGAR AS IT'S SO DIFFERENT FROM EXONERATE CIGAR

/*
 * Convert the canonical "string" to a zmap align array, note that we do this blindly
 * assuming that everything is ok because we have verified the string....
 *
 * Note that this routine does not support vulgar.
 *
 *
 */
static gboolean alignStrCigarCanon2Homol(AlignStrCanonical canon,
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
  local_map = blockArrayCreate() ;

  if (ref_strand == ZMAPSTRAND_FORWARD)
    curr_ref = p_start ;
  else
    curr_ref = p_end ;

  if (match_strand == ZMAPSTRAND_FORWARD)
    curr_match = c_start ;
  else
    curr_match = c_end ;


  for (i = 0, j = 0 ; i < (int)align->len ; ++i)
    {
      /* If you alter this code be sure to notice the += and -= which alter curr_ref and curr_match. */
      int curr_length = 0 ;
      ZMapAlignBlockStruct gap = {0} ;

      op = alignStrCanonicalGetOperator(canon, i) ;

      if (op)
        {

          // OK AT THIS POINT WE NEED TO KNOW IF THE ALIGN STRING IS ENSEMBL OR EXONERATE
          // OTHERWISE WE WON'T KNOW IF WE SHOULD MULTIPLY BY 3 FOR PROTEIN MATCHES....   

          curr_length = op->first_length ;
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

                  blockAddBlock(&local_map, &gap) ;
                  ++j ;    /* increment for next gap element. */

                  boundary_type = ALIGN_BLOCK_BOUNDARY_MATCH ;
                  break ;
                }
              default:
                {
                  zMapLogWarning("Unrecognized operator '%c' in align string\n", op->op) ;
                  zMapWarnIfReached() ;

                  break ;
                }
            } /* switch (op->op) */

          if (prev_gap)
            prev_gap->end_boundary = boundary_type ;

          if (op->op == 'M')
            prev_gap = blockArrayGetBlock(local_map, local_map->len-1) ;
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




/* This routine converts a canonical alignment array derived from a VULGAR
 * string into:
 *
 *       - an exon array
 *       - an intron array
 *       - an array of arrays of internal aligns for each exon
 *       - the cds start/end or zeros if there is no cds.
 *
 * We do the validation of the ops as we go along because otherwise we will just
 * have to recreate the parsing logic in two places: once in a validation routine
 * and once here. This is because we always need to know the order of ops,
 * we can't just blindly process whatever comes along as we could with a CIGAR string.
 *
 * Valid VULGAR operations are:    M C G N 5 3 I S F 
 *
 *      Valid parse states are:    START, IN_EXON, IN_INTRON, END
 *
 * NOTES:
 *
 * - I don't know how the 'C' operator works so I'm assuming it's just like an 'M'
 *   except that it's the coding section of the transcript.
 *
 * - currently the whole transcript is assumed to be coding....
 *
 *
 */
static gboolean alignStrVulgarCanon2Exons(AlignStrCanonical canon,
                                          ZMapStrand ref_strand, ZMapStrand match_strand,
                                          int p_start, int p_end, int c_start, int c_end,
                                          GArray **exons_out, GArray **introns_out, GArray **exon_aligns_out,
                                          int *cds_start_out, int *cds_end_out, 
                                          char **err_string)
{
  gboolean result = TRUE ;
  int curr_ref, curr_match ;
  int i ;
  GArray *exons, *introns, *aligns ;
  GArray *canon_align ;
  AlignBlockBoundaryType prev_start_boundary, prev_end_boundary ;
  AlignStrOpStruct *op = NULL ;
  VulgarParseState state ;
  char curr_op, prev_op ;
  int exon_index, intron_index ;
  ZMapSpan curr_exon, curr_intron ;
  int cds_start = 0, cds_end = 0 ;
  bool prev_spliced_exon ;

  canon_align = canon->align ;

  if (ref_strand == ZMAPSTRAND_FORWARD)
    curr_ref = p_start ;
  else
    curr_ref = p_end ;

  if (match_strand == ZMAPSTRAND_FORWARD)
    curr_match = c_start ;
  else
    curr_match = c_end ;

  exons = zMapFeatureTranscriptCreateSpanArray() ;
  introns = zMapFeatureTranscriptCreateSpanArray() ;
  aligns = g_array_new(FALSE, TRUE, sizeof(GArray *)) ;

  // loop over operators and contruct exons, introns and align arrays.
  //   
  for (i = 0, state = VulgarParseState::START, prev_op = VULGAR_NO_OP, curr_op = VULGAR_NO_OP,
         prev_start_boundary = ALIGN_BLOCK_BOUNDARY_EDGE, prev_end_boundary = ALIGN_BLOCK_BOUNDARY_EDGE,
         exon_index = 0, intron_index = 0,
         prev_spliced_exon = FALSE ;
       (result && i < (int)(canon_align->len)) ;
       ++i)
    {
      /* If you alter this code be sure to notice the += and -= which alter curr_ref and curr_match. */
      int match_length = 0, ref_length = 0 ;
      bool get_next_op ;

      get_next_op = false ;

      if ((op = alignStrCanonicalGetOperator(canon, i)))
        {
          curr_op = op->op ;
          match_length = op->first_length ;
          ref_length = op->second_length ;
        }
      else
        {
          *err_string = g_strdup("parsing failed because the cannonical ops array contained an element with no operator") ;
          result = FALSE ;
        }


      // Loop over states until a new operator is required.   
      do
        {
          switch (state)
            {
            case VulgarParseState::START:
              {
                switch (curr_op)
                  {
                  case 'M':    /* Match. */
                    {
                      state = VulgarParseState::IN_EXON ;

                      break ;
                    }

                  default:
                    {
                      *err_string = g_strdup_printf(BAD_OP_FORMAT,
                                                    ZMAP_MAKESTRING(VulgarParseState::START), curr_op) ;
                      result = FALSE ;

                      break ;
                    }
                  }
                  
                break ;
              }

            case VulgarParseState::IN_EXON:
              {
                switch (curr_op)
                  {
                  case 'M':    /* Match. */
                    {
                      switch (prev_op)
                        {
                        case VULGAR_NO_OP:
                        case '3':
                        case 'N':
                          {
                            // Need to start a new exon and a new align.
                            ZMapSpanStruct exon ;
                            GArray *local_map ;

                            calcCoords(ref_strand, curr_ref, ref_length, &(exon.x1), &(exon.x2)) ;
                            g_array_append_val(exons, exon) ;

                            if (cds_start == 0)
                              cds_start = exon.x1 ;
                            cds_end = exon.x2 ;

                            local_map = blockArrayCreate() ;
                            g_array_append_val(aligns, local_map) ;

                            if (prev_op == '3')
                              prev_start_boundary = ALIGN_BLOCK_BOUNDARY_INTRON ;
                            else
                              prev_start_boundary = ALIGN_BLOCK_BOUNDARY_EDGE ;
                            prev_end_boundary = ALIGN_BLOCK_BOUNDARY_EDGE ;

                            addNewAlign(local_map, ref_strand, curr_ref, ref_length,
                                        match_strand, curr_match, match_length,
                                        prev_start_boundary, prev_end_boundary) ;

                            break ;
                          }
                        case 'S':
                          {
                            // Exon already started with split codon so extend it and the align array.   
                            GArray *local_map ;
                            ZMapAlignBlock sub_align ;

                            curr_exon = &(g_array_index(exons, ZMapSpanStruct, (exons->len - 1))) ;
                            extendCoords(ref_strand, curr_ref, ref_length, &(curr_exon->x1), &(curr_exon->x2)) ;

                            local_map = g_array_index(aligns, GArray *, (aligns->len - 1));
                            sub_align = &g_array_index(local_map, ZMapAlignBlockStruct, (local_map->len - 1));
                            extendAlignCoords(sub_align, ref_strand, curr_ref, ref_length,
                                              match_strand, curr_match, match_length) ;

                            break ;
                          }
                        case 'F':
                          {
                            // Exon was continued with a frame shift so extend it and the align array.   
                            GArray *local_map ;
                            ZMapAlignBlock sub_align ;

                            curr_exon = &(g_array_index(exons, ZMapSpanStruct, (exons->len - 1))) ;
                            extendCoords(ref_strand, curr_ref, ref_length, &(curr_exon->x1), &(curr_exon->x2)) ;

                            local_map = g_array_index(aligns, GArray *, (aligns->len - 1));
                            sub_align = &g_array_index(local_map, ZMapAlignBlockStruct, (local_map->len - 1));
                            extendAlignCoords(sub_align, ref_strand, curr_ref, ref_length,
                                              match_strand, curr_match, match_length) ;

                            break ;
                          }
                        case 'G':
                          {
                            // Previous was a gap so continue exon but start a new align array.
                            GArray *local_map ;

                            curr_exon = &(g_array_index(exons, ZMapSpanStruct, (exons->len - 1))) ;
                            extendCoords(ref_strand, curr_ref, ref_length, &(curr_exon->x1), &(curr_exon->x2)) ;

                            local_map = g_array_index(aligns, GArray *, (aligns->len - 1)) ;

                            prev_start_boundary = prev_end_boundary ;
                            prev_end_boundary = ALIGN_BLOCK_BOUNDARY_EDGE ;

                            addNewAlign(local_map, ref_strand, curr_ref, ref_length,
                                        match_strand, curr_match, match_length,
                                        prev_start_boundary, prev_end_boundary) ;

                            break ;
                          }
                        default:
                          {
                            *err_string = g_strdup_printf(BAD_TRANSITION_FORMAT,
                                                         ZMAP_MAKESTRING(VulgarParseState::IN_EXON),
                                                         prev_op, curr_op) ;
                            result = FALSE ;

                            break ;
                          }
                        }

                      if (result)
                        {
                          incrementCoords(ref_strand, &curr_ref, ref_length, match_strand, &curr_match, match_length) ;

                          prev_op = curr_op ;
                          get_next_op = true ;
                        }

                      break ;
                    }
                    
                  case 'G':
                    {
                      // Gap in reference or match sequence within an exon.   
                      switch (prev_op)
                        {
                        case 'M':
                          {
                            GArray *local_map ;
                            ZMapAlignBlock sub_align ;

                            if (op->first_length == 0)                  // 'G 0 nn' Insertion in ref.
                              {
                                curr_exon = &(g_array_index(exons, ZMapSpanStruct, (exons->len - 1))) ;
                                extendCoords(ref_strand, curr_ref, ref_length, &(curr_exon->x1), &(curr_exon->x2)) ;

                                prev_end_boundary = ALIGN_BLOCK_BOUNDARY_MATCH ;
                              }
                            else                                        // 'G nn 0' Deletion in ref.
                              {
                                prev_end_boundary = ALIGN_BLOCK_BOUNDARY_DELETION ;
                              }

                            // Must change boundary type in gaps....   
                            local_map = g_array_index(aligns, GArray *, (aligns->len - 1));
                            sub_align = &g_array_index(local_map, ZMapAlignBlockStruct, (local_map->len - 1));
                            setAlignBoundaries(sub_align, prev_start_boundary, prev_end_boundary) ;

                            break ;
                          }
                        default:
                          {
                            *err_string = g_strdup_printf(BAD_TRANSITION_FORMAT,
                                                          ZMAP_MAKESTRING(VulgarParseState::IN_EXON),
                                                          prev_op, curr_op) ;
                            result = FALSE ;

                            break ;
                          }
                        }

                      if (result)
                        {
                          incrementCoords(ref_strand, &curr_ref, ref_length, match_strand, &curr_match, match_length) ;

                          prev_op = curr_op ;
                          get_next_op = true ;
                        }
                    
                      break ;
                    }

                  case 'F':
                    {
                      // Frame shift, extend ref, keep match the same...I think ?
                      // Maybe gaps array shouldn't be extended but added to...?? though that's a
                      // little tricky on the semantics front....
                      switch (prev_op)
                        {
                        case 'M':
                          {
                            GArray *local_map ;
                            ZMapAlignBlock sub_align ;

                            curr_exon = &(g_array_index(exons, ZMapSpanStruct, (exons->len - 1))) ;
                            extendCoords(ref_strand, curr_ref, ref_length, &(curr_exon->x1), &(curr_exon->x2)) ;

                            local_map = g_array_index(aligns, GArray *, (aligns->len - 1));
                            sub_align = &g_array_index(local_map, ZMapAlignBlockStruct, (local_map->len - 1));
                            extendAlignCoords(sub_align, ref_strand, curr_ref, ref_length,
                                              match_strand, curr_match, match_length) ;

                            break ;
                          }
                        default:
                          {
                            *err_string = g_strdup_printf(BAD_TRANSITION_FORMAT,
                                                          ZMAP_MAKESTRING(VulgarParseState::IN_EXON),
                                                          prev_op, curr_op) ;
                            result = FALSE ;

                            break ;
                          }
                        }

                      if (result)
                        {
                          incrementCoords(ref_strand, &curr_ref, ref_length, match_strand, &curr_match, match_length) ;

                          prev_op = curr_op ;
                          get_next_op = true ;
                        }
                    
                      break ;
                    }

                  case 'S':
                    {
                      // Split codon, either at 5' or 3' end of exon depending on previous op.
                      switch (prev_op)
                        {
                        case 'M':
                          {
                            // split codon, 3' end of exon, extend exon and extend align.
                            GArray *local_map ;
                            ZMapAlignBlock sub_align ;

                            prev_spliced_exon = true ;

                            curr_exon = &(g_array_index(exons, ZMapSpanStruct, (exons->len - 1))) ;
                            extendCoords(ref_strand, curr_ref, ref_length, &(curr_exon->x1), &(curr_exon->x2)) ;

                            local_map = g_array_index(aligns, GArray *, (aligns->len - 1));
                            sub_align = &g_array_index(local_map, ZMapAlignBlockStruct, (local_map->len - 1));
                            extendAlignCoords(sub_align, ref_strand, curr_ref, ref_length,
                                              match_strand, curr_match, match_length) ;

                            break ;
                          }
                        case '3':
                          {
                            // split codon at 5' end of exon - need to start a new exon and a new align.
                            ZMapSpanStruct exon ;
                            GArray *local_map ;

                            if (prev_spliced_exon)
                              {
                                prev_spliced_exon = FALSE ;

                                calcCoords(ref_strand, curr_ref, ref_length, &(exon.x1), &(exon.x2)) ;
                                g_array_append_val(exons, exon) ;

                                local_map = blockArrayCreate() ;
                                g_array_append_val(aligns, local_map) ;

                                prev_start_boundary = ALIGN_BLOCK_BOUNDARY_INTRON ;
                                prev_end_boundary = ALIGN_BLOCK_BOUNDARY_EDGE ;

                                addNewAlign(local_map, ref_strand, curr_ref, ref_length,
                                            match_strand, curr_match, match_length,
                                            prev_start_boundary, prev_end_boundary) ;
                              }
                            else
                              {
                                *err_string = g_strdup("parsing failed because encountered an 'S' (split codon)"
                                                       " operator at the start of an exon"
                                                       " but previous exon did not end with an 'S' operator") ;
                                result = FALSE ;
                              }

                            break ;
                          }
                        default:
                          {
                            *err_string = g_strdup_printf(BAD_TRANSITION_FORMAT,
                                                          ZMAP_MAKESTRING(VulgarParseState::IN_EXON),
                                                          prev_op, curr_op) ;
                            result = FALSE ;

                            break ;
                          }
                        }

                      if (result)
                        {
                          incrementCoords(ref_strand, &curr_ref, ref_length, match_strand, &curr_match, match_length) ;

                          prev_op = curr_op ;
                          get_next_op = true ;
                        }

                      break ;
                    }

                  case '5':
                    {
                      // Start of intron and therefore end of exon.
                      switch (prev_op)
                        {
                        case 'M':
                        case 'S':
                        case 'N':
                          {
                            // If there is only one match block then it's ungapped and we remove
                            // the existing gap because we use local_map->len == 0 to mean ungapped.
                            GArray *local_map ;

                            local_map = g_array_index(aligns, GArray *, (aligns->len - 1)) ;

                            if (local_map->len == 1)
                              {
                                local_map = g_array_remove_index(local_map, 0) ;
                              }
                            else
                              {
                                ZMapAlignBlock sub_align ;

                                sub_align = &g_array_index(local_map, ZMapAlignBlockStruct, (local_map->len - 1));

                                prev_end_boundary = ALIGN_BLOCK_BOUNDARY_INTRON ;
                                setAlignBoundaries(sub_align, sub_align->start_boundary, prev_end_boundary) ;
                              }

                            state = VulgarParseState::IN_INTRON ;

                            break ;
                          }
                        default:
                          {
                            *err_string = g_strdup_printf(BAD_TRANSITION_FORMAT,
                                                          ZMAP_MAKESTRING(VulgarParseState::IN_EXON),
                                                          prev_op, curr_op) ;
                            result = FALSE ;

                            break ;
                          }
                        }

                      break ;
                    }
                        
                  default:
                    {
                      *err_string = g_strdup_printf(BAD_OP_FORMAT,
                                                    ZMAP_MAKESTRING(VulgarParseState::IN_EXON), curr_op) ;
                      result = FALSE ;
                          
                      break ;
                    }
                  }

                break ;                
              }
              

            case VulgarParseState::IN_INTRON:
              {
                switch (curr_op)
                  {
                  case '5':
                    {
                      // Starting new intron
                      switch (prev_op)
                        {
                        case 'M':
                        case 'S':
                          {
                            ZMapSpanStruct intron ;

                            calcCoords(ref_strand, curr_ref, ref_length, &(intron.x1), &(intron.x2)) ;
                            g_array_append_val(introns, intron) ;

                            break ;
                          }
                        default:
                          {
                            *err_string = g_strdup_printf(BAD_TRANSITION_FORMAT,
                                                          ZMAP_MAKESTRING(VulgarParseState::IN_INTRON),
                                                          prev_op, curr_op) ;
                            result = FALSE ;

                            break ;
                          }
                        }

                      if (result)
                        {
                          incrementCoords(ref_strand, &curr_ref, ref_length, match_strand, &curr_match, match_length) ;

                          prev_op = curr_op ;
                          get_next_op = true ;
                        }

                      break ;                
                    }

                  case 'I':
                    {
                      // Continuing an intron
                      switch (prev_op)
                        {
                        case '5':
                          {
                            curr_intron = &(g_array_index(introns, ZMapSpanStruct, (introns->len - 1))) ;
                            extendCoords(ref_strand, curr_ref, ref_length, &(curr_intron->x1), &(curr_intron->x2)) ;

                            break ;
                          }
                        default:
                          {
                            *err_string = g_strdup_printf(BAD_TRANSITION_FORMAT,
                                                          ZMAP_MAKESTRING(VulgarParseState::IN_INTRON),
                                                          prev_op, curr_op) ;
                            result = FALSE ;

                            break ;
                          }
                        }

                      if (result)
                        {
                          incrementCoords(ref_strand, &curr_ref, ref_length, match_strand, &curr_match, match_length) ;

                          prev_op = curr_op ;
                          get_next_op = true ;
                        }

                      break ;
                    }

                  case '3':
                    {
                      // Ending an intron
                      switch (prev_op)
                        {
                        case 'I':
                          {
                            curr_intron = &(g_array_index(introns, ZMapSpanStruct, (introns->len - 1))) ;
                            extendCoords(ref_strand, curr_ref, ref_length, &(curr_intron->x1), &(curr_intron->x2)) ;

                            break ;
                          }
                        default:
                          {
                            *err_string = g_strdup_printf(BAD_TRANSITION_FORMAT,
                                                          ZMAP_MAKESTRING(VulgarParseState::IN_INTRON),
                                                          prev_op, curr_op) ;
                            result = FALSE ;

                            break ;
                          }
                        }

                      if (result)
                        {
                          incrementCoords(ref_strand, &curr_ref, ref_length, match_strand, &curr_match, match_length) ;

                          prev_op = curr_op ;
                          get_next_op = true ;
                        }
   
                      break ;                
                    }

                  case 'N':
                    {
                      // A non-equivalenced region, not really an intron but should
                      // happen in the same place, should be followed by a match. We will need to handle this differently somehow...
                      switch (prev_op)
                        {
                        case 'M':
                          {
                            ZMapSpanStruct intron ;

                            calcCoords(ref_strand, curr_ref, ref_length, &(intron.x1), &(intron.x2)) ;
                            g_array_append_val(introns, intron) ;

                            break ;
                          }
                        default:
                          {
                            *err_string = g_strdup_printf(BAD_TRANSITION_FORMAT,
                                                          ZMAP_MAKESTRING(VulgarParseState::IN_INTRON),
                                                          prev_op, curr_op) ;
                            result = FALSE ;

                            break ;
                          }
                        }

                      if (result)
                        {
                          incrementCoords(ref_strand, &curr_ref, ref_length, match_strand, &curr_match, match_length) ;

                          prev_op = curr_op ;
                          get_next_op = true ;
                        }

                      break ;                
                    }
                      
                  case 'M':
                  case 'S':
                    {
                      // Found next exon   
                      state = VulgarParseState::IN_EXON ;

                      break ;
                    }

                  default:
                    {
                      *err_string = g_strdup_printf(BAD_OP_FORMAT,
                                                    ZMAP_MAKESTRING(VulgarParseState::IN_EXON), curr_op) ;
                      result = FALSE ;

                      break ;
                    }
                  }

                break ;
              }
                  
            default:
              {
                *err_string = g_strdup_printf("parsing failed with unknown state: %d", state) ;
                result = FALSE ;

                break ;
              }
                
            } // switch (state)
              
        } while (result && !get_next_op) ;

    } // for (i = 0, j = 0, state = START, prev_op = VULGAR_NO_OP, curr_op = VULGAR_NO_OP ; i < (int)align->len ; ++i)


  // We leave the loop on the last element of the array and then need to check that the last
  // operator was a match otherwise it's an error. Though there is a question about whether we can
  // end on a splice...one for guy....
  if (result)
    {
      switch (prev_op)
        {
        case 'M':    /* Match. */
          {
            GArray *local_map ;

            local_map = g_array_index(aligns, GArray *, (aligns->len - 1)) ;

            if (local_map->len == 1)
              {
                local_map = g_array_remove_index(local_map, 0) ;
              }

            break ;
          }

        default:
          {
            *err_string = g_strdup_printf(BAD_OP_FORMAT,
                                          ZMAP_MAKESTRING(VulgarParseState::IN_EXON), curr_op) ;
            result = FALSE ;

            break ;
          }
        }
    }
  
  



  if (result)
    {
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      // debugging output.
      zMapFeatureAlignmentPrintExonsAligns(exons, introns, aligns) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      *exons_out = exons ;
      *introns_out = introns ;
      *exon_aligns_out = aligns ;

      *cds_start_out = cds_start ;
      *cds_end_out = cds_end ;
    }
  else
    {
      // clear up !!!!
      if (exons)
        g_array_free(exons, TRUE) ;
      if (introns)
        g_array_free(introns, TRUE) ;
      if (aligns)
        {
          freeAligns(aligns) ;

          g_array_free(aligns, TRUE) ;
        }
    }


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
      op.first_length = feature->feature.homol.y2 - feature->feature.homol.y1 + 1 ;

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
          this_block = blockArrayGetBlock(align, i) ;

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
                      op.first_length = this_block->t1-last_block->t2-1 ;
                      if (op.first_length >= 1)
                        alignStrCanonicalAddOperator(canon, &op) ;
                    }
                  else if ((last_block->q2+1 < this_block->q1) && (last_block->t2+1 == this_block->t1))
                    {
                      /* this is a I operator */
                      op.op = cI ;
                      op.first_length = this_block->q1-last_block->q2-1 ;
                      if (op.first_length >= 1)
                        alignStrCanonicalAddOperator(canon, &op) ;
                    }

                }
            }

          /*
           * Create M operator for current block and add to canon.
           */
          AlignStrOpStruct op ;
          op.op = cM ;
          op.first_length = this_block->q2 - this_block->q1 + 1 ;
          if (op.first_length >= 1)
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
        g_strdup_printf("i%c", op->first_length) : g_strdup_printf("%c%i", op->op, op->first_length) ;

      for (i=1; i<canon->num_operators; ++i)
        {
          op = alignStrCanonicalGetOperator(canon, i) ;
          buff = temp_string ;
          if (bSpaces)
            temp_string = bDigitsLeft ?
              g_strdup_printf("%s %i%c", buff, op->first_length, op->op) : g_strdup_printf("%s %c%i", buff, op->op, op->first_length);
          else
            temp_string = bDigitsLeft ?
              g_strdup_printf("%s%i%c", buff, op->first_length, op->op) : g_strdup_printf("%s%c%i", buff, op->op, op->first_length);
          g_free(buff) ;
        }

      *p_string = temp_string ;
      result = TRUE ;
    }

  return result ;
}


/*
 *
 */
static gboolean gffv3Gap2Canon(const char *match_str, AlignStrCanonical canon)
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

  alignFormat2Properties(canon->format, &properties) ;
  if ((result = parseCigarGeneral(match_str, canon, &properties, &error)))
    {
      canon_edited = alignStrCanonicalSubstituteGFFv3Gap(canon) ;
      canon_edited = parse_remove_invalid_operators(canon, operators_gffv3_gap, &num_removed) ;
    }
  if (error)
    {
      g_error_free(error) ;
    }

  return result ;
}

/*
 *
 */
static gboolean exonerateCigar2Canon(const char *match_str, AlignStrCanonical canon)
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

  alignFormat2Properties(canon->format, &properties) ;

  if ((result = parseCigarGeneral(match_str, canon, &properties, &error)))
    {
      canon_edited = alignStrCanonicalSubstituteExonerateCigar(canon) ;
      canon_edited = parse_remove_invalid_operators(canon, operators_cigar_exonerate, &num_removed) ;
    }
  else if (error)
    {
      g_error_free(error) ;
    }

  return result ;
}


/* Not straight forward to implement, wait and see what we need.... */
static gboolean exonerateVulgar2Canon(const char *match_str, AlignStrCanonical canon)
{
  gboolean result = FALSE ;
  AlignStrCanonicalPropertiesStruct properties ;
  GError *error = NULL ;

  zMapReturnValIfFail(canon &&
                      canon->align &&
                      (canon->format==ZMAPALIGN_FORMAT_VULGAR_EXONERATE) &&
                      (canon->num_operators==0), FALSE) ;

  alignFormat2Properties(canon->format, &properties) ;

  if ((result = parseCigarGeneral(match_str, canon, &properties, &error)))
    {
      

      
    }
  else if (error)
    {
      g_error_free(error) ;
    }


  return result ;
}


/*
 * Blindly converts, assumes match_str is a valid ensembl cigar string.
 */
static gboolean ensemblCigar2Canon(const char *match_str, AlignStrCanonical canon)
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

  alignFormat2Properties(canon->format, &properties) ;
  if ((result = parseCigarGeneral(match_str, canon, &properties, &error)))
    {
      canon_edited = alignStrCanonicalSubstituteEnsemblCigar(canon) ;
      canon_edited = parse_remove_invalid_operators(canon, operators_cigar_ensembl, &num_removed) ;
    }
  if (error)
    {
      g_error_free(error) ;
    }

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
static gboolean bamCigar2Canon(const char *match_str, AlignStrCanonical canon)
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

  alignFormat2Properties(canon->format, &properties) ;
  if ((result = parseCigarGeneral(match_str, canon, &properties, &error)))
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


static gboolean alignStrCanonicalSubstituteGFFv3Gap(AlignStrCanonical canon)
{
  gboolean result = FALSE ;
  zMapReturnValIfFail(canon && canon->format == ZMAPALIGN_FORMAT_GAP_GFF3, result) ;
  return result ;
}




/*
 * Test of parsing various kinds of cigar strings...
 */
#ifdef USE_PARSE_TEST_FUNCTION
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
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  bResult = parse_remove_invalid_operators(canon, operators_cigar_bam, &num_removed) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, " 33M13I52M10I1980MII") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  bResult = parse_remove_invalid_operators(canon, operators_cigar_bam, &num_removed) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33 M13I52M10I1980MII") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "3M13I52M10I 1980MII ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M13I52$10I1980MII") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "=33M13I52M10I1980MII") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
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
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3 I13M52I10M1980II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, " M3I13M52I10M1980II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M 3I13M52I10M1980II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3I13M52I10M1980II1 ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3I13M52I10 M1980II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3I13M52&10M1980II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3I13M52I10M1980I*") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
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
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, " 33M 13I 52M 10I 1980M I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M13I 52M 10I 1980M I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M 13I 52M 10I 1980MI I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M 13I 52M 10I 1980M II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M 13I 52M 10I 1980M I I1") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M 13I 52 10I 1980M I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M 13I 52M 10I 1980M I=I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
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
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M33 I13 M52 I10 M1980 I I ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M33 I13 M52 I10 M1980 II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M33 I13 M52I10 M1980 I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M33 II13 M52 I10 M1980 I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M33 I13 M52 I10M1980 I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M33 I13 M52 I10 M1980 I ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
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
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M13I52M10I1980M1I13I ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M13I52M10I1980M1I13I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M13I52M10I1980M1II") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M13I52 M10I1980M1I13I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M13I52M 10I1980M1I13I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, " 33M13I52M10I1980M1I13I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "33M13I52M10I1980M1I13I ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
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
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3I13M52I10M1980I1I18 ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3 I13M52I10M1980I1I18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, " M3I13M52I10M1980I1I18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "MI13M52I10M1980I1I18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3I13M52I10M1980I1I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3I13M52I10M1980II18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3I13M52 I10M1980I1I18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M3I13M52I10M19&80I1I18") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
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
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M 13I 52M 10I 1980M 1I 200I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "3M13I 52M 10I 1980M 1I 200I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "3M 13I 52M 10I 1980M 1I I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "3M 13I 52M 10I 1980M 1I 200I ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "3M 13I M 10I 1980M 1I 200I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, " 3M 13I 52M 10I 1980M 1I 200I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "3M 13I 52M && 10I 1980M 1I 200I") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
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
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M31I13 M52 I10 M1980 I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M31 I13 M52 10 M1980 I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M31 I13 M52 I10 M1980 I3 I32 ") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, " M31 I13 M52 I10 M1980 I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M31 I13 M52 I10 M I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "31 I13 M52 I10 M1980 I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
  alignStrCanonicalDestroy(&canon) ;
  if (error)
    {
      g_error_free(error ) ;
      error = NULL ;
    }

  sprintf(sTest01, "M31 I13 M52 I&10 M1980 I3 I32") ;
  canon = alignStrCanonicalCreate(align_format) ;
  bResult = parseCigarGeneral(sTest01, canon, &properties, &error) ;
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


static void parseTestFunction02()
{
  ZMapFeatureAlignFormat align_format = ZMAPALIGN_FORMAT_VULGAR_EXONERATE ;
  ZMapStrand ref_strand = ZMAPSTRAND_FORWARD ;
  int ref_start = 1, ref_end = 1 ;
  ZMapStrand match_strand = ZMAPSTRAND_FORWARD ;
  int match_start = 0, match_end = 0 ;
  const char *align_string ;
  GArray *exons, *introns, *exon_aligns ;


  const char *vulgar_nucleotide =   "M 10 10 G 0 1 M 20 20 5 0 2 I 0 50 3 0 2 M 10 10" ;

  const char *vulgar_nucleotide_s =   "M 10 10 G 0 1 M 20 20 S 1 1 5 0 2 I 0 50 3 0 2 S 2 2 M 10 10" ;

  const char *vulgar_nucleotide_1 = "M 10 10 G 0 1 M 20 20 5 0 2 I 0 50 3 0 2 M 10 10 5 0 2 I 0 20 3 0 2 M 10 10 5 0 2 I 0 60 3 0 2 M 20 20" ;

  const char *vulgar_protein = "M 12 36 G 1 0 M 31 93 G 0 3 M 31 93 G 0 6 M 17 51" ;


  align_string = (char *)vulgar_nucleotide ;
  ref_start = 1 ;
  ref_end = 95 ;
  match_start = 1 ;
  match_end = 40 ;
    
  zMapFeatureAlignmentString2ExonsGaps(align_format,
                                       ref_strand, ref_start, ref_end,
                                       match_strand, match_start, match_end,
                                       align_string,
                                       &exons, &introns, &exon_aligns) ;


  align_string = (char *)vulgar_protein ;
  ref_start = 1 ;
  ref_end = 282 ;
  match_start = 1 ;
  match_end = 92 ;
    
  zMapFeatureAlignmentString2ExonsGaps(align_format,
                                       ref_strand, ref_start, ref_end,
                                       match_strand, match_start, match_end,
                                       align_string,
                                       &exons, &introns, &exon_aligns) ;

  align_string = (char *)vulgar_nucleotide_s ;
  ref_start = 1 ;
  ref_end = 98 ;
  match_start = 1 ;
  match_end = 43 ;
    
  zMapFeatureAlignmentString2ExonsGaps(align_format,
                                       ref_strand, ref_start, ref_end,
                                       match_strand, match_start, match_end,
                                       align_string,
                                       &exons, &introns, &exon_aligns) ;

  align_string = (char *)vulgar_nucleotide_1 ;
  ref_start = 1 ;
  ref_end = 213 ;
  match_start = 1 ;
  match_end = 70 ;
    
  zMapFeatureAlignmentString2ExonsGaps(align_format,
                                       ref_strand, ref_start, ref_end,
                                       match_strand, match_start, match_end,
                                       align_string,
                                       &exons, &introns, &exon_aligns) ;

  return ;
}

#endif





// Frees the local map array pointed to by each element of the aligns array for vulgar exons.   
static void freeAligns(GArray *aligns)
{
  guint i ;

  for (i = 0 ; i < aligns->len ; i++)
    {
      GArray *local_map_array = (GArray *)g_array_index(aligns, GArray *, i) ;

      g_array_free(local_map_array, TRUE) ;
    }

  return ;
}


// make a new align
static void addNewAlign(GArray *local_map,
                        ZMapStrand ref_strand, int curr_ref, int ref_length,
                        ZMapStrand match_strand, int curr_match, int match_length,
                        AlignBlockBoundaryType start_boundary, AlignBlockBoundaryType end_boundary)
{
  ZMapAlignBlockStruct sub_align = {0} ;

  // Start of a new match.......
  sub_align.t_strand = ref_strand ;
  sub_align.q_strand = match_strand ;
  sub_align.start_boundary = start_boundary ;
  sub_align.end_boundary = end_boundary ;

  calcCoords(ref_strand, curr_ref, ref_length, &(sub_align.t1), &(sub_align.t2)) ;

  // I DON'T UNDERSTAND THIS ASYMMETRY...I GUESS I WILL WHEN I TRY IT
  // OUT....CHECK THIS OUT.....
  sub_align.q1 = curr_match ;
  if (match_strand == ZMAPSTRAND_FORWARD)
    sub_align.q2 = (curr_match + match_length) - 1 ;
  else
    sub_align.q2 = (curr_match - match_length) + 1 ;

  blockAddBlock(&local_map, &sub_align) ;

  return ;
}


#ifdef UNUSED
// gb10: Not sure if this is required. Commenting it out for now to avoid compiler warnings.
static void calcAlignCoords(ZMapAlignBlock sub_align,
                            ZMapStrand ref_strand, int curr_ref, int ref_length,
                            ZMapStrand match_strand, int curr_match, int match_length)
{
  calcCoords(ref_strand, curr_ref, match_length, &(sub_align->t1), &(sub_align->t2)) ;

  sub_align->q1 = curr_match ;
  if (match_strand == ZMAPSTRAND_FORWARD)
    sub_align->q2 = (curr_match + match_length) - 1 ;
  else
    sub_align->q2 = (curr_match - match_length) + 1 ;

  return ;
}
#endif


static void extendAlignCoords(ZMapAlignBlock sub_align,
                              ZMapStrand ref_strand, int curr_ref, int ref_length,
                              ZMapStrand match_strand, int curr_match, int match_length)
{
  extendCoords(ref_strand, curr_ref, match_length, &(sub_align->t1), &(sub_align->t2)) ;

  if (match_strand == ZMAPSTRAND_FORWARD)
    sub_align->q2 = (curr_match + match_length) - 1 ;
  else
    sub_align->q2 = (curr_match - match_length) + 1 ;

  return ;
}

// Set the boundary types for a single align block.
static void setAlignBoundaries(ZMapAlignBlock sub_align,
                               AlignBlockBoundaryType start_boundary, AlignBlockBoundaryType end_boundary)
{
  sub_align->start_boundary = start_boundary ;
  sub_align->end_boundary = end_boundary ;

  return ;
}


// Calculations only done if a valid strand is specified.   
static void incrementCoords(ZMapStrand ref_strand, int *curr_ref_inout, int ref_length,
                            ZMapStrand match_strand, int *curr_match_inout, int match_length)
{
  if (ref_strand)
    {
      if (ref_strand == ZMAPSTRAND_FORWARD)
        {
          *curr_ref_inout += ref_length ;
        }
      else
        {
          *curr_ref_inout -= ref_length ;
        }
    }

  if (ref_strand)
    {
      if (match_strand == ZMAPSTRAND_FORWARD)
        {
          *curr_match_inout += match_length ;
        }
      else
        {
          *curr_match_inout -= match_length ;
        }
    }

  return ;
}

static void calcCoords(ZMapStrand strand, int curr, int length, int *start, int *end)
{
  if (strand == ZMAPSTRAND_FORWARD)
    {
      *start = curr ;
      *end = (curr + length) - 1 ;
    }
  else
    {
      *start = (curr - length) + 1 ;
      *end = curr ;
    }

  return ;
}

static void extendCoords(ZMapStrand strand, int curr, int length, int *start, int *end)
{
  if (strand == ZMAPSTRAND_FORWARD)
    {
      *end = (curr + length) - 1 ;
    }
  else
    {
      *start = (curr - length) + 1 ;
    }

  return ;
}
