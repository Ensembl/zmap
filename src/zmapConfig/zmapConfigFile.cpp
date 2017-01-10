/*  File: zmapConfigFile.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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


using namespace std ;


#undef WITH_LOGGING




static gboolean has_system_file(ZMapConfigIni config);
static gboolean system_file_loaded(ZMapConfigIni config);
static gboolean has_prefs_file(ZMapConfigIni config);
static gboolean prefs_file_loaded(ZMapConfigIni config);
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
      zMapConfigIniReadSystem(config) ;
    }

  if (!system_zmap_file_loaded(config) && has_system_zmap_file(config))
    {
      zMapConfigIniReadZmap(config) ;
    }

  if (!prefs_file_loaded(config) && has_prefs_file(config))
    {
      zMapConfigIniReadPrefs(config) ;
    }

  red = zMapConfigIniReadUser(config, config_file) ;

  return red ;
}


gboolean zMapConfigIniReadFileType(ZMapConfigIni config, const char *config_file, ZMapConfigIniFileType file_type)
{
  gboolean result = FALSE ;
  zMapReturnValIfFail(config, result) ;

  switch (file_type)
    {
    case ZMAPCONFIG_FILE_SYS:
      zMapConfigIniReadSystem(config) ;
      break ;
    case ZMAPCONFIG_FILE_ZMAP:
      zMapConfigIniReadZmap(config) ;
      break ;
    case ZMAPCONFIG_FILE_USER:
      zMapConfigIniReadUser(config, config_file) ;
      break ;
    case ZMAPCONFIG_FILE_PREFS:
      zMapConfigIniReadPrefs(config) ;
      break ;
    case ZMAPCONFIG_FILE_STYLES:
      zMapConfigIniReadStyles(config, config_file) ;
      break ;
    case ZMAPCONFIG_FILE_BUFFER: // fall through - buffer is treated differently to files
    default:
      zMapWarnIfReached() ;
      break ;
    }

  return result ;
}


gboolean zMapConfigIniReadSystem(ZMapConfigIni config)
{
  const char *filename = zMapConfigDirGetSysFile() ;
  config->key_file[ZMAPCONFIG_FILE_SYS] = read_file(filename, &(config->key_error[ZMAPCONFIG_FILE_SYS])) ;
  config->key_file_name[ZMAPCONFIG_FILE_SYS] = g_quark_from_string(filename) ;
  return TRUE ;
}


gboolean zMapConfigIniReadZmap(ZMapConfigIni config)
{
  const char *filename = zMapConfigDirGetZmapHomeFile() ;
  config->key_file[ZMAPCONFIG_FILE_ZMAP] = read_file(filename, &(config->key_error[ZMAPCONFIG_FILE_ZMAP])) ;
  config->key_file_name[ZMAPCONFIG_FILE_ZMAP] = g_quark_from_string(filename) ;
  return TRUE ;
}


gboolean zMapConfigIniReadUser(ZMapConfigIni config, const char *config_file)
{
  gboolean red = FALSE ;
  zMapReturnValIfFail(config, red) ;

  if (config_file)
    {
      if (!config->unsaved_changes[ZMAPCONFIG_FILE_USER])
        {
          config->key_file[ZMAPCONFIG_FILE_USER] = read_file(config_file, &(config->key_error[ZMAPCONFIG_FILE_USER])) ;
        }
      
      if (config->key_file[ZMAPCONFIG_FILE_USER] != NULL)
        {
          config->key_file_name[ZMAPCONFIG_FILE_USER] = g_quark_from_string(config_file) ;
          red = TRUE ;
        }
    }

  return red ;
}


gboolean zMapConfigIniReadPrefs(ZMapConfigIni config)
{
  const char *filename = zMapConfigDirGetPrefsFile() ;
  config->key_file[ZMAPCONFIG_FILE_PREFS] = read_file(filename, &(config->key_error[ZMAPCONFIG_FILE_PREFS])) ;
  config->key_file_name[ZMAPCONFIG_FILE_PREFS] = g_quark_from_string(filename) ;
  return TRUE ;
}


gboolean zMapConfigIniReadBuffer(ZMapConfigIni config, const char *buffer)
{
  gboolean red = FALSE;
  /* if (!config)
    return red ; */ 
  zMapReturnValIfFail(config, red) ; 

  if(buffer != NULL)
    {
      config->key_file[ZMAPCONFIG_FILE_BUFFER] = g_key_file_new();

      if(!(g_key_file_load_from_data(config->key_file[ZMAPCONFIG_FILE_BUFFER],
             buffer, strlen(buffer),
             G_KEY_FILE_KEEP_COMMENTS,
             &(config->key_error[ZMAPCONFIG_FILE_BUFFER]))))
        {
          /* Do something with the error... */
          g_key_file_free(config->key_file[ZMAPCONFIG_FILE_BUFFER]);
          config->key_file[ZMAPCONFIG_FILE_BUFFER] = NULL;
        }
      else
        red = TRUE;
    }

  return red;
}


