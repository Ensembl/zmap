/*  File: xremote_gui_test.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Aug 14 10:14 2009 (edgrif)
 * Created: Thu Feb 15 11:25:20 2007 (rds)
 * CVS info:   $Id: xremote_gui_test.c,v 1.15 2009-08-14 09:57:56 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <gtk/gtk.h>
#include <string.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsGUI.h>
#include <ZMap/zmapXRemote.h>
#include <ZMap/zmapXML.h>
#include <ZMap/zmapUtilsXRemote.h>
#include <zmapControl/remote/zmapXRemoteAPI.h>


/* command line args defines */
#define XREMOTEARG_NO_ARG "<none>"
#define XREMOTEARG_FILE_ARG "file path"

#define XREMOTEARG_VERSION "version"
#define XREMOTEARG_VERSION_DESC "version number"

#define XREMOTEARG_CONFIG "config-file"
#define XREMOTEARG_CONFIG_DESC "file location for config"

#define XREMOTEARG_COMMAND "command-file"
#define XREMOTEARG_COMMAND_DESC "file location for commands"

/* config file defines */
#define XREMOTE_PROG_CONFIG "programs"

#define XREMOTE_PROG_ZMAP      "zmap-exe"
#define XREMOTE_PROG_ZMAP_OPTS "zmap-options"
#define XREMOTE_PROG_SERVER      "sgifaceserver-exe"
#define XREMOTE_PROG_SERVER_OPTS "sgifaceserver-options"


typedef struct
{
  int argc;
  char **argv;
  GOptionContext *opt_context;
  GError *error;

  /* and the commandline args */
  gboolean version;
  char *config_file;
  char *command_file;
} XRemoteCmdLineArgsStruct, *XRemoteCmdLineArgs;

typedef struct
{
  GtkWidget *app_toplevel, *vbox, *menu, *text_area, 
    *buttons, *sequence, *zmap_path, *client_entry;
  GtkTextBuffer *text_buffer;
  ZMapXRemoteObj xremote_server;
  GHashTable *xremote_clients;
  gulong register_client_id;
  gboolean is_register_client;
  int zmap_pid, server_pid;
  XRemoteCmdLineArgs cmd_line_args;
  ZMapConfigIniContext config_context;
  GQueue *queue;
}XRemoteTestSuiteDataStruct, *XRemoteTestSuiteData;

typedef struct
{
  XRemoteTestSuiteData suite;
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


static int zmapXremoteTestSuite(int argc, char *argv[]);
static void installPropertyNotify(GtkWidget *ignored, XRemoteTestSuiteData suite);
static char *handle_register_client(char *command_text, gpointer user_data, int *statusCode);
static GtkWidget *entry_box_widgets(XRemoteTestSuiteData suite);
static GtkWidget *menubar(XRemoteTestSuiteData suite);
static GtkWidget *message_box(XRemoteTestSuiteData suite);
static GtkWidget *button_bar(XRemoteTestSuiteData suite);

static int send_command_cb(gpointer key, gpointer hash_data, gpointer user_data);

static int events_to_text_buffer(ZMapXMLWriter writer, char *xml, int len, gpointer user_data);

static void cmdCB( gpointer data, guint callback_action, GtkWidget *w );
static void menuQuitCB(gpointer data, guint callback_action, GtkWidget *w);

static void addClientCB(GtkWidget *button, gpointer user_data);
static void quitCB(GtkWidget *button, gpointer user_data);
static void clearCB(GtkWidget *button, gpointer user_data);
static void parseCB(GtkWidget *button, gpointer user_data);
static void sendCommandCB(GtkWidget *button, gpointer user_data);
static void listClientsCB(GtkWidget *button, gpointer user_data);
static void runZMapCB(GtkWidget *button, gpointer user_data);

static gboolean api_zmap_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;
static gboolean api_zmap_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;
static gboolean api_request_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;
static gboolean api_request_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser) ;


static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement element, 
                                  ZMapXMLParser parser);
static gboolean xml_response_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser);
static gboolean xml_client_start_cb(gpointer user_data, ZMapXMLElement client_element,
                                    ZMapXMLParser parser);
static gboolean xml_client_end_cb(gpointer user_data, ZMapXMLElement client_element,
				  ZMapXMLParser parser);
static gboolean xml_error_end_cb(gpointer user_data, ZMapXMLElement element, 
                                 ZMapXMLParser parser);
static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement element, 
                                ZMapXMLParser parser);

static gboolean copy_hash_to_hash(gpointer key, gpointer value, gpointer user_data);
static char *getXML(XRemoteTestSuiteData suite, int *length_out);
static HashEntry clientToHashEntry(ZMapXRemoteObj client, gboolean is_main);
static gboolean addClientToHash(GHashTable *table, ZMapXRemoteObj client, Window id, GList *actions, gboolean is_main);
static void destroyHashEntry(gpointer entry_data);
static void appExit(int exit_rc);

static void internal_send_command(SendCommandData send_data);
static gboolean run_command(char **command, int *pid);

static gboolean start_zmap_cb(gpointer suite_data);

