/*  File: zmapWindowNavigatorMenus.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>


#include <zmapWindowNavigator_P.hpp>
#include <ZMap/zmapUtils.hpp>
#include <zmapWindowContainerFeatureSet_I.hpp>                    /* agggggghhhhhhhhhhhh!!!!!!!!!!!!! */



#define FILTER_DATA_KEY        "ZMapWindowNavigatorFilterData"
#define FILTER_CANCEL_DATA_KEY "ZMapWindowNavigatorFilterCancelData"



static void navigatorBumpMenuCB(int menu_item_id, gpointer callback_data);
static void navigatorColumnMenuCB(int menu_item_id, gpointer callback_data);
static GHashTable *access_navigator_context_to_item(gpointer user_data);
static GHashTable *access_window_context_to_item(gpointer user_data);

static GtkWidget *newTopLevel(const char *title);
static void destroyFilterCB(GtkWidget *widget, gpointer user_data);
static void filter_checkbox_toggled_cb(GtkWidget *checkbox, gpointer user_data);
static void makeFilterPanel(ZMapWindowNavigator navigator, GtkWidget *parent);
static void cancel_destroy_cb(GtkWidget *widget, gpointer user_data);
static void apply_destroy_cb(GtkWidget *widget, gpointer user_data);
static void zmapWindowNavigatorLocusFilterEditorCreate(ZMapWindowNavigator navigator);
static gboolean searchLocusSetCB(ZMapFeatureAny feature_any, gpointer user_data) ;

void zmapWindowNavigatorGoToLocusExtents(ZMapWindowNavigator navigate, FooCanvasItem *item)
{
  ZMapWindow window = NULL;
  ZMapFeature feature = NULL;
  ZMapWindowFToIPredFuncCB callback = NULL ;
  GList *result = NULL, *feature_list;
  GQuark locus_quark = 0;
  const char *wild_card = "*";

  if (!navigate || !navigate->current_window || !item)
    return ;

  feature = zmapWindowItemGetFeature(item);
  if (!feature)
    return ;

  window = navigate->current_window;

  callback    = searchLocusSetCB;
  locus_quark = g_quark_from_string(wild_card);

  if((result = zmapWindowFToIFindItemSetFull(window,window->context_to_item,
                                             feature->parent->parent->parent->unique_id,
                                             feature->parent->parent->unique_id,
                                             locus_quark, 0,
                                             wild_card, wild_card, locus_quark,
                                             callback, GUINT_TO_POINTER(feature->original_id))))
    {
      int start, end;
      if((feature_list = zmapWindowItemListToFeatureList(result)))
        {
          /* x coords are HACKED!!!! */
          if(zMapFeatureGetFeatureListExtent(feature_list, &start, &end))
            {
              zmapWindowZoomToWorldPosition(window, TRUE, 0.0, start, 100.0, end);
            }
          g_list_free(feature_list);
        }
    }
  else
    {
      zMapWarnIfReached();
    }

  return ;
}




