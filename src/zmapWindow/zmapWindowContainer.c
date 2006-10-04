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
 * Last edited: Oct  4 15:14 2006 (rds)
 * Created: Wed Dec 21 12:32:25 2005 (edgrif)
 * CVS info:   $Id: zmapWindowContainer.c,v 1.15 2006-10-04 14:28:08 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>

/* gobject get/set names */
#define CONTAINER_DATA            "container_data"  
#define CONTAINER_REDRAW_CALLBACK "container_redraw_callback"
#define CONTAINER_REDRAW_DATA     "container_redraw_data"

/* This ONLY applies to ZMAPCONTAINER_LEVEL_STRAND  CONTAINER_PARENT 
 * containers. The code enforces this with asserts and is available
 * via the zmapWindowContainer{G,S}etStrand() functions. */
#define CONTAINER_STRAND          "container_strand"

typedef struct
{
  ZMapContainerLevelType level ;

  /* A optimisation because we don't want to attach this information to every single feature.... */
  gboolean child_redraw_required ;

  /* I'd like to add a border here too..... */
  double child_spacing ;

  double this_spacing ;         /* ... to save time ... see eachContainer & containerXAxisMove */

  /* We cash this as some containers have their own special colours (e.g. 3 frame cols.). */
  GdkColor orig_background ;

  /* The long_item object, needed for freeing a record of long items. */
  ZMapWindowLongItems long_items ;
  
} ContainerDataStruct, *ContainerData ;

typedef struct
{
  FooCanvasPoints *root_points;
  FooCanvasPoints *align_points;
  FooCanvasPoints *block_points;
  FooCanvasPoints *strand_points;
  FooCanvasPoints *set_points;
  double root_bound, align_bound, 
    block_bound, strand_bound, 
    set_bound;
}ContainerPointsCacheStruct, *ContainerPointsCache;

typedef struct ContainerRecursionDataStruct_
{
  ZMapContainerLevelType stop ;

  ZMapContainerExecFunc  container_enter_cb   ;
  gpointer               container_enter_data ;

  ZMapContainerExecFunc  container_leave_cb   ;
  gpointer               container_leave_data ;

  gboolean               redraw_during_recursion;

  ContainerPointsCache   cache;

} ContainerRecursionDataStruct, *ContainerRecursionData;

typedef struct
{
  FooCanvasGroup *container;
  ZMapStrand strand;
}ContainerToStrandStruct, *ContainerToStrand;

static void containerDestroyCB(GtkObject *object, gpointer user_data) ;

static void eachContainer(gpointer data, gpointer user_data) ;

static void printlevel(FooCanvasGroup *container_parent, FooCanvasPoints *points, gpointer user_data);
static void printItem(FooCanvasItem *item, int indent, char *prefix) ;

static void itemDestroyCB(gpointer data, gpointer user_data);

static void redrawChildrenCB(gpointer data, gpointer user_data) ;
static void redrawColumn(FooCanvasItem *container, ContainerData data);


static void printFeatureSet(gpointer data, gpointer user_data) ;

static void containerPropogatePoints(FooCanvasPoints *from, FooCanvasPoints *to);
static void containerXAxisMove(FooCanvasGroup  *container, 
                               FooCanvasPoints *container_points,
                               ZMapContainerLevelType type,
                               double  this_level_spacing,
                               double *current_bound_inout);
static void containerMoveToZero(FooCanvasGroup *container);
static void containerSetMaxBackground(FooCanvasGroup *container, 
                                      FooCanvasPoints *this_points, 
                                      ContainerData container_data);


/* To cache the points when recursing ... */
static ContainerPointsCache containerPointsCacheCreate(void);
static void containerPointsCacheDestroy(ContainerPointsCache cache);
static void containerPointsCacheGetPoints(ContainerPointsCache cache, 
                                          ZMapContainerLevelType level,
                                          FooCanvasPoints **this_points_out,
                                          FooCanvasPoints **parent_points_out,
                                          double **level_current_bound);
static void containerPointsCacheResetPoints(ContainerPointsCache cache,
                                            ZMapContainerLevelType level);
static void containerPointsCacheResetBound(ContainerPointsCache cache,
                                           ZMapContainerLevelType level);



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

 * CONTAINER_DATA is a property of the PARENT
 * ITEM_FEATURE_STRAND is a property of the ZMAPCONTAINER_LEVEL_STRAND PARENT.
 * 
 * The long_items is forced on us by foocanvas not dealing with X Windows
 * limits on the longest graphical object that can be drawn.
 * 
 */
