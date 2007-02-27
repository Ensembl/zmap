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
 * Description: 
 * Exported functions: See ZMap/zmapUtilsGUI.h
 * HISTORY:
 * Last edited: Feb 27 10:42 2007 (rds)
 * Created: Thu Jul 24 14:37:35 2003 (edgrif)
 * CVS info:   $Id: zmapGUIutils.c,v 1.30 2007-02-27 10:42:25 rds Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <gtk/gtk.h>
#include <math.h>
#include <zmapUtils_P.h>
#include <ZMap/zmapUtilsGUI.h>

typedef struct
{
  int value;
  int *output;
  ZMapGUIRadioButtonCBFunc callback;
  gpointer user_data;
}RadioButtonCBDataStruct, *RadioButtonCBData;

/* ONLY NEEDED FOR OLD STYLE FILE SELECTOR, REMOVE WHEN WE CAN USE THE NEW CODE... */
typedef struct
{
  GtkWidget *file_selector ;
  char *file_path_out ;
  char *suffix ;
} FileDialogCBStruct, *FileDialogCB ;
static void store_filename(GtkWidget *widget, gpointer user_data) ;
static void killFileDialog(GtkWidget *widget, gpointer user_data) ;
/* ONLY NEEDED FOR OLD STYLE FILE SELECTOR, REMOVE WHEN WE CAN USE THE NEW CODE... */

static void butClick(GtkButton *button, gpointer user_data) ;

static void responseCB(GtkDialog *toplevel, gint arg1, gpointer user_data) ;



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
 * Gtk provides a function called  gtk_dialog_run() which blocks until the user presses
 * a button on the dialot, the function returns an int indicating which button was 
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
 * Shows the usual "About" window.
 *
 *
 * @param window_type  The sort of window it is, e.g. "feature editor"
 * @param message      Very short text, e.g. "Please Reply" or a feature name or....
 * @return             nothing
 *  */
void zMapGUIShowAbout(void)
{
#if GTK_MAJOR_VERSION == (2) && GTK_MINOR_VERSION >= (4)
  GtkWidget *about_dialog ;
  const gchar *authors[] = {"Ed Griffiths, Sanger Institute, UK <edgrif@sanger.ac.uk>",
			    "Roy Storey Sanger Institute, UK <rds@sanger.ac.uk>",
			    NULL} ;

  about_dialog = gtk_about_dialog_new() ;

  gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(about_dialog), zMapGetAppName()) ;

  gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog), zMapGetVersionString()) ;

  gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about_dialog), zMapGetCopyrightString()) ;

  gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog), zMapGetCommentsString()) ;

  gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about_dialog), zMapGetLicenseString()) ;

  gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dialog), zMapGetWebSiteString()) ;

  gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dialog), authors) ;

  gtk_widget_show_all(about_dialog) ;

#endif
  return ;
}


/*!
 * Trivial cover function for zmapGUIShowMsgOnTop(), see documentation of that function
 * for description and parameter details.
 *
 *  */
void zMapGUIShowMsg(ZMapMsgType msg_type, char *msg)
{
  zMapGUIShowMsgFull(NULL, msg, msg_type, GTK_JUSTIFY_CENTER) ;

  return ;
}



/*!
 * Display a short message in a pop-up dialog box, the behaviour of the dialog depends on
 * the message type:
 *
 *               ZMAP_MSG_INFORMATION, ZMAP_MSG_WARNING - non-blocking, non-modal
 *     ZMAP_MSG_CRITICAL, ZMAP_MSG_EXIT, ZMAP_MSG_CRASH - blocking and modal
 *
 * If parent is non-NULL then the dialog will be kept on top of that window, essential for
 * modal dialogs in particular. I think parent should be the application window that the message
 * applies to probably, or perhaps the application main window.
 *
 * @param parent       Widget that message should be kept on top of or NULL.
 * @param msg_type     ZMAP_MSG_INFORMATION | ZMAP_MSG_WARNING | ZMAP_MSG_CRITICAL | ZMAP_MSG_EXIT | ZMAP_MSG_CRASH
 * @param msg          Message to be displayed in dialog.
 * @return             nothing
 *  */
