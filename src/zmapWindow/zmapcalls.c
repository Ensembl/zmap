/*  Last edited: Jan 30 11:34 2004 (rnc) */
/*  file: zmapcalls.c
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

#include <wzmap/zmapcalls.h>


/* function prototypes ********************************************/

static methodID srMethCreate  (SeqRegion *region, char *methodName);
static srMeth  *srMethFromName(Array methods    , char *methodName);
static methodID srMethodMake  (SeqRegion *region, char *methodName);

void srActivate(void *seqRegion, 
		char *seqspec, 
		ZMapRegion *zMapRegion, 
		Coord *r1, 
		Coord *r2, 
		STORE_HANDLE *handle);

static void calcSeqRegion(void *seqRegion, 
			  Coord x1, Coord x2,
			  BOOL isReverse);

BOOL zMapDisplay(Activate_cb srActivate,
		 Calc_cb calcSeqRegion,
		 void *region,
		 char *seqspec, 
		 char *fromspec, 
		 BOOL isOldGraph);

/* functions ******************************************************/
/* zMapCall is called from w6/display.c where it is registered in
 * display functions at line 1176.  It calls zMapDisplay, passing
 * pointers to 2 callback functions and a SeqRegion structure. The
 * callback functions create a SEGs (zmap flavour segs, not fmap) 
 * array in the zMapRegion member of the region structure. */

BOOL zMapCall(KEY key, KEY from, BOOL isOldGraph, void *app_data)
 {
  BOOL ret;
  STORE_HANDLE handle = handleCreate();
  SeqRegion *region = halloc(sizeof(SeqRegion), handle);

  char *keyspec = messalloc(strlen(className(key)) + strlen(name(key)) + 2);
  char *fromspec = messalloc(strlen(className(from)) + strlen(name(from)) + 2);
				  
  sprintf(keyspec, "%s:%s", className(key), name(key));
  sprintf(fromspec, "%s:%s", className(from), name(from));
  
  ret = zMapDisplay(srActivate, calcSeqRegion, region, keyspec, fromspec, isOldGraph);

  messfree(keyspec);
  messfree(fromspec);
  return ret;
}


/* srMethodMake ****************************************************/
/* If the required method already exists in either the methods or 
 * oldMethods array, uses that, else calls srMethCreate to make a
 * new one. */

static methodID srMethodMake(SeqRegion *region, char *methodName)
{
  srMeth *meth;

  if ((meth = srMethFromName(region->zMapRegion->methods, methodName)))
    return meth->id;
  else if ((meth = srMethFromName(region->zMapRegion->oldMethods, methodName)))
    {
      srMeth *new = arrayp(region->zMapRegion->methods, arrayMax(region->zMapRegion->methods), srMeth);
      *new = *meth;
      return new->id;
    }
  else
    return srMethCreate(region, methodName);
}


/* srMethFromName *************************************************/
/* Retrieves the method structure from the methods array, based
 * on the methodName which it recieves. */

static srMeth *srMethFromName(Array methods, char *methodName)
{
  int i;
  
  if (methods)
    for (i=0; i<arrayMax(methods); i++)
      {
	srMeth *meth = arrp(methods, i, srMeth);
	if (meth->name == methodName)
	  return meth;
      }
  return NULL;
}


/* srMethCreate ****************************************************/
/* Creates a method structure from AceDB. */

