/*  File: zmapGUITreeView_I.h
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
 * Last edited: Jun  4 14:19 2008 (rds)
 * Created: Thu May 22 10:49:23 2008 (rds)
 * CVS info:   $Id: zmapWindowDNAList_I.h,v 1.3 2010-03-04 13:07:59 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef __ZMAP_WINDOWDNALIST_I_H__
#define __ZMAP_WINDOWDNALIST_I_H__

#include <zmapWindowDNAList.h>
#include <zmapGUITreeView_I.h>      /* Finding this might be difficult? */

/*
 * Type checking and casting macros
 */
/* In public header now... */

/*
 * Main object structure
 */

typedef struct _zmapWindowDNAListStruct
{
  zmapGUITreeViewStruct __parent__;

  ZMapStyleMode feature_type;
} zmapWindowDNAListStruct;


/*
 * Class definition
 */

typedef struct _zmapWindowDNAListClassStruct {
  zmapGUITreeViewClassStruct __parent__;

} zmapWindowDNAListClassStruct;


#endif /* __ZMAP_WINDOWDNALIST_I_H__ */
