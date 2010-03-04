/*  File: zmapWindowTextPositioner.h
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
 * Last edited: Jun  5 16:38 2008 (rds)
 * Created: Tue Mar 13 09:48:56 2007 (rds)
 * CVS info:   $Id: zmapWindowTextPositioner.h,v 1.3 2010-03-04 15:13:27 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_TEXT_POSITIONER_H
#define ZMAP_WINDOW_TEXT_POSITIONER_H

#define ZMAPWINDOWTEXT_ITEM_TO_LINE "text_item_to_line"

typedef struct _ZMapWindowTextPositionerStruct *ZMapWindowTextPositioner;

ZMapWindowTextPositioner zmapWindowTextPositionerCreate(double column_min, double column_max);

void zmapWindowTextPositionerAddItem(ZMapWindowTextPositioner positioner, FooCanvasItem *item);
void zmapWindowTextPositionerUnOverlap(ZMapWindowTextPositioner positioner, gboolean draw_lines);
void zmapWindowTextPositionerDestroy(ZMapWindowTextPositioner positioner);


#endif /* ZMAP_WINDOW_TEXT_POSITIONER_H */