static methodID srMethCreate(SeqRegion *region, char *methodName)
{
  srMeth *meth;
  OBJ obj ;
  int k;
  char *text;
  KEY methodKey;

  if (!lexword2key(methodName, &methodKey, _VMethod))
    messcrash ("srMethodCreate() - methodKey not found in lex.");

  if (!(obj = bsCreate (methodKey))) 
    return 0;
  
  meth = arrayp(region->zMapRegion->methods, arrayMax(region->zMapRegion->methods), srMeth);
  meth->id = region->idc++;
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


/* seqRegionConvert ************************************************/
/* Converts the the SeqRegion smap into an array of zmap-flavour 
 * segs in zMapRegion structure.   */

void seqRegionConvert(SeqRegion *region)
{
  STORE_HANDLE localHandle = handleCreate();
  KEYSET allKeys = sMapKeys(region->smap, localHandle);
  int i;
  Array units = arrayHandleCreate(256, BSunit, localHandle);

  region->zMapRegion->segs = arrayReCreate(region->zMapRegion->segs, 1000, SEG);
  region->zMapRegion->methods = arrayCreate(100, srMeth);

  for (i = 0 ; i < keySetMax(allKeys) ; i++)
    {
      KEY methodKey, key = keySet(allKeys, i);
      SMapKeyInfo *info = sMapKeyInfo(region->smap, key);
      int x1, x2;
      SEG *seg;
      OBJ obj;
      char *methodName;

      if ((obj = bsCreate(key)) && bsGetKey(obj, str2tag("Method"), &methodKey))
	{
	  Coord cdsStart, cdsEnd;
	  srType type = SR_SEQUENCE; /* default */
	  srPhase phase = 1;
	  BOOL isStartNotFound = FALSE;
	  BOOL isCDS = FALSE;
	  Array exons = NULL;
	  
	  methodName = messalloc(strlen(name(methodKey))); /* need to remember to free this? */
      	  methodName = str2p(name(methodKey), region->bucket);

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
	  
	  seg = arrayp(region->zMapRegion->segs, arrayMax(region->zMapRegion->segs), SEG);
	  seg->id = str2p(name(key), region->bucket);
	  seg->type = type;
	  seg->x1 = x1;
	  seg->x2 = x2;
	  seg->phase = phase;
	  seg->method = srMethodMake(region, methodName);
	  
	  if (type == SR_TRANSCRIPT)
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
		  
		  seg = arrayp(region->zMapRegion->segs, arrayMax(region->zMapRegion->segs), SEG);
		  seg->id = str2p(name(u[0].k), region->bucket);
		  seg->type = SR_FEATURE;
		  seg->x1 = x1;
		  seg->x2 = x2;
		  seg->score = u[3].f;
		  seg->method = srMethodMake(region, str2p(name(u[0].k), region->bucket));
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


/* srActivate ************************************************/
/* Starts initial construction of the SeqRegion, then calls
 * srCalculate and srGetDNA to complete the job. */

void srActivate(void *seqRegion, 
		char *seqspec, 
		ZMapRegion *zMapRegion, 
		Coord *r1, 
		Coord *r2, 
		STORE_HANDLE *handle)
{
  int x1, x2, seqLen;
  KEY key;
  SeqRegion *region = (SeqRegion *)seqRegion;

  /* Get out own handle */
  
  STORE_HANDLE newHandle = handleHandleCreate(*handle);
  
  region->handle = newHandle;
  region->smap = NULL;
  region->bucket = NULL;
  region->zMapRegion = zMapRegion;
  region->idc = 1;

  if (!lexClassKey(seqspec, &key) || 
      !sMapTreeRoot(key, 1, 0, &region->rootKey, &x1, &x2))
    return;
  
  region->zMapRegion->length = sMapLength(region->rootKey);
  
  /* If the key is reversed wrt to the root, record that we need to 
     make the sMap reversed, and calculate the coords that the key
     has on the reversed sMap. */
  
  if (x1 > x2)
    {
      *r1 = region->zMapRegion->length - x1 + 1;
      *r2 = region->zMapRegion->length - x2 + 1;
      region->zMapRegion->rootIsReverse = 1;
    }
  else
    {
      region->zMapRegion->rootIsReverse = 0;
      *r1 = x1;
      *r2 = x2;
    }

  /* policy: we start by calculating three times the length of
     the initial object.  This policy will have to change!  */
  seqLen = x2 - x1;
  
  srCalculate(region, x1 - seqLen, x2 + seqLen);

  srGetDNA(region);

  return;
}


/* calcSeqRegion *************************************************/
/* (Re)Initialises the dna, methods, oldMethods arrays and sets up
 * a new smap, then calls seqRegionConvert to convert the smap to
 * zmap-flavour segs. Called from several places when recalculating
 * or reverse-complementing, frinstance. */

static void calcSeqRegion(void *seqRegion, 
			  Coord x1, Coord x2,
			  BOOL isReverse)
{
  SeqRegion *region = (SeqRegion *)seqRegion;

  if (x1 < 1) x1 = 1;
  if (x2 > region->zMapRegion->length) x2 = region->zMapRegion->length;
  
  /* Note: this currently throws everything away and starts again.
   * It's designed to be capable of incrementally adding to existing structures.
   */
  
  if (region->smap)
    sMapDestroy(region->smap);
  
  if (region->zMapRegion->dna)
    arrayDestroy(region->zMapRegion->dna);
 
  if (region->bucket)
    sbDestroy(region->bucket);
  
  region->zMapRegion->oldMethods = region->zMapRegion->methods;
  region->zMapRegion->methods = NULL;
  /* currently just put in new area - might need these to see what we have 
     already got. */

  
  region->zMapRegion->area1 = x1;
  region->zMapRegion->area2 = x2;
  region->zMapRegion->rootIsReverse = isReverse;
  
  region->bucket = sbCreate(region->handle);

  /* Build an smap which seqRegionConvert will then convert
   * into zmap-flavour segs and put in zMapRegion->segs. */
  if (region->zMapRegion->rootIsReverse)
    region->smap = sMapCreateEx(region->handle, region->rootKey, 
				0, region->zMapRegion->length, 
				region->zMapRegion->area1, 
				region->zMapRegion->area2, NULL);
  else
    region->smap = sMapCreateEx(region->handle, region->rootKey, 
				region->zMapRegion->length, 1, 
				region->zMapRegion->area1, 
				region->zMapRegion->area2, NULL);
  
  seqRegionConvert(region);

  /* When recalculating, for whatever reason, we need to repopulate
   * the dna array. On initial display, srGetDNA is called by
   * srActivate, but recalculation doesn't tread that path. */
  if (!region->zMapRegion->dna)
    srGetDNA(region);

  arrayDestroy(region->zMapRegion->oldMethods);
  
}
 
/* srGetDNA ***********************************************************/
/* Populates the dna array. */

void srGetDNA(SeqRegion *region)
{
  
  /* TODO handle callback stuff */
  /* I find Simon's comment disturbing, as I've structured it not to 
   * do a callback here... RNC */
  if (!region->zMapRegion->dna)
    region->zMapRegion->dna = sMapDNA(region->smap, region->handle, 0);
  
  return;
}


/* srCalculate ********************************************************/
/* Just a wrapper, since calcSeqRegion is called variously to reverse-
 * complement and recalculate, so needs slightly different params. */

void srCalculate(SeqRegion *region, Coord x1, Coord x2)
{
  calcSeqRegion(region, x1, x2, region->zMapRegion->rootIsReverse);
}


/* srRevComp **********************************************************/
/* Calls calcSeqRegion with reversed coordinates to reverse-complement.
 * Actually it's not used yet. */

void srRevComp(SeqRegion *region)
{
  calcSeqRegion(region, 
		region->zMapRegion->length - region->zMapRegion->area2 + 1,
		region->zMapRegion->length - region->zMapRegion->area1 + 1,
		!region->zMapRegion->rootIsReverse);
}


void seqRegionDestroy(SeqRegion *region)
{
  handleDestroy(region->handle);
}
  
/***************************** end of file ******************************/