/* This has the wrong name! it only works for finding variants! */
void zmapWindowNavigatorShowSameNameList(ZMapWindowNavigator navigate, FooCanvasItem *item)
{
  ZMapWindow window = NULL;
  ZMapFeature feature = NULL;
  GQuark wild_card_id ;
  const char *wild_card = "*" ;

  wild_card_id = g_quark_from_string(wild_card) ;


  if (!navigate || !navigate->current_window || !item )
    return ;

  feature = zmapWindowItemGetFeature(item) ;
  if (!feature)
    return ;

  window = navigate->current_window;


#define USING_SET_SEARCH_DATA_METHOD

#ifndef USING_SET_SEARCH_DATA_METHOD
  ZMapWindowFToIPredFuncCB callback = NULL ;
  GList *result = NULL;


#ifdef RDS_PROBLEMATIC_CODE
  /* Is it right to use window->context_to_item here??? */
  if(!(result = zmapWindowFToIFindSameNameItems(window,window->context_to_item,
                                                ZMAPSTRAND_NONE, ZMAPFRAME_NONE,
                                                feature)))
    {
#endif

      /* Here we going to search for transcripts in any feature set in
       * the _main_ window which have a locus set which == feature->original_id
       * i.e. feature->original_id == the locus name
       */

      callback    = searchLocusSetCB;

      /* we use the wildcard to get all features... slow?? */
      result = zmapWindowFToIFindItemSetFull(window,window->context_to_item,
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
      gboolean zoom_to_item = TRUE ;
      /* We have to access the window->context_to_item in the
       * WindowList and it does that with a callback. It must
       * be the same window! */
      zmapWindowListWindowCreate(window, item,
                                 (char *)(g_quark_to_string(feature->original_id)),
                                 access_window_context_to_item, window,
                                 NULL, NULL,
                                 zoom_to_item);
      g_list_free(result);  /* clean up list. */
    }

#else /* USING_SET_SEARCH_DATA_METHOD */

  {
    ZMapWindowFToISetSearchData search_data;
    gboolean zoom_to_item = TRUE ;
    char *wild_name ;

    wild_name = g_strdup_printf("%s*", g_quark_to_string(feature->original_id)) ;
    g_free(wild_name) ;

    search_data = zmapWindowFToISetSearchCreateFull((void *)zmapWindowFToIFindItemSetFull,
                                                    NULL,
                                                    feature->parent->parent->parent->unique_id,
                                                    feature->parent->parent->unique_id,
                                                    wild_card_id,
                                                    wild_card_id,
                                                    wild_card_id,
                                                    wild_card, /* strand */
                                                    wild_card, /* frame */
                                                    searchLocusSetCB,
                                                    GUINT_TO_POINTER(feature->original_id),
                                                    NULL) ;

    zmapWindowListWindowCreate(window, item,
                               (char *)(g_quark_to_string(feature->original_id)),
                               access_window_context_to_item,  window,
                               window->context_map,
                               (ZMapWindowListSearchHashFunc)zmapWindowFToISetSearchPerform, search_data,
                               (GDestroyNotify)zmapWindowFToISetSearchDestroy, zoom_to_item);

  }
#endif /* USING_SET_SEARCH_DATA_METHOD */



  return ;
}





static void popUpVariantList(int menu_item_id, gpointer callback_data)
{
  NavigateMenuCBData menu_data = (NavigateMenuCBData)callback_data ;

  switch (menu_item_id)
    {
    case 1:
      zmapWindowNavigatorShowSameNameList(menu_data->navigate, menu_data->item);
      break;
    case 2:
      zmapWindowNavigatorLocusFilterEditorCreate(menu_data->navigate);
      break;
    case 3:
      g_list_free(menu_data->navigate->hide_filter);
      menu_data->navigate->hide_filter = NULL;
      zmapWindowNavigatorLocusRedraw(menu_data->navigate);
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
      {ZMAPGUI_MENU_NORMAL, "Show Variants List",   1, popUpVariantList, NULL},
      {ZMAPGUI_MENU_NONE,   NULL,        0, NULL, NULL}
    };

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data);

  return menu;
}

ZMapGUIMenuItem zmapWindowNavigatorMakeMenuLocusColumnOps(int *start_index_inout,
                                                          ZMapGUIMenuItemCallbackFunc callback_func,
                                                          gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, "Filter Loci",   2, popUpVariantList, NULL},
      {ZMAPGUI_MENU_NORMAL, "Show All Loci", 3, popUpVariantList, NULL},
      {ZMAPGUI_MENU_NONE,   NULL,            0, NULL, NULL}
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
                                                gpointer callback_data, ZMapStyleBumpMode curr_bump)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_TOGGLE, "Column Bump|UnBump", ZMAPBUMP_ALTERNATING, navigatorBumpMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Column Hide",        ZMAPWINDOWCOLUMN_HIDE,   NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;
  ZMapGUIMenuItem item ;

  /* Set the initial toggle button correctly....make sure this stays in step with the array above.
   * NOTE logic, this button is either "no bump" or "Name + No Bump", the latter should be
   * selectable whatever.... */
  item = &(menu[0]) ;
  if (curr_bump != ZMAPBUMP_UNBUMP)
    {
      item->type = ZMAPGUI_MENU_TOGGLEACTIVE ;
      item->id = ZMAPBUMP_UNBUMP ;
    }
  else
    {
      item->type = ZMAPGUI_MENU_TOGGLE ;
      item->id   = ZMAPBUMP_ALTERNATING ;
    }

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}

