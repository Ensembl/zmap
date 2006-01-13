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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Code implementing the menus for sequence display.
 *
 * Exported functions: ZMap/zmapWindows.h
 *              
 * HISTORY:
 * Last edited: Jan 12 15:29 2006 (edgrif)
 * Created: Thu Mar 10 07:56:27 2005 (edgrif)
 * CVS info:   $Id: zmapWindowMenus.c,v 1.6 2006-01-13 18:54:47 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsDNA.h>
#include <ZMap/zmapGFF.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>



static void bumpMenuCB(int menu_item_id, gpointer callback_data) ;
static void dumpMenuCB(int menu_item_id, gpointer callback_data) ;
static void blixemMenuCB(int menu_item_id, gpointer callback_data) ;

static FooCanvasGroup *getItemsColGroup(FooCanvasItem *item) ;

static void dumpFASTA(ZMapWindow window) ;
static void dumpFeatures(ZMapWindow window, ZMapFeatureAny feature) ;
static void dumpContext(ZMapWindow window) ;





/* Set of makeMenuXXXX functions to create common subsections of menus. If you add to this
 * you should make sure you understand how to specify menu paths in the item factory style.
 * If you get it wrong then the menus will be scr*wed up.....
 * 
 * The functions are defined in pairs: one to define the menu, one to handle the callback
 * actions, this is to emphasise that their indexes must be kept in step !
 * 
 * NOTE HOW THE MENUS ARE DECLARED STATIC IN THE VARIOUS ROUTINES TO MAKE SURE THEY STAY
 * AROUND...OTHERWISE WE WILL HAVE TO KEEP ALLOCATING/DEALLOCATING THEM.....
 */


/* Probably it would be wise to pass in the callback function, the start index for the item
 * identifier and perhaps the callback data...... */
ZMapGUIMenuItem zmapWindowMakeMenuBump(int *start_index_inout,
				       ZMapGUIMenuItemCallbackFunc callback_func,
				       gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {"_Bump", 0, NULL, NULL},
      {"Bump/Column UnBump",        ZMAPOVERLAP_COMPLETE, bumpMenuCB, NULL},
      {"Bump/Column Bump Position", ZMAPOVERLAP_POSITION, bumpMenuCB, NULL},
      {"Bump/Column Bump Overlap",  ZMAPOVERLAP_OVERLAP,  bumpMenuCB, NULL},
      {"Bump/Column Bump Name",     ZMAPOVERLAP_NAME,     bumpMenuCB, NULL},
      {"Bump/Column Bump Simple",   ZMAPOVERLAP_SIMPLE,   bumpMenuCB, NULL},
      {NULL, 0, NULL, NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}

/* Bump a column and reposition the other columns. */
static void bumpMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapStyleOverlapMode bump_type = (ZMapStyleOverlapMode)menu_item_id  ;
  FooCanvasGroup *column_group ;

  if (menu_data->item_cb)
    column_group = getItemsColGroup(menu_data->item) ;
  else
    column_group = FOO_CANVAS_GROUP(menu_data->item) ;

  zmapWindowColumnBump(column_group, bump_type) ;
  zmapWindowColumnReposition(column_group) ;

  g_free(menu_data) ;

  return ;
}


ZMapGUIMenuItem zmapWindowMakeMenuDumpOps(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {"_Dump",                  0, NULL,       NULL},
      {"Dump/Dump DNA"          , 1, dumpMenuCB, NULL},
      {"Dump/Dump Features"     , 2, dumpMenuCB, NULL},
      {"Dump/Dump Context"      , 3, dumpMenuCB, NULL},
      {NULL               , 0, NULL, NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}

static void dumpMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapFeatureAny feature ;

  feature = (ZMapFeatureAny)g_object_get_data(G_OBJECT(menu_data->item), "item_feature_data") ;

  switch (menu_item_id)
    {
    case 1:
      {
	dumpFASTA(menu_data->window) ;

	break ;
      }
    case 2:
      {
	dumpFeatures(menu_data->window, feature) ;

	break ;
      }
    case 3:
      {
	dumpContext(menu_data->window) ;

	break ;
      }
    default:
      zMapAssert("Coding error, unrecognised menu item number.") ; /* exits... */
      break ;
    }

  g_free(menu_data) ;

  return ;
}


