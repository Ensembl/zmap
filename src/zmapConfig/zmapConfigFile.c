/*  Last edited: Jul 23 10:00 2012 (edgrif) */
/*  File: zmapConfFile.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
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



gboolean zMapConfigIniReadAll(ZMapConfigIni config, char *config_file)
{
  gboolean red = FALSE ;
//  char *file_name ;

  zMapAssert(config) ;

  // mh17: this assignment invalidates the 2nd if statement and subsequent statement, we'd read the same file twice
  // testing shows that none of these files exist....
  // file_name = (config_file ? config_file : zMapConfigDirGetZmapHomeFile()) ;

  if (!system_file_loaded(config) && has_system_file(config))
    {
      config->sys_key_file = read_file(zMapConfigDirGetSysFile(), &(config->sys_key_error)) ;
    }

//  if (!system_zmap_file_loaded(config) && (config_file || has_system_zmap_file(config)))
  if (!system_zmap_file_loaded(config) && has_system_zmap_file(config))
    {
      config->zmap_key_file = read_file(zMapConfigDirGetZmapHomeFile(), &(config->zmap_key_error)) ;
    }

//printf("Read All: %s (%d), %s (%d), %s\n",zMapConfigDirGetSysFile(), has_system_file(config),  //zMapConfigDirGetZmapHomeFile(),has_system_zmap_file(config), config_file ? config_file : "");

//  red = zMapConfigIniReadUser(config, file_name) ;

//  if(config_file)
	/* rather helpfully this function accepts a null arg and then chooses the config file specified on the command line */
	/* which is much clearer than having that passed through to this function. Yeah right. */
	red = zMapConfigIniReadUser(config, config_file) ;

  return red ;
}


gboolean zMapConfigIniReadUser(ZMapConfigIni config, char *config_file)
{
  gboolean red = FALSE ;
  char *file_name ;

  zMapAssert(config) ;

  file_name = (config_file ? config_file : zMapConfigDirGetFile()) ;

  if (!config->unsaved_alterations)
    config->user_key_file = read_file(file_name, &(config->user_key_error)) ;

  if (config->user_key_file != NULL)
    red = TRUE ;

  return red ;
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

/* this is used for styles, NOTE not ever freed until the context is destroyed so can have only one file */
gboolean zMapConfigIniReadFile(ZMapConfigIni config, char *file)
{
  gboolean read = FALSE;

  zMapAssert(config);

  if ((g_path_is_absolute(file) || (file = zMapConfigDirFindFile(file)))
      && (config->extra_key_file = read_file(file, &(config->extra_key_error))))
    {
      read = TRUE;
    }

  return read ;
}


gboolean zMapConfigIniSaveUser(ZMapConfigIni config)
{
  gboolean saved = FALSE;

  if(config->user_key_file && config->unsaved_alterations)
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


gboolean zMapConfigIniHasStanza(ZMapConfigIni config,char *stanza_name,GKeyFile **which)
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

