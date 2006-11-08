/*  File: zmapUtilsMesg.h
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
 * Description: Macros/functions for outputting various types of message
 *              in production code.
 *              
 * HISTORY:
 * Last edited: Nov  7 15:38 2006 (edgrif)
 * Created: Mon Mar 29 18:23:48 2004 (edgrif)
 * CVS info:   $Id: zmapUtilsMesg.h,v 1.4 2006-11-08 09:23:29 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_MESG_H
#define ZMAP_UTILS_MESG_H

#include <glib.h>


/* Can call the message routine directly but better to use the macros below. */
typedef enum {ZMAP_MSG_INFORMATION, ZMAP_MSG_WARNING, ZMAP_MSG_EXIT, ZMAP_MSG_CRASH} ZMapMsgType ;


void zMapShowMsg(ZMapMsgType msg_type, char *format, ...) ;



/* Informational messages. */
#define zMapMessage(FORMAT, ...)                           \
G_STMT_START{                                              \
  zMapShowMsg(ZMAP_MSG_INFORMATION, FORMAT, __VA_ARGS__) ; \
}G_STMT_END


/* Warning messages. */
#define zMapWarning(FORMAT, ...)                           \
G_STMT_START{                                              \
  zMapShowMsg(ZMAP_MSG_WARNING, FORMAT, __VA_ARGS__) ;     \
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
