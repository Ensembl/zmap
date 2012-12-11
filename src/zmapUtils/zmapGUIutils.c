/*  Last edited: Nov  3 11:55 2011 (edgrif) */
/*  File: zmapGUIutils.c
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
 *        Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and,
 *          Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Various GUI convenience functions for messages, text,
 *              notebook creation and so on.
 *
 * Exported functions: See ZMap/zmapUtilsGUI.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>
#include <gdk/gdkx.h>					    /* For X Windows stuff. */
#include <gtk/gtk.h>
#include <math.h>

#include <ZMap/zmapUtilsGUI.h>
#include <ZMap/zmapWebPages.h>
#include <zmapUtils_P.h>


/* Because we need to cast to the function type to call it without debugger complaints... */
typedef void (*g_object_notify_callback)(GObject *pane, GParamSpec *scroll, gpointer user_data);

/* data for our proxying */
typedef struct
{
  g_object_notify_callback callback;
  gpointer user_data;
  gulong handler_id;
} PaneMaxPositionStruct, *PaneMaxPosition;



typedef struct
{
  int value;
  int *output;
  ZMapGUIRadioButtonCBFunc callback;
  gpointer user_data;
}RadioButtonCBDataStruct, *RadioButtonCBData;

typedef struct
{
  GtkWidget *original_parent;
  GtkWidget *popout_child;
  GtkWidget *popout_toplevel;
  gulong     original_parent_signal_id;
} PopOutDataStruct, *PopOutData;


typedef struct CursorNameStructName
{
  GdkCursorType cursor_id ;
  char *cursor_name ;
} CursorNameStruct, *CursorName ;


typedef struct TextAttrsStructName
{
  GtkTextBuffer *buffer ;
} TextAttrsStruct, *TextAttrs ;



/*
 * For printing out events.
 */

/* Irritatingly the 'time' field is in different places in different events or even completely
 * absent, these structs deal with getting hold of it....obviously they need to be kept in step
 * with the gdk event structs but they won't be changing much ! */
typedef enum {EVENT_NO_TIME, EVENT_COMMON_TIME,
	      EVENT_CROSSING_TIME, EVENT_ATOM_TIME,
	      EVENT_SELECTION_TIME, EVENT_DND_TIME, EVENT_OWNER_TIME} TimeStuctType ;

typedef struct EventCommonTimeStructName
{
  GdkEventType type;
  GdkWindow *window;
  gint8 send_event;
  guint32 time;
} EventCommonTimeStruct, *EventCommonTime ;


/* Descriptor struct for handling producing text description of events. */
typedef struct eventTxtStructName
{
  char* text ;
  GdkEventMask mask ;
  TimeStuctType time_struct_type ;
} eventTxtStruct, *eventTxt ;









static gboolean modalFromMsgType(ZMapMsgType msg_type) ;
static GtkResponseType messageFull(GtkWindow *parent, char *title_in, char *msg,
				   gboolean modal, int display_timeout, gboolean close_button,
				   ZMapMsgType msg_type, GtkJustification justify,
				   ZMapGUIMsgUserData user_data) ;
static void butClick(GtkButton *button, gpointer user_data) ;
static gboolean timeoutHandlerModal(gpointer data) ;
static gboolean timeoutHandler(gpointer data) ;

static void responseCB(GtkDialog *toplevel, gint arg1, gpointer user_data) ;

static void pane_max_position_callback(GObject *pane, GParamSpec *scroll, gpointer user_data);
static void pane_max_position_destroy_notify(gpointer pane_max_pos_data, GClosure *unused_closure);

static void handle_popout_destroy_cb(GtkWidget *toplevel, gpointer cb_data) ;
static void handle_original_parent_destroy_cb(GtkWidget *widget, gpointer cb_data) ;

static void radioButtonCB(GtkWidget *button, gpointer radio_data) ;
static void radioButtonCBDataDestroy(gpointer data) ;

static GdkCursor *makeCustomCursor(char *cursor_name) ;
static GdkCursor *makeStandardCursor(char *cursor_name) ;

static void setTextAttrs(gpointer data, gpointer user_data) ;

static void aboutLinkOldCB(GtkAboutDialog *about, const gchar *link, gpointer data) ;
static gboolean aboutLinkNewCB(GtkAboutDialog *label, gchar *uri, gpointer user_data_unused) ;




/* Holds an alternative URL for help pages if set by the application. */
static char *help_URL_base_G = NULL ;


/* If TRUE then window title prefix is abbreviated. */
static gboolean abbreviated_title_prefix_G = FALSE ;






/*! @defgroup zmapguiutils   zMapGUI: set of utility functions for use in a GUI.
 * @{
 *
 * \brief  GUI utility functions.
 *
 * zMapGUI routines provide some basic GUI functions many of which encapsulate
 * tricky bits of coding in GTK. The functions do things like display messages
 * (modal and not modal), text in a text widget, find fonts and font sizes etc.
 *
 *  */



/*!
 * Raises a toplevel widget to top.
 *
 * @param widget     The widget to raise.
 * @return           nothing
 */
void zMapGUIRaiseToTop(GtkWidget *widget)
{
  GdkWindow *window;

  if(((GTK_WIDGET_NO_WINDOW(widget)) &&
      (window = gtk_widget_get_parent_window(widget))) ||
     (window = widget->window))
    {
      gdk_window_raise(window);
      gdk_window_set_keep_above(window, FALSE);
    }

  return ;
}


/*!
 * Find GtkWindow (i.e. ultimate) parent of given widget.
 * Can return NULL if widget is not part of a proper widget tree.
 *
 * @param widget     The child widget.
 * @return           The window parent or NULL.
 */
GtkWidget *zMapGUIFindTopLevel(GtkWidget *widget)
{
  GtkWidget *toplevel = widget ;

  zMapAssert(toplevel && GTK_IS_WIDGET(toplevel)) ;

  while (toplevel && !GTK_IS_WINDOW(toplevel))
    {
      toplevel = gtk_widget_get_parent(toplevel) ;
    }

  return toplevel ;
}



/* Given a gdkEventAny, return a string name for the event.
 * 
 * NOTE, the other event masks need filling in, I've only done the ones I'm interested in...
 * 
 * If exclude_mask is zero all events are processed, otherwise events to be excluded should
 * be specified like this:
 *
 * static GdkEventMask msg_exclude_mask_G = (GDK_POINTER_MOTION_MASK | GDK_EXPOSURE_MASK
 *                                           | GDK_FOCUS_CHANGE_MASK | GDK_VISIBILITY_NOTIFY_MASK
 *					     | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK) ;
 *
 */
char *zMapGUIGetEventAsText(GdkEventMask exclude_mask, GdkEventAny *any_event)
{
  char *event_as_text = NULL ;
  int true_index ;
  eventTxtStruct event_txt[] =
    {
      {"GDK_NOTHING", 0, EVENT_COMMON_TIME},				    /* = -1 */
      {"GDK_DELETE", 0, EVENT_COMMON_TIME},				    /* = 0 */
      {"GDK_DESTROY", 0, EVENT_COMMON_TIME},				    /* = 1 */
      {"GDK_EXPOSE", GDK_EXPOSURE_MASK, EVENT_NO_TIME},		    /* = 2 */

      {"GDK_MOTION_NOTIFY", GDK_POINTER_MOTION_MASK, EVENT_COMMON_TIME},	    /* = 3 */
      {"GDK_BUTTON_PRESS", 0, EVENT_COMMON_TIME},				    /* = 4 */
      {"GDK_2BUTTON_PRESS", 0, EVENT_COMMON_TIME},				    /* = 5 */
      {"GDK_3BUTTON_PRESS", 0, EVENT_COMMON_TIME},				    /* = 6 */
      {"GDK_BUTTON_RELEASE", 0, EVENT_COMMON_TIME},			    /* = 7 */

      {"GDK_KEY_PRESS", 0, EVENT_COMMON_TIME},				    /* = 8 */
      {"GDK_KEY_RELEASE", 0, EVENT_COMMON_TIME},				    /* = 9 */

      {"GDK_ENTER_NOTIFY", GDK_ENTER_NOTIFY_MASK, EVENT_CROSSING_TIME},	    /* = 10 */
      {"GDK_LEAVE_NOTIFY", GDK_LEAVE_NOTIFY_MASK, EVENT_CROSSING_TIME},	    /* = 11 */

      {"GDK_FOCUS_CHANGE", GDK_FOCUS_CHANGE_MASK, EVENT_NO_TIME},	    /* = 12 */
      {"GDK_CONFIGURE", 0, EVENT_NO_TIME},				    /* = 13 */

      {"GDK_MAP", 0, EVENT_COMMON_TIME},					    /* = 14 */
      {"GDK_UNMAP", 0, EVENT_COMMON_TIME},					    /* = 15 */

      {"GDK_PROPERTY_NOTIFY", 0, EVENT_ATOM_TIME},			    /* = 16 */

      {"GDK_SELECTION_CLEAR", 0, EVENT_SELECTION_TIME},			    /* = 17 */
      {"GDK_SELECTION_REQUEST", 0, EVENT_SELECTION_TIME},			    /* = 18 */
      {"GDK_SELECTION_NOTIFY", 0, EVENT_SELECTION_TIME},			    /* = 19 */

      {"GDK_PROXIMITY_IN", 0, EVENT_COMMON_TIME},				    /* = 20 */
      {"GDK_PROXIMITY_OUT", 0, EVENT_COMMON_TIME},				    /* = 21 */

      {"GDK_DRAG_ENTER", 0, EVENT_DND_TIME},				    /* = 22 */
      {"GDK_DRAG_LEAVE", 0, EVENT_DND_TIME},				    /* = 23 */
      {"GDK_DRAG_MOTION", 0, EVENT_DND_TIME},				    /* = 24 */
      {"GDK_DRAG_STATUS", 0, EVENT_DND_TIME},				    /* = 25 */
      {"GDK_DROP_START", 0, EVENT_DND_TIME},				    /* = 26 */
      {"GDK_DROP_FINISHED", 0, EVENT_DND_TIME},				    /* = 27 */

      {"GDK_CLIENT_EVENT", 0, EVENT_NO_TIME},				    /* = 28 */

      {"GDK_VISIBILITY_NOTIFY", GDK_VISIBILITY_NOTIFY_MASK, EVENT_NO_TIME}, /* = 29 */
      {"GDK_NO_EXPOSE", 0, EVENT_NO_TIME},				    /* = 30 */
      {"GDK_SCROLL", 0, EVENT_COMMON_TIME},				    /* = 31 */
      {"GDK_WINDOW_STATE", 0, EVENT_NO_TIME},				    /* = 32 */
      {"GDK_SETTING", 0, EVENT_NO_TIME},				    /* = 33 */
      {"GDK_OWNER_CHANGE", 0, EVENT_OWNER_TIME},				    /* = 34 */
      {"GDK_GRAB_BROKEN", 0, EVENT_NO_TIME},				    /* = 35 */
      {"GDK_DAMAGE", 0, EVENT_NO_TIME},				    /* = 36 */
      {"GDK_EVENT_LAST", 0, EVENT_NO_TIME}				    /* helper variable for decls */
    } ;


  true_index = any_event->type + 1 ;			    /* yuch, see enum values in comments above. */

  if (!exclude_mask || !(event_txt[true_index].mask & exclude_mask))
    {
      guint32 time ;

      switch (event_txt[true_index].time_struct_type)
	{
	case EVENT_COMMON_TIME:
	  {
	    EventCommonTime common = (EventCommonTime)any_event ;

	    time = common->time ;

	    break ;
	  }

	case EVENT_CROSSING_TIME:
	  {
	    GdkEventCrossing *crossing = (GdkEventCrossing *)any_event ;

	    time = crossing->time ;

	    break ;
	  }

	case EVENT_ATOM_TIME:
	  {
	    GdkEventProperty *atom_time = (GdkEventProperty *)any_event ;

	    time = atom_time->time ;

	    break ;
	  }

	case EVENT_SELECTION_TIME:
	  {
	    GdkEventSelection *select_time = (GdkEventSelection *)any_event ;

	    time = select_time->time ;

	    break ;
	  }

	case EVENT_DND_TIME:
	  {
	    GdkEventDND *dnd_time = (GdkEventDND *)any_event ;

	    time = dnd_time->time ;

	    break ;
	  }

	case EVENT_OWNER_TIME:
	  {
	    GdkEventOwnerChange *owner_time = (GdkEventOwnerChange *)any_event ;

	    time = owner_time->time ;

	    break ;
	  }

	case EVENT_NO_TIME:
	default:
	  {
	    time = 0 ;
	    break ;
	  }
	}

      event_as_text = g_strdup_printf("Event: \"%s\"\tXWindow: %x\tTime: %u.",
				      event_txt[true_index].text,
				      (unsigned int)GDK_WINDOW_XWINDOW(any_event->window),
				      time) ;
    }

  return event_as_text ;
}



