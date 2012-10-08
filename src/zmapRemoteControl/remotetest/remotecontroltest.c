/*  File: remotecontroltest.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2010: Genome Research Ltd.
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
 * HISTORY:
 * Last edited: Apr 11 09:39 2012 (edgrif)
 * Created: Tue Oct 26 15:56:03 2010 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <gtk/gtk.h>
#include <string.h>

#include <ZMap/zmapConfigStrings.h>			    /* For peer-id etc. */
#include <ZMap/zmapCmdLineArgs.h>			    /* For cmd line args. */
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsGUI.h>

#include <ZMap/zmapRemoteControl.h>
#include <ZMap/zmapRemoteCommand.h>


/* Some of these need to go.... */
#include <ZMap/zmapXRemote.h>
#include <ZMap/zmapXML.h>
#include <ZMap/zmapUtilsXRemote.h>
#include <zmapControl/remote/zmapXRemoteAPI.h>


/* appname we want to report. */
#define REMOTECONTROL_APPNAME "remotecontrol"

/* Prefix to our unique atom for remote communication. */
#define REMOTECONTROL_PREFIX "RemoteControl"



/* command line args defines */
#define XREMOTEARG_NO_ARG "<none>"
#define XREMOTEARG_FILE_ARG "file path"

#define XREMOTEARG_VERSION "version"
#define XREMOTEARG_VERSION_DESC "version number"

#define XREMOTEARG_CONFIG "config-file"
#define XREMOTEARG_CONFIG_DESC "file location for config"

#define XREMOTEARG_COMMAND "command-file"
#define XREMOTEARG_COMMAND_DESC "file location for commands"

#define XREMOTEARG_DEBUGGER "in-debugger"
#define XREMOTEARG_DEBUGGER_DESC "Start zmap within debugger (Totalview)."

#define XREMOTEARG_XREMOTE_DEBUG "xremote-debug"
#define XREMOTEARG_XREMOTE_DEBUG_DESC "Display xremote debugging output (default false)."

#define XREMOTEARG_CMD_DEBUG "command-debug"
#define XREMOTEARG_CMD_DEBUG_DESC "Display command debugging output (default true)."


/* Args passed to zmap... */

#define XREMOTEARG_SLEEP "sleep"
#define XREMOTEARG_SLEEP_DESC "Make ZMap sleep for given seconds."

#define XREMOTEARG_SEQUENCE "sequence"
#define XREMOTEARG_SEQUENCE_DESC "Set up xremote with supplied sequence."

#define XREMOTEARG_ZMAP_CONFIG "zmap-config-file"
#define XREMOTEARG_ZMAP_CONFIG_DESC "Specify path for zmaps config file."

#define XREMOTEARG_START "start"
#define XREMOTEARG_START_DESC "Start coord in sequence."

#define XREMOTEARG_END "end"
#define XREMOTEARG_END_DESC "End coord in sequence."

#define XREMOTEARG_REMOTE_DEBUG "remote-debug"
#define XREMOTEARG_REMOTE_DEBUG_DESC "Remote debug level: off | normal | verbose."

#define XREMOTEARG_NO_TIMEOUT "no_timeout"
#define XREMOTEARG_NO_TIMEOUT_DESC "Never timeout waiting for a response."

#define XREMOTEARG_ZMAP_PATH_DESC "zmap executable path."

#define XREMOTEARG_DEFAULT_SEQUENCE "< Enter your sequence here >"


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
  char **zmap_path ;
} XRemoteCmdLineArgsStruct, *XRemoteCmdLineArgs;


/* Holds text widget/buffer for each pair of request/reponse windows. */
typedef struct
{
  GtkWidget *request_text_area ;
  GtkTextBuffer *request_text_buffer ;

  GtkWidget *response_text_area ;
  GtkTextBuffer *response_text_buffer ;
} ReqRespTextStruct, *ReqRespText ;



/* Main application struct. */
typedef struct RemoteDataStructName
{
  XRemoteCmdLineArgs cmd_line_args ;

  GtkWidget *app_toplevel, *vbox, *menu, *buttons ;
  GtkWidget *zmap_path_entry ;
  GtkWidget *sequence_entry, *start_entry, *end_entry, *config_entry ;

  GtkWidget *send_init, *receive_init ;
  GtkWidget *idle, *waiting, *sending, *receiving ;

  gulong mapCB_id ;

  ReqRespTextStruct our_req ;
  ReqRespTextStruct zmap_req ;

  char *app_name ;
  char *unique_atom_str ;
  Atom unique_atom_id ;

  char *peer_app_id ;
  char *peer_unique_id ;

  ZMapRemoteControl remote_cntl ;


  char *curr_request ;

  char *request_id ;

  RemoteCommandRCType reply_rc ;
  ZMapXMLUtilsEventStack reply ;
  char *reply_txt ;

  char *error ;

  ZMapXMLParser parser;
  gpointer parse_cbdata ;

  /* NOT NEEDED ?? */
  char *window_id ;

  /* A straight forward list of views we have created, the list contains the view_ids. */
  GList *views ;


  /* I don't think we will need any of this now..... */
  GHashTable *xremote_clients ;

  gulong register_client_id ;
  gboolean is_register_client ;

  int zmap_pid, server_pid ;

  ZMapConfigIniContext config_context ;

  GQueue *queue ;
} RemoteDataStruct, *RemoteData ;



typedef struct GetPeerStructName
{
  RemoteData remote_data ;
  char *app_id ;
  char *unique_id ;
} GetPeerStruct, *GetPeer ;








/* I THINK MOST OF THIS CAN GO NOW......... */
typedef struct
{
  RemoteData remote_data;
  GHashTable *new_clients;
  char *xml;
  int hash_size;
  char *action;
  gboolean sent;
}SendCommandDataStruct, *SendCommandData;

typedef struct
{
  ZMapXRemoteObj client;
  GList *actions;
  gboolean is_main_window;
}HashEntryStruct, *HashEntry;

typedef struct
{
  GSourceFunc main_runner;
  GSourceFunc runner_done;
  gpointer    runner_data;
  GDestroyNotify destroy;
  gboolean    runnable;
} DependRunnerStruct, *DependRunner;



/* New funcs.... */
static void appExit(gboolean exit_ok) ;
static GtkWidget *makeTestWindow(RemoteData remote_data) ;
static GtkWidget *makeMainReqResp(RemoteData remote_data) ;
static GtkWidget *makeReqRespBox(RemoteData remote_data,
				 GtkBox *parent_box, char *box_title,
				 ReqRespText text_widgs, char *request_title, char *response_title) ;
static void addTextArea(RemoteData remote_data, GtkBox *parent_box, char *text_title,
			GtkWidget **text_area_out, GtkTextBuffer **text_buffer_out) ;
static GtkWidget *makeStateBox(RemoteData remote_data) ;

static void mapCB(GtkWidget *widget, GdkEvent *event, gpointer user_data) ;

static gboolean messageProcess(char *message_xml_in, gboolean full_process,
			       char **action_out, XRemoteMessage *message_out) ;

static void errorCB(ZMapRemoteControl remote_control,
		    ZMapRemoteControlRCType error_type, char *err_msg,
		    void *user_data) ;
static void requestHandlerCB(ZMapRemoteControl remote_control,
			     ZMapRemoteControlReturnReplyFunc remote_reply_func, void *remote_reply_data,
			     char *request, void *user_data) ;
static void replySentCB(void *user_data) ;


static void requestSentCB(void *user_data) ;
static void replyHandlerCB(ZMapRemoteControl remote_control, char *reply, void *user_data) ;

void processRequest(RemoteData remote_data, char *request,
		    RemoteCommandRCType *return_code, char **reason_out,
		    ZMapXMLUtilsEventStack *reply_out, char **reply_txt_out) ;
static char *makeReply(RemoteData remote_data, char *request, char *error_msg,
		       ZMapXMLUtilsEventStack raw_reply, char *reply_txt_out) ;
static char *makeReplyFromText(char *envelope, char *dummy_body, char *reply_txt) ;


static gboolean handleHandshake(char *command_text, RemoteData remote_data) ;


static GArray *addReplyElement(GArray *xml_stack_inout, RemoteCommandRCType remote_rc, char *reply) ;


static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement element, ZMapXMLParser parser) ;
static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement element, ZMapXMLParser parser) ;
static gboolean xml_reply_start_cb(gpointer user_data, ZMapXMLElement element, ZMapXMLParser parser) ;
static gboolean xml_reply_end_cb(gpointer user_data, ZMapXMLElement element, ZMapXMLParser parser) ;
static gboolean xml_peer_start_cb(gpointer user_data, ZMapXMLElement client_element, ZMapXMLParser parser) ;
static gboolean xml_peer_end_cb(gpointer user_data, ZMapXMLElement client_element, ZMapXMLParser parser) ;
static gboolean xml_error_end_cb(gpointer user_data, ZMapXMLElement element, ZMapXMLParser parser) ;



static void sendCommandCB(GtkWidget *button, gpointer user_data);
static void listViewsCB(GtkWidget *button, gpointer user_data);

static void list_views_cb(gpointer data, gpointer user_data) ;




/* 
 *    old funcs....
 */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void installPropertyNotify(GtkWidget *ignored, GdkEvent *event, gpointer user_data) ;
static char *handle_register_client(char *command_text, gpointer user_data, int *statusCode, ZMapXRemoteObj owner);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static GtkWidget *entry_box_widgets(RemoteData remote_data);
static GtkWidget *menubar(GtkWidget *parent, RemoteData remote_data);
static GtkWidget *message_box(RemoteData remote_data);
static GtkWidget *button_bar(RemoteData remote_data);












static int send_command_cb(gpointer key, gpointer hash_data, gpointer user_data);

static int events_to_text_buffer(ZMapXMLWriter writer, char *xml, int len, gpointer user_data);


static void cmdCB( gpointer data, guint callback_action, GtkWidget *w );
static void runZMapCB( gpointer data, guint callback_action, GtkWidget *w );
static void menuQuitCB(gpointer data, guint callback_action, GtkWidget *w);

static void addClientCB(GtkWidget *button, gpointer user_data);
static void quitCB(GtkWidget *button, gpointer user_data);
static void clearCB(GtkWidget *button, gpointer user_data);
static void parseCB(GtkWidget *button, gpointer user_data);

static void toggleAndUpdate(GtkWidget *toggle_button) ;

static gboolean api_zmap_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;
static gboolean api_zmap_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;
static gboolean api_request_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;
static gboolean api_request_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;


