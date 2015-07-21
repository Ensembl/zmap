/*  File: zmapControlWindowMenubar.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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

#include <ZMap/zmap.hpp>

#include <stdlib.h>
#include <stdio.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapUtilsGUI.hpp>
#include <ZMap/zmapAppServices.hpp>
#include <zmapControl_P.hpp>


typedef enum {EDIT_COPY, EDIT_COPY_CHR, EDIT_PASTE} EditActionType ;

typedef enum {RT_INVALID, RT_ACEDB, RT_ANACODE, RT_SEQTOOLS, RT_ZMAP, RT_ZMAP_USER_TICKETS} RTQueueName ;

typedef enum {
  EXPORT_DNA,
  /*EXPORT_CONTEXT,*/
  EXPORT_FEATURES_ALL,
  EXPORT_FEATURES_MARKED,
  /*EXPORT_FEATURES_SELECTED,*/
  SAVE_FEATURES,
  SAVE_FEATURES_AS
} ExportType ;


static void newSequenceByConfigCB(gpointer cb_data, guint callback_action, GtkWidget *w) ;
static void makeSequenceViewCB(ZMapFeatureSequenceMap sequence_map, gpointer user_data) ;
static void closeSequenceDialogCB(GtkWidget *toplevel, gpointer user_data) ;
static void closeCB(gpointer cb_data, guint callback_action, GtkWidget *w) ;
static void quitCB(gpointer cb_data, guint callback_action, GtkWidget *w) ;

static void importCB(gpointer cb_data, guint callback_action, GtkWidget *window);
static void exportCB(gpointer cb_data, guint callback_action, GtkWidget *window);
static void printCB(gpointer cb_data, guint callback_action, GtkWidget *w);
static void dumpCB(gpointer cb_data, guint callback_action, GtkWidget *w);
static void redrawCB(gpointer cb_data, guint callback_action, GtkWidget *w);
static void preferencesCB(gpointer cb_data, guint callback_action, GtkWidget *w);
static void developerCB(gpointer cb_data, guint callback_action, GtkWidget *w);
static void showSessionCB(gpointer cb_data, guint callback_action, GtkWidget *window) ;
static void aboutCB(gpointer cb_data, guint callback_action, GtkWidget *w);
static void rtTicket(gpointer cb_data, guint callback_action, GtkWidget *w);
static void allHelpCB(gpointer cb_data, guint callback_action, GtkWidget *w);
static void copyPasteCB( gpointer data, guint callback_action, GtkWidget *w ) ;
static void toggleDisplayCoordinatesCB (gpointer data, guint callback_action, GtkWidget *w) ;
#ifdef ALLOW_POPOUT_PANEL
static void popout_panel( gpointer data, guint callback_action, GtkWidget *w ) ;
#endif /* ALLOW_POPOUT_PANEL */

GtkItemFactory *item_factory;


