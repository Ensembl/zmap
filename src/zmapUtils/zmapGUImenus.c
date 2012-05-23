/*  File: zmapGUImenus.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Code to construct GTK menus in a (hopefully) more
 *              straight forward way.
 *
 * Exported functions: See zmapUtilsGUI.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>
#include <zmapUtils_P.h>
#include <ZMap/zmapUtilsGUI.h>



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
 *   guint callback_action;
 *   gchar *item_type;
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
  ZMapGUIMenuItem callers_menu_copy ;
  GtkItemFactoryEntry *factory_items ;
  int num_factory_items ;
  int num_hide;
} CallbackDataStruct, *CallbackData ;


static char *makeMenuItemName(char *string) ;
static char *makeMenuTitleName(char *string, char *escape_chars) ;

static ZMapGUIMenuItem makeSingleMenu(GList *menu_item_sets, int *menu_items_out) ;
static ZMapGUIMenuItem copyMenu2NewMenu(ZMapGUIMenuItem new_menu, ZMapGUIMenuItem old_menu) ;
static void destroyMenu(ZMapGUIMenuItem menu) ;
static int itemsInMenu(ZMapGUIMenuItem menu) ;

static void ourCB(gpointer callback_data, guint callback_action, GtkWidget *widget) ;

static void activeButCB(gpointer data, gpointer user_data) ;
static void deActiveButCB(gpointer data, gpointer user_data) ;
static void toggleButCB(gpointer data, gpointer user_data) ;
static void deToggleButCB(gpointer data, gpointer user_data) ;


/*! @addtogroup zmapwindow
 * @{
 *  */