static GOptionEntry *get_main_entries(XRemoteCmdLineArgs arg_context);
static gboolean makeOptionContext(XRemoteCmdLineArgs arg_context);
static XRemoteCmdLineArgs process_command_line_args(int argc, char *argv[]);
static void process_command_file(XRemoteTestSuiteData suite, char *command_file);
static ZMapConfigIniContext get_configuration(XRemoteTestSuiteData suite);
static ZMapConfigIniContextKeyEntry get_programs_group_data(char **stanza_name, char **stanza_type);

enum
  {  
    XREMOTE_NEW_ZMAP,
    XREMOTE_NEW_VIEW,
    XREMOTE_ZOOMIN,
    XREMOTE_ZOOMOUT,
    XREMOTE_CREATE,
    XREMOTE_DELETE,
    XREMOTE_ZOOMTO,
    XREMOTE_SHUTDOWN
  };

static gboolean command_debug_G = TRUE ;
static gboolean debug_all_responses_G = TRUE ;

static GtkItemFactoryEntry menu_items_G[] =
  {
    {"/_File",                   NULL,         NULL,       0,                "<Branch>", NULL},
    {"/File/Read",               NULL,         NULL,       0,                NULL,       NULL},  
    {"/File/Quit",               "<control>Q", menuQuitCB, 0,                NULL,       NULL},  
    {"/_Commands",               NULL,         NULL,       0,                "<Branch>", NULL},
    {"/Commands/new zmap",            NULL,         cmdCB,      XREMOTE_NEW_ZMAP,      NULL,       NULL},
    {"/Commands/new view",            NULL,         cmdCB,      XREMOTE_NEW_VIEW,      NULL,       NULL},
    {"/Commands/Zoom In",        NULL,         cmdCB,      XREMOTE_ZOOMIN,   NULL,       NULL},
    {"/Commands/Zoom Out",       NULL,         cmdCB,      XREMOTE_ZOOMOUT,  NULL,       NULL},
    {"/Commands/Feature Create", NULL,         cmdCB,      XREMOTE_CREATE,   NULL,       NULL},
    {"/Commands/Feature Delete", NULL,         cmdCB,      XREMOTE_DELETE,   NULL,       NULL},
    {"/Commands/Feature Goto",   NULL,         cmdCB,      XREMOTE_ZOOMTO,   NULL,       NULL},
    {"/Commands/Shutdown",       NULL,         cmdCB,      XREMOTE_SHUTDOWN, NULL,       NULL}
  };


static ZMapXMLObjTagFunctionsStruct api_message_starts_G[] = {
  {"zmap", api_zmap_start_cb },
  {"request", api_request_start_cb },
  { NULL,  NULL }
};

static ZMapXMLObjTagFunctionsStruct api_message_ends_G[] = {
  { "zmap", api_zmap_end_cb },
  { "request", api_request_end_cb },
  { NULL,   NULL }
};


char *ZMAP_X_PROGRAM_G  = "xremote_gui" ;



/* main */
int main(int argc, char *argv[])
{
  int main_rc;

  main_rc = zmapXremoteTestSuite(argc, argv);

  return main_rc;
}


/* rest of the code. */

/* called by main to do the creation for the rest of the app and run gtk_main() */
static int zmapXremoteTestSuite(int argc, char *argv[])
{
  XRemoteTestSuiteData suite;
  XRemoteCmdLineArgs cmd_args;
  GtkWidget *toplevel, *vbox, *menu_bar, *buttons, *textarea, *frame, *entry_box;

  gtk_init(&argc, &argv);

  cmd_args = process_command_line_args(argc, argv);

  if ((suite = g_new0(XRemoteTestSuiteDataStruct, 1)))
    {
      suite->xremote_clients = g_hash_table_new_full(NULL, NULL, NULL, destroyHashEntry);
      suite->cmd_line_args   = cmd_args;
      suite->config_context  = get_configuration(suite);

      suite->app_toplevel = toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      suite->vbox         = vbox     = gtk_vbox_new(FALSE, 0);
      suite->menu         = menu_bar = menubar(suite);
      suite->text_area    = textarea = message_box(suite);
      suite->buttons      = buttons  = button_bar(suite);
      frame = gtk_frame_new("Command");

      entry_box = entry_box_widgets(suite);

      g_signal_connect(G_OBJECT(toplevel), "map",
                       G_CALLBACK(installPropertyNotify), suite);

      g_signal_connect(G_OBJECT(toplevel), "destroy", 
                       G_CALLBACK(quitCB), (gpointer)suite) ;

      gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE);
      gtk_window_set_title(GTK_WINDOW(toplevel), "ZMap XRemote Test Suite");
      gtk_container_border_width(GTK_CONTAINER(toplevel), 0);
      
      gtk_container_add(GTK_CONTAINER(toplevel), vbox);
      gtk_container_add(GTK_CONTAINER(frame), textarea);

      gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, TRUE, 5);

      gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 5);

      gtk_box_pack_start(GTK_BOX(vbox), entry_box, TRUE, TRUE, 5);

      gtk_box_pack_start(GTK_BOX(vbox), buttons, TRUE, TRUE, 5);

      gtk_widget_show_all(toplevel);

      gtk_main();

      appExit(EXIT_SUCCESS);
    }

  return EXIT_FAILURE;
}

