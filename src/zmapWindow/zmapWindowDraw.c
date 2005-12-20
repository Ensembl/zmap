/*  File: zmapWindowDraw.c
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
 * Description: General drawing functions for zmap window, e.g.
 *              repositioning columns after one has been bumped
 *              or removed etc.
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Dec 20 13:37 2005 (edgrif)
 * Created: Thu Sep  8 10:34:49 2005 (edgrif)
 * CVS info:   $Id: zmapWindowDraw.c,v 1.10 2005-12-20 15:32:36 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>



/* Used for holding state while bumping columns. */
typedef struct
{
  double offset ;
  double incr ;
  ZMapStyleOverlapMode bump_mode ;
  GHashTable *pos_hash ;
} BumpColStruct, *BumpCol ;


typedef struct
{
  double y1, y2 ;
  double offset ;
} BumpColRangeStruct, *BumpColRange ;


typedef struct
{
  BumpColRange curr ;
  BumpColRange new ;
} TempStruct, *Temp ;




/* Container package stuff...container things need to go in a separate file VERY SOON... */


/* Used to identify unambiguously which part of a zmapWindowContainer group a particular
 * foo canvas group or item represents. */
typedef enum {CONTAINER_INVALID,
	      CONTAINER_ROOT, CONTAINER_PARENT,
	      CONTAINER_FEATURES, CONTAINER_BACKGROUND,
              CONTAINER_TEXT} ContainerType ;


/* because roy can't spell container_type more than once */
#define CONTAINER_TYPE_KEY        "container_type"


/* these must go in the end... */

#define CONTAINER_REDRAW_CALLBACK "container_callback"
#define CONTAINER_REDRAW_DATA     "container_data"



/* I think most of the above will be replaced by this... */
#define CONTAINER_DATA "container_data"

typedef struct
{
  ZMapContainerLevelType level ;

  /* A optimisation because we don't want to attach this information to every single feature.... */
  gboolean child_redraw_required ;

  /* I'd like to add a border here too..... */
  double child_spacing ;

} ContainerDataStruct, *ContainerData ;



typedef void (*ZMapContainerCallback)(FooCanvasGroup *parent, gpointer user_data);



typedef struct
{
  ZMapWindow window ;
  double zoom ;

} ZoomDataStruct, *ZoomData ;




typedef struct execOnChildrenStruct_
{
  ZMapContainerLevelType stop ;

  GFunc                  down_func_cb ;
  gpointer               down_func_data ;

  GFunc                  up_func_cb ;
  gpointer               up_func_data ;

} execOnChildrenStruct, *execOnChildren ;


static void bumpColCB(gpointer data, gpointer user_data) ;
static void valueDestroyCB(gpointer data) ;
static gboolean compareOverlapCB(gpointer key, gpointer value, gpointer user_data) ;

static void tempCompareOverlapCB(gpointer key, gpointer value, gpointer user_data) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void doCol(gpointer data, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void itemDestroyCB(gpointer data, gpointer user_data);

static void repositionGroups(FooCanvasGroup *changed_group, double group_spacing) ;

static gint horizPosCompare(gconstpointer a, gconstpointer b) ;


static void printlevel(gpointer data, gpointer user_data);
static void printItem(FooCanvasItem *item, int indent, char *prefix) ;

/* Trying this out. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void defaultZoomChangedCB(FooCanvasGroup *container, double new_zoom, gpointer user_data);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void addToList(gpointer data, gpointer user_data);


static void reAlignGroups(FooCanvasGroup *container, gpointer user_data) ;


static void executeoncontainer(FooCanvasGroup        *parent, 
			       ZMapContainerLevelType stop_at_type,
			       GFunc                  down_func_cb,
			       gpointer               down_func_data,
			       GFunc                  up_func_cb,
			       gpointer               up_func_data) ;
static void eachContainer(gpointer data, gpointer user_data) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void eachContainerChild(gpointer data, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void zmapWindowContainerZoomChanged(gpointer data, gpointer user_data);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void preZoomCB(gpointer data, gpointer user_data) ;
static void postZoomCB(gpointer data, gpointer user_data) ;
static void columnZoomChanged(FooCanvasGroup *container, double new_zoom, ZMapWindow window) ;




/* Creates a "container" for our sequence features which consists of: 
 * 
 *                parent_group
 *                  /   |  \
 *                 /        \
 *                /     |    \
 *           background    sub_group of
 *             item     |  feature items
 *
 *                      |
 *                Optional text
 *                    group (dotted line)

 * should change so parent - group of all sub_group features - sub_group feature items (including Optional text group!)

 *
 * The background item is used both as a visible background for the items but also
 * more importantly to catch events _anywhere_ in the space (e.g. column) where
 * the items might be drawn.
 * This arrangement also means that code that has to process all items can do so
 * simply by processing all members of the item list of the sub_group as they
 * are guaranteed to all be feature items and it is trivial to perform such operations
 * as taking the size of all items in a group.
 * 
 * Our canvas is guaranteed to have a tree hierachy of these structures from a column
 * up to the ultimate alignment parent. If the parent is not a container_features
 * group then we make a container root, i.e. the top of the container tree.
 * 
 * Returns the container_parent.
 * 
 */
FooCanvasGroup *zmapWindowContainerCreate(FooCanvasGroup *parent,
					  ZMapContainerLevelType level,
					  double child_spacing,
					  GdkColor *background_fill_colour,
					  GdkColor *background_border_colour)
{
  FooCanvasGroup *container_parent = NULL, *container_features ;
  FooCanvasItem *container_background ;
  ContainerType parent_type = CONTAINER_INVALID, container_type ;
  ContainerData container_data ;

  /* Parent has to be a group. */
  zMapAssert(FOO_IS_CANVAS_GROUP(parent)) ;

  parent_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(parent_type == CONTAINER_INVALID || parent_type == CONTAINER_FEATURES) ;

  /* Is the parent a container itself, if not then make this the root container. */
  if (parent_type == CONTAINER_FEATURES)
    container_type = CONTAINER_PARENT ;
  else
    container_type = CONTAINER_ROOT ;


  container_parent = FOO_CANVAS_GROUP(foo_canvas_item_new(parent,
							  foo_canvas_group_get_type(),
							  NULL)) ;

  container_data = g_new0(ContainerDataStruct, 1) ;
  g_object_set_data(G_OBJECT(container_parent), CONTAINER_DATA, container_data) ;

  container_data->level = level ;

  container_data->child_spacing = child_spacing ;

  /* Default is that we always redraw the next level down. */
  container_data->child_redraw_required = TRUE ;

  g_object_set_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY,
		    GINT_TO_POINTER(container_type)) ;


  container_features = FOO_CANVAS_GROUP(foo_canvas_item_new(container_parent,
							    foo_canvas_group_get_type(),
							    NULL)) ;


  g_object_set_data(G_OBJECT(container_features), CONTAINER_TYPE_KEY,
		    GINT_TO_POINTER(CONTAINER_FEATURES)) ;

  /* We don't use the border colour at the moment but we may wish to later.... */
  container_background = zMapDrawSolidBox(FOO_CANVAS_ITEM(container_parent),
					  0.0, 0.0, 0.0, 0.0, background_fill_colour) ;
  g_object_set_data(G_OBJECT(container_background), CONTAINER_TYPE_KEY,
		    GINT_TO_POINTER(CONTAINER_BACKGROUND)) ;

 /* Note that we rely on the background being first in parent groups item list which we
  * we achieve with this lower command. This is both so that the background appears _behind_
  * the objects that are children of the subgroup and so that we can return the background
  * and subgroup items correctly. */
  foo_canvas_item_lower_to_bottom(container_background) ; 


  return container_parent ;
}




