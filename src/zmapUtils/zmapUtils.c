/*  File: zmapUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2014: Genome Research Ltd.
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
 * Description: Utility functions for the ZMap code.
 *
 * Exported functions: See ZMap/zmapUtils.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#define _ISOC99_SOURCE

#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <config.h>
#include <unistd.h>

#include <zmapUtils_P.h>



static gboolean getVersionNumbers(char *version_str,
                                  int *version_out, int *release_out, int *update_out) ;


/* Can be set on/off to turn on/off debugging output via the zMapDebug() macro. */
gboolean zmap_debug_G = FALSE ;


/* A global timer used for giving overall timings for zmap operations.
 * See the timer macros in zmapUtilsDebug.h */
GTimer *zmap_global_timer_G = NULL ;
gboolean zmap_timing_G = FALSE;     // ouput timing info?


gboolean zmap_development_G = FALSE;    // switch on/off features while testing




/* 
 *                    External interface routines.
 */


/* Returns a string which is the name of the ZMap application.
 *
 * @param   void  None.
 * @return        The applications name as a string.
 *  */
char *zMapGetAppName(void)
{
  return ZMAP_TITLE ;
}


/* Returns the Version/Release/Update string, do _not_ free the returned string.
 * 
 * The string will be in the form produced by the configure.ac file,
 * see the version section near the top.
 */
char *zMapGetAppVersionString(void)
{
  char *version_string = NULL ;

  version_string = ZMAP_VERSION_NUMBER ;

  return version_string ;
}


/*!
 * Returns a string which is a brief decription of the ZMap application.
 *
 * @param   void  None.
 * @return        The applications description as a string.
 *  */
char *zMapGetAppTitle(void)
{
  char *title = ZMAP_TITLE " - "ZMAP_VERSION_NUMBER ;

  return title ;
}


/*!
 * Compares version strings in the format "VVVV.RRRR.UUUU", e.g. "4.8.3".
 * Returns TRUE if test_version is the same or newer than reference_version,
 * FALSE otherwise. The function is dumb in that it doesn't really check the format
 * of the strings so you put rubbish in, rubbish will come out.
 *
 * @param reference_version      The version against which the comparison will be made.
 * @param test_version           The version string to be tested.
 * @return                       TRUE or FALSE
 *  */
gboolean zMapCompareVersionStings(char *reference_version, char *test_version)
{
  gboolean result = FALSE ;
  char *ref_str, *test_str ;
  int ref_vers, ref_rel, ref_upd, test_vers, test_rel, test_upd ;

  ref_str = g_strdup(reference_version) ;
  test_str = g_strdup(test_version) ;

  if ((result = getVersionNumbers(ref_str, &ref_vers, &ref_rel, &ref_upd))
      && (result = getVersionNumbers(test_str, &test_vers, &test_rel, &test_upd)))
    {
      if (test_vers >= ref_vers && test_rel >= ref_rel && test_upd >= ref_upd)
        result = TRUE ;
      else
        result = FALSE ;
    }

  g_free(ref_str) ;
  g_free(test_str) ;

  return result ;
}


/*!
 * Returns a copyright string for the ZMap application.
 *
 * @param void  None.
 * @return      The copyright as a string.
 *  */
char *zMapGetCopyrightString(void)
{
  return ZMAP_COPYRIGHT_STRING() ;
}


/* Returns the Sanger ZMap website URL as a string.
 *
 *  */
char *zMapGetWebSiteString(void)
{
  char *website_string = NULL ;

  /* PACKAGE_URL is an automake/conf generated #define from information given to the AC_INIT
   * macro....on some of our systems the version of autoconf is too old to do this....hence
   * this hack... */
#ifndef PACKAGE_URL
#define PACKAGE_URL "http://www.sanger.ac.uk/resources/software/zmap/"
#endif

  website_string = PACKAGE_URL ;

  return website_string ;
}


/* Returns the ZMap internal shared website URL.
 *
 * @param void  None.
 * @return      The website as a string.
 *  */
char *zMapGetDevWebSiteString(void)
{
  return ZMAP_WEBSITE_STRING() ;
}


/*!
 * Returns a comments string for the ZMap application.
 *
 * @param void  None.
 * @return      The comments as a string.
 *  */
char *zMapGetCommentsString(void)
{
  char *comment_string =
    "ZMap is a multi-threaded genome viewer program\n"
    "that can be used stand alone or be driven from\n"
    "an external program to provide a seamless annotation\n"
    "package. It is currently used as part of the otterlace\n"
    "package and is being added to the core Wormbase\n"
    "annotation software.\n" ;

  return comment_string ;
}

