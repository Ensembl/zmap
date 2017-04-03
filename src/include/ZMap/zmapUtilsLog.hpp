/*  File: zmapUtilsLog.h
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
 * Description: Contains macros, functions etc. for logging.
 *              
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_LOG_H
#define ZMAP_UTILS_LOG_H

#include <stdlib.h>

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* We would need this for the glog convenience macros to work.... but would have to fiddle
 * with headers to get all this to work..... */

#define ZMAPLOG_DOMAIN "ZMap"
#define G_LOG_DOMAIN   ZMAPLOG_DOMAIN

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#include <glib.h>



#define ZMAPLOG_DOMAIN "ZMap"


/* ZMap log records are tab separated tuples of information of the form:
 * 
 * ZMap_XXXX:info:info:etc
 * 
 * The following defines enumerate the XXXX
 * 
 *  */
#define ZMAPLOG_PROCESS_TUPLE "ZMAP_PROCESS"
#define ZMAPLOG_CODE_TUPLE    "ZMAP_CODE"
#define ZMAPLOG_TIME_TUPLE    "ZMAP_TIME"
#define ZMAPLOG_MESSAGE_TUPLE "ZMAP_MESSAGE"


/* Some compilers give more information than others so set up compiler dependant defines. */
#ifdef __GNUC__ 

#define ZMAP_LOG_CODE_PARAMS __FILE__, __PRETTY_FUNCTION__, __LINE__

#else /* __GNUC__ */

#define ZMAP_LOG_CODE_PARAMS __FILE__, NULL, __LINE__

#endif /* __GNUC__ */



/* The base logging routine, you should use the below macros which automatically
 * include the stuff you need instead of this routine. */
void zMapLogMsg(const char *domain, GLogLevelFlags log_level,
                const char *file, const char *function, int line,
                const char *format, ...) ;


/* Use these macros like this:
 * 
 *    zMapLogXXXX("%s is about to %s", str, str) ;
 * 
 * Don't use the macros defined in glib.h such as g_message(), these rely
 * on G_LOG_DOMAIN having been defined as something we want, e.g. "ZMap"
 * otherwise our logging callback routines will not work.
 * 
 * */
#define zMapLogMessage(FORMAT, ...)             \
  zMapLogMsg(ZMAPLOG_DOMAIN,                    \
             G_LOG_LEVEL_MESSAGE,               \
             ZMAP_LOG_CODE_PARAMS,              \
             FORMAT,                            \
             __VA_ARGS__)

#define zMapLogWarning(FORMAT, ...)             \
  zMapLogMsg(ZMAPLOG_DOMAIN,                    \
             G_LOG_LEVEL_WARNING,               \
             ZMAP_LOG_CODE_PARAMS,              \
             FORMAT,                            \
             __VA_ARGS__)

#define zMapLogCritical(FORMAT, ...)            \
  zMapLogMsg(ZMAPLOG_DOMAIN,                    \
             G_LOG_LEVEL_CRITICAL,              \
             ZMAP_LOG_CODE_PARAMS,              \
             FORMAT,                            \
             __VA_ARGS__)
     
#define zMapLogFatal(FORMAT, ...)               \
  zMapLogMsg(ZMAPLOG_DOMAIN,                    \
             G_LOG_LEVEL_ERROR,                 \
             ZMAP_LOG_CODE_PARAMS,              \
             FORMAT,                            \
             __VA_ARGS__)



/* Use this macro like this:
 * 
 *      zMapLogCriticalSysErr(errno, "System call %s failed !", [args]) ;
 * 
 */
#define zMapLogCriticalSysErr(ERRNO, FORMAT, ...)            \
  do {                                                    \
    char *errno_str = (char *)g_strerror(ERRNO) ;         \
                                                          \
    zMapLogMsg(ZMAPLOG_DOMAIN,                            \
               G_LOG_LEVEL_CRITICAL,                      \
               ZMAP_LOG_CODE_PARAMS,                      \
               FORMAT " (errno = \"%s\")",                \
               __VA_ARGS__,                               \
               errno_str) ;                               \
  } while (0)

/* Use this macro like this:
 * 
 *      zMapLogFatalSysErr(errno, "System call %s failed !", [args]) ;
 * 
 */
#define zMapLogFatalSysErr(ERRNO, FORMAT, ...)            \
  do {                                                    \
    char *errno_str = (char *)g_strerror(ERRNO) ;         \
                                                          \
    zMapLogMsg(ZMAPLOG_DOMAIN,                            \
               G_LOG_LEVEL_ERROR,                         \
               ZMAP_LOG_CODE_PARAMS,                      \
               FORMAT " (errno = \"%s\")",                \
               __VA_ARGS__,                               \
               errno_str) ;                               \
  } while (0)

/* Use this macro like this:
 * 
 * switch(some_var)
 *   {
 *    case:
 *       etc.
 * 
 *    default:
 *      zMapLogFatalLogicErr("switch(), unknown value: %d", some_var) ;
 *   }
 */
#define zMapLogFatalLogicErr(FORMAT, ...)              \
  zMapLogMsg(ZMAPLOG_DOMAIN,                           \
             G_LOG_LEVEL_ERROR,                        \
             ZMAP_LOG_CODE_PARAMS,                           \
             "Panic - Internal Logic Error: " FORMAT " !!",  \
             __VA_ARGS__)


/* Use this macro like this:
 * 
 * zMapLogReturnIfFail(widget != NULL) ;
 * 
 * Logs the error and returns from the function if widget is NULL.
 * 
 */
#define zMapLogReturnIfFail(expr)                               \
  G_STMT_START{                                                 \
     if (expr) { } else                                         \
       {                                                        \
         zMapLogMsg(ZMAPLOG_DOMAIN,                             \
                    G_LOG_LEVEL_CRITICAL,                       \
                    ZMAP_LOG_CODE_PARAMS,                       \
                    "Expr failed: \"%s\"" ,                     \
                    #expr) ;                                    \
         return;                                                \
       };                               }G_STMT_END

#define zMapLogReturnValIfFail(expr, val)       G_STMT_START{   \
     if (expr) { } else                                         \
       {                                                        \
         zMapLogMsg(ZMAPLOG_DOMAIN,                             \
                    G_LOG_LEVEL_CRITICAL,                       \
                    ZMAP_LOG_CODE_PARAMS,                       \
                    "Expr failed: \"%s\"" ,                     \
                    #expr) ;                                    \
         return (val);                                          \
       };                               }G_STMT_END




#endif /* ZMAP_UTILS_LOG_H */
