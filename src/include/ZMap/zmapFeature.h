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
 * Description: Data structures describing a genetic feature.
 *              
 * HISTORY:
 * Last edited: Jul 14 15:44 2004 (edgrif)
 * Created: Fri Jun 11 08:37:19 2004 (edgrif)
 * CVS info:   $Id: zmapFeature.h,v 1.7 2004-07-15 15:02:57 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_FEATURE_H
#define ZMAP_FEATURE_H

#include <glib.h>

/* A unique ID for each feature created, can be used to unabiguously query for that feature
 * in a database. The feature id ZMAPFEATUREID_NULL is guaranteed not to equate to any feature
 * and also means you can test using !(feature_id). */
typedef unsigned int ZMapFeatureID ;
enum {ZMAPFEATUREID_NULL = 0} ;



typedef int Coord ;					    /* we do need this here.... */



/* I'm not sure we want all of these defined here.....the coord stuff feels like it is different
 * from feature stuff........... */
typedef float ScreenCoord;
typedef int   InvarCoord;
typedef int   VisibleCoord;


/* AGAIN THIS FEELS LIKE ITS IN THE WRONG PLACE, ITS TO DO WITH DISPLAY, NOT FEATURES PER SE... */
/* structures *********************************************/
/* zMapRegionStruct is the structure zmap expects to hold
 * all the data required for the display, so whatever is
 * providing the data must populate this structure. */

struct zMapRegionStruct {
  Coord      area1, area2;
  GArray     *dna;
  GArray     *segs;
  GPtrArray  *methods, *oldMethods;
  int        length;
  gboolean   rootIsReverse;
};

typedef struct zMapRegionStruct ZMapRegion;







/* Unsure about this....probably should be some sort of key...... */
typedef int methodID ;


/* What about "sequence", atg, and allele as basic feature types ?           */
typedef enum {ZMAPFEATURE_INVALID = -1,
	      ZMAPFEATURE_BASIC = 0, ZMAPFEATURE_HOMOL,
	      ZMAPFEATURE_EXON, ZMAPFEATURE_INTRON, 
	      ZMAPFEATURE_TRANSCRIPT, ZMAPFEATURE_VARIATION,
	      ZMAPFEATURE_BOUNDARY, ZMAPFEATURE_SEQUENCE} ZMapFeatureType ;

typedef enum {ZMAPSTRAND_NONE = 0,
	      ZMAPSTRAND_DOWN, ZMAPSTRAND_UP} ZMapStrand ;

typedef enum {ZMAPPHASE_NONE = 0,
	      ZMAPPHASE_0, ZMAPPHASE_1, ZMAPPHASE_2} ZMapPhase ;

/* as in BLAST*, i.e. target is DNA, Protein, DNA translated */
typedef enum {ZMAPHOMOL_N_HOMOL, ZMAPHOMOL_X_HOMOL, ZMAPHOMOL_TX_HOMOL} ZMapHomolType ;

typedef enum {ZMAPBOUNDARY_CLONE_END, ZMAPBOUNDARY_5_SPLICE, ZMAPBOUNDARY_3_SPLICE } ZMapBoundaryType ;



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
  int t1, t2 ;						    /* coords in target sequence */
} ZMapAlignBlockStruct, *ZMapAlignBlock ;


/* the following is used to store mapping information of one span on to another, if we have
 * SMap in ZMap we can use SMap structs instead.... */
typedef struct
{
  int p1, p2 ;						    /* coords in parent. */
  int c1, c2 ;						    /* coords in child. */
} ZMapMapBlockStruct, *ZMapMapBlock ;



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
  ZMapFeatureID id ;					    /* unique DB identifier. */
  char *name ;						    /* e.g. "bA404F10.4.mRNA" */
  ZMapFeatureType type ;				    /* e.g. intron, homol etc. */

  Coord x1, x2 ;					    /* start, end of feature in absolute coords. */

  methodID method ;					    /* i.e. the "column" type */
  char *method_name ;					    /* temp...replace with quark ? */

  /* um, I'm not sure this is correct, surely a column is a column and the only thing that matters
   * is the method..... */
  /* NOTE: srType BOTH discriminates the union below, _and_ controls
     the sort of column made.  Two segs with the same method but
     different types will end up in different columns. */

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




/* Holds a set of ZMapFeature's. */
typedef struct ZMapFeatureSetStruct_
{
  char *source ;					    /* e.g. "Genewise predictions" */

  GArray *features ;					    /* An array of ZMapFeatureStruct. */

} ZMapFeatureSetStruct, *ZMapFeatureSet ;



