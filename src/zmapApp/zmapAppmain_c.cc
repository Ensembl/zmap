/*  File: zmapAppmain_C.C
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 * Description: C++ main, if the executable is compiled/linked with
 *              main then we can use C++ classes from our code.
 * Exported functions: None.
 * HISTORY:
 * Created: Thu Nov 13 14:38:41 2003 (edgrif)
 * CVS info:   $Id: zmapAppmain_c.cc,v 1.2 2007-12-05 14:19:52 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <locale.h>
#include <string.h>
#include <unistd.h>



#include <ZMap/zmapCmdLineArgs.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>
#include <ZMap/zmapGFF.h>
#include <zmapApp_P.h>




static void checkForCmdLineVersionArg(int argc, char *argv[]) ;
static int checkForCmdLineSleep(int argc, char *argv[]) ;
static void checkConfigDir(char **config_file_out, char **config_dir_out, char **styles_file_out) ;
static gboolean checkSequenceArgs(int argc, char *argv[],
                                  char *config_file, char *styles_file,
                                  GList **seq_maps_inout, GError **error) ;
static gboolean checkPeerID(char *config_file,
                            char **peer_socket_out, char **peer_timeout_list) ;
static void checkForCmdLineSequenceArg(int argc, char *argv[],
                                       char **dataset_out, char **sequence_out, GError **error) ;
static void checkForCmdLineStartEndArg(int argc, char *argv[],
                                       const char *sequence, int *start_inout, int *end_inout, GError **error) ;

static ZMapFeatureSequenceMap createSequenceMap(char *sequence, int start, int end, 
                                                char *config_file, char *styles_file, 
                                                GList **seq_maps_inout) ;

static gboolean getConfiguration(ZMapAppContext app_context, char *config_file) ;
static gboolean configureLog(char *config_file, char *config_dir, GError **error) ;

static ZMapAppContext createAppContext(void) ;

static void setupSignalHandlers(void);

static void remoteInstaller(ZMapAppContext app_context) ;
static gboolean remoteInactiveHandler(gpointer data) ;
static void remoteExitCB(void *user_data) ;

static void checkInputFiles(char *config_file, char *styles_file, GList **seq_maps_inout, GError **error) ;
static void checkInputFileForSequenceDetails(const char* const filename,
                                             char *config_file,
                                             char *styles_file,
                                             GList **seq_maps_inout,
                                             const gboolean allow_multiple,
                                             GError **error) ;
static gboolean validateSequenceDetails(GList *seq_maps, GError **error) ;



/*
 *                            Globals
 */

static gboolean verbose_startup_logging_G = TRUE ;




/*
 *                 Application Main
 */
