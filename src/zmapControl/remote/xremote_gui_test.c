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
 * Last edited: Mar  9 10:23 2007 (edgrif)
 * Created: Thu Feb 15 11:25:20 2007 (rds)
 * CVS info:   $Id: xremote_gui_test.c,v 1.4 2007-03-09 10:25:24 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <gtk/gtk.h>
#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsGUI.h>
#include <ZMap/zmapXRemote.h>
#include <ZMap/zmapXML.h>

typedef struct
{
  GtkWidget *app_toplevel, *vbox, *menu, *text_area, *buttons, *sequence, *zmap_path;
  GtkTextBuffer *text_buffer;
  zMapXRemoteObj xremote_server;
  GHashTable *xremote_clients;
  gulong register_client_id;
  gboolean is_register_client;
}XRemoteTestSuiteDataStruct, *XRemoteTestSuiteData;

typedef struct
{
  XRemoteTestSuiteData suite;
  char *xml;
  int hash_size;
}SendCommandDataStruct, *SendCommandData;

typedef struct
{
  zMapXRemoteObj client;
  gboolean is_main_window;
}HashEntryStruct, *HashEntry;

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

static void quitCB(GtkWidget *button, gpointer user_data);
static void clearCB(GtkWidget *button, gpointer user_data);
static void parseCB(GtkWidget *button, gpointer user_data);
static void sendCommandCB(GtkWidget *button, gpointer user_data);
static void listClientsCB(GtkWidget *button, gpointer user_data);
static void runZMapCB(GtkWidget *button, gpointer user_data);

static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement element, 
                                  ZMapXMLParser parser);
static gboolean xml_response_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser);
static gboolean xml_client_start_cb(gpointer user_data, ZMapXMLElement client_element,
                                    ZMapXMLParser parser);
static gboolean xml_error_end_cb(gpointer user_data, ZMapXMLElement element, 
                                 ZMapXMLParser parser);
static gboolean xml_windowid_end_cb(gpointer user_data, ZMapXMLElement element, 
                                    ZMapXMLParser parser);
static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement element, 
                                ZMapXMLParser parser);

static char *getXML(XRemoteTestSuiteData suite, int *length_out);
static HashEntry clientToHashEntry(zMapXRemoteObj client, gboolean is_main);
static gboolean addClientToHash(GHashTable *table, zMapXRemoteObj client, Window id, gboolean is_main);
static void destroyHashEntry(gpointer entry_data);
static void appExit(int exit_rc);

enum
  {  
    XREMOTE_NEW,
    XREMOTE_ZOOMIN,
    XREMOTE_ZOOMOUT,
    XREMOTE_CREATE,
    XREMOTE_DELETE,
    XREMOTE_ZOOMTO,
    XREMOTE_SHUTDOWN
  };

static gboolean command_debug_G = FALSE;
static GtkItemFactoryEntry menu_items_G[] = {
  {"/_File",                   NULL,         NULL,       0,                "<Branch>", NULL},
  {"/File/Quit",               "<control>Q", menuQuitCB, 0,                NULL,       NULL},  
  {"/_Commands",               NULL,         NULL,       0,                "<Branch>", NULL},
  {"/Commands/New",            NULL,         cmdCB,      XREMOTE_NEW,      NULL,       NULL},
  {"/Commands/Zoom In",        NULL,         cmdCB,      XREMOTE_ZOOMIN,   NULL,       NULL},
  {"/Commands/Zoom Out",       NULL,         cmdCB,      XREMOTE_ZOOMOUT,  NULL,       NULL},
  {"/Commands/Feature Create", NULL,         cmdCB,      XREMOTE_CREATE,   NULL,       NULL},
  {"/Commands/Feature Delete", NULL,         cmdCB,      XREMOTE_DELETE,   NULL,       NULL},
  {"/Commands/Feature Goto",   NULL,         cmdCB,      XREMOTE_ZOOMTO,   NULL,       NULL},
  {"/Commands/Shutdown",       NULL,         cmdCB,      XREMOTE_SHUTDOWN, NULL,       NULL}
};


/* main */

int main(int argc, char *argv[])
{
  int main_rc;

  main_rc = zmapXremoteTestSuite(argc, argv);

  return main_rc;
}


/* rest of the code. */

static int zmapXremoteTestSuite(int argc, char *argv[])
{
  XRemoteTestSuiteData suite;
  GtkWidget *toplevel, *vbox, *menu_bar, *buttons, *textarea, *frame, *entry_box;

  gtk_init(&argc, &argv);

  if((suite = g_new0(XRemoteTestSuiteDataStruct, 1)))
    {
      suite->xremote_clients = g_hash_table_new_full(NULL, NULL, NULL, destroyHashEntry);

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

      return EXIT_FAILURE;
    }

  return EXIT_FAILURE;
}

