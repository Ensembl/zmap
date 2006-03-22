/*  File: zmapWindowContainer.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Package for handling our structures that represent
 *              different levels in the feature context hierachy.
 *              This enables us to generalise certain functions.
 *              
 * Exported functions: See zmapWindowContainer.h
 * HISTORY:
 * Last edited: Mar 22 16:03 2006 (edgrif)
 * Created: Wed Dec 21 12:32:25 2005 (edgrif)
 * CVS info:   $Id: zmapWindowContainer.c,v 1.8 2006-03-22 17:17:41 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>


#define CONTAINER_DATA "container_data"			    /* gobject get/set name */


typedef struct
{
  ZMapContainerLevelType level ;

  /* A optimisation because we don't want to attach this information to every single feature.... */
  gboolean child_redraw_required ;

  /* I'd like to add a border here too..... */
  double child_spacing ;

} ContainerDataStruct, *ContainerData ;


typedef struct execOnChildrenStruct_
{
  ZMapContainerLevelType stop ;

  GFunc                  down_func_cb ;
  gpointer               down_func_data ;

  GFunc                  up_func_cb ;
  gpointer               up_func_data ;

} execOnChildrenStruct, *execOnChildren ;

static void containerDestroyCB(GtkObject *object, gpointer user_data) ;

static void eachContainer(gpointer data, gpointer user_data) ;

static void printlevel(gpointer data, gpointer user_data);
static void printItem(FooCanvasItem *item, int indent, char *prefix) ;

static void itemDestroyCB(gpointer data, gpointer user_data);

static void redrawChildrenCB(gpointer data, gpointer user_data) ;


/* Creates a "container" for our sequence features which consists of: 
 * 
 *                parent_group
 *                  /   |  \
 *                 /        \
 *                /     |    \
 *           background    sub_group of
 *             item     |  feature items
 *
 *
 *    THIS IS NOT USED CURRENTLY, MAY USE IN FUTURE FOR OVERLAYS OF TEXT.
 *                      |
 *                Optional text  
 *                    group (dotted line)
 *
 * should change so parent - group of all sub_group features - sub_group feature items (including Optional text group!)
 *
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
  container_data->level = level ;
  container_data->child_spacing = child_spacing ;
  container_data->child_redraw_required = FALSE ;

  g_object_set_data(G_OBJECT(container_parent), CONTAINER_DATA, container_data) ;
  g_object_set_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY,
		    GINT_TO_POINTER(container_type)) ;
  g_signal_connect(G_OBJECT(container_parent), "destroy", G_CALLBACK(containerDestroyCB), NULL) ;

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




ZMapContainerLevelType zmapWindowContainerGetLevel(FooCanvasGroup *container_parent)
{
  ContainerData container_data ;
  ZMapContainerLevelType level ;

  container_data = g_object_get_data(G_OBJECT(container_parent), CONTAINER_DATA) ;
  zMapAssert(container_data) ;

  level = container_data->level ;

  return level ;
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
      container_parent = FOO_CANVAS_GROUP(unknown_child->parent) ;
      break ;
    default:
      zMapAssertNotReached() ;
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
 * several different types of features....
 * 
 * Note that each column has a style but then the features within it may have
 * their own styles...
 *  */
