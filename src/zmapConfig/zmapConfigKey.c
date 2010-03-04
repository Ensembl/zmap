/*  File: zmapConfigIni.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk) after Roy Storey (rds@sanger.ac.uk)
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
 * CVS info:   $Id: zmapConfigKey.c,v 1.2 2010-03-04 15:09:45 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>		/* memset */
#include <glib.h>

#include <zmapConfigIni_P.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapConfigIni.h>

#define FILE_COUNT 5
#define IMPORTANT_COUNT 2

#undef WITH_LOGGING


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



