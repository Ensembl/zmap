/*  File: zmapGLibUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: A collection of "add-ons" to GLib that seem useful.
 *
 * Exported functions: See ZMap/zmapGLibUtils.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <stdio.h>
#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>






/* Holds a single quark set. */
typedef struct _ZMapQuarkSetStruct
{
  gint block_size ;
  GHashTable *g_quark_ht;
  gchar **g_quarks ;
  GQuark g_quark_seq_id ;
} ZMapQuarkSetStruct ;



/* Needed to get at private array struct. */
typedef struct _GRealArray  GRealArray;

struct _GRealArray
{
  guint8 *data;
  guint   len;
  guint   alloc;
  guint   elt_size;
  guint   zero_terminated : 1;
  guint   clear : 1;
};

#define g_array_elt_len(array,i) ((array)->elt_size * (i))
#define g_array_elt_pos(array,i) ((array)->data + g_array_elt_len((array),(i)))


typedef struct
{
  GQuark id;
}DatalistFirstIDStruct, *DatalistFirstID;

static inline GQuark g_quark_new(ZMapQuarkSet quark_set, gchar *string) ;
static void printCB(gpointer data, gpointer user_data) ;
static gint caseCompareFunc(gconstpointer a, gconstpointer b) ;

static gboolean getNthHashElement(gpointer key, gpointer value, gpointer user_data) ;

static void hashCopyListCB(gpointer key, gpointer value, gpointer user_data) ;
static void hashPrintListCB(gpointer key, gpointer value, gpointer user_data_unused) ;
static void hashPrintTableCB(gpointer key, gpointer value, gpointer user_data_unused);
static void destroyList(gpointer data) ;
static void mergehashCB(gpointer key, gpointer value, gpointer user_data) ;



/*! @defgroup zmapGLibutils   zMapGLibUtils: glib-derived utilities for ZMap
 * @{
 *
 * \brief  Glib-derived utilities for ZMap.
 *
 *
 * These routines are derived from the glib code available from www.gtk.org
 *
 * zMapUtils routines provide services such as debugging, testing and logging,
 * string handling, file utilities and GUI functions. They are general routines
 * used by all of ZMap.
 *
 *  */


/*!
 *                Additions to String utilities
 */



/* This routine removes all occurences of the given char from the target string,
 * this is done inplace so the returned string will be shorter. */
char *zMap_g_remove_char(char *string, char ch)
{
  char *cp ;

  zMapAssert(string) ;

  cp = string ;
  while (*cp)
    {
      if (*cp == ch)
	{
	  char *cq ;

	  cq = cp ;
	  while (*cq)
	    {
	      *cq = *(cq + 1) ;

	      cq++ ;
	    }
	  *cq = '\0' ;
	}

      cp++ ;
    }

  return string ;
}


/* This function is like strstr() but case independent. The code is modified
 * from g_ascii_strcasecmp() from glib/gstrfuncs.c. This seems like a no-brainer
 * to be added to glib. */
gchar *zMap_g_ascii_strstrcasecmp(const gchar *haystack, const gchar *needle)
{
  gchar *match_start = NULL, *needle_start = (gchar *)needle ;
  gint c1, c2;

#define ISUPPER(c)		((c) >= 'A' && (c) <= 'Z')
#define	TOLOWER(c)		(ISUPPER (c) ? (c) - 'A' + 'a' : (c))

  g_return_val_if_fail (haystack != NULL, NULL);
  g_return_val_if_fail (needle != NULL, NULL);

  while (*haystack && *needle)
    {
      c1 = (gint)(guchar) TOLOWER (*haystack);
      c2 = (gint)(guchar) TOLOWER (*needle);

      if (c1 == c2)
	{
	  if (!match_start)
	    match_start = (gchar *)haystack ;
	  haystack++ ;
	  needle++ ;
	}
      else
	{
	  match_start = NULL ;
	  needle = needle_start ;
	  haystack++ ;
	}
    }

  if ((*needle))
    match_start = NULL ;

  return match_start ;
}




/*!
 *                Additions to GList
 */


#ifdef UTILISE_SINGLE_FUNCTION
/* Go through a list in reverse calling the supplied GFunc.
 * This was lifted from the glib code of g_list. */

