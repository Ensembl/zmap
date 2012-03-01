/*  File: zmapPeptide.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Routines to translate DNA to peptide, optionally with
 *              an alternative genetic code.
 *
 * Exported functions: See ZMap/zmapPeptide.h
 *-------------------------------------------------------------------
 */

/*
 *
 * We hereby acknowledge that this code was adapted directly from
 * the dna and peptide functions in acedb (see www.acedb.org).
 *
 */


#include <ZMap/zmap.h>

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapString.h>
#include <ZMap/zmapSequence.h>
#include <ZMap/zmapDNA.h>
#include <ZMap/zmapPeptide.h>


/* NOTE: there is quite a lot of baggage here that will be sifted out
 * as I arrive at the final set of functions. */



/* This struct represents a peptide sequence. */
typedef struct _ZMapPeptideStruct
{
  GQuark sequence_name ;				    /* As in the FASTA sense. */
  GQuark gene_name ;					    /* If peptide is a gene translation. */
  int start, end ;					    /* e.g. coords of gene in sequence. */
  gboolean stop_codon ;					    /* TRUE means there is a final stop codon. */
  gboolean incomplete_final_codon ;			    /* TRUE means the end phase of the translation != 0 */
  GArray *peptide ;					    /* The peptide string. */
} ZMapPeptideStruct ;



enum {PEP_BASES = 4, PEP_CODON_LENGTH = 3, PEP_TOTAL_CODONS = PEP_BASES * PEP_BASES * PEP_BASES} ;


/* Used to construct a translation table for a particular genetic code.
 * The table is a 64 element array, the order of this string is vital since
 * translation of the DNA is done by an indexed lookup into this array
 * based on the way the DNA is encoded, so you should not reorder this array.
 * Each element also has two associated bools which control special processing
 * for the start/stop of the peptide which is required for some alternative
 * starts and stops:
 *
 * alternative_start = TRUE means that this amino acid should be replaced with
 *                          methionine when at the start of a transcript.
 *  alternative_stop = TRUE means that this amino acid should be replaced with
 *                          a stop when at the end of a transcript. */
typedef struct
{
  char amino_acid ;
  gboolean alternative_start ;
  gboolean alternative_stop ;
} CodonTranslationStruct, *CodonTranslation ;


/* This struct represents a genetic code. */
typedef struct _ZMapGeneticCodeStruct
{
  GQuark name ;						    /* Text name of the Genetic code, e.g. mitochondrial. */
  GArray *table ;					    /* The code table, an array of CodonTranslationStruct. */
} ZMapGeneticCodeStruct ;


typedef char (*CodonTranslatorFunc)(char *codon, ZMapGeneticCode genetic_code, int *index_out) ;


static GArray *translateDNA(char *dna, ZMapGeneticCode translation_table, gboolean include_stop,
			    gboolean *incomplete_final_codon) ;
static GArray *translateDNASegment(char *dna_in, int from, int length, ZMapStrand strand,
				   ZMapGeneticCode translation_table, gboolean include_stop,
				   gboolean *incomplete_final_codon) ;

static GArray *doDNATranslation(ZMapGeneticCode code_table, GArray *obj_dna, ZMapStrand strand,
				gboolean encode, gboolean include_stop) ;


static ZMapGeneticCode pepGetTranslationTable(void) ;

static char E_codon(char *s, ZMapGeneticCode genetic_code, int *index_out) ;
static char E_reverseCodon (char* cp, ZMapGeneticCode genetic_code, int *index_out) ;
#ifdef UNUSED_FUNCTIONS
static char E_antiCodon (char* cp, ZMapGeneticCode genetic_code, int *index_out) ;
#endif


/* This is the standard genetic code for most organisms, it is the default and is used to translate
 * dna into peptides unless an alternative has been specified. */
#define STANDARD_GENETIC_CODE "KNKNIIMIRSRSTTTT*Y*YLFLF*CWCSSSSEDEDVVVVGGGGAAAAQHQHLLLLRRRRPPPP"





/*

We use the standard UPAC coding for representing DNA with a single character
per base:

(Comment moved to zmapDNA.h)

*/

char complementBase[] =
 { 0, T_,A_,W_,C_,Y_,M_,H_,G_,K_,R_,D_,S_,B_,V_,N_ } ;




