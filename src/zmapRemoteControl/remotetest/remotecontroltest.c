/*  File: remotecontroltest.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2010-2015: Genome Research Ltd.
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Program to test remote control interface in ZMap.
 *
 * Exported functions: none
 *              
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <ZMap/zmapConfigStrings.h>			    /* For peer-id etc. */
#include <ZMap/zmapCmdLineArgs.h>			    /* For cmd line args. */
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsGUI.h>
#include <ZMap/zmapRemoteControl.h>
#include <ZMap/zmapRemoteCommand.h>
#include <ZMap/zmapXML.h>




#define TIMEOUT_LIST "100000,100000,1000000"



/* appname we want to report. */
#define REMOTECONTROL_APPNAME "remotecontrol"

/* Prefix to our unique atom for remote communication. */
#define REMOTECONTROL_PREFIX "RemoteControl"


#define XREMOTEARG_DEFAULT_SEQUENCE "< Enter your sequence here >"


/* command line args for remotecontrol. */
#define XREMOTEARG_NO_ARG "<none>"
#define XREMOTEARG_FILE_ARG "file path"

#define XREMOTEARG_VERSION "version"
#define XREMOTEARG_VERSION_DESC "version number"

#define XREMOTEARG_CONFIG "config-file"
#define XREMOTEARG_CONFIG_DESC "Specify path for remotecontrol's config file."

#define XREMOTEARG_COMMAND "command-file"
#define XREMOTEARG_COMMAND_DESC "file location for commands"

#define XREMOTEARG_DEBUGGER "in-debugger"
#define XREMOTEARG_DEBUGGER_DESC "Start zmap within debugger (Totalview)."

#define XREMOTEARG_XREMOTE_DEBUG "xremote-debug"
#define XREMOTEARG_XREMOTE_DEBUG_DESC "Display xremote debugging output (default false)."

#define XREMOTEARG_CMD_DEBUG "command-debug"
#define XREMOTEARG_CMD_DEBUG_DESC "Display command debugging output (default true)."


/* command line args passed to zmap. */
#define XREMOTEARG_SLEEP "zmap-sleep"
#define XREMOTEARG_SLEEP_DESC "Make ZMap sleep for given seconds."

#define XREMOTEARG_SEQUENCE "zmap-sequence"
#define XREMOTEARG_SEQUENCE_DESC "Set up xremote with supplied sequence."

#define XREMOTEARG_ZMAP_CONFIG "zmap-config-file"
#define XREMOTEARG_ZMAP_CONFIG_DESC "Specify path for zmaps config file."

#define XREMOTEARG_START "zmap-start"
#define XREMOTEARG_START_DESC "Start coord in sequence."

#define XREMOTEARG_END "zmap-end"
#define XREMOTEARG_END_DESC "End coord in sequence."

#define XREMOTEARG_REMOTE_DEBUG "zmap-remote-debug"
#define XREMOTEARG_REMOTE_DEBUG_DESC "Remote debug level: off | normal | verbose."

#define XREMOTEARG_NO_TIMEOUT "zmap-no_timeout"
#define XREMOTEARG_NO_TIMEOUT_DESC "Never timeout waiting for a response."

#define XREMOTEARG_ZMAP_PATH_DESC "zmap executable path."




/* config file defines */
#define XREMOTE_PROG_CONFIG "programs"

#define XREMOTE_PROG_ZMAP      "zmap-exe"
#define XREMOTE_PROG_ZMAP_OPTS "zmap-options"
#define XREMOTE_PROG_SERVER      "sgifaceserver-exe"
#define XREMOTE_PROG_SERVER_OPTS "sgifaceserver-options"



/* errr....why do we need a new enum here.... */
enum
  {
    XREMOTE_PING,

    XREMOTE_NEW_VIEW,
    XREMOTE_ADD_VIEW,
    XREMOTE_CLOSE_VIEW,

    XREMOTE_CREATE,
    XREMOTE_CREATE_SIMPLE,
    XREMOTE_REPLACE,
    XREMOTE_DELETE,
    XREMOTE_FIND,

    XREMOTE_LOAD_FEATURES,
    XREMOTE_ZOOM_TO,
    XREMOTE_GET_MARK,
    XREMOTE_REVCOMP,

    XREMOTE_GOODBYE,
    XREMOTE_SHUTDOWN,
    XREMOTE_ABORT,

    /* Not done yet.... */
    XREMOTE_ZOOMIN,
    XREMOTE_ZOOMOUT,

  };


/* Command line args struct. */
typedef struct
{
  int argc;
  char **argv;
  GOptionContext *opt_context;
  GError *error;

  /* and the commandline args for remote control */
  gboolean version;
  char *config_file;
  char *command_file;
  gboolean debugger ;
  gboolean xremote_debug ;
  gboolean cmd_debug ;

  /* and those to be passed to zmap */
  char *sleep_seconds ;
  char *zmap_config_file ;
  char *sequence ;
  char *start ;
  char *end ;
  char *remote_debug ;
  gboolean timeout ;
  char *zmap_path ;
} XRemoteCmdLineArgsStruct, *XRemoteCmdLineArgs;


/* Holds text widget/buffer for each pair of request/reponse windows. */
typedef struct
{
  GtkWidget *request_text_area ;
  GtkTextBuffer *request_text_buffer ;

  GtkWidget *response_text_area ;
  GtkTextBuffer *response_text_buffer ;
} ReqRespTextStruct, *ReqRespText ;


typedef struct GetPeerStructType *GetPeer ;


/* Main application struct. */
typedef struct AppDataStructType
{
  /* Startup/configuration */
  XRemoteCmdLineArgs cmd_line_args ;
  ZMapConfigIniContext config_context ;


  /* Interface. */
  GtkWidget *app_toplevel, *vbox, *menu, *buttons ;
  GtkWidget *zmap_path_entry ;
  GtkWidget *sequence_entry, *start_entry, *end_entry, *config_entry ;

  GtkWidget *send_init, *receive_init ;
  GtkWidget *idle, *waiting, *sending, *receiving ;

  ReqRespTextStruct our_req ;
  ReqRespTextStruct zmap_req ;


  /* ZMap peer */
  char *zmap_path ;
  int zmap_pid ;

  guint stop_zmap_func_id ;
  guint child_watch_cb_id ;


  /* A straight forward list of views we have created, the list contains the view_ids. */
  GList *views ;

  char *our_app_name ;
  char *our_socket_str ;

  char *peer_app_name ;
  char *peer_socket_str ;

  ZMapRemoteControl remote_cntl ;
  gboolean send_interface_init ;                            /* Have we init'd the send interface. */

  char *curr_request ;

  char *request_id ;

  RemoteCommandRCType reply_rc ;
  ZMapXMLUtilsEventStack reply ;
  char *reply_txt ;

  char *error ;


  ZMapXMLParser parser;
  GetPeer peer_cbdata ;



} AppDataStruct, *AppData ;








static AppData createAppData(char *argv_path, XRemoteCmdLineArgs cmd_args) ;
static void destroyAppData(AppData app_data) ;
static void appExit(gboolean exit_ok) ;

static char **buildZMapCmd(AppData app_data) ;
static char **buildParamList(char *params_as_string) ;

static gboolean startZMap(AppData app_data) ;
static void stopZMap(AppData app_data) ;

static void initRemote(AppData app_data) ;
static void destroyRemote(AppData app_data) ;

static gboolean createZMapProcess(AppData app_data, char **command) ;
static void destroyZMapProcess(AppData app_data) ;
static void zmapDieCB(GPid pid, gint status, gpointer user_data) ;
static void clearUpZMap(AppData app_data) ;

static void getZMapConfiguration(AppData app_data) ;




static GtkWidget *makeTestWindow(AppData app_data) ;
static GtkWidget *makeMainReqResp(AppData app_data) ;
static GtkWidget *makeReqRespBox(AppData app_data,
				 GtkBox *parent_box, char *box_title,
				 ReqRespText text_widgs, char *request_title, char *response_title) ;
static void addTextArea(AppData app_data, GtkBox *parent_box, char *text_title,
			GtkWidget **text_area_out, GtkTextBuffer **text_buffer_out) ;
static GtkWidget *makeStateBox(AppData app_data) ;

static void requestZMapStop(AppData app_data) ;
static gboolean stopZMapCB(gpointer user_data) ;
static void resetGUI(AppData app_data) ;
static void resetTextAreas(AppData app_data) ;

static void requestHandlerCB(ZMapRemoteControl remote_control,
			     ZMapRemoteControlReturnReplyFunc remote_reply_func, void *remote_reply_data,
			     char *request, void *user_data) ;
static void replySentCB(void *user_data) ;

static void requestSentCB(void *user_data) ;
static void replyHandlerCB(ZMapRemoteControl remote_control, char *reply, void *user_data) ;

static void errorCB(ZMapRemoteControl remote_control,
		    ZMapRemoteControlRCType error_type, char *err_msg,
		    void *user_data) ;

void processRequest(AppData app_data, char *request,
		    RemoteCommandRCType *return_code, char **reason_out,
		    ZMapXMLUtilsEventStack *reply_out, char **reply_txt_out) ;
static char *makeReply(AppData app_data, char *request, char *error_msg,
		       ZMapXMLUtilsEventStack raw_reply, char *reply_txt_out) ;
static char *makeReplyFromText(char *envelope, char *dummy_body, char *reply_txt) ;
static gboolean handleHandshake(char *command_text, AppData app_data) ;


static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;
static gboolean xml_peer_start_cb(gpointer user_data, ZMapXMLElement client_element, ZMapXMLParser parser) ;



static void sendCommandCB(GtkWidget *button, gpointer user_data);
static void listViewsCB(GtkWidget *button, gpointer user_data);

static void list_views_cb(gpointer data, gpointer user_data) ;




/* 
 *    old funcs....
 */


static GtkWidget *entry_box_widgets(AppData app_data);
static GtkWidget *menubar(GtkWidget *parent, AppData app_data);
static GtkWidget *message_box(AppData app_data);
static GtkWidget *button_bar(AppData app_data);



static void cmdCB( gpointer data, guint callback_action, GtkWidget *w );
static void runZMapCB( gpointer data, guint callback_action, GtkWidget *w );
static void killZMapCB( gpointer data, guint callback_action, GtkWidget *w );
static void menuQuitCB(gpointer data, guint callback_action, GtkWidget *w);

static void quitCB(GtkWidget *button, gpointer user_data);
static void clearCB(GtkWidget *button, gpointer user_data);
static void parseCB(GtkWidget *button, gpointer user_data);

static void toggleAndUpdate(GtkWidget *toggle_button) ;



static char *getXML(AppData app_data, int *length_out);




static XRemoteCmdLineArgs getCommandLineArgs(int argc, char *argv[]);
static gboolean makeOptionContext(XRemoteCmdLineArgs arg_context);
static GOptionEntry *get_main_entries(XRemoteCmdLineArgs arg_context);