/* changed this slightly to be the g_list_foreach_directional
 * function, as it was working differently to g_list_foreach, by
 * the addition of the g_list_last.  g_list_foreach doesn't do a
 * g_list_first, but then it "shouldn't" need to as the all their
 * function return the first item of the list again. Here we need to
 * get to the end of the list to do the reverse! but the only place
 * it's getting called from at the moment is zmapWindowDrawFeatures.c
 * positionColumns, where the list is the item list of the
 * foocanvasgroup.  the group holds a pointer to the end of the list,
 * so we can use that instead and the other function below...
 * However, in the dna highlighting it's better not to go all the way
 * to the end of the list... Maybe we need 2 functions as it is nice
 * to have the g_list_last encapsulated in reverse function...
 */
void zMap_g_list_foreach_reverse(GList *list, GFunc func, gpointer user_data)
{
  list = g_list_last(list) ;

  while (list)
    {
      GList *prev = list->prev ;

      (*func)(list->data, user_data) ;
      list = prev ;
    }

  return ;
}
#endif

void zMap_g_list_foreach_directional(GList   *list,
                                     GFunc    func,
                                     gpointer user_data,
                                     ZMapGListDirection forward)
{
  while (list)
    {
      GList *next = (forward == ZMAP_GLIST_FORWARD ? list->next : list->prev);
      (*func) (list->data, user_data);
      list = next;
    }
  return ;
}


/*! Just like g_list_foreach() except that the ZMapGFuncCond function can return
 * FALSE to stop the foreach loop from executing.
 *
 * Returns FALSE if ZMapGFuncCond returned FALSE, TRUE otherwise.
 *
 *  */
gboolean zMap_g_list_cond_foreach(GList *list, ZMapGFuncCond func, gpointer user_data)
{
  gboolean status = TRUE ;

  while (list)
    {
      GList *next = list->next ;

      if (!((*func)(list->data, user_data)))
	{
	  status = FALSE ;
	  break ;
	}

      list = next ;
    }

  return status ;
}


/*!
 * Finds the given element data and moves it to the given position in the list if
 * that is possible. Note that list indices start at zero.
 *
 * Returns the list unaltered if the element could not be moved.
 *
 *  */
GList *zMap_g_list_move(GList *list, gpointer user_data, gint new_index)
{
  GList *new_list = list ;

  if (new_index >= 0 && new_index < g_list_length(list))
    {
      GList *list_element ;

      if ((list_element = g_list_find(list, user_data)))
	{
	  new_list = g_list_remove(new_list, user_data) ;

	  new_list = g_list_insert(new_list, user_data, new_index) ;
	}
    }

  return new_list ;
}


GList *zMap_g_list_append_unique(GList *list, gpointer data)
{
	GList *l,*last = list;

	for(l = list; l ; l = l->next)
	{
		if(l->data == data)	/* already there, nothing to do */
			return list;
		last = l;
	}

	/* we reached the end or there was no list: add to the end */
	last = g_list_append(last,data);

	if(!list)
		list = last;

	return list;
}

/*!
 * Prints out the contents of a list assuming that each element is a GQuark. We have
 * lots of these in zmap so this is useful.
 *
 * @param quark_list             A GList where each data pointer is a GQuark.
 * @param list_name              Optional list name to be printed first.
 * @param new_line               FALSE print list on one line, TRUE print each element
 *                               on a new line.
 * @return                       <nothing>
 */
void zMap_g_list_quark_print(GList *quark_list, char *list_name, gboolean new_line)
{
  zMapAssert(quark_list) ;

  if (list_name)
    printf("\"%s\":%s", list_name, (new_line ? "\n" : "\t")) ;

  g_list_foreach(quark_list, printCB, GINT_TO_POINTER(new_line)) ;

  printf("\n") ;

  return ;
}


GList *zMap_g_list_find_quark(GList *list, GQuark str_quark)
{
  GList *result = NULL ;

  zMapAssert(list && str_quark) ;

  result = g_list_find_custom(list, GINT_TO_POINTER(str_quark), caseCompareFunc) ;

  return result ;
}

/* filter out a list of matching items */
GList *zMap_g_list_grep(GList **list_inout, gpointer data, GCompareFunc func)
{
  GList *matched = NULL;
  GList *loop_list, *tmp, *inout;

  zMapAssert(list_inout);

  loop_list = inout = *list_inout;

  while(loop_list)
    {
      if(!func(loop_list->data, data))
        {
          tmp = loop_list->next;
          inout = g_list_remove_link(inout, loop_list);
          matched = g_list_concat(matched, loop_list);
          loop_list = tmp;
        }
      else
        loop_list = loop_list->next;
    }

  /* basically in case we removed the first element */
  *list_inout = inout;

  return matched;
}

