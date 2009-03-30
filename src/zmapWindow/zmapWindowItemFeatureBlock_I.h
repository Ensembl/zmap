/*  File: zmapWindowItemFeatureBlock_I.h
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
 * Last edited: Feb 17 11:12 2009 (rds)
 * Created: Fri Feb  6 11:49:03 2009 (rds)
 * CVS info:   $Id: zmapWindowItemFeatureBlock_I.h,v 1.1 2009-03-30 09:44:54 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef __ZMAP_WINDOW_ITEM_FEATURE_BLOCK_I_H__
#define __ZMAP_WINDOW_ITEM_FEATURE_BLOCK_I_H__

#include <zmapWindow_P.h>
#include <zmapWindowItemFeatureBlock.h>

#define ZMAP_PARAM_STATIC (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
#define ZMAP_PARAM_STATIC_RW (ZMAP_PARAM_STATIC | G_PARAM_READWRITE)
#define ZMAP_PARAM_STATIC_RO (ZMAP_PARAM_STATIC | G_PARAM_READABLE)


#define ZMAP_BIN_MAX_VALUE(BITMAP) ((1 << BITMAP->bin_depth) - 1)

typedef struct _zmapWindowItemFeatureBlockDataStruct
{
  GObject __parent__;

  ZMapWindow  window;

  GList      *compressed_cols, *bumped_cols ;
  
  GHashTable *loaded_region_hash;

} zmapWindowItemFeatureBlockDataStruct;


typedef struct _zmapWindowItemFeatureBlockDataClassStruct
{
  GObjectClass __parent__;

} zmapWindowItemFeatureBlockDataClassStruct;



#endif /* __ZMAP_WINDOW_ITEM_FEATURE_BLOCK_I_H__ */
