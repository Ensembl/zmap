/*  File: zmapWindowBump.c
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
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jul  6 14:01 2005 (edgrif)
 * Created: Thu Jun 30 16:25:30 2005 (edgrif)
 * CVS info:   $Id: zmapWindowBump.c,v 1.1 2005-07-12 10:05:56 edgrif Exp $
 *-------------------------------------------------------------------
 */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include "regular.h"
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#include <zmapWindowBump_P.h>


/* Bumper works by resetting x,y

   bumpItem inserts and fills the bumper

   bumpTest tests x,y, but does not fill bumper
     this allows to reconsider the wisdom of bumping

   bumpRegister (called after bumpTest) fills 
     Test+Register == Add 

   bumpText returns number of letters that
     can be bumped without *py moving more than 3*dy  
*/



/* An idea we should use in our own modules/data structures....? */
static ZMagMagic bump_magic_G = "ZMAP_BUMP_MAGIC_VALUE" ;


#define MINBUMP ((float)(-2000000000.0))


/************* bump package based on gmap text **************/

ZMapWindowBump zmapWindowBumpCreate(int nCol, int min_space)
{
  ZMapWindowBump bump ;
  int i ;

  zMapAssert(nCol >= 1 && min_space >= 0) ;

  bump = (ZMapWindowBump)g_new0(ZMapWindowBumpStruct, 1) ;
  bump->magic = &bump_magic_G ;

  if (nCol < 1)
    nCol = 1 ;

  bump->n = nCol ;

  bump->bottom = (float*)g_malloc(nCol * sizeof(float)) ;
  for (i = 0 ; i < nCol ; i++)
    {
      bump->bottom[i] = MINBUMP ;
    }

  if (min_space < 0)
    min_space = 0 ;

  bump->min_space = min_space ;
  bump->sloppy = 0 ;
  bump->max = 0 ;

  return bump ;
}


int zmapWindowBumpMax(ZMapWindowBump bump)
{
  zMapAssert(bump && (bump->magic == &bump_magic_G)) ;

  return bump->max ;
}


float zmapWindowBumpSetSloppy( ZMapWindowBump bump, float sloppy)
{
  float old ;

  zMapAssert(bump && (bump->magic == &bump_magic_G)) ;

  old = bump->sloppy;
  bump->sloppy = sloppy ;

  return old ;
}

void zmapWindowBumpRegister(ZMapWindowBump bump, int wid, float height, int *px, float *py)
{ 
  int i = *px , j ;

  zMapAssert(bump && (bump->magic == &bump_magic_G)) ;

  j = wid < bump->n ? wid : bump->n ;

  if (bump->max < i + j - 1) 
    bump->max = i + j - 1 ;

  while (j--)	/* advance bump */
    bump->bottom[i+j] = *py + height ;

  return ;
}


gboolean zmapWindowBumpItem(ZMapWindowBump bump, int wid, float height, int *px, float *py)
{
  gboolean result ;

  zMapAssert(bump && (bump->magic == &bump_magic_G)) ;

  result = zmapWindowBumpAdd(bump, wid, height, px, py, TRUE) ;

  return result ;
}

gboolean zmapWindowBumpTest(ZMapWindowBump bump, int wid, float height, int *px, float *py)
{
  gboolean result ;

  zMapAssert(bump && (bump->magic == &bump_magic_G)) ;

  result = zmapWindowBumpAdd(bump, wid, height, px, py, FALSE) ;

  return result ;
}


     /* works by resetting x, y */
gboolean zmapWindowBumpAdd(ZMapWindowBump bump, int wid, float height, 
		       int *px, float *py, gboolean doIt)
{
  gboolean result = FALSE ;
  int i, j ;
  int x = *px ;
  float ynew, y = *py ;

  zMapAssert(bump && (bump->magic == &bump_magic_G)) ;

  if (x + wid + bump->min_space > bump->n)
    x = bump->n - wid - bump->min_space ;

  if (x < 0) 
    x = 0 ;

  if (wid > bump->n)		/* prevent infinite loops */
    wid = bump->n ;

  if (y <= MINBUMP)
    y = MINBUMP + 1 ;

  ynew = y ;

  while (TRUE)
    {
      for (i = x, j = wid ; i < bump->n ; ++i)	/* always start at x */
	{
	  if (bump->bottom[i] > y + bump->sloppy)
	    {
	      j = wid ;
	      ynew = y ; /* this line was missing in the old code */
            }
	  else 
	    {
	      if (bump->bottom[i] > ynew)
		ynew = bump->bottom[i] ;

	      if (!--j)		/* have found a gap */
		break ;
	    }
	}

      if (!j)	
	{ 
	  if (doIt)
	    {
	      for (j = 0 ; j < wid ; j++)	/* advance bump */
		bump->bottom[i - j] = ynew+height ;

	      if (bump->max < i + 1) 
		bump->max = i + 1 ;
	    }
	  *px = i - wid + 1 ;
	  *py = ynew ;

	  result = TRUE ;
	  break ;
	}
      
      y += 1 ;	/* try next row down */

      if (!doIt && bump->maxDy && y - *py > bump->maxDy)
	{
	  result = FALSE ;
	  break ;
	}
    }

  return result ;
}



