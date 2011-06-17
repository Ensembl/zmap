/*  File: zmapFileUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Set of file handling utilities.
 *              
 * Exported functions: See ZMap/zmapUtils.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <wordexp.h>
#include <zmapUtils_P.h>


static char *getRelOrAbsPath(char *path_in, gboolean home_relative) ;


/* SORT OUT THIS FILE, THE FUNCTIONS ALL NEED RATIONALISING INTO SOMETHING MORE SENSIBLE... */


/* this function also only copes with one level of directory creating.... */
/* this function is whacky at the moment and needs work.....e.g. on directory permissions... */
/* Check directory. */
char *zMapGetDir(char *directory_in, gboolean home_relative, gboolean make_dir)
{
  char *directory = NULL ;
  gboolean status = FALSE ;
  char *base_dir ;
  struct stat stat_buf ;

  /* If directory is relative to HOME then prepend HOME. */
  if (home_relative)
    {
      base_dir = (char *)g_get_home_dir() ;		    /* glib docs say home_dir string should not
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
  else if (make_dir)
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




/* If path is absolute then returns a copy of that path, if the path is relative
 * then returns $PWD/path. */
char *zMapGetPath(char *path_in)
{
  char *path ;

  path = getRelOrAbsPath(path_in, FALSE) ;

  return path ;
}


/* This functions takes a string like:
 * 
 * /nfs/team71/acedb/edgrif/ZMap/ZMap_Curr/ZMap/src/build/linux/../../zmapControl/remote/zmapXRemote.c
 * 
 * and returns a pointer in the _same_ string to the filename at the end, e.g.
 * 
 * zmapXRemote.c
 * 
 * Currently we just call g_basename() but this is a glib deprecated function so we may have
 * to do our own version one day....
 * 
 *  */
char *zMapGetBasename(char *path_in)
{
  char *basename ;

  basename = (char *) g_basename(path_in) ;

  return basename ;
}


/* This function will expand the given path according in the same way that
 * the shell does including "~" expansion. The underlying wordexp()
 * function actually does glob style expansion so could return many values
 * but we assume that the path will be the first one and return that.
 * 
 * We could put checks in to make sure that path_in does not contain
 * wildcard characters but the onus is on the caller to just be sensible.
 * If no path_in is passed in or path_in contains invalid chars then NULL
 * is returned otherwise the expanded filepath is returned, this string
 * should be released with g_free() when no longer needed.
 */
char *zMapExpandFilePath(char *path_in)
{
  char *filepath = NULL ;

  if (path_in && *path_in)
    {
      wordexp_t p = {0} ;				    /* wordaround for MacOS X failure
							       to zero it when !WE_DOOFFS */

      if (wordexp(path_in, &p, 0) == 0)
	{
	  char **w ;

	  w = p.we_wordv;

	  filepath = g_strdup(w[0]) ;

	  wordfree(&p) ;
	}
    }

  return filepath ;
}





/* Construct file path from directory and filename and check if it can be accessed, if it
 * doesn't exist then create the file as read/writeable but empty.  */
char *zMapGetFile(char *directory, char *filename, gboolean make_file)
{
  gboolean status = FALSE ;
  char *filepath ;
  /* struct stat stat_buf ; */

  filepath = g_build_path(ZMAP_SEPARATOR, directory, filename, NULL) ;

  /* Is there a configuration file in the config dir that is readable/writeable ? */
  if (!(status = zMapFileAccess(filepath, "rw")) && make_file)
    {
      int file ;

      if ((file = open(filepath, (O_RDWR | O_CREAT | O_EXCL), (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) != -1)
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


/* Can the given file be accessed for read or write or execute ?
 * If no mode then test is for "rwx". */
gboolean zMapFileAccess(char *filepath, char *mode)
{
  gboolean access = FALSE ;
  struct stat stat_buf ;
  
  zMapAssert(filepath && *filepath) ;

  if (stat(filepath, &stat_buf) == 0 && S_ISREG(stat_buf.st_mode))
    {
      if (!mode)
	mode = "rwx" ;

      access = TRUE ;

      if (access && strstr(mode, "r") && !(stat_buf.st_mode & S_IRUSR))
	access = FALSE ;

      if (access && strstr(mode, "w") && !(stat_buf.st_mode & S_IWUSR))
	access = FALSE ;

      if (access && strstr(mode, "x") && !(stat_buf.st_mode & S_IXUSR))
	access = FALSE ;
    }

  return access ;
}


/* Is the a file empty ? */
gboolean zMapFileEmpty(char *filepath)
{
  gboolean result = FALSE ;
  struct stat stat_buf ;

  if (stat(filepath, &stat_buf) == 0
      && stat_buf.st_size == 0)
    result = TRUE ;

  return result ;
}




/* 
 *             Internal functions.
 * 
 */


/* If path is absolute then returns a copy of that path, if the path is relative
 * then returns $PWD/path. */
static char *getRelOrAbsPath(char *path_in, gboolean home_relative)
{
  char *path = NULL ;
  /* gboolean absolute ;
     gboolean status = FALSE ; */
  char *base_dir ;
  /* struct stat stat_buf ; */

  if (g_path_is_absolute(path_in))
    {
      path = g_strdup(path_in) ;
    }
  else
    {
      /* If path is relative to HOME then prepend HOME. */
      if (home_relative)
	{
	  base_dir = (char *)g_get_home_dir() ;		    /* glib docs say home_dir string should not
							       be freed. */
	  path = g_build_path(ZMAP_SEPARATOR, base_dir, path_in, NULL) ;
	}
      else
	{
	  base_dir = g_get_current_dir() ;
	  path = g_build_path(ZMAP_SEPARATOR, base_dir, path_in, NULL) ;
	  g_free(base_dir) ;
	}
    }

  return path ;
}






