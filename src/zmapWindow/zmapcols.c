/*  Last edited: Jul  7 14:11 2004 (edgrif) */
/*  file: zmapcols.c
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
#include <zmapDraw.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapWindow.h>

/*************** function prototypes *****************************************/

static void zMapDNAColumn(ZMapPane pane, ZMapColumn *col, 
		   float *offsetp, int frame);

/*************** start function definitions **********************************/

static void zMapScaleColumn(ZMapPane pane, ZMapColumn *col, float *offset, int frame)
{
  //  *offset = zmMainScale(zMapPaneGetCanvas(pane), *offset,
  //			zmVisibleCoord(zMapPaneGetZMapWindow(pane), zmCoordFromScreen(pane, 0)),
  //			zmVisibleCoord(zMapPaneGetZMapWindow(pane), 
  //				       zmCoordFromScreen(pane, zMapPaneGetHeight(pane))));
  return;
}


/* cut this from zmapsequence.c and pasted here for now */
void zMapDNAColumn(ZMapPane pane, ZMapColumn *col, 
		   float *offsetp, int frame)
{
  float offset = *offsetp;
  float max = offset;
  float BPL = zMapPaneGetBPL(pane);
  ZMapRegion *zMapRegion; // = zMapPaneGetZMapRegion(pane);
  Coord seqstart = srCoord(zMapRegion, zMapPaneGetCentre(pane)) -
    (zMapPaneGetHeight(pane) * BPL) / 2;
  Coord seqend = srCoord(zMapRegion, zMapPaneGetCentre(pane)) +
    (zMapPaneGetHeight(pane) * BPL) / 2;

  Coord i;

  if (seqstart < zMapRegion->area1)
    seqstart = zMapRegion->area1;

  if (seqend > zMapRegion->area2)
    seqend = zMapRegion->area2;

  if (zMapRegion->dna->len == 0)
    return;

  for (i = seqstart; i < seqend; i += BPL)
    {
      char *dnap, buff[10];
      int j;
      ScreenCoord y =  zmScreenCoord(pane, i);
      //      sprintf(buff, "%7d", zmVisibleCoord(zMapPaneGetZMapWindow(pane), i));
      //      graphText(buff, offset, y);

      dnap = &g_array_index(zMapRegion->dna,
			   char,
			   i - (zMapRegion->area1-1));

      for (j = 0; j < BPL; j++)
	{
	  float x = offset+8+j;
	  //	  buff[0] = dnaDecodeChar[(int)*(dnap+j)];
	  //	  buff[1] = 0;
	  //	  graphText(buff, x, y);
	  
	  if (j+i > zMapRegion->area2)
	    break;

	  if (j+1 == zMapPaneGetDNAwidth(pane))
	    {
	      //	      graphText("...", x+1, y);
	      j = BPL;
	      x += 4;
	    }
	  
	  if (x > max) 
	    max = x;
	}
    }
	
  *offsetp = max+1;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void pruneCols(ZMapPane pane)
     /* Remove Columns which have invalid methods */
{ 
  int i, j;
  
  for (i = 0; i < zMapPaneGetCols(pane)->len; i++)
    {
      ZMapColumn *c = g_ptr_array_index(zMapPaneGetCols(pane), i);

      if (c->type != ZMAPFEATURE_INVALID && c->meth)
	  //	  && !srMethodFromID(zMapPaneGetZMapRegion(pane), c->meth))
	{
	  for (j = i+1; j < (zMapPaneGetCols(pane))->len; j++)
	    g_ptr_array_remove_index(zMapPaneGetCols(pane), j-1); // = arr(pane->cols, j, ZMapColumn);
	}
    }
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static void insertCol(ZMapPane pane, methodID meth, ZMapFeatureType type)
{
  /* NB call this one with type == SR_INVALID to put in default columns. */

  /* Where column code links in.
     Fields are :
     1) Creation Routine.
     2) Draw routine.
     3) Config routine.
     4) Flag set for phase sensistive column - gets repeated three times.
     5) Name - only valid for SR_INVALID columns.
     6) Right priority - only valid for SR_INVALID columns.
     7) Type - SR_INVALID for default columns which are always present
               otherwise matches values in segs from convert routines.
  */
  static struct ZMapColDefs defs[] = {
    { NULL,    zMapScaleColumn  , NULL, NULL      , FALSE, 1.0, "Scale"   , ZMAPFEATURE_INVALID},
    { nbcInit, zMapFeatureColumn, NULL, nbcSelect , FALSE, 2.0, "Features", ZMAPFEATURE_SEQUENCE   },
    { nbcInit, zMapFeatureColumn, NULL, nbcSelect , FALSE, 2.0, "Features", ZMAPFEATURE_BASIC    },
    { nbcInit, zMapGeneDraw     , NULL, geneSelect, FALSE, 2.0, "Features", ZMAPFEATURE_TRANSCRIPT },
    { NULL,    zMapDNAColumn    , NULL, NULL      , FALSE, 11.0, "DNA"    , ZMAPFEATURE_INVALID    },
  };/* init,   draw,            config, select,    isframe, prio, name,     type */

  int i, j, k;
  ZMapColumn *c;
  srMeth *methp;
  char *name;
  float priority;

  for( i=0; i < sizeof(defs)/sizeof(struct ZMapColDefs); i++)
    {
      if (defs[i].type == ZMAPFEATURE_INVALID && type == ZMAPFEATURE_INVALID)
	{ /* insert default column */
	  name = defs[i].name;
	  priority = defs[i].priority;
	  meth = 0;
	}
      else if ((type == defs[i].type))
	{
	  //	  if (!(methp = srMethodFromID(zMapPaneGetZMapRegion(pane), meth)))
	  //	    {
	      // TODO: sort out error handling here
	  //	      printf("Failed to find method in insertCol\n");
	  //	      exit;
	  //	    }
	  name = (char*)g_string_new(methp->name); //previously hung on pane->window->handle);  
	  priority = methp->priority;
	}
      else
	continue;
      

      /* TODO: This bit is completely wrong.  I think the pane->cols array needs to be in
      ** priority order, but GPtrArray doesn't have an insert function, just adds at the 
      ** end.  Might have to write an insert function or use GArray instead, but for now
      ** I'm not actually doing anything, as I've been in the pub at lunchtime and a couple
      ** of beers is a sure way to break it. */
      /*      for (j = 0; 
	   j < (&pane->cols)->len && 
	     g_ptr_array_index(pane->cols, j).priority < priority;
	   j++);
      */
      
      //      if (j < (&pane->cols)->len)
      //	{
      //	  /* default columns */
      //	  if (!meth &&
      //	      g_ptr_array_index(pane->cols, j).name == name)
      //	    continue; /* already there  */
      //	    
      //	  /* method columns */
      //	  if (meth &&
      //	      g_ptr_array_index(pane->cols, j).meth == meth &&
      //	      g_ptr_array_index(pane->cols, j).type == type)
      //	    continue; /* already there  */
      //	}
      
      //      if ( j < pane->cols->len)
      /* make space */
      //	for (k = arrayMax(pane->cols); k >= j+1; k--)
      //	  {
      //	ZMapColumn tmp =  array(pane->cols, k-1, ZMapColumn);
      //	    *arrayp(pane->cols, k, ZMapColumn) = tmp;
      //	  }

      
      //    c = g_ptr_array_index(pane->cols, j);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

      /* Needs sorting out............... */

      c->drawFunc = defs[i].drawFunc;
      c->configFunc = defs[i].configFunc;
      c->selectFunc = defs[i].selectFunc;
      c->isFrame = defs[i].isFrame;
      c->name = name;  
      c->pane = pane;
      c->priority = priority;
      c->type = type;
      c->meth = meth; /* zero for default columns */
      if (defs[i].initFunc)
	(*defs[i].initFunc)(pane, c);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

}

  
  
void buildCols(ZMapPane pane)
     /* Add a column for each method */
{
  ZMapRegion *zMapRegion; // = zMapPaneGetZMapRegion(pane);
  int i;
  for (i=0; i < zMapRegionGetSegs(zMapRegion)->len; i++)
      {
	ZMapFeature seg = &g_array_index(zMapRegionGetSegs(zMapRegion), ZMapFeatureStruct, i);
	methodID id = seg->method;
	if (id)
	  insertCol(pane, id, seg->type);
      }
}
	
void makezMapDefaultColumns(ZMapPane pane)
{
    insertCol(pane, 0, ZMAPFEATURE_INVALID);
}

/********************** end of file ****************************/
