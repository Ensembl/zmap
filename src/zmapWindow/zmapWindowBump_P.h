/*  File: zmapWindowBump_P.h
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
 * Description: 
 *
 * HISTORY:
 * Last edited: Jul  6 13:58 2005 (edgrif)
 * Created: Thu Jun 30 16:29:04 2005 (edgrif)
 * CVS info:   $Id: zmapWindowBump_P.h,v 1.1 2005-07-12 10:05:55 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOWBUMP_P_H
#define ZMAP_WINDOWBUMP_P_H

#include <ZMap/zmapUtils.h>
#include <zmapWindowBump.h>					    /* include public header */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Does not need to be an extern I think..... */
/* allow verification of a ZMAPWINDOWBUMP pointer */
extern magic_t ZMAPWINDOWBUMP_MAGIC ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



typedef struct ZMapWindowBumpStruct_
{
  ZMagMagic *magic ;					    /* == &ZMAPWINDOWBUMP_MAGIC */

  int n ;						    /* max x, i.e. number of columns */

  float *bottom ;					    /* array of largest y in each column */


  /* I don't understand any of these original acedb comments....sigh.... */
  int min_space ;					    /* longest loop in y */

  float sloppy ;

  float maxDy ;						    /* If !doIt && maxDy !=0, Do not add
							       further down */

  int max ;						    /* largest x used (maxX <= n) */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* not needed at the moment ?? */

  int xAscii, xGapAscii ;
  float yAscii ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


} ZMapWindowBumpStruct ;

#endif /* ZMAP_WINDOWBUMP_P_H */
