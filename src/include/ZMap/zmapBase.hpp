/*  File: zmapBase.h
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

#ifndef __ZMAP_BASE_H__
#define __ZMAP_BASE_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#define ZMAP_PARAM_STATIC_STRINGS (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
//(GLIB supposedly defines the above as G_..., but it's not there)
#define ZMAP_PARAM_STATIC_RW (GParamFlags)(ZMAP_PARAM_STATIC_STRINGS | G_PARAM_READWRITE)
#define ZMAP_PARAM_STATIC_RO (GParamFlags)(ZMAP_PARAM_STATIC_STRINGS | G_PARAM_READABLE)
#define ZMAP_PARAM_STATIC_WO (GParamFlags)(ZMAP_PARAM_STATIC_STRINGS | G_PARAM_WRITABLE)

#ifdef MH17_NOT_NEEDED

/*
 * Type checking and casting macros
 */
#define ZMAP_TYPE_BASE	         (zMapBaseGetType())
#define ZMAP_BASE(obj)	         (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_BASE, zmapBase))
#define ZMAP_BASE_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_BASE, zmapBase const))
#define ZMAP_BASE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_BASE, zmapBaseClass))
#define ZMAP_IS_BASE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_BASE))
#define ZMAP_BASE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_BASE, zmapBaseClass))


/* Used as retrieval key for source object for object copy code. */
#define ZMAPBASECOPY_PARAMDATA_KEY "ZMap_Base_Copy_Key"


/*
 * Main object structure
 */
typedef struct _zmapBaseStruct *ZMapBase;

typedef struct _zmapBaseStruct  zmapBase;


/*
 * Class definition
 */
typedef struct _zmapBaseClassStruct *ZMapBaseClass;

typedef struct _zmapBaseClassStruct  zmapBaseClass;

/*
 * Public methods
 */
GType zMapBaseGetType (void);


ZMapBase zMapBaseCopy(ZMapBase src);
gboolean zMapBaseCCopy(ZMapBase src, ZMapBase *dest_out);
#endif      // MH17_NOT_NEEDED

#endif /* __ZMAP_BASE_H__ */
