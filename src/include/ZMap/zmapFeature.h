/*  File: zmapFeature.h
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Data structures describing a sequence feature.
 *
 * HISTORY:
 * Last edited: Jun  7 12:04 2011 (edgrif)
 * Created: Fri Jun 11 08:37:19 2004 (edgrif)
 * CVS info:   $Id: zmapFeature.h,v 1.195 2011-05-06 14:52:20 mh17 Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_FEATURE_H
#define ZMAP_FEATURE_H

#include <gdk/gdkcolor.h>
#include <ZMap/zmapConfigStyleDefaults.h>
#include <ZMap/zmapXML.h>
#include <ZMap/zmapStyle.h>
#include <ZMap/zmapUtils.h>


/* Some basic macros. */
#define ZMAPFEATURE_FORWARD(FEATURE)       ((FEATURE)->strand == ZMAPSTRAND_FORWARD)
#define ZMAPFEATURE_REVERSE(FEATURE)       ((FEATURE)->strand == ZMAPSTRAND_REVERSE)

#define ZMAPFEATURE_IS_BASIC(FEATURE)      ((FEATURE)->type == ZMAPSTYLE_MODE_BASIC)

#define ZMAPFEATURE_IS_TRANSCRIPT(FEATURE) ((FEATURE)->type == ZMAPSTYLE_MODE_TRANSCRIPT)
#define ZMAPFEATURE_HAS_CDS(FEATURE)       (ZMAPFEATURE_IS_TRANSCRIPT(FEATURE) && \
					    ((FEATURE)->feature.transcript.flags.cds))
#define ZMAPFEATURE_HAS_EXONS(FEATURE)     (ZMAPFEATURE_IS_TRANSCRIPT(FEATURE) &&            \
					    ((FEATURE)->feature.transcript.exons != NULL) && \
					    ((FEATURE)->feature.transcript.exons->len > (guint)0))

#define ZMAPFEATURE_IS_ALIGNMENT(FEATURE)  ((FEATURE)->type == ZMAPSTYLE_MODE_ALIGNMENT)

#define ZMAPFEATURE_SWOP_STRAND(STRAND) \
  ((STRAND) == ZMAPSTRAND_FORWARD ? ZMAPSTRAND_REVERSE :                                       \
   ((STRAND) == ZMAPSTRAND_REVERSE ? ZMAPSTRAND_FORWARD : ZMAPSTRAND_NONE))




/* Macro for the Feature Data class */
#define ZMAP_TYPE_FEATURE_DATA  (zMapFeatureDataGetType())

/* We use GQuarks to give each feature a unique id, the documentation doesn't say, but you
 * can surmise from the code that zero is not a valid quark. */
enum {ZMAPFEATURE_NULLQUARK = 0} ;

enum { ZMAPFEATURE_XML_XREMOTE = 2 };


/* A unique ID for each feature created, can be used to unabiguously query for that feature
 * in a database. The feature id ZMAPFEATUREID_NULL is guaranteed not to equate to any feature
 * and also means you can test using  (!feature_id). */
typedef unsigned int ZMapFeatureID ;
enum {ZMAPFEATUREID_NULL = 0} ;


typedef int Coord ;					    /* we do need this here.... */



typedef enum {ZMAPFEATURE_STRUCT_INVALID = 0,
	      ZMAPFEATURE_STRUCT_CONTEXT, ZMAPFEATURE_STRUCT_ALIGN,
	      ZMAPFEATURE_STRUCT_BLOCK, ZMAPFEATURE_STRUCT_FEATURESET,
	      ZMAPFEATURE_STRUCT_FEATURE} ZMapFeatureStructType ;


/* GET RID OF THIS AND USE OUR ENUM UTILS STUFF. */
/* used by zmapFeatureLookUpEnum() to translate enums into strings */
typedef enum {TYPE_ENUM, STRAND_ENUM, PHASE_ENUM, HOMOLTYPE_ENUM } ZMapEnumType ;



typedef enum {
  ZMAPFEATURE_SUBPART_INVALID    = 0,
  ZMAPFEATURE_SUBPART_INTRON     = 1 << 0,
  ZMAPFEATURE_SUBPART_INTRON_CDS = 1 << 1,
  ZMAPFEATURE_SUBPART_EXON       = 1 << 2,
  ZMAPFEATURE_SUBPART_EXON_CDS   = 1 << 3,
  ZMAPFEATURE_SUBPART_GAP        = 1 << 4,
  ZMAPFEATURE_SUBPART_MATCH      = 1 << 5
} ZMapFeatureSubpartType ;

typedef enum {ZMAPSTRAND_NONE = 0, ZMAPSTRAND_FORWARD, ZMAPSTRAND_REVERSE} ZMapStrand ;

#define FRAME_PREFIX "FRAME-"				    /* For text versions. */
typedef enum {ZMAPFRAME_NONE = 0, ZMAPFRAME_0, ZMAPFRAME_1, ZMAPFRAME_2} ZMapFrame ;

typedef enum {ZMAPPHASE_NONE = 0, ZMAPPHASE_0, ZMAPPHASE_1, ZMAPPHASE_2} ZMapPhase ;

/* as in BLAST*, i.e. target is DNA, Protein, DNA translated */
typedef enum {ZMAPHOMOL_NONE = 0, ZMAPHOMOL_N_HOMOL, ZMAPHOMOL_X_HOMOL, ZMAPHOMOL_TX_HOMOL} ZMapHomolType ;


/* Used to specify the degree of colinearity between two alignment blocks. */
typedef enum
  {
    COLINEAR_INVALID,
    COLINEAR_NOT,					    /* blocks not colinear. */
    COLINEAR_IMPERFECT,					    /* blocks colinear but not contiguous. */
    COLINEAR_PERFECT					    /* blocks colinear and contiguous. */
  } ColinearityType ;



typedef enum {ZMAPBOUNDARY_NONE = 0, ZMAPBOUNDARY_CLONE_END,
	      ZMAPBOUNDARY_5_SPLICE, ZMAPBOUNDARY_3_SPLICE } ZMapBoundaryType ;

typedef enum {ZMAPSEQUENCE_NONE = 0, ZMAPSEQUENCE_DNA, ZMAPSEQUENCE_PEPTIDE} ZMapSequenceType ;

typedef enum {ZMAPFEATURELENGTH_TARGET, ZMAPFEATURELENGTH_QUERY, ZMAPFEATURELENGTH_SPLICED} ZMapFeatureLengthType ;

typedef enum
  {
    ZMAPFEATURE_DUMP_NO_FUNCTION,
    ZMAPFEATURE_DUMP_UNKNOWN_FEATURE_TYPE
  } ZMapFeatureDumpError;


/* Return values from feature context merge. */
typedef enum
  {
    ZMAPFEATURE_CONTEXT_OK,				    /* Merge worked. */
    ZMAPFEATURE_CONTEXT_ERROR,				    /* Some bad error, e.g. bad input args. */
    ZMAPFEATURE_CONTEXT_NONE				    /* No new features so nothing merged. */
  } ZMapFeatureContextMergeCode ;