/* donor should already no be part of recipient */
void insert_after(GList *donor, GList *recipient)
{
  zMapAssert(recipient && recipient->next && donor);

  /*
   * This next bit of code is confusing so here is a desciption...
   *
   * recipient A -> <- B -> <- F
   * donor     C -> <- D -> <- E
   * point = 2
   *
   * recipient[B]->next[F]->prev = g_list_last(donor)[E]
   * recipient[B]->next[F]->prev[E]->next = recipient->next[F]
   * recipient[B]->next = donor[C]
   * donor[C]->prev = recipient[B]
   *
   * complete =  A -> <- B -> <- C -> <- D -> <- E -> <- F
   *                       |   |                   |   |
   * so we've changed B->next, C->prev,      E->next & F->prev
   */
  recipient->next->prev = g_list_last(donor);
  recipient->next->prev->next = recipient->next;
  recipient->next = donor;
  donor->prev = recipient;

  return ;
}

/* point is 0 based! */
GList *zMap_g_list_insert_list_after(GList *recipient, GList *donor, int point)
{
  GList *complete = NULL;

  recipient = g_list_first(recipient);

  if(point == 0)
    {
      /* quicker to concat the recipient to the donor! */
      recipient = g_list_concat(donor, recipient);
    }
  else if((recipient = g_list_nth(recipient, point)) &&
          recipient->next)
    {
      insert_after(donor, recipient);
    }
  else if(recipient)
    recipient = g_list_concat(recipient, donor);
  else
    zMapAssertNotReached();     /* point off list! */

  complete = g_list_first(recipient);

  return complete;
}

GList *zMap_g_list_lower(GList *move, int positions)
{
  GList *before;

  if(positions < 1)
    return move;

  if(move->prev)
    {
      for(before = move->prev; positions && before; positions--)
        before = before->prev;
    }
  else
    before = NULL;

  if(before)
    {
      before = g_list_remove_link(before, move);
      if(before->next)
        insert_after(move, before);
      else
        before = g_list_concat(before, move);
      zMapAssert(g_list_find(before, move->data));
    }

  return g_list_first(move);
}

GList *zMap_g_list_raise(GList *move, int positions)
{
  GList *before;

  if(positions < 1)
    return move;

  for(before = move; positions && before; positions--)
    before = before->next;

  if(!before)
    before = g_list_last(move);

  if(before)
    {
      before = g_list_remove_link(before, move);
      if(before->next)
        insert_after(move, before);
      else
        before = g_list_concat(before, move);
      zMapAssert(g_list_find(before, move->data));
    }

  return g_list_first(move);
}



/*! Takes a list and splits it into two at new_list_head. The original
 * list is truncated and ends with the element _before_ new_list_head,
 * the new list starts with new_list_head and contains all the elements
 * from there up until the end of the original list.
 *
 * Returns new_list_head or NULL if new_list_head is not in list OR
 * if new_list_head == list.
 *  */
GList *zMap_g_list_split(GList *list, GList *new_list_head)
{
  GList *new_list = NULL ;

  if (new_list_head != list && g_list_first(new_list_head) == list)
    {
      (new_list_head->prev)->next = NULL ;
      new_list_head->prev = NULL ;
      new_list = new_list_head ;
    }

  return new_list ;
}


// return a list of quarks as a ; delimted string, free this string when finished
gchar *zMap_g_list_quark_to_string(GList *l)
{
      gchar *str = NULL;
      const gchar *q;

      while(l)
      {
            q = g_quark_to_string(GPOINTER_TO_UINT(l->data));
            if(str)
                  str = g_strconcat(str,";",q,NULL);
            else
                  str = g_strdup(q);
            l = l->next;
      }

      return(str);
}


// merge lists giving list of unique items
GList *zMap_g_list_merge(GList *a, GList *b)
{
      for(;b;b = b->next)
      {
            if(!g_list_find(a,b->data))
                  a = g_list_prepend(a,b->data);
      }
      return(a);
}

/*!
 *                Additions to GHash
 */


