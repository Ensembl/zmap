/*  File: zmapWindowContainer.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Last edited: Apr 23 09:22 2009 (rds)
 * Created: Wed Dec 21 12:32:25 2005 (edgrif)
 * CVS info:   $Id: zmapWindowContainer.c,v 1.55 2009-04-23 08:28:00 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsFoo.h>
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

#define CONTAINER_SEPARATOR_STRAND (ZMAPSTRAND_FORWARD << 4)

typedef enum
  {
    _CONTAINER_BACKGROUND_POSITION = 0,
    _CONTAINER_UNDERLAY_POSITION   = 1,
    _CONTAINER_FEATURES_POSITION   = 2,
    _CONTAINER_OVERLAY_POSITION    = 3,
  }ContainerPositions;

typedef struct
{
  ZMapContainerLevelType level ;

  /* A optimisation because we don't want to attach this information to every single feature.... */
  gboolean child_redraw_required ;

  /* I'd like to add a border here too..... */
  double child_spacing ;

  double this_spacing ;         /* ... to save time ... see eachContainer & containerXAxisMove */

  double height;

  /* For background item processing (we should have flags for background as well). */
  GdkColor orig_background ;				    /* We cash this as some containers
							       have their own special colours (e.g. 3 frame cols.). */
  gboolean has_bg_colour;

  GQueue *user_hidden_children;	   /* users want state remembered during zoom. */

  /* The long_item object, needed for keeping a record of any long items we create including the
   * background and overlays. */
  ZMapWindowLongItems long_items ;

  /* For overlay processing. */
  struct
  {
    unsigned int max_width : 1 ;			    /* maximise width of overlays. */
    unsigned int max_height : 1 ;			    /* maximise height of overlays. */
  } overlay_flags ;


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
} ContainerPointsCacheStruct, *ContainerPointsCache;


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
  GList *forward, *reverse;
}ForwardReverseColumnListsStruct, *ForwardReverseColumnLists;

typedef void (*ContainerWorldCoordsDataFunc)(FooCanvasGroup  *container_parent,
                                             FooCanvasPoints *world_coords,
                                             ContainerData    container_data);
typedef FooCanvasGroup * (*ContainerGetSubGroup)(FooCanvasGroup *conatiner_parent);

static void containerDestroyCB(GtkObject *object, gpointer user_data) ;

static void eachContainer(gpointer data, gpointer user_data) ;

static void printlevel(FooCanvasGroup *container_parent, FooCanvasPoints *points,
                       ZMapContainerLevelType level, gpointer user_data);
static void printItem(FooCanvasItem *item, int indent, char *prefix) ;

static void itemDestroyCB(gpointer data, gpointer user_data);

static void redrawColumn(FooCanvasGroup *container, FooCanvasPoints *points_inout, ContainerData data);

static void printFeatureSet(gpointer data, gpointer user_data) ;
static void printAnyFeatName(gpointer data, gpointer user_data) ;

static void propogate_points(FooCanvasPoints *from, FooCanvasPoints *to);
static void shift_container_to_target(FooCanvasGroup  *container, 
                               FooCanvasPoints *container_points,
                               ZMapContainerLevelType type,
                               double  this_level_spacing,
                               double *current_bound_inout);
static void container_reset_child_positions(FooCanvasGroup *container);

static void maximise_container_background(FooCanvasGroup *container, 
                                      FooCanvasPoints *this_points, 
                                      ContainerData container_data);
static void maximise_container_overlays(FooCanvasGroup *container, FooCanvasPoints *this_points, 
				    ContainerData container_data) ;
static void set_column_lists_cb(FooCanvasGroup *container, FooCanvasPoints *points, 
				ZMapContainerLevelType level, gpointer user_data);

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
static gboolean containerRootInvokeContainerBGEvent(FooCanvasItem *item, GdkEvent *event, gpointer data);

inline static GObject *containerGObject(FooCanvasGroup *container);

static gint comparePosition(gconstpointer a, gconstpointer b) ;


gboolean window_container_points_debug_G = FALSE;


/* Creates a "container" for our sequence features which consists of: 
 * 
 *                parent_group
 *              /     |        \
 *             /      |         \
 *            /       |          \
 *  background   sub_group of     sub_group of
 *      item     feature items    overlay items
 *
 * Note that this means that the the overlay items are above the feature items
 * which are above the background item.
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
  FooCanvasGroup *container_parent = NULL, *container_underlays,
    *container_overlays, *container_features, *parent_container;
  FooCanvasItem *container_background ;
  ContainerType parent_type = CONTAINER_INVALID, container_type ;
  ContainerData container_data, parent_data = NULL;
  double this_spacing = 0.0;    /* Only really true for the root */

  /* Parent has to be a group. */
  zMapAssert(FOO_IS_CANVAS_GROUP(parent)) ;

  parent_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(parent_type == CONTAINER_INVALID || 
             parent_type == CONTAINER_FEATURES ||
             parent_type == CONTAINER_PARENT) ;

  /* Is the parent a container itself, if not then make this the root container. */
  /* Adding code to be a little more intelligent about what this function can accept. */
  if (parent_type == CONTAINER_FEATURES)
    {
      container_type   = CONTAINER_PARENT ;
      parent_container = zmapWindowContainerGetParent(FOO_CANVAS_ITEM( parent ));
      parent_data      = g_object_get_data(G_OBJECT(parent_container), CONTAINER_DATA);
      level            = parent_data->level + 1;
      this_spacing     = parent_data->child_spacing;
    }
  else if (parent_type == CONTAINER_PARENT)
    {

      /* HAVING TALKED TO ROY, NEITHER OF US KNOW WHEN THIS CAN HAPPEN...... */

      zMapAssertNotReached() ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      container_type = CONTAINER_PARENT;
      parent_data    = g_object_get_data(G_OBJECT(parent), CONTAINER_DATA);
      level          = parent_data->level + 1;
      this_spacing   = parent_data->child_spacing;
      parent         = zmapWindowContainerGetFeatures(parent);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }
  else
    {
      container_type = CONTAINER_ROOT ;
      level = ZMAPCONTAINER_LEVEL_ROOT;
    }

  zMapAssert(level >  ZMAPCONTAINER_LEVEL_INVALID &&
             level <= ZMAPCONTAINER_LEVEL_FEATURESET);

  container_parent = FOO_CANVAS_GROUP(foo_canvas_item_new(parent,
							  foo_canvas_group_get_type(),
							  NULL)) ;

  g_signal_connect(G_OBJECT(container_parent), "event", 
                   G_CALLBACK(containerRootInvokeContainerBGEvent), NULL);

  container_data = g_new0(ContainerDataStruct, 1) ;
  container_data->level                 = level;
  container_data->child_spacing         = child_spacing ;
  container_data->this_spacing          = this_spacing ;
  container_data->child_redraw_required = FALSE ;
  if(background_fill_colour)
    {
      container_data->orig_background   = *background_fill_colour ; /* n.b. struct copy. */
      container_data->has_bg_colour     = TRUE;
    }
  container_data->long_items            = long_items ;

  g_object_set_data(G_OBJECT(container_parent), CONTAINER_DATA, container_data) ;
  g_object_set_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY,
		    GINT_TO_POINTER(container_type)) ;
  g_signal_connect(G_OBJECT(container_parent), "destroy", G_CALLBACK(containerDestroyCB), NULL) ;

  /* Add a background item to the container parent, before the groups we're going to add */
  /* This is the FIRST item (and MUST be) in the parents list of items. */
  /* We don't use the border colour at the moment but we may wish to later.... */
  container_background = zMapDrawBoxSolid(container_parent,
					  0.0, 0.0, 0.0, 0.0, background_fill_colour) ;
  g_object_set_data(G_OBJECT(container_background), CONTAINER_TYPE_KEY,
		    GINT_TO_POINTER(CONTAINER_BACKGROUND)) ;

  /* Now to add the groups which contain other items later in the drawing code */

  /* First we add an underlay group to mirror the overlay group we'll add later */
  container_underlays = FOO_CANVAS_GROUP(foo_canvas_item_new(container_parent,
                                                             foo_canvas_group_get_type(),
                                                             NULL)) ;
  g_object_set_data(G_OBJECT(container_underlays), CONTAINER_TYPE_KEY,
		    GINT_TO_POINTER(CONTAINER_UNDERLAYS)) ;

  /* Second we add a group for the features.  This is where all the ZMapFeatures go. */
  container_features = FOO_CANVAS_GROUP(foo_canvas_item_new(container_parent,
							    foo_canvas_group_get_type(),
							    NULL)) ;
  g_object_set_data(G_OBJECT(container_features), CONTAINER_TYPE_KEY,
		    GINT_TO_POINTER(CONTAINER_FEATURES)) ;

  /* Thirdly add the group for the overlays.  This later contains items such as the mark */
  container_overlays = FOO_CANVAS_GROUP(foo_canvas_item_new(container_parent,
							    foo_canvas_group_get_type(),
							    NULL)) ;
  g_object_set_data(G_OBJECT(container_overlays), CONTAINER_TYPE_KEY,
		    GINT_TO_POINTER(CONTAINER_OVERLAYS)) ;

  /* Althouigh we add the background to the parent first I've left
   * this call, together with its comment, in. */
  /* Note that we rely on the background being first in parent groups item list which we
   * we achieve with this lower command. This is both so that the background appears _behind_
   * the objects that are children of the subgroup and so that we can return the background
   * and subgroup items correctly. */
  foo_canvas_item_lower_to_bottom(container_background) ; 

  return container_parent ;
}