/* install the property notify, receives the requests from zmap, when started with --win_id option */
static void installPropertyNotify(GtkWidget *widget, XRemoteTestSuiteData suite)
{
  zMapXRemoteInitialiseWidget(widget, "xremote_gui_test", 
                              "_CLIENT_REQUEST_NAME", "_CLIENT_RESPONSE_NAME", 
                              handle_register_client, suite);
  externalPerl = TRUE;

  if(suite->cmd_line_args && 
     suite->cmd_line_args->command_file)
    {
      process_command_file(suite, suite->cmd_line_args->command_file);
    }
  
  return ;
}

/* The property notify, receives the requests from zmap, when started with --win_id option */
static char *handle_register_client(char *command_text, gpointer user_data, int *statusCode)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)user_data;
  ZMapXMLObjTagFunctionsStruct starts[] = {
    {"zmap",     xml_zmap_start_cb   },
    {"client",   xml_client_start_cb },
    { NULL, NULL}
  };
  ZMapXMLObjTagFunctionsStruct ends[] = {
    {"zmap",   xml_zmap_end_cb  },
    {"client", xml_client_end_cb },
    { NULL, NULL}
  };
  SendCommandDataStruct parser_data = {NULL};
  ZMapXMLParser parser;
  gboolean parse_ok;
  char *reply;
  int status = 500;

  parser_data.suite = suite;

  parser = zMapXMLParserCreate(&parser_data, FALSE, FALSE);

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &starts[0], &ends[0]);

  reply = g_strdup_printf("%s", "** no reply text **");

  if((parse_ok = zMapXMLParserParseBuffer(parser, command_text, strlen(command_text))))
    {
      status = 200;
    }
  else
    status = 403;

  if(statusCode)
    *statusCode = status;


  return reply ;
}

/* create the entry box widgets */
static GtkWidget *entry_box_widgets(XRemoteTestSuiteData suite)
{
  GtkWidget *entry_box, *sequence, *label, *path;

  entry_box = gtk_hbox_new(FALSE, 0);

  label = gtk_label_new("zmap path :");
  suite->zmap_path = path = gtk_entry_new();

  gtk_box_pack_start(GTK_BOX(entry_box), label, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(entry_box), path, FALSE, FALSE, 5);

  label = gtk_label_new("sequence :");
  suite->sequence = sequence = gtk_entry_new();

  /* Set the default text. The current zmap_test_suite.sh sequence */
  gtk_entry_set_text(GTK_ENTRY(sequence), "20.2748056-2977904");

  gtk_box_pack_start(GTK_BOX(entry_box), label, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(entry_box), sequence, FALSE, FALSE, 5);

  label = gtk_label_new("client :");
  gtk_box_pack_start(GTK_BOX(entry_box), label, FALSE, FALSE, 5);
  suite->client_entry = sequence = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(entry_box), sequence, FALSE, FALSE, 5);  

  return entry_box;
}

/* create the menu bar */
static GtkWidget *menubar(XRemoteTestSuiteData suite)
{
  GtkWidget *menubar;
  GtkItemFactory *factory;
  GtkAccelGroup *accel;
  gint nmenu_items = sizeof (menu_items_G) / sizeof (menu_items_G[0]);
  
  accel = gtk_accel_group_new();
  
  factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel);

  gtk_item_factory_create_items(factory, nmenu_items, menu_items_G, suite);

  gtk_window_add_accel_group(GTK_WINDOW(suite->app_toplevel), accel);

  menubar = gtk_item_factory_get_widget(factory, "<main>");

  return menubar;
}

/* the message box where xml is entered */
static GtkWidget *message_box(XRemoteTestSuiteData suite)
{
  GtkWidget *message_box;
  GtkTextBuffer *text_buffer;

  message_box = gtk_text_view_new();

  gtk_widget_set_size_request(message_box, -1, 200);

  suite->text_buffer = text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(message_box));

  gtk_text_buffer_set_text(text_buffer, "Enter xml here...", -1);

  return message_box;
}

