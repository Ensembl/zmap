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
 * Last edited: Sep 22 13:23 2005 (edgrif)
 * Created: Thu Sep  8 10:34:49 2005 (edgrif)
 * CVS info:   $Id: zmapWindowDraw.c,v 1.1 2005-09-22 12:39:51 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>

typedef struct
{
  double offset ;
  double incr ;
  ZMapWindowBumpType bump_type ;
  GHashTable *pos_hash ;
} BumpColStruct, *BumpCol ;


static void bumpColCB(gpointer data, gpointer user_data) ;

static void doCol(gpointer data, gpointer user_data) ;

static void repositionGroups(FooCanvasGroup *changed_group, double group_spacing) ;

static gint horizPosCompare(gconstpointer a, gconstpointer b) ;



static gboolean keysEqualCB(gconstpointer a, gconstpointer b) ;
static void keyDestroyCB(gpointer data) ;
static void valueDestroyCB(gpointer data) ;




/* Creates a foocanvas group and a rectangle to be used as a background for that group a
 * children of parent, returns the child group. The background a fill and border colour.
 * Note that we rely on the order in which the group and background item are added, if this
 * changes then the accessor functions for getting them need to change. */
FooCanvasGroup *zmapWindowContainerCreate(FooCanvasGroup *parent,
					  GdkColor *background_fill_colour,
					  GdkColor *background_border_colour)
{
  FooCanvasGroup *group ;
  FooCanvasItem *background_item ;
  double x1, y1, x2, y2 ;

  /* Must be a group with _no_ existing children. */
  zMapAssert(FOO_IS_CANVAS_GROUP(parent) && !parent->item_list) ;

  group = FOO_CANVAS_GROUP(foo_canvas_item_new(parent,
					       foo_canvas_group_get_type(),
					       NULL)) ;

  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(parent), &x1, &y1, &x2, &y2) ;

  background_item = zMapDrawSolidBox(FOO_CANVAS_ITEM(parent),
				     x1, y1, x2, y2, background_fill_colour) ;

  foo_canvas_item_lower_to_bottom(background_item) ; 

  return group ;
}


FooCanvasGroup *zmapWindowContainerGetParent(FooCanvasItem *child)
{
  FooCanvasGroup *parent_group ;

  parent_group = FOO_CANVAS_GROUP(child->parent) ;

  zMapAssert(FOO_IS_CANVAS_GROUP(parent_group) && g_list_length(parent_group->item_list) == 2) ;

  return parent_group ;
}


FooCanvasGroup *zmapWindowContainerGetSuperGroup(FooCanvasGroup *parent)
{
  FooCanvasGroup *super_group ;

  zMapAssert(FOO_IS_CANVAS_GROUP(parent) && g_list_length(parent->item_list) == 2) ;

  super_group = zmapWindowContainerGetParent(parent->item.parent) ;

  return super_group ;
}



FooCanvasGroup *zmapWindowContainerGetGroup(FooCanvasGroup *parent)
{
  FooCanvasGroup *container_group ;

  zMapAssert(FOO_IS_CANVAS_GROUP(parent) && g_list_length(parent->item_list) == 2) ;

  container_group = FOO_CANVAS_GROUP((g_list_nth(parent->item_list, 1))->data) ;

  return container_group ;
}


FooCanvasItem *zmapWindowContainerGetBackground(FooCanvasGroup *parent)
{
  FooCanvasItem *background ;

  zMapAssert(FOO_IS_CANVAS_GROUP(parent) && g_list_length(parent->item_list) == 2) ;

  background = FOO_CANVAS_ITEM((g_list_nth(parent->item_list, 0))->data) ;

  return background ;
}


/* Do we want the background to be the size of the parent group or the size of the
 * subgroup...the subgroup I think ??  The depth (y dimension) of the background can
 * be given, if depth is 0.0 then the depth of the child group is used.
 */
