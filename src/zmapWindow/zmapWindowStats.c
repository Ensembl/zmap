/*  File: zmapWindowStats.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2006
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Functions for window statistics, e.g. number of boxes drawn.
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Nov  7 10:19 2006 (edgrif)
 * Created: Tue Nov  7 10:10:25 2006 (edgrif)
 * CVS info:   $Id: zmapWindowStats.c,v 1.1 2006-11-07 15:20:11 edgrif Exp $
 *-------------------------------------------------------------------
 */


#include <zmapWindow_P.h>




void zmapWindowStatsReset(ZMapWindowStats stats)
{

  /* this is fine for now as struct only contains ints, when we have other data we will need to do
     other stuff. */
  memset(stats, 0, sizeof(ZMapWindowStatsStruct)) ;

  return ;
}


/* Crude, in the end we will do a window for this and a dump function. */
void zmapWindowStatsPrint(ZMapWindowStats stats)
{

  printf("\nMatches -  total: %d,  gapped: %d,  not perfect gapped: %d, ungapped: %d,\n"
	 "Boxes   -  total: %d,  gapped boxes: %d,  ungapped boxes: %d, gapped boxes not drawn: %d\n\n",
	 stats->total_matches, stats->gapped_matches, stats->not_perfect_gapped_matches, stats->ungapped_matches,
	 stats->total_boxes, stats->gapped_boxes, stats->ungapped_boxes, stats->imperfect_boxes) ;

  return ;
}