/* Holds dna or peptide.
 * Note that the sequence will be a valid string in that it will be null-terminated,
 * "length" does _not_ include the null terminator. */
typedef struct ZMapSequenceStruct_
{
  GQuark name ;						    /* optional name (zero if no name). */
  ZMapSequenceType type ;				    /* dna or peptide. */
  ZMapFrame frame ;					    /* Where possible dna frame from which
							     * peptide translated. */
  int length ;						    /* length of sequence in bases or peptides. */
  char *sequence ;					    /* Actual sequence." */
} ZMapSequenceStruct, *ZMapSequence ;



/* Could represent anything that has a span. */
typedef struct
{
  Coord x1, x2 ;
} ZMapSpanStruct, *ZMapSpan ;


/* We have had this but would like to have a simple "span" sort....as below... */
typedef struct
{
  Coord x1, x2 ;
} ZMapExonStruct, *ZMapExon ;



/* Assumes x1 <= x2. */
#define ZMAP_SPAN_LENGTH(SPAN_STRUCT)                 \
  (((SPAN_STRUCT)->x2 - (SPAN_STRUCT)->x1) + 1)



/* This is a kind of "annotated" exon struct as it contains a lot more useful info. than
 * just the span, all derived from the the transcript feature.
 *
 * Note that for EXON_NON_CODING none of the cds/peptide stuff is populated. */
typedef enum {EXON_INVALID, EXON_NON_CODING, EXON_CODING, EXON_SPLIT_CODON, EXON_START_NOT_FOUND} ExonRegionType ;

typedef struct
{
  ExonRegionType region_type ;

  /* Various coords of the exon region in different contexts. */
  ZMapSpanStruct sequence_span ;			    /* coords on reference sequence. */
  ZMapSpanStruct unspliced_span ;			    /* 1-based version of sequence_span. */
  ZMapSpanStruct spliced_span ;				    /* 1-based coords in spliced transcript. */
  ZMapSpanStruct cds_span ;				    /* 1-based coords within cds of transcript. */
  ZMapSpanStruct pep_span ;				    /* coords within peptide translation of cds. */

  /* Start and end phases of a cds. */
  int start_phase ;
  int end_phase ;

  char *peptide ;					    /* Optionally carries peptide translation. */

} ZMapFullExonStruct, *ZMapFullExon ;



/* the following is used to store alignment gap information */
typedef struct
{
  int q1, q2 ;						    /* coords in query sequence */
  ZMapStrand q_strand ;
  int t1, t2 ;						    /* coords in target sequence */
  ZMapStrand t_strand ;
} ZMapAlignBlockStruct, *ZMapAlignBlock ;



/* Probably better to use  "input" and "output" coords as terms, parent/child implies
 * a particular relationship... */
/* the following is used to store mapping information of one span on to another, if we have
 * SMap in ZMap we can use SMap structs instead....BUT strand is needed here too !!!!! */
typedef struct
{
      /* NOTE even if reversed coords are as start < end */
  ZMapSpanStruct parent;          /* start/end in parent span (context) */
  ZMapSpanStruct block;             /* start,end in align, aka child seq */
      /* NOTE for a single align parent and block coords will be the same
       * if another align exists then block is the coords in that align,
       * parent is the related master_align coords
       */
  gboolean reversed;
} ZMapMapBlockStruct, *ZMapMapBlock ;


/*
 * Sequences and Block Coordinates
 * NOTE with moving to chromosome coords this is a bit out of date
 * will update this comment when I've worked out what to do.
 * NOTE refer to docs/IntWeb/Design/notes/coord_config.shtml and ignore the below
 *
 * In the context of displaying a single align with a single block,
 * given a chromosome and a sequence from that to look at *we have:
 *
 *  1                           X      chromosome bases
 *  =============================      ZMapFeatureContext.parent_span.x1/x1
 *
 *     1                   Y           our sequence
 *     =====================           ZMapFeatureContext.sequence_to_parent
 *                                       p1,p2 = 1,X and c1,c2 = 1,Y
 *
 *        S          E                 displayed block
 *        ============                 ZMapFeatureBlockStruct.block_to_sequence
 *                                       q1,q2 = 1,Y and t1,t2 = S,E
 *                                       q_strand, t_strand
 *
 * We should always have parent_span set up, if not specified then it will be set to 1,Y.
 * ZMap will always be run using a sequence of 1,Y; it will never start from a coordinate not equal to 1.
 * Y may be specified as 0 in various places which signifies 'the end'.
 *
 * Start and end coordinates may be specified by command line args, from the main window,
 * from a GFF file header, from other servers (eg ACEDB, DAS) and these will refer to the block
 * coordinates not the sequence.
 *
 * Reverse complementing is a process that applies to blocks and this involves transforming
 * the coordinates S,E in the context of the containing sequence 1,Y
 */





/* DO THESE STRUCTS NEED TO BE EXPOSED ? PROBABLY NOT....TO HIDE THEM WOULD REQUIRE
 * QUITE A NUMBER OF ACCESS FUNCTIONS.... */



/* We need some forward declarations for pointers we include in some structs. */

typedef struct ZMapFeatureAlignmentStruct_ *ZMapFeatureAlignment ;

typedef struct ZMapFeatureAnyStruct_ *ZMapFeatureAny ;

#define FEATURES_NEED_MAGIC


/* WARNING: READ THIS BEFORE CHANGING ANY FEATURE STRUCTS:
 *
 * This is the generalised feature struct which can be used to process any feature struct.
 * It helps with certain kinds of processing, e.g. in the hash searching code which relies
 * on these fields being common to all feature structs.
 *
 * unique_id is used for the hash table that connects any feature struct to the canvas item
 * that represents it.
 *
 * original_id is used for displaying a human readable name for the struct, e.g. the feature
 * name.
 *  */
typedef struct ZMapFeatureAnyStruct_
{
#ifdef FEATURES_NEED_MAGIC
  ZMapMagic magic;
#endif
  ZMapFeatureStructType struct_type ;			    /* context or align or block etc. */
  ZMapFeatureAny parent ;				    /* The parent struct of this one, NULL
							     * if this is a feature context. */
  GQuark unique_id ;					    /* Unique id of this feature. */
  GQuark original_id ;					    /* Original id of this feature. */

  GHashTable *children ;				    /* Child objects, e.g. aligns, blocks etc. */
} ZMapFeatureAnyStruct ;



/* Holds a set of data that is the complete "view" of the requested sequence.
 *
 * This includes all the alignments which in the case of the original "fmap" like view
 * will be a single alignment containing a single block.
 *
 */
