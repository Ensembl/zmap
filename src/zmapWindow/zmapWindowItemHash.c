/*  File: zmapWindowItemHash.c
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
 * Description: Functions to go from an alignment, block, column or feature
 *              in a feature context to the corresponding foocanvas group or item.
 *              This is fundamental in linking our feature context hierachy
 *              to the foocanvas item hierachy.
 *
 * Exported functions: See zMapWindow_P.h
 * HISTORY:
 * Last edited: Feb 17 13:58 2006 (edgrif)
 * Created: Mon Jun 13 10:06:49 2005 (edgrif)
 * CVS info:   $Id: zmapWindowItemHash.c,v 1.16 2006-02-17 14:00:17 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>



/* Used to hold coord information + return a result the child search callback function. */
typedef struct
{
  int child_start, child_end ;
  FooCanvasItem *child_item ;
} ChildSearchStruct, *ChildSearch ;




/* We store ids with the group or item that represents them in the canvas.
 * May want to consider more efficient way of storing these than malloc... */
typedef struct
{
  FooCanvasItem *item ;					    /* could be group or item. */

  GHashTable *hash_table ;
} ID2CanvasStruct, *ID2Canvas ;



/* forward declaration, needed for function prototype. */
typedef struct ItemSearchStruct_ *ItemSearch ;


/* Callback function that takes the given item and checks to see if it satisfies a particular
 * predicate. */
typedef gboolean (*ZMapWindowItemValidFunc)(FooCanvasItem *item, 
					    ItemSearch curr_search, gpointer user_data) ;

/* n.b. sometimes we will want the search thing to be a quark, sometimes a string,
 * depending whether the given string is an exact name or a regexp...hmmmm
 * may need to change this to allow both.... */
/* This structure contains the id to be searched for + optionally a precedent function that
 * will test the item for validity (e.g. strand). */
typedef struct ItemSearchStruct_
{
  GQuark search_quark ;					    /* Specifies search string. */

  ZMapWindowItemValidFunc valid_func ;			    /* Function to check items for validity. */

  /* UGH not happy with this, rework this when there is time... */
  GList **results ;					    /* result of search. */

} ItemSearchStruct ;


typedef struct
{
  GList *search ;					    /* List of ItemSearch specifying search. */

  GList **results ;					    /* result of search. */

} ItemListSearchStruct, *ItemListSearch ;





/* SOMETHING I NEED TO ADD HERE IS FORWARD/REVERSE COLUMN STUFF..... */


static void destroyIDHash(gpointer data) ;
static void doHashSet(GHashTable *hash_table, GList *search, GList **result) ;
static void searchItemHash(gpointer key, gpointer value, gpointer user_data) ;
static void addItem(gpointer key, gpointer value, gpointer user_data) ;
static void childSearchCB(gpointer data, gpointer user_data) ;
static GQuark rootCanvasID(void);

static void printGlist(gpointer data, gpointer user_data) ;


gboolean filterOnForwardStrand(FooCanvasItem *item, ItemSearch curr_search, gpointer user_data) ;
gboolean filterOnReverseStrand(FooCanvasItem *item, ItemSearch curr_search, gpointer user_data) ;
gboolean filterOnStrand(FooCanvasItem *item, ZMapStrand strand) ;
gboolean isRegExp(GQuark id) ;
gboolean filterOnRegExp(FooCanvasItem *item, ItemSearch curr_search, gpointer user_data) ;



 
/* Create the table that hashes feature set ids to hash tables of features.
 * NOTE that the glib hash stuff does not store anything except the pointers to the
 * keys and values which is a pain if you are only hashing on ints....as I am.
 * I've therefore decided to try using their "direct hash" stuff to only work on
 * the pointers... */
GHashTable *zmapWindowFToICreate(void)
{
  GHashTable *feature_to_context ;

  feature_to_context = g_hash_table_new_full(NULL,	    /* Uses direct hash of keys. */
					     NULL,	    /* Uses direct comparison of key values. */
					     NULL,	    /* Nothing to destroy for hash key. */
					     destroyIDHash) ;

  return feature_to_context ;
}


/* We need to make special ids for the forward/reverse column groups as these don't exist
 * in the features.... */
