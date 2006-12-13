/*  File: zmapWindowContainer.h
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
 * Description: Package for handling canvas items that represent the
 *              feature context.
 *
 * HISTORY:
 * Last edited: Dec 12 16:20 2006 (edgrif)
 * Created: Fri Dec  9 16:40:20 2005 (edgrif)
 * CVS info:   $Id: zmapWindowContainer.h,v 1.16 2006-12-13 13:40:07 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_CONTAINER_H
#define ZMAP_WINDOW_CONTAINER_H



/* Used to identify unambiguously which part of a zmapWindowContainer group a particular
 * foo canvas group or item represents. */
typedef enum
  {
    CONTAINER_INVALID,
    CONTAINER_ROOT,					    /* Root container has this instead of CONTAINER_PARENT */
    CONTAINER_PARENT,					    /* Container parent group */
    CONTAINER_OVERLAYS,					    /* Overlay objects. */
    CONTAINER_FEATURES,					    /* Container subgroup containing features. */
    CONTAINER_BACKGROUND				    /* Rectangular item to form group background. */
  } ContainerType ;


/* because roy can't spell container_type more than once */
#define CONTAINER_TYPE_KEY        "container_type"


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

typedef void (*zmapWindowContainerZoomChangedCallback)(FooCanvasGroup *container, 
                                                       double new_zoom, 
                                                       gpointer user_data);

typedef void (*ZMapContainerExecFunc)(FooCanvasGroup        *container, 
                                      FooCanvasPoints       *container_points,
                                      ZMapContainerLevelType container_level,
                                      gpointer               func_data);

FooCanvasGroup *zmapWindowContainerCreate(FooCanvasGroup *parent,
					  ZMapContainerLevelType level,
					  double child_spacing,
					  GdkColor *background_fill_colour,
					  GdkColor *background_border_colour,
					  ZMapWindowLongItems long_items) ;

void zmapWindowContainerSetZoomEventHandler(FooCanvasGroup* featureset_container,
                                            zmapWindowContainerZoomChangedCallback handler_cb,
                                            gpointer user_data,
                                            GDestroyNotify data_destroy_notify);
gboolean zmapWindowContainerIsChildRedrawRequired(FooCanvasGroup *container_parent);
void zmapWindowContainerSetChildRedrawRequired(FooCanvasGroup *container_parent,
					       gboolean redraw_required) ;
gboolean zmapWindowContainerIsValid(FooCanvasGroup *any_group) ;
FooCanvasGroup *zmapWindowContainerGetFromItem(FooCanvasItem *any_item) ;
FooCanvasGroup *zmapWindowContainerGetParentLevel(FooCanvasItem *any_item, ZMapContainerLevelType level) ;
FooCanvasGroup *zmapWindowContainerGetSuperGroup(FooCanvasGroup *container_parent) ;
FooCanvasGroup *zmapWindowContainerGetParent(FooCanvasItem *any_container_child) ;
FooCanvasItem *zmapWindowContainerGetBackground(FooCanvasGroup *container_parent) ;
FooCanvasGroup *zmapWindowContainerGetFeatures(FooCanvasGroup *container_parent) ;
FooCanvasGroup *zmapWindowContainerGetOverlays(FooCanvasGroup *container_parent) ;
void zmapWindowContainerSetOverlayResizing(FooCanvasGroup *container_parent,
					   gboolean maximise_width, gboolean maximise_height) ;
ZMapContainerLevelType zmapWindowContainerGetLevel(FooCanvasGroup *container_parent) ;
ZMapFeatureTypeStyle zmapWindowContainerGetStyle(FooCanvasGroup *column_group) ;
double zmapWindowContainerGetSpacing(FooCanvasGroup *column_group) ;
FooCanvasGroup *zmapWindowContainerGetStrandGroup(FooCanvasGroup *strand_parent, ZMapStrand strand) ;

/* These work from ... */
FooCanvasGroup *zmapWindowContainerGetFeaturesContainerFromItem(FooCanvasItem *feature_item);
FooCanvasGroup *zmapWindowContainerGetParentContainerFromItem(FooCanvasItem *feature_item);


void zmapWindowContainerReposition(FooCanvasGroup *container) ;
gboolean zmapWindowContainerHasFeatures(FooCanvasGroup *container_parent) ;

void zmapWindowContainerSetBackgroundSize(FooCanvasGroup *container_parent, double y_extent) ;
void zmapWindowContainerSetBackgroundSizePlusBorder(FooCanvasGroup *container_parent,
                                                    double height, 
                                                    double border);
void zmapWindowContainerSetBackgroundColour(FooCanvasGroup *container_parent, GdkColor *background_fill_colour) ;
void zmapWindowContainerResetBackgroundColour(FooCanvasGroup *container_parent) ;
void zmapWindowContainerMaximiseBackground(FooCanvasGroup *container_parent) ;

ZMapStrand zmapWindowContainerGetStrand(FooCanvasGroup *container);
void zmapWindowContainerSetStrand(FooCanvasGroup *container, ZMapStrand strand);


void zmapWindowContainerPrint(FooCanvasGroup *container_parent) ;

void zmapWindowContainerPrintLevel(FooCanvasGroup *strand_container) ;

void zmapWindowContainerExecute(FooCanvasGroup        *parent, 
				ZMapContainerLevelType stop_at_type,
				ZMapContainerExecFunc  down_cb,
				gpointer               down_data) ;
void zmapWindowContainerExecuteFull(FooCanvasGroup        *parent, 
                                    ZMapContainerLevelType stop_at_type,
                                    ZMapContainerExecFunc  down_func_cb,
                                    gpointer               down_func_data,
                                    ZMapContainerExecFunc  up_func_cb,
                                    gpointer               up_func_data,
                                    gboolean               redraw_during_recursion) ;
void zmapWindowContainerZoomEvent(FooCanvasGroup *super_root, ZMapWindow window);
void zmapWindowContainerMoveEvent(FooCanvasGroup *super_root, ZMapWindow window);

void zmapWindowContainerPurge(FooCanvasGroup *unknown_child);
void zmapWindowContainerDestroy(FooCanvasGroup *container_parent) ;


/* this should be somewhere else I think.... */
void zmapWindowCanvasGroupChildSort(FooCanvasGroup *group_inout) ;


#endif /* !ZMAP_WINDOW_CONTAINER_H */
