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
 * Last edited: Nov  7 15:37 2006 (edgrif)
 * Created: Mon Mar 29 16:51:28 2004 (edgrif)
 * CVS info:   $Id: zmapUtilsLog.h,v 1.3 2006-11-07 17:00:06 edgrif Exp $
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


/* Use these macros like this:
 * 
 *    zMapLogXXXX("%s is about to %s", str, str) ;
 * 
 * Don't use the macros defined in glib.h such as g_message(), these rely
 * on G_LOG_DOMAIN having been defined as something we want, e.g. "ZMap"
 * otherwise our logging callback routines will not work.
 * 
 * */
#define ZMAPLOG_DOMAIN "ZMap"

#define zMapLogMessage(FORMAT, ...)                       \
	 g_log(ZMAPLOG_DOMAIN,				  \
	       G_LOG_LEVEL_MESSAGE,			  \
	       ZMAP_MSG_FORMAT_STRING FORMAT,             \
	       ZMAP_MSG_FUNCTION_MACRO,			  \
	       __VA_ARGS__)

#define zMapLogWarning(FORMAT, ...)                       \
	 g_log(ZMAPLOG_DOMAIN,				  \
	       G_LOG_LEVEL_WARNING,			  \
	       ZMAP_MSG_FORMAT_STRING FORMAT,             \
	       ZMAP_MSG_FUNCTION_MACRO,			  \
	       __VA_ARGS__)

#define zMapLogCritical(FORMAT, ...)                      \
	 g_log(ZMAPLOG_DOMAIN,				  \
	       G_LOG_LEVEL_CRITICAL,			  \
	       ZMAP_MSG_FORMAT_STRING FORMAT,             \
	       ZMAP_MSG_FUNCTION_MACRO,			  \
	       __VA_ARGS__)
     
#define zMapLogFatal(FORMAT, ...)                         \
	 g_log(ZMAPLOG_DOMAIN,				  \
	       G_LOG_LEVEL_ERROR,			  \
	       ZMAP_MSG_FORMAT_STRING FORMAT,             \
	       ZMAP_MSG_FUNCTION_MACRO,			  \
	       __VA_ARGS__)


/* Use this macro like this:
 * 
 *      zMapLogFatalSysErr(errno, "System call %s failed !", [args]) ;
 * 
 */
#define zMapLogFatalSysErr(ERRNO, FORMAT, ...)            \
	 g_log(ZMAPLOG_DOMAIN,				  \
	       G_LOG_LEVEL_ERROR,			  \
	       ZMAP_MSG_FORMAT_STRING FORMAT " (errno = \"%s\")",  \
	       ZMAP_MSG_FUNCTION_MACRO,				   \
	       __VA_ARGS__,					   \
	       g_strerror(ERRNO))


#endif /* ZMAP_UTILS_LOG_H */
