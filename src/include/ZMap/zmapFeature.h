/*  File: zmapFeature.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 * Last edited: Jun 24 14:13 2005 (edgrif)
 * Created: Fri Jun 11 08:37:19 2004 (edgrif)
 * CVS info:   $Id: zmapFeature.h,v 1.29 2005-06-24 13:20:39 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_FEATURE_H
#define ZMAP_FEATURE_H

#include <gdk/gdkcolor.h>
#include <libfoocanvas/libfoocanvas.h>


/* We use GQuarks to give each feature a unique id, the documentation doesn't say, but you
 * can surmise from the code that zero is not a valid quark. */
enum {ZMAPFEATURE_NULLQUARK = 0} ;


/* A unique ID for each feature created, can be used to unabiguously query for that feature
 * in a database. The feature id ZMAPFEATUREID_NULL is guaranteed not to equate to any feature
 * and also means you can test using  (!feature_id). */
typedef unsigned int ZMapFeatureID ;
enum {ZMAPFEATUREID_NULL = 0} ;


typedef int Coord ;					    /* we do need this here.... */




/* used by zmapFeatureLookUpEnums() to translate enums into strings */
typedef enum { TYPE_ENUM, STRAND_ENUM, PHASE_ENUM } ZMapEnumType ;


/* Unsure about this....probably should be some sort of key...... */
typedef int methodID ;


/* NB if you add to these enums, make sure any corresponding arrays in
** zmapFeatureLookUpEnums() are kept in synch. */

/* What about "sequence", atg, and allele as basic feature types ?           */
typedef enum {ZMAPFEATURE_INVALID = -1,
	      ZMAPFEATURE_BASIC = 0, ZMAPFEATURE_HOMOL,
	      ZMAPFEATURE_EXON, ZMAPFEATURE_INTRON, 
	      ZMAPFEATURE_TRANSCRIPT, ZMAPFEATURE_VARIATION,
	      ZMAPFEATURE_BOUNDARY, ZMAPFEATURE_SEQUENCE} ZMapFeatureType ;

typedef enum {ZMAPSTRAND_NONE = 0,
	      ZMAPSTRAND_FORWARD, ZMAPSTRAND_REVERSE} ZMapStrand ;

typedef enum {ZMAPPHASE_NONE = 0,
	      ZMAPPHASE_0, ZMAPPHASE_1, ZMAPPHASE_2} ZMapPhase ;

/* as in BLAST*, i.e. target is DNA, Protein, DNA translated */
typedef enum {ZMAPHOMOL_N_HOMOL, ZMAPHOMOL_X_HOMOL, ZMAPHOMOL_TX_HOMOL} ZMapHomolType ;

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


typedef struct ZMapFeatureAlignmentStruct_ *ZMapFeatureAlignment ;

/* Holds a set of ZMapFeatureSet's.
 * Structure would usually contain a complete set of data from a server for a particular span
 * of a sequence. */
typedef struct ZMapFeatureContextStruct_
{
  GQuark sequence_name ;				    /* The sequence to be displayed. */

  GQuark parent_name ;					    /* Name of parent, not needed ? */

  int length ;						    /* total length of sequence. */

  ZMapSequenceStruct sequence ;				    /* The dna sequence. */

  /* Mapping for the target sequence, this shows where this section of sequence fits in to its
   * overall assembly, e.g. where a clone is located on a chromosome. */
  ZMapSpanStruct parent_span ;				    /* Start/end of parent, usually we
							       will have: x1 = 1, x2 = length in
							       bases of parent. */

  ZMapMapBlockStruct sequence_to_parent ;		    /* Shows how this sequence maps to its
							       ultimate parent. */

  ZMapFeatureAlignment master_align ;			    /* The target/master alignment out of
							       the below set. */

  GList *types ;					    /* Global list of types, _only_ these
							       types are loaded into the context. */

  GData *alignments ;					    /* All the alignements for this zmap
							       as a set of ZMapFeatureAlignment. */

} ZMapFeatureContextStruct, *ZMapFeatureContext ;



typedef struct ZMapFeatureAlignmentStruct_
{
  ZMapFeatureContext parent_context ;			    /* Our parent context. */

  GQuark unique_id ;					    /* Unique id this alignment. */
  GQuark original_id ;					    /* Original id of this sequence. */

  GList *blocks ;					    /* A set of ZMapFeatureStruct. */

} ZMapFeatureAlignmentStruct;



typedef struct ZMapFeatureBlockStruct_
{
  ZMapFeatureAlignment parent_alignment ;		    /* Our parent alignment. */

  GQuark unique_id ;					    /* Unique id for this block. */
  GQuark original_id ;					    /* Original id, probably not needed ? */
  
  ZMapAlignBlockStruct block_to_sequence ;		    /* Shows how these features map to the
							       sequence, n.b. this feature set may only
							       span part of the sequence. */

  GData *feature_sets ;					    /* The features for this block as a
							       set of ZMapFeatureSetStruct. */
} ZMapFeatureBlockStruct, *ZMapFeatureBlock ;