/*!
 * Returns a license string for the ZMap application.
 *
 * @param void  None.
 * @return      The license as a string.
 *  */
char *zMapGetLicenseString(void)
{
  return ZMAP_LICENSE_STRING() ;
}



/* Attempt to make some kind of unique id. */
char *zMapMakeUniqueID(char *prefix)
{
  char *unique_id = NULL ;
  gulong pid_ulong, time_ulong ;

  pid_ulong = (gulong)getpid() ;
  time_ulong = (gulong)time(NULL) ;

  unique_id = g_strdup_printf("%s-%lu-%lu", prefix, pid_ulong, time_ulong) ;

  return unique_id ;
}



/* Convert a pointer to a string and vice-versa. */
const char *zMapUtilsPtr2Str(void *ptr)
{
  char *ptr_str ;

  ptr_str = g_strdup_printf("%p", ptr) ;

  return ptr_str ;
}

void *zMapUtilsStr2Ptr(char *ptr_str)
{
  void *ptr = NULL ;

  if (sscanf(ptr_str, "%p", &ptr) != 1)
    ptr = NULL ;

  return ptr ;
}


/* Note this function calls zmapCompileString() which is created by the Makefile
 * each time zmap is recompiled. The file is dynamically created so that it is not
 * constantly needing to be committed to GIT our source code control system. */
char *zMapGetCompileString(void)
{
  static char *compile_str = NULL ;

  if (!compile_str)
    compile_str = g_strdup_printf("(compiled on %s)", zmapCompileString()) ;

  return compile_str ;
}



/* Returns a string representing a format of the current time or NULL on error.
 * The returned string should be free'd with g_free() when no longer needed.
 *
 * If  format == ZMAPTIME_USERFORMAT  then time is formatted according to format_str.
 * Current formats are standard (i.e. as for 'date' command), astromonomical day form
 * or custom formatted.
 *
 * (N.B. just gets the current time, but could be altered to more generally get any time
 * and the time now by default.)
 *
 * format =   ZMAPTIME_STANDARD || ZMAPTIME_YMD || ZMAPTIME_LOG || ZMAPTIME_SEC_MICROSEC ||  ZMAPTIME_USERFORMAT
 * format_str = Format string in strftime() time format or NULL.
 * 
 * returns  Time according to format parameter or NULL on error.
 *  */
char *zMapGetTimeString(ZMapTimeFormat format, char *format_str_in)
{
  char *time_str = NULL ;
  enum {MAX_TIMESTRING = 1024} ;
  struct timeval tv;
  char *format_str ;
  char buffer[MAX_TIMESTRING] = {0} ;
  size_t buf_size = MAX_TIMESTRING ;
  time_t curtime ;

  zMapReturnValIfFail(((format == ZMAPTIME_STANDARD || format == ZMAPTIME_YMD
                        || format == ZMAPTIME_LOG || format == ZMAPTIME_SEC_MICROSEC)
                       || (format == ZMAPTIME_USERFORMAT && format_str_in && *format_str_in)), NULL) ;


  /* Get time of day as secs & microsecs. */
  gettimeofday(&tv, NULL) ; 
  curtime = tv.tv_sec ;

  switch(format)
    {
    case ZMAPTIME_STANDARD:
      /* e.g. "Thu Sep 30 10:05:27 2004" */
      format_str = "%a %b %d %X %Y" ;
      break ;
    case ZMAPTIME_YMD:
      /* e.g. "1997-11-08" */
      format_str = "%Y-%m-%d" ;
      break ;
    case ZMAPTIME_LOG:
      /* e.g. "2013/06/11 13:18:27.021129", i.e. includes milliseconds. */
      format_str = "%Y/%m/%d %T," ;
      break ;
    case ZMAPTIME_USERFORMAT:
      /* User provides format string which must be in strftime() format. */
      format_str = format_str_in ;
      break ;
    case ZMAPTIME_SEC_MICROSEC:
      format_str = "%ld" ZMAPTIME_SEC_MICROSEC_SEPARATOR "%ld" ;
      break ;
    default:
      zMapWarnIfReached() ;
      break ;
    }

  if (format == ZMAPTIME_SEC_MICROSEC)
    {
      time_str = g_strdup_printf(format_str, (long int)tv.tv_sec, (long int)(tv.tv_usec)) ;
    }
  else if ((strftime(&(buffer[0]), buf_size, format_str, localtime(&curtime))) > 0)
    {
      /* format the secs and then tack on the microsecs, pad to make sure we don't miss leading 0's */
      if (format == ZMAPTIME_LOG)
        time_str = g_strdup_printf("%s%06ld", buffer, (long int)(tv.tv_usec)) ;
      else
        time_str = g_strdup(buffer) ;
    }

  return time_str ;
}

