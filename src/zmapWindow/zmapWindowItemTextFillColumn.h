/*  File: zmapWindowItemTextFillColumn.h
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jun 16 08:48 2009 (rds)
 * Created: Mon Apr  2 11:51:25 2007 (rds)
 * CVS info:   $Id: zmapWindowItemTextFillColumn.h,v 1.7 2010-06-14 15:40:16 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAPWINDOWITEMTEXTFILLCOLUMN_H
#define ZMAPWINDOWITEMTEXTFILLCOLUMN_H

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsFoo.h>

/* enum for the return item coords for 
 * zmapWindowItemTextIndexGetBounds()  
 * TL means Top Left, BR means Bottom Right
 */
enum
  {
    ITEMTEXT_CHAR_BOUND_TL_X  = 0,
    ITEMTEXT_CHAR_BOUND_TL_Y  = 1,
    ITEMTEXT_CHAR_BOUND_BR_X  = 2,
    ITEMTEXT_CHAR_BOUND_BR_Y  = 3,
    ITEMTEXT_CHAR_BOUND_COUNT = 4
  };

void zmapWindowItemTextOverlayFromCellBounds(FooCanvasPoints *overlay_points,
					     double          *first, 
					     double          *last, 
					     double           minx, 
					     double           maxx);
void zmapWindowItemTextOverlayText2Overlay(FooCanvasItem   *item, 
					   FooCanvasPoints *points);
gboolean zmapWindowItemTextIndex2Item(FooCanvasItem *item,
				      int index,
				      double *bounds);


#endif /* ZMAPWINDOWITEMTEXTFILLCOLUMN_H  */
