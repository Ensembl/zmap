/*  file: seqregionconvert.c
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
#include <wh/method.h>
#include <whooks/systags.h>
#include <whooks/classes.h>

static methodID srMethCreate(SeqRegion *region, KEY methodKey);
static srMeth *methodFromKey(Array methods, KEY key);
static methodID srMethodMake(SeqRegion *region, KEY key);

/* exported functi
on */
srMeth *srMethodFromID(SeqRegion *region, methodID id)
{
  int i;

  if (region->methods)
    for (i=0; i<arrayMax(region->methods); i++)
      {
	srMeth *meth = arrp(region->methods, i, srMeth);
	if (meth->id == id)
	  return meth;
      }
  return NULL;
}

static srMeth *methodFromKey(Array methods, KEY key)
{
  int i;
  
  key = lexAliasOf(key); /* use canonical key */
  
  if (methods)
    for (i=0; i<arrayMax(methods); i++)
      {
	srMeth *meth = arrp(methods, i, srMeth);
	if (meth->key == key)
	  return meth;
      }
  return NULL;
}
 
static methodID srMethodMake(SeqRegion *region, KEY key)
{
  srMeth *meth;

  if ((meth = methodFromKey(region->methods, key)))
    return meth->id;
  else if ((meth = methodFromKey(region->oldMethods, key)))
    {
      srMeth *new = arrayp(region->methods, arrayMax(region->methods), srMeth);
      *new = *meth;
      return new->id;
    }
  else
    return srMethCreate(region, key);
}

static methodID srMethCreate(SeqRegion *region, KEY methodKey)
{
  srMeth *meth;
  OBJ obj ;
  int k;
  char *text;

  if (class(methodKey) != _VMethod)
    messcrash ("methodAdd() - methodKey not in class 'Method'");

  if (!(obj = bsCreate (methodKey))) 
    return 0;
  
  meth = arrayp(region->methods, arrayMax(region->methods), srMeth);
  meth->id = region->idc++;
  meth->key = methodKey;
  meth->name = str2p(name(methodKey), region->bucket);
  meth->flags &= ~METHOD_CALCULATED ;/* unset flag */
  meth->flags |= METHOD_DONE ;	/* set flag */
  meth->no_display = FALSE ;
  meth->colour = 0 ;
  meth->CDS_colour = 0 ;
  meth->upStrandColour = 0 ;
  meth->width = 0.0 ;
  meth->symbol = '\0' ;
  meth->priority = 0.0 ;
  meth->minScore = meth->maxScore = 0.0 ;
  

  if (bsGetKeyTags (obj, str2tag("Colour"), &k))
    meth->colour = k - str2tag("White") ;      /* colour-tag to colour-enum */
  if (bsGetKeyTags(obj, str2tag("CDS_colour"), &k))
    meth->CDS_colour = k - str2tag("White") ;  /* colour-tag to colour-enum */
  bsGetData (obj, str2tag("Width"), _Float, &meth->width) ;
  bsGetData (obj, str2tag("Right_priority"), _Float, &meth->priority) ;
  if (bsGetData (obj, str2tag("Symbol"), _Text, &text))
    meth->symbol = *text;
  if (bsFindTag (obj, str2tag("Frame_sensitive")))
    meth->flags |= METHOD_FRAME_SENSITIVE ;
  if (bsFindTag (obj, str2tag("Strand_sensitive")))
    meth->flags |= METHOD_STRAND_SENSITIVE ;
  if (bsFindTag (obj, str2tag("Show_up_strand")))
    {
      meth->flags |= METHOD_SHOW_UP_STRAND ;
      if (bsGetKeyTags(obj, str2tag("Show_up_strand"), &k))
	meth->upStrandColour = k - str2tag("White") ;		
    }
  if (bsFindTag (obj, str2tag("Blastn")))
    meth->flags |= METHOD_BLASTN ;
  if (bsFindTag (obj, str2tag("Blixem_X")))
    meth->flags |= METHOD_BLIXEM_X ;
  if (bsFindTag (obj, str2tag("Blixem_N")))
    meth->flags |= METHOD_BLIXEM_N ;
  if (bsFindTag (obj, str2tag("Blixem_P")))
    meth->flags |= METHOD_BLIXEM_P ;
  if (bsFindTag (obj, str2tag("Belvu")))
    meth->flags |= METHOD_BELVU ;
  if (bsFindTag (obj, str2tag("Bumpable")))
    meth->flags |= METHOD_BUMPABLE ;
  if (bsFindTag (obj, str2tag("Cluster")))
    meth->flags |= METHOD_CLUSTER ;
  if (bsFindTag (obj, str2tag("EMBL_dump")))
    meth->flags |= METHOD_EMBL_DUMP ;
  if (bsFindTag (obj, str2tag("Percent")))
    {
      meth->flags |= METHOD_PERCENT ;
      meth->minScore = 25 ;	/* so actual display is linear */
      meth->maxScore = 100 ;	/* can override explicitly */
    }

  if (bsGetData (obj, str2tag("Score_bounds"), _Float, &meth->minScore)
      && bsGetData (obj, _bsRight, _Float, &meth->maxScore))
    {
      if (bsFindTag (obj, str2tag("Score_by_offset")))
	meth->flags |= METHOD_SCORE_BY_OFFSET ;
      else if (bsFindTag (obj, str2tag("Score_by_width")))
	meth->flags |= METHOD_SCORE_BY_WIDTH ;
      else if (bsFindTag (obj, str2tag("Score_by_histogram")))
	{ 
	  meth->flags |= METHOD_SCORE_BY_HIST ;
	  meth->histBase = meth->minScore ;
	  bsGetData (obj, _bsRight, _Float, &meth->histBase) ;
	}
    }

  if (bsFindTag (obj, str2tag("Show_text")))
    meth->showText = TRUE;
  else
    meth->showText = FALSE;

  if (bsFindTag(obj, str2tag("No_display")))
    meth->no_display = TRUE;
  else
    meth->no_display = FALSE;

  meth->minMag = 0;
  bsGetData (obj, str2tag("Min_mag"), _Float, &meth->minMag) ;

  meth->maxMag = 0;
  bsGetData (obj, str2tag("Max_mag"), _Float, &meth->maxMag) ;

  bsDestroy (obj) ;
  return meth->id;
}