/* Holds a set of ZMapFeature's, note that the id for the set is the same as the style name for
 * the set of features. There is duplication here and probably we should hold a pointer to the
 * the style here.... */
typedef struct ZMapFeatureSetStruct_
{
  ZMapFeatureBlock parent_block ;			    /* Our parent block. */

  GQuark unique_id ;					    /* Unique id for feature set used by
							     * ZMap. */
  GQuark original_id ;					    /* Original name, e.g. "Genewise predictions" */

  GData *features ;					    /* A set of ZMapFeatureStruct. */

  GQuark style_id ;					    /* Style for these features. */

} ZMapFeatureSetStruct, *ZMapFeatureSet ;





typedef struct
{
  ZMapHomolType type ;					    /* as in Blast* */
  int y1, y2 ;						    /* target start/end */
  ZMapStrand target_strand ;
  ZMapPhase target_phase ;				    /* for tx_homol */
  float score ;
  GArray *align ;					    /* of AlignBlock, if null, align is ungapped. */
} ZMapHomolStruct, *ZMapHomol ;


typedef struct
{
  Coord cdsStart, cdsEnd ;
  gboolean start_not_found ;
  ZMapPhase cds_phase ;					    /* none if not a coding transcript ? */
  gboolean endNotFound ;
  GArray *exons ;
  GArray *introns ;					    /* experiment...should we have
							       explicit introns ? */
} ZMapTranscriptStruct, *ZMapTranscript ;



/* I don't know what this struct is for, I would guess that its to allow a floating exon to
 * be linked to some sort of feature ? */
typedef struct
{
  ZMapFeatureID *id ;					    /* backpointer */
} ZMapSingleExonStruct, *ZMapSingleExon ;



/* Structure describing a single feature, the feature may be compound (e.g. have exons/introns
 *  etc.) or a single span or point, e.g. an allele.
 *  */
typedef struct ZMapFeatureStruct_ 
{
  ZMapFeatureSet parent_set ;				    /* Our containing set. */


  ZMapFeatureID db_id ;					    /* unique DB identifier, currently
							       unused but will be..... */

  GQuark unique_id ;					    /* Unique id for just this feature for
							       use by ZMap. */
  GQuark original_id ;					    /* Original name, e.g. "bA404F10.4.mRNA" */

  ZMapFeatureType type ;				    /* e.g. intron, homol etc. */

  Coord x1, x2 ;					    /* start, end of feature in absolute coords. */

  ZMapStrand strand ;

  ZMapPhase phase ;

  float score ;

  char *text ;						    /* needed ????? */

  union
  {
    ZMapHomolStruct homol ;
    ZMapTranscriptStruct transcript ;
    ZMapSingleExonStruct exon ;				    /* What is this needed for ? */
  } feature ;

} ZMapFeatureStruct, *ZMapFeature ;



/* Styles: specifies how a feature should be drawn/processed. */


typedef enum {ZMAPCALCWIDTH_WIDTH, ZMAPCALCWIDTH_OFFSET, ZMAPCALCWIDTH_HISTOGRAM } ZMapFeatureWidthStyle ;
typedef enum {ZMAPOVERLAP_COMPLETE, ZMAPOVERLAP_BUMP, ZMAPOVERLAP_CLUSTER } ZMapFeatureOverlapStyle ;


/* Lets change all these names to just be zmapFeatureStyle, i.e. lose the type bit.....
 * could even lose the feature bit and just go straight to style, would be better. */

typedef struct ZMapFeatureTypeStyleStruct_
{
  GQuark original_id ;					    /* Original name. */
  GQuark unique_id ;					    /* Name normalised to be unique. */

  char *description ;					    /* Description of what this style
							       represents. */

  GdkColor  outline ;					    /* Surround/line colour. */
  GdkColor  foreground ;				    /* Overlaid on background. */
  GdkColor  background ;				    /* Fill colour. */

  float     right_priority ;
  ZMapFeatureWidthStyle width_style ;
  ZMapFeatureOverlapStyle overlap_style ;
  double    width ;					    /* column width */
  double    min_mag, max_mag ;                              /* bases per line */
  double    min_score, max_score ;
  gboolean  showText ;

  /* These are all linked, if strand_specific is FALSE, then so are frame_specific
   * and show_rev_strand. */
  gboolean  strand_specific ;
  gboolean  frame_specific ;
  gboolean  show_rev_strand ;

} ZMapFeatureTypeStyleStruct, *ZMapFeatureTypeStyle ;