static ZMapConfigIniContext loadRemotecontrolConfig(AppData app_data);
static ZMapConfigIniContextKeyEntry get_programs_group_data(char **stanza_name, char **stanza_type);




/* 
 *            Globals
 */

static gboolean command_debug_G = TRUE ;

static GtkItemFactoryEntry menu_items_G[] =
  {
    {"/_File",                   NULL,         NULL,       0,                  "<Branch>", NULL},
    {"/File/Read",               NULL,         NULL,       0,                  NULL,       NULL},
    {"/File/Quit",               "<control>Q", menuQuitCB, 0,                  NULL,       NULL},
    {"/_ZMap Control",               NULL,         NULL,  0,             "<Branch>", NULL},
    {"/_ZMap Control/New ZMap",  NULL,         runZMapCB,  0,             NULL, NULL},
    {"/_ZMap Control/Kill ZMap",  NULL,        killZMapCB,  0,             NULL, NULL},
    {"/_Commands",               NULL,         NULL,       0,                  "<Branch>", NULL},
    {"/Commands/ping",           NULL,         cmdCB,      XREMOTE_PING,       NULL,       NULL},

    {"/Commands/new_view",       NULL,         cmdCB,      XREMOTE_NEW_VIEW,   NULL,       NULL},
    {"/Commands/add_to_view",    NULL,         cmdCB,      XREMOTE_ADD_VIEW,   NULL,       NULL},
    {"/Commands/close_view",     NULL,         cmdCB,      XREMOTE_CLOSE_VIEW, NULL,       NULL},

    {"/Commands/load_features",  NULL,         cmdCB,      XREMOTE_LOAD_FEATURES, NULL,       NULL},
    {"/Commands/zoom_to",        NULL,         cmdCB,      XREMOTE_ZOOM_TO,    NULL,       NULL},
    {"/Commands/get_mark",       NULL,         cmdCB,      XREMOTE_GET_MARK,   NULL,       NULL},
    {"/Commands/rev_comp",       NULL,         cmdCB,      XREMOTE_REVCOMP,   NULL,       NULL},

    {"/Commands/Feature Create (simple)", NULL,         cmdCB,      XREMOTE_CREATE_SIMPLE,   NULL,       NULL},
    {"/Commands/Feature Create", NULL,         cmdCB,      XREMOTE_CREATE,   NULL,       NULL},
    {"/Commands/Feature Replace",NULL,         cmdCB,      XREMOTE_REPLACE,     NULL,       NULL},
    {"/Commands/Feature Delete", NULL,         cmdCB,      XREMOTE_DELETE,   NULL,       NULL},
    {"/Commands/Feature Find",   NULL,         cmdCB,      XREMOTE_FIND,     NULL,       NULL},

    {"/Commands/goodbye",        NULL,         cmdCB,      XREMOTE_GOODBYE,   NULL,       NULL},
    {"/Commands/shutdown",       NULL,         cmdCB,      XREMOTE_SHUTDOWN,   NULL,       NULL},
    {"/Commands/abort",          NULL,         cmdCB,      XREMOTE_ABORT,      NULL,       NULL},

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    {"/Commands/Zoom In",        NULL,         cmdCB,      XREMOTE_ZOOMIN,   NULL,       NULL},
    {"/Commands/Zoom Out",       NULL,         cmdCB,      XREMOTE_ZOOMOUT,  NULL,       NULL},

    {"/Commands/register client",     NULL,         cmdCB,      XREMOTE_REGISTER_CLIENT,   NULL,       NULL}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  };


static gboolean debug_G = TRUE ;






/* 
 *              External functions.
 */


int main(int argc, char *argv[])
{
  AppData app_data ;
  XRemoteCmdLineArgs cmd_args ;


  gtk_init(&argc, &argv) ;				    /* gtk removes it's args. */

  cmd_args = getCommandLineArgs(argc, argv) ;

  if (cmd_args->version)
    {
      /* hacked for now...in the end will be supported by general library routines. */

      printf("%s",
             "remotecontrol 1.1.1\n"
             "\n"
             "Copyright (c) 2006-2015: Genome Research Ltd.\n"
             "ZMap is distributed under the GNU  Public License, see http://www.gnu.org/copyleft/gpl.txt\n"
             "\n"
             "Written by Ed Griffiths et al.\n") ;

      appExit(TRUE) ;
    }


  /* Make the main command block. */
  app_data = createAppData(argv[0], cmd_args) ;


  /* If sequence/start/end not specified on command line look in the zmap config file, if they
   * aren't there then we just leave them blank. */
  if (!(app_data->cmd_line_args->sequence && app_data->cmd_line_args->start && app_data->cmd_line_args->end))
    getZMapConfiguration(app_data) ;


  /* Make the test program control window. */
  app_data->app_toplevel = makeTestWindow(app_data) ;


  gtk_widget_show_all(app_data->app_toplevel) ;

  gtk_main() ;

  appExit(TRUE) ;

  return EXIT_FAILURE ;					    /* we shouldn't ever get here. */
}






/*
 *                Internal functions.
 */

static AppData createAppData(char *argv_path, XRemoteCmdLineArgs cmd_args)
{
  AppData app_data = NULL ;

  app_data = g_new0(AppDataStruct, 1) ;

  app_data->our_app_name = g_path_get_basename(argv_path) ;

  app_data->cmd_line_args = cmd_args ;

  app_data->config_context = loadRemotecontrolConfig(app_data) ;

  /* Set early so we can display it in the GUI. */
  if ((app_data->cmd_line_args->zmap_path))
    app_data->zmap_path = app_data->cmd_line_args->zmap_path ;
  else
    app_data->zmap_path = "./zmap" ;


  return app_data ;
}


static void destroyAppData(AppData app_data)
{
  if (app_data->config_context)
    zMapConfigIniContextDestroy(app_data->config_context) ;

  /* May need more than this..... */
  g_option_context_free(app_data->cmd_line_args->opt_context) ;
  g_free(app_data->cmd_line_args->zmap_config_file) ;
  g_free(app_data->cmd_line_args) ;

  g_free(app_data->our_app_name) ;

  g_free(app_data) ;

  return ;
}


/* Make sure we return a pucker exit code in the right way. */
static void appExit(gboolean exit_ok)
{
  int true_rc ;

  if (exit_ok)
    true_rc = EXIT_SUCCESS ;
  else
    true_rc = EXIT_FAILURE ;

  gtk_exit(true_rc) ;

  return ;						    /* We shouldn't ever get here.... */
}



/* Package together commands to start/stop a zmap. */
static gboolean startZMap(AppData app_data)
{
  gboolean result = FALSE ;

  if (!(app_data->zmap_pid))
    {
      char **zmap_command ;

      initRemote(app_data) ;

      zmap_command = buildZMapCmd(app_data) ;

      createZMapProcess(app_data, zmap_command) ;

      g_strfreev(zmap_command) ;

      result = TRUE ;
    }

  return result ;
}

static void stopZMap(AppData app_data)
{
  if (app_data->zmap_pid)
    {
      destroyZMapProcess(app_data) ;
    }
  else
    {
      clearUpZMap(app_data) ;
    }

  return ;
}



/* Init/destroy remote control interface. */
static void initRemote(AppData app_data)
{
  gboolean result = FALSE ;

  if ((app_data->remote_cntl = zMapRemoteControlCreate(REMOTECONTROL_APPNAME,
                                                          errorCB, app_data,
                                                          NULL, NULL)))
    result = TRUE ;

  if (result)
    zMapRemoteControlSetTimeoutList(app_data->remote_cntl, TIMEOUT_LIST) ;


  if (result)
    {
      if ((result = zMapRemoteControlReceiveInit(app_data->remote_cntl,
                                                 &(app_data->our_socket_str),
						 requestHandlerCB, app_data,
						 replySentCB, app_data)))
        {
          gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app_data->receive_init), TRUE) ;
        }
    }

  if (result)
    {
      /* Needs to change to reflect state... */
      toggleAndUpdate(app_data->waiting) ;
    }

  if (!result)
    {
      zMapCrash("%s", "Could not initialise remote control, exiting") ;
      
      appExit(FALSE) ;
    }

  return ;
}


static void destroyRemote(AppData app_data)
{
  if (app_data->remote_cntl)
    {
      /* Not sure if this is the right place to do this.... */
      app_data->send_interface_init = FALSE ;

      zMapRemoteControlDestroy(app_data->remote_cntl) ;

      app_data->remote_cntl = NULL ;
    }

  return ;
}



/* Handle the zmap process start up/kill. */
static gboolean createZMapProcess(AppData app_data, char **command)
{
  char **ptr;
  char *cwd = NULL, **envp = NULL; /* inherit from parent */
  GSpawnFlags flags = (G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD) ;
  GSpawnChildSetupFunc pre_exec = NULL;
  gpointer user_data = NULL;
  GError *error = NULL;
  gboolean success = FALSE;

  if(command_debug_G)
    {
      printf("Running command: ");
      ptr = &command[0];
      while(ptr && *ptr)
	{
	  printf("%s ", *ptr);
	  ptr++;
	}
      printf("\n");
    }

  if(!(success = g_spawn_async(cwd, command, envp,
			       flags, pre_exec, user_data,
			       &(app_data->zmap_pid), &error)))
    {
      printf("Errror %s\n", error->message);
    }
  else
    {
      app_data->child_watch_cb_id = g_child_watch_add(app_data->zmap_pid, zmapDieCB, app_data) ;
    }


  return success;
}

static void destroyZMapProcess(AppData app_data)
{
  if (app_data->zmap_pid != 0)
    {
      kill(app_data->zmap_pid, SIGKILL) ;

    }

  return ;
}


/* A GChildWatchFunc() called when the child zmap process dies. */
static void zmapDieCB(GPid pid, gint status, gpointer user_data)
{
  AppData app_data = (AppData)user_data ;

  if (app_data->child_watch_cb_id)
    {
      app_data->child_watch_cb_id = 0 ;

      g_spawn_close_pid(app_data->zmap_pid) ;
      app_data->zmap_pid = 0 ;

      clearUpZMap(app_data) ;
    }

  return ;
}


static void clearUpZMap(AppData app_data)
{
  resetGUI(app_data) ;

  destroyRemote(app_data) ;

  return ;
}




