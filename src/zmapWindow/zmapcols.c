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

#include <wh/graph.h>
#include <wh/gex.h>
#include <wzmap/seqregion.h>
#include <wzmap/zmapcontrol.h>

#ifdef ZMAP

static void zMapScaleColumn(ZMapWindow *window, ZMapColumn *col, float *offset, int frame)
{
  *offset = zmDrawScale(*offset,
			zmVisibleCoord(window->root, zmCoordFromScreen(window, 0)),
			zmVisibleCoord(window->root, zmCoordFromScreen(window, window->graphHeight)));
}

static void pruneCols(ZMapWindow *window)
     /* Remove Columns which have invalid methods */
{ 
  int i, j;
  
  for (i = 0; i < arrayMax(window->cols); i++)
    {
      ZMapColumn *c = arrp(window->cols, i, ZMapColumn);
      if (c->type != SR_DEFAULT && c->meth && !srMethodFromID(window->root->region, c->meth))
	{
	  for (j = i+1; j < arrayMax(window->cols); j++)
	    arr(window->cols, j-1, ZMapColumn) = arr(window->cols, j, ZMapColumn);
	  arrayMax(window->cols)--;
	}
    }
}

static void insertCol(ZMapWindow *window, methodID meth, srType type)
{
  /* NB call this one with type == SR_DEFAULT to put in default columns. */

  /* Where column code links in.
     Fields are :
     1) Creation Routine.
     2) Draw routine.
     3) Config routine.
     4) Flag set for phase sensistive column - gets repeated three times.
     5) Name - only valid for SR_DEFAULT columns.
     6) Right priority - only valid for SR_DEFAULT columns.
     7) Type - SR_DEFAULT for default columns which are always present
               otherwise matches values in segs from convert routines.
  */
  static struct ZMapColDefs defs[] = {
    { NULL, zMapScaleColumn, NULL, NULL, FALSE, 1.0, "Scale", SR_DEFAULT },
    { nbcInit, zMapFeatureColumn, NULL, nbcSelect, FALSE, 2.0, "Features", SR_SEQUENCE },
    { nbcInit, zMapFeatureColumn, NULL, nbcSelect, FALSE, 2.0, "Features", SR_FEATURE },
    { nbcInit, zMapGeneDraw, NULL, geneSelect, FALSE, 2.0, "Features", SR_TRANSCRIPT },
    { NULL, zMapDNAColumn, NULL, NULL, FALSE, 11.0, "DNA", SR_DEFAULT },
  };

  int i, j, k;
  ZMapColumn *c;
  srMeth *methp;
  char *name;
  float priority;

  for( i=0; i < sizeof(defs)/sizeof(struct ZMapColDefs); i++)
    {
      if (defs[i].type == SR_DEFAULT && type == SR_DEFAULT)
	{ /* insert default column */
	  name = defs[i].name;
	  priority = defs[i].priority;
	  meth = 0;
	}
      else if ((type == defs[i].type))
	{
	  if (!(methp = srMethodFromID(window->root->region, meth)))
	    messcrash("Failed to find method in insertCol");
	  name = strnew(methp->name, window->root->look->handle);  
	  priority = methp->priority;
	}
      else
	continue;
      
      for (j = 0; 
	   j < arrayMax(window->cols) && 
	     arr(window->cols, j, ZMapColumn).priority < priority;
	   j++);
      
      
      if (j < arrayMax(window->cols))
	{
	  /* default columns */
	  if (!meth &&
	      arr(window->cols, j, ZMapColumn).name == name)
	    continue; /* already there  */
	    
	  /* method columns */
	  if (meth &&
	      arr(window->cols, j, ZMapColumn).meth == meth &&
	      arr(window->cols, j, ZMapColumn).type == type)
	    continue; /* already there  */
	}
      
      if ( j < arrayMax(window->cols))
	/* make space */
	for (k = arrayMax(window->cols); k >= j+1; k--)
	  {
	    ZMapColumn tmp =  array(window->cols, k-1, ZMapColumn);
	    *arrayp(window->cols, k, ZMapColumn) = tmp;
	  }
      
      c = arrayp(window->cols, j, ZMapColumn);
      c->drawFunc = defs[i].drawFunc;
      c->configFunc = defs[i].configFunc;
      c->selectFunc = defs[i].selectFunc;
      c->isFrame = defs[i].isFrame;
      c->name = name;  
      c->window = window;
      c->priority = priority;
      c->type = type;
      c->meth = meth; /* zero for default columns */
      if (defs[i].initFunc)
	(*defs[i].initFunc)(window, c);
    }
}

  
  
void buildCols(ZMapWindow *window)
     /* Add a column for each method */
{
  SeqRegion *region = window->root->region;
  int i;
  for (i=0; i < arrayMax(region->segs); i++)
      {
	SEG *seg = arrp(region->segs, i, SEG);
	methodID id = seg->method;
	if (id)
	  insertCol(window, id, seg->type);
      }
}
	
void makezMapDefaultColumns(ZMapWindow *window)
{
    insertCol(window, 0, SR_DEFAULT);
}
#endif