GQuark zmapWindowFToIMakeSetID(GQuark set_id, ZMapStrand strand)
{
  GQuark strand_set_id = 0 ;
  char *set_strand ;
  char *strand_str ;
  
  if (strand == ZMAPSTRAND_NONE || strand == ZMAPSTRAND_FORWARD)
    set_strand= "FORWARD" ;
  else
    set_strand= "REVERSE" ;

  strand_str = g_strdup_printf("%s_%s", g_quark_to_string(set_id), set_strand) ;

  strand_set_id = g_quark_from_string(strand_str) ;

  g_free(strand_str) ;

  return strand_set_id ;
}

gboolean zmapWindowFToIAddRoot(GHashTable *feature_to_context_hash,
                               FooCanvasGroup *root_group)
{
  gboolean result = TRUE ;
  GQuark root_id  = 0;

  root_id = rootCanvasID();

  if (!(g_hash_table_lookup(feature_to_context_hash, GUINT_TO_POINTER(root_id))))
    {
      ID2Canvas root ;

      root = g_new0(ID2CanvasStruct, 1) ;
      root->item = FOO_CANVAS_ITEM(root_group) ;
      root->hash_table = g_hash_table_new_full(NULL, NULL, NULL, destroyIDHash) ;

      g_hash_table_insert(feature_to_context_hash, GUINT_TO_POINTER(root_id), root) ;
    }

  return result ;
}

/* Add a hash for the given alignment. The alignment may already exist if we are merging
 * more data from another server, if so then no action is taken otherwise we would lose
 * all our existing feature hashes !
 * NOTE: always returns TRUE as either the align is already there or we add it. */
gboolean zmapWindowFToIAddAlign(GHashTable *feature_to_context_hash,
				GQuark align_id,
				FooCanvasGroup *align_group)
{
  gboolean result = TRUE ;

  if (!(g_hash_table_lookup(feature_to_context_hash, GUINT_TO_POINTER(align_id))))
    {
      ID2Canvas align ;

      align = g_new0(ID2CanvasStruct, 1) ;
      align->item = FOO_CANVAS_ITEM(align_group) ;
      align->hash_table = g_hash_table_new_full(NULL, NULL, NULL, destroyIDHash) ;

      g_hash_table_insert(feature_to_context_hash, GUINT_TO_POINTER(align_id), align) ;
    }

  return result ;
}


/* Add a hash for the given block within an alignment. The block may already exist if we are merging
 * more data from another server, if so then no action is taken otherwise we would lose
 * all our existing feature hashes !
 * Returns FALSE if the align_id cannot be found, otherwise it returns TRUE as the block_id hash
 * already exists or we add it. */
gboolean zmapWindowFToIAddBlock(GHashTable *feature_to_context_hash,
				GQuark align_id, GQuark block_id,
				FooCanvasGroup *block_group)
{
  gboolean result = FALSE ;
  ID2Canvas align ;

  if ((align = (ID2Canvas)g_hash_table_lookup(feature_to_context_hash,
					      GUINT_TO_POINTER(align_id))))
    {
      if (!(g_hash_table_lookup(align->hash_table, GUINT_TO_POINTER(block_id))))
	{
	  ID2Canvas block ;

	  block = g_new0(ID2CanvasStruct, 1) ;
	  block->item = FOO_CANVAS_ITEM(block_group) ;
	  block->hash_table = g_hash_table_new_full(NULL, NULL, NULL, destroyIDHash) ;

	  g_hash_table_insert(align->hash_table, GUINT_TO_POINTER(block_id), block) ;
	}

      result = TRUE ;
    }

  return result ;
}



/* Add a hash for the given feature set within a block within an alignment.
 * The set may already exist if we are merging more data from another server,
 * if so then no action is taken otherwise we would lose all our existing feature hashes !
 * Returns FALSE if the align_id or block_id cannot be found, otherwise it returns TRUE
 * as the block_id hash already exists or we add it. */
gboolean zmapWindowFToIAddSet(GHashTable *feature_to_context_hash,
			      GQuark align_id, GQuark block_id, GQuark set_id,
			      ZMapStrand set_strand,
			      FooCanvasGroup *set_group)
{
  gboolean result = FALSE ;
  ID2Canvas align ;
  ID2Canvas block ;

  /* We need special quarks that incorporate strand indication as the hashes are per column. */
  set_id = zmapWindowFToIMakeSetID(set_id, set_strand) ;

  if ((align = (ID2Canvas)g_hash_table_lookup(feature_to_context_hash,
					      GUINT_TO_POINTER(align_id)))
      && (block = (ID2Canvas)g_hash_table_lookup(align->hash_table,
						 GUINT_TO_POINTER(block_id))))
    {
      if (!(g_hash_table_lookup(block->hash_table, GUINT_TO_POINTER(set_id))))
	{
	  ID2Canvas set ;

	  set = g_new0(ID2CanvasStruct, 1) ;
	  set->item = FOO_CANVAS_ITEM(set_group) ;
	  set->hash_table = g_hash_table_new_full(NULL, NULL, NULL, destroyIDHash) ;

	  g_hash_table_insert(block->hash_table, GUINT_TO_POINTER(set_id), set) ;
	}

      result = TRUE ;
    }

  return result ;
}