void zmapWindowContainerSetChildRedrawRequired(FooCanvasGroup *container_parent,
					       gboolean redraw_required)
{

  ContainerType type = CONTAINER_INVALID ;
  ContainerData container_data ;

  zMapAssert(container_parent) ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT) ;

  container_data = g_object_get_data(G_OBJECT(container_parent), CONTAINER_DATA) ;

  container_data->child_redraw_required = redraw_required ;

  return ;
}



/* THIS MUST BECOME PART OF THE GENERAL FEATURES STUFF..... */
/* In general this part does not exist..., but when it does
 *  - allows addition of text to any column.
 *  - text can be bumped left or right of the features with ease.
 *  - text may be ellipsed???? later!
 *  - auto size column????
 */
FooCanvasGroup *zmapWindowContainerAddTextChild(FooCanvasGroup *container_parent, 
                                                zmapWindowContainerZoomChangedCallback redrawCB,
                                                gpointer user_data)
{
  FooCanvasGroup *text_child = NULL;
  FooCanvasGroup *has_text_child = NULL;

  if((has_text_child = zmapWindowContainerGetText(container_parent)) == NULL)
    {
      text_child = FOO_CANVAS_GROUP(foo_canvas_item_new(container_parent,
                                                        foo_canvas_group_get_type(),
                                                        NULL)) ;
      g_object_set_data(G_OBJECT(text_child), CONTAINER_TYPE_KEY,
                        GINT_TO_POINTER(CONTAINER_TEXT)) ;
      if(redrawCB)
        {
          g_object_set_data(G_OBJECT(text_child), CONTAINER_REDRAW_CALLBACK,
                            redrawCB);
          g_object_set_data(G_OBJECT(text_child), CONTAINER_REDRAW_DATA,
                            user_data);
        }
      foo_canvas_item_raise_to_top(FOO_CANVAS_ITEM(text_child));
    }
  
  return text_child;
}




/* Given either child of the container, return the container_parent. */
FooCanvasGroup *zmapWindowContainerGetParent(FooCanvasItem *unknown_child)
{
  FooCanvasGroup *container_parent = NULL ;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(unknown_child), CONTAINER_TYPE_KEY)) ;

  switch(type)
    {
    case CONTAINER_FEATURES:
    case CONTAINER_BACKGROUND:
    case CONTAINER_TEXT:
      container_parent = FOO_CANVAS_GROUP(unknown_child->parent) ;
      break ;
    default:
      zMapAssert("bad coding, unrecognised container type.") ;
    }

  return container_parent ;
}



/* Given a container parent, return the container parent above that, if the container
 * is the root then the container itself is returned. */
FooCanvasGroup *zmapWindowContainerGetSuperGroup(FooCanvasGroup *container_parent)
{
  FooCanvasGroup *super_container = NULL ;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  if (type == CONTAINER_ROOT)
    super_container = container_parent ;
  else
    super_container = FOO_CANVAS_GROUP(FOO_CANVAS_ITEM(container_parent)->parent->parent) ;

  return super_container ;
}


/* Return the sub group of the container that contains all of the "features" where
 * features might be columns, column sets, blocks etc. */
FooCanvasGroup *zmapWindowContainerGetFeatures(FooCanvasGroup *container_parent)
{
  FooCanvasGroup *container_features ;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  container_features = FOO_CANVAS_GROUP((g_list_nth(container_parent->item_list, 1))->data) ;

  return container_features ;
}

/* Return the background of the container which is used both to give colour to the
 * container and to allow event interception across the complete bounding rectangle
 * of the container. */
FooCanvasItem *zmapWindowContainerGetBackground(FooCanvasGroup *container_parent)
{
  FooCanvasItem *container_background ;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  container_background = FOO_CANVAS_ITEM((g_list_nth(container_parent->item_list, 0))->data) ;
  zMapAssert(FOO_IS_CANVAS_RE(container_background)) ;

  return container_background ;
}


/* Get the style for this columns features. AGH, problem here...bumping will need to cope with
 * several different types of features.... */