/*! For use with custom built dialogs.
 *
 * Gtk provides a function called  gtk_dialog_run() which blocks until the user presses
 * a button on the dialog, the function returns an int indicating which button was
 * pressed. The problem is though that this function blocks any user interaction with
 * the rest of the application. This function is an attempt to get round this by
 * continuing to service events while waiting for a response.
 *
 * The code _relies_ on the response never being zero, no GTK predefined responses
 * have this value and you should not use this value either (see gtk web page for dialogs).
 *
 * toplevel     This must be the dialog widget itself.
 * 
 * returns an integer which corresponds to the button pressed.
 *  */
gint my_gtk_run_dialog_nonmodal(GtkWidget *toplevel)
{
  gint result = 0 ;
  int *response_ptr ;

  zMapAssert(GTK_IS_DIALOG(toplevel)) ;

  response_ptr = g_new0(gint, 1) ;
  *response_ptr = 0 ;

  g_signal_connect(GTK_OBJECT(toplevel), "response",
		   GTK_SIGNAL_FUNC(responseCB), (gpointer)response_ptr) ;

  while (*response_ptr == 0)
    gtk_main_iteration() ;

  result = *response_ptr ;

  g_free(response_ptr) ;

  return result ;
}



/* Sets/gets abbreviated style for window title prefix, if TRUE then 
 * instead of"ZMap (version)" just "Z" is displayed, if FALSE
 * then full prefix is restored. */
void zMapGUISetAbbrevTitlePrefix(gboolean abbrev_prefix)
{
  abbreviated_title_prefix_G = abbrev_prefix ;

  return ;
}

gboolean zMapGUIGetAbbrevTitlePrefix(void)
{
  return abbreviated_title_prefix_G ;
}





/* Returns a title string in standard form to be used for the window manager title
 * bar in zmap windows.
 *
 * Currently the format is:
 *
 *             "ZMap (version) <window_type> - <text>"
 *
 * Both window_type and text can be NULL.
 *
 * The returned string should be free'd by the caller using g_free() when no longer required.
 * 
 * If abbreviated style has been set then prefix is just "Z",
 * see zMapGUISetAbbrevTitlePrefix(). 
 *
 * window_type  The sort of window it is, e.g. "feature editor"
 * message      Very short text, e.g. "Please Reply" or a feature name or....
 * returns      the title string.
 *  */
char *zMapGUIMakeTitleString(char *window_type, char *message)
{
  char *title = NULL ;

  title = g_strdup_printf("%s%s%s%s%s%s%s%s",
			  (abbreviated_title_prefix_G ? "Z" : "ZMap"),
			  (abbreviated_title_prefix_G ? "" : " ("),
			  (abbreviated_title_prefix_G ? "" : zMapGetAppVersionString()),
			  (abbreviated_title_prefix_G ? "" : ")"),
			  (window_type ? " " : ""),
			  (window_type ? window_type : ""),
			  (message ? " - " : ""),
			  (message ? message : "")) ;

  return title ;
}

/* Trivial cover function for setting toplevel title
 * as per zMapGUIMakeTitleString above.
 *
 * window_type  The sort of window it is, e.g. "feature editor"
 * message      Very short text, e.g. "Please Reply" or a feature name or....
 * returns      nothing.
 */
void zMapGUISetToplevelTitle(GtkWidget *toplevel, char *zmap_win_type, char *zmap_win_text)
{
  char *title ;

  title = zMapGUIMakeTitleString(zmap_win_type, zmap_win_text) ;
  gtk_window_set_title(GTK_WINDOW(toplevel), title) ;
  g_free(title) ;

  return ;
}

/* Trivial cover function for creating a new toplevel window complete
 * with the title set as per zMapGUIMakeTitleString above.
 *
 * window_type  The sort of window it is, e.g. "feature editor"
 * message      Very short text, e.g. "Please Reply" or a feature name or....
 * returns      the toplevel widget.
 */
GtkWidget *zMapGUIToplevelNew(char *zmap_win_type, char *zmap_win_text)
{
  GtkWidget *toplevel = NULL ;

  toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
  zMapGUISetToplevelTitle(toplevel, zmap_win_type, zmap_win_text) ;

  return toplevel ;
}

/*! zMapGUIPopOutWidget
 *
 * \brief Simple function to reparent a widget to a new toplevel,
 * complete with handlers to reparent back to original and handle
 * original parent destruction.
 *
 * @param popout   The widget to reparent
 * @param title    The title for the new toplevel
 * @return GtkWidget *toplevel or NULL if already a direct child of a toplevel
 *
 */
GtkWidget *zMapGUIPopOutWidget(GtkWidget *popout, char *title)
{
  GtkWidget *new_toplevel = NULL;
  GtkWidget *curr_toplevel, *curr_parent;

  curr_parent = gtk_widget_get_parent(popout);
  curr_toplevel = zMapGUIFindTopLevel(popout);

  if ((curr_toplevel != curr_parent)
      && (new_toplevel = zMapGUIToplevelNew(title, "close to return to original position")))
    {
      PopOutData popout_data;

      if((popout_data = g_new0(PopOutDataStruct, 1)))
	{
	  popout_data->original_parent   = curr_parent;
	  popout_data->popout_toplevel = new_toplevel;
	  popout_data->popout_child    = popout;

	  gtk_widget_reparent(popout, new_toplevel);

	  popout_data->original_parent_signal_id =
	    g_signal_connect(G_OBJECT(curr_parent), "destroy",
			     G_CALLBACK(handle_original_parent_destroy_cb),
			     popout_data);

	  g_signal_connect(G_OBJECT(new_toplevel), "destroy",
			   G_CALLBACK(handle_popout_destroy_cb),
			   popout_data);
	}
    }

  return new_toplevel;
}


/*!
 * Shows the usual "About" window.
 *
 *
 * @param window_type  The sort of window it is, e.g. "feature editor"
 * @param message      Very short text, e.g. "Please Reply" or a feature name or....
 * @return             nothing
 *  */
void zMapGUIShowAbout(void)
{
  const gchar *authors[] = {"Ed Griffiths, Sanger Institute, UK <edgrif@sanger.ac.uk>",
			    "Roy Storey Sanger Institute, UK <rds@sanger.ac.uk>",
			    "Malcolm Hinsley, Sanger Institute, UK <mh17@sanger.ac.uk>",
			    NULL} ;
  char *comment_str ;

  comment_str = g_strdup_printf("%s\n\n%s\n", zMapGetCompileString(), zMapGetCommentsString()) ;

#if MAC_VERSION_HAS_BUG
  if (gtk_major_version == 2 && gtk_minor_version < 24)
#endif
    {
      gtk_about_dialog_set_url_hook(aboutLinkOldCB, NULL, NULL) ;

      gtk_show_about_dialog(NULL,
			    "authors", authors,
			    "comments", comment_str,
			    "copyright", zMapGetCopyrightString(),
			    "license", zMapGetLicenseString(),
			    "program-name", zMapGetAppName(),
			    "version", zMapGetAppVersionString(),
			    "website", zMapGetWebSiteString(),
			    "title", zMapGUIMakeTitleString(NULL, "About the program"),
			    NULL) ;
    }
#if MAC_VERSION_HAS_BUG
  else
    {
      GtkAboutDialog *about_dialog ;

      about_dialog = (GtkAboutDialog *)gtk_about_dialog_new() ;

      gtk_about_dialog_set_program_name(about_dialog, zMapGetAppName()) ;
      gtk_about_dialog_set_version(about_dialog, zMapGetAppVersionString()) ;
      gtk_about_dialog_set_copyright(about_dialog, zMapGetCopyrightString()) ;
      gtk_about_dialog_set_comments(about_dialog, comment_str) ;
      gtk_about_dialog_set_license(about_dialog, zMapGetLicenseString()) ;
      gtk_about_dialog_set_website(about_dialog, zMapGetWebSiteString()) ;
      gtk_about_dialog_set_authors(about_dialog, authors) ;

      g_signal_connect(GTK_OBJECT(about_dialog), "activate-link",
		       GTK_SIGNAL_FUNC(aboutLinkNewCB),
		       NULL) ;

      zMapGUISetToplevelTitle((GtkWidget)about_dialog, NULL, "About the program") ;

      gtk_widget_show_all(GTK_WIDGET(about_dialog)) ;
    }
#endif

  g_free(comment_str) ;

  return ;
}