/* Check whether a given FooCanvasGroup is a valid container. */
gboolean zmapWindowContainerIsValid(FooCanvasGroup *any_group)
{
  gboolean valid = FALSE ;
  ContainerType container_type ;
  ContainerData container_data ;

  if (((container_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(any_group), CONTAINER_TYPE_KEY)))
       == CONTAINER_PARENT)
      && (container_data = g_object_get_data(G_OBJECT(any_group), CONTAINER_DATA)))
    {
      valid = TRUE ;
    }

  return valid ;
}


/* If the item is a valid container then it is returned, otherwise its parent(s) are tested
 * until a valid container is found or the root of the canvas is reached. */
FooCanvasGroup *zmapWindowContainerGetFromItem(FooCanvasItem *any_item)
{
  FooCanvasGroup *container = NULL ;
  gboolean result ;

  while (!(result = zmapWindowContainerIsValid((FooCanvasGroup *)any_item)))
    {
      if (!(any_item = any_item->parent))
	break ;
    }

  if (result)
    container = (FooCanvasGroup *)any_item ;

  return container ;
}


/* Return If the item is a valid container then it is returned, otherwise its parent(s) are tested
 * until a valid container is found or the root of the canvas is reached. */
FooCanvasGroup *zmapWindowContainerGetParentLevel(FooCanvasItem *any_item, ZMapContainerLevelType level)
{
  FooCanvasGroup *container = NULL ;
  ZMapContainerLevelType curr_level ;

  if ((container = zmapWindowContainerGetFromItem(any_item)))
    {
      while ((curr_level = zmapWindowContainerGetLevel(container)) != level)
	{
	  if (!(container = zmapWindowContainerGetSuperGroup(container)))
	    break ;
	}
    }
    

  return container ;
}



/* Currently this function only works with columns, but the intention
 * is that it could work with blocks and aligns too at some later
 * point in time... */

/* In zmapWindowItemFeatureSet there is similar code to this to set
 * visibility of features.  _IF_ features were containers we _might_
 * use this same function.  However they aren't at the moment and it
 * might be worth it staying that way... just a thought (RDS)... */
gboolean zmapWindowContainerSetVisibility(FooCanvasGroup *container_parent, gboolean visible)
{
  ContainerType container_type;
  ContainerData container_data, strand_data;
  FooCanvasGroup *strand_parent;
  gboolean setable = FALSE;     /* Most columns aren't */

  /* We make sure that the container_parent is 
   * - A parent
   * - Has Container Data
   * - Is a featureset... This is probably a limit too far.
   */

  if (((container_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), 
                                                           CONTAINER_TYPE_KEY))) == CONTAINER_PARENT)
      && (container_data = g_object_get_data(G_OBJECT(container_parent), CONTAINER_DATA))
      && (container_data->level == ZMAPCONTAINER_LEVEL_FEATURESET)
      && (strand_parent = zmapWindowContainerGetSuperGroup(container_parent))
      && (strand_data   = g_object_get_data(G_OBJECT(strand_parent), CONTAINER_DATA)))
    {
      if (visible)
	{
	  foo_canvas_item_show(FOO_CANVAS_ITEM(container_parent)) ;
	}
      else
	{
	  foo_canvas_item_hide(FOO_CANVAS_ITEM(container_parent)) ;
	}

      setable = TRUE ;
    }

  return setable ;
}



/* This is weak and immature ATM, but better than NO type checking for the callback as it was! */
void zmapWindowContainerSetZoomEventHandler(FooCanvasGroup *featureset_container,
                                            zmapWindowContainerZoomChangedCallback handler_cb,
                                            gpointer user_data,
                                            GDestroyNotify data_destroy_notify)
{
  ContainerData container_data = NULL;
  ContainerType container_type = CONTAINER_INVALID;

  if (((container_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(featureset_container), 
                                                          CONTAINER_TYPE_KEY))) == CONTAINER_PARENT)
      && (container_data = g_object_get_data(G_OBJECT(featureset_container), CONTAINER_DATA))
      && container_data->level == ZMAPCONTAINER_LEVEL_FEATURESET)
    {
      zmapWindowContainerSetChildRedrawRequired(featureset_container, TRUE) ;
      g_object_set_data(G_OBJECT(featureset_container), CONTAINER_REDRAW_CALLBACK,
                        handler_cb) ;
      g_object_set_data_full(G_OBJECT(featureset_container), CONTAINER_REDRAW_DATA,
                             (gpointer)user_data, data_destroy_notify) ;
    }
  else
    zMapAssertNotReached();

  return ;
}