/* MH17: this function is only ever called with nth == 0 */
/* Returns nth hash table element, elements start with the zero'th. */
gpointer zMap_g_hash_table_nth(GHashTable *hash_table, int nth)
{
  gpointer entry = NULL ;

  zMapAssert(hash_table && nth >= 0) ;

  if (g_hash_table_size(hash_table) > nth)
    entry = g_hash_table_find(hash_table, getNthHashElement, &nth) ;

  return entry ;
}


/* Supported values for data_format_str are: "gquark", "pointer"  */
void zMap_g_hash_table_print(GHashTable *hash_table, char *data_format_str)
{
  g_hash_table_foreach(hash_table, hashPrintTableCB, data_format_str) ;

  return ;
}

/*! A set of functions to handle a hash where each key is a GQuark and each entry is a
 * simple list that can be directly copied. Several places in our code need to
 * construct and look at his list hence these functions.
 *
 * Copy function could easily be extended to allow deep list copying with a callback.
 *
 * NOTE WELL: There is a destroy function for the hash values and this WILL get
 * called by g_hash_table for inserts/replaces. Below you will see we copy lists
 * because as soon as we reinsert the list the old copy will be deleted by the
 * delete callback.
 *
 *  */


/* ! Make the hash of lists table. */
GHashTable *zMap_g_hashlist_create(void)
{
  GHashTable *new_hashlist = NULL ;

  new_hashlist = g_hash_table_new_full(NULL, NULL, NULL, destroyList) ;

  return new_hashlist ;
}


/* ! Insert a key value into the keyed list. */
void zMap_g_hashlist_insert(GHashTable *hashlist, GQuark key, gpointer value)
{
  GList *list ;

  /* Slightly tricky coding, if we don't find a list the append creates a new list,
   * otherwise we append to the existing list. Either way we then _always_ replace
   * the list in the hash as list start in theory could change. BUT note that
   * we must copy the list before we insert it because otherwise it is erased
   * by g_hash_table calling the existing entries delete function. */
  list = (GList *)g_hash_table_lookup(hashlist, GINT_TO_POINTER(key)) ;

  /* This makes this as bad as a g_datalist... Well nearly.   */
  if (!g_list_find(list, value))
    {
      list = g_list_copy(list) ;

      list = g_list_append(list, value) ;

      g_hash_table_insert(hashlist, GINT_TO_POINTER(key), list) ;
    }

  return ;
}


/* ! Insert a list of key values into the keyed list, if replace is TRUE then the list
 * replaces any existing list, if FALSE and a list already exists then the function simply
 * returns. */
void zMap_g_hashlist_insert_list(GHashTable *hashlist, GQuark key, GList *key_values, gboolean replace)
{
  /* Slightly tricky coding, the existing copy of the list will automatically
   * be free'd by g_hash_table calling our registered hash delete function. */
  if (replace || !(g_hash_table_lookup(hashlist, GINT_TO_POINTER(key))))
    g_hash_table_insert(hashlist, GINT_TO_POINTER(key), key_values) ;

  return ;
}


/*! Make a copy of the hash table of featureset names to styles lists. */
GHashTable *zMap_g_hashlist_copy(GHashTable *orig_hashlist)
{
  GHashTable *new_hashlist = NULL ;

  new_hashlist = zMap_g_hashlist_create() ;

  g_hash_table_foreach(orig_hashlist, hashCopyListCB, new_hashlist) ;

  return new_hashlist ;
}



/*! Merge two featureset/style hashes, any elements from "in" are merged into
 * in_out, in is left unaltered. */
void zMap_g_hashlist_merge(GHashTable *in_out, GHashTable *in)
{

  g_hash_table_foreach(in, mergehashCB, in_out) ;

  return ;
}



/*! Print out the featureset -> styles lists. */
void zMap_g_hashlist_print(GHashTable *hashlist)
{
  g_hash_table_foreach(hashlist, hashPrintListCB, NULL) ;

  return ;
}


void zMap_g_hashlist_destroy(GHashTable *hashlist)
{
  g_hash_table_destroy(hashlist) ;

  return ;
}


// replacement for functions not avail in old versions of glib
// return a list of key->value structs

static void get_key_value(gpointer key, gpointer value, gpointer user)
{
      ZMapGHashIter data;
      GList **list = (GList **) user;

      data = g_new0(ZMapGHashIterStruct,1);
      data->key = key;
      data->value = value;
      *list = g_list_prepend(*list,data); // faster than append
}