/*!
 * Sets the URL where all the help pages for zmap can be found e.g. a local directory
 * of help pages. No URL checking is done, we just trust it's correct.
 * A copy is taken of the string.
 *
 * @param     URL_base  string specifying base URL for all help files.
 * @return              nothing
 *  */
void zMapGUISetHelpURL(char *URL_base)
{
  zMapAssert(URL_base && *URL_base) ;

  if (help_URL_base_G)
    g_free(help_URL_base_G) ;

  help_URL_base_G = g_strdup(URL_base) ;

  return ;
}



/*!
 * Displays the requested help, currently this is done by making a request to the users
 * web browser.
 *
 * @param help_content  One of an expanding set, controls which page is shown to the user.
 * @return              nothing
 *  */
void zMapGUIShowHelp(ZMapHelpType help_contents)
{
  char *web_page = NULL ;
  GError *error = NULL ;

  if (!help_URL_base_G)
    help_URL_base_G = ZMAPWEB_DOC_URL ;

  switch(help_contents)
    {
    case ZMAPGUI_HELP_GENERAL:
      web_page = g_strdup_printf("%s/%s", help_URL_base_G, ZMAPWEB_HELP_DOC) ;
      break ;

    case ZMAPGUI_HELP_ALIGNMENT_DISPLAY:
      web_page = g_strdup_printf("%s/%s#%s", help_URL_base_G, ZMAPWEB_HELP_DOC, ZMAPWEB_HELP_ALIGNMENT_SECTION) ;
      break ;

    case ZMAPGUI_HELP_KEYBOARD:
      web_page = g_strdup_printf("%s/%s", help_URL_base_G, ZMAPWEB_KEYBOARD_DOC) ;
      break ;

    case ZMAPGUI_HELP_RELEASE_NOTES:
      web_page = g_strdup_printf("%s/%s/%s", help_URL_base_G, ZMAPWEB_RELEASE_NOTES_DIR, ZMAPWEB_RELEASE_NOTES) ;
      break ;

    case ZMAPGUI_HELP_WHATS_NEW:
      web_page = g_strdup_printf("%s",ZMAP_INTERNAL_WEB_WHATSNEW);	/* a temporary fix via the internal wiki */
	break;

    default:
      zMapWarning("menu choice (%d) temporarily unavailable",help_contents);
      /* this used to assert: why crash? */
      /* NOTE gcc flags up switches with enums not handled, so doubly useless */
      break ;

    }

  if (zMapLaunchWebBrowser(web_page, &error))
    {
      zMapGUIShowMsgFull(NULL, "Please wait, help page will be shown in your browser in a few seconds.",
			 ZMAP_MSG_INFORMATION,
			 GTK_JUSTIFY_CENTER, 5, TRUE) ;
    }
  else
    {
      zMapWarning("Error: %s\n", error->message) ;

      g_error_free(error) ;
    }

  g_free(web_page) ;

  return ;
}




/*! Message Display System.
 *
 * Display a short message in a pop-up dialog box, if user data is required the dialog
 * will be modal and blocking otherwise the behaviour of the dialog depends on
 * the message type:
 *
 *               ZMAP_MSG_INFORMATION, ZMAP_MSG_WARNING - non-blocking, non-modal
 *     ZMAP_MSG_CRITICAL, ZMAP_MSG_EXIT, ZMAP_MSG_CRASH - blocking and modal
 *
 * If parent is non-NULL then the dialog will be kept on top of that window, essential for
 * modal dialogs in particular. I think parent should be the application window that the message
 * applies to probably, or perhaps the application main window.
 *
 * If user_data is non-NULL it gives the type of data to be returned. The dialog will be
 * constructed so the user can return the correct sort of data.
 *
 * The following parameters are supported (not that not all parameters are available for all
 * functions):
 *
 * @param parent       Widget that message should be kept on top of or NULL.
 * @param title_in     Alternative dialog window title.
 * @param msg          Message to be displayed in dialog.
 * @param msg_type     ZMAP_MSG_INFORMATION | ZMAP_MSG_WARNING | ZMAP_MSG_CRITICAL | ZMAP_MSG_EXIT | ZMAP_MSG_CRASH
 * @param justify      Alignment of text in msg.
 * @param display_timeout  Time in seconds after which message is automatically removed (ignored for
 *                     modal dialogs).
 * @param user_data    struct giving type of data that should be returned.
 *
 * Some of the message functions return a boolean, if they do the meaning is this:
 *
 * @return             TRUE if user data not required or if user data returned, FALSE otherwise.
 *  */


/*!
 * Formats and displays a message, note that the message may be displayed in a window
 * or at the terminal depending on how the message system has been initialised.
 *
 * @param msg_type      The type of message: information, warning etc.
 * @param format        A printf() style format string.
 * @param ...           The parameters matching the format string.
 * @return              nothing
 *  */
void zMapShowMsg(ZMapMsgType msg_type, char *format, ...)
{
  va_list args ;
  char *msg_string ;

  va_start(args, format) ;
  msg_string = g_strdup_vprintf(format, args) ;
  va_end(args) ;

  zMapGUIShowMsg(msg_type, msg_string) ;

  g_free(msg_string) ;

  return ;
}


/*!
 * Show a message in a simple pop-up window.
 *
 * See zMapGUIMsgFull() for complete documentation of parameters.
 *
 * @return  void
 *  */
void zMapGUIShowMsg(ZMapMsgType msg_type, char *msg)
{
  gboolean modal ;

  modal = modalFromMsgType(msg_type) ;

  messageFull(NULL, NULL, msg, modal, 0, TRUE, msg_type, GTK_JUSTIFY_CENTER, NULL) ;

  return ;
}


/*!
 * Show a message in a simple pop-up window with timeout and justify options.
 *
 * See zMapGUIMsgFull() for complete documentation of parameters.
 *
 * @return  void
 */
void zMapGUIShowMsgFull(GtkWindow *parent, char *msg,
			ZMapMsgType msg_type,
			GtkJustification justify, int display_timeout, gboolean close_button)
{
  gboolean modal ;

  modal = modalFromMsgType(msg_type) ;

  messageFull(parent, NULL, msg, modal, display_timeout, close_button, msg_type, justify, NULL) ;

  return ;
}


/*!
 * Show a message where user must select "ok" or "cancel".
 * 
 * Works differently from zMapGUIMsgGetText(), can only return
 * TRUE or FALSE, these are only choices for user.
 *
 * See zMapGUIMsgFull() for complete documentation of parameters.
 *
 * @return  gboolean, TRUE if user selected "ok", FALSE otherwise.
 */
gboolean zMapGUIMsgGetBool(GtkWindow *parent, ZMapMsgType msg_type, char *msg)
{
  gboolean result = FALSE ;
  ZMapGUIMsgUserDataStruct user_data = {ZMAPGUI_USERDATA_BOOL, FALSE, {FALSE}} ;
  gboolean modal ;

  modal = modalFromMsgType(msg_type) ;

  /* We ignore the return value, it's not needed, just pick up the boolean. */
  messageFull(parent, NULL, msg, modal, 0, TRUE, msg_type, GTK_JUSTIFY_CENTER, &user_data) ;

  result = user_data.data.bool ;

  return result ;
}



/*!
 * Show a message where user must enter some text, caller should use
 * g_free() to release text when finished with.
 *
 * See zMapGUIMsgFull() for complete documentation of parameters.
 *
 * @return  char *, users text or NULL if no text.
 */
GtkResponseType zMapGUIMsgGetText(GtkWindow *parent, ZMapMsgType msg_type, char *msg, gboolean hide_text,
				  char **text_out)
{
  GtkResponseType result = GTK_RESPONSE_CANCEL ;
  ZMapGUIMsgUserDataStruct user_data = {ZMAPGUI_USERDATA_TEXT, FALSE, {FALSE}} ;
  gboolean modal ;

  modal = modalFromMsgType(msg_type) ;

  user_data.hide_input = hide_text ;

  if ((result = messageFull(parent, NULL, msg, modal, 0, TRUE, msg_type, GTK_JUSTIFY_CENTER, &user_data))
      == GTK_RESPONSE_OK)
    *text_out = user_data.data.text ;

  return result ;
}




/*!
 * Display longer text in a text widget. The code attempts to display the window
 * as 80 chars wide. Currently both title and text must be provided. This function
 * is intended for displaying multi-line text, to ask the user for passwords or
 * other single line text use zMapGUIGetText()
 *
 * NOTE that while text can be edittable this isn't very useful as there is no
 * mechanism for saving the text although it can be cut/pasted.
 *
 * @param title        Window title
 * @param text         Text to display in window.
 * @param edittable    Can the text be editted ?
 * @return             nothing
 *  */
void zMapGUIShowText(char *title, char *text, gboolean edittable)
{
  GtkWidget *dialog ;

  dialog = zMapGUIShowTextFull(title, text, edittable, NULL, NULL) ;

  return ;
}

/*!
 * The full version of zMapGUIShowText which returns the dialog widget
 * and fills in the pointer to point to the text buffer.
 *
 * @param title        Window title
 * @param text         Text to display in window.
 * @param edittable    Can the text be editted ?
 * @param buffer_out   location to return the text buffer to.
 * @return             the GtkWidget *dialog
 */