/*
 * Extended Codon translator
 *
 * This translates 3 dna bases into the equivalent amino acid. Since mappings
 * between bases and amino acids can change between different organisms, between
 * cell organelles (nucleus v.s. mitochondria, or between transcripts (see
 * Selenocysteine usage) the appropriate mapping must be selected for each
 * piece of DNA to be translated. This is achieved as follows:
 *
 * The Sequence object in the database contains a tag named Genetic_code.
 * That field refers to an object of class Genetic_code that defines the
 * mapping.  You need two steps to make the translation:
 *
 * 1) use pepGetTranslationTable() to translate the genetic_code key into
 *    a char * that is used for the translation.  (returns null on error)
 * 2) pass the char * to e_codon() along with the 3 bases to translate.
 *
 * You never free the value returned by pepGetTranslationTable(); it is
 * cached internally.
 *
 */





/* Takes a dna string and returns a simple string containing the peptide translation. */
char *zMapPeptideCreateRaw(char *dna, ZMapGeneticCode translation_table, gboolean include_stop)
{
  char *peptide_str = NULL ;
  GArray *peptide ;
  gboolean incomplete_final_stop = FALSE ;

  zMapAssert(dna && *dna) ;

  if ((peptide = translateDNA(dna, translation_table, include_stop, &incomplete_final_stop)))
    {
      peptide_str = g_array_free(peptide, FALSE) ;
    }

  return peptide_str ;
}



/* Takes a dna string and returns a simple string containing the peptide translation for the given
 * section of dna. */
char *zMapPeptideCreateRawSegment(char *dna,  int from, int length, ZMapStrand strand,
				  ZMapGeneticCode translation_table, gboolean include_stop)
{
  char *peptide_str = NULL ;
  GArray *peptide ;
  gboolean incomplete_final_stop = FALSE ;

  zMapAssert(dna && *dna) ;

  if ((peptide = translateDNASegment(dna, from, length, strand,
				     translation_table, include_stop, &incomplete_final_stop)))
    {
      peptide_str = g_array_free(peptide, FALSE) ;
    }

  return peptide_str ;
}


/* Takes a peptide string and lower cases it inplace. */
gboolean zMapPeptideCanonical(char *peptide)
{
  gboolean result = TRUE ;				    /* Nothing to fail currently. */
  char *base ;

  zMapAssert(peptide && *peptide) ;

  base = peptide ;
  while (*base)
    {
      *base = g_ascii_toupper(*base) ;
      base++ ;
    }

  return result ;
}


/* Checks that peptide is a valid string (). */
gboolean zMapPeptideValidate(char *peptide)
{
  gboolean valid = FALSE ;

  if (peptide && *peptide)
    {
      char *aa ;

      aa = peptide ;
      valid = TRUE ;
      while (*aa)
	{
	 if (*aa != 'A' && *aa != 'R' && *aa != 'N' && *aa != 'D'
	     && *aa != 'C' && *aa != 'E' && *aa != 'Q' && *aa != 'G'
	     && *aa != 'H' && *aa != 'I' && *aa != 'L' && *aa != 'K'
	     && *aa != 'M' && *aa != 'F' && *aa != 'P' && *aa != 'S'
	     && *aa != 'T' && *aa != 'W' && *aa != 'Y' && *aa != 'V'
	     && *aa != 'X')
	    {
	      valid = FALSE ;
	      break ;
	    }
	  else
	    aa++ ;
	}
    }

  return valid ;
}



/* Translate dna (which must be a valid C string) into the corresponding
 * peptide string. If a translation table is supplied then this is used
 * to do the translation, otherwise the Standard Genetic Code is used.
 * If include_stop is TRUE then the peptide will include an "*" to show
 * the terminating stop codon (assuming the dna ends with a stop codon).
 */
ZMapPeptide zMapPeptideCreate(char *sequence_name, char *gene_name,
			      char *dna, ZMapGeneticCode translation_table, gboolean include_stop)
{
  ZMapPeptide pep = NULL ;

  pep = g_new0(ZMapPeptideStruct, 1) ;

  if (sequence_name && *sequence_name)
    pep->sequence_name = g_quark_from_string(sequence_name) ;

  if (gene_name && *gene_name)
    pep->gene_name = g_quark_from_string(gene_name) ;

  pep->peptide = translateDNA(dna, translation_table, include_stop, &(pep->incomplete_final_codon)) ;

  if (include_stop && g_array_index(pep->peptide, char, (pep->peptide->len - 2)) == '*')
    pep->stop_codon = TRUE ;

  return pep ;
}


