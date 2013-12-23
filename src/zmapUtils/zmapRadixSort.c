/*  File: zmapRadixSort.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 * to sort in the reverse direction subtarct the integers from G_MAXUINT
 * if you have range limitations on key data (eg 24 bits not 32) then you can make the keys shorter

 * memory use is light: 1k for 256 32 bit pointers, mutiply that by 2 for tail end list pointers
 * and multiple again by 2 for 64 bit machines, so 4k in total.
 * if constricting a key is time consuming then consider doing it once only, which will use O(n) memory
 */

#include <ZMap/zmap.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <ZMap/zmapUtils.h>


#define N_DIGITS  256               /* this will never change, so don't even think about it */

/* int zmapUtils.h
typedef guint (ZMapRadixKeyFunc (gconstpointer thing,int digit);
*/

typedef struct
{
      GList *heads[N_DIGITS+1];     /* the lists that comprise our bins */
      GList *tails[N_DIGITS+1];     /* ... + and extra one for a zero terminator */
      GList *data;                  /* input and output list */
}
ZMapRadixSorterStruct, *ZMapRadixSorter;



GList * zMapRadixSort(GList *list, ZMapRadixKeyFunc key_func,int n_digit)
{
      ZMapRadixSorter this = g_new0(ZMapRadixSorterStruct,1);
      int i;
      int digit;
      GList *item;
      GList *cur;
      int cur_digit = 0;

      this->data = list;

      while(cur_digit < n_digit)
      {
                  /* sort the list into 256 bins */
            for(item = this->data; item; )
            {
                  digit = key_func(item->data,cur_digit);

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

            for(item = 0,i = N_DIGITS;i > 0;)
            {
                  GList *digit_list = this->heads[--i];

                  if(digit_list)
                  {
                        GList *tail = this->tails[i];
                        tail->next = item;
                        if(item)
                              item->prev = tail;
                        item = digit_list;
                  }
            }

            this->data = item;

            cur_digit++;
            if(cur_digit < n_digit)
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


/*
 * see MH17_test_radix_sort in zmapViewFeatureMask.c and zmapWindowContainerSummarise.c
 *
 * simple tests show this is much slower than merge sort!
 * generating a byte addressable key in the thing object would speed this up probably a lot
 * eg:

      guchar key[sizeof guint) * 2];
      guint *x = (giunt *) &key;
      *x++ = feature->x1;
      *x++ = G_MAXINT - feature->x2;

test results:
-----------------------------------------------------------
set         #feat times in ms ratio #feat/time
                  merge radix       merge radix
------------8 byte key sorting 1000 times------------------
est_mouse   87    7     34    x 5   /12   /3
est_mouse   87    7     33    x 5   /12   /3
est_other   322   54    124   x 2.5 /6    /3
est_other   322   31    111   x 3.5 /10   /3
est_human   528   54    269   x 5   /10   /2
est human   528   55    192   x 4   /10   /2.5
------------6 byte key sorting 100 times-----(adjust ratio/data x 10)
swissprot   9563  173   423   * 2.5 /5   /2
trembl      28292 683   2177  * 3.5 /4   /1.2
-----------------------------------------------------------

run time stats look very flaky
There's inaccuracies in the timing due to scheduling (results are real time)
But both merge sort and radix sort operate as worst case == best case
There has to be a mistake somewhere...
      merge sort should get slower as data increases (it does)
      radix sort should be consistently linear (it's not)

The theory was that this could be used in column summarise and run faster but it doesn't!

raw data:
est_other has 322 items
Start 8.700 normal sort
Stop  8.754 normal sort
Start 8.754 radix sort
Stop  8.878 radix sort

est_other has 322 items
Start 8.878 normal sort
Stop  8.909 normal sort
Start 8.909 radix sort
Stop  9.020 radix sort

est_mouse has 87 items
Start 9.020 normal sort
Stop  9.027 normal sort
Start 9.027 radix sort
Stop  9.061 radix sort

est_mouse has 87 items
Start 9.061 normal sort
Stop  9.068 normal sort
Start 9.068 radix sort
Stop  9.101 radix sort

est_human has 528 items
Start 9.102 normal sort
Stop  9.156 normal sort
Start 9.156 radix sort
Stop  9.325 radix sort

est_human has 528 items
Start 9.326 normal sort
Stop  9.381 normal sort
Start 9.381 radix sort
Stop  9.573 radix sort

swissprot has 9563 items
Start 8.923 normal sort
Stop  9.096 normal sort
Start 9.098 radix sort
Stop  9.519 radix sort
summarise swissprot: 444+9119/9563, max was 2

trembl has 28292 items
Start 9.595 normal sort
Stop  10.277      normal sort
Start 10.280      radix sort
Stop  12.457      radix sort
summarise trembl: 492+27800/28292, max was 3


 */
