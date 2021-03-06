/*  File: zmapControlWindow.cpp
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
 * Description: Creates the top level window of a ZMap.
 *
 * Exported functions: See zmapTopWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapCmdLineArgs.hpp>
#include <zmapControl_P.hpp>
#include <gbtools/gbtools.hpp>


static void setTooltips(ZMap zmap) ;
static void makeStatusTooltips(ZMap zmap) ;
static GtkWidget *makeStatusPanel(ZMap zmap) ;
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data) ;
static gboolean rotateTextCB(gpointer user_data) ;



/* 
 *                  Globals
 */




/* 
 *                  Package External routines.
 */


/* Makes the toplevel window and control panels for an individual zmap. */
gboolean zmapControlWindowCreate(ZMap zmap, GdkCursor *normal_cursor)
{
  gboolean result = FALSE ; 
  GtkWidget *toplevel, *vbox, *menubar, *frame, *controls_box, *button_box, *status_box,
    *info_panel_box, *info_box ;
  ZMapCmdLineArgsType shrink_arg = {FALSE} ;

  zMapReturnValIfFail(zmap, result) ; 


  /* Make tooltips groups for the main zmap controls and the feature information. */
  zmap->tooltips = gtk_tooltips_new() ;
  zmap->feature_tooltips = gtk_tooltips_new() ;

  zmap->toplevel = toplevel = zMapGUIToplevelNew(zMapGetZMapID(zmap), NULL) ;

  /* allow shrink for charlie'ss RT 215415, ref to GTK help: it says 'don't allow shrink' */
  if (zMapCmdLineArgsValue(ZMAPARG_SHRINK, &shrink_arg))
    zmap->shrinkable = shrink_arg.b ;
  gtk_window_set_policy(GTK_WINDOW(toplevel), zmap->shrinkable, TRUE, FALSE ) ;

  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
     GTK_SIGNAL_FUNC(toplevelDestroyCB), (gpointer)zmap) ;

  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(toplevel), vbox) ;

  zmap->menubar = menubar = zmapControlWindowMakeMenuBar(zmap) ;
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);

  frame = gtk_frame_new(NULL);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);

  zmap->button_info_box = controls_box = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(frame), controls_box) ;

  info_box = gtk_hbox_new(FALSE, 0) ;
  gtk_box_pack_start(GTK_BOX(controls_box), info_box, FALSE, TRUE, 0);

  button_box = zmapControlWindowMakeButtons(zmap) ;
  gtk_box_pack_start(GTK_BOX(info_box), button_box, FALSE, TRUE, 0);

  status_box = makeStatusPanel(zmap) ;
  gtk_box_pack_end(GTK_BOX(info_box), status_box, FALSE, TRUE, 0) ;


  //info_panel_box = zmapControlWindowMakeInfoPanel(zmap) ;
  info_panel_box = zmap->info_panel_vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(controls_box), info_panel_box, FALSE, FALSE, 0) ;

  zmap->navview_frame = zmapControlWindowMakeFrame(zmap) ;
  gtk_box_pack_start(GTK_BOX(vbox), zmap->navview_frame, TRUE, TRUE, 0);

  gtk_widget_show_all(toplevel) ;

  /* Now we can set the cursor. */
  zMapGUISetCursor(toplevel, normal_cursor) ;


  /* Tooltips can only be added to widgets after the widgets have been "shown". */
  setTooltips(zmap) ;
  gtk_tooltips_enable(zmap->tooltips) ;

  result = TRUE ; 


  return result ;
}



void zmapControlWindowDestroy(ZMap zmap)
{
  zMapReturnIfFail(zmap) ; 

  /* We must disconnect the "destroy" callback otherwise we will enter toplevelDestroyCB()
   * below and that will try to call our callers destroy routine which has already
   * called this routine...i.e. a circularity which results in attempts to
   * destroy already destroyed windows etc. */
  gtk_signal_disconnect_by_data(GTK_OBJECT(zmap->toplevel), (gpointer)zmap) ;

  gtk_widget_destroy(zmap->toplevel) ;

  gtk_object_destroy(GTK_OBJECT(zmap->tooltips)) ;
  gtk_object_destroy(GTK_OBJECT(zmap->feature_tooltips)) ;

  return ;
}



/* Called to update the user information sections of the zmapControl sections
 * of the display. Currently sets forward/revcomp status and coords.
 * 
 * Currently it uses a static idle_handle, I think this is ok even if we change
 * the focus_viewwindow because we will be resetting the text and then starting
 * to rotate it or stopping if the view is loaded.
 *  */
