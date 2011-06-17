/*  File: zmapUtilsHandle.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Add allocated memory to handle which can be called to
 *              free all the attached memory in one go. This is a bit
 *              like calling destructors in C++...but only a bit like it.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_MEMHANDLE_H
#define ZMAP_MEMHANDLE_H

#include <glib.h>


/*! @addtogroup zmapmemoryhandle
 * @{
 *  */


/*!
 * Memory handle, an opaque type which can be used to hold and free blocks of memory. */
typedef struct _ZMapMemoryHandleStruct *ZMapMemoryHandle ;


/*!
 * User specified function to free a memory block, called when the memory handle
 * is destroyed. */
typedef void (*ZMapMemoryHandleFreeFunc)(gpointer memory, gpointer user_data) ;


/*! @} end of zmapmemoryhandle docs. */




ZMapMemoryHandle zMapMemoryHandleCreate(void) ;
ZMapMemoryHandle zMapMemoryHandleCreateOnHandle(ZMapMemoryHandle memory_handle) ;
void zMapMemoryHandleAdd(ZMapMemoryHandle memory_handle, gpointer memory,
			 ZMapMemoryHandleFreeFunc memory_free_func, gpointer user_data) ;
gboolean zMapMemoryHandleRemove(ZMapMemoryHandle memory_handle, gpointer memory, gboolean free_mem) ;
void zMapMemoryHandleDestroy(ZMapMemoryHandle memory_handle) ;


#endif /* !ZMAP_MEMHANDLE_H */
