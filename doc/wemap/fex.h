/*  File: fex.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *          and Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 2002
 *-------------------------------------------------------------------
 * Acedb is free software; you can redistribute it and/or
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
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: The interface for querying a database to get genome
 *              information for displaying a physical map and then
 *              annotating the genetic information in that map.
 *              
 *              The name "fex" is an acronym for "Feature EXchange".
 *              
 *              This is the interface as seen by the C code in the
 *              client program, in some places the CORBA layer will
 *              be need to be a little different.
 *              
 * HISTORY:
 * Last edited: Jan 29 14:06 2002 (edgrif)
 * Created: Wed Jan 16 15:23:44 2002 (edgrif)
 * CVS info:   $Id: fex.h,v 1.1 2003-12-12 14:30:38 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef FEX_DEF
#define FEX_DEF



/*********** basic types ************/

/* ALL_FEATURES is for fexAvailableFeatureSets() */
/* What about "sequence", atg, and allele as basic feature types ?           */
typedef enum { BASIC_FEATURE, HOMOL_FEATURE, EXON_FEATURE, 
	       TRANSCRIPT_FEATURE, VARIATION_FEATURE, 
	       INTRON_FEATURE, BOUNDARY_FEATURE, ALL_FEATURES
             } FexFeatureType ;

typedef enum { NO_STRAND = 0, DOWN_STRAND, UP_STRAND } FexStrand ;

typedef enum { NO_PHASE = 0, PHASE_0, PHASE_1, PHASE_2 } FexPhase ;

/* as in BLAST*, i.e. target is DNA, Protein, DNA translated */
typedef enum { N_HOMOL, X_HOMOL, TX_HOMOL } FexHomolType ;

typedef enum { CLONE_END, 5_SPLICE, 3_SPLICE } FexBoundaryType ;

typedef enum { OK_STATUS = 0, NO_WRITE_ACCESS, ILLEGAL_FEATURE, 
	       ILLEGAL_ATTRIBUTE 
             } FexStatus ;

/*********** opaque types for server handles ************/

typedef struct DatabaseContext *FexDB ;
typedef struct SequenceContext *FexSeq ; /* should cache DB handle */
typedef unsigned int FexID ;

/*********** local types corresponding to CORBA structs ***********/


/* Describes the type of features to be found in a particular feature set,   */
/* the set is indentified by its name. The structure can be quizzed to check */
/* details such as "are these features strand sensitive ?".                  */
/*                                                                           */

typedef struct {
  char *name ;						    /* e.g. "Genewise predictions" */
  FexFeatureType type ;					    /* e.g. INTRON_FEATURE */
  char *category ;		/* GFF_feature, BioPerl primary_tag */
  char *source ;		/* GFF_source = ?? */
  BOOL isStrandSensitive ;
  BOOL isFrameSensitive ;	/* phase becomes important */
  BOOL isScore ;
  FexHomolType homolType ;	/* for Homols only */
  char *targetDataSource ;	/* for Homols only */
  FexBoundaryType boundType ;	/* for Boundaries only */
  BOOL isWritable ;
  Array legalAttributeTags ;	/* of char*, allowable attributes for this feature set. */
} FexFeatureSetDataInfo ;


/* the following is used to store alignment gap information */
typedef struct
{
  int s1, s2 ;			/* coords in input space */
  int r1, r2 ;			/* coords in output space */
} FexAlignBlock ;	/* this is identical to SMapMap in smap.h */



/* A feature, it will be of type FeatureType and be indentified uniquely by  */
/* some kind of database id, but also by a text name.                        */

/* Big decision:
   The primary Feature object is a union, not set of subclasses 
   inheriting from a core class.
   This simplifies and helps make transfer protocols efficient for C
   in particular.
   Problems are:
   Efficiency: core feature is 26 bytes assuming 1 byte per enum.
   The biggest union components add 18 extra bytes, and a high 
   fraction of the features (e.g. all homols) will need the max 
   anyway.  It doesn't seem worth separating the data structures and 
   duplicating lots of calls, requiring switches etc.

   It may turn out to be useful to use the style of struct used for XEvents.

   Elegance: in the eye of the beholder, but we'll try to keep it
   clean.
*/

