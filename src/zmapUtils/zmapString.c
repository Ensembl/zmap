/*  File: zmapString.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Ideally we wouldn't have this but instead use an
 *              existing library but that's not always so easy.
 *
 * Exported functions: See ZMap/zmapString.h
 * HISTORY:
 * Last edited: Sep 25 13:42 2007 (edgrif)
 * Created: Thu Sep 20 15:21:42 2007 (edgrif)
 * CVS info:   $Id: zmapString.c,v 1.1 2007-09-27 13:07:40 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapString.h>


static int findMatch(char *target, char *query, gboolean caseSensitive) ;


/* This code was taken from acedb (www.acedb.org)

 match to template with wildcards.   Authorized wildchars are * ? #
     ? represents any single char
     * represents any set of chars
   case sensitive.   Example: *Nc*DE# fits abcaNchjDE23

   returns 0 if not found
           1 + pos of first sigificant match (i.e. not a *) if found
*/
int zMapStringFindMatch(char *target, char *query)
{
  int result ;

  result = findMatch(target, query, FALSE) ;

  return result ;
}

int zMapStringFindMatchCase(char *target, char *query, gboolean caseSensitive)
{
  int result ;

  result = findMatch(target, query, caseSensitive) ;

  return result ;
}





/* 
 *                Internal functions.
 */



static int findMatch(char *target, char *query, gboolean caseSensitive)
{
  int result = 0 ;
  char *c=target, *t=query;
  char *ts = 0, *cs = 0, *s = 0 ;
  int star=0;

  while (TRUE)
    {
      switch(*t)
	{
	case '\0':
	  if (!*c)
	    return  ( s ? 1 + (s - target) : 1) ;

	  if (!star)
	    return 0 ;

	  /* else not success yet go back in template */
	  t=ts; c=cs+1;
	  if(ts == query) s = 0 ;
	  break ;

	case '?':
	  if (!*c)
	    return 0 ;
	  if(!s) s = c ;
	  t++ ;  c++ ;
	  break;

	case '*':
	  ts=t;
	  while( *t == '?' || *t == '*')
	    t++;

	  if (!*t)
	    return s ? 1 + (s-target) : 1 ;

	  if (!caseSensitive)
	    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      while (g_ascii_toupper(*c) != g_ascii_toupper(*t))
		{
		  if(*c)
		    c++;
		  else
		    return 0 ;
		}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	      while (g_ascii_toupper(*c) != g_ascii_toupper(*t))
		{
		  c++;

		  if (!(*c))
		    return 0 ;
		}
	    }
	  else
	    {
	      while (*c != *t)
		{
		  if(*c)
		    c++;
		  else
		    return 0 ;
		}
	    }

	  star=1;
	  cs=c;
	  if(!s) s = c ;
	  break;

	default:
	  if (!caseSensitive)
	    {      
	      if (g_ascii_toupper(*t++) != g_ascii_toupper(*c++))
		{
		  if(!star)
		    return 0 ;

		  t=ts; c=cs+1;

		  if(ts == query)
		    s = 0 ;
		}
	      else
		{
		  if(!s)
		    s = c - 1 ;
		}
	    }
	  else
	    {
	      if (*t++ != *c++)
		{
		  if(!star)
		    return 0 ;

		  t=ts; c=cs+1;
		  if(ts == query)
		    s = 0 ;
		}
	      else
		{
		  if(!s) s = c - 1 ;
		}
	    }
	  break;
	}
    }

  return result ;
}