/*!
 * Takes an array of ZMapGUIMenuItemStruct menu items and uses them to make a
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
void zMapGUIMakeMenu(char *menu_title, GList *menu_item_sets, GdkEventButton *button_event)
{
  int num_menu_items, num_factory_items, i ;
  ZMapGUIMenuItem menu_items ;
  GtkItemFactoryEntry *factory_items, *item ;
  GtkItemFactory *item_factory ;
  CallbackData our_cb_data ;
  char *radio_title = NULL ;
  GList *active_radio_buttons = NULL, *deactive_radio_buttons = NULL ;
  GList *active_toggle_buttons = NULL, *deactive_toggle_buttons = NULL ;
  int num_hide = 0;

  /* Make a single menu item list out of the supplied menu_item sets. */
  menu_items = makeSingleMenu(menu_item_sets, &num_menu_items) ;

  /* Set up our callback data, we use this to call our callers callback. */
  our_cb_data = g_new0(CallbackDataStruct, 1) ;
  our_cb_data->callers_menu_copy = menu_items ;


  /*
   * Make the actual gtk item factory.
   */

  our_cb_data->num_factory_items = num_factory_items = num_menu_items + 2 ;
							    /* + 2 for title + separator items. */
  our_cb_data->factory_items = item = factory_items = g_new0(GtkItemFactoryEntry, num_factory_items) ;


  /* Do title/separator, the title needs to have any '_' or '/' chars escaped to stop them being
   * interpretted as keyboard shortcuts or submenu indicators. */
  item->path = makeMenuTitleName(menu_title, "/_") ;
  item->item_type = "<Title>" ;
  item->accelerator = "O";
  item++ ;

  item->path = makeMenuItemName("separator") ;
  item->item_type = "<Separator>" ;
  item++ ;

  /* Do actual items. N.B. we use the callback_action to index into our copy of the users
   * menu to its vital that i starts at zero. */
  for (i = 0 ; i < num_menu_items ; i++)
    {
      if(menu_items[i].type == ZMAPGUI_MENU_HIDE)
      {
            num_hide++;
            continue;
      }

      /* User does not have to set a name for a separator but to make our code more uniform
       * we add one. */
      if (menu_items[i].type == ZMAPGUI_MENU_SEPARATOR)
	item->path = makeMenuItemName("separator") ;
      else
	item->path = makeMenuItemName(menu_items[i].name) ;

      item->callback = ourCB ;
      item->callback_action = i ;

      zMapAssert(menu_items[i].type != ZMAPGUI_MENU_NONE) ;

      switch (menu_items[i].type)
	{
	case ZMAPGUI_MENU_BRANCH:
	  {
	    item->item_type = "<Branch>" ;
	    item->callback = NULL ;
	    item->callback_action = -1 ;
	    break ;
	  }
	case ZMAPGUI_MENU_SEPARATOR:
	  {
	    item->item_type = "<Separator>" ;
	    item->callback = NULL ;
	    item->callback_action = -1 ;
	    break ;
	  }
	case ZMAPGUI_MENU_RADIO:
	case ZMAPGUI_MENU_RADIOACTIVE:
	  {
	    /* If this is the first radio item, remember its title as we need to use it in all
	     * the subsequent radio buttons as the key to cluster the buttons in a radio group. */
	    if (!radio_title)
	      {
		radio_title = item->path ;
		item->item_type = "<RadioItem>" ;

		deactive_radio_buttons = g_list_append(deactive_radio_buttons, item->path) ;
	      }
	    else
	      item->item_type = radio_title ;

	    if (menu_items[i].type == ZMAPGUI_MENU_RADIOACTIVE)
	      active_radio_buttons = g_list_append(active_radio_buttons, item->path) ;

	    break ;
	  }
	case ZMAPGUI_MENU_TOGGLE:
	case ZMAPGUI_MENU_TOGGLEACTIVE:
	  {

	    /* It doesn't seem to matter which style I use here it still looks cr*p */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	    item->item_type = "<ToggleItem>" ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	    item->item_type = "<CheckItem>" ;

	    if (menu_items[i].type == ZMAPGUI_MENU_TOGGLEACTIVE)
	      active_toggle_buttons = g_list_append(active_toggle_buttons, item->path) ;
	    else
	      deactive_toggle_buttons = g_list_append(deactive_toggle_buttons, item->path) ;

	    break ;
	  }
	default:					    /* ZMAPGUI_MENU_NORMAL */
	  {
	    item->item_type = NULL ;
	    break ;
	  }
	}

      /* Reset radio button indicator if we have moved out of the radio button group. */
      if (radio_title
	  && (menu_items[i].type != ZMAPGUI_MENU_RADIO
	      && menu_items[i].type != ZMAPGUI_MENU_RADIOACTIVE))
	radio_title = NULL ;

      if(menu_items[i].accelerator)
	item->accelerator = menu_items[i].accelerator;

      item++ ;
    }

  /* Construct the menu. */
  item_factory = gtk_item_factory_new(GTK_TYPE_MENU, ITEM_FACTORY_NAME, NULL) ;

  our_cb_data->num_hide = num_hide;
  gtk_item_factory_create_items(item_factory, num_factory_items - num_hide, factory_items, our_cb_data) ;


  {
    /* Give menu title widget a name so we can colour it from a resource file. */
    GtkWidget *title = NULL;

    /* NOTE this time we need the title string without escaping any '_' so we can find
     * it in the item factory path, but the '/' must still be escaped. */
    title = gtk_item_factory_get_widget(item_factory, makeMenuTitleName(menu_title, "/")) ;

    gtk_widget_set_name(title, "zmap-menu-title");
  }


  /* Set up any toggle buttons, stupid item factory makes this stupidly difficult. */
  if (deactive_toggle_buttons)
    {
      g_list_foreach(deactive_toggle_buttons, deToggleButCB, item_factory) ;

      g_list_free(deactive_toggle_buttons) ;
    }
  if (active_toggle_buttons)
    {
      g_list_foreach(active_toggle_buttons, toggleButCB, item_factory) ;

      g_list_free(active_toggle_buttons) ;
    }


  /* By default stupid item factory makes the first radio button active but does
   * not allow us to specify any other one...so de-activate all the first buttons. */
  if (deactive_radio_buttons)
    {
      g_list_foreach(deactive_radio_buttons, deActiveButCB, item_factory) ;

      g_list_free(deactive_radio_buttons) ;
    }


  /* Activate any radio buttons that need to be. */
  if (active_radio_buttons)
    {
      g_list_foreach(active_radio_buttons, activeButCB, item_factory) ;

      g_list_free(active_radio_buttons) ;
    }


  /* Show the menu, note that we need the root window coords for the popup. */
  gtk_item_factory_popup(item_factory, (guint)button_event->x_root, (guint)button_event->y_root,
			 button_event->button, button_event->time) ;

  return ;
}



/* Overrides settings in menu with supplied data. */
void zMapGUIPopulateMenu(ZMapGUIMenuItem menu,
			 int *start_index_inout,
			 ZMapGUIMenuItemCallbackFunc callback_func,
			 gpointer callback_data)
{
  ZMapGUIMenuItem menu_item ;
  int index ;

  zMapAssert(menu) ;

  if (start_index_inout)
    index = *start_index_inout ;

  menu_item = menu ;

  while (menu_item->type != ZMAPGUI_MENU_NONE)
    {
      if (start_index_inout)
	menu_item->id = index ;
      if (callback_func)
	menu_item->callback_func = callback_func ;
      if (callback_data)
	menu_item->callback_data = callback_data ;

      menu_item++ ;
    }

  if (start_index_inout)
    *start_index_inout = index ;

  return ;
}





