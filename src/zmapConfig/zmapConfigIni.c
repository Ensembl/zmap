/*  File: zmapConfigIni.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: May 26 15:30 2009 (edgrif)
 * Created: Wed Aug 27 16:21:40 2008 (rds)
 * CVS info:   $Id: zmapConfigIni.c,v 1.10 2009-12-03 14:58:29 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>		/* memset */
#include <glib.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapConfigIni.h>

#define FILE_COUNT 5
#define IMPORTANT_COUNT 2

#undef WITH_LOGGING

typedef struct _ZMapConfigIniStruct
{
  //  ZMapMagic magic;
  GKeyFile *buffer_key_file;
  GKeyFile *extra_key_file;
  GKeyFile *user_key_file;
  GKeyFile *zmap_key_file;
  GKeyFile *sys_key_file;

  GError *buffer_key_error;
  GError *extra_key_error;
  GError *user_key_error;
  GError *zmap_key_error;
  GError *sys_key_error;

  unsigned int unsaved_alterations : 1;

}ZMapConfigIniStruct;


static gboolean zmapConfigIniGetValueFull(ZMapConfigIni config, 
					  char * stanza_name, 
					  char * key_name, 
					  GValue *value, 
					  GType type,
					  gboolean merge_files);
static gboolean get_merged_key_value(ZMapConfigIni config, 
				     char * stanza_name, 
				     char * key_name, 
				     GValue *value, 
				     GType type);
static gboolean get_value(GKeyFile *key_file, 
			  char *stanza_name, 
			  char *key_name, 
			  GValue *value, 
			  GType type,
			  gboolean allow_overwrite,
			  gboolean clear_if_not_exist);
static gboolean has_system_file(ZMapConfigIni config);
static gboolean system_file_loaded(ZMapConfigIni config);
static gboolean has_system_zmap_file(ZMapConfigIni config);
static gboolean system_zmap_file_loaded(ZMapConfigIni config);
static GKeyFile *read_file(char *file_name, GError **error_in_out);



ZMapConfigIni zMapConfigIniNew(void)
{
  ZMapConfigIni config = NULL;

  if(!(config = g_new0(ZMapConfigIniStruct, 1)))
    {
      zMapAssertNotReached();
    }
  else
    {
      g_type_init();
      //config->magic = config_magic_G;
    }
  
  return config;
}

gboolean zMapConfigIniReadAll(ZMapConfigIni config)
{
  gboolean red = FALSE;
  zMapAssert(config);

  if(!system_file_loaded(config) && has_system_file(config))
    {
      config->sys_key_file = read_file(zMapConfigDirGetSysFile(), &(config->sys_key_error));
    }

  if(!system_zmap_file_loaded(config) && has_system_zmap_file(config))
    {
      config->zmap_key_file = read_file(zMapConfigDirGetZmapHomeFile(), &(config->zmap_key_error));
    }

  red = zMapConfigIniReadUser(config);

  return red;
}

gboolean zMapConfigIniReadUser(ZMapConfigIni config)
{
  gboolean red = FALSE;
  zMapAssert(config);

  if(!config->unsaved_alterations)
    config->user_key_file = read_file(zMapConfigDirGetFile(), &(config->user_key_error));

  if(config->user_key_file != NULL)
    red = TRUE;

  return red;
}

gboolean zMapConfigIniReadBuffer(ZMapConfigIni config, char *buffer)
{
  gboolean red = FALSE;
  zMapAssert(config);

  if(buffer != NULL)
    {
      config->buffer_key_file = g_key_file_new();
      
      if(!(g_key_file_load_from_data(config->buffer_key_file, 
				     buffer, strlen(buffer),
				     G_KEY_FILE_KEEP_COMMENTS, 
				     &(config->buffer_key_error))))
	{
	  /* Do something with the error... */
	  g_key_file_free(config->buffer_key_file);
	  config->buffer_key_file = NULL;
	}
      else
	red = TRUE;
    }

  return red;
}