static void installPropertyNotify(GtkWidget *ignored, XRemoteTestSuiteData suite)
{
  zMapXRemoteNotifyData notify_data;
  zMapXRemoteObj xremote;
  Window id = 0;

  zMapAssert(ignored == suite->app_toplevel);

  if((id = (Window)(GDK_DRAWABLE_XID(suite->app_toplevel->window))) && (xremote = zMapXRemoteNew()))
    {
      suite->xremote_server = xremote;
      notify_data = g_new0(zMapXRemoteNotifyDataStruct, 1);

      notify_data->xremote  = xremote;
      notify_data->callback = ZMAPXREMOTE_CALLBACK(handle_register_client);
      notify_data->data     = suite;
      /* These client request and response names are hard coded somewhere else, which is bad! */
      zMapXRemoteInitServer(xremote, id, "xremote_gui_test", "_CLIENT_REQUEST_NAME", "_CLIENT_RESPONSE_NAME");

      gtk_widget_add_events(suite->app_toplevel, GDK_PROPERTY_CHANGE_MASK);
      
      suite->register_client_id = g_signal_connect(G_OBJECT(suite->app_toplevel), "property_notify_event",
                                                   G_CALLBACK(zMapXRemotePropertyNotifyEvent), 
                                                   notify_data);
    }
  else if(!id)
    {
      printf("Toplevel has no window id!\n");
    }
}

static char *handle_register_client(char *command_text, gpointer user_data, int *statusCode)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)user_data;
  ZMapXMLObjTagFunctionsStruct starts[] = {
    {"zmap",     xml_zmap_start_cb   },
    {"client",   xml_client_start_cb },
    { NULL, NULL}
  };
  ZMapXMLObjTagFunctionsStruct ends[] = {
    {"zmap",  xml_zmap_end_cb  },
    { NULL, NULL}
  };
  ZMapXMLParser parser;
  gboolean parse_ok;
  char *reply;
  int status = 500;

  parser = zMapXMLParserCreate(suite, FALSE, FALSE);

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &starts[0], &ends[0]);

  reply = g_strdup_printf("%s", "[reply text]");

  if((parse_ok = zMapXMLParserParseBuffer(parser, command_text, strlen(command_text))))
    {
      status = 200;
    }
  else
    status = 403;

  if(statusCode)
    *statusCode = status;


  return reply;
}


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

  gtk_entry_set_text(GTK_ENTRY(sequence), "20.3013641-3258367");

  gtk_box_pack_start(GTK_BOX(entry_box), label, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(entry_box), sequence, FALSE, FALSE, 5);

  return entry_box;
}

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

static GtkWidget *button_bar(XRemoteTestSuiteData suite)
{
  GtkWidget *button_bar, *run_zmap, *send_command, 
    *clear, *parse, *list_clients, *exit;

  button_bar   = gtk_hbutton_box_new();
  run_zmap     = gtk_button_new_with_label("Run ZMap");
  clear        = gtk_button_new_with_label("Clear XML");
  parse        = gtk_button_new_with_label("Check XML");
  send_command = gtk_button_new_with_label("Send Command");
  list_clients = gtk_button_new_with_label("List Clients");
  exit         = gtk_button_new_with_label("Exit");

  gtk_container_add(GTK_CONTAINER(button_bar), clear);
  gtk_container_add(GTK_CONTAINER(button_bar), parse);
  gtk_container_add(GTK_CONTAINER(button_bar), run_zmap);
  gtk_container_add(GTK_CONTAINER(button_bar), send_command);
  gtk_container_add(GTK_CONTAINER(button_bar), list_clients);
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

  g_signal_connect(G_OBJECT(exit), "clicked",
                   G_CALLBACK(quitCB), suite);

  return button_bar;
}

static void quitCB(GtkWidget *button, gpointer user_data)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)user_data;

  printf("Free/Destroying suite data\n");

  g_hash_table_destroy(suite->xremote_clients);

  g_free(suite);

  appExit(EXIT_SUCCESS);

  return ;
}


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