/* N.B. the item factory stuff is GLOBAL...perhaps we need unique names ??? */
static void ourCB(gpointer callback_data, guint callback_action, GtkWidget *widget)
{
  CallbackData our_data = (CallbackData)callback_data ;
  ZMapGUIMenuItem caller_menu_item ;
  GtkItemFactoryEntry *item ;
  int i ;
  gboolean call_user_cb = TRUE ;

  /* For radio buttons this function gets called twice, once for deactivating
   * the currently selected button and once for activating the new one, we need
   * to detect this and act accordingly. */
  if (GTK_IS_RADIO_MENU_ITEM(widget) && !(GTK_CHECK_MENU_ITEM(widget)->active))
    call_user_cb = FALSE ;

  caller_menu_item = our_data->callers_menu_copy + callback_action ;

  if (call_user_cb && caller_menu_item->callback_func)
    {
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
      for (i = 0, item = our_data->factory_items ; i < our_data->num_factory_items - our_data->num_hide ; i++)
	{
	  g_free(item->path) ;
	  item++ ;
	}
      g_free(our_data->factory_items) ;

      /* Now free the callback data struct. */
      g_free(our_data) ;
    }


  return ;
}


/*
 * The menu item/path stuff is complicated by the fact that some of the menu titles we wish
 * to set have embedded '_' or '/' chars which have a special meaning to the item factory
 * ...sigh...
 * This happens because gene names etc. often have '_' embedded in them and DAS names may
 * have '/'.
 */


/* Normal Menu Item Name strings must have the format "/item_name". */
static char *makeMenuItemName(char *string)
{
  char *item_string ;

  zMapAssert(string && *string) ;

  item_string = g_strdup_printf("/%s", string) ;

  return item_string ;
}


/* Menu titles are just like Item Name strings and must have the format "/menu_title",
 * but quite often there are embedded '_' chars which will erroneously be interpretted
 * as keyboard short cuts and so must be escaped by turning them into "__".
 *
 * For DAS there are also embedded '/' which must be escaped as well as "\/"
 *
 */
static char *makeMenuTitleName(char *string, char *escape_chars)
{
  char *item_string ;
  GString *tmp ;
  char *cp ;
  gssize pos ;

  zMapAssert(string && *string) ;

  tmp = g_string_new(string) ;

  pos = 0 ;
  cp = (tmp->str + pos) ;
  while (*cp)
    {
      if (*cp == '_' || *cp == '/')
	{
	  char prepend_ch = *cp ;

	  if (*cp == '/')
	    prepend_ch = '\\' ;

	  if (strchr(escape_chars, *cp))
	    {
	      tmp = g_string_insert_c(tmp, pos, prepend_ch) ;
	      pos++ ;
	    }
	}

      /* Must always reset cp from our position in the string in case string gets relocated. */
      pos++ ;
      cp = (tmp->str + pos) ;
    }

  tmp = g_string_prepend_c(tmp, '/') ;

  item_string = g_string_free(tmp, FALSE) ;

  return item_string ;
}



static ZMapGUIMenuItem makeSingleMenu(GList *menu_item_sets, int *menu_items_out)
{
  ZMapGUIMenuItem new_menu, curr_menu ;
  GList *curr_set ;
  int num_menu_items ;

  num_menu_items = 0 ;
  curr_set = g_list_first(menu_item_sets) ;
  while (curr_set)
    {
      ZMapGUIMenuItem sub_menu ;

      sub_menu = (ZMapGUIMenuItem)curr_set->data ;

      num_menu_items += itemsInMenu(sub_menu) ;

      curr_set = g_list_next(curr_set) ;
    }


  /* Allocate memory for new menu. */
  new_menu = g_new0(ZMapGUIMenuItemStruct, (num_menu_items + 1)) ;

  /* Here we want to call a modified version of copy menu..... */
  curr_set = g_list_first(menu_item_sets) ;
  curr_menu = new_menu ;
  while (curr_set)
    {
      ZMapGUIMenuItem sub_menu ;

      sub_menu = (ZMapGUIMenuItem)curr_set->data ;

      curr_menu = copyMenu2NewMenu(curr_menu, sub_menu) ;

      curr_set = g_list_next(curr_set) ;
    }


  if (menu_items_out)
    *menu_items_out = num_menu_items ;

  return new_menu ;
}


