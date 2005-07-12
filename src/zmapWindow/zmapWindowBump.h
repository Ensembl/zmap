/*  File: zmapWindowBump.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: This code controls the alignment of features within
 *              a column, e.g. where there are lots of features all
 *              much the in same position, it helps to display them
 *              shifted to the right in certain ways.
 *              
 *              The code is adapted from the acedb bumping code.
 *
 * HISTORY:
 * Last edited: Jul  6 13:29 2005 (edgrif)
 * Created: Thu Jun 30 16:27:32 2005 (edgrif)
 * CVS info:   $Id: zmapWindowBump.h,v 1.1 2005-07-12 10:05:55 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOWBUMP_H
#define ZMAP_WINDOWBUMP_H

#include <glib.h>

typedef struct ZMapWindowBumpStruct_ *ZMapWindowBump ;


ZMapWindowBump zmapWindowBumpCreate(int num_cols, int min_space) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* DO WE REALLY NEED THIS ??? */
ZMapWindowBump zmapWindowBumpReCreate (ZMapWindowBump bump, int ncol, int minSpace);

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


float zmapWindowBumpSetSloppy( ZMapWindowBump bump, float sloppy) ;
int zmapWindowBumpMax(ZMapWindowBump bump) ;
gboolean zmapWindowBumpItem(ZMapWindowBump bump, int wid, float height, int *px, float *py) ;
gboolean zmapWindowBumpTest(ZMapWindowBump bump, int wid, float height, int *px, float *py) ;
gboolean zmapWindowBumpAdd(ZMapWindowBump bump, int wid, float height, int *px, float *py,
			   gboolean doIt);
void zmapWindowBumpRegister(ZMapWindowBump bump, int wid, float height, int *px, float *py) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* NEEDED ??? */
int zmapWindowBumpText(ZMapWindowBump bump, char *text, int *px, float *py, float dy,
		       gboolean vertical) ;

void ZmapWindowBumpItemAscii(ZMapWindowBump bump, int wid, float height, int *px, float *py) ;
                                /* works by resetting x, y */

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


void zmapWindowbumpDestroy(ZMapWindowBump bump) ;



#endif /* ZMAP_WINDOWBUMP_H */
