/*  File: zmapConfigIni.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk), Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <algorithm>
#include <string.h>/* memset */
#include <glib.h>

#include <ZMap/zmapStyle.hpp>
#include <ZMap/zmapStyleTree.hpp>
#include <ZMap/zmapConfigDir.hpp>
#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapConfigIni.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <zmapConfigIni_P.hpp>

using namespace std ;


#undef WITH_LOGGING



// Unnamed namespace for internal linkage
namespace
{

typedef struct
{
  const char *stanza_name;
  const char *stanza_type;
  GList *keys;
}ZMapConfigIniContextStanzaEntryStruct, *ZMapConfigIniContextStanzaEntry;


typedef struct
{
  ZMapConfigIniContext context;
  ZMapConfigIniContextStanzaEntry stanza;
  gboolean result;
} CheckRequiredKeysStruct, *CheckRequiredKeys;


/* Struct to pass data to the callback function to update a style */
typedef struct
{
  ZMapStyleTree *styles ;
  ZMapConfigIniContext context ;
} UpdateStyleDataStruct, *UpdateStyleData ;


/* Case-insensitive comparison of two strings (as GQuarks) */
class GQuarkNCaseCmp
{
public:
  GQuarkNCaseCmp(const GQuark &val) : m_val(val) {} ;

  bool operator()(const GQuark &compare_val) const
  {
    // convert to unique ids to be case-insensitive
    return zMapStyleCreateIDFromID(compare_val) == zMapStyleCreateIDFromID(m_val);
  }

private:
  GQuark m_val{0};
} ;


GList *copy_keys(ZMapConfigIniContextKeyEntryStruct *keys);
void check_required(gpointer list_data, gpointer user_data);
gboolean check_required_keys(ZMapConfigIniContext context,
                             ZMapConfigIniContextStanzaEntry stanza);
GType get_stanza_key_type(ZMapConfigIniContext context,
                          const char *stanza_name,
                          const char *stanza_type,
                          const char *key_name);
gint match_name_type(gconstpointer list_data, gconstpointer user_data);
void setErrorMessage(ZMapConfigIniContext context,char *error_message);
void context_update_style(ZMapFeatureTypeStyle style, gpointer data);


} // Unnamed namespace



/*
 *                  External Interface functions
 */


ZMapConfigIniContext zMapConfigIniContextCreate(const char *config_file)
{
  ZMapConfigIniContext context = NULL;

  if((context = g_new0(ZMapConfigIniContextStruct, 1)))
    {
      context->config = zMapConfigIniNew();

      context->config_read = zMapConfigIniReadAll(context->config, config_file) ;
    }

  return context;
}


ZMapConfigIniContext zMapConfigIniContextCreateType(const char *config_file, ZMapConfigIniFileType file_type)
{
  ZMapConfigIniContext context = NULL;

  if((context = g_new0(ZMapConfigIniContextStruct, 1)))
    {
      context->config = zMapConfigIniNew();

      switch (file_type)
        {
        case ZMAPCONFIG_FILE_USER:
          context->config_read = zMapConfigIniReadUser(context->config, config_file) ;
          break ;
        case ZMAPCONFIG_FILE_PREFS:
          context->config_read = zMapConfigIniReadPrefs(context->config) ;
          break ;
        case ZMAPCONFIG_FILE_SYS:
          context->config_read = zMapConfigIniReadSystem(context->config) ;
          break ;
        case ZMAPCONFIG_FILE_ZMAP:
          context->config_read = zMapConfigIniReadZmap(context->config) ;
          break ;
        case ZMAPCONFIG_FILE_STYLES:
          context->config_read = zMapConfigIniReadStyles(context->config, config_file) ;
          break ;
        case ZMAPCONFIG_FILE_BUFFER:
          context->config_read = zMapConfigIniReadBuffer(context->config, config_file) ;
          break ;

        default:
          break ;
        }
    }

  return context;
}


// all stanzas froma file, assumed to be of the same type
gchar **zMapConfigIniContextGetAllStanzaNames(ZMapConfigIniContext context)
{
  gchar **names = NULL;
  ZMapConfigIniFileType file_type = ZMAPCONFIG_FILE_NUM_TYPES ;
  int i = 0 ;

  for(i = 0; i < ZMAPCONFIG_FILE_NUM_TYPES; i++)
    {
      file_type = (ZMapConfigIniFileType)i ;

      if(context->config->key_file[file_type])
        break;
    }

  if(i < ZMAPCONFIG_FILE_NUM_TYPES)
    names = g_key_file_get_groups(context->config->key_file[file_type], NULL);

  return names ;
}