static gboolean copy_hash_to_hash(gpointer key, gpointer value, gpointer user_data);
static char *getXML(RemoteData remote_data, int *length_out);
static HashEntry clientToHashEntry(ZMapXRemoteObj client, gboolean is_main);
static gboolean addClientToHash(GHashTable *table, ZMapXRemoteObj client, Window id, GList *actions, gboolean is_main);
static void destroyHashEntry(gpointer entry_data);

static void internal_send_command(SendCommandData send_data);
static gboolean run_command(char **command, int *pid);

static gboolean start_zmap_cb(gpointer remote_data_data);

static GOptionEntry *get_main_entries(XRemoteCmdLineArgs arg_context);
static gboolean makeOptionContext(XRemoteCmdLineArgs arg_context);
static XRemoteCmdLineArgs process_command_line_args(int argc, char *argv[]);
static void process_command_file(RemoteData remote_data, char *command_file);
static ZMapConfigIniContext get_configuration(RemoteData remote_data);
static ZMapConfigIniContextKeyEntry get_programs_group_data(char **stanza_name, char **stanza_type);



/* Testbed for ZMapXRemoteAPI */
/* ...Roys remark....BUT I'M NOT SURE IF THIS IS ACTUALLY USED ANYWHERE..... */

typedef struct
{
  XRemoteMessage message_out;
  ZMapXMLParser xml_parser;
  char *xml_message;
  unsigned int arrest_processing;
  unsigned int full_processing;
  unsigned int xml_length;
} APIProcessingStruct, *APIProcessing;





static gboolean command_debug_G = TRUE ;


static GtkItemFactoryEntry menu_items_G[] =
  {
    {"/_File",                   NULL,         NULL,       0,                  "<Branch>", NULL},
    {"/File/Read",               NULL,         NULL,       0,                  NULL,       NULL},
    {"/File/Quit",               "<control>Q", menuQuitCB, 0,                  NULL,       NULL},
    {"/_ZMap Control",               NULL,         NULL,  0,             "<Branch>", NULL},
    {"/_ZMap Control/New ZMap",  NULL,         runZMapCB,  0,             NULL, NULL},
    {"/_Commands",               NULL,         NULL,       0,                  "<Branch>", NULL},
    {"/Commands/ping",           NULL,         cmdCB,      XREMOTE_PING,       NULL,       NULL},

    {"/Commands/new_view",       NULL,         cmdCB,      XREMOTE_NEW_VIEW,   NULL,       NULL},
    {"/Commands/add_to_view",    NULL,         cmdCB,      XREMOTE_ADD_VIEW,   NULL,       NULL},
    {"/Commands/close_view",     NULL,         cmdCB,      XREMOTE_CLOSE_VIEW, NULL,       NULL},

    {"/Commands/load_features",  NULL,         cmdCB,      XREMOTE_LOAD_FEATURES, NULL,       NULL},
    {"/Commands/zoom_to",        NULL,         cmdCB,      XREMOTE_ZOOM_TO,    NULL,       NULL},
    {"/Commands/get_mark",       NULL,         cmdCB,      XREMOTE_GET_MARK,   NULL,       NULL},
    {"/Commands/rev_comp",       NULL,         cmdCB,      XREMOTE_REVCOMP,   NULL,       NULL},

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


static ZMapXMLObjTagFunctionsStruct api_message_starts_G[] =
  {
    {"zmap", api_zmap_start_cb },
    {"request", api_request_start_cb },
    { NULL,  NULL }
  };

static ZMapXMLObjTagFunctionsStruct api_message_ends_G[] =
  {
    { "zmap", api_zmap_end_cb },
    { "request", api_request_end_cb },
    { NULL,   NULL }
  };


/* Gets used in debug messages embedded in the xremote lib. */
char *ZMAP_X_PROGRAM_G  = "xremote_gui" ;



static gboolean debug_G = TRUE ;




/* 
 *              External functions.
 */


int main(int argc, char *argv[])
{
  RemoteData remote_data ;
  XRemoteCmdLineArgs cmd_args ;


  gtk_init(&argc, &argv) ;				    /* gtk removes it's args. */

  cmd_args = process_command_line_args(argc, argv) ;

  /* Make the main command block. */
  remote_data = g_new0(RemoteDataStruct, 1) ;

  remote_data->xremote_clients = g_hash_table_new_full(NULL, NULL, NULL, destroyHashEntry) ;
  remote_data->cmd_line_args   = cmd_args ;
  remote_data->config_context  = get_configuration(remote_data) ;

  /* Make the test program control window. */
  remote_data->app_toplevel = makeTestWindow(remote_data) ;

  /* Set up remote communication with zmap. */
  remote_data->app_name = g_path_get_basename(argv[0]) ;
  remote_data->unique_atom_str = zMapMakeUniqueID(REMOTECONTROL_APPNAME) ;

  gtk_widget_show_all(remote_data->app_toplevel) ;

  gtk_main() ;

  appExit(TRUE) ;

  return EXIT_FAILURE ;					    /* we shouldn't ever get here. */
}






/*
 *                Internal functions.
 */


/* called by main to do the creation for the rest of the app and run gtk_main() */
static GtkWidget *makeTestWindow(RemoteData remote_data)
{
  GtkWidget *toplevel = NULL ;
  GtkWidget *top_hbox, *top_vbox, *menu_bar, *buttons, *entry_box, *state_box ;

  toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;

  /* Only after map-event are we guaranteed that there's a window for us to work with. */
  remote_data->mapCB_id = g_signal_connect(G_OBJECT(toplevel), "map-event",
					   G_CALLBACK(mapCB), remote_data) ;
  g_signal_connect(G_OBJECT(toplevel), "destroy",
		   G_CALLBACK(quitCB), (gpointer)remote_data) ;

  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE);
  gtk_window_set_title(GTK_WINDOW(toplevel), "ZMap XRemote Test Suite");
  gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;


  /* Top level vbox for menu bar, request/response windows etc. */
  remote_data->vbox = top_vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(toplevel), top_vbox);


  /* Make the menu bar. */
  remote_data->menu = menu_bar = menubar(toplevel, remote_data) ;
  gtk_box_pack_start(GTK_BOX(top_vbox), menu_bar, FALSE, TRUE, 5) ;


  /* Make the buttons. */
  remote_data->buttons = buttons = button_bar(remote_data) ;
  gtk_box_pack_start(GTK_BOX(top_vbox), buttons, TRUE, TRUE, 5) ;


  /* Make the sub text entry boxes. */
  entry_box = entry_box_widgets(remote_data) ;
  gtk_box_pack_start(GTK_BOX(top_vbox), entry_box, TRUE, TRUE, 5);


  /* Make the State box. */
  state_box = makeStateBox(remote_data) ;
  gtk_box_pack_start(GTK_BOX(top_vbox), state_box, TRUE, TRUE, 5);


  /* Main request/response display */
  top_hbox = makeMainReqResp(remote_data) ;
  gtk_box_pack_start(GTK_BOX(top_vbox), top_hbox, TRUE, TRUE, 5);


  return toplevel ;
}


/* Make all the request/response display windows. */
static GtkWidget *makeMainReqResp(RemoteData remote_data)
{
  GtkWidget *top_hbox = NULL ;
  GtkWidget *send_vbox, *receive_vbox ;

  top_hbox = gtk_hbox_new(FALSE, 10) ;

  /* Build Send command boxes. */
  send_vbox = GTK_WIDGET(makeReqRespBox(remote_data,
					GTK_BOX(top_hbox), "SEND TO ZMAP",
					&(remote_data->our_req), "Our Request", "ZMap's Response")) ;

  /* Build Receive command boxes. */
  receive_vbox = GTK_WIDGET(makeReqRespBox(remote_data,
					   GTK_BOX(top_hbox), "RECEIVE FROM ZMAP",
					   &(remote_data->zmap_req), "ZMap's Request", "Our Response")) ;

  return top_hbox ;
}


/* Make a pair of request/response windows. */
static GtkWidget *makeReqRespBox(RemoteData remote_data,
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

  addTextArea(remote_data, GTK_BOX(vbox), request_title,
	      &(text_widgs->request_text_area), &(text_widgs->request_text_buffer)) ;

  /* move up several levels ?? */
  gtk_text_buffer_set_text(text_widgs->request_text_buffer, "Enter xml here...", -1) ;

  addTextArea(remote_data, GTK_BOX(vbox), response_title,
	      &(text_widgs->response_text_area), &(text_widgs->response_text_buffer)) ;

  gtk_text_view_set_editable(GTK_TEXT_VIEW(text_widgs->response_text_area), FALSE) ;
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_widgs->response_text_area), FALSE) ;

  return vbox ;
}


static void addTextArea(RemoteData remote_data, GtkBox *parent_box, char *text_title,
			GtkWidget **text_area_out, GtkTextBuffer **text_buffer_out)
{
  GtkWidget *our_vbox, *frame, *scrwin, *textarea ;

  our_vbox = GTK_WIDGET(gtk_vbox_new(FALSE, 0)) ;
  gtk_box_pack_start(parent_box, our_vbox, TRUE, TRUE, 5) ;

  frame = gtk_frame_new(text_title) ;

  scrwin = gtk_scrolled_window_new(NULL, NULL) ;

  *text_area_out = textarea = message_box(remote_data) ;
  *text_buffer_out = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textarea)) ;

  gtk_container_add(GTK_CONTAINER(scrwin), textarea) ;

  gtk_container_add(GTK_CONTAINER(frame), scrwin) ;

  gtk_box_pack_start(GTK_BOX(our_vbox), frame, TRUE, TRUE, 5);

  return ;
}



/* create the entry box widgets */
static GtkWidget *entry_box_widgets(RemoteData remote_data)
{
  GtkWidget *frame ;
  GtkWidget *entry_box, *sequence, *label, *path;

  frame = gtk_frame_new("Set Parameters") ;

  entry_box = gtk_hbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), entry_box) ;

  label = gtk_label_new("sequence :");
  gtk_box_pack_start(GTK_BOX(entry_box), label, FALSE, FALSE, 5);
  remote_data->sequence_entry = sequence = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(entry_box), sequence, FALSE, FALSE, 5);
  gtk_entry_set_text(GTK_ENTRY(sequence), remote_data->cmd_line_args->sequence);

  label = gtk_label_new("start :");
  gtk_box_pack_start(GTK_BOX(entry_box), label, FALSE, FALSE, 5) ;
  remote_data->start_entry = sequence = gtk_entry_new() ;
  gtk_box_pack_start(GTK_BOX(entry_box), sequence, FALSE, FALSE, 5) ;
  if (remote_data->cmd_line_args->start)
    gtk_entry_set_text(GTK_ENTRY(sequence), remote_data->cmd_line_args->start) ;

  label = gtk_label_new("start :");
  gtk_box_pack_start(GTK_BOX(entry_box), label, FALSE, FALSE, 5);
  remote_data->end_entry = sequence = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(entry_box), sequence, FALSE, FALSE, 5);
  if (remote_data->cmd_line_args->end)
    gtk_entry_set_text(GTK_ENTRY(sequence), remote_data->cmd_line_args->end) ;

  label = gtk_label_new("config file :");
  gtk_box_pack_start(GTK_BOX(entry_box), label, FALSE, FALSE, 5);
  remote_data->config_entry  = sequence = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(entry_box), sequence, FALSE, FALSE, 5);
  if (remote_data->cmd_line_args->zmap_config_file)
    gtk_entry_set_text(GTK_ENTRY(sequence), remote_data->cmd_line_args->zmap_config_file) ;

  label = gtk_label_new("zmap path :");
  gtk_box_pack_start(GTK_BOX(entry_box), label, FALSE, FALSE, 5);
  remote_data->zmap_path_entry = path = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(entry_box), path, FALSE, FALSE, 5);

  return frame ;
}