void seqRegionConvert(SeqRegion *region)
{
  STORE_HANDLE localHandle = handleCreate();
  KEYSET allKeys = sMapKeys(region->smap, localHandle);
  int i;
  Array units = arrayHandleCreate(256, BSunit, localHandle);

  region->segs = arrayReCreate(region->segs, 1000, SEG);
  region->methods = arrayCreate(100, srMeth);

  for (i = 0 ; i < keySetMax(allKeys) ; i++)
    {
      KEY method, key = keySet(allKeys, i);
      SMapKeyInfo *info = sMapKeyInfo(region->smap, key);
      int x1, x2;
      SMapStatus status ;
      SEG *seg;
      OBJ obj;
      
      if ((obj = bsCreate(key)) && bsGetKey(obj, str2tag("Method"), &method))
	{
	  Coord cdsStart, cdsEnd;
	  srType type = SR_SEQUENCE; /* default */
	  srPhase phase = 1;
	  BOOL isStartNotFound = FALSE;
	  BOOL isCDS = FALSE;
	  Array exons = NULL;
	  
	  if (bsFindTag(obj, str2tag("Start_not_Found")))
	    {
	      isStartNotFound = TRUE;
	      bsGetData(obj, _bsRight, _Int, &phase);
	    }
	  
	  if (bsFindTag(obj, str2tag("Source_Exons")) &&
	      (bsFlatten (obj, 2, units)))
	    {
	      int j;	      type = SR_TRANSCRIPT;
	      sMapUnsplicedMap(info, 1, 0, &x1, &x2, NULL, NULL);
	      dnaExonsSort (units) ;
	      
	      exons = arrayHandleCreate(20, srExon, region->handle); 
	      for(j = 0; j < arrayMax(units); j += 2)
		{
		  srExon *e = arrayp(exons, arrayMax(exons), srExon);
		  sMapUnsplicedMap(info, 
				   arr(units, j, BSunit).i, arr(units, j+1, BSunit).i,
				   &e->x1, &e->x2, NULL, NULL);
		}	      
	    }
	  else
	    sMapMap(info, 1, 0, &x1, &x2, NULL, NULL);
	  
	  if (bsFindTag(obj, str2tag("CDS")))
	    {
	      int c1, c2;
	      
	      type = SR_TRANSCRIPT;
	      if (bsGetData(obj, _bsRight, _Int, &c1) && 
		  bsGetData(obj, _bsRight, _Int, &c2))
		{
		  isCDS = TRUE;
		  sMapMap(info, c1, c2, &cdsStart, &cdsEnd, NULL, NULL);
		  if (isStartNotFound && c1 != 1)
		    messerror("StartNotFound but CDS doesn't start at 1 in object %s",
			      name(key));
		}
	      /* If no source exons, fake one up which covers the whole of the CDS. */
	      if (!exons)
		{ 
		  exons = arrayHandleCreate(1, srExon, region->handle); 
		  array(exons, 0, srExon).x1 = cdsStart;
		  array(exons, 0, srExon).x2 = cdsEnd;
		}
	    }
	  
	  seg = arrayp(region->segs, arrayMax(region->segs), SEG);
	  seg->id = str2p(name(key), region->bucket);
	  seg->type = type;
	  seg->x1 = x1;
	  seg->x2 = x2;
	  seg->phase = phase;
	  seg->method = srMethodMake(region, method);if (type == SR_TRANSCRIPT);
	  
	  if (type = SR_TRANSCRIPT)
	    {  
	      if (!isCDS)
		sMapMap(info, 1, 0, &cdsStart, &cdsEnd, NULL, NULL);
	      
	      if (bsFindTag(obj, str2tag("End_not_found")))
		seg->u.transcript.endNotFound = TRUE;
	      else
		seg->u.transcript.endNotFound = FALSE;
		  
	      
	      seg->u.transcript.cdsStart = cdsStart;
	      seg->u.transcript.cdsEnd = cdsEnd;
	      seg->u.transcript.exons = exons;
	    }
			  
	  if (bsFindTag(obj, str2tag("Feature")) &&
	      bsFlatten (obj, 5, units))
	    {
	      int j ;
	      
	      for (j = 0 ; j < arrayMax(units) ; j += 5)
		{ 
		  BSunit *u = arrayp(units, j, BSunit);
		  SEG *seg;
		  SMapStatus status ;
		  
		  status = sMapMap(info, u[1].i, u[2].i, &x1, &x2, NULL, NULL) ;
		  if (status & SMAP_STATUS_NO_OVERLAP)
		    continue;
		  
		  seg = arrayp(region->segs, arrayMax(region->segs), SEG);
		  seg->id = str2p(name(u[0].k), region->bucket);
		  seg->type = SR_FEATURE;
		  seg->x1 = x1;
		  seg->x2 = x2;
		  seg->score = u[3].f;
		  seg->method = srMethodMake(region, u[0].k);
		}
	    }
	  if (bsFindTag(obj, str2tag("Homol")))
	    {
	      

	    }

	}
      
      bsDestroy(obj);
	
    }
  handleDestroy(localHandle);
}
