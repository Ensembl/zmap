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
 * Last edited: Oct  7 11:34 2005 (rds)
 * Created: Thu Sep  8 10:34:49 2005 (edgrif)
 * CVS info:   $Id: zmapWindowDraw.c,v 1.3 2005-10-07 10:54:31 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>


/* Used for holding state while bumping columns. */
typedef struct
{
  double offset ;
  double incr ;
  ZMapWindowBumpType bump_type ;
  GHashTable *pos_hash ;
} BumpColStruct, *BumpCol ;


/* Used to identify unambiguously which part of a zmapWindowContainer group a particular
 * foo canvas group or item represents. */
typedef enum {CONTAINER_INVALID,
	      CONTAINER_ROOT, CONTAINER_PARENT,
	      CONTAINER_FEATURES, CONTAINER_BACKGROUND} ContainerType ;


static void bumpColCB(gpointer data, gpointer user_data) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void doCol(gpointer data, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void valueDestroyCB(gpointer data) ;

static void repositionGroups(FooCanvasGroup *changed_group, double group_spacing) ;

static gint horizPosCompare(gconstpointer a, gconstpointer b) ;


static void doprint(FooCanvasGroup *container_parent, int indent) ;
static void printItem(FooCanvasItem *item, int indent, char *prefix) ;
static void listCB(gpointer data, gpointer user_data) ;




/* Creates a "container" for our sequence features which consists of: 
 * 
 *                parent_group
 *                  /      \
 *                 /        \
 *                /          \
 *           background    sub_group of
 *             item        feature items
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
					  GdkColor *background_fill_colour,
					  GdkColor *background_border_colour)
{
  FooCanvasGroup *container_parent = NULL, *container_features ;
  FooCanvasItem *container_background ;
  double x1, y1, x2, y2 ;
  ContainerType parent_type = CONTAINER_INVALID, container_type ;


  /* Parent has to be a group. */
  zMapAssert(FOO_IS_CANVAS_GROUP(parent)) ;


  /* Is the parent a container itself, if not then make this the root container. */
  parent_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(parent), "container_type")) ;
  zMapAssert(parent_type == CONTAINER_INVALID || parent_type == CONTAINER_FEATURES) ;
  if (parent_type == CONTAINER_FEATURES)
    container_type = CONTAINER_PARENT ;
  else
    container_type = CONTAINER_ROOT ;


  container_parent = FOO_CANVAS_GROUP(foo_canvas_item_new(parent,
							  foo_canvas_group_get_type(),
							  NULL)) ;
  g_object_set_data(G_OBJECT(container_parent), "container_type",
		    GINT_TO_POINTER(container_type)) ;

  container_features = FOO_CANVAS_GROUP(foo_canvas_item_new(container_parent,
							    foo_canvas_group_get_type(),
							    NULL)) ;
  g_object_set_data(G_OBJECT(container_features), "container_type",
		    GINT_TO_POINTER(CONTAINER_FEATURES)) ;

  /* We don't use the border colour at the moment but we may wish to later.... */
  container_background = zMapDrawSolidBox(FOO_CANVAS_ITEM(container_parent),
					  0.0, 0.0, 0.0, 0.0, background_fill_colour) ;
  g_object_set_data(G_OBJECT(container_background), "container_type",
		    GINT_TO_POINTER(CONTAINER_BACKGROUND)) ;

 /* Note that we rely on the background being first in parent groups item list which we
  * we achieve with this lower command. This is both so that the background appears _behind_
  * the objects that are children of the subgroup and so that we can return the background
  * and subgroup items correctly. */
  foo_canvas_item_lower_to_bottom(container_background) ; 

  return container_parent ;
}



/* Given either child of the container, return the container_parent. */
FooCanvasGroup *zmapWindowContainerGetParent(FooCanvasItem *unknown_child)
{
  FooCanvasGroup *container_parent = NULL ;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(unknown_child), "container_type")) ;

  switch(type)
    {
    case CONTAINER_FEATURES:
    case CONTAINER_BACKGROUND:
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

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), "container_type")) ;
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

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), "container_type")) ;
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

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), "container_type")) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  container_background = FOO_CANVAS_ITEM((g_list_nth(container_parent->item_list, 0))->data) ;

  return container_background ;
}


/* Setting the background size is not completely straight forward because we always
 * want the hieght of the background to be the full height of the group (e.g. column)
 * but we want the width to be set from the horizontal extent of the features.
 *
 * The height (y dimension) of the background can be given, if height is 0.0 then
 * the height of the child group is used.
 */
void zmapWindowContainerSetBackgroundSize(FooCanvasGroup *container_parent, double height)
{
  FooCanvasGroup *container_features ;
  FooCanvasItem *container_background ;
  double x1, y1, x2, y2 ;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), "container_type")) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  zMapAssert(height >= 0.0) ;

  container_features = zmapWindowContainerGetFeatures(container_parent) ;
  container_background = zmapWindowContainerGetBackground(container_parent) ;

  /* Get the height from the main group */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(container_parent), NULL, &y1, NULL, &y2) ;

  /* Get the width from the child group */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(container_features), &x1, NULL, &x2, NULL) ;

  if (height != 0.0)
    y2 = y2 + height ;

  foo_canvas_item_set(container_background,
		      "x1", x1,
		      "y1", y1,
		      "x2", x2,
		      "y2", y2,
		      NULL) ;

  return ;
}



void zmapWindowContainerPrint(FooCanvasGroup *container_parent)
{
  int indent = 0 ;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), "container_type")) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;


  doprint(container_parent, indent) ;

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


