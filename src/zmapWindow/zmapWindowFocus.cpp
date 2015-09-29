/*  File: zmapWindowFocus.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Code to implement "focus" items on the canvas.
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>
#include <ZMap/zmapUtilsFoo.hpp>
#include <ZMap/zmapUtils.hpp>
#include <zmapWindow_P.hpp>
#include <zmapWindowContainerUtils.hpp>
#include <zmapWindowContainerFeatureSet_I.hpp>



#if N_FOCUS_GROUPS > 8
#error array too short: FIX THIS, ref to WINDOW_FOCUS_GROUP_BITMASK
// (right now there are only 3)
#endif


/* Used for window focus items and column.
 *
 * Note that if only a column has been selected then focus_item_set may be NULL, BUT
 * if an item has been selected then focus_column will be that items parent.
 *
 * MH17: subverted to handle lists of evidence features from several columns
 * which are to be highlit on a semi-permanent basis
 * Normal focus operations require a global focus column and these are preserved as was
 *
 * evidence features (or other lists of features) may come from many columns.
 */
typedef struct _ZMapWindowFocusStruct
{
  ZMapWindow window;     // retrograde pointer, we need it for highlighting

  /* The selected/focused items as a list of ZMapWindowFocusItem,
   * interesting operations on these should be possible... */
  GList *focus_item_set ;


  ZMapWindowContainerFeatureSet focus_column ;
  FooCanvasItem *hot_item ;                                    // current hot focus item or NULL
  ZMapFeature hot_feature ;                                    // from the hot item to allow lookup

  int cache_id ;


} ZMapWindowFocusStruct ;




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
typedef struct ZMapWindowFocusItemStructType
{
  ZMapWindowContainerFeatureSet item_column ;   // column if on display: interacts w/ focus_column
  FooCanvasItem *item ;

  ZMapFeature feature ;
  /* for density items we have one foo and many features
   * and we need to store the feature
   */

  /* TRY REINSTATING SUBPART SELECTION.... */
  ZMapFeatureSubPartSpan sub_part ;



  int flags;                                    // a bitmap of focus types and misc data
  int display_state;
  // selected flags if any
  // all states need to be kept as more than one may be visible
  // as some use fill and some use border

  // if needed add this for data structs for any group
  //      void *data[N_FOCUS_GROUPS];

  // from the previous ZMapWindowFocusItemAreaStruct
  //  gboolean        highlighted;    // this was used but never unset
  //  gpointer        associated;     // this had set values preserved but never set

} ZMapWindowFocusItemStruct, *ZMapWindowFocusItem ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




typedef struct
{
  FooCanvasItem *result;
  ZMapFrame frame;
}MatchFrameStruct, *MatchFrame;


/* cache of global highlight colours ref'd by a window index
 * we normally expect only one window (or more with identical config)
 * but due to scope issues cannot find the colours without a window, which a canvas item does not have
 * howver they can have flags (an integer) and we can use that to provide a blank interface
 * these structs are held in a module global list, set by zMapWindowFocusCreate()
 * due to implemntation if we split a window then we get another one of thse with identical data
 * but it's too much grief to amend the window code just now
 */
typedef struct _zmapWindowFocusCacheStruct
{
  int id;        /* counts from 1 */
  ZMapWindow window;        /* boringly we need this to set global colours after the window is realised */

  gulong fill_pixel[N_FOCUS_GROUPS];                /* these are globally set colours */
  gboolean fill_set[N_FOCUS_GROUPS];
  gulong outline_pixel[N_FOCUS_GROUPS];
  gboolean outline_set[N_FOCUS_GROUPS];

  gboolean done_colours;

} zmapWindowFocusCacheStruct, *ZMapWindowFocusCache;


static void focusItemDestroy(ZMapWindowFocusItem list_item);
static ZMapWindowFocusItem addUnique(ZMapWindowFocus focus,
                                      FooCanvasItem *item, ZMapFeature feature, ZMapFeatureSubPart sub_part,
                                      ZMapWindowFocusType type);
static void freeFocusItems(ZMapWindowFocus focus, ZMapWindowFocusType type);
static void setFocusColumn(ZMapWindowFocus focus, FooCanvasGroup *column) ;
static void hideFocusItemsCB(gpointer data, gpointer user_data) ;
static void highlightCB(gpointer data, gpointer user_data) ;
static void highlightItem(ZMapWindow window, ZMapWindowFocusItem item, ZMapFeatureSubPart sub_part) ;



/*
 *                Globals
 */


// indexed by ZMapWindowFocusType enum
// item flags can be stored in higher bits - define here
int focus_group_mask[] = { 1,2,4,8,16,32,64,128,258 };

GList *focus_cache_G = NULL;

/* one for each window, we allow for up to 64k
 * and just pray that no-one makes that many windows
 * i'm not a gambling man but it looks like a good punt
 */
int focus_cache_id_G = 0;




/*
 *                External interface routines
 */