/* Note that this routine is different in that the feature does not have a hash table associated
 * with it, the table pointer is set to NULL.
 * 
 * Returns FALSE if the align_id, block_id or set_id cannot be found, otherwise it returns TRUE
 * if the feature_id hash already exists or we add it. */
gboolean zmapWindowFToIAddFeature(GHashTable *feature_to_context_hash,
				  GQuark align_id, GQuark block_id, GQuark set_id,
				  GQuark feature_id, FooCanvasItem *feature_item)
{
  gboolean result = FALSE ;
  ID2Canvas align ;
  ID2Canvas block ;
  ID2Canvas set ;

  if ((align = (ID2Canvas)g_hash_table_lookup(feature_to_context_hash,
					      GUINT_TO_POINTER(align_id)))
      && (block = (ID2Canvas)g_hash_table_lookup(align->hash_table,
						 GUINT_TO_POINTER(block_id)))
      && (set = (ID2Canvas)g_hash_table_lookup(block->hash_table,
					       GUINT_TO_POINTER(set_id))))
    {
      if (!(g_hash_table_lookup(set->hash_table, GUINT_TO_POINTER(feature_id))))
	{
	  ID2Canvas feature ;

	  feature = g_new0(ID2CanvasStruct, 1) ;
	  feature->item = feature_item ;
	  feature->hash_table = g_hash_table_new_full(NULL, NULL, NULL, destroyIDHash) ;

	  g_hash_table_insert(set->hash_table, GUINT_TO_POINTER(feature_id), feature) ;
	}

      result = TRUE ;
    }

  return result ;
}


/* Remove functions...not quite as simple as they seem, because removing an item may imply
 * removing its hash table and the hash table below that and the hash table below that.... */



/* Remove a hash for the given feature set.
 * The set may already exist if we are merging more data from another server,
 * if so then no action is taken otherwise we would lose all our existing feature hashes !
 * Returns FALSE if the set_id cannot be found, otherwise it returns TRUE and removes
 * the set_id and its hash tables. */
gboolean zmapWindowFToIRemoveSet(GHashTable *feature_to_context_hash,
				 GQuark align_id, GQuark block_id, GQuark set_id,
				 ZMapStrand set_strand)
{
  gboolean result = FALSE ;
  ID2Canvas align ;
  ID2Canvas block ;
  ID2Canvas set ;

  /* We need special quarks that incorporate strand indication as the hashes are per column. */
  set_id = zmapWindowFToIMakeSetID(set_id, set_strand) ;

  if ((align = (ID2Canvas)g_hash_table_lookup(feature_to_context_hash,
					      GUINT_TO_POINTER(align_id)))
      && (block = (ID2Canvas)g_hash_table_lookup(align->hash_table,
						 GUINT_TO_POINTER(block_id)))
      && (set = (ID2Canvas)g_hash_table_lookup(block->hash_table,
					       GUINT_TO_POINTER(set_id))))
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* I DON'T THINK THIS IS NEEDED, GETS TRIGGERED BY THE REMOVE BELOW...BUT I BET THIS
       * DOESN'T WORK IF YOU DO IT UP AT SAY THE ALIGN LEVEL, I DON'T THINK IT WILL CASCADE
       * DOWN PROPERLY....PERHAPS TIME FOR A TEST.... */
      if (set->hash_table)
	g_hash_table_destroy(set->hash_table) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      result = g_hash_table_remove(block->hash_table, GUINT_TO_POINTER(set_id)) ;
      zMapAssert(result == TRUE) ;
    }

  return result ;
}



/* Remove a hash for the given feature.
 * Returns FALSE if the feature_id cannot be found, otherwise it returns TRUE and removes
 * the feature_id. */