gboolean zMapConfigIniReadFile(ZMapConfigIni config, char *file)
{
  gboolean read = FALSE;

  zMapAssert(config);


  if ((g_path_is_absolute(file)
       || (file = zMapConfigDirFindFile(file)))
      && (config->extra_key_file = read_file(file, &(config->extra_key_error))))
    {
      read = TRUE;
    }

  return read ;
}

void zMapConfigIniGetStanza(ZMapConfigIni config, char *stanza_name)
{
  
  
  return ;
}

void zMapConfigIniGetAllStanzas(ZMapConfigIni config)
{
  return ;
}

gboolean zMapConfigIniGetUserValue(ZMapConfigIni config,
				   char * stanza_name,
				   char * key_name,
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
			       char * stanza_name,
			       char * key_name,
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

gboolean zMapConfigIniHasStanza(ZMapConfigIni config,
				char *stanza_name)
{
  GKeyFile *files[FILE_COUNT];
  gboolean result = FALSE;
  int i;

  files[0] = config->sys_key_file;
  files[1] = config->zmap_key_file;
  files[2] = config->user_key_file;
  files[3] = config->extra_key_file;
  files[4] = config->buffer_key_file;
  
  for (i = 0; result == FALSE && i < FILE_COUNT; i++)
    {
      if(files[i])
	result = g_key_file_has_group(files[i], stanza_name);
    }
  
  return result;
}

void zMapConfigIniSetValue(ZMapConfigIni config, 
			   char *stanza_name, 
			   char *key_name, 
			   GValue *value)
{
  GKeyFile *user_key_file = NULL;

  if((user_key_file = config->user_key_file))
    {
      GType type = 0;
      type = G_VALUE_TYPE(value);

      switch(type)
	{
	case G_TYPE_STRING:
	  {
	    char *string_value = NULL;
	    string_value = (char *)g_value_get_string(value);
	    g_key_file_set_string(user_key_file, stanza_name, key_name, string_value);
	  }
	  break;
	case G_TYPE_INT:
	  {
	    int int_value = 0;
	    int_value = g_value_get_int(value);
	    g_key_file_set_integer(user_key_file, stanza_name, key_name, int_value);
	  }
	  break;
	case G_TYPE_BOOLEAN:
	  {
	    gboolean bool_value = FALSE;
	    bool_value = g_value_get_boolean(value);
	    g_key_file_set_boolean(user_key_file, stanza_name, key_name, bool_value);
	  }
	  break;
	case G_TYPE_DOUBLE:
	  {
	    double double_value = FALSE;
	    double_value = g_value_get_double(value);
	    g_key_file_set_double(user_key_file, stanza_name, key_name, double_value);
	  }
	  break;
	default:
	  break;
	}
    }

  return ;
}

gboolean zMapConfigIniSaveUser(ZMapConfigIni config)
{
  gboolean saved = FALSE;

  if(config->user_key_file)
    {
      char *file_contents = NULL;
      gsize file_size = 0;
      GError *key_error = NULL;

      file_contents = g_key_file_to_data(config->user_key_file,
					 &file_size,
					 &key_error);

      if((!file_contents) || key_error)
	{
	  /* There was an error */
	}
      else
	{
	  /* Ok we can write the file contents to disk */
	  char *filename = NULL;

	  if((filename = zMapConfigDirGetFile()))
	    {
	      GIOChannel *output = NULL;
	      GError *error = NULL;

	      if((output = g_io_channel_new_file(filename, "w", &error)) && (!error))
		{
		  GIOStatus status;
		  gsize bytes_written = 0;

		  status = g_io_channel_write_chars(output, file_contents, file_size,
						    &bytes_written, &error);

		  if((status == G_IO_STATUS_NORMAL) && (!error) && bytes_written == file_size)
		    {
		      /* Everything was ok */
		      /* need to flush file... */
		    }
		  else if(error)
		    {
		      /* Cry about the error */
		      zMapLogCritical("%s", error->message);
		      g_error_free(error);
		      error = NULL;
		    }
		  else
		    zMapLogCritical("g_io_channel_write_chars returned error status '%d', but no error.", status);

		  status = g_io_channel_shutdown(output, TRUE, &error);

		  if(status != G_IO_STATUS_NORMAL && error)
		    {
		      /* Cry about another error... */
		      zMapLogCritical("%s", error->message);
		      g_error_free(error);
		      error = NULL;
		    }
		}

	      if(filename != zMapConfigDirGetFile())
		{
		  g_free(filename);
		  filename = NULL;
		}
	    }
	  else
	    {
	      /* problem with your file */
	    }
	}

      if(file_contents)
	{
	  g_free(file_contents);
	  file_contents = NULL;
	}
    }
  

  return saved;
}

void zMapConfigIniDestroy(ZMapConfigIni config, gboolean save_user)
{
  if(save_user)
    zMapConfigIniSaveUser(config);

  /* Free the key files */
  if(config->sys_key_file)
    g_key_file_free(config->sys_key_file);
  if(config->zmap_key_file)
    g_key_file_free(config->zmap_key_file);
  if(config->user_key_file)
    g_key_file_free(config->user_key_file);
  if(config->extra_key_file)
    g_key_file_free(config->extra_key_file);

  /* And any errors left hanging around */
  if(config->sys_key_error)
    g_error_free(config->sys_key_error);
  if(config->zmap_key_error)
    g_error_free(config->zmap_key_error);
  if(config->user_key_error)
    g_error_free(config->user_key_error);
  if(config->extra_key_error)
    g_error_free(config->extra_key_error);

  /* zero the memory */
  memset(config, 0, sizeof(ZMapConfigIniStruct));

  return ;
}




/* internal calls */


static gboolean zmapConfigIniGetValueFull(ZMapConfigIni config, 
					  char * stanza_name, 
					  char * key_name, 
					  GValue *value, 
					  GType type,
					  gboolean merge_files)
{
  gboolean success = FALSE;

  g_return_val_if_fail(value, FALSE);

  if((!merge_files) && (config->user_key_file != NULL))
    {
      success = get_value(config->user_key_file, stanza_name, key_name, value, type, TRUE, TRUE);
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
				     char * stanza_name, 
				     char * key_name, 
				     GValue *value, 
				     GType type)
{
  GKeyFile *files[FILE_COUNT], *important_files[IMPORTANT_COUNT];
  gboolean key_found = FALSE;
  gboolean honour_important = TRUE;
  char *important_key = NULL;
  int i;

  if(honour_important)
    {
      important_key = g_strdup_printf("!%s", key_name);

      /* get the system first, then the zmap one */
      important_files[1] = config->sys_key_file;
      important_files[0] = config->zmap_key_file;
      
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
      files[0] = config->buffer_key_file;
      files[1] = config->extra_key_file;
      files[2] = config->user_key_file;
      files[3] = config->zmap_key_file;
      files[4] = config->sys_key_file;

      for(i = 0; key_found == FALSE && i < FILE_COUNT; i++)
	{
	  if(files[i])
	    {
	      if(get_value(files[i], stanza_name, key_name, value, type, TRUE, FALSE))
		key_found = TRUE;
	    }
	}
    }

  return key_found;
}

static gboolean get_value(GKeyFile *key_file, 
			  char *stanza_name, 
			  char *key_name, 
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
	  break;
	case G_TYPE_BOOLEAN:
	  tmp_bool = g_key_file_get_boolean(key_file,
					    stanza_name,
					    key_name,
					    &error);
	  break;
	case G_TYPE_POINTER:
	  {
	    unsigned int vector_length;
	    tmp_vector = g_key_file_get_string_list(key_file,
						    stanza_name,
						    key_name,
						    &vector_length,
						    &error);
	  }
	  break;
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
	     * the get_strig/integer/double */
	    if(type != G_TYPE_STRING)
	      try_again = g_key_file_get_value(key_file, stanza_name, key_name, NULL);

	    /* It would be nice to have line numbers here,
	     * but group and key should be enough I guess. */
#ifdef WITH_LOGGING
	    zMapLogWarning("Failed reading/converting value '%s' for key '%s' in stanza '%s'. Expected type <>", 
			   (try_again == NULL ? tmp_string : try_again),
			   key_name, stanza_name);
#endif /* WITH_LOGGING */
	    if(try_again)
	      g_free(try_again);
	  }
	  break;
	case G_KEY_FILE_ERROR_UNKNOWN_ENCODING:	/* the text being parsed was in an unknown encoding  */
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

  return value_set;
}

static gboolean has_system_file(ZMapConfigIni config)
{
  gboolean exists = FALSE;
  char *file = NULL;

  if((file = zMapConfigDirGetSysFile()))
    exists = TRUE;

  return exists;
}

static gboolean system_file_loaded(ZMapConfigIni config)
{
  gboolean exists = FALSE;

  if(config->sys_key_file)
    exists = TRUE;

  return exists;
}

static gboolean has_system_zmap_file(ZMapConfigIni config)
{
  gboolean exists = FALSE;
  char *file = NULL;

  if((file = zMapConfigDirGetZmapHomeFile()))
    exists = TRUE;

  return exists;
}

static gboolean system_zmap_file_loaded(ZMapConfigIni config)
{
  gboolean exists = FALSE;

  if(config->zmap_key_file)
    exists = TRUE;

  return exists;
}

static GKeyFile *read_file(char *file_name, GError **error_in_out)
{
  GKeyFile *key_file = NULL;
  GError *error = NULL;

  g_return_val_if_fail((error_in_out && *error_in_out == NULL), NULL);

  key_file = g_key_file_new();

  if(!(g_key_file_load_from_file(key_file, file_name, G_KEY_FILE_KEEP_COMMENTS, &error)))
    {
      /* Do something with the error... */
      g_key_file_free(key_file);
      key_file = NULL;

      *error_in_out = error;
    }


  return key_file;
}



typedef struct
{
  char *stanza_name;
  char *stanza_type;
  GList *keys;
}ZMapConfigIniContextStanzaEntryStruct, *ZMapConfigIniContextStanzaEntry;

typedef struct _ZMapConfigIniContextStruct
{
  ZMapConfigIni config;

  gboolean config_read;

  char *error_message;

  GList *groups;
}ZMapConfigIniContextStruct;

typedef struct
{
  ZMapConfigIniContext context;
  ZMapConfigIniContextStanzaEntry stanza;
  gboolean result;
} CheckRequiredKeysStruct, *CheckRequiredKeys;

static gint match_name_type(gconstpointer list_data, gconstpointer user_data);
static GType get_stanza_key_type(ZMapConfigIniContext context,
				 char *stanza_name,
				 char *stanza_type,
				 char *key_name);
static ZMapConfigIniContextStanzaEntry get_stanza_with_type(ZMapConfigIniContext context,
							    char *stanza_type);
static GList *copy_keys(ZMapConfigIniContextKeyEntryStruct *keys);
static void check_required(gpointer list_data, gpointer user_data);
static gboolean check_required_keys(ZMapConfigIniContext context, 
				    ZMapConfigIniContextStanzaEntry stanza);


ZMapConfigIniContext zMapConfigIniContextCreate(void)
{
  ZMapConfigIniContext context = NULL;

  if((context = g_new0(ZMapConfigIniContextStruct, 1)))
    {
      context->config = zMapConfigIniNew();
      context->config_read = zMapConfigIniReadAll(context->config);
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


char *zMapConfigIniContextErrorMessage(ZMapConfigIniContext context)
{
  char *e = NULL;
  e = context->error_message;
  return e;
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
	      context->error_message = g_strdup_printf("failed to get value for %s, %s",
						       stanza_name, key_name);
	      *value_out = NULL;
	    }
	}
      else
	{
	  context->error_message = g_strdup_printf("failed to get type for %s, %s", 
						   stanza_name, key_name);
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

  memset(context, 0, sizeof(ZMapConfigIniContextStruct));

  g_free(context);

  return NULL;
}





/* 
 *                    Context internals
 */



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
	  result = 0;		/* zero == found */
	}
      else
	result = 1;
    }
  /* not and exact name match, do the types match? */
  else if(g_ascii_strcasecmp( query->stanza_type, list_entry->stanza_type ) == 0)
    {
      /* make sure the stanza_name == * in the list the context
       * has. This is so stanzas with different names can have the
       * same basic type. e.g. the same list of keys/info/G_TYPE data
       * for keys... */
      if(g_ascii_strcasecmp(list_entry->stanza_name, "*") == 0)
	{
	  query->keys = list_entry->keys;
	  result = 0;		/* zero == found */
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

static ZMapConfigIniContextStanzaEntry get_stanza_with_type(ZMapConfigIniContext context,
							    char *stanza_type)
{
  ZMapConfigIniContextStanzaEntryStruct user_request = {NULL};
  ZMapConfigIniContextStanzaEntry stanza = NULL;
  GList *entry_found;

  user_request.stanza_name = "*";
  user_request.stanza_type = stanza_type;

  if((entry_found = g_list_find_custom(context->groups, &user_request, match_name_type)))
    {
      stanza = (ZMapConfigIniContextStanzaEntry)(entry_found->data);
    }

  return stanza;
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
	  context->error_message = g_strdup_printf("Failed to get required key '%s' in stanza %s",
						   key->key, stanza->stanza_name);
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



/* Context Config Loading */
typedef struct
{
  ZMapConfigIniContext context;
  ZMapConfigIniContextStanzaEntry stanza;
  
  char *current_stanza_name;

  ZMapConfigIniUserDataCreateFunc object_create_func;
  gpointer current_object;

  GList *object_list_out;
} FetchReferencedStanzasStruct, *FetchReferencedStanzas;

static void fill_stanza_key_value(gpointer list_data, gpointer user_data)
{
  FetchReferencedStanzas full_data = (FetchReferencedStanzas)user_data;
  ZMapConfigIniContextKeyEntry key = (ZMapConfigIniContextKeyEntry)list_data;
  GValue *value = NULL;

  if (zMapConfigIniGetValue(full_data->context->config,
			    full_data->current_stanza_name,
			    key->key,
			    &value,
			    key->type))
    {
      if (key->set_property)
	(key->set_property)(full_data->current_stanza_name, key->key, key->type, full_data->current_object, value);
      else
	zMapLogWarning("No set_property function for key '%s'", key->key);

      g_free(value);
    }

  return ;
}

static void fetch_referenced_stanzas(gpointer list_data, gpointer user_data)
{
  FetchReferencedStanzas full_data = (FetchReferencedStanzas)user_data;
  char *stanza_name = (char *)list_data;

  full_data->current_stanza_name = stanza_name;

  if (zMapConfigIniHasStanza(full_data->context->config, stanza_name) && (full_data->object_create_func))
    {
      if ((full_data->current_object = (full_data->object_create_func)()))
	{
	  /* get stanza keys */
	  g_list_foreach(full_data->stanza->keys, fill_stanza_key_value, user_data);
	  
	  full_data->object_list_out = g_list_append(full_data->object_list_out,
						     full_data->current_object);
	}
      else
	zMapLogWarning("Object Create Function for stanza '%s'"
		       " failed to return anything", stanza_name);
    }

  return ;
}

GList *get_child_stanza_names_as_list(ZMapConfigIniContext context,
				      char *parent_name,
				      char *parent_key)
{
  GList *list = NULL;
  GValue *value = NULL;

  if (parent_name && parent_key &&  zMapConfigIniGetValue(context->config,
							  parent_name, parent_key,
							  &value, G_TYPE_POINTER))
    {
      char **strings_list = NULL;

      if ((strings_list = g_value_get_pointer(value)))
	{
	  char **ptr = strings_list;

	  while (ptr && *ptr)
	    {
	      *ptr = g_strstrip(*ptr) ;
	      list = g_list_append(list, *ptr) ;
	      ptr++ ;
	    }

	  g_free(strings_list);
	}
    }
  
  return list;
}

static void free_source_names(gpointer list_data, gpointer unused_user_data)
{
  g_free(list_data);
  return ;
}

GList *zMapConfigIniContextGetStanza(ZMapConfigIniContext context,
				     ZMapConfigIniUserDataCreateFunc create,
				     char *stanza_name, char *stanza_type)
{
  GList *list = NULL;

  if(strcmp(stanza_name, stanza_type) == 0)
    list = zMapConfigIniContextGetReferencedStanzas(context, create, 
						    NULL, NULL, 
						    NULL, stanza_type);

  return list;
}

GList *zMapConfigIniContextGetReferencedStanzas(ZMapConfigIniContext context,
						ZMapConfigIniUserDataCreateFunc object_create_func,
						char *parent_name, 
						char *parent_type, 
						char *parent_key,
						char *child_type)
{
  FetchReferencedStanzasStruct data = {NULL};
  GList *sources      = NULL;
  GList *source_names = NULL;

  data.context = context;
  data.stanza  = get_stanza_with_type(context, child_type);
  data.object_create_func = object_create_func;

  source_names = get_child_stanza_names_as_list(context, 
						parent_name, 
						parent_key);

  if(!source_names)
    source_names = g_list_append(source_names, g_strdup(child_type));

  g_list_foreach(source_names, fetch_referenced_stanzas, &data);

  g_list_foreach(source_names, free_source_names, NULL);
  g_list_free(source_names);

  sources = data.object_list_out;

  return sources;
}



// somehow !! 'styles = one ; two ; three' becomes a list of pointers to strings before we get here
GList *get_names_as_list(char *styles)
{
  GList *list = NULL;

  char **strings_list = NULL;

  if ((strings_list = g_strsplit(styles,";",0)))
  {
    char **ptr = strings_list;

    while (ptr && *ptr)
      {
       *ptr = g_strstrip(*ptr) ;
       list = g_list_append(list, *ptr);
       ptr++ ;
      }

    g_free(strings_list);
  }
  return list;
}


/* see zMapConfigIniContextGetNamed(ZMapConfigIniContext context, char *stanza_name)
 * in zmapConfigIni.c, which is a wrapper for GetReferencedStanzas() above
 * For a list of styles in a source stanza we can't get these easily as we don't know the name 
 * of the source stanza, and GetNamed hard codes 'source' as the name of the parent.
 * so we find each stanza bu name from the list and add to our return list.
 */
  
GList *zMapConfigIniContextGetListedStanzas(ZMapConfigIniContext context,
                                    ZMapConfigIniUserDataCreateFunc object_create_func,
                                    char *styles_list,char *child_type)
{
  FetchReferencedStanzasStruct data = {NULL};
  GList *styles      = NULL;
  GList *style_names = NULL;

  data.context = context;
  data.stanza  = get_stanza_with_type(context, child_type);
  data.object_create_func = object_create_func;

  style_names = get_names_as_list(styles_list);

  zMapAssert(style_names) ;

  g_list_foreach(style_names, fetch_referenced_stanzas, &data);

//  g_list_foreach(style_names, free_source_names, NULL);
  g_list_free(style_names);

  styles = data.object_list_out;

  return styles;
}