ZMapGUIMenuItem zmapWindowMakeMenuDNAHomol(int *start_index_inout,
					   ZMapGUIMenuItemCallbackFunc callback_func,
					   gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {"_Blixem",      0, NULL, NULL},
      {"Blixem/Show multiple dna alignment",                                 1, blixemMenuCB, NULL},
      {"Blixem/Show multiple dna alignment for just this type of homology",  2, blixemMenuCB, NULL},
      {NULL,                                                          0, NULL,         NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


ZMapGUIMenuItem zmapWindowMakeMenuProteinHomol(int *start_index_inout,
					       ZMapGUIMenuItemCallbackFunc callback_func,
					       gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {"_Blixem",      0, NULL, NULL},
      {"Blixem/Show multiple protein alignment",                                 1, blixemMenuCB, NULL},
      {"Blixem/Show multiple protein alignment for just this type of homology",  2, blixemMenuCB, NULL},
      {NULL,                                                              0, NULL,         NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


/* call blixem either for a single type of homology or for all homologies. */
static void blixemMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  gboolean single_homol_type = FALSE ;
  gboolean status ;

  switch (menu_item_id)
    {
    case 1:
      single_homol_type = FALSE ;
      break ;
    case 2:
      single_homol_type = TRUE ;
      break ;
    default:
      zMapAssert("Coding error, unrecognised menu item number.") ; /* exits... */
      break ;
    }

  status = zmapWindowCallBlixem(menu_data->window, menu_data->item, single_homol_type) ;
  
  g_free(menu_data) ;

  return ;
}





/* this needs to be a general function... */
static FooCanvasGroup *getItemsColGroup(FooCanvasItem *item)
{
  FooCanvasGroup *group = NULL ;
  ZMapWindowItemFeatureType item_feature_type ;


  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							"item_feature_type")) ;

  switch (item_feature_type)
    {
    case ITEM_FEATURE_SIMPLE:
    case ITEM_FEATURE_PARENT:
      group = zmapWindowContainerGetParent(item->parent) ;
      break ;
    case ITEM_FEATURE_CHILD:
    case ITEM_FEATURE_BOUNDING_BOX:
      group = zmapWindowContainerGetParent(item->parent->parent) ;
      break ;
    default:
      zMapAssert("Coding error, unrecognised menu item number.") ;
      break ;
    }

  return group ;
}





static void dumpFASTA(ZMapWindow window)
{
  gboolean dna = TRUE ;
  char *filepath = NULL ;
  GIOChannel *file = NULL ;
  GError *error = NULL ;
  char *seq_name = NULL ;
  int seq_len = 0 ;
  char *sequence = NULL ;
  char *error_prefix = "FASTA DNA dump failed:" ;

  if (!(dna = zmapFeatureContextDNA(window->feature_context, &seq_name, &seq_len, &sequence))
      || !(filepath = zmapGUIFileChooser(window->toplevel, "FASTA filename ?", NULL, NULL))
      || !(file = g_io_channel_new_file(filepath, "w", &error))
      || !zmapDNADumpFASTA(file, seq_name, seq_len, sequence, &error))
    {
      char *err_msg = NULL ;

      /* N.B. if there is no filepath it means user cancelled so take no action... */
      if (!dna)
	err_msg = "there is no DNA to dump." ;
      else if (error)
	err_msg = error->message ;

      if (err_msg)
	zMapShowMsg(ZMAP_MSG_WARNING, "%s  %s", error_prefix, err_msg) ;

      if (error)
	g_error_free(error) ;
    }


  if (file)
    {
      GIOStatus status ;

      if ((status = g_io_channel_shutdown(file, TRUE, &error)) != G_IO_STATUS_NORMAL)
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "%s  %s", error_prefix, error->message) ;

	  g_error_free(error) ;
	}
    }


  return ;
}


static void dumpFeatures(ZMapWindow window, ZMapFeatureAny feature)
{
  char *filepath = NULL ;
  GIOChannel *file = NULL ;
  GError *error = NULL ;
  char *error_prefix = "Features dump failed:" ;
  ZMapFeatureBlock feature_block ;

  /* Should extend this any type.... */
  zMapAssert(feature->struct_type == ZMAPFEATURE_STRUCT_FEATURESET
	     || feature->struct_type == ZMAPFEATURE_STRUCT_FEATURE) ;

  /* Find the block from whatever pointer we are sent...  */
  if (feature->struct_type == ZMAPFEATURE_STRUCT_FEATURESET)
    feature_block = (ZMapFeatureBlock)(feature->parent) ;
  else
    feature_block = (ZMapFeatureBlock)(feature->parent->parent) ;


  if (!(filepath = zmapGUIFileChooser(window->toplevel, "Feature Dump filename ?", NULL, "gff"))
      || !(file = g_io_channel_new_file(filepath, "w", &error))
      || !zMapGFFDump(file, feature_block, &error))
    {
      /* N.B. if there is no filepath it means user cancelled so take no action...,
       * otherwise we output the error message. */
      if (error)
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "%s  %s", error_prefix, error->message) ;

	  g_error_free(error) ;
	}
    }


  if (file)
    {
      GIOStatus status ;

      if ((status = g_io_channel_shutdown(file, TRUE, &error)) != G_IO_STATUS_NORMAL)
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "%s  %s", error_prefix, error->message) ;

	  g_error_free(error) ;
	}
    }


  return ;
}