gboolean zmapWindowFToIRemoveFeature(GHashTable *feature_to_context_hash,
				     GQuark align_id, GQuark block_id, GQuark set_id,
				     ZMapStrand set_strand, GQuark feature_id)
{
  gboolean result = FALSE ;
  ID2Canvas align ;
  ID2Canvas block ;
  ID2Canvas set ;
  ID2Canvas feature ;


  /* We need special quarks that incorporate strand indication as the hashes are per column. */
  set_id = zmapWindowFToIMakeSetID(set_id, set_strand) ;

  if ((align = (ID2Canvas)g_hash_table_lookup(feature_to_context_hash,
					      GUINT_TO_POINTER(align_id)))
      && (block = (ID2Canvas)g_hash_table_lookup(align->hash_table,
						 GUINT_TO_POINTER(block_id)))
      && (set = (ID2Canvas)g_hash_table_lookup(block->hash_table,
					       GUINT_TO_POINTER(set_id)))
      && (feature = (ID2Canvas)g_hash_table_lookup(set->hash_table,
						   GUINT_TO_POINTER(feature_id))))
    {
      result = g_hash_table_remove(set->hash_table, GUINT_TO_POINTER(feature_id)) ;
      zMapAssert(result == TRUE) ;
    }

  return result ;
}




/* RENAME TO FINDFEATUREITEM.... */
FooCanvasItem *zmapWindowFToIFindFeatureItem(GHashTable *feature_to_context_hash,
                                             ZMapFeature feature)
{
  FooCanvasItem *item = NULL ;

  item = zmapWindowFToIFindItemFull(feature_to_context_hash,
				    feature->parent->parent->parent->unique_id,
				    feature->parent->parent->unique_id,
				    feature->parent->unique_id,
				    feature->strand,
				    feature->unique_id) ;

  return item ;
}


FooCanvasItem *zmapWindowFToIFindSetItem(GHashTable *feature_to_context_hash,
					 ZMapFeatureSet feature_set, ZMapStrand strand)
{
  FooCanvasItem *item = NULL ;

  item = zmapWindowFToIFindItemFull(feature_to_context_hash,
				    feature_set->parent->parent->unique_id,
				    feature_set->parent->unique_id,
				    feature_set->unique_id,
				    strand,
				    0) ;

  return item ;
}



/* Use this function to find the single Foo canvas item/group corresponding to
 * the supplied ids, returns NULL if the any of the id(s) could not be found.
 * 
 * Which hash tables are searched is decided by the ids supplied:
 * 
 *   align     block      set     feature
 * 
 *     id        0         < not read >         returns the alignment item
 * 
 *     id        id        0     <not read>     returns the block item within the alignment
 * 
 * etc.
 * 
 * Warning, may return null so result MUST BE TESTED by caller.
 *  */
FooCanvasItem *zmapWindowFToIFindItemFull(GHashTable *feature_to_context_hash,
					  GQuark align_id, GQuark block_id, GQuark set_id,
					  ZMapStrand strand,
					  GQuark feature_id)
{
  FooCanvasItem *item = NULL ;
  ID2Canvas root ;
  ID2Canvas align ;
  ID2Canvas block ;
  ID2Canvas set ;
  ID2Canvas feature ;

  /* Required for minimum query. */
  zMapAssert(feature_to_context_hash) ; /* && align_id */

  /* Cascade down through the hashes until we reach the point the caller wants to stop at. */
  if (align_id && (align = (ID2Canvas)g_hash_table_lookup(feature_to_context_hash,
                                                          GUINT_TO_POINTER(align_id))))
    {
      if (!block_id)
	{
	  item = FOO_CANVAS_ITEM(align->item) ;
	}
      else if ((block = (ID2Canvas)g_hash_table_lookup(align->hash_table,
						       GUINT_TO_POINTER(block_id))))
	{
	  GQuark tmp_set_id = zmapWindowFToIMakeSetID(set_id, strand) ;

	  if (!set_id)
	    {
	      item = FOO_CANVAS_ITEM(block->item) ;
	    }
	  else if ((set = (ID2Canvas)g_hash_table_lookup(block->hash_table,
							 GUINT_TO_POINTER(tmp_set_id))))
	    {
	      if (!feature_id)
		{
		  item = FOO_CANVAS_ITEM(set->item) ;
		}
	      else
		{
		  feature = (ID2Canvas)g_hash_table_lookup(set->hash_table,
							   GUINT_TO_POINTER(feature_id)) ;
		  item = feature->item ;
		}
	    }
	}
    }
  else
    {
      /* We just want the root_group of our columns. Not the canvas
       * root_group, get that by foo_canvas_root(canvas)! */
      /* The best check that we are actually asking for the root. */
      zMapAssert(!block_id && !set_id && !feature_id); // "Warning: You are asking for the root_group.\n"

      if((root = (ID2Canvas)g_hash_table_lookup(feature_to_context_hash,
                                                GUINT_TO_POINTER(rootCanvasID()))))
        item = FOO_CANVAS_ITEM(root->item); /* This is actually a group. */
    }

  return item ;
}



