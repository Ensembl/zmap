/*  File: zmapConfigKey.c
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

#include <string.h>/* memset */
#include <glib.h>

#include <zmapConfigIni_P.hpp>
#include <ZMap/zmapConfigDir.hpp>
#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapConfigIni.hpp>



#define FILE_COUNT 5
#define IMPORTANT_COUNT 2



static gboolean zmapConfigIniGetValueFull(ZMapConfigIni config,
                                          const char * stanza_name,
                                          const char * key_name,
                                          GValue *value,
                                          GType type,
                                          gboolean merge_files);
static gboolean get_merged_key_value(ZMapConfigIni config,
                                     const char * stanza_name,
                                     const char * key_name,
                                     GValue *value,
                                     GType type);
static gboolean get_value(GKeyFile *key_file,
                          const char *stanza_name,
                          const char *key_name,
                          GValue *value,
                          GType type,
                          gboolean allow_overwrite,
                          gboolean clear_if_not_exist);



gboolean zMapConfigIniGetUserValue(ZMapConfigIni config,
                                   const char * stanza_name,
                                   const char * key_name,
                                   GValue **value_out,
                                   GType type)
{
  gboolean success = FALSE;
  GValue *value = NULL;

  /* Get the value from _only_ the user config file.  Do not merge any
   * other files that might be present. */
  if(value_out && (value   = g_new0(GValue, 1)))
    {
      value   = g_value_init(value, type);
      success = zmapConfigIniGetValueFull(config, stanza_name, key_name,
                                  value, type, FALSE);
      if(success)
        *value_out = value;
      else
        {
          g_value_unset(value);
          g_free(value);
        }
    }

  return success;
}

gboolean zMapConfigIniGetValue(ZMapConfigIni config,
       const char * stanza_name,
       const char * key_name,
       GValue **value_out,
       GType type)
{
  gboolean success = FALSE;
  GValue *value = NULL;

  /* Get the value from all files that exist, merging as appropriate. */
  if(value_out && (type != 0) && (value = g_new0(GValue, 1)))
    {
      value   = g_value_init(value, type);
      success = zmapConfigIniGetValueFull(config, stanza_name, key_name,
                                          value, type, TRUE);
      if(success)
        *value_out = value;
      else
        {
          g_value_unset(value);
          g_free(value);
        }
    }

  return success;
}


void zMapConfigIniSetValue(ZMapConfigIni config,
                           ZMapConfigIniFileType file_type,
                           const char *stanza_name,
                           const char *key_name,
                           GValue *value)
{
  GKeyFile *key_file = NULL;
  zMapReturnIfFail(config) ; 

  if((key_file = config->key_file[file_type]))
    {
      GType type = 0;
      type = G_VALUE_TYPE(value);

      switch(type)
        {
        case G_TYPE_STRING:
          {
            char *string_value = NULL;
            string_value = (char *)g_value_get_string(value);

            if (string_value)
              g_key_file_set_string(key_file, stanza_name, key_name, string_value);
          }
          break;
        case G_TYPE_INT:
          {
            int int_value = 0;
            int_value = g_value_get_int(value);
            g_key_file_set_integer(key_file, stanza_name, key_name, int_value);
          }
          break;
        case G_TYPE_BOOLEAN:
          {
            gboolean bool_value = FALSE;
            bool_value = g_value_get_boolean(value);
            g_key_file_set_boolean(key_file, stanza_name, key_name, bool_value);
          }
          break;
        case G_TYPE_DOUBLE:
          {
            double double_value = FALSE;
            double_value = g_value_get_double(value);
            g_key_file_set_double(key_file, stanza_name, key_name, double_value);
          }
          break;
        default:
          break;
        }
    }

  return ;
}


/* internal calls */


static gboolean zmapConfigIniGetValueFull(ZMapConfigIni config,
  const char * stanza_name,
  const char * key_name,
  GValue *value,
  GType type,
  gboolean merge_files)
{
  gboolean success = FALSE;

  zMapReturnValIfFail(value, success) ;
  zMapReturnValIfFail(config, success) ; 

  if((!merge_files) && (config->key_file[ZMAPCONFIG_FILE_USER] != NULL))
    {
      success = get_value(config->key_file[ZMAPCONFIG_FILE_USER], stanza_name, key_name, value, type, TRUE, TRUE);
    }
  else
    {
      success = get_merged_key_value(config, stanza_name, key_name, value, type);
    }

  return success;
}


/* dynamic merge, versus static (initial) merge??? */

/* dynamic is going to take a lot more time, but might be more
   accurate and easy to trace which setting came from where, at the
   expense of having multiple GKeyFile instances around */