typedef struct ZMapFeatureContextStruct_
{
  /* FeatureAny section. */
#ifdef FEATURES_NEED_MAGIC
  ZMapMagic magic;
#endif

  ZMapFeatureStructType struct_type ;			    /* context or align or block etc. */
  ZMapFeatureAny no_parent ;				    /* Always NULL in a context. */
  GQuark unique_id ;					    /* Unique id of this feature. */
  GQuark original_id ;					    /* Sequence name. */
  GHashTable *alignments ;				    /* All the alignments for this zmap
							       as a set of ZMapFeatureAlignment. */


  /* Context only data. */


  /* Hack...forced on us because GHash has a global destroy function per hash table, not one
   * per element of the hashtable (datalists have one per node but they have their own
   * problems as they are slow). There are two ways a context can be freed
   *
   * See the write up for the zMapFeatureContextMerge() for how this is used.
   */
  gboolean diff_context ;				    /* TRUE means this is a diff context. */
  GHashTable *elements_to_destroy ;			    /* List of elements that we copied
							       which need to be destroyed. */


  GQuark sequence_name ;				    /* The sequence to be displayed. */

  GQuark parent_name ;					    /* Name of parent sequence
							       (== sequence_name if no parent). */

  ZMapSpanStruct parent_span ;                      /* Start/end of ultimate parent, usually we
                                                      will have: x1 = 1, x2 = length in
                                                      bases of parent. */
  GList *req_feature_set_names ;	            /* Global list of _names_ of all requested
							       * feature sets for the context.
                                                 * for ACEDB these are given as columns
                                                 * and are returned as featuresets
							       */
  GList *src_feature_set_names ;                  /* Global list of _names_ of all source
                                                 feature sets actually in the context,
                                                 _only_ these sets are loaded into
                                                 the context. */

  ZMapFeatureAlignment master_align ;			    /* The target/master alignment out of
							       the below set. */

} ZMapFeatureContextStruct, *ZMapFeatureContext ;



typedef struct ZMapFeatureAlignmentStruct_
{
  /* FeatureAny section. */
#ifdef FEATURES_NEED_MAGIC
  ZMapMagic magic;
#endif
  ZMapFeatureStructType struct_type ;			    /* context or align or block etc. */
  ZMapFeatureAny parent ;				    /* Our parent context. */
  GQuark unique_id ;					    /* Unique id this alignment. */
  GQuark original_id ;					    /* Original id of this sequence. */
  GHashTable *blocks ;					    /* A set of ZMapFeatureStruct. */

  /* Alignment only data should go here. */

  /* Mapping for the target sequence, this shows where this section of sequence fits in to its
   * overall assembly, e.g. where a clone is located on a chromosome. */
  ZMapSpanStruct sequence_span;                       /* start/end of our sequence */

} ZMapFeatureAlignmentStruct;



typedef struct ZMapFeatureBlockStruct_
{
  /* FeatureAny section. */
#ifdef FEATURES_NEED_MAGIC
  ZMapMagic magic;
#endif
  ZMapFeatureStructType struct_type ;	/* context or align or block etc. */
  ZMapFeatureAny parent ;				    /* Our parent alignment. */
  GQuark unique_id ;					    /* Unique id for this block. */
  GQuark original_id ;					    /* Original id, probably not needed ? */
  GHashTable *feature_sets ;				    /* The feature sets for this block as a
							       set of ZMapFeatureSetStruct. */

  /* Block only data. */
  ZMapMapBlockStruct block_to_sequence ;	/* Shows how these features map to the
							       sequence, n.b. this feature set may only
							       span part of the sequence. */
  ZMapSequenceStruct sequence ;	      /* DNA sequence for this block,
							       n.b. there may not be any dna. */

  gboolean revcomped;                     /* block RevComp'd relative to the window */

//  int features_start, features_end ;    /* coord limits for fetching features. */

} ZMapFeatureBlockStruct, *ZMapFeatureBlock ;




/*!\struct ZMapFeatureSetStruct_
 * \brief a set of ZMapFeature structs.
 * Holds a set of ZMapFeature structs, note that the id for the set is by default the same name
 * as the style for all features in the set. BUT it may not be, the set may consist of
 * features with many types/styles. The set id is completely independent of the style name.
 */
typedef struct ZMapFeatureSetStruct_
{
  /* FeatureAny section. */
#ifdef FEATURES_NEED_MAGIC
  ZMapMagic magic;
#endif
  ZMapFeatureStructType struct_type ;			    /* context or align or block etc. */
  ZMapFeatureAny parent ;				    /* Our parent block. */
  GQuark unique_id ;					    /* Unique id of this feature set. */
  GQuark original_id ;					    /* Original name,
							       e.g. "Genewise predictions" */

  GHashTable *features ; 				    /* The features for this set as a
							       set of ZMapFeatureStruct. */
  char *description ;					    /* As it says... */

  ZMapFeatureTypeStyle style;

      /* NB we don't expect to use both these on the same featureset but play safe... */
  GList *masker_sorted_features;    /* or NULL if not sorted */

  GList *loaded;              /* strand and end coordinate pairs in numerical order of start coord */
                              /* we use ZMapSpanStruct (x1,x2) to hold this */
                              /* NOTE:may be null after context merge into view context */

} ZMapFeatureSetStruct, *ZMapFeatureSet ;




/*
 * The Feature struct.
 *
 * Feature subtypes, homologies, transcripts, etc come first and then the ZMapFeatureStruct
 * in which they are embedded.
 *
 */


/* Basic feature: a "box" on the screen. */
typedef struct
{
  /* Used to detect when data fields are set. */
  struct
  {
    unsigned int variation_str : 1 ;
  } has_attr ;


  GQuark known_name ;					    /* Known or external name for feature. */

  char *variation_str ;					    /* e.g. "A/T" for SNP etc. */

} ZMapBasicStruct, *ZMapBasic ;



/* Homology feature. */
typedef struct
{
  ZMapHomolType type ;					    /* as in Blast* */

  int length ;						    /* Length of homol/align etc. */

  /* Quality measures. (NEED TO GET SCORE IN HERE) */
  float percent_id ;


  GQuark clone_id ;					    /* Clone this match is aligned to. */

  /* Coords are _always_ for the forward strand of the match sequence and always x1 <= x2,
   * strand shows which strand is aligned. */
  int y1, y2 ;						    /* Query start/end */
  ZMapStrand strand ;					    /* Which strand of the homol was
							       aligned to the sequence. */

  ZMapPhase target_phase ;				    /* for tx_homol */


  /* The coords in this array are start/end pairs for sub blocks and start < end always. */
  GArray *align ;					    /* of AlignBlock, if null, align is ungapped. */

  struct
  {
    /* If align != NULL and perfect == TRUE then gaps array is a "perfect"
     * alignment with allowance for a style specified slop factor. */
    gboolean perfect;

    gboolean has_sequence;			    /* This homology has sequence in the database. */

    gboolean has_clone_id;			    /* This homol feature is matched to this clone. */

    gboolean masked;                          /* flagged for no-display
                                               * for an EST - is it covered completely by an mRNA
                                               */
    gboolean displayed;                       /* is it in the foo canvas right now ? */

  } flags ;

} ZMapHomolStruct, *ZMapHomol ;


