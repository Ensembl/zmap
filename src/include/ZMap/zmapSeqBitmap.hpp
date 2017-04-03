/*  File: zmapSeqBitmap.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
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