static GtkItemFactoryEntry menu_items[] = {
  { (char*)"/_File",                                NULL,                NULL,                              0,  (char*)"<Branch>" },
         { (char*)"/File/_New Sequence",                   NULL,                G_CALLBACK(newSequenceByConfigCB), 2,  NULL },
         { (char*)"/File/sep1",                            NULL,                NULL,                              0,  (char*)"<Separator>" },
         { (char*)"/File/_Save",                           (char*)"<control>S", G_CALLBACK(exportCB),              SAVE_FEATURES, NULL },
         { (char*)"/File/Save _As",                        (char*)"<shift><control>S", G_CALLBACK(exportCB),       SAVE_FEATURES_AS, NULL },
         { (char*)"/File/sep1",                            NULL,                NULL,                              0,  (char*)"<Separator>" },
         { (char*)"/File/_Import",                         (char*)"<control>I", G_CALLBACK(importCB),              0,  NULL },/* or Read ? */
         { (char*)"/File/_Export",                         NULL,                NULL,                              0,  (char*)"<Branch>" },
         /*{ (char*)"/File/Export/_Data",                      NULL,                NULL,                  0,  "<Branch>" }, */
         { (char*)"/File/Export/_DNA",                     NULL,                G_CALLBACK(exportCB),              EXPORT_DNA,  NULL },
         { (char*)"/File/Export/_Features",                (char*)"<control>E", G_CALLBACK(exportCB),              EXPORT_FEATURES_ALL,  NULL },
         { (char*)"/File/Export/_Features (marked)",       (char*)"<shift><control>E", G_CALLBACK(exportCB),              EXPORT_FEATURES_MARKED, NULL },
         /*{ "/File/Export/_Features (selected)",     NULL,                exportCB,              EXPORT_FEATURES_SELECTED, NULL },*/
         /* { (char*)"/File/Export/_Context",              NULL,                exportCB,              3,  NULL },
         { (char*)"/File/Export/_Marked Features",         NULL,                NULL,                  0,  (char*)"<Branch>" },
         { (char*)"/File/Export/Marked Features/_DNA",     NULL,                exportCB,              1,  NULL },
         { (char*)"/File/Export/Marked Features/_Context", NULL,                exportCB,              3,  NULL }, */
         { (char*)"/File/sep1",                            NULL,                NULL,                              0,  (char*)"<Separator>" },
         { (char*)"/File/Save screen sho_t",               NULL,                G_CALLBACK(dumpCB),                0,  NULL },
         { (char*)"/File/_Print screen shot",              (char*)"<control>P", G_CALLBACK(printCB),               0,  NULL },
         { (char*)"/File/sep1",                            NULL,                NULL,                              0,  (char*)"<Separator>" },
         { (char*)"/File/Close",                           (char*)"<control>W", G_CALLBACK(closeCB),               0,  NULL },
         { (char*)"/File/Quit",                            (char*)"<control>Q", G_CALLBACK(quitCB),                0,  NULL },

    { (char*)"/_Edit",                                NULL,                NULL,                              0,  (char*)"<Branch>" },
    { (char*)"/Edit/_Copy Feature Coords",            (char*)"<control>C", G_CALLBACK(copyPasteCB),           EDIT_COPY, NULL },
    { (char*)"/Edit/_UCopy Feature Coords (CHR)",     (char*)"<control>U", G_CALLBACK(copyPasteCB),           EDIT_COPY_CHR, NULL },
    { (char*)"/Edit/_Paste Feature Coords",           (char*)"<control>V", G_CALLBACK(copyPasteCB),           EDIT_PASTE, NULL },

    { (char*)"/Edit/_Redraw",                NULL, G_CALLBACK(redrawCB),       0, NULL },
    { (char*)"/Edit/sep1",                   NULL, NULL,                       0, (char*)"<Separator>" },
    { (char*)"/Edit/P_references",           NULL, G_CALLBACK(preferencesCB),  0, NULL },
    { (char*)"/Edit/_Set Developer status",  NULL, G_CALLBACK(developerCB),    0, NULL },

    { (char*)"/_View",                       NULL, NULL,                       0, (char*)"<Branch>" },
    { (char*)"/View/Session Details",        NULL, G_CALLBACK(showSessionCB),  0, NULL },
    { (char*)"/View/Toggle coords", NULL, G_CALLBACK(toggleDisplayCoordinatesCB), 0, NULL }, /* change between chromosome and slice coordinates */

#ifdef ALLOW_POPOUT_PANEL
    { (char*)"/View/'Pop Out' Control Info Panel", NULL, popout_panel, 0, NULL },
#endif/* ALLOW_POPOUT_PANEL */

    { (char*)"/_Raise ticket",                               NULL, NULL,                 0, (char*)"<LastBranch>" },
    { (char*)"/Raise ticket/See ZMap tickets",               NULL, G_CALLBACK(rtTicket), RT_ZMAP_USER_TICKETS, NULL },
    { (char*)"/Raise ticket/ZMap ticket",                    NULL, G_CALLBACK(rtTicket), RT_ZMAP, NULL },
    { (char*)"/Raise ticket/Anacode ticket",                 NULL, G_CALLBACK(rtTicket), RT_ANACODE, NULL },
    { (char*)"/Raise ticket/Blixem, Dotter or Belvu ticket", NULL, G_CALLBACK(rtTicket), RT_SEQTOOLS, NULL },
    { (char*)"/Raise ticket/Acedb ticket",                   NULL, G_CALLBACK(rtTicket), RT_ACEDB, NULL },

    { (char*)"/_Help",         NULL,         NULL, 0, (char*)"<LastBranch>" },
    /*{ (char*)"/Help/General Help", NULL,     allHelpCB, ZMAPGUI_HELP_GENERAL, NULL }, */
    { (char*)"/Help/Quick Start Guide", NULL, G_CALLBACK(allHelpCB), ZMAPGUI_HELP_QUICK_START, NULL },
    { (char*)"/Help/Keyboard & Mouse", NULL,  G_CALLBACK(allHelpCB), ZMAPGUI_HELP_KEYBOARD, NULL },
    { (char*)"/Help/Alignment Display", NULL, G_CALLBACK(allHelpCB), ZMAPGUI_HELP_ALIGNMENT_DISPLAY, NULL },
    { (char*)"/Help/Release Notes", NULL,     G_CALLBACK(allHelpCB), ZMAPGUI_HELP_RELEASE_NOTES, NULL },
    { (char*)"/Help/About ZMap",    NULL,     G_CALLBACK(aboutCB), 0, NULL }
  };




