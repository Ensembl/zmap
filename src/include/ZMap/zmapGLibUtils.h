/*  File: zmapGLibUtils.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 *
 * Description: Extra Home grown functions that are compatible with
 *              glib but not included with their distribution.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_GLIBUTILS_H
#define ZMAP_GLIBUTILS_H

#include <glib.h>



/* Represents a quark set. */
typedef struct _ZMapQuarkSetStruct *ZMapQuarkSet ;


typedef enum
  {
    ZMAP_GLIST_FORWARD,
    ZMAP_GLIST_REVERSE
  } ZMapGListDirection;


typedef gboolean (*ZMapGFuncCond)(gpointer data, gpointer user_data) ;



typedef struct ZMapGHashIterStructType
{
  gpointer key;
  gpointer value;
} ZMapGHashIterStruct, *ZMapGHashIter;






gchar *zMap_g_remove_char(char *string, char ch) ;
gchar *zMap_g_ascii_strstrcasecmp(const gchar *haystack, const gchar *needle) ;
gboolean zMap_g_string_replace(GString *string, const char *target, char *source) ;

void zMap_g_list_foreach_reverse(GList *list, GFunc func, gpointer user_data);
void zMap_g_list_foreach_directional(GList *list, GFunc func, gpointer user_data,
                                     ZMapGListDirection forward);
gboolean zMap_g_list_cond_foreach(GList *list, ZMapGFuncCond func, gpointer user_data) ;
GList *zMap_g_list_move(GList *list, gpointer user_data, gint new_index) ;
GList *zMap_g_list_grep(GList **list_inout, gpointer data, GCompareFunc func);
GList *zMap_g_list_insert_list_after(GList *recipient, GList *donor, int point, gboolean copy_donor) ;
GList *zMap_g_list_lower(GList *move, int positions);
GList *zMap_g_list_raise(GList *move, int positions);
GList *zMap_g_list_split(GList *list, GList *new_list_head) ;
GList *zMap_g_list_append_unique(GList *list, gpointer data);
GList *zMap_g_list_merge(GList *a, GList *b);

void zMap_g_list_quark_print(GList *quark_list, const char *list_name, gboolean new_line) ;
GList *zMap_g_list_find_quark(GList *list, GQuark str_quark) ;
gchar *zMap_g_list_quark_to_string(GList *l, const char *delimiter);
GList *zMap_g_list_remove_quarks(GList *quark_target_list, GList *quarks) ;

gpointer zMap_g_hash_table_nth(GHashTable *hash_table, int nth) ;
void zMap_g_hash_table_print(GHashTable *hash_table, const char *data_format_str) ;
void zMap_g_hash_table_iter_init(GList **iter, GHashTable *h);
gboolean zMap_g_hash_table_iter_next(GList **iter,gpointer *key, gpointer *value);
void zMap_g_hash_table_iter_free(GList **list);       // not implemented
void  zMap_g_hash_table_get_keys(GList **iter, GHashTable *h);
void  zMap_g_hash_table_get_data(GList **iter, GHashTable *h);

GHashTable *zMap_g_hashlist_create(void) ;
void zMap_g_hashlist_insert(GHashTable *hashlist, GQuark key, gpointer value) ;
void zMap_g_hashlist_insert_list(GHashTable *hashlist, GQuark key, GList *key_values, gboolean replace) ;
GHashTable *zMap_g_hashlist_copy(GHashTable *orig_hashlist) ;
void zMap_g_hashlist_merge(GHashTable *in_out, GHashTable *in) ;
void zMap_g_hashlist_print(GHashTable *hashlist) ;
void zMap_g_hashlist_destroy(GHashTable *hashlist) ;

gpointer zMap_g_datalist_first(GData **datalist);
gint zMap_g_datalist_length(GData **datalist);



/* Returns a pointer to an element of the array instead of the element itself. */
#define zMap_g_array_index_ptr(a, t, i)      (&(((t*) (a)->data) [(i)]))

gpointer zMap_g_array_element(GArray **array_inout, guint index) ;


/* This is our version of the Glib Quark code (see www.gtk.org for more information).
 * Our version allows you to throw away the quark table which is important for us
 * as may have thousands of quarks.
 */
ZMapQuarkSet zMap_g_quark_create_set(guint block_size) ;
GQuark zMap_g_quark_try_string(ZMapQuarkSet quark_set, gchar *string);
GQuark zMap_g_quark_from_string(ZMapQuarkSet quark_set, gchar *string);
gchar *zMap_g_quark_to_string(ZMapQuarkSet quark_set, GQuark quark) ;
void zMap_g_quark_destroy_set(ZMapQuarkSet quark_set) ;

#endif /* !ZMAP_GLIBUTILS_H */
