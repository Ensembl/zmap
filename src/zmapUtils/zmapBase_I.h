/*  File: zmapZMap_I.h
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
 * Last edited: Jun 12 14:13 2008 (rds)
 * Created: Thu Jun 12 12:02:56 2008 (rds)
 * CVS info:   $Id: zmapBase_I.h,v 1.1 2008-06-13 08:52:59 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef __ZMAP_BASE_I_H__
#define __ZMAP_BASE_I_H__

#include <ZMap/zmapBase.h>

#define ZMAP_PARAM_STATIC    (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
#define ZMAP_PARAM_STATIC_RW (ZMAP_PARAM_STATIC | G_PARAM_READWRITE)
#define ZMAP_PARAM_STATIC_RO (ZMAP_PARAM_STATIC | G_PARAM_READABLE)


typedef struct _zmapBaseStruct
{
  GObject __parent__;

  unsigned int debug : 1;

} zmapBaseStruct;

typedef struct _zmapBaseClassStruct
{
  GObjectClass __parent__;

  /* similar to gobject_class->set_property, but required for copy construction */
  GObjectSetPropertyFunc copy_set_property;

} zmapBaseClassStruct;

#endif /* __ZMAP_BASE_I_H__ */