/*
 *              Set of routines to handle focus items.
 *    extended by mh17 to handle evidence features highlighting too.
 *
 * Holds a list of items that are highlighted/focussed, one of these is the "hot" item
 * which is the last one selected by the user.
 *
 * The hot item is in the list but is alos pointed at directly.
 *
 * The "add" functions just append item(s) to the list, they do not remove any items from it.
 *
 * The "hot" item must be set with a separate call.
 *
 * To replace the list you need to free it first then add new item(s).
 *
 * Items appear in the list once only and have flags to indicate which group(s)
 *
 * Note that these routines do not handle highlighting of items, that must be done with
 * a separate call.
 *
 * NOTE removal of previous focus items is done by
 * zmapWindowItem,c/zMapWindowUnHighlightFocusItems()
 * which is called via zmapWindowHighlightObject()
 * and not the SetHotItem function in this file which had code to do this that was never run
 */


char *zMapWindowGetHotColumnName(ZMapWindow window)
{
  char *column_name = NULL ;
  ZMapWindowContainerFeatureSet column ;

  if ((column = window->focus->focus_column))
    {
      column_name = zmapWindowContainerFeaturesetGetColumnName(column) ;
    }

  return column_name ;
}


gboolean zmapWindowFocusHasType(ZMapWindowFocus focus, ZMapWindowFocusType type)
{
  GList *l;
  ZMapWindowFocusItem fl;

  if(type == WINDOW_FOCUS_GROUP_FOCUSSED)
    return(focus->focus_item_set != NULL);

  for(l = focus->focus_item_set;l;l = l->next)
    {
      fl  = (ZMapWindowFocusItem) l->data;

      if(fl->flags & focus_group_mask[type])
        return(TRUE);
    }

  return(FALSE);
}


ZMapWindowFocus zmapWindowFocusCreate(ZMapWindow window)
{
  ZMapWindowFocus focus ;
  ZMapWindowFocusCache cache;

  focus = g_new0(ZMapWindowFocusStruct, 1) ;
  focus->window = window;

  cache = g_new0(zmapWindowFocusCacheStruct,1);
  if(cache)
    {
      cache->id = ++focus_cache_id_G;        /* zero means highlight not ever set; global window colours */

      focus_cache_G = g_list_prepend(focus_cache_G,cache);
      focus->cache_id = cache->id;
      cache->window = window;
    }

  return focus ;
}



/* return global highlight colours according to a feature's focus state, if set
 * also return whether the item is focussed/ selected
 * NOTE that this is used to get focus colours and also global colours such as EST masked
 * if the cache/window id is 0 just use the first one - allow lookup of global colours w/ no window
 */