int main(int argc, char *argv[])
{
  int main_rc = EXIT_FAILURE ;
  ZMapAppContext app_context ;
  char *peer_socket, *peer_timeout_list ;
  int log_size ;
  int sleep_seconds = 0 ;
  GList *seq_maps = NULL ;
  gboolean remote_control = FALSE ;
  GError *g_error = NULL ;
  char *config_file = NULL ;
  char *config_dir = NULL ;
  char *styles_file = NULL ;
  PangoFont *tmp_font = NULL ;
  PangoFontDescription *tmp_font_desc = NULL ;
  ZMapRemoteAppMakeRequestFunc remote_control_cb_func = NULL ;
  void *remote_control_cb_data = NULL ;


#ifdef ZMAP_MEMORY_DEBUG
  if (g_mem_is_system_malloc())
    zMapDebugPrintf("%s", "Using system malloc.") ;
  else
    zMapDebugPrintf("%s", "Using glib special malloc.") ;

  g_mem_set_vtable(glib_mem_profiler_table) ;
#endif



  /*       Application initialisation.        */


  /* User can ask for an immediate sleep, useful for attaching a debugger. */
  if ((sleep_seconds = checkForCmdLineSleep(argc, argv)))
    sleep(sleep_seconds) ;


  /* Since thread support is crucial we do compile and run time checks that its all initialised.
   * the function calls look obscure but its what's recommended in the glib docs. */
#if !defined G_THREADS_ENABLED || defined G_THREADS_IMPL_NONE || !defined G_THREADS_IMPL_POSIX
#error "Cannot compile, threads not properly enabled."
#endif


  /* Set up stack tracing for when we get killed by a signal. */
  setupSignalHandlers() ;


  /* Set up user type, i.e. developer or normal user. */
  zMapUtilsUserInit() ;


  /* Set up command line parsing object, globally available anywhere, this function exits if
   * there are bad command line args. */
  zMapCmdLineArgsCreate(&argc, argv) ;

  /* If user specified version flag, show zmap version and exit with zero return code. */
  checkForCmdLineVersionArg(argc, argv) ;


  /* Don't output messages before we've checked the version flag which should be the only
   * output when user asks for version. */
  zmapAppConsoleLogMsg(verbose_startup_logging_G, INIT_FORMAT, "ZMap starting.") ;


  /* The main control block. */
  app_context = createAppContext() ;

  app_context->verbose_startup_logging = verbose_startup_logging_G ;
  

  /* Set up configuration directory/files */
  checkConfigDir(&config_file, &config_dir, &styles_file) ;


  /* Set any global debug flags from config file. */
  zMapUtilsConfigDebug(config_file) ;


  /* Check if a peer program was specified on the command line or in the config file. */
  peer_socket = peer_timeout_list = NULL ;
  if (!checkPeerID(config_file, &peer_socket, &peer_timeout_list))
    {
      zmapAppConsoleLogMsg(app_context->verbose_startup_logging, INIT_FORMAT, "No remote peer to contact.") ;
    }
  else
    {
      if (!zmapAppRemoteControlIsAvailable())
        {
          zmapAppConsoleLogMsg(app_context->verbose_startup_logging, INIT_FORMAT,
                               "Remote peer command line args given but zmap was not built with the zeromq library"
                               " so remote control is disabled.") ;
        }
      else
        {
          remote_control_cb_func = zmapAppRemoteControlGetRequestCB() ;
          remote_control_cb_data = (void *)app_context ;


          if (zmapAppRemoteControlCreate(app_context, peer_socket, peer_timeout_list))
            {
              zmapAppConsoleLogMsg(app_context->verbose_startup_logging, INIT_FORMAT, "Contacting remote peer.") ;

              remoteInstaller(app_context) ;

              zmapAppRemoteControlSetExitRoutine(app_context, remoteExitCB) ;

              zmapAppConsoleLogMsg(app_context->verbose_startup_logging, INIT_FORMAT, "Connected to remote peer.") ;
            }
          else
            {
              zMapCritical("%s", "Could not initialise remote control interface.") ;
            }

          remote_control = TRUE ;
        }
    }
  

  if (peer_socket)
    g_free(peer_socket) ;
  if (peer_timeout_list)
    g_free(peer_timeout_list) ;


  /* Init manager, must happen just once in application. */
  zMapManagerInit(remote_control_cb_func, remote_control_cb_data) ;

  /* Add the ZMaps manager. */
  app_context->zmap_manager = zMapManagerCreate((void *)app_context) ;


  /* Set up logging for application....NO LOGGING BEFORE THIS. */
  if (!zMapLogCreate(NULL) || !configureLog(config_file, config_dir, &g_error))
    {
      zmapAppConsoleLogMsg(TRUE, "Error creating log file: %s", (g_error ? g_error->message : "<no error message>")) ;
      zmapAppDoTheExit(EXIT_FAILURE) ;
    }
  else
    {
      if (g_error)
        {
          g_error_free(g_error) ;
          g_error = NULL ;
        }

      zMapWriteStartMsg() ;
    }

  zmapAppConsoleLogMsg(app_context->verbose_startup_logging, INIT_FORMAT, "Reading configuration file.") ;

  /* Get general zmap configuration from config. file. */
  getConfiguration(app_context, config_file) ;


  {
    /* Check locale setting, vital for message handling, text parsing and much else. */
    const char *default_locale = "POSIX";
    char *user_req_locale, *new_locale;

    setlocale(LC_ALL, NULL);

    if (!(user_req_locale = app_context->locale))
      user_req_locale = app_context->locale = g_strdup(default_locale);

    /* should we store locale_in_use and new_locale somewhere? */
    if (!(new_locale = setlocale(LC_ALL, app_context->locale)))
      {
        zMapLogWarning("Failed setting locale to '%s'", app_context->locale) ;
        zmapAppConsoleLogMsg(TRUE, "Failed setting locale to '%s'", app_context->locale) ;
        zmapAppDoTheExit(EXIT_FAILURE) ;
      }
  }

  /* Check for sequence/start/end on command line or in config. file, must be
   * either completely correct or not specified. */
  if (!checkSequenceArgs(argc, argv, config_file, styles_file, &seq_maps, &g_error))
    {
      if (g_error)
        {
          zMapLogCritical("%s", g_error->message) ;
          zmapAppConsoleLogMsg(TRUE, "%s", g_error->message) ;
          g_error_free(g_error);
          g_error = NULL;
        }
      else
        {
          zMapLogCritical("%s", "Fatal error: no error message given") ;
          zmapAppConsoleLogMsg(TRUE, "%s", "Fatal error: no error message given") ;
        }

      zmapAppDoTheExit(EXIT_FAILURE) ;
    }

  /* Use the first sequence map as the default sequence */
  if (seq_maps && g_list_length(seq_maps) > 0)
    app_context->default_sequence = (ZMapFeatureSequenceMap)(seq_maps->data) ;

  // Display the main window, we would usually not return from this function.   
  if (zmapMainMakeAppWindow(argc, argv, app_context, seq_maps))
    main_rc = EXIT_SUCCESS ;
  else
    main_rc = EXIT_FAILURE ;

  zmapAppDoTheExit(main_rc) ;    /* exits.... */

  // Shouldn't get here so exit with failure code.   
  return(EXIT_FAILURE) ;
}




/*
 *                   Package routines
 */   

void zmapAppConsoleMsg(gboolean err_msg, const char *format, ...)
{
  va_list args ;
  char *msg_string ;
  FILE *stream ;
  char *time_string ;

  va_start(args, format) ;
  msg_string = g_strdup_vprintf(format, args) ;
  va_end(args) ;

  if (err_msg)
    stream = stderr ;
  else
    stream = stdout ;

  time_string = zMapGetTimeString(ZMAPTIME_LOG, NULL) ;

  /* show message and flush to make sure it appears in a timely way. */
  fprintf(stream, "ZMap [%s] %s\n", time_string, msg_string) ;
  fflush(stream) ;

  g_free(time_string) ;

  g_free(msg_string) ;

  return ;
}