/* Build the command parameters to run zmap */
static char **buildZMapCmd(AppData app_data)
{
  char **zmap_command = NULL ;
  char *zmap_path  = NULL;
  char *tmp_string = NULL;
  gboolean debugger = app_data->cmd_line_args->debugger ;
  GString *cmd_str ;
  char *sleep = app_data->cmd_line_args->sleep_seconds ;
  char *remote_debug = app_data->cmd_line_args->remote_debug ;
  int screen_num ;

  /* Have to do all this as user may blank out the path or change it or indeed lose it.... */
  if ((tmp_string = (char *)gtk_entry_get_text(GTK_ENTRY(app_data->zmap_path_entry))) && *tmp_string)
    {
      /* Is there a path in the GUI. */
      zmap_path = tmp_string ;
    }
  else if ((app_data->config_context)
	   && (zMapConfigIniContextGetString(app_data->config_context,
					     XREMOTE_PROG_CONFIG, XREMOTE_PROG_CONFIG, XREMOTE_PROG_ZMAP,
					     &tmp_string)))
    {
      /* Is there a path in the remote control config file. */
      zmap_path = tmp_string ;
    }
  else if (app_data->zmap_path)
    {
      /* Do we have one already. */
      zmap_path = app_data->zmap_path ;
    }
  else if (app_data->cmd_line_args->zmap_path)
    {
      /* Was one specified on the command line. */
      zmap_path = app_data->cmd_line_args->zmap_path ;
    }
  else
    {
      /* If all else fails. */
      zmap_path = "./zmap";
    }

  tmp_string = NULL ;

  if (app_data->config_context != NULL)
    zMapConfigIniContextGetString(app_data->config_context, XREMOTE_PROG_CONFIG, XREMOTE_PROG_CONFIG,
                                  XREMOTE_PROG_ZMAP_OPTS, &tmp_string) ;

  cmd_str = g_string_sized_new(500) ;

  if (debugger)
    g_string_append(cmd_str, "totalview -a") ;

  g_string_append_printf(cmd_str, " %s", zmap_path) ;

  /* Run zmap on same screen as remote tool by default, better for debugging. */
  if ((screen_num = gdk_screen_get_number(gtk_widget_get_screen(app_data->app_toplevel))) != 0)
    g_string_append_printf(cmd_str, " --screen=%d", screen_num) ;


  if (sleep)
    g_string_append_printf(cmd_str, " --sleep=%s", sleep) ;

  if (remote_debug)
    g_string_append_printf(cmd_str, " --remote-debug=%s", remote_debug) ;

  g_string_append_printf(cmd_str, " --%s=%s",
                         ZMAPARG_PEER_SOCKET,
                         app_data->our_socket_str) ;

  if (app_data->cmd_line_args->zmap_config_file)
    {
      char *dirname ;

      dirname = g_path_get_dirname(app_data->cmd_line_args->zmap_config_file) ;

      g_string_append_printf(cmd_str, " --%s=%s", ZMAPARG_CONFIG_DIR, dirname) ;

      g_free(dirname) ;
    }

  if (tmp_string)
    g_string_append(cmd_str, tmp_string) ;

  zmap_command = buildParamList(cmd_str->str) ;

  g_string_free(cmd_str, TRUE) ;

  g_free(tmp_string) ;


  return zmap_command ;
}

/* build parameter list for call to start a process. */
static char **buildParamList(char *params_as_string)
{
  char **command_out = NULL;
  char **split ;
  char *copy ;

  copy = g_strdup(params_as_string) ;
  copy = g_strstrip(copy) ;

  if ((split = g_strsplit(copy, " ", 0)))
    command_out = split ;

  g_free(copy) ;

  return command_out ;
}




/* 
 *             Routines to make the GUI
 */

static GtkWidget *makeTestWindow(AppData app_data)
{
  GtkWidget *toplevel = NULL ;
  GtkWidget *top_hbox, *top_vbox, *menu_bar, *buttons, *entry_box, *state_box ;

  toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;

  g_signal_connect(G_OBJECT(toplevel), "destroy",
		   G_CALLBACK(quitCB), (gpointer)app_data) ;

  gtk_window_set_title(GTK_WINDOW(toplevel), "ZMap XRemote Test Suite");
  gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;

  /* Top level vbox for menu bar, request/response windows etc. */
  app_data->vbox = top_vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(toplevel), top_vbox);


  /* Make the menu bar. */
  app_data->menu = menu_bar = menubar(toplevel, app_data) ;
  gtk_box_pack_start(GTK_BOX(top_vbox), menu_bar, FALSE, TRUE, 5) ;


  /* Make the buttons. */
  app_data->buttons = buttons = button_bar(app_data) ;
  gtk_box_pack_start(GTK_BOX(top_vbox), buttons, TRUE, TRUE, 5) ;


  /* Make the sub text entry boxes. */
  entry_box = entry_box_widgets(app_data) ;
  gtk_box_pack_start(GTK_BOX(top_vbox), entry_box, TRUE, TRUE, 5);


  /* Make the State box. */
  state_box = makeStateBox(app_data) ;
  gtk_box_pack_start(GTK_BOX(top_vbox), state_box, TRUE, TRUE, 5);


  /* Main request/response display */
  top_hbox = makeMainReqResp(app_data) ;
  gtk_box_pack_start(GTK_BOX(top_vbox), top_hbox, TRUE, TRUE, 5);


  return toplevel ;
}


/* Make all the request/response display windows. */
static GtkWidget *makeMainReqResp(AppData app_data)
{
  GtkWidget *top_hbox = NULL ;
  GtkWidget *send_vbox, *receive_vbox ;

  top_hbox = gtk_hbox_new(FALSE, 10) ;

  /* Build Send command boxes. */
  send_vbox = GTK_WIDGET(makeReqRespBox(app_data,
					GTK_BOX(top_hbox), "SEND TO ZMAP",
					&(app_data->our_req), "Our Request", "ZMap's Response")) ;

  /* Build Receive command boxes. */
  receive_vbox = GTK_WIDGET(makeReqRespBox(app_data,
					   GTK_BOX(top_hbox), "RECEIVE FROM ZMAP",
					   &(app_data->zmap_req), "ZMap's Request", "Our Response")) ;

  return top_hbox ;
}


/* Make a pair of request/response windows. */
static GtkWidget *makeReqRespBox(AppData app_data,
				 GtkBox *parent_box, char *box_title,
				 ReqRespText text_widgs,
				 char *request_title, char *response_title)
{
  GtkWidget *vbox = NULL ;
  GtkWidget *frame ;

  frame = gtk_frame_new(box_title) ;
  gtk_box_pack_start(parent_box, frame, TRUE, TRUE, 0) ;

  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(frame), vbox) ;

  addTextArea(app_data, GTK_BOX(vbox), request_title,
	      &(text_widgs->request_text_area), &(text_widgs->request_text_buffer)) ;

  /* move up several levels ?? */
  gtk_text_buffer_set_text(text_widgs->request_text_buffer, "Enter xml here...", -1) ;

  addTextArea(app_data, GTK_BOX(vbox), response_title,
	      &(text_widgs->response_text_area), &(text_widgs->response_text_buffer)) ;

  gtk_text_view_set_editable(GTK_TEXT_VIEW(text_widgs->response_text_area), FALSE) ;
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_widgs->response_text_area), FALSE) ;

  return vbox ;
}


static void addTextArea(AppData app_data, GtkBox *parent_box, char *text_title,
			GtkWidget **text_area_out, GtkTextBuffer **text_buffer_out)
{
  GtkWidget *our_vbox, *frame, *scrwin, *textarea ;

  our_vbox = GTK_WIDGET(gtk_vbox_new(FALSE, 0)) ;
  gtk_box_pack_start(parent_box, our_vbox, TRUE, TRUE, 5) ;

  frame = gtk_frame_new(text_title) ;

  scrwin = gtk_scrolled_window_new(NULL, NULL) ;

  *text_area_out = textarea = message_box(app_data) ;
  *text_buffer_out = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textarea)) ;

  gtk_container_add(GTK_CONTAINER(scrwin), textarea) ;

  gtk_container_add(GTK_CONTAINER(frame), scrwin) ;

  gtk_box_pack_start(GTK_BOX(our_vbox), frame, TRUE, TRUE, 5);

  return ;
}



/* create the entry box widgets */
static GtkWidget *entry_box_widgets(AppData app_data)
{
  GtkWidget *frame ;
  GtkWidget *entry_box, *sequence, *label, *path;

  frame = gtk_frame_new("Set Parameters") ;

  entry_box = gtk_hbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), entry_box) ;

  label = gtk_label_new("sequence :");
  gtk_box_pack_start(GTK_BOX(entry_box), label, FALSE, FALSE, 5);
  app_data->sequence_entry = sequence = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(entry_box), sequence, FALSE, FALSE, 5);
  gtk_entry_set_text(GTK_ENTRY(sequence), app_data->cmd_line_args->sequence);

  label = gtk_label_new("start :");
  gtk_box_pack_start(GTK_BOX(entry_box), label, FALSE, FALSE, 5) ;
  app_data->start_entry = sequence = gtk_entry_new() ;
  gtk_box_pack_start(GTK_BOX(entry_box), sequence, FALSE, FALSE, 5) ;
  if (app_data->cmd_line_args->start)
    gtk_entry_set_text(GTK_ENTRY(sequence), app_data->cmd_line_args->start) ;

  label = gtk_label_new("end :");
  gtk_box_pack_start(GTK_BOX(entry_box), label, FALSE, FALSE, 5);
  app_data->end_entry = sequence = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(entry_box), sequence, FALSE, FALSE, 5);
  if (app_data->cmd_line_args->end)
    gtk_entry_set_text(GTK_ENTRY(sequence), app_data->cmd_line_args->end) ;

  label = gtk_label_new("config file path :");
  gtk_box_pack_start(GTK_BOX(entry_box), label, FALSE, FALSE, 5);
  app_data->config_entry  = sequence = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(entry_box), sequence, FALSE, FALSE, 5);
  if (app_data->cmd_line_args->zmap_config_file)
    gtk_entry_set_text(GTK_ENTRY(sequence), app_data->cmd_line_args->zmap_config_file) ;

  label = gtk_label_new("zmap executable path :");
  gtk_box_pack_start(GTK_BOX(entry_box), label, FALSE, FALSE, 5);
  app_data->zmap_path_entry = path = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(entry_box), path, FALSE, FALSE, 5);
  if (app_data->zmap_path)
    gtk_entry_set_text(GTK_ENTRY(path), app_data->zmap_path) ;


  return frame ;
}

/* create the menu bar */
static GtkWidget *menubar(GtkWidget *parent, AppData app_data)
{
  GtkWidget *menubar;
  GtkItemFactory *factory;
  GtkAccelGroup *accel;
  gint nmenu_items = sizeof (menu_items_G) / sizeof (menu_items_G[0]);

  accel = gtk_accel_group_new();

  factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel);

  gtk_item_factory_create_items(factory, nmenu_items, menu_items_G, app_data);

  gtk_window_add_accel_group(GTK_WINDOW(parent), accel);

  menubar = gtk_item_factory_get_widget(factory, "<main>");

  return menubar;
}

/* the message box where xml is entered */
static GtkWidget *message_box(AppData app_data)
{
  GtkWidget *message_box;

  message_box = gtk_text_view_new() ;
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(message_box), GTK_WRAP_WORD) ;

  gtk_widget_set_size_request(message_box, -1, 200) ;

  return message_box;
}