gboolean zmapWindowContainerIsChildRedrawRequired(FooCanvasGroup *container_parent)
{
  ContainerType type = CONTAINER_INVALID ;
  ContainerData container_data ;
  gboolean redraw_required = FALSE;

  zMapAssert(container_parent) ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT) ;

  container_data = g_object_get_data(G_OBJECT(container_parent), CONTAINER_DATA) ;

  redraw_required = container_data->child_redraw_required;

  return redraw_required;
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

gboolean zmapWindowContainerIsStrandSeparator(FooCanvasGroup *container)
{
  ZMapStrand strand;
  gboolean is = FALSE;
  
  strand = zmapWindowContainerGetStrand(container);

  is = (gboolean)(strand == CONTAINER_SEPARATOR_STRAND);
  
  return is;
}

void zmapWindowContainerSetAsStrandSeparator(FooCanvasGroup *container)
{
  zmapWindowContainerSetStrand(container, (ZMapStrand)CONTAINER_SEPARATOR_STRAND);

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
  GList *item_list;
  int max = 3, i, llength;

  container_data = g_object_get_data(G_OBJECT(strand_parent), CONTAINER_DATA);
  container_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(strand_parent), CONTAINER_TYPE_KEY));

  zMapAssert((container_data && 
	      container_data->level == ZMAPCONTAINER_LEVEL_BLOCK)
	     && container_type == CONTAINER_PARENT) ;

  block_features = zmapWindowContainerGetFeatures(strand_parent) ;
  item_list      = block_features->item_list;
  llength        = g_list_length(item_list) ;

  zMapAssert(llength <= max && llength > 0);

  for(i = 0; i < max && item_list; i++)
    {
      strand_group = FOO_CANVAS_GROUP(item_list->data) ;
      group_strand = zmapWindowContainerGetStrand(strand_group) ;
      if(group_strand == strand)
	break;
      else
	strand_group = NULL;
      item_list = item_list->next;
    }

  return strand_group ;
}

FooCanvasGroup *zmapWindowContainerGetStrandSeparatorGroup(FooCanvasGroup *strand_parent)
{
  FooCanvasGroup *strand_group = NULL ;

  strand_group = zmapWindowContainerGetStrandGroup(strand_parent, (ZMapStrand)CONTAINER_SEPARATOR_STRAND);

  return strand_group;
}

