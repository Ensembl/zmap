/*  file: zmapsequence.c
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
#include <wh/dna.h>

#ifdef ZMAP

void zMapDNAColumn(ZMapWindow *window, ZMapColumn *col, 
		   float *offsetp, int frame)
{
  float offset = *offsetp;
  float max = offset;
  Coord seqstart = srCoord(window->root->region, window->centre) -
    (window->graphHeight * window->basesPerLine) / 2;
  Coord seqend = srCoord(window->root->region, window->centre) +
    (window->graphHeight * window->basesPerLine) / 2;

  Coord i;

  if (seqstart < window->root->region->area1)
    seqstart = window->root->region->area1;

  if (seqend > window->root->region->area2)
    seqend = window->root->region->area2;

  if (!srGetDNA(window->root->region))
    return;

  for (i = seqstart; i < seqend; i += window->basesPerLine)
    {
      char *dnap, buff[10];
      int j;
      ScreenCoord y =  zmScreenCoord(window, i);
      sprintf(buff, "%7d", zmVisibleCoord(window->root, i));
      graphText(buff, offset, y);

      dnap = arrp(window->root->region->dna,
		  i - (window->root->region->area1-1), 
		  char);

      for (j = 0; j < window->basesPerLine; j++)
	{
	  float x = offset+8+j;
	  buff[0] = dnaDecodeChar[(int)*(dnap+j)];
	  buff[1] = 0;
	  graphText(buff, x, y);
	  
	  if (j+i > window->root->region->area2)
	    break;

	  if (j+1 == window->DNAwidth)
	    {
	      graphText("...", x+1, y);
	      j = window->basesPerLine;
	      x += 4;
	    }
	  
	  if (x > max) 
	    max = x;
	}
    }
	
  *offsetp = max+1;
}

#endif