ZMapFeatureTypeStyle zmapWindowContainerGetStyle(FooCanvasGroup *column_group)
{
  ZMapFeatureTypeStyle style = NULL ;
  FooCanvasGroup *column_features ;
  GList *list_item ;
  FooCanvasItem *feature_item ;
  ZMapFeature feature ;

  /* Crikey.... */
  column_features = zmapWindowContainerGetFeatures(column_group) ;
  list_item = g_list_first(column_features->item_list) ;
  feature_item = (FooCanvasItem *)list_item->data ;
  feature = (ZMapFeature)(g_object_get_data(G_OBJECT(feature_item), "item_feature_data")) ;
  style = feature->style ;

  return style ;
}


/* Return the text of the container.  This may return NULL, caller MUST check this! */
FooCanvasGroup *zmapWindowContainerGetText(FooCanvasGroup *container_parent)
{
  FooCanvasGroup *container_text = NULL;
  GList *nth = NULL;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  if((nth = g_list_nth(container_parent->item_list, 2)) != NULL)
    container_text = FOO_CANVAS_GROUP(nth->data) ;

  return container_text ;
}


/* Setting the background size is not completely straight forward because we always
 * want the height of the background to be the full height of the group (e.g. column)
 * but we want the width to be set from the horizontal extent of the features.
 *
 * The width/height of the background can be given, if either is 0.0 then the width/height
 * is taken from the child group.
 */

/* THIS NEEDS TO DO A BORDER WIDTH AS WELL.... */

void zmapWindowContainerSetBackgroundSize(FooCanvasGroup *container_parent,
					  double height)
{
  FooCanvasGroup *container_features ;
  FooCanvasGroup *container_text  = NULL;
  FooCanvasItem *container_background ;
  double x1, y1, x2, y2 ;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  zMapAssert(height >= 0.0) ;

  container_features   = zmapWindowContainerGetFeatures(container_parent) ;
  container_text       = zmapWindowContainerGetText(container_parent) ;  
  container_background = zmapWindowContainerGetBackground(container_parent) ;


  /* Either the caller sets the height or we get the height from the main group.
   * We do this because features may cover only part of the range of the group, e.g. transcripts
   * in a column, and we need the background to cover the whole range to allow us to get
   * mouse clicks etc. */
  if (height != 0.0)
    {
      y1 = 0 ;
      y2 = height ;
    }
  else
    {
      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(container_parent), NULL, &y1, NULL, &y2) ;
      zmapWindowExt2Zero(&y1, &y2) ;
    }


  /* Get the width from the child group */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(container_features), &x1, NULL, &x2, NULL) ;

  /* And if there's a text group, that too. */
  if(container_text != NULL)
    {
      double tx1, tx2;
      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(container_text), &tx1, NULL, &tx2, NULL) ;
      if(tx1 < x1)
        x1 = tx1;
      if(tx2 > x2)
        x2 = tx2;
    }

  foo_canvas_item_set(container_background,
                      "x1", x1,
                      "y1", y1,
                      "x2", x2,
                      "y2", y2,
                      NULL) ;

  return ;
}


/* Much like zmapWindowContainerSetBackgroundSize() but sets the background as big as the
 * parent in width _and_ height. */
void zmapWindowContainerMaximiseBackground(FooCanvasGroup *container_parent)
{
  FooCanvasItem *container_background ;
  double x1, y1, x2, y2 ;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  container_background = zmapWindowContainerGetBackground(container_parent) ;

  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(container_parent), &x1, &y1, &x2, &y2) ;

  zmapWindowExt2Zero(&x1, &x2) ;
  zmapWindowExt2Zero(&y1, &y2) ;

  foo_canvas_item_set(container_background,
		      "x1", x1,
		      "y1", y1,
		      "x2", x2,
		      "y2", y2,
		      NULL) ;

  return ;
}


/* Does the container contain any child features ? */
gboolean zmapWindowContainerHasFeatures(FooCanvasGroup *container_parent)
{
  gboolean result = FALSE ;
  FooCanvasGroup *container_features ;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  container_features = zmapWindowContainerGetFeatures(container_parent) ;

  if (container_features->item_list)
    result = TRUE ;

  return result ;
}

/* Does the container contain any child text ? */
gboolean zmapWindowContainerHasText(FooCanvasGroup *container_parent)
{
  gboolean result = FALSE ;
  FooCanvasGroup *container_text = NULL;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  if(((container_text = zmapWindowContainerGetText(container_parent)) != NULL) 
     && container_text->item_list)
    result = TRUE ;

  return result ;
}

void zmapWindowContainerPrint(FooCanvasGroup *container_parent)
{
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  executeoncontainer(container_parent,
		     ZMAPCONTAINER_LEVEL_FEATURESET,
		     printlevel, NULL,
		     NULL, NULL) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  doprint(container_parent, indent) ;
#endif

  return ;
}

/* WARNING, this is a DUMB destroy, it just literally destroys all the canvas items starting
 * with the container_parent so if you haven't registered destroy callbacks then you may be in
 * trouble with dangling/inaccessible data. */
void zmapWindowContainerDestroy(FooCanvasGroup *container_parent)
{
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  /* The FooCanvas seems to use GTK_OBJECTs but I don't know if this will automatically destroy
   * all children, I think it must do. */
  gtk_object_destroy(GTK_OBJECT(container_parent)) ;

  return ;
}

void zmapWindowContainerPurge(FooCanvasGroup *unknown_child)
{
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(unknown_child), CONTAINER_TYPE_KEY)) ;

  switch(type)
    {
    case CONTAINER_FEATURES:
    case CONTAINER_TEXT:
      g_list_foreach(unknown_child->item_list, itemDestroyCB, NULL);
      break ;
    default:
      zMapAssert("bad coding, unrecognised container type.") ;
    }
  
  return ;
}

/* Sorts the children of a group by horizontal position, sorts all children that
 * are groups and leaves children that are items untouched, this is because item children
 * are background items that do not need to be ordered.
 */
void zmapWindowCanvasGroupChildSort(FooCanvasGroup *group_inout)
{

  zMapAssert(FOO_IS_CANVAS_GROUP(group_inout)) ;

  group_inout->item_list = g_list_sort(group_inout->item_list, horizPosCompare) ;

  return ;
}