/* Transcript feature. */
typedef struct
{
  GQuark known_name ;					    /* Known or external name for transcript. */

  GQuark locus_id ;					    /* Locus this transcript belongs to. */

  /* If cds == TRUE, then these must show the position of the cds in sequence coords... */
  Coord cds_start, cds_end ;

  /* If start of cds is known to be missing this gives start position in cds for translation,
   * i.e. it must have a value in the range 1 -> 3. */
  int start_not_found ;

  GArray *exons ;					    /* Of ZMapSpanStruct. */
  GArray *introns ;					    /* Of ZMapSpanStruct. */

  struct
  {
    unsigned int cds : 1 ;
    unsigned int start_not_found : 1 ;
    unsigned int end_not_found : 1 ;
  } flags ;

} ZMapTranscriptStruct, *ZMapTranscript ;


/* Assembly feature. */
typedef struct
{
  /* May not need this.... */
  GQuark clone_id ;					    /* Clone name. */

  ZMapStrand strand ;					    /* Which strand the assembly came from. */

  int length ;						    /* Length of Clone. */

  GArray *path ;					    /* of ZMapSpanStruct, if NULL then
							       even though this clone overlaps the
							       assembly none of it is used. */

} ZMapAssemblyPathStruct, *ZMapAssemblyPath ;


/* The Feature structure itsself. Describes a single feature, the feature may be compound
 * (e.g. have exons/introns *  etc.) or a single span or point, e.g. an allele.
 *  */
typedef struct ZMapFeatureStruct_
{
  /* FeatureAny section. */
#ifdef FEATURES_NEED_MAGIC
  ZMapMagic magic;
#endif
  ZMapFeatureStructType struct_type ;			    /* context or align or block etc. */
  ZMapFeatureAny parent ;				    /* Our containing set. */
  GQuark unique_id ;					    /* Unique id for just this feature for
							       use by ZMap. */
  GQuark original_id ;					    /* Original name, e.g. "bA404F10.4.mRNA" */
  GHashTable *no_children ;				    /* Should always be NULL. */



  /* Feature only data. */


  /* OK, THIS IS THE WRONG APPROACH, WE ARE EXPANDING ALL FEATURE STRUCTS WITH THIS
   * STUFF WHEN ONLY A PREDICTABLE FEW WILL NEED THESE....MOVE INTO BASIC.... */
  /* flags field holds extra information about various aspects of the feature. */
  /* I'm going to try the bitfields syntax here.... */
  struct
  {
    unsigned int has_score : 1 ;
    unsigned int has_boundary : 1 ;
  } flags ;



  ZMapFeatureID db_id ;					    /* unique DB identifier, currently
							       unused but will be..... */

  ZMapStyleMode type ;					    /* Basic, transcript, alignment. */



  /* This type identifies the sort of feature this is. We are using SO because it's enough
   * for this program, it includes terms like mRNA, polyA_site etc. We store the accession
   * because it is imutable whereas the term could change. */
  GQuark SO_accession ;


  // style id tp be removed when style fully working
  GQuark style_id ;					    /* Style defining how this feature is processed.
							       (use Styles _unique_ id.) */

  ZMapFeatureTypeStyle style;                   // pointer to the style structure


  /* coords are _always_ with reference to forward strand, i.e. x1 <= x2, strand flag gives the
   * strand of the feature. */
  Coord x1, x2 ;
  ZMapStrand strand ;

  ZMapBoundaryType boundary_type ;			    /* splice, clone end ? */


  /* MOVE THIS...WE ONLY NEED IT IN SOME FEATURES....NEEDS SOME RESEARCH BECAUSE WHILE
   * THIS IS BASICALLY AN ALIGNMENT THING THERE MAY BE OTHER FEATURES THAT HAVE SCORES...
   *
   * e.g. TRANSCRIPTS WILL NEED SCORE BECAUSE THEY MAY BE PRODUCED BY PREDICTION PROGRAMS...
   *  */
  float score ;

  /* Source name and text, gives information about all features of a particular type. */
  GQuark source_id ;
  GQuark source_text ;

  char *description ;					    /* notes for this feature. */

  char *url ;						    /* Could be a quark but we would need to
							       to use our own table otherwise
							       memory usage will be too high. */

  /* Feature specific data, keyed from type in mode in style....errr, doesn't really work ? */
  union
  {
    ZMapBasicStruct        basic ;
    ZMapHomolStruct        homol ;
    ZMapTranscriptStruct   transcript ;
    ZMapSequenceStruct     sequence   ;
    ZMapAssemblyPathStruct assembly_path ;
  } feature ;

} ZMapFeatureStruct, *ZMapFeature ;



/* Struct that supplies text descriptions of the parts of a feature, which fields are filled
 * in depends on the feature type. */
typedef struct
{
  /* Use these fields to interpret and give more info. for the feature parts. */
  ZMapFeatureStructType struct_type ;
  ZMapStyleMode type ;
  ZMapFeatureSubpartType subpart_type ;	/* want to remove so that the strings are self describing */

  /* general feature details (all strings) */
  char *feature_name ;
  char *feature_known_name ;
  char *feature_total_length ;				    /* e.g. length of _whole_ match sequence. */
  char *feature_term ; /* This feature's style (was feature_style)
			* "ZMAPSTYLE_MODE_TRANSCRIPT" isn't
			* very helpful, changed in favour of
			* "Transcript" (could add feature_so_term) */

  char *feature_start ;
  char *feature_end ;
  char *feature_length ;
  char *feature_strand ;
  char *feature_frame ;

  char *feature_query_start ;
  char *feature_query_end ;
  char *feature_query_length ;
  char *feature_query_strand ;


  /* sub feature details (still all strings) */
  char *sub_feature_index ;
  char *sub_feature_start ;
  char *sub_feature_end ;
  char *sub_feature_query_start ;
  char *sub_feature_query_end ;
  char *sub_feature_length ;
  char *sub_feature_none_txt ; /* If no subfeature, gives reason.... */
  char *sub_feature_term;     /* Avoid monkeying all over the shop. */

  /* more specific details (more strings) */
  char *feature_score ;
  char *feature_percent_id ;

  char *feature_type ;
  char *feature_description ;

  /* Parent featureset in the feature context. */
  char *feature_set ;
  char *feature_set_description ;

  /* Original source of this feature (e.g. wublast, genefinder etc.). */
  char *feature_source ;
  char *feature_source_description ;

  /* some transcripts have locus information too. */
  char *feature_locus ;

} ZMapFeatureDescStruct, *ZMapFeatureDesc ;



