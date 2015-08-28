/*  File: zmapConfigFile.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
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

#include <ZMap/zmap.hpp>

#include <string.h>    /* memset */
#include <glib.h>


#include <ZMap/zmapConfigDir.hpp>
#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapConfigIni.hpp>
#include <zmapConfigIni_P.hpp>



#undef WITH_LOGGING




static gboolean has_system_file(ZMapConfigIni config);
static gboolean system_file_loaded(ZMapConfigIni config);
static gboolean has_system_zmap_file(ZMapConfigIni config);
static gboolean system_zmap_file_loaded(ZMapConfigIni config);
static GKeyFile *read_file(const char *file_name, GError **error_in_out);


ZMapConfigIni zMapConfigIniNew(void)
{
  ZMapConfigIni config = NULL;

  /* if(!(config = g_new0(ZMapConfigIniStruct, 1)))
    {
      zMapWarnIfReached();
    }
  else
    {
      g_type_init();
    } */

  config = g_new0(ZMapConfigIniStruct, 1) ; 
  zMapReturnValIfFail(config, config) ; 

  g_type_init() ; 

  return config;
}



gboolean zMapConfigIniReadAll(ZMapConfigIni config, const char *config_file)
{
  gboolean red = FALSE ;
  zMapReturnValIfFail(config, red) ; 

  if (!system_file_loaded(config) && has_system_file(config))
    {
      config->sys_key_file = read_file(zMapConfigDirGetSysFile(), &(config->sys_key_error)) ;
    }

  if (!system_zmap_file_loaded(config) && has_system_zmap_file(config))
    {
      config->zmap_key_file = read_file(zMapConfigDirGetZmapHomeFile(), &(config->zmap_key_error)) ;
    }

  red = zMapConfigIniReadUser(config, config_file) ;

  return red ;
}


gboolean zMapConfigIniReadUser(ZMapConfigIni config, const char *config_file)
{
  gboolean red = FALSE ;
  zMapReturnValIfFail(config && config_file, red) ;

  if (!config->unsaved_alterations)
    {
      config->user_key_file = read_file(config_file, &(config->user_key_error)) ;
    }

  if (config->user_key_file != NULL)
    {
      config->user_file_name = g_quark_from_string(config_file) ;
      red = TRUE ;
    }

  return red ;
}


gboolean zMapConfigIniReadBuffer(ZMapConfigIni config, const char *buffer)
{
  gboolean red = FALSE;
  /* if (!config)
    return red ; */ 
  zMapReturnValIfFail(config, red) ; 

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


/* this is used for styles, NOTE not ever freed until the context is destroyed so can have only one file */
gboolean zMapConfigIniReadFile(ZMapConfigIni config, const char *file)
{
  gboolean read = FALSE;

  /* if (!config)
    return read ; */ 
  zMapReturnValIfFail(config, read ) ; 

  char *file_expanded = zMapExpandFilePath(file) ;
  char *filepath = NULL ;

  if (!g_path_is_absolute(file_expanded))
    filepath = zMapConfigDirFindFile(file_expanded) ;
  else
    filepath = g_strdup(file_expanded) ;

  if (filepath
      && (config->extra_key_file = read_file(filepath, &(config->extra_key_error))))
    {
      config->extra_file_name = g_quark_from_string(file) ;
      read = TRUE;
    }

  if (file_expanded)
    g_free(file_expanded) ;

  if (filepath)
    g_free(filepath) ;

  return read ;
}


gboolean zMapConfigIniSaveUser(ZMapConfigIni config)
{
  gboolean saved = FALSE;
  zMapReturnValIfFail(config, saved) ;

  if(config->user_file_name && config->user_key_file && config->unsaved_alterations)
    {
      const char *filename = g_quark_to_string(config->user_file_name) ;
      char *file_contents = NULL;
      gsize file_size = 0;
      GError *g_error = NULL;

      file_contents = g_key_file_to_data(config->user_key_file,
                                         &file_size,
                                         &g_error);

      if(file_contents && !g_error)
        {
          /* Ok we can write the file contents to disk */
          GIOChannel *output = g_io_channel_new_file(filename, "w", &g_error);
        
          if(output && !g_error)
            {
              GIOStatus status;
              gsize bytes_written = 0;

              status = g_io_channel_write_chars(output, file_contents, file_size,
                                                &bytes_written, &g_error);
                
              if (status == G_IO_STATUS_NORMAL && !g_error && bytes_written == file_size)
                {
                  /* Everything was ok */
                  /* need to flush file... */
                }
              else
                {
                  /* Cry about the error */
                  zMapLogCritical("Failed writing to file '%s': %s", 
                                  filename,
                                  g_error ? g_error->message : "<no error message>");

                  g_error_free(g_error);
                  g_error = NULL;
                }
        
              status = g_io_channel_shutdown(output, TRUE, &g_error);
        
              if(status != G_IO_STATUS_NORMAL && g_error)
                {
                  /* Cry about another error... */
                  zMapLogCritical("Failed flushing channel: %s", 
                                  g_error ? g_error->message : "<no error message>");

                  g_error_free(g_error);
                  g_error = NULL;
                }
            }
          else
            {
              zMapLogCritical("Failed to create channel for file '%s': %s", 
                              filename,
                              g_error ? g_error->message : "<no error message>");

              g_error_free(g_error);
              g_error = NULL;
            }
        }
      else
        {
          zMapLogCritical("Failed to create config file contents: %s", 
                          g_error ? g_error->message : "<no error message>");

          g_error_free(g_error);
          g_error = NULL;
        }
        
      if(file_contents)
        {
          g_free(file_contents);
        }
    }


  return saved;
}


gboolean zMapConfigIniHasStanza(ZMapConfigIni config, const char *stanza_name,GKeyFile **which)
{
  GKeyFile *files[FILE_COUNT];
  gboolean result = FALSE;
  int i;

  files[0] = config->sys_key_file;
  files[1] = config->zmap_key_file;
  files[2] = config->user_key_file;
  files[3] = config->extra_key_file;
  files[4] = config->buffer_key_file;

  for (i = 0;i < FILE_COUNT; i++)
    {
      if(files[i])
        result = g_key_file_has_group(files[i], stanza_name);
      if(result)
            break;
    }
  if(result && which)
      *which = files[i];

  return result;
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

static GKeyFile *read_file(const char *file_name, GError **error_in_out)
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