gboolean zmapWindowContainerBlockGetColumnLists(FooCanvasGroup *column_container,
						GList **forward_columns_out,
						GList **reverse_columns_out)
{
  FooCanvasGroup *block_container;
  gboolean found_block = FALSE;	/* about all we gonna do. */

  if((column_container) && 
     (block_container = zmapWindowContainerGetParentLevel(FOO_CANVAS_ITEM(column_container), 
							  ZMAPCONTAINER_LEVEL_BLOCK)))
    {
      ForwardReverseColumnListsStruct for_rev_lists = {NULL};

      zmapWindowContainerExecute(block_container, 
				 ZMAPCONTAINER_LEVEL_FEATURESET,
				 set_column_lists_cb, &for_rev_lists);

      /* Return the column lists. */
      if(forward_columns_out)
	*forward_columns_out = for_rev_lists.forward;
      if(reverse_columns_out)
	*reverse_columns_out = for_rev_lists.reverse;
    }

  return found_block;
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
    case CONTAINER_OVERLAYS:
    case CONTAINER_UNDERLAYS:
    case CONTAINER_FEATURES:
    case CONTAINER_BACKGROUND:
      container_parent = FOO_CANVAS_GROUP(unknown_child->parent) ;
      break ;
    case CONTAINER_PARENT:
    case CONTAINER_ROOT:
      container_parent = FOO_CANVAS_GROUP(unknown_child);
      break;
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


/* Return the background of the container which is used both to give colour to the
 * container and to allow event interception across the complete bounding rectangle
 * of the container. */
FooCanvasItem *zmapWindowContainerGetBackground(FooCanvasGroup *container_parent)
{
  FooCanvasItem *container_background ;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  container_background = FOO_CANVAS_ITEM((g_list_nth(container_parent->item_list, _CONTAINER_BACKGROUND_POSITION))->data) ;
  zMapAssert(FOO_IS_CANVAS_RE(container_background)) ;

  return container_background ;
}

/* Return the sub group of the container that contains all of the "overlays" where
 * overlays might be text, rectangles etc. */
FooCanvasGroup *zmapWindowContainerGetUnderlays(FooCanvasGroup *container_parent)
{
  FooCanvasGroup *container_overlays ;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  container_overlays = FOO_CANVAS_GROUP((g_list_nth(container_parent->item_list, _CONTAINER_UNDERLAY_POSITION))->data) ;

  return container_overlays ;
}

/* Return the sub group of the container that contains all of the "features" where
 * features might be columns, column sets, blocks etc. */
FooCanvasGroup *zmapWindowContainerGetFeatures(FooCanvasGroup *container_parent)
{
  FooCanvasGroup *container_features ;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  container_features = FOO_CANVAS_GROUP((g_list_nth(container_parent->item_list, _CONTAINER_FEATURES_POSITION))->data) ;

  return container_features ;
}


/* Returns the nth item in the container (0-based), there are two special values for
 * nth_item for the first and last items:  ZMAPCONTAINER_ITEM_FIRST, ZMAPCONTAINER_ITEM_LAST */
FooCanvasItem *zmapWindowContainerGetNthFeatureItem(FooCanvasGroup *container, int nth_item)
{
  FooCanvasItem *item = NULL ;
  FooCanvasGroup *features ;

  features = zmapWindowContainerGetFeatures(container) ;

  if (nth_item == 0 || nth_item == ZMAPCONTAINER_ITEM_FIRST)
    item = FOO_CANVAS_ITEM(features->item_list->data) ;
  else if (nth_item == ZMAPCONTAINER_ITEM_LAST)

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    item = FOO_CANVAS_ITEM((g_list_last(features->item_list))->data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    item = FOO_CANVAS_ITEM(features->item_list_end->data) ;
  else
    item = FOO_CANVAS_ITEM((g_list_nth(features->item_list, nth_item))->data) ;

  return item ;
}


/* Given any item that is a direct child of a column group (e.g. not a subfeature), returns
 * the previous or next item that optionally satisfies item_test_func_cb(). The function skips
 * over items that fail these tests.
 * 
 * direction controls which way the item list is processed and if wrap is TRUE then processing
 * wraps around on reaching the end of the list.
 * 
 * If no item can be found then the original will be returned, note that if item_test_func_cb()
 * was specified and the original item does not satisfy item_test_func_cb() then NULL is returned.
 * 
 *  */
FooCanvasItem *zmapWindowContainerGetNextFeatureItem(FooCanvasItem *orig_item,
						     ZMapContainerItemDirection direction, gboolean wrap,
						     zmapWindowContainerItemTestCallback item_test_func_cb,
						     gpointer user_data)
{
  FooCanvasItem *item = NULL ;
  FooCanvasGroup *parent, *features ;
  ContainerType type = CONTAINER_INVALID ;
  GList *feature_ptr ;
  FooCanvasItem *found_item = NULL ;

  features = FOO_CANVAS_GROUP(orig_item->parent) ;
  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(features), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_FEATURES) ;

  parent = zmapWindowContainerGetParent(FOO_CANVAS_ITEM(features)) ;

  feature_ptr = zmapWindowContainerFindItemInList(parent, orig_item) ;

  while (feature_ptr && !item && found_item != orig_item)
    {
      if (direction == ZMAPCONTAINER_ITEM_NEXT)
	{
	  feature_ptr = g_list_next(feature_ptr) ;

	  if (!feature_ptr && wrap)
	    feature_ptr = g_list_first(features->item_list) ;
	}
      else
	{
	  feature_ptr = g_list_previous(feature_ptr) ;
	
	  if (!feature_ptr && wrap)
	    feature_ptr = g_list_last(features->item_list) ;
	}

      /* If wrap is FALSE then feature_ptr can be NULL */
      if (feature_ptr)
	{
	  found_item = (FooCanvasItem *)(feature_ptr->data) ;

	  if (!item_test_func_cb || item_test_func_cb(found_item, user_data))
	    item = found_item ;
	}
    }

  return item ;
}



/* Return the sub group of the container that contains all of the "overlays" where
 * overlays might be text, rectangles etc. */
FooCanvasGroup *zmapWindowContainerGetOverlays(FooCanvasGroup *container_parent)
{
  FooCanvasGroup *container_overlays ;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  container_overlays = FOO_CANVAS_GROUP((g_list_nth(container_parent->item_list, _CONTAINER_OVERLAY_POSITION))->data) ;

  return container_overlays ;
}

/* Set overlay resizing policy. */
void zmapWindowContainerSetOverlayResizing(FooCanvasGroup *container_parent,
					   gboolean maximise_width, gboolean maximise_height)
{
  ContainerType type = CONTAINER_INVALID ;
  ContainerData container_data = NULL;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  container_data = g_object_get_data(G_OBJECT(container_parent), CONTAINER_DATA);
  zMapAssert(container_data) ;

  container_data->overlay_flags.max_width = maximise_width ;
  container_data->overlay_flags.max_height = maximise_height ;

  return ;
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

  style = NULL;

  return style ;
}



/* Setting the background size is not completely straight forward because we always
 * want the height of the background to be the full height of the group (e.g. column)
 * but we want the width to be set from the horizontal extent of the features.
 *
 * The width/height of the background can be given, if either is 0.0 then the width/height
 * is taken from the child group.
 */

/* Should I set the overlay size here as well...perhaps..... */

/* THIS NEEDS TO DO A BORDER WIDTH AS WELL.... */

void zmapWindowContainerSetBackgroundSize(FooCanvasGroup *container_parent,
					  double height)
{
  FooCanvasGroup *container_features, *container_overlays ;
  FooCanvasItem *container_background ;
  /* double x1, x2; */
  double y1, y2 ;
  ContainerType type = CONTAINER_INVALID ;
  ContainerData container_data = NULL;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  zMapAssert(height >= 0.0) ;

  container_background = zmapWindowContainerGetBackground(container_parent) ;

  container_features = zmapWindowContainerGetFeatures(container_parent) ;

  container_overlays = zmapWindowContainerGetOverlays(container_parent) ;

  if((container_data = g_object_get_data(G_OBJECT(container_parent), CONTAINER_DATA)))
    {
      /* Either the caller sets the height or we get the height from the main group.
       * We do this because features may cover only part of the range of the group, e.g. transcripts
       * in a column, and we need the background to cover the whole range to allow us to get
       * mouse clicks etc. */
      if (height != 0.0)
        {
          y1 = 0 ;
          y2 = height ;
          container_data->height = y2;
        }
      else
        {
          zMapLogWarning("%s", "Background size should be set manually, not attempted automatically...");
          foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(container_parent), NULL, &y1, NULL, &y2) ;
          zmapWindowExt2Zero(&y1, &y2) ;
          container_data->height = y2;
        }

#ifdef NOT_SURE_THIS_SHOULD_BE_HERE_EVEN      
      /* Get the width from the child group */
      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(container_features), &x1, NULL, &x2, NULL) ;
      
      foo_canvas_item_set(container_background,
                          "x1", x1,
                          "y1", y1,
                          "x2", x2,
                          "y2", y2,
                          NULL) ;
#endif
      
      if(container_data->long_items)
        {
          zmapWindowLongItemCheck(container_data->long_items, container_background, y1, y2);
        }
    }

  /* Should long item check overlays here.... */


  return ;
}


void zmapWindowContainerSetBackgroundSizePlusBorder(FooCanvasGroup *container_parent,
                                                    double height, double border)
{
#ifdef NEED_ZMAPWINDOWCONTAINERSETBACKGROUNDSIZEPLUSBORDER
  FooCanvasItem *container_background = NULL;
  double x1, x2, size, two_x;

  zmapWindowContainerSetBackgroundSize(container_parent, height);

  container_background = zmapWindowContainerGetBackground(container_parent) ;

  foo_canvas_item_get_bounds(container_background, &x1, NULL, &x2, NULL) ;

  two_x = border * 2.0;
  foo_canvas_item_set(container_background,
                      "x1", x1,
                      "x2", x2 + two_x,
                      NULL) ;
  border *= -1.0;

  if((size = ((x2 + two_x) - x1)) > (double)(1 << 15))
    zMapLogWarning("%s [%d < %f]", "Container background larger than 1 << 15 in x coords.", 1 << 15, size);

  /* Move background over to left by "border" units. */
  foo_canvas_item_move(container_background, border, 0.0) ;
#endif /* NEED_ZMAPWINDOWCONTAINERSETBACKGROUNDSIZEPLUSBORDER */

  return ;
}


/* Much like zmapWindowContainerSetBackgroundSize() but sets the background as big as the
 * parent in width _and_ height. */
