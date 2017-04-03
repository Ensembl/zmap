/*  File: zmapSignals.c
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
 * Description: Signal (as in Unix signals) handling functions.
 *
 * Exported functions: See ZMap/zMapUtils.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>


#include <unistd.h>
#include <sys/types.h>
#include <string.h>

#include <ZMap/zmapUtils.hpp>


static void signal_write(int fd, const char *buffer, int size, gboolean *result_in_out) ;



/*
 *                 Globals
 */

static gboolean enable_core_dumping_G = TRUE;



/*
 *                    External interface routines
 */


/* Our Unix signal handler function, prints a stack trace for some signals. */
void zMapSignalHandler(int sig_no)
{
  const char *sig_name = NULL, *pid_output = NULL ;
  int sig_name_len = 0 ;
  gboolean write_result = TRUE ;

  /* ensure we don't get back in here. */
  /* If we return rather than exit, we'll also produce a core file. */
  signal(sig_no, SIG_DFL);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  pid_output = g_strdup_printf("(ZMAP_PID = %ld)", getpid() ) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  pid_output = g_strdup_printf("(ZMAP_PID = %d)", getpid() ) ;

  switch(sig_no)
    {
    case SIGSEGV:
      sig_name = "SIGSEGV";
      sig_name_len = 7;
      break;
    case SIGBUS:
      sig_name = "SIGBUS";
      sig_name_len = 6;
      break;
    case SIGABRT:
      sig_name = "SIGABRT";
      sig_name_len = 7;
      break;
    default:
      break;
    }

  /* Only do stuff segv, abrt & bus signals */
  switch(sig_no)
    {
    case SIGSEGV:
    case SIGBUS:
    case SIGABRT:
      /*                           123456789012345678901234567890123456789012345678901234567890 */
      signal_write(STDERR_FILENO, "=== signal handler - caught signal ", 35, &write_result);
      signal_write(STDERR_FILENO, sig_name, sig_name_len, &write_result);
      signal_write(STDERR_FILENO, " - ZMap ", 8, &write_result);
      signal_write(STDERR_FILENO, zMapGetAppVersionString(), strlen(zMapGetAppVersionString()), &write_result);
      signal_write(STDERR_FILENO, pid_output, strlen(pid_output), &write_result) ;
      signal_write(STDERR_FILENO, " ===\nStack:\n", 12, &write_result);
      /*                           123456789012345678901234567890123456789012345678901234567890 */
      if (!zMapStack2fd(0, STDERR_FILENO))
        signal_write(STDERR_FILENO, "*** no backtrace() available ***\n", 33, &write_result);

      /* If we exit, no core file is dumped. */
      if (!enable_core_dumping_G)
        _exit(EXIT_FAILURE);
      break;
    default:
      break;
    }

  if (pid_output)
    g_free((void *)pid_output) ;

  return ;
}



/*
 *                    Internal routines
 */


static void signal_write(int fd, const char *buffer, int size, gboolean *result_in_out)
{
  if (result_in_out && (*result_in_out == TRUE))
    {
      ssize_t expected = size, actual = 0 ;

      if ((write(fd, (void *)NULL, 0)) == 0)
        {
          actual = write(fd, (void *)buffer, size) ;

          if (actual == 0 || actual == -1)
            *result_in_out = FALSE ;
          else if (actual == expected)
            *result_in_out = TRUE ;
        }
      else
        {
          *result_in_out = FALSE ;
        }
    }

  return ;
}

