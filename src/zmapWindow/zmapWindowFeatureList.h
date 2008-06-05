/*  File: zmapWindowFeatureList.h
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
 * Last edited: Jun  5 18:04 2008 (rds)
 * Created: Wed Jun  4 13:17:50 2008 (rds)
 * CVS info:   $Id: zmapWindowFeatureList.h,v 1.2 2008-06-05 17:21:50 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef __ZMAP_WINDOWFEATURELIST_H__
#define __ZMAP_WINDOWFEATURELIST_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include <ZMap/zmapStyle.h>
#include <ZMap/zmapGUITreeView.h>

#include <zmapWindow_P.h>


#define ZMAP_WINDOWFEATURELIST_NAME_COLUMN_NAME    "Name"
#define ZMAP_WINDOWFEATURELIST_START_COLUMN_NAME   "Start"
#define ZMAP_WINDOWFEATURELIST_END_COLUMN_NAME     "End"
#define ZMAP_WINDOWFEATURELIST_STRAND_COLUMN_NAME  "Strand"
#define ZMAP_WINDOWFEATURELIST_FRAME_COLUMN_NAME   "Frame"
#define ZMAP_WINDOWFEATURELIST_PHASE_COLUMN_NAME   "Phase"
#define ZMAP_WINDOWFEATURELIST_QSTART_COLUMN_NAME  "Query Start"
#define ZMAP_WINDOWFEATURELIST_QEND_COLUMN_NAME    "Query End"
#define ZMAP_WINDOWFEATURELIST_QSTRAND_COLUMN_NAME "Query Strand"
#define ZMAP_WINDOWFEATURELIST_SCORE_COLUMN_NAME   "Score"
#define ZMAP_WINDOWFEATURELIST_SET_COLUMN_NAME     "Feature Set"
#define ZMAP_WINDOWFEATURELIST_TYPE_COLUMN_NAME    "Type"

/*
 * Type checking and casting macros
 */
#define ZMAP_TYPE_WINDOWFEATURELIST            (zMapWindowFeatureListGetType())
#define ZMAP_WINDOWFEATURELIST(obj)	        G_TYPE_CHECK_INSTANCE_CAST((obj), zMapGUITreeViewGetType(), zmapWindowFeatureList)
#define ZMAP_WINDOWFEATURELIST_CONST(obj)	G_TYPE_CHECK_INSTANCE_CAST((obj), zMapGUITreeViewGetType(), zmapWindowFeatureList const)
#define ZMAP_WINDOWFEATURELIST_CLASS(klass)	G_TYPE_CHECK_CLASS_CAST((klass),  zMapGUITreeViewGetType(), zmapWindowFeatureListClass)
#define ZMAP_IS_WINDOWFEATURELIST(obj)	        G_TYPE_CHECK_INSTANCE_TYPE((obj), zMapGUITreeViewGetType())
#define ZMAP_WINDOWFEATURELIST_GET_CLASS(obj)	G_TYPE_INSTANCE_GET_CLASS((obj),  zMapGUITreeViewGetType(), zmapWindowFeatureListClass)


/*
 * Main object structure
 */

typedef struct _zmapWindowFeatureListStruct *ZMapWindowFeatureList;

typedef struct _zmapWindowFeatureListStruct  zmapWindowFeatureList;

/*
 * Class definition
 */
typedef struct _zmapWindowFeatureListClassStruct *ZMapWindowFeatureListClass;

typedef struct _zmapWindowFeatureListClassStruct  zmapWindowFeatureListClass;

/*
 * Public methods
 */
GType zMapWindowFeatureListGetType (void);
ZMapWindowFeatureList zMapWindowFeatureListCreate(ZMapStyleMode feature_type);
void zMapWindowFeatureListAddFeature(ZMapWindowFeatureList zmap_tv,
				     ZMapFeatureAny        feature);
void zmapWindowFeatureListAddFeatures(ZMapWindowFeatureList zmap_tv,
				      GList *list_of_features);


/* FeatureItem */

/*
 * Type checking and casting macros
 */

#define ZMAP_TYPE_WINDOWFEATUREITEMLIST           (zMapWindowFeatureItemListGetType())
#define ZMAP_WINDOWFEATUREITEMLIST(obj)	          G_TYPE_CHECK_INSTANCE_CAST((obj), zMapWindowFeatureItemListGetType(), zmapWindowFeatureItemList)
#define ZMAP_WINDOWFEATUREITEMLIST_CONST(obj)     G_TYPE_CHECK_INSTANCE_CAST((obj), zMapWindowFeatureItemListGetType(), zmapWindowFeatureItemList const)
#define ZMAP_WINDOWFEATUREITEMLIST_CLASS(klass)   G_TYPE_CHECK_CLASS_CAST((klass),  zMapWindowFeatureItemListGetType(), zmapWindowFeatureItemListClass)
#define ZMAP_IS_WINDOWFEATUREITEMLIST(obj)	  G_TYPE_CHECK_INSTANCE_TYPE((obj), zMapWindowFeatureItemListGetType())
#define ZMAP_WINDOWFEATUREITEMLIST_GET_CLASS(obj) G_TYPE_INSTANCE_GET_CLASS((obj),  zMapWindowFeatureItemListGetType(), zmapWindowFeatureItemListClass)


/*
 * Main object structure
 */

typedef struct _zmapWindowFeatureItemListStruct *ZMapWindowFeatureItemList;

typedef struct _zmapWindowFeatureItemListStruct  zmapWindowFeatureItemList;

/*
 * Class definition
 */
typedef struct _zmapWindowFeatureItemListClassStruct *ZMapWindowFeatureItemListClass;

typedef struct _zmapWindowFeatureItemListClassStruct  zmapWindowFeatureItemListClass;

/*
 * Public methods
 */
GType zMapWindowFeatureItemListGetType (void);
ZMapWindowFeatureItemList zMapWindowFeatureItemListCreate(ZMapStyleMode feature_type);
void zMapWindowFeatureItemListAddItem(ZMapWindowFeatureItemList zmap_tv,
				      ZMapWindow                window,
				      FooCanvasItem            *feature_item);
void zMapWindowFeatureItemListAddItems(ZMapWindowFeatureItemList zmap_tv,
				       ZMapWindow                window,
				       GList                    *list_of_feature_items);
void zMapWindowFeatureItemListUpdateItem(ZMapWindowFeatureItemList zmap_tv,
					 ZMapWindow                window,
					 GtkTreeIter              *iterator,
					 FooCanvasItem            *feature_item);
void zMapWindowFeatureItemListUpdateAll(ZMapWindowFeatureItemList zmap_tv,
					ZMapWindow                window,
					GHashTable               *context_to_item);

#endif /* __ZMAP_WINDOWFEATURELIST_H__ */