/* this is used for styles, NOTE not ever freed until the context is destroyed so can have only one file */
gboolean zMapConfigIniReadStyles(ZMapConfigIni config, const char *file)
{
  gboolean read = FALSE;

  /* if (!config)
    return read ; */ 
  zMapReturnValIfFail(config, read ) ; 

  if (file)
    {
      char *file_expanded = zMapExpandFilePath(file) ;
      char *filepath = NULL ;

      if (!g_path_is_absolute(file_expanded))
        filepath = zMapConfigDirFindFile(file_expanded) ;
      else
        filepath = g_strdup(file_expanded) ;

      if (filepath
          && (config->key_file[ZMAPCONFIG_FILE_STYLES] = read_file(filepath, &(config->key_error[ZMAPCONFIG_FILE_STYLES]))))
        {
          config->key_file_name[ZMAPCONFIG_FILE_STYLES] = g_quark_from_string(file) ;
          read = TRUE;
        }

      if (file_expanded)
        g_free(file_expanded) ;

      if (filepath)
        g_free(filepath) ;
    }

  return read ;
}


gboolean zMapConfigIniSave(ZMapConfigIni config, ZMapConfigIniFileType file_type)
{
  gboolean saved = FALSE;
  zMapReturnValIfFail(config, saved) ;

  if(config->key_file_name[file_type] && 
     config->key_file[file_type] && 
     config->unsaved_changes[file_type])
    {
      const char *filename = g_quark_to_string(config->key_file_name[file_type]) ;
      char *file_contents = NULL;
      gsize file_size = 0;
      GError *g_error = NULL;

      file_contents = g_key_file_to_data(config->key_file[file_type],
                                         &file_size,
                                         &g_error);

      if(file_contents && !g_error)
        {
          /* Must make sure it's UTF8 or g_io_channel_write_chars will crash,
           * for now just replace the bad character with a question mark */
          const gchar *bad_char_out = NULL ;
          while (!g_utf8_validate(file_contents, -1, &bad_char_out))
            {
              char *bad_char = (char *)bad_char_out ;
              *bad_char='?';
            }

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


/* Return true if any of our config files contains the given stanza. Sets the key file to the
 * first config file found with this stanza */
gboolean zMapConfigIniHasStanza(ZMapConfigIni config, const char *stanza_name,GKeyFile **which)
{
  gboolean result = FALSE;

  /* gb10: This used to be in reverse order but that doesn't seem to make sense. It returns the
   * first key file with the given stanza so we want to check higher priority config files first
   * which I think should be in the same order as they are listed in the enum. */
  int i = 0 ;
  ZMapConfigIniFileType file_type = ZMAPCONFIG_FILE_NONE ;

  for ( ; i < (int)ZMAPCONFIG_FILE_NUM_TYPES ; ++i)
    {
      file_type = (ZMapConfigIniFileType)i ;

      if(config->key_file[file_type])
        result = g_key_file_has_group(config->key_file[file_type], stanza_name);

      if(result)
        break;
    }

  if(result && which)
    *which = config->key_file[file_type];

  return result;
}


/* Return true if any of our config files contains the given stanza. Sets the list of key files to
 * contain all config files that have this stanza */
gboolean zMapConfigIniHasStanzaAll(ZMapConfigIni config, const char *stanza_name, list<GKeyFile*> &which)
{
  gboolean result = FALSE;

  /* gb10: This used to be in reverse order but that doesn't seem to make sense. It returns the
   * first key file with the given stanza so we want to check higher priority config files first
   * which I think should be in the same order as they are listed in the enum. */
  int i = 0 ;
  ZMapConfigIniFileType file_type = ZMAPCONFIG_FILE_NONE ;

  for ( ; i < (int)ZMAPCONFIG_FILE_NUM_TYPES ; ++i)
    {
      file_type = (ZMapConfigIniFileType)i ;

      if(config->key_file[file_type] && g_key_file_has_group(config->key_file[file_type], stanza_name))
        {
          result = TRUE ;
          which.push_back(config->key_file[file_type]);
        }
    }

  return result;
}



void zMapConfigIniDestroy(ZMapConfigIni config, gboolean save_user)
{
  if(save_user)
    zMapConfigIniSave(config, ZMAPCONFIG_FILE_USER);

  for (int i = 0; i < ZMAPCONFIG_FILE_NUM_TYPES; ++i)
    {
      ZMapConfigIniFileType file_type = (ZMapConfigIniFileType)i ;

      /* Free the key files */
      if(config->key_file[file_type])
        g_key_file_free(config->key_file[file_type]);

      if (config->key_error[file_type])
        g_error_free(config->key_error[file_type]) ;
    }

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

  if(config->key_file[ZMAPCONFIG_FILE_SYS])
    exists = TRUE;

  return exists;
}

static gboolean has_prefs_file(ZMapConfigIni config)
{
  gboolean exists = FALSE;
  char *file = NULL;

  if((file = zMapConfigDirGetPrefsFile()))
    exists = TRUE;

  return exists;
}

static gboolean prefs_file_loaded(ZMapConfigIni config)
{
  gboolean exists = FALSE;

  if(config->key_file[ZMAPCONFIG_FILE_PREFS])
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

  if(config->key_file[ZMAPCONFIG_FILE_ZMAP])
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

