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
 * Last edited: Sep  9 15:15 2003 (edgrif)
 * Created: Thu Jul 24 14:37:35 2003 (edgrif)
 * CVS info:   $Id: zmapGUIutils.c,v 1.1 2003-11-13 15:03:08 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <gtk/gtk.h>


typedef struct
{
  GtkWidget *dialog ;
  GtkWidget *button ;
} zmapDialogCBStruct, *zmapDialogCB ;




static void clickButton(GtkWidget *widget, zmapDialogCB cb_data) ;


void zmapGUIShowMsg(char *msg)
{
  GtkWidget *dialog, *label, *button;
  zmapDialogCBStruct cb_data ;


  dialog = gtk_dialog_new();


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gtk_window_set_policy( GTK_WINDOW( dialog ), FALSE, FALSE, FALSE );
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  gtk_window_set_title( GTK_WINDOW( dialog ), "ZMAP - Error !" );

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
