/*  File: zmapPeptide.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2006
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
 * Description: Routines to translate DNA to peptide, optionally with
 *              an alternative genetic code.
 *
 * Exported functions: See ZMap/zmapPeptide.h
 * HISTORY:
 * Last edited: Mar 15 15:51 2006 (edgrif)
 * Created: Mon Mar 13 11:43:42 2006 (edgrif)
 * CVS info:   $Id: zmapPeptide.c,v 1.1 2006-03-17 17:10:34 edgrif Exp $
 *-------------------------------------------------------------------
 */


/* 
 * 
 * We hereby acknowledge that this code was adapted directly from
 * the dna and peptide functions in acedb (see www.acedb.org).
 * 
 */


#include <ZMap/zmapUtils.h>



/* NOTE to Ed and Roy, there is quite a lot of baggage here that will be sifted out
 * as I arrive at the final set of functions. */



/* This struct represents a peptide translated from a piece of dna. */
typedef struct _ZMapPeptideStruct
{
  GQuark sequence_name ;				    /* As in the FASTA sense. */
  GQuark gene_name ;					    /* If peptide is a gene translation. */
  int start, end ;					    /* e.g. coords of gene in sequence. */
  GArray *peptide ;					    /* The peptide string. */
} ZMapPeptideStruct, *ZMapPeptide ;



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
} PepTranslateStruct, *PepTranslate ;


/* This is the standard genetic code for most organisms, it is used to translate
 * dna into peptides unless an alternative has been specified in the database. */
#define STANDARD_GENETIC_CODE "KNKNIIMIRSRSTTTT*Y*YLFLF*CWCSSSSEDEDVVVVGGGGAAAAQHQHLLLLRRRRPPPP"


static GArray *doDNATranslation(GArray *code_table, GArray *obj_dna,
				gboolean encode, gboolean include_stop) ;
static char E_codon(char *s, GArray *genetic_code, int *index_out) ;
static char E_reverseCodon (char* cp, GArray *genetic_code, int *index_out) ;
static char E_antiCodon (char* cp, GArray *genetic_code, int *index_out) ;

static GArray *pepGetTranslationTable(void) ;
static void dnaEncodeString(char *cp) ;


/*

We use the standard UPAC coding for representing DNA with a single character
per base:

Exactly known bases

A
T  (or U for RNA)
G
C

Double ambiguity

R	AG	~Y		puRine
Y	CT	~R		pYrimidine
M	AC	~K		aMino
K	GT	~M		Keto
S	CG	~S		Strong
W	AT	~w		Weak

Triple ambiguity

H	AGT	~D		not G	
B	CGT	~V		not A
V	ACG	~B		not T
D	AGT	~H		not C

Total ambiguity

N	ACGT	~N		unkNown

Possible existence of a base

-       0       padding

Note that:
We do NOT use U internally, but just use it for display if tag RNA is set

NCBI sometimes uses X in place of N, but we only use X for peptides

The - padding character is used when no sequencing is available and
therefore means that the exact length is unknown, 300 - might not mean
exactly 300 unkown bases.

In memory, we use one byte per base, the lower 4 bits correspond to
the 4 bases A_, T_, G_, C_. More tha one bit is set in the ambiguous cases.
The - padding is represented as zero. In some parts of the code, the upper 
4 bits are used as flags.

On disk, we store the dna as 4 bases per byte, if there is no ambiguity, or
2 bases per byte otherwise.
*/


#define A_ 1
#define T_ 2
#define U_ 2
#define G_ 4
#define C_ 8

#define R_ (A_ | G_)
#define Y_ (T_ | C_)
#define M_ (A_ | C_)
#define K_ (G_ | T_)
#define S_ (C_ | G_)
#define W_ (A_ | T_)

#define H_ (A_ | C_ | T_)
#define B_ (G_ | C_ | T_)
#define V_ (G_ | A_ | C_)
#define D_ (G_ | A_ | T_)

#define N_ (A_ | T_ | G_ | C_)






char complementBase[] =	
 { 0, T_,A_,W_,C_,Y_,M_,H_,G_,K_,R_,D_,S_,B_,V_,N_ } ;


