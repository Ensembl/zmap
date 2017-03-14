/*  File: zmapFileUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Set of file handling utilities.
 *
 * Exported functions: See ZMap/zmapUtils.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <wordexp.h>

#include <zmapUtils_P.hpp>

static char *getRelOrAbsPath(char *path_in, gboolean home_relative) ;


/* SORT OUT THIS FILE, THE FUNCTIONS ALL NEED RATIONALISING INTO SOMETHING MORE SENSIBLE... */


/* this function also only copes with one level of directory creating.... */
/* this function is whacky at the moment and needs work.....e.g. on directory permissions... */
/* Check directory. */
char *zMapGetDir(const char *directory_in, gboolean home_relative, gboolean make_dir)
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


/* Get the zmap temp directory. This is used e.g. for storing temp gff files to send to blixem,
 * temp cache location for processing BED files etc. */
const char *zMapGetTmpDir()
{
  static char *tmp_dir = NULL ;

  if (!tmp_dir)
    {
      const char *login = (char *)g_get_user_name() ;

      if (login)
        {
          char *path = g_strdup_printf("/tmp/zmap_%s/", login);

          if (path)
            {
              tmp_dir = zMapGetDir(path, FALSE, TRUE) ;

              if (tmp_dir)
                {
                  mode_t mode = (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) ;

                  if (chmod(tmp_dir, mode) != 0)
                    {
                      zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not set permissions on temp directory:  %s.", tmp_dir) ;
                      g_free(tmp_dir) ;
                      tmp_dir = NULL ;
                    }
                }
              else
                {
                  zMapShowMsg(ZMAP_MSG_WARNING, "Program error: could not create tmp directory for path: %s", path);
                }

              g_free(path) ;
            }
          else
            {
              zMapShowMsg(ZMAP_MSG_WARNING, "Program error: could not create tmp directory path.");
            }
        }
      else
        {
          zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not create tmp directory: could not determine your login.");
        }
    }

  return tmp_dir ;
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
char *zMapExpandFilePath(const char *path_in)
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
 * doesn't exist then create the file as read/writeable but empty.'permissions' should be
 * the required permissions, e.g. 'rw' if read and write access are required   */
char *zMapGetFile(char *directory, const char *filename, gboolean make_file, const char *permissions, GError **error)
{
  gboolean status = FALSE ;
  GError *g_error = NULL ;
  char *filepath ;
  /* struct stat stat_buf ; */

  filepath = g_build_path(ZMAP_SEPARATOR, directory, filename, NULL) ;

  /* Is there a configuration file in the config dir that is readable/writeable ? */
  if (!(status = zMapFileAccess(filepath, permissions)))
    {
      if (make_file)
        {
          int file ;

          if ((file = open(filepath, (O_RDWR | O_CREAT | O_EXCL), (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) != -1)
              && (close(file) != -1))
            {
              status = TRUE ;
            }
          else
            {
              g_set_error(&g_error, ZMAP_UTILS_ERROR, ZMAPUTILS_ERROR_OPEN_FILE,
                          "Cannot create file '%s'", filepath) ;
            }
        }
      else
        {
          g_set_error(&g_error, ZMAP_UTILS_ERROR, ZMAPUTILS_ERROR_OPEN_FILE,
                      "Cannot access file '%s'", filepath) ;
        }
    }

  if (!status)
    {
      g_free(filepath) ;
      filepath = NULL ;
    }

  if (g_error)
    g_propagate_error(error, g_error) ;

  return filepath ;
}


/* Can the given file be accessed for read or write or execute ?
 * If no mode then test is for "rwx". */
gboolean zMapFileAccess(const char *filepath, const char *mode)
{
  gboolean can_access = FALSE ;
  struct stat stat_buf ;

  /* zMapAssert(filepath && *filepath) ; */
  if (!filepath || !*filepath)
    return can_access ;

  if (stat(filepath, &stat_buf) == 0 && S_ISREG(stat_buf.st_mode))
    {
      if (!mode)
        mode = "rwx" ;

      int permissions = F_OK ;

      if (strstr(mode, "r"))
        permissions = R_OK ;

      if (strstr(mode, "w"))
        permissions |= W_OK;

      if (strstr(mode, "x"))
        permissions |= X_OK;

      can_access = (access(filepath, permissions) == 0) ;
    }

  return can_access ;
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
          base_dir = (char *)g_get_home_dir() ;		    /* glib docs say home_dir string should not be freed. */
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