static void cmdCB( gpointer data, guint callback_action, GtkWidget *w )
{
  static ZMapXMLUtilsEventStackStruct start[] = {
    {ZMAPXML_START_ELEMENT_EVENT, "zmap",   ZMAPXML_EVENT_DATA_NONE,  {0}},
    {ZMAPXML_ATTRIBUTE_EVENT,     "action", ZMAPXML_EVENT_DATA_QUARK, {NULL}},
    {0}
  }, end[] = {
    {ZMAPXML_END_ELEMENT_EVENT,   "zmap",   ZMAPXML_EVENT_DATA_NONE,  {0}},
    {0}
  }, feature[] = {
    {ZMAPXML_START_ELEMENT_EVENT, "featureset", ZMAPXML_EVENT_DATA_NONE,    {0}},
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
    {0}
  }, segment[] = {
    {ZMAPXML_START_ELEMENT_EVENT, "segment",  ZMAPXML_EVENT_DATA_NONE,    {0}},
    {ZMAPXML_ATTRIBUTE_EVENT,     "sequence", ZMAPXML_EVENT_DATA_QUARK,   {NULL}},
    {ZMAPXML_ATTRIBUTE_EVENT,     "start",    ZMAPXML_EVENT_DATA_INTEGER, {(char *)1}},
    {ZMAPXML_ATTRIBUTE_EVENT,     "end",      ZMAPXML_EVENT_DATA_INTEGER, {0}},
    {ZMAPXML_END_ELEMENT_EVENT,   "segment",  ZMAPXML_EVENT_DATA_NONE,    {0}},
    {0}
  };
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)data;
  ZMapXMLWriterErrorCode xml_status;
  ZMapXMLUtilsEventStack data_ptr;
  ZMapXMLWriter writer;
  GArray *events;
  char **action;

  action = &(start[1].value.s);

  switch(callback_action)
    {
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
    case XREMOTE_NEW:
      *action  = "new";
      data_ptr = &segment[0];
      segment[1].value.s = (char *)gtk_entry_get_text(GTK_ENTRY(suite->sequence));
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

  if(data_ptr)
    events = zMapXMLUtilsAddStackToEventsArray(data_ptr, events);

  events = zMapXMLUtilsAddStackToEventsArray(&end[0], events);

  if((writer = zMapXMLWriterCreate(events_to_text_buffer, suite)))
    {
      gtk_text_buffer_set_text(suite->text_buffer, "", -1);
      if((xml_status = zMapXMLWriterProcessEvents(writer, events)) != ZMAPXMLWRITER_OK)
        zMapGUIShowMsg(ZMAP_MSG_WARNING, zMapXMLWriterErrorMsg(writer));
    }

  return ;
}

static void menuQuitCB(gpointer data, guint callback_action, GtkWidget *w)
{
  quitCB(w, data);
  return ;
}

static void clearCB(GtkWidget *button, gpointer user_data)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)user_data;

  gtk_text_buffer_set_text(suite->text_buffer, "", -1);

  return ;
}

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


/* Just results in the entry being removed from the hash table, the destroy func
 * does the real work. */
static gboolean remove_cb(gpointer key, gpointer hash_data, gpointer user_data)
{
  return TRUE ;
}


static gint send_command_cb(gpointer key, gpointer hash_data, gpointer user_data)
{
  HashEntry entry = (HashEntry)hash_data;
  ZMapXMLParser parser;
  zMapXRemoteObj client;
  SendCommandData send_data = (SendCommandData)user_data;
  char *full_response, *xml;
  int result, code, dead_client = 0;
  gboolean parse_ok = FALSE;
  ZMapXMLObjTagFunctionsStruct starts[] = {
    {"zmap",     xml_zmap_start_cb     },
    {"response", xml_response_start_cb },
    { NULL, NULL}
  };
  ZMapXMLObjTagFunctionsStruct ends[] = {
    {"zmap",     xml_zmap_end_cb  },
    {"windowid", xml_windowid_end_cb },
    {"error",    xml_error_end_cb },
    { NULL, NULL}
  };

  client = entry->client;

  if((result = zMapXRemoteSendRemoteCommand(client, send_data->xml, &full_response)) == ZMAPXREMOTE_SENDCOMMAND_SUCCEED)
    {
      zMapGUIShowMsg(ZMAP_MSG_INFORMATION, full_response);

      if(!zMapXRemoteResponseIsError(client, full_response))
        {
          parser = zMapXMLParserCreate(send_data->suite, FALSE, FALSE);
          zMapXMLParserSetMarkupObjectTagHandlers(parser, &starts[0], &ends[0]);
          
          zMapXRemoteResponseSplit(client, full_response, &code, &xml);

          if((parse_ok = zMapXMLParserParseBuffer(parser, xml, strlen(xml))))
            {
              
            }
          else
            zMapGUIShowMsg(ZMAP_MSG_WARNING, zMapXMLParserLastErrorMsg(parser));
        }
    }
  else
    {
      char *message;
      message = g_strdup_printf("send command failed, deleting client (0x%x)...", GPOINTER_TO_INT(key));
      zMapGUIShowMsg(ZMAP_MSG_WARNING, message);
      g_free(message);
      dead_client = 1;
    }

  return dead_client;
}

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

      send_data.hash_size = g_hash_table_size(suite->xremote_clients);
      
      g_hash_table_foreach_remove(suite->xremote_clients, send_command_cb, &send_data);
    }

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

