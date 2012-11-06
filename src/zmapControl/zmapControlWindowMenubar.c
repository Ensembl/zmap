/*  Last edited: Jul 23 10:33 2012 (edgrif) */
/*  File: zmapControlWindowMenubar.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 * and was written by
 *     Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *       Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *  Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Code for the menubar in a zmap window.
 *
 *              NOTE that this code uses the itemfactory which is
 *              deprecated in GTK 2, BUT to replace it means
 *              understanding their UI manager...we need to do this
 *              but its quite a lot of work...
 *
 * Exported functions: See zmapControl_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <stdlib.h>
#include <stdio.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsGUI.h>
#include <ZMap/zmapAppServices.h>
#include <zmapControl_P.h>


typedef enum {RT_INVALID, RT_ACEDB, RT_ANACODE, RT_SEQTOOLS, RT_ZMAP, RT_ZMAP_USER_TICKETS} RTQueueName ;


static void newSequenceByConfigCB(gpointer cb_data, guint callback_action, GtkWidget *w) ;
static void makeSequenceViewCB(ZMapFeatureSequenceMap sequence_map, gpointer user_data) ;
static void closeCB(gpointer cb_data, guint callback_action, GtkWidget *w) ;
static void quitCB(gpointer cb_data, guint callback_action, GtkWidget *w) ;

static void importCB(gpointer cb_data, guint callback_action, GtkWidget *window);
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void exportCB(gpointer cb_data, guint callback_action, GtkWidget *w);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void printCB(gpointer cb_data, guint callback_action, GtkWidget *w);
static void dumpCB(gpointer cb_data, guint callback_action, GtkWidget *w);
static void redrawCB(gpointer cb_data, guint callback_action, GtkWidget *w);
static void preferencesCB(gpointer cb_data, guint callback_action, GtkWidget *w);
static void developerCB(gpointer cb_data, guint callback_action, GtkWidget *w);
static void showStatsCB(gpointer cb_data, guint callback_action, GtkWidget *window) ;
static void showSessionCB(gpointer cb_data, guint callback_action, GtkWidget *window) ;
static void aboutCB(gpointer cb_data, guint callback_action, GtkWidget *w);
static void rtTicket(gpointer cb_data, guint callback_action, GtkWidget *w);
static void allHelpCB(gpointer cb_data, guint callback_action, GtkWidget *w);
static void formatSession(gpointer data, gpointer user_data) ;
static void print_hello( gpointer data, guint callback_action, GtkWidget *w ) ;
#ifdef ALLOW_POPOUT_PANEL
static void popout_panel( gpointer data, guint callback_action, GtkWidget *w ) ;
#endif /* ALLOW_POPOUT_PANEL */

GtkItemFactory *item_factory;


