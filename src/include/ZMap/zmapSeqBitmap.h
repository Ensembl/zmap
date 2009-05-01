/*  File: zmapSeqBitmap.h
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
 * Last edited: Apr 16 15:16 2009 (rds)
 * Created: Tue Feb 17 10:24:32 2009 (rds)
 * CVS info:   $Id: zmapSeqBitmap.h,v 1.2 2009-05-01 17:01:12 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef __ZMAP_SEQ_BITMAP_H__
#define __ZMAP_SEQ_BITMAP_H__

#include <glib.h>

typedef struct _zmapSeqBitmapStruct *ZMapSeqBitmap;


/* public */
ZMapSeqBitmap zmapSeqBitmapCreate    (int start, int size, int bin_size);
void          zmapSeqBitmapMarkRegion(ZMapSeqBitmap bitmap, int world1, int world2);
void          zmapSeqBitmapPrint     (ZMapSeqBitmap bitmap);
ZMapSeqBitmap zmapSeqBitmapDestroy   (ZMapSeqBitmap bitmap);

gboolean zmapSeqBitmapIsRegionFullyMarked(ZMapSeqBitmap bitmap, int world1, int world2);


#endif /* __ZMAP_SEQ_BITMAP_H__ */
