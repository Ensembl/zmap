/*  File: zmapRadixSort.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: generic radix sort for linked lists keyed by integers
 *

 * CVS info:   $Id: zmapRadixSort.c,v 1.1 2011-04-07 13:52:39 mh17 Exp $
 *-------------------------------------------------------------------
 */


/*
 * (radix sort can be read about on wikipedia)
 * it's linear efficiency and sorts integers digit by digit
 * we use base 256 to represent the integers and store intermediate results in a table of 256 lists
 * 256 is a benign compromise between minimising the number of iterations and memory required
 * and processor architecture (byte addressable)
 * the algorithm is stable and is processed by the least significant digit first
 * to make this generic we require a function that extracts an integer key from an object

 * to combine keys make the array longer and place them in order (least significant first
 * to sort in the reverse direction negate the integers
 * if you have range limitations on key data (eg 24 bits not 32) then you can make the keys shorter

 * memory use is light: 1k for 256 32 bit pointers, mutiply that by 2 for tail end list pointers
 * and multiple again by 2 for 64 bit machines, so 4k in total.
 * if constricting a key is time consuming then consider doing it once only, which will use O(n) memory
 */

#include <ZMap/zmap.h>
#include <memory.h>
#include <ZMap/zmapUtils.h>


#define N_DIGITS  256               /* this will never change, so don't even think about it */

/* int zmapUtils.h
typedef int (*ZMapRadixKey) (gpointer thing,int digit);
*/

typedef struct
{
      GList *heads[N_DIGITS+1];       /* the lists that comprise our bins */
      GList *tails[N_DIGITS+1];       /* extra one for a zero terminator */

      ZMapRadixKey radix_key;       /* something to get the key */
      int cur_digit;                /* being processed now */
      int n_digit;                  /* how many iterations */

      GList *data;                  /* input and output list */
}
ZMapRadixSorterStruct, *ZMapRadixSorter;



GList * zMapRadixSort(GList *list, ZMapRadixKey key_func,int key_size)
{
      ZMapRadixSorter this = g_new0(ZMapRadixSorterStruct,1);
      int i;
      int digit;
      GList *item;
      GList *cur;

      this->data = list;
      this->n_digit = key_size;
      this->radix_key = key_func;

      while(this->cur_digit < this->n_digit)
      {
                  /* sort the list into 256 bins */
            for(item = this->data; item; )
            {
                  digit = this->radix_key(item->data,this->cur_digit);

                  cur = item;
                  item = item->next;

                  /* here for efficiency we code the next and prev pointers directly */
                  /* as GLib can't append without major grief, it would be quadratic */

                  cur->next = 0;
                  cur->prev = this->tails[digit];
                  if(cur->prev)
                        cur->prev->next = cur;
                  this->tails[digit] = cur;
                  if(!this->heads[digit])
                        this->heads[digit] = cur;
            }

                  /* join the bins into a single list in order */
            this->data = NULL;

            for(i = 0;i < N_DIGITS;i++)
            {
                  if(!this->data)
                        this->data = this->heads[i];

                  item = this->tails[i]->next = this->heads[i + 1];
                  if(item)
                        item->prev = this->tails[i];
            }

            this->cur_digit++;
            if(this->cur_digit < this->n_digit)
            {
                  /* empty the bins and do some more */
                  int size = sizeof(GList *) * N_DIGITS;
                  memset(this->heads,0,size);
                  memset(this->tails,0,size);
            }
      }

      list = this->data;
      g_free(this);
      return(list);
}