static GtkItemFactoryEntry menu_items[] = {
 { "/_File",                        NULL,         NULL,                  0, "<Branch>" },
 { "/File/_New Sequence",           NULL,         newSequenceByConfigCB, 2, NULL },


 { "/File/sep1",     NULL,         NULL, 0, "<Separator>" },
 { "/File/_Import",  "<control>I", importCB, 0, NULL },		/* or Read ? */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
{ "/File/_Export",  "<control>E", exportCB, 0, NULL },
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

 { "/File/sep1",                     NULL,         NULL, 0, "<Separator>" },
 { "/File/_Save screen shot",        "<control>D", dumpCB, 0, NULL },
 { "/File/_Print screen shot",       "<control>P", printCB, 0, NULL },
 { "/File/sep1",                     NULL,           NULL, 0, "<Separator>" },
 { "/File/Close",                    "<control>W", closeCB, 0, NULL },
 { "/File/Quit",                     "<control>Q", quitCB, 0, NULL },
 { "/_Edit",                         NULL,         NULL, 0, "<Branch>" },
 { "/Edit/Cu_t",                     "<control>X", print_hello, 0, NULL },
 { "/Edit/_Copy",                    "<control>C", print_hello, 0, NULL },
 { "/Edit/_Paste",   "<control>V", print_hello, 0, NULL },
 { "/Edit/_Redraw",  NULL,         redrawCB, 0, NULL },
 { "/Edit/sep1",     NULL,         NULL, 0, "<Separator>" },
 { "/Edit/P_references",  NULL,    preferencesCB, 0, NULL },
 { "/Edit/_Set Developer status",  NULL,    developerCB, 0, NULL },
 { "/_View",         NULL,         NULL, 0, "<Branch>" },
#ifdef ALLOW_POPOUT_PANEL
 { "/View/'Pop Out' Control Info Panel", NULL, popout_panel, 0, NULL },
#endif	/* ALLOW_POPOUT_PANEL */
 { "/View/Statistics", NULL,       showStatsCB, 0, NULL },
 { "/View/Session Details", NULL,  showSessionCB, 0, NULL },
 { "/_Raise ticket",  NULL,        NULL, 0, "<LastBranch>" },
 { "/Raise ticket/See ZMap tickets", NULL, rtTicket, RT_ZMAP_USER_TICKETS, NULL },
 { "/Raise ticket/ZMap ticket",       NULL, rtTicket, RT_ZMAP, NULL },
 { "/Raise ticket/Anacode ticket",    NULL, rtTicket, RT_ANACODE, NULL },
 { "/Raise ticket/Blixem, Dotter or Belvu ticket",      NULL, rtTicket, RT_SEQTOOLS, NULL },
 { "/Raise ticket/Acedb ticket",      NULL, rtTicket, RT_ACEDB, NULL },
 { "/_Help",         NULL,         NULL, 0, "<LastBranch>" },
// { "/Help/General Help", NULL,     allHelpCB, ZMAPGUI_HELP_GENERAL, NULL },
// { "/Help/Keyboard & Mouse", NULL, allHelpCB, ZMAPGUI_HELP_KEYBOARD, NULL },
// { "/Help/Alignment Display", NULL, allHelpCB, ZMAPGUI_HELP_ALIGNMENT_DISPLAY, NULL },
// { "/Help/Release Notes", NULL,    allHelpCB, ZMAPGUI_HELP_RELEASE_NOTES, NULL },
 { "/Help/What's New", NULL,    allHelpCB, ZMAPGUI_HELP_WHATS_NEW, NULL },
 { "/Help/About ZMap",    NULL,    aboutCB, 0, NULL }
};




GtkWidget *zmapControlWindowMakeMenuBar(ZMap zmap)
{
  GtkWidget *menubar ;
  GtkAccelGroup *accel_group ;
  gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]) ;

  accel_group = gtk_accel_group_new() ;

  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group) ;

  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, (gpointer)zmap) ;

  gtk_window_add_accel_group(GTK_WINDOW(zmap->toplevel), accel_group) ;

  menubar = gtk_item_factory_get_widget (item_factory, "<main>") ;

  return menubar ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Should pop up a dialog box to ask for a file name....e.g. the file chooser. */
static void exportCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  ZMap zmap = (ZMap)cb_data ;
  gchar *file = "ZMap.features" ;

  zmapViewFeatureDump(zmap->focus_viewwindow, file) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



static void controlImportFileCB(gpointer user_data)
{
	zMapWarning("controlImportFileCB not implemented","");
	/* this is a callback to report something */
}

static void importCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  ZMap zmap = (ZMap)cb_data ;
  ZMapViewWindow vw = zmap->focus_viewwindow;
  ZMapFeatureSequenceMap map;
  ZMapFeatureSequenceMap view_seq;
  int start,end;

  view_seq = zMapViewGetSequenceMap( zMapViewGetView(vw) );

  /* get view sequence and coords */
  map = g_new0(ZMapFeatureSequenceMapStruct,1);
  map->start = view_seq->start;
  map->end = view_seq->end;
  map->sequence = view_seq->sequence;
  map->config_file = view_seq->config_file;

  /* limit to mark if set */
  start = view_seq->start;
  end   = view_seq->end;

  if(zMapWindowMarkIsSet(zMapViewGetWindow(vw)))
  {
	zMapWindowGetMark(zMapViewGetWindow(vw), &start, &end);	/* NOTE we get -fwd coords from this function if revcomped */

	if(start < 0)
		start = -start;
	if(end < 0)
		end = -end;

	start += map->start;
	end   += map->start;
  }

  /* need sequence_map to set default seq coords and map sequence name */
  zMapControlImportFile(controlImportFileCB, cb_data, map, start, end);

  return ;
}