/* Use this routine to exit the application with a portable (as in POSIX) return
 * code. If exit_code == 0 then application exits with EXIT_SUCCESS, otherwise
 * exits with EXIT_FAILURE.
 *
 * Note: gtk now deprecates use of gtk_exit() for exit().
 *
 * exit_code = 0 for success, anything else for failure.
 *  */
void zmapAppDoTheExit(int exit_code)
{
  int true_exit_code ;

  if (exit_code)
    true_exit_code = EXIT_FAILURE ;
  else
    true_exit_code = EXIT_SUCCESS ;

  exit(true_exit_code) ;

  return ;    /* we never get here. */
}




/*
 *                 Internal routines
 */


/* Did user specify the version arg. */
static void checkForCmdLineVersionArg(int argc, char *argv[])
{
  ZMapCmdLineArgsType value ;
  gboolean show_version = FALSE ;

  if (zMapCmdLineArgsValue(ZMAPARG_VERSION, &value))
    show_version = value.b ;

  if (show_version)
    {
      printf("%s - %s\n", zMapGetAppName(), zMapGetAppVersionString()) ;

      exit(EXIT_SUCCESS) ;
    }
  else if (zMapCmdLineArgsValue(ZMAPARG_RAW_VERSION, &value) && value.b)
    {
      printf("%s\n", zMapGetAppVersionString()) ;

      exit(EXIT_SUCCESS) ;
    }


  return ;
}

/* Did user ask for zmap to sleep initially - useful for attaching a debugger.
 * Returns number of seconds to sleep or zero if user did not specify sleep.
 *  */
static int checkForCmdLineSleep(int argc, char *argv[])
{
  int seconds = 0 ;
  char **curr_arg ;

  /* We do this by steam because the cmdline args stuff is not set up immediately. */

  curr_arg = argv ;
  while (*curr_arg)
    {
      if (g_str_has_prefix(*curr_arg, "--" ZMAPSTANZA_APP_SLEEP "="))
        {
          seconds = atoi((strstr(*curr_arg, "=") + 1)) ;
          break ;
        }

      curr_arg++ ;
    }

  return seconds ;
}




/* Did user specify a config directory and/or config file within that directory on the command
 * line. Also check for the stylesfile on the command line or in the config dir. */
static void checkConfigDir(char **config_file_out, char **config_dir_out, char **styles_file_out)
{
  ZMapCmdLineArgsType dir = {FALSE}, file = {FALSE}, stylesfile = {FALSE} ;

  zMapCmdLineArgsValue(ZMAPARG_CONFIG_DIR, &dir) ;
  zMapCmdLineArgsValue(ZMAPARG_CONFIG_FILE, &file) ;
  zMapCmdLineArgsValue(ZMAPARG_STYLES_FILE, &stylesfile) ;

  if (zMapConfigDirCreate(dir.s, file.s))
    {
      *config_file_out = zMapConfigDirGetFile() ;
      *config_dir_out = zMapConfigDirGetDir() ;

      /* If the stylesfile is a relative path, look for it in the config dir. If it
       * was not given on the command line, check if there's one with the default name */
      if (stylesfile.s)
        {
          char *path = zMapExpandFilePath(stylesfile.s) ;

          if (!g_path_is_absolute(path))
            *styles_file_out = zMapConfigDirFindFile(path) ;
        }
      else
        {
          *styles_file_out = zMapConfigDirFindFile("styles.ini") ;
        }
    }

  /* If the styles file wasn't found in the config dir look for it now as an absolute path or
   * path relative to the current directory */
  if (styles_file_out && *styles_file_out == NULL && stylesfile.s)
    {
      char *path = zMapExpandFilePath(stylesfile.s) ;

      if (!g_path_is_absolute(path))
        {
          char *base_dir = g_get_current_dir() ;
          char *tmp = g_build_path("/", base_dir, path, NULL) ;
          g_free(base_dir) ;

          g_free(path) ;
          path = tmp ;
        }

      *styles_file_out = path;
    }
}




/* Did caller specify a peer socket id on the command line or in a config file ?
 * If a peer socket is found then function returns TRUE and may also return
 * a peer timeout list, otherwise returns FALSE. */
static gboolean checkPeerID(char *config_file, char **peer_socket_out, char **peer_timeout_list)
{
  gboolean result = FALSE ;
  ZMapCmdLineArgsType socket_value = {FALSE} ;
  ZMapConfigIniContext context ;


  /* Command line params override config file. */
  if (zMapCmdLineArgsValue(ZMAPARG_PEER_SOCKET, &socket_value) && socket_value.s)
    {
      *peer_socket_out = g_strdup(socket_value.s) ;
    }

  if ((context = zMapConfigIniContextProvide(config_file)))
    {
      char *tmp_string = NULL ;

      /* If peer app name/clipboard not set then look in the config file... */
      if (!(*peer_socket_out))
        {
          if (zMapConfigIniContextGetString(context, ZMAPSTANZA_PEER_CONFIG, ZMAPSTANZA_PEER_CONFIG,
                                            ZMAPSTANZA_PEER_SOCKET, &tmp_string))
            *peer_socket_out = g_strdup(tmp_string) ;
        }

      if (*peer_socket_out)
        {
          if (zMapConfigIniContextGetString(context, ZMAPSTANZA_PEER_CONFIG, ZMAPSTANZA_PEER_CONFIG,
                                            ZMAPSTANZA_PEER_TIMEOUT_LIST, &tmp_string))
            *peer_timeout_list = g_strdup(tmp_string) ;
        }

      zMapConfigIniContextDestroy(context);
    }

  if (*peer_socket_out)
    result = TRUE ;

  return result ;
}




