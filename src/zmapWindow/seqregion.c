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

#include <wh/acedb.h>
#include <wh/regular.h>
#include <wh/a.h>
#include <wh/lex.h>
#include <wh/smap.h>
#include <wzmap/stringbucket.h>
#include <wzmap/seqregion.h>



InvarCoord srInvarCoord(SeqRegion *region, Coord coord)
{
  if (region->rootIsReverse)
    return coord - region->length + 1;
  else
    return coord;
}

Coord srCoord(SeqRegion *region, InvarCoord coord)
{
   if (region->rootIsReverse)
    return coord - region->length + 1;
  else
    return coord;
}

BOOL srActivate(char *seqspec, SeqRegion *region, Coord *r1, Coord *r2)
{
  
  int x1, x2;
  KEY key;

  if (!lexClassKey(seqspec, &key) || 
      !sMapTreeRoot(key, 1, 0, &region->rootKey, &x1, &x2))
    return FALSE;
  
  region->length = sMapLength(region->rootKey);
  
  /* If the key is reversed wrt to the root, record that we need to 
     make the sMap reversed, and calculate the coords that the key
     has on the reversed sMap. */
  
  if (x1 > x2)
    {
      *r1 = region->length - x1 + 1;
      *r2 = region->length - x2 + 1;
      region->rootIsReverse = 1;
    }
  else
    {
      region->rootIsReverse = 0;
      *r1 = x1;
      *r2 = x2;
    }

  return TRUE;
}

static void calcSeqRegion(SeqRegion *region, 
			  Coord x1, Coord x2,
			  BOOL isReverse)
{
  if (x1 < 1) x1 = 1;
  if (x2 > region->length) x2 = region->length;
  
  /* Note: this currently throws everything away and starts again.
     It's designed by capable of incrementally adding to existing structures.
  */
  
  if (region->smap)
    sMapDestroy(region->smap);
  
  if (region->dna)
    arrayDestroy(region->dna);
 
  if (region->bucket)
    sbDestroy(region->bucket);
  
  region->oldMethods = region->methods;
  region->methods = NULL;
  /* currently just put in new area - might need these to see what we have 
     already got. */

  
  region->area1 = x1;
  region->area2 = x2;
  region->rootIsReverse = isReverse;
  
  region->bucket = sbCreate(region->handle);
  if (region->rootIsReverse)
    region->smap = sMapCreateEx(region->handle, region->rootKey, 
				0, region->length, 
				region->area1, region->area2, NULL);
  else
    region->smap = sMapCreateEx(region->handle, region->rootKey, 
				region->length, 1, 
				region->area1, region->area2, NULL);
  
  seqRegionConvert(region);
  arrayDestroy(region->oldMethods);
  
}

BOOL srGetDNA(SeqRegion *region)
{
  
  /* TODO handle callback stuff */
  if (!region->dna)
    region->dna = sMapDNA(region->smap, region->handle, 0);
  
  return region->dna != NULL;
}

void srCalculate(SeqRegion *region, Coord x1, Coord x2)
{
  calcSeqRegion(region, x1, x2, region->rootIsReverse);
}

void srRevComp(SeqRegion *region)
{
  calcSeqRegion(region, 
		region->length - region->area2 + 1,
		region->length - region->area1 + 1,
		!region->rootIsReverse);
}

SeqRegion *srCreate(STORE_HANDLE handle)
{
  /* Get out own handle */
  
  STORE_HANDLE newHandle = handleHandleCreate(handle);
  
  SeqRegion *region = halloc(sizeof(SeqRegion), newHandle);
  
  region->handle = newHandle;
  region->smap = NULL;
  region->dna = NULL;
  region->bucket = NULL;
  region->area1 = region->area2 = 0;
  region->methods = NULL;
  region->oldMethods = NULL;
  region->idc = 1;
  return region;
}

void seqRegionDestroy(SeqRegion *region)
{
  handleDestroy(region->handle);
}

/***********************************************************************/
  


			     

  
