/*  File: zmapUtilsDebug.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Contains macros, functions etc. useful for testing/debugging.
 *              
 * HISTORY:
 * Last edited: Apr  8 17:05 2004 (edgrif)
 * Created: Mon Mar 29 16:51:28 2004 (edgrif)
 * CVS info:   $Id: zmapUtilsDebug.h,v 1.1 2004-04-08 16:14:53 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_DEBUG_H
#define ZMAP_UTILS_DEBUG_H

#include <stdlib.h>
#include <glib.h>


/* Define ZMAP_ASSERT_DISABLE before compiling to disable all asserts. */
#ifdef ZMAP_ASSERT_DISABLE
#define G_DISABLE_ASSERT
#endif


/* Usual assert() usage. */
#define zMapAssert(EXPR)                              \
g_assert((EXPR))


/* Put this in sections of code which should never be reached. */
#define zMapAssertNotReached()                        \
g_assert_not_reached()


/* If this global is TRUE then debugging messages will be displayed. */
extern gboolean zmap_debug_G ;


#ifdef HOME
/* Stupid home system needs updating.... */

#define zMapDebug(FORMAT, ...)


#else

/* Takes the standard printf like args:   ZMAP_DEBUG("format string", lots, of, args) ; */
#define zMapDebug(FORMAT, ...)                            \
G_STMT_START{                                             \
  if (zmap_debug_G)                                       \
       g_printerr(ZMAP_MSG_FORMAT_STRING FORMAT,          \
		  __FILE__,			          \
                  ZMAP_MSG_FUNCTION_MACRO                 \
		  __LINE__,				  \
		  __VA_ARGS__) ;                          \
}G_STMT_END

#endif /*HOME*/


#endif /* ZMAP_UTILS_DEBUG_H */
