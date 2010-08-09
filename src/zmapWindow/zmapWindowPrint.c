/*  File: zmapWindowPrint.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Contains functions to print current window.
 *
 * Exported functions: See ZMap/zmapWindow.h
 * HISTORY:
 * Last edited: Jul 30 21:18 2010 (edgrif)
 * Created: Thu Mar 30 16:48:34 2006 (edgrif)
 * CVS info:   $Id: zmapWindowPrint.c,v 1.10 2010-08-09 09:04:30 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>







#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>

#include <zmapWindow_P.h>


/* Probably need this in a config file ? */
#define ZMAP_APP_CONFIG "ZMap"



/* For most systems this is the file...but there will be exceptions so may need to allow this
 * to be configured. */
#define SYSTEM_PRINT_FILE "/etc/printcap"


/* This template is used by the g_mkstemp() function which fills in the "XXXXXX" uniquely. */
#define TMP_FILE_TMPLATE "/tmp/ZMapPrintFile.XXXXXX"

/* Send image to printer or save to file ? */
typedef enum {TO_PRINTER, TO_FILE} PrintFileDestination ;


typedef struct
{
  GError *print_file_err ;

  GList *printers ;

  /* We hold this string within the struct because mkstemp only needs a fixed length buffer
   * to write into. */
  gboolean tmp_file_ok ;				    /* Flags that tmp file was created. */
  char print_template[30] ;				    /* Keep in step with TMP_FILE_TMPLATE */

  PrintFileDestination sendto ;
  GtkWidget *print ;
  GtkWidget *file ;

  GtkWidget *print_select ;
  GtkWidget *combo ;

  GtkWidget *save_button, *print_button ;
  

  char *default_printer ;
  char *selected_printer ;
} PrintCBDataStruct, *PrintCBData ;



