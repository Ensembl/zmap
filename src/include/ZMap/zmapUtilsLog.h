/*  File: zmapUtilsLog.h
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
 * Description: Contains macros, functions etc. for logging.
 *              
 * HISTORY:
 * Last edited: Apr  8 17:08 2004 (edgrif)
 * Created: Mon Mar 29 16:51:28 2004 (edgrif)
 * CVS info:   $Id: zmapUtilsLog.h,v 1.1 2004-04-08 16:14:53 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_LOG_H
#define ZMAP_UTILS_LOG_H

#include <stdlib.h>
#include <glib.h>


#ifdef HOME

#define zMapLogFatal(FORMAT, ...)

#define zMapLogFatalSysErr(ERRNO, FORMAT, ...)

#else


/* Add in informational, warning, log level messages.... */


/* Use this one like this:             ZMAPERR("%s blah, blah, %d", str, int) ; */
#define zMapLogMessage(FORMAT, ...)                       \
	 g_log(G_LOG_DOMAIN,				  \	
	       G_LOG_LEVEL_MESSAGE,			  \
	       ZMAP_MSG_FORMAT_STRING FORMAT,             \
		__FILE__,				  \
                  ZMAP_MSG_FUNCTION_MACRO                 \
		__LINE__,				  \
		__VA_ARGS__)


/* Use this one like this:             ZMAPERR("%s blah, blah, %d", str, int) ; */
#define zMapLogWarning(FORMAT, ...)                       \
	 g_log(G_LOG_DOMAIN,				  \
	       G_LOG_LEVEL_WARNING,			  \
	       ZMAP_MSG_FORMAT_STRING FORMAT,             \
		__FILE__,				  \
                  ZMAP_MSG_FUNCTION_MACRO                 \
		__LINE__,				  \
		__VA_ARGS__)


/* Use this one like this:             ZMAPERR("%s blah, blah, %d", str, int) ; */
#define zMapLogCritical(FORMAT, ...)                      \
	 g_log(G_LOG_DOMAIN,				  \
	       G_LOG_LEVEL_CRITICAL,			  \
	       ZMAP_MSG_FORMAT_STRING FORMAT,             \
		__FILE__,				  \
                  ZMAP_MSG_FUNCTION_MACRO                 \
		__LINE__,				  \
		__VA_ARGS__)

/* Note that G_LOG_LEVEL_ERROR messages cause g_log() to abort always. */
#define zMapLogFatal(FORMAT, ...)                         \
	 g_log(G_LOG_DOMAIN,				  \
	       G_LOG_LEVEL_ERROR,			  \
	       ZMAP_MSG_FORMAT_STRING FORMAT,             \
		__FILE__,				  \
                  ZMAP_MSG_FUNCTION_MACRO                 \
		__LINE__,				  \
		__VA_ARGS__)


/* Use this macro like this:
 *      zMapLogFatalSysErr(errno, "System call %s failed !", [args]) ;
 */
#define zMapLogFatalSysErr(ERRNO, FORMAT, ...)            \
	 g_log(G_LOG_DOMAIN,				  \
	       G_LOG_LEVEL_ERROR,			  \
	       ZMAP_MSG_FORMAT_STRING FORMAT " (errno = \"%s\")",  \
		__FILE__,				  \
                  ZMAP_MSG_FUNCTION_MACRO                 \
		__LINE__,				  \
		__VA_ARGS__,                              \
	       g_strerror(ERRNO))


#endif /*HOME*/

#endif /* ZMAP_UTILS_LOG_H */