GtkWidget *zmapControlWindowMakeMenuBar(ZMap zmap)
{
  GtkWidget *menubar = NULL ;
  GtkAccelGroup *accel_group ;
  gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]) ;
  zMapReturnValIfFail(zmap, menubar) ;

  accel_group = gtk_accel_group_new() ;

  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group) ;

  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, (gpointer)zmap) ;

  gtk_window_add_accel_group(GTK_WINDOW(zmap->toplevel), accel_group) ;

  menubar = gtk_item_factory_get_widget (item_factory, "<main>") ;

  return menubar ;
}


/*
 * Function to toggle displpay coordinates... a bit experimental.
 */
static void toggleDisplayCoordinatesCB (gpointer data, guint callback_action, GtkWidget *w)
{
  ZMap zmap = NULL ;
  ZMapView current_view = NULL ;
  zMapReturnIfFail(data) ;

  zmap = (ZMap) data ;
  current_view = zMapViewGetView(zmap->focus_viewwindow) ;

  /*
   * Now call appropriate function for View to run over all windows...
   */
  if (current_view)
    {
      zMapViewToggleDisplayCoordinates(current_view) ;
    }
}


/* Should pop up a dialog box to ask for a file name....e.g. the file chooser. */
static void exportCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  ZMap zmap = NULL ;
  zMapReturnIfFail(cb_data) ;
  ZMapWindow curr_window = NULL ;
  ZMapView curr_view = NULL ;
  GError *error = NULL ;
  gboolean result = FALSE ;

  zmap = (ZMap)cb_data ;
  if (!zmap)
    return ;

  curr_window = zMapViewGetWindow(zmap->focus_viewwindow) ;
  curr_view = zMapViewGetView(zmap->focus_viewwindow) ;

  switch (callback_action)
    {
    /* Export DNA */
    case EXPORT_DNA:
      {
        result = zMapWindowExportFASTA(curr_window, NULL, &error) ;
        break ;
      }

    /* Export all features */
    case EXPORT_FEATURES_ALL:
      {
        result = zMapWindowExportFeatures(curr_window, TRUE, FALSE, NULL, NULL, &error) ;
        break ;
      }

    /* Export features for marked region */
    case EXPORT_FEATURES_MARKED:
      {
        result = zMapWindowExportFeatures(curr_window, TRUE, TRUE, NULL, NULL, &error) ;
        break ;
      }

    /* todo: Export selected features */
    //case EXPORT_FEATURES_SELECTED:
    //  {
    //    result = zMapWindowExportFeatures(curr_window, FALSE, TRUE, NULL, NULL, &error) ;
    //    break ;
    //  }

    /* Export context; (sm23) I've removed this since it's pointless for the
     * user.
     */
    /*case 3:
      {
        result = zMapWindowExportContext(curr_window, &error) ;
        break ;
      }*/

    /* Save - same as export all features but we pass the existing filename, if there is one;
     * if there isn't then the file chooser will be shown and we'll save the file the user
     * selects for future Save operations. */
    case SAVE_FEATURES:
      {
        char *filename = g_strdup(zMapViewGetSaveFile(curr_view, TRUE)) ;
        result = zMapWindowExportFeatures(curr_window, TRUE, FALSE, NULL, &filename, &error) ;

        if (result)
          zMapViewSetSaveFile(curr_view, filename) ;

        if (filename)
          g_free(filename) ;

        break ;
      }

    /* Save As - same as export all features but we save the filename for the next Save operation */
    case SAVE_FEATURES_AS:
      {
        char *filename = NULL ;
        result = zMapWindowExportFeatures(curr_window, TRUE, FALSE, NULL, &filename, &error) ;

        if (result)
          zMapViewSetSaveFile(curr_view, filename) ;

        if (filename)
          g_free(filename) ;

        break ;
      }

    default:
      {
        break ;
      }
    }

  /* We can get result==FALSE if the user cancelled in which case the error is not set, so don't
   * report an error in that case */
  if (error)
    {
      zMapWarning("Export failed: %s", error->message) ;
      g_error_free(error) ;
      error = NULL ;
    }

  return ;
}