int zMapWindowFocusCacheGetSelectedColours(int id_flags, gulong *fill_pix, gulong *outline)
{
  GList *l;
  ZMapWindowFocusCache cache;
  int focus_type;
  int i;
  int ret = 0;
  int cache_id = (id_flags >> 16);

  if(!(id_flags & (WINDOW_FOCUS_GROUP_BITMASK | WINDOW_FOCUS_ID)))        /* use normal colours */
    return 0;

  for(l = focus_cache_G; l; l = l->next)
    {
      cache = (ZMapWindowFocusCache) l->data;
      if(cache->id == cache_id || !cache_id)
        {

          if(!cache->done_colours)
            {
              /* set global selected/ highlight colours if configured */
              /* window must be realised first so can only do this when displaying */
              ZMapWindow window = cache->window;
              GdkColor *gdk;
              gulong pixel;

              if(window->highlights_set.item)
                {
                  gdk = &(window->colour_item_highlight);
                  pixel = zMap_gdk_color_to_rgba(gdk);
                  cache->fill_pixel[WINDOW_FOCUS_GROUP_FOCUS] = foo_canvas_get_color_pixel(window->canvas, pixel);
                  cache->fill_set[WINDOW_FOCUS_GROUP_FOCUS] = TRUE;
                }

              if(window->highlights_set.evidence)
                {
                  gdk = &(window->colour_evidence_fill);
                  pixel = zMap_gdk_color_to_rgba(gdk);
                  cache->fill_pixel[WINDOW_FOCUS_GROUP_EVIDENCE] = foo_canvas_get_color_pixel(window->canvas, pixel);
                  cache->fill_set[WINDOW_FOCUS_GROUP_EVIDENCE] = TRUE;

                  gdk = &(window->colour_evidence_border);
                  pixel = zMap_gdk_color_to_rgba(gdk);
                  cache->outline_pixel[WINDOW_FOCUS_GROUP_EVIDENCE] = foo_canvas_get_color_pixel(window->canvas, pixel);
                  cache->outline_set[WINDOW_FOCUS_GROUP_EVIDENCE] = TRUE;
                }

              if(window->highlights_set.masked)
                {
                  gdk = &(window->colour_masked_feature_fill);
                  pixel = zMap_gdk_color_to_rgba(gdk);
                  cache->fill_pixel[WINDOW_FOCUS_GROUP_MASKED] = foo_canvas_get_color_pixel(window->canvas, pixel);
                  cache->fill_set[WINDOW_FOCUS_GROUP_MASKED] = TRUE;

                  gdk = &(window->colour_masked_feature_border);
                  pixel = zMap_gdk_color_to_rgba(gdk);
                  cache->outline_pixel[WINDOW_FOCUS_GROUP_MASKED] = foo_canvas_get_color_pixel(window->canvas, pixel);
                  cache->outline_set[WINDOW_FOCUS_GROUP_MASKED] = TRUE;
                }

              if(window->highlights_set.filtered)
                {
                  gdk = &(window->colour_filtered_column);
                  pixel = zMap_gdk_color_to_rgba(gdk);
                  cache->fill_pixel[WINDOW_FOCUS_GROUP_FILTERED] = foo_canvas_get_color_pixel(window->canvas, pixel);
                  cache->fill_set[WINDOW_FOCUS_GROUP_FILTERED] = TRUE;
                }

              cache->done_colours = TRUE;
            }

          focus_type = id_flags & WINDOW_FOCUS_GROUP_BITMASK;

          for(i = 0; focus_type; i++, focus_type >>= 1)
            {
              if(focus_type & 1)
                {
                  if(cache->fill_set[i])
                    {
                      if(fill_pix)
                        *fill_pix = cache->fill_pixel[i];
                      ret |= WINDOW_FOCUS_CACHE_FILL;
                    }
                  if(cache->outline_set[i])
                    {
                      if(outline)
                        *outline = cache->outline_pixel[i];
                      ret |= WINDOW_FOCUS_CACHE_OUTLINE;
                    }

                  if(i < N_FOCUS_GROUPS_FOCUS)
                    ret |= WINDOW_FOCUS_CACHE_SELECTED;

                  return ret;
                }
            }
        }
    }

  return 0;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* a debug thing: print out the foo canvas parent string from the focussed item,
 * plus also the foo canvas current item
 * we suspect a race condition on clear canvas and item destroy via queued callbacks
 * Oh the joy of object oriented programming: hand over control and hope for the best */
void zMapWindowFocusDump(char *str)
{
  /* we assume one window and one hot item */
  ZMapWindowFocusCache cache;
  ZMapWindow window;
  FooCanvasItem *hot;
  static FooCanvas * canvas = NULL;

  if(canvas)
    {
      hot = canvas->current_item;

      printf("%s focus dump current item %p %p %p\n",str,hot,canvas->focused_item,canvas->grabbed_item);
      while(hot)
        {
          ZMapWindowCanvasItem z;
          ZMapWindowContainerGroup g;
          char * name;
          int level;

          name = "not ZMWCI";  zMapWindowFocusDump("set focus");
          level = 0;
          if(ZMAP_IS_CANVAS_ITEM(hot))
            {
              z = (ZMapWindowCanvasItem) hot;
              if(!z->feature)
                name = "canvas item";
              else
                name = g_quark_to_string(z->feature->unique_id);
            }
          else if (ZMAP_IS_CONTAINER_GROUP(hot))
            {
              g = (ZMapWindowContainerGroup) hot;
              if(!g->feature_any)
                name = "group";
              else
                name = g_quark_to_string(g->feature_any->unique_id);
              level = (int) g->level;

            }
          printf("  foo item = %p -> %p %s level %d\n",hot,hot->parent,name,level);
          hot = hot->parent;
        }
    }

  if(!focus_cache_G)
    {
      printf("%s: no focus cache\n",str);
      return;
    }

  cache = (ZMapWindowFocusCache) focus_cache_G->data;
  window = cache->window;
  if(!window || !window->focus || !window->focus->hot_item)
    {
      printf("%s: no focus item\n",str);
      return;
    }
  hot = window->focus->hot_item;
  canvas = hot->canvas;
  printf("%s focus dump item\n",str);
  while(hot)
    {
      ZMapWindowCanvasItem z;
      ZMapWindowContainerGroup g;
      char * name;
      int level;

      name = "not ZMWCI";
      if(ZMAP_IS_CANVAS_ITEM(hot))
        {
          z = (ZMapWindowCanvasItem) hot;
          if(!z->feature)
            name = "canvas item";
          else
            name = g_quark_to_string(z->feature->unique_id);
          level = 0;
        }
      else if (ZMAP_IS_CONTAINER_GROUP(hot))
        {
          g = (ZMapWindowContainerGroup) hot;
          if(!g->feature_any)
            name = "group";
          else
            name = g_quark_to_string(g->feature_any->unique_id);
          level = (int) g->level;

        }
      printf("  foo item = %p -> %p %s level %d\n",hot,hot->parent,name,level);
      hot = hot->parent;
    }
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/*
 *               Package routines
 */


/*   General focus routines   */

void zmapWindowFocusResetType(ZMapWindowFocus focus, ZMapWindowFocusType type)
{
  freeFocusItems(focus,type) ;

  return ;
}


void zmapWindowFocusReset(ZMapWindowFocus focus)
{
  zmapWindowFocusResetType(focus,WINDOW_FOCUS_GROUP_FOCUS) ;

  zmapWindowFocusUnHighlightHotColumn(focus);
  focus->focus_column = NULL ;

  /* I think/hope this is the correct place to do this.... */
  if ((focus->hot_item))
    {
      zMapWindowCanvasFeaturesetUnsetPointFeature(focus->hot_item) ;

      focus->hot_item = NULL ;
      focus->hot_feature = NULL ;
    }

  return ;
}



void zmapWindowFocusDestroy(ZMapWindowFocus focus)
{
  GList *l;

  if (!focus)
    return ;

  freeFocusItems(focus, (ZMapWindowFocusType)WINDOW_FOCUS_GROUP_FOCUSSED) ;

  for(l = focus_cache_G;l;l = l->next)
    {
      ZMapWindowFocusCache cache;

      cache = (ZMapWindowFocusCache) l->data;
      if(cache->id == focus->cache_id)
          {
          focus_cache_G = g_list_delete_link(focus_cache_G,l);
          break;
          }
    }

  focus->focus_column = NULL ;

  g_free(focus) ;

  return ;
}



/*    Focus item routines     */


/* N.B. unless there are no items, then item is added to end of list,
 * it is _not_ the new hot item so we do not reset the focus column for instance. */
void zmapWindowFocusAddItemType(ZMapWindowFocus focus, FooCanvasItem *item,
                                ZMapFeature feature, ZMapFeatureSubPart sub_part, ZMapWindowFocusType type)
{
  if (item)  // in case we add GetHotItem() which could return NULL
    {
      addUnique(focus, item, feature, sub_part, type) ;

      if (!focus->hot_item && type == WINDOW_FOCUS_GROUP_FOCUS)
        zmapWindowFocusSetHotItem(focus, item, feature) ;
    }

  return ;
}

/* Same remark applies to this routine as to zmapWindowFocusAddItem() */
void zmapWindowFocusAddItemsType(ZMapWindowFocus focus, GList *glist, FooCanvasItem *hot, ZMapWindowFocusType type)
{
  gboolean first;
  FooCanvasItem *foo;
  ZMapFeature hotfeature = NULL;

  first = glist && glist->data;        /* ie is there one? */

  for (; glist ; glist = glist->next)
    {
      ID2Canvas id2c;

      id2c = (ID2Canvas) glist->data;
      foo = FOO_CANVAS_ITEM(id2c->item);
      if(hot == foo)
              hotfeature = (ZMapFeature) id2c->feature_any;
      addUnique(focus, foo, (ZMapFeature)id2c->feature_any, NULL, type) ;
    }

  if (hot && !focus->hot_item && first && type == WINDOW_FOCUS_GROUP_FOCUS)
    zmapWindowFocusSetHotItem(focus, hot,hotfeature) ;


  return ;
}


/* Is the given item in the hot focus column ? */
/* SHOULD BE RENAMED TO BE _ANY_ ITEM AS IT CAN INCLUDE BACKGROUND ITEMS.... */
gboolean zmapWindowFocusIsItemInHotColumn(ZMapWindowFocus focus, FooCanvasItem *item)
{
  gboolean result = FALSE ;

  if (focus->focus_column)
    {
      ZMapWindowContainerGroup column_container;

      column_container = (ZMapWindowContainerGroup)focus->focus_column;

      if (column_container == zmapWindowContainerCanvasItemGetContainer(item))
        result = TRUE ;
    }

  return result ;
}


gboolean zmapWindowFocusIsItemFocused(ZMapWindowFocus focus, FooCanvasItem *item)
{
  gboolean result = FALSE ;

  if ((focus && focus->focus_item_set))
    {
      GList *set_item = focus->focus_item_set ;

      for ( ; set_item ; set_item = set_item->next)
        {
          ZMapWindowFocusItem focus_item = (ZMapWindowFocusItem)(set_item->data) ;

          if (focus_item->item == item)
            {
              result = TRUE ;
              break ;
            }
        }
    }

  return result ;
}




// set the hot item, which is already in the focus set
// NB: this is only called from this file
// previously this function removed existing focus but only if was never set
void zmapWindowFocusSetHotItem(ZMapWindowFocus focus, FooCanvasItem *item, ZMapFeature feature)
{
  FooCanvasGroup *column ;

  /* FEELS DANGEROUS THAT ITEM AND FEATURE ARE SET SEPARATELY ?? */

  focus->hot_item = item;

  /* mh17 NOTE this code assumes that composite foo canvas items (eg GraphDensity)
   * have called set_feature() before we get here
   */
  focus->hot_feature = feature ;

  /* Set the focus item's column as the focus column. */
  column = (FooCanvasGroup *) zmapWindowContainerCanvasItemGetContainer(item) ;

  setFocusColumn(focus, column) ;                            /* N.B. May sort features. */

  return ;
}


FooCanvasItem *zmapWindowFocusGetHotItem(ZMapWindowFocus focus)
{
  FooCanvasItem *item = NULL ;

  if ((focus->hot_item))
    {
      /* for composite canvas items */
      /* NOTE there's a theory that iten may be Foo but not Zmap eg when clicking on a trancript's exon */
      /* it's quite difficult to tell how true this is, need to trawl thro'
       * zmapWindowUpdateInfoPanel() and up/downstream functions all; of which meander somewhat
       */
      if ((focus->hot_item) && ZMAP_IS_WINDOW_FEATURESET_ITEM(focus->hot_item))
        {
          zMapWindowCanvasItemSetFeaturePointer((ZMapWindowCanvasItem)focus->hot_item, focus->hot_feature) ;

          item = focus->hot_item ;
        }
    }

  return item ;
}


/* check the usage of that and replace with what the func returning the actual focus
 * items if possible. */
GList *zmapWindowFocusGetFocusItemsType(ZMapWindowFocus focus, ZMapWindowFocusType type)
{
  GList *lists = NULL, *items = NULL;
  ZMapWindowFocusItem flist = NULL;
  int mask = focus_group_mask[type];
  ID2Canvas id2c;

  if (focus->focus_item_set)
    {
      lists = g_list_first(focus->focus_item_set) ;
      while(lists)
        {
          flist = (ZMapWindowFocusItem)(lists->data);
          if((flist->flags & mask))
            {
              /* for compatability with FToI search functions we must return this struct, NB no hash table set */
              id2c = g_new0(ID2CanvasStruct,1);
              id2c->item = flist->item;
              id2c->feature_any = (ZMapFeatureAny) flist->feature;

              items = g_list_append(items, id2c);
            }
          lists = lists->next;
        }
    }

  return items ;
}

/* Returns a list of items that are focussed.
 *
 * NOTE: the caller should free the returned list with g_list_free(item_list),
 * the items belong to focus and should not be free'd. */
GList *zmapWindowFocusGetFocusItems(ZMapWindowFocus focus)
{
  GList *focus_items = NULL ;


  if (focus->focus_item_set)
    {
      GList *curr_item ;

      curr_item = g_list_first(focus->focus_item_set) ;
      while (curr_item)
        {
          ZMapWindowFocusItem focus_item = (ZMapWindowFocusItem)(curr_item->data) ;

          if (focus_item->flags & WINDOW_FOCUS_GROUP_FOCUSSED)
            focus_items = g_list_append(focus_items, focus_item) ;

          curr_item = g_list_next(curr_item) ;
        }
    }

  return focus_items ;
}


/* Return the focused items as a GList of ZMapFeature's */
GList *zmapWindowFocusGetFeatureList(ZMapWindowFocus focus)
{
  GList *result = NULL ;

  if (focus && focus->focus_item_set)
    {
      GList *set_item = focus->focus_item_set ;

      for ( ; set_item ; set_item = set_item->next)
        {
          ZMapWindowFocusItem focus_item = (ZMapWindowFocusItem)(set_item->data) ;

          if (focus_item->feature)
            result = g_list_append(result, focus_item->feature) ;
        }
    }

  return result ;
}


// Return the focused items as a GList of ZMapFeature's, optionally only return those that are
// within the mark (if set, otherwise ignored).
gboolean zmapWindowFocusGetFeatureListFull(ZMapWindow window, gboolean in_mark,
                                           FooCanvasGroup **focus_column_out,
                                           FooCanvasItem **focus_item_out,
                                           GList **filterfeatures_out,
                                           char **err_msg)
{
  gboolean result = FALSE ;
  FooCanvasGroup *focus_column ;
  FooCanvasItem *focus_item ;
  GList *filterfeatures = NULL ;
  int mark_start = 0, mark_end = 0 ;

  if (!((focus_column = zmapWindowFocusGetHotColumn(window->focus))
        && (focus_item = zmapWindowFocusGetHotItem(window->focus))
        && (filterfeatures = zmapWindowFocusGetFeatureList(window->focus))))
    {
      /* We can't do anything if there is no highlight feature. */
      *err_msg = g_strdup_printf("%s", "No features selected.") ;
    }
  else
    {
      /* If the mark is set then exclude any features not overlapping it, this may
         accidentally result in there being no features for filtering. */
      if (in_mark && zmapWindowMarkIsSet(window->mark) && zmapWindowMarkGetSequenceRange(window->mark,
                                                                                         &mark_start, &mark_end))
        {
          filterfeatures = zMapFeatureGetOverlapFeatures(filterfeatures,
                                                         mark_start, mark_end,
                                                         ZMAPFEATURE_OVERLAP_ALL) ;
        }

      if (!filterfeatures)
        {
          *err_msg = g_strdup_printf("%s", "Current mark excludes all selected filter features.") ;
        }
      else
        {
          *focus_column_out = focus_column ;
          *focus_item_out = focus_item ;
          *filterfeatures_out = filterfeatures ;

          result = TRUE ;
        }
    }

  return result ;
}


/* Call given user function for all highlighted items.
 * The data passed to the function will be a ZMapWindowFocusItem
 *
 * void callback(gpointer list_item_data, gpointer user_data);
 *
 */
void zmapWindowFocusForEachFocusItemType(ZMapWindowFocus focus, ZMapWindowFocusType type,
                                         GFunc callback, gpointer user_data)
{
  GList *l;
  ZMapWindowFocusItem list_item;

  for(l = focus->focus_item_set;l;l = l->next)
    {
      list_item = (ZMapWindowFocusItem) l->data;
      if(type == WINDOW_FOCUS_GROUP_FOCUSSED || (list_item->flags & focus_group_mask[type]))
        callback((gpointer) list_item,user_data);
    }

  return ;
}


void zmapWindowFocusHighlightFocusItems(ZMapWindowFocus focus, ZMapWindow window)
{
  zmapWindowFocusForEachFocusItemType(focus, WINDOW_FOCUS_GROUP_FOCUS, highlightCB, window) ;
}


void zmapWindowFocusUnhighlightFocusItems(ZMapWindowFocus focus, ZMapWindow window)
{
  // this unhighlights on free
  freeFocusItems(focus,WINDOW_FOCUS_GROUP_FOCUS);
}




/* Remove single item, a side effect is that if this is the hot item
 * then we find the first one in the list
 * so no point in replacing the item in its original position
 * esp w/ shift select being possible, previous code only handled one item
 *
 * NOTE need to replace the item back where it was to handle feature tabbing
 * this means we need to handle many items moving around not just one focus item
 */

/* MH17: this may be called from a destroy object callback in which case the canvas is not in a safe state
 * the assumption in this code if that the focus is removed from an object while it still exists
 * but alignments appear to call this when thier parent has dissappeared causing a crash
 * which is a bug in the canvas item code not here
 * The fix is to add another option to say 'don't highlight it because it's not there'
 */

/* NOTE this is only called from zmapWindowFeature.c/canvasItemDestroyCB()
   and also some temporary test code in zmapWindow.c/keyboardEvent(GDK_E)
*/

void zmapWindowFocusRemoveFocusItemType(ZMapWindowFocus focus,
                                        FooCanvasItem *item, ZMapWindowFocusType type, gboolean unhighlight)
{
  ZMapWindowFocusItem gonner,data;
  GList *remove,*l;

#if MH17_CHASING_FOCUS_CRASH
  zMapLogWarning("focus item removed","");
#endif

  /* always try to remove, if item is not in list then list is unchanged. */
  if (focus->focus_item_set && item)
    {
      for (remove = focus->focus_item_set;remove; remove = remove->next)
        {
          gonner = (ZMapWindowFocusItem) remove->data;
          if (gonner->item == item)
            {

              if(type == WINDOW_FOCUS_GROUP_FOCUSSED)
                gonner->flags &= ~WINDOW_FOCUS_GROUP_FOCUSSED;        // zap the lot
              else
                gonner->flags &= ~focus_group_mask[type];        // remove from this group

              if (unhighlight)
                highlightItem(focus->window, gonner, NULL) ;

              if(!(gonner->flags & WINDOW_FOCUS_GROUP_FOCUSSED))   // no groups: remove from list
                {
                  focusItemDestroy(gonner);
                  focus->focus_item_set = g_list_delete_link(focus->focus_item_set,remove);
                  /* although we are removing the link that gets processed by the for loop
                   * we will break out of it so this is safe
                   */
                }
              break;

            }
        }

      if(item == focus->hot_item && type == WINDOW_FOCUS_GROUP_FOCUS)
        {
          focus->hot_item = NULL;
          for(l = focus->focus_item_set;l;l = l->next)
            {
              data = (ZMapWindowFocusItem) l->data;
              if((data->flags & focus_group_mask[WINDOW_FOCUS_GROUP_FOCUS]))
                {
                  focus->hot_item = data->item;
                  break;
                }
            }
        }
    }

  return ;
}

void zmapWindowFocusHideFocusItems(ZMapWindowFocus focus, GList **hidden_items)
{
  /* NOTE these itens remain in the focus list as normal but are not visible
   * arguably it would be better to handle the hidden items lists in this module
   * but it's done in zmapWindow.c due to history
   * for a review of this see notepad.html
   * NB this is not critical as when features are hidden and others selected others the focus will be cleared anyway
   */

  zmapWindowFocusForEachFocusItemType(focus,WINDOW_FOCUS_GROUP_FOCUS, hideFocusItemsCB, hidden_items) ;

  return ;
}




/*   Focus Column routines   */


/* this one is different, a new column can be set _without_ setting a new item, so we
 * need to get rid of the old items. */
void zmapWindowFocusSetHotColumn(ZMapWindowFocus focus, FooCanvasGroup *column, FooCanvasItem *item)
{
  freeFocusItems(focus,WINDOW_FOCUS_GROUP_FOCUS) ;

  focus->hot_item = NULL;
  setFocusColumn(focus, column) ;

  return ;
}

FooCanvasGroup *zmapWindowFocusGetHotColumn(ZMapWindowFocus focus)
{
  FooCanvasGroup *column = NULL ;

  column =(FooCanvasGroup *)( focus->focus_column );

  return column ;
}

/* highlight/unhiglight cols. */
void zmapWindowFocusHighlightHotColumn(ZMapWindowFocus focus)
{
  ZMapWindowContainerGroup hot_column = (ZMapWindowContainerGroup) zmapWindowFocusGetHotColumn(focus);

  if(hot_column)
    {
      GdkColor *colour = NULL;
      GQuark column_id ;
      ZMapFeatureColumn column ;

      /* Check if there's a selection colour set in the column style */
      column_id = zmapWindowContainerFeaturesetGetColumnUniqueId(ZMAP_CONTAINER_FEATURESET(hot_column)) ;

      column = zMapWindowGetColumn(focus->window->context_map,column_id) ;

      if(hot_column && column->style_id)
        {
          ZMapFeatureTypeStyle s ;

          s = (ZMapFeatureTypeStyle)g_hash_table_lookup(focus->window->context_map->styles,GUINT_TO_POINTER(column->style_id));

          if(s)
            zMapStyleGetColours(s, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_SELECTED, &colour, NULL, NULL);
        }

      if (!colour)
        {
          colour = &focus->window->colour_column_highlight;
        }

      zmapWindowDrawSetGroupBackground(focus->window, hot_column, 0, 1, 1.0, ZMAP_CANVAS_LAYER_COL_BACKGROUND, colour, NULL);
      foo_canvas_item_request_redraw((FooCanvasItem *) hot_column);
    }
}


void zmapWindowFocusUnHighlightHotColumn(ZMapWindowFocus focus)
{
  /* NOTE these items remain in the focus list as normal but are not visible
   * arguably it would be better to handle the hidden items lists in this module
   * but it's done in zmapWindow.c due to history
   * for a review of this see notepad.html
   * NB this is not critical as when features are hidden and others selected others the focus will be cleared anyway
   */

  /* I DONT' UNDERSTAND THIS MERGE........ */

  /* THIS IS WHAT'S IN DEVEL */
  ZMapWindowContainerGroup column = (ZMapWindowContainerGroup) zmapWindowFocusGetHotColumn(focus);


  if (column)
    {
      GdkColor *fill_col = column->background_fill;

      if(column->flags.filtered)
        zMapWindowGetFilteredColour(focus->window,&fill_col);

      zmapWindowDrawSetGroupBackground(focus->window, column, 0, 1, 1.0, ZMAP_CANVAS_LAYER_COL_BACKGROUND, fill_col, NULL);
      foo_canvas_item_request_redraw((FooCanvasItem *) column);
    }

  /* THIS IS WHAT i USED TO HAVE.... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapWindowFocusForEachFocusItemType(focus, WINDOW_FOCUS_GROUP_FOCUS, hideFocusItemsCB, hidden_items) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

}











/*
 *                           Internal routines.
 */




static void focusItemDestroy(ZMapWindowFocusItem list_item)
{
  list_item->item = NULL;

  g_free(list_item);

  return ;
}



/* GFunc() to hide the given item and record it in the user hidden list. */
static void hideFocusItemsCB(gpointer data, gpointer user_data)
{
  ZMapWindowFocusItem list_item = (ZMapWindowFocusItem)data;
  FooCanvasItem *item = (FooCanvasItem *)list_item->item ;
  GList **list_ptr = (GList **)user_data ;
  GList *user_hidden_items = *list_ptr ;
  ID2Canvas id2c;

  if (!FOO_IS_CANVAS_ITEM(item))
    return ;

  id2c = g_new0(ID2CanvasStruct,1);
  id2c->item = list_item->item;
  id2c->feature_any = (ZMapFeatureAny) list_item->feature;
  /* hash table is NULL, that is correct */

  if(ZMAP_IS_WINDOW_FEATURESET_ITEM(item))
    {
      zMapWindowCanvasItemSetFeaturePointer((ZMapWindowCanvasItem) item, (ZMapFeature) list_item->feature);
      zMapWindowCanvasItemShowHide((ZMapWindowCanvasItem) item, FALSE);
    }
  else
    {
      foo_canvas_item_hide(item) ;
    }

  user_hidden_items = g_list_append(user_hidden_items, id2c) ;

  *list_ptr = user_hidden_items ;

  return ;
}


/* Do the right thing with groups and items
 * also does unhighlight and is called on free
 */
static void highlightItem(ZMapWindow window, ZMapWindowFocusItem item, ZMapFeatureSubPart sub_part)
{
  GdkColor *fill_col = NULL, *border_col = NULL;
  int n_focus;
  GList *l;


  if ((item->flags & WINDOW_FOCUS_GROUP_FOCUSSED) == item->display_state)
    return;

  if ((item->flags & WINDOW_FOCUS_GROUP_FOCUSSED))
    {
      if((item->flags & focus_group_mask[WINDOW_FOCUS_GROUP_FOCUS]))
        {
          if(window->highlights_set.item)
            fill_col = &(window->colour_item_highlight);

        }
      else if((item->flags & focus_group_mask[WINDOW_FOCUS_GROUP_EVIDENCE]))
        {
          if(window->highlights_set.evidence)
            {
              fill_col = &(window->colour_evidence_fill);
              border_col = &(window->colour_evidence_border);
            }
        }

      zMapWindowCanvasItemSetIntervalColours(item->item, item->feature,
                                             sub_part, ZMAPSTYLE_COLOURTYPE_SELECTED, item->flags, fill_col, border_col) ;
    }
  else
    {
      zMapWindowCanvasItemSetIntervalColours(item->item, item->feature, NULL, ZMAPSTYLE_COLOURTYPE_NORMAL, 0, NULL,NULL);

      /* this is a pain: to keep ordering stable we have to put the focus item back where it was
       * so we have to compare it with items not in the focus list
       */
      /* find out how many focus items there are in this item's column */
      for(n_focus = 0,l = window->focus->focus_item_set;l;l = l->next)
        {
          ZMapWindowFocusItem focus_item = (ZMapWindowFocusItem) l->data;

          if(item->item_column == focus_item->item_column)      /* count includes self */
            n_focus++;
        }

    }

  item->display_state = item->flags & WINDOW_FOCUS_GROUP_FOCUSSED;

  return ;
}


// handle highlighting for all groups
// maintain current state to avoid repeat highlighting
static void highlightCB(gpointer list_data, gpointer user_data)
{
  ZMapWindowFocusItem data = (ZMapWindowFocusItem)list_data;
  ZMapWindow window = (ZMapWindow)user_data ;

  GdkColor *highlight = NULL;

  highlightItem(window, data, NULL) ;

  if(window->highlights_set.item)
    highlight = &(window->colour_item_highlight);

  return ;
}


// add a struct to the list, no duplicates
// return the list and a pointer to the struct
static ZMapWindowFocusItem addUnique(ZMapWindowFocus focus,
                                     FooCanvasItem *item, ZMapFeature feature, ZMapFeatureSubPart sub_part,
                                     ZMapWindowFocusType type)
{
  GList *gl;
  ZMapWindowFocusItem list_item = NULL;

  for (gl = focus->focus_item_set ; gl ; gl = gl->next)
    {
      list_item = (ZMapWindowFocusItem) gl->data ;

      if (list_item->item == item && list_item->feature == feature
          && ((sub_part && list_item->sub_part.start)
              && sub_part->start == list_item->sub_part.start && sub_part->end == list_item->sub_part.end))
        break ;
    }

  if (!gl)     // didn't find it
    {
      list_item = g_new0(ZMapWindowFocusItemStruct,1) ;
      focus->focus_item_set = g_list_prepend(focus->focus_item_set,list_item) ;
    }

  if(!feature)
    feature = zMapWindowCanvasItemGetFeature(item) ;

  list_item->flags |= focus_group_mask[type] | (focus->cache_id << 16) ;
  list_item->item = item ;
  list_item->feature = feature ;
  if (sub_part)
    list_item->sub_part = *sub_part ;

  /* tricky -> a container is a foo canvas group
   * that has a fixed size list of foo canvas groups
   * that contain lists of features
   * that all have the same parent
   */
  list_item->item_column = (ZMapWindowContainerFeatureSet) FOO_CANVAS_ITEM(item)->parent ;
  // also need to fill in featureset

  highlightItem(focus->window, list_item, sub_part) ;

  return list_item ;
}





static void freeFocusItems(ZMapWindowFocus focus, ZMapWindowFocusType type)
{
  GList *l ;

  for (l = focus->focus_item_set ; l ; )
    {
      ZMapWindowFocusItem focus_item ;
      GList *del ;

      focus_item = (ZMapWindowFocusItem)(l->data) ;
      del = l ;
      l = l->next ;

      if(type == WINDOW_FOCUS_GROUP_FOCUSSED)
        focus_item->flags &= ~WINDOW_FOCUS_GROUP_FOCUSSED ;
      else
        focus_item->flags &= ~focus_group_mask[type] ;

      highlightItem(focus->window, focus_item, NULL) ;

      if (!(focus_item->flags & WINDOW_FOCUS_GROUP_FOCUSSED))
        {
          if (focus->hot_item == focus_item->item)
            focus->hot_item = NULL ;

          focusItemDestroy(focus_item) ;

          focus->focus_item_set = g_list_delete_link(focus->focus_item_set, del) ;

          l = focus->focus_item_set ;                            /* shouldn't this be reset !!!! */
        }
    }

  return ;
}





static void setFocusColumn(ZMapWindowFocus focus, FooCanvasGroup *column)
{
  ZMapWindowContainerFeatureSet container;

  if(ZMAP_IS_CONTAINER_FEATURESET(column))
    {
      if(focus->focus_column)
        zmapWindowFocusUnHighlightHotColumn(focus);

      focus->focus_column = container = (ZMapWindowContainerFeatureSet)column ;

      zmapWindowFocusHighlightHotColumn(focus);
    }

  return ;
}