/* Create a peptide large enough to hold the supplied dna but do not
 * translate the dna, contents of the peptide are undefined.
 * If include_stop is TRUE then the peptide will include an "*" to show
 * the terminating stop codon (assuming the dna ends with a stop codon).
 */
ZMapPeptide zMapPeptideCreateEmpty(char *sequence_name, char *gene_name, char *dna, gboolean include_stop)
{
  ZMapPeptide pep = NULL ;
  int bases, len ;

  pep = g_new0(ZMapPeptideStruct, 1) ;

  if (sequence_name && *sequence_name)
    pep->sequence_name = g_quark_from_string(sequence_name) ;

  if (gene_name && *gene_name)
    pep->gene_name = g_quark_from_string(gene_name) ;

  /* Get length of  */
  bases = strlen(dna) ;
  len = bases / 3 ;

  if (bases % 3)					    /* + 1 for 'X' if an incomplete codon. */
    len++ ;

  /* Create the array, TRUE param adds a null byte to the end. */
  pep->peptide = g_array_sized_new(TRUE, FALSE, sizeof(char), len) ;
  pep->peptide->len = len ;


  return pep ;
}


/* THIS NEEDS REMOVING AS IT DOESN'T DO ANYTHING MORE THAN zMapPeptideCreate() */
/* The same as zMapPeptideCreate, but decodes dna string, which makes
 * it slower. It could be done with a copy, but then that uses
 * memory... */
ZMapPeptide zMapPeptideCreateSafely(char *sequence_name, char *gene_name,
                                    char *dna, ZMapGeneticCode translation_table,
                                    gboolean include_stop)
{
  ZMapPeptide pep = NULL;

  pep = zMapPeptideCreate(sequence_name, gene_name, dna,
                          translation_table, include_stop);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Not needed now..... */

  dnaDecodeString(dna);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return pep;
}



/* The length of the amino acid sequence */
int zMapPeptideLength(ZMapPeptide peptide)
{
  int length ;

  length = peptide->peptide->len - 1 ;			    /* Remove trailing NULL. */

  return length ;
}



int zMapPeptideFullCodonAALength(ZMapPeptide peptide)
{
  int length;

  length = zMapPeptideLength(peptide);

  if(peptide->incomplete_final_codon)
    length--;

  return length;
}

/* The length of the amino acid sequence than came from _full_ codons of the dna */
int zMapPeptideFullSourceCodonLength(ZMapPeptide peptide)
{
  int length ;

  length = (zMapPeptideFullCodonAALength(peptide) * 3) ;

  return length ;
}

gboolean zMapPeptideHasStopCodon(ZMapPeptide peptide)
{
  return peptide->stop_codon ;
}

char *zMapPeptideSequence(ZMapPeptide peptide)
{
  char *sequence ;

  sequence = (char *)peptide->peptide->data ;

  return sequence ;
}


char *zMapPeptideSequenceName(ZMapPeptide peptide)
{
  char *sequence_name ;

  zMapAssert(peptide) ;

  sequence_name = (char *)g_quark_to_string(peptide->sequence_name) ;

  return sequence_name ;
}


char *zMapPeptideGeneName(ZMapPeptide peptide)
{
  char *gene_name ;

  zMapAssert(peptide) ;

  gene_name = (char *)g_quark_to_string(peptide->gene_name) ;

  return gene_name ;
}



gboolean zMapPeptideMatch(char *cp, char *end,
			  char *template, ZMapStrand strand, ZMapGeneticCode translation_table,
			  char **start_out, char **end_out, char **match_str)
{
  gboolean result = FALSE ;
  char *match_template ;
  int i ;

  /* Create the template for matching... */
  match_template = g_strdup_printf("*%s*", template) ;
  if (strand == ZMAPSTRAND_REVERSE)
    match_template = g_strreverse(match_template) ;

  /* Do a general string match. */
  if ((i = zMapStringFindMatch(cp, match_template)) != 0)
    {
      int match_length = strlen(match_template) - 2 ;	    /* remember start/end "*" */

      result = TRUE ;
      *start_out = cp + (i - 1) ;
      *end_out = *start_out + (match_length - 1) ; /* We've gone past the last match so
								reset. */
      if (match_str)
	*match_str = g_strndup(*start_out, *end_out - *start_out + 1) ;
    }

  return result ;
}


