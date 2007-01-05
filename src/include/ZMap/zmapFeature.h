/*  File: zmapFeature.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Data structures describing a sequence feature.
 *              
 * HISTORY:
 * Last edited: Jan  5 12:32 2007 (rds)
 * Created: Fri Jun 11 08:37:19 2004 (edgrif)
 * CVS info:   $Id: zmapFeature.h,v 1.109 2007-01-05 22:23:37 rds Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_FEATURE_H
#define ZMAP_FEATURE_H

#include <gdk/gdkcolor.h>
#include <ZMap/zmapConfigStyleDefaults.h>
#include <ZMap/zmapXML.h>


/* We use GQuarks to give each feature a unique id, the documentation doesn't say, but you
 * can surmise from the code that zero is not a valid quark. */
enum {ZMAPFEATURE_NULLQUARK = 0} ;


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


/* used by zmapFeatureLookUpEnum() to translate enums into strings */
typedef enum {TYPE_ENUM, STRAND_ENUM, PHASE_ENUM, HOMOLTYPE_ENUM } ZMapEnumType ;


/* Unsure about this....probably should be some sort of key...... */
typedef int methodID ;


/* NB if you add to these enums, make sure any corresponding arrays in
 * zmapFeatureLookUpEnum() are kept in synch. */
/* What about "sequence", atg, and allele as basic feature types ?           */


/* ZMAPFEATURE_RAW_SEQUENCE and now ZMAPFEATURE_PEP_SEQUENCE are temporary.... */
typedef enum {ZMAPFEATURE_INVALID = 0,
	      ZMAPFEATURE_BASIC, ZMAPFEATURE_ALIGNMENT, ZMAPFEATURE_TRANSCRIPT,
	      ZMAPFEATURE_RAW_SEQUENCE, ZMAPFEATURE_PEP_SEQUENCE} ZMapFeatureType ;

typedef enum {ZMAPFEATURE_SUBPART_INVALID = 0,
	      ZMAPFEATURE_SUBPART_INTRON, ZMAPFEATURE_SUBPART_EXON, ZMAPFEATURE_SUBPART_EXON_CDS,
	      ZMAPFEATURE_SUBPART_GAP, ZMAPFEATURE_SUBPART_MATCH} ZMapFeatureSubpartType ;

typedef enum {ZMAPSTRAND_NONE = 0, ZMAPSTRAND_FORWARD, ZMAPSTRAND_REVERSE} ZMapStrand ;

#define FRAME_PREFIX "FRAME-"				    /* For text versions. */
typedef enum {ZMAPFRAME_NONE = 0,
	      ZMAPFRAME_0, ZMAPFRAME_1, ZMAPFRAME_2} ZMapFrame ;

typedef enum {ZMAPPHASE_NONE = 0,
	      ZMAPPHASE_0, ZMAPPHASE_1, ZMAPPHASE_2} ZMapPhase ;

/* as in BLAST*, i.e. target is DNA, Protein, DNA translated */
typedef enum {ZMAPHOMOL_NONE = 0, 
	      ZMAPHOMOL_N_HOMOL, ZMAPHOMOL_X_HOMOL, ZMAPHOMOL_TX_HOMOL} ZMapHomolType ;

typedef enum {ZMAPBOUNDARY_CLONE_END, ZMAPBOUNDARY_5_SPLICE, ZMAPBOUNDARY_3_SPLICE } ZMapBoundaryType ;

typedef enum {ZMAPSEQUENCE_NONE = 0, ZMAPSEQUENCE_DNA, ZMAPSEQUENCE_PEPTIDE} ZMapSequenceType ;



/* Holds dna or peptide.
 * Note that the sequence will be a valid string in that it will be null-terminated,
 * "length" does _not_ include the null terminator. */
typedef struct ZMapSequenceStruct_
{
  ZMapSequenceType type ;				    /* dna or peptide. */
  int length ;						    /* length of sequence in bases or peptides. */
  char *sequence ;					    /* Actual sequence." */
} ZMapSequenceStruct, *ZMapSequence ;



/* We have had this but would like to have a simple "span" sort....as below... */
typedef struct
{
  Coord x1, x2 ;
} ZMapExonStruct, *ZMapExon ;



/* Could represent anything that has a span. */
typedef struct
{
  Coord x1, x2 ;
} ZMapSpanStruct, *ZMapSpan ;


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
 * SMap in ZMap we can use SMap structs instead.... */