GtkWidget *zMapGUIShowTextFull(char *title, char *text, gboolean edittable, GList *text_attributes,
			       GtkTextBuffer **buffer_out)
{
  enum {TEXT_X_BORDERS = 32, TEXT_Y_BORDERS = 50} ;
  char *full_title ;
  GtkWidget *dialog, *scrwin, *view ;
  GtkTextBuffer *buffer ;
  GList *fixed_font_list = NULL ;
  GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT ;
  PangoFont *font ;
  PangoFontDescription *font_desc ;
  int width ;
  double x, y ;
  int text_width, text_height ;


  zMapAssert(title && *title && text && *text) ;

  full_title = zMapGUIMakeTitleString(NULL, title) ;
  dialog = gtk_dialog_new_with_buttons(full_title, NULL, flags,
				       "Close", GTK_RESPONSE_NONE,
				       NULL) ;
  g_free(full_title) ;

  gtk_container_set_border_width(GTK_CONTAINER(dialog), 5) ;

  /* Ensure that the dialog box is destroyed when the user responds. */
  g_signal_connect_swapped(dialog,
			   "response",
			   G_CALLBACK(gtk_widget_destroy),
			   dialog) ;

  scrwin = gtk_scrolled_window_new(NULL, NULL) ;
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), scrwin, TRUE, TRUE, 0) ;

  view = gtk_text_view_new() ;
  gtk_container_add(GTK_CONTAINER(scrwin), view) ;
  gtk_text_view_set_editable(GTK_TEXT_VIEW(view), edittable) ;
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)) ;
  gtk_text_buffer_set_text(buffer, text, -1) ;

  if (buffer_out)
    *buffer_out = buffer ;

  /* Construct a list of possible fonts to use, they must be fixed width but take care
   * altering this list as some fixed width fonts seem to come back as variable width ! */

  fixed_font_list = g_list_append(fixed_font_list, ZMAP_ZOOM_FONT_FAMILY) ;
  fixed_font_list = g_list_append(fixed_font_list, "fixed") ;


  /* Here we try to set a fixed width font in the text widget and set the size of the dialog
   * so that a sensible amount of text is displayed. */
  if (!zMapGUIGetFixedWidthFont(view,
				fixed_font_list, ZMAP_ZOOM_FONT_SIZE, PANGO_WEIGHT_NORMAL,
				&font, &font_desc))
    {
      zMapGUIShowMsg(ZMAP_MSG_WARNING, "Could not get fixed width font, "
		     "message window may be a strange size") ;
    }
  else
    {
      /* code attempts to get font size and then set window size accordingly, it now appears
       * to get this wrong so I have hacked this for now... */
      TextAttrsStruct text_attrs_data = {buffer} ;


      zMapGUIGetFontWidth(font, &width) ;

      zMapGUIGetPixelsPerUnit(ZMAPGUI_PIXELS_PER_POINT, dialog, &x, &y) ;

      text_width = (int)(((double)(80 * width)) / x) ;
      text_height = text_width / 2 ;

      /* This is all a bit hacky but basically the borders around the window take up quite a lot of
       * size and its not easy to calculate how big to make the window so the text shows....sigh... */
      text_width += TEXT_X_BORDERS ;
      text_height += TEXT_Y_BORDERS ;

      /* Big hack here...see comment above.... */
      if (text_width < 700)
	text_width = 700 ;

      gtk_widget_modify_font(view, font_desc) ;


      /* Set any specified text attributes. */
      g_list_foreach(text_attributes, setTextAttrs, &text_attrs_data) ;



      gtk_window_set_default_size(GTK_WINDOW(dialog), text_width, text_height) ;
    }

  g_list_free(fixed_font_list) ;

  gtk_widget_show_all(dialog) ;

  return dialog;
}


/* Returns path of file chosen by user which can be used directly to open the file,
 * it is the callers responsibility to free the filepath using g_free().
 * Caller can optionally specify a default directory. */
char *zmapGUIFileChooserFull(GtkWidget *toplevel, char *title, char *directory_in, char *file_suffix,
			     ZMapFileChooserContentAreaCB content_func, gpointer content_data)
{
  char *full_title ;
  char *file_path = NULL ;
  GtkWidget *dialog ;

  zMapAssert(toplevel) ;

  if (!title)
    title = "Please give a file name:" ;

  full_title = zMapGUIMakeTitleString(title, NULL) ;

  /* Set up the dialog. */
  dialog = gtk_file_chooser_dialog_new(full_title,
				       GTK_WINDOW(toplevel),
				       GTK_FILE_CHOOSER_ACTION_SAVE,
				       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				       GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				       NULL) ;

  g_free(full_title) ;

  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE) ;

  if (directory_in)
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), directory_in) ;

  if (file_suffix)
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER (dialog), file_suffix) ;

  if(content_func)
    {
      GtkWidget *content_vbox;
      //content_vbox = gtk_dialog_get_content_area (GTK_DIALOG(dialog));
      content_vbox = GTK_WIDGET(GTK_DIALOG(dialog)->vbox);
      if (content_vbox)
	{
	  (content_func)(content_vbox, content_data);
	  gtk_widget_show_all(content_vbox);
	}
    }

  /* Wait for a response, we don't have to do anything with this new dialog....yipeeeee.... */
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    file_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog)) ;

  gtk_widget_destroy (dialog);

  return file_path ;
}

char *zmapGUIFileChooser(GtkWidget *toplevel,  char *title, char *directory_in, char *file_suffix)
{
  char *file = NULL;

  file = zmapGUIFileChooserFull(toplevel, title, directory_in, file_suffix, NULL, NULL);

  return file;
}



/* Takes a colour spec in the standard string form given for X11 in the file rgb.txt, parses it
 * and allocates it in the colormap returning the colour description in colour_inout
 * (the contents of which are overwritten).
 *
 * Returns TRUE if all worked, FALSE otherwise. */
gboolean zMapGUIGetColour(GtkWidget *widget, char *colour_spec, GdkColor *colour_inout)
{
  gboolean result = FALSE ;
  GdkColormap *colourmap ;

  if ((result = gdk_color_parse(colour_spec, colour_inout)))
    {
      colourmap = gtk_widget_get_colormap(widget) ;

      gdk_rgb_find_color(colourmap, colour_inout) ;
    }

  return result ;
}





/*!
 * Tries to return a fixed font from the list given in pref_families, returns
 * TRUE if it succeeded in finding a matching font, FALSE otherwise.
 * The list of preferred fonts is treated with most preferred first and least
 * preferred last.  The function will attempt to return the most preferred font
 * it finds.
 *
 * @param widget         Needed to get a context, ideally should be the widget you want to
 *                       either set a font in or find out about a font for.
 * @param pref_families  List of font families (as text strings).
 * @param points         Size of font in points.
 * @param weight         Weight of font (e.g. PANGO_WEIGHT_NORMAL)
 * @param font_out       If non-NULL, the font is returned.
 * @param desc_out       If non-NULL, the font description is returned.
 * @return               TRUE if font found, FALSE otherwise.
 */
gboolean zMapGUIGetFixedWidthFont(GtkWidget *widget,
				  GList *pref_families, gint points, PangoWeight weight,
				  PangoFont **font_out, PangoFontDescription **desc_out)
{
  gboolean found = FALSE, found_most_preferred = FALSE ;
  PangoContext *context ;
  PangoFontFamily **families;
  gint n_families, i, most_preferred, current ;
  PangoFontFamily *match_family = NULL ;
  PangoFont *font = NULL;

  context = gtk_widget_get_pango_context(widget) ;

  pango_context_list_families(context, &families, &n_families) ;

  most_preferred = g_list_length(pref_families);

  for (i = 0 ; (i < n_families && !found_most_preferred) ; i++)
    {
      const gchar *name ;
      GList *pref ;

      name = pango_font_family_get_name(families[i]) ;
      current = 0;

      pref = g_list_first(pref_families) ;
      while(pref && ++current)
	{
	  char *pref_font = (char *)pref->data ;

	  if (g_ascii_strncasecmp(name, pref_font, strlen(pref_font)) == 0
#if GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION >= 6
	      && pango_font_family_is_monospace(families[i])
#endif
	      )
	    {
	      found = TRUE ;
              if(current <= most_preferred)
                {
                  if((most_preferred = current) == 1)
                    found_most_preferred = TRUE;
                  match_family   = families[i] ;
                }
              match_family = families[i] ;
	      break ;
	    }

	  pref = g_list_next(pref) ;
	}
    }

  if(families)
    {
      g_free(families);
      families = NULL ;
    }

  if (found)
    {
      PangoFontDescription *desc ;
      const gchar *name ;

      name = pango_font_family_get_name (match_family) ;

      desc = pango_font_description_from_string(name) ;
      pango_font_description_set_family(desc, name) ;
      pango_font_description_set_size  (desc, points * PANGO_SCALE) ;
      pango_font_description_set_weight(desc, weight) ;

      font = pango_context_load_font(context, desc) ;
      zMapAssert(font) ;

      if (font_out)
	*font_out = font ;
      if (desc_out)
	*desc_out = desc ;

      found = TRUE ;
    }

  return found ;
}


/*!
 * Returns approx/average width in _point_ units (i.e. 1/72 of an inch).
 * NOTE that we try to do this for english fonts only....
 *
 * @param font           The font for which the width is needed.
 * @param width_out      The font width is returned here.
 * @return               nothing
 */
void zMapGUIGetFontWidth(PangoFont *font, int *width_out)
{
  PangoLanguage *language ;
  PangoFontMetrics*  metrics ;
  int width ;

  language = pango_language_from_string("en-UK") ; /* Does not need to be free'd. */

  metrics = pango_font_get_metrics(font, language) ;
  zMapAssert(metrics) ;

  width = pango_font_metrics_get_approximate_char_width(metrics) ;
  width = PANGO_PIXELS(width) ;				    /* PANGO_PIXELS confusingly converts
							       to points not pixels...sigh... */

  pango_font_metrics_unref(metrics) ;

  *width_out = width ;

  return ;
}


/*!
 * Returns x and y _screen_ pixels for various absolute measures.
 * NOTE that x and y may differ as some screens report different pixel densities
 * for the x and y directions even though they display as if the pixels are square...
 *
 * @param conv_type      ZMAPGUI_PIXELS_PER_CM | ZMAPGUI_PIXELS_PER_INCH | ZMAPGUI_PIXELS_PER_POINT
 * @param widget         Used to get the screen where display is happening.
 * @param x_out          X pixels
 * @param y_out          Y pixels
 * @return               nothing
 */