void zMapGUIShowMsgFull(GtkWindow *parent, char *msg,
			ZMapMsgType msg_type,
			GtkJustification justify)
{
  GtkWidget *dialog, *button, *label ;
  char *title = NULL ;
  GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT ;
  gboolean modal = FALSE ;


  /* relies on order of ZMapMsgType enum.... */
  zMapAssert((msg_type >= ZMAP_MSG_INFORMATION || msg_type <= ZMAP_MSG_CRASH)
	     && (msg && *msg)) ;

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

  /* Some times of dialog should be modal. */
  if (msg_type == ZMAP_MSG_CRITICAL || msg_type == ZMAP_MSG_EXIT || msg_type == ZMAP_MSG_CRASH)
    modal = TRUE ;

  if (modal)
    flags |= GTK_DIALOG_MODAL ;

  dialog = gtk_dialog_new_with_buttons(title, parent, flags,
				       "Close", GTK_RESPONSE_NONE,
				       NULL) ;
  gtk_container_set_border_width( GTK_CONTAINER(dialog), 5 );


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


  gtk_widget_show_all(dialog) ;


  /* For modal messages we block waiting for a response otherwise we return and gtk will
   * clear up for us when the user quits.... */
  if (modal)
    {
      gint result ;

      /* block waiting for user to answer dialog. */
      result = gtk_dialog_run(GTK_DIALOG(dialog)) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* currently we don't need to monitor the response...but if we did... */
      switch (result)
	{
	case GTK_RESPONSE_ACCEPT:
	  do_application_specific_something ();
	  break;
	default:
	  do_nothing_since_dialog_was_cancelled ();
	  break;
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      gtk_widget_destroy(dialog) ;
    }
  else
    {
      /* Ensure that the dialog box is destroyed when the user responds. */
      g_signal_connect_swapped(dialog,
			       "response", 
			       G_CALLBACK(gtk_widget_destroy),
			       dialog) ;
    }


  return ;
}



/*!
 * 
 * WARNING: THIS NEEDS MERGING WITH THE ROUTINE ABOVE.....
 * 
 * 
 * Display a short message in a pop-up dialog box, the behaviour of the dialog depends on
 * the message type:
 *
 *     ZMAP_MSG_INFORMATION, ZMAP_MSG_WARNING - non-blocking, non-modal
 *     ZMAP_MSG_CRITICAL, ZMAP_MSG_EXIT, ZMAP_MSG_CRASH - blocking and modal
 *
 * If parent is non-NULL then the dialog will be kept on top of that window, essential for
 * modal dialogs in particular. I think parent should be the application window that the message
 * applies to probably, or perhaps the application main window.
 *
 * @param parent       Widget that message should be kept on top of or NULL.
 * @param msg_type     ZMAP_MSG_INFORMATION | ZMAP_MSG_WARNING | ZMAP_MSG_CRITICAL | ZMAP_MSG_EXIT | ZMAP_MSG_CRASH
 * @param msg          Message to be displayed in dialog.
 * @return             nothing
 *  */
gboolean zMapGUIShowChoice(GtkWindow *parent, ZMapMsgType msg_type, char *msg)
{
  gboolean accept = FALSE ;
  GtkWidget *dialog, *button, *label ;
  char *title = NULL ;
  GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT ;
  gboolean modal = FALSE ;


  /* relies on order of ZMapMsgType enum.... */
  zMapAssert((msg_type >= ZMAP_MSG_INFORMATION || msg_type <= ZMAP_MSG_CRASH)
	     && (msg && *msg)) ;

  switch(msg_type)
    {
    case ZMAP_MSG_INFORMATION:
      title = "ZMAP - Information" ;
      break ;
    case ZMAP_MSG_WARNING:
      title = "ZMAP - Warning!" ;
      break;
    case ZMAP_MSG_CRITICAL:
      title = "ZMAP - Critical!" ;
      break;
    case ZMAP_MSG_EXIT:
      title = "ZMAP - Error!" ;
      break;
    case ZMAP_MSG_CRASH:
      title = "ZMAP - Crash!" ;
      break;
    }

  /* Some times of dialog should be modal. */
  if (msg_type == ZMAP_MSG_EXIT || msg_type == ZMAP_MSG_CRASH)
    modal = TRUE ;


  /* Force this for now... */
  modal = TRUE ;


  if (modal)
    flags |= GTK_DIALOG_MODAL ;

  dialog = gtk_dialog_new_with_buttons(title, parent, flags,
				       GTK_STOCK_OK,
				       GTK_RESPONSE_ACCEPT,
				       GTK_STOCK_CANCEL,
				       GTK_RESPONSE_REJECT,
				       NULL) ;
  gtk_container_set_border_width( GTK_CONTAINER(dialog), 5 );


  /* Set up the message text in a button widget so that it can put in the primary
   * selection buffer for cut/paste when user clicks on it. */
  button = gtk_button_new_with_label(msg) ;
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), button, TRUE, TRUE, 20);
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE) ;
  label = gtk_bin_get_child(GTK_BIN(button)) ;		    /* Center + wrap long lines. */
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER) ;
  gtk_label_set_line_wrap(GTK_LABEL(label), TRUE) ;

  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     GTK_SIGNAL_FUNC(butClick), button) ;


  gtk_widget_show_all(dialog) ;


  /* For modal messages we block waiting for a response otherwise we return and gtk will
   * clear up for us when the user quits.... */
  if (modal)
    {
      gint result ;

      /* block waiting for user to answer dialog. */
      result = gtk_dialog_run(GTK_DIALOG(dialog)) ;

      /* currently we don't need to monitor the response...but if we did... */
      switch (result)
	{
	case GTK_RESPONSE_ACCEPT:
	  accept = TRUE ;
	  break;
	default:
	  accept = FALSE ;
	  break;
	}

      gtk_widget_destroy(dialog) ;
    }
  else
    {
      /* Ensure that the dialog box is destroyed when the user responds. */
      g_signal_connect_swapped(dialog,
			       "response", 
			       G_CALLBACK(gtk_widget_destroy),
			       dialog) ;
    }


  return accept ;
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