GQuark zMapFeatureAlignmentCreateID(char *align_sequence, gboolean query_sequence) ; 
GQuark zMapFeatureBlockCreateID(char *align_sequence,
				int target_start, int target_end, ZMapStrand target_strand,
				int query_start, int query_end, ZMapStrand query_strand) ;


char *zMapFeatureCreateName(ZMapFeatureType feature_type, char *feature_name,
			    ZMapStrand strand, int start, int end, int query_start, int query_end) ;
GQuark zMapFeatureCreateID(ZMapFeatureType feature_type, char *feature_name,
			   ZMapStrand strand, int start, int end,
			   int query_start, int query_end) ;
gboolean zMapFeatureSetCoords(ZMapStrand strand, int *start, int *end,
			      int *query_start, int *query_end) ;
char *zmapFeatureLookUpEnum (int id, int enumType) ;


ZMapFeature zMapFeatureFindFeatureInContext(ZMapFeatureContext feature_context,
					    GQuark type_id, GQuark feature_id) ;

ZMapFeature zMapFeatureFindFeatureInSet(ZMapFeatureSet feature_set, GQuark feature_id) ;

GData *zMapFeatureFindSetInContext(ZMapFeatureContext feature_context, GQuark set_id) ;


ZMapFeatureContext zMapFeatureContextCreate(char *sequence, int start, int end, GList *types) ;
gboolean zMapFeatureContextMerge(ZMapFeatureContext *current_context_inout,
				 ZMapFeatureContext new_context,
				 ZMapFeatureContext *diff_context_out) ;
void zMapFeatureContextAddAlignment(ZMapFeatureContext feature_context,
				    ZMapFeatureAlignment alignment, gboolean master) ;
void zMapFeatureDump(ZMapFeatureContext feature_context, char *file) ;
void zMapFeatureContextDestroy(ZMapFeatureContext context, gboolean free_data) ;

ZMapFeatureAlignment zMapFeatureAlignmentCreate(char *align_name) ;
void zMapFeatureAlignmentAddBlock(ZMapFeatureAlignment alignment, ZMapFeatureBlock block) ;
void zMapFeatureAlignmentDestroy(ZMapFeatureAlignment alignment) ;

ZMapFeatureBlock zMapFeatureBlockCreate(char *ref_seq,
					int ref_start, int ref_end, ZMapStrand ref_strand,
					char *non_seq,
					int non_start, int non_end, ZMapStrand non_strand) ;
void zMapFeatureBlockAddFeatureSet(ZMapFeatureBlock feature_block, ZMapFeatureSet feature_set) ;
void zMapFeatureBlockDestroy(ZMapFeatureBlock block, gboolean free_data) ;

ZMapFeature zmapFeatureCreateEmpty(void) ;
gboolean zmapFeatureAugmentData(ZMapFeature feature, char *feature_name_id, char *name,
				char *sequence, ZMapFeatureType feature_type,
				int start, int end, double score, ZMapStrand strand,
				ZMapPhase phase,
				ZMapHomolType homol_type_out, int start_out, int end_out,
				GArray *gaps) ;
GQuark zMapFeatureGetStyleQuark(ZMapFeature feature) ;
ZMapFeatureTypeStyle zMapFeatureGetStyle(ZMapFeature feature) ;
void zmapFeatureDestroy(ZMapFeature feature) ;

ZMapFeatureSet zMapFeatureSetCreate(char *source, GData *features) ;
ZMapFeatureSet zMapFeatureSetIDCreate(GQuark original_id, GQuark unique_id, GData *features) ;
void zMapFeatureSetAddFeature(ZMapFeatureSet feature_set, ZMapFeature feature) ;
void zMapFeatureSetDestroy(ZMapFeatureSet feature_set, gboolean free_data) ;


ZMapFeatureTypeStyle zMapFeatureTypeCreate(char *name,
					   char *outline, char *foreground, char *background,
					   double width, double min_mag) ;
void zMapStyleSetStrandAttrs(ZMapFeatureTypeStyle type,
			     gboolean strand_specific, gboolean frame_specific,
			     gboolean show_rev_strand) ;
char *zMapStyleCreateName(char *style_name) ;
GQuark zMapStyleCreateID(char *style_name) ;

char *zMapStyleGetName(ZMapFeatureTypeStyle style) ;
ZMapFeatureTypeStyle zMapFindStyle(GList *styles, GQuark style_id) ;
ZMapFeatureTypeStyle zMapFeatureTypeCopy(ZMapFeatureTypeStyle type) ;
void zMapFeatureTypeDestroy(ZMapFeatureTypeStyle type) ;
GList *zMapFeatureTypeGetFromFile(char *types_file) ;
gboolean zMapFeatureTypeSetAugment(GData **current, GData **new) ;
void zMapFeatureTypePrintAll(GData *type_set, char *user_string) ;


#endif /* ZMAP_FEATURE_H */
