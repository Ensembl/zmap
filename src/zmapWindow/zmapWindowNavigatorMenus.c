/*  File: zmapWindowNavigatorMenus.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Apr 18 11:09 2008 (rds)
 * Created: Wed Oct 18 08:21:15 2006 (rds)
 * CVS info:   $Id: zmapWindowNavigatorMenus.c,v 1.14 2008-04-18 10:13:22 rds Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindowNavigator_P.h>
#include <ZMap/zmapUtils.h>

static void navigatorBumpMenuCB(int menu_item_id, gpointer callback_data);
static void navigatorColumnMenuCB(int menu_item_id, gpointer callback_data);
static GHashTable *access_navigator_context_to_item(gpointer user_data);
static GHashTable *access_window_context_to_item(gpointer user_data);


static gboolean searchLocusSetCB(FooCanvasItem *item, gpointer user_data)
{
  GQuark locus_name = GPOINTER_TO_UINT(user_data);
  ZMapFeatureAny feature_any = NULL;
  gboolean match = FALSE;

  feature_any = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);
  zMapAssert(feature_any);

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
        ZMapFeature feature = (ZMapFeature)feature_any;
#ifdef THIS_LOCUS_STUFF_IS_A_PAIN
	/* quick fix to zmapWindowNavigatorShowSameNameList...  */
	if(locus_name == feature_any->original_id) 
	  match = TRUE;
#endif /* THIS_LOCUS_STUFF_IS_A_PAIN */

	/* Having just (locus_name == feature->locus_id) resulted in duplicates... */
	/* Added (feature->locus_id != feature->original_id) to fix RT # 64287 */
        if((locus_name == feature->locus_id)
	   && (feature->locus_id != feature->original_id))
          match = TRUE;
      }
      break;
    default:
      break;
    }
  
  return match;
}

void zmapWindowNavigatorGoToLocusExtents(ZMapWindowNavigator navigate, FooCanvasItem *item)
{
  ZMapWindow window = NULL;
  ZMapFeature feature = NULL;
  ZMapWindowFToIPredFuncCB callback = NULL ;
  GList *result = NULL, *feature_list;
  GQuark locus_quark = 0;
  char *wild_card = "*";

  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);
  zMapAssert(feature);

  window = navigate->current_window;

  callback    = searchLocusSetCB;
  locus_quark = g_quark_from_string(wild_card);
  
  if((result = zmapWindowFToIFindItemSetFull(window->context_to_item,
                                             feature->parent->parent->parent->unique_id,
                                             feature->parent->parent->unique_id,
                                             locus_quark, // feature->parent->unique_id, 
                                             wild_card, wild_card, locus_quark,
                                             callback, GUINT_TO_POINTER(feature->original_id))))
    {
      int start, end;
      if((feature_list = zmapWindowItemListToFeatureList(result)))
        {
          /* x coords are HACKED!!!! */
          if(zMapFeatureGetFeatureListExtent(feature_list, &start, &end))
            zmapWindowZoomToWorldPosition(window, TRUE, 0.0, start, 100.0, end);
          g_list_free(feature_list);
        }
    }
  else
    {
      zMapAssertNotReached();
    }

  return ;
}

/* This has the wrong name! it only works for finding variants! */
void zmapWindowNavigatorShowSameNameList(ZMapWindowNavigator navigate, FooCanvasItem *item)
{
  ZMapWindow window = NULL;
  ZMapFeature feature = NULL;
  ZMapWindowFToIPredFuncCB callback = NULL ;
  GList *result = NULL;
  GQuark locus_quark = 0;
  char *wild_card = "*";

  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);
  zMapAssert(feature);

  window = navigate->current_window;

#ifdef RDS_PROBLEMATIC_CODE
  /* Is it right to use window->context_to_item here??? */
  if(!(result = zmapWindowFToIFindSameNameItems(window->context_to_item,
                                                ZMAPSTRAND_NONE, ZMAPFRAME_NONE,
                                                feature)))
    {
#endif

      /* Here we going to search for transcripts in any feature set in
       * the _main_ window which have a locus set which == feature->original_id 
       * i.e. feature->original_id == the locus name
       */

      callback    = searchLocusSetCB;
      locus_quark = g_quark_from_string(wild_card);

      /* we use the wildcard to get all features... slow?? */
      result = zmapWindowFToIFindItemSetFull(window->context_to_item,
                                             feature->parent->parent->parent->unique_id,
                                             feature->parent->parent->unique_id,
					     locus_quark, /* feature->parent->unique_id,  */
                                             wild_card, wild_card, locus_quark,
                                             callback, GUINT_TO_POINTER(feature->original_id));


#ifdef RDS_PROBLEMATIC_CODE
    }
#endif

  if(result)
    {
      /* We have to access the window->context_to_item in the
       * WindowList and it does that with a callback. It must 
       * be the same window! */
      zmapWindowListWindowCreate(window, access_window_context_to_item, window, result,
                                 (char *)(g_quark_to_string(feature->original_id)), item, TRUE);
      g_list_free(result);  /* clean up list. */
    }

  return ;
}