/* All the buttons of the button bar */
static GtkWidget *button_bar(AppData app_data)
{
  GtkWidget *button_bar, *send_command,
    *clear, *parse, *list_views ;

  button_bar   = gtk_hbox_new(FALSE, 0) ;

  send_command = gtk_button_new_with_label("Send Command");
  list_views = gtk_button_new_with_label("List Views");
  clear        = gtk_button_new_with_label("Clear XML");
  parse        = gtk_button_new_with_label("Check XML");

  gtk_box_pack_start(GTK_BOX(button_bar), send_command, TRUE, TRUE, 5) ;
  gtk_box_pack_start(GTK_BOX(button_bar), list_views, TRUE, TRUE, 5) ;
  gtk_box_pack_start(GTK_BOX(button_bar), clear, TRUE, TRUE, 5) ;
  gtk_box_pack_start(GTK_BOX(button_bar), parse, TRUE, TRUE, 5) ;


  g_signal_connect(G_OBJECT(send_command), "clicked",
                   G_CALLBACK(sendCommandCB), app_data);

  g_signal_connect(G_OBJECT(list_views), "clicked",
                   G_CALLBACK(listViewsCB), app_data);

  g_signal_connect(G_OBJECT(clear), "clicked",
                   G_CALLBACK(clearCB), app_data);

  g_signal_connect(G_OBJECT(parse), "clicked",
                   G_CALLBACK(parseCB), app_data);

  return button_bar;
}


/* Make a set of radio buttons which show current state of remote control. */
static GtkWidget *makeStateBox(AppData app_data)
{
  GtkWidget *frame, *hbox, *init_box, *state_box, *button = NULL ;

  hbox = gtk_hbox_new(FALSE, 0);

  frame = gtk_frame_new("RemoteControl Initialisation") ;
  gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 5) ;

  init_box = gtk_hbutton_box_new() ;
  gtk_container_add(GTK_CONTAINER(frame), init_box) ;

  app_data->send_init = button = gtk_check_button_new_with_label("Send Interface initialised") ;
  gtk_box_pack_start(GTK_BOX(init_box), button, TRUE, TRUE, 5) ;
  app_data->receive_init = button = gtk_check_button_new_with_label("Receive Interface initialised") ;
  gtk_box_pack_start(GTK_BOX(init_box), button, TRUE, TRUE, 5) ;


  frame = gtk_frame_new("RemoteControl State") ;
  gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 5) ;

  state_box = gtk_hbutton_box_new() ;
  gtk_container_add(GTK_CONTAINER(frame), state_box) ;

  button = NULL ;
  app_data->idle = button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(button), "Idle") ;
  gtk_box_pack_start(GTK_BOX(state_box), button, TRUE, TRUE, 5) ;
  app_data->waiting = button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(button), "Waiting") ;
  gtk_box_pack_start(GTK_BOX(state_box), button, TRUE, TRUE, 5) ;
  app_data->sending = button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(button), "Sending") ;
  gtk_box_pack_start(GTK_BOX(state_box), button, TRUE, TRUE, 5) ;
  app_data->receiving = button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(button), "Receiving") ;
  gtk_box_pack_start(GTK_BOX(state_box), button, TRUE, TRUE, 5) ;

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app_data->idle), TRUE) ;


  return hbox ;
}


static void requestZMapStop(AppData app_data)
{
  app_data->stop_zmap_func_id = g_timeout_add(100, stopZMapCB, app_data) ;

 return ;
}

static gboolean stopZMapCB(gpointer user_data)
{
  gboolean stop = FALSE ;
  AppData app_data = (AppData)user_data ;

  app_data->stop_zmap_func_id = 0 ;

  stopZMap(app_data) ;

  return stop ;
}



/* Called after zmap process is stopped to reset parts of the interface,
 * the message areas are not cleared so that the user can see any last
 * messages. */
static void resetGUI(AppData app_data)
{
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app_data->send_init), FALSE) ;

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app_data->receive_init), FALSE) ;

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app_data->idle), TRUE) ;

  return ;
}

static void resetTextAreas(AppData app_data)
{
  gtk_text_buffer_set_text(app_data->our_req.request_text_buffer, "", -1);
  gtk_text_buffer_set_text(app_data->our_req.response_text_buffer, "", -1);
  gtk_text_buffer_set_text(app_data->zmap_req.request_text_buffer, "", -1) ;
  gtk_text_buffer_set_text(app_data->zmap_req.response_text_buffer, "", -1) ;

  return ;
}





/*
 *                   Widget callbacks
*/

/* time to clean up */
static void quitCB(GtkWidget *unused_button, gpointer user_data)
{
  AppData app_data = (AppData)user_data;

  stopZMap(app_data) ;

  destroyAppData(app_data) ;

  appExit(TRUE) ;

  return ;
}





/* 
 *             Functions handling requests received from zmap
 */

/* Called our remotecontrol receives a request from zmap, we should check the request and
 * handle it and then call our remotecontrol back to pass our reply back.
 */
static void requestHandlerCB(ZMapRemoteControl remote_control,
			     ZMapRemoteControlReturnReplyFunc remote_reply_func, void *remote_reply_data,
			     char *request, void *user_data)
{
  AppData app_data = (AppData)user_data ;
  RemoteValidateRCType result = REMOTE_VALIDATE_RC_OK ;
  RemoteCommandRCType request_rc ;
  ZMapXMLUtilsEventStack reply = NULL ;
  char *reply_txt = NULL ;
  char *full_reply = NULL ;
  char *error_msg = NULL ;


  zMapDebugPrint(debug_G, "%s", "Enter...") ;

  /* Show state: we are ready to receive now... */
  toggleAndUpdate(app_data->receiving) ;


  /* Set their request in the window and blank reply window.... */
  gtk_text_buffer_set_text(app_data->zmap_req.request_text_buffer, request, -1) ;
  gtk_text_buffer_set_text(app_data->zmap_req.response_text_buffer, "", -1) ;


  /* Sort out error handling here....poor..... */
  if ((result = zMapRemoteCommandValidateRequest(remote_control, request, &error_msg))
      == REMOTE_VALIDATE_RC_OK)
    {
      processRequest(app_data, request, &request_rc, &error_msg, &reply, &reply_txt) ;

      /* Format reply..... */
      full_reply = makeReply(app_data, request, error_msg, reply, reply_txt) ;

      /* Set our reply in the window. */
      gtk_text_buffer_set_text(app_data->zmap_req.response_text_buffer, full_reply, -1) ;
    }
  else
    {
      full_reply = error_msg ;
    }


  /* MUST REPLY WHATEVER HAPPENS.... */

  /* this is where we need to process the zmap commands perhaps passing on the remote_reply_func
   * and data to be called from a callback for an asynch. request.
   * 
   * For now we just call straight back for testing.... */
  (remote_reply_func)(remote_reply_data, FALSE, full_reply) ;


  zMapDebugPrint(debug_G, "%s", "Exit...") ;

  return ;
}


/* Called by our remotecontrol when it receives acknowledgement that the reply
 * has been received by zmap, at this point we are free to send another command
 * or to wait for the next command. */
static void replySentCB(void *user_data)
{
  AppData app_data = (AppData)user_data ;

  toggleAndUpdate(app_data->idle) ;

  toggleAndUpdate(app_data->waiting) ;

  return ;
}



/* 
 *             Functions handling requests sent to zmap
 */


/* Called by our remotecontrol when it receives acknowledgement from zmap
 * that it has our request. */
static void requestSentCB(void *user_data)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  AppData app_data = (AppData)user_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}


/* Called by our remotecontrol to process the reply it gets from zmap. */
static void replyHandlerCB(ZMapRemoteControl remote_control, char *reply, void *user_data)
{
  AppData app_data = (AppData)user_data ;
  char *command = NULL ;
  RemoteCommandRCType command_rc ;
  char *reason = NULL ;
  char *reply_body = NULL ;
  char *error_out = NULL ;

  zMapDebugPrint(debug_G, "%s", "Enter...") ;


  /* Need to parse out reply components..... */
  if (!(zMapRemoteCommandReplyGetAttributes(reply,
					    &command,
					    &command_rc, &reason,
					    &reply_body,
					    &error_out)))
    {
      /* Report a bad reply..... */
      char *err_msg ;

      err_msg = g_strdup_printf("Bad format reply: \"%s\".", reply) ;
      gtk_text_buffer_set_text(app_data->our_req.response_text_buffer, err_msg, -1) ;
      g_free(err_msg) ;
    }
  else if (command_rc != REMOTE_COMMAND_RC_OK)
    {
      gtk_text_buffer_set_text(app_data->our_req.response_text_buffer, reply, -1) ;

      zMapWarning("Command \"%s\" failed with return code \"%s\" and reason \"%s\".",
		  command, zMapRemoteCommandRC2Str(command_rc), reason) ;
    }
  else
    {
      /* Display and process the reply. */
      char *attribute_value = NULL ;
      char *err_msg = NULL ;

      gtk_text_buffer_set_text(app_data->our_req.response_text_buffer, reply, -1) ;

      if (strcmp(command, ZACP_NEWVIEW) == 0 || strcmp(command, ZACP_ADD_TO_VIEW) == 0)
	{
	  /* Get hold of the new view id and store it. */
	  if (!zMapRemoteCommandGetAttribute(reply, 
					     ZACP_VIEW, ZACP_VIEWID, &attribute_value,
					     &err_msg))
	    {
	      zMapWarning("Could not find element \"%s\", attribute \"%s\" in reply.",
			  ZACP_VIEW, ZACP_VIEWID) ;
	    }
	  else
	    {
	      GQuark view_id ;

	      view_id = g_quark_from_string(attribute_value) ;

	      app_data->views = g_list_append(app_data->views, (char *)g_quark_to_string(view_id)) ;
	    }
	}
      else if (strcmp(command, ZACP_CLOSEVIEW) == 0)
	{
	  char *attribute_value = NULL ;
	  char *err_msg = NULL ;

	  /* Closed a view so remove it from our list. */
	  if (!zMapRemoteCommandGetAttribute(app_data->curr_request,
					     ZACP_REQUEST, ZACP_VIEWID, &attribute_value, &err_msg))
	    {
	      zMapCritical("Could not find \"%s\" in current request \"%s\".",
			   ZACP_VIEW, app_data->curr_request) ;
	    }
	  else
	    {
	      GQuark view_id ;

	      view_id = g_quark_from_string(attribute_value) ;

	      app_data->views = g_list_remove(app_data->views, g_quark_to_string(view_id)) ;
	    }
	}
      else if (strcmp(command, ZACP_SHUTDOWN) == 0)
	{
          /* We need to clean up much more than this !!!!!!! */
          
          requestZMapStop(app_data) ;

	  g_list_free(app_data->views) ;
	  app_data->views = NULL ;
	}
    }


  /* Clear our copy of current_request, important as user may edit request directly which means
   * we are out of sync.... */
  if (app_data->curr_request)
    {
      g_free(app_data->curr_request) ;

      app_data->curr_request = NULL ;
    }


  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app_data->idle), TRUE) ;

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app_data->waiting), TRUE) ;



  zMapDebugPrint(debug_G, "%s", "Exit...") ;

  return ;
}


