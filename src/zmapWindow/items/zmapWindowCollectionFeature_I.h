/*  File: zmapWindowCollectionFeature_I.h
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
 * Last edited: Feb 15 15:49 2010 (edgrif)
 * Created: Wed Dec  3 08:25:28 2008 (rds)
 * CVS info:   $Id: zmapWindowCollectionFeature_I.h,v 1.2 2010-02-16 10:24:31 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_COLLECTION_FEATURE_I_H
#define ZMAP_WINDOW_COLLECTION_FEATURE_I_H

#include <zmapWindowCanvasItem_I.h>
#include <zmapWindowCollectionFeature.h>

typedef struct _zmapWindowCollectionFeatureClassStruct
{
  zmapWindowCanvasItemClass __parent__;

} zmapWindowCollectionFeatureClassStruct;

typedef struct _zmapWindowCollectionFeatureStruct
{
  zmapWindowCanvasItem __parent__;

  FooCanvasItem *item;

} zmapWindowCollectionFeatureStruct;


#endif /* ZMAP_WINDOW_COLLECTION_FEATURE_I_H */