/* Returns pointer to next free menu item. */
static ZMapGUIMenuItem copyMenu2NewMenu(ZMapGUIMenuItem new_menu, ZMapGUIMenuItem old_menu)
{
  ZMapGUIMenuItem next_menu = NULL ;
  ZMapGUIMenuItem old_item, new_item ;
  int i, num_menu_items, menu_size  ;


  /* Copy over menu items from old to new. */
  num_menu_items = itemsInMenu(old_menu) ;
  menu_size = sizeof(ZMapGUIMenuItemStruct) * num_menu_items ;
  g_memmove(new_menu, old_menu, menu_size) ;

  /* Remember to copy the strings in the old menu. */
  for (i = 0, old_item = old_menu, new_item = new_menu ;
       i < num_menu_items ; i++, old_item++, new_item++)
    {
      new_item->name = g_strdup(old_item->name) ;
    }

  next_menu = new_menu + num_menu_items ;

  return next_menu ;
}


/* Destroy a menu allocated by copyMenu() function. */
static void destroyMenu(ZMapGUIMenuItem menu)
{
  int i, num_menu_items  ;
  ZMapGUIMenuItem menu_item ;

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
static int itemsInMenu(ZMapGUIMenuItem menu)
{
  int num_menu_items ;
  ZMapGUIMenuItem menu_item ;

  /* Count items in menu. */
  for (num_menu_items = 0, menu_item = menu ; menu_item->type != ZMAPGUI_MENU_NONE ; num_menu_items++, menu_item++) ;

  return num_menu_items ;
}



/* Note that for these activate and deactivate functions we _cannot_ use the
 * proper widget calls (gtk_check_menu_item_set_active) as these will cause
 * the code to think the user has already selected an item and the menu
 * gets popped up/down without them being able to do anything.
 *
 * Hence the need to poke directly into the widget struct. */
static void activeButCB(gpointer data, gpointer user_data)
{
  char *widg_path = (char *)data ;
  GtkItemFactory *item_factory = (GtkItemFactory *)user_data ;
  GtkWidget *but_widg ;

  but_widg = gtk_item_factory_get_widget(item_factory, widg_path) ;

  GTK_CHECK_MENU_ITEM(but_widg)->active = TRUE ;

  return ;
}


static void deActiveButCB(gpointer data, gpointer user_data)
{
  char *widg_path = (char *)data ;
  GtkItemFactory *item_factory = (GtkItemFactory *)user_data ;
  GtkWidget *but_widg ;

  but_widg = gtk_item_factory_get_widget(item_factory, widg_path) ;

  GTK_CHECK_MENU_ITEM(but_widg)->active = FALSE ;

  return ;
}


/* I'M NOT SURE WE NEED SEPARATE FUNCS FOR THE TOGGLE STUFF...I CAN'T SEEM TO FIND A WAY
 * TO GET THE BUTTONS DISPLAYED NICELY....SIGH.... */

/* Note that for these activate and deactivate functions we _cannot_ use the
 * proper widget calls (gtk_check_menu_item_set_active) as these will cause
 * the code to think the user has already selected an item and the menu
 * gets popped up/down without them being able to do anything.
 *
 * Hence the need to poke directly into the widget struct. */
static void toggleButCB(gpointer data, gpointer user_data)
{
  char *widg_path = (char *)data ;
  GtkItemFactory *item_factory = (GtkItemFactory *)user_data ;
  GtkWidget *but_widg ;

  but_widg = gtk_item_factory_get_widget(item_factory, widg_path) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* this doesn't seem to work, you can't get the toggle permanently displayed....sigh.. */
  gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(but_widg), TRUE) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  GTK_CHECK_MENU_ITEM(but_widg)->active = TRUE ;

  return ;
}


static void deToggleButCB(gpointer data, gpointer user_data)
{
  char *widg_path = (char *)data ;
  GtkItemFactory *item_factory = (GtkItemFactory *)user_data ;
  GtkWidget *but_widg ;

  but_widg = gtk_item_factory_get_widget(item_factory, widg_path) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* this doesn't seem to work, you can't get the toggle permanently displayed....sigh.. */
  gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(but_widg), TRUE) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  GTK_CHECK_MENU_ITEM(but_widg)->active = FALSE ;

  return ;
}