/* All the buttons of the button bar */
static GtkWidget *button_bar(XRemoteTestSuiteData suite)
{
  GtkWidget *button_bar, *run_zmap, *send_command, 
    *clear, *parse, *list_clients, *add_client, *exit;

  button_bar   = gtk_hbutton_box_new();
  run_zmap     = gtk_button_new_with_label("Run ZMap");
  clear        = gtk_button_new_with_label("Clear XML");
  parse        = gtk_button_new_with_label("Check XML");
  send_command = gtk_button_new_with_label("Send Command");
  list_clients = gtk_button_new_with_label("List Clients");
  add_client   = gtk_button_new_with_label("Add Client");
  exit         = gtk_button_new_with_label("Exit");

  gtk_container_add(GTK_CONTAINER(button_bar), clear);
  gtk_container_add(GTK_CONTAINER(button_bar), parse);
  gtk_container_add(GTK_CONTAINER(button_bar), run_zmap);
  gtk_container_add(GTK_CONTAINER(button_bar), send_command);
  gtk_container_add(GTK_CONTAINER(button_bar), list_clients);
  gtk_container_add(GTK_CONTAINER(button_bar), add_client);
  gtk_container_add(GTK_CONTAINER(button_bar), exit);

  g_signal_connect(G_OBJECT(clear), "clicked",
                   G_CALLBACK(clearCB), suite);

  g_signal_connect(G_OBJECT(parse), "clicked",
                   G_CALLBACK(parseCB), suite);

  g_signal_connect(G_OBJECT(run_zmap), "clicked",
                   G_CALLBACK(runZMapCB), suite);

  g_signal_connect(G_OBJECT(send_command), "clicked",
                   G_CALLBACK(sendCommandCB), suite);

  g_signal_connect(G_OBJECT(list_clients), "clicked",
                   G_CALLBACK(listClientsCB), suite);

  g_signal_connect(G_OBJECT(add_client), "clicked",
                   G_CALLBACK(addClientCB), suite);

  g_signal_connect(G_OBJECT(exit), "clicked",
                   G_CALLBACK(quitCB), suite);

  return button_bar;
}

/* ---------------- */
/* Widget callbacks */
/* ---------------- */

/* in case you want to add a x window as a client */
static void addClientCB(GtkWidget *button, gpointer user_data)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)user_data;
  ZMapXRemoteObj client;
  char *client_text = NULL;
  Window wxid = 0;

  client_text = (char *)gtk_entry_get_text(GTK_ENTRY(suite->client_entry));
  wxid = (Window)(strtoul(client_text, (char **)NULL, 16));

  if((client = zMapXRemoteNew()))
    {
      zMapXRemoteInitClient(client, wxid);
      addClientToHash(suite->xremote_clients, client, wxid, NULL, TRUE);
    }
  

  return ;
}

/* time to clean up */
static void quitCB(GtkWidget *unused_button, gpointer user_data)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)user_data;

  printf("Free/Destroying suite data\n");

  if(suite->zmap_pid != 0)
    {
      kill(suite->zmap_pid, SIGKILL);
      suite->zmap_pid = 0;
    }

  if(suite->server_pid != 0)
    {
      kill(suite->server_pid, SIGKILL);
      suite->server_pid = 0;
    }

  g_hash_table_destroy(suite->xremote_clients);

  g_free(suite);

  appExit(EXIT_SUCCESS);

  return ;
}


/* Menu commands internal to put xml events into the text buffer as text (xml) */
static int events_to_text_buffer(ZMapXMLWriter writer, char *xml, int len, gpointer user_data)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)user_data;
  GtkTextIter end; 
  GtkTextBuffer *buffer;

  buffer = suite->text_buffer;
  
  gtk_text_buffer_get_end_iter(buffer, &end);

  gtk_text_buffer_insert(buffer, &end, xml, len);

  return len;
}

