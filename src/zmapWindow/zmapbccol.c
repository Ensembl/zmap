/*  Last edited: Nov 20 14:21 2003 (rnc) */
/*  file: zmapbccol.c
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

#ifdef ZMAP
/* Generic box drawing column code */

/* This code was written on the day the US and UK 
   started the Second Oil War. 
   I think the module name "nbc" is somehow appropriate - srk */
#include <wh/graph.h>
#include <wh/gex.h>
#include <wzmap/seqregion.h>
#include <wzmap/zmapcontrol.h>
#include <wh/method.h>
#include <wh/bump.h>

typedef enum { DEFAULT=0, WIDTH, OFFSET, HIST } BoxColModeType;

typedef struct {
   MethodOverlapModeType overlap_mode ;	 /* See wh/method.h */

  /* following not under user control */
  float offset ;
  BOOL isDown ;
  BoxColModeType mode ;
  float fmax ;
  BUMP bump ;
  Associator cluster ;		/* only non-zero if METHOD_CLUSTER */
  int clusterCount ;
  int width;
  float histBase;
} nbcPrivate;

static void nbcFinalise(void *arg)
{
  nbcPrivate *priv = (nbcPrivate *)arg;
  
  if (priv->bump)
    bumpDestroy(priv->bump);

}
void nbcInit(ZMapWindow *window, ZMapColumn *col)
{
  srMeth *meth = srMethodFromID(window->root->region, col->meth);
  nbcPrivate *bc = handleAlloc(nbcFinalise, window->root->look->handle,
			       sizeof(nbcPrivate));
 
  col->private = bc;

  if (meth->flags & METHOD_SCORE)
    {
      bc->mode = WIDTH;
      if (meth->flags & METHOD_SCORE_BY_OFFSET
	  && !col->isFrame)
	bc->mode = OFFSET ;
      else if (meth->flags & METHOD_SCORE_BY_HIST)
	bc->mode = HIST ;

      /* checks to prevent arithmetic crash */
      if (bc->mode == OFFSET)
	{
	  if (!meth->minScore)
	    meth->minScore = 1 ;
	}
      else
	{
	  if (meth->maxScore == meth->minScore)
	    meth->maxScore = meth->minScore + 1 ;
	}
    }
  else
    /* none of the SCORE_BY_xxx flags set */
    bc->mode = DEFAULT ;

  if (meth->width)
    {
      if (bc->mode == WIDTH
	  && meth->flags & METHOD_SCORE_BY_OFFSET
	  && meth->width > 2)
        bc->width = 2 ;
      else
	bc->width = meth->width ;
    }
  else
    /* the method doesn't set the width */
    {
      if (bc->mode == OFFSET)
	bc->width = 7 ;
      else
	bc->width = 2 ;
    }
  
  if (bc->mode == HIST)
    { 
      /* normalise the BoxCol's histBase */
      if (meth->minScore == meth->maxScore)
	bc->histBase = meth->minScore ;
      else
	bc->histBase = (meth->histBase - meth->minScore) / 
	  (meth->maxScore - meth->minScore) ;

      if (bc->histBase < 0) bc->histBase = 0 ;
      if (bc->histBase > 1) bc->histBase = 1 ;
      
      bc->fmax = (bc->width * bc->histBase) + 0.2 ;
    }
  else
    bc->fmax = 0 ;

}