gboolean zMapConfigIniContextIncludeBuffer(ZMapConfigIniContext context, const char *buffer)
{
  gboolean result = FALSE;

  if(buffer && *buffer && strlen(buffer) > 0)
    result = zMapConfigIniReadBuffer(context->config, buffer);

  return result;
}

/* file can be a full path or a filename. */
gboolean zMapConfigIniContextIncludeFile(ZMapConfigIniContext context, const char *file, ZMapConfigIniFileType file_type)
{
  gboolean result = FALSE;

  if (file && *file)
    result = zMapConfigIniReadFileType(context->config, file, file_type) ;

  return result;
}


gchar *zMapConfigIniContextErrorMessage(ZMapConfigIniContext context)
{
  gchar *e = NULL ;

  zMapReturnValIfFail(context, e) ;

  e = context->error_message ;

  return e ;
}

gchar *zMapConfigIniContextKeyFileErrorMessage(ZMapConfigIniContext context)
{
  gchar *error_msg = NULL ;
  zMapReturnValIfFail(context, error_msg) ;

  error_msg = context->config->key_error[ZMAPCONFIG_FILE_STYLES]->message ;


  return error_msg ;
}


gboolean zMapConfigIniContextAddGroup(ZMapConfigIniContext context,
                                      const char *stanza_name, const char *stanza_type,
                                      ZMapConfigIniContextKeyEntryStruct *keys)
{
  ZMapConfigIniContextStanzaEntryStruct user_request = {NULL};
  gboolean not_exist = FALSE, result = FALSE;
  GList *entry_found = NULL;
  zMapReturnValIfFail(context, result) ;

  user_request.stanza_name = stanza_name;
  user_request.stanza_type = stanza_type;

  if(!(entry_found = g_list_find_custom(context->groups, &user_request, match_name_type)))
    not_exist = TRUE;

  if(not_exist && !context->error_message)
    {
      ZMapConfigIniContextStanzaEntry new_stanza = NULL;

      if((new_stanza = g_new0(ZMapConfigIniContextStanzaEntryStruct, 1)))
        {
          new_stanza->stanza_name = stanza_name;
          new_stanza->stanza_type = stanza_type;
          /* copy keys */
          new_stanza->keys = copy_keys(keys);

          context->groups  = g_list_append(context->groups, new_stanza);

          result = TRUE;

          /* unfortunately we can only check for required keys when we have a stanza name */
          /*
           * NOTE this function does more than it says on the can
           * addgroup apears to suggest that it add a stanza template data structure to the context
           * but if it has a name we get to read the file and check that required fields are there
           * so we add a group and return a failure if the content is wrong
           * even though we do not appear to be reading it yet.
           * calling code does not process the return code, so that's pointless
           */
          if(!(g_ascii_strcasecmp(stanza_name, "*") == 0))
            result = check_required_keys(context, new_stanza);
        }
    }

  return result;
}


/* Update the filename for the given config file. This is used to be able to export to a different
 * file from the original file */
void zMapConfigIniContextSetFile(ZMapConfigIniContext context, 
                                 ZMapConfigIniFileType file_type,
                                 const char *filename)
{
  zMapReturnIfFail(context && context->config) ;

  context->config->key_file_name[file_type] = g_quark_from_string(filename) ;
}


/* Create the key file for the given type, if it doesn't already exist */
void zMapConfigIniContextCreateKeyFile(ZMapConfigIniContext context, ZMapConfigIniFileType file_type)
{
  zMapReturnIfFail(context && context->config) ;

  if (!context->config->key_file[file_type])
    {
      context->config->key_file[file_type] = g_key_file_new() ;
    }
}


/* Update the given key file in the given context with the values from all the styles in the
 * given table */
void zMapConfigIniContextSetStyles(ZMapConfigIniContext context, ZMapConfigIniFileType file_type, gpointer data)
{
  zMapReturnIfFail(file_type == ZMAPCONFIG_FILE_STYLES) ;

  ZMapStyleTree *styles = (ZMapStyleTree*)data ;

  UpdateStyleDataStruct update_data = {styles, context} ;

  styles->foreach(context_update_style, &update_data) ;
}


