/*  File: zmapUtils.h
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
 * Description: Utility functions for ZMap.
 * HISTORY:
 * Last edited: May 13 14:12 2005 (edgrif)
 * Created: Thu Feb 26 10:33:10 2004 (edgrif)
 * CVS info:   $Id: zmapUtils.h,v 1.15 2005-05-18 10:49:54 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_H
#define ZMAP_UTILS_H

#include <glib.h>



/* Some compilers give more information than others so set up compiler dependant defines. */

#ifdef __GNUC__	

#define ZMAP_MSG_FORMAT_STRING  "(%s, %s(), line %d) - "

#define ZMAP_MSG_FUNCTION_MACRO __PRETTY_FUNCTION__,

#else /* __GNUC__ */

#define ZMAP_MSG_FORMAT_STRING  "(%s, line %d) - "

#define ZMAP_MSG_FUNCTION_MACRO        

#endif /* __GNUC__ */



#include <ZMap/zmapUtilsDebug.h>
#include <ZMap/zmapUtilsCheck.h>
#include <ZMap/zmapUtilsMesg.h>
#include <ZMap/zmapUtilsLog.h>
#include <ZMap/zmapReadLine.h>


/*! @addtogroup zmaputils
 * @{
 *  */

/*!
 * Represents a ZMap logging object which will log messages, open/close the log etc. */
typedef struct  _ZMapLogStruct *ZMapLog ;

/*! @} end of zmaputils docs. */


#define ZMAPLOG_STANZA  "logging"			    /* Does this need to be public ? */


ZMapLog zMapLogCreate(char *logname) ;
gboolean zMapLogStart(ZMapLog log) ;
gboolean zMapLogStop(ZMapLog log) ;
void zMapLogDestroy(ZMapLog log) ;

char *zMapGetDir(char *directory_in, gboolean home_relative, gboolean make_dir) ;
char *zMapGetFile(char *directory, char *filename, gboolean make_file) ;
char *zMapGetPath(char *path_in) ;
gboolean zMapFileAccess(char *filepath) ;
gboolean zMapFileEmpty(char *filepath) ;


/* You can use ZMAP_MAKESTRING() to create a string version of a number:
 * 
 *         ZMAP_MAKESTRING(6)  produces "6" 
 * 
 * n.b. the indirection of ZMAP_PUTSTRING() is required because of the
 * way the ANSI preprocessor handles strings */
#define ZMAP_PUTSTRING(x) #x
#define ZMAP_MAKESTRING(x) ZMAP_PUTSTRING(x)


/* routines to return basic version/information about zmap. */
char *zMapGetAppName(void) ;
char *zMapGetAppTitle(void) ;
int zMapGetVersion(void) ;
char *zMapGetVersionString(void) ;
gboolean zMapCompareVersionStings(char *reference_version, char *test_version) ;
char *zMapGetCopyrightString(void) ;

void zMapExit(int exit_code) ;

gboolean zMapUtilsConfigDebug(char *debug_flag, gboolean *value) ;

char *zMapGetTimeString(void) ;

gboolean zMapStr2Int(char *str, int *int_out) ;
gboolean zMapStr2LongInt(char *str, long int *long_int_out) ;
gboolean zMapStr2Double(char *str, double *double_out) ;

#endif /* ZMAP_UTILS_H */
