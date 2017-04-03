/*  File: zmapWindowContainerContext.h
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

#ifndef ZMAP_WINDOW_CONTAINER_CONTEXT_H
#define ZMAP_WINDOW_CONTAINER_CONTEXT_H

#include <glib-object.h>
#include <libzmapfoocanvas/libfoocanvas.h>
#include <zmapWindowContainerGroup_I.hpp>

#define ZMAP_WINDOW_CONTAINER_CONTEXT_NAME 	"ZMapWindowContainerContext"


#define ZMAP_TYPE_CONTAINER_CONTEXT           (zmapWindowContainerContextGetType())

#define ZMAP_CONTAINER_CONTEXT(obj)	      (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_CONTEXT, zmapWindowContainerContext))
#define ZMAP_CONTAINER_CONTEXT_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_CONTEXT, zmapWindowContainerContext const))

#define ZMAP_CONTAINER_CONTEXT_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_CONTAINER_CONTEXT, zmapWindowContainerContextClass))
#define ZMAP_IS_CONTAINER_CONTEXT(obj)	      (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_CONTAINER_CONTEXT))
#define ZMAP_CONTAINER_CONTEXT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_CONTAINER_CONTEXT, zmapWindowContainerContextClass))


/* Instance */
typedef struct _zmapWindowContainerContextStruct  zmapWindowContainerContext, *ZMapWindowContainerContext ;


/* Class */
typedef struct _zmapWindowContainerContextClassStruct  zmapWindowContainerContextClass, *ZMapWindowContainerContextClass ;


/* Public funcs */
GType zmapWindowContainerContextGetType(void);

ZMapWindowContainerContext zMapWindowContainerContextCreate(FooCanvasGroup *parent);
ZMapWindowContainerContext zMapWindowContainerContextDestroy(ZMapWindowContainerContext canvas_item);


#endif /* ZMAP_WINDOW_CONTAINER_CONTEXT_H */
