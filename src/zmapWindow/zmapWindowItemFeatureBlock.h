/*  File: zmapWindowItemFeatureBlock.h
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
 * Last edited: Apr 16 15:11 2009 (rds)
 * Created: Fri Feb  6 15:32:46 2009 (rds)
 * CVS info:   $Id: zmapWindowItemFeatureBlock.h,v 1.2 2009-04-16 14:37:24 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef __ZMAP_WINDOW_ITEM_FEATURE_BLOCK_H__
#define __ZMAP_WINDOW_ITEM_FEATURE_BLOCK_H__

#include <glib.h>
#include <glib-object.h>
#include <ZMap/zmapWindow.h>

#define ZMAP_TYPE_WINDOW_ITEM_FEATURE_BLOCK           (zmapWindowItemFeatureBlockGetType())
#define ZMAP_WINDOW_ITEM_FEATURE_BLOCK(obj)	      (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_ITEM_FEATURE_BLOCK, zmapWindowItemFeatureBlockData))
#define ZMAP_WINDOW_ITEM_FEATURE_BLOCK_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_ITEM_FEATURE_BLOCK, zmapWindowItemFeatureBlockData const))
#define ZMAP_WINDOW_ITEM_FEATURE_BLOCK_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_WINDOW_ITEM_FEATURE_BLOCK, zmapWindowItemFeatureBlockDataClass))
#define ZMAP_IS_WINDOW_ITEM_FEATURE_BLOCK(obj)	      (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_WINDOW_ITEM_FEATURE_BLOCK))
#define ZMAP_WINDOW_ITEM_FEATURE_BLOCK_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_WINDOW_ITEM_FEATURE_BLOCK, zmapWindowItemFeatureBlockDataClass))


/* Instance */
typedef struct _zmapWindowItemFeatureBlockDataStruct      zmapWindowItemFeatureBlockData,      *ZMapWindowItemFeatureBlockData;

/* Class */
typedef struct _zmapWindowItemFeatureBlockDataClassStruct zmapWindowItemFeatureBlockDataClass, *ZMapWindowItemFeatureBlockDataClass ;



/* Public funcs */
GType zmapWindowItemFeatureBlockGetType(void);

ZMapWindowItemFeatureBlockData zmapWindowItemFeatureBlockCreate(ZMapWindow window);


void zmapWindowItemFeatureBlockAddCompressedColumn(ZMapWindowItemFeatureBlockData block_data, 
						   FooCanvasGroup *container);
GList *zmapWindowItemFeatureBlockRemoveCompressedColumns(ZMapWindowItemFeatureBlockData block_data);
void zmapWindowItemFeatureBlockAddBumpedColumn(ZMapWindowItemFeatureBlockData block_data, 
					       FooCanvasGroup *container);
GList *zmapWindowItemFeatureBlockRemoveBumpedColumns(ZMapWindowItemFeatureBlockData block_data);
ZMapWindow zmapWindowItemFeatureBlockGetWindow(ZMapWindowItemFeatureBlockData block_data);
void zmapWindowItemFeatureBlockMarkRegion(ZMapWindowItemFeatureBlockData block_data,
					  ZMapFeatureBlock               block);
void zmapWindowItemFeatureBlockMarkRegionForStyle(ZMapWindowItemFeatureBlockData block_data,
						  ZMapFeatureBlock               block, 
						  ZMapFeatureTypeStyle           style);
GList *zmapWindowItemFeatureBlockFilterMarkedColumns(ZMapWindowItemFeatureBlockData block_data,
						     GList *list, int world1, int world2);
gboolean zmapWindowItemFeatureBlockIsColumnLoaded(ZMapWindowItemFeatureBlockData block_data,
						  FooCanvasGroup *column_group, int world1, int world2);


ZMapWindowItemFeatureBlockData zmapWindowItemFeatureBlockDestroy(ZMapWindowItemFeatureBlockData block_data);



#endif /* __ZMAP_WINDOW_ITEM_FEATURE_BLOCK_H__ */