typedef struct
{
  /* I'm not completely sure this is a good idea but somehow we do need to be able to find out
   * whether something is in intron/exon or whatever.... */

  /* Having thought about it, it's pretty much essential without
   * altering the features to have addressable spans, which themselves
   * need a type too. The previous version of this struct was in the
   * ZMapWindow code, but a couple of ZMapFeature calls need to know
   * about subpart types, start and end too, so it's moved here. */

  ZMapFeatureSubpartType subpart ;			    /* Exon, Intron etc. */

  int index ;						    /* Index number of intron/exon
							       etc. starts at 1. */

  int start, end ;					    /* start/end of subpart in sequence coords. */

} ZMapFeatureSubPartSpanStruct, *ZMapFeatureSubPartSpan ;





/* stuff to handle GFF input and config stanzas that map featurests styles and columns */

/* Struct for "feature set" information. Used to look up "meta" information for each feature set. */
typedef struct
{
      // really need to change feature_set to column: it's confusing
  GQuark column_id ;           /* The set name. (the display column) as a key value*/
  GQuark column_ID ;           /* The set name. (the display column) as display text*/

  GQuark feature_src_ID;            // the name of the source featureset (with upper case)
                                    // struct is keyed with normalised name
  char *feature_set_text;           // renamed so we can search for this

} ZMapFeatureSetDescStruct, *ZMapFeatureSetDesc ;


/* all the info about a display column */
/* NOTE these are logical columns, real display columns get munged with strand and frame */
typedef struct
{
  GQuark unique_id ;                /* column name as a key value*/
  GQuark column_id ;                /* column name as display text*/

  char *column_desc;                /* description */

  int order;                        // column ordering

  ZMapFeatureTypeStyle style;       /* column specific style data
                                     * may be config'd explicitly or derived from contained featuresets
                                     */
  GQuark style_id;                  /* this can be set before we get the style itself */

  GList *style_table;               /* all the styles needed by the column */

  GList * featuresets;              /* list of those configured
                                     * these get filled in when servers request featureset-names
                                     * for pipe servers we could do this during server config
                                     * but for ACE (and possibly DAS) we have to wait till they provide data
                                     */
/* performance betterment
   generate with zMapFeatureGetColumnFeatureSets() after config and update if a server provides mapping */
/* change all code around that function */

} ZMapFeatureColumnStruct, *ZMapFeatureColumn ;




/* Struct holding "per source" information for GFF data. Can be used to look up the
 * style for a GFF feature plus other stuff. */
typedef struct
{
  GQuark source_id ;    /* The source name. From ACE this is the key used to ref this struct */
                        /* but we can config an alternate name (requested by graham) */

  GQuark source_text ;  /* Description. */

  GQuark style_id ;     /* The style for processing the source. */

} ZMapFeatureSourceStruct, *ZMapFeatureSource ;


/* all the featureset mapping data - used by view and window */

typedef struct
{
  GHashTable *styles;                     /* All the styles know to the view or window */

  GHashTable *column_2_styles ;           /* Mapping of each column to all the styles
                                           * it requires. using a GHashTable of
                                           * GLists of style quark id's.
                                           *
                                           * NB: this stores data from ZMap config
                                           * sections [featureset_styles] _and_ [column_styles]
                                           * _and_ ACEDB
                                           * collisions are merged
                                           * Columns treated as fake featuresets so as to have a style
                                           */

  GHashTable *featureset_2_column ;       /* Mapping of a feature source to a column using ZMapFeatureDesc
                                           * NB: this contains data from ZMap config
                                           * sections [columns] [featureset_description] _and_ ACEDB
                                           */

  GHashTable *source_2_sourcedata ;       /* Mapping of a feature source to its data using ZMapFeatureSource
                                           * This consists of style id and description and source id
                                           * NB: the GFFSource.source  (quark) is the GFF_source name
                                           * the hash table is indexed by the featureset name quark
                                           */

  GHashTable *columns;                    /* All the columns that ZMap will display
                                           * stored as ZMapFeatureColumn
                                           * These may contain several featuresets each
                                           * They are in display order left to right
                                           */

  GList * seq_data_featuresets;           /* sources of BAM data */


} ZMapFeatureContextMapStruct, *ZMapFeatureContextMap;



/* Holds a sequence to be fetched, in the end this will include aligns/blocks etc. */
/* MH17: moved from zmapView.h */
/* currently (Apr 2011) this is used for the 'default-sequence' or the one loeaded vuia Otterlace */
/* multiple aligns must also include this if used */
typedef struct
{
  char *dataset ;       /* eg human */
  char *sequence ;      /* eg chr6-18 */
  int start, end ;      /* chromosome coordinates */
} ZMapFeatureSequenceMapStruct, *ZMapFeatureSequenceMap ;



typedef enum
  {
    ZMAP_CONTEXT_EXEC_STATUS_OK           = 0,
    ZMAP_CONTEXT_EXEC_STATUS_OK_DELETE    = 1 << 0,
    ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND = 1 << 1,
    ZMAP_CONTEXT_EXEC_STATUS_ERROR        = 1 << 2
  } ZMapFeatureContextExecuteStatus;



typedef ZMapFeatureContextExecuteStatus (*ZMapGDataRecurseFunc)(GQuark   key_id,
                                                                gpointer list_data,
                                                                gpointer user_data,
                                                                char   **error);

typedef gboolean (*ZMapFeatureDumpFeatureFunc)(ZMapFeatureAny feature_any,
					       GHashTable    *styles,
					       GString       *dump_string_in_out,
					       GError       **error,
					       gpointer       user_data);


/* FeatureAny funcs. */

ZMapFeatureAny zMapFeatureAnyCreate(ZMapStyleMode feature_type) ;
ZMapFeatureAny zMapFeatureAnyCopy(ZMapFeatureAny orig_feature_any) ;
gboolean zMapFeatureAnyFindFeature(ZMapFeatureAny feature_set, ZMapFeatureAny feature) ;
ZMapFeatureAny zMapFeatureAnyGetFeatureByID(ZMapFeatureAny feature_set, GQuark feature_id) ;
gboolean zMapFeatureAnyAddModesToStyles(ZMapFeatureAny feature_any, GHashTable *styles) ;
gboolean zMapFeatureAnyRemoveFeature(ZMapFeatureAny feature_set, ZMapFeatureAny feature) ;
void zMapFeatureAnyDestroy(ZMapFeatureAny feature) ;

void zMapCoords2FeatureCoords(ZMapFeatureBlock block, int *x1_inout, int *x2_inout) ;



/* used by item factory */
typedef struct _zmapFeatureStack
{
      ZMapFeatureContext context;
      ZMapFeatureAlignment align;
      ZMapFeatureBlock block;
      ZMapFeatureSet set;
      ZMapFeature feature;
      GQuark id;        /* used for density plots, set to zero */
} ZMapFeatureStackStruct, *ZMapFeatureStack;


/* ***************
 * FEATURE METHODS
 */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
char *zMapFeatureCreateName(ZMapFeatureType feature_type,
                            char *feature_name,
			    ZMapStrand strand,
                            int start, int end,
                            int query_start, int query_end) ;
GQuark zMapFeatureCreateID(ZMapFeatureType feature_type,
                           char *feature_name,
			   ZMapStrand strand,
                           int start, int end,
			   int query_start, int query_end) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
