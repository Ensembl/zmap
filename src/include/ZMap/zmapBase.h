/*  File: zmapBase.h
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
 * Last edited: Jun 12 14:07 2008 (rds)
 * Created: Thu Jun 12 12:09:39 2008 (rds)
 * CVS info:   $Id: zmapBase.h,v 1.1 2008-06-13 08:53:29 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef __ZMAP_BASE_H__
#define __ZMAP_BASE_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>


/*
 * Type checking and casting macros
 */
#define ZMAP_TYPE_BASE	         (zMapBaseGetType())
#define ZMAP_BASE(obj)	         (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_BASE, zmapBase))
#define ZMAP_BASE_CONST(obj)	 (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_BASE, zmapBase const))
#define ZMAP_BASE_CLASS(klass)	 (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_BASE, zmapBaseClass))
#define ZMAP_IS_BASE(obj)	 (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_BASE))
#define ZMAP_BASE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_BASE, zmapBaseClass))

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

#endif /* __ZMAP_BASE_H__ */
