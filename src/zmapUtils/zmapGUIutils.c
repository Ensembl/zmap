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
 * Last edited: Nov  8 12:17 2005 (edgrif)
 * Created: Thu Jul 24 14:37:35 2003 (edgrif)
 * CVS info:   $Id: zmapGUIutils.c,v 1.4 2005-11-08 17:11:33 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <gtk/gtk.h>
#include <zmapUtils_P.h>

typedef struct
{
  GtkWidget *dialog ;
  GtkWidget *button ;
} zmapDialogCBStruct, *zmapDialogCB ;



static void clickButton(GtkWidget *widget, zmapDialogCB cb_data) ;



/* ONLY NEEDED FOR OLD STYLE FILE SELECTOR, REMOVE WHEN WE CAN USE THE NEW CODE... */
typedef struct
{
  GtkWidget *file_selector ;
  char *file_path_out ;
} FileDialogCBStruct, *FileDialogCB ;
static void store_filename(GtkWidget *widget, gpointer user_data) ;
static void killFileDialog(GtkWidget *widget, gpointer user_data) ;




/* LIFT MY CODE FROM ACEDB THAT ALLOWED YOU TO CLICK TO CUT/PASTE MESSAGE CONTENTS....
 * ALSO ADD CODE FOR BLOCKING/NON-BLOCKING MESSAGES..... */
/* Message type is unused currently......... */
void zmapGUIShowMsg(ZMapMsgType msg_type, char *msg)
{
  GtkWidget *dialog, *label, *button;
  zmapDialogCBStruct cb_data ;

  dialog = gtk_dialog_new();

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gtk_window_set_policy( GTK_WINDOW( dialog ), FALSE, FALSE, FALSE );
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  switch(msg_type)
    {
    case ZMAP_MSG_INFORMATION:
      gtk_window_set_title( GTK_WINDOW( dialog ), "ZMAP - Information" );
      break;
    case ZMAP_MSG_WARNING:
      gtk_window_set_title( GTK_WINDOW( dialog ), "ZMAP - Warning!" );
      break;
    case ZMAP_MSG_EXIT:
      gtk_window_set_title( GTK_WINDOW( dialog ), "ZMAP - Error!" );
      break;
    case ZMAP_MSG_CRASH:
      gtk_window_set_title( GTK_WINDOW( dialog ), "ZMAP - Crash!" );
      break;
    default:
      gtk_window_set_title( GTK_WINDOW( dialog ), "ZMAP" );
      break;
    }

  gtk_container_set_border_width( GTK_CONTAINER(dialog), 5 );

  label = gtk_label_new(msg) ;

  gtk_label_set_justify( GTK_LABEL(label), GTK_JUSTIFY_CENTER );
  gtk_label_set_line_wrap( GTK_LABEL(label), TRUE );

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, TRUE, TRUE, 20);

  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
		      button, FALSE, FALSE, 0);
  gtk_widget_grab_default (button);
  gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(clickButton), &cb_data) ;

  cb_data.dialog = dialog ;
  cb_data.button = button ;

  gtk_widget_show_all( dialog );

  /* This is supposed to block interaction with the rest of the application...it doesn't. */
  gtk_grab_add(button) ;


  /* note this call will block until user clicks....not good we need to choose when to be modal
   * or not.... */
  gtk_main();


  return ;
}


static void clickButton(GtkWidget *widget, zmapDialogCB cb_data)
{

  gtk_grab_remove(cb_data->button) ;

  gtk_widget_destroy(cb_data->dialog) ;

  gtk_main_quit() ;

  return ;
}



/* THERE IS SOME ARCHAIC CODE HERE, FORCED ON US BY OUR BACK LEVEL OF INSTALLATION OF GTK... */


/* THIS IS THE OLD CODE..... */


/* Returns path of file chosen by user which can be used directly to open the file,
 * it is the callers responsibility to free the returned filepath using g_free().
 * Caller can optionally specify a title and a default directory for the dialog. */
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



/* The file selection widget and the string to store the chosen filename */

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

