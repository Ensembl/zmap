/*  File: zmapSeqBitmap.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Apr 16 09:26 2009 (edgrif)
 * Created: Tue Feb 17 09:50:34 2009 (rds)
 * CVS info:   $Id: zmapSeqBitmap.c,v 1.4 2010-06-14 15:40:14 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <stdio.h>
#include <glib.h>
#include <ZMap/zmapSeqBitmap.h>


#define ZMAP_BIN_MAX_VALUE(BITMAP) ((1 << BITMAP->bin_depth) - 1)

typedef struct _zmapSeqBitmapStruct
{
  int start_value;
  int bitmap_size;
  int bin_size;
  int bin_depth;
  int *bitmap;
} ZMapSeqBitmapStruct;


/* internal */
/* get the index and it's value from the bitmap */
static int get_bin_index_value(ZMapSeqBitmap bitmap, int world, int *value_out);

/* get the index and it's value by computation for inserting into the bitmap. Does not insert. */
static int get_bin_index_calculate_start(ZMapSeqBitmap bitmap, int world, int *value_out);

/* get the index and it's value by computation for inserting into the bitmap. Does not insert. */
static int get_bin_index_calculate_end(ZMapSeqBitmap bitmap, int world, int *value_out);

/* get the index and it's value by computation for inserting into the bitmap. Does not insert. */
static gboolean get_bin_index_calculate_full(ZMapSeqBitmap bitmap, int world,
					     gboolean is_start, 
					     int *index_out, int *value_out);
/* actually set the value. */
static void set_index_value(ZMapSeqBitmap bitmap, int index, int value);

/* insert the values for the indices and full values for indices between index1 and index2 */
static void fill_between_indices_values(ZMapSeqBitmap bitmap, 
					int index1, int value1,
					int index2, int value2);


static char *get_morse(ZMapSeqBitmap bitmap, int value);


/* 
 *   binning...
 *
 *  -----------------    ----- -     ---        --------------
 *  012345678901234567890123456789012345678901234567890123456789
 *  !         !         !         !         !         !         !
 *     512         64       159       25        497       128       
 */

ZMapSeqBitmap zmapSeqBitmapCreate(int start, int size, int bin_size)
{
  ZMapSeqBitmap bitmap = NULL;

  if((bitmap = g_new0(ZMapSeqBitmapStruct, 1)))
    {
      int tmp, depth = 30;

      tmp  = (int)(bin_size / depth);
      tmp *= depth;

      bitmap->start_value = start;
      bitmap->bin_depth   = depth;
      /*             size = (39216 / 3000) + 1*/
      bitmap->bin_size    = tmp;
      bitmap->bitmap_size = (size / bitmap->bin_size) + 1;
      bitmap->bitmap      = g_new0(int, bitmap->bitmap_size);
    }

  return bitmap;
}

void zmapSeqBitmapMarkRegion(ZMapSeqBitmap bitmap, int world1, int world2)
{
  int start_idx, end_idx;
  int start_val, end_val;

  if((start_idx = get_bin_index_calculate_start(bitmap, world1, &start_val)) != -1)
    {
      if((end_idx = get_bin_index_calculate_end(bitmap, world2, &end_val)) != -1)
	{
	  fill_between_indices_values(bitmap, 
				      start_idx, start_val,
				      end_idx, end_val);
#ifdef RDS_DONT_INCLUDE
	  zmapSeqBitmapPrint2(bitmap);
#endif
	}
    }

  return ;
}
#ifdef RDS_DONT_INCLUDE
GList *zmapSeqBitmapGetCoordsMarked(ZMapSeqBitmap bitmap)
{
  GList *list = NULL;
  int i, j, k;
  int coord, interval;

  coord = bitmap->start_value;

  for(i = 0; i< bitmap->bitmap_size; i++)
    {
      ZMapSpan span;
      int bin_value;
      int marked = 0;

      bin_value = bitmap->bitmap[i];
      k = 1;

      span = g_new0(ZMapSpanStruct, 1);

      for(j = 0; j < bitmap_size->bin_depth; j++)
	{
	  if(bin_value & k)
	    {
	      /* marked */
	      
	    }
	  else
	    {
	      /* not marked */
	    }

	  coord += 
	  k *= 2;
	}

      g_free(span);

    }
  

  return list;
}
#endif
gboolean zmapSeqBitmapIsRegionFullyMarked(ZMapSeqBitmap bitmap, int world1, int world2)
{
  gboolean fully_marked = TRUE;
  int start_idx, end_idx;
  int start_val, end_val;
  int start_calc_val, end_calc_val;

  if((start_idx = get_bin_index_value(bitmap, world1, &start_val)) != -1)
    {
      get_bin_index_calculate_start(bitmap, world1, &start_calc_val);

      if((end_idx = get_bin_index_value(bitmap, world2, &end_val)) != -1)
	{
	  int i, max;

	  get_bin_index_calculate_end(bitmap, world2, &end_calc_val);

	  max = ZMAP_BIN_MAX_VALUE(bitmap);

	  if(start_idx < end_idx)
	    {
	      for(i = start_idx; i <= end_idx; i++)
		{
		  if(i == start_idx)
		    {
		      if((start_calc_val & start_val) != start_calc_val)
			fully_marked = FALSE;
		    }
		  else if(i == end_idx)
		    {
		      if((end_calc_val & end_val) != end_calc_val)
			fully_marked = FALSE;
		    }
		  else if(bitmap->bitmap[i] < max)
		    fully_marked = FALSE;
		}
	    }
	  else
	    {
	      int calc, actual;
	      calc   = (start_calc_val & end_calc_val);
	      actual = (start_val & end_val);

	      if((calc & actual) != calc)
		fully_marked = FALSE;
	    }
	}
      else
	fully_marked = FALSE;
    }
  else
    fully_marked = FALSE;

  return fully_marked;
}

