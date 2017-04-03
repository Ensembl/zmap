/*  File: zmapWindowContainerUtils.h
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
 * Description: Utility functions for containers (context, align, block etc).
 *
 *-------------------------------------------------------------------
 */

#ifndef __ZMAP_WINDOW_CONTAINER_UTILS_H__
#define __ZMAP_WINDOW_CONTAINER_UTILS_H__

#include <ZMap/zmapFeature.hpp>
#include <zmapWindowContainers.hpp>


typedef void (*ZMapContainerUtilsExecFunc)(ZMapWindowContainerGroup container,
					   FooCanvasPoints         *container_points,
					   ZMapContainerLevelType   container_level,
					   gpointer                 func_data);

/* Check a canvas group is a valid container */
gboolean zmapWindowContainerUtilsIsValid(FooCanvasGroup *any_group);



/* this seems completely broken by Malcolm's changes and left half-finished..shame...
 * I don't even see what the point of these two macros is...what are they supposed to
 * do ???? */

#define ZMapWindowContainerFeatures	ZMapWindowContainerGroup
#define zmapWindowContainerGetFeatures(container)	container



ZMapWindowFeaturesetItem ZMapWindowContainerGetFeatureSetItem(ZMapWindowContainerGroup container) ;



/* Used by zmapWindowContainer calls for going forward/backward through lists. */
typedef enum
  {
    ZMAPCONTAINER_ITEM_FIRST = -1,			    /* These two _must_ be < 0 */
    ZMAPCONTAINER_ITEM_LAST = -2,
    ZMAPCONTAINER_ITEM_PREV,
    ZMAPCONTAINER_ITEM_NEXT
  } ZMapContainerItemDirection ;

typedef gboolean (*zmapWindowContainerItemTestCallback)(FooCanvasItem *item, gpointer user_data) ;



ZMapWindowContainerGroup zmapWindowContainerChildGetParent(FooCanvasItem *item);
ZMapWindowContainerGroup zmapWindowContainerGetNextParent(FooCanvasItem *item);

ZMapWindowContainerFeatures zmapWindowContainerUtilsItemGetFeatures(FooCanvasItem         *item,
								    ZMapContainerLevelType level);
ZMapWindowContainerGroup zmapWindowContainerUtilsGetParentLevel(ZMapWindowContainerGroup container_group,
								ZMapContainerLevelType level);
ZMapWindowContainerGroup zmapWindowContainerUtilsItemGetParentLevel(FooCanvasItem *item,
								    ZMapContainerLevelType level);
ZMapContainerLevelType zmapWindowContainerUtilsGetLevel(FooCanvasItem *item);
ZMapWindowContainerGroup zmapWindowContainerCanvasItemGetContainer(FooCanvasItem *item);

int zmapWindowContainerGetItemPosition(ZMapWindowContainerGroup container_parent, FooCanvasItem *item) ;
gboolean zmapWindowContainerSetItemPosition(ZMapWindowContainerGroup container_parent,
					    FooCanvasItem *item, int position) ;


/* Block level utilities */


GList *zmapWindowContainerFindItemInList(ZMapWindowContainerGroup container_parent, FooCanvasItem *item);
FooCanvasItem *zmapWindowContainerGetNthFeatureItem(ZMapWindowContainerGroup container, int nth_item);
FooCanvasItem *zmapWindowContainerGetNextFeatureItem(FooCanvasItem *orig_item,
						     ZMapContainerItemDirection direction, gboolean wrap,
						     zmapWindowContainerItemTestCallback item_test_func_cb,
						     gpointer user_data);

/* Features */
gboolean zmapWindowContainerAttachFeatureAny(ZMapWindowContainerGroup container, ZMapFeatureAny feature_any);
gboolean zmapWindowContainerGetFeatureAny(ZMapWindowContainerGroup container, ZMapFeatureAny *feature_any_out);

ZMapFeatureSet zmapWindowContainerGetFeatureSet(ZMapWindowContainerGroup feature_group);
ZMapFeatureBlock zmapWindowContainerGetFeatureBlock(ZMapWindowContainerGroup feature_group);

gboolean zmapWindowContainerHasFeatures(ZMapWindowContainerGroup container) ;

gboolean zmapWindowContainerUtilsGetColumnLists(ZMapWindowContainerGroup block_or_strand_group,
						GList **forward_columns_out,
						GList **reverse_columns_out);
void zmapWindowContainerSetOverlayResizing(ZMapWindowContainerGroup container_group,
					   gboolean maximise_width, gboolean maximise_height);
void zmapWindowContainerSetUnderlayResizing(ZMapWindowContainerGroup container_group,
					    gboolean maximise_width, gboolean maximise_height);
void zmapWindowContainerUtilsRemoveAllItems(FooCanvasGroup *group);

void zmapWindowContainerUtilsExecuteFull(ZMapWindowContainerGroup   parent,
					 ZMapContainerLevelType     stop_at_type,
					 ZMapContainerUtilsExecFunc container_enter_cb,
					 gpointer                   container_enter_data,
					 ZMapContainerUtilsExecFunc container_leave_cb,
					 gpointer                   container_leave_data);
void zmapWindowContainerUtilsExecute(ZMapWindowContainerGroup   parent,
				     ZMapContainerLevelType     stop_at_type,
				     ZMapContainerUtilsExecFunc container_enter_cb,
				     gpointer                   container_enter_data);


FooCanvasItem *zMapFindCanvasColumn(ZMapWindowContainerGroup group,
      GQuark align, GQuark block, GQuark set, ZMapStrand strand, ZMapFrame frame);


#endif /* ! __ZMAP_WINDOW_CONTAINER_UTILS_H__ */