/* Find the child item that matches the supplied start/end, use for finding feature items
 * that are part of a compound feature, e.g. exons/introns in a transcript.
 * Warning, may return null so result MUST BE TESTED by caller. */
FooCanvasItem *zmapWindowFToIFindItemChild(GHashTable *feature_to_context_hash,
					   ZMapFeature feature,
					   int child_start, int child_end)
{
  FooCanvasItem *item = NULL ;

  /* If the returned item is not a compound item then return NULL, otherwise look for the correct
   * child using the start/end. */
  if ((item = zmapWindowFToIFindFeatureItem(feature_to_context_hash, feature))
      && FOO_IS_CANVAS_GROUP(item))
    {
      FooCanvasGroup *group = FOO_CANVAS_GROUP(item) ;
      ChildSearchStruct child_search = {0} ;

      child_search.child_start = child_start ;
      child_search.child_end = child_end ;

      g_list_foreach(group->item_list, childSearchCB, (void *)&child_search) ;

      if (child_search.child_item)
	item = child_search.child_item ;
      else
	item = NULL;
    }

  return item ;
}



/* Use this function to find the _set_ of Foo canvas item/group corresponding to
 * the supplied ids. Returns a GList of the Foo canvas item/groups or
 * NULL if the id(s) could not be found.
 * 
 * Which hash tables are searched is decided by the ids supplied, "*" acts as the
 * the wild card and 0 acts as search limiter.
 * 
 *   align     block      set     feature
 * 
 *     "*"       0        <  not  read  >     returns all the alignment items
 * 
 *     id       "*"        0     <not read>   returns all block items within the alignment
 * 
 *     "*"      id        "*"       0         returns all sets in the block "id" in all alignments.
 * 
 * etc.
 * 
 * Warning, may return null so result MUST BE TESTED by caller.
 * 
 * NOTE that strand_spec is restricted to the values:
 * 
 *                 "."  =  ZMAPSTRAND_NONE
 *                 "+"  =  ZMAPSTRAND_FORWARD
 *                 "-"  =  ZMAPSTRAND_REVERSE
 *                 "*"  =  <ZMAPSTRAND_FORWARD && ZMAPSTRAND_REVERSE>
 * 
 *  */
