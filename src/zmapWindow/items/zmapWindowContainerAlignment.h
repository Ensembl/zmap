/*  File: zmapWindowContainerAlignment.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 * Last edited: Jun 10 14:53 2009 (rds)
 * Created: Wed Dec  3 08:21:03 2008 (rds)
 * CVS info:   $Id: zmapWindowContainerAlignment.h,v 1.4 2010-06-08 08:31:26 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_CONTAINER_ALIGNMENT_H
#define ZMAP_WINDOW_CONTAINER_ALIGNMENT_H

#include <glib-object.h>
#include <libfoocanvas/libfoocanvas.h>
#include <zmapWindowContainerGroup_I.h>

#define ZMAP_WINDOW_CONTAINER_ALIGNMENT_NAME 	"ZMapWindowContainerAlignment"


#define ZMAP_TYPE_CONTAINER_ALIGNMENT           (zmapWindowContainerAlignmentGetType())

#if GOBJ_CAST
#define ZMAP_CONTAINER_ALIGNMENT(obj)           ((ZMapWindowContainerAlignment) obj)
#define ZMAP_CONTAINER_ALIGNMENT_CONST(obj)     ((ZMapWindowContainerAlignment const) obj)
#else
#define ZMAP_CONTAINER_ALIGNMENT(obj)	        (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_ALIGNMENT, zmapWindowContainerAlignment))
#define ZMAP_CONTAINER_ALIGNMENT_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_ALIGNMENT, zmapWindowContainerAlignment const))
#endif

#define ZMAP_CONTAINER_ALIGNMENT_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_CONTAINER_ALIGNMENT, zmapWindowContainerAlignmentClass))
#define ZMAP_IS_CONTAINER_ALIGNMENT(obj)	(G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_CONTAINER_ALIGNMENT))
#define ZMAP_CONTAINER_ALIGNMENT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_CONTAINER_ALIGNMENT, zmapWindowContainerAlignmentClass))


/* Instance */
typedef struct _zmapWindowContainerAlignmentStruct  zmapWindowContainerAlignment, *ZMapWindowContainerAlignment ;


/* Class */
typedef struct _zmapWindowContainerAlignmentClassStruct  zmapWindowContainerAlignmentClass, *ZMapWindowContainerAlignmentClass ;


/* Public funcs */
GType zmapWindowContainerAlignmentGetType(void);

ZMapWindowContainerAlignment zmapWindowContainerAlignmentCreate(FooCanvasGroup *parent);
ZMapWindowContainerAlignment zmapWindowContainerAlignmentAugment(ZMapWindowContainerAlignment alignment,
								 ZMapFeatureAlignment feature);
ZMapWindowContainerAlignment zMapWindowContainerAlignmentDestroy(ZMapWindowContainerAlignment canvas_item);


#endif /* ZMAP_WINDOW_CONTAINER_ALIGNMENT_H */