static void dumpContext(ZMapWindow window)
{
  char *filepath = NULL ;
  GIOChannel *file = NULL ;
  GError *error = NULL ;
#ifdef RDS_DONT_INCLUDE
  char *seq_name = NULL ;
  int seq_len = 0 ;
  char *sequence = NULL ;
#endif
  char *error_prefix = "Feature context dump failed:" ;

  if (!(filepath = zmapGUIFileChooser(window->toplevel, "Context Dump filename ?", NULL, "zmap"))
      || !(file = g_io_channel_new_file(filepath, "w", &error))
      || !zMapFeatureContextDump(file, window->feature_context, &error))
    {
      /* N.B. if there is no filepath it means user cancelled so take no action...,
       * otherwise we output the error message. */
      if (error)
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "%s  %s", error_prefix, error->message) ;

	  g_error_free(error) ;
	}
    }


  if (file)
    {
      GIOStatus status ;

      if ((status = g_io_channel_shutdown(file, TRUE, &error)) != G_IO_STATUS_NORMAL)
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "%s  %s", error_prefix, error->message) ;

	  g_error_free(error) ;
	}
    }


  return ;
}





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE


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
  ZMapWindowMenuItem callers_menu_copy ;
  GtkItemFactoryEntry *factory_items ;
  int num_factory_items ;
} CallbackDataStruct, *CallbackData ;


static char *makeMenuItemName(char *string) ;
static ZMapWindowMenuItem makeSingleMenu(GList *menu_item_sets, int *menu_items_out) ;
static ZMapWindowMenuItem copyMenu2NewMenu(ZMapWindowMenuItem new_menu, ZMapWindowMenuItem old_menu) ;
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
void zMapWindowMakeMenu(char *menu_title, GList *menu_item_sets,
			GdkEventButton *button_event)
{
  int num_menu_items, num_factory_items, i, menu_size ;
  ZMapWindowMenuItem menu_items, menu_copy ;
  GtkItemFactoryEntry *factory_items, *item ;
  GtkItemFactory *item_factory ;
  CallbackData our_cb_data ;


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

      /* This is not great, I need to add some types etc. to deal with submenus vs. other things. */
      if (*(menu_items[i].name) == '_')
	{
	  item->item_type = "<Branch>" ;
	  item->callback = NULL ;
	}

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



static ZMapWindowMenuItem makeSingleMenu(GList *menu_item_sets, int *menu_items_out)
{
  ZMapWindowMenuItem new_menu, curr_menu ;
  GList *curr_set ;
  int num_menu_items ;

  num_menu_items = 0 ;
  curr_set = g_list_first(menu_item_sets) ;
  while (curr_set)
    {
      ZMapWindowMenuItem sub_menu ;

      sub_menu = (ZMapWindowMenuItem)curr_set->data ;

      num_menu_items += itemsInMenu(sub_menu) ;

      curr_set = g_list_next(curr_set) ;
    }


  /* Allocate memory for new menu. */
  new_menu = g_new0(ZMapWindowMenuItemStruct, (num_menu_items + 1)) ;

  /* Here we want to call a modified version of copy menu..... */
  curr_set = g_list_first(menu_item_sets) ;
  curr_menu = new_menu ;
  while (curr_set)
    {
      ZMapWindowMenuItem sub_menu ;

      sub_menu = (ZMapWindowMenuItem)curr_set->data ;

      curr_menu = copyMenu2NewMenu(curr_menu, sub_menu) ;

      curr_set = g_list_next(curr_set) ;
    }


  if (menu_items_out)
    *menu_items_out = num_menu_items ;

  return new_menu ;
}


/* Returns pointer to next free menu item. */
static ZMapWindowMenuItem copyMenu2NewMenu(ZMapWindowMenuItem new_menu, ZMapWindowMenuItem old_menu)
{
  ZMapWindowMenuItem next_menu = NULL ;
  ZMapWindowMenuItem old_item, new_item ;
  int i, num_menu_items, menu_size  ;


  /* Copy over menu items from old to new. */
  num_menu_items = itemsInMenu(old_menu) ;
  menu_size = sizeof(ZMapWindowMenuItemStruct) * num_menu_items ;
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
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