/* Set the hidden status of a column group, currently this depends on the mag setting in the
 * style for the column but we could allow the user to switch this on and off as well. */
void zmapWindowColumnSetMagState(ZMapWindow window,
				 FooCanvasGroup *col_group, ZMapFeatureTypeStyle style)
{

  zMapAssert(window && FOO_IS_CANVAS_GROUP(col_group) && style) ;

  if (style->min_mag || style->max_mag)
    {
      double curr_zoom ;

      curr_zoom = zMapWindowGetZoomMagnification(window) ;

      if ((style->min_mag && curr_zoom < style->min_mag)
	  || (style->max_mag && curr_zoom > style->max_mag))
	foo_canvas_item_hide(FOO_CANVAS_ITEM(col_group)) ;
      else
	foo_canvas_item_show(FOO_CANVAS_ITEM(col_group)) ;
    }

  return ;
}


/* NOTE: this function bumps an individual column but it DOES NOT move any other columns,
 * to do that you need to use zmapWindowColumnReposition(). The split is made because
 * we don't always want to reposition all the columns following a bump, e.g. when we
 * are creating a zmap window. */
void zmapWindowColumnBump(FooCanvasGroup *column_group, ZMapStyleOverlapMode bump_mode)
{
  BumpColStruct bump_data = {0.0} ;
  FooCanvasGroup *column_features ;
  ZMapFeatureTypeStyle style ;

  column_features = zmapWindowContainerGetFeatures(column_group) ;

  style = zmapWindowContainerGetStyle(column_group) ;

  bump_data.bump_mode = bump_mode ;
  bump_data.incr = style->width + BUMP_SPACING ;

  switch (bump_mode)
    {
    case ZMAPOVERLAP_COMPLETE:
      bump_data.incr = 0.0 ;
      break ;
    case ZMAPOVERLAP_SIMPLE:
      break ;
    case ZMAPOVERLAP_POSITION:
      bump_data.pos_hash = g_hash_table_new_full(NULL, NULL, /* NULL => use direct hash */
						 NULL, valueDestroyCB) ;
      break ;
      case ZMAPOVERLAP_OVERLAP:
      bump_data.pos_hash = g_hash_table_new_full(NULL, NULL, /* NULL => use direct hash */
						 NULL, valueDestroyCB) ;
      break ;
    case ZMAPOVERLAP_NAME:
      bump_data.pos_hash = g_hash_table_new_full(NULL, NULL, /* NULL => use direct hash */
						 NULL, valueDestroyCB) ;
      break ;
    default:
      zMapAssert("Coding error, unrecognised ZMapWindowItemFeatureType") ;
      break ;
    }

  /* bump all the features. */
  g_list_foreach(column_features->item_list, bumpColCB, (gpointer)&bump_data) ;

  if (bump_mode == ZMAPOVERLAP_POSITION || bump_mode == ZMAPOVERLAP_OVERLAP
      || bump_mode == ZMAPOVERLAP_NAME)
    g_hash_table_destroy(bump_data.pos_hash) ;

  /* Make the parent groups bounding box as large as the group.... */
  zmapWindowContainerSetBackgroundSize(column_group, 0.0) ;

  return ;
}