char dnaEncodeChar[] =
{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,

   0,  A_,  B_,  C_,  D_,   0,   0,  G_,  H_,   0,   0,  K_,   0,  M_,  N_,   0,
   0,   0,  R_,  S_,  T_,  U_,  V_,  W_,   0,  Y_,   0,   0,   0,   0,   0,   0,
   0,  A_,  B_,  C_,  D_,   0,   0,  G_,  H_,   0,   0,  K_,   0,  M_,  N_,   0,
   0,   0,  R_,  S_,  T_,  U_,  V_,  W_,   0,  Y_,   0,   0,   0,   0,   0,   0,
} ;






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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
GArray *pepGetTranslationTable(KEY key, KEY *geneticCodep)
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static GArray *pepGetTranslationTable(void)
{
  GArray *translationTable = NULL ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  KEY gKey = KEY_UNDEFINED ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  int index = 0 ;
  static GPtrArray *maps = NULL ;


  /* NOTE lots of temp code in here while I get this working.... */



  /* First time through create cached array of maps. */
  if (!maps)
    {
      GArray *standard_code ;
      int i ;

      maps = g_ptr_array_sized_new(20) ;

      /* zero'th entry is standard genetic code, note this all works because we use
       * KEYKEY of Genetic_code obj to index into array and no keys will have value zero
       * so we are safe to put standard code in zero'th element. */
      standard_code = g_array_sized_new(FALSE, TRUE, sizeof(PepTranslateStruct), PEP_TOTAL_CODONS) ;


      for (i = 0 ; i < PEP_TOTAL_CODONS ; i++)
	{
	  PepTranslate amino = &g_array_index(standard_code, PepTranslateStruct, i) ;
	  
	  amino->amino_acid = STANDARD_GENETIC_CODE[i] ;
	  amino->alternative_start = amino->alternative_stop = FALSE ;
	}

      g_ptr_array_index(maps, 0) = standard_code ;
    }


  /* TEMP CODE... */
  translationTable = g_ptr_array_index(maps, 0) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
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




/* Translate dna (which must be a valid C string) into the corresponding
 * peptide string. If a translation table is supplied then this is used
 * to do the translation, otherwise the Standard Genetic Code is used.
 * If include_stop is TRUE then the peptide will include an "*" to show
 * the terminating stop codon (assuming the dna ends with a stop codon).
 */
ZMapPeptide zMapPeptideCreate(char *sequence_name, char *gene_name,
			      char *dna, GArray *translation_table, gboolean include_stop)
{
  ZMapPeptide pep = NULL ;
  GArray *dna_array ;
  int dna_len ;

  pep = g_new0(ZMapPeptideStruct, 1) ;

  if (sequence_name && *sequence_name)
    pep->sequence_name = g_quark_from_string(sequence_name) ;
  if (gene_name && *gene_name)
    pep->gene_name = g_quark_from_string(gene_name) ;

  dna_len = strlen(dna) ;

  dnaEncodeString(dna) ;

  dna_array = g_array_new(TRUE,
			  FALSE,
			  sizeof(char)) ;

  dna_array = g_array_append_vals(dna_array, dna, dna_len) ;

  if (!translation_table)
    translation_table = pepGetTranslationTable() ;


  pep->peptide = doDNATranslation(translation_table, dna_array, FALSE, include_stop) ;

  return pep ;
}


int zMapPeptideLength(ZMapPeptide peptide)
{
  int length ;

  length = peptide->peptide->len - 1 ;			    /* Remove trailing NULL. */

  return length ;
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




/* This function encapsulates the translation of DNA to peptide and returns either an encoded
 * or decoded peptide array with or without the stop codon (if there is one).
 * The rules for translation are a bit more complex now that alternative start/stop's can be
 * specified. */
static GArray *doDNATranslation(GArray *code_table, GArray *obj_dna,
				gboolean encode, gboolean include_stop)
{
  GArray *pep ;
  int dna_min, dna_max ;
  int bases, pepmax, x, code_table_index ;
  char cc ;
  char str_null = '\0' ;


  zMapAssert(code_table && obj_dna) ;


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
  cc = E_codon(&g_array_index(obj_dna, char, dna_min), code_table, &code_table_index) ;
  if (cc != 'X' && cc != 'M')
    {
      PepTranslate amino = &g_array_index(code_table, PepTranslateStruct, code_table_index) ;
      if (amino->alternative_start)
	cc = 'M' ;
    }
  g_array_append_val(pep, cc) ;


  /* Deal with the rest. */
  x++ ;
  dna_min += 3 ;
  while (x < pepmax)
    {
      cc = E_codon(&g_array_index(obj_dna, char, dna_min), code_table, &code_table_index) ;
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
	  PepTranslate amino = &g_array_index(code_table, PepTranslateStruct, code_table_index) ;
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
static char E_codon(char *s, GArray *genetic_code, int *index_out)
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
			  PepTranslate amino =
			    &g_array_index(genetic_code, PepTranslateStruct, ((x<<4)|(y<<2)|z)) ;

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

static char E_reverseCodon (char* cp, GArray *genetic_code, int *index_out)
{
  char temp[3] ;

  temp[0] = complementBase[(int)cp[2]] ;
  temp[1] = complementBase[(int)cp[1]] ;
  temp[2] = complementBase[(int)cp[0]] ;

  return E_codon(temp, genetic_code, index_out) ;
}

static char E_antiCodon (char* cp, GArray *genetic_code, int *index_out)
{
  char temp[3] ;

  temp[0] = complementBase[(int)cp[0]] ;
  temp[1] = complementBase[(int)cp[-1]] ;
  temp[2] = complementBase[(int)cp[-2]] ;

  return E_codon (temp, genetic_code, index_out) ;
}




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



/* This is not ideal, the acedb code works on encoded strings so we have to convert the
 * dna to this format before translating it. */
static void dnaEncodeString(char *cp)
{

  --cp ;
  while(*++cp)
    *cp = dnaEncodeChar[((int)*cp) & 0x7f] ;

  return ;
}

