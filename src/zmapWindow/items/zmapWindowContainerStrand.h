/*  File: zmapWindowContainerStrand.h
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

#ifndef __ZMAP_WINDOW_CONTAINER_STRAND_H__
#define __ZMAP_WINDOW_CONTAINER_STRAND_H__

#include <glib-object.h>
#include <libzmapfoocanvas/libfoocanvas.h>
#include <ZMap/zmapFeature.h>

#define ZMAP_WINDOW_CONTAINER_STRAND_NAME 	"ZMapWindowContainerStrand"


#define ZMAP_TYPE_CONTAINER_STRAND           (zmapWindowContainerStrandGetType())

#if GOBJ_CAST
#define ZMAP_CONTAINER_STRAND(obj)       ((ZMapWindowContainerStrand) obj)
#define ZMAP_CONTAINER_STRAND_CONST(obj) ((ZMapWindowContainerStrand const) obj)
#else
#define ZMAP_CONTAINER_STRAND(obj)	     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_STRAND, zmapWindowContainerStrand))
#define ZMAP_CONTAINER_STRAND_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_STRAND, zmapWindowContainerStrand const))
#endif

#define ZMAP_CONTAINER_STRAND_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_CONTAINER_STRAND, zmapWindowContainerStrandClass))
#define ZMAP_IS_CONTAINER_STRAND(obj)	     (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_CONTAINER_STRAND))
#define ZMAP_CONTAINER_STRAND_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_CONTAINER_STRAND, zmapWindowContainerStrandClass))


/* Instance */
typedef struct _zmapWindowContainerStrandStruct  zmapWindowContainerStrand, *ZMapWindowContainerStrand ;


/* Class */
typedef struct _zmapWindowContainerStrandClassStruct  zmapWindowContainerStrandClass, *ZMapWindowContainerStrandClass ;


/* Public funcs */
GType zmapWindowContainerStrandGetType(void);

ZMapWindowContainerStrand zmapWindowContainerStrandAugment(ZMapWindowContainerStrand container_strand,
							   ZMapStrand strand);
void zmapWindowContainerStrandSetAsSeparator(ZMapWindowContainerStrand container_strand);

ZMapWindowContainerStrand zMapWindowContainerStrandDestroy(ZMapWindowContainerStrand container_strand);


#endif /* ! __ZMAP_WINDOW_CONTAINER_STRAND_H__ */
