/*  File: zmapSkipList.c
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
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
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * a simple implemenation of a skip list data structure
 * NOTE we expect the lists to be static except exceptionally
 * and the module is designed with this in mind
 * used initially by zmapWindowGraphDensityItem.c
 *-------------------------------------------------------------------
 */

/*
 * NOTE William Pugh's original example code can be found via ftp://ftp.cs.umd.edu/pub/skipLists/
 * but we adopt slightly different approach:
 * - we use forward and backward pointers not just forward
 * - index levels are implemented by list pointers up and down not an array, which means that we
 *   do not concern ourselves with how nuch data we want to index and data is allocated only when needed
 * - the list layers are explicitly balanced by inserting and deleting nodes
 * - layers are removed when they become empty
 *
 * NOTE initially we do not implement balancing, and stop at the point where
 * the data structure is secure through insert and delete operations
 */

#include <memory.h>
#include <stdio.h>
#include <ZMap/zmapSkipList.h>


#if SLOW_BUT_EASY

#define allocSkipList() g_new0(zmapSkipListStruct,1)
#define freeSkipList(x) g_free(x)

#else

/* faster but very difficult to detect the change */

#define N_SKIP_LIST_ALLOC	1000

static long n_block_alloc = 0;
static long n_skip_alloc = 0;
static long n_skip_free = 0;

static ZMapSkipList allocSkipList(void);
static void freeSkipList(ZMapSkipList sl);

static ZMapSkipList skip_list_free_G = NULL;

static ZMapSkipList allocSkipList(void)
{
      ZMapSkipList sl;

      if(!skip_list_free_G)
      {
            int i;
            sl = g_new(zmapSkipListStruct,N_SKIP_LIST_ALLOC);

            for(i = 0;i < N_SKIP_LIST_ALLOC;i++)
                  freeSkipList(sl++);

		n_block_alloc++;
      }

      sl = skip_list_free_G;
      if(sl)
      {
      	skip_list_free_G = sl->next;

      	/* these can get re-allocated so must zero */
      	memset((gpointer) sl,0,sizeof(zmapSkipListStruct));
      }

      n_skip_alloc++;
      return(sl);
}


/* need to be a ZMapSkipListFreeFunc for use as a callback */
static void freeSkipList(ZMapSkipList sl)
{
      sl->next = skip_list_free_G;
      skip_list_free_G = sl;

	n_skip_free++;
}

#endif



ZMapSkipList zMapSkipListCreate(GList *data_in, GCompareFunc cmp)
{
	int n_node = SKIP_LIST_NORM_SKIP+1,n_skip;
	GList *l;
	ZMapSkipList head = 0,cur = 0,new, old;

	/* NOTE data_in must be pre-sorted if cmp is NULL*/
	if(cmp)
		data_in = g_list_sort(data_in,cmp);

	/* NOTE these two loops are very similar, make any changes in both! */

	/* create base layer above the data */
	for(l = data_in,n_node = 0;l; l = l->next)
	{
		new = allocSkipList();
		new->data = l->data;

		if(cur)
		{
			cur->next = new;
			new->prev = cur;
		}
		cur = new;
		if(!head)
			head = new;
		n_node++;
	}

	/* create upper layers */
	while (n_node > SKIP_LIST_NORM_SKIP)
	{
		for(old = head,head = cur = NULL,n_node = 0,n_skip = 0;old;old = (ZMapSkipList) old->next)
		{
			if(!n_skip)
			{
				new = allocSkipList();
				new->down = old;
				new->data = old->data;

				if(cur)
				{
					cur->next = new;
					new->prev = cur;
				}
				cur = new;
				if(!head)
					head = new;
				n_skip = SKIP_LIST_NORM_SKIP;
				n_node++;

				old->up = new;
			}
		}

	}

	return(head);
}



/* to avoid handling NULL in the cmp function we provide this to get the first leaf node
 * NOTE that it's possible to descend to ta leaf frpm the head ans still have previous leaves
 * depending on implementation details - this is due to add/ delete operations
 * but we do know that downward links go direct to a leaf
 * NOTE finding the last item efficiently is a bit harder - romm for more design here??
 * an alternative strategy is to know min and max key values and Find them
 */
ZMapSkipList zMapSkipListFirst(ZMapSkipList head)
{
	if(head)
	{
		while(head->down)
			head = head->down;
		while(head->prev)
			head = head->prev;
	}
	return head;
}


int zMapSkipListCount(ZMapSkipList head)
{
	int i;

	head = zMapSkipListFirst(head);
	for(i = 0; head; i++)
		head = head->next;
	return(i);
}


/* we could had a tail pointer handy, but we'd need to wrap the data in a class thingy
 * keep it simple: we don't expect to call this fucntion repeatedly
 */