char *zMapFeatureCreateName(ZMapStyleMode feature_type,
                            char *feature_name,
			    ZMapStrand strand,
                            int start, int end,
                            int query_start, int query_end) ;
GQuark zMapFeatureCreateID(ZMapStyleMode feature_type,
                           char *feature_name,
			   ZMapStrand strand,
                           int start, int end,
			   int query_start, int query_end) ;



ZMapFeature zMapFeatureCreateEmpty(void) ;
ZMapFeature zMapFeatureCreateFromStandardData(char *name, char *sequence, char *ontology,
					      ZMapStyleMode feature_type,
                                              ZMapFeatureTypeStyle style,
                                              int start, int end,
                                              gboolean has_score, double score,
					      ZMapStrand strand) ;

gboolean zMapFeatureAddStandardData(ZMapFeature feature, char *feature_name_id, char *name,
				    char *sequence, char *ontology,
				    ZMapStyleMode feature_type,
				    ZMapFeatureTypeStyle style,
				    int start, int end,
				    gboolean has_score, double score,
				    ZMapStrand strand) ;

gboolean zMapFeatureAddKnownName(ZMapFeature feature, char *known_name) ;
gboolean zMapFeatureAddSplice(ZMapFeature feature, ZMapBoundaryType boundary) ;
gboolean zMapFeatureAddTranscriptData(ZMapFeature feature,
				      gboolean cds, Coord cds_start, Coord cds_end,
				      GArray *exons, GArray *introns) ;
gboolean zMapFeatureAddTranscriptStartEnd(ZMapFeature feature,
					  gboolean start_not_found_flag, int start_not_found,
					  gboolean end_not_found_flag) ;
gboolean zMapFeatureAddTranscriptExonIntron(ZMapFeature feature,
					    ZMapSpanStruct *exon, ZMapSpanStruct *intron) ;
void zMapFeatureTranscriptExonForeach(ZMapFeature feature, GFunc function, gpointer user_data);
gboolean zMapFeatureAddAlignmentData(ZMapFeature feature,
				     GQuark clone_id,
				     double percent_id,
				     int query_start, int query_end,
				     ZMapHomolType homol_type,
				     int query_length,
				     ZMapStrand query_strand,
				     ZMapPhase target_phase,
				     GArray *gaps, unsigned int align_error,
				     gboolean has_local_sequence) ;
gboolean zMapFeatureAlignmentIsGapped(ZMapFeature feature) ;

gboolean zMapFeatureSequenceSetType(ZMapFeature feature, ZMapSequenceType type) ;
gboolean zMapFeatureSequenceIsDNA(ZMapFeature feature) ;
gboolean zMapFeatureSequenceIsPeptide(ZMapFeature feature) ;

gboolean zMapFeatureAddAssemblyPathData(ZMapFeature feature,
					int length, ZMapStrand strand, GArray *path) ;
gboolean zMapFeatureAddSOaccession(ZMapFeature feature, GQuark SO_accession) ;
gboolean zMapFeatureSetCoords(ZMapStrand strand, int *start, int *end,
			      int *query_start, int *query_end) ;
void zMapFeature2MasterCoords(ZMapFeature feature, double *feature_x1, double *feature_x2) ;
void zMapFeature2BlockCoords(ZMapFeatureBlock block, int *x1_inout, int *x2_inout) ;
void zMapBlock2FeatureCoords(ZMapFeatureBlock block, int *x1_inout, int *x2_inout) ;

void zMapFeatureContextReverseComplement(ZMapFeatureContext context, GHashTable *styles) ;
void zMapFeatureReverseComplement(ZMapFeatureContext context, ZMapFeature feature) ;
void zMapFeatureReverseComplementCoords(ZMapFeatureBlock block, int *start_inout, int *end_inout) ;

ZMapFrame zMapFeatureFrame(ZMapFeature feature) ;
gboolean zMapFeatureAddVariationString(ZMapFeature feature, char *variation_string) ;
gboolean zMapFeatureAddURL(ZMapFeature feature, char *url) ;
gboolean zMapFeatureAddLocus(ZMapFeature feature, GQuark locus_id) ;
gboolean zMapFeatureAddText(ZMapFeature feature, GQuark source_id, char *source_text, char *feature_text) ;
void     zMapFeatureSortGaps(GArray *gaps) ;
int      zMapFeatureLength(ZMapFeature feature, ZMapFeatureLengthType length_type) ;
void     zMapFeatureDestroy(ZMapFeature feature) ;


/* *******************
 * FEATURE SET METHODS
 */
GQuark zMapFeatureSetCreateID(char *feature_set_name) ;
ZMapFeatureSet zMapFeatureSetCreate(char *source, GHashTable *features) ;
ZMapFeatureSet zMapFeatureSetIDCreate(GQuark original_id, GQuark unique_id,
				      ZMapFeatureTypeStyle style, GHashTable *features) ;
gboolean zMapFeatureSetAddFeature(ZMapFeatureSet feature_set, ZMapFeature feature) ;
gboolean zMapFeatureSetFindFeature(ZMapFeatureSet feature_set, ZMapFeature feature) ;
ZMapFeature zMapFeatureSetGetFeatureByID(ZMapFeatureSet feature_set,
                                         GQuark feature_id);
gboolean zMapFeatureSetRemoveFeature(ZMapFeatureSet feature_set, ZMapFeature feature) ;
void zMapFeatureSetDestroyFeatures(ZMapFeatureSet feature_set) ;
void     zMapFeatureSetDestroy(ZMapFeatureSet feature_set, gboolean free_data) ;
void  zMapFeatureSetStyle(ZMapFeatureSet feature_set, ZMapFeatureTypeStyle style) ;
char *zMapFeatureSetGetName(ZMapFeatureSet feature_set) ;
GList *zMapFeatureSetGetRangeFeatures(ZMapFeatureSet feature_set, int start, int end) ;
GList *zMapFeatureSetGetNamedFeatures(ZMapFeatureSet feature_set, GQuark original_id) ;


gboolean zMapFeatureSetIsLoadedInRange(ZMapFeatureBlock block, GQuark unique_id,int start, int end);


/* *********************
 * FEATURE BLOCK METHODS
 */
GQuark zMapFeatureBlockCreateID(int ref_start, int ref_end, ZMapStrand ref_strand,
                                int non_start, int non_end, ZMapStrand non_strand);
gboolean zMapFeatureBlockDecodeID(GQuark id,
                                  int *ref_start, int *ref_end, ZMapStrand *ref_strand,
                                  int *non_start, int *non_end, ZMapStrand *non_strand);
ZMapFeatureBlock zMapFeatureBlockCreate(char *block_seq,
					int ref_start, int ref_end, ZMapStrand ref_strand,
					int non_start, int non_end, ZMapStrand non_strand) ;
gboolean zMapFeatureBlockSetFeaturesCoords(ZMapFeatureBlock feature_block,
					   int features_start, int features_end) ;