static GList *getPrinters(char *sys_print_file, GError **print_file_err_inout) ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void  printNames(gpointer data, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
static gint mySortFunc(gconstpointer a, gconstpointer b) ;

static gboolean getPrintFileName(PrintCBData print_cb, GError **print_file_err_inout) ;
static gboolean printFile(ZMapWindow window, PrintCBData print_cb) ;

static gboolean printDialog(PrintCBData print_cb) ;
static void addNames(gpointer data, gpointer user_data) ;
static void getButtons(GtkDialog *dialog, PrintCBData print_cb) ;
static void setSensitive(GtkWidget *button, gpointer data, gboolean active) ;

static gboolean getConfiguration(PrintCBData print_cb) ;




gboolean zMapWindowPrint(ZMapWindow window)
{
  gboolean result = TRUE ;
  char *sys_printers = SYSTEM_PRINT_FILE ;		    /* May require alternative names for porting. */
  PrintCBDataStruct print_cb = {NULL} ;


  memcpy(&(print_cb.print_template[0]), TMP_FILE_TMPLATE, (strlen(TMP_FILE_TMPLATE) + 1)) ;

  /* Get any default printer. */
  getConfiguration(&print_cb) ;


  /* Get the list of available printers. */
  print_cb.printers = getPrinters(sys_printers, &(print_cb.print_file_err)) ;


  /* do the dialog bit.... */
  if((result = printDialog(&print_cb)))
    {
      /* Depending on what user wanted either print the window or save it. */
      if (print_cb.sendto == TO_FILE)
        {
          zMapWindowDump(window) ;
        }
      else
        {
          result = printFile(window, &print_cb) ;
        }
    }

  if (print_cb.print_file_err)
    g_error_free(print_cb.print_file_err) ;

  if (print_cb.default_printer)
    g_free(print_cb.default_printer) ;

  if (print_cb.selected_printer)
    g_free(print_cb.selected_printer) ;


  return result ;
}


static gboolean printFile(ZMapWindow window, PrintCBData print_cb)
{
  gboolean result = TRUE ;


  if (result)
    {
      if (!(result = getPrintFileName(print_cb, &(print_cb->print_file_err))))
	{
	  zMapWarning("%s", print_cb->print_file_err->message) ;
	}
    }

  if (result)
    {
      if (!(result = zmapWindowDumpFile(window, print_cb->print_template)))
	{
	  zMapWarning("%s", "Could not dump window to temporary postscript file.") ;
	}
    }


  /* print the file... */
  if (result)
    {
      gboolean result ;
      char *err_msg = NULL ;
      char *print_command = NULL ;

      print_command = g_strdup_printf("lpr -P%s %s",
				      print_cb->selected_printer, print_cb->print_template) ;

      if (!(result = zMapUtilsSysCall(print_command, &err_msg)))
	{
	  zMapWarning("Could not print postscript file: %s.", print_cb->print_template) ;
	}

      g_free(print_command) ;
    }


  if (print_cb->tmp_file_ok)
    {
      if (unlink(print_cb->print_template) != 0)
	zMapWarning("Could not remove printer temporary file \"%s\".", print_cb->print_template) ;
    }


  return result ;
}







/* Need to add selection of custom, orientation and fit to window...plus maybe "save to file"
 * using the widget.... */
static gboolean printDialog(PrintCBData print_cb)
{
  gboolean status = FALSE ;
  char *window_title ;
  GtkWidget *dialog, *vbox, *frame, *hbox, *combo ;
  GtkEntry *text_box ;
  int dialog_rc = 0 ;
  ZMapGUIRadioButtonStruct print_to_buttons[] = {
    {TO_PRINTER, "Print", NULL},
    {TO_FILE,    "File",  NULL},
    {-1,         NULL,    NULL}};

  window_title = zMapGUIMakeTitleString("Print", "Please select printer") ;
  dialog = gtk_dialog_new_with_buttons (window_title,
					NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					GTK_STOCK_PRINT, GTK_RESPONSE_ACCEPT,
					NULL);
  g_free(window_title) ;


  vbox = GTK_DIALOG(dialog)->vbox ;


  /* Add print or file selection. */
  frame = gtk_frame_new(" Destination ") ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 10) ;
  gtk_container_set_border_width(GTK_CONTAINER (hbox), 10) ;
  gtk_container_add(GTK_CONTAINER(frame), hbox) ;

  zMapGUICreateRadioGroup(hbox,
                          &print_to_buttons[0],
                          TO_PRINTER,
                          (int *)&(print_cb->sendto),
                          setSensitive, print_cb);

  /* Add print select combo box. */
  print_cb->print_select = frame = gtk_frame_new(" Available Printers ") ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);


  hbox = gtk_hbox_new(FALSE, 10) ;
  gtk_container_set_border_width(GTK_CONTAINER(hbox), 10) ;
  gtk_container_add(GTK_CONTAINER(frame), hbox) ;


  /* Create the printer list read from the system print file. */
  combo = print_cb->combo = gtk_combo_box_entry_new_text() ;
  text_box = GTK_ENTRY(GTK_BIN(combo)->child) ;
  gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0) ;

  g_list_foreach(print_cb->printers, addNames, print_cb) ;


  /* Set a default printer. */
  if (print_cb->default_printer)
    gtk_entry_set_text(text_box, print_cb->default_printer) ;
  else
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0) ;


  gtk_widget_show_all(dialog) ;


  /* Get the buttons so we can make sensitive or not.... */
  getButtons(GTK_DIALOG(dialog), print_cb) ;


  /* Set GUI up. */
  setSensitive(NULL, print_cb, TRUE) ;


  dialog_rc = gtk_dialog_run(GTK_DIALOG(dialog)) ;

  if (dialog_rc == GTK_RESPONSE_ACCEPT)
    {
      char *text = NULL ;

      /* Need to get the text from the text entry box, _not_ the list because if user types
       * over the text then the list functions such as gtk_combo_box_get_active() return
       * invalid values. */
      text = (char *)gtk_entry_get_text(text_box) ;

      /* Note that the text box returns "", not NULL, when the user erases all the input. */
      if (text && *text)
	{
	  print_cb->selected_printer = g_strdup(text) ;
      
	  status = TRUE ;
	}
    }

  gtk_widget_destroy (dialog) ;

  return status ;
}



