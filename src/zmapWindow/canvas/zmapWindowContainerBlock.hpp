/*  File: zmapWindowContainerBlock.h
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

#ifndef __ZMAP_WINDOW_CONTAINER_BLOCK_H__
#define __ZMAP_WINDOW_CONTAINER_BLOCK_H__

#include <glib-object.h>
#include <libzmapfoocanvas/libfoocanvas.h>

// this should not be at this level....window files should not be here....
#include <zmapWindowMark.hpp>	/* ZMapWindowMark ... */

#include <zmapWindowContainerGroup_I.hpp>
#include <zmapWindowContainerFeatureSet.hpp>


#define ZMAP_WINDOW_CONTAINER_BLOCK_NAME 	"ZMapWindowContainerBlock"


#define ZMAP_TYPE_CONTAINER_BLOCK           (zmapWindowContainerBlockGetType())

#define ZMAP_CONTAINER_BLOCK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_BLOCK, zmapWindowContainerBlock))
#define ZMAP_CONTAINER_BLOCK_CONST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_BLOCK, zmapWindowContainerBlock const))
#define ZMAP_CONTAINER_BLOCK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_CONTAINER_BLOCK, zmapWindowContainerBlockClass))
#define ZMAP_IS_CONTAINER_BLOCK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_CONTAINER_BLOCK))
#define ZMAP_CONTAINER_BLOCK_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_CONTAINER_BLOCK, zmapWindowContainerBlockClass))


/* Instance */
typedef struct _zmapWindowContainerBlockStruct  zmapWindowContainerBlock, *ZMapWindowContainerBlock ;


/* Class */
typedef struct _zmapWindowContainerBlockClassStruct  zmapWindowContainerBlockClass, *ZMapWindowContainerBlockClass ;


/* Public funcs */
GType zmapWindowContainerBlockGetType(void);
ZMapWindowContainerBlock zmapWindowContainerBlockAugment(ZMapWindowContainerBlock container_block,
							 ZMapFeatureBlock feature_block);

ZMapWindowContainerFeatureSet zmapWindowContainerBlockFindColumn(ZMapWindowContainerBlock block_container,
                                                                 const char *col_name, ZMapStrand strand) ;
GList *zmapWindowContainerBlockColumnList(ZMapWindowContainerBlock block_container,
                                          gboolean unique, ZMapStrand strand, gboolean visible) ;

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
						    GList *glist, int world1, int world2);
gboolean zmapWindowContainerBlockIsColumnLoaded(ZMapWindowContainerBlock      container_block,
						ZMapWindowContainerFeatureSet container_set,
						int world1, int world2);


ZMapWindowContainerBlock zMapWindowContainerBlockDestroy(ZMapWindowContainerBlock canvas_item);


#endif /* ! __ZMAP_WINDOW_CONTAINER_BLOCK_H__ */
