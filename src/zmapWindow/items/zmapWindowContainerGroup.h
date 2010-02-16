/*  File: zmapWindowContainerGroup.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
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
 * Last edited: Feb 16 09:56 2010 (edgrif)
 * Created: Wed Dec  3 08:21:03 2008 (rds)
 * CVS info:   $Id: zmapWindowContainerGroup.h,v 1.6 2010-02-16 10:29:06 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_CONTAINER_GROUP_H
#define ZMAP_WINDOW_CONTAINER_GROUP_H

#include <gdk/gdk.h>		/* GdkColor */
#include <glib-object.h>
#include <libfoocanvas/libfoocanvas.h>
#include <zmapWindowContainerChildren.h>


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
    ZMAPCONTAINER_LEVEL_STRAND,
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

ZMapWindowContainerGroup zmapWindowContainerGroupCreate(ZMapWindowContainerFeatures parent,
							ZMapContainerLevelType level,
							double    child_spacing,
							GdkColor *background_fill_colour,
							GdkColor *background_border_colour);
ZMapWindowContainerGroup zmapWindowContainerGroupCreateFromFoo(FooCanvasGroup         *parent,
							       ZMapContainerLevelType level,
							       double    child_spacing,
							       GdkColor *background_fill_colour,
							       GdkColor *background_border_colour);
gboolean zmapWindowContainerSetVisibility(FooCanvasGroup *container_parent, gboolean visible);
void zmapWindowContainerRequestReposition(ZMapWindowContainerGroup container);
void zmapWindowContainerGroupBackgroundSize(ZMapWindowContainerGroup container, double height);
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
