/*  Last edited: Jun 25 11:20 2004 (rnc) */
/*  file: seqregion.h
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

#ifndef SEQREGION_H
#define SEQREGION_H

#include <../acedb/acedb.h>
#include <../zmapWindow/zmapcalls.h>


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

#define SEQUENCE 1
#define FEATURE 2

  
/* Routines */

/* Create a new one - must call srActivate next */
ZMapRegion *srCreate(STORE_HANDLE handle);

/* Coordinate conversion. */
InvarCoord srInvarCoord(ZMapRegion *zMapRegion, Coord coord);

Coord srCoord(ZMapRegion *zMapRegion, InvarCoord coord);

/* Move from id to struct. */
srMeth *srMethodFromID(ZMapRegion *zmapRegion, methodID id);

#endif
/********************** end of file ************************/
