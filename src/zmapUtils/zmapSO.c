/*  File: zmapSO.c
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Functions to handle SO stuff.
 *
 * Exported functions: See zmapSO.h
 *              
 *-------------------------------------------------------------------
 */

#include <glib.h>

#include <ZMap/zmap.h>
#include <ZMap/zmapSO.h>



/* struct for SO stuff....only putting in SO_accession and SO_term for now but could add description later. */
typedef struct SOEntryStructName
{
  char *accession ;
  char *term ;
} SOEntryStruct, *SOEntry ;



static GHashTable *getHashTable(void) ;
static void fillHashTable(GHashTable *acc2SO) ;
static SOEntry findSOentry(GQuark SO_acc) ;





/* 
 *                 External interface.
 */



/* Given the SO accession as a quark will return the SO_term (e.g. "exon") as
 * a string. */
char *zMapSOAcc2Term(GQuark SO_accession)
{
  char *SO_term = NULL ;

  if (SO_accession)
    {
      SOEntry SO_entry ;

      if ((SO_entry = findSOentry(SO_accession)))
	SO_term = SO_entry->term ;
    }

  return SO_term ;
}


/* Given the SO accession as a quark will return the SO_term (e.g. "exon") as
 * a string. */
GQuark zMapSOAcc2TermID(GQuark SO_accession)
{
  GQuark SO_term_id = 0 ;

  if (SO_accession)
    {
      SOEntry SO_entry ;

      if ((SO_entry = findSOentry(SO_accession)))
	SO_term_id = g_quark_from_string(SO_entry->term) ;
    }

  return SO_term_id ;
}


/* Takes a variation string (e.g. "A/T") and returns a SO_accession
 * for that type of variation. Supported string formats are:
 *
 *   SNP:             "A/G"
 *   
 *   insertion:       "-/G"
 *                    "-/CTTA"
 *                    "-/(LARGEINSERTION)"
 *   
 *   deletion:        "T/-"
 *                    "TAAA/-"
 *                    "AAAAAAGTTCCTTGCATGATTAAAAAAGTATT/-"
 *   
 *   CNV:             "CNV_PROBE"
 * 
 *   Mixed:           "A/T/-"
 * 
 *
 *  */
GQuark zMapSOVariation2SO(char *variation_str)
{
  GQuark SO_acc = 0 ;


  /* Temporary check until we can move on to a new gtk. GRegex did not come along until minor
   * version 14. */
#if GLIB_MINOR_VERSION > 13

  static GRegex *insertion_exp = NULL, *deletion_exp, *snp_exp, *substitution_exp, *alteration_exp ;
  GMatchInfo *match_info ;
  GQuark var_id ;

  if (!insertion_exp)
    {
      insertion_exp = g_regex_new("-/[ACGT]+", 0, 0, NULL) ;
      deletion_exp = g_regex_new("[ACGT]+/-", 0, 0, NULL) ;
      snp_exp = g_regex_new("[ACGT]/[ACGT]", 0, 0, NULL) ;
      substitution_exp = g_regex_new("[ACGT]+/[ACGT]+", 0, 0, NULL) ;
      alteration_exp = g_regex_new("([ACGT]*|-)/([ACGT]*|-)/([ACGT]*|-)", 0, 0, NULL) ;
    }

  if (g_regex_match(alteration_exp, variation_str, 0, &match_info))
    var_id = g_quark_from_string(SO_ACC_SEQ_ALT) ;
  else if (g_regex_match(insertion_exp, variation_str, 0, &match_info)
      || g_ascii_strcasecmp("-/(LARGEINSERTION)", variation_str) ==0)
    var_id = g_quark_from_string(SO_ACC_INSERTION) ;
  else if (g_regex_match(deletion_exp, variation_str, 0, &match_info))
    var_id = g_quark_from_string(SO_ACC_DELETION) ;
  else if (g_regex_match(snp_exp, variation_str, 0, &match_info))
    var_id = g_quark_from_string(SO_ACC_SNP) ;
  else if (g_regex_match(substitution_exp, variation_str, 0, &match_info))
    var_id = g_quark_from_string(SO_ACC_SUBSTITUTION) ;
  else if (g_ascii_strcasecmp("CNV_PROBE", variation_str) == 0)
    var_id = g_quark_from_string(SO_ACC_CNV) ;

  /* belt and braces to make sure that we know about this id. */
  if (var_id && (findSOentry(var_id)))
    SO_acc = var_id ;

#endif

  return SO_acc ;
}


/* Probably need a has here in the end but this will get us started... */
GQuark zMapSOFeature2SO(ZMapFeature feature)
{
  GQuark SO_acc = 0, tmp_id ;


  if (g_ascii_strcasecmp(g_quark_to_string(feature->source_id), "polya_site") == 0)
    tmp_id = g_quark_from_string(SO_ACC_POLYA_SITE) ;
  else if (g_ascii_strcasecmp(g_quark_to_string(feature->source_id), "polya_signal") == 0)
    tmp_id = g_quark_from_string(SO_ACC_POLYA_SIGNAL) ;

  /* belt and braces to make sure that we know about this id. */
  if (tmp_id && (findSOentry(tmp_id)))
    SO_acc = tmp_id ;

  return SO_acc ;
}