void zMapConfigIniContextSetUnsavedChanges(ZMapConfigIniContext context, 
                                           ZMapConfigIniFileType file_type,
                                           const gboolean value)
{
  zMapReturnIfFail(context && context->config) ;

  context->config->unsaved_changes[file_type] = (unsigned int)value ;
}


gboolean zMapConfigIniContextSave(ZMapConfigIniContext context, ZMapConfigIniFileType file_type)
{
  gboolean saved = TRUE;
  zMapReturnValIfFail(context, FALSE);

  saved = zMapConfigIniSave(context->config, file_type);

  return saved;
}

ZMapConfigIniContext zMapConfigIniContextDestroy(ZMapConfigIniContext context)
{
  zMapReturnValIfFail(context, NULL) ;
  if(context->error_message)
    g_free(context->error_message);

  zMapConfigIniDestroy(context->config,TRUE);

  memset(context, 0, sizeof(ZMapConfigIniContextStruct));

  g_free(context);

  return NULL;
}



gboolean zMapConfigIniContextGetValue(ZMapConfigIniContext context,
                                      const char *stanza_name,
                                      const char *stanza_type,
                                      const char *key_name,
                                      GValue **value_out)
{
  gboolean obtained = FALSE;
  GValue *value = NULL;
  GType type = 0;

  zMapReturnValIfFail( context, obtained ) ;

  if(value_out)
    {
      type  = get_stanza_key_type(context, stanza_name, stanza_type, key_name);

      if((type != 0))
        {
          if((obtained = zMapConfigIniGetValue(context->config,
               stanza_name, key_name,
               &value, type)))
            *value_out = value;
          else
            {
              setErrorMessage(context,
              g_strdup_printf("failed to get value for %s, %s",
              stanza_name, key_name));
              *value_out = NULL;
            }
        }
      else
        {
          setErrorMessage(context,
          g_strdup_printf("failed to get type for %s, %s",
          stanza_name, key_name));
          *value_out = NULL;
        }
    }

  return obtained;
}


gboolean zMapConfigIniContextGetBoolean(ZMapConfigIniContext context,
                                        const char *stanza_name,
                                        const char *stanza_type,
                                        const char *key_name,
                                        gboolean *value)
{
  GValue *value_out = NULL;
  gboolean success = FALSE;

  if(zMapConfigIniContextGetValue(context,
  stanza_name, stanza_type,
  key_name, &value_out))
    {
      if(value)
        {
          if(G_VALUE_HOLDS_BOOLEAN(value_out))
            {
              *value  = g_value_get_boolean(value_out);
              success = TRUE;
            }

          g_value_unset(value_out);

          g_free(value_out);

          value_out = NULL;
        }
    }

  return success;
}

gboolean zMapConfigIniContextGetString(ZMapConfigIniContext context,
                                       const char *stanza_name,
                                       const char *stanza_type,
                                       const char *key_name,
                                       char **value)
{
  GValue *value_out = NULL;
  gboolean success = FALSE;

  if(zMapConfigIniContextGetValue(context,
                                  stanza_name, stanza_type,
                                  key_name,    &value_out))
    {
      if(value)
        {
          if(G_VALUE_HOLDS_STRING(value_out))
            {
              *value  = g_value_dup_string(value_out);
              success = TRUE;
            }

          g_value_unset(value_out);
          g_free(value_out);
          value_out = NULL;
        }
    }

  return success;
}

/* Same as string only expands any tilde in the filepath, if the expansion fails
 * then this call fails. */
gboolean zMapConfigIniContextGetFilePath(ZMapConfigIniContext context,
 const char *stanza_name,
 const char *stanza_type,
 const char *key_name,
 char **value)
{
  GValue *value_out = NULL ;
  gboolean success = FALSE ;

  if (zMapConfigIniContextGetValue(context, stanza_name, stanza_type, key_name, &value_out))
    {
      if (value)
        {
          if (G_VALUE_HOLDS_STRING(value_out))
            {
              char *value_str ;
              char *expanded_value ;

              value_str = (char *)g_value_get_string(value_out) ;

              if ((expanded_value = zMapExpandFilePath(value_str)))
                {
                  *value  = expanded_value ;

                  success = TRUE ;
                }
            }

          g_value_unset(value_out) ;
          g_free(value_out) ;
          value_out = NULL ;
        }
    }

  return success ;
}