/* Read ZMap application defaults. */
static gboolean getConfiguration(ZMapAppContext app_context, char *config_file)
{
  gboolean result = FALSE ;
  ZMapConfigIniContext context;

  app_context->show_mainwindow = TRUE;
  app_context->exit_timeout = ZMAP_DEFAULT_EXIT_TIMEOUT;

  if ((context = zMapConfigIniContextProvide(config_file)))
    {
      gboolean tmp_bool = FALSE;
      char *tmp_string  = NULL;
      int tmp_int = 0;

      /* Do we show the main window? if not then on ZMap close we should exit */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                         ZMAPSTANZA_APP_MAINWINDOW, &tmp_bool))
        app_context->show_mainwindow = tmp_bool ;

      /* Should window title prefix be abbreviated ? */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                         ZMAPSTANZA_APP_ABBREV_TITLE, &tmp_bool))
        app_context->abbrev_title_prefix = tmp_bool ;
      zMapGUISetAbbrevTitlePrefix(app_context->abbrev_title_prefix) ;


      /* How long to wait when closing, before timeout */
      if (zMapConfigIniContextGetInt(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                     ZMAPSTANZA_APP_EXIT_TIMEOUT, &tmp_int))
        app_context->exit_timeout = tmp_int ;

      if (app_context->exit_timeout < 0)
        app_context->exit_timeout = ZMAP_DEFAULT_EXIT_TIMEOUT ;


      /* help url to use */
      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                        ZMAPSTANZA_APP_HELP_URL, &tmp_string))
        zMapGUISetHelpURL(tmp_string) ;

      /* session colour for visual grouping of applications. */
      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                        ZMAPSTANZA_APP_SESSION_COLOUR, &tmp_string))
        {
          gdk_color_parse(tmp_string, &(app_context->session_colour)) ;
          app_context->session_colour_set = TRUE ;
        }

      /* locale to use */
      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                        ZMAPSTANZA_APP_LOCALE, &tmp_string))
        app_context->locale = tmp_string;

      /* Turn on/off debugging for xremote connection to peer client. */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                         ZMAPSTANZA_APP_XREMOTE_DEBUG, &tmp_bool))
        {
          app_context->xremote_debug = tmp_bool ;
        }

      zMapConfigIniContextDestroy(context);

      result = TRUE;
    }

  return result ;
}


/* Read logging configuration from ZMap stanza and apply to log. Tries to get
 * the (writable) location for the log file first from the config file, then 
 * from the config directory. If neither are writable, it uses the default location
 * in the user's home directory. */
static gboolean configureLog(char *config_file, char *config_dir, GError **error)
{
  gboolean result = TRUE ;/* if no config, we can't fail to configure */
  GError *g_error = NULL ;
  ZMapConfigIniContext context ;
  gboolean logging, log_to_file, show_process, show_code, show_time, catch_glib, echo_glib ;
  char *log_name, *logfile_path = NULL ;

  /* ZMap's default values (as opposed to the logging packages...). */
  logging = TRUE ;
  log_to_file = TRUE  ;
  show_process = FALSE ;
  show_code = TRUE ;
  show_time = FALSE ;
  catch_glib = TRUE ;
  echo_glib = TRUE ;
  /* if we run config free we use .ZMap, creating it if necessary */
  log_name = g_strdup(ZMAPLOG_FILENAME) ;


  if (config_file && (context = zMapConfigIniContextProvide(config_file)))
    {
      gboolean tmp_bool ;
      char *tmp_string = NULL;

      /* logging at all */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_LOGGING, &tmp_bool))
        logging = tmp_bool ;

      /* logging to the file */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_FILE, &tmp_bool))
        log_to_file = tmp_bool ;

      /* how much detail to show...process/pid... */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_SHOW_PROCESS, &tmp_bool))
        show_process = tmp_bool ;

      /* how much detail to show...code... */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_SHOW_CODE, &tmp_bool))
        show_code = tmp_bool;

      /* how much detail to show...time... */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_SHOW_TIME, &tmp_bool))
        show_time = tmp_bool;

      /* catch GLib errors, else they stay on stdout */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_CATCH_GLIB, &tmp_bool))
        catch_glib = tmp_bool;

      /* catch GLib errors, else they stay on stdout */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_CONFIG,
                                         ZMAPSTANZA_LOG_ECHO_GLIB, &tmp_bool))
        echo_glib = tmp_bool;

      /* See if there is a user-specified directory to use for logfile path */
      if (zMapConfigIniContextGetFilePath(context, ZMAPSTANZA_LOG_CONFIG,
                                          ZMAPSTANZA_LOG_CONFIG,
                                          ZMAPSTANZA_LOG_DIRECTORY, &tmp_string))
        {
          char *full_dir = zMapGetDir(tmp_string, TRUE, TRUE) ;
          logfile_path = zMapGetFile(full_dir, log_name, TRUE, "rw", &g_error) ;
          g_free(full_dir) ;

          if (g_error)
            {
              char *msg = g_strdup_printf("Cannot use configured directory '%s' for log file: %s", config_dir, g_error->message) ;
              zmapAppConsoleLogMsg(verbose_startup_logging_G, INIT_FORMAT, msg) ;
              g_free(msg) ;

              g_error_free(g_error) ;
              g_error = NULL ;
            }
        }


      /* user specified file, default to zmap.log */
      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_LOG_CONFIG,
                                        ZMAPSTANZA_LOG_CONFIG,
                                        ZMAPSTANZA_LOG_FILENAME, &tmp_string))
        {
          g_free(log_name) ;
          log_name = tmp_string;
        }

      /* config context needs freeing */
      zMapConfigIniContextDestroy(context);
    }

  if (!logfile_path && config_dir)
    {
      /* See if we can get logfile path from the given config_dir */
      char *full_dir = zMapGetDir(config_dir, FALSE, FALSE) ;
      logfile_path = zMapGetFile(full_dir, log_name, TRUE, "rw", &g_error) ;
      g_free(full_dir) ;

      if (g_error)
        {
          char *msg = g_strdup_printf("Cannot use conf_dir '%s' for log file: %s", config_dir, g_error->message) ;
          zmapAppConsoleLogMsg(verbose_startup_logging_G, INIT_FORMAT, msg) ;
          g_free(msg) ;

          g_error_free(g_error) ;
          g_error = NULL ;
        }
    }

  if (!logfile_path)
    {
      /* Use the default directory for logfile path. Create the full path and mkdir if necessary. */
      char *full_dir = zMapGetDir(ZMAP_USER_CONFIG_DIR, TRUE, TRUE) ;
      logfile_path = zMapGetFile(full_dir, log_name, TRUE, "rw", &g_error) ;
      g_free(full_dir) ;
    }

  /* all our strings need freeing */
  g_free(log_name) ;

  if (logfile_path)
    {
      char *msg = g_strdup_printf("Setting up logging to '%s'", logfile_path) ;
      zmapAppConsoleLogMsg(verbose_startup_logging_G, INIT_FORMAT, msg) ;
      g_free(msg) ;

      result = zMapLogConfigure(logging, log_to_file,
                                show_process, show_code, show_time,
                                catch_glib, echo_glib,
                                logfile_path, &g_error) ;
    }

  if (g_error)
    {
      result = FALSE ;
      g_propagate_error(error, g_error) ;
    }

  return result ;
}