/* The menu command to handle the commands menu items */
static void cmdCB( gpointer data, guint callback_action, GtkWidget *w )
{
  static ZMapXMLUtilsEventStackStruct
    start[] = {{ZMAPXML_START_ELEMENT_EVENT, "zmap",   ZMAPXML_EVENT_DATA_NONE,  {0}},
	       {0}},
    end[] = {{ZMAPXML_END_ELEMENT_EVENT,   "zmap",   ZMAPXML_EVENT_DATA_NONE,  {0}},
	     {0}},
    req_start[] = {{ZMAPXML_START_ELEMENT_EVENT, "request",   ZMAPXML_EVENT_DATA_NONE,  {0}},
	       {ZMAPXML_ATTRIBUTE_EVENT,     "action", ZMAPXML_EVENT_DATA_QUARK, {NULL}},
		   {0}},
    req_end[] = {{ZMAPXML_END_ELEMENT_EVENT,   "request",   ZMAPXML_EVENT_DATA_NONE,  {0}},
		 {0}},
    feature[] = {{ZMAPXML_START_ELEMENT_EVENT, "featureset", ZMAPXML_EVENT_DATA_NONE,    {0}},
		   {ZMAPXML_START_ELEMENT_EVENT, "feature",    ZMAPXML_EVENT_DATA_NONE,    {0}},
		   {ZMAPXML_ATTRIBUTE_EVENT,     "name",       ZMAPXML_EVENT_DATA_QUARK,   {""}},
		   {ZMAPXML_ATTRIBUTE_EVENT,     "start",      ZMAPXML_EVENT_DATA_INTEGER, {0}},
		   {ZMAPXML_ATTRIBUTE_EVENT,     "end",        ZMAPXML_EVENT_DATA_INTEGER, {0}},
		   {ZMAPXML_ATTRIBUTE_EVENT,     "strand",     ZMAPXML_EVENT_DATA_QUARK,   {"+|-"}},
		   {ZMAPXML_ATTRIBUTE_EVENT,     "style",      ZMAPXML_EVENT_DATA_QUARK,   {""}},
		   {ZMAPXML_START_ELEMENT_EVENT, "subfeature", ZMAPXML_EVENT_DATA_NONE,    {0}},
		   {ZMAPXML_ATTRIBUTE_EVENT,     "start",      ZMAPXML_EVENT_DATA_INTEGER, {0}},
		   {ZMAPXML_ATTRIBUTE_EVENT,     "end",        ZMAPXML_EVENT_DATA_INTEGER, {0}},
		   {ZMAPXML_ATTRIBUTE_EVENT,     "ontology",   ZMAPXML_EVENT_DATA_QUARK,   {""}},
		   {ZMAPXML_END_ELEMENT_EVENT,   "subfeature", ZMAPXML_EVENT_DATA_NONE,    {0}},
		   {ZMAPXML_END_ELEMENT_EVENT,   "feature",    ZMAPXML_EVENT_DATA_NONE,    {0}},
		   {ZMAPXML_END_ELEMENT_EVENT,   "featureset", ZMAPXML_EVENT_DATA_NONE,    {0}},
		   {0}},
    segment[] = {{ZMAPXML_START_ELEMENT_EVENT, "segment",  ZMAPXML_EVENT_DATA_NONE,    {0}},
		     {ZMAPXML_ATTRIBUTE_EVENT,     "sequence", ZMAPXML_EVENT_DATA_QUARK,   {NULL}},
		     {ZMAPXML_ATTRIBUTE_EVENT,     "start",    ZMAPXML_EVENT_DATA_INTEGER, {(char *)1}},
		     {ZMAPXML_ATTRIBUTE_EVENT,     "end",      ZMAPXML_EVENT_DATA_INTEGER, {0}},
		     {ZMAPXML_END_ELEMENT_EVENT,   "segment",  ZMAPXML_EVENT_DATA_NONE,    {0}},
		     {0}};
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)data;
  ZMapXMLUtilsEventStack data_ptr = NULL ;
  ZMapXMLWriter writer;
  GArray *events;
  char **action;

  action = &(req_start[1].value.s);

  switch(callback_action)
    {
    case XREMOTE_NEW_ZMAP:
      *action  = "new_zmap";

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      data_ptr = &segment[0];
      segment[1].value.s = (char *)gtk_entry_get_text(GTK_ENTRY(suite->sequence));
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      break;
    case XREMOTE_NEW_VIEW:
      *action  = "new_view";
      data_ptr = &segment[0];
      segment[1].value.s = (char *)gtk_entry_get_text(GTK_ENTRY(suite->sequence));
      break;

    case XREMOTE_ZOOMTO:
      *action  = "zoom_to";
      data_ptr = &feature[0];
      break;
    case XREMOTE_DELETE:
      *action  = "delete_feature";
      data_ptr = &feature[0];
      break;
    case XREMOTE_CREATE:
      *action  = "create_feature";
      data_ptr = &feature[0];
      break;
    case XREMOTE_ZOOMIN:
      *action  = "zoom_in";
      data_ptr = NULL;
      break;
    case XREMOTE_ZOOMOUT:
      *action  = "zoom_out";
      data_ptr = NULL;
      break;
    case XREMOTE_SHUTDOWN:
      *action  = "shutdown";
      data_ptr = NULL;
      break;
    default:
      zMapAssertNotReached();
      break;
    }

  events = zMapXMLUtilsStackToEventsArray(&start[0]);

  events = zMapXMLUtilsAddStackToEventsArray(&req_start[0], events);

  if (data_ptr)
    events = zMapXMLUtilsAddStackToEventsArray(data_ptr, events);

  events = zMapXMLUtilsAddStackToEventsArray(&req_end[0], events);

  events = zMapXMLUtilsAddStackToEventsArray(&end[0], events);

  if ((writer = zMapXMLWriterCreate(events_to_text_buffer, suite)))
    {
      ZMapXMLWriterErrorCode xml_status ;

      gtk_text_buffer_set_text(suite->text_buffer, "", -1) ;

      if ((xml_status = zMapXMLWriterProcessEvents(writer, events)) != ZMAPXMLWRITER_OK)
        zMapGUIShowMsg(ZMAP_MSG_WARNING, zMapXMLWriterErrorMsg(writer));
    }

  return ;
}

/* quit from the menu */
static void menuQuitCB(gpointer data, guint callback_action, GtkWidget *w)
{
  quitCB(w, data);

  return ;
}

/* clear the xml message input buffer from the clear button */
static void clearCB(GtkWidget *button, gpointer user_data)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)user_data;

  gtk_text_buffer_set_text(suite->text_buffer, "", -1);

  return ;
}

/* check the xml in the message input buffer parses as valid xml */
static void parseCB(GtkWidget *button, gpointer user_data)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)user_data;
  ZMapXMLParser parser;
  gboolean parse_ok;
  int xml_length = 0;
  char *xml;

  parser = zMapXMLParserCreate(user_data, FALSE, FALSE);
  
  xml = getXML(suite, &xml_length);

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
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)user_data;
  char *xml;
  int xml_length;
  SendCommandDataStruct send_data = {NULL};
 
  if((xml = getXML(suite, &xml_length)) && xml_length > 0)
    {
      send_data.suite = suite;
      send_data.xml   = xml;

      zMapXRemoteAPIMessageProcess(xml, FALSE, &(send_data.action), NULL);

      internal_send_command(&send_data);
    }

  return ;
}

