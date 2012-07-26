/*  File: zmapConfigIni.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk) after Roy Storey (rds@sanger.ac.uk)
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
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <string.h>		/* memset */
#include <glib.h>
#include <zmapConfigIni_P.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapConfigIni.h>


#undef WITH_LOGGING

static void zmapConfigIniContextSetErrorMessage(ZMapConfigIniContext context,char *error_message);


void zMapConfigIniGetStanza(ZMapConfigIni config, char *stanza_name)
{
  return ;
}


// all stanzas froma file, assumed to be of the same type
gchar **zMapConfigIniContextGetAllStanzaNames(ZMapConfigIniContext context)
{
  gchar **names = NULL;

  names = g_key_file_get_groups(context->config->extra_key_file,NULL);
  return names ;
}


typedef struct
{
  char *stanza_name;
  char *stanza_type;
  GList *keys;
}ZMapConfigIniContextStanzaEntryStruct, *ZMapConfigIniContextStanzaEntry;


typedef struct
{
  ZMapConfigIniContext context;
  ZMapConfigIniContextStanzaEntry stanza;
  gboolean result;
} CheckRequiredKeysStruct, *CheckRequiredKeys;



static GList *copy_keys(ZMapConfigIniContextKeyEntryStruct *keys);
static void check_required(gpointer list_data, gpointer user_data);
static gboolean check_required_keys(ZMapConfigIniContext context, 
				    ZMapConfigIniContextStanzaEntry stanza);
static GType get_stanza_key_type(ZMapConfigIniContext context,
                         char *stanza_name,
                         char *stanza_type,
                         char *key_name);
static gint match_name_type(gconstpointer list_data, gconstpointer user_data);



ZMapConfigIniContext zMapConfigIniContextCreate(char *config_file)
{
  ZMapConfigIniContext context = NULL;

  if((context = g_new0(ZMapConfigIniContextStruct, 1)))
    {
      context->config = zMapConfigIniNew();
      context->config_read = zMapConfigIniReadAll(context->config, config_file) ;
    }

  return context;
}


gboolean zMapConfigIniContextIncludeBuffer(ZMapConfigIniContext context, char *buffer)
{
  gboolean result = FALSE;

  if(buffer && *buffer && strlen(buffer) > 0)
    result = zMapConfigIniReadBuffer(context->config, buffer);

  return result;
}

/* file can be a full path or a filename. */
gboolean zMapConfigIniContextIncludeFile(ZMapConfigIniContext context, char *file)
{
  gboolean result = FALSE;

  if (file && *file)
    result = zMapConfigIniReadFile(context->config, file) ;

  return result;
}


gchar *zMapConfigIniContextErrorMessage(ZMapConfigIniContext context)
{
  gchar *e = NULL;
  e = context->error_message;
  return e;
}

static void zmapConfigIniContextSetErrorMessage(ZMapConfigIniContext context,char *error_message)
{
  if(context->error_message)
      g_free(context->error_message);
  context->error_message = error_message;
}



gboolean zMapConfigIniContextAddGroup(ZMapConfigIniContext context, 
				      char *stanza_name, char *stanza_type,
				      ZMapConfigIniContextKeyEntryStruct *keys)
{
  ZMapConfigIniContextStanzaEntryStruct user_request = {NULL};
  gboolean not_exist = FALSE, result = FALSE;
  GList *entry_found = NULL;

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
	  if(!(g_ascii_strcasecmp(stanza_name, "*") == 0))
	    result = check_required_keys(context, new_stanza);
	}
    }

  return result;
}


static void check_required(gpointer list_data, gpointer user_data)
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
        zmapConfigIniContextSetErrorMessage(context,
                  g_strdup_printf("Failed to get required key '%s' in stanza %s",
                  key->key, stanza->stanza_name));
      }
    }

  return ;
}

static gboolean check_required_keys(ZMapConfigIniContext context, ZMapConfigIniContextStanzaEntry stanza)
{
  CheckRequiredKeysStruct checking_data = {};

  checking_data.context = context;
  checking_data.stanza  = stanza;
  checking_data.result  = TRUE;

  g_list_foreach(stanza->keys, check_required, &checking_data);

  return checking_data.result;
}


gboolean zMapConfigIniContextSave(ZMapConfigIniContext context)
{
  gboolean saved = TRUE;

  saved = zMapConfigIniSaveUser(context->config);

  return saved;
}