void  zMap_g_hash_table_iter_init(GList **iter, GHashTable *h)
{
      *iter = NULL;
      if(h)
            g_hash_table_foreach(h,get_key_value,iter);
}


static void get_hash_key(gpointer key, gpointer value, gpointer user)
{
      GList **list = (GList **) user;


      *list = g_list_prepend(*list,key); // faster than append
}

void  zMap_g_hash_table_get_keys(GList **iter, GHashTable *h)
{
      *iter = NULL;
      if(h)
            g_hash_table_foreach(h,get_hash_key,iter);
}


static void get_hash_val(gpointer key, gpointer value, gpointer user)
{
      GList **list = (GList **) user;


      *list = g_list_prepend(*list,value); // faster than append
}

void  zMap_g_hash_table_get_data(GList **iter, GHashTable *h)
{
      *iter = NULL;
      if(h)
            g_hash_table_foreach(h,get_hash_val,iter);
}


// must iter through all to free memory
gboolean zMap_g_hash_table_iter_next(GList **iter,gpointer *key, gpointer *value)
{
      ZMapGHashIter data;
      GList *list = *iter;

      if(!list)
            return(FALSE);

      data = list->data;
      *iter = g_list_remove_link(*iter,list);
      g_list_free_1(list);

      *key = data->key;
      *value = data->value;
      g_free(data);

      return(TRUE);
}

// in case we decide to not iter through the whole lot
// is there a generic list free function?
// yes: g_list_free()
//void zMap_g_hash_table_iter_free(GList **list) { }



/*
 *                Additions to GDatalist
 */

#if MH17_NOT_USED
G_LOCK_DEFINE_STATIC(datalist_first);

static void get_datalist_length(GQuark key, gpointer data, gpointer user_data)
{
  gint *length = (gint *)user_data;

  (*length)++;

  return ;
}

static void get_first_datalist_key(GQuark id, gpointer data, gpointer user_data)
{
  DatalistFirstID key = (DatalistFirstID)user_data;

  if(!key->id)
    key->id = id;

  return ;
}


gpointer zMap_g_datalist_first(GData **datalist)
{
  DatalistFirstIDStruct key = {0};
  gpointer data;
  G_LOCK(datalist_first);
  g_datalist_foreach(datalist, get_first_datalist_key, &key);
  G_UNLOCK(datalist_first);
  data = g_datalist_id_get_data(datalist, key.id);

  return data;
}

gint zMap_g_datalist_length(GData **datalist)
{
  gint length = 0;
  G_LOCK(datalist_first);
  g_datalist_foreach(datalist, get_datalist_length, &length);
  G_UNLOCK(datalist_first);
  return length;
}

#endif

/*
 *                Additions to GArray
 */


/* My new routine for returning a pointer to an element in the array, the array will be
 * expanded to include the requested element if necessary. If "clear" was not set when
 * the array was created then the element may contain random junk instead of zeros. */
GArray* zMap_g_array_element (GArray *farray, guint index, gpointer *element_out)
{
  GRealArray *array = (GRealArray*) farray;
  gint length;

  length = (index + 1) - array->len;

  if (length > 0)
    {
      farray = g_array_set_size (farray, length);
    }

  *element_out = g_array_elt_pos (array, index);

  return farray;
}


/*!
 *                Additions to GString
 *
 * GString is a very good package but there are some more operations it could usefully do.
 *
 */


/*!
 * Substitute one string for another in a GString.
 *
 * You should note that this routine simply uses the existing GString erase/insert
 * functions and so may not be incredibly efficient. If the target string was not
 * found the GString remains unaltered and FALSE is returned.
 *
 * @param string                 A valid GString.
 * @param target                 The string to be replaced.
 * @param source                 The string to be inserted.
 * @return                       TRUE if a string was replaced, FALSE otherwise.
 *  */
gboolean zMap_g_string_replace(GString *string, char *target, char *source)
{
  gboolean result = FALSE ;
  int source_len ;
  int target_len ;
  int target_pos ;
  char *template_ptr ;


  target_len = strlen(target) ;
  source_len = strlen(source) ;

  template_ptr = string->str ;
  while ((template_ptr = strstr(template_ptr, target)))
    {
      result = TRUE ;

      target_pos = template_ptr - string->str ;

      string = g_string_erase(string, target_pos, target_len) ;

      string = g_string_insert(string, target_pos, source) ;

      template_ptr = string->str + target_pos + source_len ; /* Shouldn't go off the end. */
    }

  return result ;
}





