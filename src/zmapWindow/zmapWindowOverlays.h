/*  File: zmapWindowOverlays.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * Last edited: Jun 14 17:16 2007 (rds)
 * Created: Mon Mar 12 12:28:40 2007 (rds)
 * CVS info:   $Id: zmapWindowOverlays.h,v 1.2 2007-06-15 09:43:52 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_OVERLAYS_H
#define ZMAP_WINDOW_OVERLAYS_H

typedef struct _ZMapWindowOverlayStruct *ZMapWindowOverlay;

typedef gboolean (*ZMapWindowOverlaySizeRequestCB)(FooCanvasPoints **points_out, FooCanvasItem *item_subject, gpointer user_data);


ZMapWindowOverlay zmapWindowOverlayCreate(FooCanvasItem *parent,
                                          FooCanvasItem *subject);
void zmapWindowOverlayUpdate(ZMapWindowOverlay overlay);
void zmapWindowOverlaySetLimitItem(ZMapWindowOverlay overlay, FooCanvasItem *limit_item);
void zmapWindowOverlaySetSubject(ZMapWindowOverlay overlay, FooCanvasItem *subject);
void zmapWindowOverlaySetSizeRequestor(ZMapWindowOverlay overlay, 
                                       ZMapWindowOverlaySizeRequestCB request_cb,
                                       gpointer user_data);
void zmapWindowOverlaySetGdkBitmap(ZMapWindowOverlay overlay, GdkBitmap *bitmap);
void zmapWindowOverlaySetGdkColor(ZMapWindowOverlay overlay, char *colour);
void zmapWindowOverlaySetGdkColorFromGdkColor(ZMapWindowOverlay overlay, GdkColor *input);

FooCanvasItem *zmapWindowOverlayLimitItem(ZMapWindowOverlay overlay);

void zmapWindowOverlayMask(ZMapWindowOverlay overlay);
void zmapWindowOverlayUnmask(ZMapWindowOverlay overlay);
void zmapWindowOverlayUnmaskAll(ZMapWindowOverlay overlay);

ZMapWindowOverlay zmapWindowOverlayDestroy(ZMapWindowOverlay overlay);

#endif /* ZMAP_WINDOW_OVERLAYS_H */