ZMapFeatureTypeStyle zmapWindowContainerGetStyle(FooCanvasGroup *column_group)
{
  ZMapFeatureTypeStyle style = NULL ;

  style = g_object_get_data(G_OBJECT(column_group), "item_feature_style") ;

  return style ;
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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  container_text       = zmapWindowContainerGetText(container_parent) ;  
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

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
void zmapWindowContainerSetBackgroundSizePlusBorder(FooCanvasGroup *container_parent,
                                                    double height, 
                                                    double border)
{
  FooCanvasItem *container_background = NULL;
  double x1, x2;

  zmapWindowContainerSetBackgroundSize(container_parent, height);

  container_background = zmapWindowContainerGetBackground(container_parent) ;

  foo_canvas_item_get_bounds(container_background, &x1, NULL, &x2, NULL) ;

  foo_canvas_item_set(container_background,
                      "x1", x1,
                      "x2", x2 + (border * 2.0),
                      NULL) ;
  border *= -1.0;

  my_foo_canvas_item_goto(container_background, &border, NULL);
  
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


void zmapWindowContainerPrint(FooCanvasGroup *container_parent)
{
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  zmapWindowContainerExecute(container_parent,
			     ZMAPCONTAINER_LEVEL_FEATURESET,
			     printlevel, NULL,
			     NULL, NULL) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  doprint(container_parent, indent) ;
#endif

  return ;
}


/* unified way to descend and do things to ALL and every or just some */
/* Something I'm just trying out! */
void zmapWindowContainerExecute(FooCanvasGroup        *parent, 
			       ZMapContainerLevelType stop_at_type,
			       GFunc                  down_func_cb,
			       gpointer               down_func_data,
			       GFunc                  up_func_cb,
			       gpointer               up_func_data)
{
  execOnChildrenStruct exe  = {0,NULL};
  ContainerType        type = CONTAINER_INVALID ;

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


/* NOTE THAT THIS NEEDS TO BE ALMALGAMATED WITH repositionGroups() */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Repositions all the children of a group and makes sure they are correctly aligned
 * and that the background is the right size. */
void zmapWindowContainerReposition(FooCanvasGroup *container)
{
  ContainerData container_data ;
  ZMapContainerLevelType type;
  FooCanvasGroup *all_groups ;
  double x1, y1, x2, y2 ;
  GList *curr_child ;
  double curr_bound ;

  container_data = g_object_get_data(G_OBJECT(container), CONTAINER_DATA) ;
  type           = container_data->level;
  all_groups     = zmapWindowContainerGetFeatures(container) ;

  
  /* Now move all the groups over so they are positioned properly,
   * N.B. this might mean moving them in either direction depending on whether the changed
   * group got bigger or smaller. */
  curr_child = g_list_first(all_groups->item_list) ;
  x1 = y1 = x2 = y2 = 0.0 ; 
  curr_bound = 0.0 ;

  do
    {
      FooCanvasGroup *curr_group = FOO_CANVAS_GROUP(curr_child->data) ;
      double dx = 0.0 ;

      if (FOO_CANVAS_ITEM(curr_group)->object.flags & FOO_CANVAS_ITEM_VISIBLE)
	{
	  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(curr_group), &x1, &y1, &x2, &y2) ;

	  dx = curr_bound - x1 ;			    /* can be +ve or -ve */

	  foo_canvas_item_move(FOO_CANVAS_ITEM(curr_group), dx, 0.0) ;
							    /* N.B. we only shift in x, not in y. */

	  curr_bound = x2 + dx + container_data->child_spacing ;

	  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(curr_group), &x1, &y1, &x2, &y2) ;
	}
    }
  while ((curr_child = g_list_next(curr_child))) ;

  /* Make the parent groups bounding box as large as the group.... */
  /* We need to check for strand level to add the border so that 
   * we don't end up flush with the edge of the window and "strand separator"
   * A better model would be to have a separate group for the strand separator
   * rather than rely on the background of the group below.
   */
  if(type == ZMAPCONTAINER_LEVEL_STRAND)
      zmapWindowContainerSetBackgroundSizePlusBorder(container, 0.0, COLUMN_SPACING);
  else
    zmapWindowContainerSetBackgroundSize(container, 0.0) ;
  
  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* Repositions all the children of a group and makes sure they are correctly aligned
 * and that the background is the right size. */
void zmapWindowContainerReposition(FooCanvasGroup *container)
{
  ContainerData container_data ;
  ZMapContainerLevelType type;
  FooCanvasGroup *all_groups ;
  double x1, y1, x2, y2 ;
  GList *curr_child ;
  double curr_bound ;

  container_data = g_object_get_data(G_OBJECT(container), CONTAINER_DATA) ;
  type           = container_data->level;
  all_groups     = zmapWindowContainerGetFeatures(container) ;


  /* Now move all the groups over so they are positioned properly, but only if there
   * are children, it's perfectly legal for there to be no columns on the reverse
   * strand for instance.
   * N.B. this might mean moving them in either direction depending on whether the changed
   * group got bigger or smaller. */
  if ((curr_child = g_list_first(all_groups->item_list)))
    {
      x1 = y1 = x2 = y2 = 0.0 ; 
      curr_bound = 0.0 ;

      /* We don't bump blocks within a single align, we want them to appear one above the other. */
      if (type != ZMAPCONTAINER_LEVEL_ALIGN)
	{
	  do
	    {
	      FooCanvasGroup *curr_group = FOO_CANVAS_GROUP(curr_child->data) ;
	      double dx = 0.0 ;

	      if (FOO_CANVAS_ITEM(curr_group)->object.flags & FOO_CANVAS_ITEM_VISIBLE)
		{
		  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(curr_group), &x1, &y1, &x2, &y2) ;

		  dx = curr_bound - x1 ;			    /* can be +ve or -ve */

		  foo_canvas_item_move(FOO_CANVAS_ITEM(curr_group), dx, 0.0) ;
		  /* N.B. we only shift in x, not in y. */

		  curr_bound = x2 + dx + container_data->child_spacing ;

		  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(curr_group), &x1, &y1, &x2, &y2) ;
		}
	    }
	  while ((curr_child = g_list_next(curr_child))) ;
	}


      /* Make the parent groups bounding box as large as the group.... */
      /* We need to check for strand level to add the border so that 
       * we don't end up flush with the edge of the window and "strand separator"
       * A better model would be to have a separate group for the strand separator
       * rather than rely on the background of the group below.
       */
      if(type == ZMAPCONTAINER_LEVEL_STRAND)
	zmapWindowContainerSetBackgroundSizePlusBorder(container, 0.0, COLUMN_SPACING);
      else
	zmapWindowContainerSetBackgroundSize(container, 0.0) ;
    }
  
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
      g_list_foreach(unknown_child->item_list, itemDestroyCB, NULL);
      break ;
    default:
      zMapAssertNotReached() ;
    }
  
  return ;
}