/* this needs to be more generalised....as it will be used from a number of places... */
static void reAlignGroups(FooCanvasGroup *container, gpointer user_data)
{
  gpointer cb_data = NULL ;
  ContainerType type = CONTAINER_INVALID ;
  ZMapFeatureAny feature ;
  double spacing = 0.0 ;


  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container), CONTAINER_TYPE_KEY)) ;
  
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Need to set feature spacing here..... */
  switch(feature->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      spacing = 0.0 ;
      break ;

    case ZMAPFEATURE_STRUCT_ALIGN:
      spacing = 0.0 ;
      break ;


    case ZMAPFEATURE_STRUCT_BLOCK:
      spacing = 0.0 ;
      break ;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      spacing = COLUMN_SPACING ;
      break ;

      /* Not sure we should even get here..... */
    case ZMAPFEATURE_STRUCT_FEATURE:
      spacing = 0.0 ;
      break ;

    default:
      zMapAssert(!"bad coding, unrecognised container type.") ;
      break ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  switch(type)
    {
    case CONTAINER_ROOT:
    case CONTAINER_PARENT:    
      {
        FooCanvasGroup *text = NULL;
        if((text = zmapWindowContainerGetText(container)))
          zmapWindowContainerRegionChanged(text, NULL);
      }
      break;
    case CONTAINER_TEXT:
      if(callback)
        (callback)(container, 0.0, cb_data);
      break;
    case CONTAINER_FEATURES:
    case CONTAINER_BACKGROUND:
      break;
    default:
      zMapAssert(0 && "bad coding, unrecognised container type.") ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  repositionGroups(container, spacing) ;


  return ;
}






/* We assume that the columns have been added sequentially in the order they are displayed in,
 * if this changes then this function will need to be rewritten to look at the position/size
 * of the columns to decide whether to reposition them or not. */
void zmapWindowColumnReposition(FooCanvasGroup *column)
{
  double x1, y1, x2, y2, bound_x1, bound_x2 ;
  FooCanvasGroup *column_set, *block, *align, *root ;


  column_set = zmapWindowContainerGetSuperGroup(column) ;

  block = zmapWindowContainerGetSuperGroup(column_set) ;	/* We don't use this as we don't
								   need to actually move the blocks. */
  align = zmapWindowContainerGetSuperGroup(block) ;

  root = zmapWindowContainerGetSuperGroup(align) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapWindowContainerPrint(root) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  /* Move all the columns in the current group that need to be moved. */
  repositionGroups(column, COLUMN_SPACING) ;

  /* Move all the column groups in the containing block that need to be moved. */
  repositionGroups(column_set, STRAND_SPACING) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Move all the block groups in the canvas that need to be moved. */
  repositionGroups(block, ALIGN_SPACING) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  {
    /* Actually blocks have to be looked at differently, we should be looking to see
     * the maximum block size and then changing the background to fit that.... */

    FooCanvasGroup *parent_container ;

    parent_container = zmapWindowContainerGetSuperGroup(block) ;
    zmapWindowContainerSetBackgroundSize(parent_container, 0.0) ;
  }

  /* Move all the alignment groups in the canvas that need to be moved. */
  repositionGroups(align, ALIGN_SPACING) ;

  /* Reset scroll region */
  foo_canvas_get_scroll_region(column->item.canvas, &x1, &y1, &x2, &y2) ;

  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(foo_canvas_root(column->item.canvas)),
			     &bound_x1, NULL, &bound_x2, NULL) ;

  /* I think its a mistake to reset the x1 as we have the scale bar there, but when that 
   * goes we will need to move the x1 as well.... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  foo_canvas_set_scroll_region(column->item.canvas, bound_x1, y1, bound_x2, y2) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  foo_canvas_set_scroll_region(column->item.canvas, x1, y1, bound_x2, y2) ;


  return ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* GFunc to call on potentially all container groups.
 * This is still a little creased, get out the ironing board... 
 * Issues include:
 *  - Being unsure on whether parents should call this on children
 *  - small problem on timing, but that might need a check in setbackgroundsize.
 *  - Can parents and children (text esp.) be sorted by moving to sub features style organisation?
 */
static void zmapWindowContainerZoomChanged(gpointer data, 
                                           gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data;
  double          new_zoom  = 0.0;
  gpointer        cb_data   = NULL;
  ContainerType   type      = CONTAINER_INVALID ;
  zmapWindowContainerZoomChangedCallback callback = NULL;

  new_zoom = *((double *)user_data);

  callback = g_object_get_data(G_OBJECT(container), CONTAINER_REDRAW_CALLBACK);
  cb_data  = g_object_get_data(G_OBJECT(container), CONTAINER_REDRAW_DATA);

  type     = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container), 
                                               CONTAINER_TYPE_KEY));
  
  switch(type)
    {
      /* Is this convoluted?  Could we get round this, by just having
       * callbacks on features and text? I think this depends on where 
       * the style is going to sit.  If it sits on the parent it can
       * easily apply to both the feature, bckgrnd & text.  If it sits
       * on the feature then, maybe it's only a feature thing.  Will
       * we need an extra style for the text? Is this reasonable anyway? 
       * Where does the text get it's width from is it just guessed/
       * automatic?
       */
    case CONTAINER_ROOT:
    case CONTAINER_PARENT:    
      if(callback)
        (callback)(container, new_zoom, cb_data);
      /* Issue with whether we have a text child.... */
      /* Do we have a text child? Yes -> zmapWindowContainerZoomChangedCB(text_child, new_zoom, ...); */
      {
        FooCanvasGroup *text = NULL;
        if((text = zmapWindowContainerGetText(container)))
          zmapWindowContainerZoomChanged(text, &new_zoom);
      }
      /* We assume that the child will have changed it's size, so sort
       * out the background. No need for children to do this! */
      /* MUST watch out for the resize to bigger than 32K!!!!  THIS
       * function MUST be called BEFORE zmapWindowScrollRegionTool so
       * that the backgrounds get cropped by longitemcrop */
      /* However some columns need to know what the scroll region is
       * so zmapWindowScrollRegionTool MUST be called FIRST!
       * Arrrrgh. */
      break;
    case CONTAINER_TEXT:
    case CONTAINER_FEATURES:
    case CONTAINER_BACKGROUND:
      if(callback)
        (callback)(container, new_zoom, cb_data);
      break;
    default:
      zMapAssert(0 && "bad coding, unrecognised container type.") ;
    }

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



static void zmapWindowContainerRegionChanged(gpointer data, 
                                             gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data;
  gpointer        cb_data   = NULL;
  ContainerType   type      = CONTAINER_INVALID ;
  zmapWindowContainerZoomChangedCallback callback = NULL;

  callback = g_object_get_data(G_OBJECT(container), CONTAINER_REDRAW_CALLBACK);
  cb_data  = g_object_get_data(G_OBJECT(container), CONTAINER_REDRAW_DATA);

  type     = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container), 
                                               CONTAINER_TYPE_KEY));
  
  switch(type)
    {
    case CONTAINER_ROOT:
    case CONTAINER_PARENT:    
      {
        FooCanvasGroup *text = NULL;
        if((text = zmapWindowContainerGetText(container)))
          zmapWindowContainerRegionChanged(text, NULL);
      }
      break;
    case CONTAINER_TEXT:
      if(callback)
        (callback)(container, 0.0, cb_data);
      break;
    case CONTAINER_FEATURES:
    case CONTAINER_BACKGROUND:
      break;
    default:
      zMapAssert(0 && "bad coding, unrecognised container type.") ;
    }

  return ;
}


void zmapWindowContainerGetAllColumns(FooCanvasGroup *super_root, GList **list)
{
  ContainerType type = CONTAINER_INVALID ;
  
  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(super_root), CONTAINER_TYPE_KEY)) ;

  executeoncontainer(super_root,
		     ZMAPCONTAINER_LEVEL_STRAND,
		     addToList,
		     list,
		     NULL, NULL) ;
  
  return ;
}

void zmapWindowContainerMoveEvent(FooCanvasGroup *super_root, ZMapWindow window)
{
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(super_root), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type = CONTAINER_ROOT);

  executeoncontainer(FOO_CANVAS_GROUP(super_root),
		     ZMAPCONTAINER_LEVEL_FEATURESET,
		     zmapWindowContainerRegionChanged, NULL,
		     NULL, NULL) ;
  return ;
}






