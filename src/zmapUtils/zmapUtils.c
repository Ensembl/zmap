/*  File: zmapUtils.c
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
 * Description: Utility functions for the ZMap code.
 *              
 * Exported functions: See ZMap/zmapUtils.h
 * HISTORY:
 * Last edited: Jul 14 11:23 2006 (edgrif)
 * Created: Fri Mar 12 08:16:24 2004 (edgrif)
 * CVS info:   $Id: zmapUtils.c,v 1.17 2006-07-14 10:24:09 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <gtk/gtk.h>

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#include <ZMap/zmapUtilsGUI.h>
#include <zmapUtils_P.h>



static gboolean getVersionNumbers(char *version_str,
				  int *version_out, int *release_out, int *update_out) ;


/*! @defgroup zmaputils   zMapUtils: utilities for ZMap
 * @{
 * 
 * \brief  Utilities for ZMap.
 * 
 * zMapUtils routines provide services such as debugging, testing and logging,
 * string handling, file utilities and GUI functions. They are general routines
 * used by all of ZMap.
 *
 *  */


/*! Can be set on/off to turn on/off debugging output via the zMapDebug() macro. */
gboolean zmap_debug_G = FALSE ; 



/*!
 * Use this routine to exit the application with a portable (as in POSIX) return
 * code. If exit_code == 0 then application exits with EXIT_SUCCESS, otherwise
 * exits with EXIT_FAILURE. This routine actually calls gtk_exit() because ZMap
 * is a gtk routine and should use this call to exit.
 *
 * @param exit_code              0 for success, anything else for failure.
 * @return                       nothing
 *  */
void zMapExit(int exit_code)
{
  int true_exit_code ;

  if (exit_code)
    true_exit_code = EXIT_FAILURE ;
  else
    true_exit_code = EXIT_SUCCESS ;

  gtk_exit(true_exit_code) ;

  return ;						    /* we never get here. */
}



/*!
 * Returns a single number representing the Version/Release/Update of the ZMap code.
 *
 * @param   void  None.
 * @return        The version as an integer.
 *  */
int zMapGetVersion(void)
{
  return ZMAP_MAKE_VERSION_NUMBER(ZMAP_VERSION, ZMAP_RELEASE, ZMAP_UPDATE) ;
}


/*!
 * Returns a string representing the Version/Release/Update of the ZMap code.
 *
 * @param   void  None.
 * @return        The version number as a string.
 *  */
char *zMapGetVersionString(void)
{
  return ZMAP_MAKE_VERSION_STRING(ZMAP_VERSION, ZMAP_RELEASE, ZMAP_UPDATE) ;
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
 * Returns a string which is the name of the ZMap application.
 *
 * @param   void  None.
 * @return        The applications name as a string.
 *  */
char *zMapGetAppName(void)
{
  return ZMAP_TITLE ;
}


/*!
 * Returns a string which is a brief decription of the ZMap application.
 *
 * @param   void  None.
 * @return        The applications description as a string.
 *  */
char *zMapGetAppTitle(void)
{
  return ZMAP_MAKE_TITLE_STRING(ZMAP_TITLE, ZMAP_VERSION, ZMAP_RELEASE, ZMAP_UPDATE) ;
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

/*!
 * Returns a comments string for the ZMap application.
 *
 * @param void  None.
 * @return      The comments as a string.
 *  */
char *zMapGetCommentsString(void)
{
  return ZMAP_COMMENTS_STRING(ZMAP_TITLE, ZMAP_VERSION, ZMAP_RELEASE, ZMAP_UPDATE) ;
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



/*!
 * Formats and displays a message, note that the message may be displayed in a window
 * or at the terminal depending on how the message system has been initialised.
 *
 * @param msg_type      The type of message: information, warning etc.
 * @param format        A printf() style format string.
 * @param ...           The parameters matching the format string.
 * @return              nothing
 *  */
void zMapShowMsg(ZMapMsgType msg_type, char *format, ...)
{
  va_list args ;
  char *msg_string ;

  va_start(args, format) ;
  msg_string = g_strdup_vprintf(format, args) ;
  va_end(args) ;


  zMapGUIShowMsg(msg_type, msg_string) ;
  
  g_free(msg_string) ;

  return ;
}


/*!
 * Returns a string representing a format of the current time or NULL on error.
 * The returned string should be free'd with g_free() when no longer needed.
 *
 * If  format == ZMAPTIME_USERFORMAT  then time is formatted according to format_str.
 * Current formats are standard (i.e. as for 'date' command), astromonomical day form
 * or custom formatted.
 *
 * (N.B. just gets the current time, but could be altered to more generally get any time
 * and the time now by default.)
 *
 * @param   format    ZMAPTIME_STANDARD || ZMAPTIME_YMD || ZMAPTIME_USERFORMAT
 * @param   format_str  Format string in strftime() time format or NULL.
 * @return  Time according to format parameter or NULL on error.
 *  */
char *zMapGetTimeString(ZMapTimeFormat format, char *format_str_in)
{
  enum {MAX_TIMESTRING = 1024} ;
  char *time_str = NULL ;
  time_t now ;
  struct tm *time_struct ;
  char *format_str ;
  char buffer[MAX_TIMESTRING] = {0} ;
  size_t buf_size = MAX_TIMESTRING, bytes_written ;


  zMapAssert(format == ZMAPTIME_STANDARD || format == ZMAPTIME_YMD
	     || (format == ZMAPTIME_USERFORMAT && format_str_in && *format_str_in)) ;


  /* Get time ready for formatting. */
  now = time((time_t *)NULL) ;
  time_struct = localtime(&now) ;

  switch(format)
    {
    case ZMAPTIME_STANDARD:
      format_str = "%a %b %d %X %Y" ;
      break ;
    case ZMAPTIME_YMD:
      format_str = "%Y-%m-%d" ;
      break ;
    case ZMAPTIME_USERFORMAT:
      format_str = format_str_in ;
      break ;
    }


  if ((bytes_written = strftime(&(buffer[0]), buf_size, format_str, time_struct)) > 0)
    {
      time_str = g_strdup(&(buffer[0])) ;
    }


  return time_str ;
}



gboolean zMapStr2Int(char *str, int *int_out)
{
  gboolean result = FALSE ;
  long int retval ;

  zMapAssert(str && *str) ;


  if (zMapStr2LongInt(str, &retval))
    {
      if (retval <= INT_MAX || retval >= INT_MIN)
	{
	  *int_out = (int)retval ;
	  result = TRUE ;
	}
    }

  return result ;
}


gboolean zMapInt2Str(int int_in, char **str_out)
{
  gboolean result = FALSE ;
  char *str = NULL ;

  zMapAssert(str_out) ;

  /* I think this can never fail as the input is just an int. */
  str = g_strdup_printf("%d", int_in) ;
  if (str && *str)
    {
      *str_out = str ;
      result = TRUE ;
    }

  return result ;
}


gboolean zMapStr2LongInt(char *str, long int *long_int_out)
{
  gboolean result = FALSE ;
  char *endptr = NULL;
  long int retval ;

  zMapAssert(str && *str) ;

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
      *long_int_out = retval ;
    }


  return result ;
}


gboolean zMapStr2Double(char *str, double *double_out)
{
  gboolean result = FALSE ;
  char *end_ptr ;
  double ret_val ;

  zMapAssert(str && *str) ;

  errno = 0 ;
 
  ret_val = strtod(str, &end_ptr) ;

  if (*end_ptr != '\0' || errno != 0)
    result = FALSE ;
  else
    {
      result = TRUE ;
      if (double_out)
	*double_out = ret_val ;
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