gboolean zMapConfigIniContextGetInt(ZMapConfigIniContext context,
                                    const char *stanza_name,
                                    const char *stanza_type,
                                    const char *key_name,
                                    int  *value)
{
  GValue *value_out = NULL;
  gboolean success = FALSE;

  if(zMapConfigIniContextGetValue(context, stanza_name, stanza_type, key_name, &value_out))
    {
      if(value)
        {
          if(G_VALUE_HOLDS_INT(value_out))
            {
              *value  = g_value_get_int(value_out);
              success = TRUE;
            }

          g_value_unset(value_out);
          g_free(value_out);
          value_out = NULL;
        }
    }

  return success;
}

gboolean zMapConfigIniContextSetString(ZMapConfigIniContext context,
                                       ZMapConfigIniFileType file_type,
                                       const char *stanza_name,
                                       const char *stanza_type,
                                       const char *key_name,
                                       const char *value_str)
{
  GType type = 0;
  gboolean is_set = TRUE;

  type = get_stanza_key_type(context, stanza_name, stanza_type, key_name);

  if(type == G_TYPE_STRING)
    {
      GValue value = {0};

      g_value_init(&value, G_TYPE_STRING);
      
      /* Allow null values - represent them as empty strings */
      const char *value_str_non_null = value_str ? value_str : "" ;

      g_value_set_static_string(&value, value_str_non_null);

      is_set = zMapConfigIniContextSetValue(context, 
                                            file_type,
                                            stanza_name,
                                            key_name,
                                            &value);
    }

  return is_set;
}

gboolean zMapConfigIniContextSetInt(ZMapConfigIniContext context,
                                    ZMapConfigIniFileType file_type,
                                    const char *stanza_name,
                                    const char *stanza_type,
                                    const char *key_name,
                                    int   value_int)
{
  GType type = 0;
  gboolean is_set = TRUE;

  type = get_stanza_key_type(context, stanza_name, stanza_type, key_name);

  if(type == G_TYPE_INT)
    {
      GValue value = {0};

      g_value_init(&value, type);

      g_value_set_int(&value, value_int);

      is_set = zMapConfigIniContextSetValue(context, 
                                            file_type,
                                            stanza_name,
                                            key_name,
                                            &value);
    }

  return is_set;
}


gboolean zMapConfigIniContextSetBoolean(ZMapConfigIniContext context,
                                        ZMapConfigIniFileType file_type,
                                        const char *stanza_name,
                                        const char *stanza_type,
                                        const char *key_name,
                                        gboolean value_bool)
{
  GType type = 0;
  gboolean is_set = TRUE;

  type = get_stanza_key_type(context, stanza_name, stanza_type, key_name);

  if(type == G_TYPE_BOOLEAN)
    {
      GValue value = {0};

      g_value_init(&value, type);

      g_value_set_boolean(&value, value_bool);

      is_set = zMapConfigIniContextSetValue(context, 
                                            file_type,
                                            stanza_name,
                                            key_name, 
                                            &value);
    }

  return is_set;
}

gboolean zMapConfigIniContextSetValue(ZMapConfigIniContext context,
                                      ZMapConfigIniFileType file_type,
                                      const char *stanza_name,
                                      const char *key_name,
                                      GValue *value)
{
  gboolean is_set = FALSE ;
  zMapReturnValIfFail(context, is_set ) ;

  zMapConfigIniSetValue(context->config, file_type, stanza_name, key_name, value);

  is_set = TRUE ;

  return is_set;
}


/*
 * Write a named stanza from values in a hash table
 * NOTE: this function operates differently from normal ConfigIni in that we do not know
 *  the names of the keys in the stanza and cannot create a struct to hold these and thier values
 * So instead we have to use GLib directly.
 * This is a simple generic table of quark->quark
 * used for [featureset_styles] [GFF_source] and [column_styles]
 */
void zMapConfigIniSetQQHash(ZMapConfigIniContext context, ZMapConfigIniFileType file_type, 
                            const char *stanza, GHashTable *ghash)
{
  zMapReturnIfFail(context && context->config) ;

  GKeyFile *gkf = context->config->key_file[file_type] ;

  if (gkf)
    {
      /* Loop through all entries in the hash table */
      GList *iter = NULL ;
      gpointer key = NULL,value = NULL;

      zMap_g_hash_table_iter_init(&iter, ghash) ;

      while(zMap_g_hash_table_iter_next(&iter, &key, &value))
        {
          if (key && value)
            {
              const char *key_str = g_quark_to_string(GPOINTER_TO_INT(key)) ;
              const char *value_str = g_quark_to_string(GPOINTER_TO_INT(value)) ;

              g_key_file_set_string(gkf, stanza, key_str, value_str) ;
            }
        }
    }
}