/* button -> run a zmap */
static void runZMapCB(GtkWidget *button, gpointer user_data)
{

  start_zmap_cb(user_data);

  return ;
}

static void list_clients_cb(gpointer key, gpointer hash_data, gpointer user_data)
{
  GString *string = (GString *)user_data;

  g_string_append_printf(string, "Window id = 0x%x\n", GPOINTER_TO_INT(key));

  return ;
}

static void listClientsCB(GtkWidget *button, gpointer user_data)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)user_data;
  GString *client_list;
  client_list = g_string_sized_new(512);
  g_string_append_printf(client_list, "%s", "Clients:\n");

  g_hash_table_foreach(suite->xremote_clients, list_clients_cb, client_list);

  /* This is a bit simple at the moment, we should really have a list
   * store which does a similar thing to the manager bit of zmap. windows
   * could then be raised on selection and a little more info displayed.
   * To do this XQueryTree, gdk_window_foreign_new_for_display, 
   * gdk_window_raise or similar should be used.  The id is not neccessarily
   * the toplevel I guess.
   */

  zMapGUIShowMsg(ZMAP_MSG_INFORMATION, client_list->str);

  g_string_free(client_list, TRUE);

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

static gboolean xml_client_end_cb(gpointer user_data, ZMapXMLElement client_element,
				  ZMapXMLParser parser)
{
  SendCommandData send_data  = (SendCommandData)user_data;
  XRemoteTestSuiteData suite = send_data->suite;
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
  
  if(wxid && req && resp && (new_client = zMapXRemoteNew()))
    {
      GList *actions = NULL;

      zMapXRemoteInitClient(new_client, wxid);
      zMapXRemoteSetRequestAtomName(new_client, req);
      zMapXRemoteSetResponseAtomName(new_client, resp);

      /* make actions list */
      g_list_foreach(client_element->children, action_elements_to_list, &actions);

      addClientToHash(suite->xremote_clients, new_client, wxid, actions, TRUE);
    }

  return TRUE;
}


static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement zmap_element,
                                  ZMapXMLParser parser)
{
  SendCommandData send_data  = (SendCommandData)user_data;
  XRemoteTestSuiteData suite = send_data->suite;
  ZMapXMLAttribute attr = NULL;

  if((attr = zMapXMLElementGetAttributeByName(zmap_element, "action")) != NULL)
    {
      GQuark action = zMapXMLAttributeGetValue(attr);

      if (action == g_quark_from_string("register_client"))
        suite->is_register_client = TRUE;
      else
        suite->is_register_client = FALSE;

      /* When we get this it means the zmap is exitting so clean up the client
       * connection. */
      if (action == g_quark_from_string("finalised"))
	{
	  g_hash_table_foreach_remove(suite->xremote_clients, remove_cb, NULL) ;
	}
    }

  return FALSE;
}

static gboolean xml_client_start_cb(gpointer user_data, ZMapXMLElement client_element,
                                    ZMapXMLParser parser)
{
  SendCommandData send_data  = (SendCommandData)user_data;
  XRemoteTestSuiteData suite = send_data->suite;
  ZMapXRemoteObj new_client;
  ZMapXMLAttribute attr;
  Window wxid = 0;
  char *xid = NULL, *req = NULL, *resp = NULL;

  if (suite->is_register_client)
    {
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
      
      if (wxid && req && resp && (new_client = zMapXRemoteNew()))
        {
	  GList *actions = NULL;

          zMapXRemoteInitClient(new_client, wxid);
          zMapXRemoteSetRequestAtomName(new_client, req);
          zMapXRemoteSetResponseAtomName(new_client, resp);

	  actions = g_list_append(actions, g_strdup("new_zmap"));
	  actions = g_list_append(actions, g_strdup("shutdown"));

          addClientToHash(suite->xremote_clients, new_client, wxid, actions, TRUE);
        }
    }

  return FALSE;
}

static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement element, 
                                ZMapXMLParser parser)
{
  return TRUE;
}

static gboolean xml_error_end_cb(gpointer user_data, ZMapXMLElement element, 
                                 ZMapXMLParser parser)
{
  return TRUE;
}

static gboolean xml_response_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser)
{
  return FALSE;
}


