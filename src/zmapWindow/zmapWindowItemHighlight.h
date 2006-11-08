/*  File: zmapWindowItemHighlight.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * Last edited: Oct 13 08:14 2006 (rds)
 * Created: Thu Oct 12 14:42:05 2006 (rds)
 * CVS info:   $Id: zmapWindowItemHighlight.h,v 1.2 2006-11-08 09:25:20 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAPWINDOW_TEXT_ITEM_HL_H
#define ZMAPWINDOW_TEXT_ITEM_HL_H

#include <zmapWindow_P.h> 

#define ZMAP_TEXT_ITEM_HIGHLIGHT_KEY "item_highlight_data"

typedef struct _ZMapWindowTextItemHLStruct *ZMapWindowTextItemHL;


ZMapWindowTextItemHL zmapWindowTextItemHLCreate(FooCanvasGroup *parent_container);

void zmapWindowTextItemHLAddItem(ZMapWindowTextItemHL object, 
                                 FooCanvasItem *text_item);
void zmapWindowTextItemHLUseContainer(ZMapWindowTextItemHL object, 
                                      FooCanvasGroup *features_container);

void zmapWindowTextItemHLEvent(ZMapWindowTextItemHL object, 
                               FooCanvasItem *receiving_item,
                               GdkEvent *event, gboolean first_event);

void zmapWindowTextItemHLSetFullText(ZMapWindowTextItemHL object, char *text);
gboolean zmapWindowTextItemHLSelectedText(ZMapWindowTextItemHL object, char **selected_text);

void zmapWindowTextItemHLDestroy(ZMapWindowTextItemHL object);



#endif /* ZMAPWINDOW_TEXT_ITEM_HL_H */