static void nbcCalcBox (srMeth *methp, nbcPrivate *bc, float *x1p, float *x2p,
			float *y1p, float *y2p, float score)
{
  int xoff ;
  float x1, x2, y1, y2;
  float left, dx;
  float numerator, denominator ;
  double logdeux = log((double)2.0) ;
   
  numerator = score - methp->minScore ;
  denominator = methp->maxScore - methp->minScore ;
  left = *x1p;
  y1 = *y1p;
  y2 = *y2p;
  
  switch (bc->mode)    
    {
    case OFFSET:
      if (denominator == 0)				    /* catch div by zero */
	{
	  if (numerator < 0)
	    dx = 0.8 ;
	  else if (numerator > 0)
	    dx = 3 ;
	}
      else
	{
	  dx = 1 + (((float)numerator)/ ((float)(denominator))) ;
	}
      if (dx < .8) dx = .8 ; if (dx > 3) dx = 3 ; /* allow some leeway & catch dx == 0 */
      dx = bc->width * log((double)dx)/logdeux ;
      
      x1 = left + dx ;
      x2 = left + dx + .9 ;

      if (bc->fmax < dx + 1.5)
	bc->fmax = dx + 1.5 ;
      break ;

    case HIST:
      if (denominator == 0)				    /* catch div by zero */
	{
	  if (numerator < 0)
	    dx = 0 ;
	  else if (numerator > 1)
	    dx = 1 ;
	}
      else
	{
	  dx = numerator / denominator ;
	  if (dx < 0) dx = 0 ;
	  if (dx > 1) dx = 1 ;
	}
      x1 = left + (bc->width * bc->histBase) ;
      x2 = left + (bc->width * dx) ;
      
      if (bc->fmax < bc->width*dx)
	bc->fmax = bc->width*dx ;
      break ;

    case WIDTH:
      if (denominator == 0)				    /* catch div by zero */
	{
	  if (numerator < 0)
	    dx = 0.25 ;
	  else if (numerator > 0)
	    dx = 1 ;
	}
      else
	{
	  dx = 0.25 + ((0.75 * numerator) / denominator) ;
	}
      if (dx < 0.25) dx = 0.25 ;
      if (dx > 1) dx = 1 ;
      
      /* !WARNING! fall-through */

    case DEFAULT:
      if (bc->mode == DEFAULT)
	dx = 0.75 ;

      xoff = 1 ;
      if (bc->bump)
	bumpItem (bc->bump, 2, (y2-y1), &xoff, &y1) ;
 #if 0
      else if (bc->cluster)	/* one subcolumn per key */
	{
	  void *v ;
	  
	  if (assFind (bc->cluster, assVoid(seg->parent), &v))
	    xoff = assInt(v) ;
	  else
	  {
	    xoff = ++bc->clusterCount ;
	    assInsert (bc->cluster, assVoid(seg->parent), assVoid(xoff)) ;
	  }
	}
 #endif
      x1 = left + (0.5 * bc->width * (xoff - dx)) ;
      x2 = left + (0.5 * bc->width * (xoff + dx)) ;
      
      if (bc->fmax < 0.5*bc->width*(xoff+1))
	bc->fmax = 0.5*bc->width*(xoff+1) ;
      break ;
      
    default:
      messcrash("Unknown bc->mode in fMapShowHomol(): %d", bc->mode) ;
    }
  

  *x1p = x1;
  *x2p = x2;
  *y1p = y1;
  *y2p = y2;

  
  
} /* nbcDrawBox */


struct geneSelectData{
  SEG *seg;
  BOOL isIntron;
  int exonNumber;
};

void zMapGeneDraw(ZMapWindow *window, ZMapColumn *col, float *offset, int frame)
{
  SeqRegion *region = window->root->region;
  srMeth *meth = srMethodFromID(region, col->meth);
  int i, j;
  int box;
  nbcPrivate *bc = (nbcPrivate *)col->private;
  float maxwidth = *offset;

  if (meth && (meth->flags & METHOD_BUMPABLE))
    bc->bump = bumpCreate(30, 0);
  else
    bc->bump = NULL;

  for (i=0; i < arrayMax(region->segs); i++)
      {
	SEG *seg = arrp(region->segs, i, SEG);
	if (seg->method == col->meth &&
	    seg->type == col->type &&
	    zmIsOnScreen(window, seg->x1, seg->x2))
	  {
	    Array exons = seg->u.transcript.exons;
	    float e1, e2, y, x; 
	    int xoff = 1;
	    struct geneSelectData *sd;

	    if (bc->bump)
	      bumpItem(bc->bump, 1, seg->x2 - seg->x1, &xoff, &y);

	    x = *offset + xoff;
	    if (x > maxwidth)
	      maxwidth = x;
	    
	    /* NB the code below draws all the exons before drawing 
	       any of the introns. This is deliberate as otherwise 
	       the background on the adjoining intron obscures a little
	       bit of the next Exon. */

	    for(j = 1; j <arrayMax(exons); j++)
	      { /* Intron */
		float middle;
		e1 = zmScreenCoord(col->window, array(exons, j-1, srExon).x2);
		e2 = zmScreenCoord(col->window, array(exons, j, srExon).x1);
		middle = 0.5 * (e1 + e2);
		box = graphBoxStart();
		sd = (struct geneSelectData *)
		  halloc(sizeof(struct geneSelectData), window->drawHandle);
		sd->exonNumber = j;
		sd->isIntron = TRUE;
		sd->seg = seg;
		zmRegBox(window, box, col, sd);
		graphLine(x+0.5, e1, x+0.9, middle);
		graphLine(x+0.9, middle, x+0.5, e2);
		graphBoxEnd();
		graphBoxDraw(box, meth->colour, -1) ;
	      }
	    
	    for (j = 0; j <arrayMax(exons); j++)
	      { /* exon */
		struct geneSelectData *sd;
		e1 = zmScreenCoord(col->window, array(exons, j, srExon).x1);
		e2 = zmScreenCoord(col->window, array(exons, j, srExon).x2);
		sd = (struct geneSelectData *)
		  halloc(sizeof(struct geneSelectData), window->drawHandle);
		box = graphBoxStart();
		sd->exonNumber = j;
		sd->isIntron = FALSE;
		sd->seg = seg;
		e2 = zmScreenCoord(col->window, array(exons, j, srExon).x2);
		zmRegBox(window, box, col, sd);
		graphRectangle(x, e1, x+1, e2);
		graphBoxEnd();
		graphBoxDraw(box, meth->colour, -1) ;
	      }
	  }
      }

  if (bc->bump) 
    bumpDestroy (bc->bump);
  
  *offset = maxwidth + 2.0 ;
}