static void navigatorBumpMenuCB(int menu_item_id, gpointer callback_data)
{
  NavigateMenuCBData   menu_data = (NavigateMenuCBData)callback_data ;
  ZMapStyleBumpMode bump_type = (ZMapStyleBumpMode)menu_item_id  ;
  FooCanvasItem *style_item ;

  /* This will only toggle the bumping */

  style_item = menu_data->item ;

  if(!ZMAP_IS_CONTAINER_GROUP(style_item))
    style_item = (FooCanvasItem *)zmapWindowContainerCanvasItemGetContainer(style_item);


  zmapWindowColumnBump(style_item, bump_type) ;

  zmapWindowFullReposition(menu_data->navigate->container_root,TRUE, "nav bump");

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
        ZMapWindowContainerFeatureSet container;
        ZMapWindowFToISetSearchData search_data;
        FooCanvasItem *set_item = menu_data->item;
        gboolean zoom_to_item = TRUE ;

        feature = zmapWindowItemGetFeatureAny(menu_data->item);

        if (feature->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
          {
            /* a small hack for the time being... */
            set_item = FOO_CANVAS_ITEM(zmapWindowContainerCanvasItemGetContainer(menu_data->item));
            feature = (ZMapFeatureAny)(feature->parent);
          }

        container = (ZMapWindowContainerFeatureSet)set_item;

        search_data = zmapWindowFToISetSearchCreate((void *)zmapWindowFToIFindItemSetFull, NULL,
                                                    feature->parent->parent->unique_id,
                                                    feature->parent->unique_id,
                                                    container->unique_id,
                                                    0,
                                                    g_quark_from_string("*"),
                                                    zMapFeatureStrand2Str(container->strand),
                                                    zMapFeatureFrame2Str(container->frame));

        zmapWindowListWindowCreate(menu_data->navigate->current_window,
                                   NULL,
                                   (char *)g_quark_to_string(feature->original_id),
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                                   access_navigator_context_to_item,
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
                                   access_window_context_to_item,
                                   menu_data->navigate->current_window,
                                   menu_data->navigate->current_window->context_map,
                                   (ZMapWindowListSearchHashFunc)zmapWindowFToISetSearchPerform, search_data,
                                   (GDestroyNotify)zmapWindowFToISetSearchDestroy, zoom_to_item) ;

        break ;
      }
    case 2:
      zmapWindowCreateSearchWindow(menu_data->navigate->current_window,
                                   access_navigator_context_to_item,
                                   menu_data->navigate,
                                   menu_data->navigate->current_window->context_map,
                                   menu_data->item) ;
      break ;

    case 5:
      zmapWindowCreateSequenceSearchWindow(menu_data->navigate->current_window, menu_data->item, ZMAPSEQUENCE_DNA) ;

      break ;

    case 6:
      zmapWindowCreateSequenceSearchWindow(menu_data->navigate->current_window, menu_data->item, ZMAPSEQUENCE_PEPTIDE) ;

      break ;

    default:
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

static GtkWidget *newTopLevel(const char *title)
{
  GtkWidget *window;

  window = zMapGUIToplevelNew(NULL, title) ;

  gtk_window_set_default_size(GTK_WINDOW(window), -1, -1);
  gtk_container_border_width(GTK_CONTAINER(window), 5) ;

  return window;
}

static void destroyFilterCB(GtkWidget *widget, gpointer user_data)
{
  GList *copied_list;

  if((copied_list = (GList *)g_object_get_data(G_OBJECT(widget), FILTER_CANCEL_DATA_KEY)))
    g_list_free(copied_list);

  return ;
}


static void filter_checkbox_toggled_cb(GtkWidget *checkbox, gpointer user_data)
{
  ZMapWindowNavigator navigator = (ZMapWindowNavigator)user_data;
  GList *in_hide_list;
  char *filter = NULL;
  gboolean button_pressed;

  button_pressed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbox));
  filter = (char *)g_object_get_data(G_OBJECT(checkbox), FILTER_DATA_KEY);

  if (!filter)
    return ;

  if((in_hide_list = g_list_find(navigator->hide_filter, filter)))
    {
      if(!button_pressed)
        navigator->hide_filter = g_list_remove_all(navigator->hide_filter, filter);
    }
  else if(button_pressed)
    {
      navigator->hide_filter = g_list_append(navigator->hide_filter, filter);
    }
  else if(!button_pressed)
    {
      zMapLogWarning("Turning filter '%s' off, but it's not in the list.", filter);
    }

  return ;
}