ZMapSkipList zMapSkipListLast(ZMapSkipList head)
{
	while(head)		/* if true will stay so */
	{
		/* there's alwasy a down pointer, not always an up
		 * so we keep going next, then down, then next...
		 */
		while(head->next)
			head = head->next;
		if(!head->down)
			break;
		head = head->down;
	}
	return head;
}

/* return a pointer to the lowest list layer */
/* find first occurence of matching data */
/* if none then the one before or the one after if at the start of the list */
/* we allow for non-unique values */
/* key is a struct like the ones at the bottom of the list */

ZMapSkipList zMapSkipListFind(ZMapSkipList head, GCompareFunc cmp, gconstpointer key)
{
	ZMapSkipList sl = head;

	if(!head)
		return NULL;

	while(cmp(sl->data,key) < 0 && sl->next)
		sl = (ZMapSkipList) sl->next;

	if(sl->down)
		return(zMapSkipListFind(sl->down, cmp, key));

	while(sl->prev && cmp(sl->prev->data,key) >= 0)
		sl = (ZMapSkipList) sl->prev;

	if(sl->prev && cmp(sl->data,key) > 0)	/* bias to one before if no exact match possible */
		sl = sl->prev;

	return(sl);
}



/* for the moment just add to bottom layer */
/* we don't use dynamic data hardly ever */
/* returns head, it may change if there is only one layer */
/* if you need the actual base skip list struct do a Find and test for data pointer */
ZMapSkipList zMapSkipListAdd(ZMapSkipList head, GCompareFunc cmp, gpointer data)
{
	ZMapSkipList here;
	int side;
	ZMapSkipList new = allocSkipList();

	new->data = data;

	if(head)
	{
		here = zMapSkipListFind(head,cmp,data);
#warning SkipListAdd balancing upper layers not implemented

		/* this is less critical than remove */

		side = cmp(here->data, data);
		new->up = here->up;

		if(side > 0)
		{
			if(here->next)
				here->next->prev = new;
			new->next = here->next;
			new->prev = here;
			here->next = new;
		}
		else /* equal or before here: insert before */
		{
			if(here->prev)
			{
				here->prev->next = new;
				new->up = here->prev->up;
			}
			new->prev = here->prev;
			new->next = here;
			here->prev = new;

			if(here == head)		/* ->up will be NULL */
				head = new;
			/*
			 * NOTE that the new item may be before the first key in the head
			 * but can still be found due to the lookup implementation
			 */
		}
	}
	else
	{
		head = new;
	}

	return head;
}


/* this removes a data element, sl is assumed to be a base list */
/* if not we make it one */
/* to remove by value do a SkipListFind first */
/* returns the head (may change) */
ZMapSkipList zMapSkipListRemove(ZMapSkipList head, ZMapSkipListFreeFunc free_func)
{
	/* NOTE: we must tidy up downward pointers from upper layers */
	/* NOTE: we must tidy up upward pointers from lower layers
	 * as these are only non-NULL when layers are stacked
	 * this is taken care of by removing the stack of layers
	 */
	ZMapSkipList up;
	ZMapSkipList sl = head;
	gpointer data = sl->data;

	if(!sl)
		return NULL;

	while(sl->down)
		sl = sl->down;
	if(free_func)
		free_func(sl->data);

	if(sl->prev)
		sl->prev->next = sl->next;
	if(sl->next)
		sl->next->prev = sl->prev;

	up = sl->up;


	while(up && up->data == data)
	{
		freeSkipList(sl);

#warning SkipListRemove upper layer balancing not implemented
		/* just remove offending downward pointers */
		/* if we remove a deep key then we double the skip distance */
		/* and the list starts to go degnerate */
		/* FTM i didn't bother to sort this as data is almost entirely static */

/*
 * - balancing intermediate/upper layers -
 * calculate # links fwd and back between this link and its neighbours
 * - move data pointer fwd/back -> stack slides sideways !!
 * - remove link if sum < max threshold then repeat process further up
 * if only one link remove layer
 */

		if(up->prev)
			up->prev->next = up->next;
		if(up->next)
			up->next->prev = up->prev;

		sl = up;
		up = up->up;	/* possibly the whackiest line of code i've written for years :-) */
	}

	/* if up != NULL then we didn't change the head */
	/* sl is the last one to delete */

	if(sl == head)
		head = sl->next;	/* may be NULL if we remove the last one */

	freeSkipList(sl);

	return head;
}


void zMapSkipListDestroy(ZMapSkipList skip_list, ZMapSkipListFreeFunc free_func)
{
	ZMapSkipList delete;

	if(!skip_list)
		return;
	if(skip_list->down)
		zMapSkipListDestroy(skip_list->down,free_func);

	while(skip_list)
	{
		if(!skip_list->down && free_func)	/* skip list stack all points down to the same data */
			free_func(skip_list->data);
		delete = skip_list;
		skip_list = (ZMapSkipList) skip_list->next;
		freeSkipList(delete);
	}


//	printf("skip list: %ld %ld %ld\n", n_block_alloc, n_skip_alloc, n_skip_free);
}

