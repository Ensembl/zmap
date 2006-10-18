/*  File: zmapWindowNavigatorMenus.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2006
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Oct 18 09:28 2006 (rds)
 * Created: Wed Oct 18 08:21:15 2006 (rds)
 * CVS info:   $Id: zmapWindowNavigatorMenus.c,v 1.1 2006-10-18 15:28:41 rds Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindowNavigator_P.h>
#include <ZMap/zmapUtils.h>

static void navigatorBumpMenuCB(int menu_item_id, gpointer callback_data);
static void navigatorColumnMenuCB(int menu_item_id, gpointer callback_data);

ZMapGUIMenuItem zmapWindowNavigatorMakeMenuColumnOps(int *start_index_inout,
                                                     ZMapGUIMenuItemCallbackFunc callback_func,
                                                     gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, "Show Feature List",      1, navigatorColumnMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Feature Search Window",  2, navigatorColumnMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                       0, NULL,                  NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}

ZMapGUIMenuItem zmapWindowNavigatorMakeMenuBump(int *start_index_inout,
                                                ZMapGUIMenuItemCallbackFunc callback_func,
                                                gpointer callback_data, ZMapStyleOverlapMode curr_overlap)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_TOGGLE, "Column Bump",      ZMAPOVERLAP_COMPLETE,   NULL, NULL},
      {ZMAPGUI_MENU_NORMAL, "Column Hide",      ZMAPWWINDOWCOLUMN_HIDE, NULL, NULL},
      {ZMAPGUI_MENU_BRANCH, "Column Bump Opts", 0,                      NULL, NULL},
      {ZMAPGUI_MENU_RADIO,  "Column Bump Opts/Compact Cluster + No Interleave", ZMAPOVERLAP_NO_INTERLEAVE, navigatorBumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  "Column Bump Opts/Compact Cluster + Interleave",    ZMAPOVERLAP_COMPLEX,       navigatorBumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  "Column Bump Opts/Cluster",                ZMAPOVERLAP_NAME,     navigatorBumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  "Column Bump Opts/No Overlap",             ZMAPOVERLAP_OVERLAP,  navigatorBumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  "Column Bump Opts/Bump on Start Position", ZMAPOVERLAP_POSITION, navigatorBumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  "Column Bump Opts/Bump everything",        ZMAPOVERLAP_SIMPLE,   navigatorBumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  "Column Bump Opts/Unbump",                 ZMAPOVERLAP_COMPLETE, navigatorBumpMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;
  ZMapGUIMenuItem item ;

  /* Set the initial toggle button correctly....make sure this stays in step with the array above.
   * NOTE logic, this button is either "no bump" or "Name + No Overlap", the latter should be
   * selectable whatever.... */
  item = &(menu[0]) ;
  if (curr_overlap == ZMAPOVERLAP_NO_INTERLEAVE)
    {
      item->type = ZMAPGUI_MENU_TOGGLEACTIVE ;
      item->id = ZMAPOVERLAP_NO_INTERLEAVE ;
    }
  else
    {
      item->type = ZMAPGUI_MENU_TOGGLE ;
      item->id = ZMAPOVERLAP_COMPLETE ;
    }


  /* Unset any previous active radio button.... */
  item = &(menu[0]) ;
  while (item->type != ZMAPGUI_MENU_NONE)
    {
      if (item->type == ZMAPGUI_MENU_RADIOACTIVE)
	{
	  item->type = ZMAPGUI_MENU_RADIO ;
	  break ;
	}

      item++ ;
    }

  /* Set new one... */
  item = &(menu[0]) ;
  while (item->type != ZMAPGUI_MENU_NONE)
    {
      if (item->type == ZMAPGUI_MENU_RADIO && item->id == curr_overlap)
	{
	  item->type = ZMAPGUI_MENU_RADIOACTIVE ;
	  break ;
	}

      item++ ;
    }

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}

static void navigatorBumpMenuCB(int menu_item_id, gpointer callback_data)
{
  NavigateMenuCBData   menu_data = (NavigateMenuCBData)callback_data ;
  ZMapStyleOverlapMode bump_type = (ZMapStyleOverlapMode)menu_item_id  ;
  FooCanvasGroup *column_group ;
  FooCanvasItem *style_item ;

  if (menu_data->item_cb)
    {
      //column_group = getItemsColGroup(menu_data->item) ;
      printf("what does this do?\n");
    }
  else
    {
      column_group = FOO_CANVAS_GROUP(menu_data->item) ;
    }

  style_item = menu_data->item ;

  zmapWindowColumnBump(style_item, bump_type) ;


  g_free(menu_data) ;

  return ;
}

static void navigatorColumnMenuCB(int menu_item_id, gpointer callback_data)
{
  NavigateMenuCBData menu_data = (NavigateMenuCBData)callback_data ;

  switch (menu_item_id)
    {
    case 1:
      {
        ZMapFeatureAny feature ;
	ZMapWindowItemFeatureSetData set_data ;
        GList *list ;
	
        feature = (ZMapFeatureAny)g_object_get_data(G_OBJECT(menu_data->item), ITEM_FEATURE_DATA) ;

	set_data = g_object_get_data(G_OBJECT(menu_data->item), ITEM_FEATURE_SET_DATA) ;
	zMapAssert(set_data) ;

	list = zmapWindowFToIFindItemSetFull(menu_data->navigate->ftoi_hash, 
					     feature->parent->parent->unique_id,
					     feature->parent->unique_id,
					     feature->unique_id,
					     zMapFeatureStrand2Str(set_data->strand),
					     zMapFeatureFrame2Str(set_data->frame),
					     g_quark_from_string("*"), NULL, NULL) ;
	
        zmapWindowListWindowCreate(menu_data->navigate->current_window, list, 
                                   (char *)g_quark_to_string(feature->original_id), 
                                   NULL) ;

	break ;
      }
    case 2:
      zmapWindowCreateSearchWindow(menu_data->navigate->current_window, menu_data->item) ;
      break ;

    case 5:
      zmapWindowCreateDNAWindow(menu_data->navigate->current_window, menu_data->item) ;

      break ;

    default:
      zMapAssert("Coding error, unrecognised menu item number.") ;
      break ;
    }

  g_free(menu_data) ;

  return ;
}
