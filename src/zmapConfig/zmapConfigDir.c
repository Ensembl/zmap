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
 * Description:
 *
 * Exported functions: See ZMap/zmapConfigDir.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <glib.h>

#include <ZMap/zmapUtils.h>
#include <zmapConfigDir_P.h>
#include <zmapConfigIni_P.h>





static ZMapConfigDir dir_context_G = NULL ;



/* The context is only created the first time this function is called, after that
 * it just returns a reference to the existing context.
 *
 * Note that as this routine should be called once per application it is not
 * thread safe, it could be made so but is overkill as the GUI/master thread is
 * not thread safe anyway.
 *
 * If config_dir is NULL, the default ~/.ZMap directory is used.
 * If config_file is NULL, the default ZMap file is used.
 *
 * returns FALSE if the configuration directory does not exist/read/writeable.
 *
 * if file_opt then config files must be explicit and if not specified we run config free
 * which is done by creating a config string in memory as if it came from a file
 * there's code that assumes we have a file and directory so we get to keep this regardless
 */
gboolean zMapConfigDirCreate(char *config_dir, char *config_file, gboolean file_opt)
{
  gboolean result = FALSE ;
  ZMapConfigDir dir_context = NULL ;
  gboolean home_relative = FALSE, make_dir = FALSE ;
  char *zmap_home;

  g_return_val_if_fail(dir_context_G == NULL, FALSE);

  dir_context_G = dir_context = g_new0(ZMapConfigDirStruct, 1) ;

  if (!config_file && (config_dir || !file_opt))
    config_file = ZMAP_USER_CONFIG_FILE ;

  if (!config_dir)
    {
	   if(config_file || !file_opt)
	   {
		config_dir = ZMAP_USER_CONFIG_DIR ;
		home_relative = TRUE ;
	   }
    }
  else if(*config_dir == '~' && *(config_dir + 1) == '/')
    {
      config_dir += 2 ;
      home_relative = TRUE ;
    }

  if(config_dir && config_file)
  {
	if ((dir_context->config_dir = zMapGetDir(config_dir, home_relative, make_dir))
		&& (dir_context->config_file = zMapGetFile(dir_context->config_dir, config_file, FALSE)))
	result = TRUE ;
  }
  else if(file_opt)
  {
	  result = TRUE;		/* we make up a config file internally */
  }

  if((zmap_home = getenv("ZMAP_HOME")))
    {
      zmap_home = g_strdup_printf("%s/etc", zmap_home);
      if((dir_context->zmap_conf_dir  = zMapGetDir(zmap_home, FALSE, FALSE)))
	dir_context->zmap_conf_file = zMapGetFile(dir_context->zmap_conf_dir, ZMAP_USER_CONFIG_FILE, FALSE);
      g_free(zmap_home);
    }
  else
    dir_context->zmap_conf_dir = dir_context->zmap_conf_file = NULL;

  if(!((dir_context->sys_conf_dir  = zMapGetDir("/etc", FALSE, FALSE)) &&
       (dir_context->sys_conf_file = zMapGetFile(dir_context->sys_conf_dir, ZMAP_USER_CONFIG_FILE, FALSE))))
    {
      dir_context->sys_conf_dir = dir_context->sys_conf_file = NULL;
    }

  if(!result)
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

#warning this free causes memory corruption
//  g_free(dir_context->config_dir) ;

  g_free(dir_context) ;

  return ;
}