static void errorCB(ZMapRemoteControl remote_control,
		    ZMapRemoteControlRCType error_type, char *err_msg,
		    void *user_data)
{
  AppData app_data = (AppData)user_data ;

  zMapDebugPrint(debug_G, "%s", "Enter...") ;

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app_data->waiting), TRUE) ;

  zMapDebugPrint(debug_G, "%s", "Exit...") ;

  return ;
}


/* Processes all requests from zmap. */
void processRequest(AppData app_data, char *request,
		    RemoteCommandRCType *return_code_out, char **reason_out,
		    ZMapXMLUtilsEventStack *reply_out, char **reply_txt_out)
{
  char *action ;

  app_data->error = NULL ;
  app_data->reply = NULL ;
  app_data->reply_txt = NULL ;

  /* Find out what action has been requested. */
  if (!(action = zMapRemoteCommandRequestGetCommand(request)))
    {
      app_data->reply_rc = REMOTE_COMMAND_RC_BAD_XML ;

      app_data->error = "Cannot find \"command\" in request." ;
    }
  else
    {
      char *attribute_value = NULL ;
      char *err_msg = NULL ;

      app_data->parser = zMapXMLParserCreate(app_data, FALSE, FALSE) ;

      if (g_ascii_strcasecmp(action, ZACP_HANDSHAKE) == 0)
	{
	  handleHandshake(request, app_data) ;
	}
      else if (g_ascii_strcasecmp(action, ZACP_GOODBYE) == 0)
	{
	  app_data->reply_rc = REMOTE_COMMAND_RC_OK ;

	  app_data->reply = zMapRemoteCommandMessage2Element("goodbye received, goodbye !") ;
	}
      else if (g_ascii_strcasecmp(action, ZACP_VIEW_DELETED) == 0)
	{
	  /* Closed a view so remove it from our list. */
	  if (!zMapRemoteCommandGetAttribute(request,
					     ZACP_VIEW, ZACP_VIEWID, &attribute_value, &err_msg))
	    {
	      zMapWarning("Could not find \"%s\" in current request \"%s\".", ZACP_VIEW, request) ;
	    }
	  else
	    {
	      GQuark view_id ;

	      view_id = g_quark_from_string(attribute_value) ;

	      app_data->views = g_list_remove(app_data->views, g_quark_to_string(view_id)) ;
	    }

	  app_data->reply_rc = REMOTE_COMMAND_RC_OK ;

	  app_data->reply = zMapRemoteCommandMessage2Element("view deleted...thanks !") ;
	}
      else if (g_ascii_strcasecmp(action, ZACP_VIEW_CREATED) == 0)
	{
	  /* Get hold of the new view id and store it. */
	  if (!zMapRemoteCommandGetAttribute(request, 
					     ZACP_VIEW, ZACP_VIEWID, &attribute_value, &err_msg))
	    {
	      zMapWarning("Could not find \"%s\" in current request \"%s\".", ZACP_VIEW, request) ;
	    }
	  else
	    {
	      GQuark view_id ;

	      view_id = g_quark_from_string(attribute_value) ;

	      app_data->views = g_list_append(app_data->views, (char *)g_quark_to_string(view_id)) ;
	    }

	  app_data->reply_rc = REMOTE_COMMAND_RC_OK ;

	  app_data->reply = zMapRemoteCommandMessage2Element("view created...thanks !") ;
	}
      else if (g_ascii_strcasecmp(action, ZACP_SELECT_FEATURE) == 0)
	{
	  app_data->reply_rc = REMOTE_COMMAND_RC_OK ;

	  app_data->reply = zMapRemoteCommandMessage2Element("single select received..thanks !") ;
	}
      else if (g_ascii_strcasecmp(action, ZACP_DETAILS_FEATURE) == 0)
	{
	  /* Need to detect that this is a transcript and return long result or return
	   * no data....to test zmap's response.... */
	  if (!(strstr(request, "exon")))
	    {
	      app_data->reply_rc = REMOTE_COMMAND_RC_FAILED ;

	      app_data->error = "No feature details available." ;
	    }
	  else
	    {
	      app_data->reply_rc = REMOTE_COMMAND_RC_OK ;

	      app_data->reply_txt = g_strdup("  <notebook>"
					    "    <chapter>"
					    "     <page name=\"Details\">"
					    "       <subsection name=\"Annotation\">"
					    "       <paragraph type=\"tagvalue_table\">"
					    "         <tagvalue name=\"Transcript Stable ID\" type=\"simple\">OTTHUMT00000254615</tagvalue>"
					    "         <tagvalue name=\"Translation Stable ID\" type=\"simple\">OTTHUMP00000162488</tagvalue>"
					    "         <tagvalue name=\"Transcript author\" type=\"simple\">mt4</tagvalue>"
					    "       </paragraph>"
					    "       <paragraph type=\"tagvalue_table\">"
					    "         <tagvalue name=\"Annotation remark\" type=\"scrolled_text\">corf</tagvalue>"
					    "       </paragraph>"
					    "       <paragraph columns=\"&apos;Type&apos; &apos;Accession.SV&apos;\" name=\"Evidence\" type=\"compound_table\" column_types=\"string string\">"
					    "         <tagvalue type=\"compound\">EST Em:AL528275.3</tagvalue>"
					    "         <tagvalue type=\"compound\">EST Em:AL551081.3</tagvalue>"
					    "         <tagvalue type=\"compound\">EST Em:BF668035.1</tagvalue>"
					    "         <tagvalue type=\"compound\">EST Em:BG708432.1</tagvalue>"
					    "         <tagvalue type=\"compound\">EST Em:BU151181.1</tagvalue>"
					    "         <tagvalue type=\"compound\">EST Em:CB142784.1</tagvalue>"
					    "         <tagvalue type=\"compound\">Protein Tr:Q969U7</tagvalue>"
					    "         <tagvalue type=\"compound\">cDNA Em:AF276707.1</tagvalue>"
					    "         <tagvalue type=\"compound\">cDNA Em:AK057005.1</tagvalue>"
					    "         <tagvalue type=\"compound\">cDNA Em:BC013356.2</tagvalue>"
					    "         <tagvalue type=\"compound\">cDNA Em:CR596477.1</tagvalue>"
					    "         <tagvalue type=\"compound\">cDNA Em:CR599075.1</tagvalue>"
					    "         <tagvalue type=\"compound\">cDNA Em:CR604839.1</tagvalue>"
					    "       </paragraph>"
					    "       </subsection>"
					    "       <subsection name=\"Locus\">"
					    "         <paragraph type=\"tagvalue_table\">"
					    "           <tagvalue name=\"Symbol\" type=\"simple\">PSMG2</tagvalue>"
					    "           <tagvalue name=\"Full name\" type=\"simple\">proteasome (prosome, macropain) assembly chaperone 2</tagvalue>"
					    "           <tagvalue name=\"Locus Stable ID\" type=\"simple\">OTTHUMG00000131703</tagvalue>"
					    "           <tagvalue name=\"Locus author\" type=\"simple\">mt4</tagvalue>"
					    "         </paragraph>"
					    "       </subsection>"
					    "     </page>"
					    "     <page name=\"Exons\">"
					    "       <subsection>"
					    "         <paragraph columns=\"&apos;Start&apos; &apos;End&apos; &apos;Stable ID&apos;\" type=\"compound_table\" column_types=\"int int string\">"
					    "           <tagvalue type=\"compound\">82130 82868 OTTHUME00001460988</tagvalue>"
					    "           <tagvalue type=\"compound\">86254 86425 OTTHUME00001460984</tagvalue>"
					    "           <tagvalue type=\"compound\">92406 92464 OTTHUME00001460987</tagvalue>"
					    "           <tagvalue type=\"compound\">98221 98339 OTTHUME00001460985</tagvalue>"
					    "           <tagvalue type=\"compound\">100214 100387 OTTHUME00001460982</tagvalue>"
					    "           <tagvalue type=\"compound\">104203 104323 OTTHUME00001460986</tagvalue>"
					    "           <tagvalue type=\"compound\">105143 105444 OTTHUME00001460983</tagvalue>"
					    "         </paragraph>"
					    "       </subsection>"
					    "     </page>"
					    "    </chapter>"
					    "  </notebook>") ;
	    }
	}
      else if (g_ascii_strcasecmp(action, ZACP_FEATURES_LOADED) == 0)
	{
	  app_data->reply_rc = REMOTE_COMMAND_RC_OK ;

	  app_data->reply = zMapRemoteCommandMessage2Element("got features loaded...thanks !") ;
	}
      else
	{
	  /* Catch all for commands we haven't supported yet !!! */
	  zMapWarning("Support for command \"%s\" has not been implemented yet !!", request) ;

	  app_data->reply_rc = REMOTE_COMMAND_RC_CMD_UNKNOWN ;

	  app_data->error = "Unsupported command !" ;
	}



    }

  *return_code_out = app_data->reply_rc ;

  if (app_data->error)
    *reason_out = zMapXMLUtilsEscapeStr(app_data->error) ;
  else if (app_data->reply)
    *reply_out = app_data->reply ;
  else
    *reply_txt_out = app_data->reply_txt ;

  return ;
}



