/*  File: zmapUtilsDebug.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Contains macros, functions etc. useful for testing/debugging.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_DEBUG_H
#define ZMAP_UTILS_DEBUG_H

#include <stdlib.h>
#include <stdio.h>

#include <glib.h>


/* Define ZMAP_ASSERT_DISABLE before compiling to disable all asserts. */
#ifdef ZMAP_ASSERT_DISABLE
#define G_DISABLE_ASSERT
#endif




/* I'd like to do this but we aren't quite ready.............. */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#ifdef ZMAP_ASSERT_DISABLE

#define zMapAssert(expr)                do { (void) 0; } while (0)

#else /* !ZMAP_ASSERT_DISABLE */

#define zMapAssert(expr) \
        do \
	  { \
	    if G_LIKELY (expr) \
	      ;								\
	    else							\
	      { \
		char *file ;			\
									\
		file = g_strdup_printf("%s - %s", zMapGetAppVersionString(), __FILE__) ; \
		g_assertion_message_expr (G_LOG_DOMAIN, file, __LINE__, G_STRFUNC, \
					  #expr); \
	      } \
	  } while (0)

#endif /* !ZMAP_ASSERT_DISABLE */



#ifdef ZMAP_ASSERT_DISABLE

#define zMapAssertNotReached()          do { (void) 0; } while (0)

#else /* !ZMAP_ASSERT_DISABLE */

#define zMapAssertNotReached() \
  do \
    { \
      char *file ; \
                   \
      file = g_strdup_printf("%s - %s", zMapGetAppVersionString(), __FILE__) ; \
      g_assertion_message (G_LOG_DOMAIN, file, __LINE__, G_STRFUNC, NULL ) ; \
    } while (0) 

#endif /* !ZMAP_ASSERT_DISABLE */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* SI I'VE REVERTED TO THIS... */

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


/* These macros should be used to check arguments instead of assert.
 * The macros that cause the function to return should ONLY be used at
 * the START of a function because we want to avoid multiple returns
 * from within the body of the function. */

/* Check a critical condition. Returns and issues a warning if the
 * condition fails. */
#define zMapReturnIfFail(EXPR)                                           \
G_STMT_START{                                                            \
  if G_LIKELY (EXPR)                                                     \
    {                                                                    \
    }                                                                    \
  else                                                                   \
    {                                                                    \
      g_printerr(ZMAP_MSG_FORMAT_STRING " Assertion '%s' failed\n",      \
                      ZMAP_MSG_FUNCTION_MACRO,                           \
                      #EXPR) ;                                           \
      zMapLogCritical(ZMAP_MSG_FORMAT_STRING " Assertion '%s' failed\n", \
                      ZMAP_MSG_FUNCTION_MACRO,                           \
                      #EXPR) ;                                           \
      zMapLogStack();                                                    \
      return;                                                            \
    }                                                                    \
}G_STMT_END

/* Check a condition and return if it fails. This is for non-error
 * scenarios and does not emit any warnings. */
#define zMapReturnIfFailSafe(EXPR)                                       \
  G_STMT_START{                                                          \
    if G_LIKELY (EXPR)                                                   \
      {                                                                  \
      }                                                                  \
    else                                                                 \
      {                                                                  \
        return;                                                          \
      }                                                                  \
}G_STMT_END

/* Check a critical condition. Returns the given value and issues a
 * warning if the condition fails. */
#define zMapReturnValIfFail(EXPR, VALUE)                                 \
G_STMT_START{                                                            \
  if G_LIKELY (EXPR)                                                     \
    {                                                                    \
    }                                                                    \
  else                                                                   \
    {                                                                    \
      g_printerr(ZMAP_MSG_FORMAT_STRING " Assertion '%s' failed\n",      \
                      ZMAP_MSG_FUNCTION_MACRO,                           \
                      #EXPR) ;                                           \
      zMapLogCritical(ZMAP_MSG_FORMAT_STRING " Assertion '%s' failed\n", \
                      ZMAP_MSG_FUNCTION_MACRO,                           \
                      #EXPR) ;                                           \
      zMapLogStack();                                                    \
      return VALUE;                                                      \
    }                                                                    \
}G_STMT_END

/* Check a condition and return the given value if it fails. This
 * is for non-error scenarios and does not emit any warnings. */
#define zMapReturnValIfFailSafe(EXPR, VALUE)                             \
  G_STMT_START{                                                          \
    if G_LIKELY (EXPR)                                                   \
      {                                                                  \
      }                                                                  \
    else                                                                 \
      {                                                                  \
        return VALUE;                                                    \
      }                                                                  \
}G_STMT_END

/* Issue a warning if the given condition fails */
#define zMapWarnIfFail(EXPR)                                                   \
G_STMT_START{                                                                  \
  if G_LIKELY (EXPR)                                                           \
    {                                                                          \
    }                                                                          \
  else                                                                         \
    {                                                                          \
      g_printerr(ZMAP_MSG_FORMAT_STRING " Runtime check failed (%s)\n",        \
                      ZMAP_MSG_FUNCTION_MACRO,                                 \
                      #EXPR) ;                                                 \
      zMapLogCritical(ZMAP_MSG_FORMAT_STRING " Runtime check failed (%s)\n",   \
                      ZMAP_MSG_FUNCTION_MACRO,                                 \
                      #EXPR) ;                                                 \
      zMapLogStack();                                                          \
    }                                                                          \
}G_STMT_END

/* Issue a warning if this code is reached */
#define zMapWarnIfReached()                                               \
G_STMT_START{                                                             \
  g_printerr(ZMAP_MSG_FORMAT_STRING " Code should not be reached\n",      \
                  ZMAP_MSG_FUNCTION_MACRO) ;                              \
  zMapLogCritical(ZMAP_MSG_FORMAT_STRING " Code should not be reached\n", \
                  ZMAP_MSG_FUNCTION_MACRO) ;                              \
  zMapLogStack();                                                         \
}G_STMT_END



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
      zMapUtilsDebugPrintf(stderr, "%s: " FORMAT "\n", __PRETTY_FUNCTION__, __VA_ARGS__) ; \
  } G_STMT_END
#else /* __GNUC__ */
#define zMapDebugPrint(BOOLEAN_VAR, FORMAT, ...)		      \
  G_STMT_START                                                        \
  {								      \
    if ((BOOLEAN_VAR))						      \
      zMapUtilsDebugPrintf(stderr, "%s: " FORMAT "\n", NULL, __VA_ARGS__) ;  \
  } G_STMT_END
#endif /* __GNUC__ */


/* Define debug messages more easily. */
#ifdef __GNUC__
#define zMapDebugPrintf(FORMAT, ...)                      \
  zMapUtilsDebugPrintf(stderr, "%s: " FORMAT "\n", __PRETTY_FUNCTION__, __VA_ARGS__) 
#else /* __GNUC__ */
#define zMapDebugPrintf(FORMAT, ...)		      \
  zMapUtilsDebugPrintf(stderr, "%s: " FORMAT "\n", NULL, __VA_ARGS__) 
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
      zMapLogMessage("Timer %s\t%.3f\t%s\t%s", \
            FUNC,\
            g_timer_elapsed(ZMAP_GLOBAL_TIMER, NULL),\
            ID,OPT); \
   }

#define zMapElapsedSeconds	g_timer_elapsed(ZMAP_GLOBAL_TIMER, NULL)

// can use start then stop, but stop on its own can be interpreted as previous event as start
#define zMapStartTimer(TEXT,OPT)    zMapPrintTime("Start",TEXT,OPT)
#define zMapStopTimer(TEXT,OPT)     zMapPrintTime("Stop",TEXT,OPT)


// interface to legacy calls: removed to avoid clutter, redef the macro if you want them back
// grep for PrintTimer to see them all (about 30)
/* Takes an optional Gtimer* and an optional char* (you must supply the args but either can be NULL */
#define zMapPrintTimer(TIMER, TEXT) (void) 0    // zmapPrintTime(TEXT ? TEXT : "?","")


/* time in seconds as a double */
#define zMap_elapsed()	g_timer_elapsed(ZMAP_GLOBAL_TIMER, NULL)

/* see zmapLogging.c for these: do not change without changing the code */
#define TIMER_NONE	0
#define TIMER_EXPOSE	1
#define TIMER_UPDATE	2
#define TIMER_DRAW	3
#define TIMER_DRAW_CONTEXT	4
#define TIMER_REVCOMP	5
#define TIMER_ZOOM	6
#define TIMER_BUMP	7
#define TIMER_SETVIS	8
#define TIMER_LOAD	9

#define N_TIMES		10

#define TIMER_CLEAR	0
#define TIMER_START	1
#define TIMER_STOP	2
#define TIMER_ELAPSED	3

void zMapLogTime(int what, int how, long data, char *string);

#endif /* ZMAP_DISABLE_TIMER */



void zMapUtilsDebugPrintf(FILE *stream, char *format, ...) ;


#endif /* ZMAP_UTILS_DEBUG_H */