typedef struct
{
  int p1, p2 ;						    /* coords in parent. */
  int c1, c2 ;						    /* coords in child. */
} ZMapMapBlockStruct, *ZMapMapBlock ;




/* DO THESE STRUCTS NEED TO BE EXPOSED ? PROBABLY NOT....TO HIDE THEM WOULD REQUIRE
 * QUITE A NUMBER OF ACCESS FUNCTIONS.... */



/* We need some forward declarations for pointers we include in some structs. */

typedef struct ZMapFeatureAlignmentStruct_ *ZMapFeatureAlignment ;

typedef struct ZMapFeatureTypeStyleStruct_ *ZMapFeatureTypeStyle ;

typedef struct ZMapFeatureAnyStruct_ *ZMapFeatureAny ;

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
  ZMapFeatureStructType struct_type ;			    /* context or align or block etc. */
  ZMapFeatureAny parent ;				    /* The parent struct of this one, NULL
							     * if this is a feature context. */
  GQuark unique_id ;					    /* Unique id of this feature. */
  GQuark original_id ;					    /* Original id of this feature. */
} ZMapFeatureAnyStruct ;



/* Holds a set of data that is the complete "view" of the requested sequence.
 * 
 * This includes all the alignments which in the case of the original "fmap" like view
 * will be a single alignment containing a single block.
 * 
 */
typedef struct ZMapFeatureContextStruct_
{
  ZMapFeatureStructType struct_type ;			    /* context or align or block etc. */
  ZMapFeatureAny parent ;				    /* Always NULL in a context. */
  GQuark unique_id ;					    /* Unique id of this feature. */
  GQuark original_id ;					    /* Sequence name. */


  GQuark sequence_name ;				    /* The sequence to be displayed. */

  GQuark parent_name ;					    /* Name of parent sequence
							       (== sequence_name if no parent). */

  int length ;						    /* total length of sequence. */


  /* I think we should remove this as in fact our code will break IF  (start != 1)  !!!!!!!  */
  /* Mapping for the target sequence, this shows where this section of sequence fits in to its
   * overall assembly, e.g. where a clone is located on a chromosome. */
  ZMapSpanStruct parent_span ;				    /* Start/end of parent, usually we
							       will have: x1 = 1, x2 = length in
							       bases of parent. */

  ZMapMapBlockStruct sequence_to_parent ;		    /* Shows how this sequence maps to its
							       ultimate parent. */


  GList *feature_set_names ;				    /* Global list of _names_ of all requested
							       feature sets for the context,
							       _only_ these sets are loaded into
							       the context. */

  GList *styles ;					    /* Global list of all styles, some of
							       these styles may not be used if not
							       required for the list given by
							       feature_set_names. */

  ZMapFeatureAlignment master_align ;			    /* The target/master alignment out of
							       the below set. */

  GData *alignments ;					    /* All the alignments for this zmap
							       as a set of ZMapFeatureAlignment. */

} ZMapFeatureContextStruct, *ZMapFeatureContext ;



typedef struct ZMapFeatureAlignmentStruct_
{
  ZMapFeatureStructType struct_type ;			    /* context or align or block etc. */
  ZMapFeatureAny parent ;				    /* Our parent context. */
  GQuark unique_id ;					    /* Unique id this alignment. */
  GQuark original_id ;					    /* Original id of this sequence. */


  /* Can we change this to be a gdata ? then we would have consistency..... */

  GList *blocks ;					    /* A set of ZMapFeatureStruct. */

} ZMapFeatureAlignmentStruct;



typedef struct ZMapFeatureBlockStruct_
{
  ZMapFeatureStructType struct_type ;			    /* context or align or block etc. */
  ZMapFeatureAny parent ;				    /* Our parent alignment. */
  GQuark unique_id ;					    /* Unique id for this block. */
  GQuark original_id ;					    /* Original id, probably not needed ? */


  ZMapAlignBlockStruct block_to_sequence ;		    /* Shows how these features map to the
							       sequence, n.b. this feature set may only
							       span part of the sequence. */

  ZMapSequenceStruct sequence ;				    /* DNA sequence for this block,
							       n.b. there may not be any dna. */


  GData *feature_sets ;					    /* The features for this block as a
							       set of ZMapFeatureSetStruct. */
} ZMapFeatureBlockStruct, *ZMapFeatureBlock ;




