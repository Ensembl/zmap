/*  File: zmapFileUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Set of file handling utilities.
 *              
 * Exported functions: See ZMap/zmapUtils.h
 * HISTORY:
 * Last edited: May  7 08:17 2004 (edgrif)
 * Created: Thu May  6 15:16:05 2004 (edgrif)
 * CVS info:   $Id: zmapFileUtils.c,v 1.1 2004-05-07 09:27:01 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <zmapUtils_P.h>


/* Not sure if this is really necessary....maybe.... */
/* DO NOT FREE THE RESULTING STRING...COPY IF NEED BE.... */
const char *zMapGetControlDirName(void)
{
  char *dir_name = ZMAP_USER_CONTROL_DIR ;

  return dir_name ;
}


/* If directory is absolute then returns a copy of that directory, if the directory is relative
 * then returns $HOME/directory. Useful because when people specify directories in resource
 * files then this makes life easier. */
char *zMapGetControlFileDir(char *directory_in)
{
  char *control_dir ;

  control_dir = zMapGetDir(directory_in, TRUE) ;

  return control_dir ;
}


/* this function also only copes with one level of directory creating.... */
/* this function is whacky at the moment and needs work.....e.g. on directory permissions... */
/* Check directory. */
char *zMapGetDir(char *directory_in, gboolean home_relative)
{
  char *directory = NULL ;
  gboolean absolute ;
  gboolean status = FALSE ;
  char *base_dir ;
  struct stat stat_buf ;

  /* If directory is relative to HOME then prepend HOME. */
  if (home_relative)
    {
      base_dir = g_get_home_dir() ;			    /* glib docs say home_dir string should not
							       be freed. */
      directory = g_build_path(ZMAP_SEPARATOR, base_dir, directory_in, NULL) ;
    }
  else if (!g_path_is_absolute(directory_in))
    {
      base_dir = g_get_current_dir() ;
      directory = g_build_path(ZMAP_SEPARATOR, base_dir, directory_in, NULL) ;
      g_free(base_dir) ;
    }
  else
    {
      directory = g_strdup(directory_in) ;
    }


  /* If directory exists, is it readable/writeable/executable, if not, then make it. */
  if (stat(directory, &stat_buf) == 0)
    {
      if (S_ISDIR(stat_buf.st_mode) && (stat_buf.st_mode & S_IRWXU))
	{
	  status = TRUE ;
	}
      /* If this fails we should probably try to change the mode to be rwx for user only. */
    }
  else
    {
      if (mkdir(directory, S_IRWXU) == 0)
	{
	  status = TRUE ;
	}
    }

  if (!status)
    {
      g_free(directory) ;
      directory = NULL ;
    }

  return directory ;
}



/* Can we access given file in given directory. */
char *zMapGetFile(char *directory, char *filename)
{
  gboolean status = FALSE ;
  char *filepath ;
  struct stat stat_buf ;

  filepath = g_build_path(ZMAP_SEPARATOR, directory, filename, NULL) ;

  /* Is there a configuration file in the config dir that is readable/writeable ? */
  if (stat(filepath, &stat_buf) == 0)
    {
      if (S_ISREG(stat_buf.st_mode) && (stat_buf.st_mode & S_IRWXU))
	{
	  status = TRUE ;
	}
    }
  else
    {
      int file ;

      if ((file = open(filepath, (O_RDWR | O_CREAT | O_EXCL), (S_IRUSR | S_IWUSR)) != -1)
	  && (close(file) != -1))
	status = TRUE ;
    }

  if (!status)
    {
      g_free(filepath) ;
      filepath = NULL ;
    }

  return filepath ;
}


/* Is the a file empty ? */
gboolean zMapFileEmpty(char *filepath)
{
  gboolean result = FALSE ;
  struct stat stat_buf ;

  if (stat(filepath, &stat_buf) == 0)
    {
      if (stat_buf.st_size == 0)
	{
	  result = TRUE ;
	}
    }

  return result ;
}






