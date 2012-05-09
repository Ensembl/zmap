/*  File: zmapWindowContainerBlock.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#ifndef __ZMAP_WINDOW_CONTAINER_BLOCK_H__
#define __ZMAP_WINDOW_CONTAINER_BLOCK_H__

#include <glib-object.h>
#include <libzmapfoocanvas/libfoocanvas.h>
#include <zmapWindowContainerGroup_I.h>
#include <zmapWindowContainerFeatureSet.h>
#include <zmapWindowMark_P.h>	/* ZMapWindowMark ... */

#define ZMAP_WINDOW_CONTAINER_BLOCK_NAME 	"ZMapWindowContainerBlock"


#define ZMAP_TYPE_CONTAINER_BLOCK           (zmapWindowContainerBlockGetType())

#if GOBJ_CAST
#define ZMAP_CONTAINER_BLOCK(obj)       ((ZMapWindowContainerBlock) obj)
#define ZMAP_CONTAINER_BLOCK_CONST(obj) ((ZMapWindowContainerBlock const ) obj)
#else
#define ZMAP_CONTAINER_BLOCK(obj)	    (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_BLOCK, zmapWindowContainerBlock))
#define ZMAP_CONTAINER_BLOCK_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_BLOCK, zmapWindowContainerBlock const))
#endif

#define ZMAP_CONTAINER_BLOCK_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_CONTAINER_BLOCK, zmapWindowContainerBlockClass))
#define ZMAP_IS_CONTAINER_BLOCK(obj)	    (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_CONTAINER_BLOCK))
#define ZMAP_CONTAINER_BLOCK_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_CONTAINER_BLOCK, zmapWindowContainerBlockClass))


/* Instance */
typedef struct _zmapWindowContainerBlockStruct  zmapWindowContainerBlock, *ZMapWindowContainerBlock ;


/* Class */
typedef struct _zmapWindowContainerBlockClassStruct  zmapWindowContainerBlockClass, *ZMapWindowContainerBlockClass ;


/* Public funcs */
GType zmapWindowContainerBlockGetType(void);
ZMapWindowContainerBlock zmapWindowContainerBlockAugment(ZMapWindowContainerBlock container_block,
							 ZMapFeatureBlock feature_block);
void   zmapWindowContainerBlockAddCompressedColumn(ZMapWindowContainerBlock block_data,
						   FooCanvasGroup *container);
GList *zmapWindowContainerBlockRemoveCompressedColumns(ZMapWindowContainerBlock block_data);
void   zmapWindowContainerBlockAddBumpedColumn(ZMapWindowContainerBlock block_data,
					       FooCanvasGroup *container);
GList *zmapWindowContainerBlockRemoveBumpedColumns(ZMapWindowContainerBlock block_data);

void zmapWindowContainerBlockMark(ZMapWindowContainerBlock container_block,
				  ZMapWindowMark mark);
void zmapWindowContainerBlockUnmark(ZMapWindowContainerBlock container_block);
void zmapWindowContainerBlockFlagRegion(ZMapWindowContainerBlock block_data,
					ZMapFeatureBlock         block);
void zmapWindowContainerBlockFlagRegionForColumn(ZMapWindowContainerBlock       container_block,
						 ZMapFeatureBlock               block,
						 ZMapWindowContainerFeatureSet  container_set);
GList *zmapWindowContainerBlockFilterFlaggedColumns(ZMapWindowContainerBlock block_data,
						    GList *list, int world1, int world2);
gboolean zmapWindowContainerBlockIsColumnLoaded(ZMapWindowContainerBlock      container_block,
						ZMapWindowContainerFeatureSet container_set,
						int world1, int world2);


ZMapWindowContainerBlock zMapWindowContainerBlockDestroy(ZMapWindowContainerBlock canvas_item);


#endif /* ! __ZMAP_WINDOW_CONTAINER_BLOCK_H__ */
