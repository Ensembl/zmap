/*  Last edited: Jun 25 11:20 2004 (rnc) */
/*  file: zmapcalls.h
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
 */

#ifndef ZMAPCALLS_H
#define ZMAPCALLS_H

#include <../acedb/acedb.h>
#include <../acedb/regular.h>
#include <../acedb/lex.h>
#include <../acedb/smap.h>
#include <../acedb/method.h>
#include <../acedb/smap.h>
#include <../acedb/systags.h>  /* _Float, _Text, _bsRight, etc */
#include <../acedb/classes.h>  /* _VMethod */
#include <ZMap/zmapcommon.h>
#include <ZMap/zmapWindow.h>
#include <../zmapWindow/stringbucket.h>
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

struct seqRegionStruct {
  ZMapRegion *zMapRegion;

  /* Private elements - acedb version. */
  StringBucket *bucket;
  KEY rootKey;
  STORE_HANDLE handle;
  SMap *smap;
  methodID idc; /* increment to make unique ids. */
};

typedef struct seqRegionStruct SeqRegion;


  
/* Routines */

/* Finds root and orientation: returns coords of seq in r1 and r2 */
void srActivate(void *seqRegion, 
		char* seqspec, 
		ZMapRegion *zMapRegion, 
		Coord *r1, 
		Coord *r2, 
		STORE_HANDLE *handle);

/* Populate region between x1 and x2 */
/* This may be called repeatedly as required with changing x1, x2 */
void srCalculate(SeqRegion *region, Coord x1, Coord x2);

/* Determine DNA for region. Call this again if you call srCalculate again. */
void srGetDNA(SeqRegion *region);

void seqRegionConvert(SeqRegion *region);

/* Flip orientation of region. */
void srRevComp(SeqRegion *region);


#endif

/*********************** end of file ********************************/