/*!\struct ZMapFeatureSetStruct_
 * \brief a set of ZMapFeature structs.
 * Holds a set of ZMapFeature structs, note that the id for the set is by default the same name
 * as the style for all features in the set. BUT it may not be, the set may consist of
 * features with many types/styles. The set id is completely independent of the style name.
 */
typedef struct ZMapFeatureSetStruct_
{
  ZMapFeatureStructType struct_type ;			    /* context or align or block etc. */
  ZMapFeatureAny parent ;				    /* Our parent block. */
  GQuark unique_id ;					    /* Unique id of this feature set. */
  GQuark original_id ;					    /* Original name,
							       e.g. "Genewise predictions" */
  ZMapFeatureTypeStyle style ;				    /* Style defining how this set is
							       drawn, this applies only to the set
							       * itself, _not_ the features within
							       * the set. */
  GData *features ;					    /* A set of ZMapFeatureStruct. */
} ZMapFeatureSetStruct, *ZMapFeatureSet ;




/* Feature subtypes, homologies and transcripts, the basic feature is just the ZMapFeatureStruct. */
typedef struct
{
  ZMapHomolType type ;					    /* as in Blast* */
  int y1, y2 ;						    /* Query start/end */
  int length ;						    /* Length of homol/align etc. */
  ZMapStrand target_strand ;
  ZMapPhase target_phase ;				    /* for tx_homol */
  GArray *align ;					    /* of AlignBlock, if null, align is ungapped. */

  struct
  {
    /* If align != NULL and perfect == TRUE then gaps array is a "perfect"
     * alignment with allowance for a style specified slop factor. */
    unsigned int perfect : 1 ;
  } flags ;

} ZMapHomolStruct, *ZMapHomol ;


typedef struct
{
  /* If cds == TRUE, then these must show the position of the cds in sequence coords... */
  Coord cds_start, cds_end ;

  ZMapPhase start_phase ;
  GArray *exons ;					    /* Of ZMapSpanStruct. */
  GArray *introns ;					    /* Of ZMapSpanStruct. */

  struct
  {
    unsigned int cds : 1 ;
    unsigned int start_not_found : 1 ;
    unsigned int end_not_found : 1 ;
  } flags ;

} ZMapTranscriptStruct, *ZMapTranscript ;



/* Structure describing a single feature, the feature may be compound (e.g. have exons/introns
 *  etc.) or a single span or point, e.g. an allele.
 *  */
typedef struct ZMapFeatureStruct_ 
{
  /* We could embed a structany here... */
  ZMapFeatureStructType struct_type ;			    /* context or align or block etc. */
  ZMapFeatureAny parent ;				    /* Our containing set. */
  GQuark unique_id ;					    /* Unique id for just this feature for
							       use by ZMap. */
  GQuark original_id ;					    /* Original name, e.g. "bA404F10.4.mRNA" */


  /* flags field holds extra information about various aspects of the feature. */
  /* I'm going to try the bitfields syntax here.... */
  struct
  {
    unsigned int has_score : 1 ;
    unsigned int has_boundary : 1 ;
  } flags ;

  ZMapFeatureType type ;				    /* Basic, transcript, alignment. */

  GQuark ontology ;					    /* Basically a detailed name for this
							       feature such as
							       "trans_splice_acceptor_site", this
							       might be recognised SO term or not. */

  ZMapFeatureTypeStyle style ;				    /* Style defining how this feature is
							       drawn. */

  ZMapFeatureID db_id ;					    /* unique DB identifier, currently
							       unused but will be..... */


  Coord x1, x2 ;					    /* start, end of feature in absolute coords. */

  ZMapStrand strand ;

  ZMapBoundaryType boundary_type ;			    /* splice, clone end ? */

  ZMapPhase phase ;

  float score ;

  GQuark locus_id ;					    /* needed for a lot of annotation. */

  char *text ;						    /* needed ????? */


  char *url ;						    /* Could be a quark but we would need to
							       to use our own table otherwise
							       memory usage will be too high. */

  union
  {
    ZMapHomolStruct homol ;
    ZMapTranscriptStruct transcript ;

    /* I think this is _not_ the correct place for this..... */
    /* ZMapSequenceStruct sequence; */
  } feature ;

} ZMapFeatureStruct, *ZMapFeature ;