/* really send an xremote command */
static gboolean send_command_cb(gpointer key, gpointer hash_data, gpointer user_data)
{
  HashEntry entry = (HashEntry)hash_data;
  ZMapXMLParser parser;
  ZMapXRemoteObj client;
  SendCommandData send_data = (SendCommandData)user_data;
  char *full_response, *xml;
  int result, code;
  gboolean parse_ok = FALSE, dead_client = FALSE;
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

	  if(!zMapXRemoteResponseIsError(client, full_response))
	    {
	      if(debug_all_responses_G)
		zMapGUIShowMsg(ZMAP_MSG_INFORMATION, full_response);

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
	    zMapGUIShowMsg(ZMAP_MSG_INFORMATION, full_response);
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

  return dead_client;
}

/* handle the hashes so we can correctly iterate and remove/append to the hash of clients... */
static void internal_send_command(SendCommandData send_data)
{
  if(send_data->suite && send_data->xml && !send_data->sent)
    {
      send_data->new_clients = g_hash_table_new_full(NULL, NULL, NULL, destroyHashEntry);
  
      send_data->hash_size   = g_hash_table_size(send_data->suite->xremote_clients);
  
      g_hash_table_foreach_remove(send_data->suite->xremote_clients, 
				  send_command_cb, send_data);
  
      g_hash_table_foreach_steal(send_data->new_clients, 
				 copy_hash_to_hash, 
				 send_data->suite->xremote_clients);
  
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
static char *getXML(XRemoteTestSuiteData suite, int *length_out)
{
  GtkTextBuffer *buffer;
  GtkTextIter start, end;
  char *xml;
  
  buffer = suite->text_buffer;

  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);
  
  xml = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

  if(length_out)
    *length_out = gtk_text_iter_get_offset(&end);

  return xml;
}

/* an app exit function to return a return code of meaning */
static void appExit(int exit_rc)
{
  int true_rc;

  if(exit_rc)
    true_rc = EXIT_FAILURE;
  else
    true_rc = EXIT_SUCCESS;

  gtk_exit(true_rc);

  return ;
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
    { NULL }
  };

  if(entries[0].arg_data == NULL)
    {
      entries[0].arg_data = &(arg_context->version);
      entries[1].arg_data = &(arg_context->config_file);
      entries[2].arg_data = &(arg_context->command_file);
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

  if (g_option_context_parse (arg_context->opt_context, 
			      &arg_context->argc, 
			      &arg_context->argv, 
			      &arg_context->error))
    {
      success = TRUE;
    }
  else
    {
      g_print ("option parsing failed: %s\n", arg_context->error->message);
      success = FALSE;
    }

  return success;
}

static XRemoteCmdLineArgs process_command_line_args(int argc, char *argv[])
{
  XRemoteCmdLineArgs arg_context = NULL;
  gboolean failure = FALSE;

  if((arg_context = g_new0(XRemoteCmdLineArgsStruct, 1)))
    {
      arg_context->argc = argc;
      arg_context->argv = argv;

      if(!makeOptionContext(arg_context))
	failure = TRUE;
    }
  else
    failure = TRUE;

  /* If there was a failure  */
  if(failure)
    appExit(TRUE);

  return arg_context;
}

static char **build_command(char *exe, char *params_as_string)
{
  char **command_out = NULL;
  char **ptr, **split = NULL;
  int i, c = 0;

  split = ptr = g_strsplit(params_as_string, " ", 0);
  
  while(ptr && *ptr != '\0'){ c++; ptr++; }

  command_out = g_new0(char *, c + 2);

  command_out[0] = exe;

  for(i = 0; i < c; i++)
    {
      if(split[i] && *(split[i]))
	command_out[i+1] = split[i]; 
    }

  return command_out;
}

static GList *read_command_file(XRemoteTestSuiteData suite, char *file_name)
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
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)user_data;
  GString *command = (GString *)list_data;

  if((command->str))
    {
      SendCommandData send_data = NULL;
      DependRunner depend_data = NULL;

      send_data        = g_new0(SendCommandDataStruct, 1);
      send_data->suite = suite;
      send_data->xml   = command->str;

      zMapXRemoteAPIMessageProcess(send_data->xml, FALSE, &(send_data->action), NULL);

      depend_data = g_new0(DependRunnerStruct, 1);
      depend_data->main_runner = queue_send_command_cb;
      depend_data->destroy     = queue_destroy_send_data_cb;
      depend_data->runner_data = send_data;
      depend_data->runnable    = TRUE;
      depend_data->runner_done = queue_sent_command_cb;

      g_queue_push_tail(suite->queue, depend_data);
      
    }


  return;
}

static gboolean command_source_cb(gpointer user_data)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)user_data;
  gboolean remove_when_false = TRUE;

  if(!g_queue_is_empty(suite->queue))
    {
      DependRunner depend_data = g_queue_peek_head(suite->queue);
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
	  depend_data = g_queue_pop_head(suite->queue);
	  if(depend_data->destroy)
	    (depend_data->destroy)(depend_data->runner_data);
	  g_free(depend_data);
	}
    }
  else
    remove_when_false = FALSE;

  return remove_when_false;
}

static gboolean start_server_cb(gpointer suite_data)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)suite_data;
  char *tmp_string = NULL;

  /* run sgifaceserver */
  if(zMapConfigIniContextGetString(suite->config_context, XREMOTE_PROG_CONFIG, XREMOTE_PROG_CONFIG,
				   XREMOTE_PROG_SERVER, &tmp_string))
    {
      char *server_path = tmp_string;
      char **command = NULL;
      tmp_string = NULL;

      zMapConfigIniContextGetString(suite->config_context, XREMOTE_PROG_CONFIG, XREMOTE_PROG_CONFIG,
				    XREMOTE_PROG_SERVER_OPTS, &tmp_string);

      command = build_command(server_path, tmp_string);

      run_command(command, &(suite->server_pid));

      g_free(command);
    }

  /* Make sure we only run once */
  return FALSE;
}

