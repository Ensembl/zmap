/*  File: zmapWindowAlignmentFeature_I.h
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
 * Last edited: Feb 15 15:50 2010 (edgrif)
 * Created: Wed Dec  3 08:25:28 2008 (rds)
 * CVS info:   $Id: zmapWindowAlignmentFeature_I.h,v 1.2 2010-02-16 10:15:37 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_ALIGNMENT_FEATURE_I_H
#define ZMAP_WINDOW_ALIGNMENT_FEATURE_I_H

#include <zmapWindowCanvasItem.h>
#include <zmapWindowAlignmentFeature.h>



typedef struct _zmapWindowAlignmentFeatureClassStruct
{
  zmapWindowCanvasItemClass __parent__;

  /* do we need to extend the interface? */
} zmapWindowAlignmentFeatureClassStruct;

typedef struct _zmapWindowAlignmentFeatureStruct
{
  zmapWindowCanvasItem __parent__;

  FooCanvasItem       *no_gaps_hidden_item;
  struct
  {
    unsigned int gapped_display  : 1;
    unsigned int no_gaps_display : 1;
    unsigned int no_gaps_hidden  : 1;
  }flags;
} zmapWindowAlignmentFeatureStruct;


#endif /* ZMAP_WINDOW_ALIGNMENT_FEATURE_I_H */

/* 
;; Local Variables: **
;; mode:c **
;; End: **
*/