GKeyFile *zMapConfigIniGetKeyFile(ZMapConfigIniContext context,
                                  ZMapConfigIniFileType file_type)
{
  GKeyFile *result = NULL ;

  if (context && context->config)
    result = context->config->key_file[file_type] ;

  return result ;
}


/* Given two lists of column names, merge the src list into the dest list. Any items that are not
 * in dest list are added as close to a known position as possible based on adjacent items'
 * positions. If unique is true, the dest_list must be a list of unique ids and the result will 
 * be a list of unique ids; otherwise, original ids are used */
list<GQuark> zMapConfigIniMergeColumnsLists(list<GQuark> &src_list, 
                                            list<GQuark> &dest_list,
                                            const bool unique)
{
  if (dest_list.size() < 1)
    {
      // Just use the src list
      if (unique)
        {
          // convert to unique ids
          for (auto src_iter = src_list.begin(); src_iter != src_list.end(); ++src_iter)
            {
              /* Convert src id to a unique id */
              GQuark src = zMapStyleCreateID(g_quark_to_string(*src_iter)) ;
              dest_list.push_back(src) ;
            }
        }
      else
        {
          dest_list = src_list ;
        }
    }
  else if (src_list.size() > 0)
    {
      // Merge src into dest. 

      // Loop through all items in src list.
      auto src_iter_prev = src_list.begin();
      for (auto src_iter = src_list.begin(); src_iter != src_list.end(); ++src_iter)
        {
          /* Convert src id to a unique id */
          GQuark src = unique ? zMapStyleCreateIDFromID(*src_iter) : *src_iter ;

          // See if this item is already in the dest list
          auto dest_iter = find_if(dest_list.begin(), dest_list.end(), GQuarkNCaseCmp(src)) ;

          if (dest_iter == dest_list.end())
            {
              // It's not in the dest list so we need to add it. Add it into dest_list just after
              // the previous item from src_list to try to keep things grouped together.

              // If the prev item is the same as the current item that means it's the first in the
              // list. 
              if (src_iter_prev == src_iter)
                {
                  // There is no previous item. Add the src item to the start of the dest_list.
                  dest_list.push_front(src) ;
                }
              else
                {
                  // Find the previous src item in the dest list (it should be there because we
                  // have processed it already).
                  dest_iter = find(dest_list.begin(), dest_list.end(), *src_iter_prev) ;

                  // Add the src item just after the prev item.
                  if (dest_iter != dest_list.end())
                    {
                      ++dest_iter ;
                      dest_list.insert(dest_iter, src);
                    }
                }
            }
          else
            {
              // It's already in the dest list so we don't need to add it. However, if the case is
              // different then use the case from the src
              *dest_iter = src ;
            }

          src_iter_prev = src_iter ;
        }
    }

  return dest_list;
}




/*
 *                   Internal functions
 */