/* create the menu bar */
static GtkWidget *menubar(GtkWidget *parent, RemoteData remote_data)
{
  GtkWidget *menubar;
  GtkItemFactory *factory;
  GtkAccelGroup *accel;
  gint nmenu_items = sizeof (menu_items_G) / sizeof (menu_items_G[0]);

  accel = gtk_accel_group_new();

  factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel);

  gtk_item_factory_create_items(factory, nmenu_items, menu_items_G, remote_data);

  gtk_window_add_accel_group(GTK_WINDOW(parent), accel);

  menubar = gtk_item_factory_get_widget(factory, "<main>");

  return menubar;
}

/* the message box where xml is entered */
static GtkWidget *message_box(RemoteData remote_data)
{
  GtkWidget *message_box;

  message_box = gtk_text_view_new() ;
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(message_box), GTK_WRAP_WORD) ;

  gtk_widget_set_size_request(message_box, -1, 200) ;

  return message_box;
}

/* All the buttons of the button bar */
static GtkWidget *button_bar(RemoteData remote_data)
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
                   G_CALLBACK(sendCommandCB), remote_data);

  g_signal_connect(G_OBJECT(list_views), "clicked",
                   G_CALLBACK(listViewsCB), remote_data);

  g_signal_connect(G_OBJECT(clear), "clicked",
                   G_CALLBACK(clearCB), remote_data);

  g_signal_connect(G_OBJECT(parse), "clicked",
                   G_CALLBACK(parseCB), remote_data);

  return button_bar;
}


/* Make a set of radio buttons which show current state of remote control. */
static GtkWidget *makeStateBox(RemoteData remote_data)
{
  GtkWidget *frame, *hbox, *init_box, *state_box, *button = NULL ;

  hbox = gtk_hbox_new(FALSE, 0);

  frame = gtk_frame_new("RemoteControl Initialisation") ;
  gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 5) ;

  init_box = gtk_hbutton_box_new() ;
  gtk_container_add(GTK_CONTAINER(frame), init_box) ;

  remote_data->send_init = button = gtk_check_button_new_with_label("Send Interface initialised") ;
  gtk_box_pack_start(GTK_BOX(init_box), button, TRUE, TRUE, 5) ;
  remote_data->receive_init = button = gtk_check_button_new_with_label("Receive Interface initialised") ;
  gtk_box_pack_start(GTK_BOX(init_box), button, TRUE, TRUE, 5) ;


  frame = gtk_frame_new("RemoteControl State") ;
  gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 5) ;

  state_box = gtk_hbutton_box_new() ;
  gtk_container_add(GTK_CONTAINER(frame), state_box) ;

  button = NULL ;
  remote_data->idle = button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(button), "Idle") ;
  gtk_box_pack_start(GTK_BOX(state_box), button, TRUE, TRUE, 5) ;
  remote_data->waiting = button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(button), "Waiting") ;
  gtk_box_pack_start(GTK_BOX(state_box), button, TRUE, TRUE, 5) ;
  remote_data->sending = button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(button), "Sending") ;
  gtk_box_pack_start(GTK_BOX(state_box), button, TRUE, TRUE, 5) ;
  remote_data->receiving = button = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(button), "Receiving") ;
  gtk_box_pack_start(GTK_BOX(state_box), button, TRUE, TRUE, 5) ;

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remote_data->idle), TRUE) ;


  return hbox ;
}



/* ---------------- */
/* Widget callbacks */
/* ---------------- */

/* time to clean up */
static void quitCB(GtkWidget *unused_button, gpointer user_data)
{
  RemoteData remote_data = (RemoteData)user_data;

  
  zMapRemoteControlDestroy(remote_data->remote_cntl) ;


  if(remote_data->zmap_pid != 0)
    {
      kill(remote_data->zmap_pid, SIGKILL);
      remote_data->zmap_pid = 0;
    }

  if(remote_data->server_pid != 0)
    {
      kill(remote_data->server_pid, SIGKILL);
      remote_data->server_pid = 0;
    }

  g_hash_table_destroy(remote_data->xremote_clients);

  g_free(remote_data->window_id) ;

  g_free(remote_data);

  appExit(TRUE) ;

  return ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* THE OLD CODE..... */

/* install the property notify, receives the requests from zmap, when started with --win_id option */
static void installPropertyNotify(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  RemoteData remote_data = (RemoteData)user_data ;

  zMapXRemoteInitialiseWidget(widget, "xremote_gui_test",
                              "_CLIENT_REQUEST_NAME", "_CLIENT_RESPONSE_NAME",
                              handle_register_client, remote_data);
  externalPerl = TRUE;

  if (remote_data->cmd_line_args && remote_data->cmd_line_args->command_file)
    {
      process_command_file(remote_data, remote_data->cmd_line_args->command_file);
    }

  remote_data->window_id = g_strdup_printf("0x%lx", GDK_DRAWABLE_XID(remote_data->app_toplevel->window)) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Called when main window mapped, this is where we init our remote control object. */
static void mapCB(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  RemoteData remote_data = (RemoteData)user_data ;
  gboolean result = TRUE ;

  /* Stop us being called multiple times. */
  g_signal_handler_disconnect(remote_data->app_toplevel, remote_data->mapCB_id) ;
  remote_data->mapCB_id = 0 ;

  if (result)
    result = ((remote_data->remote_cntl = zMapRemoteControlCreate(REMOTECONTROL_APPNAME,
								  errorCB, remote_data,
								  NULL, NULL)) != NULL) ;

  if (result)
    {
      if ((result = zMapRemoteControlReceiveInit(remote_data->remote_cntl,
						 remote_data->unique_atom_str,
						 requestHandlerCB, remote_data,
						 replySentCB, remote_data)))
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remote_data->receive_init), TRUE) ;
    }

  if (result)
    {
      if ((result = zMapRemoteControlReceiveWaitForRequest(remote_data->remote_cntl)))

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remote_data->waiting), TRUE) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      toggleAndUpdate(remote_data->waiting) ;

    }

  if (!result)
    {
      zMapCrash("%s", "Could not initialise remote control, exiting") ;
      
      appExit(FALSE) ;
    }

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
  RemoteData remote_data = (RemoteData)user_data ;
  RemoteValidateRCType result = REMOTE_VALIDATE_RC_OK ;
  RemoteCommandRCType request_rc ;
  ZMapXMLUtilsEventStack reply = NULL ;
  char *reply_txt = NULL ;
  char *full_reply = NULL ;
  char *error_msg = NULL ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  char *test_data = "Test reply from remotetest." ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  zMapDebugPrint(debug_G, "%s", "Enter...") ;

  /* Set state, we are receiving now... */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remote_data->receiving), TRUE) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      toggleAndUpdate(remote_data->receiving) ;


  /* Set their request in the window and blank reply window.... */
  gtk_text_buffer_set_text(remote_data->zmap_req.request_text_buffer, request, -1) ;
  gtk_text_buffer_set_text(remote_data->zmap_req.response_text_buffer, "", -1) ;


  /* Sort out error handling here....poor..... */
  if ((result = zMapRemoteCommandValidateRequest(remote_control, request, &error_msg))
      == REMOTE_VALIDATE_RC_OK)
    {
      processRequest(remote_data, request, &request_rc, &error_msg, &reply, &reply_txt) ;

      /* Format reply..... */
      full_reply = makeReply(remote_data, request, error_msg, reply, reply_txt) ;

      /* Set our reply in the window. */
      gtk_text_buffer_set_text(remote_data->zmap_req.response_text_buffer, full_reply, -1) ;
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
  RemoteData remote_data = (RemoteData)user_data ;
  static int req_count = 0 ;


  toggleAndUpdate(remote_data->idle) ;

  /* Need to return to waiting here..... */
  if (!zMapRemoteControlReceiveWaitForRequest(remote_data->remote_cntl))
    zMapCritical("%s", "Cannot set remote controller to wait for requests.") ;
  else
    toggleAndUpdate(remote_data->waiting) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (!(req_count % 2))
    {
      /* try sending another command from here... */
      cmdCB(remote_data, XREMOTE_PING, NULL) ;

      sendCommandCB(NULL, remote_data) ;

      req_count++ ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  return ;
}



/* 
 *             Functions handling requests sent to zmap
 */


/* Called by our remotecontrol when it receives acknowledgement from zmap
 * that it has our request. */
static void requestSentCB(void *user_data)
{
  RemoteData remote_data = (RemoteData)user_data ;

  return ;
}


/* Called by our remotecontrol to process the reply it gets from zmap. */
static void replyHandlerCB(ZMapRemoteControl remote_control, char *reply, void *user_data)
{
  RemoteData remote_data = (RemoteData)user_data ;
  char *command = NULL ;
  RemoteCommandRCType command_rc ;
  char *reason = NULL ;
  char *reply_body = NULL ;
  char *error_out = NULL ;

  zMapDebugPrint(debug_G, "%s", "Enter...") ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* signal zmap that we've got the reply. */
  (remote_reply_func)(remote_reply_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


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
      gtk_text_buffer_set_text(remote_data->our_req.response_text_buffer, err_msg, -1) ;
      g_free(err_msg) ;
    }
  else if (command_rc != REMOTE_COMMAND_RC_OK)
    {
      gtk_text_buffer_set_text(remote_data->our_req.response_text_buffer, reply, -1) ;

      zMapWarning("Command \"%s\" failed with return code \"%s\" and reason \"%s\".",
		  command, zMapRemoteCommandRC2Str(command_rc), reason) ;
    }
  else
    {
      /* Display and process the reply. */
      char *attribute_value = NULL ;
      char *err_msg = NULL ;

      gtk_text_buffer_set_text(remote_data->our_req.response_text_buffer, reply, -1) ;

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

	      remote_data->views = g_list_append(remote_data->views, (char *)g_quark_to_string(view_id)) ;
	    }
	}
      else if (strcmp(command, ZACP_CLOSEVIEW) == 0)
	{
	  char *attribute_value = NULL ;
	  char *err_msg = NULL ;

	  /* Closed a view so remove it from our list. */
	  if (!zMapRemoteCommandGetAttribute(remote_data->curr_request,
					     ZACP_REQUEST, ZACP_VIEWID, &attribute_value, &err_msg))
	    {
	      zMapCritical("Could not find \"%s\" in current request \"%s\".",
			   ZACP_VIEW, remote_data->curr_request) ;
	    }
	  else
	    {
	      GQuark view_id ;

	      view_id = g_quark_from_string(attribute_value) ;

	      remote_data->views = g_list_remove(remote_data->views, g_quark_to_string(view_id)) ;
	    }
	}
      else if (strcmp(command, ZACP_SHUTDOWN) == 0)
	{
	  g_list_free(remote_data->views) ;
	  remote_data->views = NULL ;
	}
    }



  /* Clear our copy of current_request, important as user may edit request directly which means
   * we are out of sync.... */
  if (remote_data->curr_request)
    {
      g_free(remote_data->curr_request) ;

      remote_data->curr_request = NULL ;
    }


  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remote_data->idle), TRUE) ;



  /* Need to return to waiting here..... */
  if (!zMapRemoteControlReceiveWaitForRequest(remote_data->remote_cntl))
    zMapCritical("%s", "Cannot set remote controller to wait for requests.") ;
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remote_data->waiting), TRUE) ;


  zMapDebugPrint(debug_G, "%s", "Exit...") ;

  return ;
}