void zmapWindowContainerSetBackgroundSize(FooCanvasGroup *parent_group, double depth)
{
  FooCanvasGroup *child_group ;
  FooCanvasItem *background ;
  double x1, y1, x2, y2 ;

  zMapAssert(FOO_IS_CANVAS_GROUP(parent_group) && g_list_length(parent_group->item_list) == 2) ;
  zMapAssert(depth >= 0.0) ;

  child_group = zmapWindowContainerGetGroup(parent_group) ;
  background = zmapWindowContainerGetBackground(parent_group) ;

  /* Get the height from the main group */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(parent_group), NULL, &y1, NULL, &y2) ;

  /* Get the width from the child group */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(child_group), &x1, NULL, &x2, NULL) ;

  if (depth != 0.0)
    y2 = y2 + depth ;

  foo_canvas_item_set(background,
		      "x1", x1,
		      "y1", y1,
		      "x2", x2,
		      "y2", y2,
		      NULL) ;

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
  FooCanvasItem *background ;

  zMapAssert(FOO_IS_CANVAS_GROUP(column_group)) ;

  column_features = zmapWindowContainerGetGroup(column_group) ;

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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      bump_data.pos_hash = g_hash_table_new_full(NULL, keysEqualCB, /* NULL => use direct hash */
						 keyDestroyCB, valueDestroyCB) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

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
  background = zmapWindowContainerGetBackground(column_group) ;
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
  FooCanvasGroup *column_parent, *block, *align ;

  zMapAssert(FOO_IS_CANVAS_GROUP(column)) ;


  column_parent = zmapWindowContainerGetSuperGroup(column) ;

  block = zmapWindowContainerGetSuperGroup(column_parent) ;	/* We don't use this as we don't
								   need to actually move the blocks. */
  align = zmapWindowContainerGetSuperGroup(block) ;

  /* Move all the columns in the current group that need to be moved. */
  repositionGroups(column, COLUMN_SPACING) ;

  /* Move all the column groups in the containing block that need to be moved. */
  repositionGroups(column_parent, STRAND_SPACING) ;

  /* Move all the alignment groups in the canvas that need to be moved. */
  repositionGroups(block, ALIGN_SPACING) ;

  /* Move all the alignment groups in the canvas that need to be moved. */
  repositionGroups(align, ALIGN_SPACING) ;

  /* Reset scroll region */
  foo_canvas_get_scroll_region(column->item.canvas, &x1, &y1, &x2, &y2) ;

  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(foo_canvas_root(column->item.canvas)),
			     &bound_x1, NULL, &bound_x2, NULL) ;

  foo_canvas_set_scroll_region(column->item.canvas, bound_x1, y1, bound_x2, y2) ;


  return ;
}






/* 
 *                Internal routines.
 */

static gboolean keysEqualCB(gconstpointer a, gconstpointer b)
{
  gboolean keys_equal = FALSE ;

  if (*(double *)a == *(double *)b)
    keys_equal = TRUE ;

  return keys_equal ;
}

static void keyDestroyCB(gpointer data)
{
  g_free(data) ;

  return ;
}

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
  FooCanvasGroup *group_parent ;
  FooCanvasItem *background ;
  GList *curr_child ;
  double curr_bound ;

  zMapAssert(FOO_IS_CANVAS_GROUP(changed_group)) ;


  group_parent = FOO_CANVAS_GROUP(changed_group->item.parent) ;


  /* Get the position of the changed group within its parent. */
  x1 = y1 = x2 = y2 = 0.0 ; 
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(changed_group), &x1, &y1, &x2, &y2) ;
  curr_bound = x2 + group_spacing ;


  /* Now move all the groups to the right of this one over so they are positioned properly,
   * N.B. this might mean moving them in either direction depending on whether the changed
   * group got bigger or smaller. */
  curr_child = g_list_find(group_parent->item_list, changed_group) ;
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
  zmapWindowContainerSetBackgroundSize(changed_group, 0.0) ;


  return ;
}



static void bumpColCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  BumpCol bump_data = (BumpCol)user_data ;
  ZMapWindowItemFeatureType item_feature_type ;
  double y1 = 0.0 ;
  gpointer y1_ptr = 0 ;
  gpointer key = NULL, value = NULL ;
  double offset = 0.0 ;

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
