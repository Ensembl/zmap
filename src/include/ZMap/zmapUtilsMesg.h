/*  File: zmapUtilsMesg.h
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Macros/functions for outputting various types of message
 *              in production code.
 *              
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_MESG_H
#define ZMAP_UTILS_MESG_H

#include <glib.h>



typedef enum {ZMAP_MSG_INFORMATION, ZMAP_MSG_WARNING, ZMAP_MSG_CRITICAL, ZMAP_MSG_EXIT, ZMAP_MSG_CRASH} ZMapMsgType ;


/* Calling the message routine directly is deprecated, please use the macros below. */
void zMapShowMsg(ZMapMsgType msg_type, char *format, ...) ;



/* Informational messages. */
#define zMapMessage(FORMAT, ...)                           \
G_STMT_START{                                              \
  zMapShowMsg(ZMAP_MSG_INFORMATION, FORMAT, __VA_ARGS__) ; \
}G_STMT_END


/* Warning messages, use for errors that are _not_ coding problems but are also not serious,
 *  e.g. no file name given for export of data to a file. */
#define zMapWarning(FORMAT, ...)                           \
G_STMT_START{                                              \
  zMapShowMsg(ZMAP_MSG_WARNING, FORMAT, __VA_ARGS__) ;     \
}G_STMT_END


/* Critical messages, use for serious errors that are _not_ coding problems, e.g. a bad configuration file. */
#define zMapCritical(FORMAT, ...)                           \
G_STMT_START{                                              \
  zMapShowMsg(ZMAP_MSG_CRITICAL, FORMAT, __VA_ARGS__) ;     \
}G_STMT_END


/* Unrecoverable errors not caused by code, e.g. wrong command line args etc. */
#define zMapExitMsg(FORMAT, ...)                     \
G_STMT_START{                                     \
  zMapShowMsg(ZMAP_MSG_EXIT,                      \
	      ZMAP_MSG_FORMAT_STRING FORMAT,      \
	      ZMAP_MSG_FUNCTION_MACRO,		  \
	      __VA_ARGS__) ;                      \
}G_STMT_END


/* Unrecoverable errors caused by some problem in the code, likely a coding error. */
#define zMapCrash(FORMAT, ...)                    \
G_STMT_START{                                     \
  zMapShowMsg(ZMAP_MSG_CRASH,                     \
	      ZMAP_MSG_FORMAT_STRING FORMAT,      \
	      ZMAP_MSG_FUNCTION_MACRO,		  \
	      __VA_ARGS__) ;                      \
}G_STMT_END



#endif /* ZMAP_UTILS_MESG_H */