static void runZMapCB(GtkWidget *button, gpointer user_data)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)user_data;
  char *zmap_path;
  char *command[] = {NULL,       /* 0 path */
                     "--win_id", /* 1 option */
                     NULL,       /* 2 toplevel id  */
                     NULL};      /* 3 terminating NULL */

  if((zmap_path = (char *)gtk_entry_get_text(GTK_ENTRY(suite->zmap_path))) == NULL || (*zmap_path == '\0'))
    zmap_path = "./zmap";       /* make sure we get the CWD zmap */

  if((command[0] = g_strdup_printf("%s", zmap_path)) &&
     (command[2] = g_strdup_printf("0x%lx", GDK_DRAWABLE_XID(suite->app_toplevel->window))))
    {
      int child_pid;
      char **ptr;
      char *cwd = NULL, **envp = NULL; /* inherit from parent */
      GSpawnFlags flags = G_SPAWN_SEARCH_PATH;
      GSpawnChildSetupFunc pre_exec = NULL;
      gpointer user_data = NULL;
      GError *error = NULL;

      if(command_debug_G)
        {
          printf("command: ");
          ptr = &command[0];
          while(ptr && *ptr)
            {
              printf("%s ", *ptr);
              ptr++;
            }
          printf("\n");
        }

      if(!g_spawn_async(cwd, command, envp, flags, pre_exec, user_data, &child_pid, &error))
        {
          printf("Errror %s\n", error->message);
        }
    }

  return ;
}

static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement zmap_element,
                                  ZMapXMLParser parser)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)user_data;
  ZMapXMLAttribute attr = NULL;

  if((attr = zMapXMLElementGetAttributeByName(zmap_element, "action")) != NULL)
    {
      GQuark action = zMapXMLAttributeGetValue(attr);
      if(action == g_quark_from_string("register_client"))
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
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)user_data;
  zMapXRemoteObj new_client;
  ZMapXMLAttribute attr;
  Window wxid = 0;
  char *xid = NULL, *req = NULL, *resp = NULL;

  if(suite->is_register_client)
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
      
      if(wxid && req && resp && (new_client = zMapXRemoteNew()))
        {
          zMapXRemoteInitClient(new_client, wxid);
          zMapXRemoteSetRequestAtomName(new_client, req);
          zMapXRemoteSetResponseAtomName(new_client, resp);

          addClientToHash(suite->xremote_clients, new_client, wxid, TRUE);
        }
    }

  return FALSE;
}

static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement element, 
                                ZMapXMLParser parser)
{
  return TRUE;
}

static gboolean xml_windowid_end_cb(gpointer user_data, ZMapXMLElement element, 
                                    ZMapXMLParser parser)
{
  XRemoteTestSuiteData suite = (XRemoteTestSuiteData)user_data;
  zMapXRemoteObj client;
  Window wxid;
  char *content;

  content = zMapXMLElementStealContent(element);
  
  if((client = zMapXRemoteNew()))
    {
      wxid = (Window)(strtoul(content, (char **)NULL, 16));
      zMapXRemoteInitClient(client, wxid);
      addClientToHash(suite->xremote_clients, client, wxid, FALSE);
    }

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


static HashEntry clientToHashEntry(zMapXRemoteObj client, gboolean is_main)
{
  HashEntry entry;

  if((entry = g_new0(HashEntryStruct, 1)))
    {
      entry->client = client;
      entry->is_main_window = is_main;
    }

  return entry;
}

static gboolean addClientToHash(GHashTable *table, zMapXRemoteObj client, Window id, gboolean is_main)
{
  HashEntry new_entry;
  gboolean inserted = FALSE;

  if(!(g_hash_table_lookup(table, GINT_TO_POINTER(id))))
    {
      new_entry = clientToHashEntry(client, is_main);

      g_hash_table_insert(table, GINT_TO_POINTER(id), new_entry);

      inserted = TRUE;
    }

  return inserted;
}

static void destroyHashEntry(gpointer entry_data)
{
  HashEntry entry = (HashEntry)entry_data;

  zMapXRemoteDestroy(entry->client);
  entry->client = NULL;
  g_free(entry);

  return ;
}

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

