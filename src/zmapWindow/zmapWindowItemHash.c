/*  File: zmapWindowItemHash.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Functions to go from an alignment, block, column or feature
 *              in a feature context to the corresponding foocanvas group or item.
 *              This is fundamental in linking our feature context hierachy
 *              to the foocanvas item hierachy.
 *
 * Exported functions: See zMapWindow_P.h
 * HISTORY:
 * Last edited: Jul  3 15:19 2009 (rds)
 * Created: Mon Jun 13 10:06:49 2005 (edgrif)
 * CVS info:   $Id: zmapWindowItemHash.c,v 1.47 2010-06-14 15:40:16 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFeature.h>
#include <zmapWindow_P.h>
#include <zmapWindowCanvasItem.h> /* zMapWindowCanvasItemIntevalGetData() */


#define zmapWindowFToIFindAlignById(FTOI_HASH, ID)                                 \
zmapWindowFToIFindItemFull(FTOI_HASH, ID, 0, 0, ZMAPSTRAND_NONE, ZMAPFRAME_NONE, 0)
#define zmapWindowFToIFindBlockById(FTOI_HASH, ALIGN, BLOCK)                              \
zmapWindowFToIFindItemFull(FTOI_HASH, ALIGN, BLOCK, 0, ZMAPSTRAND_NONE, ZMAPFRAME_NONE, 0)


/* A small problemeto is that the GQuark interface does not allow the removal of strings
 * so our routine here will pile up strings in the quark hash table which is global, if
 * this turns out to be a problem we could provide our own quark table implementation...
 * 
 * In fact we do now have our own quark implementation and so could provide a table that would
 * contain just the quarks we actually construct in this function...that would be useful...
 * The quark table could be held within the context struct and reused/emptied as needed.
 * 
 * 
 *  */



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



/* n.b. sometimes we will want the search thing to be a quark, sometimes a string,
 * depending whether the given string is an exact name or a regexp...hmmmm
 * may need to change this to allow both.... */
/* This structure contains the id to be searched for + optionally a precedent function that
 * will test the item for validity (e.g. strand). */
typedef struct ItemSearchStruct_
{
  GQuark search_quark ;					    /* Specifies search string. */
  gboolean is_reg_exp ;					    /* Is the search quark a regexp ? */


  /* Function to check the final results item for validity, only used when we reach the
   * terminating leaf of the search. */
  ZMapWindowFToIPredFuncCB pred_func ;			    /* Function to check items for validity. */
  gpointer user_data ;					    /* Passed to pred_func. */


  /* Actually, I'm not sure why I'm unhappy with this...???? is it because it duplicates the
   * field in the itemliststruct ?? */
  /* UGH not happy with this, rework this when there is time... */
  GList **results ;					    /* result of search. */

} ItemSearchStruct ;




typedef struct
{
  ItemSearch curr_search ;				    /* Current search params. */

  GList *search ;					    /* List of ItemSearch specifying search. */

  GList **results ;					    /* result of search. */

} ItemListSearchStruct, *ItemListSearch ;




static void destroyIDHash(gpointer data) ;
static void doHashSet(GHashTable *hash_table, GList *search, GList **result) ;
static void searchItemHash(gpointer key, gpointer value, gpointer user_data) ;
static void addItem(gpointer key, gpointer value, gpointer user_data) ;
static void childSearchCB(gpointer data, gpointer user_data) ;
static GQuark rootCanvasID(void);

static void printHashKeys(GQuark align, GQuark block, GQuark set, GQuark feature);
static void printGlist(gpointer data, gpointer user_data) ;

gboolean isRegExp(GQuark id) ;
gboolean filterOnRegExp(ItemSearch curr_search, gpointer user_data) ;

static GQuark makeSetID(GQuark set_id, ZMapStrand strand, ZMapFrame frame) ;
static GQuark makeSetIDFromStr(GQuark set_id, ZMapStrand strand, GQuark frame_id) ;

static GQuark feature_same_name_id(ZMapFeature feature);
static void print_search_data(ZMapWindowFToISetSearchData search_data);


static gboolean window_ftoi_debug_G = FALSE;



/* 
 *         Functions externally visible but only for the Window code to use.
 */


 
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