FooCanvasGroup *zmapWindowContainerCreate(FooCanvasGroup *parent,
					  ZMapContainerLevelType level,
					  double child_spacing,
					  GdkColor *background_fill_colour,
					  GdkColor *background_border_colour,
					  ZMapWindowLongItems long_items)
{
  FooCanvasGroup *container_parent = NULL, *container_features, *parent_container;
  FooCanvasItem *container_background ;
  ContainerType parent_type = CONTAINER_INVALID, container_type ;
  ContainerData container_data, parent_data = NULL;
  double this_spacing = 0.0;    /* Only really true for the root */

  /* Parent has to be a group. */
  zMapAssert(FOO_IS_CANVAS_GROUP(parent)) ;

  parent_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(parent_type == CONTAINER_INVALID || parent_type == CONTAINER_FEATURES) ;

  /* Is the parent a container itself, if not then make this the root container. */
  if (parent_type == CONTAINER_FEATURES)
    {
      container_type   = CONTAINER_PARENT ;
      parent_container = zmapWindowContainerGetParent(FOO_CANVAS_ITEM( parent ));
      parent_data  = g_object_get_data(G_OBJECT(parent_container), CONTAINER_DATA);
      this_spacing = parent_data->child_spacing;
    }
  else
    container_type = CONTAINER_ROOT ;


  container_parent = FOO_CANVAS_GROUP(foo_canvas_item_new(parent,
							  foo_canvas_group_get_type(),
							  NULL)) ;

  container_data = g_new0(ContainerDataStruct, 1) ;
  container_data->level = level ;
  container_data->child_spacing = child_spacing ;
  container_data->this_spacing  = this_spacing ;
  container_data->child_redraw_required = FALSE ;
  container_data->orig_background = *background_fill_colour ; /* n.b. struct copy. */
  container_data->long_items = long_items ;

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

/* This is weak and immature ATM, but better than NO type checking for the callback as it was! */
void zmapWindowContainerSetZoomEventHandler(FooCanvasGroup *c_level_featureset,
                                            zmapWindowContainerZoomChangedCallback handler_cb,
                                            ZMapWindow data)
{
  ContainerData container_data = NULL;
  ContainerType container_type = CONTAINER_INVALID;

  if(((container_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(c_level_featureset), 
                                                          CONTAINER_TYPE_KEY))) == CONTAINER_PARENT)
     && (container_data = g_object_get_data(G_OBJECT(c_level_featureset), CONTAINER_DATA))
     && container_data->level == ZMAPCONTAINER_LEVEL_FEATURESET)
    {
      zmapWindowContainerSetChildRedrawRequired(c_level_featureset, TRUE) ;
      g_object_set_data(G_OBJECT(c_level_featureset), CONTAINER_REDRAW_CALLBACK,
                        handler_cb) ;
      g_object_set_data(G_OBJECT(c_level_featureset), CONTAINER_REDRAW_DATA,
                        (gpointer)data) ;
    }
  else
    zMapAssertNotReached();

  return ;
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

void zmapWindowContainerSetStrand(FooCanvasGroup *container, ZMapStrand strand)
{
  ContainerData container_data = NULL;
  ContainerType container_type = CONTAINER_INVALID;

  container_data = g_object_get_data(G_OBJECT(container), CONTAINER_DATA);
  container_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container), CONTAINER_TYPE_KEY));

  zMapAssert(container_data && 
             container_data->level == ZMAPCONTAINER_LEVEL_STRAND &&
             container_type        == CONTAINER_PARENT);

  g_object_set_data(G_OBJECT(container), CONTAINER_STRAND, GINT_TO_POINTER(strand));

  return ;
}

