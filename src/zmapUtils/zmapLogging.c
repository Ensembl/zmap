/*  File: zmapLogging.c
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
 * Description: 
 * Exported functions: See ZMap/zmapUtils.h
 * HISTORY:
 * Last edited: May 12 16:14 2004 (edgrif)
 * Created: Thu Apr 29 14:59:37 2004 (edgrif)
 * CVS info:   $Id: zmapLogging.c,v 1.2 2004-05-17 14:24:33 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <stdio.h>

#include <glib.h>
#include <ZMap/zmapConfig.h>
#include <ZMap/zmapUtils.h>


#define ZMAPLOG_FILENAME "zmap.log"

typedef struct
{
  guint cb_id ;						    /* needed by glib to install/uninstall
							       handler routines. */
  GLogFunc log_cb ;					    /* handler routine must change to turn
							       logging on/off. */

  /* log file name/file etc. */
  char *log_path ;
  GIOChannel* logfile ;


} zmapLogHandlerStruct, *zmapLogHandler ;


typedef struct  _ZMapLogStruct
{
  gboolean logging ;					    /* logging on or off ? */
  gboolean log_to_file ;				    /* log to file or leave glib to log to
							       stdout/err ? */
  zmapLogHandlerStruct active_handler ;			    /* Used when logging is on. */
  zmapLogHandlerStruct inactive_handler ;		    /* Used when logging is off. */

} ZMapLogStruct ;




static ZMapLog createLog(void) ;
static void destroyLog(ZMapLog log) ;

static gboolean configureLog(ZMapLog log) ;
static gboolean getLogConf(ZMapLog log) ;

static gboolean startLogging(ZMapLog log) ;
static gboolean stopLogging(ZMapLog log) ;

static gboolean openLogFile(ZMapLog log) ;
static gboolean closeLogFile(ZMapLog log) ;

static void nullLogger(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message,
		       gpointer user_data) ;
static void fileLogger(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message,
		       gpointer user_data) ;



ZMapLog zMapLogCreate(char *logname)
{
  ZMapLog log = NULL ; 

  log = createLog() ;

  if (!configureLog(log))
    {
      destroyLog(log) ;
      log = NULL ;
    }

  return log ;
}




gboolean zMapLogStart(ZMapLog log)
{
  gboolean result = FALSE ;

  result = startLogging(log) ;

  return result ;
}




gboolean zMapLogStop(ZMapLog log)
{
  gboolean result = FALSE ;

  result = stopLogging(log) ;

  return result ;
}



void zMapLogDestroy(ZMapLog log)
{
  stopLogging(log) ;

  destroyLog(log) ;

  return ;
}




/* 
 *                Internal routines.
 */




static ZMapLog createLog(void)
{
  ZMapLog log ;

  log = g_new(ZMapLogStruct, sizeof(ZMapLogStruct)) ;

  /* Default will be logging active. */
  log->logging = TRUE ;
  log->log_to_file = TRUE ;

  /* By default we log to a file, but if we don't specify a callback, glib logging will default
   * to stdout/err so logging can still be active without a file. Note that most of this gets
   * filled in later. */
  log->active_handler.cb_id = 0 ;
  log->active_handler.log_cb = fileLogger ;
  log->active_handler.log_path = NULL ;
  log->active_handler.logfile = NULL ;

  /* inactive handler will be our nullLogger routine. */
  log->inactive_handler.cb_id = 0 ;
  log->inactive_handler.log_cb = nullLogger ;
  log->inactive_handler.log_path = NULL ;
  log->inactive_handler.logfile = NULL ;


  return log ;
}


static void destroyLog(ZMapLog log)
{
  if (log->active_handler.log_path)
    g_free(log->active_handler.log_path) ;

  g_free(log) ;

  return ;
}



/* We start and stop logging by swapping from a logging routine that writes to file with one
 * that does nothing. */
static gboolean startLogging(ZMapLog log)
{
  gboolean result = FALSE ;

  if (!(log->logging))
    {
      /* If logging inactive then remove our inactive handler. */
      if (log->inactive_handler.cb_id)
	{
	  g_log_remove_handler(ZMAPLOG_DOMAIN, log->inactive_handler.cb_id) ;
	  log->inactive_handler.cb_id = 0 ;
	}

      /* Only need to do something if we are logging to a file. */
      if (log->log_to_file)
	{
	  if (openLogFile(log))
	    {
	      log->active_handler.cb_id = g_log_set_handler(ZMAPLOG_DOMAIN,
							    G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
							    | G_LOG_FLAG_RECURSION, 
							    log->active_handler.log_cb,
							    (gpointer)log) ;
	      result = TRUE ;
	    }
	}
      else
	result = TRUE ;

      if (result)
	log->logging = TRUE ;

    }

  return result ;
}

/* Error handling is crap here.... */
static gboolean stopLogging(ZMapLog log)
{
  gboolean result = FALSE ;

  if (log->logging)
    {
      /* If we are logging to a file then remove our handler routine and close the file. */
      if (log->log_to_file)
	{
	  g_log_remove_handler(ZMAPLOG_DOMAIN, log->active_handler.cb_id) ;
	  log->active_handler.cb_id = 0 ;
	  
	  /* Need to close the log file here. */
	  if (!closeLogFile(log))
	    result = FALSE ;
	}

      /* Always need to install a null handler to stop logging. */
      log->inactive_handler.cb_id = g_log_set_handler(ZMAPLOG_DOMAIN,
						      G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
						      | G_LOG_FLAG_RECURSION, 
						      log->inactive_handler.log_cb,
						      (gpointer)log) ;
      if (result)
	log->logging = FALSE ;
    }

  return result ;
}