ZMapConfigIniContext zMapConfigIniContextDestroy(ZMapConfigIniContext context)
{
  if(context->error_message)
    g_free(context->error_message);

  zMapConfigIniDestroy(context->config,TRUE);

  memset(context, 0, sizeof(ZMapConfigIniContextStruct));

  g_free(context);

  return NULL;
}



gboolean zMapConfigIniContextGetValue(ZMapConfigIniContext context, 
                              char *stanza_name,
                              char *stanza_type,
                              char *key_name,
                              GValue **value_out)
{
  gboolean obtained = FALSE;
  GValue *value = NULL;
  GType type = 0;

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
            zmapConfigIniContextSetErrorMessage(context,
                        g_strdup_printf("failed to get value for %s, %s",
                        stanza_name, key_name));
            *value_out = NULL;
          }
      }
      else
      {
        zmapConfigIniContextSetErrorMessage(context,
                  g_strdup_printf("failed to get type for %s, %s", 
                  stanza_name, key_name));
        *value_out = NULL;
      }
    }

  return obtained;
}


gboolean zMapConfigIniContextGetBoolean(ZMapConfigIniContext context,
                               char *stanza_name,
                               char *stanza_type,
                               char *key_name,
                               gboolean *value)
{
  GValue *value_out = NULL;
  gboolean success = FALSE;

  if(zMapConfigIniContextGetValue(context, 
                          stanza_name, stanza_type, 
                          key_name,    &value_out))
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
				       char *stanza_name,
				       char *stanza_type,
				       char *key_name,
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
					 char *stanza_name,
					 char *stanza_type,
					 char *key_name,
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
				    char *stanza_name,
				    char *stanza_type,
				    char *key_name,
				    int  *value)
{
  GValue *value_out = NULL;
  gboolean success = FALSE;

  if(zMapConfigIniContextGetValue(context, 
                          stanza_name, stanza_type, 
                          key_name,    &value_out))
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
                               char *stanza_name,
                               char *stanza_type,
                               char *key_name,
                               char *value_str)
{
  GType type = 0;
  gboolean set = TRUE;

  type = get_stanza_key_type(context, stanza_name, stanza_type, key_name);

  if(type == G_TYPE_STRING)
    {
      GValue value = {0};

      g_value_init(&value, G_TYPE_STRING);

      g_value_set_static_string(&value, value_str);

      set = zMapConfigIniContextSetValue(context, stanza_name,
                               key_name, &value);
    }
  
  return set;
}

gboolean zMapConfigIniContextSetInt(ZMapConfigIniContext context,
                            char *stanza_name,
                            char *stanza_type,
                            char *key_name,
                            int   value_int)
{
  GType type = 0;
  gboolean set = TRUE;

  type = get_stanza_key_type(context, stanza_name, stanza_type, key_name);

  if(type == G_TYPE_INT)
    {
      GValue value = {0};

      g_value_init(&value, type);

      g_value_set_int(&value, value_int);

      set = zMapConfigIniContextSetValue(context, stanza_name,
                               key_name, &value);
    }
  
  return set;
}


gboolean zMapConfigIniContextSetBoolean(ZMapConfigIniContext context,
                              char *stanza_name,
                              char *stanza_type,
                              char *key_name,
                              gboolean value_bool)
{
  GType type = 0;
  gboolean set = TRUE;

  type = get_stanza_key_type(context, stanza_name, stanza_type, key_name);

  if(type == G_TYPE_BOOLEAN)
    {
      GValue value = {0};

      g_value_init(&value, type);

      g_value_set_boolean(&value, value_bool);

      set = zMapConfigIniContextSetValue(context, stanza_name,
                               key_name, &value);
    }
  
  return set;
}

gboolean zMapConfigIniContextSetValue(ZMapConfigIniContext context,
                              char *stanza_name,
                              char *key_name,
                              GValue *value)
{
  gboolean set = TRUE;

  zMapConfigIniSetValue(context->config, stanza_name, key_name, value);

  return set;
}


/* here we want to prefer exact matches of name and type */
/* then we'll only match types with a name == * already in the list */
static gint match_name_type(gconstpointer list_data, gconstpointer user_data)
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



static gint match_key(gconstpointer list_data, gconstpointer user_data)
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


static GType get_stanza_key_type(ZMapConfigIniContext context,
                         char *stanza_name,
                         char *stanza_type,
                         char *key_name)
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

static GList *copy_keys(ZMapConfigIniContextKeyEntryStruct *keys)
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


