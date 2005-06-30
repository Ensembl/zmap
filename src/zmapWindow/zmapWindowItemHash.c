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
 * Last edited: Jun 30 16:16 2005 (rds)
 * Created: Mon Jun 13 10:06:49 2005 (edgrif)
 * CVS info:   $Id: zmapWindowItemHash.c,v 1.3 2005-06-30 15:20:06 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>


/* The plan is to expand the functions here to provide a kind of querying service so that
 * we can look for all features in a column for instance. */


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
  union
  {
    FooCanvasGroup *group ;
    FooCanvasItem *item ;
  } canvas ;

  GHashTable *hash_table ;
} ID2CanvasStruct, *ID2Canvas ;




/* SOMETHING I NEED TO ADD HERE IS FORWARD/REVERSE COLUMN STUFF..... */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gboolean equalIDHash(gconstpointer a, gconstpointer b) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void destroyIDHash(gpointer data) ;

static void childSearchCB(gpointer data, gpointer user_data) ;


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
      align->canvas.group = align_group ;
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
	  block->canvas.group = block_group ;
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
			      FooCanvasGroup *set_group)
{
  gboolean result = FALSE ;
  ID2Canvas align ;
  ID2Canvas block ;

  if ((align = (ID2Canvas)g_hash_table_lookup(feature_to_context_hash,
					      GUINT_TO_POINTER(align_id)))
      && (block = (ID2Canvas)g_hash_table_lookup(align->hash_table,
						 GUINT_TO_POINTER(block_id))))
    {
      if (!(g_hash_table_lookup(block->hash_table, GUINT_TO_POINTER(set_id))))
	{
	  ID2Canvas set ;

	  set = g_new0(ID2CanvasStruct, 1) ;
	  set->canvas.group = set_group ;

	  /* Actually, we probably don't need to have a destroy for the next level down...  */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  set->hash_table = g_hash_table_new_full(NULL, NULL, NULL, destroyIDHash) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	  set->hash_table = g_hash_table_new(NULL, NULL) ;

	  g_hash_table_insert(block->hash_table, GUINT_TO_POINTER(set_id), set) ;
	}

      result = TRUE ;
    }

  return result ;
}


/* Note that this routine is different in that we just insert the canvas item itself, there
 * is no need for a hash table at this level.
 * Returns FALSE if the align_id, block_id or set_id cannot be found, otherwise it returns TRUE
 * as the feature_id hash already exists or we add it. */
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
	  g_hash_table_insert(set->hash_table, GUINT_TO_POINTER(feature_id), feature_item) ;
	}

      result = TRUE ;
    }

  return result ;
}


/* RENAME TO FINDFEATUREITEM.... */
FooCanvasItem *zmapWindowFToIFindFeatureItem(GHashTable *feature_to_context_hash,
                                             ZMapFeature feature)
{
  FooCanvasItem *item = NULL ;
  GQuark column_id ;

  if (feature->strand == ZMAPSTRAND_FORWARD || feature->strand == ZMAPSTRAND_NONE)
    column_id = zmapWindowFToIMakeSetID(feature->parent_set->unique_id, ZMAPSTRAND_FORWARD) ;
  else
    column_id = zmapWindowFToIMakeSetID(feature->parent_set->unique_id, ZMAPSTRAND_REVERSE) ;


  item = zmapWindowFToIFindItemFull(feature_to_context_hash,
				    feature->parent_set->parent_block->parent_alignment->unique_id,
				    feature->parent_set->parent_block->unique_id,
				    column_id,
				    feature->unique_id) ;

  return item ;
}


FooCanvasItem *zmapWindowFToIFindSetItem(GHashTable *feature_to_context_hash,
					 ZMapFeatureSet feature_set, ZMapStrand strand)
{
  FooCanvasItem *item = NULL ;
  GQuark column_id ;

  if (strand == ZMAPSTRAND_FORWARD || strand == ZMAPSTRAND_NONE)
    column_id = zmapWindowFToIMakeSetID(feature_set->unique_id, ZMAPSTRAND_FORWARD) ;
  else
    column_id = zmapWindowFToIMakeSetID(feature_set->unique_id, ZMAPSTRAND_REVERSE) ;


  item = zmapWindowFToIFindItemFull(feature_to_context_hash,
				    feature_set->parent_block->parent_alignment->unique_id,
				    feature_set->parent_block->unique_id,
				    column_id,
				    0) ;

  return item ;
}




