/*  File: zmapWindowContainerContext.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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