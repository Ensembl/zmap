/*  File: zmapUtilsGUI.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Set of general GUI functions.
 *
 * HISTORY:
 * Last edited: Jan 12 14:18 2006 (edgrif)
 * Created: Fri Nov  4 16:59:52 2005 (edgrif)
 * CVS info:   $Id: zmapUtilsGUI.h,v 1.5 2006-01-13 18:47:54 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_GUI_H
#define ZMAP_UTILS_GUI_H

#include <gtk/gtk.h>
#include <ZMap/zmapUtilsMesg.h>

typedef enum {ZMAPGUI_PIXELS_PER_CM, ZMAPGUI_PIXELS_PER_INCH,
	      ZMAPGUI_PIXELS_PER_POINT} ZMapGUIPixelConvType ;


/*! Callback function for menu items. The id indicates which menu_item was selected
 *  resulting in the call to this callback. */
typedef void (*ZMapGUIMenuItemCallbackFunc)(int menu_item_id, gpointer callback_data) ;

/*!
 * Defines a menu item. */
typedef struct
{
  char *name ;						    /*!< Title string of menu item. */
  int id ;						    /*!< Number uniquely identifying this
							      menu item within a menu. */
  ZMapGUIMenuItemCallbackFunc callback_func ;	    /*!< Function to call when this item
							      is selected.  */
  gpointer callback_data ;				    /*!< Data to pass to callback function. */
} ZMapGUIMenuItemStruct, *ZMapGUIMenuItem ;




void zMapGUIMakeMenu(char *menu_title, GList *menu_sets, GdkEventButton *button_event) ;
void zMapGUIPopulateMenu(ZMapGUIMenuItem menu,
			 int *start_index_inout,
			 ZMapGUIMenuItemCallbackFunc callback_func,
			 gpointer callback_data) ;

gboolean zMapGUIGetFixedWidthFont(GtkWidget *widget, 
                                  GList *prefFamilies, gint points, PangoWeight weight,
				  PangoFont **font_out, PangoFontDescription **desc_out) ;
void zMapGUIGetFontWidth(PangoFont *font, int *width_out) ;
void zMapGUIGetPixelsPerUnit(ZMapGUIPixelConvType conv_type, GtkWidget *widget, double *x, double *y) ;

void zMapGUIShowMsg(ZMapMsgType msg_type, char *msg) ;
void zMapGUIShowMsgOnTop(GtkWindow *parent, ZMapMsgType msg_type, char *msg) ;
void zMapGUIShowText(char *title, char *text, gboolean edittable) ;
char *zmapGUIFileChooser(GtkWidget *toplevel, char *title, char *directory, char *file_suffix) ;


#endif /* ZMAP_UTILS_GUI_H */