ZMapStrand zmapWindowContainerGetStrand(FooCanvasGroup *container)
{
  ZMapStrand strand = ZMAPSTRAND_NONE;
  ContainerData container_data = NULL;
  ZMapContainerLevelType level = ZMAPCONTAINER_LEVEL_INVALID;
  FooCanvasGroup *strand_level_container = NULL;

  container_data = g_object_get_data(G_OBJECT(container), CONTAINER_DATA);

  zMapAssert(container_data); 

  level = container_data->level;

  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_STRAND:
      strand_level_container = container;
      break;
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      strand_level_container = zmapWindowContainerGetSuperGroup(container);
      container_data = g_object_get_data(G_OBJECT(strand_level_container), CONTAINER_DATA);
      break;
    case ZMAPCONTAINER_LEVEL_INVALID:
    default:
      /* Coding Error ONLY the containers BELOW ZMAPCONTAINER_LEVEL_STRAND are logically stranded */
      zMapAssertNotReached();   
      break;
    }

  zMapAssert(container_data->level == ZMAPCONTAINER_LEVEL_STRAND);

  strand = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(strand_level_container), CONTAINER_STRAND));
  
  return strand;
}

FooCanvasGroup *zmapWindowContainerGetStrandGroup(FooCanvasGroup *strand_parent, ZMapStrand strand)
{
  FooCanvasGroup *strand_group = NULL ;
  ContainerData container_data = NULL;
  ContainerType container_type = CONTAINER_INVALID;
  FooCanvasGroup *block_features ;
  ZMapStrand group_strand ;

  container_data = g_object_get_data(G_OBJECT(strand_parent), CONTAINER_DATA);
  container_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(strand_parent), CONTAINER_TYPE_KEY));

  zMapAssert((container_data && 
	      container_data->level == ZMAPCONTAINER_LEVEL_BLOCK)
	     && container_type == CONTAINER_PARENT) ;

  block_features = zmapWindowContainerGetFeatures(strand_parent) ;

  zMapAssert(g_list_length(block_features->item_list) == 2) ;

  strand_group = FOO_CANVAS_GROUP(g_list_nth_data(block_features->item_list, 0)) ;
  group_strand = zmapWindowContainerGetStrand(strand_group) ;

  if (group_strand != strand)
    {
      strand_group = FOO_CANVAS_GROUP(g_list_nth_data(block_features->item_list, 1)) ;
      group_strand = zmapWindowContainerGetStrand(strand_group) ;
    }

  return strand_group ;
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