void zmapWindowContainerMaximiseBackground(FooCanvasGroup *container_parent)
{
#ifdef NEED_ZMAPWINDOWCONTAINERMAXIMISEBACKGROUND
  FooCanvasItem *container_background ;
  double x1, y1, x2, y2, size ;
  ContainerType type = CONTAINER_INVALID ;
  ContainerData container_data = NULL;

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

  if((size = (x2 - x1)) > (double)(1 << 15))
    zMapLogWarning("%s [%d < %f]", "Container background larger than 1 << 15 in x coords.", 1 << 15, size);

  if((container_data = g_object_get_data(G_OBJECT(container_parent), CONTAINER_DATA)) 
     && container_data->long_items)
    {
      zmapWindowLongItemCheck(container_data->long_items, container_background, y1, y2);
    }

#endif /* NEED_ZMAPWINDOWCONTAINERMAXIMISEBACKGROUND */

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


GdkColor *zmapWindowContainerGetBackgroundColour(FooCanvasGroup *container_parent)
{
  GdkColor *background_colour = NULL ;
  ContainerType type ;

  zMapAssert(FOO_IS_CANVAS_GROUP(container_parent)) ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  background_colour = g_object_get_data(G_OBJECT(container_parent), "fill_color_gdk") ;

  return background_colour ;
}


void zmapWindowContainerResetBackgroundColour(FooCanvasGroup *container_parent)
{
  FooCanvasItem *container_background ;
  ContainerType type ;
  ContainerData container_data ;
  GdkColor *bg_colour = NULL;

  zMapAssert(FOO_IS_CANVAS_GROUP(container_parent)) ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  container_data = g_object_get_data(G_OBJECT(container_parent), CONTAINER_DATA) ;

  container_background = zmapWindowContainerGetBackground(container_parent) ;

  if(container_data->has_bg_colour)
    bg_colour = &(container_data->orig_background);

  foo_canvas_item_set(container_background,
                      "fill_color_gdk", bg_colour,
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


/* Is the group the parent of a container ? */
gboolean zmapWindowContainerIsParent(FooCanvasGroup *container_parent)
{
  gboolean result = FALSE ;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;

  if (type == CONTAINER_PARENT || type == CONTAINER_ROOT)
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


void zmapWindowContainerPrintFeatures(FooCanvasGroup *container_parent)
{

  FooCanvasGroup *container_features ;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  container_features = zmapWindowContainerGetFeatures(container_parent) ;

  if (container_features->item_list)
    g_list_foreach(container_features->item_list, printAnyFeatName, NULL) ;

  return ;
}




/* Sort the features in a container by either vertical or horizontal position. */
void zmapWindowContainerSortFeatures(FooCanvasGroup *container_parent, ZMapContainerOrientationType direction)
{
  FooCanvasGroup *container_features ;
  ContainerType type = CONTAINER_INVALID ;
  GCompareFunc sort_func ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  container_features = FOO_CANVAS_GROUP((g_list_nth(container_parent->item_list, _CONTAINER_FEATURES_POSITION))->data) ;

  if (direction == ZMAPCONTAINER_VERTICAL)
    sort_func = comparePosition ;
  else
    sort_func = comparePosition ;

  zMap_foo_canvas_sort_items(container_features, comparePosition) ;

  return ;
}


/* Get the index of an item in the feature list. */
int zmapWindowContainerGetItemPosition(FooCanvasGroup *container_parent, FooCanvasItem *item)
{
  int index = 0 ;
  FooCanvasGroup *container_features ;
  ContainerType type = CONTAINER_INVALID ;
  GList *list_item ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  /* Note that we need to check if the feature list can be found, if this function is called
   * from an item destroy routine then the feature list may already have gone. */
  if ((list_item = g_list_nth(container_parent->item_list, _CONTAINER_FEATURES_POSITION)))
    {
      container_features = FOO_CANVAS_GROUP(list_item->data) ;

      index = zMap_foo_canvas_find_item(container_features, item) ;
    }

  return index ;
}


void zmapWindowContainerSetItemPosition(FooCanvasGroup *container_parent, FooCanvasItem *item, int position)
{
  int index ;
  FooCanvasGroup *container_features ;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  container_features = FOO_CANVAS_GROUP((g_list_nth(container_parent->item_list, _CONTAINER_FEATURES_POSITION))->data) ;

  index = zMap_foo_canvas_find_item(container_features, item) ;

  if (position > index)
    foo_canvas_item_raise(item, (position - index)) ;
  else if (position < index)
    foo_canvas_item_lower(item, (index - position)) ;

  return ;
}


/* Get the index of an item in the feature list. */
GList *zmapWindowContainerFindItemInList(FooCanvasGroup *container_parent, FooCanvasItem *item)
{
  GList *list_item = NULL ;
  FooCanvasGroup *container_features ;
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container_parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  container_features = FOO_CANVAS_GROUP((g_list_nth(container_parent->item_list, _CONTAINER_FEATURES_POSITION))->data) ;

  list_item = zMap_foo_canvas_find_list_item(container_features, item) ;

  return list_item ;
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
  ContainerData container_data = NULL;
  ContainerType        type = CONTAINER_INVALID ;

  zMapAssert(stop_at_type >= ZMAPCONTAINER_LEVEL_ROOT ||
	     stop_at_type <= ZMAPCONTAINER_LEVEL_FEATURESET) ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(parent), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_ROOT || type == CONTAINER_PARENT);

  container_data = g_object_get_data(G_OBJECT(parent), CONTAINER_DATA) ;

  data.stop                    = stop_at_type;
  data.redraw_during_recursion = redraw_during_recursion;

  data.container_enter_cb   = container_enter_cb;
  data.container_enter_data = container_enter_data;
  data.container_leave_cb   = container_leave_cb;
  data.container_leave_data = container_leave_data;

  data.cache = containerPointsCacheCreate();

  if(redraw_during_recursion &&
     container_data->long_items)
    zmapWindowLongItemPushInterruption(container_data->long_items);

  eachContainer((gpointer)parent, &data) ;

  if(redraw_during_recursion && 
     container_data->long_items)
    zmapWindowLongItemPopInterruption(container_data->long_items);

  containerPointsCacheDestroy(data.cache);

  /* RDS_CANT_CROP_LONG_ITEMS_HERE
   * This seems like a good idea, but is flawed in a number of ways, here's 2:
   * 1) The scroll region can move after this call... needs to be done again.
   * 2) The scroll region results in unclamped coords meaning extra unneccessary
   *    calls to the guts in LongItemCrop.
   */

  return ;
}

#ifdef NEED_ZMAPWINDOWCONTAINERREPOSITION
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
#endif

FooCanvasGroup *zmapWindowContainerGetFeaturesContainerFromItem(FooCanvasItem *feature_item)
{
  FooCanvasGroup *features_container = NULL ;
  ZMapWindowItemFeatureType item_feature_type ;
  ContainerType type = CONTAINER_INVALID ;
  FooCanvasItem *parent_item = NULL ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(feature_item),
                                                        ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type != ITEM_FEATURE_INVALID) ;
  
  if (item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT)
    {
      parent_item = feature_item ;
    }
  else
    {
      parent_item = feature_item->parent ;
    }

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(parent_item),
                                                        ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE ||
             item_feature_type == ITEM_FEATURE_PARENT) ;
  
  /* It's possible for us to be called when we have no parent, e.g. when this routine is
   * called as a result of a GtkDestroy on one of our parents. */
  if (parent_item->parent)
    {
      features_container = FOO_CANVAS_GROUP(parent_item->parent);

      type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(features_container), CONTAINER_TYPE_KEY)) ;

      zMapAssert(type == CONTAINER_FEATURES) ;
    }

  return features_container ;
}