static gboolean handleHandshake(char *command_text, AppData app_data)
{
  gboolean parse_result = FALSE ;
  ZMapXMLObjTagFunctionsStruct starts[] =
    {
      {"zmap", xml_zmap_start_cb },
      {"peer", xml_peer_start_cb },
      {NULL, NULL}
    };
  ZMapXMLObjTagFunctionsStruct ends[] =
    {
      {NULL, NULL}
    };
  static ZMapXMLUtilsEventStackStruct
    result_message[] = {{ZMAPXML_START_ELEMENT_EVENT, ZACP_MESSAGE, ZMAPXML_EVENT_DATA_NONE,    {0}},
			{ZMAPXML_CHAR_DATA_EVENT,     NULL,       ZMAPXML_EVENT_DATA_STRING,  {0}},
			{ZMAPXML_END_ELEMENT_EVENT,   ZACP_MESSAGE, ZMAPXML_EVENT_DATA_NONE,    {0}},
			{0}},
    peer[] = {{ZMAPXML_START_ELEMENT_EVENT, ZACP_PEER,      ZMAPXML_EVENT_DATA_NONE,    {0}},
	      {ZMAPXML_ATTRIBUTE_EVENT,     ZACP_SOCKET_ID,    ZMAPXML_EVENT_DATA_QUARK,   {0}},
	      {ZMAPXML_END_ELEMENT_EVENT,   ZACP_PEER,      ZMAPXML_EVENT_DATA_NONE,    {0}},
	      {0}} ;



  zMapXMLParserSetMarkupObjectTagHandlers(app_data->parser, &starts[0], &ends[0]) ;

  if ((parse_result = zMapXMLParserParseBuffer(app_data->parser, command_text, strlen(command_text))))
    {
      /* Parse was ok so act on it.... */
      if (app_data->reply_rc == REMOTE_COMMAND_RC_OK)
	{
	  char *message ;


          if (!app_data->send_interface_init)
            {
              /* Need to init peer interface........ */
              if (zMapRemoteControlSendInit(app_data->remote_cntl,
                                            app_data->peer_socket_str,
                                            requestSentCB, app_data,
                                            replyHandlerCB, app_data))
                {
                  app_data->send_interface_init = TRUE ;
                }
              else
                {
                  app_data->error = g_strdup_printf("Disaster, could not init send interface during"
                                                       " handshake for peer \"%s\", id \"%s\".",
                                                       app_data->peer_app_name, app_data->peer_socket_str) ;

                  zMapWarning("%s", app_data->error) ;

                  app_data->reply_rc = REMOTE_COMMAND_RC_FAILED ;
                }
            }

          if (app_data->reply_rc != REMOTE_COMMAND_RC_FAILED)
	    {
	      GArray *reply_array ;
	      char *error_text = NULL ;

	      message = g_strdup_printf("Handshake successful with peer \"%s\", socket id \"%s\".",
					app_data->peer_app_name, app_data->peer_socket_str) ;

	      /* Need to add this....
		 <peer socket_id="tcp://127.0.0.1:9999"/>
	      */

	      result_message[1].value.s = message ;
	      peer[1].value.q = g_quark_from_string(app_data->peer_socket_str) ;

	      reply_array = zMapXMLUtilsCreateEventsArray() ;

	      reply_array = zMapXMLUtilsAddStackToEventsArrayEnd(reply_array,
								 &result_message[0]) ;
	      
	      reply_array = zMapXMLUtilsAddStackToEventsArrayEnd(reply_array,
								 &peer[0]) ;

	      app_data->reply_txt = zMapXMLUtilsStack2XML(reply_array, &error_text, FALSE) ;

	      /* should free array here..... */

	      app_data->reply_rc = REMOTE_COMMAND_RC_OK ;

	      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app_data->send_init), TRUE) ;
	    }
	}
    }

  return parse_result ;
}



static char *makeReply(AppData app_data, char *request, char *error_msg,
		       ZMapXMLUtilsEventStack raw_reply, char *reply_txt)
{
  char *full_reply = NULL ;
  GArray *xml_stack ;
  char *err_msg = NULL ;

  if (error_msg)
    {
      if ((xml_stack = zMapRemoteCommandCreateReplyFromRequest(app_data->remote_cntl,
							       request, app_data->reply_rc,
							       error_msg, NULL, &err_msg)))
	full_reply = zMapXMLUtilsStack2XML(xml_stack, &err_msg, FALSE) ;
    }
  else if (raw_reply)
    {
      if ((xml_stack = zMapRemoteCommandCreateReplyFromRequest(app_data->remote_cntl,
							       request, app_data->reply_rc,
							       NULL, raw_reply, &err_msg)))
	full_reply = zMapXMLUtilsStack2XML(xml_stack, &err_msg, FALSE) ;
    }
  else if (reply_txt)
    {
      char *final_reply ;

      if ((xml_stack = zMapRemoteCommandCreateReplyEnvelopeFromRequest(app_data->remote_cntl,
								       request,
								       app_data->reply_rc, NULL, NULL,
								       &err_msg)))
	full_reply = zMapXMLUtilsStack2XML(xml_stack, &err_msg, TRUE) ;

      /* Insert full reply into final html.... */
      final_reply = makeReplyFromText(full_reply, NULL, reply_txt) ;
      g_free(full_reply) ;
      full_reply = final_reply ;
    }


  return full_reply ;
}


/* error checking not done...it's a test prog.... */
static char *makeReplyFromText(char *envelope, char *dummy_body, char *reply_txt)
{
  char *full_reply = NULL ;
  GString *full_reply_str ;
  char *pos ;
  size_t bytes ;

  full_reply_str = g_string_sized_new(1000) ;

  /* Look for "<reply", look for closing ">", copy up to there. */
  if ((pos = strstr(envelope, "<reply")))
    {
      if ((pos = strstr(pos, ">")))
	{
	  bytes = (pos - envelope) + 1 ;
	  full_reply_str = g_string_append_len(full_reply_str, envelope, bytes) ;
	  full_reply_str = g_string_append_c(full_reply_str, '\n') ;
	}
    }

  /* Copy in reply body. */
  full_reply_str = g_string_append(full_reply_str, reply_txt) ;

  /* Look for "</reply", copy rest of envelope. */
  if ((pos = strstr(pos, "</reply")))
    {
      full_reply_str = g_string_append_c(full_reply_str, '\n') ;
      full_reply_str = g_string_append(full_reply_str, pos) ;
    }

  full_reply = g_string_free(full_reply_str, FALSE) ;

  return full_reply ;
}



/* menu item -> run a zmap */
static void runZMapCB(gpointer user_data, guint callback_action, GtkWidget *w)
{
  AppData app_data = (AppData)user_data ;

  if ((app_data->zmap_pid))
    zMapGUIShowMsg(ZMAP_MSG_WARNING, "ZMap already running !") ;
  else if (!startZMap(app_data))
    zMapGUIShowMsg(ZMAP_MSG_WARNING, "ZMap launch failed for reasons unknown !") ;

  return ;
}


/* menu item -> kill a zmap */
static void killZMapCB(gpointer user_data, guint callback_action, GtkWidget *w)
{
  AppData app_data = (AppData)user_data ;

  stopZMap(app_data) ;

  return ;
}