static ZMapAppContext createAppContext(void)
{
  ZMapAppContext app_context ;

  app_context = g_new0(ZMapAppContextStruct, 1) ;

  app_context->state = ZMAPAPP_INIT ;

  app_context->app_id = zMapGetAppTitle() ;

  app_context->exit_timeout = ZMAP_DEFAULT_EXIT_TIMEOUT ;
  app_context->ping_timeout = ZMAP_DEFAULT_PING_TIMEOUT ;

  return app_context ;
}


/* Called  */
static void setupSignalHandlers(void)
{
  /* Not much point if there's no sigaction! */
#ifndef NO_SIGACTION
  struct sigaction signal_action, prev;

  sigemptyset (&(signal_action.sa_mask));

  /* Block other signals while handler runs. */
  sigaddset (&(signal_action.sa_mask), SIGSEGV);
  sigaddset (&(signal_action.sa_mask), SIGBUS);
  sigaddset (&(signal_action.sa_mask), SIGABRT);

  signal_action.sa_flags   = 0;
  signal_action.sa_handler = zMapSignalHandler;

  sigaction(SIGSEGV, &signal_action, &prev);
  sigaction(SIGBUS,  &signal_action, &prev);
  sigaction(SIGABRT, &signal_action, &prev);
#endif /* NO_SIGACTION */

  return ;
}




/* THIS NOW NEEDS TO BE A STRAIGHT FORWARD FUNCTION CALL...DO NOT NEED MAPPING BIT... */
/* **NEW XREMOTE**
 *
 * create the new xremote object.
 */
static void remoteInstaller(ZMapAppContext app_context)
{

  zmapAppConsoleLogMsg(TRUE, "ZMAP %s() - %s",
             __PRETTY_FUNCTION__,
             "In remoteInstaller, about to init RemoteControl.") ;

  /* Set up our remote handler. */
  if (zmapAppRemoteControlInit(app_context))
    {
      if (!zmapAppRemoteControlConnect(app_context))
        zMapCritical("Failed to connect to peer using socket %s.",
                     app_context->remote_control->peer_socket) ;
    }
  else
    {
      zMapLogCritical("%s", "Cannot initialise zmap remote control interface.") ;
    }

  zmapAppConsoleLogMsg(TRUE, "ZMAP %s() - %s",
             __PRETTY_FUNCTION__,
             "In remoteInstaller, about to hide main window.") ;

  /* Set an inactivity timeout, so we can warn the user if nothing happens for a long time in case
   * zmap has become disconnected from the peer. (note timeout is in milliseconds) */
  app_context->remote_control->inactive_timeout_interval_s = ZMAP_APP_REMOTE_TIMEOUT_S ;
  app_context->remote_control->inactive_func_id = g_timeout_add(ZMAP_APP_REMOTE_TIMEOUT_S * 1000,
                                                                remoteInactiveHandler, (gpointer)app_context) ;

  /* Now set the time of day we did this. */
  app_context->remote_control->last_active_time_s = zMapUtilsGetRawTime() ;

  return ;
}


/* Check to see if user specied a sequence/start/end on the command line or in
 * a config file, only returns FALSE if they tried to specify it and it was
 * wrong. If FALSE an error message is returned in err_msg_out which should
 * be g_free'd by caller. */