/*!
 *                Additions to GQuark
 *
 * We need quarks that are allocated in sets that we can throw away as zmap can
 * have many thousands of quarks for each zmap displayed.
 *
 * The following routines create, manipulate and destroy a set of quarks.
 *
 */


/*!
 * Create the quark set.
 *
 * The block size allows control over the number of string
 * pointers allocated at one go, this may be a good optimisation where there are
 * large numbers of quarks to be allocated. If block size is 0 it defaults to 512
 * which is the glib default.
 * The function returns a new quark set which should only be destroyed with a call
 * to zMap_g_quark_destroy_set().
 *
 * @param block_size             0 for success, anything else for failure.
 * @return                       ZMapQuarkSet quark set.
 *  */
ZMapQuarkSet zMap_g_quark_create_set(guint block_size)
{
  ZMapQuarkSet quark_set = NULL ;

  quark_set = g_new0(ZMapQuarkSetStruct, 1) ;

  if (block_size == 0)
    block_size = 512 ;
  quark_set->block_size = block_size ;

  quark_set->g_quark_ht = g_hash_table_new(g_str_hash, g_str_equal) ;

  quark_set->g_quarks = NULL ;

  quark_set->g_quark_seq_id = 0 ;

  return quark_set ;
}

/*!
 * Look for a string in the quark set, if the string is not found no quark is
 * created and 0 is returned.
 *
 * @param quark_set              A valid quark set.
 * @param string                 The string to look for in the set.
 * @return                       GQuark for the string or 0 if string not found.
 *  */
GQuark zMap_g_quark_try_string(ZMapQuarkSet quark_set, gchar *string)
{
  GQuark quark = 0 ;

  zMapAssert(quark_set && string) ;

  quark = GPOINTER_TO_UINT(g_hash_table_lookup(quark_set->g_quark_ht, string)) ;

  return quark ;
}


/*!
 * Return a quark for a given string, if the string is not found, the string
 * is added to the set and its quark returned.
 *
 * @param quark_set              A valid quark set.
 * @param string                 The string to look for in the set.
 * @return                       GQuark for the string or 0 if string not found.
 *  */
GQuark zMap_g_quark_from_string(ZMapQuarkSet quark_set, gchar *string)
{
  GQuark quark ;

  /* May be too draconian to insist on *string...just remove if so. */
  zMapAssert(quark_set && string && *string) ;

  if (!(quark = (gulong) g_hash_table_lookup(quark_set->g_quark_ht, string)))
    quark = g_quark_new(quark_set, g_strdup (string));

  return quark;
}

/*!
 * Return the string for a given quark, if the quark cannot be found in the set
 * NULL is returned.
 *
 * @param quark_set              A valid quark set.
 * @param quark                  The quark for which the string should be returned.
 * @return                       A string or NULL.
 *  */
gchar *zMap_g_quark_to_string(ZMapQuarkSet quark_set, GQuark quark)
{
  gchar* result = NULL;

  zMapAssert(quark_set && quark) ;

  if (quark > 0 && quark <= quark_set->g_quark_seq_id)
    result = quark_set->g_quarks[quark - 1] ;

  return result;
}


/*!
 * Destroy a quark_set, freeing all of its resources <B>including</B> the strings
 * it holds (i.e. you should make copies of any strings you need after the set
 * has been destroyed.
 *
 * @param quark_set              A valid quark set.
 * @return                       nothing
 *  */
void zMap_g_quark_destroy_set(ZMapQuarkSet quark_set)
{
  int i ;

  g_hash_table_destroy(quark_set->g_quark_ht) ;

  /* Free the individual strings and then the string table itself. */
  for (i = 0 ; i < quark_set->g_quark_seq_id ; i++)
    {
      g_free(quark_set->g_quarks[i]) ;
    }
  g_free(quark_set->g_quarks) ;

  g_free(quark_set) ;

  return ;
}


/*! @} end of zmapGLibutils docs. */





/*
 *                 Internal functions.
 */