double zmapWindowContainerGetSpacing(FooCanvasGroup *container_parent)
{
  ContainerData container_data ;
  double spacing ;

  container_data = g_object_get_data(G_OBJECT(container_parent), CONTAINER_DATA) ;
  zMapAssert(container_data) ;

  spacing = container_data->child_spacing ;

  return spacing ;
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


/* Note that each column has a style but then the features within it will have
 * their own styles...
 *  */
ZMapFeatureTypeStyle zmapWindowContainerGetStyle(FooCanvasGroup *column_group)
{
  ZMapFeatureTypeStyle style = NULL ;
  ZMapWindowItemFeatureSetData set_data ;

  /* These should go in container some time.... */
  set_data = g_object_get_data(G_OBJECT(column_group), ITEM_FEATURE_SET_DATA) ;
  zMapAssert(set_data) ;

  style = set_data->style ;

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
                                                    double height, double border)
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

  /* Move background over to left by "border" units. */
  foo_canvas_item_move(container_background, border, 0.0) ;

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


void zmapWindowContainerSetBackgroundColour(FooCanvasGroup *container_parent,
					    GdkColor *background_fill_colour)
{
  FooCanvasItem *container_background ;
  ContainerType type ;

  zMapAssert(FOO_IS_CANVAS_GROUP(container_parent) && background_fill_colour) ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  container_background = zmapWindowContainerGetBackground(container_parent) ;

  foo_canvas_item_set(container_background,
                      "fill_color_gdk", background_fill_colour,
                      NULL) ;

  return ;
}


void zmapWindowContainerResetBackgroundColour(FooCanvasGroup *container_parent)
{
  FooCanvasItem *container_background ;
  ContainerType type ;
  ContainerData container_data ;

  zMapAssert(FOO_IS_CANVAS_GROUP(container_parent)) ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  container_data = g_object_get_data(G_OBJECT(container_parent), CONTAINER_DATA) ;

  container_background = zmapWindowContainerGetBackground(container_parent) ;

  foo_canvas_item_set(container_background,
                      "fill_color_gdk", &(container_data->orig_background),
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


void zmapWindowContainerPrintLevel(FooCanvasGroup *strand_container)
{
  ContainerType type = CONTAINER_INVALID ;
  ContainerData container_data ;
  ZMapContainerLevelType level ;
  FooCanvasGroup *columns ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(strand_container), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT) ;

  container_data = g_object_get_data(G_OBJECT(strand_container), CONTAINER_DATA) ;
  level = container_data->level ;
  zMapAssert(level == ZMAPCONTAINER_LEVEL_STRAND) ;

  columns = zmapWindowContainerGetFeatures(strand_container) ;

  g_list_foreach(columns->item_list, printFeatureSet, NULL) ;

  return ;
}



void zmapWindowContainerPrint(FooCanvasGroup *container_parent)
{
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  zmapWindowContainerExecute(container_parent,
                             ZMAPCONTAINER_LEVEL_FEATURESET,
                             printlevel, NULL);

  return ;
}


/* unified way to descend and do things to ALL and every or just some */
/* Something I'm just trying out! */
void zmapWindowContainerExecute(FooCanvasGroup        *parent, 
                                ZMapContainerLevelType stop_at_type,
                                ZMapContainerExecFunc  container_enter_cb,
                                gpointer               container_enter_data)
{
  zmapWindowContainerExecuteFull(parent, stop_at_type, 
                                 container_enter_cb, 
                                 container_enter_data, 
                                 NULL, NULL, FALSE);
  return ;
}
/* unified way to descend and do things to ALL and every or just some */
/* Something I'm just trying out! */
void zmapWindowContainerExecuteFull(FooCanvasGroup        *parent, 
                                    ZMapContainerLevelType stop_at_type,
                                    ZMapContainerExecFunc  container_enter_cb,
                                    gpointer               container_enter_data,
                                    ZMapContainerExecFunc  container_leave_cb,
                                    gpointer               container_leave_data,
                                    gboolean               redraw_during_recursion)
{
  ContainerRecursionDataStruct data  = {0,NULL};
  ContainerType        type = CONTAINER_INVALID ;

  zMapAssert(stop_at_type >= ZMAPCONTAINER_LEVEL_ROOT ||
	     stop_at_type <= ZMAPCONTAINER_LEVEL_FEATURESET) ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_ROOT || type == CONTAINER_PARENT);

  data.stop                    = stop_at_type;
  data.redraw_during_recursion = redraw_during_recursion;

  data.container_enter_cb   = container_enter_cb;
  data.container_enter_data = container_enter_data;
  data.container_leave_cb   = container_leave_cb;
  data.container_leave_data = container_leave_data;

  data.cache = containerPointsCacheCreate();

  eachContainer((gpointer)parent, &data) ;

  containerPointsCacheDestroy(data.cache);

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
	zmapWindowContainerSetBackgroundSizePlusBorder(container, 0.0, COLUMN_SPACING) ;
      else
	zmapWindowContainerSetBackgroundSize(container, 0.0) ;
    }
  
  return ;
}

static void getStrandLevelWithStrand(FooCanvasGroup *container_parent, 
                                     FooCanvasPoints *points, 
                                     gpointer user_data)
{
  ContainerData container_data = NULL;
  ZMapContainerLevelType level = ZMAPCONTAINER_LEVEL_INVALID;
  ContainerToStrand io_data = (ContainerToStrand)user_data;

  container_data = g_object_get_data(G_OBJECT(container_parent), CONTAINER_DATA) ;
  level          = container_data->level;

  if(level == ZMAPCONTAINER_LEVEL_STRAND)
    {
      ZMapStrand strand = ZMAPSTRAND_NONE;

      strand = zmapWindowContainerGetStrand(container_parent);
      
      if(strand == io_data->strand)
        io_data->container = container_parent;
      
    }

  return ;
}

FooCanvasItem *zmapWindowContainerBlockGetStrandContainer(FooCanvasGroup *block_container, 
                                                          ZMapStrand strand)
{
  ContainerData container_data = NULL;
  FooCanvasItem *item = NULL;
  ZMapContainerLevelType level = ZMAPCONTAINER_LEVEL_INVALID;
  ContainerToStrandStruct test = {0};

  if((container_data = g_object_get_data(G_OBJECT(block_container), CONTAINER_DATA)))
    level = container_data->level;

  if(level == ZMAPCONTAINER_LEVEL_BLOCK)
    {
      ContainerType container_type = CONTAINER_INVALID ;

      container_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(block_container), CONTAINER_TYPE_KEY)) ;
      zMapAssert(container_type == CONTAINER_PARENT || container_type == CONTAINER_ROOT) ;

      test.strand = strand;
  
      zmapWindowContainerExecute(block_container,
                                 ZMAPCONTAINER_LEVEL_STRAND,
                                 getStrandLevelWithStrand, &test);

    }

  if(test.container != NULL)
    item = FOO_CANVAS_ITEM( test.container );

  return item;
}