static gboolean checkSequenceArgs(int argc, char *argv[],
                                  char *config_file, char *styles_file,
                                  GList **seq_maps_inout, GError **error)
{
  gboolean result = FALSE ;
  GError *tmp_error = NULL ;
  char *dataset = NULL ;
  char *sequence = NULL ;
  int start = 0 ;
  int end = 0 ;
  ZMapFeatureSequenceMap seq_map = NULL ;

  /* Check for sequence on command-line */
  if (!tmp_error)
    checkForCmdLineSequenceArg(argc, argv, &dataset, &sequence, &tmp_error) ;

  /* Check for coords on command-line */
  if (!tmp_error)
    checkForCmdLineStartEndArg(argc, argv, sequence, &start, &end, &tmp_error) ;

  /* Create the sequence map from the details so far (this is blank if none given) */
  if (!tmp_error)
    seq_map = createSequenceMap(sequence, start, end, config_file, styles_file, seq_maps_inout) ;

  /* Check for sequence details in the config file, if one was given. If the sequence details
   * are already set then this just validates the config file values against the existing ones. */
  if (!tmp_error)
    zMapAppGetSequenceConfig(seq_map, &tmp_error) ;

  /* Next check any input file(s). If there are multiple files for different sequences/regions
   * then we end up with multiple sequence maps */
  if (!tmp_error)
    checkInputFiles(config_file, styles_file, seq_maps_inout, &tmp_error) ;

  /* If details were set, check that they are valid. */
  if (!tmp_error)
    result = validateSequenceDetails(*seq_maps_inout, &tmp_error) ;

  if (tmp_error)
    g_propagate_error(error, tmp_error) ;

  return result ;
}


/* Did user specify seqence/start/end on command line? */
static void checkForCmdLineSequenceArg(int argc, char *argv[], char **dataset_out, char **sequence_out, GError **error)
{
  ZMapCmdLineArgsType value ;
  char *sequence ;

  if (zMapCmdLineArgsValue(ZMAPARG_SEQUENCE, &value))
    {
      sequence = value.s ;

      if(dataset_out)
        {
          gchar *delim;

          delim = g_strstr_len(sequence,-1,"/");
          if(delim)
            {
              *dataset_out = sequence;
              *delim++ = 0;
              sequence = delim;
            }
        }

      *sequence_out = sequence ;
    }

  return ;
}


/* Did user specify seqence/start/end on command line. */
static void checkForCmdLineStartEndArg(int argc, char *argv[],
                                       const char *sequence, int *start_inout, int *end_inout, GError **error)
{
  GError *tmp_error = NULL ;
  ZMapCmdLineArgsType start_value ;
  ZMapCmdLineArgsType end_value ;

  gboolean got_start = zMapCmdLineArgsValue(ZMAPARG_SEQUENCE_START, &start_value) ;
  gboolean got_end = zMapCmdLineArgsValue(ZMAPARG_SEQUENCE_END, &end_value) ;

  if (got_start && got_end && !sequence)
    {
      g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_NO_SEQUENCE,
                  "The coords were specified on the command line but the sequence name missing: must specify all or none of sequence/start/end") ;
    }
  else if (got_start && got_end)
    {
      *start_inout = start_value.i ;
      *end_inout = end_value.i ;
    }
  else if (got_start)
    {
      g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_BAD_COORDS,
                  "The start coord was specified on the command line but the end coord is missing: must specify both or none") ;
    }
  else if (got_end)
    {
      g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_BAD_COORDS,
                  "The end coord was specified on the command line but the start coord is missing: must specify both or none") ;
    }

  if (tmp_error)
    g_propagate_error(error, tmp_error) ;

  return ;
}




/* Called by app remote control when we've finished saying goodbye to a peer
 * and can clear up and exit. */
static void remoteExitCB(void *user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;

  zmapAppSignalAppDestroy(app_context) ;

  return ;
}


/* A GSourceFunc, only called when there is a peer program. If there is no currently
 * displayed sequence and there has been no communication with the peer since before
 * the timeout started then we warn the user. The user can elect to continue or to
 * exit zmap. If the function returns FALSE it will not be called again.
 */
static gboolean remoteInactiveHandler(gpointer data)
{
  gboolean result = TRUE ;
  ZMapAppContext app_context = (ZMapAppContext)data ;
  guint zmap_num ;
  time_t current_time_s ;

  current_time_s = zMapUtilsGetRawTime() ;

  /* Only do this if there are no displayed zmaps and inactivity has been longer than timeout. */
  if (((current_time_s - app_context->remote_control->last_active_time_s) > ZMAP_APP_REMOTE_TIMEOUT_S)
      && !(zmap_num = zMapManagerCount(app_context->zmap_manager)))
    {
      char *msg ;
      long pid, ppid ;
      char *log_msg ;

      pid = getpid() ;
      ppid = getppid() ;

      log_msg = g_strdup_printf("This zmap (PID = %ld) has been running for more than %d mins"
                                " with no sequence displayed"
                                " and without any interaction with its peer \"%s\" (PID = %ld).",
                                pid, (ZMAP_APP_REMOTE_TIMEOUT_S / 60),
                                app_context->remote_control->peer_name, ppid) ;

      /*
       * (sm23) for debugging this must also be made available to logging and
       * stderr (sent to otterlace log)
       */
      zMapLogWarning("%s", log_msg) ;
      zMapDebugPrintf("%s", log_msg) ;

      msg = g_strdup_printf("%s Do you want to continue ?", log_msg) ;


      if (!zMapGUIMsgGetBoolFull((GtkWindow *)(app_context->app_widg), ZMAP_MSG_WARNING, msg,
                                 "Continue", "Exit"))
        {
          zmapAppDestroy(app_context) ;

          result = FALSE ;
        }

      g_free(log_msg) ;
      g_free(msg) ;
    }

  return result ;
}



