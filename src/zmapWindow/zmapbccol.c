/*  Last edited: Jul  2 10:14 2004 (rnc) */
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

/* Generic box drawing column code */

/* This code was written on the day the US and UK 
   started the Second Oil War. 
   I think the module name "nbc" is somehow appropriate - srk */

#include <math.h>

#include <glib.h>

#include <ZMap/zmapFeature.h>
#include <zmapDraw.h>
#include <seqregion.h>

/* A lot of this comes direct from acedb.......... */


enum Colour    {WHITE, BLACK, LIGHTGRAY, DARKGRAY,
                RED, GREEN, BLUE,
                YELLOW, CYAN, MAGENTA,
		LIGHTRED, LIGHTGREEN, LIGHTBLUE,
		DARKRED, DARKGREEN, DARKBLUE,
		PALERED, PALEGREEN, PALEBLUE,
		PALEYELLOW, PALECYAN, PALEMAGENTA,
		BROWN, ORANGE, PALEORANGE,
		PURPLE, VIOLET, PALEVIOLET,
		GRAY, PALEGRAY,
		CERISE, MIDBLUE,
		NUM_TRUECOLORS,
                TRANSPARENT,	/* pseudocolour only for background */
		FORECOLOR,	/* pseudocolor to force box->fcol after graphColor() */
		BACKCOLOR	/* pseudocolor to force box->bcol after graphColor() */
               } ;



/* Defines the way columns are displayed when there are lots of features overlapping within
 * a column. */
typedef enum {METHOD_OVERLAP_COMPLETE, METHOD_OVERLAP_BUMPED, METHOD_OVERLAP_CLUSTER} MethodOverlapModeType ;


/* AGH......THIS ALL NEEDS SORTING OUT......... */
/* forward declaration of opaque type */
typedef struct BumpStruct *BUMP;



#define METHOD_FRAME_SENSITIVE	0x00000004U /* if tag "Frame_sensitive" */
#define METHOD_STRAND_SENSITIVE	0x00000008U /* if tag "Stand_sensitive" */
#define METHOD_SHOW_UP_STRAND	0x00010000U /* if tag "Show_up_strand" */
#define METHOD_SCORE_BY_OFFSET  0x00000010U /* if tag "Score_by_offset" */
#define METHOD_SCORE_BY_WIDTH   0x00000020U /* if tag "score_by_width" */
#define METHOD_SCORE_BY_HIST	0x00002000U /* if tag "Score_by_histogram"*/
#define METHOD_PERCENT          0x00000400U /* if tag "Percent"
					      defaults Score_bounds
					      min/max to 25/100 
					      (can be overridden) */

#define METHOD_BUMPABLE		0x00000200U /* if tag "Bumpable" */
#define METHOD_CLUSTER          0x00020000U /* if tag "Cluster" */

#define METHOD_BLASTN		0x00000001U /* if tag "BlastN" -
					      can calculate percent 
					      from score */
#define METHOD_BELVU    	0x00004000U /* if tag "Belvu"
					       esr, for PEPMAP */
#define METHOD_BLIXEM_X		0x00000040U /* if tag "Blixem_X" */
#define METHOD_BLIXEM_N		0x00000080U /* if tag "Blixem_N" */
#define METHOD_BLIXEM_P		0x00008000U /* if tag "Blixem_P" */
#define METHOD_EMBL_DUMP	0x00001000U /* if tag "EMBL_dump" */



#define METHOD_DONE		0x00000002U /* if method already cached */
#define METHOD_CALCULATED	0x00000800U /* used in addOldSegs */
#define METHOD_GAPS             0x00040000U /* Show gapped sequences and homols */
#define METHOD_JOIN_BLOCKS      0x00080000U /* Join all blocks of a single match with lines. */
/*#define METHOD_CACHED		0x00000100U * set if Method exists
					    * in cache and has a
					    * real obj in the DB */




/* convenience flag */
#define METHOD_SCORE		(METHOD_SCORE_BY_OFFSET | \
				 METHOD_SCORE_BY_WIDTH | \
				 METHOD_SCORE_BY_HIST)







typedef enum { DEFAULT=0, WIDTH, OFFSET, HIST } BoxColModeType;


typedef struct {

   MethodOverlapModeType overlap_mode ;	 /* See wh/method.h */

  /* following not under user control */
  float offset ;
  gboolean isDown ;
  BoxColModeType mode ;
  float fmax ;
  BUMP bump ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  Associator cluster ;		/* only non-zero if METHOD_CLUSTER */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  GHashTable* cluster ;

  int clusterCount ;
  int width;
  float histBase;
} nbcPrivate;