/*
 * Destroys a container, so long as it's a ContainerType CONTAINER_PARENT or CONTAINER_ROOT.
 *
 * @param container_parent  A FooCanvasGroup to be destroyed
 * @return                  nothing
 * */

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

/*
 * Destroys all a container's children, so long as it's a ContainerType CONTAINER_FEATURES!
 * Unlike zmapWindowContainerDestroy, zmapWindowContainerPurge does not destroy itself.
 *
 * @param unknown_child   A FooCanvasGroup to empty.
 * @return                nothing
 * */
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
  FooCanvasItem *background ;

  container_data = g_object_get_data(G_OBJECT(object), CONTAINER_DATA) ;

  if(container_data->long_items)
    {
      background = zmapWindowContainerGetBackground(FOO_CANVAS_GROUP(object)) ;
      
      zmapWindowLongItemRemove(container_data->long_items, background) ;
    }

  g_free(container_data) ;

  return ;
}


/* Called for every container while descending.... */
static void eachContainer(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;
  ContainerRecursionData all_data = (ContainerRecursionData)user_data ;
  ContainerData container_data;
  ZMapContainerLevelType level = ZMAPCONTAINER_LEVEL_INVALID;
  FooCanvasGroup *children ;
  FooCanvasPoints *this_points = NULL, *parent_points = NULL;
  double *bound = NULL, spacing = 0.0;

  container_data = g_object_get_data(G_OBJECT(container), CONTAINER_DATA) ;
  children = zmapWindowContainerGetFeatures(container) ;

  zMapAssert(container_data);
  /* We need to get what we need in case someone destroys it under us. */
  level   = container_data->level;
  spacing = container_data->this_spacing;
  
  zMapAssert(level >  ZMAPCONTAINER_LEVEL_INVALID && 
             level <= ZMAPCONTAINER_LEVEL_FEATURESET);

  containerPointsCacheGetPoints(all_data->cache, level, 
                                &this_points, &parent_points, &bound);

  /* Execute pre-recursion function. */
  if(all_data->container_enter_cb)
    (all_data->container_enter_cb)(container, this_points, all_data->container_enter_data);

  /* Recurse ? */
  if (children)
    {
      if (all_data->stop != level)
	{
	  g_list_foreach(children->item_list, eachContainer, user_data) ;
	}
    }

  /* If we're redrawing then we need to do extra work... */
  if (all_data->redraw_during_recursion)
    {
      switch(level)
        {
        case ZMAPCONTAINER_LEVEL_ROOT:
          containerMoveToZero(container);
          containerSetMaxBackground(container, this_points, container_data);
          containerPointsCacheResetBound(all_data->cache, ZMAPCONTAINER_LEVEL_ALIGN);
          break;
        case ZMAPCONTAINER_LEVEL_ALIGN:
          containerMoveToZero(container);
          containerSetMaxBackground(container, this_points, container_data);
          containerPointsCacheResetBound(all_data->cache, ZMAPCONTAINER_LEVEL_BLOCK);
          break;
        case ZMAPCONTAINER_LEVEL_BLOCK:
          containerMoveToZero(container);
          containerSetMaxBackground(container, this_points, container_data);
          containerPointsCacheResetBound(all_data->cache, ZMAPCONTAINER_LEVEL_STRAND);
          break;
        case ZMAPCONTAINER_LEVEL_STRAND:
          containerMoveToZero(container);
          
          /* We only need to do this here, but adds weight to the arguement for having a border. */
          this_points->coords[2] += container_data->child_spacing;
          
          containerSetMaxBackground(container, this_points, container_data);
          containerPointsCacheResetBound(all_data->cache, ZMAPCONTAINER_LEVEL_FEATURESET);
          break;
        case ZMAPCONTAINER_LEVEL_FEATURESET:
          /* If this featureset requires a redraw... */
          if (children && container_data->child_redraw_required)
            {
              redrawColumn(FOO_CANVAS_ITEM(container), container_data);
            }
          
          containerMoveToZero(container);
          foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(container),
                                     &(this_points->coords[0]),
                                     &(this_points->coords[1]),
                                     &(this_points->coords[2]),
                                     &(this_points->coords[3]));
          break;
        case ZMAPCONTAINER_LEVEL_INVALID:
        default:
          break;
        }
      
      /* we need to shift the containers in the X axis. 
       * This should probably be done by a callback, but it's nice to have it contained here. */
      containerXAxisMove(container, this_points, level, spacing, bound);
      /* propogate the points up to the next level. */
      containerPropogatePoints(this_points, parent_points);
    }

  /* Execute post-recursion function. */
  if(all_data->container_leave_cb)
    (all_data->container_leave_cb)(container, this_points, all_data->container_leave_data);

  containerPointsCacheResetPoints(all_data->cache, level);

  return ;
}