static void dumpCB(gpointer cb_data, guint callback_action, GtkWidget *widget)
{
  ZMap zmap = (ZMap)cb_data ;
  ZMapWindow window ;

  window = zMapViewGetWindow(zmap->focus_viewwindow) ;

  zMapWindowDump(window) ;

  return ;
}


static void printCB(gpointer cb_data, guint callback_action, GtkWidget *widget)
{
  ZMap zmap = (ZMap)cb_data ;
  ZMapWindow window ;

  window = zMapViewGetWindow(zmap->focus_viewwindow) ;

  zMapWindowPrint(window) ;

  return ;
}


/* Causes currently focussed zmap window to redraw itself. */
static void redrawCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  ZMap zmap = (ZMap)cb_data ;

  zMapViewRedraw(zmap->focus_viewwindow) ;

  return ;
}


/* Shows preference edit window. */
static void preferencesCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  ZMap zmap = (ZMap)cb_data ;

  zmapControlShowPreferences(zmap) ;

  return ;
}


/* Shows developer status dialog window. */
static void developerCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  ZMap zmap = (ZMap)cb_data ;
  char *passwd = NULL ;
  GtkResponseType result ;

  result = zMapGUIMsgGetText(GTK_WINDOW(zmap->toplevel),
			     ZMAP_MSG_INFORMATION, "Enter Developer Password:", TRUE,
			     &passwd) ;

  if (result == GTK_RESPONSE_OK)
    {
      if (!passwd || !(*passwd) || !zMapUtilsUserSetDeveloper(passwd))
	zMapGUIShowMsg(ZMAP_MSG_WARNING, "Password Verification Failed") ;
    }

  return ;
}





/* Display stats for currently focussed zmap window. */
static void showStatsCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  ZMap zmap = (ZMap)cb_data ;
  ZMapViewSession view_data ;
  GString *session_text ;
  char *title ;

  session_text = g_string_new(NULL) ;


  g_string_append(session_text, "General\n") ;
  g_string_append_printf(session_text, "\tProgram: %s\n\n", zMapGetAppTitle()) ;
  g_string_append_printf(session_text, "\tUser: %s (%s)\n\n", g_get_user_name(), g_get_real_name()) ;
  g_string_append_printf(session_text, "\tMachine: %s\n\n", g_get_host_name()) ;
  view_data = zMapViewSessionGetData(zmap->focus_viewwindow) ;
  g_string_append_printf(session_text, "\tSequence: %s\n\n", view_data->sequence) ;

  g_string_append(session_text, "Session Statistics\n") ;
  zMapViewStats(zmap->focus_viewwindow,session_text) ;

  title = zMapGUIMakeTitleString(NULL, "Session Statistics") ;
  zMapGUIShowText(title, session_text->str, FALSE) ;
  g_free(title) ;
  g_string_free(session_text, TRUE) ;


  return ;
}



/* Display session data, this is a mixture of machine and per view data. */
static void showSessionCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  ZMap zmap = (ZMap)cb_data ;
  ZMapViewSession view_data ;
  GString *session_text ;
  char *title ;

  session_text = g_string_new(NULL) ;

  g_string_append(session_text, "General\n") ;
  g_string_append_printf(session_text, "\tProgram: %s\n\n", zMapGetAppTitle()) ;
  g_string_append_printf(session_text, "\tUser: %s (%s)\n\n", g_get_user_name(), g_get_real_name()) ;
  g_string_append_printf(session_text, "\tMachine: %s\n\n", g_get_host_name()) ;

  view_data = zMapViewSessionGetData(zmap->focus_viewwindow) ;

  g_string_append_printf(session_text, "\tSequence: %s\n\n", view_data->sequence) ;

  if (view_data->servers)
    {
      g_list_foreach(view_data->servers, formatSession, session_text) ;
    }

  title = zMapGUIMakeTitleString(NULL, "Session Details") ;
  zMapGUIShowText(title, session_text->str, FALSE) ;
  g_free(title) ;
  g_string_free(session_text, TRUE) ;

  return ;
}