typedef struct {
  FEXFeatureType type ;					    /* e.g. INTRON_FEATURE */
  FEXID id ;						    /* unique DB identifier. */
  char *name ;						    /* e.g. bA404F10.4.mRNA */
  int x1, x2 ;						    /* start, end */
  FexStrand strand ;
  FexPhase phase ;
  float score ;
  char *text ;						    /* What is this for ? */
  union
  {
    struct
    {
      int y1, y2 ;					    /* targetStart, targetEnd */
      FexStrand targetStrand ;
      FexPhase targetPhase ;				    /* for TX_HOMOL */
      Array align ;					    /* of AlignBlock */
							    /* if align is null, then ungapped */
    } homol ;
    struct
    {
      int cdsStart, cdsEnd ;
      BOOL startNotFound ;				    /* is this really a bool ? */
      FexPhase cdsPhase ;				    /* = NO_PHASE unless startNotFound */
      BOOL endNotFound ;				    /* is this really a bool ? */
      Array exons ;					    /* of Feature */
    } transcript ;
    struct
    {
      FexID transcriptID ;
    } exon ;
  } feature ;

} Feature ;





/**********************************************/

/* There will need to be a set of "database management" type functions       */
/* I've trivially added a couple here just as a placeholder to remind us     */
/* that these will be required.                                              */
FexDB fexOpenDatabase(char *locator, char *user, char *password) ;
void fexCloseDatabase(FexDB db) ;
BOOL fexCheckWriteAccess(FexDB db) ;
BOOL fexSetWriteAccess(FexDB db, BOOL access) ;



/* Ask for all possible feature sets for sequences.                          */
Array fexQueryFeatureSets(FexDB db) ;

/* Assume we don't need to ask for all possible basic features.              */


/* end = 0 for end_of_sequence, so (start, end) = (1,0) for whole sequence */
FexSeq fexSequence(FexDB db, char* seqName, int start, int end) ;

char* fexDNA(FexSeq context) ;				    /* returns IUPAC plus '-' for blank */

Array fexQuerySeqFeatSets(FexSeq context) ;	    /* Array of char* of feature set names */

FexFeatureSetDataInfo *fexFeatSetDataInfo(FexDB db, char *fsetName) ;
							    /* Get the info. for just one feature */
							    /* set. */

Array fexTypeFeatureSetDataInfo(FEXContext context, FEXFeatureType type) ;
							    /* Get info. for all features of a */
							    /* certain type. */

/* Get the info. for all feature sets available for a piece of sequence.     */
Array fexAllFeatureSetDataInfo(FexDB db) ;


/* Get features by feature set name.                                         */
Array fexGetFeatures(FexContext context, Array feature_names) ; /* Array of Feature structs */

/* Get features by basic type.                                               */
Array fexGetFeaturesType(FexContext context, FEXFeatureType type) ;

/* Clear up all of the sequence on the server and locally.                   */
void fexDestroySequence(FexSeq context) ;




/*****************************************************************************/
/**********************     search     ***************************************/

/* We separated this out from FeatureSet, so can get information on where
   to find the sequence from other sources than the database.  I guess the
   fetch thing should also be accessed via URLget, and we should set up
   interfaces for that.
*/

typedef struct {
  char *urlFormat ;	/* printf style format with %s for target name */
  char *fetchFormat ;	/* similar format, but to fetch sequence */
  char *removePrefix ;	/* prefix to remove from names before substituting in formats */
  /* Apollo has something good here that is a bit more powerful for good reasons */
} FexTargetDataSource ;

FexTargetDataSource *fexTargetDataSource(FexDB db, char* dataSource) ;