static gboolean get_merged_key_value(ZMapConfigIni config,
     const char * stanza_name,
     const char * key_name,
     GValue *value,
     GType type)
{
  GKeyFile *important_files[IMPORTANT_COUNT];
  gboolean key_found = FALSE;
  gboolean honour_important = TRUE;
  char *important_key = NULL;
  int i;

  if(honour_important)
    {
      important_key = g_strdup_printf("!%s", key_name);

      /* get the system first, then the zmap one */
      important_files[1] = config->key_file[ZMAPCONFIG_FILE_SYS];
      important_files[0] = config->key_file[ZMAPCONFIG_FILE_ZMAP];

      for(i = 0; key_found == FALSE && i < IMPORTANT_COUNT; i++)
        {
          if(important_files[i])
            {
              if(get_value(important_files[i], stanza_name, key_name, value, type, TRUE, FALSE))
                key_found = TRUE;
            }
        }
        
      if(important_key)
        g_free(important_key);
    }

  if(key_found == FALSE)
    {
      for(i = 0; key_found == FALSE && i < FILE_COUNT; i++)
        {
          ZMapConfigIniFileType file_type = (ZMapConfigIniFileType)i ;

          if(config->key_file[file_type])
            {
              if(get_value(config->key_file[file_type], stanza_name, key_name, value, type, TRUE, FALSE))
                key_found = TRUE;
            }
        }
    }

  return key_found;
}

static gboolean get_value(GKeyFile *key_file,
                          const char *stanza_name,
                          const char *key_name,
                          GValue *value,
                          GType type,
                          gboolean allow_overwrite,
                          gboolean clear_if_not_exist)
{
  char *tmp_string = NULL, **tmp_vector = NULL;
  int tmp_integer;
  double tmp_double;
  GError *error = NULL;
  gboolean value_set = FALSE;
  gboolean tmp_bool;

  g_return_val_if_fail(value != NULL, FALSE);
  g_return_val_if_fail(G_VALUE_TYPE(value) == type, FALSE);

  if(key_file)
    {
      switch(type)
        {
        case G_TYPE_INT:
          tmp_integer = g_key_file_get_integer (key_file,
        stanza_name,
        key_name,
        &error);
          break;
        case G_TYPE_DOUBLE:
          tmp_double = g_key_file_get_double (key_file,
              stanza_name,
              key_name,
              &error);
                if(error && error->code == G_KEY_FILE_ERROR_INVALID_VALUE)
                {
                    // there is a bug in glib that reports an error on -ve values
                    // although the data gets through
                    g_error_free(error);
                    error = NULL;
                }
          break;
        case G_TYPE_BOOLEAN:
          tmp_bool = g_key_file_get_boolean(key_file,
            stanza_name,
            key_name,
            &error);
          break;
        case G_TYPE_POINTER:
          {
            gsize vector_length ;
        
            tmp_vector = g_key_file_get_string_list(key_file,
            stanza_name,
            key_name,
            &vector_length,
            &error);
          }
          break;
        
          /* WOW, CAN THIS POSSIBLY BE SAFE...???? */
        case G_TYPE_STRING:
        default:
          tmp_string = g_key_file_get_value (key_file,
             stanza_name,
             key_name,
             &error);
          break;
        }
    }

  if(error)
    {
      /* handle error correctly */
      switch(error->code)
        {
        case G_KEY_FILE_ERROR_KEY_NOT_FOUND: /* a requested key was not found  */
        case G_KEY_FILE_ERROR_GROUP_NOT_FOUND: /* a requested group was not found  */
          {
            /* Is this really an error for us? */
            if(clear_if_not_exist)
              {
                g_value_reset(value);
              }
          }
          break;
        case G_KEY_FILE_ERROR_INVALID_VALUE: /* a value could not be parsed  */
          {
            char *try_again = NULL;

            /* This is reasonably serious, type didn't match in
             * the get_string/integer/double */
            if(type != G_TYPE_STRING)
              try_again = g_key_file_get_value(key_file, stanza_name, key_name, NULL);
        
            /* It would be nice to have line numbers here,
             * but group and key should be enough I guess. */
            zMapLogWarning("Failed reading/converting value '%s' for key '%s' in stanza '%s'",
                           (try_again ? try_again : ""),
                           key_name, stanza_name);
            if(try_again)
              g_free(try_again);
          }
          break;
        case G_KEY_FILE_ERROR_UNKNOWN_ENCODING:/* the text being parsed was in an unknown encoding  */
        case G_KEY_FILE_ERROR_PARSE: /* document was ill-formed  */
        case G_KEY_FILE_ERROR_NOT_FOUND: /* the file was not found  */
          /* We really shouldn't be getting these here.  */
          break;
        default:
          break;
        }
    }
  else if(allow_overwrite)
    {
      g_value_reset(value);

      switch(type)
        {
        case G_TYPE_INT:
          g_value_set_int(value, tmp_integer);
          break;
        case G_TYPE_DOUBLE:
          g_value_set_double(value, tmp_double);
          break;
        case G_TYPE_BOOLEAN:
          g_value_set_boolean(value, tmp_bool);
          break;
        case G_TYPE_POINTER:
          g_value_set_pointer(value, tmp_vector);
          break;
        case G_TYPE_STRING:
        default:
          g_value_set_string(value, tmp_string);
          break;
        }

      value_set = TRUE;
    }

  /* Just free tmp_string... */
  if(tmp_string)
    g_free(tmp_string);

  if(error)
      g_error_free(error);

  return value_set;
}



