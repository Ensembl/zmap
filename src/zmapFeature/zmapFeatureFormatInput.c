/*  File: zmapFeatureFormatInput.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 * Description: Routines to handle converting from Feature to Type and
 *              vice-versa.
 *
 * Exported functions: See ZMap/zmapFeature.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapFeature_P.h>



/* Utility to map strings to enums, needed by lots of the format calls in here and the styles
 * code. The last entry in the table must end with  type_str == NULL .
 *  */
gboolean zmapStr2Enum(ZMapFeatureStr2Enum type_table, char *type_str, int *type_out)
{
  gboolean result = FALSE ;
  ZMapFeatureStr2Enum type ;

  zMapAssert(type_table && type_str && *type_str && type_out) ;

  type = type_table ;
  while (type->type_str)
    {
      if (g_ascii_strcasecmp(type_str, type->type_str) == 0)
	{
	  *type_out = type->type_enum ;
	  result = TRUE ;

	  break;
	}
      else
	type++ ;
    }

  return result ;
}


/* Type must equal something that the code understands. In GFF v1 this is unregulated and
 * could be anything. By default unrecognised terms are not converted.
 *
 *
 * SO_compliant == TRUE   only recognised SO terms will be accepted for feature types.
 * SO_compliant == FALSE  both SO and the earlier more adhoc names will be accepted (source for
 *                        these terms is wormbase GFF dumps).
 *
 * This option is only valid when SO_compliant == FALSE.
 * misc_default == TRUE   unrecognised terms will be returned as "misc_feature" features types.
 * misc_default == FALSE  unrecognised terms will not be converted.
 *
 *
 *  */