/* Makes sure all the things that need to be redrawn for zooming get redrawn. */
void zmapWindowDrawZoom(ZMapWindow window)
{
  FooCanvasGroup *super_root ;
  ContainerType type = CONTAINER_INVALID ;
  ZoomDataStruct zoom_data = {NULL} ;

  super_root = FOO_CANVAS_GROUP(zmapWindowFToIFindItemFull(window->context_to_item, 0,0,0,0,0)) ;
  zMapAssert(super_root) ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(super_root), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type = CONTAINER_ROOT) ;

  zoom_data.window = window ;
  zoom_data.zoom = zMapWindowGetZoomFactor(window) ;

  executeoncontainer(FOO_CANVAS_GROUP(super_root),
		     ZMAPCONTAINER_LEVEL_FEATURESET,
		     preZoomCB,
		     &zoom_data,
		     postZoomCB,
		     &zoom_data) ;

  return ;
}






/* For James all methods that mention score have:
 *
 * Score_by_width	
 * Score_bounds	 70.000000 130.000000
 *
 * (the bounds are different for different data sources...)
 *
 * x1 will be used and reset, x2 is not looked at but just reset...
 * 
 * 
 */
double zmapWindowGetPosFromScore(ZMapFeatureTypeStyle style, double score,
				 double *curr_x1_inout, double *curr_x2_out)
{
  double width ;
  double dx ;
  double numerator, denominator ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zMapAssert(!(!style->max_score && !style->min_score)) ;

  numerator = score - style->min_score ;
  denominator = style->max_score - style->min_score ;

  dx = 0.25 + ((0.75 * numerator) / denominator) ;

  if (dx < 0.25)
    dx = 0.25 ;
  else if (dx > 1)
    dx = 1 ;

  *curr_x_inout1 = *curr_x1 + (0.5 * style->width * (xoff - dx)) ;
  *curr_x2_inout = *curr_x1 + (0.5 * style->width * (xoff + dx)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return width ;
}





/* 
 *                Internal routines.
 */




/*! g_list_foreach function to remove items */
static void itemDestroyCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;

  gtk_object_destroy(GTK_OBJECT(item));

  return ;
}



/* THIS SHOULD BE MERGED SOMEHOW WITH THE REALIGN FUNCTION....ITS ALL THE SAME REALLY.... */
/* The changed group is found in its parents list of items and then all items to the right
 * of the changed group are repositioned with respect to the changed group.
 * 
 * NOTE that we assume that the child items of the group_parent occur in the list
 * in their positional order. */
static void repositionGroups(FooCanvasGroup *changed_group, double group_spacing)
{
  double x1, y1, x2, y2 ;
  FooCanvasGroup *parent_container, *all_groups ;
  GList *curr_child ;
  double curr_bound ;

  parent_container = zmapWindowContainerGetSuperGroup(changed_group) ;

  all_groups = zmapWindowContainerGetFeatures(parent_container) ;

  /* Get the position of the changed group within its parent. */
  x1 = y1 = x2 = y2 = 0.0 ; 
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(changed_group), &x1, &y1, &x2, &y2) ;
  curr_bound = x2 + group_spacing ;


  /* Now move all the groups to the right of this one over so they are positioned properly,
   * N.B. this might mean moving them in either direction depending on whether the changed
   * group got bigger or smaller. */
  curr_child = g_list_find(all_groups->item_list, changed_group) ;
  while ((curr_child = g_list_next(curr_child)))	    /* n.b. start with next group to the right. */
    {
      FooCanvasGroup *curr_group = FOO_CANVAS_GROUP(curr_child->data) ;
      double dx = 0.0 ;

      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(curr_group), &x1, &y1, &x2, &y2) ;

      dx = curr_bound - x1 ;				    /* can be +ve or -ve */

      if (dx != 0.0)
	{
	  curr_bound = x2 + dx + group_spacing ;

	  foo_canvas_item_move(FOO_CANVAS_ITEM(curr_group), dx, 0.0) ;
							    /* N.B. we only shift in x, not in y. */
	}
    }


  /* Make the parent groups bounding box as large as the group.... */
  zmapWindowContainerSetBackgroundSize(parent_container, 0.0) ;


  return ;
}



static void bumpColCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  BumpCol bump_data   = (BumpCol)user_data ;
  ZMapWindowItemFeatureType item_feature_type ;
  double y1 = 0.0, y2 = 0.0 ;
  gpointer y1_ptr = 0 ;
  gpointer key = NULL, value = NULL ;
  double offset = 0.0 ;
  BumpColRange range ;


  if(!(zmapWindowItemIsShown(item)))
    return ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "item_feature_type")) ;

  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;

  switch (bump_data->bump_mode)
    {
    case ZMAPOVERLAP_POSITION:
      {
	foo_canvas_item_get_bounds(item, NULL, &y1, NULL, NULL) ;

	y1_ptr = GINT_TO_POINTER((int)y1) ;

	if (g_hash_table_lookup_extended(bump_data->pos_hash, y1_ptr, &key, &value))
	  {
	    offset = *(double *)value + bump_data->incr ;
	    
	    *(double *)value = offset ;
	  }
	else
	  {
	    value = g_new(double, 1) ;
	    *(double *)value = offset ;
	    
	    g_hash_table_insert(bump_data->pos_hash, y1_ptr, value) ;
	  }
	
	break ;
      }
    case ZMAPOVERLAP_OVERLAP:
      {
	BumpColRangeStruct new_range ;
	BumpColRange curr_range ;

	/* TEMP CODE UNTIL 2.6 */
#warning "temp 2.2 code...agh"
	TempStruct callback_data = {NULL} ;
	

	foo_canvas_item_get_bounds(item, NULL, &y1, NULL, &y2) ;

	new_range.y1 = y1 ;
	new_range.y2 = y2 ;
	new_range.offset = 0.0 ;


	callback_data.new = &new_range ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	/* THIS FUNCTION IS NOT AVAILABLE IN GLIB 2.2 leaving me to go quietly spare.... */

	/* if ((curr_range = g_hash_table_find(bump_data->pos_hash, compareOverlap, &new_range))) */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	g_hash_table_foreach(bump_data->pos_hash, tempCompareOverlapCB, &callback_data) ;
	if ((curr_range = callback_data.curr))

	  {
	    if (new_range.y1 < curr_range->y1)
	      curr_range->y1 = new_range.y1 ;

	    if (new_range.y2 > curr_range->y2)
	      curr_range->y2 = new_range.y2 ;

	    curr_range->offset += bump_data->incr ;
	  }
	else
	  {
	    curr_range = g_memdup(&new_range, sizeof(BumpColRangeStruct)) ;

	    g_hash_table_insert(bump_data->pos_hash, curr_range, curr_range) ;
	  }

	offset = curr_range->offset ;
	
	break ;
      }
    case ZMAPOVERLAP_NAME:
      {
	ZMapFeature feature ;

	feature = (ZMapFeature)(g_object_get_data(G_OBJECT(item), "item_feature_data")) ;

	if (g_hash_table_lookup_extended(bump_data->pos_hash,
					 GINT_TO_POINTER(feature->original_id), &key, &value))
	  {
	    offset = *(double *)value ;
	  }
	else
	  {
	    offset = bump_data->offset ;

	    value = g_new(double, 1) ;
	    *(double *)value = offset ;
	    
	    g_hash_table_insert(bump_data->pos_hash, GINT_TO_POINTER(feature->original_id), value) ;

	    bump_data->offset += bump_data->incr ;
	  }
	
	break ;
      }
    case ZMAPOVERLAP_COMPLETE:
    case ZMAPOVERLAP_SIMPLE:
      {
	offset = bump_data->offset ;
	bump_data->offset += bump_data->incr ;

	break ;
      }
    default:
      zMapAssert(0 && "Coding error, unrecognised bump type.") ;
      break ;
    }

  my_foo_canvas_item_goto(item, &(offset), NULL) ; 

  return ;
}