void zMapGUIGetPixelsPerUnit(ZMapGUIPixelConvType conv_type, GtkWidget *widget, double *x_out, double *y_out)
{
  GdkScreen *screen ;
  double width, height, width_mm, height_mm ;
  double multiplier ;
  /* double mm_2_inches = 25.4 ; */
  double x, y ;

  screen = gtk_widget_get_screen(widget) ;

  /* Get the raw measurements for the screen. */
  width = (double)(gdk_screen_get_width(screen)) ;
  height = (double)(gdk_screen_get_height(screen)) ;
  width_mm = ((double)(gdk_screen_get_width_mm(screen))) ;
  height_mm = ((double)(gdk_screen_get_height_mm(screen))) ;

  x = width / width_mm ;
  y = height / height_mm ;

  switch(conv_type)
    {
    case ZMAPGUI_PIXELS_PER_CM:
      multiplier = 10.0 ;
      break ;
    case ZMAPGUI_PIXELS_PER_INCH:
      multiplier = 25.4 ;
      break ;
    case ZMAPGUI_PIXELS_PER_POINT:
      multiplier = (25.4 / 72.0) ;
      break ;
    }

  x = x * multiplier ;
  y = y * multiplier ;

  if (x_out)
    *x_out = x ;

  if (y_out)
    *y_out = y ;

  return ;
}


/* Possibly sensible to make value_out point to a member of clickedData if you are setting clickedCB. */
void zMapGUICreateRadioGroup(GtkWidget *gtkbox,
                             ZMapGUIRadioButton all_buttons,
                             int default_button, int *value_out,
                             ZMapGUIRadioButtonCBFunc clickedCB, gpointer clickedData)
{
  GtkWidget *radio = NULL;
  GSList    *group = NULL;
  ZMapGUIRadioButton buttons = NULL;

  buttons = all_buttons;

  while(buttons->name)
    {
      RadioButtonCBData data = NULL;

      radio = NULL;
      data  = g_new0(RadioButtonCBDataStruct, 1);
      data->value     = buttons->value;
      data->output    = value_out;
      data->callback  = clickedCB;
      data->user_data = clickedData;

      buttons->widget =
        radio = gtk_radio_button_new_with_label(group, buttons->name);

      g_signal_connect_data(G_OBJECT(radio), "clicked",
                            G_CALLBACK(radioButtonCB), (gpointer)data,
                            (GClosureNotify)radioButtonCBDataDestroy, 0);

      /* If value == default set it so */
      if(buttons->value == default_button)
        {
          gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
          *(data->output) = data->value; /* In case we're never clicked */
        }

      /* pack into the box */
      gtk_box_pack_start(GTK_BOX(gtkbox), radio, TRUE, TRUE, 0);

      /* Get the group it's in */
      group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio));

      /* move to the next input */
      buttons++;
    }

  return ;
}




ZMapGUIClampType zMapGUICoordsClampToLimits(double  top_limit, double  bot_limit,
                                            double *top_inout, double *bot_inout)
{
  ZMapGUIClampType ct = ZMAPGUI_CLAMP_INIT;
  double top, bot;

  top = *top_inout;
  bot = *bot_inout;

  if (top < top_limit)
    {
      top = top_limit;
      ct |= ZMAPGUI_CLAMP_START;
    }
  else if (floor(top) == top_limit)
    ct |= ZMAPGUI_CLAMP_START;

  if (bot > bot_limit)
    {
      bot = bot_limit;
      ct |= ZMAPGUI_CLAMP_END;
    }
  else if(ceil(bot) == bot_limit)
    ct |= ZMAPGUI_CLAMP_END;

  *top_inout = top;
  *bot_inout = bot;

  return ct;
}

ZMapGUIClampType zMapGUICoordsClampSpanWithLimits(double  top_limit, double  bot_limit,
                                                  double *top_inout, double *bot_inout)
{
  ZMapGUIClampType ct = ZMAPGUI_CLAMP_INIT;
  double top, bot;

  top = *top_inout;
  bot = *bot_inout;

  if (top < top_limit)
    {
      if ((bot = bot + (top_limit - top)) > bot_limit)
        {
          bot = bot_limit ;
          ct |= ZMAPGUI_CLAMP_END;
        }
      ct |= ZMAPGUI_CLAMP_START;
      top = top_limit;
    }
  else if (bot > bot_limit)
    {
      if ((top = top - (bot - bot_limit)) < top_limit)
        {
          ct |= ZMAPGUI_CLAMP_START;
          top = top_limit;
        }
      ct |= ZMAPGUI_CLAMP_END;
      bot = bot_limit;
    }

  *top_inout = top;
  *bot_inout = bot;

  return ct;
}


void zMapGUISetClipboard(GtkWidget *widget, char *text)
{
  if(text)
    {
      GtkClipboard* clip = NULL;
      if((clip = gtk_widget_get_clipboard(GTK_WIDGET(widget),
                                          GDK_SELECTION_PRIMARY)) != NULL)
        {
          //printf("Setting clipboard to: '%s'\n", text);
          gtk_clipboard_set_text(clip, text, -1);
        }
      else
        zMapLogWarning("%s", "Failed to get clipboard (GDK_SELECTION_PRIMARY)");
    }


  return ;
}



/* GtkPaned Widget helper functions... */

/*!
 * \brief To safely set a maximum position of a paned handle from the
 * GtkPaned:position notify signal handler.
 *
 * Without this doing a gtk_paned_set_position in the handler results
 * in another call to the handler and it's very easy to get into a
 * loop... So instead of doing
 * g_signal_connect(widget, "notify::position", cb, data);
 * use this function.
 *
 * \param               GtkWidget * should be GtkPaned *
 * \param               Your GCallback
 * \param               Your data (this is never freed)
 * \return              void
 ************************************************** */
void zMapGUIPanedSetMaxPositionHandler(GtkWidget *widget, GCallback callback, gpointer user_data)
{
  PaneMaxPosition pane_data = NULL;

  if(GTK_IS_PANED(widget))
    {
      pane_data = g_new0(PaneMaxPositionStruct, 1);

      pane_data->callback   = (g_object_notify_callback)callback;
      pane_data->user_data  = user_data;
      pane_data->handler_id = g_signal_connect_data(G_OBJECT(widget),
                                                    "notify::position",
                                                    G_CALLBACK(pane_max_position_callback),
                                                    (gpointer)pane_data,
                                                    pane_max_position_destroy_notify, 0);
    }

  return ;
}



/* Return a GdkCursor which can be used to set the mouse pointer
 * for a window.
 *
 * If cursor name begins with "zmap_" then look in list of custom cursors
 * otherwise look in list of standard X Windows cursors.
 */
GdkCursor *zMapGUIGetCursor(char *cursor_name)
{
  GdkCursor *cursor = NULL ;

  if (g_ascii_strncasecmp(cursor_name, ZMAPGUI_CURSOR_PREFIX, strlen(ZMAPGUI_CURSOR_PREFIX)) == 0)
    {
      cursor = makeCustomCursor(cursor_name) ;
    }
  else
    {
      cursor = makeStandardCursor(cursor_name) ;
    }

  return cursor ;
}




/*
 *                      Internal functions
 */


static void pane_max_position_callback(GObject *pane, GParamSpec *scroll, gpointer user_data)
{
  PaneMaxPosition pane_data = (PaneMaxPosition)user_data;

  if(pane_data->callback)
    {
      g_signal_handlers_block_by_func(G_OBJECT(pane),
				      G_CALLBACK(pane_max_position_callback),
				      pane_data);

      (pane_data->callback)(pane, scroll, pane_data->user_data);

      g_signal_handlers_unblock_by_func(G_OBJECT(pane),
					G_CALLBACK(pane_max_position_callback),
					pane_data);
    }

  return;
}

static void pane_max_position_destroy_notify(gpointer pane_max_pos_data, GClosure *unused_closure)
{
  PaneMaxPosition pane_data = (PaneMaxPosition)pane_max_pos_data;

  g_free(pane_data);

  return ;
}


/* Callback for my_gtk_run_dialog_nonmodal(). Records the response from
 * the dialog so we can detect it in our event handling loop. */
static void responseCB(GtkDialog *toplevel, gint arg1, gpointer user_data)
{
  int *response_ptr = (int *)user_data ;

  zMapAssert(arg1 != 0) ;				    /* It will never exit if this is true... */

  *response_ptr = arg1 ;

  return ;
}


static gboolean modalFromMsgType(ZMapMsgType msg_type)
{
  gboolean modal = FALSE ;

  if (msg_type == ZMAP_MSG_CRITICAL || msg_type == ZMAP_MSG_EXIT || msg_type == ZMAP_MSG_CRASH)
    modal = TRUE ;

  return modal ;
}



/* Called by all the zmap gui message functions to display a short message in
 * a pop-up dialog box. Note that parameters are not independently variable,
 * e.g. if user_data is required the dialog will have buttons including a
 * "close" button, the close_button parameter is ignored.
 *
 * If parent is non-NULL then the dialog will be kept on top of that window, essential for
 * modal dialogs in particular. I think parent should be the application window that the message
 * applies to probably, or perhaps the application main window.
 *
 * If user_data is non-NULL it gives the type of data to be returned. The dialog will be
 * constructed so the user can return the correct sort of data.
 *
 * parent       Widget that message should be kept on top of or NULL.
 * title_in     Alternative dialog window title.
 * msg          Message to be displayed in dialog.
 * msg_type     ZMAP_MSG_INFORMATION | ZMAP_MSG_WARNING | ZMAP_MSG_CRITICAL | ZMAP_MSG_EXIT | ZMAP_MSG_CRASH
 * justify      Alignment of text in msg.
 * display_timeout  Time in seconds after which message is automatically removed (ignored for
 *                     modal dialogs).
 * user_data    struct giving type of data that should be returned.
 *
 * returns TRUE if user data not required or if user data returned, FALSE otherwise.
 *  */