void geneSelect(ZMapWindow *window, ZMapColumn *col,
		void *arg, int box, double x, double y, BOOL isSelect)
{
  struct geneSelectData *sd = (struct geneSelectData *)arg;
  SEG *seg = sd->seg;
  Array exons = seg->u.transcript.exons;
  Coord x1, x2;
  char *string;
  srMeth *meth = srMethodFromID(window->root->region, col->meth);
  int colour = WHITE;
  
  if (isSelect)
    {
    
      colour = LIGHTRED;
      
      if (sd->isIntron)
	{
	  x1 = arr(exons, sd->exonNumber-1, srExon).x2 + 1 ;
	  x2 = arr(exons, sd->exonNumber, srExon).x1 - 1 ;
	}
      else
	{
	  x1 = arr(exons, sd->exonNumber, srExon).x1;
	  x2 = arr(exons, sd->exonNumber, srExon).x2;
	}
      
      string = g_strdup_printf("%s [%f] %d %d", 
			       seg->id, seg->score, 
			       zmVisibleCoord(window->root, x1),
			       zmVisibleCoord(window->root, x2));
      
      gtk_entry_set_text(window->root->look->infoSpace, string);
      g_free(string);
    }
  
  graphBoxDraw(box, meth->colour, colour);

}

void zMapFeatureColumn(ZMapWindow *window, ZMapColumn *col, float *offset, int frame)
{
  SeqRegion *region = window->root->region;
  srMeth *meth = srMethodFromID(region, col->meth);
  int i;
  nbcPrivate *bc = (nbcPrivate *)col->private;
  float maxwidth = *offset;
  if (meth->flags & METHOD_BUMPABLE)
    bc->bump = bumpCreate(30, 0);
  else
    bc->bump = NULL;

  for (i=0; i < arrayMax(region->segs); i++)
      {
	SEG *seg = arrp(region->segs, i, SEG);
	if (seg->method == col->meth &&
	    seg->type == col->type &&
	    zmIsOnScreen(window, seg->x1, seg->x2))
	  {
	    int box =  graphBoxStart();
	    float y1, y2, x1, x2;
	    y1 = zmScreenCoord(col->window, seg->x1);
	    y2 = zmScreenCoord(col->window, seg->x2+1) ;
	    x1 = *offset;
	    nbcCalcBox(meth, bc, &x1, &x2, &y1, &y2, seg->score);
	    graphRectangle(x1, y1, x2, y2);
	    graphBoxEnd();
	    graphBoxDraw (box, BLACK, meth->colour) ;
	    zmRegBox(window, box, col, seg);
	    if (x2 > maxwidth)
	      maxwidth = x2;
	  }
      }

  if (bc->bump) 
    bumpDestroy (bc->bump);
  
  *offset = maxwidth + 0.5 ;
}



void nbcSelect(ZMapWindow *window, ZMapColumn *col,
	     void *arg, int box, double x, double y, BOOL isSelect)
{
  SEG *seg = (SEG *)arg;char *string = g_strdup_printf("%s [%f] %d %d", 
				 seg->id, seg->score, 
				 zmVisibleCoord(window->root, seg->x1),
				 zmVisibleCoord(window->root, seg->x2));

  gtk_entry_set_text(window->root->look->infoSpace, string);
  g_free(string);
}
#endif