/* I don't like the need for revcomp'd....something must be wrong ?? */
GList *zMapPeptideMatchFindAll(char *target, char *query,
			       gboolean rev_comped,
			       ZMapStrand orig_strand, ZMapFrame orig_frame,
			       int from_in, int length,
			       int max_errors, int max_Ns, gboolean return_matches)
{
  GList *sites = NULL ;
  int  dna_len, n ;
  char *cp ;
  char *start, *end ;
  char *search_start, *search_end ;
  char *match_str = NULL ;
  char **match_ptr = NULL ;
  int from ;
  int frames[6] = {0}, frame_num, i, frame ;
  ZMapStrand strand ;

  zMapAssert(target && *target && query && *query) ;

  dna_len = n = strlen(target) ;
  zMapAssert(from_in >= 0 && length > 0 && (from_in + length) <= dna_len) ;

  if (n > from_in + length)
    n = from_in + length ;


  /* Return the actual match string ? */
  if (return_matches)
    match_ptr = &match_str ;

  search_start = target + from_in ;
  search_end = search_start + length - 1 ;
  start = end = cp = search_start ;


  frame_num = 0 ;
  if (orig_strand == ZMAPSTRAND_REVERSE || orig_strand == ZMAPSTRAND_NONE)
    {
      switch (orig_frame)
	{
	case ZMAPFRAME_0:
	  frames[frame_num] = -3 ;
	  frame_num++ ;
	  break ;
	case ZMAPFRAME_1:
	  frames[frame_num] = -2 ;
	  frame_num++ ;
	  break ;
	case ZMAPFRAME_2:
	  frames[frame_num] = -1 ;
	  frame_num++ ;
	  break ;
	default:
	  frames[frame_num] = -3 ;
	  frame_num++ ;
	  frames[frame_num] = -2 ;
	  frame_num++ ;
	  frames[frame_num] = -1 ;
	  frame_num++ ;
	  break ;
	}
    }

  if (orig_strand == ZMAPSTRAND_FORWARD || orig_strand == ZMAPSTRAND_NONE)
    {
      switch (orig_frame)
	{
	case ZMAPFRAME_0:
	  frames[frame_num] = 0 ;
	  frame_num++ ;
	  break ;
	case ZMAPFRAME_1:
	  frames[frame_num] = 1 ;
	  frame_num++ ;
	  break ;
	case ZMAPFRAME_2:
	  frames[frame_num] = 2 ;
	  frame_num++ ;
	  break ;
	default:
	  frames[frame_num] = 0 ;
	  frame_num++ ;
	  frames[frame_num] = 1 ;
	  frame_num++ ;
	  frames[frame_num] = 2 ;
	  frame_num++ ;
	  break ;
	}
    }


  for (i = 0 ; i < frame_num ; i++)
    {
      char *protein ;
      ZMapFrame zmap_frame ;
      int frame_offset ;

      frame = frames[i] ;

      if (frame < 0)
	strand = ZMAPSTRAND_REVERSE ;
      else
	strand = ZMAPSTRAND_FORWARD ;

      if (frame == -3 || frame == 0)
	zmap_frame = ZMAPFRAME_0 ;
      else if (frame == -2 || frame == 1)
	zmap_frame = ZMAPFRAME_1 ;
      else
	zmap_frame = ZMAPFRAME_2 ;

      /* must calculate from here so frame is moved for protein translate.... */
      from = from_in + ((frame + 6) % 3) ;

      if (from + length > dna_len)
	length = dna_len - from ;

      /* Sort this out....needs better error handling.... */
      zMapAssert(length > 0) ;

      /* offset in reference sequence where search will start. */
      frame_offset = from - from_in ;

      protein = zMapPeptideCreateRawSegment(target, from, length, strand,
					    NULL, FALSE) ;

      length = 3 * (length / 3) ;
      if (length < 3)					    /* This should be done much earlier... */
	return NULL ;

      search_start = protein ;
      search_end = search_start + length - 1 ;
      start = end = cp = search_start ;

      while (zMapPeptideMatch(cp, search_end, query, strand, NULL,
			      &start, &end, match_ptr))
	{
	  ZMapDNAMatch match ;
	  int incr ;

	  /* Record this match but match needs to record if its dna or peptide so coords are
	   * interpreted correctly.... */
	  match = g_new0(ZMapDNAMatchStruct, 1) ;
	  match->match_type = ZMAPSEQUENCE_PEPTIDE ;
	  match->strand = strand ;
	  match->frame = zmap_frame ;

	  match->start = start - search_start ;
	  match->end = end - search_start ;

	  /* There are routines to do these calculations...find them.... */
	  /* start/end on reference (dna) sequence. */
	  if (rev_comped)
	    incr = -2 ;
	  else
	    incr = 0 ;

	  match->ref_start = from_in + ((match->start * 3)  + frame_offset + 1) ;
	  match->ref_end = from_in + (((match->end * 3) + 2)  + frame_offset + 1) ;

	  if (return_matches)
	    match->match = *match_ptr ;

	  sites = g_list_append(sites, match) ;

	  /* Move pointers on. */
	  cp = end + 1 ;
	}
    }


  return sites ;
}



