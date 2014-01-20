/*  File: zmapSkipList.h
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
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * a simple implementation of a skip list data structure
 * NOTE we expect the lists to be static except exceptionally
 * and the module is designed with this in mind
 * used initially by zmapWindowGraphDensityItem.c
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_SKIP_LIST_H
#define ZMAP_WINDOW_SKIP_LIST_H

#include <glib.h>


/*
	Each layer is a doubly linked list with the data element pointing to the real data
	and a downwards pointer to the layer below
	note that upward pointers are one to one relation and are often NULL
	We don't use GLists as the insert functions are a bit iffy and also require heads

	we are aiming for fast random search and fast sequential access
	insert/delete are less critical as we expect not to do this much

	to create:
		take a normal GList and sort it
		add skip layers on top till bored

	to find data:
		starting from the head test the data to see if it's before or beyond what's required
		jump to next level down when current list can get no closer

	to add a node:
		find the place and add the data in the base GList
		adjust layers upwards
		add nodes if #links is above a threshold

	to delete a node:
		find it and remove
		adjust layers upwards
		remove nodes if #links is below a threshold (count these on the way down)

	NOTE an oddity of the first implementation is that we can Add data
	before the head key but it can still be found.
	NOTE upper layer balancing is not implemented, excessive use of add and remove will
	make the data structure degenerate

*/


typedef struct _zmapSkipList *ZMapSkipList;

typedef struct _zmapSkipList
{
	ZMapSkipList next,prev;	/* fwd, bwd on this layer */
	ZMapSkipList down,up;	/* nearest layers, down always exists, up only if list is stacked */
	gpointer data;		/* points to the real data from each layer */

} zmapSkipListStruct;


typedef void (*ZMapSkipListFreeFunc) (gpointer data);

ZMapSkipList zMapSkipListCreate(GList *data_in, GCompareFunc cmp);
ZMapSkipList zMapSkipListFirst(ZMapSkipList head);
ZMapSkipList zMapSkipListLast(ZMapSkipList head);
ZMapSkipList zMapSkipListFind(ZMapSkipList head, GCompareFunc cmp, gconstpointer key);
ZMapSkipList zMapSkipListAdd(ZMapSkipList head, GCompareFunc cmp, gpointer key);
ZMapSkipList zMapSkipListRemove(ZMapSkipList sl, ZMapSkipListFreeFunc free_func);
void zMapSkipListDestroy(ZMapSkipList skip_list, ZMapSkipListFreeFunc free_func);

int zMapSkipListCount(ZMapSkipList head);

#define SKIP_LIST_CLASS	0

#if SKIP_LIST_CLASS
/*
	if more generality is needed try something like this, and adjust all the functions:
 */

typedef struct _zmapSkipListClass
{
	ZmapSkipList head;
	ZMapSkipListCmpFunc cmp;
	int min_skip;
	int max_skip;
	int norm_skip;

} zmapSkipListClassStruct, *ZMapSkipListClass;

#else
/* avoiding premature generalistaion .... */

/*
   hard coded thresholds for add/delete
   this stuff is not critical!
*/

#define SKIP_LIST_MIN_SKIP	4
#define SKIP_LIST_NORM_SKIP	10
#define SKIP_LIST_MAX_SKIP	16
#endif


#endif