static gboolean server_started_cb(gpointer suite_data)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)suite_data;
  gboolean remove_when_true = FALSE;

  if(suite->server_pid)
    remove_when_true = TRUE;

  return remove_when_true;
}

static gboolean start_zmap_cb(gpointer suite_data)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)suite_data;
  char *zmap_path  = NULL;
  char *tmp_string = NULL;

  /* run zmap */
  if ((suite->config_context)
     && (zMapConfigIniContextGetString(suite->config_context, XREMOTE_PROG_CONFIG, XREMOTE_PROG_CONFIG,
				       XREMOTE_PROG_ZMAP, &tmp_string)))
    {
      zmap_path = tmp_string;
    }
  else if((zmap_path = (char *)gtk_entry_get_text(GTK_ENTRY(suite->zmap_path))) == NULL || 
	  (*zmap_path == '\0'))
    {
      zmap_path = "./zmap";
    }

  if (zmap_path != NULL)
    {
      char **command = NULL;
      tmp_string = NULL;

      if ((suite->config_context == NULL) ||
	  !zMapConfigIniContextGetString(suite->config_context, XREMOTE_PROG_CONFIG, XREMOTE_PROG_CONFIG,
					 XREMOTE_PROG_ZMAP_OPTS, &tmp_string))
	tmp_string = "";

      tmp_string = g_strdup_printf("%s 0x%lx %s", 
				   "--win_id", 
				   GDK_DRAWABLE_XID(suite->app_toplevel->window),
				   tmp_string);
      command = build_command(zmap_path, tmp_string);

      g_free(tmp_string);

      run_command(command, &(suite->zmap_pid));

      g_free(command);
    }

  return FALSE;
}

static gboolean zmap_started_cb(gpointer suite_data)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)suite_data;
  gboolean remove_when_true = FALSE;

  if(suite->zmap_pid)
    remove_when_true = TRUE;

  return remove_when_true;
}

static gboolean quit_fin_cb(gpointer suite_data)
{
  return FALSE;			/* Keep on trying to quit! */
}

static gboolean invoke_quit_cb(gpointer suite_data)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)suite_data;

  quitCB(NULL, suite);

  return FALSE;
}

static void process_command_file(XRemoteTestSuiteData suite, char *command_file)
{
  DependRunner depend_data;
  GList *command_list = NULL;

  suite->queue = g_queue_new();

  /* Start the server. */
  depend_data = g_new0(DependRunnerStruct, 1);
  depend_data->main_runner = start_server_cb;
  depend_data->runner_data = suite;
  depend_data->runner_done = server_started_cb;
  depend_data->runnable    = TRUE;

  g_queue_push_tail(suite->queue, depend_data);
  
  /* Start zmap */
  depend_data = g_new0(DependRunnerStruct, 1);
  depend_data->main_runner = start_zmap_cb;
  depend_data->runner_data = suite;
  depend_data->runner_done = zmap_started_cb;
  depend_data->runnable    = TRUE;

  g_queue_push_tail(suite->queue, depend_data);

  /* get individual commands (GList *) */
  command_list = read_command_file(suite, command_file);
  /* run individual commands, waiting 30 seconds between each command */
  g_list_foreach(command_list, command_file_run_command_cb, suite);
  g_list_foreach(command_list, command_file_free_strings, NULL);
  g_list_free(command_list);

  depend_data = g_new0(DependRunnerStruct, 1);
  depend_data->main_runner = invoke_quit_cb;
  depend_data->runner_data = suite;
  depend_data->runner_done = quit_fin_cb;
  depend_data->runnable    = TRUE;

  g_queue_push_tail(suite->queue, depend_data);

  g_timeout_add(5000, command_source_cb, suite);

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

static ZMapConfigIniContext get_configuration(XRemoteTestSuiteData suite)
{
  ZMapConfigIniContext context = NULL;
  
  if(suite->cmd_line_args && suite->cmd_line_args->config_file)
    {
      char *dir  = NULL;
      char *base = NULL;

      dir  = g_path_get_dirname(suite->cmd_line_args->config_file);
      base = g_path_get_basename(suite->cmd_line_args->config_file);

      zMapConfigDirCreate(dir, base); 

      if((context = zMapConfigIniContextCreate()))
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








/* Testbed for ZMapXRemoteAPI */

typedef struct
{
  XRemoteMessage message_out;
  ZMapXMLParser xml_parser;
  char *xml_message;
  unsigned int arrest_processing;
  unsigned int full_processing;
  unsigned int xml_length;
} APIProcessingStruct, *APIProcessing;


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






gboolean zMapXRemoteAPIMessageProcess(char           *message_xml_in, 
				      gboolean        full_process, 
				      char          **action_out,
				      XRemoteMessage *message_out)
{
  APIProcessingStruct process_data = { NULL };
  gboolean success = FALSE;

  if(full_process && message_out)
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