/* The "Commands" menu callback which creates the request xml. */
static void cmdCB(gpointer data, guint callback_action, GtkWidget *w)
{
  static ZMapXMLUtilsEventStackStruct

    req_start[] = {{ZMAPXML_START_ELEMENT_EVENT, ZACP_REQUEST,   ZMAPXML_EVENT_DATA_NONE,  {0}},
		   {ZMAPXML_ATTRIBUTE_EVENT,     ZACP_CMD, ZMAPXML_EVENT_DATA_QUARK, {0}},
		   {0}},

    featureset[] = {{ZMAPXML_START_ELEMENT_EVENT, "featureset", ZMAPXML_EVENT_DATA_NONE,    {0}},
		    {ZMAPXML_ATTRIBUTE_EVENT,     "name",       ZMAPXML_EVENT_DATA_QUARK,   {0}},
		    {ZMAPXML_END_ELEMENT_EVENT,   "featureset", ZMAPXML_EVENT_DATA_NONE,    {0}},
		    {0}},

    feature_simple[] = {{ZMAPXML_START_ELEMENT_EVENT, "feature",    ZMAPXML_EVENT_DATA_NONE,    {0}},
			{ZMAPXML_ATTRIBUTE_EVENT,     "name",       ZMAPXML_EVENT_DATA_QUARK,   {0}},
			{ZMAPXML_ATTRIBUTE_EVENT,     "start",      ZMAPXML_EVENT_DATA_INTEGER, {0}},
			{ZMAPXML_ATTRIBUTE_EVENT,     "end",        ZMAPXML_EVENT_DATA_INTEGER, {0}},
			{ZMAPXML_ATTRIBUTE_EVENT,     "strand",     ZMAPXML_EVENT_DATA_QUARK,   {0}},
			{ZMAPXML_END_ELEMENT_EVENT,   "feature",    ZMAPXML_EVENT_DATA_NONE,    {0}},
			{0}},

    feature[] = {{ZMAPXML_START_ELEMENT_EVENT, "feature",    ZMAPXML_EVENT_DATA_NONE,    {0}},
		 {ZMAPXML_ATTRIBUTE_EVENT,     "name",       ZMAPXML_EVENT_DATA_QUARK,   {0}},
		 {ZMAPXML_ATTRIBUTE_EVENT,     "start",      ZMAPXML_EVENT_DATA_INTEGER, {0}},
		 {ZMAPXML_ATTRIBUTE_EVENT,     "end",        ZMAPXML_EVENT_DATA_INTEGER, {0}},
		 {ZMAPXML_ATTRIBUTE_EVENT,     "strand",     ZMAPXML_EVENT_DATA_QUARK,   {0}},
		 {ZMAPXML_START_ELEMENT_EVENT, "subfeature", ZMAPXML_EVENT_DATA_NONE,    {0}},
		 {ZMAPXML_ATTRIBUTE_EVENT,     "start",      ZMAPXML_EVENT_DATA_INTEGER, {0}},
		 {ZMAPXML_ATTRIBUTE_EVENT,     "end",        ZMAPXML_EVENT_DATA_INTEGER, {0}},
		 {ZMAPXML_ATTRIBUTE_EVENT,     "ontology",   ZMAPXML_EVENT_DATA_QUARK,   {0}},
		 {ZMAPXML_END_ELEMENT_EVENT,   "subfeature", ZMAPXML_EVENT_DATA_NONE,    {0}},
		 {ZMAPXML_END_ELEMENT_EVENT,   "feature",    ZMAPXML_EVENT_DATA_NONE,    {0}},
		 {0}},

    sequence[] = {{ZMAPXML_START_ELEMENT_EVENT, ZACP_SEQUENCE_TAG,    ZMAPXML_EVENT_DATA_NONE,    {0}},
		 {ZMAPXML_ATTRIBUTE_EVENT,      ZACP_SEQUENCE_NAME,   ZMAPXML_EVENT_DATA_STRING,  {0}},
		 {ZMAPXML_ATTRIBUTE_EVENT,      ZACP_SEQUENCE_START,  ZMAPXML_EVENT_DATA_INTEGER, {0}},
		 {ZMAPXML_ATTRIBUTE_EVENT,      ZACP_SEQUENCE_END,    ZMAPXML_EVENT_DATA_INTEGER, {0}},
		 {ZMAPXML_ATTRIBUTE_EVENT,      ZACP_SEQUENCE_CONFIG, ZMAPXML_EVENT_DATA_STRING,  {0}},
		 {ZMAPXML_END_ELEMENT_EVENT,    ZACP_SEQUENCE_TAG,    ZMAPXML_EVENT_DATA_NONE,    {0}},
		 {0}},

    abort[] = {{ZMAPXML_START_ELEMENT_EVENT, ZACP_SHUTDOWN_TAG,   ZMAPXML_EVENT_DATA_NONE,    {0}},
	       {ZMAPXML_ATTRIBUTE_EVENT,     ZACP_SHUTDOWN_TYPE,  ZMAPXML_EVENT_DATA_QUARK,  {0}},
	       {ZMAPXML_END_ELEMENT_EVENT,   ZACP_SHUTDOWN_TAG,   ZMAPXML_EVENT_DATA_NONE,    {0}},
	       {0}} ;


  char *default_feature_set = "curated" ;
  char *default_feature_name = "eds_feature" ;
  char *default_replace_feature_name = "eds_replace_feature" ;
  int default_start = 5000 ;
  int default_end = 6000 ;
  char *default_strand = "+" ;
    

  AppData app_data = (AppData)data;
  ZMapXMLUtilsEventStack data_ptr = NULL ;
  GQuark *action ;
  gboolean do_feature_xml = FALSE ;
  gboolean do_replace_xml = FALSE ;

  /* New stuff.... */
  char *command = NULL ;
  char *view_id = NULL ;
  GArray *request_stack ;
  char *request ;
  char *err_msg = NULL ;
  char *next_element ;

  action = &(req_start[1].value.q) ;

  switch(callback_action)
    {
    case XREMOTE_PING:
      {
	*action  = g_quark_from_string(ZACP_PING) ;
	command = ZACP_PING ;

	data_ptr = NULL;

	break;
      }
    case XREMOTE_NEW_VIEW:
    case XREMOTE_ADD_VIEW:
      {
	if (callback_action == XREMOTE_NEW_VIEW)
	  {
	    *action = g_quark_from_string(ZACP_NEWVIEW) ;
	    command = ZACP_NEWVIEW ;
	  }
	else
	  {
	    *action = g_quark_from_string(ZACP_ADD_TO_VIEW) ;
	    command = ZACP_ADD_TO_VIEW ;

	    if (g_list_length(app_data->views) == 1)
	      view_id = (char *)(app_data->views->data) ;
	    else
	      view_id= "NOT_SET" ;
	  }

	data_ptr = &sequence[0] ;
	sequence[1].value.s = g_strdup((char *)gtk_entry_get_text(GTK_ENTRY(app_data->sequence_entry))) ;
	sequence[2].value.i = atoi((char *)gtk_entry_get_text(GTK_ENTRY(app_data->start_entry))) ;
	sequence[3].value.i = atoi((char *)gtk_entry_get_text(GTK_ENTRY(app_data->end_entry))) ;
	sequence[4].value.s = g_strdup((char *)gtk_entry_get_text(GTK_ENTRY(app_data->config_entry))) ;

	break;
      }
    case XREMOTE_CLOSE_VIEW:
      {
	*action = g_quark_from_string(ZACP_CLOSEVIEW) ;
	command = ZACP_CLOSEVIEW ;

	if (g_list_length(app_data->views) == 1)
	  view_id = (char *)(app_data->views->data) ;
	else
	  view_id= "NOT_SET" ;

	break;
      }

    case XREMOTE_LOAD_FEATURES:
      {
	*action = g_quark_from_string(ZACP_LOAD_FEATURES) ;
	command = ZACP_LOAD_FEATURES ;

	featureset[1].value.q = g_quark_from_string("genomic_canonical") ;
	data_ptr = &featureset[0] ;

	break;
      }

    case XREMOTE_ZOOM_TO:
      {
	*action = g_quark_from_string(ZACP_ZOOM_TO) ;
	command = ZACP_ZOOM_TO ;

	data_ptr = &feature[0];
	do_feature_xml = TRUE ;

	break;
      }

    case XREMOTE_GET_MARK:
      {
	*action = g_quark_from_string(ZACP_GET_MARK) ;
	command = ZACP_GET_MARK ;

	break;
      }

    case XREMOTE_REVCOMP:
      {
	*action = g_quark_from_string(ZACP_REVCOMP) ;
	command = ZACP_REVCOMP ;

	break;
      }

    case XREMOTE_GOODBYE:
      {
	*action  = g_quark_from_string(ZACP_GOODBYE) ;
	command = ZACP_GOODBYE ;

	data_ptr = NULL ;

	break;
      }

    case XREMOTE_SHUTDOWN:
    case XREMOTE_ABORT:
      {
	*action  = g_quark_from_string(ZACP_SHUTDOWN) ;
	command = ZACP_SHUTDOWN ;

	if (callback_action == XREMOTE_ABORT)
	  {
	    data_ptr = &abort[0] ;
	    abort[1].value.q = g_quark_from_string(ZACP_ABORT) ;
	  }
	else
	  {
	    data_ptr = NULL ;
	  }

	break;
      }

    case XREMOTE_CREATE_SIMPLE:
      *action  = g_quark_from_string(ZACP_CREATE_FEATURE) ;
      command = ZACP_CREATE_FEATURE ;

      data_ptr = &feature_simple[0];

      do_feature_xml = TRUE ;
      break;

    case XREMOTE_CREATE:
      *action  = g_quark_from_string(ZACP_CREATE_FEATURE) ;
      command = ZACP_CREATE_FEATURE ;

      data_ptr = &feature[0];

      do_feature_xml = TRUE ;
      break;

    case XREMOTE_REPLACE:
      *action  = g_quark_from_string(ZACP_REPLACE_FEATURE) ;
      command = ZACP_REPLACE_FEATURE ;

      data_ptr = &feature[0];

      do_feature_xml = TRUE ;
      do_replace_xml = TRUE ;
      break;

    case XREMOTE_DELETE:
      *action = g_quark_from_string(ZACP_DELETE_FEATURE) ;
      command = ZACP_DELETE_FEATURE ;

      data_ptr = &feature[0];

      do_feature_xml = TRUE ;
      break;

    case XREMOTE_FIND:
      *action  = g_quark_from_string(ZACP_FIND_FEATURE) ;
      command = ZACP_FIND_FEATURE ;

      data_ptr = &feature[0];

      do_feature_xml = TRUE ;
      break;

    case XREMOTE_ZOOMIN:
      *action  = g_quark_from_string("zoom_in") ;

      data_ptr = NULL;

      do_feature_xml = TRUE ;
      break;
    case XREMOTE_ZOOMOUT:
      *action  = g_quark_from_string("zoom_out") ;

      data_ptr = NULL;

      do_feature_xml = TRUE ;
      break;

    default:
      zMapWarnIfReached();
      break;
    }


  /* Create the xml request and display it in our request window. */
  request_stack = zMapRemoteCommandCreateRequest(app_data->remote_cntl, command, -1,
                                                 ZACP_VIEWID, view_id,
                                                 (char*)NULL) ;


  /* Feature or basic command ? */
  if (do_feature_xml)
    {
      ZMapXMLUtilsEventStack xml ;


      /* set a load of default stuff.... */
      featureset[1].value.q = g_quark_from_string(default_feature_set) ;


      if (callback_action == XREMOTE_CREATE_SIMPLE)
	xml = &feature_simple[0] ;
      else
	xml = &feature[0] ;

      xml[1].value.q = g_quark_from_string(default_feature_name) ;
      xml[2].value.i = default_start ;
      xml[3].value.i = default_end ;
      xml[4].value.q = g_quark_from_string("+") ;

      request_stack = zMapXMLUtilsAddStackToEventsArrayToElement(request_stack,
								 ZACP_REQUEST, 0,
								 NULL, NULL,
								 &featureset[0]) ;

      next_element = ZACP_FEATURESET ;
    }
  else
    {
      next_element = ZACP_REQUEST ;
    }

  /* ok, insert data_ptr at right place. */
  if (data_ptr)
    request_stack = zMapXMLUtilsAddStackToEventsArrayToElement(request_stack,
							       next_element, 0,
							       NULL, NULL,
							       data_ptr) ;


  if (do_replace_xml)
    {
      /* set a load of default stuff.... */
      featureset[1].value.q = g_quark_from_string(default_feature_set) ;

      feature[1].value.q = g_quark_from_string(default_replace_feature_name) ;
      feature[2].value.i = 5000 ;
      feature[3].value.i = 6000 ;
      feature[4].value.q = g_quark_from_string(default_strand) ;

      request_stack = zMapXMLUtilsAddStackToEventsArrayAfterElement(request_stack,
								    ZACP_FEATURESET, 0,
								    ZACP_SEQUENCE_NAME, default_feature_set,
								    &featureset[0]) ;

      request_stack = zMapXMLUtilsAddStackToEventsArrayToElement(request_stack,
								 ZACP_FEATURESET, 2,
								 ZACP_SEQUENCE_NAME, default_feature_set,
								 data_ptr) ;



      next_element = ZACP_FEATURESET ;
    }





  /* Create the string from the xml stack and put it into the text buffer for the user. */
  request = zMapXMLUtilsStack2XML(request_stack, &err_msg, FALSE) ;

  gtk_text_buffer_set_text(app_data->our_req.request_text_buffer, request, -1) ;
  gtk_text_buffer_set_text(app_data->our_req.response_text_buffer, "", -1) ;

  g_free(request) ;

  return ;
}

/* quit from the menu */
static void menuQuitCB(gpointer data, guint callback_action, GtkWidget *w)
{
  quitCB(w, data);

  return ;
}

/* clear the xml messages from all the buffers */
static void clearCB(GtkWidget *button, gpointer user_data)
{
  AppData app_data = (AppData)user_data;

  resetTextAreas(app_data) ;

  return ;
}

/* check the xml in the message input buffer parses as valid xml */
static void parseCB(GtkWidget *button, gpointer user_data)
{
  AppData app_data = (AppData)user_data;
  ZMapXMLParser parser;
  gboolean parse_ok;
  int xml_length = 0;
  char *xml;

  parser = zMapXMLParserCreate(user_data, FALSE, FALSE);

  xml = getXML(app_data, &xml_length);

  if((parse_ok = zMapXMLParserParseBuffer(parser, xml, xml_length)) == TRUE)
    {
      /* message dialog, all ok */
      zMapGUIShowMsg(ZMAP_MSG_INFORMATION, "Info: XML ok");
    }
  else
    {
      char *error;
      /* message dialog, something wrong. position cursor? */
      error = zMapXMLParserLastErrorMsg(parser);
      zMapGUIShowMsg(ZMAP_MSG_WARNING, error);
    }

  zMapXMLParserDestroy(parser);

  return ;
}

/* button -> send command */
static void sendCommandCB(GtkWidget *button, gpointer user_data)
{
  AppData app_data = (AppData)user_data;
  char *request ;
  int req_length ;

  /* User may have erased whole request so just check there is something. */
  if ((request = getXML(app_data, &req_length)) && req_length > 0)
    {
      if (!zMapRemoteControlSendRequest(app_data->remote_cntl, request))
	{
	  zMapCritical("Could not send request to peer program: %s.", request) ;
	}
      else
	{
	  if (app_data->curr_request)
	    g_free(app_data->curr_request) ;

	  app_data->curr_request = g_strdup(request) ;

	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app_data->sending), TRUE) ;
	}
    }

  return ;
}





/* Show a list of all the views created so far. */
static void listViewsCB(GtkWidget *button, gpointer user_data)
{
  AppData app_data = (AppData)user_data ;
  char *view_txt ;

  if (!(app_data->views))
    {
      view_txt = g_strdup("No views currently") ;
    }
  else
    {
      GString *client_list ;

      client_list = g_string_sized_new(512) ;

      g_list_foreach(app_data->views, list_views_cb, client_list) ;

      view_txt = g_string_free(client_list, FALSE) ;
    }

  zMapGUIShowText("Current ZMap Views", view_txt, FALSE) ;

  g_free(view_txt) ;

  return ;
}

static void list_views_cb(gpointer data, gpointer user_data)
{
  GString *string = (GString *)user_data ;

  g_string_append_printf(string, "View id = %s\n", (char *)data) ;

  return ;
}