/* Struct that supplies text descriptions of the parts of a feature, which fields are filled
 * in depends on the feature type. */
typedef struct
{
  /* Use these fields to interpret and give more info. for the feature parts. */
  ZMapFeatureStructType struct_type ;
  ZMapFeatureType type ;
  ZMapFeatureSubpartType subpart_type ;

  char *feature_name ;
  char *feature_strand ;
  char *feature_frame ;
  char *feature_start ; char *feature_end ;
  char *feature_query_start ; char *feature_query_end ; char *feature_query_length ;
  char *feature_length ;
  char *sub_feature_start ; char *sub_feature_end ;
  char *sub_feature_query_start ; char *sub_feature_query_end ;
  char *sub_feature_length ;
  char *feature_score ; char *feature_type ;
  char *feature_set ; char *feature_style ;
  char *feature_description ; char *feature_locus ;

} ZMapFeatureDescStruct, *ZMapFeatureDesc ;







/* 
 *      Styles: specifies how a feature should be drawn/processed.
 */


/* Specifies how features that reference this style will be processed. */
typedef enum
  {
    ZMAPSTYLE_MODE_NONE,				    /* Features are not processed or for meta=style. */
    ZMAPSTYLE_MODE_BASIC,				    /* Basic box features. */
    ZMAPSTYLE_MODE_TRANSCRIPT,				    /* Usual transcript like structure. */
    ZMAPSTYLE_MODE_ALIGNMENT,				    /* Usual homology structure. */
    ZMAPSTYLE_MODE_TEXT,				    /* Text only display. */
    ZMAPSTYLE_MODE_GRAPH				    /* Graphs of various types. */
  } ZMapStyleMode ;



/* Specifies the style of graph. */
typedef enum
  {
    ZMAPSTYLE_GRAPH_INVALID,				    /* Initial setting. */
    ZMAPSTYLE_GRAPH_LINE,				    /* Just points joining a line. */
    ZMAPSTYLE_GRAPH_HISTOGRAM				    /* Usual blocky like graph. */
  } ZMapStyleGraphMode ;



/* Specifies how wide features should be in relation to their score. */
typedef enum
  {
    ZMAPSCORE_WIDTH,					    /* Use column width only - default. */
    ZMAPSCORE_OFFSET,
    ZMAPSCORE_HISTOGRAM,
    ZMAPSCORE_PERCENT
  } ZMapStyleScoreMode ;


/* Specifies how features in columns should be overlapped for compact display. */
typedef enum
  {
    ZMAPOVERLAP_START,
    ZMAPOVERLAP_COMPLETE,				    /* draw on top - default */
    ZMAPOVERLAP_OVERLAP,				    /* bump if feature coords overlap. */
    ZMAPOVERLAP_POSITION,				    /* bump if features start at same coord. */
    ZMAPOVERLAP_NAME,					    /* one column per homol target */
    ZMAPOVERLAP_COMPLEX,				    /* all features with same name in a
							       single column, several names in one
							       column but no 2 features overlap. */
    ZMAPOVERLAP_NO_INTERLEAVE,				    /* all features with same name in a
							       single column, several names in one
							       column but no interleaving of sets
							       of features. */
    ZMAPOVERLAP_COMPLEX_RANGE,				    /* All features with same name in a
							       single column if they overlap the
							       focus range, all other features in
							       a single column.  */
    ZMAPOVERLAP_ITEM_OVERLAP,                               /* bump if item coords overlap in canvas space... */
    ZMAPOVERLAP_SIMPLE,					    /* one column per feature, for testing... */
    ZMAPOVERLAP_END
  } ZMapStyleOverlapMode ;



/* Lets change all these names to just be zmapFeatureStyle, i.e. lose the type bit.....
 * could even lose the feature bit and just go straight to style, would be better. */

