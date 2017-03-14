/*  File: zmapWindowDNAList.h
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

#ifndef __ZMAP_WINDOWDNALIST_H__
#define __ZMAP_WINDOWDNALIST_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <ZMap/zmapGUITreeView.hpp>

#define ZMAP_WINDOWDNALIST_MATCH_COLUMN_NAME   "Match"
#define ZMAP_WINDOWDNALIST_START_COLUMN_NAME   "-start-"
#define ZMAP_WINDOWDNALIST_END_COLUMN_NAME     "-end-"
#define ZMAP_WINDOWDNALIST_STRAND_COLUMN_NAME  "Strand"
#define ZMAP_WINDOWDNALIST_FRAME_COLUMN_NAME   "Frame"
#define ZMAP_WINDOWDNALIST_SEQTYPE_COLUMN_NAME "-match-type-"

#define ZMAP_WINDOWDNALIST_STRAND_ENUM_COLUMN_NAME  "-strand-enum-"
#define ZMAP_WINDOWDNALIST_FRAME_ENUM_COLUMN_NAME   "-frame-enum-"


/*
 * Type checking and casting macros
 */

#define ZMAP_TYPE_WINDOWDNALIST           (zMapWindowDNAListGetType())
#define ZMAP_WINDOWDNALIST(obj)	          G_TYPE_CHECK_INSTANCE_CAST((obj), zMapWindowDNAListGetType(), zmapWindowDNAList)
#define ZMAP_WINDOWDNALIST_CONST(obj)	  G_TYPE_CHECK_INSTANCE_CAST((obj), zMapWindowDNAListGetType(), zmapWindowDNAList const)
#define ZMAP_WINDOWDNALIST_CLASS(klass)	  G_TYPE_CHECK_CLASS_CAST((klass),  zMapWindowDNAListGetType(), zmapWindowDNAListClass)
#define ZMAP_IS_WINDOWDNALIST(obj)	  G_TYPE_CHECK_INSTANCE_TYPE((obj), zMapWindowDNAListGetType())
#define ZMAP_WINDOWDNALIST_GET_CLASS(obj) G_TYPE_INSTANCE_GET_CLASS((obj),  zMapWindowDNAListGetType(), zmapWindowDNAListClass)


/*
 * Main object structure
 */

typedef struct _zmapWindowDNAListStruct *ZMapWindowDNAList;

typedef struct _zmapWindowDNAListStruct  zmapWindowDNAList;

/*
 * Class definition
 */
typedef struct _zmapWindowDNAListClassStruct *ZMapWindowDNAListClass;

typedef struct _zmapWindowDNAListClassStruct  zmapWindowDNAListClass;

/*
 * Public methods
 */
GType zMapWindowDNAListGetType (void);
ZMapWindowDNAList zMapWindowDNAListCreate(void);
void zMapWindowDNAListAddMatch(ZMapWindowDNAList zmap_tv,
			       ZMapDNAMatch match);
void zMapWindowDNAListAddMatches(ZMapWindowDNAList zmap_tv,
				 GList *list_of_matches);


#endif /* __ZMAP_GUITREEVIEW_H__ */
