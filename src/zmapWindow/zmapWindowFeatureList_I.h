/*  File: zmapGUITreeView_I.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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

#ifndef __ZMAP_WINDOWFEATURELIST_I_H__
#define __ZMAP_WINDOWFEATURELIST_I_H__


#include <zmapGUITreeView_I.h>
#include <zmapWindowFeatureList.h>
#include <zmapWindow_P.h>

/*
 * Type checking and casting macros
 */
/* In public header now... */

/*
 * Main object structure
 */

typedef struct _zmapWindowFeatureListStruct 
{
  zmapGUITreeViewStruct __parent__;

  ZMapWindow window;		/* transient! */

  ZMapStyleMode feature_type;
} zmapWindowFeatureListStruct;


/*
 * Class definition
 */

typedef struct _zmapWindowFeatureListClassStruct {
  zmapGUITreeViewClassStruct __parent__;

} zmapWindowFeatureListClassStruct;


/*
 * Public methods
 */

/* In public header... */




/* FeatureItem... */
/*
 * Type checking and casting macros
 */
/* In public header now... */

/*
 * Main object structure
 */

typedef struct _zmapWindowFeatureItemListStruct 
{
  zmapGUITreeViewStruct __parent__;

  ZMapWindow window;		/* transient! */

  ZMapStyleMode feature_type;
} zmapWindowFeatureItemListStruct;


/*
 * Class definition
 */

typedef struct _zmapWindowFeatureItemListClassStruct {
  zmapGUITreeViewClassStruct __parent__;

} zmapWindowFeatureItemListClassStruct;


/*
 * Public methods
 */

/* In public header... */


#endif /* __ZMAP_WINDOWFEATURELIST_I_H__ */
