/*  File: zmapWindowAllBase.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: There are some things common to all groups and items
 *              and this file contains them.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_ALL_BASE_H
#define ZMAP_WINDOW_ALL_BASE_H

#include <libzmapfoocanvas/libfoocanvas.h>

/* currently needed in alignmentfeatuer and featureset, if we stop using it in
 * alignment then it should go to the featureset internal header. */
/* Colours for matches to indicate degrees of colinearity. */
#define ZMAP_WINDOW_MATCH_PERFECT       "green"
#define ZMAP_WINDOW_MATCH_COLINEAR      "orange"
#define ZMAP_WINDOW_MATCH_NOTCOLINEAR   "red"


/* 
 * Should add some of the stats stuff here....
 * 
 *  */


/* Struct to hold basic stats for a particular zmap item. */
typedef struct ZMapWindowItemStatsType
{
  GType type ;						    /* GType for item. */
  char *name ;						    /* Text string name of item. */
  unsigned int size ;					    /* size in bytes of item struct. */
  unsigned int total ;					    /* Total objects created. */
  unsigned int curr ;					    /* Curr objects existing. */
} ZMapWindowItemStatsStruct, *ZMapWindowItemStats ;


GType zmapWindowItemTrueType(FooCanvasItem *item) ;
GType zmapWindowItemTrueTypePrint(FooCanvasItem *item) ;

void zmapWindowItemStatsInit(ZMapWindowItemStats stats, GType type) ;
void zmapWindowItemStatsIncr(ZMapWindowItemStats stats) ;
void zmapWindowItemStatsDecr(ZMapWindowItemStats stats) ;




#endif /* ZMAP_WINDOW_ALL_BASE_H */