/* Read the configuration information logging and set up the log. */
static gboolean configureLog(ZMapLog log)
{
  gboolean result = FALSE ;

  if ((result = getLogConf(log)))
    {
      /* This seems stupid, but the start/stopLogging routines need logging to be in the
       * opposite state so we negate the current logging state before calling them,
       * only pertains to initialisation. */
      log->logging = !(log->logging) ;

      if (log->logging)
	{

	  result = stopLogging(log) ;
	}
      else
	{
	  /* If we are logging to a file then we will log via our own routine,
	   * otherwise by default glib will log to stdout/err. */
	  if (log->log_to_file)
	    {
	      log->active_handler.log_cb = fileLogger ;
	    }

	  result = startLogging(log) ;
	}
    }

  return result ;
}



/* Read logging information from the configuration, note that we read _only_ the first
 * logging stanza found in the configuration, subsequent ones are not read. */
static gboolean getLogConf(ZMapLog log)
{
  gboolean result = FALSE ;
  ZMapConfig config ;
  ZMapConfigStanzaSet logging_list = NULL ;
  ZMapConfigStanza logging_stanza ;
  char *logging_stanza_name = "logging" ;
  ZMapConfigStanzaElementStruct logging_elements[] = {{"logging", ZMAPCONFIG_BOOL, {NULL}},
						      {"file", ZMAPCONFIG_BOOL, {NULL}},
						      {"directory", ZMAPCONFIG_STRING, {NULL}},
						      {"filename", ZMAPCONFIG_STRING, {NULL}},
						      {NULL, -1, {NULL}}} ;

  /* Set default values in stanza, keep this in synch with initialisation of logging_elements array. */
  logging_elements[0].data.b = TRUE ;			    /* logging "on" */
  logging_elements[1].data.b = TRUE ;			    /* use a file. */

  if ((config = zMapConfigCreate()))
    {
      char *log_dir, *log_name, *full_dir ;

      logging_stanza = zMapConfigMakeStanza(logging_stanza_name, logging_elements) ;

      if (zMapConfigFindStanzas(config, logging_stanza, &logging_list))
	{
	  ZMapConfigStanza next_log ;
	  
	  /* Get the first logging stanza found, we will ignore any others. */
	  next_log = zMapConfigGetNextStanza(logging_list, NULL) ;
	  
	  log->logging = zMapConfigGetElementBool(next_log, "logging") ;
	  log->log_to_file = zMapConfigGetElementBool(next_log, "file") ;
	  if ((log_dir = zMapConfigGetElementString(next_log, "directory")))
	    log_dir = g_strdup(log_dir) ;
	  if ((log_name = zMapConfigGetElementString(next_log, "filename")))
	    log_name = g_strdup(log_name) ;
	  
	  zMapConfigDeleteStanzaSet(logging_list) ;		    /* Not needed anymore. */
	}
      
      zMapConfigDestroyStanza(logging_stanza) ;
      
      zMapConfigDestroy(config) ;


      if (!log_dir)
	{
	  log_dir = g_strdup(zMapGetControlDirName()) ;
	}
      if (!log_name)
	log_name = g_strdup(ZMAPLOG_FILENAME) ;

      full_dir = zMapGetControlFileDir(log_dir) ;

      log->active_handler.log_path = zMapGetFile(full_dir, log_name) ;

      g_free(log_dir) ;
      g_free(log_name) ;
      g_free(full_dir) ;

      result = TRUE ;
    }

  return result ;
}



static gboolean openLogFile(ZMapLog log)
{
  gboolean result = FALSE ;
  GError *g_error = NULL ;

  /* Must be opened for appending..... */
  if ((log->active_handler.logfile = g_io_channel_new_file(log->active_handler.log_path,
							   "a", &g_error)))
    {
      result = TRUE ;
    }
  else
    {
      result = FALSE ;

      /* We should be using the Gerror here..... */
    }


  return result ;
}



static gboolean closeLogFile(ZMapLog log)
{
  gboolean result = FALSE ;
  GIOStatus g_status ;
  GError *g_error = NULL ;

  if ((g_status = g_io_channel_shutdown(log->active_handler.logfile, TRUE, &g_error))
      != G_IO_STATUS_NORMAL)
    {
      /* disaster....need to report the error..... */


    }
  else
    {
      g_io_channel_unref(log->active_handler.logfile) ;
      result = TRUE ;
    }

  return result ;
}





/* BIG QUESTION HERE...ARE WE SUPPOSED TO FREE THE MESSAGE FROM WITHIN THIS ROUTINE OR IS THAT
 * DONE BY THE glog calling code ?? Docs say nothing, perhaps need to look in code..... */


/* We use this routine to "stop" logging because it doesn't do anything so no log records are
 * output. When we want to restart logging we simply restore the fileLogger() routine as the
 * logging routine. */
static void nullLogger(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message,
		       gpointer user_data)
{


  return ;
}


static void fileLogger(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message,
		       gpointer user_data)
{
  ZMapLog log = (ZMapLog)user_data ;
  char *msg ;
  GIOStatus g_status ;
  GError *g_error = NULL ;
  gsize bytes_written = 0 ;

  zMapAssert(log->logging) ;				    /* logging must be on.... */


  msg = g_strdup_printf("%s\n", message) ;

  if ((g_io_channel_write_chars(log->active_handler.logfile, msg, -1, &bytes_written, &g_error)
       != G_IO_STATUS_NORMAL)
      || (g_io_channel_flush(log->active_handler.logfile, &g_error) != G_IO_STATUS_NORMAL))
    {
      /* We should unregister our handler and go back to the default handler and issue
       * a warning message using the g_error. */

      printf("failed to write to log file\n") ;

    }

  g_free(msg) ;

  return ;
}