gboolean zMapFeatureFormatType(gboolean SO_compliant, gboolean default_to_basic,
                               char *feature_type, ZMapStyleMode *type_out)
{
  gboolean result = FALSE ;
  ZMapStyleMode type = ZMAPSTYLE_MODE_INVALID ;


  /* Is feature_type a SO term, note that I do case-independent compares, hope this is correct. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* I don't know what this is about, how to represent it or anything........... */
  /*  else if (g_ascii_strcasecmp(feature_type, "virtual_sequence") == 0)
      {
      type = ZMAPFEATURE_SEQUENCE ;
      }
      else */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* There are usually many more alignments/exons than anything else in a GFF dump so do them first. */
  if (g_ascii_strcasecmp(feature_type, "nucleotide_match") == 0
//      || g_ascii_strcasecmp(feature_type, "read_pair") == 0		/* is this an abuse of SO?? */
      || g_ascii_strcasecmp(feature_type, "expressed_sequence_match") == 0
      || g_ascii_strcasecmp(feature_type, "EST_match") == 0
      || g_ascii_strcasecmp(feature_type, "cDNA_match") == 0
      || g_ascii_strcasecmp(feature_type, "repeat_region") == 0
      || g_ascii_strcasecmp(feature_type, "inverted_repeat") == 0
      || g_ascii_strcasecmp(feature_type, "tandem_repeat") == 0
      || g_ascii_strcasecmp(feature_type, "translated_nucleotide_match") == 0
      || g_ascii_strcasecmp(feature_type, "protein_match") == 0)
    {
      type = ZMAPSTYLE_MODE_ALIGNMENT ;
    }
  else if (g_ascii_strcasecmp(feature_type, "exon") == 0
	   || g_ascii_strcasecmp(feature_type, "intron") == 0
	   ||g_ascii_strcasecmp(feature_type, "pseudogene") == 0
	   || g_ascii_strcasecmp(feature_type, "transcript") == 0
	   || g_ascii_strcasecmp(feature_type, "protein_coding_primary_transcript") == 0
	   || g_ascii_strcasecmp(feature_type, "CDS") == 0
	   || g_ascii_strcasecmp(feature_type, "mRNA") == 0
	   || g_ascii_strcasecmp(feature_type, "nc_primary_transcript") == 0)
    {
      type = ZMAPSTYLE_MODE_TRANSCRIPT ;
    }
  else if (g_ascii_strcasecmp(feature_type, "reagent") == 0
	   || g_ascii_strcasecmp(feature_type, "assembly_path") == 0
	   || g_ascii_strcasecmp(feature_type, "oligo") == 0
	   || g_ascii_strcasecmp(feature_type, "PCR_product") == 0
	   || g_ascii_strcasecmp(feature_type, "RNAi_reagent") == 0
	   || g_ascii_strcasecmp(feature_type, "clone") == 0
	   || g_ascii_strcasecmp(feature_type, "clone_end") == 0
	   || g_ascii_strcasecmp(feature_type, "clone_end") == 0
	   || g_ascii_strcasecmp(feature_type, "trans_splice_acceptor_site") == 0
	   || g_ascii_strcasecmp(feature_type, "transposable_element_insertion_site") == 0
	   || g_ascii_strcasecmp(feature_type, "deletion") == 0
	   || g_ascii_strcasecmp(feature_type, "region") == 0
	   || g_ascii_strcasecmp(feature_type, "UTR") == 0
	   || g_ascii_strcasecmp(feature_type, "polyA_signal_sequence") == 0
	   || g_ascii_strcasecmp(feature_type, "polyA_site") == 0
	   || g_ascii_strcasecmp(feature_type, "operon") == 0
	   || g_ascii_strcasecmp(feature_type, "experimental_result_region") == 0
	   || g_ascii_strcasecmp(feature_type, "chromosomal_structural_element") == 0
	   || g_ascii_strcasecmp(feature_type, "transposable_element") == 0
	   || g_ascii_strcasecmp(feature_type, "SNP") == 0
	   || g_ascii_strcasecmp(feature_type, "STS") == 0
	   || g_ascii_strcasecmp(feature_type, "sequence_variant") == 0
	   || g_ascii_strcasecmp(feature_type, "substitution") == 0)
    {
      type = ZMAPSTYLE_MODE_BASIC ;
    }


  /* Currently there will be lots of non-SO terms in any GFFv2 dump. */
  if (!SO_compliant)
    {
      if (g_ascii_strcasecmp(feature_type, "transcript") == 0
	  || g_ascii_strcasecmp(feature_type, "protein-coding_primary_transcript") == 0
	  /* N.B. "protein-coding_primary_transcript" is a typo in wormDB currently, should
	   * be all underscores. */

	  /* I'm trying this here because it is the first record for a transcript in acedb
	   * output.... */
	  || g_ascii_strcasecmp(feature_type, "Sequence") == 0

	  || g_ascii_strcasecmp(feature_type, "protein_coding_primary_transcript") == 0
	  || g_ascii_strcasecmp(feature_type, "miRNA_primary_transcript") == 0
	  || g_ascii_strcasecmp(feature_type, "snRNA_primary_transcript") == 0
	  || g_ascii_strcasecmp(feature_type, "snoRNA_primary_transcript") == 0
	  || g_ascii_strcasecmp(feature_type, "tRNA_primary_transcript") == 0
	  || g_ascii_strcasecmp(feature_type, "rRNA_primary_transcript") == 0
	  || g_ascii_strcasecmp(feature_type, "Pseudogene") == 0
	  || g_ascii_strcasecmp(feature_type, "coding_exon") == 0
	  || g_ascii_strcasecmp(feature_type, "exon") == 0
	  || g_ascii_strcasecmp(feature_type, "intron") == 0)
	{
	  type = ZMAPSTYLE_MODE_TRANSCRIPT ;
	}
      else if (g_ascii_strcasecmp(feature_type, "similarity") == 0
	       || g_ascii_strcasecmp(feature_type, "transcription") == 0)
	{
	  type = ZMAPSTYLE_MODE_ALIGNMENT ;
	}
      else if (g_ascii_strcasecmp(feature_type, "Clone") == 0

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	       /* Problem here is that this is used for overall transcript record... */
	       || g_ascii_strcasecmp(feature_type, "Sequence") == 0
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	       || g_ascii_strcasecmp(feature_type, "repeat") == 0

	       || g_ascii_strcasecmp(feature_type, "atg") == 0
	       || g_ascii_strcasecmp(feature_type, "splice3") == 0
	       || g_ascii_strcasecmp(feature_type, "splice5") == 0
	       || g_ascii_strcasecmp(feature_type, "Clone_left_end") == 0
	       || g_ascii_strcasecmp(feature_type, "Clone_right_end") == 0
	       || g_ascii_strcasecmp(feature_type, ZMAP_FIXED_STYLE_LOCUS_NAME) == 0
	       || g_ascii_strcasecmp(feature_type, "SL1_acceptor_site") == 0
	       || g_ascii_strcasecmp(feature_type, "SL2_acceptor_site") == 0
	       || g_ascii_strcasecmp(feature_type, "utr") == 0
	       || g_ascii_strcasecmp(feature_type, "experimental") == 0
	       || g_ascii_strcasecmp(feature_type, "reagent") == 0
	       || g_ascii_strcasecmp(feature_type, "structural") == 0
	       || g_ascii_strcasecmp(feature_type, "contig") == 0
	       || g_ascii_strcasecmp(feature_type, "supercontig") == 0
	       || g_ascii_strcasecmp(feature_type, "misc_feature") == 0
	       || g_ascii_strcasecmp(feature_type, "SNP") == 0
	       || g_ascii_strcasecmp(feature_type, "complex_change_in_nucleotide_sequence") == 0
	       || g_ascii_strcasecmp(feature_type, "trans-splice_acceptor") == 0
	       /* needed to read BAM files formatted as for Blixem */
	       || g_ascii_strcasecmp(feature_type, "read") == 0)
	{
	  type = ZMAPSTYLE_MODE_BASIC ;
	}
      else if (default_to_basic)
	{
	  /* If we allow defaulting of unrecognised features, the default is a "basic" feature. */
	  type = ZMAPSTYLE_MODE_BASIC ;
	}
    }


  if (type != ZMAPSTYLE_MODE_INVALID)
    {
      result = TRUE ;
      *type_out = type ;
    }

  return result ;
}