FooCanvasGroup *zmapWindowContainerGetParentContainerFromItem(FooCanvasItem *feature_item)
{
  FooCanvasGroup *features_container = NULL, *parent_container = NULL;

  if((features_container = zmapWindowContainerGetFeaturesContainerFromItem(feature_item)))
    {
      parent_container = zmapWindowContainerGetParent(FOO_CANVAS_ITEM(features_container));
      zMapAssert(parent_container) ;
    }

  return parent_container;
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
 * Destroys all a container's children, so long as it's a ContainerType CONTAINER_FEATURES, 
 * CONTAINER_OVERLAYS or CONTAINER_UNDERLAYS
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
    case CONTAINER_OVERLAYS:
    case CONTAINER_UNDERLAYS:
      g_list_foreach(unknown_child->item_list, itemDestroyCB, NULL);
      break ;
    default:
      zMapAssertNotReached() ;
    }
  
  return ;
}


void zmapWindowContainerSetData(FooCanvasGroup *container, const gchar *key, gpointer data)
{
  g_object_set_data(containerGObject(container), key, data);

  return ;
}

gpointer zmapWindowContainerGetData(FooCanvasGroup *container, const gchar *key)
{
  return g_object_get_data(containerGObject(container), key);
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

  if(container_data->user_hidden_children)
    {
      g_queue_free(container_data->user_hidden_children);
      container_data->user_hidden_children = NULL;
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
    (all_data->container_enter_cb)(container, this_points, level, all_data->container_enter_data);

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
          container_reset_child_positions(container);
          maximise_container_background(container, this_points, container_data);
          containerPointsCacheResetBound(all_data->cache, ZMAPCONTAINER_LEVEL_ALIGN);
          break;
        case ZMAPCONTAINER_LEVEL_ALIGN:
          container_reset_child_positions(container);
          maximise_container_background(container, this_points, container_data);
          containerPointsCacheResetBound(all_data->cache, ZMAPCONTAINER_LEVEL_BLOCK);
          break;
        case ZMAPCONTAINER_LEVEL_BLOCK:
          container_reset_child_positions(container);
          maximise_container_background(container, this_points, container_data);


	  /* THIS SEEMS COMPLETELY WRONG, WE SHOULDN'T NEED TO ZOOM THE OVERLAYS AT ALL.... */
          /* Yes, but if we've redrawn a strand or column and the width has changed
           * then the width needs to change for the overlay...
           */

	  /* I'm trying this for my overlays.... */
          maximise_container_overlays(container, this_points, container_data);

          containerPointsCacheResetBound(all_data->cache, ZMAPCONTAINER_LEVEL_STRAND);
          break;
        case ZMAPCONTAINER_LEVEL_STRAND:
          container_reset_child_positions(container);
          
          /* We only need to do this here, but adds weight to the arguement for having a border. */
          this_points->coords[2] += container_data->child_spacing;

	  if(!children->item_list)
	    {
	      /* empty strand containers behave badly */
	      this_points->coords[2] += container->xpos;
	      maximise_container_background(container, this_points, container_data);
	    }
	  else
	    maximise_container_background(container, this_points, container_data);

          containerPointsCacheResetBound(all_data->cache, ZMAPCONTAINER_LEVEL_FEATURESET);
          break;
        case ZMAPCONTAINER_LEVEL_FEATURESET:
          {
            FooCanvasItem *container_features = FOO_CANVAS_ITEM(zmapWindowContainerGetFeatures(container));

            /* If this featureset requires a redraw... */
            if (children && container_data->child_redraw_required)
              {
                redrawColumn(container, this_points, container_data);
                container_data->height = 0.0; /* reset, although, maybe not sensible. see maximise_container_background */
              }

            container_reset_child_positions(container);
            if(FOO_CANVAS_ITEM(container)->object.flags & FOO_CANVAS_ITEM_VISIBLE)
              {
                foo_canvas_item_get_bounds(container_features,
                                           &(this_points->coords[0]),
                                           &(this_points->coords[1]),
                                           &(this_points->coords[2]),
                                           &(this_points->coords[3]));
           
                /* correct for width by score columns */
                this_points->coords[2] = this_points->coords[2] - (0.0 -  this_points->coords[0]);
                this_points->coords[0] = 0.0;
                
                maximise_container_background(container, this_points, container_data);
                
                foo_canvas_item_i2w(container_features, &(this_points->coords[0]), &(this_points->coords[1]));
                foo_canvas_item_i2w(container_features, &(this_points->coords[2]), &(this_points->coords[3]));
                
                foo_canvas_item_w2i(FOO_CANVAS_ITEM(container), &(this_points->coords[0]), &(this_points->coords[1]));
                foo_canvas_item_w2i(FOO_CANVAS_ITEM(container), &(this_points->coords[2]), &(this_points->coords[3]));
              }
          }
          break;
        case ZMAPCONTAINER_LEVEL_INVALID:
        default:
          break;
        }
      
      /* we need to shift the containers in the X axis. 
       * This should probably be done by a callback, but it's nice to have it contained here. */
      if(FOO_CANVAS_ITEM(container)->object.flags & FOO_CANVAS_ITEM_VISIBLE)
        shift_container_to_target(container, this_points, level, spacing, bound);

      /* making the coords in the points cache world coords. */
      foo_canvas_item_i2w(FOO_CANVAS_ITEM(container), &(this_points->coords[0]), &(this_points->coords[1]));
      foo_canvas_item_i2w(FOO_CANVAS_ITEM(container), &(this_points->coords[2]), &(this_points->coords[3]));

      /* propogate the points up to the next level. */
      propogate_points(this_points, parent_points);
    }

  /* Execute post-recursion function. */
  if(all_data->container_leave_cb)
    (all_data->container_leave_cb)(container, this_points, level, all_data->container_leave_data);

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


static void printlevel(FooCanvasGroup *container_parent, FooCanvasPoints *points, 
                       ZMapContainerLevelType level, gpointer user_data)
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