static void nbcFinalise(void *arg)
{
  nbcPrivate *priv = (nbcPrivate *)arg;
  
  //  if (priv->bump)
  //    bumpDestroy(priv->bump);

  return;
}
void nbcInit(ZMapPane pane, ZMapColumn *col)
{
  /* this whole bit needs to be rewritten, so it's OK to comment it out, I think
  srMeth *meth = srMethodFromID(zMapPaneGetZMapRegion(pane), col->meth);
  nbcPrivate *bc = (nbcPrivate*)malloc(sizeof(nbcPrivate));
  //  nbcPrivate *bc = handleAlloc(nbcFinalise, zMapWindowGetHandle(pane->window),
  // sizeof(nbcPrivate));
 
  col->private = bc;

  if (meth->flags & METHOD_SCORE)
    {
      bc->mode = WIDTH;
      if (meth->flags & METHOD_SCORE_BY_OFFSET
	  && !col->isFrame)
	bc->mode = OFFSET ;
      else if (meth->flags & METHOD_SCORE_BY_HIST)
	bc->mode = HIST ;

      // checks to prevent arithmetic crash 
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
    // none of the SCORE_BY_xxx flags set 
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
    // the method doesn't set the width 
    {
      if (bc->mode == OFFSET)
	bc->width = 7 ;
      else
	bc->width = 2 ;
    }
  
  if (bc->mode == HIST)
    { 
      // normalise the BoxCol's histBase 
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
  */
  return;
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
      //      if (bc->bump)
      //	bumpItem (bc->bump, 2, (y2-y1), &xoff, &y1) ;
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
      //TODO: sort out error handling here
      printf("Unknown bc->mode in fMapShowHomol(): %d", bc->mode) ;
      exit;
    }
  

  *x1p = x1;
  *x2p = x2;
  *y1p = y1;
  *y2p = y2;

  return;  
  
} /* nbcDrawBox */


struct geneSelectData{
  ZMapFeature seg ;
  gboolean isIntron ;
  int exonNumber ;
};

void zMapGeneDraw(ZMapPane pane, ZMapColumn *col, float *offset, int frame)
{
  ZMapRegion    *zMapRegion; // = zMapPaneGetZMapRegion(pane);
  srMeth        *meth = srMethodFromID(zMapRegion, col->meth);
  int            i, j;
  int            box;
  nbcPrivate    *bc = (nbcPrivate *)col->private;
  float          maxwidth = *offset, width;
  FooCanvasItem *group;
  char           colours[30][10] = { "red", "blue", "pink", "green", "orange", "yellow", "purple", "cyan", "magenta",
				     "red", "blue", "pink", "green", "orange", "yellow", "purple", "cyan", "magenta",
				     "red", "blue", "pink", "green", "orange", "yellow", "purple", "cyan", "magenta" };
				     /* I made these up.  Need to find out what meth->colour
					equates to */
				     
  /* temporary kludge */
  if (!zMapRegion) return;

  //  if (meth && (meth->flags & METHOD_BUMPABLE))
  //    bc->bump = bumpCreate(30, 0);
  //  else
    bc->bump = NULL;

    for (i=0; i < zMapRegionGetSegs(zMapRegion)->len; i++)
      {
	ZMapFeature seg = &g_array_index(zMapRegionGetSegs(zMapRegion), ZMapFeatureStruct, i) ;

	if (seg->method == col->meth &&
	    seg->type == col->type &&
	    zmIsOnScreen(pane, seg->x1, seg->x2))
	  {
	    GArray *exons = g_array_new(FALSE, FALSE, sizeof(ZMapExon));
	    float e1, e2, x, y = 0;  /* y needs better default */ 
	    int xoff = 1;
	    struct geneSelectData *sd;

	    exons = g_array_append_vals(exons,
					&seg->feature.transcript.exons, 
					seg->feature.transcript.exons->len);
 
	    //	    if (bc->bump)
	    //	      bumpItem(bc->bump, 1, seg->x2 - seg->x1, &xoff, &y);

	    x = *offset + xoff + 30;

	    if (x > maxwidth)
	      maxwidth = x;
	    
	    /* NB the code below draws all the introns before drawing 
	       any of the exons. This is deliberate as otherwise the
	       background on the adjoining intron obscures a little
	       bit of the next exon. */

	    /* make a group to contain the gene */
	    group = foo_canvas_item_new(FOO_CANVAS_GROUP(zMapPaneGetGroup(pane)),
					foo_canvas_group_get_type(),
					"x", (double)x,
					"y", (double)y ,
					NULL);

	    width = meth->width * 15; /* arbitrary multiplier - needs to be rationalised */

	    for (j = 1; j < exons->len; j++)
	      { /* Intron */
		float middle;

		e1 = zmScreenCoord(col->pane, g_array_index(exons, ZMapExon, j-1)->x2);
		e2 = zmScreenCoord(col->pane, g_array_index(exons, ZMapExon, j  )->x1);
		middle = (e1 + e2)/2.0;

		/* TODO: if e1 < e2 then it's on the reverse strand so needs different handling */
		if (e1 > e2); 
		  
		drawLine(FOO_CANVAS_GROUP(group), x+(width/2), e1, x+width, middle, colours[meth->colour], 1.0);
		drawLine(FOO_CANVAS_GROUP(group), x+width, middle, x+(width/2), e2, colours[meth->colour], 1.0);
	      }
	    
	    for (j = 0; j <exons->len; j++)
	      { /* exon */
		e1 = zmScreenCoord(col->pane, g_array_index(exons, ZMapExon, j)->x1);
		e2 = zmScreenCoord(col->pane, g_array_index(exons, ZMapExon, j)->x2);

		/* if e1 < e2 then it's on the reverse strand so needs different handling */
		if (e1 < e2)
		  drawBox(group, x, e1, x+width, e2, "black", colours[meth->colour]);
		else
		  drawBox(group, x, e2, x+width, e1, "red", colours[meth->colour]);
	      }
	  }
      }

    //  if (bc->bump) 
    //    bumpDestroy (bc->bump);
  
  *offset = maxwidth + 2.0 ;
  return;
}

