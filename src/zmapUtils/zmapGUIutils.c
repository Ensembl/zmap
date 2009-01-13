/*  File: zmapGUIutils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Various GUI convenience functions for messages, text,
 *              notebook creation and so on.
 *              
 * Exported functions: See ZMap/zmapUtilsGUI.h
 * HISTORY:
 * Last edited: Jan 13 14:57 2009 (edgrif)
 * Created: Thu Jul 24 14:37:35 2003 (edgrif)
 * CVS info:   $Id: zmapGUIutils.c,v 1.54 2009-01-13 15:03:22 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <gtk/gtk.h>
#include <math.h>

#include <ZMap/zmapUtilsGUI.h>
#include <ZMap/zmapWebPages.h>
#include <zmapUtils_P.h>


typedef struct
{
  int value;
  int *output;
  ZMapGUIRadioButtonCBFunc callback;
  gpointer user_data;
}RadioButtonCBDataStruct, *RadioButtonCBData;

static gboolean modalFromMsgType(ZMapMsgType msg_type) ;
static gboolean messageFull(GtkWindow *parent, char *title_in, char *msg,
			    gboolean modal, int display_timeout, gboolean close_button,
			    ZMapMsgType msg_type, GtkJustification justify,
			    ZMapGUIMsgUserData user_data) ;
static void butClick(GtkButton *button, gpointer user_data) ;
static gboolean timeoutHandlerModal(gpointer data) ;
static gboolean timeoutHandler(gpointer data) ;


static void responseCB(GtkDialog *toplevel, gint arg1, gpointer user_data) ;


/* Holds an alternative URL for help pages if set by the application. */
static char *help_URL_base_G = NULL ;



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
 *
 * @param toplevel     This must be the dialog widget itself.
 * @return             an integer which corresponds to the button pressed.
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

  while (*response_ptr == 0 || gtk_events_pending())
    gtk_main_iteration() ;

  result = *response_ptr ;

  g_free(response_ptr) ;

  return result ;
}



/*!
 * Returns a title string in standard form to be used for the window manager title
 * bar in zmap windows.
 *
 * Currently the format is:
 * 
 *             "ZMap (version) <window_type> - <text>"
 * 
 * Either the window_type or the text can be NULL but not both.
 * 
 * The returned string should be free'd by the caller using g_free() when no longer required.
 * 
 *
 * @param window_type  The sort of window it is, e.g. "feature editor"
 * @param message      Very short text, e.g. "Please Reply" or a feature name or....
 * @return             the title string.
 *  */
char *zMapGUIMakeTitleString(char *window_type, char *message)
{
  char *title = NULL ;

  zMapAssert(!(window_type == NULL && message == NULL)) ;

  title = g_strdup_printf("ZMap (%s)%s%s%s%s",
			  zMapGetVersionString(),
			  (window_type ? " " : ""),
			  (window_type ? window_type : ""),
			  (message ? " - " : ""),
			  (message ? message : "")) ;

  return title ;
}

/*!
 * Trivial cover function for creating a new toplevel window complete 
 * with the title set as per zMapGUIMakeTitleString above.
 *
 * @param window_type  The sort of window it is, e.g. "feature editor"
 * @param message      Very short text, e.g. "Please Reply" or a feature name or....
 * @return             the toplevel widget.
 */