/* Holds a set of ZMapFeatureSet's.
 * Structure would usually contain a complete set of data from a server for a particular span
 * of a sequence. */
typedef struct ZMapFeatureContextStruct_
{
  char *sequence ;					    /* The sequence to be displayed. */
  char *parent ;					    /* Name of parent, not be needed ? */
  int parent_length ;					    /* Length in bases of parent. */

  ZMapMapBlockStruct sequence_to_parent ;		    /* Shows how this sequence maps to its
							       ultimate parent. */
  ZMapMapBlockStruct features_to_sequence ;		    /* Shows how these features map to the
							       sequence, n.b. this feature set may only
							       span part of the sequence. */

  GData *features ;					    /* A set of ZMapFeatureSet. */

} ZMapFeatureContextStruct, *ZMapFeatureContext ;





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* SIMONS STUFF........ */

typedef struct methodStruct
{
  methodID id;
  char *name, *remark;
  unsigned int flags;
  int colour, CDS_colour, upStrandColour;
  float minScore, maxScore;
  float minMag, maxMag;
  float width ;
  char symbol ;
  float priority;
  float histBase;
  gboolean showText, no_display;
} srMeth;


/* FEX STUFF.... */

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
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* AND THIS IS OUR DERIVED STUFF.......... */


typedef enum {ZMAPCALCWIDTH_WIDTH, ZMAPCALCWIDTH_OFFSET, ZMAPCALCWIDTH_HISTOGRAM } ZMapFeatureWidthStyle ;
typedef enum {ZMAPOVERLAP_COMPLETE, ZMAPOVERLAP_BUMP, ZMAPOVERLAP_CLUSTER } ZMapFeatureOverlapStyle ;


typedef struct ZMapFeatureTypeStyleStruct_
{
  char *outline ;					    /* Surround/line colour. */
  char *foreground ;					    /* Overlaid on background. */
  char *background ;					    /* Fill colour. */
  float right_priority ;
  ZMapFeatureWidthStyle width_style ;
  ZMapFeatureOverlapStyle overlap_style ;
  float width ;						    /* column width */
  float min_mag, max_mag ;
  float min_score, max_score ;
  gboolean  showText ;
  gboolean  showUpStrand ;
} ZMapFeatureTypeStyleStruct, *ZMapFeatureTypeStyle ;





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* CURRENTLY NOT NEEDED I THINK...... */


typedef struct segStruct SEG;

/* Coord structure */

typedef struct {
  Coord x1, x2;
} srExon;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* ALL OF THIS NEEDS SOME ATTENTION.......... */

// the following structs all lifted from zmapcommon.h and may be unnecessary

/* zmap-flavour seg structure that holds the data to be displayed */

struct segStruct {
  char *id; /* stringBucket */
  methodID method;
  /* NOTE: srType BOTH discriminates the union below, _and_ controls
     the sort of column made.  Two segs with the same method but
     different types will end up in different columns. */
  //  srType type;
  //  srStrand strand;
  //  srPhase phase;
  float score;
  Coord x1, x2;
  union {
    struct {
      int y1, y2;
      //      srStrand strand;
      float score;
      GPtrArray align;
    } homol;
    struct {
      Coord cdsStart, cdsEnd;
      gboolean endNotFound;
      GPtrArray exons;
    } transcript;
    struct {
      char *id; /* backpointer */
    } exon;
  } u;
};

typedef struct segStruct SEG;


typedef struct methodStruct
{
  methodID id;
  char *name, *remark;
  unsigned int flags;
  int colour, CDS_colour, upStrandColour;
  float minScore, maxScore;
  float minMag, maxMag;
  float width ;
  char symbol ;
  float priority;
  float histBase;
  gboolean showText, no_display;
} srMeth;


/* callback routines **************************************/
/* This routine returns the initial set of zmap-flavour segs
 * when zmap is first called. */

typedef void (*Activate_cb)(void *seqRegion,
			    char *seqspec, 
			    ZMapRegion *zMapRegion, 
			    Coord *r1, 
			    Coord *r2) ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* snipped this off the end.... */
			    STORE_HANDLE *handle);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* This routine is called by the reverse-complement and
 * recalculate routines. */

typedef void (*Calc_cb)    (void *seqRegion, 
			    Coord x1, Coord x2,
			    gboolean isReverse);

     

#endif /* ZMAP_FEATURE_H */
