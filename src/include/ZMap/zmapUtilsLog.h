/*  Last edited: Jul  8 14:23 2011 (edgrif) */
/*  File: zmapUtilsLog.h
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
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
#define ZMAPLOG_MESSAGE_TUPLE "ZMAP_MESSAGE"


/* Some compilers give more information than others so set up compiler dependant defines. */
#ifdef __GNUC__	

#define ZMAP_LOG_CODE_PARAMS __FILE__, __PRETTY_FUNCTION__, __LINE__

#else /* __GNUC__ */

#define ZMAP_LOG_CODE_PARAMS __FILE__, NULL, __LINE__

#endif /* __GNUC__ */



/* The base logging routine, you should use the below macros which automatically
 * include the stuff you need instead of this routine. */
void zMapLogMsg(char *domain, GLogLevelFlags log_level,
		char *file, const char *function, int line,
		char *format, ...) ;


/* Use these macros like this:
 * 
 *    zMapLogXXXX("%s is about to %s", str, str) ;
 * 
 * Don't use the macros defined in glib.h such as g_message(), these rely
 * on G_LOG_DOMAIN having been defined as something we want, e.g. "ZMap"
 * otherwise our logging callback routines will not work.
 * 
 * */
#define zMapLogMessage(FORMAT, ...)		\
  zMapLogMsg(ZMAPLOG_DOMAIN,			\
	     G_LOG_LEVEL_MESSAGE,		\
	     ZMAP_LOG_CODE_PARAMS,	        \
	     FORMAT,	                        \
	     __VA_ARGS__)

#define zMapLogWarning(FORMAT, ...)		\
  zMapLogMsg(ZMAPLOG_DOMAIN,			\
	     G_LOG_LEVEL_WARNING,		\
	     ZMAP_LOG_CODE_PARAMS,	        \
	     FORMAT,                  		\
	     __VA_ARGS__)

#define zMapLogCritical(FORMAT, ...)		\
  zMapLogMsg(ZMAPLOG_DOMAIN,			\
	     G_LOG_LEVEL_CRITICAL,		\
	     ZMAP_LOG_CODE_PARAMS,	        \
	     FORMAT,             		\
	     __VA_ARGS__)
     
#define zMapLogFatal(FORMAT, ...)		\
  zMapLogMsg(ZMAPLOG_DOMAIN,			\
	     G_LOG_LEVEL_ERROR,			\
	     ZMAP_LOG_CODE_PARAMS,	        \
	     FORMAT,                            \
	     __VA_ARGS__)



/* Use this macro like this:
 * 
 *      zMapLogFatalSysErr(errno, "System call %s failed !", [args]) ;
 * 
 */
#define zMapLogFatalSysErr(ERRNO, FORMAT, ...)            \
  do {							  \
    char *errno_str = (char *)g_strerror(ERRNO) ;	  \
                                                          \
    zMapLogMsg(ZMAPLOG_DOMAIN,				  \
	       G_LOG_LEVEL_ERROR,			  \
	       ZMAP_LOG_CODE_PARAMS,		          \
	       FORMAT " (errno = \"%s\")",		  \
	       __VA_ARGS__,			          \
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
  zMapLogMsg(ZMAPLOG_DOMAIN,			       \
	     G_LOG_LEVEL_ERROR,			       \
	     ZMAP_LOG_CODE_PARAMS,		       \
	     "Panic - Internal Logic Error: " FORMAT " !!",  \
	     __VA_ARGS__)


/* make logging from totalview evaluations a lot easier... */
void zMapLogQuark(GQuark quark);




/* Use this macro like this:
 * 
 * zMapLogReturnIfFail(widget != NULL) ;
 * 
 * Logs the error and returns from the function if widget is NULL.
 * 
 */
#define zMapLogReturnIfFail(expr)                                     \
  G_STMT_START{							\
     if (expr) { } else						\
       {							\
  zMapLogMsg(ZMAPLOG_DOMAIN,			\
	     G_LOG_LEVEL_CRITICAL,		\
	     ZMAP_LOG_CODE_PARAMS,	        \
	     "Expr failed: \"%s\"" ,     \
	     #expr) ;					\
	 return;						\
       };				}G_STMT_END

#define zMapLogReturnValIfFail(expr, val)	G_STMT_START{		\
     if (expr) { } else						\
       {							\
  zMapLogMsg(ZMAPLOG_DOMAIN,			\
	     G_LOG_LEVEL_CRITICAL,		\
	     ZMAP_LOG_CODE_PARAMS,	        \
	     "Expr failed: \"%s\"" ,     \
	     #expr) ;					\
	 return (val);						\
       };				}G_STMT_END




#endif /* ZMAP_UTILS_LOG_H */