static void redrawColumn(FooCanvasGroup *container_parent, FooCanvasPoints *points_inout, ContainerData data)
{
  zmapWindowContainerZoomChangedCallback redraw_cb;

  zMapAssert(data->child_redraw_required);

  if((redraw_cb = g_object_get_data(G_OBJECT(container_parent), CONTAINER_REDRAW_CALLBACK )))
    {
      gpointer redraw_data = NULL;

      redraw_data = g_object_get_data(G_OBJECT(container_parent), CONTAINER_REDRAW_DATA);

      /* do the callback.... */
      redraw_cb(container_parent, 0.0, redraw_data) ;

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

static void propogate_points(FooCanvasPoints *from, FooCanvasPoints *to)
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
  

  if(window_container_points_debug_G)
    {
      int i = 0;
      printf("from->coords: ");
      for(i = 0; i < from->num_points * 2; i++)
        {
          printf("%f ", from->coords[i]);
        }
      printf("\n");
      
      printf("to->coords  : ");
      for(i = 0; i < to->num_points * 2; i++)
        {
          printf("%f ", to->coords[i]);
        }
      printf("\n");
    }

  return ;
}

static void shift_container_to_target(FooCanvasGroup  *container, 
                                      FooCanvasPoints *container_points,
                                      ZMapContainerLevelType type,
                                      double  this_level_spacing,
                                      double *current_x_bound_inout)
{
  double x1, x2, xpos, dx;
  double target = 0.0;

  /* N.B. container_points are world coords! */

  if (type != ZMAPCONTAINER_LEVEL_ROOT && type != ZMAPCONTAINER_LEVEL_BLOCK)
    {
      if (FOO_CANVAS_ITEM(container)->object.flags & FOO_CANVAS_ITEM_VISIBLE)
        {
          if (current_x_bound_inout)
            {
              target = *current_x_bound_inout;
            }

          /* Are we the first container in this level, hokey... */
          if (target == 0.0)
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
          if(((dx = target - xpos) > 0.5) || dx < -0.5)
            {
              /* move the distance to the mark */
              foo_canvas_item_move(FOO_CANVAS_ITEM(container), dx, 0.0);
              /* update the world points... */
              x2 = container_points->coords[2] += dx;
	      xpos = container->xpos;
            }

          /* update the current bound... */
          if (current_x_bound_inout)
            {
	      /* We must use the real width of the container here! */
              *current_x_bound_inout = target + (x2 - xpos + 1.0);
            }
        }
    }

  return ;
}

#define CONTAINER_SUB_GROUP_COUNT 3
/* This function intends to move all of the container's direct group children to
 * the same origin as itself. i.e. 0.0, 0.0 */
static void container_reset_child_positions(FooCanvasGroup *container)
{
  ContainerGetSubGroup subgroup_getters[CONTAINER_SUB_GROUP_COUNT] = {
    zmapWindowContainerGetFeatures,
    zmapWindowContainerGetOverlays,
    zmapWindowContainerGetUnderlays
  };
  FooCanvasGroup *subgroup;
  double target = 0.0, cx, cy, dx, dy;
  int i;

  for(i = 0; i < CONTAINER_SUB_GROUP_COUNT; i++)
    {
      if((subgroup = (subgroup_getters[i])(container)))
        {
          cx = subgroup->xpos;
          cy = subgroup->ypos;
          dx = target - cx;
          dy = target - cy;
          foo_canvas_item_move(FOO_CANVAS_ITEM(subgroup), dx, dy);
        }
    }

  return ;
}

/* Trying to minimise the number of item_get_bounds calls */
static void maximise_container_background(FooCanvasGroup  *container, 
                                          FooCanvasPoints *this_points, 
                                          ContainerData    container_data)
{
  double nx1, nx2, ny1, ny2, size, zero = 0.0;
  FooCanvasItem *container_background;

  container_background = zmapWindowContainerGetBackground(container);

  nx1 = this_points->coords[0] = zero;
  ny1 = this_points->coords[1] = zero;

  /* OK so I've bitten the bullet and added a height to the container_data... */
  if((container_data->height == 0.0) || 
     ((container_data->level == ZMAPCONTAINER_LEVEL_FEATURESET) && 
      (container_data->height < this_points->coords[3])))
    container_data->height = this_points->coords[3];

  nx2 = this_points->coords[2];
  ny2 = this_points->coords[3] = container_data->height;

  if(container_data->level != ZMAPCONTAINER_LEVEL_FEATURESET)
    {
      foo_canvas_item_w2i(FOO_CANVAS_ITEM(zmapWindowContainerGetFeatures(container)), &nx2, &zero);
      if(nx2 < nx1)
	zMapAssertNotReached();
    }

  foo_canvas_item_set(container_background,
                      "x1", nx1,
                      "y1", ny1,
                      "x2", nx2,
                      "y2", ny2,
                      NULL);
  
  if((size = (nx2 - nx1)) > (double)(1 << 15))
    zMapLogWarning("%s [%d < %f]", "Container background larger than 1 << 15 in x coords.", 1 << 15, size);
#ifdef WHEN_DOES_THIS_HAPPEN
  printf("[maximise_container_background] setting container to %f, %f\n", ny1, ny2);
#endif
  if(container_data->long_items)
    {
      zmapWindowLongItemCheck(container_data->long_items, container_background, ny1, ny2);
    }

  return ;
}


/* Set maximum size for overlays if required. */
static void maximise_container_overlays(FooCanvasGroup  *container, 
                                        FooCanvasPoints *this_points, 
                                        ContainerData    container_data)
{
  ContainerType type = CONTAINER_INVALID ;
  double nx1, nx2, ny1, ny2, size ;
  FooCanvasGroup *container_overlays ;
  
  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(container), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type == CONTAINER_PARENT || type == CONTAINER_ROOT) ;

  container_overlays = zmapWindowContainerGetOverlays(container) ;

  if (container_overlays->item_list
      && (container_data->overlay_flags.max_width || container_data->overlay_flags.max_height))
    {
      GList *list_item ;

      list_item = g_list_first(container_overlays->item_list) ;
      do
	{
	  FooCanvasItem *overlay_item ;
	  double item_x1, item_y1, item_x2, item_y2 ;

	  overlay_item = FOO_CANVAS_ITEM(list_item->data) ;

	  nx1 = this_points->coords[0];
	  ny1 = this_points->coords[1];
	  nx2 = this_points->coords[2];
	  ny2 = this_points->coords[3];

	  /* Get items actual coords. */
	  foo_canvas_item_get_bounds(overlay_item, &item_x1, &item_y1, &item_x2, &item_y2) ;

	  /* Decide which we want to change. */
	  if (!(container_data->overlay_flags.max_width))
	    {
	      nx1 = item_x1 ;
	      nx2 = item_x2 ;
	    }
	  if (!(container_data->overlay_flags.max_height))
	    {
	      ny1 = item_y1 ;
	      ny2 = item_y2 ;
	    }

	  foo_canvas_item_set(overlay_item,
			      "x1", nx1,
			      "y1", ny1,
			      "x2", nx2,
			      "y2", ny2,
			      NULL);

	  
  
	  if ((size = (nx2 - nx1)) > (double)(1 << 15))
	    zMapLogWarning("%s [%d < %f]", "Container background larger than 1 << 15 in x coords.", 1 << 15, size);

	  list_item = g_list_next(list_item) ;

	} while (list_item) ;
    }
  
  return ;
}