gboolean zMapFeatureBlockAddFeatureSet(ZMapFeatureBlock feature_block, ZMapFeatureSet feature_set) ;
gboolean zMapFeatureBlockFindFeatureSet(ZMapFeatureBlock feature_block,
                                        ZMapFeatureSet   feature_set);
ZMapFeatureSet zMapFeatureBlockGetSetByID(ZMapFeatureBlock feature_block,
                                          GQuark set_id) ;
gboolean zMapFeatureBlockRemoveFeatureSet(ZMapFeatureBlock feature_block,
                                          ZMapFeatureSet   feature_set);
void zMapFeatureBlockDestroy(ZMapFeatureBlock block, gboolean free_data) ;

gboolean zMapFeatureBlockDNA(ZMapFeatureBlock block,
                             char **seq_name, int *seq_len, char **sequence) ;


/* *************************
 * FEATURE ALIGNMENT METHODS
 */
GQuark zMapFeatureAlignmentCreateID(char *align_sequence, gboolean master_alignment) ;
ZMapFeatureAlignment zMapFeatureAlignmentCreate(char *align_name, gboolean master_alignment) ;
gboolean zMapFeatureAlignmentAddBlock(ZMapFeatureAlignment feature_align,
				      ZMapFeatureBlock     feature_block) ;
gboolean zMapFeatureAlignmentFindBlock(ZMapFeatureAlignment feature_align,
                                       ZMapFeatureBlock     feature_block);
ZMapFeatureBlock zMapFeatureAlignmentGetBlockByID(ZMapFeatureAlignment feature_align,
                                                  GQuark block_id);
gboolean zMapFeatureAlignmentRemoveBlock(ZMapFeatureAlignment feature_align,
                                         ZMapFeatureBlock     feature_block);
void zMapFeatureAlignmentDestroy(ZMapFeatureAlignment alignment, gboolean free_data) ;


/* ***********************
 * FEATURE CONTEXT METHODS
 */


ZMapFeatureContext zMapFeatureContextCreate(char *sequence, int start, int end, GList *feature_set_names) ;
ZMapFeatureContext zMapFeatureContextCreateEmptyCopy(ZMapFeatureContext feature_context);
ZMapFeatureContext zMapFeatureContextCopyWithParents(ZMapFeatureAny orig_feature) ;
ZMapFeatureContextMergeCode zMapFeatureContextMerge(ZMapFeatureContext *current_context_inout,
						    ZMapFeatureContext *new_context_inout,
						    ZMapFeatureContext *diff_context_out,
                                        GList *featureset_names) ;
gboolean zMapFeatureContextErase(ZMapFeatureContext *current_context_inout,
				 ZMapFeatureContext remove_context,
				 ZMapFeatureContext *diff_context_out);
gboolean zMapFeatureContextAddAlignment(ZMapFeatureContext feature_context,
					ZMapFeatureAlignment alignment,
					gboolean master) ;
gboolean zMapFeatureContextFindAlignment(ZMapFeatureContext   feature_context,
                                         ZMapFeatureAlignment feature_align);
ZMapFeatureAlignment zMapFeatureContextGetAlignmentByID(ZMapFeatureContext feature_context,
                                                        GQuark align_id);
gboolean zMapFeatureContextRemoveAlignment(ZMapFeatureContext   feature_context,
                                           ZMapFeatureAlignment feature_align);
gboolean zMapFeatureContextGetMasterAlignSpan(ZMapFeatureContext context,int *start,int *end);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
ZMapFeatureTypeStyle zMapFeatureContextFindStyle(ZMapFeatureContext context, char *style_name);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

void zMapFeatureContextDestroy(ZMapFeatureContext context, gboolean free_data) ;

gboolean zMapFeatureContextGetDNAStatus(ZMapFeatureContext context);

/* THOSE IN FEATURECONTEXT.C */

GList *zMapFeatureString2QuarkList(char *string_list) ;
GList *zMapFeatureCopyQuarkList(GList *quark_list_orig) ;

void zMapFeatureContextExecute(ZMapFeatureAny feature_any,
                               ZMapFeatureStructType stop,
                               ZMapGDataRecurseFunc callback,
                               gpointer data);
void zMapFeatureContextExecuteFull(ZMapFeatureAny feature_any,
                                   ZMapFeatureStructType stop,
                                   ZMapGDataRecurseFunc callback,
                                   gpointer data);
void zMapFeatureContextExecuteComplete(ZMapFeatureAny feature_any,
                                       ZMapFeatureStructType stop,
                                       ZMapGDataRecurseFunc start_callback,
                                       ZMapGDataRecurseFunc end_callback,
                                       gpointer data);
void zMapFeatureContextExecuteSubset(ZMapFeatureAny feature_any,
                                     ZMapFeatureStructType stop,
                                     ZMapGDataRecurseFunc callback,
                                     gpointer data);
void zMapFeatureContextExecuteRemoveSafe(ZMapFeatureAny feature_any,
					 ZMapFeatureStructType stop,
					 ZMapGDataRecurseFunc start_callback,
					 ZMapGDataRecurseFunc end_callback,
					 gpointer data);
void zMapFeatureContextExecuteStealSafe(ZMapFeatureAny feature_any,
					ZMapFeatureStructType stop,
					ZMapGDataRecurseFunc start_callback,
					ZMapGDataRecurseFunc end_callback,
					gpointer data);





/* UTILITY METHODS */

int zmapFeatureRevCompCoord(int coord, int start, int end);

void zMapFeatureRevComp(int seq_start, int seq_end, int *coord_1, int *coord_2) ;




gboolean zMapFeatureIsValid(ZMapFeatureAny any_feature) ;
gboolean zMapFeatureIsValidFull(ZMapFeatureAny any_feature, ZMapFeatureStructType type) ;
gboolean zMapFeatureNameCompare(ZMapFeatureAny any_feature, char *name) ;
gboolean zMapFeatureTypeIsValid(ZMapFeatureStructType group_type) ;
gboolean zMapFeatureAnyIsSane(ZMapFeatureAny feature, char **insanity_explained);
gboolean zMapFeatureIsSane(ZMapFeature feature, char **insanity_explained);
ZMapFeatureAny zMapFeatureGetParentGroup(ZMapFeatureAny any_feature, ZMapFeatureStructType group_type) ;
char *zMapFeatureName(ZMapFeatureAny any_feature) ;
char *zMapFeatureCanonName(char *feature_name) ;
ZMapFeatureTypeStyle zMapFeatureGetStyle(ZMapFeatureAny feature) ;
gboolean zMapSetListEqualStyles(GList **feature_set_names, GList **styles) ;
gboolean zMapFeatureAnyForceModesToStyles(ZMapFeatureAny feature_any, GHashTable *styles) ;

GList *zMapFeatureGetColumnFeatureSets(ZMapFeatureContextMap map,GQuark column_id,gboolean unique_id);