static void errorCB(ZMapRemoteControl remote_control,
		    ZMapRemoteControlRCType error_type, char *err_msg,
		    void *user_data)
{
  RemoteData remote_data = (RemoteData)user_data ;

  zMapDebugPrint(debug_G, "%s", "Enter...") ;


  /* NOTE QUITE SURE WE ARE GOING BACK TO WAITING HERE.......... */
  if (!zMapRemoteControlReceiveWaitForRequest(remote_data->remote_cntl))
    zMapWarning("%s", "Call to wait for peer requests failed, cannot communicate with peer.") ;
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remote_data->waiting), TRUE) ;


  zMapDebugPrint(debug_G, "%s", "Exit...") ;

  return ;
}


/* Processes all requests from zmap. */
void processRequest(RemoteData remote_data, char *request,
		    RemoteCommandRCType *return_code_out, char **reason_out,
		    ZMapXMLUtilsEventStack *reply_out, char **reply_txt_out)
{
  char *action ;
  char *error = NULL ;


  remote_data->error = NULL ;
  remote_data->reply = NULL ;
  remote_data->reply_txt = NULL ;

  /* Find out what action has been requested. */
  if (!(action = zMapRemoteCommandRequestGetCommand(request)))
    {
      remote_data->reply_rc = REMOTE_COMMAND_RC_BAD_XML ;

      remote_data->error = "Cannot find \"command\" in request." ;
    }
  else
    {
      char *attribute_value = NULL ;
      char *err_msg = NULL ;

      remote_data->parser = zMapXMLParserCreate(remote_data, FALSE, FALSE) ;

      if (g_ascii_strcasecmp(action, ZACP_HANDSHAKE) == 0)
	{
	  handleHandshake(request, remote_data) ;
	}
      else if (g_ascii_strcasecmp(action, ZACP_GOODBYE) == 0)
	{
	  remote_data->reply_rc = REMOTE_COMMAND_RC_OK ;

	  remote_data->reply = zMapRemoteCommandMessage2Element("goodbye received, goodbye !") ;
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

	      remote_data->views = g_list_remove(remote_data->views, g_quark_to_string(view_id)) ;
	    }

	  remote_data->reply_rc = REMOTE_COMMAND_RC_OK ;

	  remote_data->reply = zMapRemoteCommandMessage2Element("view deleted...thanks !") ;
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

	      remote_data->views = g_list_append(remote_data->views, (char *)g_quark_to_string(view_id)) ;
	    }

	  remote_data->reply_rc = REMOTE_COMMAND_RC_OK ;

	  remote_data->reply = zMapRemoteCommandMessage2Element("view created...thanks !") ;
	}
      else if (g_ascii_strcasecmp(action, ZACP_SELECT_FEATURE) == 0)
	{
	  remote_data->reply_rc = REMOTE_COMMAND_RC_OK ;

	  remote_data->reply = zMapRemoteCommandMessage2Element("single select received..thanks !") ;
	}
      else if (g_ascii_strcasecmp(action, ZACP_DETAILS_FEATURE) == 0)
	{
	  /* Need to detect that this is a transcript and return long result or return
	   * no data....to test zmap's response.... */
	  if (!(strstr(request, "exon")))
	    {
	      remote_data->reply_rc = REMOTE_COMMAND_RC_FAILED ;

	      remote_data->error = "No feature details available." ;
	    }
	  else
	    {
	      remote_data->reply_rc = REMOTE_COMMAND_RC_OK ;

	      remote_data->reply_txt = g_strdup("  <notebook>"
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
	  remote_data->reply_rc = REMOTE_COMMAND_RC_OK ;

	  remote_data->reply = zMapRemoteCommandMessage2Element("got features loaded...thanks !") ;
	}
      else
	{
	  /* Catch all for commands we haven't supported yet !!! */
	  zMapWarning("Support for command \"%s\" has not been implemented yet !!", request) ;

	  remote_data->reply_rc = REMOTE_COMMAND_RC_CMD_UNKNOWN ;

	  remote_data->error = "Unsupported command !" ;
	}



    }

  *return_code_out = remote_data->reply_rc ;

  if (remote_data->error)
    *reason_out = remote_data->error ;
  else if (remote_data->reply)
    *reply_out = remote_data->reply ;
  else
    *reply_txt_out = remote_data->reply_txt ;

  return ;
}



static gboolean handleHandshake(char *command_text, RemoteData remote_data)
{
  gboolean parse_result = FALSE ;
  ZMapXMLObjTagFunctionsStruct starts[] =
    {
      {"peer",     xml_peer_start_cb },
      {NULL, NULL}
    };
  ZMapXMLObjTagFunctionsStruct ends[] =
    {
      {"peer",    xml_peer_end_cb},
      {NULL, NULL}
    };
  GetPeerStruct peer_data = {NULL} ;

  remote_data->parse_cbdata = &peer_data ;

  zMapXMLParserSetMarkupObjectTagHandlers(remote_data->parser, &starts[0], &ends[0]) ;

  if ((parse_result = zMapXMLParserParseBuffer(remote_data->parser, command_text, strlen(command_text))))
    {
      /* Parse was ok so act on it.... */
      if (remote_data->reply_rc == REMOTE_COMMAND_RC_OK)
	{
	  char *message ;

	  /* Need to init peer interface........ */
	  if (zMapRemoteControlSendInit(remote_data->remote_cntl,
					peer_data.app_id, peer_data.unique_id,
					requestSentCB, remote_data,
					replyHandlerCB, remote_data))
	    {
	      remote_data->peer_app_id = g_strdup(peer_data.app_id) ;
	      remote_data->peer_unique_id = g_strdup(peer_data.unique_id) ;

	      message = g_strdup_printf("Handshake successful with peer \"%s\", id \"%s\".",
					peer_data.app_id, peer_data.unique_id) ;

	      remote_data->reply = zMapRemoteCommandMessage2Element(message) ;

	      remote_data->reply_rc = REMOTE_COMMAND_RC_OK ;

	      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remote_data->send_init), TRUE) ;
	    }
	  else
	    {
	      remote_data->error = g_strdup_printf("Handshake failed for peer \"%s\", id \"%s\".",
						   peer_data.app_id, peer_data.unique_id) ;

	      zMapCritical("%s", remote_data->error) ;

	      remote_data->reply_rc = REMOTE_COMMAND_RC_FAILED ;
	    }
	}
    }

  return parse_result ;
}



