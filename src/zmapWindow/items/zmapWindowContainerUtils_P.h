/*  File: zmapWindowContainerUtils_P.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2009: Genome Research Ltd.
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
 * Last edited: Feb 15 17:21 2010 (edgrif)
 * Created: Wed May 20 08:33:22 2009 (rds)
 * CVS info:   $Id: zmapWindowContainerUtils_P.h,v 1.5 2010-02-16 10:32:29 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_CONTAINER_UTILS_P_H
#define ZMAP_WINDOW_CONTAINER_UTILS_P_H

#include <zmapWindowContainerUtils.h>


/* enums, macros etc... */
#define ZMAP_PARAM_STATIC    (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
#define ZMAP_PARAM_STATIC_RW (ZMAP_PARAM_STATIC   | G_PARAM_READWRITE)
#define ZMAP_PARAM_STATIC_RO (ZMAP_PARAM_STATIC   | G_PARAM_READABLE)
#define ZMAP_PARAM_STATIC_WO (ZMAP_PARAM_STATIC   | G_PARAM_WRITABLE)

#define CONTAINER_DATA     "container_data"
#define CONTAINER_TYPE_KEY "container_type"


typedef enum
  {
    CONTAINER_GROUP_INVALID,
    CONTAINER_GROUP_ROOT,
    CONTAINER_GROUP_PARENT,
    CONTAINER_GROUP_OVERLAYS,
    CONTAINER_GROUP_FEATURES,
    CONTAINER_GROUP_BACKGROUND,
    CONTAINER_GROUP_UNDERLAYS,
    CONTAINER_GROUP_COUNT
  } ZMapWindowContainerType;


#endif /* !ZMAP_WINDOW_CONTAINER_UTILS_P_H */