void zmapSeqBitmapPrint2(ZMapSeqBitmap bitmap)
{
  int i;

  printf("bitmap (%p): ", bitmap);

  if(bitmap->bitmap_size > 0)
    printf("%s", get_morse(bitmap, bitmap->bitmap[0]));

  for(i = 1; i < bitmap->bitmap_size; i++)
    {
      printf(",%s", get_morse(bitmap, bitmap->bitmap[i]));
    }

  printf("\n");

  return ;
}


void zmapSeqBitmapPrint(ZMapSeqBitmap bitmap)
{
  int i;

  printf("bitmap (%p): ", bitmap);

  if(bitmap->bitmap_size > 0)
    printf("%d", bitmap->bitmap[0]);

  for(i = 1; i < bitmap->bitmap_size; i++)
    {
      printf(",%d", bitmap->bitmap[i]);
    }

  printf("\n");

  return ;
}

ZMapSeqBitmap zmapSeqBitmapDestroy(ZMapSeqBitmap bitmap)
{
  g_free(bitmap->bitmap);

  g_free(bitmap);

  bitmap = NULL;

  return bitmap;
}


/* INTERNALS */

static int get_bin_index_value(ZMapSeqBitmap bitmap, int world, int *value_out)
{
  int index = -1;
  int value = 0;
  int tmp;

  if(get_bin_index_calculate_full(bitmap, world, FALSE, &tmp, NULL))
    {
      index = tmp;

      value = bitmap->bitmap[index];

      if(value_out)
	*value_out = value;
    }

  return index;
}

static int get_bin_index_calculate_start(ZMapSeqBitmap bitmap, int world, int *value_out)
{
  int index = -1, tmp;

  if(get_bin_index_calculate_full(bitmap, world, TRUE, &tmp, value_out))
    index = tmp;

  return index;
}

static int get_bin_index_calculate_end(ZMapSeqBitmap bitmap, int world, int *value_out)
{
  int index = -1, tmp;
  
  if(get_bin_index_calculate_full(bitmap, world, FALSE, &tmp, value_out))
    index = tmp;

  return index;
}

static gboolean get_bin_index_calculate_full(ZMapSeqBitmap bitmap, int world,
					     gboolean is_start, 
					     int *index_out, int *value_out)
{
  gboolean valid = TRUE;
  int index = -1;

  world -= bitmap->start_value;

  index  = world / bitmap->bin_size;

  if(index > bitmap->bitmap_size)
    valid = FALSE;

  if(index_out && valid)
    *index_out = index;

  if(value_out && valid)
    {
      int value = 0;
      int mod_index, bin_denom, tmp;

      mod_index = world % bitmap->bin_size;   
      
      bin_denom = bitmap->bin_size / bitmap->bin_depth;
      
      if(is_start)
	{
	  tmp = mod_index / bin_denom;
	  /* |-----------------------------| */
	  /*           s-------------------| */
	  /* (1<<tmp)-1                      */
	  value = ZMAP_BIN_MAX_VALUE(bitmap) - ((1 << tmp) - 1);
	}
      else
	{
	  tmp = mod_index / bin_denom + 1;
	  /* 0 -> no_depth_index */
	  value = (1 << (tmp + 1)) - 1;
	}

      *value_out = value;
    }

  return valid;
}

static void set_index_value(ZMapSeqBitmap bitmap, int index, int value)
{
  bitmap->bitmap[index] |= value;
  return ;
}

static void fill_between_indices_values(ZMapSeqBitmap bitmap, 
					int index1, int value1,
					int index2, int value2)
{
  int i;

  g_return_if_fail(index2 >= index1);

  if(index1 < index2)
    {
      for(i = index1 + 1; i < index2; i++)
	{
	  set_index_value(bitmap, i, ZMAP_BIN_MAX_VALUE(bitmap));
	}

      set_index_value(bitmap, index1, value1);
      set_index_value(bitmap, index2, value2);
    }
  else if(index1 == index2)
    {
      /* need to modify value1 & value2 */
      set_index_value(bitmap, index1, value1 & value2);
    }

  return ;
}

static char *get_morse(ZMapSeqBitmap bitmap, int value)
{
  static char morse[32] = {'\0'};
  int i, m = 1;

  for(i = 0; i < bitmap->bin_depth; i++)
    {
      char c;

      if(value & m)
	c = '-';
      else
	c = '.';

      morse[i] = c;

      m *= 2;
    }

  return &morse[0];
}

