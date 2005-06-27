/*  File: zmapWindowMenus.c
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Code implementing the menus for sequence display.
 *
 * Exported functions: ZMap/zmapWindows.h
 *              
 * HISTORY:
 * Last edited: Jun 27 22:49 2005 (rds)
 * Created: Thu Mar 10 07:56:27 2005 (edgrif)
 * CVS info:   $Id: zmapWindowMenus.c,v 1.4 2005-06-27 21:49:26 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>


/* READ THIS:
 * 
 * There seem to be 3 choices for implementing menus in gtk:
 * 
 * 1) roll your own using Menu and MenuItem: but I can't see how to specify
 *    callback functions/data in the description of either of these widgets....
 * 
 * 2) use itemfactories which specify all the stuff you need for each menu
 *    item: but these have been deprecated since GTK 2.4
 * 
 * 3) Use gtks new xml based UIManager system which is meant to replace 1) and 2):
 *    this just seems COMPLETE overkill for small menus, there seem to be no
 *    examples, talk about throwing the baby out with the bath water...sigh....
 * 
 * I have decided to go for option 2) as it seems to be the quickest way to get
 * a working menu system. We may have to revisit this if they decide to remove
 * item factories.
 * 
 * Do not get confused by the different callback prototypes specified for itemfactories,
 * we must use the GtkItemFactoryCallback1 prototype.
 * 
 * Here is the typedef for the item factory struct:
 *
 * typedef struct {
 *   gchar *path;
 *   gchar *accelerator;
 *   GtkItemFactoryCallback callback;
 *   guint			 callback_action;
 *   gchar		 *item_type;
 *   gconstpointer extra_data;
 * } GtkItemFactoryEntry;
 *
 *
 */


/* Used for all item factories. */
#define ITEM_FACTORY_NAME "<zmap_item_factory_name>"



/* Passed into our callback by gtk menu code. */
typedef struct
{
  ZMapWindowMenuItem callers_menu_copy ;
  GtkItemFactoryEntry *factory_items ;
  int num_factory_items ;
} CallbackDataStruct, *CallbackData ;


static char *makeMenuItemName(char *string) ;
static ZMapWindowMenuItem copyMenu(ZMapWindowMenuItem old_menu, int *menu_size) ;
static void destroyMenu(ZMapWindowMenuItem menu) ;
static int itemsInMenu(ZMapWindowMenuItem menu) ;

static void ourCB(gpointer callback_data, guint callback_action, GtkWidget *widget) ;



/*! @addtogroup zmapwindow
 * @{
 *  */


/*!
 * Takes an array of ZMapWindowMenuItemStruct menu items and uses them to make a
 * GTK menu displaying those items. When an item is selected the callback routine
 * for that item will be called with the specified identifier and data.
 * The identifier can be used to select which menu item was chosen when more
 * than one menu item specifies the same callback function.
 * 
 * The caller's menu is converted into a GTK itemfactory struct which is then
 * use to build the GTK menu. This has several advantages:
 * 
 * - it avoids duplicating the tedious GTK menu code everywhere.
 * 
 * - the caller does not have to understand the slightly arcance GTK rules
 *   for the GTK item factory.
 *
 * - its possible to specify different callback data for each menu item,
 *   something which cannot be done with the item factory code.
 * 
 * @param menu_title    Title string for this menu instance.
 * @param menu_items    Array of window items.
 * @param button_event  The button event that triggered the menu to be popped up.
 * @return              <nothing>
 *  */
void zMapWindowMakeMenu(char *menu_title, ZMapWindowMenuItemStruct menu_items[],
			GdkEventButton *button_event)
{
  int num_menu_items, num_factory_items, i, menu_size ;
  ZMapWindowMenuItem menu_copy ;
  GtkItemFactoryEntry *factory_items, *item ;
  GtkItemFactory *item_factory ;
  CallbackData our_cb_data ;


  /* Set up our callback data, we use this to call our callers callback. */
  our_cb_data = g_new0(CallbackDataStruct, 1) ;


  /* Make a copy of the callers menu for use in our callback. */
  our_cb_data->callers_menu_copy = menu_copy = copyMenu(menu_items, &num_menu_items) ;


  /*
   * Make the actual gtk item factory.
   */

  our_cb_data->num_factory_items = num_factory_items = num_menu_items + 2 ;
							    /* + 2 for title + separator items. */
  our_cb_data->factory_items = item = factory_items = g_new0(GtkItemFactoryEntry, num_factory_items) ;
				

  /* Do title/separator. */
  item->path = makeMenuItemName(menu_title) ;
  item->item_type = "<Title>" ;
  item++ ;

  item->path = makeMenuItemName("separator") ;
  item->item_type = "<Separator>" ;
  item++ ;

  /* Do actual items. N.B. we use the callback_action to index into our copy of the users
   * menu to its vital that i starts at zero. */
  for (i = 0 ; i < num_menu_items ; i++)
    {
      item->path = makeMenuItemName(menu_items[i].name) ;
      item->callback = ourCB ;
      item->callback_action = i ;
      item++ ;
    }

  /* Construct the menu and pop it up. */
  item_factory = gtk_item_factory_new(GTK_TYPE_MENU, ITEM_FACTORY_NAME, NULL) ;

  gtk_item_factory_create_items(item_factory, num_factory_items, factory_items, our_cb_data) ;


  /* Note that we need the root window coords for the popup. */
  gtk_item_factory_popup(item_factory, (guint)button_event->x_root, (guint)button_event->y_root,
			 button_event->button, button_event->time) ;

  return ;
}