static GtkResponseType messageFull(GtkWindow *parent, char *title_in, char *msg,
				   gboolean modal, int display_timeout, gboolean close_button,
				   ZMapMsgType msg_type, GtkJustification justify,
				   ZMapGUIMsgUserData user_data)
{
  GtkResponseType result = GTK_RESPONSE_CANCEL ;
  GtkWidget *dialog, *button, *label ;
  GtkWidget *entry = NULL ;
  char *title = NULL, *full_title ;
  GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT ;
  guint timeout_func_id ;
  int interval = display_timeout * 1000 ;		    /* glib needs time in milliseconds. */

  /* relies on order of ZMapMsgType enum.... */
  zMapAssert((msg_type >= ZMAP_MSG_INFORMATION || msg_type <= ZMAP_MSG_CRASH)
	     && (msg && *msg)
	     && (!user_data
		 || (user_data
		     && (user_data->type > ZMAPGUI_USERDATA_INVALID && user_data->type <= ZMAPGUI_USERDATA_TEXT)))) ;

  /* Set up title. */
  if (title_in && *title_in)
    {
      title = title_in ;
    }
  else
    {
      switch(msg_type)
	{
	case ZMAP_MSG_INFORMATION:
	  title = "Information" ;
	  break ;
	case ZMAP_MSG_WARNING:
	  title = "Warning !" ;
	  break;
	case ZMAP_MSG_CRITICAL:
	  title = "CRITICAL !" ;
	  break;
	case ZMAP_MSG_EXIT:
	  title = "EXIT !" ;
	  break;
	case ZMAP_MSG_CRASH:
	  title = "CRASH !" ;
	  break;
	}
    }

  full_title = zMapGUIMakeTitleString(NULL, title) ;

  /* Set up the dialog, if user data is required we need all the buttons, otherwise
   * there will be a "close" or perhaps no buttons for messages that time out. */
  if (modal)
    flags |= GTK_DIALOG_MODAL ;

  if (user_data)
    dialog = gtk_dialog_new_with_buttons(full_title, parent, flags,
					 GTK_STOCK_OK,
					 GTK_RESPONSE_OK,
					 GTK_STOCK_CANCEL,
					 GTK_RESPONSE_CANCEL,
					 NULL) ;
  else if (close_button)
    dialog = gtk_dialog_new_with_buttons(full_title, parent, flags,
					 "Close", GTK_RESPONSE_NONE,
					 NULL) ;
  else
    dialog = gtk_dialog_new_with_buttons(full_title, parent, flags,
					 NULL) ;

  g_free(full_title) ;


  gtk_container_set_border_width(GTK_CONTAINER(dialog), 5) ;


  /* Set up the message text in a button widget so that it can put in the primary
   * selection buffer for cut/paste when user clicks on it. */
  button = gtk_button_new_with_label(msg) ;
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), button, TRUE, TRUE, 20);
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE) ;
  label = gtk_bin_get_child(GTK_BIN(button)) ;		    /* Center + wrap long lines. */
  gtk_label_set_justify(GTK_LABEL(label), justify) ;
  gtk_label_set_line_wrap(GTK_LABEL(label), TRUE) ;

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     GTK_SIGNAL_FUNC(butClick), button) ;


  /* For text set up a an entry widget, optionally with hidden text entry. */
  if (user_data && user_data->type == ZMAPGUI_USERDATA_TEXT)
    {
      entry = gtk_entry_new() ;
      gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, TRUE, TRUE, 0) ;

      if (user_data->hide_input)
	gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE) ;
    }


  /* Different handlers are required for modal vs. non-modal. */
  if (interval > 0)
    {
      if (modal)
	timeout_func_id = g_timeout_add(interval, timeoutHandlerModal, (gpointer)dialog) ;
      else
	timeout_func_id = g_timeout_add(interval, timeoutHandler, (gpointer)dialog) ;
    }


  gtk_widget_show_all(dialog) ;


  /* Note flow of control here: for non-modal/no-data-returned messages simply connect gtk destroy
   * to ensure dialog gets cleaned up and return, for modal/data messages we _block_ waiting
   * for a response which we then return from this routine. */
  if (!modal && !user_data)
    {
      g_signal_connect_swapped(dialog,
			       "response",
			       G_CALLBACK(gtk_widget_destroy),
			       dialog) ;
    }
  else
    {
      /* block waiting for user to answer dialog, for modal we can use straight gtk call,
       * for non-modal must use our home-grown (and probably buggy) version. */
      if (modal)
	result = gtk_dialog_run(GTK_DIALOG(dialog)) ;
      else
	result = my_gtk_run_dialog_nonmodal(dialog) ;

      /* Return any data that was requested by caller according to which button they click. */
      if (user_data)
	{
	  switch (result)
	    {
	    case GTK_RESPONSE_OK:
	      {
		switch (user_data->type)
		  {
		  case ZMAPGUI_USERDATA_BOOL:
		    {
		      user_data->data.bool = TRUE ;
		      break ;
		    }
		  case ZMAPGUI_USERDATA_TEXT:
		    {
		      char *text ;

		      /* entry returns "" for no string so check there is some text. */
		      if ((text = (char *)gtk_entry_get_text(GTK_ENTRY(entry))) && *text)
			user_data->data.text = g_strdup(text) ;

		      break ;
		    }
		  default:
		    {
		      zMapAssertNotReached() ;

		      break ;
		    }
		  }

		break ;
	      }
	    default:
	      {
		switch (user_data->type)
		  {
		  case ZMAPGUI_USERDATA_BOOL:
		    {
		      user_data->data.bool = FALSE ;
		      break ;
		    }
		  default:
		    {
		      /* No need to do anything, just return CANCEL. */

		      break ;
		    }
		  }

		break ;
	      }
	    }
	}

      gtk_widget_destroy(dialog) ;
    }


  return result ;
}



/* When user clicks on the message itself, we put the message text in the cut buffer. */
static void butClick(GtkButton *button, gpointer user_data)
{
  const char *label_text ;
  GtkClipboard* clip_board ;

  label_text = gtk_button_get_label(button) ;

  clip_board = gtk_clipboard_get(GDK_SELECTION_PRIMARY) ;

  gtk_clipboard_set_text(clip_board, label_text, -1) ;

  return ;
}



/* A GSourceFunc, called to send a message to gtk_dialog_run() code to terminate.
 * We return GTK_RESPONSE_NONE to indicate that we have signalled that the dialog
 * should be destroyed. */
static gboolean timeoutHandlerModal(gpointer data)
{
  GtkDialog *dialog = (GtkDialog *)data ;

  gtk_dialog_response(dialog, GTK_RESPONSE_NONE) ;

  return FALSE ;
}

/* A GSourceFunc, called to pop down a message after a certain time. */
static gboolean timeoutHandler(gpointer data)
{
  GtkWidget *dialog = (GtkWidget *)data ;

  gtk_widget_destroy(dialog) ;

  return FALSE ;
}




static void handle_popout_destroy_cb(GtkWidget *toplevel, gpointer cb_data)
{
  PopOutData popout_data = (PopOutData)cb_data;

  if(toplevel == popout_data->popout_toplevel)
    {
      if(popout_data->original_parent)
	{
	  g_signal_handler_disconnect(popout_data->original_parent,
				      popout_data->original_parent_signal_id);
	  gtk_widget_reparent(popout_data->popout_child,
			      popout_data->original_parent);
	}
      else			/* just free the data */
	{
	  popout_data->popout_child = NULL;
	  popout_data->popout_toplevel = NULL;
	  g_free(popout_data);
	}
    }

  return ;
}

static void handle_original_parent_destroy_cb(GtkWidget *widget, gpointer cb_data)
{
  PopOutData popout_data = (PopOutData)cb_data;

  popout_data->original_parent = NULL;

  gtk_widget_destroy(popout_data->popout_toplevel);

  return ;
}




/* Handle creating a radio group.
 * pass in a vbox or hbox and the buttons will get packed in order.
 */