/* 
 *                    Internal functions.
 */


/* Returns the internal hash table of SO entries ensuring that the table
 * is created if it does not exist already. */
static GHashTable *getHashTable(void)
{
  static GHashTable *acc2SO = NULL ;

  if (!acc2SO)
    {
      acc2SO = g_hash_table_new(NULL, NULL) ;

      fillHashTable(acc2SO) ;
    }

  return acc2SO ;
}


/* Creates/returns the internal hash table of SO entries.
 *
 * We are using a hash table for lookup because the terms may be looked up from
 * performance sensitive code, e.g. the parsers and dumpers, and therefore speed
 * will be important. */
static void fillHashTable(GHashTable *acc2SO)
{
  static SOEntryStruct SO_table[] =
    {
      {SO_ACC_CNV, "copy_number_variation"},
      {SO_ACC_DELETION, "deletion"},
      {SO_ACC_INSERTION, "insertion"},
      {SO_ACC_POLYA_JUNC, "polyA_junction"},
      {SO_ACC_POLYA_SEQ, "polyA_sequence"},
      {SO_ACC_POLYA_SIGNAL, "polyA_signal_sequence"},
      {SO_ACC_POLYA_SITE, "polyA_site"},
      {SO_ACC_SEQ_ALT, "sequence_alteration"},
      {SO_ACC_SNP, "SNP"},
      {SO_ACC_SUBSTITUTION, "substitution"},
      {SO_ACC_TRANSCRIPT, "transcript"},
      {NULL, NULL}					    /* Must be last value. */
    } ;
  SOEntry curr ;

  curr = &(SO_table[0]) ;
  while (curr->accession)
    {
      g_hash_table_insert(acc2SO, GINT_TO_POINTER(g_quark_from_string(curr->accession)), curr) ;

      curr++ ;
    }

  return ;
}


/* Given a SO_acc, finds the corresponding SO entry. */
static SOEntry findSOentry(GQuark SO_acc)
{
  SOEntry SO_entry ; ;

  SO_entry = g_hash_table_lookup(getHashTable(), GINT_TO_POINTER(SO_acc)) ;

  return SO_entry ;
}








#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* Here's a sample of terms from acedb..... */



/* Makes a hash table to convert acedb source/feature names into SO terms.
 * Basis of hash is concatenation of "source==feature", the "==" is not allowed
 * in acedb so this ensures a unique string which hashes to the SO term.
 * In addition the string is lower cased. */
static GHashTable *makeAcedb2SOHash(void)
{
  GHashTable *acedb2SO = NULL ;
  FeatureToSOStruct featureSO[] =
    {
      {"sequence", "link", "SO:0000148"},
      {"region", "genomic_canonical", "SO:0000001"},
      {"sequence", "genomic_canonical", "SO:0000149"},

      {"sequence", "curated", "SO:0000185"},
      {"transcript", "coding_transcript", "SO:0000185"},
      {"coding_exon", "curated", "SO:0000316"},
      {"exon", "curated", "SO:0000316"},
      {"exon", "coding_transcript", "SO:0000316"},
      {"intron", "curated", "SO:0000188"},
      {"intron", "coding_transcript", "SO:0000188"},
      {"CDS", "curated", "SO:0000316"},

      {"pseudogene", "pseudogene", "SO:0000336"},
      {"sequence", "pseudogene", "SO:0000336"},

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      {"exon", "psuedogene", "decayed_exon"},
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      {"experimental", "cDNA_for_RNAi", "RNAi_reagent"},
      {"experimental", "RNAi", "RNAi_reagent"},


      {"repeat", "inverted", "inverted_repeat"},
      {"repeat", "tandem", "tandem_repeat"},

      {"similarity", "RepeatMasker", "nucleotide_motif"},
      {"similarity", "wublastx", "protein_match"},
      {"similarity", "wublastx_fly", "protein_match"},
      {"similarity", "waba_weak", "cross_genome_match"},
      {"similarity", "waba_strong", "cross_genome_match"},
      {"similarity", "waba_coding", "cross_genome_match"},

      {"transcription", "BLASTN_TC1", "nucleotide_match"},

      {"structural", "BLASTN_TC1", "nucleotide_match"},
      {"structural", "GenePair_STS", "PCR_product"},

      {"UTR", "UTR", "UTR"},

      {"trans-splice_acceptor", "UTR", "UTR"},
      {"trans-splice_acceptor", "SL1", "trans_splice_acceptor_site"},

      {"SNP", "Allele", "SNP"},

      {"reagent", "Orfeome_project", "PCR_product"},
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      {NULL, NULL, NULL}				    /* Last one must be NULL. */
    } ;

  acedb2SO = makeHash(featureSO) ;

  return acedb2SO ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