/*!
 * Display longer text in a text widget. The code attempts to display the window
 * as 80 chars wide. Currently both title and text must be provided.
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


  return ;
}





/* THERE IS SOME ARCHAIC CODE HERE, FORCED ON US BY OUR BACK LEVEL OF INSTALLATION OF GTK... */
/* THIS IS THE OLD CODE.....THE NEW CODE IS COMMENTED OUT AFTER THIS... */

/*!
 * Displays a GTK file chooser dialog and returns path of file chosen by user,
 * it is the callers responsibility to free the returned filepath using g_free().
 * Caller can optionally specify a title and a default directory for the dialog.
 *
 * @param toplevel     Currently unused but will be used when we switch to next
 *                     level of GTK.
 * @param title        Window title or NULL.
 * @param directory_in Default directory for file chooser (CWD is default) or NULL. NOTE
 *                     currently this seems not to work...bug in GTK ?
 * @param file_suffix  Specify a suffix that will be added to the filename if the user does
 *                     not supply it.
 * @return             nothing
 */
char *zmapGUIFileChooser(GtkWidget *toplevel, char *title,
			 char *directory_in, char *file_suffix)
{
  char *file_path = NULL ;
  GtkWidget *file_selector;
  FileDialogCBStruct cb_data = {NULL} ;

  zMapAssert(toplevel) ;

  if (file_suffix)
    cb_data.suffix = g_strdup(file_suffix) ;

  /* Create the selector */
  if (!title)
    title = "Please give a file name:" ;
  cb_data.file_selector = file_selector = gtk_file_selection_new(title) ;

  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(file_selector)) ;

  /* this appears just not to work at all so we can't set the directory or suffix... sigh............ */
  if (directory_in)
    {
      char *directory = NULL ;

      directory = g_strdup_printf("%s/",  directory_in) ;
      gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selector), directory) ;
      g_free(directory) ;
    }

  g_signal_connect (GTK_FILE_SELECTION(file_selector)->ok_button,
		    "clicked",
		    G_CALLBACK (store_filename),
		    &cb_data) ;
   			   
  /* Ensure that the dialog box is destroyed when the user clicks a button. */
  g_signal_connect(GTK_FILE_SELECTION(file_selector)->ok_button,
		   "clicked",
		   G_CALLBACK(killFileDialog), 
		   &cb_data);

  g_signal_connect(GTK_FILE_SELECTION(file_selector)->cancel_button,
		   "clicked",
		   G_CALLBACK(killFileDialog),
		   &cb_data); 
   

  /* Display the file dialog and block until we get an action from the user. */
  gtk_widget_show (file_selector);
  gtk_main() ;

  file_path = cb_data.file_path_out ;

  return file_path ;
}


/* THIS IS THE NEW CODE THAT WE WOULD LIKE TO USE.... */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Returns path of file chosen by user which can be used directly to open the file,
 * it is the callers responsibility to free the filepath using g_free().
 * Caller can optionally specify a default directory. */
char *zmapGUIFileChooser(GtkWidget *toplevel, char *directory)
{
  char *file_path ;
  GtkWidget *dialog;

  zMapAssert(toplevel) ;

  dialog = gtk_file_chooser_dialog_new ("Open File",
					toplevel,
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					NULL) ;

  if (directory)
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), directory) ;

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    file_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog)) ;

  gtk_widget_destroy (dialog);

  return file_path ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



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

/*! @} end of zmapguiutils docs. */


/*
 *  ------------------- Internal functions -------------------
 */

static void store_filename(GtkWidget *widget, gpointer user_data)
{
  FileDialogCB cb_data = (FileDialogCB)user_data ;
  char *file_path = NULL ;
  gboolean has_suffix ;

  file_path = (char *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(cb_data->file_selector)) ;

  if (cb_data->suffix)
    has_suffix = g_str_has_suffix(file_path, cb_data->suffix) ;

  cb_data->file_path_out = g_strdup_printf("%s%s%s",
					   file_path,
					   has_suffix ? "" : ".",
					   has_suffix ? "" : cb_data->suffix) ;

  return ;
}

static void killFileDialog(GtkWidget *widget, gpointer user_data)
{
  FileDialogCB cb_data = (FileDialogCB)user_data ;

  gtk_widget_destroy(cb_data->file_selector) ;

  gtk_main_quit() ;

  return ;
}



/* Record the response from the dialog so we can detect it in our event hanlding loop. */
static void responseCB(GtkDialog *toplevel, gint arg1, gpointer user_data)
{
  int *response_ptr = (int *)user_data ;

  zMapAssert(arg1 != 0) ;				    /* It will never exit if this is true... */

  *response_ptr = arg1 ;

  return ;
}