static void makeFilterPanel(ZMapWindowNavigator navigator, GtkWidget *parent)
{
  int list_length;

  if((list_length = g_list_length(navigator->available_filters)) > 0 &&
     GTK_IS_CONTAINER(parent))
    {
      GList *tmp;
      tmp = navigator->available_filters;
      do
        {
          GtkWidget *checkbox_with_label;
          GList *filter_list;
          char *filter;
          gboolean filter_active = FALSE;

          filter = (char *)(tmp->data);

          checkbox_with_label = gtk_check_button_new_with_label(filter);

          g_object_set_data(G_OBJECT(checkbox_with_label), FILTER_DATA_KEY, filter);

          if((filter_list = g_list_find(navigator->hide_filter, filter)))
            filter_active = TRUE;

          g_object_set(G_OBJECT(checkbox_with_label),
                       "active", filter_active,
                       NULL);

          g_signal_connect(G_OBJECT(checkbox_with_label), "toggled",
                           G_CALLBACK(filter_checkbox_toggled_cb), navigator);

          if(GTK_IS_BOX(parent))
            gtk_box_pack_start(GTK_BOX(parent), checkbox_with_label, FALSE, FALSE, 0);
          else
            gtk_container_add(GTK_CONTAINER(parent), checkbox_with_label);
        }
      while((tmp = g_list_next(tmp)));
    }

  return ;
}

static void cancel_destroy_cb(GtkWidget *widget, gpointer user_data)
{
  ZMapWindowNavigator navigator = (ZMapWindowNavigator)user_data;
  GtkWidget *toplevel;

  if((toplevel = zMapGUIFindTopLevel(widget)))
    {
      GList *tmp;

      /* logic here is just to swap the pointers between the navigator and
       * the toplevel g_object_set_data version... */
      tmp = navigator->hide_filter;

      navigator->hide_filter = (GList *)g_object_get_data(G_OBJECT(toplevel), FILTER_CANCEL_DATA_KEY);

      /* Hope this works... It should, we're not destroying yet... */
      g_object_set_data(G_OBJECT(toplevel), FILTER_CANCEL_DATA_KEY, tmp);

      /* The destroy cb will liberate the list for us... */
      gtk_widget_destroy(toplevel);
    }

  return ;
}

static void apply_destroy_cb(GtkWidget *widget, gpointer user_data)
{
  ZMapWindowNavigator navigator = (ZMapWindowNavigator)user_data;
  GtkWidget *toplevel;

  /* request a redraw of the navigator... */
  zmapWindowNavigatorLocusRedraw(navigator);

  /* Get the toplevel to destroy */
  if((toplevel = zMapGUIFindTopLevel(widget)))
    {
      gtk_widget_destroy(toplevel);
    }

  return ;
}