/* Check the input file(s) on the command line, if any. Checks the sequence details are valid and/or
 * sets the sequence details if they are not already set. */
static void checkInputFiles(char *config_file, char *styles_file, GList **seq_maps_inout, GError **error)
{
  GError *tmp_error = NULL ;
  char **file_list = zMapCmdLineFinalArg() ;
  ZMapFeatureSequenceMap seq_map = NULL ;

  if (seq_maps_inout && *seq_maps_inout && g_list_length(*seq_maps_inout) > 0)
    seq_map = (ZMapFeatureSequenceMap)((*seq_maps_inout)->data) ;

  if (file_list)
    {
      /* If details have been set by the command line or config file then take those as absolute
       * and don't allow multiple seq_maps to be created - just validate for each file that the
       * sequence details match those already set. If the sequence details are not already set, 
       * we allow multiple seq_maps to be created for different input sequences/regions and 
       * we also allow the range for a single sequence to be expanded if there are multiple
       * requested regions that overlap. */
      const gboolean allow_multiple = (seq_map && !seq_map->sequence && !seq_map->start && !seq_map->end);

      /* Loop through the input files */
      char **file = file_list ;

      for (; file && *file; ++file)
        {
          checkInputFileForSequenceDetails(*file, config_file, styles_file, seq_maps_inout, allow_multiple, &tmp_error) ;

          if (tmp_error)
            {
              /* This is not a fatal error so just give a warning and continue */
              zMapLogWarning("Cannot open file '%s': %s", *file, tmp_error->message) ;
              zmapAppConsoleLogMsg(TRUE, "Cannot open file '%s': %s", *file, tmp_error->message) ;
              g_error_free(tmp_error) ;
              tmp_error = NULL ;
            }
        }

      g_strfreev(file_list) ;
    }

  if (tmp_error)
    g_propagate_error(error, tmp_error) ;
}


/* Validate the sequence details set in the sequence map. Sets the error if there is a problem. */
static gboolean validateSequenceDetails(GList *seq_maps, GError **error)
{
  gboolean result = TRUE ;
  GError *tmp_error = NULL ;
  
  GList *seq_map_item = seq_maps ;

  for ( ; seq_map_item && !tmp_error; seq_map_item = seq_map_item->next)
    {
      ZMapFeatureSequenceMap seq_map = (ZMapFeatureSequenceMap)(seq_map_item->data) ;

      /* The sequence name, start and end must all be specified, or none of them. */
      if ((seq_map->sequence && seq_map->start && seq_map->end)
          || (!(seq_map->sequence) && !(seq_map->start) && !(seq_map->end)))
        {
          /* We must have a config file or some files on the command line (otherwise we have no
           * sources) */
          if (seq_map->config_file || seq_map->file_list)
            {

            }
          else
            {
              result = FALSE ;

              g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_NO_SOURCES,
                          "No data sources - you must specify a config file, or pass data in GFF files on the command line") ;
            }
        }
      else
        {
          result = FALSE ;

          g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_BAD_SEQUENCE_DETAILS,
                      "Bad sequence args: must set all or none of sequence name, start and end. Got: %s, %d, %d",
                      (!seq_map->sequence ? "<no sequence name>" : seq_map->sequence),
                      seq_map->start, seq_map->end) ;
        }
    }

  if (tmp_error)
    g_propagate_error(error, tmp_error) ;

  return result ;
}



/* Check the given file to see if we can extract sequence details. allow_multiple means that
 * multiple seq_maps are allowed so if the new details do not match the existing seq_map(s) then
 * we can create a new seq_map; otherwise the new details must match or we set the error. */
