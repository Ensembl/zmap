/*  File: seqregion.h
 *  Author: Simon Kelley (srk@sanger.ac.uk)
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
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jun 30 13:57 2004 (edgrif)
 * Created: Wed Jun 30 13:50:48 2004 (edgrif)
 * CVS info:   $Id: seqregion.h,v 1.6 2004-07-01 09:26:24 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef SEQREGION_H
#define SEQREGION_H

#include <ZMap/zmapWindow.h>


/*

THIS HAS NOT COME TO PASS...........THE WHOLE SEQREGION FILE MAY
DISAPPEAR........



This file defines the seqRegion interface.

The seqregion package populates a data structure with all the information
about a particular DNA area. This package is the interface between the drawing 
code  and the underlying database engine. Different version of the code access
acedb directly via the BS and sMap interfaces, or a remote server via Fex.

The public part of the seqRegion datastructure is exposed to the
drawing code. This may not depend on any back-end data structures or types: 
No KEYS, classes or sMapInfos allowed. 

*/



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#define SEQUENCE 1
#define FEATURE 2
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Create a new one - must call srActivate next */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* totally unused currently........... */

ZMapRegion *srCreate() ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* Coordinate conversion. */
InvarCoord srInvarCoord(ZMapRegion *zMapRegion, Coord coord);
Coord srCoord(ZMapRegion *zMapRegion, InvarCoord coord);


/* Move from id to struct. */
srMeth *srMethodFromID(ZMapRegion *zmapRegion, methodID id);




#endif
/********************** end of file ************************/