static void popUpVariantList(int menu_item_id, gpointer callback_data)
{
  NavigateMenuCBData menu_data = (NavigateMenuCBData)callback_data ;

  switch (menu_item_id)
    {
    case 1:
      {
        zmapWindowNavigatorShowSameNameList(menu_data->navigate, menu_data->item);
      }
      break;
    default:
      break;
    }
  return ;
}

ZMapGUIMenuItem zmapWindowNavigatorMakeMenuLocusOps(int *start_index_inout,
                                                    ZMapGUIMenuItemCallbackFunc callback_func,
                                                    gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] = 
    {
      {ZMAPGUI_MENU_NORMAL, "Show Variants List", 1, popUpVariantList, NULL},
      {ZMAPGUI_MENU_NONE,   NULL,        0, NULL, NULL}
    };

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data);

  return menu;
}

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
      {ZMAPGUI_MENU_TOGGLE, "Column Bump|UnBump", ZMAPOVERLAP_ITEM_OVERLAP, navigatorBumpMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Column Hide",        ZMAPWWINDOWCOLUMN_HIDE,   NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;
  ZMapGUIMenuItem item ;

  /* Set the initial toggle button correctly....make sure this stays in step with the array above.
   * NOTE logic, this button is either "no bump" or "Name + No Overlap", the latter should be
   * selectable whatever.... */
  item = &(menu[0]) ;
  if (curr_overlap == ZMAPOVERLAP_ITEM_OVERLAP)
    {
      item->type = ZMAPGUI_MENU_TOGGLEACTIVE ;
      item->id = ZMAPOVERLAP_COMPLETE ; 
    }
  else
    {
      item->type = ZMAPGUI_MENU_TOGGLE ;
      item->id = ZMAPOVERLAP_ITEM_OVERLAP ;
    }

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}

static void navigatorBumpMenuCB(int menu_item_id, gpointer callback_data)
{
  NavigateMenuCBData   menu_data = (NavigateMenuCBData)callback_data ;
  ZMapStyleOverlapMode bump_type = (ZMapStyleOverlapMode)menu_item_id  ;
  FooCanvasItem *style_item ;

  /* This will only toggle the bumping */

  style_item = menu_data->item ;

  zmapWindowColumnBump(style_item, bump_type) ;

  zmapWindowNavigatorPositioning(menu_data->navigate);

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
        FooCanvasItem *set_item = menu_data->item;
        GList *list ;
	
        feature = (ZMapFeatureAny)g_object_get_data(G_OBJECT(menu_data->item), ITEM_FEATURE_DATA) ;

        if(feature->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
          {
            /* a small hack for the time being... */
            set_item = FOO_CANVAS_ITEM(zmapWindowContainerGetParentContainerFromItem(menu_data->item));
            feature = (ZMapFeatureAny)(feature->parent);
          }

        set_data = g_object_get_data(G_OBJECT(set_item), ITEM_FEATURE_SET_DATA) ;
	zMapAssert(set_data) ;

	list = zmapWindowFToIFindItemSetFull(menu_data->navigate->ftoi_hash, 
					     feature->parent->parent->unique_id,
					     feature->parent->unique_id,
					     feature->unique_id,
					     zMapFeatureStrand2Str(set_data->strand),
					     zMapFeatureFrame2Str(set_data->frame),
					     g_quark_from_string("*"), NULL, NULL) ;
	
        zmapWindowListWindowCreate(menu_data->navigate->current_window, 
				   access_navigator_context_to_item,
				   menu_data->navigate,
				   list, 
                                   (char *)g_quark_to_string(feature->original_id), 
                                   NULL, TRUE) ;

	break ;
      }
    case 2:
      zmapWindowCreateSearchWindow(menu_data->navigate->current_window, 
				   access_navigator_context_to_item,
				   menu_data->navigate,
				   menu_data->item) ;
      break ;

    case 5:
      zmapWindowCreateSequenceSearchWindow(menu_data->navigate->current_window, menu_data->item, ZMAPSEQUENCE_DNA) ;

      break ;

    case 6:
      zmapWindowCreateSequenceSearchWindow(menu_data->navigate->current_window, menu_data->item, ZMAPSEQUENCE_PEPTIDE) ;

      break ;

    default:
      zMapAssert("Coding error, unrecognised menu item number.") ;
      break ;
    }

  g_free(menu_data) ;

  return ;
}

static GHashTable *access_navigator_context_to_item(gpointer user_data)
{
  ZMapWindowNavigator navigator = (ZMapWindowNavigator)user_data;
  GHashTable *context_to_item;

  context_to_item = navigator->ftoi_hash;

  return context_to_item;
}

static GHashTable *access_window_context_to_item(gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)user_data;
  GHashTable *context_to_item;

  context_to_item = window->context_to_item;

  return context_to_item;
}