void zmapControlWindowSetStatus(ZMap zmap)
{
  char *status_text ;
  static int idle_handle = 0 ;
  enum {ROTATION_DELAY = 500} ;    /* delay in microseconds for each text move. */
  ZMapViewState view_state = ZMAPVIEW_INIT ;
  char *sources_loading = NULL, *sources_empty = NULL, *sources_failing = NULL ;

  zMapReturnIfFail(zmap) ; 

  switch(zmap->state)
    {
    case ZMAP_DYING:
      {
        status_text = g_strdup("ZMap closing.....") ;
        break ;
      }
    case ZMAP_VIEWS:
      {
        ZMapView view ;
        ZMapWindow window ;
        const char *strand_txt = NULL ;
        int start = 0, end = 0 ;
        char *coord_txt = NULL ;
        char *tmp ;

        view = zMapViewGetView(zmap->focus_viewwindow) ;
        window = zMapViewGetWindow(zmap->focus_viewwindow) ;

        view_state = zMapViewGetStatus(view) ;

        /* Set strand. */
        if (zMapViewGetRevCompStatus(view))
          strand_txt = " - " ;
        else
          strand_txt = " + " ;
        gtk_label_set_text(GTK_LABEL(zmap->status_revcomp), strand_txt) ;


        /* Set displayed start/end. */
        if (zmapWindowGetCurrentSpan(window, &start, &end))
          {
            coord_txt = g_strdup_printf(" %d  %d ", start, end) ;
            gtk_label_set_text(GTK_LABEL(zmap->status_coords), coord_txt) ;
            g_free(coord_txt) ;
          }

        tmp = zMapViewGetLoadStatusStr(view, &sources_loading, &sources_empty, &sources_failing) ;
        status_text = g_strdup_printf("%s.......", tmp) ;   /* Add spacing...better for rotating text. */
        g_free(tmp) ;

        break ;
      }
    case ZMAP_INIT:
    default:
      {
        status_text = g_strdup("No data.....") ;
        break ;
      }
    }


  if (zmap->state == ZMAP_VIEWS)
    {
      /* While we are loading we want the text to rotate but then we stop once
       * we've finished loading. */
      if (view_state == ZMAPVIEW_LOADED || view_state == ZMAPVIEW_LOADING || view_state == ZMAPVIEW_UPDATING)
        {
          if (sources_loading || sources_failing)
            {
              GString *load_status_str ;
        
              load_status_str = g_string_new("Selected View\n\n") ;
        
              if (sources_loading)
                g_string_append_printf(load_status_str, "Columns still loading:\n %s\n\n", sources_loading) ;

              if (sources_empty)
                g_string_append_printf(load_status_str, "Columns empty:\n %s\n\n", sources_empty) ;

              if (sources_failing)
                g_string_append_printf(load_status_str, "Columns failed to load:\n %s\n\n", sources_failing) ;

              gtk_tooltips_set_tip(zmap->tooltips, zmap->status_entry,
                                   load_status_str->str,
                                   "") ;

              g_string_free(load_status_str, TRUE) ;
              g_free(sources_loading) ;
              g_free(sources_empty) ;
              g_free(sources_failing) ;
            }
          else
            {
              gtk_tooltips_set_tip(zmap->tooltips, zmap->status_entry, /* reset. */
                                   "Status of selected view",
                                   "") ;
            }



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          if (view_state == ZMAPVIEW_LOADED)
            {
              /* hack to stop the timeout...seems to be no way of removing it even though we have a handle..... */
              rotateTextCB(zmap) ;
        
              idle_handle = 0 ;
            }
          else if (view_state == ZMAPVIEW_LOADING || view_state == ZMAPVIEW_UPDATING)
            {
              /* Start the timeout if it's not already going. */
              if (!idle_handle)
                idle_handle = g_timeout_add(ROTATION_DELAY, rotateTextCB, zmap->status_entry) ;
            }
        #endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
        
          /* Start the timeout if it's not already going. */
          if ((view_state == ZMAPVIEW_LOADING || view_state == ZMAPVIEW_UPDATING)
              && !idle_handle)
            {
                idle_handle = g_timeout_add(ROTATION_DELAY, rotateTextCB, zmap) ;
            }
        }
              else
        {
          gtk_tooltips_set_tip(zmap->tooltips, zmap->status_entry, /* reset. */
                               "Status of selected view",
                               "") ;
        }


    }

  /* Update the status with the next text. */
  gtk_entry_set_text(GTK_ENTRY(zmap->status_entry), status_text) ;
  g_free(status_text) ;

  return ;
}



/* Users have asked us to make the zmap window as big as possible vertically, the horizontal size
 * is not usually an issue, we can set it small and then gtk will expand it to accomodate all the
 * buttons etc along the top of the window which results in a good horizontal size. The vertical
 * size is more tricky as lots of window managers provide tool bars etc which take up screen
 * space. In addition the user may have multiple actual monitors configured as one large
 * virtual screen. gbtools::GUIGetTrueMonitorSize() attempts to encapsulate all this.
 * 
 * Note that we set the size of width small, it will be expanded automatically as widgets
 * are added to display stuff like the info panel.
 *
 */
void zmapControlWindowMaximize(GtkWidget *widget, ZMap zmap)
{
  GtkWidget *toplevel = widget ;
  int window_width_guess = 300, window_height_guess ;

  gbtools::GUIGetTrueMonitorSize(toplevel, NULL, &window_height_guess) ;

  gtk_window_resize(GTK_WINDOW(toplevel), window_width_guess, window_height_guess) ;

  return ;
}







/*
 *                      Internal functions
 */


