/*  File: seqregion.c
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
 * Last edited: Jul 16 09:50 2004 (edgrif)
 * Created: Wed Jun 30 13:38:10 2004 (edgrif)
 * CVS info:   $Id: seqregion.c,v 1.6 2004-07-16 08:51:29 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <seqregion.h>


/* Returns a coordinate in the absolute direction. */
InvarCoord srInvarCoord(ZMapRegion *zMapRegion, Coord coord)
{
  if (zMapRegion->rootIsReverse)
    return coord - zMapRegion->length + 1;
  else
    return coord;
}



/* Returns a coordinate in the current direction. */
Coord srCoord(ZMapRegion *zMapRegion, InvarCoord coord)
{
   if (zMapRegion->rootIsReverse)
    return coord - zMapRegion->length + 1;
  else
    return coord;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* this is not called from anywhere at the moment..... */

/* Creates a new, empty ZMapRegion structure. */
ZMapRegion *srCreate(void)
{
  ZMapRegion *zMapRegion = (ZMapRegion*)g_malloc0(sizeof(ZMapRegion)) ;

  zMapRegion->area1 = zMapRegion->area2 = 0 ;
  zMapRegion->methods = NULL ;
  zMapRegion->oldMethods = NULL ;
  zMapRegion->dna = NULL ;

  return zMapRegion ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* Retrieves a method structure from the methods array based on
 * the ID it receives.  Knows nothing of AceDB, which is why it's
 * here, but could as well be in zmapcalls.c with it's sisters. */
srMeth *srMethodFromID(ZMapRegion *zMapRegion, methodID id)
{
  srMeth *result = NULL ;

  if (zMapRegion && zMapRegion->methods)
    {
      int i ;

      for (i = 0 ; i < zMapRegionGetMethods(zMapRegion)->len ; i++)
	{
	  srMeth *meth = g_ptr_array_index(zMapRegionGetMethods(zMapRegion), i) ;

	  if (meth->id == id)
	    {
	      result = meth ;
	      break ;
	    }
	}
    }

  return result ;
}

 
/********************** end of file **********************************/
  


			     

  