/* Probably should be merged at some time.... */
gboolean zMapFeatureDumpStdOutFeatures(ZMapFeatureContext feature_context, GHashTable *styles, GError **error_out) ;
gboolean zMapFeatureDumpToFileName(ZMapFeatureContext feature_context,char *filename,char *header, GHashTable *styles, GError **error_out);
gboolean zMapFeatureContextDump(ZMapFeatureContext feature_context, GHashTable *styles,
				GIOChannel *file, GError **error_out) ;

gboolean zMapFeatureContextDumpToFile(ZMapFeatureAny             feature_any,
				      GHashTable *styles,
				      ZMapFeatureDumpFeatureFunc dump_func,
				      gpointer                   dump_user_data,
				      GIOChannel                *dump_file,
				      GError                   **dump_error_out);
gboolean zMapFeatureContextRangeDumpToFile(ZMapFeatureAny             dump_set,
					   GHashTable                     *styles,
					   ZMapSpan                   span_data,
					   ZMapFeatureDumpFeatureFunc dump_func,
					   gpointer                   dump_user_data,
					   GIOChannel                *dump_file,
					   GError                   **dump_error_out) ;
gboolean zMapFeatureListDumpToFile(GList                     *feature_list,
				   GHashTable *styles,
				   ZMapFeatureDumpFeatureFunc dump_func,
				   gpointer                   dump_user_data,
				   GIOChannel                *dump_file,
				   GError                   **dump_error_out);
gboolean zMapFeatureListForeachDumperCreate(ZMapFeatureDumpFeatureFunc dump_func,
					    GHashTable *styles,
					    gpointer                   dump_user_data,
					    GDestroyNotify             dump_user_free,
					    GIOChannel                *dump_file,
					    GError                   **dump_error_out,
					    GFunc                     *dumper_func_out,
					    gpointer                  *dumper_data_out);
gboolean zMapFeatureListForeachDumperDestroy(gpointer dumper_data);

gboolean zMapFeatureGetFeatureListExtent(GList *feature_list, int *start_out, int *end_out);


//void zMapFeature3FrameTranslationRevComp(ZMapFeatureSet feature_set, RevCompData cb_data); mh17: moved to zmapFeature_P.h
char *zMapFeature3FrameTranslationFeatureName(ZMapFeatureSet feature_set, ZMapFrame frame);
void zMapFeature3FrameTranslationPopulate(ZMapFeatureSet feature_set);
gboolean zMapFeature3FrameTranslationCreateSet(ZMapFeatureBlock block, ZMapFeatureSet *set_out);


gboolean zMapFeatureWorld2Transcript(ZMapFeature feature,
				     int w1, int w2,
				     int *t1, int *t2);
ZMapFrame zMapFeatureTranscriptFrame(ZMapFeature feature);
ZMapFrame zMapFeatureSubPartFrame(ZMapFeature feature, int coord);
gboolean zMapFeatureWorld2CDS(ZMapFeature feature,
			      int exon1, int exon2,
			      int *cds1, int *cds2);
gboolean zMapFeatureExon2CDS(ZMapFeature feature,
			     int exon_start, int exon_end,
			     int *exon_cds_start, int *exon_cds_end, int *phase_out) ;
gboolean zMapFeatureAnnotatedExonsCreate(ZMapFeature feature, gboolean include_protein, GList **exon_list_out) ;
void zMapFeatureAnnotatedExonsDestroy(GList *exon_list) ;



/* ============================================================== for teh === */
/* functions in zmapFeatureFormatInput.c */
/* ================================================================= */

gboolean zMapFeatureFormatType(gboolean SO_compliant, gboolean default_to_basic,
                               char *feature_type, ZMapStyleMode *type_out);
char *zMapFeatureStructType2Str(ZMapFeatureStructType type) ;
char *zMapFeatureType2Str(ZMapStyleMode type) ;
char *zMapFeatureSubPart2Str(ZMapFeatureSubpartType subpart) ;
gboolean zMapFeatureFormatStrand(char *strand_str, ZMapStrand *strand_out);
gboolean zMapFeatureStr2Strand(char *string, ZMapStrand *strand);
char *zMapFeatureStrand2Str(ZMapStrand strand) ;
gboolean zMapFeatureFormatFrame(char *frame_str, ZMapFrame *frame_out);
gboolean zMapFeatureStr2Frame(char *string, ZMapFrame *frame);
char *zMapFeatureFrame2Str(ZMapFrame frame) ;
gboolean zMapFeatureFormatPhase(char *phase_str, ZMapPhase *phase_out);
gboolean zMapFeatureValidatePhase(char *value, ZMapPhase *phase);
char *zMapFeaturePhase2Str(ZMapPhase phase) ;
char *zMapFeatureHomol2Str(ZMapHomolType homol) ;
gboolean zMapFeatureFormatScore(char *score_str, gboolean *has_score, gdouble *score_out);


char *zMapFeatureGetDNA(ZMapFeatureAny feature_any, int start, int end, gboolean revcomp) ;
char *zMapFeatureGetFeatureDNA(ZMapFeature feature) ;
char *zMapFeatureGetTranscriptDNA(ZMapFeature transcript, gboolean spliced, gboolean cds_only) ;
char *zMapFeatureDNAFeatureName(ZMapFeatureBlock block);
GQuark zMapFeatureDNAFeatureID(ZMapFeatureBlock block);
gboolean zMapFeatureDNACreateFeatureSet(ZMapFeatureBlock block, ZMapFeatureSet *feature_set_out);
void zMapFeatureDNAAddSequenceData(ZMapFeature dna_feature, char *dna_str, int sequence_length);
ZMapFeature zMapFeatureDNACreateFeature(ZMapFeatureBlock     block,
					ZMapFeatureTypeStyle style,
					char *dna_str,
					int   sequence_length);

void zMapFeature3FrameTranslationSetCreateFeatures(ZMapFeatureSet feature_set,
						   ZMapFeatureTypeStyle style);

char *zMapFeatureTranscriptTranslation(ZMapFeature feature, int *length) ;
char *zMapFeatureTranslation(ZMapFeature feature, int *length) ;


GArray *zMapFeatureAnyAsXMLEvents(ZMapFeatureAny feature_any,
                                  /* ZMapFeatureXMLType xml_type */
                                  int xml_type);
gboolean zMapFeatureAnyAsXML(ZMapFeatureAny feature_any,
                             ZMapXMLWriter xml_writer,
                             GArray **xml_events_out,
                             int xml_type);

gboolean zMapFeatureAnyHasMagic(ZMapFeatureAny feature_any);

ZMapFeatureAny zMapFeatureContextFindFeatureFromFeature(ZMapFeatureContext context,
							ZMapFeatureAny from_feature);


GType zMapFeatureDataGetType(void);
gboolean zMapFeatureGetInfo(ZMapFeatureAny         feature_any,
			    ZMapFeatureSubPartSpan sub_feature,
			    const gchar           *first_property_name,
			    ...);


#endif /* ZMAP_FEATURE_H */