void zmapWindowColumnBump(FooCanvasGroup *column_group, ZMapWindowBumpType bump_type)
{
  BumpColStruct bump_data = {0.0} ;
  FooCanvasGroup *column_features ;

  column_features = zmapWindowContainerGetFeatures(column_group) ;

  bump_data.bump_type = bump_type ;

  switch (bump_type)
    {
    case ZMAP_WINDOW_BUMP_NONE:
      bump_data.incr = 0.0 ;
      break ;
    case ZMAP_WINDOW_BUMP_SIMPLE:
      bump_data.incr = 20.0 ;
      break ;
    case ZMAP_WINDOW_BUMP_POSITION:
      bump_data.incr = 20.0 ;

      bump_data.pos_hash = g_hash_table_new_full(NULL, NULL, /* NULL => use direct hash */
						 NULL, valueDestroyCB) ;
      break ;
    default:
      zMapAssert("Coding error, unrecognised ZMapWindowItemFeatureType") ;
      break ;
    }

  /* bump all the features. */
  g_list_foreach(column_features->item_list, bumpColCB, (gpointer)&bump_data) ;


  if (bump_type == ZMAP_WINDOW_BUMP_POSITION)
    g_hash_table_destroy(bump_data.pos_hash) ;

  /* Make the parent groups bounding box as large as the group.... */
  zmapWindowContainerSetBackgroundSize(column_group, 0.0) ;

  /* Now make sure all the columns are positioned correctly. */
  zmapWindowColumnReposition(column_group) ;

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

  /* I think its a mistake to reset the x1 as we have the scroll bar there.... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  foo_canvas_set_scroll_region(column->item.canvas, bound_x1, y1, bound_x2, y2) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  foo_canvas_set_scroll_region(column->item.canvas, x1, y1, bound_x2, y2) ;


  return ;
}






/* 
 *                Internal routines.
 */


/* GDestroyNotify() function for freeing the data part of a hash association. */
static void valueDestroyCB(gpointer data)
{
  g_free(data) ;

  return ;
}




/* The changed group is found in its parents list of items and then all items to the right
 * of the changed group are repositioned with respect to the changed group.
 * 
 * NOTE that we assume that the child items of the group_parent occur in the list
 * in their positional order. */
static void repositionGroups(FooCanvasGroup *changed_group, double group_spacing)
{
  double x1, y1, x2, y2 ;
  FooCanvasGroup *parent_container, *all_groups ;
  FooCanvasItem *background ;
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
  double y1 = 0.0 ;
  gpointer y1_ptr = 0 ;
  gpointer key = NULL, value = NULL ;
  double offset = 0.0 ;

  if(!(zmapWindowItemIsVisible(item)))
    return ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "item_feature_type")) ;

  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;

  switch (bump_data->bump_type)
    {
    case ZMAP_WINDOW_BUMP_POSITION:
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
    case ZMAP_WINDOW_BUMP_NONE:
    case ZMAP_WINDOW_BUMP_SIMPLE:
      {
	offset = bump_data->offset ;
	bump_data->offset += bump_data->incr ;

	break ;
      }
    default:
      zMapAssert("Coding error, unrecognised bump type.") ;
      break ;
    }

  my_foo_canvas_item_goto(item, &(offset), NULL) ; 

  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void doCol(gpointer data, gpointer user_data)
{
  FooCanvasGroup *group = (FooCanvasGroup *)data ;
  ZMapWindowItemFeatureType item_feature_type ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(group), "item_feature_type")) ;

  switch (item_feature_type)
    {
    case ITEM_SET:

      printf("set position: %f, %f\n" , group->xpos, group->ypos) ;

      break ;
    default:
      zMapAssert("Coding error, unrecognised ZMapWindowItemFeatureType") ;
      break ;
    }

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



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


static void doprint(FooCanvasGroup *container_parent, int indent)
{
  int i ;
  FooCanvasGroup *container_features ;
  FooCanvasItem *container_background ;
  ContainerType type = CONTAINER_INVALID ;

  for (i = 0 ; i < indent ; i++)
    {
      printf("  ") ;
    }
  printf("Level %d:\n", indent) ;

  
  printItem(FOO_CANVAS_ITEM(container_parent),
	    indent, "    parent:") ;

  printItem(FOO_CANVAS_ITEM(zmapWindowContainerGetFeatures(container_parent)),
	    indent, "  features:") ;

  printItem(zmapWindowContainerGetBackground(container_parent),
	    indent, "background:") ;

  container_features = zmapWindowContainerGetFeatures(container_parent) ;

  if (container_features->item_list)
    {
      type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_features->item_list->data),
					       "container_type")) ;
      if (type == CONTAINER_PARENT)
	{
	  g_list_foreach(container_features->item_list, listCB, GINT_TO_POINTER(indent + 1)) ;
	}
    }

  return ;
}


static void listCB(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container_parent = FOO_CANVAS_GROUP(data) ;
  int indent = GPOINTER_TO_INT(user_data) ;

  doprint(container_parent, indent) ;

  return ;
}


static void printItem(FooCanvasItem *item, int indent, char *prefix)
{
  double x1, y1, x2, y2 ;
  int i ;

  for (i = 0 ; i < indent ; i++)
    {
      printf("  ") ;
    }
  printf("%s", prefix) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;
  printf("%f, %f, %f, %f\n", x1, y1, x2, y2) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  zmapWindowPrintItemCoords(item) ;


  return ;
}