void zmapWindowBumpDestroy(ZMapWindowBump bump)
{
  zMapAssert(bump && (bump->magic == &bump_magic_G)) ;

  /* Reset to catch apps that continue to point at free'd bump structs. */
  bump->magic = NULL ;

  g_free(bump->bottom) ;

  g_free(bump) ;

  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* NOT SURE WE NEED THESE.... */


/* I THINK WE SHOULD NOT NEED THIS, IT SEEMS KIND OF USELESS.... */

ZMapWindowBump zmapWindowBumpReCreate(ZMapWindowBump bump, int nCol, int min_space)
{
  int i ;
    
  if (!bump)
    return bumpCreate(nCol, min_space) ;

  if (bump->magic != &bump_magic_G)
    messcrash("bumpReCreate received corrupt bump->magic");

  if (nCol < 1)
    nCol = 1 ;
  if (nCol != bump->n)
    {  messfree (bump->bottom) ;
       bump->bottom = (float*) messalloc (nCol*sizeof (float)) ;
       bump->n = nCol ;
     }
    
  for (i = 0 ; i < nCol ; ++i)
    bump->bottom[i] = MINBUMP ;
  if (min_space < 0)
    min_space = 0 ;
  bump->min_space = min_space ;
  bump->sloppy = 0 ;
  bump->max = 0 ;
  
  return bump ;
} /* bumpReCreate */


/* mhmp 16/05/97 */
void ZmapWindowBumpItemAscii(ZMapWindowBump bump, int wid, float height, int *px, float *py)
                                /* works by resetting x, y */
{
  int x = *px ;
  float ynew, y = *py ;

  zMapAssert(bump->magic == &bump_magic_G) ;
    messcrash("asciiBumpItem received corrupt bump->magic");

  if (bump->xAscii != 0)
    {
      if (bump->xAscii + wid + bump->xGapAscii > bump->n)
	{
	  ynew = y + 1 ; 
	  x = 0 ;
	  bump->xAscii = wid ;
	}
      else
	{
	  ynew = y ;
	  x = bump->xAscii + bump->xGapAscii ;
	  bump->xAscii = x + wid ;
	}
    }
  else
    {
      if (x + wid > bump->n)
	{
	  if ((y - bump->yAscii) < 1 && (int) y == (int) bump->yAscii)
	    ynew = y + 1 ;  
	  else
	    ynew = y ; 
	  x = 0 ;
	  bump->xAscii = wid ;
	}
      else
	{
	  if (y != bump->yAscii && (int) y == (int) bump->yAscii)
	    ynew = y + 1 ; 
	  else
	    ynew = y ;
	}
    }
  *px = x ;
  *py = ynew ;
  bump->yAscii = ynew ;
}


/* abbreviate text, if vertical bump exceeds dy 
   return accepted length 
*/
int zmapWindowBumpText (ZMapWindowBump bump, char *text, int *px, float *py, float dy, gboolean vertical)
{ 
  int w, n, x = *px;  
  float y = *py, h, old = bump->maxDy ;

  zMapAssert(bump->magic == &bump_magic_G) ;

  if (!text || !*text) return 0 ;
  n = strlen(text) ;
  bump->maxDy = dy ;
  while (TRUE)
    {
      x = *px ; y = *py ; /* try always from desired position */
      if (vertical)  /*like in the genetic map */
	{ w = n + 1 ; h = 1 ;}
      else           /* like in pmap */
	{ w = 1 ; h = n + 1 ;}  
      if (bumpAdd (bump, w, h, &x, &y, FALSE))
	{ /* success */
	  bump->maxDy = old ;
	  bumpRegister(bump, w, h, &x, &y) ;
	  *px = x ; *py = y ;
	  return n ;
	}

      if (n > 7)
	{ n = 7 ; continue ; }
      if (n > 3)
	{ bump->maxDy = 2 * dy ; n = 3 ; continue ; }
      if (n > 1)
	{ bump->maxDy = 3 * dy ; n = 1 ; continue ; }
      bump->maxDy = old ;
      return 0 ; /* failure */
    } 
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