/*! g_list_foreach function to remove items */
static void itemDestroyCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;

  gtk_object_destroy(GTK_OBJECT(item));

  return ;
}


static void printlevel(FooCanvasGroup *container_parent, FooCanvasPoints *points, gpointer user_data)
{
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

static void redrawColumn(FooCanvasItem *container, ContainerData data)
{
  zmapWindowContainerZoomChangedCallback redraw_cb = NULL;

  zMapAssert(data->child_redraw_required);

  if((redraw_cb = g_object_get_data(G_OBJECT(container), CONTAINER_REDRAW_CALLBACK )))
    {
      ZMapWindow window = NULL;

      window = (ZMapWindow)g_object_get_data(G_OBJECT(container), CONTAINER_REDRAW_DATA);

      /* do the callback.... */
      redraw_cb(container, zMapWindowGetZoomFactor(window), window) ;
    }

  return ;
}



static void printFeatureSet(gpointer data, gpointer user_data)
{
  FooCanvasItem *col_parent = (FooCanvasItem *)data ;
  ZMapFeatureSet feature_set ;


  feature_set = g_object_get_data(G_OBJECT(col_parent), ITEM_FEATURE_DATA) ;

  printf("col_name: %s\n", g_quark_to_string(feature_set->unique_id)) ;

  return ;
}

static void containerPropogatePoints(FooCanvasPoints *from, FooCanvasPoints *to)
{
  if(!from || !to)
    return ;

  if(from->coords[0] < to->coords[0])
    to->coords[0] = from->coords[0];
  if(from->coords[1] < to->coords[1])
    to->coords[1] = from->coords[1];

  if(from->coords[2] > to->coords[2])
    to->coords[2] = from->coords[2];
  if(from->coords[3] > to->coords[3])
    to->coords[3] = from->coords[3];

  return ;
}

static void containerXAxisMove(FooCanvasGroup  *container, 
                               FooCanvasPoints *container_points,
                               ZMapContainerLevelType type,
                               double  this_level_spacing,
                               double *current_bound_inout)
{
  double x1, x2, xpos, dx;
  double target = 0.0;

  if(type != ZMAPCONTAINER_LEVEL_ROOT)
    {
      if(FOO_CANVAS_ITEM(container)->object.flags & FOO_CANVAS_ITEM_VISIBLE)
        {
          if(current_bound_inout)
            {
              target = *current_bound_inout;
            }

          /* Are we the first container in this level, hokey... */
          if(target == 0.0)
            {
              if(type == ZMAPCONTAINER_LEVEL_FEATURESET)
                target += this_level_spacing;
            }
          else
            target += this_level_spacing;

          xpos = container->xpos;
          x1 = container_points->coords[0];
          x2 = container_points->coords[2];

          /* how far off the mark are we? */
          dx = target - xpos - (x1 - xpos);

          /* move the distance to the mark */
          foo_canvas_item_move(FOO_CANVAS_ITEM(container), dx, 0.0);

          /* update the points. */
          container_points->coords[0] += dx; /* move by dx */
          container_points->coords[2] += dx; /* move by dx */

          /* update the current bound... */
          if(current_bound_inout)
            {
              *current_bound_inout = target + (x2 - x1);
            }
        }
    }

  return ;
}

static void containerMoveToZero(FooCanvasGroup *container)
{
  double target = 0.0, current = 0.0, dx = 0.0;

  current = container->xpos;
  dx      = target - current;
  foo_canvas_item_move(FOO_CANVAS_ITEM(container), dx, 0.0);

  return ;
}

static void containerSetMaxBackground(FooCanvasGroup *container, 
                                      FooCanvasPoints *this_points, 
                                      ContainerData container_data)
{
  double nx1, nx2, ny1, ny2;
  FooCanvasItem *bg = NULL;
  
  bg = zmapWindowContainerGetBackground(container);
  
  nx1 = ny1 = 0.0;
  nx2 = this_points->coords[2];
  ny2 = this_points->coords[3];
  foo_canvas_item_set(bg,
                      "x1", 0.0,
                      "y1", 0.0,
                      "x2", nx2,
                      "y2", ny2,
                      NULL);
  return ;
}

static ContainerPointsCache containerPointsCacheCreate(void)
{
  ContainerPointsCache cache = NULL;
  int num_points = 2, i = 0;

  if(!(cache = g_new0(ContainerPointsCacheStruct, 1)))
    {
      zMapAssertNotReached();
    }

  cache->root_points   = foo_canvas_points_new(num_points);
  cache->align_points  = foo_canvas_points_new(num_points);
  cache->block_points  = foo_canvas_points_new(num_points);
  cache->strand_points = foo_canvas_points_new(num_points);
  cache->set_points    = foo_canvas_points_new(num_points);

  for(i = ZMAPCONTAINER_LEVEL_ROOT; i <= ZMAPCONTAINER_LEVEL_FEATURESET; i++)
    {
      containerPointsCacheResetPoints(cache, i);
    }

  cache->root_bound   = 0.0;
  cache->align_bound  = 0.0;
  cache->block_bound  = 0.0;
  cache->strand_bound = 0.0;
  cache->set_bound    = 0.0;

  return cache;
}

static void containerPointsCacheDestroy(ContainerPointsCache cache)
{
  zMapAssert(cache);

  foo_canvas_points_free(cache->root_points);
  foo_canvas_points_free(cache->align_points);
  foo_canvas_points_free(cache->block_points);
  foo_canvas_points_free(cache->strand_points);
  foo_canvas_points_free(cache->set_points);

  g_free(cache);

  return ;
}

static void containerPointsCacheGetPoints(ContainerPointsCache cache, 
                                          ZMapContainerLevelType level,
                                          FooCanvasPoints **this_points_out,
                                          FooCanvasPoints **parent_points_out,
                                          double **level_current_bound)
{
  FooCanvasPoints *this = NULL, *parent = NULL;
  double *current_bound = NULL;
  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_ROOT:
      this   = cache->root_points;
      parent = NULL;
      current_bound = &(cache->root_bound);
      break;
    case ZMAPCONTAINER_LEVEL_ALIGN:
      this   = cache->align_points;
      parent = cache->root_points;
      current_bound = &(cache->align_bound);
      break;
    case ZMAPCONTAINER_LEVEL_BLOCK:
      this   = cache->block_points;
      parent = cache->align_points;
      current_bound = &(cache->block_bound);
      break;
    case ZMAPCONTAINER_LEVEL_STRAND:
      this   = cache->strand_points;
      parent = cache->block_points;
      current_bound = &(cache->strand_bound);
      break;
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      this   = cache->set_points;
      parent = cache->strand_points;
      current_bound = &(cache->set_bound);
      break;
    case ZMAPCONTAINER_LEVEL_INVALID:
    default:
      zMapAssertNotReached();
      break;
    }

  if(this_points_out)
    *this_points_out = this;

  if(parent_points_out)
    *parent_points_out = parent;

  if(level_current_bound)
    *level_current_bound = current_bound;

  return ;
}

static void containerPointsCacheResetPoints(ContainerPointsCache cache,
                                            ZMapContainerLevelType level)
{
  FooCanvasPoints *this = NULL;
  int i = 0;

  containerPointsCacheGetPoints(cache, level, &this, NULL, NULL);

  for(i = 0; i < this->num_points * 2; i++)
    {
      this->coords[i] = 0.0;
    }

  return ;
}
static void containerPointsCacheResetBound(ContainerPointsCache cache,
                                           ZMapContainerLevelType level)
{
  double *bound = NULL;

  containerPointsCacheGetPoints(cache, level, NULL, NULL, &bound);

  if(bound)
    *bound = 0.0;

  return ;
}