/* HOLDS: g_quark_global_lock */
static inline GQuark g_quark_new (ZMapQuarkSet quark_set, gchar *string)
{
  GQuark quark;

  /* Allocate another block of quark string pointers if needed. */
  if (quark_set->g_quark_seq_id % quark_set->block_size == 0)
    quark_set->g_quarks = g_renew(gchar*, quark_set->g_quarks,
				  quark_set->g_quark_seq_id + quark_set->block_size) ;

  quark_set->g_quarks[quark_set->g_quark_seq_id] = string ;
  quark_set->g_quark_seq_id++ ;
  quark = quark_set->g_quark_seq_id ;

  g_hash_table_insert(quark_set->g_quark_ht, string, GUINT_TO_POINTER(quark)) ;

  return quark ;
}



static void printCB(gpointer data, gpointer user_data)
{
  GQuark quark = GPOINTER_TO_INT(data) ;
  gboolean new_line = GPOINTER_TO_INT(user_data) ;

  printf("\"%s\"%s", g_quark_to_string(quark), (new_line ? "\n" : "\t")) ;

  return ;
}



/*
The function takes two gconstpointer arguments, the GList element's data and the given user data
*/

static gint caseCompareFunc(gconstpointer a, gconstpointer b)
{
  gint result = -1 ;
  GQuark list_quark = GPOINTER_TO_INT(a), str_quark = GPOINTER_TO_INT(b) ;

  result = g_ascii_strcasecmp(g_quark_to_string(list_quark), g_quark_to_string(str_quark)) ;

  return result ;
}





/* A hack really, this is GHRFunc() called from g_hash_table_find() which
 * will return TRUE on encountering the nth hash element thus stopping the find function and
 * returning the nth element of the hash.
 *
 * This function is NOT thread safe.
 *  */
static gboolean getNthHashElement(gpointer key, gpointer value, gpointer user_data)
{
  gboolean found_element = FALSE ;
  static int n = 0 ;
  int nth = *((int *)user_data) ;

  if (n == nth)
    {
      found_element = TRUE ;
      n = 0 ;
    }
  else
    n++ ;

  return found_element ;
}







/* A GHFunc() to copy the list attached to a hash entry. */
static void hashCopyListCB(gpointer key, gpointer value, gpointer user_data)
{
  GList *orig_list = (GList *)value ;
  GHashTable *new_hash = (GHashTable *)user_data ;
  GList *new_list = NULL ;

  new_list = g_list_copy(orig_list) ;

  g_hash_table_insert(new_hash, key, new_list) ;

  return ;
}



/* A GHFunc() to print the list attached to a hash entry. */
static void hashPrintListCB(gpointer key, gpointer value, gpointer user_data_unused)
{
  GList *styles_list = (GList *)value ;

  printf("\"%s\":  ", g_quark_to_string(GPOINTER_TO_INT(key))) ;

  zMap_g_list_quark_print(styles_list, "Styles", FALSE) ;

  return ;
}


/* A GHFunc() to print the quark attached to a hash entry. */
static void hashPrintTableCB(gpointer key, gpointer value, gpointer user_data)
{
  char *data_format_str = (char *)user_data ;
  char *value_str ;

  if (!data_format_str || g_ascii_strcasecmp(data_format_str, "pointer") == 0)
    {
      value_str = g_strdup_printf("%p", value) ;
    }
  else if (g_ascii_strcasecmp(data_format_str, "gquark") == 0)
    {
      value_str = g_strdup_printf("%s", g_quark_to_string(GPOINTER_TO_INT(value))) ;
    }
  else
    {
      value_str = g_strdup("unknown data value") ;
    }

  printf("\"%s\": %s\n", g_quark_to_string(GPOINTER_TO_INT(key)), value_str) ;

  g_free(value_str) ;

  return ;
}



/* A GDestroyNotify() to free Glists held as the data in a hash. */
static void destroyList(gpointer data)
{
  GList *list = (GList *)data ;

  g_list_free(list) ;

  return ;
}




static void mergehashCB(gpointer key, gpointer value, gpointer user_data)
{
  GHashTable *hash = (GHashTable *)user_data ;

  /* If the feature set id is not in the hash then copy its list of styles and add it to the
   * target hash. */
  if (!(g_hash_table_lookup(hash, key)))
    {
      GList *copy ;

      copy = g_list_copy((GList *)value) ;

      zMap_g_hashlist_insert_list(hash, GPOINTER_TO_INT(key), copy, TRUE) ;
    }


  return ;
}