/* GDestroyNotify() function for freeing the data part of a hash association. */
static void valueDestroyCB(gpointer data)
{
  g_free(data) ;

  return ;
}


/* GHRFunc callback func, called from g_hash_table_find() to test whether current
 * element matches supplied overlap coords. */
static gboolean compareOverlapCB(gpointer key, gpointer value, gpointer user_data)
{
  gboolean result = TRUE ;
  BumpColRange curr_range = (BumpColRange)value, new_range = (BumpColRange)user_data ;

  /* Easier to test no overlap. */
  if (new_range->y1 > curr_range->y2 || new_range->y2 < curr_range->y1)
    result = FALSE ;

  return result ;
}


/* this is temporary code until we move completely to glib 2.6, it does the same as
 * compareOverlapCB but badly..., it can be deleted when we move to 2.6... */
static void tempCompareOverlapCB(gpointer key, gpointer value, gpointer user_data)
{
  BumpColRange curr_range = (BumpColRange)value ;
  Temp callback_data = (Temp)user_data ;

  if (!callback_data->curr)
    {
      if (callback_data->new->y1 > curr_range->y2 || callback_data->new->y2 < curr_range->y1)
	callback_data->curr = NULL ;
      else
	callback_data->curr = curr_range ;
    }

  return ;
}



/* MAY NEED TO INGNORE BACKGROUND BOXES....WHICH WILL BE ITEMS.... */

/* A GCompareFunc() called from g_list_sort(), compares groups on their horizontal position. */
static gint horizPosCompare(gconstpointer a, gconstpointer b)
{
  gint result = 0 ;
  FooCanvasGroup *group_a = (FooCanvasGroup *)a, *group_b = (FooCanvasGroup *)b ;

  if (!FOO_IS_CANVAS_GROUP(group_a) || !FOO_IS_CANVAS_GROUP(group_b))
    {
      result = 0 ;
    }
  else
    {
      if (group_a->xpos < group_b->xpos)
	result = -1 ;
      else if (group_a->xpos > group_b->xpos)
	result = 1 ;
      else
	result = 0 ;
    }

  return result ;
}

static void printItem(FooCanvasItem *item, int indent, char *prefix)
{
  int i ;

  for (i = 0 ; i < indent ; i++)
    {
      printf("  ") ;
    }
  printf("%s", prefix) ;

  zmapWindowPrintItemCoords(item) ;

  return ;
}



static void addToList(gpointer data, gpointer user_data)
{
  FooCanvasGroup *grp = FOO_CANVAS_GROUP(data);
  GList **list = (GList **)user_data;
  *list = g_list_append(*list, grp);
  return ;
}




/* unified way to descend and do things to ALL and every or just some */
/* Something I'm just trying out! */
static void executeoncontainer(FooCanvasGroup        *parent, 
			       ZMapContainerLevelType stop_at_type,
			       GFunc                  down_func_cb,
			       gpointer               down_func_data,
			       GFunc                  up_func_cb,
			       gpointer               up_func_data)
{
  execOnChildrenStruct exe  = {0,NULL};
  ContainerType        type = CONTAINER_INVALID ;
  GList               *list = NULL ;

  zMapAssert(stop_at_type >= ZMAPCONTAINER_LEVEL_ROOT
	     || stop_at_type <= ZMAPCONTAINER_LEVEL_FEATURESET) ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_ROOT || type == CONTAINER_PARENT);

  exe.stop                   = stop_at_type;

  exe.down_func_cb         = down_func_cb;
  exe.down_func_data       = down_func_data;
  exe.up_func_cb         = up_func_cb;
  exe.up_func_data       = up_func_data;

  eachContainer((gpointer)parent, &exe) ;

  return ;
}


/* Called for every container while descending.... */
static void eachContainer(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;
  execOnChildren all_data = (execOnChildren)user_data ;
  ContainerData container_data ;

  /* Execute pre-recursion function. */
  if (all_data->down_func_cb)
    (all_data->down_func_cb)(container, all_data->down_func_data);


  /* Recurse ? */
  container_data = g_object_get_data(G_OBJECT(container), CONTAINER_DATA) ;
  if (all_data->stop != container_data->level)
    {
      FooCanvasGroup *sub_parents ;
      
      if ((sub_parents = zmapWindowContainerGetFeatures(container)))
	{
	  g_list_foreach(sub_parents->item_list, eachContainer, user_data) ;
	}
    }


  /* Execute post-recursion function. */
  if (all_data->up_func_cb)
    (all_data->up_func_cb)(container, all_data->up_func_data) ;

  return ;
}