GtkWidget *zMapGUIToplevelNew(char *zmap_win_type, char *zmap_win_text)
{
  GtkWidget *toplevel = NULL;
  char *title = NULL;

  toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  if((title = zMapGUIMakeTitleString(zmap_win_type, zmap_win_text)))
    {
      gtk_window_set_title(GTK_WINDOW(toplevel), title);
      g_free(title);
    }

  return toplevel;
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
#if GTK_MAJOR_VERSION == (2) && GTK_MINOR_VERSION >= (6)
  const gchar *authors[] = {"Ed Griffiths, Sanger Institute, UK <edgrif@sanger.ac.uk>",
			    "Roy Storey Sanger Institute, UK <rds@sanger.ac.uk>",
			    NULL} ;

  gtk_show_about_dialog(NULL,
			"authors", authors,
			"comments", zMapGetCommentsString(), 
			"copyright", zMapGetCopyrightString(),
			"license", zMapGetLicenseString(),
			"name", zMapGetAppName(),
			"version", zMapGetVersionString(),
			"website", zMapGetWebSiteString(),
			NULL) ;
#endif

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
  gboolean result ;
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

    default:
      zMapAssertNotReached() ;

    }

  if (!(result = zMapLaunchWebBrowser(web_page, &error)))
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

  if (messageFull(parent, NULL, msg, modal, 0, TRUE, msg_type, GTK_JUSTIFY_CENTER, &user_data))
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
char *zMapGUIMsgGetText(GtkWindow *parent, ZMapMsgType msg_type, char *msg, gboolean hide_text)
{
  char *text = NULL ;
  ZMapGUIMsgUserDataStruct user_data = {ZMAPGUI_USERDATA_TEXT, FALSE, {FALSE}} ;
  gboolean modal ;

  modal = modalFromMsgType(msg_type) ;

  user_data.hide_input = hide_text ;

  if (messageFull(parent, NULL, msg, modal, 0, TRUE, msg_type, GTK_JUSTIFY_CENTER, &user_data))
    text = user_data.data.text ;

  return text ;
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

  dialog = zMapGUIShowTextFull(title, text, edittable, NULL) ;

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
GtkWidget *zMapGUIShowTextFull(char *title, char *text, gboolean edittable, GtkTextBuffer **buffer_out)
{
  enum {TEXT_X_BORDERS = 32, TEXT_Y_BORDERS = 50} ;
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


  dialog = gtk_dialog_new_with_buttons(title, NULL, flags,
				       "Close", GTK_RESPONSE_NONE,
				       NULL) ;
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

  /* Construct a list of possible fonts to use. */
  fixed_font_list = g_list_append(fixed_font_list, "Monospace") ;
  fixed_font_list = g_list_append(fixed_font_list, "fixed") ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* I tried to give several alternative fonts but some of them, e.g. Cursor, do not display
   * as monospace even though they apparently are...I don't know why... */
  fixed_font_list = g_list_append(fixed_font_list, "Serif") ;
  fixed_font_list = g_list_append(fixed_font_list, "Cursor") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




  /* Here we try to set a fixed width font in the text widget and set the size of the dialog
   * so that a sensible amount of text is displayed. */
  if (!zMapGUIGetFixedWidthFont(view,
				fixed_font_list, 10, PANGO_WEIGHT_NORMAL,
				&font, &font_desc))
    {
      zMapGUIShowMsg(ZMAP_MSG_WARNING, "Could not get fixed width font, "
		     "message window may be a strange size") ;
    }
  else
    {
      /* code attempts to get font size and then set window size accordingly, it now appears
       * to get this wrong so I have hacked this for now... */


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

      gtk_window_set_default_size(GTK_WINDOW(dialog), text_width, text_height) ;
    }

  g_list_free(fixed_font_list) ;

  gtk_widget_show_all(dialog) ;

  return dialog;
}


/* Returns path of file chosen by user which can be used directly to open the file,
 * it is the callers responsibility to free the filepath using g_free().
 * Caller can optionally specify a default directory. */
char *zmapGUIFileChooserFull(GtkWidget *toplevel,  char *title, char *directory_in, char *file_suffix,
			     ZMapFileChooserContentAreaCB content_func, gpointer content_data)
{
  char *file_path = NULL ;
  GtkWidget *dialog ;

  zMapAssert(toplevel) ;

  if (!title)
    title = "Please give a file name:" ;

  /* Set up the dialog. */
  dialog = gtk_file_chooser_dialog_new(title,
				       GTK_WINDOW(toplevel),
				       GTK_FILE_CHOOSER_ACTION_SAVE,
				       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				       GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				       NULL) ;

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

/* Because we need to cast to the function type to call it without debugger complaints... */
typedef void (*g_object_notify_callback)(GObject *pane, GParamSpec *scroll, gpointer user_data);

/* data for our proxying */
typedef struct
{
  g_object_notify_callback callback;
  gpointer user_data;
  gulong handler_id;
} PaneMaxPositionStruct, *PaneMaxPosition;

static void pane_max_position_callback(GObject *pane, GParamSpec *scroll, gpointer user_data);
static void pane_max_position_destroy_notify(gpointer pane_max_pos_data, GClosure *unused_closure);


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



/*! @} end of zmapguiutils docs. */



/*
 *                      Internal functions                      
 */


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
static gboolean messageFull(GtkWindow *parent, char *title_in, char *msg,
			    gboolean modal, int display_timeout, gboolean close_button,
			    ZMapMsgType msg_type, GtkJustification justify,
			    ZMapGUIMsgUserData user_data)
{
  gboolean result = TRUE ;
  GtkWidget *dialog, *button, *label ;
  GtkWidget *entry = NULL ;
  char *title = NULL ;
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
	  title = "ZMAP - Information" ;
	  break ;
	case ZMAP_MSG_WARNING:
	  title = "ZMAP - Warning !" ;
	  break;
	case ZMAP_MSG_CRITICAL:
	  title = "ZMAP - CRITICAL !" ;
	  break;
	case ZMAP_MSG_EXIT:
	  title = "ZMAP - EXIT !" ;
	  break;
	case ZMAP_MSG_CRASH:
	  title = "ZMAP - CRASH !" ;
	  break;
	}
    }


  /* Set up the dialog, if user data is required we need all the buttons, otherwise
   * there will be a "close" or perhaps no buttons for messages that time out. */
  if (modal)
    flags |= GTK_DIALOG_MODAL ;

  if (user_data)
    dialog = gtk_dialog_new_with_buttons(title, parent, flags,
					 GTK_STOCK_OK,
					 GTK_RESPONSE_ACCEPT,
					 GTK_STOCK_CANCEL,
					 GTK_RESPONSE_REJECT,
					 NULL) ;
  else if (close_button)
    dialog = gtk_dialog_new_with_buttons(title, parent, flags,
					 "Close", GTK_RESPONSE_NONE,
					 NULL) ;
  else
    dialog = gtk_dialog_new_with_buttons(title, parent, flags,
					 NULL) ;
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
      gint result ;

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
	    case GTK_RESPONSE_ACCEPT:
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
		      if ((text = (char *)gtk_entry_get_text(GTK_ENTRY(entry)))
			  && *text)			    /* entry returns "" for no string. */
			user_data->data.text = g_strdup(text) ;
		      else
			result = FALSE ;

		      break ;
		    }
		  default:
		    {
		      result = FALSE ;

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
		      result = FALSE ;

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

