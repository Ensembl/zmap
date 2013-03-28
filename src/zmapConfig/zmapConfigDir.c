/*  File: zmapConfigDir.c
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: ZMap config file handling.
 *
 * Exported functions: See ZMap/zmapConfigDir.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <glib.h>

#include <ZMap/zmapUtils.h>
#include <zmapConfigDir_P.h>
#include <zmapConfigIni_P.h>




/* Holds the single instance of our configuration file stuff. */
static ZMapConfigDir dir_context_G = NULL ;


/* Actually this all needs revisiting....we may have multiple config files now.... */

/* At one time we thought of having a default configuration string internally but
 * it looks like it's fallen into disrepair...... */


/* The context is only created the first time this function is called, after that
 * it just returns a reference to the existing context. Checks to see if the
 * configuration can be accessed.
 *
 * Note that as this routine should be called once per application it is not
 * thread safe, it could be made so but is overkill as the GUI/master thread is
 * not thread safe anyway.
 * 
 * Rules for producing the filepath for the configuration file are:
 * 
 * config_dir && config_file 
 *    config_dir and config_file are concatenated
 * 
 * !config_dir && config_file
 *    "~/.ZMap" and config_file are concatenated unless config_file
 *    is a filepath in which case just config_file is used.
 * 
 * config_dir && !config_file 
 *    config_dir and "Zmap" are concatenated
 * 
 * !config_dir && !config_file
 *    "~/.ZMap" and "Zmap" are concatenated
 * 
 * If conf_dir or any _directory_ part of config_file are relative then
 * they are expanded.
 * 
 * returns FALSE if the configuration file does not exist/read/writeable.
 *
 */
gboolean zMapConfigDirCreate(char *config_dir_in, char *config_file_in)
{
  gboolean result = FALSE ;
  ZMapConfigDir dir_context ;
  char *config_dir = NULL, *config_file = NULL ;
  char *zmap_home ;

  g_return_val_if_fail(dir_context_G == NULL, FALSE) ;

  dir_context_G = dir_context = g_new0(ZMapConfigDirStruct, 1) ;

  if (!config_dir_in || !(*config_dir_in))
    {
      config_dir = zMapExpandFilePath("~/" ZMAP_USER_CONFIG_DIR) ;
    }
  else
    {
      if (g_path_is_absolute(config_dir_in))
	config_dir = g_strdup(config_dir_in) ;
      else
	config_dir = zMapExpandFilePath(config_dir_in) ;
    }

  if (!config_file_in || !(*config_file_in))
    {
      config_file = g_strdup(ZMAP_USER_CONFIG_FILE) ;
    }
  else
    {
      char *dirname ;

      dirname = g_path_get_dirname(config_file_in) ;

      if (g_ascii_strcasecmp(dirname, ".") == 0)
	{
	  config_file = g_strdup(config_file_in) ;
	}
      else
	{
	  g_free(config_dir) ;

	  config_dir = zMapExpandFilePath(dirname) ;

	  config_file = g_path_get_basename(config_file_in) ; /* Allocates string. */
	}

      g_free(dirname) ;
    }

  if (config_dir && config_file)
    {
      if ((dir_context->config_dir = zMapGetDir(config_dir, FALSE, FALSE))
	  && (dir_context->config_file = zMapGetFile(dir_context->config_dir, config_file, FALSE)))
	result = TRUE ;
    }

  if ((zmap_home = getenv("ZMAP_HOME")))
    {
      zmap_home = g_strdup_printf("%s/etc", zmap_home) ;

      if((dir_context->zmap_conf_dir  = zMapGetDir(zmap_home, FALSE, FALSE)))
	dir_context->zmap_conf_file = zMapGetFile(dir_context->zmap_conf_dir, ZMAP_USER_CONFIG_FILE, FALSE) ;

      g_free(zmap_home) ;
    }
  else
    {
      dir_context->zmap_conf_dir = dir_context->zmap_conf_file = NULL ;
    }

  if (!((dir_context->sys_conf_dir  = zMapGetDir("/etc", FALSE, FALSE))
	&& (dir_context->sys_conf_file = zMapGetFile(dir_context->sys_conf_dir, ZMAP_USER_CONFIG_FILE, FALSE))))
    {
      dir_context->sys_conf_dir = dir_context->sys_conf_file = NULL;
    }

  if (!result)
    {
      //  for error reporting  NOTE these may be NULL if we run config free
      dir_context->config_dir = config_dir;
      dir_context->config_file = config_file;
    }

  return result ;
}



/* Not sure if this is really necessary....maybe.... */
/* DO NOT FREE THE RESULTING STRING...COPY IF NEED BE.... */
const char *zMapConfigDirDefaultName(void)
{
  char *dir_name = ZMAP_USER_CONFIG_DIR ;

  return dir_name ;
}



char *zMapConfigDirGetDir(void)
{
  char *config_dir ;
  ZMapConfigDir dir_context = dir_context_G ;

  zMapAssert(dir_context) ;

  config_dir = dir_context->config_dir ;

  return config_dir ;
}


char *zMapConfigDirGetFile(void)
{
  char *config_file ;
  ZMapConfigDir dir_context = dir_context_G ;

  zMapAssert(dir_context) ;

  config_file = dir_context->config_file ;

  return config_file ;
}


char *zMapConfigDirFindFile(char *filename)
{
  char *file_path = NULL ;
  ZMapConfigDir dir_context = dir_context_G ;

  zMapAssert(dir_context) ;

  file_path = zMapGetFile(dir_context->config_dir, filename, FALSE) ;

  return file_path ;
}



char *zMapConfigDirFindDir(char *directory_in)
{
  char *control_dir ;

  control_dir = zMapGetDir(directory_in, TRUE, FALSE) ;

  return control_dir ;
}


char *zMapConfigDirGetZmapHomeFile(void)
{
  char *config_file ;
  ZMapConfigDir dir_context = dir_context_G ;

  zMapAssert(dir_context) ;

  config_file = dir_context->zmap_conf_file ;

  return config_file ;
}


char *zMapConfigDirGetSysFile(void)
{
  char *config_file ;
  ZMapConfigDir dir_context = dir_context_G ;

  zMapAssert(dir_context) ;

  config_file = dir_context->sys_conf_file ;

  return config_file ;
}


void zMapConfigDirDestroy(void)
{
  ZMapConfigDir dir_context = dir_context_G ;

  zMapAssert(dir_context) ;

  g_free(dir_context) ;

  return ;
}
