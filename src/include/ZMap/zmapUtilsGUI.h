/*  File: zmapUtilsGUI.h
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Set of general GUI functions.
 *
 * HISTORY:
 * Last edited: Oct 16 14:39 2007 (edgrif)
 * Created: Fri Nov  4 16:59:52 2005 (edgrif)
 * CVS info:   $Id: zmapUtilsGUI.h,v 1.25 2007-10-16 15:14:04 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_GUI_H
#define ZMAP_UTILS_GUI_H

#include <gtk/gtk.h>
#include <ZMap/zmapUtilsMesg.h>


typedef enum {ZMAPGUI_PIXELS_PER_CM, ZMAPGUI_PIXELS_PER_INCH,
	      ZMAPGUI_PIXELS_PER_POINT} ZMapGUIPixelConvType ;

/* A bit field I found I needed to make it easier to calc what had been clamped. */
typedef enum
  {
    ZMAPGUI_CLAMP_INIT  = 0,
    ZMAPGUI_CLAMP_NONE  = (1 << 0),
    ZMAPGUI_CLAMP_START = (1 << 1),
    ZMAPGUI_CLAMP_END   = (1 << 2)
  } ZMapGUIClampType;



/*! Callback function for menu items. The id indicates which menu_item was selected
 *  resulting in the call to this callback. */
typedef void (*ZMapGUIMenuItemCallbackFunc)(int menu_item_id, gpointer callback_data) ;
typedef void (*ZMapGUIRadioButtonCBFunc)(GtkWidget *button, gpointer data, gboolean button_active);


/*! Types of menu entry. */
typedef enum {ZMAPGUI_MENU_NONE, ZMAPGUI_MENU_BRANCH, ZMAPGUI_MENU_SEPARATOR,
	      ZMAPGUI_MENU_TOGGLE, ZMAPGUI_MENU_TOGGLEACTIVE,
	      ZMAPGUI_MENU_RADIO, ZMAPGUI_MENU_RADIOACTIVE,
	      ZMAPGUI_MENU_NORMAL} ZMapGUIMenuType ;


typedef enum {ZMAPGUI_HELP_GENERAL, ZMAPGUI_HELP_ALIGNMENT_DISPLAY,
	      ZMAPGUI_HELP_RELEASE_NOTES, ZMAPGUI_HELP_KEYBOARD} ZMapHelpType ;


/* Bit of explaination here....
 * I changed my mind and we now have an array of structs which contain all the information required.
 * This includes which window to split (original or new) and which orientation to split!
 */
typedef enum {
  ZMAPSPLIT_NONE,
  ZMAPSPLIT_ORIGINAL,
  ZMAPSPLIT_LAST,
  ZMAPSPLIT_NEW,
  ZMAPSPLIT_BAD_PATTERN
}ZMapSplitWindow;

typedef struct
{
  ZMapSplitWindow subject;
  GtkOrientation orientation;
  /* ... This shouldn't get too specific if it stays here! ... */
} ZMapSplitPatternStruct, *ZMapSplitPattern;


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



/* A couple of macros to correctly test keyboard modifiers for events that include them:
 * 
 *       zMapGUITestModifiers() will return TRUE even if other modifiers are on (e.g. caps lock).
 *   zMapGUITestModifiersOnly() the second will return TRUE _only_ if no other modifiers are on.
 * 
 * Use them like this:
 * 
 * if (zMapGUITestModifiers(but_event, (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
 *   {
 *      doSomething() ;
 *   }
 *
 *  */
#define zMapGUITestModifiers(EVENT, MODS) \
  (((EVENT)->state & (MODS)) == (MODS))

#define zMapGUITestModifiersOnly(EVENT, MODS) \
  (((EVENT)->state | (MODS)) == ((EVENT)->state & (MODS)))




gint my_gtk_run_dialog_nonmodal(GtkWidget *toplevel) ;

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
void zMapGUIShowMsgFull(GtkWindow *parent, char *msg, ZMapMsgType msg_type, GtkJustification justify, int display_timeout) ;
gboolean zMapGUIShowChoice(GtkWindow *parent, ZMapMsgType msg_type, char *msg) ;

void zMapGUIShowAbout(void) ;
void zMapGUIShowHelp(ZMapHelpType help_contents) ;

void zMapGUIShowText(char *title, char *text, gboolean edittable) ;
GtkWidget *zMapGUIShowTextFull(char *title, char *text, gboolean edittable, GtkWidget **buffer_out);

char *zmapGUIFileChooser(GtkWidget *toplevel, char *title, char *directory, char *file_suffix) ;

void zMapGUICreateRadioGroup(GtkWidget *gtkbox, 
                             ZMapGUIRadioButton all_buttons,
                             int default_button, int *value_out,
                             ZMapGUIRadioButtonCBFunc clickedCB, gpointer clickedData);

ZMapGUIClampType zMapGUICoordsClampSpanWithLimits(double  top_limit, double  bot_limit, 
                                                  double *top_inout, double *bot_inout);
ZMapGUIClampType zMapGUICoordsClampToLimits(double  top_limit, double  bot_limit, 
                                            double *top_inout, double *bot_inout);

void zMapGUISetClipboard(GtkWidget *widget, char *contents);

void zMapGUIPanedSetMaxPositionHandler(GtkWidget *widget, GCallback callback, gpointer user_data);


#endif /* ZMAP_UTILS_GUI_H */