/* Warning, may return null so result MUST BE TESTED by caller. */
FooCanvasItem *zmapWindowFToIFindItemFull(GHashTable *feature_to_context_hash,
					  GQuark align_id, GQuark block_id, GQuark set_id,
					  GQuark feature_id)
{
  FooCanvasItem *item = NULL ;
  ID2Canvas align ;
  ID2Canvas block ;
  ID2Canvas set ;

  /* Required for minimum query. */
  zMapAssert(feature_to_context_hash && align_id) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if ((align = (ID2Canvas)g_hash_table_lookup(feature_to_context_hash,
					      GUINT_TO_POINTER(align_id)))
      && (block = (ID2Canvas)g_hash_table_lookup(align->hash_table,
						 GUINT_TO_POINTER(block_id)))
      && (set = (ID2Canvas)g_hash_table_lookup(block->hash_table,
					       GUINT_TO_POINTER(set_id))))
    {
      item = (FooCanvasItem *)g_hash_table_lookup(set->hash_table, GUINT_TO_POINTER(feature_id)) ;
    }

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* Cascade down through the hashes until we reach the point the caller wants to stop at. */
  if ((align = (ID2Canvas)g_hash_table_lookup(feature_to_context_hash,
					      GUINT_TO_POINTER(align_id))))
    {
      if (!block_id)
	item = FOO_CANVAS_ITEM(align->canvas.group) ;
      else if ((block = (ID2Canvas)g_hash_table_lookup(align->hash_table,
						       GUINT_TO_POINTER(block_id))))
	{
	  if (!set_id)
	    item = FOO_CANVAS_ITEM(block->canvas.group) ;
	  else if ((set = (ID2Canvas)g_hash_table_lookup(block->hash_table,
							 GUINT_TO_POINTER(set_id))))
	    {
	      if (!feature_id)
		item = FOO_CANVAS_ITEM(set->canvas.group) ;
	      else
		item = (FooCanvasItem *)g_hash_table_lookup(set->hash_table,
							    GUINT_TO_POINTER(feature_id)) ;
	    }
	}
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
    }

  return item ;
}



/* Destroy the feature context hash, this will cause all the hash tables/datastructs that it
 * references to be destroyed as well via the hash table destroy function. */
void zmapWindowFToIDestroy(GHashTable *feature_to_context_hash)
{
  g_hash_table_destroy(feature_to_context_hash) ;
  
  return ;
}


FooCanvasItem *zMapWindowFindFeatureItemByQuery(ZMapWindow window, ZMapWindowFeatureQuery ft_q)
{
  FooCanvasItem *item  = NULL;
  GQuark feature_id ;
  GQuark column_id ;


  if(ft_q->name &&
     ft_q->type != ZMAPFEATURE_INVALID &&
     ft_q->strand_set 
     )
    {
      column_id  = zmapWindowFToIMakeSetID(g_quark_from_string(ft_q->style), ft_q->strand);

      feature_id = zMapFeatureCreateID(ft_q->type, ft_q->name, ft_q->strand, 
                                       ft_q->target_start, ft_q->target_end,
                                       ft_q->query_start, ft_q->query_end
                                       );
#ifdef HHHHHHHHHHHHHH
FooCanvasItem *zmapWindowFToIFindItemFull(GHashTable *feature_to_context_hash,
					  GQuark align_id, GQuark block_id, GQuark set_id,
					  GQuark feature_id)
#endif
      item = zmapWindowFToIFindItemFull(window->context_to_item,
                                        g_quark_from_string(ft_q->alignment),
                                        g_quark_from_string(ft_q->block),
                                        column_id,
                                        feature_id);
      /* MUST CHECK item != NULL users send rubbish! */
    }

  return item;
}


/* 
 *                      Internal routines.
 */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* RUBBISH, NOT NEEDED..... */

/* Compare function, called any time a new item is to be inserted into a table or to be
 * found in a table. */
static gboolean equalIDHash(gconstpointer a, gconstpointer b)
{
  gboolean result = FALSE ;
  ID2Canvas id_a = (ID2Canvas)a, id_b = (ID2Canvas)b ;

  if (id_a.id == id_b.id)
    result = TRUE ;

  return result ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




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
	  ZMapWindowItemFeature item_feature_data ;

	  item_feature_data = (ZMapWindowItemFeature)g_object_get_data(G_OBJECT(item),
								       "item_feature_data") ;

	  if (item_feature_data->start == child_search->child_start
	      && item_feature_data->end == child_search->child_end)
	    child_search->child_item = item ;
	}
    }

  return ;
}