static void formatSession(gpointer data, gpointer user_data)
{
  ZMapViewSessionServer server_data = (ZMapViewSessionServer)data ;
  GString *session_text = (GString *)user_data ;


  g_string_append(session_text, "Server\n") ;

  g_string_append_printf(session_text, "\tURL: %s\n\n", server_data->url) ;
  g_string_append_printf(session_text, "\tProtocol: %s\n\n", server_data->protocol) ;

  switch(server_data->scheme)
    {
    case SCHEME_ACEDB:
      {
	g_string_append_printf(session_text, "\tServer: %s\n\n", server_data->scheme_data.acedb.host) ;
	g_string_append_printf(session_text, "\tPort: %d\n\n", server_data->scheme_data.acedb.port) ;
	g_string_append_printf(session_text, "\tDatabase: %s\n\n", server_data->scheme_data.acedb.database) ;
	break ;
      }
    case SCHEME_FILE:
      {
	g_string_append_printf(session_text, "\tFormat: %s\n\n", server_data->format) ;
	g_string_append_printf(session_text, "\tFile: %s\n\n", server_data->scheme_data.file.path) ;
	break ;
      }
    case SCHEME_PIPE:
      {
      g_string_append_printf(session_text, "\tFormat: %s\n\n", server_data->format) ;
      g_string_append_printf(session_text, "\tScript: %s\n\n", server_data->scheme_data.pipe.path) ;
      g_string_append_printf(session_text, "\tQuery: %s\n\n", server_data->scheme_data.pipe.query) ;
      break ;
      }
    default:
      {
	g_string_append(session_text, "\tUnsupported server type !") ;
	break ;
      }
    }

  return ;
}




/* Show the usual tedious "About" dialog. */
static void aboutCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  zMapGUIShowAbout() ;

  return ;
}


/* Show the web page for raising ZMap tickets. */
static void rtTicket(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  gboolean raise_ticket = TRUE ;
  char *url_raise_ticket_base = "https://rt.sanger.ac.uk/rt/Ticket/Create.html?Queue=" ;
  char *web_page = NULL ;
  gboolean result ;
  GError *error = NULL ;
  RTQueueName queue_name = (RTQueueName)callback_action ;
  int queue_number ;

  switch (queue_name)
    {
    case RT_ACEDB:
      queue_number = 38 ;
      break ;
    case RT_SEQTOOLS:
      queue_number = 120 ;
      break ;
    case RT_ANACODE:
      queue_number = 49 ;
      break ;
    case RT_ZMAP:
      queue_number = 7 ;
      break ;
    case RT_ZMAP_USER_TICKETS:
      queue_number = 7 ;
      raise_ticket = FALSE ;
      break ;
    default:
      zMapAssertNotReached() ;
      break ;
    }

  if (raise_ticket)
    {
      web_page = g_strdup_printf("%s%d", url_raise_ticket_base, queue_number) ;
    }
  else
    {
      web_page = g_strdup_printf("%s", "https://rt.sanger.ac.uk/Search/Results.html?Format='%20%20%20%3Cb%3E%3Ca%20href%3D%22__WebPath__%2FTicket%2FDisplay.html%3Fid%3D__id__%22%3E__id__%3C%2Fa%3E%3C%2Fb%3E%2FTITLE%3A%23'%2C%0A'%3Cb%3E%3Ca%20href%3D%22__WebPath__%2FTicket%2FDisplay.html%3Fid%3D__id__%22%3E__Subject__%3C%2Fa%3E%3C%2Fb%3E%2FTITLE%3ASubject'%2C%0A'__Status__'%2C%0A'__QueueName__'%2C%0A'__OwnerName__'%2C%0A'__Priority__'%2C%0A'__NEWLINE__'%2C%0A''%2C%0A'%3Csmall%3E__Requestors__%3C%2Fsmall%3E'%2C%0A'%3Csmall%3E__CreatedRelative__%3C%2Fsmall%3E'%2C%0A'%3Csmall%3E__ToldRelative__%3C%2Fsmall%3E'%2C%0A'%3Csmall%3E__LastUpdatedRelative__%3C%2Fsmall%3E'%2C%0A'%3Csmall%3E__TimeLeft__%3C%2Fsmall%3E'&Order=DESC&OrderBy=Created&Page=1&Query=Status%20!%3D%20'resolved'%20AND%20Queue%20%3D%20'zmap'%20AND%20'CF.%7BImportance%7D'%20%3D%20'Important'%20AND%20'CF.%7BUrgency%7D'%20%3D%20'Urgent'&Rows=50") ;
    }

  if (!(result = zMapLaunchWebBrowser(web_page, &error)))
    {
      zMapWarning("Error: %s\n", error->message) ;

      g_error_free(error) ;
    }
  else
    {
      zMapGUIShowMsgFull(NULL, "Please wait, ticket page will be shown in your browser in a few seconds.",
			 ZMAP_MSG_INFORMATION,
			 GTK_JUSTIFY_CENTER, 5, TRUE) ;
    }

  g_free(web_page) ;

  return ;
}


