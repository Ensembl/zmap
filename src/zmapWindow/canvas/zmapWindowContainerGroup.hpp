/*  File: zmapWindowContainerGroup.h
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
#ifndef ZMAP_WINDOW_CONTAINER_GROUP_H
#define ZMAP_WINDOW_CONTAINER_GROUP_H

#include <gdk/gdk.h>		/* GdkColor */
#include <glib-object.h>
#include <libzmapfoocanvas/libfoocanvas.h>

/* It feels wrong that you can't compile this header without including this...the encapsution
 * is wrong at some level...perhaps more of the stuff from zmapWindowContainerChildren.h
 * should be in this header.... */
//#include <zmapWindowContainerChildren.h>


#if USE_CHILDREN

/*!
 * ZMapWindowContainerGroup for containing and positioning of canvas items.
 * Each ZMapWindowContainerGroup consists of:
 *
 *               -------- parent_group -----------
 *              /         /        \              \
 *             /         /          \              \
 *            /         /            \              \
 *   background   sub_group of      sub_group of    sub_group of
 *      item     underlay items    feature items   overlay items
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
 * The new object code means that each container member including the member itself
 * has a G_TYPE*  This means code that does ZMAP_IS_CONTAINER_GROUP(pointer) is
 * simpler and hopefully more readable.
 *
 * The ZMapWindowContainerGroup are sub classes of FooCanvasGroup and implement the
 * FooCanvasItem interface (draw, update, bounds, etc...).  The update code takes
 * care of cropping the Container "owned" items, such as the background and any
 * overlays/underlays that might be being drawn.  It also includes hooks to
 * provide similar functionality to the ContainerExecute callbacks.  These are
 * attached/owned by each specific container so only get called by the container
 * they relate to.  This again leads to simpler code, without the switch on the
 * container level.
 *
 */
#endif




#define ZMAP_WINDOW_CONTAINER_GROUP_NAME 	"ZMapWindowContainerGroup"


#define ZMAP_TYPE_CONTAINER_GROUP           (zmapWindowContainerGroupGetType())

#define ZMAP_CONTAINER_GROUP(obj)	    (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_GROUP, zmapWindowContainerGroup))
#define ZMAP_CONTAINER_GROUP_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_GROUP, zmapWindowContainerGroup const))
#define ZMAP_CONTAINER_GROUP_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_CONTAINER_GROUP, zmapWindowContainerGroupClass))
#define ZMAP_IS_CONTAINER_GROUP(obj)	    (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_CONTAINER_GROUP))
#define ZMAP_CONTAINER_GROUP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_CONTAINER_GROUP, zmapWindowContainerGroupClass))


/* I don't really like having to do this...seems clumsy...these represent the level in the
 * feature context hierachy...forced on us by the lack of a "strand" level in the actual
 * feature context but we need it for the canvas item hierachy.... */
typedef enum
  {
    ZMAPCONTAINER_LEVEL_INVALID,
    ZMAPCONTAINER_LEVEL_ROOT,
    ZMAPCONTAINER_LEVEL_ALIGN,
    ZMAPCONTAINER_LEVEL_BLOCK,
    ZMAPCONTAINER_LEVEL_FEATURESET,
    ZMAPCONTAINER_LEVEL_FEATURESET_GROUP
} ZMapContainerLevelType ;


/* Instance */
typedef struct _zmapWindowContainerGroupStruct  zmapWindowContainerGroup, *ZMapWindowContainerGroup ;


/* Class */
typedef struct _zmapWindowContainerGroupClassStruct  zmapWindowContainerGroupClass, *ZMapWindowContainerGroupClass ;


typedef gboolean (* ZMapWindowContainerUpdateHook)(ZMapWindowContainerGroup group_updated,
						   FooCanvasPoints         *group_bounds,
						   ZMapContainerLevelType   group_level,
						   gpointer                 user_data);

/* Public funcs */
GType zmapWindowContainerGroupGetType(void);

ZMapWindowContainerGroup zmapWindowContainerGroupCreate(FooCanvasGroup *parent,
							ZMapContainerLevelType level,
							double    child_spacing,
							GdkColor *background_fill_colour,
							GdkColor *background_border_colour);
ZMapWindowContainerGroup zmapWindowContainerGroupCreateFromFoo(FooCanvasGroup         *parent,
							       ZMapContainerLevelType level,
							       double    child_spacing,
							       GdkColor *background_fill_colour,
							       GdkColor *background_border_colour);

void zMapWindowContainerGroupSortByLayer(FooCanvasGroup * group);

GdkColor *zmapWindowContainerGroupGetFill(ZMapWindowContainerGroup group);
GdkColor *zmapWindowContainerGroupGetBorder(ZMapWindowContainerGroup group);

gboolean zmapWindowContainerSetVisibility(FooCanvasGroup *container_parent, gboolean visible);
void zmapWindowContainerRequestReposition(ZMapWindowContainerGroup container);
void zmapWindowContainerGroupChildRedrawRequired(ZMapWindowContainerGroup container,
						 gboolean redraw_required);
void zmapWindowContainerGroupSetBackgroundColour(ZMapWindowContainerGroup container,
						 GdkColor *new_colour);
void zmapWindowContainerGroupResetBackgroundColour(ZMapWindowContainerGroup container);
void zmapWindowContainerGroupAddUpdateHook(ZMapWindowContainerGroup container,
					   ZMapWindowContainerUpdateHook hook,
					   gpointer user_data);
void zmapWindowContainerGroupRemoveUpdateHook(ZMapWindowContainerGroup container,
					      ZMapWindowContainerUpdateHook hook,
					      gpointer user_data);

ZMapWindowContainerGroup zmapWindowContainerGroupDestroy(ZMapWindowContainerGroup container);


#endif /* ZMAP_WINDOW_CONTAINER_GROUP_H */