/* Opposite of zMapGetTimeString(), kind of...as it operates not on current time
 * but the supplied time string. Currently only supports ZMAPTIME_SEC_MICROSEC, rest would need
 * to be added */
gboolean zMapTimeGetTime(char *time_str_in, ZMapTimeFormat format, char *format_str,
                         long int *secs_out, long int *microsecs_out)
{
  gboolean result = FALSE ;

  switch(format)
    {
    case ZMAPTIME_SEC_MICROSEC:
      {
        gchar **time_substrs ;
        long int secs = 0, microsecs = 0 ;

        if ((time_substrs = g_strsplit(time_str_in, ZMAPTIME_SEC_MICROSEC_SEPARATOR, 3)))
          {
            if (zMapStr2LongInt(time_substrs[0], &secs)
                && zMapStr2LongInt(time_substrs[1], &microsecs))
              {
                *secs_out = secs ;
                *microsecs_out = microsecs ;
                result = TRUE ;
              }

            g_strfreev(time_substrs) ;
          }

        break ;
      }
    default:
      zMapWarnIfReached() ;
      break ;
    }


 return result ;
}



/* Get raw microseconds time......here's what is returned to us:
 * 
 * struct timeval
 *   {
 *     time_t      tv_sec;     seconds
 *     suseconds_t tv_usec;    microseconds
 *   };
 *
 * We just use seconds.
 */
long int zMapUtilsGetRawTime(void)
{
  long int rawtime = 0 ;
  struct timeval tv = {0, 0} ;

  if (!gettimeofday(&tv, NULL))
    rawtime = tv.tv_sec ;

  return rawtime ;
}






/* Given an int between 0 and 9 returns the corresponding char representation,
 * (surely this must exist somewhere ??).
 *
 * Returns '.' if number not in [0-9]
 *  */