typedef struct ZMapFeatureTypeStyleStruct_
{
  GQuark original_id ;					    /* Original name. */
  GQuark unique_id ;					    /* Name normalised to be unique. */

  char *description ;					    /* Description of what this style
							       represents. */

  ZMapStyleMode mode ;					    /* Specifies how features that
							       reference this style will be processed. */

  ZMapStyleGraphMode graph_mode ;			    /* Says how to draw a graph. */
  double baseline ;

  struct
  {
    /* I don't want a general show/hide flag here because we should
     * get that dynamically from the state of the column canvas
     * item.  */
    unsigned int hidden_always   : 1 ;			    /* Column always hidden. */

    unsigned int hidden_now      : 1 ;			    /* Column hidden now ? */

    unsigned int show_when_empty : 1 ;			    /* If TRUE, features' column is
							       displayed even if there are no features. */

    unsigned int showText        : 1 ;			    /* Should feature text be displayed. */

    unsigned int parse_gaps      : 1 ;
    unsigned int align_gaps      : 1 ;			    /* TRUE: gaps within alignment are displayed,
							       FALSE: alignment is displayed as a single block. */

    unsigned int join_aligns     : 1 ;			    /* TRUE: joins aligns if between_align_error not exceeded,
							       FALSE: does not joing aligns. */



    /* These are all linked, if strand_specific is FALSE, then so are
     * frame_specific and show_rev_strand. */
    unsigned int strand_specific : 1 ;			    /* Feature that is on one strand of the dna. */
    unsigned int show_rev_strand : 1 ;			    /* Only display the feature on the
							       reverse strand if this is set. */
    unsigned int frame_specific  : 1 ;			    /* Feature that is in some way linked
							       to the reading frame of the dna. */
    unsigned int show_only_as_3_frame : 1 ;		    /* frame specific feature that should
							       only be displayed when all 3 frames
							       are shown. */

    unsigned int directional_end : 1 ;			    /* Display pointy ends on exons etc. */

  } opts ;

  struct
  {
    unsigned int background_set : 1;
    unsigned int foreground_set : 1;
    unsigned int outline_set    : 1;
    GdkColor  foreground ;      /* Overlaid on background. */
    GdkColor  background ;      /* Fill colour. */
    GdkColor  outline ;         /* Surround/line colour. */
  } colours;

  ZMapStyleOverlapMode overlap_mode ;			    /* Controls how features are grouped
							       into sub columns within a column. */

  double min_mag ;					    /* Don't display if fewer bases/line */
  double max_mag ;					    /* Don't display if more bases/line */

  double   width ;					    /* column width */
  double bump_width;

  ZMapStyleScoreMode   score_mode ;			    /* Controls width of features that
							       have scores. */
  double    min_score, max_score ;			    /* Min/max for score width calc. */


  /* Allowable align errors, used to decide whether a match should be classified as "perfect".
   *   within_align_error   is used to assess the blocks in a single gapped alignment if align_gaps = TRUE
   *  between_align_error   is used to assess several alignments (e.g. for exon matches) if join_homols = TRUE
   * 
   * Number is allowable number of missing bases between blocks/alignments, default is 0. */
  unsigned int within_align_error ;
  unsigned int between_align_error ;

  /* GFF feature dumping, allows specifying of source/feature types independently of feature
   * attributes. */
  GQuark gff_source ;
  GQuark gff_feature ;

} ZMapFeatureTypeStyleStruct ;


typedef enum
  {
    ZMAP_CONTEXT_EXEC_STATUS_OK,
    ZMAP_CONTEXT_EXEC_STATUS_DONT_DESCEND,
    ZMAP_CONTEXT_EXEC_STATUS_ERROR
  } ZMapFeatureContextExecuteStatus;

typedef ZMapFeatureContextExecuteStatus (*ZMapGDataRecurseFunc)(GQuark   key_id,
                                                                gpointer list_data,
                                                                gpointer user_data,
                                                                char   **error);
/* Callback function for calls to zMapFeatureDumpFeatures(), caller can use this function to
 * format features in any way they want. */
typedef gboolean (*ZMapFeatureDumpFeatureCallbackFunc)(GIOChannel *file,
						       gpointer user_data,
						       ZMapFeatureTypeStyle style,
						       char *parent_name,
						       char *feature_name,
						       char *style_name,
						       char *ontology,
						       int x1, int x2,
						       gboolean has_score, float score,
						       ZMapStrand strand,
						       ZMapPhase phase,
						       ZMapFeatureType feature_type,
						       gpointer feature_data,
						       GError **error_out) ;