/* I'm really not too sure what the purpose of this was, it is not integrated with the rest of the
 * hashes at all....
 *  */
gboolean zmapWindowFToIAddRoot(GHashTable *feature_to_context_hash,
                               FooCanvasGroup *root_group)
{
  gboolean result = TRUE ;
  GQuark root_id  = 0 ;

  root_id = rootCanvasID() ;

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



gboolean zmapWindowFToIRemoveRoot(GHashTable *feature_to_context_hash)
{
  gboolean result = FALSE ;
  GQuark root_id ;
  ID2Canvas root ;

  root_id = rootCanvasID() ;

  if ((root = g_hash_table_lookup(feature_to_context_hash, GUINT_TO_POINTER(root_id))))
    {
      result = g_hash_table_remove(feature_to_context_hash, GUINT_TO_POINTER(root_id)) ;
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


gboolean zmapWindowFToIRemoveAlign(GHashTable *feature_to_context_hash,
				    GQuark align_id)
{
  gboolean result = TRUE ;
  ID2Canvas align ;

  if ((align = (ID2Canvas)g_hash_table_lookup(feature_to_context_hash, GUINT_TO_POINTER(align_id))))
    {
      result = g_hash_table_remove(feature_to_context_hash, GUINT_TO_POINTER(align_id)) ;
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


gboolean zmapWindowFToIRemoveBlock(GHashTable *feature_to_context_hash,
				    GQuark align_id, GQuark block_id)
{
  gboolean result = FALSE;
  ID2Canvas align, block;

  if ((align = (ID2Canvas)g_hash_table_lookup(feature_to_context_hash,
                                              GUINT_TO_POINTER(align_id)))
      && (block = (ID2Canvas)g_hash_table_lookup(align->hash_table,
                                                 GUINT_TO_POINTER(block_id))))
    {
      result = g_hash_table_remove(align->hash_table, GUINT_TO_POINTER(block_id)) ;
    }
  
  return result;
}


/* Add a hash for the given feature set within a block within an alignment.
 * The set may already exist if we are merging more data from another server,
 * if so then no action is taken otherwise we would lose all our existing feature hashes !
 * Returns FALSE if the align_id or block_id cannot be found, otherwise it returns TRUE
 * as the block_id hash already exists or we add it. */
gboolean zmapWindowFToIAddSet(GHashTable *feature_to_context_hash,
			      GQuark align_id, GQuark block_id, GQuark set_id,
			      ZMapStrand set_strand, ZMapFrame set_frame,
			      FooCanvasGroup *set_group)
{
  gboolean result = FALSE ;
  ID2Canvas align ;
  ID2Canvas block ;

  /* We need special quarks that incorporate strand/frame indication because any one feature set
   * may be displayed in multiple columns. */
  set_id = makeSetID(set_id, set_strand, set_frame) ;

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


gboolean zmapWindowFToIRemoveSet(GHashTable *feature_to_context_hash,
				  GQuark align_id, GQuark block_id, GQuark set_id,
				  ZMapStrand set_strand, ZMapFrame set_frame)
{
  gboolean result = FALSE;
  ID2Canvas align, block, set;

  /* We need special quarks that incorporate strand indication as the hashes are per column. */
  set_id = makeSetID(set_id, set_strand, set_frame) ;

  if ((align = (ID2Canvas)g_hash_table_lookup(feature_to_context_hash,
                                              GUINT_TO_POINTER(align_id)))
      && (block = (ID2Canvas)g_hash_table_lookup(align->hash_table,
                                                 GUINT_TO_POINTER(block_id)))
      && (set = (ID2Canvas)g_hash_table_lookup(block->hash_table,
					       GUINT_TO_POINTER(set_id))))
    {
      result = g_hash_table_remove(block->hash_table, GUINT_TO_POINTER(set_id)) ;
      zMapAssert(result) ;
    }
  
  return result;
}




/* Note that this routine is different in that the feature does not have a hash table associated
 * with it, the table pointer is set to NULL.
 * 
 * Returns FALSE if the align_id, block_id or set_id cannot be found, otherwise it returns TRUE
 * if the feature_id hash already exists or we add it. */
gboolean zmapWindowFToIAddFeature(GHashTable *feature_to_context_hash,
				  GQuark align_id, GQuark block_id,
				  GQuark set_id, ZMapStrand set_strand, ZMapFrame set_frame,
				  GQuark feature_id,
				  FooCanvasItem *feature_item)
{
  gboolean result = FALSE ;
  ID2Canvas align = NULL ;
  ID2Canvas block = NULL ;
  ID2Canvas set = NULL ;
  ZMapFeature item_feature_obj = NULL;

  /* We need special quarks that incorporate strand indication as there are separate column
   * hashes are per strand. */
  item_feature_obj = zmapWindowItemGetFeature(feature_item);
  zMapAssert(item_feature_obj) ;

  set_id = makeSetID(set_id, set_strand, set_frame) ;

  if(window_ftoi_debug_G)
    {
      printf("AddFeature (%p): ", feature_item);
      printHashKeys(align_id, block_id, set_id, feature_id);
    }

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
          feature->hash_table = NULL; // we don't need g_hash_table_new_full(NULL, NULL, NULL, destroyIDHash) ;
          
          g_hash_table_insert(set->hash_table, GUINT_TO_POINTER(feature_id), feature) ;
        }
      
      result = TRUE ;

    }
  return result ;
}




/* Remove functions...not quite as simple as they seem, because removing an item may imply
 * removing its hash table and the hash table below that and the hash table below that.... */



/* Remove a hash for the given feature.
 * Returns FALSE if the feature_id cannot be found, otherwise it returns TRUE and removes
 * the feature_id. */
gboolean zmapWindowFToIRemoveFeature(GHashTable *feature_to_context_hash,
				     ZMapStrand set_strand, ZMapFrame set_frame, ZMapFeature feature_in)
{
  gboolean result = FALSE ;
  GQuark align_id = feature_in->parent->parent->parent->unique_id,
    block_id = feature_in->parent->parent->unique_id,
    set_id = feature_in->parent->unique_id, feature_id = feature_in->unique_id ;
  ID2Canvas align ;
  ID2Canvas block ;
  ID2Canvas set ;
  ID2Canvas feature ;


  set_id = makeSetID(set_id, set_strand, set_frame) ;


#ifdef RDS_DEBUG_ITEM_IN_HASH_DESTROY
  printf("  |--- RemoveFeature: ");
  printHashKeys(align_id, block_id, set_id, feature_id);
#endif /* RDS_DEBUG_ITEM_IN_HASH_DESTROY */

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




FooCanvasItem *zmapWindowFToIFindFeatureItem(GHashTable *feature_to_context_hash,
					     ZMapStrand set_strand, ZMapFrame set_frame,
                                             ZMapFeature feature)
{
  FooCanvasItem *item = NULL ;

  item = zmapWindowFToIFindItemFull(feature_to_context_hash,
				    feature->parent->parent->parent->unique_id,
				    feature->parent->parent->unique_id,
				    feature->parent->unique_id,
				    set_strand, set_frame,
				    feature->unique_id) ;

  return item ;
}


FooCanvasItem *zmapWindowFToIFindSetItem(GHashTable *feature_to_context_hash,
					 ZMapFeatureSet feature_set,
					 ZMapStrand set_strand, ZMapFrame set_frame)
{
  FooCanvasItem *item = NULL ;

  item = zmapWindowFToIFindItemFull(feature_to_context_hash,
				    feature_set->parent->parent->unique_id,
				    feature_set->parent->unique_id,
				    feature_set->unique_id,
				    set_strand, set_frame,
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
 * NOTE that the caller must be careful to set "strand" properly, they cannot just use
 * the strand from a feature, they need to see if the feature is strand specific which is
 * specified in the style. The call to get the strand from a feature is zmapWindowFeatureStrand().
 * 
 * 
 * Warning, may return null so result MUST BE TESTED by caller.
 *  */
FooCanvasItem *zmapWindowFToIFindItemFull(GHashTable *feature_to_context_hash,
					  GQuark align_id, GQuark block_id,
					  GQuark set_id, ZMapStrand set_strand, ZMapFrame set_frame,
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
	  GQuark tmp_set_id = makeSetID(set_id, set_strand, set_frame) ;

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
		  if((feature = (ID2Canvas)g_hash_table_lookup(set->hash_table,
                                                               GUINT_TO_POINTER(feature_id))))
                    item = feature->item ;
		}
	    }
	}
    }
  else if(!align_id)
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
					   ZMapStrand set_strand, ZMapFrame set_frame,
					   ZMapFeature feature,
					   int child_start, int child_end)
{
  FooCanvasItem *item = NULL ;

  /* If the returned item is not a compound item then return NULL, otherwise look for the correct
   * child using the start/end. */
  if ((item = zmapWindowFToIFindFeatureItem(feature_to_context_hash, set_strand, set_frame, feature))
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
 * NOTE that the caller must be careful to set "strand" properly, they cannot just use
 * the strand from a feature, they need to see if the feature is strand specific which is
 * specified in the style. The call to get the strand from a feature is zmapWindowFeatureStrand().
 * 
 * NOTE that frame_spec is restricted to the values:
 * 
 *                 "."  =  ZMAPFRAME_NONE
 *                 "0"  =  ZMAPFRAME_0
 *                 "1"  =  ZMAPFRAME_1
 *                 "2"  =  ZMAPFRAME_2
 *                 "*"  =  all of the above
 * 
 *  */
GList *zmapWindowFToIFindItemSetFull(GHashTable *feature_to_context_hash,
				     GQuark align_id, GQuark block_id,
				     GQuark set_id, char *strand_spec, char *frame_spec,
				     GQuark feature_id,
				     ZMapWindowFToIPredFuncCB pred_func, gpointer user_data)
{
  GList *result = NULL ;
  GQuark strand_id, strand_none, strand_forward, strand_reverse, strand_both ;
  GQuark frame_id, frame_none, frame_0, frame_1, frame_2, frame_all ;
  GQuark forward_set_id = 0, reverse_set_id = 0 ;
  GList *search = NULL;
  ItemSearchStruct align_search = {0}, block_search = {0},
	 forward_set_search = {0}, reverse_set_search = {0},
         feature_search = {0}, terminal_search = {0} ;

  /* Required for minimum query. */
  zMapAssert(feature_to_context_hash && align_id) ;


  align_search.search_quark = align_id ;
  if (align_id && isRegExp(align_id))
    align_search.is_reg_exp = TRUE ;


  block_search.search_quark = block_id ;
  if (block_id && isRegExp(block_id))
    block_search.is_reg_exp = TRUE ;


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

      /* Convert frame_spec to something useful. */
      frame_id = 0 ;
      if (frame_spec)
	{
	  frame_none = g_quark_from_string(".") ;
	  frame_0 = g_quark_from_string("1") ;
	  frame_1 = g_quark_from_string("2") ;
	  frame_2 = g_quark_from_string("3") ;
	  frame_all = g_quark_from_string("*") ;
	  
	  frame_id = g_quark_from_string(frame_spec) ;
	  zMapAssert(frame_id == frame_none
		     || frame_id == frame_0 || frame_id == frame_1 || frame_id == frame_2
		     || frame_id == frame_all) ;
	}


      /* Some care needed here as we may need to wild card the frame to ensure we search
       * enough columns.... */
      if (strand_id == strand_none || strand_id == strand_forward || strand_id == strand_both)
	{
	  forward_set_id = makeSetIDFromStr(set_id, ZMAPSTRAND_FORWARD, frame_id) ;

	  forward_set_search.search_quark = forward_set_id ;

	  if (isRegExp(forward_set_id))
	    forward_set_search.is_reg_exp = TRUE ;
	}

      if (strand_id == strand_reverse || strand_id == strand_both)
	{
	  reverse_set_id = makeSetIDFromStr(set_id, ZMAPSTRAND_REVERSE, frame_id) ;

	  reverse_set_search.search_quark = reverse_set_id ;

	  if (isRegExp(reverse_set_id))
	    reverse_set_search.is_reg_exp = TRUE ;
	}

    }


  feature_search.search_quark = feature_id ;
  if (feature_id && isRegExp(feature_id))
    feature_search.is_reg_exp = TRUE ;


  if (pred_func)
    {
      /* clunky logic...damn those strands.... */
      ItemSearch last_search = NULL ;

      /* N.B. there is always an align id.... */
      if (!block_id)
	last_search = &align_search ;
      else if (!set_id)
	last_search = &block_search ;
      else if (!feature_id)
	{
	  if (forward_set_id)
	    {
	      forward_set_search.pred_func = pred_func ;
	      forward_set_search.user_data = user_data ;
	    }
	  if (reverse_set_id)
	    {
	      reverse_set_search.pred_func = pred_func ;
	      reverse_set_search.user_data = user_data ;
	    }
	}
      else
	last_search = &feature_search ;

      if (last_search)
	{
	  last_search->pred_func = pred_func ;
	  last_search->user_data = user_data ;
	}
    }


  /* build the search list (terminal stop is needed to halt the search if none of the given
   * parameters is a stop). */
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

/* Find all the feature items in a column that have the same name, the name is constructed
 * from the original id as a lower cased regular expression: "original_id*", this is then fed into
 * the hash search function to find all the items. We lowercase it so that it will match
 * against the unique ids of the items.
 * 
 * Returns NULL if there no matching features. I think probably this should never happen
 * as all features match at least themselves.
 *  */
GList *zmapWindowFToIFindSameNameItems(GHashTable *feature_to_context_hash,
				       char *set_strand, char *set_frame,
				       ZMapFeature feature)
{
  GList *item_list    = NULL ;
  GQuark same_name_id = 0;

  same_name_id = feature_same_name_id(feature);

  item_list = zmapWindowFToIFindItemSetFull(feature_to_context_hash,
					    feature->parent->parent->parent->unique_id,
					    feature->parent->parent->unique_id,
					    feature->parent->unique_id,
					    set_strand,
					    set_frame,
					    same_name_id, NULL, NULL) ;
  
  return item_list ;
}


#ifdef SET_DATA_USAGE
{
  GList *result;

  if(cache_search)
    {
      ZMapWindowFToISetSearchData search;
      search = zmapWindowFToISetSearchCreate(zmapWindowFToIFindSameNameItems,
					     feature, 0, 0, 0, 0, "+", "*");
      result = zmapWindowFToISetSearchPerform(context_to_item_hash, search);
      if(search_cache_out)
	*search_cache_out = search;
    }
  else
    result = zmapWindowFToIFindSameNameItems(context_to_item_hash, "+", "*", feature);

  return result;
}
#endif /* SET_DATA_USAGE */

/*!
 * \brief Create a cache of data that can be used by zmapWindowFToISetSearchPerform
 *        to run the Set queries above.
 */
ZMapWindowFToISetSearchData zmapWindowFToISetSearchCreateFull(gpointer    search_function,
							      ZMapFeature feature,
							      GQuark      align_id,
							      GQuark      block_id,
							      GQuark      set_id,
							      GQuark      feature_id,
							      char       *strand_str,
							      char       *frame_str,
							      ZMapWindowFToIPredFuncCB predicate_func,
							      gpointer                 predicate_data,
							      GDestroyNotify           predicate_free)
{
  ZMapWindowFToISetSearchData search_data = NULL;
  gboolean wrong_params = FALSE;
  gboolean debug_caching = FALSE;

  search_data = g_new0(ZMapWindowFToISetSearchDataStruct, 1);

  search_data->search_function = search_function; /* Use this as a type... */

  search_data->predicate_func  = predicate_func;
  search_data->predicate_data  = predicate_data;
  search_data->predicate_free  = predicate_free;

  if(search_function == zmapWindowFToIFindItemSetFull)
    {
      if(feature)
	{
	  search_data->align_id   = feature->parent->parent->parent->unique_id;
	  search_data->block_id   = feature->parent->parent->unique_id;
	  search_data->set_id     = feature->parent->unique_id;
	  search_data->feature_id = feature->unique_id;
	  search_data->strand_str = strand_str;
	  search_data->frame_str  = frame_str;
	}
      else if(align_id)
	{
	  search_data->align_id   = align_id;
	  search_data->block_id   = block_id;
	  search_data->set_id     = set_id;
	  search_data->feature_id = feature_id;
	  search_data->strand_str = strand_str;
	  search_data->frame_str  = frame_str;
	}
      else 
	wrong_params = TRUE;
    }
  else if(search_function == zmapWindowFToIFindSameNameItems)
    {
      GQuark same_name_id = 0;

      if(feature && strand_str && frame_str)
	{
	  same_name_id            = feature_same_name_id(feature);
	  
	  search_data->align_id   = feature->parent->parent->parent->unique_id;
	  search_data->block_id   = feature->parent->parent->unique_id;
	  search_data->set_id     = feature->parent->unique_id;
	  search_data->feature_id = same_name_id;
	  search_data->strand_str = strand_str;
	  search_data->frame_str  = frame_str;

	  /* a predicate_func could cause trouble... unless we NULL it. */
	  search_data->predicate_func = NULL;
	  search_data->predicate_data = NULL;
	}
      else
	wrong_params = TRUE;
    }
  else
    {
      wrong_params = TRUE;
    }

  if(wrong_params)
    {
      g_free(search_data);
      search_data = NULL;
    }
  else if(debug_caching)
    {
      print_search_data(search_data);
    }

  return search_data;
}

ZMapWindowFToISetSearchData zmapWindowFToISetSearchCreate(gpointer    search_function,
							  ZMapFeature feature,
							  GQuark      align_id,
							  GQuark      block_id,
							  GQuark      set_id,
							  GQuark      feature_id,
							  char       *strand_str,
							  char       *frame_str)
{
  ZMapWindowFToISetSearchData search_data;

  search_data = zmapWindowFToISetSearchCreateFull(search_function, feature,
						  align_id, block_id, set_id,
						  feature_id, strand_str, frame_str,
						  NULL, NULL, NULL);

  return search_data;
}

/*!
 * \brief Perform a search using cached data.  This is a Set search so returns a list.
 */
GList *zmapWindowFToISetSearchPerform(GHashTable                 *feature_to_context_hash,
				      ZMapWindowFToISetSearchData search_data)
{
  GList *list = NULL;

  if(search_data->search_function == zmapWindowFToIFindItemSetFull   ||
     search_data->search_function == zmapWindowFToIFindSameNameItems)
    {
      list = zmapWindowFToIFindItemSetFull(feature_to_context_hash, 
					   search_data->align_id,
					   search_data->block_id,
					   search_data->set_id,
					   search_data->strand_str,
					   search_data->frame_str,
					   search_data->feature_id,
					   search_data->predicate_func, 
					   search_data->predicate_data) ;
    }
  
  return list;
}

void zmapWindowFToISetSearchDestroy(ZMapWindowFToISetSearchData search_data)
{
  if(search_data->predicate_free && search_data->predicate_data)
    (search_data->predicate_free)(search_data->predicate_data);
  
  g_free(search_data);

  return ;
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


/* The frame/strand strings used here should be rationalised with those in zmapFeature code..... */

/* We need to make special ids for the forward/reverse column groups as these don't exist
 * in the features and we also need to account of frame. */
static GQuark makeSetID(GQuark set_id, ZMapStrand strand, ZMapFrame frame)
{
  GQuark full_set_id = 0 ;
  char *set_str, *strand_str, *frame_str ;
  
  if (strand == ZMAPSTRAND_NONE || strand == ZMAPSTRAND_FORWARD)
    strand_str = "FORWARD" ;
  else
    strand_str = "REVERSE" ;

  switch (frame)
    {
    case ZMAPFRAME_0:
      frame_str = "0" ;
      break ;
    case ZMAPFRAME_1:
      frame_str = "1" ;
      break ;
    case ZMAPFRAME_2:
      frame_str = "2" ;
      break ;
    default:
      frame_str = "." ;
    }

  set_str = g_strdup_printf("%s_%s_%s%s", g_quark_to_string(set_id),
			    strand_str,
			    FRAME_PREFIX, frame_str) ;

  full_set_id = g_quark_from_string(set_str) ;

  g_free(set_str) ;

  return full_set_id ;
}

/* We need to make special ids for the forward/reverse column groups as these don't exist
 * in the features and we also need to account of frame. */
static GQuark makeSetIDFromStr(GQuark set_id, ZMapStrand strand, GQuark frame_id)
{
  GQuark full_set_id = 0 ;
  char *set_str, *strand_str, *frame_str ;

  /* Need to check frame id here.... */
  
  if (strand == ZMAPSTRAND_NONE || strand == ZMAPSTRAND_FORWARD)
    strand_str = "FORWARD" ;
  else
    strand_str = "REVERSE" ;

  frame_str = (char *)g_quark_to_string(frame_id) ;

  set_str = g_strdup_printf("%s_%s_%s%s", g_quark_to_string(set_id),
			    strand_str,
			    FRAME_PREFIX, frame_str) ;

  full_set_id = g_quark_from_string(set_str) ;

  g_free(set_str) ;

  return full_set_id ;
}


/* Is called for all entries in the hash table of feature set ids -> hash table of features. For
 * each one we need to destroy the feature hash table and then free the struct itself. */
static void destroyIDHash(gpointer data)
{
  ID2Canvas id = (ID2Canvas)data ;

  if(id->hash_table)            /* Feature ID2Canvas don't need hash_tables... */
    g_hash_table_destroy(id->hash_table) ;

#ifdef RDS_DEBUG_ITEM_IN_HASH_DESTROY
  printf("     |--- destroyIDHash (%x): freeing id2canvas\n", id->item);
#endif /* RDS_DEBUG_ITEM_IN_HASH_DESTROY */

  g_free(id) ;

  return ;
}



/* This is a g_list callback function. */
static void childSearchCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ChildSearch child_search = (ChildSearch)user_data ;

  /* We take the first match we find so this function does nothing if we have already
   * found matching child item. */
  if (!(child_search->child_item))
    {
      ZMapFeatureSubPartSpan item_subfeature_data ;
      
      if((item_subfeature_data = zMapWindowCanvasItemIntervalGetData(item)))
	{
	  if (item_subfeature_data->start == child_search->child_start &&
	      item_subfeature_data->end   == child_search->child_end)
	    {
	      child_search->child_item = item ;
	    }
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


  if (next_search_id == stop)
    {
      /* If we've reached the end it's time to actually add items to the list. */

      /* If the search id matches exactly then add it to the results, otherwise if its a wild card
       * or we need to validate on some criteria then look at all items in the hash. */
      if ((item_id = (ID2Canvas)g_hash_table_lookup(hash_table,
						    GUINT_TO_POINTER(curr_search_id))))
	{
	  if (!curr_search->pred_func || curr_search->pred_func(item_id->item, curr_search->user_data))
	    results = g_list_append(results, item_id->item) ;
	}
      else if (curr_search_id == wild_card || curr_search->is_reg_exp)
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

      search_data.curr_search = curr_search ;
      search_data.search = search ;
      search_data.results = &results ;

      /* If the search id matches exactly then so that hash set, otherwise if its a wild card
       * or we need to search on some validation criteria then do all the sets. */
      if ((item_id = (ID2Canvas)g_hash_table_lookup(hash_table,
						    GUINT_TO_POINTER(curr_search_id))))
	{
	  doHashSet(item_id->hash_table, search_data.search, search_data.results) ;
	}
      else if (curr_search_id == wild_card || curr_search->is_reg_exp)
	{
	  g_hash_table_foreach(hash_table, searchItemHash, (gpointer)&search_data) ;
	}
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

  if (curr_search->is_reg_exp && filterOnRegExp(curr_search, key)
      && (!(curr_search->pred_func)
	  || (curr_search->pred_func
	      && curr_search->pred_func(hash_item->item, curr_search->user_data))))
    *results = g_list_append(*results, hash_item->item) ;

  return ;
}


/* A GHFunc() callback which calls doHashSet() to search this items own hash table. */
static void searchItemHash(gpointer key, gpointer value, gpointer user_data)
{
  ID2Canvas hash_item = (ID2Canvas)value ;
  ItemListSearch search = (ItemListSearch)user_data ;

  if (!search->curr_search->is_reg_exp
      || filterOnRegExp(search->curr_search, key))
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

  if ((feature_any = zmapWindowItemGetFeatureAny(item)))
    {
      switch (feature_any->struct_type)
	{
	case ZMAPFEATURE_STRUCT_ALIGN:
	  feature_type = "Align" ;
	  break ;
	case ZMAPFEATURE_STRUCT_BLOCK:
	  feature_type = "Block" ;
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


/* Use simple glib regexp func to compare wild card string with unique ids in hash.
 *  */
gboolean filterOnRegExp(ItemSearch curr_search, gpointer user_data)
{
  gboolean result = FALSE ;
  char *pattern, *key_string ;
  GQuark key_id = GPOINTER_TO_INT(user_data) ;

  key_string = (char *)g_quark_to_string(key_id) ;

  pattern = (char *)g_quark_to_string(curr_search->search_quark) ;

  /* Compare pattern to hash key string, _not_ features unique_id string. */
  if (g_pattern_match_simple(pattern, key_string))
    result = TRUE ;

  return result ;
}


static GQuark rootCanvasID(void)
{
  return g_quark_from_string("ROOT_ID");
}


/* RT:1589
 * This doesn't work as planned, the plan was to attach a destroy
 * handler to the item which gets added to the hash.  When this gets
 * cleaned up by *any* means the destroy handler would get called and
 * remove the entry from the appropriate hash table.  This doesn't
 * work for two reasons: 
 *
 * 1 - The feature may have its strand changed.  This is solved by as this
 *     code does attaching the original strand as data for the destroy
 *     handler.  This creates a separate issue of where to get the hash
 *     table from...  solution here is a g_object_set(canas, WINDOW, window)
 *     so g_object_get(item_in_hash->canvas, WINDOW) returns the window
 *     and hence window->context_to_item_hash.... This is therefore a minor
 *     issue of the two.
 * 2 - The canvas group/items don't get their destroy handlers called in
 *     the correct order. Destroy signals are emitted on groups, before 
 *     the signals on the items they contain.  This is despite the following
 *     logic in the group destroy method
 *     list = grp->item_list;
 *     while (list) {
 *        child = list->data;
 *        list = list->next;
 *        gtk_object_destroy (GTK_OBJECT (child));
 *     }
 *     if (GTK_OBJECT_CLASS (group_parent_class)->destroy)
 *       (* GTK_OBJECT_CLASS (group_parent_class)->destroy) (object);
 *
 */

static void printHashKeys(GQuark align, GQuark block, GQuark set, GQuark feature)
{
  GQuark empty = 0;
  empty = g_quark_from_string("<empty>");

  printf("keys: %s %s %s %s\n", 
         (char *)g_quark_to_string(align   ? align   : empty),
         (char *)g_quark_to_string(block   ? block   : empty),
         (char *)g_quark_to_string(set     ? set     : empty),
         (char *)g_quark_to_string(feature ? feature : empty));

  return ;
}

static GQuark feature_same_name_id(ZMapFeature feature)
{
  GString *reg_ex_name ;
  GQuark reg_ex_name_id ;

  /* I use a GString because it lowercases in place. */
  reg_ex_name    = g_string_new(NULL) ;

  g_string_append_printf(reg_ex_name, "%s*", (char *)g_quark_to_string(feature->original_id)) ;

  reg_ex_name    = g_string_ascii_down(reg_ex_name) ;
  reg_ex_name_id = g_quark_from_string(reg_ex_name->str) ;

  g_string_free(reg_ex_name, TRUE) ;

  return reg_ex_name_id;
}

static void print_search_data(ZMapWindowFToISetSearchData search_data)
{
  printf("Search Data for function ");

  if(search_data->search_function == zmapWindowFToIFindSameNameItems)
    printf("zmapWindowFToIFindSameNameItems");
  else if(search_data->search_function == zmapWindowFToIFindItemSetFull)
    printf("zmapWindowFToIFindItemSetFull");
  else
    printf("<unsupported function>");

  printf(":\n");

  printf("  align_id: '%s'\n", (search_data->align_id ? g_quark_to_string(search_data->align_id) : "0"));
  
  printf("  block_id: '%s'\n", (search_data->block_id ? g_quark_to_string(search_data->block_id) : "0"));
  
  printf("  set_id: '%s'\n", (search_data->set_id ? g_quark_to_string(search_data->set_id) : "0"));
  
  printf("  feature_id: '%s'\n", (search_data->feature_id ? g_quark_to_string(search_data->feature_id) : "0"));
  
  printf("  strand: '%s'\n", (search_data->strand_str ? search_data->strand_str : "<unset>"));
  
  printf("  frame: '%s'\n", (search_data->frame_str ? search_data->frame_str : "<unset>"));
  
  return ;
}
