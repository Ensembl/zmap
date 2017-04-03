/*  File: zmapUtilsMesg.h
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
 * Description: Macros/functions for outputting various types of message
 *              in production code.
 *              
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_MESG_H
#define ZMAP_UTILS_MESG_H

#include <glib.h>


#include <ZMap/zmapUtilsPrivate.hpp>



typedef enum {ZMAP_MSG_INFORMATION, ZMAP_MSG_WARNING, ZMAP_MSG_CRITICAL, ZMAP_MSG_EXIT, ZMAP_MSG_CRASH} ZMapMsgType ;


/* Calling the message routine directly is deprecated, please use the macros below. */
void zMapShowMsg(ZMapMsgType msg_type, const char *format, ...) ;



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
#define zMapExitMsg(FORMAT, ...)                  \
  G_STMT_START{                                   \
    zMapShowMsg(ZMAP_MSG_EXIT,                         \
                ZMAP_MSG_CODE_FORMAT_STRING FORMAT,       \
                ZMAP_MSG_CODE_FORMAT_ARGS,		  \
                __VA_ARGS__) ;                            \
}G_STMT_END


/* Unrecoverable errors caused by some problem in the code, likely a coding error. */
#define zMapCrash(FORMAT, ...)                    \
G_STMT_START{                                     \
  zMapShowMsg(ZMAP_MSG_CRASH,                     \
              ZMAP_MSG_CODE_FORMAT_STRING FORMAT,      \
              ZMAP_MSG_CODE_FORMAT_ARGS,		  \
	      __VA_ARGS__) ;                      \
}G_STMT_END



#endif /* ZMAP_UTILS_MESG_H */