GList *zmapWindowFToIFindItemSetFull(GHashTable *feature_to_context_hash,
				     GQuark align_id, GQuark block_id, GQuark set_id,
				     char *strand_spec,
				     GQuark feature_id)
{
  GList *result = NULL ;
  GQuark strand_id, strand_none, strand_forward, strand_reverse, strand_both ;
  GQuark forward_set_id = 0, reverse_set_id = 0 ;
  GList *search = NULL ;
  ItemSearchStruct align_search = {0}, block_search = {0},
	 forward_set_search = {0}, reverse_set_search = {0},
         feature_search = {0}, terminal_search = {0} ;

  /* Required for minimum query. */
  zMapAssert(feature_to_context_hash && align_id) ;


  /* There is some muckiness here for strand stuff...strands cannot just be done as a filter
   * because we need to be able to _find_ the reverse or forward groups in the hash tables and
   * they cannot both be hashed to the same set_id...aaaaaaaaghhhhhhhh.......
   * For now I am doing the hash stuff _twice_ if both strands are required.... */

  /* A small problemeto is that the GQuark interface does not allow the removal of strings
   * so our routine here will pile up strings in the quark hash table which is global, if
   * this turns out to be a problem we could provide our own quark table implementation... */

  align_search.search_quark = align_id ;
  if (align_id && isRegExp(align_id))
    align_search.valid_func = filterOnRegExp ;

  block_search.search_quark = block_id ;
  if (block_id && isRegExp(block_id))
    block_search.valid_func = filterOnRegExp ;


  if (set_id)
    {
      /* convert strand spec to something useful. */
      strand_id = 0 ;
      if (strand_spec)
	{
	  strand_none = g_quark_from_string(".") ;
	  strand_forward = g_quark_from_string("+") ;
	  strand_reverse = g_quark_from_string("-") ;
	  strand_both = g_quark_from_string("*") ;
	  
	  strand_id = g_quark_from_string(strand_spec) ;
	  zMapAssert(strand_id == strand_none || strand_id == strand_forward
		     || strand_id == strand_reverse || strand_id == strand_both) ;
	}

      if (strand_id == strand_none || strand_id == strand_forward || strand_id == strand_both)
	{
	  forward_set_id = zmapWindowFToIMakeSetID(set_id, ZMAPSTRAND_FORWARD) ;
	  forward_set_search.search_quark = forward_set_id ;
	  if (isRegExp(set_id))
	    forward_set_search.valid_func = filterOnRegExp ;	      
	}

      if (strand_id == strand_reverse || strand_id == strand_both)
	{
	  reverse_set_id = zmapWindowFToIMakeSetID(set_id, ZMAPSTRAND_REVERSE) ;
	  reverse_set_search.search_quark = reverse_set_id ;
	  if (isRegExp(set_id))
	    reverse_set_search.valid_func = filterOnRegExp ;
	}
    }

  feature_search.search_quark = feature_id ;
  if (feature_id && isRegExp(feature_id))
    feature_search.valid_func = filterOnRegExp ;


  /* build the search list (terminal stop is needed to halt the search if none of the given
   * parameters is a stop. */
  if (!set_id
      || (strand_id == strand_none || strand_id == strand_forward || strand_id == strand_both))
    {
      search = NULL ;

      search = g_list_append(search, &align_search) ;
      search = g_list_append(search, &block_search) ;
      search = g_list_append(search, &forward_set_search) ;
      search = g_list_append(search, &feature_search) ;
      search = g_list_append(search, &terminal_search) ;	    /* Terminal stop. */

      /* Now do the recursive search */
      doHashSet(feature_to_context_hash, search, &result) ;

      g_list_free(search) ;
      search = NULL ;
    }

  /* No point in doing a second search unless we are going to search as far down as the reverse
   * strand sets. */
  if (set_id && (strand_id == strand_reverse || strand_id == strand_both))
    {
      search = NULL ;

      search = g_list_append(search, &align_search) ;
      search = g_list_append(search, &block_search) ;
      search = g_list_append(search, &reverse_set_search) ;
      search = g_list_append(search, &feature_search) ;
      search = g_list_append(search, &terminal_search) ;	    /* Terminal stop. */

      /* Now do the recursive search */
      doHashSet(feature_to_context_hash, search, &result) ;

      g_list_free(search) ;
      search = NULL ;
    }

  return result ;
}



FooCanvasItem *zMapWindowFindFeatureItemByQuery(ZMapWindow window, ZMapWindowFeatureQuery ftq)
{
  FooCanvasItem *item  = NULL;
  GQuark feature_id ;

  /* Do a (hash, '.', '.', 'style', 'feature') lookup */
  /* Check we won't fail any Asserts!!!!
   * type >= 0 (!= ZMAPFEATURE_INVALID)
   * feature name != NULL
   * start >= 1
   * start <= end
   */
  if(ftq->ft_name && ftq->type != ZMAPFEATURE_INVALID
     && ftq->strand_set  && ftq->start >= 1
     && ftq->start <= ftq->end
     )
    {
      GQuark style_id ;

      style_id = zMapStyleCreateID(ftq->style) ;


      feature_id = zMapFeatureCreateID(ftq->type, ftq->ft_name, ftq->strand, 
                                       ftq->start, ftq->end,
                                       ftq->query_start, ftq->query_end
                                       );
      if(!ftq->alignment)
        ftq->alignment = g_strdup_printf("%s_master", window->sequence);

      if(!ftq->block)
        {
          int start, end;
          start = window->seq_start;
          end   = (window->seq_end == window->seqLength ? 0 : window->seq_end);
          /* This next bit is backward in coming forward... */
          ftq->block = g_strdup(g_quark_to_string(zMapFeatureBlockCreateID(start, end, ZMAPSTRAND_FORWARD,
                                                                           start, end, ZMAPSTRAND_FORWARD)));
        }

      item = zmapWindowFToIFindItemFull(window->context_to_item,
                                        g_quark_from_string(ftq->alignment),
                                        g_quark_from_string(ftq->block),
                                        style_id,
					ftq->strand,
                                        feature_id);
      /* MUST CHECK item != NULL users send rubbish! */
    }

  return item;
}


/* This code can be used to test the zmapWindowFToIFindItemSetFull() function but also
 * shows how to parse the list returned by that function. */
void zmapWindowFToIPrintList(GList *item_list)
{
  zMapAssert(item_list) ;

  g_list_foreach(item_list, printGlist, NULL) ;

  return ;
}