void zMapPeptideDestroy(ZMapPeptide peptide)
{
  zMapAssert(peptide) ;

  g_array_free(peptide->peptide, TRUE) ;

  g_free(peptide) ;

  return ;
}






/*
 *                       Internal functions.
 */





/* COMMENTS NEED UPDATING FOR ZMAP.... */
/* Returns the translation table for an object or NULL if there was some
 * kind of error, if there was an error then geneticCodep is not changed.
 *
 * The genetic code will either be the standard genetic code, or, if an alternative
 * code was specified via a Genetic_code object in the object or any of the objects
 * parents in the sequence or SMap'd hierachy, then that is returned.
 *
 * If a valid genetic code was found, then if geneticCodep is non-null and
 * if a Genetic_code object was found, its key is returned, otherwise KEY_UNDEFINED
 * is returned (i.e. the standard genetic code was used).
 *
 * Genetic_code model:
 *
 * ?Genetic_code   Other_name ?Text
 *                 Remark Text
 *                 Translation UNIQUE Text
 *                 Start UNIQUE Text
 *                 Stop   UNIQUE Text
 *                 Base1 UNIQUE Text
 *                 Base2 UNIQUE Text
 *                 Base3 UNIQUE Text
 */
static ZMapGeneticCode pepGetTranslationTable(void)
{
  ZMapGeneticCode translationTable = NULL ;
  static GPtrArray *maps = NULL ;			    /* To hold cache of genetic codes...?? */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  KEY gKey = KEY_UNDEFINED ;
  int index = 0 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  /* NOTE lots of temp code in here while I get this working.... */



  /* First time through create cache for maps and make first entry the standard code. */
  if (!maps)
    {
      ZMapGeneticCode standard_code ;
      int i ;

      maps = g_ptr_array_sized_new(20) ;

      /* zero'th entry is standard genetic code */
      standard_code = g_new0(ZMapGeneticCodeStruct, 1) ;
      standard_code->name = g_quark_from_string("Standard Code") ;
      standard_code->table = g_array_sized_new(FALSE, TRUE, sizeof(CodonTranslationStruct), PEP_TOTAL_CODONS) ;

      for (i = 0 ; i < PEP_TOTAL_CODONS ; i++)
	{
	  CodonTranslation amino = &g_array_index(standard_code->table, CodonTranslationStruct, i) ;

	  amino->amino_acid = STANDARD_GENETIC_CODE[i] ;
	  amino->alternative_start = amino->alternative_stop = FALSE ;
	}

      g_ptr_array_index(maps, 0) = standard_code ;
    }


  /* TEMP CODE... */
  translationTable = g_ptr_array_index(maps, 0) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

  /* ORIGINAL ACEDB CODE.....WE WILL NEED THIS WHEN WE READ IN ALTERNATIVE TRANSLATION TABLES,
   * WE'LL NEED TO ADD CODE TO GET THESE TABLES FROM ACEDB.... */

  /* Find the correct genetic_code from the set of cached codes or get it from the database,
   * remember that if gKey is KEY_UNDEFINED (i.e. zero) then we automatically use the standard
   * code. */
  gKey = pepGetGeneticCode(key) ;


  /* Lets hope we don't encounter a long running database where loads of genetic code objects
   * have been created/deleted otherwise our array may be v. big.....one to watch... */
  index = KEYKEY(gKey) ;

  if (!(translationTable = g_ptr_array_index(maps, index)))
    {
      OBJ obj ;

      if ((obj = bsCreate(gKey)))
	{
	  char *translation, *start, *stop, *base1, *base2, *base3 ;
	  char result[4*4*4+1] ;
	  int x,n ;

	  /* These fields encode the translation table, they must all be correct. */
	  if (bsGetData(obj, str2tag("Translation"), _Text, &translation)
	      && bsGetData(obj, str2tag("Start"), _Text, &start)
	      && bsGetData(obj, str2tag("Stop"), _Text, &stop)
	      && bsGetData(obj, str2tag("Base1"), _Text, &base1)
	      && bsGetData(obj, str2tag("Base2"), _Text, &base2)
	      && bsGetData(obj, str2tag("Base3"), _Text, &base3)
	      && strlen(translation) == 64
	      && strlen(start) == 64
	      && strlen(stop) == 64
	      && strlen(base1) == 64
	      && strlen(base2) == 64
	      && strlen(base3) == 64)
	    {
	      GArray *genetic_code ;

	      memset(result,0,4*4*4) ;
	      result[4*4*4] = 0 ;

	      genetic_code = arrayCreate(PEP_TOTAL_CODONS, PepTranslateStruct) ;

	      /* walk through the combinations and do the conversion.  I don't
	       * assume that the bases are in a particular order, even though it
	       * looks like it from the database. */
	      for (x = 0 ; x < 64 ; x++)
		{
		  char xl[3] ;

		  xl[0] = base1[x] ;
		  xl[1] = base2[x] ;
		  xl[2] = base3[x] ;

		  n = t_codon_code(xl) ;
		  if (n >= 0)
		    {
		      PepTranslate amino = &g_array_index(genetic_code, PepTranslateStruct, n) ;
		      result[n] = translation[x] ;

		      amino->amino_acid = translation[x] ;
		      if (start[x] == 'M')
			amino->alternative_start = TRUE ;
		      else
			amino->alternative_start = FALSE ;

		      if (stop[x] == '*')
			amino->alternative_stop = TRUE ;
		      else
			amino->alternative_stop = FALSE ;
		    }
		  else
		    break ;
		}

	      /* Must have set all 64 translations for valid genetic code. */
	      if (x != 64)
		arrayDestroy(genetic_code) ;
	      else
		translationTable = genetic_code ;
	    }
	}

      bsDestroy(obj) ;
    }

  if (translationTable)
    {
      if (geneticCodep)
	*geneticCodep = gKey ;

      if (pep_debug_G)
	printTranslationTable(gKey, translationTable) ;
    }
  else
    {
      messerror("%s does not specify genetic code correctly.", nameWithClassDecorate(gKey));
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




  return translationTable ;
}



/* Note that I have just used the code from acedb which held dna in a compact encoded form so
 * in order to do the translation the dna needs to be encoded and then decoded back so it
 * stays the same. */
static GArray *translateDNA(char *dna, ZMapGeneticCode translation_table, gboolean include_stop,
			    gboolean *incomplete_final_codon)
{
  GArray *peptide = NULL ;

  zMapAssert(dna && *dna && incomplete_final_codon) ;

  peptide = translateDNASegment(dna, 0, -1, TRUE,
				translation_table, include_stop, incomplete_final_codon) ;

  return peptide ;
}


/* Note that I have just used the code from acedb which held dna in a compact encoded form so
 * in order to do the translation the dna needs to be encoded and then decoded back so it
 * stays the same.
 *
 * NOTE:
 *          0 <= from <= (dna_length - 1)
 *          0 < length <= (dna_length - from) || length == -1 => the rest of the dna.
 *          -3 <= frame < 3 (makes for easier loop coding)
 *
 *  */
static GArray *translateDNASegment(char *dna_in, int from, int length, ZMapStrand strand,
				   ZMapGeneticCode translation_table, gboolean include_stop,
				   gboolean *incomplete_final_codon)
{
  GArray *peptide = NULL ;
  char *dna ;
  int dna_len ;
  GArray *dna_array ;
  char *data ;

  zMapAssert(dna_in && *dna_in && incomplete_final_codon) ;

  dna_len = strlen(dna_in) ;

  length = dna_len ;

  /* agh...I have to copy the dna now...sigh... */
  if((dna = g_strndup((dna_in + from), length)))
    {
      dnaEncodeString(dna) ;

      dna_array = g_array_new(TRUE, FALSE, sizeof(char)) ;

      dna_array = g_array_append_vals(dna_array, dna, length) ;

      /* I think we can free the dna now... */
      g_free(dna);
    }

  if (!translation_table)
    translation_table = pepGetTranslationTable() ;

  {
    int dna_min = 1 ;
    int dna_max = dna_array->len ;
    int bases = dna_max - dna_min + 1 ;

    if (bases % 3)
      *incomplete_final_codon = TRUE ;
    else
      *incomplete_final_codon = FALSE ;
  }

  peptide = doDNATranslation(translation_table, dna_array, strand, FALSE, include_stop) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Tidy up, note how we just leave the dna as it is still the original data. */
  data = g_array_free(dna_array, FALSE) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  /* in this version we now free the dna... */
  data = g_array_free(dna_array, TRUE) ;

  return peptide ;
}



/* This function encapsulates the translation of DNA to peptide and returns either an encoded
 * or decoded peptide array with or without the stop codon (if there is one).
 * The rules for translation are a bit more complex now that alternative start/stop's can be
 * specified. */
static GArray *doDNATranslation(ZMapGeneticCode code_table, GArray *obj_dna, ZMapStrand strand,
				gboolean encode, gboolean include_stop)
{
  GArray *pep ;
  int dna_min, dna_max ;
  int bases, pepmax, x, code_table_index ;
  char cc ;
  char str_null = '\0' ;
  CodonTranslatorFunc trans_func ;

  zMapAssert(code_table && obj_dna) ;

  /* Set up appropriate translator func. */
  if (strand == ZMAPSTRAND_FORWARD)
    trans_func = E_codon ;
  else
    trans_func = E_reverseCodon ;

  /* Allocate array for peptides. NOTE that incomplete codons at the end of
   * the sequence are represented by an X so we make sure the peptide array
   * is long enough for this. And for the terminating NULL char to make it
   * a valid C string. */
  dna_min = 1 ;
  dna_max = obj_dna->len ;
  bases = dna_max - dna_min + 1 ;
  x = pepmax = bases / 3 ;

  if (bases % 3)					    /* + 1 for 'X' if an incomplete codon. */
    x++ ;

  x++ ;							    /* +1 for the terminating NULL char. */

  pep = g_array_sized_new(TRUE, TRUE, sizeof(char), x) ;


  dna_min-- ;
  x = cc = 0 ;

  /* Deal with alternative start codons. */
  cc = trans_func(&g_array_index(obj_dna, char, dna_min), code_table, &code_table_index) ;
  if (cc != 'X' && cc != 'M')
    {
      CodonTranslation amino = &g_array_index(code_table->table, CodonTranslationStruct, code_table_index) ;

      if (amino->alternative_start)
	cc = 'M' ;
    }
  g_array_append_val(pep, cc) ;


  /* Deal with the rest. */
  x++ ;
  dna_min += 3 ;
  while (x < pepmax)
    {
      cc = trans_func(&g_array_index(obj_dna, char, dna_min), code_table, &code_table_index) ;
      g_array_append_val(pep, cc) ;

      x++ ;
      dna_min += 3 ;
    }


  /* Deal with incomplete or alternative end codons. */
  if (bases % 3)
    {
      char end = 'X' ;

      g_array_append_val(pep, end) ;
    }
  else
    {
      if (cc != 'X' && cc != '*')
	{
	  CodonTranslation amino = &g_array_index(code_table->table, CodonTranslationStruct, code_table_index) ;

	  if (amino->alternative_stop)
	    {
	      cc = '*' ;

	      g_array_index(pep, char, x - 1) = cc ;
	    }
	}

      if (!include_stop && cc == '*')
	pep = g_array_remove_index(pep, pep->len - 1) ;
    }


  /* add null to end.... */
  g_array_append_val(pep, str_null) ;


  return pep ;
}



/* The e_NNNN calls allow external callers to do simple translation of a codon to
 * an amino acid, no account is taken of alternative start/stop information.
 *
 * The E_NNNN calls are internal to peptide.c and return information that can be
 * used by the peptide routines to take account of alternative start/stops when
 * translating an entire transcript. */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* I'm not sure I need these.... */

/* External translation functions. */
char e_codon(char *s, GArray *genetic_code)
{
  return E_codon(s, genetic_code, NULL) ;
}

char e_reverseCodon(char* cp, GArray *genetic_code)
{
  return E_reverseCodon(cp, genetic_code, NULL) ;
}

char e_antiCodon(char* cp, GArray *genetic_code)
{
  return E_antiCodon(cp, genetic_code, NULL) ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* E_codon() - do the actual translation
 *
 * s points at the 3 bytes of DNA code to translate, encoded as in dna.h
 *
 * genetic_code is an array containing the 64 codon map to use.  Get it
 * from pepGetTranslationTable()
 *
 * The return value is the single character protein identifier or X if
 * the codon cannot be translated (e.g. this happens when the DNA contains.
 * IUPAC ambiguity codes for instance).
 *
 * If index_out is non-NULL the array index of the codon translation is
 * returned, this enables the calling routine to deal with alternative
 * starts and stops, if the codon cannot be translated, -1 is returned.
 * (It would be easy enough to return a list of all the proteins it
 *  could be, if that were of interest. )
 *
 * This function always examines all 64 possibilities.  You could
 * optimize it by recognizing specific bit patterns and duplicating
 * the loop bodies, but it probably isn't worth bothering.
 */
static char E_codon(char *s, ZMapGeneticCode genetic_code, int *index_out)
{
  int x, y, z ;
  char it = 0 ;
  int index = -1 ;

  for (x=0 ; x < 4 ; x++)
    {
      if (s[0] & (1<<x))
	{
	  for (y=0 ; y < 4 ; y++)
	    {
	      if (s[1] & (1<<y))
		{
		  for (z=0 ; z < 4 ; z++)
		    {
		      if (s[2] & (1<<z))
			{
			  CodonTranslation amino =
			    &g_array_index(genetic_code->table, CodonTranslationStruct, ((x<<4)|(y<<2)|z)) ;

			  if (!it)
			    {
			      it = amino->amino_acid ;
			      index = ((x<<4)|(y<<2)|z) ;
			    }

			  if (amino->amino_acid != it)
			    {
			      if (index_out)
				*index_out = -1 ;
			      return 'X' ;
			    }
			}
		    }
		}
	    }
	}
    }

  if (!it)						    /*  I think this never happens */
    {
      it = 'X' ;
      index = -1 ;
    }

  if (index_out)
    *index_out = index ;

  return it ;
}


static char E_reverseCodon(char* cp, ZMapGeneticCode genetic_code, int *index_out)
{
  char temp[3] ;

  temp[0] = complementBase[(int)cp[2]] ;
  temp[1] = complementBase[(int)cp[1]] ;
  temp[2] = complementBase[(int)cp[0]] ;

  return E_codon(temp, genetic_code, index_out) ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static char E_antiCodon (char* cp, ZMapGeneticCode genetic_code, int *index_out)
{
  char temp[3] ;

  temp[0] = complementBase[(int)cp[0]] ;
  temp[1] = complementBase[(int)cp[-1]] ;
  temp[2] = complementBase[(int)cp[-2]] ;

  return E_codon(temp, genetic_code, index_out) ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* For debugging, outputs the translation table the code has constructed from the
 * Genetic_code object. */
static void printTranslationTable(KEY genetic_code_key, GArray *translationTable)
{
  int i ;

  printf("Translation table derived from %s\n", nameWithClassDecorate(genetic_code_key)) ;
  printf(" peptides = ") ;
  for (i = 0 ; i < 64 ; i++)
    {
      printf("%c", &g_array_index(translationTable, PepTranslateStruct, i)->amino_acid) ;
    }
  printf("\n") ;

  printf("   starts = ") ;
  for (i = 0 ; i < 64 ; i++)
    {
      printf("%c",
	     (&g_array_index(translationTable, PepTranslateStruct, i)->alternative_start ? 'M' : '-')) ;
    }
  printf("\n") ;

  printf("     stops = ") ;
  for (i = 0 ; i < 64 ; i++)
    {
      printf("%c",
	     (&g_array_index(translationTable, PepTranslateStruct, i)->alternative_stop ? '*' : '-')) ;
    }
  printf("\n") ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