/* Callback to set the value and call any user specified callback */
static void radioButtonCB(GtkWidget *button, gpointer radio_data)
{
  RadioButtonCBData data = (RadioButtonCBData)radio_data;
  gboolean active;

  if((active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
     && data->output)
    *(data->output) = data->value;

  if(data->callback)
    (data->callback)(button, data->user_data, active);

  return ;
}

/* GDestroyNotify for the RadioButtonCBData attached to radio buttons
 * for "clicked" signal */
static void radioButtonCBDataDestroy(gpointer data)
{
  RadioButtonCBData button_data = (RadioButtonCBData)data;

  /* clear pointers */
  button_data->output    = NULL;
  button_data->callback  = NULL;
  button_data->user_data = NULL;
  /* and free .. */
  g_free(button_data);

  return ;
}


/* Constructs custom shaped cursors, just add more as required but
 * make sure their text names begin with "zmap_" to distinguish
 * them from standard X cursors.
 *
 * The sample X windows utility program "bitmap" was used to construct
 * files containing definitions for the shape, mask, size and hotspots
 * in a format suitable for the GDK calls that create cursors (i.e. X bitmaps).
 *
 * If you ever need to edit any of the below cursors simply cut/paste the
 * the bitmap data into a normal text file with format as follows and then
 * use the bitmap program to edit them:
 *
 * #define zmap_cursor_shape_width 16
 * #define zmap_cursor_shape_height 16
 * #define zmap_cursor_shape_x_hot 7
 * #define zmap_cursor_shape_y_hot 7
 * static unsigned char zmap_cursor_shape_bits[] = {
 *    0x00, 0x00, 0xe0, 0x03, 0x10, 0x04, 0x08, 0x08, 0x04, 0x10, 0x02, 0x20,
 *    0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x04, 0x10, 0x08, 0x08,
 *    0x10, 0x04, 0xe0, 0x03, 0x00, 0x00, 0x00, 0x00};
 *
 * Once you've done that you can just extract the hot spot and bits data and
 * insert it below as for the other cursors.
 *
 *  */
static GdkCursor *makeCustomCursor(char *cursor_name)
{
  GdkCursor *cursor = NULL ;

#define zmap_cursor_width 16
#define zmap_cursor_height 16

  /* My thick cross cursor. */
#define zmap_cross_shape_x_hot 7
#define zmap_cross_shape_y_hot 7
static unsigned char zmap_cross_shape_bits[] = {
   0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01,
   0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01,
   0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01};
static unsigned char zmap_cross_mask_bits[] = {
   0xe0, 0x03, 0xe0, 0x03, 0xe0, 0x03, 0xe0, 0x03, 0xe0, 0x03, 0xff, 0x7f,
   0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xff, 0x7f, 0xe0, 0x03, 0xe0, 0x03,
   0xe0, 0x03, 0xe0, 0x03, 0xe0, 0x03, 0xe0, 0x03};

/* Thin cross. */
#define zmap_thincross_shape_x_hot 7
#define zmap_thincross_shape_y_hot 7
static unsigned char zmap_thincross_shape_bits[] = {
   0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
   0x80, 0x01, 0x7f, 0xfe, 0x7f, 0xfe, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
   0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01};
static unsigned char zmap_thincross_mask_bits[] = {
   0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03,
   0xff, 0xff, 0x7f, 0xfe, 0x7f, 0xfe, 0xff, 0xff, 0xc0, 0x03, 0xc0, 0x03,
   0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03};

  /* My crosshair cursor. */
#define zmap_crosshair_shape_x_hot 7
#define zmap_crosshair_shape_y_hot 7
static unsigned char zmap_crosshair_shape_bits[] = {
   0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x0f, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00,
   0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00};
static unsigned char zmap_crosshair_mask_bits[] = {
   0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x0f, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00,
   0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00};

  /* My colour cross cursor. */
#define zmap_colour_cross_shape_x_hot 7
#define zmap_colour_cross_shape_y_hot 7
static unsigned char zmap_colour_cross_shape_bits[] = {
   0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
   0x80, 0x01, 0xff, 0xff, 0xff, 0xff, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
   0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01};
static unsigned char zmap_colour_cross_mask_bits[] = {
   0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02,
   0x7f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfe, 0x40, 0x02, 0x40, 0x02,
   0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02};

/* My circle cursor. */
#define zmap_circle_shape_x_hot 7
#define zmap_circle_shape_y_hot 7
 static unsigned char zmap_circle_shape_bits[] = {
   0x00, 0x00, 0xe0, 0x03, 0x10, 0x04, 0x08, 0x08, 0x04, 0x10, 0x02, 0x20,
   0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x04, 0x10, 0x08, 0x08,
   0x10, 0x04, 0xe0, 0x03, 0x00, 0x00, 0x00, 0x00};
 static unsigned char zmap_circle_mask_bits[] = {
   0xc0, 0x01, 0xf0, 0x07, 0x18, 0x0c, 0x0c, 0x18, 0x06, 0x30, 0x02, 0x20,
   0x03, 0x60, 0x03, 0x60, 0x03, 0x60, 0x02, 0x20, 0x06, 0x30, 0x0c, 0x18,
   0x18, 0x0c, 0xf0, 0x07, 0xc0, 0x01, 0x00, 0x00};

 /* My No Entry cursor. */
#define zmap_noentry_shape_x_hot 7
#define zmap_noentry_shape_y_hot 7
static unsigned char zmap_noentry_shape_bits[] = {
   0xc0, 0x01, 0x30, 0x06, 0x0c, 0x18, 0x1c, 0x10, 0x3a, 0x20, 0x72, 0x20,
   0xe1, 0x40, 0xc1, 0x41, 0x81, 0x43, 0x02, 0x27, 0x02, 0x2e, 0x04, 0x1c,
   0x0c, 0x18, 0x30, 0x06, 0xc0, 0x01, 0x00, 0x00};
static unsigned char zmap_noentry_mask_bits[] = {
   0xc0, 0x01, 0x30, 0x06, 0x0c, 0x18, 0x1c, 0x10, 0x3a, 0x20, 0x72, 0x20,
   0xe1, 0x40, 0xc1, 0x41, 0x81, 0x43, 0x02, 0x27, 0x02, 0x2e, 0x04, 0x1c,
   0x0c, 0x18, 0x30, 0x06, 0xc0, 0x01, 0x00, 0x00};


 gchar *shape_data, *mask_data ;
 gint hot_x, hot_y ;
 gboolean found_cursor = FALSE ;
 GdkPixmap *source, *mask;
 GdkColor red = { 0, 65535, 0, 0 };			    /* Red. */
 GdkColor blue = { 0, 0, 0, 65535 };			    /* Blue. */
 GdkColor black = { 0, 0, 0, 0 };			    /* Black. */
 GdkColor white = { 0, 65535, 65535, 65535 };		    /* White. */
 GdkColor *fg, *bg ;


 if (g_ascii_strcasecmp(cursor_name, ZMAPGUI_CURSOR_COLOUR_CROSS) == 0)
   {
     shape_data = (gchar *)zmap_colour_cross_shape_bits ;
     mask_data = (gchar *)zmap_colour_cross_mask_bits ;
     hot_x = zmap_colour_cross_shape_x_hot ;
     hot_y = zmap_colour_cross_shape_y_hot ;
     fg = &red ;
     bg = &blue ;

     found_cursor = TRUE ;
   }
 else if (g_ascii_strcasecmp(cursor_name, ZMAPGUI_CURSOR_CROSS) == 0)
   {
     shape_data = (gchar *)zmap_cross_shape_bits ;
     mask_data = (gchar *)zmap_cross_mask_bits ;
     hot_x = zmap_cross_shape_x_hot ;
     hot_y = zmap_cross_shape_y_hot ;
     fg = &black ;
     bg = &white ;
     found_cursor = TRUE ;
   }
 else if (g_ascii_strcasecmp(cursor_name, ZMAPGUI_THINCURSOR_CROSS) == 0)
   {
     shape_data = (gchar *)zmap_thincross_shape_bits ;
     mask_data = (gchar *)zmap_thincross_mask_bits ;
     hot_x = zmap_thincross_shape_x_hot ;
     hot_y = zmap_thincross_shape_y_hot ;
     fg = &black ;
     bg = &white ;
     found_cursor = TRUE ;
   }
 else if (g_ascii_strcasecmp(cursor_name, ZMAPGUI_CURSOR_CROSSHAIR) == 0)
   {
     shape_data = (gchar *)zmap_crosshair_shape_bits ;
     mask_data = (gchar *)zmap_crosshair_mask_bits ;
     hot_x = zmap_crosshair_shape_x_hot ;
     hot_y = zmap_crosshair_shape_y_hot ;
     fg = bg = &black ;

     found_cursor = TRUE ;
   }
 else if (g_ascii_strcasecmp(cursor_name, ZMAPGUI_CURSOR_CIRCLE) == 0)
   {
     shape_data = (gchar *)zmap_circle_shape_bits ;
     mask_data = (gchar *)zmap_circle_mask_bits ;
     hot_x = zmap_circle_shape_x_hot ;
     hot_y = zmap_circle_shape_y_hot ;
     fg = &red ;
     bg = &blue ;

     found_cursor = TRUE ;
   }
 else if (g_ascii_strcasecmp(cursor_name, ZMAPGUI_CURSOR_NOENTRY) == 0)
   {
     shape_data = (gchar *)zmap_noentry_shape_bits ;
     mask_data = (gchar *)zmap_noentry_mask_bits ;
     hot_x = zmap_noentry_shape_x_hot ;
     hot_y = zmap_noentry_shape_y_hot ;
     fg = &black ;
     bg = &white ;

     found_cursor = TRUE ;
   }

 if (found_cursor)
   {
     source = gdk_bitmap_create_from_data(NULL, shape_data,
					  zmap_cursor_width, zmap_cursor_height) ;
     mask = gdk_bitmap_create_from_data(NULL, mask_data,
					zmap_cursor_width, zmap_cursor_height);

     cursor = gdk_cursor_new_from_pixmap (source, mask, fg, bg, hot_x, hot_y) ;

     g_object_unref (source);
     g_object_unref (mask);
   }


  return cursor ;
}


GdkCursor *makeStandardCursor(char *cursor_name)
{
  GdkCursor *cursor = NULL ;
  CursorNameStruct cursors[] =
    {
      {GDK_X_CURSOR, "X_CURSOR"},
      {GDK_ARROW, "ARROW"},
      {GDK_BASED_ARROW_DOWN, "BASED_ARROW_DOWN"},
      {GDK_BASED_ARROW_UP, "BASED_ARROW_UP"},
      {GDK_BOAT, "BOAT"},
      {GDK_BOGOSITY, "BOGOSITY"},
      {GDK_BOTTOM_LEFT_CORNER, "BOTTOM_LEFT_CORNER"},
      {GDK_BOTTOM_RIGHT_CORNER, "BOTTOM_RIGHT_CORNER"},
      {GDK_BOTTOM_SIDE, "BOTTOM_SIDE"},
      {GDK_BOTTOM_TEE, "BOTTOM_TEE"},
      {GDK_BOX_SPIRAL, "BOX_SPIRAL"},
      {GDK_CENTER_PTR, "CENTER_PTR"},
      {GDK_CIRCLE, "CIRCLE"},
      {GDK_CLOCK, "CLOCK"},
      {GDK_COFFEE_MUG, "COFFEE_MUG"},
      {GDK_CROSS, "CROSS"},
      {GDK_CROSS_REVERSE, "CROSS_REVERSE"},
      {GDK_CROSSHAIR, "CROSSHAIR"},
      {GDK_DIAMOND_CROSS, "DIAMOND_CROSS"},
      {GDK_DOT, "DOT"},
      {GDK_DOTBOX, "DOTBOX"},
      {GDK_DOUBLE_ARROW, "DOUBLE_ARROW"},
      {GDK_DRAFT_LARGE, "DRAFT_LARGE"},
      {GDK_DRAFT_SMALL, "DRAFT_SMALL"},
      {GDK_DRAPED_BOX, "DRAPED_BOX"},
      {GDK_EXCHANGE, "EXCHANGE"},
      {GDK_FLEUR, "FLEUR"},
      {GDK_GOBBLER, "GOBBLER"},
      {GDK_GUMBY, "GUMBY"},
      {GDK_HAND1, "HAND1"},
      {GDK_HAND2, "HAND2"},
      {GDK_HEART, "HEART"},
      {GDK_ICON, "ICON"},
      {GDK_IRON_CROSS, "IRON_CROSS"},
      {GDK_LEFT_PTR, "LEFT_PTR"},
      {GDK_LEFT_SIDE, "LEFT_SIDE"},
      {GDK_LEFT_TEE, "LEFT_TEE"},
      {GDK_LEFTBUTTON, "LEFTBUTTON"},
      {GDK_LL_ANGLE, "LL_ANGLE"},
      {GDK_LR_ANGLE, "LR_ANGLE"},
      {GDK_MAN, "MAN"},
      {GDK_MIDDLEBUTTON, "MIDDLEBUTTON"},
      {GDK_MOUSE, "MOUSE"},
      {GDK_PENCIL, "PENCIL"},
      {GDK_PIRATE, "PIRATE"},
      {GDK_PLUS, "PLUS"},
      {GDK_QUESTION_ARROW, "QUESTION_ARROW"},
      {GDK_RIGHT_PTR, "RIGHT_PTR"},
      {GDK_RIGHT_SIDE, "RIGHT_SIDE"},
      {GDK_RIGHT_TEE, "RIGHT_TEE"},
      {GDK_RIGHTBUTTON, "RIGHTBUTTON"},
      {GDK_RTL_LOGO, "RTL_LOGO"},
      {GDK_SAILBOAT, "SAILBOAT"},
      {GDK_SB_DOWN_ARROW, "SB_DOWN_ARROW"},
      {GDK_SB_H_DOUBLE_ARROW, "SB_H_DOUBLE_ARROW"},
      {GDK_SB_LEFT_ARROW, "SB_LEFT_ARROW"},
      {GDK_SB_RIGHT_ARROW, "SB_RIGHT_ARROW"},
      {GDK_SB_UP_ARROW, "SB_UP_ARROW"},
      {GDK_SB_V_DOUBLE_ARROW, "SB_V_DOUBLE_ARROW"},
      {GDK_SHUTTLE, "SHUTTLE"},
      {GDK_SIZING, "SIZING"},
      {GDK_SPIDER, "SPIDER"},
      {GDK_SPRAYCAN, "SPRAYCAN"},
      {GDK_STAR, "STAR"},
      {GDK_TARGET, "TARGET"},
      {GDK_TCROSS, "TCROSS"},
      {GDK_TOP_LEFT_ARROW, "TOP_LEFT_ARROW"},
      {GDK_TOP_LEFT_CORNER, "TOP_LEFT_CORNER"},
      {GDK_TOP_RIGHT_CORNER, "TOP_RIGHT_CORNER"},
      {GDK_TOP_SIDE, "TOP_SIDE"},
      {GDK_TOP_TEE, "TOP_TEE"},
      {GDK_TREK, "TREK"},
      {GDK_UL_ANGLE, "UL_ANGLE"},
      {GDK_UMBRELLA, "UMBRELLA"},
      {GDK_UR_ANGLE, "UR_ANGLE"},
      {GDK_WATCH, "WATCH"},
      {GDK_XTERM, "XTERM"},
      {GDK_LAST_CURSOR, NULL}				    /* end of array marker. */
    } ;
  CursorName curr_cursor ;

  curr_cursor = cursors ;
  while (curr_cursor->cursor_id != GDK_LAST_CURSOR)
    {
      if (g_ascii_strcasecmp(cursor_name, curr_cursor->cursor_name) == 0)
	{
	  cursor = gdk_cursor_new(curr_cursor->cursor_id) ;
	  break ;
	}
      else
	{
	  curr_cursor++ ;
	}
    }

  return cursor ;
}


/* Convert ZMapGuiTextAttr to GTK TextTag structs. */
static void setTextAttrs(gpointer data, gpointer user_data)
{
  ZMapGuiTextAttr text_attrs = (ZMapGuiTextAttr)data ;
  TextAttrs text_attr_data = (TextAttrs)user_data ;
  GtkTextBuffer *buffer = text_attr_data->buffer ;
  GtkTextIter start, end ;
  GtkTextTag *tag ;
  char *first_attr, *second_attr ;
  gpointer first_value, second_value ;
  int iter_start, iter_end ;
  int line_start, line_end, line_diff ;
  char *curr_char ;
  int tmp_start, tmp_end, tmp_line_start, tmp_line_end ;
  gboolean text_debug = FALSE ;



  /*
   * Create a tag to colour the text.
   */

  /* note that there is no array style call so you cannot (neatly) dynamically
   * construct the args so here call is with all possible args but setting them
   * so that required ones come first and the rest are NULL. */
  first_attr = second_attr = NULL ;
  first_value = second_value = NULL ;

  if (text_attrs->foreground)
    {
      first_attr = "foreground-gdk" ;
      first_value = text_attrs->foreground ;
    }

  if (text_attrs->background)
    {
      if (!first_attr)
	{
	  first_attr = "background-gdk" ;
	  first_value = text_attrs->background ;
	}
      else
	{
	  second_attr = "background-gdk" ;
	  second_value = text_attrs->background ;
	}
    }

  /* Create anonymous tags as we don't reuse them. */
  tag = gtk_text_buffer_create_tag(buffer, NULL,
				   first_attr, first_value,
				   second_attr, second_value,
				   NULL) ;



  /*
   * Now calculate the correct range in the text over which to apply the tag.
   */

  /* We are given character positions and need to widen them to iter positions
   * which are inter-character positions. */
  iter_start = text_attrs->start ;
  iter_end = text_attrs->end + 1 ;

  /* Newlines in the text count as characters so we need to modify the
   * character start/ends to take account of them otherwise highlighting
   * is done in wrong place. (NOTE: really gtk should allow us to request this.)
   * Finding the number of newlines and hence our true position is iterative (we
   * don't know the content of the text so can't count newlines directly). */
  tmp_start = iter_start ;
  tmp_end = iter_end ;

  /* Find out where we are intially. */
  gtk_text_buffer_get_iter_at_offset(buffer, &start, tmp_start) ;
  gtk_text_buffer_get_iter_at_offset (buffer, &end, tmp_start + 1) ;

  /* Pathological case is that we are at a newline so bump over it. */
  if (*(curr_char = gtk_text_buffer_get_text(buffer, &start, &end, FALSE)) == '\n')
    {
      gtk_text_buffer_get_iter_at_offset(buffer, &start, tmp_start + 1) ;
      gtk_text_buffer_get_iter_at_offset (buffer, &end, tmp_end + 1) ;

      curr_char = gtk_text_buffer_get_text(buffer, &start, &end, FALSE) ;
    }


  /* Get number of newlines from our current line position and correct start/end position. */
  line_start = gtk_text_iter_get_line(&start) ;
  line_end = gtk_text_iter_get_line(&end) ;

  tmp_start += line_start ;
  tmp_end += line_start ;


  /* Find out where we are now. */
  gtk_text_buffer_get_iter_at_offset(buffer, &start, tmp_start) ;
  gtk_text_buffer_get_iter_at_offset (buffer, &end, tmp_end) ;

  if (text_debug)
    curr_char = gtk_text_buffer_get_text(buffer, &start, &end, FALSE) ;


  /* If adjusting our position has bumped us on a few lines then correct
   * our position again, we have to iterate here because each correction
   * may move us past more newlines but we do stop in the end. */
  tmp_line_start = gtk_text_iter_get_line(&start) ;

  while (tmp_line_start != line_start)
    {
      int diff ;

      diff = tmp_line_start - line_start ;

      tmp_start += diff ;
      tmp_end += diff ;

      gtk_text_buffer_get_iter_at_offset(buffer, &start, tmp_start) ;
      gtk_text_buffer_get_iter_at_offset (buffer, &end, tmp_end) ;

      if (text_debug)
	curr_char = gtk_text_buffer_get_text(buffer, &start, &end, FALSE) ;

      line_start = tmp_line_start ;
      tmp_line_start = gtk_text_iter_get_line(&start) ;
    }


  iter_start = tmp_start ;
  iter_end = tmp_end ;


  /* Extent of this text may be spread over several lines so adjust for that. */
  tmp_start = iter_start ;
  tmp_end = iter_end ;

  line_start = gtk_text_iter_get_line(&start) ;
  line_end = gtk_text_iter_get_line(&end) ;

  line_diff = line_end - line_start ;
  tmp_end += line_diff ;

  gtk_text_buffer_get_iter_at_offset(buffer, &start, tmp_start) ;
  gtk_text_buffer_get_iter_at_offset (buffer, &end, tmp_end) ;

  if (text_debug)
    curr_char = gtk_text_buffer_get_text(buffer, &start, &end, FALSE) ;


  /* If adjusting our position has bumped us on a few lines then correct
   * our position again, we have to iterate here because each correction
   * may move us past more newlines. */
  tmp_line_end = gtk_text_iter_get_line(&end) ;

  while (tmp_line_end != line_end)
      {
	int diff ;

	diff = tmp_line_end - line_end ;

	tmp_end += diff ;

	gtk_text_buffer_get_iter_at_offset(buffer, &start, tmp_start) ;
	gtk_text_buffer_get_iter_at_offset (buffer, &end, tmp_end) ;

	if (text_debug)
	  curr_char = gtk_text_buffer_get_text(buffer, &start, &end, FALSE) ;

	line_end = tmp_line_end ;
	tmp_line_end = gtk_text_iter_get_line(&end) ;
      }


    iter_start = tmp_start ;
    iter_end = tmp_end ;



  /*
   * Phew...now apply the tag to the text.
   */

  gtk_text_buffer_apply_tag(buffer, tag, &start, &end) ;



  return ;
}

/* A GtkAboutDialogActivateLinkFunc() called when user clicks on website link in "About" window.
 * This is the pre GTK 2.24 version. */
static void aboutLinkOldCB(GtkAboutDialog *about, const gchar *link, gpointer data)
  {

    aboutLinkNewCB(NULL, (gchar *)link, NULL) ;

    return ;
  }


/* "activate-link" callback called when user clicks on website link in "About" window.
 * This is the GTK 2.24 and later version. */
static gboolean aboutLinkNewCB(GtkAboutDialog *label, gchar *uri, gpointer user_data_unused)
{
  gboolean result = TRUE ;
  GError *error = NULL ;

  if (!zMapLaunchWebBrowser(uri, &error))
    zMapLogWarning("Cannot show link in web browser: \"%s\"", uri) ;

  return result ;
}