/* ***************
 * FEATURE METHODS 
 */

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
ZMapFeature zMapFeatureCopy(ZMapFeature feature) ;
ZMapFeature zMapFeatureCreateEmpty(void) ;
ZMapFeature zMapFeatureCreateFromStandardData(char *name, char *sequence, char *ontology,
                                              ZMapFeatureType feature_type, 
                                              ZMapFeatureTypeStyle style,
                                              int start, int end,
                                              gboolean has_score, double score,
                                              ZMapStrand strand, ZMapPhase phase);

gboolean zMapFeatureAddStandardData(ZMapFeature feature, char *feature_name_id, char *name,
				    char *sequence, char *ontology,
				    ZMapFeatureType feature_type, ZMapFeatureTypeStyle style,
				    int start, int end,
				    gboolean has_score, double score,
				    ZMapStrand strand, ZMapPhase phase) ;
gboolean zMapFeatureAddSplice(ZMapFeature feature, ZMapBoundaryType boundary) ;
gboolean zMapFeatureAddTranscriptData(ZMapFeature feature,
				      gboolean cds, Coord cds_start, Coord cds_end,
				      gboolean start_not_found, ZMapPhase start_phase,
				      gboolean end_not_found,
				      GArray *exons, GArray *introns) ;
gboolean zMapFeatureAddTranscriptExonIntron(ZMapFeature feature,
					    ZMapSpanStruct *exon, ZMapSpanStruct *intron) ;
gboolean zMapFeatureAddAlignmentData(ZMapFeature feature,
				     ZMapHomolType homol_type,
				     ZMapStrand target_strand, ZMapPhase target_phase,
				     int query_start, int query_end, int query_length,
				     GArray *gaps) ;
char    *zMapFeatureMakeDNAFeatureName(ZMapFeatureBlock block);
gboolean zMapFeatureSetCoords(ZMapStrand strand, int *start, int *end,
			      int *query_start, int *query_end) ;
void     zMapFeature2MasterCoords(ZMapFeature feature, double *feature_x1, double *feature_x2) ;
void     zMapFeatureReverseComplement(ZMapFeatureContext context) ;
gboolean zMapFeatureAddURL(ZMapFeature feature, char *url) ;
gboolean zMapFeatureAddLocus(ZMapFeature feature, GQuark locus_id) ;
void     zMapFeatureSortGaps(GArray *gaps) ;
int      zMapFeatureLength(ZMapFeature feature) ;
void     zMapFeatureDestroy(ZMapFeature feature) ;


/* ******************* 
 * FEATURE SET METHODS 
 */

GQuark zMapFeatureSetCreateID(char *feature_set_name) ; 
ZMapFeatureSet zMapFeatureSetCreate(char *source, GData *features) ;
ZMapFeatureSet zMapFeatureSetIDCreate(GQuark original_id, GQuark unique_id,
				      ZMapFeatureTypeStyle style, GData *features) ;
gboolean zMapFeatureSetAddFeature(ZMapFeatureSet feature_set, ZMapFeature feature) ;
gboolean zMapFeatureSetFindFeature(ZMapFeatureSet feature_set, ZMapFeature feature) ;
ZMapFeature zMapFeatureSetGetFeatureByID(ZMapFeatureSet feature_set, 
                                         GQuark feature_id);
gboolean zMapFeatureSetRemoveFeature(ZMapFeatureSet feature_set, ZMapFeature feature) ;
void     zMapFeatureSetDestroy(ZMapFeatureSet feature_set, gboolean free_data) ;
void  zMapFeatureSetStyle(ZMapFeatureSet feature_set, ZMapFeatureTypeStyle style) ;
char *zMapFeatureSetGetName(ZMapFeatureSet feature_set) ;


/* *********************
 * FEATURE BLOCK METHODS 
 */

GQuark zMapFeatureBlockCreateID(int ref_start, int ref_end, ZMapStrand ref_strand,
                                int non_start, int non_end, ZMapStrand non_strand);
ZMapFeatureBlock zMapFeatureBlockCreate(char *block_seq,
					int ref_start, int ref_end, ZMapStrand ref_strand,
					int non_start, int non_end, ZMapStrand non_strand) ;

void zMapFeatureBlockAddFeatureSet(ZMapFeatureBlock feature_block, ZMapFeatureSet feature_set) ;
gboolean zMapFeatureBlockFindFeatureSet(ZMapFeatureBlock feature_block,
                                        ZMapFeatureSet   feature_set);
ZMapFeatureSet zMapFeatureBlockGetSetByID(ZMapFeatureBlock feature_block, 
                                          GQuark set_id) ;