static void checkInputFileForSequenceDetails(const char* const filename,
                                             char *config_file,
                                             char *styles_file,
                                             GList **seq_maps_inout,
                                             const gboolean allow_multiple,
                                             GError **error)
{
  gboolean result = FALSE ;
  GError *tmp_error = NULL ;
  GError *gff_pipe_err = NULL ;
  int gff_version = 0 ;
  ZMapGFFParser parser = NULL ;
  GString *gff_line = NULL ;
  GIOStatus status = G_IO_STATUS_NORMAL ;
  GIOChannel *gff_pipe = NULL ;

  zMapReturnIfFail(filename) ;

  gff_line = g_string_sized_new(2000) ; /* Probably not many lines will be > 2k chars. */

  /* Check for the special case "-" which means stdin */
  if (!strcmp(filename, "-"))
    gff_pipe = g_io_channel_unix_new(0) ; /* file descriptor for stdin is 0 */
  else
    gff_pipe = g_io_channel_new_file(filename, "r", &gff_pipe_err) ;


  if (!gff_pipe)
    {
      g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_OPENING_FILE,
                  "Could not open file %s", filename);
    }
  else
    {
      if (!zMapGFFGetVersionFromGIO(gff_pipe, gff_line,
                                    &gff_version,
                                    &status, &gff_pipe_err))
        {
          const char *tmp ;

          if (status == G_IO_STATUS_EOF)
            tmp = "No data returned from pipe" ;
          else
            tmp = "Serious GIO error returned from file" ;

          g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_GFF_VERSION,
                      "Could not get gff-version from file %s: %s.",
                      filename, tmp);
        }
      else
        {
          parser = zMapGFFCreateParser(gff_version, NULL, 0, 0) ;

          if (parser)
            {
              gsize terminator_pos = 0 ;
              gboolean done_header = FALSE ;
              ZMapGFFHeaderState header_state = GFF_HEADER_NONE ; /* got all the ones we need ? */

              zMapGFFParserSetSequenceFlag(parser);  // reset done flag for seq else skip the data

              /* Read the header, needed for feature coord range. */
              while ((status = g_io_channel_read_line_string(gff_pipe, gff_line,
                                                             &terminator_pos,
                                                             &gff_pipe_err)) == G_IO_STATUS_NORMAL)
                {
                  *(gff_line->str + terminator_pos) = '\0' ; /* Remove terminating newline. */

                  if (zMapGFFParseHeader(parser, gff_line->str, &done_header, &header_state))
                    {
                      if (done_header)
                        {
                          result = TRUE ;
                          break ;
                        }
                      else
                        {
                          gff_line = g_string_truncate(gff_line, 0) ;  /* Reset line to empty. */
                        }
                    }
                  else
                    {
                      g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_GFF_HEADER,
                                  "Error reading GFF header for file %s", filename) ;
                      break ;
                    }
                }
            }
          else
            {
              g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_GFF_PARSER,
                          "Error creating GFF parser for file %s", filename);
            }
        }
    }

  if (result && !tmp_error)
    {
      /* Cache the sequence details in a sequence map - merge with an existing sequence map if
       * possible, otherwise create a new one. */
      char *sequence = zMapGFFGetSequenceName(parser) ;
      int start = zMapGFFGetFeaturesStart(parser) ;
      int end = zMapGFFGetFeaturesEnd(parser) ;
      ZMapFeatureSequenceMap seq_map = NULL ;

      GList *seq_map_item = seq_maps_inout ? g_list_first(*seq_maps_inout) : NULL ;

      for ( ; seq_map_item; seq_map_item = seq_map_item->next)
        {
          seq_map = (ZMapFeatureSequenceMap)(seq_map_item->data) ;

          zMapAppMergeSequenceName(seq_map, sequence, allow_multiple, &tmp_error) ;

          if (!tmp_error)
            zMapAppMergeSequenceCoords(seq_map, start, end, FALSE, allow_multiple, &tmp_error) ;

          if (tmp_error && allow_multiple)
            {
              /* The error indicates we failed to merge with this seq_map but multiple seq_maps
               * are allowed so reset the error and result and continue to the next seq_map. If we 
               * don't find one then we'll create a new seq_map. */
              g_error_free(tmp_error) ;
              tmp_error = NULL ;
              seq_map = NULL ;
            }
          else if (tmp_error)
            {
              /* We failed to match this seq_map and multiple seq_maps are not allowed so 
               * this is an error. */
              break ;
            }
          else
            {
              /* We successfully found a suitable seq_map (and merged if applicable), so we're done */
              break ;
            }
        }

      if (!tmp_error)
        {
          /* If we didn't manage to merge with an existing seq map, create a new one. Use the 
           * config/styles file if given because we want to inherit all of the config 
           * apart from the sequence details */
          if (!seq_map)
            seq_map = createSequenceMap(sequence, start, end, config_file, styles_file, seq_maps_inout) ;

          /* Cache the filename and parser state */
          seq_map->file_list = g_slist_append(seq_map->file_list, g_strdup(filename)) ;

          ZMapFeatureParserCache parser_cache = g_new0(ZMapFeatureParserCacheStruct, 1) ;
          parser_cache->parser = (gpointer)parser ;
          parser_cache->line = gff_line ;
          parser_cache->pipe = gff_pipe ;
          parser_cache->pipe_status = status ;

          seq_map->cached_parsers = g_hash_table_new(NULL, NULL) ;
          g_hash_table_insert(seq_map->cached_parsers, GINT_TO_POINTER(g_quark_from_string(filename)), parser_cache) ;
        }
    }
  else if (!tmp_error)
    {
      /* If result is false then tmp_error should be set, but it's possible it might not be if we
       * didn't find a full header in the GFF file. */
      g_set_error(&tmp_error, ZMAP_APP_ERROR, ZMAPAPP_ERROR_CHECK_FILE,
                  "Error reading GFF header information for file %s", filename);
    }

  if (tmp_error && parser)
    zMapGFFDestroyParser(parser) ;

  if (tmp_error)
    g_propagate_error(error, tmp_error) ;
}

static ZMapFeatureSequenceMap createSequenceMap(char *sequence, int start, int end, 
                                                char *config_file, char *styles_file, 
                                                GList **seq_maps_inout)
{
  ZMapFeatureSequenceMap seq_map = g_new0(ZMapFeatureSequenceMapStruct, 1) ;

  seq_map->sequence = sequence ;
  seq_map->start = start ;
  seq_map->end = end ;
  seq_map->config_file = config_file ;
  seq_map->stylesfile = styles_file ;

  *seq_maps_inout = g_list_append(*seq_maps_inout, seq_map) ;

  return seq_map ;
}