static void zmapWindowNavigatorLocusFilterEditorCreate(ZMapWindowNavigator navigator)
{
  GtkWidget
    *toplevel = NULL,
    *vbox_1,
    *vbox_2,
    *frame,
    *label,
    *button_box,
    *cancel_button,
    *apply_button;

  toplevel = newTopLevel("ZMap - Filter Loci By Name");

  vbox_1   = gtk_vbox_new(FALSE, 0);

  gtk_container_add(GTK_CONTAINER(toplevel), vbox_1);

  frame    = gtk_frame_new("Filter");
  gtk_box_pack_start(GTK_BOX(vbox_1), frame, TRUE, TRUE, 5);

  vbox_2   = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), vbox_2);

  label    = gtk_label_new("This simple dialog controls the filtering\n"
                           "system used when drawing the Loci in the\n"
                           "navigator.");
  gtk_box_pack_start(GTK_BOX(vbox_2), label, FALSE, FALSE, 5);

  makeFilterPanel(navigator, vbox_2);

  label    = gtk_label_new("[Selected loci prefixes will not be shown\n"
                           "in the navigator.]");
  gtk_box_pack_start(GTK_BOX(vbox_2), label, FALSE, FALSE, 5);

  {
    GList *copied_list = NULL;

    copied_list = g_list_copy(navigator->hide_filter);
    /* This gets liberated in destroy */
    g_object_set_data(G_OBJECT(toplevel), FILTER_CANCEL_DATA_KEY, copied_list);

    g_signal_connect(G_OBJECT(toplevel), "destroy",
                     G_CALLBACK(destroyFilterCB), navigator);
  }

  button_box = gtk_hbutton_box_new();

  cancel_button = gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX(button_box), cancel_button, FALSE, FALSE, 5);
  g_signal_connect(G_OBJECT(cancel_button), "clicked",
                   G_CALLBACK(cancel_destroy_cb), navigator);

  apply_button  = gtk_button_new_with_label("Apply");
  gtk_box_pack_end(GTK_BOX(button_box), apply_button, FALSE, FALSE, 5);
  g_signal_connect(G_OBJECT(apply_button), "clicked",
                   G_CALLBACK(apply_destroy_cb), navigator);

  gtk_box_pack_end(GTK_BOX(vbox_1), button_box, FALSE, FALSE, 0);

  gtk_widget_show_all(toplevel);

  return ;
}




static gboolean searchLocusSetCB(ZMapFeatureAny feature_any, gpointer user_data)
{
  gboolean match = FALSE;
  GQuark locus_name = GPOINTER_TO_UINT(user_data);

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
//        ZMapFeature feature = (ZMapFeature)feature_any;

#ifdef THIS_LOCUS_STUFF_IS_A_PAIN
        /* quick fix to zmapWindowNavigatorShowSameNameList...  */
        if(locus_name == feature_any->original_id)
          match = TRUE;
#endif /* THIS_LOCUS_STUFF_IS_A_PAIN */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
        /* SHOULDN'T NEED THIS ANYMORE AS LOCUS DOES NOT HAVE A LOCUS_ID */

        /* Having just (locus_name == feature->locus_id) resulted in duplicates... */
        /* Added (feature->locus_id != feature->original_id) to fix RT # 64287 */
        if((locus_name == feature->feature.transcript.locus_id)
           && (feature->feature.transcript.locus_id != feature->original_id))
          match = TRUE ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#if MH17_CANNOT_BE
//locus is a separate featureset and is not a transcript feature??
        if (locus_name == feature->feature.transcript.locus_id)
          match = TRUE ;
#else
        if(locus_name == feature_any->original_id)
          match = TRUE;
#endif

        break;
      }

    default:
      break;
    }

  return match;
}

