/*  File: zmapZMap_I.h
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
 * Last edited: Jan 12 10:03 2009 (rds)
 * Created: Thu Jun 12 12:02:56 2008 (rds)
 * CVS info:   $Id: zmapBase_I.h,v 1.5 2010-03-04 15:11:00 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef __ZMAP_BASE_I_H__
#define __ZMAP_BASE_I_H__

#include <ZMap/zmapBase.h>


typedef void (* ZMapBaseValueCopyFunc)(const GValue *src_value, GValue *dest_value);

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

  /* Our version of the GTypeValueTable->value_copy function.
   * Ordinarily we'd just use the value_table member of GTypeInfo,
   * but threading scuppers that. */
  void (*value_copy)(const GValue *src_value, GValue *dest_value);

} zmapBaseClassStruct;

#endif /* __ZMAP_BASE_I_H__ */