static void controlImportFileCB(gpointer user_data)
{
  zMapWarning("controlImportFileCB not implemented","");
  /* this is a callback to report something */

  return ;
}


static void importCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  ZMap zmap = (ZMap)cb_data ;
  ZMapViewWindow vw ;
  ZMapFeatureSequenceMap sequence_map ;
  ZMapFeatureSequenceMap view_seq ;
  int start, end ;

  zMapReturnIfFail(zmap && zmap->focus_viewwindow) ;

  vw = zmap->focus_viewwindow ;

  view_seq = zMapViewGetSequenceMap( zMapViewGetView(vw) );

  /* get view sequence and coords */
  sequence_map = g_new0(ZMapFeatureSequenceMapStruct,1);
  sequence_map->start = view_seq->start;
  sequence_map->end = view_seq->end;
  sequence_map->sequence = view_seq->sequence;
  sequence_map->dataset = view_seq->dataset ;
  sequence_map->config_file = view_seq->config_file;

  /* limit to mark if set */
  start = view_seq->start;
  end   = view_seq->end;

  if(zMapWindowMarkIsSet(zMapViewGetWindow(vw)))
    {
      zMapWindowGetMark(zMapViewGetWindow(vw), &start, &end);

      /* NOTE we get -fwd coords from this function if revcomped */
      if(start < 0)
        start = -start ;
      if(end < 0)
        end = -end ;

      start += sequence_map->start ;
      end += sequence_map->start ;
    }

  /* need sequence_map to set default seq coords and map sequence name */
  zMapControlImportFile(controlImportFileCB, cb_data, sequence_map, start, end);

  return ;
}


static void dumpCB(gpointer cb_data, guint callback_action, GtkWidget *widget)
{
  ZMap zmap = NULL ;
  zMapReturnIfFail(cb_data) ;
  zmap = (ZMap)cb_data ;
  zMapReturnIfFail(zmap && zmap->focus_viewwindow) ;
  ZMapWindow window ;

  window = zMapViewGetWindow(zmap->focus_viewwindow) ;

  zMapWindowDump(window) ;

  return ;
}


static void printCB(gpointer cb_data, guint callback_action, GtkWidget *widget)
{
  ZMap zmap = NULL ;
  zMapReturnIfFail(cb_data) ;
  zmap = (ZMap)cb_data ;
  zMapReturnIfFail(zmap && zmap->focus_viewwindow) ;
  ZMapWindow window ;

  window = zMapViewGetWindow(zmap->focus_viewwindow) ;

  zMapWindowPrint(window) ;

  return ;
}


/* Causes currently focussed zmap window to redraw itself. */
static void redrawCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  ZMap zmap = NULL ;
  zMapReturnIfFail(cb_data) ;
  zmap = (ZMap)cb_data ;
  zMapReturnIfFail(zmap && zmap->focus_viewwindow) ;

  zMapViewRedraw(zmap->focus_viewwindow) ;

  return ;
}


/* Shows preference edit window. */
static void preferencesCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  ZMap zmap = NULL ;
  zMapReturnIfFail(cb_data) ;
  zmap = (ZMap)cb_data ;

  zmapControlShowPreferences(zmap) ;

  return ;
}


/* Shows developer status dialog window. */
static void developerCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  ZMap zmap = NULL ;
  zMapReturnIfFail(cb_data) ;
  zmap = (ZMap)cb_data ;
  zMapReturnIfFail(zmap) ;
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


/* Display session data, this is a mixture of machine and per view data. */
static void showSessionCB(gpointer cb_data, guint callback_action, GtkWidget *window)
{
  ZMap zmap = NULL ;
  zMapReturnIfFail(cb_data) ;
  zmap = (ZMap)cb_data ;
  zMapReturnIfFail(zmap) ;
  GString *session_text ;

  session_text = g_string_new(NULL) ;

  g_string_append(session_text, "General\n") ;
  g_string_append_printf(session_text, "\tProgram: %s\n\n", zMapGetAppTitle()) ;
  g_string_append_printf(session_text, "\tUser: %s (%s)\n\n", g_get_user_name(), g_get_real_name()) ;
  g_string_append_printf(session_text, "\tMachine: %s\n\n", g_get_host_name()) ;

  if (!zMapViewSessionGetAsText(zmap->focus_viewwindow, session_text))
    g_string_append_printf(session_text, "\n\t<< %s >>\n\n", "No sessions currently.") ;

  zMapGUIShowText("Session Details", session_text->str, FALSE) ;

  g_string_free(session_text, TRUE) ;

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
  static const char *url_raise_ticket_base = "https://rt.sanger.ac.uk/rt/Ticket/Create.html?Queue=" ;
  char *web_page = NULL ;
  gboolean result ;
  GError *error = NULL ;
  RTQueueName queue_name = (RTQueueName)callback_action ;
  int queue_number = 0 ;

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
      zMapWarnIfReached() ;
      break ;
    }

  if (raise_ticket && queue_number)
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
  zMapReturnIfFail(zmap) ;

  zmapControlSignalKill(zmap) ;

  return ;
}