static void set_column_lists_cb(FooCanvasGroup *container, FooCanvasPoints *points, 
				ZMapContainerLevelType level, gpointer user_data)
{
  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      {
	ForwardReverseColumnLists lists_data;
	ZMapWindowItemFeatureSetData set_data;

	lists_data = (ForwardReverseColumnLists)user_data;

	if((set_data = g_object_get_data(G_OBJECT(container), ITEM_FEATURE_SET_DATA)))
	  {
	    if(set_data->strand == ZMAPSTRAND_FORWARD)
	      lists_data->forward = g_list_append(lists_data->forward, container);
	    else
	      lists_data->reverse = g_list_append(lists_data->reverse, container);
	  }
      }
      break;
    default:
      break;
    }

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

/* the foo canvas event stuff doesn't really do events by point although 
 * it says it does...
 *
 * It does an invoke_point to find a leaf item.
 * then while(item && !finished){ ... item = item->parent; ... }
 * 
 * This doesn't quite work for us in our containers...
 * Our organisation is a little like this, where containers(group)
 * have a bg(item) and a features(group) which is the nxt container. 
 *
 * Root -> Align -> Block -> Strand -> Set -> Feature *
 *  \       \        \        \         \      \
 *   bg *    bg *     bg *     bg *      bg *   bg *
 * if handlers are attached(*) 
 * a feature handler returning false will not make a feature bg 
 * handler get called, nor a set bg handler, nor ...
 * 
 * containerRootInvokeContainerBGEvent calls the bg handler for you.
 * I hope it's fast enough...
 */
static gboolean containerRootInvokeContainerBGEvent(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  FooCanvasItem *background = NULL;
  gboolean event_handled = FALSE;
  unsigned int item_event = 0, detail = 0, count = 0, *ids;

  background = zmapWindowContainerGetBackground(FOO_CANVAS_GROUP(item));

  if((ids = g_signal_list_ids(foo_canvas_item_get_type(), &count)))
    {
      zMapAssert(count == 1 && ids); /* failure here, possibly means the next bit is no longer true.
				      * item_event == ITEM_EVENT 
				      * (foo-canvas.c enum{ ITEM_EVENT, ITEM_LAST_SIGNAL }) */
      g_signal_emit(GTK_OBJECT(background), ids[item_event], detail, event, &event_handled);

      g_free(ids);
    }
  else
    zMapLogCritical("No ids returned... Check %s", "foo-canvas.c enum{ ITEM_EVENT, ITEM_LAST_SIGNAL }");

  return event_handled;
}

inline static GObject *containerGObject(FooCanvasGroup *container)
{
  container = zmapWindowContainerGetParent(FOO_CANVAS_ITEM( container ));

  zMapAssert(container);

  return G_OBJECT(container);
}

#ifdef RDS_CONTAINER_EXECUTE_THOUGHTS

#define CONTAINER_LIST_SIZE 100
static ContainerRecursionDataStruct container_list[CONTAINER_LIST_SIZE] = {NULL};

container_execute_internal_pre_list_cb(container, this_points, level, gpointer user_data)
{
  int i;

  for(i = 0; i < CONTAINER_LIST_SIZE; i++)
  {
    ContainerRecursionData tmp;
    if((tmp = &(container_list[i])))
      {
        (tmp->container_enter_cb)(container, this_points, level, tmp->container_enter_data);
      }
  }

  return ;
}

container_execute_internal_post_list_cb(container, this_points, level, gpointer user_data)
{
  int i;

  for(i = 0; i < CONTAINER_LIST_SIZE; i++)
  {
    ContainerRecursionData tmp;
    if((tmp = &(container_list[i])))
      {
        (tmp->container_leave_cb)(container, this_points, level, tmp->container_leave_data);
      }
  }

  return ;
}


void zmapWindowContainerExecuteRunAll(root)
{
  int i;
  zmapWindowContainerExecute(root, stop,
                             container_execute_internal_pre_list_cb,  NULL,
                             container_execute_internal_ppst_list_cb, NULL, 
                             TRUE);

  for(i = 0; i < CONTAINER_LIST_SIZE; i++)
    {
      container_list[i] = {NULL};
    }
  
  return ;
}

void zmapWindowContainerExecuteAddFunction(root, stop, enter_cb, enter_data, leave_cb, leave_data, redraw)
{
  ContainerRecursionData tmp;
  int i, m;

  for(i = 0; i < CONTAINER_LIST_SIZE; i++)
    {
      if(container_list[i])
        m = i;
    }

  m++;

  tmp = &(container_list[m]);
  tmp->stop                 = stop;
  tmp->container_enter_cb   = enter_cb;
  tmp->container_enter_data = enter_data;
  tmp->container_leave_cb   = leave_cb;
  tmp->container_leave_data = leave_data;
  tmp->redraw               = redraw;

  return ;
}

#endif /* RDS_CONTAINER_EXECUTE_THOUGHTS */




/* GCompareFunc() to compare canvas item features by position and size. */
static gint comparePosition(gconstpointer a, gconstpointer b)
{
  gint result = -1 ;
  FooCanvasItem *item1 = (FooCanvasItem *)a, *item2 = (FooCanvasItem *)b ;
  ZMapFeature feature1, feature2 ;

  feature1 = g_object_get_data(G_OBJECT(item1), ITEM_FEATURE_DATA) ;
  feature2 = g_object_get_data(G_OBJECT(item2), ITEM_FEATURE_DATA) ;

  zMapAssert(zMapFeatureIsValid((ZMapFeatureAny)feature1) && zMapFeatureIsValid((ZMapFeatureAny)feature2)) ;

  if (feature1->x1 > feature2->x2)
    result = 1 ;
  else if (feature1->x1 == feature2->x1)
    {
      int diff1, diff2 ;

      diff1 = feature1->x2 - feature1->x1 ;
      diff2 = feature2->x2 - feature2->x1 ;

      if (diff1 < diff2)
	result = 1 ;
      else if (diff1 == diff2)
	result = 0 ;
    }

  return result ;
}



/* A GFunc() to print out feature names.. */
static void printAnyFeatName(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ZMapFeature feature ;

  feature = (ZMapFeature)g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;

  printf("%s : %s\n", g_quark_to_string(feature->original_id), g_quark_to_string(feature->unique_id)) ;

  return ;
}