char zMapInt2Char(int num)
{
  char result = '.' ;
  char table[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'} ;

  if (num >= 0 && num <= 9)
    result = table[num] ;

  return result ;
}



/* There follow a series of conversion routines. These are provided either
 * because there are no existing ones or because the existing ones have
 * arcane usage.
 *
 * All of them can just be used to test for validity without needing a
 * return variable.
 *  */

gboolean zMapStr2Bool(char *str, gboolean *bool_out)
{
  gboolean result = FALSE ;
  gboolean retval ;

  if (str && *str)
    {
      if (g_ascii_strcasecmp("true", str) == 0)
        {
          retval = TRUE ;
          result = TRUE ;
        }
      else if (g_ascii_strcasecmp("false", str) == 0)
        {
          retval = FALSE ;
          result = TRUE ;
        }

      if (result && bool_out)
        *bool_out = retval ;
    }

  return result ;
}


gboolean zMapStr2Int(char *str, int *int_out)
{
  gboolean result = FALSE ;
  long int retval ;

  if (str && *str)
    {
      if (zMapStr2LongInt(str, &retval))
        {
          if (retval <= INT_MAX || retval >= INT_MIN)
            {
              if (int_out)
        *int_out = (int)retval ;
              result = TRUE ;
            }
        }
    }

  return result ;
}


gboolean zMapInt2Str(int int_in, char **str_out)
{
  gboolean result = FALSE ;
  char *str = NULL ;

  if (str_out)
    {
      /* I think this can never fail as the input is just an int. */
      str = g_strdup_printf("%d", int_in) ;
      if (str && *str)
        {
          if (str_out)
            *str_out = str ;
          result = TRUE ;
        }
    }

  return result ;
}


gboolean zMapStr2LongInt(char *str, long int *long_int_out)
{
  gboolean result = FALSE ;
  char *endptr = NULL;
  long int retval ;

  if (str && *str)
    {
      errno = 0 ;
      retval = strtol(str, &endptr, 10) ;

      if (retval == 0 && (errno != 0 || str == endptr))
        {
          /* Invalid string in some way. */
          result = FALSE ;
        }
      else if (*endptr != '\0')
        {
          /* non-digit found in string */
          result = FALSE;
        }
      else if (errno !=0 && (retval == LONG_MAX || retval == LONG_MIN))
        {
          /* Number exceeds long int size. */
          result = FALSE ;
        }
      else
        {
          result = TRUE ;
          if (long_int_out)
            *long_int_out = retval ;
        }
    }

  return result ;
}


gboolean zMapStr2Float(char *str, float *float_out)
{
  gboolean result = FALSE ;
  char *end_ptr = NULL ;
  float ret_val ;

  if (str && *str)
    {
      errno = 0 ;

      ret_val = strtof(str, &end_ptr) ;

      if (ret_val == 0 && end_ptr == str)
        {
          /* Standard says:  no conversion performed */
          result = FALSE ;
        }
      else if (*end_ptr != '\0')
        {
          /* Standard says:  string contains stuff that could not be converted. */
          result = FALSE ;
        }
      else if (errno == ERANGE)
        {
          if (ret_val == 0)
            {
              /* Standard says:  Underflow */
              result = FALSE ;
            }
          else
            {
              /* Standard says:  Overflow (+ or - HUGEVALF resturned. */
              result = FALSE ;
            }
        }
      else
        {
          result = TRUE ;
          if (float_out)
            *float_out = ret_val ;
        }
    }

  return result ;
}

gboolean zMapStr2Double(char *str, double *double_out)
{
  gboolean result = FALSE ;
  char *end_ptr = NULL ;
  double ret_val ;

  if (str && *str)
    {
      errno = 0 ;

      ret_val = strtod(str, &end_ptr) ;

      if (ret_val == 0 && end_ptr == str)
        {
          /* Standard says:  no conversion performed */
          result = FALSE ;
        }
      else if (*end_ptr != '\0')
        {
          /* Standard says:  string contains stuff that could not be converted. */
          result = FALSE ;
        }
      else if (errno == ERANGE)
        {
          if (ret_val == 0)
            {
              /* Standard says:  Underflow */
              result = FALSE ;
            }
          else
            {
              /* Standard says:  Overflow (+ or - HUGEVAL resturned. */
              result = FALSE ;
            }
        }
      else
        {
          result = TRUE ;
          if (double_out)
            *double_out = ret_val ;
        }
    }

  return result ;
}




/*!
 * Runs the system() call and returns TRUE if it worked or FALSE if it didn't.
 * If the call failed and err_msg_out is non-NULL then an error message is returned
 * explaining the failure. The caller should free error message using g_free() when
 * no longer required.
 *
 * It is not straight forward to interpret the return value of the system call so this
 * function attempts to interpret the value correctly.
 *
 * @param cmd_str       The command to be run (exactly as it would be for the Bourne shell).
 * @param err_msg_out   A pointer to an internal error message is returned here.
 * @return              TRUE if system command succeeded, FALSE otherwise.
 *  */
gboolean zMapUtilsSysCall(char *cmd_str, char **err_msg_out)
{
  gboolean result = FALSE ;
  int sys_rc ;

  sys_rc = system(cmd_str) ;

  if (cmd_str == NULL)
    {
      /* Deal with special return codes when cmd is NULL. */

      if (sys_rc != 0)
        result = TRUE ;
      else
        {
          result = FALSE ;
          if (err_msg_out)
            *err_msg_out = g_strdup("No shell available !") ;
        }
    }
  else
    {
      /* All other command strings... */

      if (sys_rc == -1)
        {
          result = FALSE ;
          if (err_msg_out)
            *err_msg_out = g_strdup_printf("Failed to run child process correctly, sys error: %s.",
           g_strerror(errno)) ;
        }
      else
        {
          /* If the shell cannot be exec'd the docs say it behaves as though a command returned
           * exit(127) but I don't know if you can disambiguate this from any other command that
           * might return 127..... */
        
          if (!WIFEXITED(sys_rc))
            {
              result = FALSE ;
              if (err_msg_out)
                *err_msg_out = g_strdup("Child process did not exit normally.") ;
            }
          else
            {
              /* you can only use WEXITSTATUS() if WIFEXITED() is true. */
              int true_rc ;
        
              true_rc = WEXITSTATUS(sys_rc) ;
        
              if (true_rc == EXIT_SUCCESS)
                result = TRUE ;
              else
                {
                  result = FALSE ;
                  if (err_msg_out)
                    *err_msg_out = g_strdup_printf("Command \"%s\" failed with return code %d",
                   cmd_str, true_rc) ;
                }
            }
        }
    }


  return result ;
}



/* Takes an integer that is the termination status of a process (e.g. as generated
 * by waitpid() etc) and returns the actual return code form the child and also
 * the a termination type in termination_type_out. See ZMapProcessTerminationType
 * in zmapUtils.h for meaning of value returned.
 *  */
int zMapUtilsProcessTerminationStatus(int status, ZMapProcessTerminationType *termination_type_out)
{
  int exit_rc = EXIT_FAILURE ;
  ZMapProcessTerminationType termination_type = ZMAP_PROCTERM_OK ;

  if (WIFEXITED(status))
    {
      exit_rc = WEXITSTATUS(status) ;

      if (exit_rc)
        *termination_type_out = ZMAP_PROCTERM_ERROR ;
      else
        *termination_type_out =  ZMAP_PROCTERM_OK ;
    }
  else if (WIFSIGNALED(status))
    {
      *termination_type_out = ZMAP_PROCTERM_SIGNAL ;
    }
  else if (WIFSTOPPED(status))
    {
      *termination_type_out = ZMAP_PROCTERM_STOPPED ;
    }
  else
    {
      /* No other child status should be returned for a normally executing zmap. */
      zMapWarnIfReached() ;
    }

  return exit_rc ;
}

/* auto define function to take a ZMapProcessTerminationType and return a short descriptive string. */
ZMAP_ENUM_TO_SHORT_TEXT_FUNC(zmapProcTerm2ShortText, ZMapProcessTerminationType, ZMAP_PROCTERM_LIST) ;

/* make printing from totalview evaluations a lot easier... */
void zMapPrintQuark(GQuark quark)
{
  printf("GQuark (%d) = '%s'\n", quark, g_quark_to_string(quark));
  return ;
}

/* make logging from totalview evaluations a lot easier... */
void zMapLogQuark(GQuark quark)
{
  zMapLogMessage("GQuark (%d) = '%s'", quark, g_quark_to_string(quark));
  return ;
}



/*! Case insensitive compare.
 * @param quark         Quark to be stringified and compared.
 * @param str           String to be compared.
 * @return              TRUE if compared worked, FALSE otherwise.
 */
gboolean zMapLogQuarkIsStr(GQuark quark, char *str)
{
  gboolean result = FALSE ;

  if (g_ascii_strcasecmp(g_quark_to_string(quark), str) == 0)
    result = TRUE ;

  return result ;
}

/*! Case sensitive compare.
 * @param quark         Quark to be stringified and compared.
 * @param str           String to be compared.
 * @return              TRUE if compared worked, FALSE otherwise.
 */
gboolean zMapLogQuarkIsExactStr(GQuark quark, char *str)
{
  gboolean result = FALSE ;

  if (strcmp(g_quark_to_string(quark), str) == 0)
    result = TRUE ;

  return result ;
}


/*! Is str in quark.
 * @param quark         Quark to be stringified and compared.
 * @param sub_str       Sub-string to be compared.
 * @return              TRUE if compared worked, FALSE otherwise.
 */
gboolean zMapLogQuarkHasStr(GQuark quark, char *sub_str)
{
  gboolean result = FALSE ;

  if ((strstr(g_quark_to_string(quark), sub_str)))
    result = TRUE ;

  return result ;
}


/* Call directly or via zMapDebugPrint() macros. Does a fflush every time to 
 * ensure messages are seen in a timely way. */
void zMapUtilsDebugPrintf(FILE *stream, char *format, ...)
{
  va_list args1, args2 ;

  va_start(args1, format) ;
  G_VA_COPY(args2, args1) ;

  vfprintf(stream, format, args2) ;
  fflush(stream) ;

  va_end(args1) ;
  va_end(args2) ;

  return ;
}





/*! @} end of zmaputils docs. */




/*
 *                   Internal routines.
 */




static gboolean getVersionNumbers(char *version_str,
  int *version_out, int *release_out, int *update_out)
{
  gboolean result = FALSE ;
  char *next ;
  int version, release, update ;

  if (((next = strtok(version_str, "."))
       && (version = atoi(next)))
      && ((next = strtok(NULL, "."))
  && (release = atoi(next)))
      && ((next = strtok(NULL, "."))
  && (update = atoi(next))))
    {
      result = TRUE ;
      *version_out = version ;
      *release_out = release ;
      *update_out = update ;
    }


  return result ;
}