/* Kill the whole zmap application. */
static void quitCB(gpointer cb_data, guint callback_action, GtkWidget *w)
{
  ZMap zmap = (ZMap)cb_data ;
  zMapReturnIfFail(zmap && zmap->zmap_cbs_G) ;

  /* Call the application exit callback to get everything killed...including this zmap. */
  (*(zmap->zmap_cbs_G->quit_req))(zmap, zmap->app_data) ;

  return ;
}



static void copyPasteCB(gpointer cb_data, guint callback_action, GtkWidget *w)
{
  ZMap zmap = (ZMap)cb_data ;
  EditActionType action = (EditActionType)callback_action ;
  ZMapWindow curr_window ;

  zMapReturnIfFail(zmap && zmap->focus_viewwindow) ;

  curr_window = zMapViewGetWindow(zmap->focus_viewwindow) ;

  switch (action)
    {
    case EDIT_COPY:
    case EDIT_COPY_CHR:
      {
        char *selection_text ;
        ZMapWindowDisplayStyleStruct display_style = {ZMAPWINDOW_COORD_NATURAL, ZMAPWINDOW_PASTE_FORMAT_BROWSER,
                                                      ZMAPWINDOW_PASTE_TYPE_SELECTED} ;

        if (action == EDIT_COPY_CHR)
          display_style.paste_style = ZMAPWINDOW_PASTE_FORMAT_BROWSER_CHR ;

        if ((selection_text = zMapWindowGetSelectionText(curr_window, &display_style)))
          {
            /* Set on both common X clipboards. */
            zMapGUISetClipboard(zmap->toplevel, GDK_SELECTION_PRIMARY, selection_text) ;
            zMapGUISetClipboard(zmap->toplevel, GDK_SELECTION_CLIPBOARD, selection_text) ;
          }

        break ;
      }
    case EDIT_PASTE:
      {
        zMapWindowZoomFromClipboard(curr_window) ;
        break ;
      }

    default:
      {
        zMapWarnIfReached() ;
      }
    }

  return ;
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
  ZMap zmap = NULL ;
  zMapReturnIfFail(data) ;
  zmap = (ZMap)data;

  if((toplevel = zMapGUIPopOutWidget(zmap->button_info_box, zmap->zmap_id)))
    gtk_widget_show_all(toplevel);

  return ;
}
#endif /* ALLOW_POPOUT_PANEL */


/* Load a new sequence by config file into a zmap. */
static void newSequenceByConfigCB(gpointer cb_data, guint callback_action, GtkWidget *w)
{
  ZMap zmap = NULL ;

  zMapReturnIfFail(cb_data) ;

  zmap = (ZMap)cb_data ;
  zMapReturnIfFail(zmap) ;

  if (!(zmap->sequence_dialog))
    zmap->sequence_dialog = zMapAppGetSequenceView(makeSequenceViewCB, zmap, closeSequenceDialogCB, zmap,
                                                   zmap->default_sequence, FALSE) ;

  return ;
}

/* Called once user has selected a new sequence. */
static void makeSequenceViewCB(ZMapFeatureSequenceMap seq_map, gpointer user_data)
{
  ZMap zmap = (ZMap)user_data ;
  ZMapView view = NULL ;
  char *err_msg = NULL ;
  zMapReturnIfFail(zmap) ;

  if ((view = zMapControlInsertView(zmap, seq_map, &err_msg)))
    {
      ZMapCallbacks zmap_cbs_G ;

      zmap_cbs_G = zmapControlGetCallbacks() ;

      (*(zmap_cbs_G->view_add))(zmap, view, zmap->app_data) ;
    }
  else
    {
      zMapWarning("%s", err_msg) ;
      g_free(err_msg) ;
    }

  return ;
}


static void closeSequenceDialogCB(GtkWidget *toplevel, gpointer user_data)
{
  ZMap zmap = (ZMap)user_data ;
  zMapReturnIfFail(zmap) ;

  zmap->sequence_dialog = NULL ;

  return ;
}