/*! @} end of zmapwindow docs. */




/* N.B. the item factory stuff is GLOBAL...perhaps we need unique names ??? */
static void ourCB(gpointer callback_data, guint callback_action, GtkWidget *widget)
{
  CallbackData our_data = (CallbackData)callback_data ;
  ZMapWindowMenuItem caller_menu_item ;
  GtkItemFactoryEntry *item ;
  int i ;

  caller_menu_item = our_data->callers_menu_copy + callback_action ;

  /* Call the original menu item callback with its callback data and its id. */
  (caller_menu_item->callback_func)(caller_menu_item->id, caller_menu_item->callback_data) ;


  /* What this is trying to do is make sure that menu gets destroyed. It is popped down
   * automatically by the button release but the docs don't detail if it is destroyed. 
   * You seem to have to set both the item_factory path and the widget path to the
   * same thing otherwise it moans.... */
  gtk_item_factories_path_delete(ITEM_FACTORY_NAME, ITEM_FACTORY_NAME) ;

  /* Get rid of our copy of the callers original menu. */
  destroyMenu(our_data->callers_menu_copy) ;
  
  /* Now we have got rid of the factory, clean up the item factory input data we created
   * when we made the menu. NOTE that we do this here because on some systems if we delete
   * this before the menu is popped up the GTK menu code segfaults...this is poor because
   * it should be taking its own copy of our data....sigh... */
  for (i = 0, item = our_data->factory_items ; i < our_data->num_factory_items ; i++)
    {
      g_free(item->path) ;
      item++ ;
    }
  g_free(our_data->factory_items) ;

  /* Now free the callback data struct. */
  g_free(our_data) ;

  return ;
}


/* Item Name strings must have the format "/item_name". */
static char *makeMenuItemName(char *string)
{
  char *item_string ; 

  zMapAssert(string && *string) ;

  item_string = g_strdup_printf("/%s", string) ;

  return item_string ;
}



/* Returns copy of menu and size of menu _NOT_ including the final terminating null entry. */
static ZMapWindowMenuItem copyMenu(ZMapWindowMenuItem old_menu, int *menu_items_out)
{
  ZMapWindowMenuItem new_menu, old_item, new_item ;
  int i, num_menu_items, menu_size  ;

  num_menu_items = itemsInMenu(old_menu) ;

  /* Make a copy of the callers menu for use in our callback (remember null item on the end. */
  menu_size = sizeof(ZMapWindowMenuItemStruct) * (num_menu_items + 1) ;
  new_menu = g_memdup(old_menu, menu_size) ;

  /* Remember to copy the strings in the old menu. */
  for (i = 0, old_item = old_menu, new_item = new_menu ;
       i < num_menu_items ; i++, old_item++, new_item++)
    {
      new_item->name = g_strdup(old_item->name) ;
    }

  if (menu_items_out)
    *menu_items_out = num_menu_items ;

  return new_menu ;
}


/* Destroy a menu allocated by copyMenu() function. */
static void destroyMenu(ZMapWindowMenuItem menu)
{
  int i, num_menu_items  ;
  ZMapWindowMenuItem menu_item ;

  /* Count items in menu. */
  num_menu_items = itemsInMenu(menu) ;

  for (i = 0, menu_item = menu ; i < num_menu_items ; i++, menu_item++)
    {
      g_free(menu_item->name) ;
    }
  g_free(menu) ;

  return ;
}


/* Counts number of items in menu _NOT_ including final NULL item. */
static int itemsInMenu(ZMapWindowMenuItem menu)
{
  int num_menu_items ;
  ZMapWindowMenuItem menu_item ;

  /* Count items in menu. */
  for (num_menu_items = 0, menu_item = menu ;
       menu_item->name != NULL ;
       num_menu_items++, menu_item++) ;

  return num_menu_items ;
}


