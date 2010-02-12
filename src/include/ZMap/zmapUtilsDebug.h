/*  File: zmapUtilsDebug.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Contains macros, functions etc. useful for testing/debugging.
 *              
 * HISTORY:
 * Last edited: Feb 12 13:11 2010 (edgrif)
 * Created: Mon Mar 29 16:51:28 2004 (edgrif)
 * CVS info:   $Id: zmapUtilsDebug.h,v 1.9 2010-02-12 13:53:28 edgrif Exp $
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


/* Takes the standard printf like args:   ZMAP_DEBUG("format string", lots, of, args) ; */
#define zMapDebug(FORMAT, ...)                            \
G_STMT_START{                                             \
  if (zmap_debug_G)                                       \
       g_printerr(ZMAP_MSG_FORMAT_STRING FORMAT,          \
                  ZMAP_MSG_FUNCTION_MACRO,		  \
		  __VA_ARGS__) ;                          \
}G_STMT_END



/* Define debug messages more easily. */
#define zMapDebugPrint(BOOLEAN_VAR, FORMAT, ...)       \
  if ((BOOLEAN_VAR))                                   \
    printf("%s: " #FORMAT "\n", __PRETTY_FUNCTION__, __VA_ARGS__)


/* Timer functions, just simplifies printing etc a bit and provides a global timer if required.
 * Just comment out #define ZMAP_DISABLE_TIMER to turn it all on.
 */
#define ZMAP_DISABLE_TIMER


#ifdef ZMAP_DISABLE_TIMER


#define zMapStartTimer(TIMER_PTR) (void)0
#define zMapPrintTimer(TIMER, TEXT) (void)0
#define zMapResetTimer(TIMER) (void)0

#else


/* Define reference to global timer. */
#define ZMAP_GLOBAL_TIMER zmap_global_timer_G
extern GTimer *ZMAP_GLOBAL_TIMER ;

/* A bit clumsy but couldn't see a neat way to allow just putting NULL for the timer to get
 * the global one.
 * Do this for the global one: zMapStartTimer(ZMAP_GLOBAL_TIMER) ;
 * and this for your one:      zMapStartTimer(your_timer_ptr) ;
 *  */
#define zMapStartTimer(TIMER_PTR)                                                       \
(TIMER_PTR) = g_timer_new()

/* Takes an optional Gtimer* and an optional char* (you must supply the args but either can be NULL */
#define zMapPrintTimer(TIMER, TEXT)	                              \
  printf(ZMAP_MSG_FORMAT_STRING " %s   - elapsed time: %g\n",         \
  ZMAP_MSG_FUNCTION_MACRO,                                            \
  ((TEXT) ? (TEXT) : ""),             				      \
  g_timer_elapsed(((TIMER) ? (TIMER) : ZMAP_GLOBAL_TIMER), NULL)) ;

/* Takes an optional Gtimer* (you must supply the arg but it can be NULL */
#define zMapResetTimer(TIMER)	                              \
  g_timer_reset(((TIMER) ? (TIMER) : ZMAP_GLOBAL_TIMER))


#endif /* ZMAP_DISABLE_TIMER */






#endif /* ZMAP_UTILS_DEBUG_H */
