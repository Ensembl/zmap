/*  File: zmapUtilsDebug.h
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Contains macros, functions etc. useful for testing/debugging.
 *
 * HISTORY:
 * Last edited: Mar 12 12:58 2010 (edgrif)
 * Created: Mon Mar 29 16:51:28 2004 (edgrif)
 * CVS info:   $Id: zmapUtilsDebug.h,v 1.14 2011-03-14 11:35:17 mh17 Exp $
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


extern gboolean zmap_development_G ;      // patch in/out half working code


/* Takes the standard printf like args:   ZMAP_DEBUG("format string", lots, of, args) ; */
#define zMapDebug(FORMAT, ...)                            \
G_STMT_START{                                             \
  if (zmap_debug_G)                                       \
       g_printerr(ZMAP_MSG_FORMAT_STRING FORMAT,          \
                  ZMAP_MSG_FUNCTION_MACRO,		  \
		  __VA_ARGS__) ;                          \
}G_STMT_END



/* Define debug messages more easily. */
#ifdef __GNUC__
#define zMapDebugPrint(BOOLEAN_VAR, FORMAT, ...)                      \
  G_STMT_START                                                        \
  {								      \
    if ((BOOLEAN_VAR))						      \
      printf("%s: " #FORMAT "\n", __PRETTY_FUNCTION__, __VA_ARGS__) ; \
  } G_STMT_END
#else /* __GNUC__ */
#define zMapDebugPrint(BOOLEAN_VAR, FORMAT, ...)                      \
  G_STMT_START                                                        \
  {								      \
    if ((BOOLEAN_VAR))						      \
      printf("%s: " #FORMAT "\n", NULL, __VA_ARGS__) ;                \
  } G_STMT_END
#endif /* __GNUC__ */

/* Timer functions, just simplifies printing etc a bit and provides a global timer if required.
 * Just comment out #define ZMAP_DISABLE_TIMER to turn it all on.
 */
// make this into a build option
//#define ZMAP_DISABLE_TIMER


#ifdef ZMAP_DISABLE_TIMER

// the void zero bit is so we can have a semicolon after the call.
#define zMapStartTimer(TIMER_PTR) (void)0
#define zMapPrintTimer(TIMER, TEXT) (void)0
#define zMapResetTimer(TIMER) (void)0

#else


/* Define reference to global timer from view creation */
#define ZMAP_GLOBAL_TIMER zmap_global_timer_G
extern GTimer *ZMAP_GLOBAL_TIMER ;
extern gboolean zmap_timing_G;

#define zMapInitTimer() \
 if(zmap_timing_G) \
 { GTimeVal gtv; \
   GDate gd;  \
   if(ZMAP_GLOBAL_TIMER) \
      g_timer_reset(ZMAP_GLOBAL_TIMER); \
   else  \
       ZMAP_GLOBAL_TIMER = g_timer_new();\
   g_get_current_time(&gtv);\
   g_date_set_time_val(&gd,&gtv);\
   printf("# %s\t%02d/%02d/%4d\n",zMapConfigDirGetFile(),\
      g_date_get_day (&gd), g_date_get_month(&gd),g_date_get_year(&gd));\
   zMapPrintTime("Reset","Timer","");\
 }


// this output format is TAB delimited.
// Func is Start or Stop
// ID may be readable text but may not contain TABS.
// OPT is another (descriptive) text string
// timings in seconds.milliseconds
// output is now configured from [ZMap] - can leave code compiled in
// NULL args should be  ""
#define zMapPrintTime(FUNC, ID, OPT)   \
   { if(ZMAP_GLOBAL_TIMER) \
      printf("%s\t%.3f\t%s\t%s\n", \
            FUNC,\
            g_timer_elapsed(ZMAP_GLOBAL_TIMER, NULL),\
            ID,OPT); \
   }

// can use start then stop, but stop on its own can be interpreted as previous event as start
#define zMapStartTimer(TEXT,OPT)    zMapPrintTime("Start",TEXT,OPT)
#define zMapStopTimer(TEXT,OPT)     zMapPrintTime("Stop",TEXT,OPT)


// interface to legacy calls: removed to avoid clutter, redef the macro if you want them back
// grep for PrintTimer to see them all (about 30)
/* Takes an optional Gtimer* and an optional char* (you must supply the args but either can be NULL */
#define zMapPrintTimer(TIMER, TEXT) (void) 0    // zmapPrintTime(TEXT ? TEXT : "?","")

#endif /* ZMAP_DISABLE_TIMER */






#endif /* ZMAP_UTILS_DEBUG_H */
