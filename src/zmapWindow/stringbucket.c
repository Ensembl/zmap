/*  Last edited: Apr 13 15:00 2004 (rnc) */
/*  file: stringbucket.c
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

#include <../acedb/regular.h>
#include <stringbucket.h>

struct sbucket {
  STORE_HANDLE handle;
  struct sbucketchain *chain;
};

struct sbucketchain {
   struct sbucketchain *next;
};

StringBucket *sbCreate(STORE_HANDLE handle)
{
  StringBucket *b = halloc(sizeof(struct sbucket), handle);
  
  b->handle = handle;
  b->chain = NULL;

  return b;
			     
}

void sbDestroy(StringBucket *b)
{
  struct sbucketchain *c = b->chain;
  messfree(b);
  while (c) 
    {
       struct sbucketchain *tmp = c->next;
       messfree(c);
       c = tmp;
    }
}

char *str2p(char *string, StringBucket *b)
{
  struct sbucketchain *c = b->chain;
  while (c && (strcmp((char *)(c+1), string) !=0))
    c = c->next;
  
  if (!c)
    {
      c = halloc(sizeof(struct sbucketchain) + strlen(string) + 1, b->handle);
      c->next = b->chain;
      b->chain = c;
      strcpy((char *)(c+1), string);
    }
  return (char *)(c+1);
}

/*********************** end of file ***********************************/
