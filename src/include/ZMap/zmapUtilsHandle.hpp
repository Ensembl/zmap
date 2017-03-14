/*  File: zmapUtilsHandle.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
