/*  Last edited: Jun 29 10:31 2004 (edgrif) */
/*  file: zmapcommon.h
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
 *-------------------------------------------------------------------
 * Zmap is free software; you can redistribute it and/or
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
 * and was written by
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 *
 * zmapcommon.h holds definitions which are required both by xace and zmap.
 */

#ifndef ZMAPCOMMON_H
#define ZMAPCOMMON_H


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <../acedb/graph.h>
#include <../acedb/gex.h>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#include <ZMap/zmapFeature.h>
/* 

Coordinates.

We define two coordinate types.

1) Coord: coordinates in the root object in the current direction.
2) InvarCoord: coordinates in the root object in absolute direction.

Seqregion can calculate a region on either the forward or reverse strand 
of the root object. Most of the data it produces are Coords, these have the y
property that they do not change as the region is expanded/contracted and that
they increase the 5'->3' direction on the current strand. The latter property
is important for drawing, but it means the _Coords_are_not_invariant_ over a 
reverse-complement. All of the data-structures in seqRegion containing Coords 
get re-calculated on rev-comp, so this is not a problem. 

For calling code to store coordinates over a  rev-comp it is 
necessary to use InvarCoords. Routines are supplied to convert between
Coords and Invarcoords. Converting a Coord to an Invarcoord, 
reverse-complementing and then converting back from InvarCoord to Coord
will result in a new coordinate which points to the same base as the original.
There is one gotcha to be aware of: consider an object with start and end
coordinates, start < end as usual, and we store its identity as the 
InvarCoord of the start. After a rev-comp we get the coord back, but it is now
the _larger_ of the objects coordinates - ie the end coodinate.  

*/

typedef float ScreenCoord;
typedef int   InvarCoord;
typedef int   VisibleCoord;

typedef enum { NO_STRAND = 0, DOWN_STRAND, UP_STRAND } srStrand;
typedef enum { NO_PHASE = 0, PHASE_0, PHASE_1, PHASE_2 } srPhase;
typedef enum { 
  SR_DEFAULT = 0, SR_HOMOL, SR_FEATURE,
  SR_SEQUENCE, SR_TRANSCRIPT
} srType;

extern char dnaDecodeChar[] ;	/* this is the mapping used to decode a single base */

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
  BOOL       rootIsReverse;
};

typedef struct zMapRegionStruct ZMapRegion;

/* zmap-flavour seg structure that holds the data to be displayed */

struct segStruct {
  char *id; /* stringBucket */
  methodID method;
  /* NOTE: srType BOTH discriminates the union below, _and_ controls
     the sort of column made.  Two segs with the same method but
     different types will end up in different columns. */
  srType type;
  srStrand strand;
  srPhase phase;
  float score;
  Coord x1, x2;
  union {
    struct {
      int y1, y2;
      srStrand strand;
      float score;
      GPtrArray align;
    } homol;
    struct {
      Coord cdsStart, cdsEnd;
      BOOL endNotFound;
      GPtrArray exons;
    } transcript;
    struct {
      char *id; /* backpointer */
    } exon;
  } u;
};

typedef struct segStruct SEG;

/* Coord structure */

typedef struct {
  Coord x1, x2;
} srExon;

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
  BOOL showText, no_display;
} srMeth;


/* callback routines **************************************/
/* This routine returns the initial set of zmap-flavour segs
 * when zmap is first called. */

typedef void (*Activate_cb)(void *seqRegion,
			    char *seqspec, 
			    ZMapRegion *zMapRegion, 
			    Coord *r1, 
			    Coord *r2, 
			    STORE_HANDLE *handle);

/* This routine is called by the reverse-complement and
 * recalculate routines. */

typedef void (*Calc_cb)    (void *seqRegion, 
			    Coord x1, Coord x2,
			    BOOL isReverse);
     
#endif  

/************************ end of file *********************/