gboolean zMapFeatureBlockRemoveFeatureSet(ZMapFeatureBlock feature_block,
                                          ZMapFeatureSet   feature_set);
gboolean zMapFeatureBlockThreeFrameTranslation(ZMapFeatureBlock block, ZMapFeatureSet *set_out);
void zMapFeatureBlockDestroy(ZMapFeatureBlock block, gboolean free_data) ;

gboolean zMapFeatureBlockDNA(ZMapFeatureBlock block,
                             char **seq_name, int *seq_len, char **sequence) ;


/* *************************
 * FEATURE ALIGNMENT METHODS
 */


GQuark zMapFeatureAlignmentCreateID(char *align_sequence, gboolean query_sequence) ; 
ZMapFeatureAlignment zMapFeatureAlignmentCreate(char *align_name, gboolean master_alignment) ;
void zMapFeatureAlignmentAddBlock(ZMapFeatureAlignment feature_align, 
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


ZMapFeatureContext zMapFeatureContextCreate(char *sequence, int start, int end,
					    GList *styles, GList *feature_set_names) ;
gboolean zMapFeatureContextMerge(ZMapFeatureContext *current_context_inout,
				 ZMapFeatureContext new_context,
				 ZMapFeatureContext *diff_context_out) ;
void zMapFeatureContextAddAlignment(ZMapFeatureContext feature_context,
				    ZMapFeatureAlignment alignment, 
                                    gboolean master) ;
gboolean zMapFeatureContextFindAlignment(ZMapFeatureContext   feature_context,
                                         ZMapFeatureAlignment feature_align);
ZMapFeatureAlignment zMapFeatureContextGetAlignmentByID(ZMapFeatureContext feature_context,
                                                        GQuark align_id);
gboolean zMapFeatureContextRemoveAlignment(ZMapFeatureContext   feature_context,
                                           ZMapFeatureAlignment feature_align);
void zMapFeatureContextDestroy(ZMapFeatureContext context, gboolean free_data) ;


/* THOSE IN FEATURECONTEXT.C */

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



/* UTILITY METHODS */


gboolean zMapFeatureIsValid(ZMapFeatureAny any_feature) ;
gboolean zMapFeatureTypeIsValid(ZMapFeatureStructType group_type) ;
ZMapFeatureAny zMapFeatureGetParentGroup(ZMapFeatureAny any_feature, ZMapFeatureStructType group_type) ;
char *zMapFeatureName(ZMapFeatureAny any_feature) ;
char *zMapFeatureCanonName(char *feature_name) ;
ZMapFeatureTypeStyle zMapFeatureGetStyle(ZMapFeature feature) ;
gboolean zMapSetListEqualStyles(GList **feature_set_names, GList **styles) ;

/* Probably should be merged at some time.... */
gboolean zMapFeatureContextDump(GIOChannel *file,
				ZMapFeatureContext feature_context, GError **error_out) ;
gboolean zMapFeatureDumpFeatures(GIOChannel *file, ZMapFeatureAny dump_set,
				 ZMapFeatureDumpFeatureCallbackFunc dump_func,
				 gpointer user_data,
				 GError **error) ;


/* 
 *     Style functions, name should all be rationalised to just use "style", not "type".
 */

ZMapFeatureTypeStyle zMapFeatureTypeCreate(char *name, char *description, ZMapStyleMode mode,
					   char *outline, char *foreground, char *background,
					   double width) ;
void zMapStyleSetMode(ZMapFeatureTypeStyle style, ZMapStyleMode mode) ;
gboolean zMapStyleFormatMode(char *mode_str, ZMapStyleMode *mode_out) ;
void zMapStyleSetColours(ZMapFeatureTypeStyle style, char *outline, char *foreground, char *background) ;
void zMapStyleSetMag(ZMapFeatureTypeStyle style, double min_mag, double max_mag) ;
void zMapStyleSetScore(ZMapFeatureTypeStyle style, double min_score, double max_score) ;
void zMapStyleSetGraph(ZMapFeatureTypeStyle style, ZMapStyleGraphMode mode, double min, double max, double baseline) ;
void zMapStyleSetStrandAttrs(ZMapFeatureTypeStyle type,
			     gboolean strand_specific, gboolean frame_specific,
			     gboolean show_rev_strand, gboolean show_as_3_frame) ;
void zMapStyleGetStrandAttrs(ZMapFeatureTypeStyle type,
			     gboolean *strand_specific, gboolean *frame_specific,
			     gboolean *show_rev_strand, gboolean *show_as_3_frame) ;
void zMapStyleSetHideAlways(ZMapFeatureTypeStyle style, gboolean hide_always) ;
void zMapStyleSetHidden(ZMapFeatureTypeStyle style, gboolean hidden) ;
gboolean zMapStyleGetHidden(ZMapFeatureTypeStyle style) ;
void zMapStyleSetEndStyle(ZMapFeatureTypeStyle style, gboolean directional) ;
void zMapStyleSetGappedAligns(ZMapFeatureTypeStyle style, gboolean show_gaps, gboolean parse_gaps,
			      unsigned int within_align_error) ;
gboolean zMapStyleGetGappedAligns(ZMapFeatureTypeStyle style, unsigned int *within_align_error) ;
void zMapStyleSetJoinAligns(ZMapFeatureTypeStyle style, gboolean join_aligns, unsigned int between_align_error) ;
gboolean zMapStyleGetJoinAligns(ZMapFeatureTypeStyle style, unsigned int *between_align_error) ;
char *zMapStyleCreateName(char *style_name) ;
GQuark zMapStyleCreateID(char *style_name) ;
char *zMapStyleGetName(ZMapFeatureTypeStyle style) ;
void zMapFeatureTypeDestroy(ZMapFeatureTypeStyle type) ;
ZMapFeatureTypeStyle zMapStyleGetPredefined(char *style_name) ;
gboolean zMapFeatureTypeSetAugment(GData **current, GData **new) ;
void zMapFeatureTypeGetColours(ZMapFeatureTypeStyle style,
                               GdkColor **background,
                               GdkColor **foreground,
                               GdkColor **outline) ;
void zMapStyleSetBump(ZMapFeatureTypeStyle type, char *bump) ;
ZMapStyleOverlapMode zMapStyleGetOverlapMode(ZMapFeatureTypeStyle style) ;
void zMapStyleSetOverlapMode(ZMapFeatureTypeStyle style, ZMapStyleOverlapMode overlap_mode) ;

ZMapFeatureTypeStyle zMapFeatureStyleCopy(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleMerge(ZMapFeatureTypeStyle curr_style, ZMapFeatureTypeStyle new_style) ;

void zMapFeatureTypePrintAll(GData *type_set, char *user_string) ;
void zMapFeatureStylePrintAll(GList *styles, char *user_string) ;
GList *zMapFeatureTypeGetFromFile(char *types_file) ;
gboolean zMapStyleNameExists(GList *style_name_list, char *style_name) ;
ZMapFeatureTypeStyle zMapFindStyle(GList *styles, GQuark style_id) ;
GList *zMapStylesGetNames(GList *styles) ;
GList *zMapStyleGetAllPredefined(void) ;
GList *zMapStyleMergeStyles(GList *curr_styles, GList *new_styles) ;
void zMapStyleDestroyStyles(GList *styles) ;


/* ================================================================= */
/* functions in zmapFeatureFormatInput.c */
/* ================================================================= */

gboolean zMapFeatureFormatType(gboolean SO_compliant, gboolean default_to_basic,
                               char *feature_type, ZMapFeatureType *type_out);
char *zMapFeatureStructType2Str(ZMapFeatureStructType type) ;
char *zMapFeatureType2Str(ZMapFeatureType type) ;
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


char *zMapFeatureGetDNA(ZMapFeatureBlock block, int start, int end, gboolean revcomp) ;
char *zMapFeatureGetFeatureDNA(ZMapFeatureContext context, ZMapFeature feature) ;
char *zMapFeatureGetTranscriptDNA(ZMapFeatureContext context, ZMapFeature transcript,
				  gboolean spliced, gboolean cds_only) ;


GArray *zMapFeatureAnyAsXMLEvents(ZMapFeatureAny feature_any, 
                                  /* ZMapFeatureXMLType xml_type */
                                  int xml_type);
gboolean zMapFeatureAnyAsXML(ZMapFeatureAny feature_any, 
                             ZMapXMLWriter xml_writer,
                             GArray **xml_events_out,
                             int xml_type);



#endif /* ZMAP_FEATURE_H */