static char *makeReply(RemoteData remote_data, char *request, char *error_msg,
		       ZMapXMLUtilsEventStack raw_reply, char *reply_txt)
{
  char *full_reply = NULL ;
  GArray *xml_stack ;
  char *err_msg = NULL ;
  ZMapXMLWriter writer ;

  if (error_msg)
    {
      if ((xml_stack = zMapRemoteCommandCreateReplyFromRequest(remote_data->remote_cntl,
							       request, remote_data->reply_rc,
							       error_msg, NULL, &err_msg)))
	full_reply = zMapXMLUtilsStack2XML(xml_stack, &err_msg, FALSE) ;
    }
  else if (raw_reply)
    {
      if ((xml_stack = zMapRemoteCommandCreateReplyFromRequest(remote_data->remote_cntl,
							       request, remote_data->reply_rc,
							       NULL, raw_reply, &err_msg)))
	full_reply = zMapXMLUtilsStack2XML(xml_stack, &err_msg, FALSE) ;
    }
  else if (reply_txt)
    {
      char *final_reply ;

      if ((xml_stack = zMapRemoteCommandCreateReplyEnvelopeFromRequest(remote_data->remote_cntl,
								       request,
								       remote_data->reply_rc, NULL, NULL,
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



static GArray *addReplyElement(GArray *xml_stack_inout, RemoteCommandRCType remote_rc, char *reply)
{
  GArray *xml_stack = xml_stack_inout ;
  static ZMapXMLUtilsEventStackStruct
    reply_body[] =
    {
      {ZMAPXML_ATTRIBUTE_EVENT,     "result",    ZMAPXML_EVENT_DATA_QUARK,  {0}},
      {ZMAPXML_CHAR_DATA_EVENT,     "",          ZMAPXML_EVENT_DATA_STRING, {0}},
      {ZMAPXML_NULL_EVENT}
    } ;

  /* Fill in peer element attributes. */
  reply_body[0].value.q = g_quark_from_string(zMapRemoteCommandRC2Str(remote_rc)) ;
  reply_body[1].value.s = g_strdup(reply) ;

  xml_stack = zMapXMLUtilsAddStackToEventsArrayAfterElement(xml_stack, "reply", &reply_body[0]) ;

  return xml_stack ;
}



/* Menu commands internal to put xml events into the text buffer as text (xml) */
static int events_to_text_buffer(ZMapXMLWriter writer, char *xml, int len, gpointer user_data)
{
  RemoteData remote_data = (RemoteData)user_data;
  GtkTextIter end;
  GtkTextBuffer *buffer;

  buffer = remote_data->our_req.request_text_buffer;

  gtk_text_buffer_get_end_iter(buffer, &end);

  gtk_text_buffer_insert(buffer, &end, xml, len);

  return len;
}

/* button -> run a zmap */
static void runZMapCB(gpointer user_data, guint callback_action, GtkWidget *w)
{
  RemoteData remote_data = (RemoteData)user_data ;

  start_zmap_cb(user_data) ;

  return ;
}


/* The "Commands" menu callback which creates the request xml. */
static void cmdCB(gpointer data, guint callback_action, GtkWidget *w)
{
  static ZMapXMLUtilsEventStackStruct
    start[] = {{ZMAPXML_START_ELEMENT_EVENT, ZACP_TAG,   ZMAPXML_EVENT_DATA_NONE,  {0}},
	       {0}},
    end[] = {{ZMAPXML_END_ELEMENT_EVENT,   ZACP_TAG,   ZMAPXML_EVENT_DATA_NONE,  {0}},
	     {0}},

    req_start[] = {{ZMAPXML_START_ELEMENT_EVENT, ZACP_REQUEST,   ZMAPXML_EVENT_DATA_NONE,  {0}},
		   {ZMAPXML_ATTRIBUTE_EVENT,     ZACP_CMD, ZMAPXML_EVENT_DATA_QUARK, {0}},
		   {0}},
    req_end[] = {{ZMAPXML_END_ELEMENT_EVENT,   ZACP_REQUEST,   ZMAPXML_EVENT_DATA_NONE,  {0}},
		 {0}},

    align_start[] = {{ZMAPXML_START_ELEMENT_EVENT, "align", ZMAPXML_EVENT_DATA_NONE,  {0}},
		     {ZMAPXML_ATTRIBUTE_EVENT,     "name",  ZMAPXML_EVENT_DATA_QUARK, {0}},
		     {ZMAPXML_END_ELEMENT_EVENT,   "align",   ZMAPXML_EVENT_DATA_NONE,  {0}},
		     {0}},

    block_start[] = {{ZMAPXML_START_ELEMENT_EVENT, "block", ZMAPXML_EVENT_DATA_NONE,  {0}},
		     {ZMAPXML_ATTRIBUTE_EVENT,     "name",  ZMAPXML_EVENT_DATA_QUARK, {0}},
		     {ZMAPXML_END_ELEMENT_EVENT,   "block",   ZMAPXML_EVENT_DATA_NONE,  {0}},
		     {0}},

    featureset[] = {{ZMAPXML_START_ELEMENT_EVENT, "featureset", ZMAPXML_EVENT_DATA_NONE,    {0}},
		    {ZMAPXML_ATTRIBUTE_EVENT,     "name",       ZMAPXML_EVENT_DATA_QUARK,   {0}},
		    {ZMAPXML_END_ELEMENT_EVENT,   "featureset", ZMAPXML_EVENT_DATA_NONE,    {0}},
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

    add_view[] = {{ZMAPXML_START_ELEMENT_EVENT, ZACP_SEQUENCE_TAG,    ZMAPXML_EVENT_DATA_NONE,    {0}},
		 {ZMAPXML_ATTRIBUTE_EVENT,      ZACP_SEQUENCE_NAME,   ZMAPXML_EVENT_DATA_STRING,  {0}},
		 {ZMAPXML_ATTRIBUTE_EVENT,      ZACP_SEQUENCE_START,  ZMAPXML_EVENT_DATA_INTEGER, {0}},
		 {ZMAPXML_ATTRIBUTE_EVENT,      ZACP_SEQUENCE_END,    ZMAPXML_EVENT_DATA_INTEGER, {0}},
		 {ZMAPXML_ATTRIBUTE_EVENT,      ZACP_SEQUENCE_CONFIG, ZMAPXML_EVENT_DATA_STRING,  {0}},
		 {ZMAPXML_END_ELEMENT_EVENT,    ZACP_SEQUENCE_TAG,    ZMAPXML_EVENT_DATA_NONE,    {0}},
		 {0}},

    view_close[] =
      {
	{ZMAPXML_START_ELEMENT_EVENT, ZACP_VIEW,      ZMAPXML_EVENT_DATA_NONE,  {0}},
	{ZMAPXML_ATTRIBUTE_EVENT,     ZACP_VIEWID,    ZMAPXML_EVENT_DATA_QUARK, {0}},
	{ZMAPXML_END_ELEMENT_EVENT,   ZACP_VIEW,      ZMAPXML_EVENT_DATA_NONE,  {0}},
	{ZMAPXML_NULL_EVENT}
      },

    abort[] = {{ZMAPXML_START_ELEMENT_EVENT, ZACP_SHUTDOWN_TAG,   ZMAPXML_EVENT_DATA_NONE,    {0}},
	       {ZMAPXML_ATTRIBUTE_EVENT,     ZACP_SHUTDOWN_TYPE,  ZMAPXML_EVENT_DATA_QUARK,  {0}},
	       {ZMAPXML_END_ELEMENT_EVENT,   ZACP_SHUTDOWN_TAG,   ZMAPXML_EVENT_DATA_NONE,    {0}},
	       {0}},

    client[] = {{ZMAPXML_START_ELEMENT_EVENT, "client",  ZMAPXML_EVENT_DATA_NONE,    {0}},
		{ZMAPXML_ATTRIBUTE_EVENT,     "xwid", ZMAPXML_EVENT_DATA_QUARK,   {0}},
		{ZMAPXML_ATTRIBUTE_EVENT,     "request_atom",    ZMAPXML_EVENT_DATA_QUARK, {0}},
		{ZMAPXML_ATTRIBUTE_EVENT,     "response_atom",      ZMAPXML_EVENT_DATA_QUARK, {0}},
		{ZMAPXML_END_ELEMENT_EVENT,   "client",  ZMAPXML_EVENT_DATA_NONE,    {0}},
		{0}} ;


  RemoteData remote_data = (RemoteData)data;
  ZMapXMLUtilsEventStack data_ptr = NULL ;
  ZMapXMLWriter writer;
  GArray *events ;
  GQuark *action ;
  gboolean do_feature_xml = FALSE ;

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

	    if (g_list_length(remote_data->views) == 1)
	      view_id = (char *)(remote_data->views->data) ;
	    else
	      view_id= "NOT_SET" ;
	  }

	data_ptr = &sequence[0] ;
	sequence[1].value.s = g_strdup((char *)gtk_entry_get_text(GTK_ENTRY(remote_data->sequence_entry))) ;
	sequence[2].value.i = atoi((char *)gtk_entry_get_text(GTK_ENTRY(remote_data->start_entry))) ;
	sequence[3].value.i = atoi((char *)gtk_entry_get_text(GTK_ENTRY(remote_data->end_entry))) ;
	sequence[4].value.s = g_strdup_printf("%sZMap",
					      (char *)gtk_entry_get_text(GTK_ENTRY(remote_data->config_entry))) ;

	break;
      }
    case XREMOTE_CLOSE_VIEW:
      {
	*action = g_quark_from_string(ZACP_CLOSEVIEW) ;
	command = ZACP_CLOSEVIEW ;

	if (g_list_length(remote_data->views) == 1)
	  view_id = (char *)(remote_data->views->data) ;
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
      zMapAssertNotReached();
      break;
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Create the xml for the request. */
  events = zMapXMLUtilsStackToEventsArray(&start[0]);

  events = zMapXMLUtilsAddStackToEventsArrayEnd(events, &req_start[0]);

  if (do_feature_xml)
    {
      events = zMapXMLUtilsAddStackToEventsArrayEnd(events, &align_start[0]);

      events = zMapXMLUtilsAddStackToEventsArrayEnd(events, &block_start[0]);
    }

  if (data_ptr)
    events = zMapXMLUtilsAddStackToEventsArrayEnd(events, data_ptr);

  if (do_feature_xml)
    {
      events = zMapXMLUtilsAddStackToEventsArrayEnd(events, &block_end[0]);

      events = zMapXMLUtilsAddStackToEventsArrayEnd(events, &align_end[0]);
    }

  events = zMapXMLUtilsAddStackToEventsArrayEnd(events, &req_end[0]);

  events = zMapXMLUtilsAddStackToEventsArrayEnd(events, &end[0]);

  if ((writer = zMapXMLWriterCreate(events_to_text_buffer, remote_data)))
    {
      ZMapXMLWriterErrorCode xml_status ;

      gtk_text_buffer_set_text(remote_data->our_req.request_text_buffer, "", -1) ;
      gtk_text_buffer_set_text(remote_data->our_req.response_text_buffer, "", -1) ;

      if ((xml_status = zMapXMLWriterProcessEvents(writer, events)) != ZMAPXMLWRITER_OK)
        zMapGUIShowMsg(ZMAP_MSG_WARNING, zMapXMLWriterErrorMsg(writer));
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* Create the xml request and display it in our request window. */
  request_stack = zMapRemoteCommandCreateRequest(remote_data->remote_cntl, command, view_id, -1) ;

  /* how to do this ??? */
  if (do_feature_xml)
    {
      /* set a load of default stuff.... */
      featureset[1].value.q = g_quark_from_string("history") ;

      feature[1].value.q = g_quark_from_string("eds_feature") ;
      feature[2].value.i = 5000 ;
      feature[3].value.i = 6000 ;
      feature[4].value.q = g_quark_from_string("+") ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Make align/block optional. */
      request_stack = zMapXMLUtilsAddStackToEventsArrayAfterElement(request_stack,
								    ZACP_REQUEST, &align_start[0]) ;
      request_stack = zMapXMLUtilsAddStackToEventsArrayAfterElement(request_stack,
								    ZACP_ALIGN, &block_start[0]) ;
      request_stack = zMapXMLUtilsAddStackToEventsArrayAfterElement(request_stack,
								    ZACP_BLOCK, &featureset[0]) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      request_stack = zMapXMLUtilsAddStackToEventsArrayAfterElement(request_stack,
								    ZACP_REQUEST, &featureset[0]) ;

      next_element = ZACP_FEATURESET ;
    }
  else
    {
      next_element = ZACP_REQUEST ;
    }

  if (data_ptr)
    request_stack = zMapXMLUtilsAddStackToEventsArrayAfterElement(request_stack,
								  next_element, data_ptr) ;

  request = zMapXMLUtilsStack2XML(request_stack, &err_msg, FALSE) ;

  gtk_text_buffer_set_text(remote_data->our_req.request_text_buffer, request, -1) ;
  gtk_text_buffer_set_text(remote_data->our_req.response_text_buffer, "", -1) ;

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
  RemoteData remote_data = (RemoteData)user_data;

  gtk_text_buffer_set_text(remote_data->our_req.request_text_buffer, "", -1);
  gtk_text_buffer_set_text(remote_data->our_req.response_text_buffer, "", -1);

  return ;
}

/* check the xml in the message input buffer parses as valid xml */
static void parseCB(GtkWidget *button, gpointer user_data)
{
  RemoteData remote_data = (RemoteData)user_data;
  ZMapXMLParser parser;
  gboolean parse_ok;
  int xml_length = 0;
  char *xml;

  parser = zMapXMLParserCreate(user_data, FALSE, FALSE);

  xml = getXML(remote_data, &xml_length);

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
  RemoteData remote_data = (RemoteData)user_data;
  char *request ;
  int req_length ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  char *xml;
  int xml_length;
  SendCommandDataStruct send_data = {NULL};

  if ((xml = getXML(remote_data, &xml_length)) && xml_length > 0)
    {
      send_data.remote_data = remote_data;
      send_data.xml   = xml;

      messageProcess(xml, FALSE, &(send_data.action), NULL);

      internal_send_command(&send_data) ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* User may have erased whole request so just check there is something. */
  if ((request = getXML(remote_data, &req_length)) && req_length > 0)
    {
      if (!zMapRemoteControlSendRequest(remote_data->remote_cntl, request))
	{
	  zMapCritical("Could not send request to peer program: %s.", request) ;
	}
      else
	{
	  if (remote_data->curr_request)
	    g_free(remote_data->curr_request) ;

	  remote_data->curr_request = g_strdup(request) ;

	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remote_data->sending), TRUE) ;
	}
    }

  return ;
}





/* Show a list of all the views created so far. */
static void listViewsCB(GtkWidget *button, gpointer user_data)
{
  RemoteData remote_data = (RemoteData)user_data ;
  char *view_txt ;

  if (!(remote_data->views))
    {
      view_txt = g_strdup("No views currently") ;
    }
  else
    {
      GString *client_list ;

      client_list = g_string_sized_new(512) ;

      g_list_foreach(remote_data->views, list_views_cb, client_list) ;

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




/* Because you can't iterate over a hash and alter it we need to have
 * a hash we alter while iterating over the main one, which then
 * alters the main one at the end */

/* Just results in the entry being removed from the hash table, the destroy func
 * does the real work. */
static gboolean remove_cb(gpointer key, gpointer hash_data, gpointer user_data)
{
  return TRUE ;
}

static gint action_lookup_cb(gconstpointer list_data, gconstpointer user_data)
{
  char *action_to_find = (char *)user_data;
  char *action_current = (char *)list_data;
  gint match = -1;

  if(strcmp(action_to_find, action_current) == 0)
    match = 0;

  return match;
}



static gboolean messageProcess(char *message_xml_in, gboolean full_process,
			       char **action_out, XRemoteMessage *message_out)
{
  APIProcessingStruct process_data = { NULL };
  gboolean success = FALSE;

  if (full_process && message_out)
    {
      /* Do further processing */
      process_data.full_processing = full_process;
    }

  /* No point doing anything if there's no where to stick it */
  if (action_out || message_out)
    {
      process_data.xml_parser  = zMapXMLParserCreate(&process_data, FALSE, FALSE);
      process_data.xml_message = message_xml_in;
      process_data.xml_length  = strlen(message_xml_in);

      process_data.message_out = g_new0(XRemoteMessageStruct, 1);

      zMapXMLParserSetMarkupObjectTagHandlers(process_data.xml_parser,
					      api_message_starts_G,
					      api_message_ends_G);

      if((success = zMapXMLParserParseBuffer(process_data.xml_parser,
					     process_data.xml_message,
					     process_data.xml_length)))
	{
	  /* Need to make sure everything is good */

	  if(action_out)
	    *action_out = g_strdup(process_data.message_out->action);

	  if(message_out)
	    *message_out = process_data.message_out;
	  else
	    g_free(process_data.message_out);
	}
      else
	g_free(process_data.message_out);
    }

  return success;
}



static void action_elements_to_list(gpointer list_data, gpointer user_data)
{
  ZMapXMLElement element = (ZMapXMLElement)list_data;
  GList **list_ptr = (GList **)user_data;

  if(list_ptr)
    {
      char *action = NULL;
      if((element->name == g_quark_from_string("action")))
	action = zMapXMLElementStealContent(element);

      if(action != NULL)
	*list_ptr = g_list_append(*list_ptr, action);
    }

  return ;
}



/* ------------------ */
/* XML event handlers */
/* ------------------ */

static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement zmap_element,
                                  ZMapXMLParser parser)
{
  SendCommandData send_data  = (SendCommandData)user_data;
  RemoteData remote_data = send_data->remote_data;
  ZMapXMLAttribute attr = NULL;

  if((attr = zMapXMLElementGetAttributeByName(zmap_element, "action")) != NULL)
    {
      GQuark action = zMapXMLAttributeGetValue(attr);

      if (action == g_quark_from_string("register_client"))
        remote_data->is_register_client = TRUE;
      else
        remote_data->is_register_client = FALSE;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* sort out.... */

      /* When we get this it means the zmap is exitting so clean up the client
       * connection. */
      if (action == g_quark_from_string("finalised"))
	{
	  g_hash_table_foreach_remove(remote_data->xremote_clients, remove_cb, NULL) ;
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  return FALSE;
}

static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement element, ZMapXMLParser parser)
{
  return TRUE;
}


static gboolean xml_request_start_cb(gpointer user_data, ZMapXMLElement element, ZMapXMLParser parser)
{



  return TRUE ;
}


static gboolean xml_request_end_cb(gpointer user_data, ZMapXMLElement element, ZMapXMLParser parser)
{



  return TRUE ;
}

static gboolean xml_reply_start_cb(gpointer user_data, ZMapXMLElement element, ZMapXMLParser parser)
{



  return TRUE ;
}


static gboolean xml_reply_end_cb(gpointer user_data, ZMapXMLElement element, ZMapXMLParser parser)
{



  return TRUE ;
}





static gboolean xml_peer_start_cb(gpointer user_data, ZMapXMLElement peer_element, ZMapXMLParser parser)
{
  gboolean result = TRUE ;
  RemoteData remote_data = (RemoteData)user_data ;
  GetPeer peer_data = (GetPeer)(remote_data->parse_cbdata) ;
  ZMapXMLAttribute attr ;
  char *app_id = NULL, *unique_id = NULL ;

  if (result && (attr = zMapXMLElementGetAttributeByName(peer_element, "app_id")) != NULL)
    {
      app_id  = (char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)) ;
    }
  else
    {
      zMapXMLParserRaiseParsingError(parser, "app_id is a required attribute for the \"peer\" element.") ;
      remote_data->reply_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
      result = FALSE ;
    }

  if (result && (attr  = zMapXMLElementGetAttributeByName(peer_element, "unique_id")) != NULL)
    {
      unique_id = (char *)g_quark_to_string(zMapXMLAttributeGetValue(attr));
    }
  else
    {
      zMapXMLParserRaiseParsingError(parser, "\"unique_id\" is a required attribute for the \"peer\" element.") ;
      remote_data->reply_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
      result = FALSE ;
    }

  if (result)
    {
      peer_data->app_id = g_strdup(app_id) ;
      peer_data->unique_id = g_strdup(unique_id) ;
      remote_data->reply_rc = REMOTE_COMMAND_RC_OK ;
    }

  return result ;
}


static gboolean xml_peer_end_cb(gpointer user_data, ZMapXMLElement peer_element, ZMapXMLParser parser)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  SendCommandData send_data  = (SendCommandData)user_data;
  RemoteData remote_data = send_data->remote_data;
  ZMapXRemoteObj new_client;
  ZMapXMLAttribute attr;
  Window wxid = 0;
  char *xid = NULL, *req = NULL, *resp = NULL;

  if((attr = zMapXMLElementGetAttributeByName(client_element, "xwid")) != NULL)
    {
      xid  = (char *)g_quark_to_string(zMapXMLAttributeGetValue(attr));
      wxid = (Window)(strtoul(xid, (char **)NULL, 16));
    }
  else
    zMapXMLParserRaiseParsingError(parser, "id is a required attribute for client.");

  if((attr  = zMapXMLElementGetAttributeByName(client_element, "request_atom")) != NULL)
    req = (char *)g_quark_to_string(zMapXMLAttributeGetValue(attr));
  if((attr  = zMapXMLElementGetAttributeByName(client_element, "response_atom")) != NULL)
    resp = (char *)g_quark_to_string(zMapXMLAttributeGetValue(attr));

  if(wxid && req && resp && (new_client = zMapXRemoteNew(NULL)))
    {
      GList *actions = NULL;

      zMapXRemoteInitClient(new_client, wxid);
      zMapXRemoteSetRequestAtomName(new_client, req);
      zMapXRemoteSetResponseAtomName(new_client, resp);

      if ((remote_data->cmd_line_args->timeout))
	zMapXRemoteSetTimeout(new_client, 0.0) ;

      /* make actions list */
      g_list_foreach(client_element->children, action_elements_to_list, &actions);

      addClientToHash(remote_data->xremote_clients, new_client, wxid, actions, TRUE);
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return TRUE;
}






static gboolean xml_error_end_cb(gpointer user_data, ZMapXMLElement element,
                                 ZMapXMLParser parser)
{
  return TRUE;
}



/* really send an xremote command */
static gboolean send_command_cb(gpointer key, gpointer hash_data, gpointer user_data)
{
  gboolean dead_client = FALSE;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

  /* DISABLE OLD CODE.... */

  HashEntry entry = (HashEntry)hash_data;
  ZMapXMLParser parser;
  ZMapXRemoteObj client;
  SendCommandData send_data = (SendCommandData)user_data;
  char *full_response, *xml;
  int result, code;
  gboolean parse_ok = FALSE ;
  ZMapXMLObjTagFunctionsStruct starts[] = {
    {"zmap",     xml_zmap_start_cb     },
    {"response", xml_response_start_cb },
    { NULL, NULL}
  };
  ZMapXMLObjTagFunctionsStruct ends[] = {
    {"zmap",     xml_zmap_end_cb  },
    {"client",   xml_client_end_cb },
    {"error",    xml_error_end_cb },
    { NULL, NULL}
  };

  client = entry->client;

  /* Check the client will understand the command... */
  if(send_data->action && g_list_find_custom(entry->actions, send_data->action, action_lookup_cb))				/* find the action in the hash entry list */
    {
      /* send it to the client that understands */
      if((result = zMapXRemoteSendRemoteCommand(client, send_data->xml, &full_response)) == ZMAPXREMOTE_SENDCOMMAND_SUCCEED)
	{
	  send_data->sent = TRUE;

	  if (!zMapXRemoteResponseIsError(client, full_response))
	    {
	      gtk_text_buffer_set_text(send_data->remote_data->our_req.response_text_buffer, full_response, -1) ;

	      parser = zMapXMLParserCreate(send_data, FALSE, FALSE);
	      zMapXMLParserSetMarkupObjectTagHandlers(parser, &starts[0], &ends[0]);

	      zMapXRemoteResponseSplit(client, full_response, &code, &xml);

	      if((parse_ok = zMapXMLParserParseBuffer(parser, xml, strlen(xml))))
		{

		}
	      else
		zMapGUIShowMsg(ZMAP_MSG_WARNING, zMapXMLParserLastErrorMsg(parser));
	    }
	  else
	    {
	      zMapGUIShowMsg(ZMAP_MSG_INFORMATION, full_response);
	    }

	}
      else
	{
	  char *message;
	  message = g_strdup_printf("send command failed, deleting client (0x%x)...", GPOINTER_TO_INT(key));
	  zMapGUIShowMsg(ZMAP_MSG_WARNING, message);
	  g_free(message);
	  message = zMapXRemoteGetResponse(NULL);
	  zMapGUIShowMsg(ZMAP_MSG_WARNING, message);
	  dead_client = TRUE;
	}
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */






  return dead_client;
}






/* handle the hashes so we can correctly iterate and remove/append to the hash of clients... */
static void internal_send_command(SendCommandData send_data)
{
  if (send_data->remote_data && send_data->xml && !send_data->sent)
    {
      send_data->new_clients = g_hash_table_new_full(NULL, NULL, NULL, destroyHashEntry);

      send_data->hash_size   = g_hash_table_size(send_data->remote_data->xremote_clients);

      g_hash_table_foreach_remove(send_data->remote_data->xremote_clients,
				  send_command_cb, send_data);

      g_hash_table_foreach_steal(send_data->new_clients,
				 copy_hash_to_hash,
				 send_data->remote_data->xremote_clients);

      g_hash_table_destroy(send_data->new_clients);
    }

  return ;
}

/* Hash related functions */

static HashEntry clientToHashEntry(ZMapXRemoteObj client, gboolean is_main)
{
  HashEntry entry;

  if((entry = g_new0(HashEntryStruct, 1)))
    {
      entry->client = client;
      entry->is_main_window = is_main;
    }

  return entry;
}

static gboolean addClientToHash(GHashTable *table, ZMapXRemoteObj client, Window id, GList *actions, gboolean is_main)
{
  HashEntry new_entry;
  gboolean inserted = FALSE;

  if(!(g_hash_table_lookup(table, GINT_TO_POINTER(id))))
    {
      new_entry = clientToHashEntry(client, is_main);
      new_entry->actions = actions;
      g_hash_table_insert(table, GINT_TO_POINTER(id), new_entry);

      inserted = TRUE;
    }

  return inserted;
}

static gboolean copy_hash_to_hash(gpointer key, gpointer value, gpointer user_data)
{
  HashEntry hash_entry = (HashEntry)value;
  gboolean   copied_ok = FALSE;
  GHashTable *to = (GHashTable *)user_data;
  Window      id = GPOINTER_TO_INT(key);

  copied_ok = addClientToHash(to, hash_entry->client,
                              id, hash_entry->actions,
			      hash_entry->is_main_window);

  return copied_ok;
}

static void destroyHashEntry(gpointer entry_data)
{
  HashEntry entry = (HashEntry)entry_data;

  zMapXRemoteDestroy(entry->client);
  entry->client = NULL;
  g_free(entry);

  return ;
}


/* simple function to run a command and return its pid. */
static gboolean run_command(char **command, int *pid)
{
  int child_pid, *pid_ptr;
  char **ptr;
  char *cwd = NULL, **envp = NULL; /* inherit from parent */
  GSpawnFlags flags = G_SPAWN_SEARCH_PATH;
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

  if(pid)
    pid_ptr = pid;
  else
    pid_ptr = &child_pid;

  if(!(success = g_spawn_async(cwd, command, envp,
			       flags, pre_exec, user_data,
			       pid_ptr, &error)))
    {
      printf("Errror %s\n", error->message);
    }

  return success;
}

/* get the xml and its length from the text buffer */
static char *getXML(RemoteData remote_data, int *length_out)
{
  GtkTextBuffer *buffer;
  GtkTextIter start, end;
  char *xml;

  buffer = remote_data->our_req.request_text_buffer;

  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  xml = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

  if (length_out)
    *length_out = gtk_text_iter_get_offset(&end);

  return xml;
}


/* command line bits */
static GOptionEntry *get_main_entries(XRemoteCmdLineArgs arg_context)
{
  static GOptionEntry entries[] = {
    /* long_name, short_name, flags, arg, arg_data, description, arg_description */
    { XREMOTEARG_VERSION, 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE,
      NULL, XREMOTEARG_VERSION_DESC, XREMOTEARG_NO_ARG },
    { XREMOTEARG_CONFIG, 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_STRING,
      NULL, XREMOTEARG_CONFIG_DESC, XREMOTEARG_FILE_ARG },
    { XREMOTEARG_COMMAND, 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_STRING,
      NULL, XREMOTEARG_COMMAND_DESC, XREMOTEARG_FILE_ARG },
    { XREMOTEARG_DEBUGGER, 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, NULL,
      XREMOTEARG_DEBUGGER_DESC, XREMOTEARG_NO_ARG },
    { XREMOTEARG_XREMOTE_DEBUG, 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, NULL,
      XREMOTEARG_XREMOTE_DEBUG_DESC, XREMOTEARG_NO_ARG },
    { XREMOTEARG_CMD_DEBUG, 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, NULL,
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
    { XREMOTEARG_NO_TIMEOUT, 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, NULL,
      XREMOTEARG_NO_TIMEOUT_DESC, XREMOTEARG_NO_ARG },
    { G_OPTION_REMAINING, 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_FILENAME_ARRAY, NULL,
      XREMOTEARG_NO_TIMEOUT_DESC, XREMOTEARG_NO_ARG },
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

static gboolean makeOptionContext(XRemoteCmdLineArgs arg_context)
{
  GOptionEntry *main_entries;
  gboolean success = FALSE;

  arg_context->opt_context = g_option_context_new(NULL) ;

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

static XRemoteCmdLineArgs process_command_line_args(int argc, char *argv[])
{
  XRemoteCmdLineArgs arg_context = NULL ;

  arg_context = g_new0(XRemoteCmdLineArgsStruct, 1) ;

  arg_context->argc = argc ;
  arg_context->argv = argv ;

  arg_context->sequence = XREMOTEARG_DEFAULT_SEQUENCE ;

  if (!makeOptionContext(arg_context))
    appExit(FALSE) ;

  return arg_context ;
}


static char **build_command(char *params_as_string)
{
  char **command_out = NULL;
  char **ptr, **split = NULL;
  char *copy, *strip ;
  int i, c = 0;

  copy = g_strdup(params_as_string) ;
  strip = g_strstrip(copy) ;

  split = ptr = g_strsplit(strip, " ", 0);

  while(ptr && *ptr != '\0')
    {
      c++;
      ptr++;
    }

  command_out = g_new0(char *, c + 2);

  for (i = 0; i < c; i++)
    {
      if (split[i] && *(split[i]))
	command_out[i] = split[i];
    }

  g_free(copy) ;

  return command_out ;
}

static GList *read_command_file(RemoteData remote_data, char *file_name)
{
  GIOChannel *io_channel;
  GError *open_error = NULL;
  GList *commands = NULL;
  /* do we
   * a) parse the file using expat.
   * b) parse line by line expecting empty lines to separate commands.
   * c) something else.
   */

  /* lets try (b) */
  if((io_channel = g_io_channel_new_file(file_name, "r", &open_error)))
    {
      GIOStatus status;
      GString *string, *current;
      GError  *io_error = NULL;
      gsize term = 0;

      string  = g_string_sized_new(2000);
      current = g_string_sized_new(2000);

      while((status = g_io_channel_read_line_string(io_channel, string,
						    &term, &io_error)) == G_IO_STATUS_NORMAL)
	{
	  /* separator = __EOC__ (End of Command)*/

	  if(g_ascii_strcasecmp(string->str, "__eoc__\n") == 0)			/* line is a separator */
	    {
	      /* append to list */
	      commands = g_list_append(commands, current);

	      /* clear up current string */
	      current = g_string_sized_new(2000);
	      g_string_truncate(string, 0);
	    }
	  else
	    {
	      /* append to current string */
	      g_string_append(current, string->str);
	      g_string_truncate(string, 0);
	    }
	}

      g_string_free(string, TRUE);

      g_io_channel_shutdown(io_channel, TRUE, &open_error);
    }


  return commands;
}

static void command_file_free_strings(gpointer list_data, gpointer unused_data)
{
  GString *command = (GString *)list_data;
  g_string_free(command, FALSE);
  return ;
}


static gboolean queue_send_command_cb(gpointer user_data)
{
  SendCommandData send_data = (SendCommandData)user_data;
  gboolean sent = TRUE;

  internal_send_command(send_data);

  return sent;
}

static void queue_destroy_send_data_cb(gpointer destroy_data)
{
  SendCommandData send_data = (SendCommandData)destroy_data;

  if(send_data->xml)
    g_free(send_data->xml);

  g_free(send_data);

  return ;
}

static gboolean queue_sent_command_cb(gpointer user_data)
{
  SendCommandData send_data = (SendCommandData)user_data;
  gboolean remove_when_true = FALSE;

  remove_when_true = send_data->sent;

  return remove_when_true;
}

static void command_file_run_command_cb(gpointer list_data, gpointer user_data)
{
  RemoteData remote_data = (RemoteData)user_data;
  GString *command = (GString *)list_data;

  if((command->str))
    {
      SendCommandData send_data = NULL;
      DependRunner depend_data = NULL;

      send_data        = g_new0(SendCommandDataStruct, 1);
      send_data->remote_data = remote_data;
      send_data->xml   = command->str;

      messageProcess(send_data->xml, FALSE, &(send_data->action), NULL);

      depend_data = g_new0(DependRunnerStruct, 1);
      depend_data->main_runner = queue_send_command_cb;
      depend_data->destroy     = queue_destroy_send_data_cb;
      depend_data->runner_data = send_data;
      depend_data->runnable    = TRUE;
      depend_data->runner_done = queue_sent_command_cb;

      g_queue_push_tail(remote_data->queue, depend_data);

    }


  return;
}

static gboolean command_source_cb(gpointer user_data)
{
  RemoteData remote_data = (RemoteData)user_data;
  gboolean remove_when_false = TRUE;

  if(!g_queue_is_empty(remote_data->queue))
    {
      DependRunner depend_data = g_queue_peek_head(remote_data->queue);
      gboolean finished;

      if(!(finished = (depend_data->runner_done)(depend_data->runner_data)))
	{
	  if(depend_data->main_runner && depend_data->runnable)
	    depend_data->runnable = (depend_data->main_runner)(depend_data->runner_data);
	  else
	    finished = TRUE;
	}

      if(finished)
	{
	  depend_data = g_queue_pop_head(remote_data->queue);
	  if(depend_data->destroy)
	    (depend_data->destroy)(depend_data->runner_data);
	  g_free(depend_data);
	}
    }
  else
    remove_when_false = FALSE;

  return remove_when_false;
}

static gboolean start_server_cb(gpointer remote_data_data)
{
  RemoteData remote_data = (RemoteData)remote_data_data;
  char *tmp_string = NULL;

  /* run sgifaceserver */
  if(zMapConfigIniContextGetString(remote_data->config_context, XREMOTE_PROG_CONFIG, XREMOTE_PROG_CONFIG,
				   XREMOTE_PROG_SERVER, &tmp_string))
    {
      char *server_path = tmp_string;
      char **command = NULL;
      tmp_string = NULL;
      char *cmd_string ;

      zMapConfigIniContextGetString(remote_data->config_context, XREMOTE_PROG_CONFIG, XREMOTE_PROG_CONFIG,
				    XREMOTE_PROG_SERVER_OPTS, &tmp_string);

      cmd_string = g_strdup_printf("%s %s", server_path, tmp_string) ;
      command = build_command(cmd_string) ;

      run_command(command, &(remote_data->server_pid)) ;

      g_free(command);
    }

  /* Make sure we only run once */
  return FALSE;
}

static gboolean server_started_cb(gpointer remote_data_data)
{
  RemoteData remote_data = (RemoteData)remote_data_data;
  gboolean remove_when_true = FALSE;

  if(remote_data->server_pid)
    remove_when_true = TRUE;

  return remove_when_true;
}



/* run zmap */
static gboolean start_zmap_cb(gpointer remote_data_data)
{
  RemoteData remote_data = (RemoteData)remote_data_data;
  char *zmap_path  = NULL;
  char *tmp_string = NULL;
  gboolean debugger = remote_data->cmd_line_args->debugger ;


  if ((tmp_string = (char *)gtk_entry_get_text(GTK_ENTRY(remote_data->zmap_path_entry)))
      && (*tmp_string != '\0'))
    {
      zmap_path = tmp_string ;
    }
  else if ((remote_data->config_context)
	   && (zMapConfigIniContextGetString(remote_data->config_context,
					     XREMOTE_PROG_CONFIG, XREMOTE_PROG_CONFIG, XREMOTE_PROG_ZMAP,
					     &tmp_string)))
    {
      zmap_path = tmp_string ;
    }
  else if (remote_data->cmd_line_args->zmap_path)
    {
      zmap_path = *(remote_data->cmd_line_args->zmap_path) ;
    }
  else
    {
      /* Ghastly last resort..... */
      zmap_path = "./zmap";
    }

  if (zmap_path != NULL)
    {
      char **command = NULL ;
      tmp_string = NULL ;
      GString *cmd_str ;
      char *sleep = remote_data->cmd_line_args->sleep_seconds ;
      char *remote_debug = remote_data->cmd_line_args->remote_debug ;
      int screen_num ;


      if (remote_data->config_context != NULL)
	zMapConfigIniContextGetString(remote_data->config_context, XREMOTE_PROG_CONFIG, XREMOTE_PROG_CONFIG,
				      XREMOTE_PROG_ZMAP_OPTS, &tmp_string) ;

      cmd_str = g_string_sized_new(500) ;

      if (debugger)
	g_string_append(cmd_str, "totalview -a") ;

      g_string_append_printf(cmd_str, " %s", zmap_path) ;

      /* Run zmap on same screen as remote tool by default, better for debugging. */
      if ((screen_num = gdk_screen_get_number(gtk_widget_get_screen(remote_data->app_toplevel))) != 0)
	g_string_append_printf(cmd_str, " --screen=%d", screen_num) ;


      if (sleep)
	g_string_append_printf(cmd_str, " --sleep=%s", sleep) ;

      if (remote_debug)
	g_string_append_printf(cmd_str, " --remote-debug=%s", remote_debug) ;

      g_string_append_printf(cmd_str, " --%s=%s",
			     ZMAPSTANZA_APP_PEER_NAME,
			     remote_data->app_name) ;

      g_string_append_printf(cmd_str, " --%s=%s",
			     ZMAPSTANZA_APP_PEER_CLIPBOARD,
			     remote_data->unique_atom_str) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* HACK.... */
      if (remote_data->cmd_line_args->zmap_config_file)
	g_string_append_printf(cmd_str, " --%s=/nfs/users/nfs_e/edgrif/.ZMap/ --%s=ZMap",
			       ZMAPARG_CONFIG_DIR, ZMAPARG_CONFIG_FILE) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      if (remote_data->cmd_line_args->zmap_config_file)
	g_string_append_printf(cmd_str, " --%s=%s", ZMAPARG_CONFIG_DIR,
			       remote_data->cmd_line_args->zmap_config_file) ;



      if (tmp_string)
	g_string_append(cmd_str, tmp_string) ;

      command = build_command(cmd_str->str) ;

      run_command(command, &(remote_data->zmap_pid)) ;

      g_free(command) ;

      g_string_free(cmd_str, TRUE) ;

      g_free(tmp_string) ;
    }

  return FALSE;
}

static gboolean zmap_started_cb(gpointer remote_data_data)
{
  RemoteData remote_data = (RemoteData)remote_data_data;
  gboolean remove_when_true = FALSE;

  if(remote_data->zmap_pid)
    remove_when_true = TRUE;

  return remove_when_true;
}

static gboolean quit_fin_cb(gpointer remote_data_data)
{
  return FALSE;			/* Keep on trying to quit! */
}

static gboolean invoke_quit_cb(gpointer remote_data_data)
{
  RemoteData remote_data = (RemoteData)remote_data_data;

  quitCB(NULL, remote_data);

  return FALSE;
}

static void process_command_file(RemoteData remote_data, char *command_file)
{
  DependRunner depend_data;
  GList *command_list = NULL;

  remote_data->queue = g_queue_new();

  /* Start the server. */
  depend_data = g_new0(DependRunnerStruct, 1);
  depend_data->main_runner = start_server_cb;
  depend_data->runner_data = remote_data;
  depend_data->runner_done = server_started_cb;
  depend_data->runnable    = TRUE;

  g_queue_push_tail(remote_data->queue, depend_data);

  /* Start zmap */
  depend_data = g_new0(DependRunnerStruct, 1);
  depend_data->main_runner = start_zmap_cb;
  depend_data->runner_data = remote_data;
  depend_data->runner_done = zmap_started_cb;
  depend_data->runnable    = TRUE;

  g_queue_push_tail(remote_data->queue, depend_data);

  /* get individual commands (GList *) */
  command_list = read_command_file(remote_data, command_file);
  /* run individual commands, waiting 30 seconds between each command */
  g_list_foreach(command_list, command_file_run_command_cb, remote_data);
  g_list_foreach(command_list, command_file_free_strings, NULL);
  g_list_free(command_list);

  depend_data = g_new0(DependRunnerStruct, 1);
  depend_data->main_runner = invoke_quit_cb;
  depend_data->runner_data = remote_data;
  depend_data->runner_done = quit_fin_cb;
  depend_data->runnable    = TRUE;

  g_queue_push_tail(remote_data->queue, depend_data);

  g_timeout_add(5000, command_source_cb, remote_data);

  return ;
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

static ZMapConfigIniContext get_configuration(RemoteData remote_data)
{
  ZMapConfigIniContext context = NULL;

  if(remote_data->cmd_line_args && remote_data->cmd_line_args->config_file)
    {
      char *dir  = NULL;
      char *base = NULL;

      dir  = g_path_get_dirname(remote_data->cmd_line_args->config_file);
      base = g_path_get_basename(remote_data->cmd_line_args->config_file);

      zMapConfigDirCreate(dir, base, FALSE) ;

      if((context = zMapConfigIniContextCreate(NULL)))
	{
	  ZMapConfigIniContextKeyEntry stanza_group = NULL;
	  char *stanza_name, *stanza_type;

	  if((stanza_group = get_programs_group_data(&stanza_name, &stanza_type)))
	    zMapConfigIniContextAddGroup(context, stanza_name,
					 stanza_type, stanza_group);
	}
      if(dir)
	g_free(dir);
      if(base)
	g_free(base);
    }

  return context;
}




/* THIS STUFF SEEMS NOT TO BE USED...NOT SURE WHAT THE INTENTION WAS.... */


static gboolean api_zmap_start_cb(gpointer user_data, ZMapXMLElement zmap_element,
                                  ZMapXMLParser parser)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  APIProcessing process_data = (APIProcessing)user_data;

  ZMapXMLAttribute attr;

  if((attr = zMapXMLElementGetAttributeByName(zmap_element, "action")) != NULL)
    {
      GQuark action = zMapXMLAttributeGetValue(attr);
      process_data->message_out->action = (char *)g_quark_to_string(action);
    }

  if(!process_data->full_processing)
    zMapXMLParserPauseParsing(parser);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* I DON'T UNDERSTAND WHY WE RETURN FALSE HERE..... */
  return FALSE;
}


static gboolean api_zmap_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser)
{
  return TRUE;
}

static gboolean api_request_start_cb(gpointer user_data, ZMapXMLElement zmap_element,
				     ZMapXMLParser parser)
{
  APIProcessing process_data = (APIProcessing)user_data;
  ZMapXMLAttribute attr;

  if((attr = zMapXMLElementGetAttributeByName(zmap_element, "action")) != NULL)
    {
      GQuark action = zMapXMLAttributeGetValue(attr);
      process_data->message_out->action = (char *)g_quark_to_string(action);
    }

  if(!process_data->full_processing)
    zMapXMLParserPauseParsing(parser);

  return FALSE;
}


static gboolean api_request_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser)
{
  return TRUE;
}




/* Try to get toggle updates displayed immediately.... */
static void toggleAndUpdate(GtkWidget *toggle_button)
{

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_button), TRUE) ;


  while (gtk_events_pending())
    {
      gtk_main_iteration () ;
    }

  return ;
}