/* Destroy the feature context hash, this will cause all the hash tables/datastructs that it
 * references to be destroyed as well via the hash table destroy function. */
void zmapWindowFToIDestroy(GHashTable *feature_to_context_hash)
{
  g_hash_table_destroy(feature_to_context_hash) ;
  
  return ;
}





/* 
 *                      Internal routines.
 */




/* Is called for all entries in the hash table of feature set ids -> hash table of features. For
 * each one we need to destroy the feature hash table and then free the struct itself. */
static void destroyIDHash(gpointer data)
{
  ID2Canvas id = (ID2Canvas)data ;

  g_hash_table_destroy(id->hash_table) ;

  g_free(id) ;

  return ;
}



/* This is a g_datalist callback function. */
static void childSearchCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ChildSearch child_search = (ChildSearch)user_data ;
  ZMapWindowItemFeatureType item_feature_type ;

  /* We take the first match we find so this function does nothing if we have already
   * found matching child item. */
  if (!(child_search->child_item))
    {
      item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							    "item_feature_type")) ;
      if (item_feature_type == ITEM_FEATURE_CHILD)
	{
	  ZMapWindowItemFeature item_subfeature_data ;

	  item_subfeature_data = (ZMapWindowItemFeature)g_object_get_data(G_OBJECT(item),
									  "item_subfeature_data") ;

	  if (item_subfeature_data->start == child_search->child_start
	      && item_subfeature_data->end == child_search->child_end)
	    child_search->child_item = item ;
	}
    }

  return ;
}



/* 
 *          These functions are used by  zmapWindowFToIFindItemSetFull()
 *          to recursively search our hash tables.
 */

/* This function gets called recursively, either directly or via a 'foreach' function
 * call for a hash table.
 * 
 * Note the common pattern in the "stop" and "continue" sections below of either
 * operating on all items in the current hash table or just the requested item in
 * current hash.
 *  */
