/*  Last edited: Jun 17 13:53 2004 (rnc) */
/*  file: seqregion.c
 *  Author: Simon Kelley (srk@sanger.ac.uk)
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

#include <seqregion.h>


/* srInvarCoord ***************************************************/
/* Returns a coordinate in the absolute direction. */

InvarCoord srInvarCoord(ZMapRegion *zMapRegion, Coord coord)
{
  if (zMapRegion->rootIsReverse)
    return coord - zMapRegion->length + 1;
  else
    return coord;
}

/* srCoord ********************************************************/
/* Returns a coordinate in the current direction. */

Coord srCoord(ZMapRegion *zMapRegion, InvarCoord coord)
{
   if (zMapRegion->rootIsReverse)
    return coord - zMapRegion->length + 1;
  else
    return coord;
}



/* srCreate ******************************************************/
/* Creates a new, empty ZMapRegion structure.Does nothing with the handle. */

ZMapRegion *srCreate(STORE_HANDLE handle)
{
  ZMapRegion *zMapRegion = (ZMapRegion*)malloc(sizeof(ZMapRegion));

  zMapRegion->area1 = zMapRegion->area2 = 0;
  zMapRegion->methods = NULL;
  zMapRegion->oldMethods = NULL;
  zMapRegion->dna = NULL;
  return zMapRegion;
}

/* srMethodFromID ***********************************************/
/* Retrieves a method structure from the methods array based on
 * the ID it receives.  Knows nothing of AceDB, which is why it's
 * here, but could as well be in zmapcalls.c with it's sisters. */

srMeth *srMethodFromID(ZMapRegion *zMapRegion, methodID id)
{
  int i;

  if (zMapRegion && zMapRegion->methods)
    for (i = 0; i < zMapRegionGetMethods(zMapRegion)->len; i++)
      {
	srMeth *meth = g_ptr_array_index(zMapRegionGetMethods(zMapRegion), i);
	if (meth->id == id)
	  return meth;
      }
  return NULL;
}

 
/********************** end of file **********************************/
  


			     

  