void geneSelect(ZMapPane pane, ZMapColumn *col,
		void *arg, int box, double x, double y, gboolean isSelect)
{
  struct geneSelectData *sd = (struct geneSelectData *)arg;
  ZMapFeature seg = sd->seg;
  GArray *exons = g_array_new(FALSE, FALSE, sizeof(ZMapExon));
  Coord x1, x2;
  char *string;
  srMeth *meth; // = srMethodFromID(zMapPaneGetZMapRegion(pane), col->meth);
  int colour = WHITE;

  exons = g_array_append_vals(exons, 
			      &seg->feature.transcript.exons,
			      seg->feature.transcript.exons->len);
  if (isSelect)
    {
    
      colour = LIGHTRED;
      
      if (sd->isIntron)
	{
	  x1 = g_array_index(exons, ZMapExon, sd->exonNumber-1)->x2 + 1 ;
	  x2 = g_array_index(exons, ZMapExon, sd->exonNumber)->x1 - 1 ;
	}
      else
	{
	  x1 = g_array_index(exons, ZMapExon, sd->exonNumber)->x1;
	  x2 = g_array_index(exons, ZMapExon, sd->exonNumber)->x2;
	}
      
      //      string = g_strdup_printf("%s [%f] %d %d", 
      //			       seg->id, seg->score, 
      //			       zmVisibleCoord(zMapPaneGetZMapWindow(pane), x1),
      //			       zmVisibleCoord(zMapPaneGetZMapWindow(pane), x2));
      
      //      gtk_entry_set_text(GTK_ENTRY(pane->window->infoSpace), string);
      //      g_free(string);
    }
  
  //  graphBoxDraw(box, meth->colour, colour);

  return;
}

void zMapFeatureColumn(ZMapPane  pane, ZMapColumn *col, float *offset, int frame)
{
  ZMapRegion *zMapRegion; // = zMapPaneGetZMapRegion(pane);
  srMeth *meth; // = srMethodFromID(zMapRegion, col->meth);
  int i;
  nbcPrivate *bc = (nbcPrivate *)col->private;
  float maxwidth = *offset;
  //  if (meth->flags & METHOD_BUMPABLE)
  //    bc->bump = bumpCreate(30, 0);
  //  else
    bc->bump = NULL;

    for (i=0; i < zMapRegionGetSegs(zMapRegion)->len; i++)
      {
	ZMapFeature seg = &g_array_index(zMapRegionGetSegs(zMapRegion), ZMapFeatureStruct, i);
	if (seg->method == col->meth &&
	    seg->type == col->type &&
	    zmIsOnScreen(pane, seg->x1, seg->x2))
	  {
	    int box =  0; //graphBoxStart();
	    float y1, y2, x1, x2;
	    y1 = zmScreenCoord(col->pane, seg->x1);
	    y2 = zmScreenCoord(col->pane, seg->x2+1) ;
	    x1 = *offset;
	    nbcCalcBox(meth, bc, &x1, &x2, &y1, &y2, seg->score);
	    //	    graphRectangle(x1, y1, x2, y2);
	    //	    graphBoxEnd();
	    //	    graphBoxDraw (box, BLACK, meth->colour) ;
	    zmRegBox(pane, box, col, seg);
	    if (x2 > maxwidth)
	      maxwidth = x2;
	  }
      }

  //  if (bc->bump) 
  //    bumpDestroy (bc->bump);
  
  *offset = maxwidth + 0.5 ;
  return;
}



void nbcSelect(ZMapPane pane, ZMapColumn *col,
	     void *arg, int box, double x, double y, gboolean isSelect)
{
  ZMapFeature seg = (ZMapFeature)arg ;
  //  char *string = g_strdup_printf("%s [%f] %d %d", 
  //				 seg->id, seg->score, 
  //				 zmVisibleCoord(zMapPaneGetZMapWindow(pane), seg->x1),
  //				 zmVisibleCoord(zMapPaneGetZMapWindow(pane), seg->x2));

  //  gtk_entry_set_text(GTK_ENTRY(pane->window->infoSpace), string);
  //  g_free(string);
  return;
}


/************************ end of file **********************************/