char *zMapFeatureStructType2Str(ZMapFeatureStructType type)
{
  static char *struct_types[] = {".", "Context", "Alignment", "Block", "FeatureSet", "Feature"} ;
  char *type_str ;

  zMapAssert(type == ZMAPFEATURE_STRUCT_INVALID || type == ZMAPFEATURE_STRUCT_CONTEXT
	     || type == ZMAPFEATURE_STRUCT_ALIGN || type == ZMAPFEATURE_STRUCT_BLOCK
	     || type ==  ZMAPFEATURE_STRUCT_FEATURESET || type == ZMAPFEATURE_STRUCT_FEATURE) ;

  type_str = struct_types[type] ;

  return type_str ;
}




char *zMapFeatureSubPart2Str(ZMapFeatureSubpartType subpart)
{
  char *subpart_str ;

  zMapAssert(subpart == ZMAPFEATURE_SUBPART_INVALID
	     || subpart == ZMAPFEATURE_SUBPART_INTRON || subpart == ZMAPFEATURE_SUBPART_EXON
	     || subpart == ZMAPFEATURE_SUBPART_EXON_CDS
	     || subpart == ZMAPFEATURE_SUBPART_GAP || subpart == ZMAPFEATURE_SUBPART_MATCH) ;

  /* Ok, for now the subpart enum has shifted values to allow OR'ing of other flags into it,
   * hence we need to do these tests...this may change back sometime. */
  if (subpart & ZMAPFEATURE_SUBPART_EXON_CDS)
    subpart_str = "Exon (CDS)" ;
  else if (subpart & ZMAPFEATURE_SUBPART_EXON)
    subpart_str = "Exon" ;
  else if (subpart & ZMAPFEATURE_SUBPART_INTRON)
    subpart_str = "Intron" ;
  else if (subpart & ZMAPFEATURE_SUBPART_GAP)
    subpart_str = "Gap" ;
  else if (subpart & ZMAPFEATURE_SUBPART_MATCH)
    subpart_str = "Match" ;
  else
    zMapAssertNotReached() ;

  return subpart_str ;
}



/* Strand must be one of the chars '+', '-' or '.'. */
gboolean zMapFeatureFormatStrand(char *strand_str, ZMapStrand *strand_out)
{
  gboolean result = FALSE ;

  if (strlen(strand_str) == 1
      && (*strand_str == '+' || *strand_str == '-' || *strand_str == '.'))
    {
      result = TRUE ;

      switch (*strand_str)
	{
	case '+':
	  *strand_out = ZMAPSTRAND_FORWARD ;
	  break ;
	case '-':
	  *strand_out = ZMAPSTRAND_REVERSE ;
	  break ;
	default:
	  *strand_out = ZMAPSTRAND_NONE ;
	  break ;
	}
    }

  return result ;
}



gboolean zMapFeatureStr2Strand(char *string, ZMapStrand *strand)
{
  gboolean status = TRUE;

  if (g_ascii_strcasecmp(string, "forward") == 0)
    *strand = ZMAPSTRAND_FORWARD;
  else if (g_ascii_strcasecmp(string, "reverse") == 0)
    *strand = ZMAPSTRAND_REVERSE;
  else if (g_ascii_strcasecmp(string, "none") == 0)
    *strand = ZMAPSTRAND_NONE;
  else
    status = FALSE;

  return status;
}


char *zMapFeatureStrand2Str(ZMapStrand strand)
{
  static char *strands[] = {".", "+", "-" } ;
  char *strand_str ;

  zMapAssert(strand == ZMAPSTRAND_NONE
	     || strand == ZMAPSTRAND_FORWARD || strand == ZMAPSTRAND_REVERSE) ;

  strand_str = strands[strand] ;

  return strand_str ;
}