static void addNames(gpointer data, gpointer user_data)
{
  PrintCBData print_cb = (PrintCBData)user_data ;
  char *printer = (char *)data ;
  

  gtk_combo_box_append_text(GTK_COMBO_BOX(print_cb->combo), printer) ;



  return ;
}



static void getButtons(GtkDialog *dialog, PrintCBData print_cb)
{
  GtkHButtonBox *but_box = GTK_HBUTTON_BOX(dialog->action_area) ;
  GList *children ;

  children = gtk_container_get_children(GTK_CONTAINER(but_box)) ;

  /* Fragile code...I don't understand the button order at all.... */
  print_cb->save_button = GTK_WIDGET((g_list_nth(children, 1))->data) ;
  print_cb->print_button = GTK_WIDGET((g_list_nth(children, 0))->data) ;

  return ;
}

static void setSensitive(GtkWidget *button, gpointer data, gboolean active)
{
  PrintCBData cb_data = (PrintCBData)data;

  if (cb_data->sendto == TO_PRINTER)
    {
      gtk_widget_set_sensitive(cb_data->print_select, TRUE) ;
      gtk_widget_set_sensitive(cb_data->print_button, TRUE) ;
      gtk_widget_set_sensitive(cb_data->save_button, FALSE) ;
    }
  else
    {
      gtk_widget_set_sensitive(cb_data->print_select, FALSE) ;
      gtk_widget_set_sensitive(cb_data->print_button, FALSE) ;
      gtk_widget_set_sensitive(cb_data->save_button, TRUE) ;
    }

  return ;
}




/* Returns a list of available printers for the user to choose from.
 * 
 * The list is extracted from /etc/printcap which contains stanzas
 * in this format (note that this can be quite bizarre...):
 * 
 * 
 * # d204bw.dynamic.sanger.ac.uk
 * lp|d204bw:					      \				   
 *  #alternate names
 *   |lp2|lp3
 *   :cm=D204BW - LaserWriter 16/600		      \				   
 *   :sd=/var/spool/lpd/d204bw			      \				   
 *   :lf=/var/spool/lpd/d204bw/log                    \				   
 *   :rm=printsrv2:rp=d204bw			      \				   
 *   :force_queuename=d204bw			      \				   
 *   :network_connect_grace#1			      \				   
 *   :mx#0:sh::lk		
 * 
 * The code should end up with printers "lp", "d204bw", "lp2", "lp3".
 * 
 *  */