/* XML event handlers */


static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser)
{
  gboolean result = TRUE ;
  AppData app_data = (AppData)user_data ;
  ZMapXMLAttribute attr ;

  if (result && (attr = zMapXMLElementGetAttributeByName(zmap_element, ZACP_APP_ID)) != NULL)
    {
      app_data->peer_app_name  = g_strdup((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr))) ;
    }
  else
    {
      zMapXMLParserRaiseParsingError(parser, "app_id is a required attribute for the \""ZACP_PEER"\" element.") ;
      app_data->reply_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
      result = FALSE ;
    }

  return result ;
}

static gboolean xml_peer_start_cb(gpointer user_data, ZMapXMLElement peer_element, ZMapXMLParser parser)
{
  gboolean result = TRUE ;
  AppData app_data = (AppData)user_data ;
  ZMapXMLAttribute attr ;

  if (result && (attr  = zMapXMLElementGetAttributeByName(peer_element, ZACP_SOCKET_ID)) != NULL)
    {
      app_data->peer_socket_str = g_strdup((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr))) ;

      app_data->reply_rc = REMOTE_COMMAND_RC_OK ;
    }
  else
    {
      zMapXMLParserRaiseParsingError(parser, "\""ZACP_SOCKET_ID"\" is a required attribute for the \""
				     ZACP_PEER"\" element.") ;
      app_data->reply_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
      result = FALSE ;
    }

  return result ;
}



/* get the xml and its length from the text buffer */
static char *getXML(AppData app_data, int *length_out)
{
  GtkTextBuffer *buffer;
  GtkTextIter start, end;
  char *xml;

  buffer = app_data->our_req.request_text_buffer;

  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  xml = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

  if (length_out)
    *length_out = gtk_text_iter_get_offset(&end);

  return xml;
}


/* Command line arg processing. */
static XRemoteCmdLineArgs getCommandLineArgs(int argc, char *argv[])
{
  XRemoteCmdLineArgs arg_context = NULL ;

  arg_context = g_new0(XRemoteCmdLineArgsStruct, 1) ;

  arg_context->argc = argc ;
  arg_context->argv = argv ;

  arg_context->sequence = "" ;


  if (!makeOptionContext(arg_context))
    {
      appExit(FALSE) ;
    }
  else
    {
      if (!(arg_context->zmap_config_file))
	arg_context->zmap_config_file = g_strdup_printf("%s/.ZMap/ZMap", g_get_home_dir()) ;

      if (!(arg_context->zmap_path))
        arg_context->zmap_path = "./zmap" ;
    }

  return arg_context ;
}

static gboolean makeOptionContext(XRemoteCmdLineArgs arg_context)
{
  GOptionEntry *main_entries;
  gboolean success = FALSE;

  arg_context->opt_context = g_option_context_new(NULL) ;

  g_option_context_set_summary(arg_context->opt_context,
                               "Run zmap executable displaying a control panel for interactive testing.") ;

  g_option_context_set_description(arg_context->opt_context,
                                   "Examples:\n"
                                   " remotecontrol\n"
                                   "  Defaults to using ./zmap as the executable"
                                   " and reads configuration information"
                                   " and the sequence to be displayed from ~/.ZMap/ZMap.\n"
                                   " remotecontrol"
                                   " --no_timeout --sleep=30 --sequence=B0250"
                                   " --start=1 --end=32000 ./some/directory/zmap\n"
                                   "  Never timeout peer program, pass sleep arg to zmap.") ;

  main_entries = get_main_entries(arg_context);

  g_option_context_add_main_entries(arg_context->opt_context, main_entries, NULL);

  if (g_option_context_parse(arg_context->opt_context,
			     &arg_context->argc,
			     &arg_context->argv,
			     &arg_context->error))
    {
      success = TRUE;
    }
  else
    {
      g_print("option parsing failed: %s\n", arg_context->error->message) ;
      success = FALSE ;
    }

  return success;
}

static GOptionEntry *get_main_entries(XRemoteCmdLineArgs arg_context)
{
  static GOptionEntry entries[] = {
    /* long_name, short_name, flags, arg, arg_data, description, arg_description */
    { XREMOTEARG_VERSION, 0, 0, G_OPTION_ARG_NONE,
      NULL, XREMOTEARG_VERSION_DESC, XREMOTEARG_NO_ARG },
    { XREMOTEARG_CONFIG, 0, 0, G_OPTION_ARG_STRING,
      NULL, XREMOTEARG_CONFIG_DESC, XREMOTEARG_FILE_ARG },
    { XREMOTEARG_COMMAND, 0, 0, G_OPTION_ARG_STRING,
      NULL, XREMOTEARG_COMMAND_DESC, XREMOTEARG_FILE_ARG },
    { XREMOTEARG_DEBUGGER, 0, 0, G_OPTION_ARG_NONE, NULL,
      XREMOTEARG_DEBUGGER_DESC, XREMOTEARG_NO_ARG },
    { XREMOTEARG_XREMOTE_DEBUG, 0, 0, G_OPTION_ARG_NONE, NULL,
      XREMOTEARG_XREMOTE_DEBUG_DESC, XREMOTEARG_NO_ARG },
    { XREMOTEARG_CMD_DEBUG, 0, 0, G_OPTION_ARG_NONE, NULL,
      XREMOTEARG_CMD_DEBUG_DESC, XREMOTEARG_NO_ARG },

    { XREMOTEARG_ZMAP_CONFIG, 0, 0, G_OPTION_ARG_STRING, NULL,
      XREMOTEARG_ZMAP_CONFIG, "zmap-config-file" },
    { XREMOTEARG_SEQUENCE, 0, 0, G_OPTION_ARG_STRING, NULL,
      XREMOTEARG_SEQUENCE_DESC, "sequence" },
    { XREMOTEARG_START, 0, 0, G_OPTION_ARG_STRING, NULL,
      XREMOTEARG_START_DESC, "start" },
    { XREMOTEARG_END, 0, 0, G_OPTION_ARG_STRING, NULL,
      XREMOTEARG_END_DESC, "end" },
    { XREMOTEARG_REMOTE_DEBUG, 0, 0, G_OPTION_ARG_STRING, NULL,
      XREMOTEARG_REMOTE_DEBUG_DESC, "remote-debug" },
    { XREMOTEARG_SLEEP, 0, 0, G_OPTION_ARG_STRING, NULL,
      XREMOTEARG_SLEEP_DESC, "sleep" },
    { XREMOTEARG_NO_TIMEOUT, 0, 0, G_OPTION_ARG_NONE, NULL,
      XREMOTEARG_NO_TIMEOUT_DESC, XREMOTEARG_NO_ARG },
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING, NULL,
      NULL, "<zmap executable>" },
    { NULL }
  };


  if (entries[0].arg_data == NULL)
    {
      entries[0].arg_data = &(arg_context->version);
      entries[1].arg_data = &(arg_context->config_file);
      entries[2].arg_data = &(arg_context->command_file);
      entries[3].arg_data = &(arg_context->debugger) ;
      entries[4].arg_data = &(arg_context->xremote_debug) ;
      entries[5].arg_data = &(arg_context->cmd_debug) ;
      entries[6].arg_data = &(arg_context->zmap_config_file);
      entries[7].arg_data = &(arg_context->sequence) ;
      entries[8].arg_data = &(arg_context->start) ;
      entries[9].arg_data = &(arg_context->end) ;
      entries[10].arg_data = &(arg_context->remote_debug) ;
      entries[11].arg_data = &(arg_context->sleep_seconds) ;
      entries[12].arg_data = &(arg_context->timeout) ;
      entries[13].arg_data = &(arg_context->zmap_path) ;
    }

  return entries;
}


static ZMapConfigIniContextKeyEntry get_programs_group_data(char **stanza_name, char **stanza_type)
{
  static ZMapConfigIniContextKeyEntryStruct stanza_keys[] = {
    { XREMOTE_PROG_ZMAP,        G_TYPE_STRING,  NULL, FALSE },
    { XREMOTE_PROG_ZMAP_OPTS,   G_TYPE_STRING,  NULL, FALSE },
    { XREMOTE_PROG_SERVER,      G_TYPE_STRING,  NULL, FALSE },
    { XREMOTE_PROG_SERVER_OPTS, G_TYPE_STRING,  NULL, FALSE },
    { NULL }
  };
  static char *name = XREMOTE_PROG_CONFIG;
  static char *type = XREMOTE_PROG_CONFIG;

  if(stanza_name)
    *stanza_name = name;
  if(stanza_type)
    *stanza_type = type;

  return stanza_keys;
}

static void getZMapConfiguration(AppData app_data)
{
  ZMapConfigIniContext context;


  if ((context = zMapConfigIniContextProvide(app_data->cmd_line_args->zmap_config_file)))
    {
      gboolean got_sequence, got_start, got_end ;
      char *tmp_sequence ;
      int tmp_start = 0 ;
      int tmp_end = 0 ;

      got_sequence = zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                                   ZMAPSTANZA_APP_SEQUENCE, &tmp_sequence) ;


      got_start = zMapConfigIniContextGetInt(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                             ZMAPSTANZA_APP_START, &tmp_start) ;

      got_end = zMapConfigIniContextGetInt(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                           ZMAPSTANZA_APP_END, &tmp_end) ;

      if (got_sequence && got_start && got_end)
        {
          app_data->cmd_line_args->sequence = g_strdup(tmp_sequence) ;
          app_data->cmd_line_args->start = g_strdup_printf("%d", tmp_start) ;
          app_data->cmd_line_args->end = g_strdup_printf("%d", tmp_end) ;
        }

      zMapConfigIniContextDestroy(context) ;
    }

  return ;
}


static ZMapConfigIniContext loadRemotecontrolConfig(AppData app_data)
{
  ZMapConfigIniContext context = NULL;

  if (app_data->cmd_line_args && app_data->cmd_line_args->config_file)
    {
      char *dir  = NULL;
      char *base = NULL;

      dir  = g_path_get_dirname(app_data->cmd_line_args->config_file);
      base = g_path_get_basename(app_data->cmd_line_args->config_file);

      zMapConfigDirCreate(dir, base) ;

      if ((context = zMapConfigIniContextCreate(NULL)))
	{
	  ZMapConfigIniContextKeyEntry stanza_group = NULL;
	  char *stanza_name, *stanza_type;

	  if ((stanza_group = get_programs_group_data(&stanza_name, &stanza_type)))
	    zMapConfigIniContextAddGroup(context, stanza_name, stanza_type, stanza_group) ;
	}

      g_free(base) ;
      g_free(dir) ;
    }

  return context;
}





/* Try to get toggle updates displayed immediately.... */
static void toggleAndUpdate(GtkWidget *toggle_button)
{

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_button), TRUE) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* I can't remember why I disabled this.....perhaps it just doesn't work properly..... */

  while (gtk_events_pending())
    {
      gtk_main_iteration () ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}