/* Frame must be one of the chars '.', '0' or '1' or '2'. */
gboolean zMapFeatureFormatFrame(char *frame_str, ZMapFrame *frame_out)
{
  gboolean result = FALSE ;

  if (strlen(frame_str) == 1
      && (*frame_str == '.' || *frame_str == '0' || *frame_str == '1' || *frame_str == '2'))
    {
      result = TRUE ;

      switch (*frame_str)
	{
	case '0':
	  *frame_out = ZMAPFRAME_0 ;
	  break ;
	case '1':
	  *frame_out = ZMAPFRAME_1 ;
	  break ;
	case '2':
	  *frame_out = ZMAPFRAME_2 ;
	  break ;
	default:
	  *frame_out = ZMAPFRAME_NONE ;
	  break ;
	}
    }

  return result ;
}

gboolean zMapFeatureStr2Frame(char *string, ZMapFrame *frame)
{
  gboolean status = TRUE;

  if (g_ascii_strcasecmp(string, "0") == 0)
    *frame = ZMAPFRAME_0;
  else if (g_ascii_strcasecmp(string, "1") == 0)
    *frame = ZMAPFRAME_1;
  else if (g_ascii_strcasecmp(string, "2") == 0)
    *frame = ZMAPFRAME_2;
  else if (g_ascii_strcasecmp(string, "none") == 0)
    *frame = ZMAPFRAME_NONE;
  else
    status = FALSE;

  return status;
}

char *zMapFeatureFrame2Str(ZMapFrame frame)
{
  static char *frames[] = {".", "1", "2", "3" } ;
  char *frame_str ;

  zMapAssert(frame == ZMAPFRAME_NONE
	     || frame == ZMAPFRAME_0 || frame == ZMAPFRAME_1 || frame == ZMAPFRAME_2) ;

  frame_str = frames[frame] ;

  return frame_str ;
}




/* Phase must either be the char '.' or 0, 1 or 2. */
gboolean zMapFeatureFormatPhase(char *phase_str, ZMapPhase *phase_out)
{
  gboolean result = FALSE ;

  if (strlen(phase_str) == 1)
#ifdef RDS_DONT_INCLUDE
      && (*phase_str == '.' == 0
	  || *phase_str == '0' == 0 || *phase_str == '1' == 0 || *phase_str == '2' == 0))
#endif
    {
      result = TRUE ;

      switch (*phase_str)
	{
	case '0':
	  *phase_out = ZMAPPHASE_0 ;
	  break ;
	case '1':
	  *phase_out = ZMAPPHASE_1 ;
	  break ;
	case '2':
	  *phase_out = ZMAPPHASE_2 ;
	  break ;
	case '.':
	  *phase_out = ZMAPPHASE_NONE ;
	  break;
	default:
	  result = FALSE;
	  /* zMapAssertNotReached(); */
	  *phase_out = ZMAPPHASE_NONE ;
	  break ;
	}
    }

  return result ;
}


gboolean zMapFeatureValidatePhase(char *value, ZMapPhase *phase)
{
  gboolean status = FALSE ;
  int phase_num ;

  if (zMapStr2Int(value, &phase_num)
      && ((ZMapPhase)phase_num >= ZMAPPHASE_NONE || (ZMapPhase)phase_num <= ZMAPPHASE_2))
    {
      *phase = (ZMapPhase)phase_num ;
      status = TRUE ;
    }

  return status ;
}


char *zMapFeaturePhase2Str(ZMapPhase phase)
{
  static char *phases[] = {".", "0", "1", "2"} ;
  char *phase_str ;

  zMapAssert(phase == ZMAPPHASE_NONE || phase == ZMAPPHASE_0
	     || phase == ZMAPPHASE_1 || phase == ZMAPPHASE_2) ;

  phase_str = phases[phase] ;

  return phase_str ;
}




char *zMapFeatureHomol2Str(ZMapHomolType homol)
{
  static char *homols[] = {".", "dna", "protein", "translated"} ;
  char *homol_str ;

  zMapAssert(homol == ZMAPHOMOL_NONE || homol == ZMAPHOMOL_N_HOMOL
	     || homol == ZMAPHOMOL_X_HOMOL || homol == ZMAPHOMOL_TX_HOMOL) ;

  homol_str = homols[homol] ;

  return homol_str ;
}








/* Score must either be the char '.' or a valid floating point number.
 * Returns TRUE if the score is '.' or a valid floating point number, FALSE otherwise.
 * If the score is '.' then we return has_score = FALSE and score_out is not set,
 * otherwise has_score = TRUE and score_out contains the score. */
gboolean zMapFeatureFormatScore(char *score_str, gboolean *has_score, gdouble *score_out)
{
  gboolean result = FALSE ;

  if (strlen(score_str) == 1 && strcmp(score_str, ".") == 0)
    {
      result = TRUE ;
      *has_score = FALSE ;
    }
  else
    {
      gdouble score ;

      if ((result = zMapStr2Double(score_str, &score)))
	{
	  *has_score = TRUE ;
	  *score_out = score ;
	}
    }

  return result ;
}