static void printlevel(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container_parent = (FooCanvasGroup *)data;
  ContainerType type = CONTAINER_INVALID ;
  ZMapFeatureAny any_feature ;
  char *feature_type = NULL ;
  int i, indent ;

  any_feature = (ZMapFeatureAny)(g_object_get_data(G_OBJECT(container_parent), "item_feature_data")) ;

  if (any_feature)
    {
      switch (any_feature->struct_type)
	{
	case ZMAPFEATURE_STRUCT_CONTEXT:
	  feature_type = "Context" ;
          indent = 0;
	  break ;
	case ZMAPFEATURE_STRUCT_ALIGN:
	  feature_type = "Align" ;
          indent = 1;
	  break ;
	case ZMAPFEATURE_STRUCT_BLOCK:
	  feature_type = "Block" ;
          indent = 2;
	  break ;
	case ZMAPFEATURE_STRUCT_FEATURESET:
	  feature_type = "Set" ;
          indent = 4;
	  break ;
	case ZMAPFEATURE_STRUCT_FEATURE:
	  feature_type = "Feature" ;
          indent = 5;
	  break ;
        default:
          break;
	}
    }
  else
    {
      feature_type = "Colgroup" ;
      indent = 3;
    }

  for (i = 0 ; i < indent ; i++){ printf("  "); }

  printf("%s:\n", feature_type) ;

  printItem(FOO_CANVAS_ITEM(container_parent),
	    indent, "       parent:  ") ;
  
  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  if(!(type == CONTAINER_PARENT || type == CONTAINER_ROOT))
    return ;

  printItem(FOO_CANVAS_ITEM(zmapWindowContainerGetFeatures(container_parent)),
	    indent, "     features:  ") ;
  
  if(zmapWindowContainerGetText(container_parent))
    printItem(FOO_CANVAS_ITEM(zmapWindowContainerGetText(container_parent)),
              indent, "         text:  ") ;
  
  printItem(zmapWindowContainerGetBackground(container_parent),
	    indent, "   background:  ") ;

  return ;
}



/* We need the window in here.... */
/* GFunc to call on potentially all container groups.
 */
static void preZoomCB(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;
  ZoomData zoom_data = (ZoomData)user_data ;
  ContainerData container_data ;

  container_data = g_object_get_data(G_OBJECT(container), CONTAINER_DATA) ;

  switch(container_data->level)
    {
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      columnZoomChanged(container, zoom_data->zoom, zoom_data->window) ;
      break ;
    default:
      break ;
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Maybe this should only happen for featurs...also we should check to see if the column
   * is hidden, not much point in redrawing if hidden ? Think about this..... */

  if (container_data->child_redraw_required)
    {
      /* This function must go through child list of container getting the redraw
       * callback from each child and calling the redraw.... */
      redrawChildren() ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}



static void postZoomCB(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;
  ZoomData zoom_data = (ZoomData)user_data ;
  ContainerData container_data ;


  container_data = g_object_get_data(G_OBJECT(container), CONTAINER_DATA) ;

  if (container_data->level != ZMAPCONTAINER_LEVEL_FEATURESET)
    repositionContainer(container) ;

  /* Correct for the stupid scale bar.... */
  if (container_data->level == ZMAPCONTAINER_LEVEL_ROOT)
    my_foo_canvas_item_goto(FOO_CANVAS_ITEM(container), &(zoom_data->window->alignment_start), NULL) ; 


  return ;
}





static void columnZoomChanged(FooCanvasGroup *container, double new_zoom, ZMapWindow window)
{
  ZMapFeatureTypeStyle style = NULL;

  style = g_object_get_data(G_OBJECT(container), "item_feature_style") ;
  zMapAssert(style) ;

  zmapWindowColumnSetMagState(window, container, style) ;

  return ;
}



/* NOTE THAT THIS NEEDS TO BE ALMALGAMATED WITH repositionGroups() */

/* Repositions all the children of a group and makes sure they are correctly aligned
 * and that the background is the right size. */
static void repositionContainer(FooCanvasGroup *container)
{
  ContainerData container_data ;
  FooCanvasGroup *all_groups ;
  double x1, y1, x2, y2 ;
  GList *curr_child ;
  double curr_bound ;

  container_data = g_object_get_data(G_OBJECT(container), CONTAINER_DATA) ;

  all_groups = zmapWindowContainerGetFeatures(container) ;


  /* Now move all the groups over so they are positioned properly,
   * N.B. this might mean moving them in either direction depending on whether the changed
   * group got bigger or smaller. */
  curr_child = g_list_first(all_groups->item_list) ;

  /* Get the position of the first group within its parent. */
  x1 = y1 = x2 = y2 = 0.0 ; 
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(curr_child->data), &x1, &y1, &x2, &y2) ;
  curr_bound = x2 + container_data->child_spacing ;

  do
    {
      FooCanvasGroup *curr_group = FOO_CANVAS_GROUP(curr_child->data) ;
      double dx = 0.0 ;

      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(curr_group), &x1, &y1, &x2, &y2) ;

      dx = curr_bound - x1 ;				    /* can be +ve or -ve */

      if (dx != 0.0)
	{
	  curr_bound = x2 + dx + container_data->child_spacing ;

	  foo_canvas_item_move(FOO_CANVAS_ITEM(curr_group), dx, 0.0) ;
							    /* N.B. we only shift in x, not in y. */
	}
    }
  while ((curr_child = g_list_next(curr_child))) ;

  /* Make the parent groups bounding box as large as the group.... */
  zmapWindowContainerSetBackgroundSize(container, 0.0) ;


  return ;
}