static void doHashSet(GHashTable *hash_table, GList *search, GList **results_inout)
{
  ItemSearch curr_search, next_search ;
  GQuark curr_search_id, next_search_id ;
  GQuark wild_card ;
  GQuark stop = 0 ;
  GList *results = *results_inout ;
  ID2Canvas item_id ;

  wild_card = g_quark_from_string("*") ;

  curr_search = (ItemSearch)(search->data) ; 
  next_search = (ItemSearch)(search->next->data) ;

  curr_search_id = curr_search->search_quark ; 
  next_search_id = next_search->search_quark ;

  /* I think this can't happen, we should stop recursing _before_ the current search becomes
   * stop. */
  zMapAssert(curr_search_id != stop) ;


  /* BAD LOGIC BELOW, NEED TO SORT OUT REGEXP VALIDATION FROM SAY POSITION VALIDATION... */

  if (next_search_id == stop)
    {
      /* If we've reached the end it's time to actually add items to the list. */

      if ((item_id = (ID2Canvas)g_hash_table_lookup(hash_table,
						    GUINT_TO_POINTER(curr_search_id))))
	{
	  if (!curr_search->valid_func || curr_search->valid_func(item_id->item, curr_search, NULL))
	    results = g_list_append(results, item_id->item) ;
	}
      else if (curr_search_id == wild_card || curr_search->valid_func)
	{
	  curr_search->results = &results ;

	  g_hash_table_foreach(hash_table, addItem, (gpointer)curr_search) ;
	}
    }
  else
    {
      /* We haven't reached the end of the search yet so carry searching down the hashes. */
      ItemListSearchStruct search_data ;

      /* bump on to next search pattern. */
      search = g_list_next(search) ;

      search_data.search = search ;
      search_data.results = &results ;

      if ((item_id = (ID2Canvas)g_hash_table_lookup(hash_table,
						    GUINT_TO_POINTER(curr_search_id))))
	{
	  doHashSet(item_id->hash_table, search_data.search, search_data.results) ;
	}

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

      /* I THINK THIS PROBABLY JUST SHOULDN'T BE HERE....IF WE DON'T FIND SOMETHING WE SHOULD STOP
       * this is probably left over from when I was just experimenting.... */

      else
	{
	  g_hash_table_foreach(hash_table, searchItemHash, (gpointer)&search_data) ;
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  *results_inout = results ;

  return ;
}


/* A GHFunc() callback to add the foocanvas item stored in this item in the parent hash
 * to a list of foocanvas items. */
static void addItem(gpointer key, gpointer value, gpointer user_data)
{
  ID2Canvas hash_item = (ID2Canvas)value ;
  ItemSearch curr_search = (ItemSearch)user_data ;
  GList **results = curr_search->results ;

  if (!curr_search->valid_func || curr_search->valid_func(hash_item->item, curr_search, key))
    *results = g_list_append(*results, hash_item->item) ;

  return ;
}


/* A GHFunc() callback which calls doHashSet() to search this items own hash table. */
static void searchItemHash(gpointer key, gpointer value, gpointer user_data)
{
  ID2Canvas hash_item = (ID2Canvas)value ;
  ItemListSearch search = (ItemListSearch)user_data ;

  doHashSet(hash_item->hash_table, search->search, search->results) ;

  return ;
}


/* Prints out names/types of foocanvas items in a list..... */
static void printGlist(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ZMapFeatureAny feature_any ;
  char *feature_type =  NULL ;
  GQuark feature_id = 0 ;

  if ((feature_any = (ZMapFeatureAny)(g_object_get_data(G_OBJECT(item), "item_feature_data"))))
    {
      switch (feature_any->struct_type)
	{
	case ZMAPFEATURE_STRUCT_ALIGN:
	  feature_type = "Align" ;
	  break ;
	case ZMAPFEATURE_STRUCT_BLOCK:
	  feature_type = "Align" ;
	  break ;
	case ZMAPFEATURE_STRUCT_FEATURESET:
	  feature_type = "Set" ;
	  break ;
	case ZMAPFEATURE_STRUCT_FEATURE:
	  feature_type = "Feature" ;
	  break ;
	default:
	  {
	    zMapLogFatal("Coding error, bad ZMapWindowItemFeatureType: %d", feature_any->struct_type) ;
	    break ;
	  }
	}

      feature_id = feature_any->unique_id ;
    }

  printf("%s:  %s\n",
	 (feature_type ? feature_type : "<no feature data attached>"),
	 (feature_id ? g_quark_to_string(feature_id) : "")) ;

  return ;
}



gboolean filterOnForwardStrand(FooCanvasItem *item, ItemSearch curr_search, gpointer user_data)
{
  gboolean result ;

  result = filterOnStrand(item, ZMAPSTRAND_FORWARD) ;

  return result ;
}


gboolean filterOnReverseStrand(FooCanvasItem *item, ItemSearch curr_search, gpointer user_data)
{
  gboolean result ;

  result = filterOnStrand(item, ZMAPSTRAND_REVERSE) ;

  return result ;
}

gboolean filterOnStrand(FooCanvasItem *item, ZMapStrand strand)
{
  gboolean result = FALSE ;
  ZMapFeature feature ;

  feature = (ZMapFeature)g_object_get_data(G_OBJECT(item), "item_feature_data") ;
  zMapAssert(feature) ;

  if (feature->strand == strand)
    result = TRUE ;

  return result ;
}


/* Check to see if the string represented by the quark id contains regexp chars,
 * currently this means "*" or "?". */
gboolean isRegExp(GQuark id)
{
  gboolean result= FALSE ;
  char *str ;

  str = (char *)g_quark_to_string(id) ;

  if (strstr(str, "*") || strstr(str, "?"))
    result = TRUE ;

  return result ;
}


/* We will need a user_data param to hold the compiled string for efficiency....
 * 
 * OK, I think we need to filter on the string that was used to hash this item,
 * _not_ the unique id of the item. This is because some items are hashed using
 * a string _constructed_ from their unique id, e.g. most obviously strand specific
 * stuff.
 * 
 *  */
gboolean filterOnRegExp(FooCanvasItem *item, ItemSearch curr_search, gpointer user_data)
{
  gboolean result = FALSE ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapFeature feature ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  char *pattern, *key_string ;
  GQuark key_id = GPOINTER_TO_INT(user_data) ;

  key_string = (char *)g_quark_to_string(key_id) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Not needed for this compare but will be in other filter callbacks... */
  feature = (ZMapFeature)g_object_get_data(G_OBJECT(item), "item_feature_data") ;
  zMapAssert(feature) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  pattern = (char *)g_quark_to_string(curr_search->search_quark) ;

  /* Compare pattern to hash key string, _not_ features unique_id string. */
  if (g_pattern_match_simple(pattern, key_string))
    result = TRUE ;

  return result ;
}

static GQuark rootCanvasID(void)
{
  return g_quark_from_string("this should be completely random and unique and nowhere else in the hash");
}