/*****************************************************************************/
/**********************     drawing    ***************************************/

/* We separated this out from FeatureSet, so can get information on how
   to display the features from sources other than the database, perhaps
   a user preference file.
*/

typedef enum { BY_WIDTH, BY_OFFSET, BY_HISTOGRAM } FexDrawStyle ;
typedef enum { OVERLAP, BUMP, CLUSTER } FexOverlapStyle ;


typedef struct
{
  Colour outline ;					    /* Surround/line colour. */
  Colour foreground ;					    /* Overlaid on background. */
  Colour background ;					    /* Fill colour. */
  float right_priority ;
  FexDrawStyle drawStyle ;
  FexOverlapStyle overlapStyle ;
  float width ;						    /* column width */
  float min_mag, max_mag ;
  float min_score, max_score ;
  BOOL  showText ;
  BOOL  showUpStrand ;
} FexFeatureSetDrawInfo ;


FexFeatureSetDrawInfo *fexFeatureSetDrawInfo(FexDB db, char *feature_set_name) ;
							    /* Get a single sets drawing info. */
Array fexFeatureSetsDrawInfo(FexDB db, Array feature_set_names) ;
							    /* Get multiple sets of draw info. */


/*****************************************************************************/
/*********************    write back    **************************************/

/* In the following, attributes is additional textual information in 
   attribute/value per line format (as for .ace files).  If you want to
   communicate gene_name hints/info back and forth with the server you
   can use this.  Also attributes to be assigned to the gene.  The client
   will just neutrally put these up and allow them to be edited, deleted
   and added to, and passed back to the server.  The attributeTagList field 
   of FexFeatureSetDataInfo lists the fields allowed for each feature set.
*/

typedef struct
{
  char *tag ;
  char *value ;
} Attribute ;

FexStatus fexNewFeature(FexDB db, char *fsetName, FexID *dbidPtr,
			Feature *feature,  Array attributes) ;

FexStatus fexReplaceFeature(FexDB db, char *fsetName, FexID dbid,
			    Feature *feature, Array attributes) ;
		    /* if feature is null, just replace attributes and vice versa */

FexStatus fexKillFeature(FexDB db, FexID dbid, Array attributes) ;
	/* may want to change attributes because "kill" will only archive,
	   depending on server behaviour */

/* need also */

Array fexGetAttributes(FexDB db, FexID dbid) ;  /* of FexAttribute */

/* Note there is no way to write a gene as such, nor a naked exon.

   010811 RD: Current proposal is as follows
   Within an fset:

   In the interface, a "curate" action on a transcript does the followring:
     - if identical to a curated transcript then killTranscript or editAttributes
     - else options are storeTranscript, or replaceTranscript for each that 
       it overlaps.

   If you store a transcript not overlapping any genes it creates a new gene.  
   On the server side, an idea to explore would be that if this new gene 
   overlaps one or more history genes then it is marked as having split 
   from the most recent that it overlaps.

   If you store a transcript overlapping only transcripts from one gene, then
   you can either replace one of those transcripts or create a new one.

   If you store a transcript that overlaps multiple current genes then it
   must replace one of the existing transcripts.  This is because the new
   transcript will merge all the genes covered, and the requirement to
   replace an existing transcript allows us to determine which gene the
   resulting merge becomes.

   Similarly if you were allowed to kill a transcript that acts as a 
   bridge, hence creating new genes by splitting, it would not be clear
   which child should keep the parent's name.  So I plan to ban such kills,
   requiring a replace action to remove the bridging transcript, so it is
   clear which child inherits the name (the one containing the replaced 
   transcript).

   Of course all these restrictions would go if the gene identifier model
   was that the products of merges and splits all had new identifiers.
   So I guess the restrictions listed above should be a client option,
   maybe settable via a server.  Or perhaps should be rules the server
   uses to reject a curate event.
*/

/******************* end of file ***********************/
#endif  /* FEX_DEF */

