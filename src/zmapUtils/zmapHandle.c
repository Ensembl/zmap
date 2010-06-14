/*  File: zmapHandle.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Rewrite of acedb's "handle" system for freeing loads of
 *              memory with one call.
 *              
 *              NOTE that we cannot simply use the acedb code because we
 *              have lots of memory that is not allocated by us but by
 *              glib.
 *
 * Exported functions: See ZMap/zMapMemoryHandle.h
 * HISTORY:
 * Last edited: May  5 11:21 2006 (rds)
 * Created: Tue Nov 29 15:27:32 2005 (edgrif)
 * CVS info:   $Id: zmapHandle.c,v 1.6 2010-06-14 15:40:14 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsHandle.h>


typedef struct _ZMapMemoryBlockStruct
{
  gpointer memory ;
  ZMapMemoryHandleFreeFunc free_func ;
  gpointer user_data ;
} ZMapMemoryBlockStruct, *ZMapMemoryBlock ;


typedef struct _ZMapMemoryHandleStruct
{
  GList *blocks ;					    /* list of ZMapMemoryBlock */
} ZMapMemoryHandleStruct ;



static void defaultMemoryFree(gpointer memory, gpointer unused_user_data) ;
static void handleDestroy(gpointer memory, gpointer unused_user_data) ;
static void freeBlockCB(gpointer data, gpointer user_data) ;




/*! @defgroup zmapmemoryhandle   zMapMemoryHandle: package for freeing of sets of memory blocks.
 * @{
 * 
 * \brief  Package for freeing sets of memory blocks.
 * 
 * zMapMemoryHandle routines allow the freeing of sets of memory blocks by one application call.
 * Each block can be free'd by a custom routine which can be supplied with user data or by default
 * is freed with a call to g_free().
 * 
 * This package is completely inspired by the Acedb "STORE_HANDLE" package which I think was
 * largely coded by Simon Kelley (srk@sanger.ac.uk). Note that it is a complete rewrite as this
 * package uses glib functions and works in a different way: the caller allocates the memory and
 * passes in a pointer to that memory. This makes the package more flexible in that the memory
 * could have been allocated by any package.
 * 
 *  */



/*!
 * Creates a memory handle on which to allocate memory blocks.
 *
 * @return Returns a pointer to memory handle.
 *  */
ZMapMemoryHandle zMapMemoryHandleCreate(void)
{
  ZMapMemoryHandle handle = NULL ;

  handle = zMapMemoryHandleCreateOnHandle(NULL) ;

  return handle ;
}


/*!
 * Creates a memory handle on which to allocate memory blocks, the handle is itself
 * allocated on a memory handle. When the parent memory handle is destroyed, it will
 * call an internal routine which will ensure that this child handle is also destroyed.
 *
 * If the memory_handle parameter is NULL then this call is the same as zMapMemoryHandleCreate().
 *
 * @param memory_handle      The handle on which the new handle should be allocated.
 * @return Returns a pointer to memory handle.
 *  */
ZMapMemoryHandle zMapMemoryHandleCreateOnHandle(ZMapMemoryHandle memory_handle)
{
  ZMapMemoryHandle new_handle = NULL ;

  new_handle = g_new0(ZMapMemoryHandleStruct, 1) ;

  if (memory_handle)
    zMapMemoryHandleAdd(memory_handle,
			new_handle, handleDestroy, new_handle) ;

  return new_handle ;
}



/*!
 * Adds a memory block to the handle. If no memory_free_func is given then g_free()
 * is used to free the memory so it <B>must</B> have been allocated with one of the
 * g_XXXX() memory functions originally. If a memory_free_func is specified, it will
 * be called to free the memory block with user_data as a parameter.
 *
 * <B>NOTE</B> that it is the <B>callers responsibility</B> to ensure that the user_data
 * is still valid when the memory_free_func accesses it.
 *
 * @param memory_handle      The handle on which the block should be allocated.
 * @param memory             The memory block.
 * @param memory_free_func   Optional function to call to free the memory.
 * @param user_data          Optional data to pass to the memory_free_func.
 * @return   <nothing>
 *  */
void zMapMemoryHandleAdd(ZMapMemoryHandle memory_handle,
			 gpointer memory, ZMapMemoryHandleFreeFunc memory_free_func, gpointer user_data)
{
  ZMapMemoryBlock block ;

  zMapAssert(memory
	     && (memory_free_func || (!memory_free_func && !user_data))) ;

  block = g_new0(ZMapMemoryBlockStruct, 1) ;
  block->memory = memory ;

  if (memory_free_func)
    block->free_func = memory_free_func ;
  else
    block->free_func = defaultMemoryFree ;

  block->user_data = user_data ;

  return ;
}


/*!
 * Finds the memory block and removes it from the handle, optionally freeing
 * the memory block as well.
 *
 * @param memory_handle  The handle on which the block was allocated.
 * @param memory         The memory block to be removed from the list.
 * @param free_mem       If TRUE then memory block is freed as well.
 * @return  TRUE if memory block was found and removed, FALSE otherwise.
 *  */
gboolean zMapMemoryHandleRemove(ZMapMemoryHandle memory_handle, gpointer memory, gboolean free_mem)
{
  gboolean result = FALSE ;
  GList *item ;

  if ((item = g_list_find(memory_handle->blocks, memory)))
    {
      /* ZMapMemoryBlock block = item->data ; */

      freeBlockCB(memory, GINT_TO_POINTER(free_mem)) ;

      memory_handle->blocks = g_list_delete_link(memory_handle->blocks, item) ;
    }

  return result ;
}



/*!
 * Destroys a handle and all the attached memory blocks, calling the appropriate
 * free functions.
 *
 * @param memory_handle  The handle to destroy.
 * @return  <nothing>
 *  */
void zMapMemoryHandleDestroy(ZMapMemoryHandle memory_handle)
{

  zMapAssert(memory_handle) ;

  handleDestroy((gpointer)memory_handle, NULL) ;

  return ;
}


/*! @} end of zmapmemoryhandle docs. */





/*
 *  ------------------- Internal functions -------------------
 */


/* Default block memory free routine, this is called to free memory unless user has specified
 * their own memory free routine. (Lets hope the user used one of the g_XXXX()
 * routines to allocate the memory if they don't specify their own free routine !) */
static void defaultMemoryFree(gpointer memory, gpointer unused_user_data)
{

  g_free(memory) ;

  return ;
}


/* Destroy a memory handle, may be called either directly from zMapMemoryHandleDestroy() or
 * indirectly if a handle was allocated on a handle and then the top level handle is destroyed. */
static void handleDestroy(gpointer memory, gpointer unused_user_data)
{
  ZMapMemoryHandle memory_handle = (ZMapMemoryHandle)memory ;

  zMapAssert(memory_handle) ;

  g_list_foreach(memory_handle->blocks, freeBlockCB, NULL) ;

  g_list_free(memory_handle->blocks) ;

  g_free(memory_handle) ;

  return ;
}



/* A GFunc callback for GLists, frees the memory block if required and then frees the handle block. */
static void freeBlockCB(gpointer data, gpointer user_data)
{
  ZMapMemoryBlock block = (ZMapMemoryBlock)data ;
  gboolean free_mem = GPOINTER_TO_INT(user_data) ;

  if (free_mem)
    block->free_func(block->memory, block->user_data) ;

  g_free(block) ;

  return ;
}