// Unnamed namespace
namespace
{

void setErrorMessage(ZMapConfigIniContext context,char *error_message)
{
  zMapReturnIfFail(context) ;

  if (context->error_message)
    g_free(context->error_message);

  context->error_message = error_message;

  return ;
}

void check_required(gpointer list_data, gpointer user_data)
{
  CheckRequiredKeys checking_data = (CheckRequiredKeys)user_data;
  ZMapConfigIniContextKeyEntry key = (ZMapConfigIniContextKeyEntry)list_data;
  ZMapConfigIniContextStanzaEntry stanza;
  ZMapConfigIniContext context;

  context = checking_data->context;
  stanza  = checking_data->stanza;

  if(checking_data->result && !context->error_message && key->required)
    {
      GValue *value = NULL;

      if(zMapConfigIniGetValue(context->config,
                               stanza->stanza_name,
                               key->key,
                               &value, key->type))
        {
          checking_data->result = TRUE;
          g_value_unset(value);
          g_free(value);
        }
      else
        {
          checking_data->result = FALSE;
          setErrorMessage(context,
          g_strdup_printf("Failed to get required key '%s' in stanza %s",
          key->key, stanza->stanza_name));
        }
    }

  return ;
}

gboolean check_required_keys(ZMapConfigIniContext context, ZMapConfigIniContextStanzaEntry stanza)
{
  CheckRequiredKeysStruct checking_data = {};

  checking_data.context = context;
  checking_data.stanza  = stanza;
  checking_data.result  = TRUE;

  g_list_foreach(stanza->keys, check_required, &checking_data);

  return checking_data.result;
}



/* here we want to prefer exact matches of name and type */
/* then we'll only match types with a name == * already in the list */
gint match_name_type(gconstpointer list_data, gconstpointer user_data)
{
  ZMapConfigIniContextStanzaEntry query = (ZMapConfigIniContextStanzaEntry)user_data;
  ZMapConfigIniContextStanzaEntry list_entry = (ZMapConfigIniContextStanzaEntry)list_data;
  gint result = -1;

  /* Look for exact name matches first */
  if(g_ascii_strcasecmp( query->stanza_name, list_entry->stanza_name ) == 0)
    {
      /* Checking that the types match.  Necessary? */
      if(g_ascii_strcasecmp( query->stanza_type, list_entry->stanza_type ) == 0)
        {
          query->keys = list_entry->keys;
          result = 0;           /* zero == found */
        }
      else
        result = 1;
    }
  /* not an exact name match, do the types match? */
  else if(g_ascii_strcasecmp( query->stanza_type, list_entry->stanza_type ) == 0)
    {
      /* make sure the stanza_name == * in the list the context
       * has. This is so stanzas with different names can have the
       * same basic type. e.g. the same list of keys/info/G_TYPE data
       * for keys... */
      if(g_ascii_strcasecmp(list_entry->stanza_name, "*") == 0)
        {
          query->keys = list_entry->keys;
          result = 0;           /* zero == found */
        }
      else
        result = 1;
    }
  else
    result = -1;

  return result;
}



gint match_key(gconstpointer list_data, gconstpointer user_data)
{
  ZMapConfigIniContextKeyEntry query = (ZMapConfigIniContextKeyEntry)user_data;
  ZMapConfigIniContextKeyEntry list_key = (ZMapConfigIniContextKeyEntry)list_data;
  gint result = -1;

  if(g_ascii_strcasecmp(query->key, list_key->key) == 0)
    {
      query->type = list_key->type;
      result = 0;
    }

  return result ;
}


GType get_stanza_key_type(ZMapConfigIniContext context,
                                 const char *stanza_name,
                                 const char *stanza_type,
                                 const char *key_name)
{
  ZMapConfigIniContextStanzaEntryStruct user_request = {NULL};
  GList *entry_found = NULL;
  GType type = 0;

  user_request.stanza_name = stanza_name;
  user_request.stanza_type = stanza_type;

  if((entry_found = g_list_find_custom(context->groups, &user_request, match_name_type)))
    {
      ZMapConfigIniContextKeyEntryStruct key_request = {};
      GList *key_found = NULL;

      key_request.key = key_name;

      if((key_found = g_list_find_custom(user_request.keys, &key_request, match_key)))
        type = key_request.type;
    }

  return type;
}

GList *copy_keys(ZMapConfigIniContextKeyEntryStruct *keys)
{
  GList *new_keys = NULL;

  while(keys && keys->key)
    {
      ZMapConfigIniContextKeyEntry k = NULL;

      if((k = g_new0(ZMapConfigIniContextKeyEntryStruct, 1)))
        {
          k->key      = keys->key;
          k->type     = keys->type;
          k->required = keys->required;
          k->set_property = keys->set_property;
          new_keys    = g_list_append(new_keys, k);
        }

      keys++;
    }

  return new_keys;
}


/* Check if there are differences between the given parameter value and its parent value. Returns
 * true if there are differences. It may update the value pointer to a new string if there are
 * partial differences, e.g. if some colours in a colours string but others do not then it will
 * only return the differing colours. */
gboolean param_value_diffs(char **value, const char *parent_value, const char *param_name)
{
  gboolean diffs = TRUE ;
  
  if (value && *value && parent_value)
    {
      if (strlen(parent_value) == strlen(*value) && 
          strncasecmp(parent_value, *value, strlen(*value)) == 0)
        {
          /* Strings are identical, so there are no diffs */
          diffs = FALSE ;
        }
      else
        {
          /* Strings differ, but if it's a colours string then we only want to export the
           * specific colour types that are different, rather than the whole string */
          if (strcmp(param_name, ZMAPSTYLE_PROPERTY_COLOURS) == 0 || 
              strcmp(param_name, ZMAPSTYLE_PROPERTY_TRANSCRIPT_CDS_COLOURS) == 0)
            {
              /* Separate value into a list colour strings and search for each individual colour
               * string (i.e. "normal border black") in the parent value */
              GList *list = zMapConfigString2QuarkGList(*value, FALSE);
              GString *diffs_string = NULL ;
              
              for (GList *item = list ; item ; item = item->next)
                {
                  const char *cur_string = g_quark_to_string(GPOINTER_TO_INT(item->data)) ;

                  /* If it doesn't exist in the parent, then include it in the diffs string */
                  if (!strstr(parent_value, cur_string))
                    {
                      if (!diffs_string)
                        diffs_string = g_string_new(cur_string) ;
                      else
                        g_string_append_printf(diffs_string, " ; %s", cur_string) ;
                    }
                }

              if (diffs_string)
                {
                  /* Replace original string with the string containing just the diffs */
                  g_free(*value) ;
                  *value = g_string_free(diffs_string, FALSE) ;
                  diffs = TRUE ;
                }
            }
          else
            {
              /* Not a colours string, so there are straight diffs */
              diffs = TRUE ;
            }
        }
    }

  return diffs ;
}


/* Update the context given in the user data with the style given in the value. This loops
 * through all the parameters that are set in the style and updates them in the context's  */
void context_update_style(ZMapFeatureTypeStyle style, gpointer data)
{
  UpdateStyleData update_data = (UpdateStyleData)data ;

  zMapReturnIfFail(style && 
                   style->unique_id &&
                   update_data && 
                   update_data->context && 
                   update_data->context->config &&
                   update_data->context->config->key_file[ZMAPCONFIG_FILE_STYLES]) ;
  
  /* Loop through each possible parameter type */
  for (int param_id = STYLE_PROP_NONE + 1 ; param_id < _STYLE_PROP_N_ITEMS; ++param_id)
    {
      const char *param_name = zmapStyleParam2Name((ZMapStyleParamId)param_id) ;
      gboolean ok = TRUE ;

      /* Check for various params that we don't want to export */
      if (!param_name ||
          strcmp(param_name, ZMAPSTYLE_PROPERTY_NAME) == 0 ||
          strcmp(param_name, ZMAPSTYLE_PROPERTY_DESCRIPTION) == 0 ||
          strcmp(param_name, ZMAPSTYLE_PROPERTY_IS_SET) == 0 ||
          strcmp(param_name, ZMAPSTYLE_PROPERTY_DISPLAYABLE) == 0 ||
          strcmp(param_name, ZMAPSTYLE_PROPERTY_ORIGINAL_ID) == 0 ||
          strcmp(param_name, ZMAPSTYLE_PROPERTY_UNIQUE_ID) == 0 ||
          strcmp(param_name, ZMAPSTYLE_PROPERTY_GFF_SOURCE) == 0 ||
          strcmp(param_name, ZMAPSTYLE_PROPERTY_GLYPH_SHAPE) == 0 
          )
        {
           ok = FALSE ;
        }

      if (ok)
        {
          char *value = zMapStyleGetValueAsString(style, (ZMapStyleParamId)param_id) ;

          if (value)
            {
              /* If the style has a parent with the same value then only include that value in the
               * parent, i.e. exclude it from this child. Always include the parent-style value
               * itself though. */
              if (ok && strcmp(param_name, "parent-style") != 0 && style->parent_id)
                {
                  ZMapFeatureTypeStyle parent_style = update_data->styles->find_style(style->parent_id) ;

                  if (parent_style)
                    {
                      /* Get the value of this field in the parent (or the parent's parent etc.) */
                      char *parent_value = zMapStyleGetValueAsString(parent_style, (ZMapStyleParamId)param_id) ;

                      /* If child value is the same as parent value, flag that we don't want to continue */
                      ok = param_value_diffs(&value, parent_value, param_name) ;

                      if (parent_value)
                        g_free(parent_value) ;
                    }
                }

              if (ok)
                {
                  const char *stanza_name = g_quark_to_string(style->original_id) ;

                  if (!stanza_name)
                    stanza_name = g_quark_to_string(style->unique_id) ;

                  g_key_file_set_string(update_data->context->config->key_file[ZMAPCONFIG_FILE_STYLES], stanza_name, param_name, value);
                }

              g_free(value) ;
            }
        }
    }

  return ;
}


} // Unnamed namespace
