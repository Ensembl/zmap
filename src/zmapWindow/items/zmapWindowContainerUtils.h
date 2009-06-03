/*  File: zmapWindowContainerUtils.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2009: Genome Research Ltd.
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
 * Last edited: Jun  3 21:48 2009 (rds)
 * Created: Thu Apr 30 14:40:12 2009 (rds)
 * CVS info:   $Id: zmapWindowContainerUtils.h,v 1.2 2009-06-03 22:29:08 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_CONTAINER_UTILS_H
#define ZMAP_WINDOW_CONTAINER_UTILS_H

#include <zmapWindowContainerGroup.h>
#include <zmapWindowContainerChildren.h>


typedef void (*ZMapContainerUtilsExecFunc)(ZMapWindowContainerGroup container, 
					   FooCanvasPoints         *container_points,
					   ZMapContainerLevelType   container_level,
					   gpointer                 func_data);

FooCanvasGroup *zmapWindowContainerCreate(FooCanvasGroup *parent,
					  ZMapContainerLevelType level,
					  double child_spacing,
					  GdkColor *background_fill_colour,
					  GdkColor *background_border_colour,
					  ZMapWindowLongItems long_items);

/* Check a canvas group is a valid container */
gboolean zmapWindowContainerUtilsIsValid(FooCanvasGroup *any_group);


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


/* Block level utilities */
ZMapWindowContainerStrand zmapWindowContainerBlockGetContainerStrand(ZMapWindowContainerBlock container_block,
								     ZMapStrand               strand);
ZMapWindowContainerStrand zmapWindowContainerBlockGetContainerSeparator(ZMapWindowContainerBlock container_block);


ZMapWindowContainerFeatures   zmapWindowContainerGetFeatures  (ZMapWindowContainerGroup container);
ZMapWindowContainerBackground zmapWindowContainerGetBackground(ZMapWindowContainerGroup container);
ZMapWindowContainerOverlay    zmapWindowContainerGetOverlay   (ZMapWindowContainerGroup container);
ZMapWindowContainerUnderlay   zmapWindowContainerGetUnderlay  (ZMapWindowContainerGroup container);

ZMapStrand zmapWindowContainerGetStrand(ZMapWindowContainerGroup container);
gboolean zmapWindowContainerIsStrandSeparator(ZMapWindowContainerGroup container);

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
					 gpointer                   container_leave_data,
					 gboolean                   redraw_during_recursion);
void zmapWindowContainerUtilsExecute(ZMapWindowContainerGroup   parent, 
				     ZMapContainerLevelType     stop_at_type,
				     ZMapContainerUtilsExecFunc container_enter_cb,
				     gpointer                   container_enter_data);


#endif /* ZMAP_WINDOW_CONTAINER_UTILS_H */
