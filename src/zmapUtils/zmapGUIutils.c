/*  File: zmapGUIutils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Nov 11 12:05 2005 (edgrif)
 * Created: Thu Jul 24 14:37:35 2003 (edgrif)
 * CVS info:   $Id: zmapGUIutils.c,v 1.5 2005-11-11 12:10:05 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <gtk/gtk.h>
#include <zmapUtils_P.h>
#include <ZMap/zmapUtilsGUI.h>


/* ONLY NEEDED FOR OLD STYLE FILE SELECTOR, REMOVE WHEN WE CAN USE THE NEW CODE... */
typedef struct
{
  GtkWidget *file_selector ;
  char *file_path_out ;
} FileDialogCBStruct, *FileDialogCB ;
static void store_filename(GtkWidget *widget, gpointer user_data) ;
static void killFileDialog(GtkWidget *widget, gpointer user_data) ;
/* ONLY NEEDED FOR OLD STYLE FILE SELECTOR, REMOVE WHEN WE CAN USE THE NEW CODE... */



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






/* LIFT MY CODE FROM ACEDB THAT ALLOWED YOU TO CLICK TO CUT/PASTE MESSAGE CONTENTS....*/


/*!
 * Trivial cover function for zmapGUIShowMsgOnTop(), see documentation of that function
 * for description and parameter details.
 *
 *  */
void zMapGUIShowMsg(ZMapMsgType msg_type, char *msg)
{
  zMapGUIShowMsgOnTop(NULL, msg_type, msg) ;

  return ;
}



/*!
 * Display a short message in a pop-up dialog box, the behaviour of the dialog depends on
 * the message type:
 *
 *     ZMAP_MSG_INFORMATION, ZMAP_MSG_WARNING - non-blocking, non-modal
 *              ZMAP_MSG_EXIT, ZMAP_MSG_CRASH - blocking and modal
 *
 * If parent is non-NULL then the dialog will be kept on top of that window, essential for
 * modal dialogs in particular. I think parent should be the application window that the message
 * applies to probably, or perhaps the application main window.
 *
 * @param parent       Widget that message should be kept on top of or NULL.
 * @param msg_type     ZMAP_MSG_INFORMATION | ZMAP_MSG_WARNING | ZMAP_MSG_EXIT | ZMAP_MSG_CRASH
 * @param msg          Message to be displayed in dialog.
 * @return             nothing
 *  */
void zMapGUIShowMsgOnTop(GtkWindow *parent, ZMapMsgType msg_type, char *msg)
{
  GtkWidget *dialog, *label ;
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

  if (modal)
    flags |= GTK_DIALOG_MODAL ;

  dialog = gtk_dialog_new_with_buttons(title, parent, flags,
				       "Close", GTK_RESPONSE_NONE,
				       NULL) ;
  gtk_container_set_border_width( GTK_CONTAINER(dialog), 5 );


  label = gtk_label_new(msg) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER) ;
  gtk_label_set_line_wrap(GTK_LABEL(label), TRUE) ;
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, TRUE, TRUE, 20);


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
  GtkWidget *dialog, *scrwin, *view, *button ;
  GtkTextBuffer *buffer ;
  GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT ;
  PangoFont *font ;
  PangoFontDescription *font_desc ;
  int width ;
  double x, y ;
  int text_width, text_height ;
  char *font_text ;

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


  /* Here we try to set a fixed width font in the text widget and set the size of the dialog
   * so that a sensible amount of text is displayed. */
  if (!zMapGUIGetFixedWidthFont(view,
				g_list_append(NULL, "fixed"), 10, PANGO_WEIGHT_NORMAL,
				&font, &font_desc))
    {
      zMapGUIShowMsg(ZMAP_MSG_WARNING, "Could not get fixed width font, "
		     "message window may be a strange size") ;
    }
  else
    {
      zMapGUIGetFontWidth(font, &width) ;

      zMapGUIGetPixelsPerUnit(ZMAPGUI_PIXELS_PER_POINT, dialog, &x, &y) ;

      text_width = (int)(((double)(80 * width)) / x) ;
      text_height = text_width / 2 ;
  
      /* This is all a bit hacky but basically the borders around the window take up quite a lot of
       * size and its not easy to calculate how big to make the window so the text shows....sigh... */
      text_width += TEXT_X_BORDERS ;
      text_height += TEXT_Y_BORDERS ;

      gtk_widget_modify_font(view, font_desc) ;

      gtk_window_set_default_size(GTK_WINDOW(dialog), text_width, text_height) ;
    }


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
 * @return             nothing
 */
char *zmapGUIFileChooser(GtkWidget *toplevel, char *title, char *directory_in)
{
  char *file_path = NULL ;
  GtkWidget *file_selector;
  FileDialogCBStruct cb_data = {NULL} ;

  zMapAssert(toplevel) ;

  /* Create the selector */
  if (!title)
    title = "Please give a file name:" ;

  cb_data.file_selector = file_selector = gtk_file_selection_new (title) ;

  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(file_selector)) ;

  /* this appears just not to work at all, sigh............ */
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
 *
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
  gboolean found = FALSE ;
  PangoContext *context ;
  PangoFontFamily **families;
  gint n_families, i ;
  PangoFontFamily *match_family = NULL ;
  PangoFont *font = NULL;

  context = gtk_widget_get_pango_context(widget) ;

  pango_context_list_families(context, &families, &n_families) ;

  for (i = 0 ; (i < n_families && found == FALSE) ; i++)
    {
      const gchar *name ;
      GList *pref ;

      name = pango_font_family_get_name(families[i]) ;

      pref = g_list_first(pref_families) ;
      while(pref)
	{
	  if(!g_ascii_strcasecmp(name, (char *)pref->data)

#warning "We need to put the monospace test in when we move to GTK 2.6"
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	     /* NEED GTK 2.6 FOR THIS.... */
	     && pango_font_family_is_monospace(families[i])
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	     )
	    {
	      found = TRUE ;
	      match_family = families[i] ;
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
void zMapGUIGetPixelsPerUnit(int conv_type, GtkWidget *widget, double *x_out, double *y_out)
{
  GdkScreen *screen ;
  double width, height, width_mm, height_mm ;
  double multiplier ;
  double mm_2_inches = 25.4 ;
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




/*! @} end of zmapguiutils docs. */


/*
 *  ------------------- Internal functions -------------------
 */

static void store_filename(GtkWidget *widget, gpointer user_data)
{
  FileDialogCB cb_data = (FileDialogCB)user_data ;
  char *file_path = NULL ;

  file_path = (char *)gtk_file_selection_get_filename(GTK_FILE_SELECTION(cb_data->file_selector)) ;

  cb_data->file_path_out = g_strdup(file_path) ;

  return ;
}

static void killFileDialog(GtkWidget *widget, gpointer user_data)
{
  FileDialogCB cb_data = (FileDialogCB)user_data ;

  gtk_widget_destroy(cb_data->file_selector) ;

  gtk_main_quit() ;

  return ;
}