/* Show the web page of release notes. */
static void allHelpCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  zMapGUIShowHelp((ZMapHelpType)callback_action) ;

  return ;
}


/* Close just this zmap... */
static void closeCB(gpointer cb_data, guint callback_action, GtkWidget *w)
{
  ZMap zmap = (ZMap)cb_data ;

  zmapControlSignalKill(zmap) ;

  return ;
}



/* Kill the whole zmap application. */
static void quitCB(gpointer cb_data, guint callback_action, GtkWidget *w)
{
  ZMap zmap = (ZMap)cb_data ;

  /* Call the application exit callback to get everything killed...including this zmap. */
  (*(zmap->zmap_cbs_G->quit_req))(zmap, zmap->app_data) ;

  return ;
}



static void print_hello( gpointer data, guint callback_action, GtkWidget *w )
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	GtkWidget *myWidget;
	printf( "widget is %x data is %s\n", w, data );
	g_message ("Hello, World!\n");

	myWidget = gtk_item_factory_get_widget (item_factory, "/File/New");
	printf( "File/New is %x\n", myWidget );

	gtk_item_factory_delete_item( item_factory, "/Edit" );
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

}

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void handle_option( gpointer data, guint callback_action, GtkWidget *w )
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	GtkCheckMenuItem *checkMenuItem = (GtkCheckMenuItem *) w;

	printf( "widget is %x data is %s\n", w, data );
	g_message ("Hello, World!\n");
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ALLOW_POPOUT_PANEL
static void popout_panel( gpointer data, guint callback_action, GtkWidget *w )
{
  GtkWidget *toplevel;
  ZMap zmap = (ZMap)data;

  if((toplevel = zMapGUIPopOutWidget(zmap->button_info_box, zmap->zmap_id)))
    gtk_widget_show_all(toplevel);

  return ;
}
#endif /* ALLOW_POPOUT_PANEL */


/* Load a new sequence by config file into a zmap. */
static void newSequenceByConfigCB(gpointer cb_data, guint callback_action, GtkWidget *w)
{
  ZMap zmap = (ZMap)cb_data ;

  zMapAppGetSequenceView(makeSequenceViewCB, zmap, NULL) ;

  return ;
}

/* Called once user has selected a new sequence. */
static void makeSequenceViewCB(ZMapFeatureSequenceMap seq_map, gpointer user_data)
{
  ZMap zmap = (ZMap)user_data ;
  ZMapView view ;
  char *err_msg = NULL ;

  if (!(view = zmapControlInsertView(zmap, seq_map, &err_msg)))
    {
      zMapWarning("%", err_msg) ;
      g_free(err_msg) ;
    }

  return ;
}