GList *getPrinters(char *sys_print_file, GError **print_file_err_inout)
{
  GList *printer_list = NULL ;
  GIOChannel* print_file ;

  if ((print_file = g_io_channel_new_file(sys_print_file, "r", print_file_err_inout)))
    {
      GString *curr_line ;
      GIOStatus status ;
      gsize terminator_pos ;

      curr_line = g_string_sized_new(200) ;		    /* Lines in /etc/printcap are short. */

      while ((status = g_io_channel_read_line_string(print_file, curr_line,
						     &terminator_pos,
						     print_file_err_inout)) == G_IO_STATUS_NORMAL)
	{
	  gboolean finished ;
	  char *cp ;

	  cp = curr_line->str ;

	  /* Read until not whitespace. */
	  while (*cp == ' ' || *cp == '\t')
	    {
	      cp++ ;
	    }

	  /* Any of these means its not a printer name and we go to the next line. */
	  switch (*cp)
	    {
	    case '#':
	    case ':':
	    case '\n':
	    case '\\':
	      continue ;
	      break ;
	    }

	  /* Look for printer names. Could amalgamate two 'while' loops but watch out for subtle
	   * errors... */
	  finished = FALSE ;
	  while (!finished)
	    {
	      char buffer[500] = {'\0'} ;			    /* Resets buffer */
	      char *cq ;

	      cq = &buffer[0] ;

	      /* Parse next printer name. */
	      while (!finished)
		{
		  char *printer_name = NULL ;

		  /* I'm not sure if embedded blanks are allowed in printer names,
		   * for now I assume not. */
		  switch (*cp)
		    {
		    case ':':
		    case '\n':
		    case ' ':
		    case '\t':
		    case '\\':
		      /* All these finish a printer name. */
		      finished = TRUE ;
		      /* N.B. deliberate fall through. */
		    case '|':
		      /* '|' can start a line after initial printer name, also
		       * ignore the include keyword which includes another printer file. */
		      if (buffer[0] != '\0' && strcmp(&buffer[0], "include") != 0)
			{
			  printer_name = g_strdup(&buffer[0]) ;

			  /* Sort printer names as we insert them as the list can be long. */
			  printer_list = g_list_insert_sorted(printer_list, printer_name,
							      mySortFunc) ;

			  cq = &buffer[0] ;		    /* reset buffer for next name. */
			}
		      cp++ ;
		      break ;
		    default:
		      /* Copy printer name to buffer. */
		      *cq = *cp ;
		      cq++ ;
		      cp++ ;
		      break ;
		    }
		}
	    }
	}


      if (status != G_IO_STATUS_EOF)
	zMapLogCritical("Could not close printers file \"%s\"", sys_print_file) ;

      if (g_io_channel_shutdown(print_file, FALSE, print_file_err_inout) != G_IO_STATUS_NORMAL)
	{
	  zMapLogCritical("Could not close printers file \"%s\"", sys_print_file) ;
	}
      else
	{
	  /* this seems to be required to destroy the GIOChannel.... */
	  g_io_channel_unref(print_file) ;
	}
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* DEBUGGGING..... */
  if (printer_list)
    g_list_foreach(printer_list, printNames, NULL) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return printer_list ;
}


/* straight forward string compare for printer names so they are sorted as inserted into list. */
static gint mySortFunc(gconstpointer a, gconstpointer b)
{
  gint result ;

  result = g_ascii_strcasecmp(a, b) ;

  return result ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void  printNames(gpointer data, gpointer user_data)
{

  printf("%s\n", (char *)data) ;


  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* This is a bit hokey, we open a temp file and then close it so that we can pass the name
 * on to the print code, should be ok, but in theory someone could reuse the file
 * under our feet...*/
static gboolean getPrintFileName(PrintCBData print_cb, GError **print_file_err_inout)
{
  gboolean status = TRUE ;
  gint print_file ;

  if ((print_file = g_mkstemp(print_cb->print_template)) == -1)
    {
      status = FALSE ;

      g_set_error(print_file_err_inout, g_quark_from_string("ZMap"), 99,
		  "%s", "ZMap print: g_mkstemp() failed.") ;
    }

  if (status)
    {
      if ((print_file = close(print_file)) == -1)
	{
	  status = FALSE ;

	  g_set_error(print_file_err_inout, g_quark_from_string("ZMap"), 99,
		      "%s", "ZMap print: close() failed.") ;
	}
      else
	print_cb->tmp_file_ok = TRUE ;			    /* Record that file created ok. */
    }


 return status ;
}

/* Read ZMap application defaults for default printer. Falling back to environment PRINTER and XPRINTER variables */
static gboolean getConfiguration(PrintCBData print_cb)
{
  ZMapConfigIniContext context = NULL;
  gboolean result  = FALSE;

  if((context = zMapConfigIniContextProvide()))
    {
      char *tmp_string;

      result = TRUE;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
				       ZMAPSTANZA_APP_PRINTER, &tmp_string))
	print_cb->default_printer = tmp_string;
      else if ((print_cb->default_printer = getenv("PRINTER"))
	       || (print_cb->default_printer = getenv("XPRINTER")))
        print_cb->default_printer = g_strdup_printf("%s", print_cb->default_printer) ;
      else
	result = FALSE;

      zMapConfigIniContextDestroy(context);
    }
  else if ((print_cb->default_printer = getenv("PRINTER")) || 
	   (print_cb->default_printer = getenv("XPRINTER")))
    {
      print_cb->default_printer = g_strdup_printf("%s", print_cb->default_printer);
      result = TRUE;
    }
  else
    result = FALSE;

  return result;
}