/* this panel displays revcomp, sequence coord and status information for
 * the selected view.
 *
 * Note that the labels have to be parented by an event box because we want
 * them to have tooltips which require a window in order to work and this
 * is provided by the event box. */
static GtkWidget *makeStatusPanel(ZMap zmap)
{
  GtkWidget *status_box = NULL , *frame, *event_box ;

  zMapReturnValIfFail(zmap, status_box) ; 

  status_box = gtk_hbox_new(FALSE, 0) ;

  frame = gtk_frame_new(NULL) ;
  gtk_box_pack_start(GTK_BOX(status_box), frame, TRUE, TRUE, 0) ;
  event_box = gtk_event_box_new() ;
  gtk_container_add(GTK_CONTAINER(frame), event_box) ;
  zmap->status_revcomp = gtk_label_new(NULL) ;
  gtk_container_add(GTK_CONTAINER(event_box), zmap->status_revcomp) ;

  frame = gtk_frame_new(NULL) ;
  gtk_box_pack_start(GTK_BOX(status_box), frame, TRUE, TRUE, 0) ;
  event_box = gtk_event_box_new() ;
  gtk_container_add(GTK_CONTAINER(frame), event_box) ;
  zmap->status_coords = gtk_label_new(NULL) ;
  gtk_container_add(GTK_CONTAINER(event_box), zmap->status_coords) ;

  frame = gtk_frame_new(NULL) ;
  gtk_box_pack_start(GTK_BOX(status_box), frame, TRUE, TRUE, 0) ;
  zmap->status_entry = gtk_entry_new();
  gtk_container_add(GTK_CONTAINER(frame), zmap->status_entry) ;

  return status_box ;
}



/* Called when a destroy signal has been sent to the top level window/widget,
 * this may have been by the user clicking a button or maybe by zmapControl
 * code. */
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data)
{
  ZMap zmap = NULL ; 
  zMapReturnIfFail(cb_data) ; 
  zmap = (ZMap)cb_data ;
  GList *destroyed_views = NULL ;
  
  /* This function is called when gtk has sent a destroy signal to our toplevel window,
   * setting this to NULL ensures that we don't try to destroy the window again. */
  zmap->toplevel = NULL ;

  zmapControlDoKill(zmap, &destroyed_views) ;

  /* Need to tell peer (if there is one) that all is destroyed.... */
  if (zmap->remote_control)
    zmapControlSendViewDeleted(zmap, destroyed_views) ;

  return ;
}



/* Note that tool tips cannot be set on a widget until that widget has a window,
 * so this routine gets called after the various widgets have been realised.
 * This is a bit of pity as it means we end up splitting creation of widgets from
 * creation of their tooltips. */
static void setTooltips(ZMap zmap)
{

  zmapControlButtonTooltips(zmap) ;

  makeStatusTooltips(zmap) ;


  return ;
}



static void makeStatusTooltips(ZMap zmap)
{
  zMapReturnIfFail(zmap) ; 

  gtk_tooltips_set_tip(zmap->tooltips, gtk_widget_get_parent(zmap->status_revcomp),
                       "\"+\" = forward complement,\n \"-\"  = reverse complement",
                       "") ;

  gtk_tooltips_set_tip(zmap->tooltips, gtk_widget_get_parent(zmap->status_coords),
                       "start/end coords of displayed sequence",
                       "") ;

  gtk_tooltips_set_tip(zmap->tooltips, zmap->status_entry,
                       "Status of selected view",
                       "") ;

  return ;
}




/* Called at intervals and rotates the text in the entry widget given by user_data
 * round and round and round.
 * 
 * Note that glib does not provide a way to remove a timeout handler so we
 * hack this by removing ourselves if we are passed a null pointer.
 *  */
static gboolean rotateTextCB(gpointer user_data)
{
  gboolean call_again = TRUE ;    /* Keep calling us. */
  ZMap zmap = NULL ; 

  zMapReturnValIfFail(user_data, call_again) ; 

  zmap = (ZMap)user_data ;

  if (zmap->state == ZMAP_DYING)
    {
      call_again = FALSE ;    /* Stop calling us. */
    }
  else
    {
      static GString *buffer = NULL ;
      ZMapView view ;
      ZMapViewState view_state ;
      GtkWidget *entry_widg ;
      char *entry_text ;

      if (!buffer)
        buffer = g_string_sized_new(1000) ;

      view = zMapViewGetView(zmap->focus_viewwindow) ;

      if (view)
        {
          view_state = zMapViewGetStatus(view) ;

          entry_widg = zmap->status_entry ;
          if ((entry_text = (char *)gtk_entry_get_text(GTK_ENTRY(entry_widg))))
            {
              buffer = g_string_assign(buffer, (entry_text + 1)) ;
              buffer = g_string_append_c(buffer, *entry_text) ;

              gtk_entry_set_text(GTK_ENTRY(entry_widg), buffer->str) ;
            }

          if (view_state == ZMAPVIEW_LOADED)
            call_again = FALSE ;    /* Stop calling us. */
        }
    }

  return call_again ;
}
