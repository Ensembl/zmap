/*  file: seqregion.h
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
 * -------------------------------------------------------------------
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
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 */

#include <wzmap/stringbucket.h>
#include <wh/smap.h>
/*
This file defines the seqRegion interface.

The seqregion package populates a data structure with all the information
about a particular DNA area. This package is the interface between the drawing 
code  and the underlying database engine. Different version of the code access
acedb directly via the BS and sMap interfaces, or a remote server via Fex.

The public part of the seqRegion datastructure is exposed to the
drawing code. This may not depend on any back-end data structures or types: 
No KEYS, classes or sMapInfos allowed. 

*/

/* 

Coordinates.

We define two coordinate types.

1) Coord: coordinates in the root object in the current direction.
2) InvarCoord: coordinates in the root object in absolute direction.

Seqregion can calculate a region on either the forward or reverse strand 
of the root object. Most of the data it produces are Coords, these have the 
property that they do not change as the region is expanded/contracted and that
they increase the 5'->3' direction on the current strand. The later property
is important for drawing, but it means the _Coords_are_not_invariant_ over a 
reverse-complement. All of the data-structures in seqRegion  containing Coords 
get re-calculated of rev-comp, so this is not a problem. 

For calling code to store coordinates over a  rev-comp it is 
necessary to use InvarCoords. Routines are supplied to convert between
Coords and Invarcoords. Convertinga Coord to an Invarcoord, 
reverse-complementing and then converting back from InvarCoord to Coord
will result in a new coordinate which points to the same base as the original.
There is one gotcha to be aware of: consider an object with start and end
coordinates, start < end as usual, and we store its identity as the 
InvarCoord of the start. After a rev-comp we get the coord back, but it is now
the _larger_ of the objects coordinates - ie the end coodinate.  

*/
typedef int Coord;
typedef int InvarCoord;

typedef int methodID;

struct seqRegionStruct {

  /* Public elements - these are visible to the caller */
  Coord area1, area2;
  Array dna;
  Array segs;
  Array methods, oldMethods;
  int length;

  /* Private elements - acedb version. */
  StringBucket *bucket;
  KEY rootKey;
  BOOL rootIsReverse;
  STORE_HANDLE handle;
  SMap *smap;
  methodID idc; /* increment to make unique ids. */
};

typedef struct seqRegionStruct SeqRegion;

#define SEQUENCE 1
#define FEATURE 2

typedef enum { NO_STRAND = 0, DOWN_STRAND, UP_STRAND } srStrand;
typedef enum { NO_PHASE = 0, PHASE_0, PHASE_1, PHASE_2 } srPhase;
typedef enum { 
  SR_DEFAULT = 0, SR_HOMOL, SR_FEATURE,
  SR_SEQUENCE, SR_TRANSCRIPT
} srType;

typedef struct {
  Coord x1, x2;
} srExon;

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
      Array align;
    } homol;
    struct {
      Coord cdsStart, cdsEnd;
      BOOL endNotFound;
      Array exons;
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
  KEY key; /* private */
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

  
/* Routines */

/* Create a new one - must call srActivate next */
SeqRegion *srCreate(STORE_HANDLE handle);

/* Finds root and orientation: returns coords of seq in r1 and r2 */
BOOL srActivate(char* seqspec, SeqRegion *region, Coord *r1, Coord *r2);

/* Populate region between x1 and x2 */
/* This may be called repeatedly as required with changing x1, x2 */
void srCalculate(SeqRegion *region, Coord x1, Coord x2);

/* Determine DNA for region. Call this again if you call srCalculate again. */
BOOL srGetDNA(SeqRegion *region);

/* Flip orientation of region. */
void srRevComp(SeqRegion *region);

/* Coordinate conversion. */
InvarCoord srInvarCoord(SeqRegion *region, Coord coord);
Coord srCoord(SeqRegion *region, InvarCoord coord);

/* Move from id to struct. */
srMeth *srMethodFromID(SeqRegion *region, methodID id);