/* 
 *                Internal routines.
 */



static void containerDestroyCB(GtkObject *object, gpointer user_data)
{
  ContainerData container_data ;

  container_data = g_object_get_data(G_OBJECT(object), CONTAINER_DATA) ;
  g_free(container_data) ;

  return ;
}




/* Called for every container while descending.... */
static void eachContainer(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;
  execOnChildren all_data = (execOnChildren)user_data ;
  ContainerData container_data ;
  FooCanvasGroup *children ;
      

  container_data = g_object_get_data(G_OBJECT(container), CONTAINER_DATA) ;
  children = zmapWindowContainerGetFeatures(container) ;

  /* Execute pre-recursion function. */
  if (all_data->down_func_cb)
    (all_data->down_func_cb)(container, all_data->down_func_data);



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  {
    if (container_data->level == ZMAPCONTAINER_LEVEL_FEATURESET)
      {
	ZMapFeatureTypeStyle style ;

	style = g_object_get_data(G_OBJECT(container), "item_feature_style") ;

	if (g_ascii_strcasecmp("dna", g_quark_to_string(style->original_id)) == 0)
	  printf("found it\n") ;
	else
	  printf("col: %s\n", g_quark_to_string(style->original_id)) ;
      }
  }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* Optimisation: if the features need redrawing then we do them as a special...
   * note that currently this is only done for children of a feature set..i.e. features. */
  if (container_data->level == ZMAPCONTAINER_LEVEL_FEATURESET
      && container_data->child_redraw_required
      && children)
    {
      /* Surely we will need some user_data here ???? */
      g_list_foreach(children->item_list, redrawChildrenCB, NULL) ;
    }


  /* Recurse ? */
  if (all_data->stop != container_data->level)
    {
      if (children)
	{
	  g_list_foreach(children->item_list, eachContainer, user_data) ;
	}
    }


  /* Execute post-recursion function. */
  if (all_data->up_func_cb)
    (all_data->up_func_cb)(container, all_data->up_func_data) ;

  return ;
}



/*! g_list_foreach function to remove items */
static void itemDestroyCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;

  gtk_object_destroy(GTK_OBJECT(item));

  return ;
}


static void printlevel(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container_parent = (FooCanvasGroup *)data;
  ContainerType type = CONTAINER_INVALID ;
  ZMapFeatureAny any_feature ;
  char *feature_type = NULL ;
  int i, indent ;

  any_feature = (ZMapFeatureAny)(g_object_get_data(G_OBJECT(container_parent), ITEM_FEATURE_DATA)) ;

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
  
  printItem(zmapWindowContainerGetBackground(container_parent),
	    indent, "   background:  ") ;

  return ;
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


static void redrawChildrenCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  zmapWindowContainerZoomChangedCallback redraw_cb ;

  
  if ((redraw_cb = g_object_get_data(G_OBJECT(item), CONTAINER_REDRAW_CALLBACK)))
    {
      ZMapWindow window ;

      window = (ZMapWindow)g_object_get_data(G_OBJECT(item), CONTAINER_REDRAW_DATA) ;

      /* do the callback.... */
      redraw_cb(item, zMapWindowGetZoomFactor(window), window) ;
    }


  return ;
}
