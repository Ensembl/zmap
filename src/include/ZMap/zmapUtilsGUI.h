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
 * Last edited: Jun 30 16:03 2006 (rds)
 * Created: Fri Nov  4 16:59:52 2005 (edgrif)
 * CVS info:   $Id: zmapUtilsGUI.h,v 1.13 2006-06-30 15:18:39 rds Exp $
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
typedef void (*ZMapGUIRadioButtonCBFunc)(GtkWidget *button, gpointer data, gboolean button_active);


/*! Types of menu entry. */
typedef enum {ZMAPGUI_MENU_NONE, ZMAPGUI_MENU_BRANCH, ZMAPGUI_MENU_SEPARATOR,
	      ZMAPGUI_MENU_TOGGLE, ZMAPGUI_MENU_TOGGLEACTIVE,
	      ZMAPGUI_MENU_RADIO, ZMAPGUI_MENU_RADIOACTIVE,
	      ZMAPGUI_MENU_NORMAL} ZMapGUIMenuType ;



/*!
 * Defines a menu item. */
typedef struct
{
  ZMapGUIMenuType type ;				    /* Title, separator etc. */
  char *name ;						    /*!< Title string of menu item. */
  int id ;						    /*!< Number uniquely identifying this
							      menu item within a menu. */
  ZMapGUIMenuItemCallbackFunc callback_func ;		    /*!< Function to call when this item
							      is selected.  */
  gpointer callback_data ;				    /*!< Data to pass to callback function. */
} ZMapGUIMenuItemStruct, *ZMapGUIMenuItem ;



typedef struct
{
  int        value;
  char      *name;
  GtkWidget *widget;
} ZMapGUIRadioButtonStruct, *ZMapGUIRadioButton ;

/* A wrongly named (makes it look too general) data struct to hold
 * info for align/block information plus the original data that may
 * have been set in the GUIMenuItem. Code currently does a swap 
 *
 * tmp = item->callback_data;
 * sub->original = tmp;
 * item->callback_data = sub;
 *
 * Not really that much fun, but I couldn't think of an alternative...
 *
 */

typedef struct
{
  GQuark align_unique_id;
  GQuark block_unique_id;
  gpointer original_data;
}ZMapGUIMenuSubMenuDataStruct, *ZMapGUIMenuSubMenuData;


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
char *zMapGUIMakeTitleString(char *window_type, char *message) ;

void zMapGUIShowMsg(ZMapMsgType msg_type, char *msg) ;
void zMapGUIShowMsgFull(GtkWindow *parent, char *msg, ZMapMsgType msg_type, GtkJustification justify) ;
gboolean zMapGUIShowChoice(GtkWindow *parent, ZMapMsgType msg_type, char *msg) ;
void zMapGUIShowAbout(void) ;

void zMapGUIShowText(char *title, char *text, gboolean edittable) ;

char *zmapGUIFileChooser(GtkWidget *toplevel, char *title, char *directory, char *file_suffix) ;

void zMapGUICreateRadioGroup(GtkWidget *gtkbox, 
                             ZMapGUIRadioButton all_buttons,
                             int default_button, int *value_out,
                             ZMapGUIRadioButtonCBFunc clickedCB, gpointer clickedData);

#endif /* ZMAP_UTILS_GUI_H */
