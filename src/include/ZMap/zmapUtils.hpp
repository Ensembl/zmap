/*  File: zmapUtils.h
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
 * Description: Utility functions for ZMap.
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_H
#define ZMAP_UTILS_H

#include <glib.h>

#include <ZMap/zmapEnum.hpp>



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* The following defines deliberately come before the subsequent include files because
 * those files use these definitions. */

/* Some compilers give more information than others so set up compiler dependant defines. */

#ifdef __GNUC__

#define ZMAP_MSG_FORMAT_STRING  "%s:%s:%d"

#define ZMAP_MSG_FUNCTION_MACRO zMapGetBasename(__FILE__), __PRETTY_FUNCTION__, __LINE__

#else /* __GNUC__ */

#define ZMAP_MSG_FORMAT_STRING  "%s:%d"

#define ZMAP_MSG_FUNCTION_MACRO zMapGetBasename(__FILE__), __LINE__

#endif /* __GNUC__ */


#include <ZMap/zmapUtilsPrivate.hpp>

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#include <ZMap/zmapUtilsDebug.hpp>
#include <ZMap/zmapUtilsCheck.hpp>
#include <ZMap/zmapUtilsMesg.hpp>
#include <ZMap/zmapUtilsLog.hpp>
#include <ZMap/zmapReadLine.hpp>



/* You can use ZMAP_MAKESTRING() to create a string version of a number:
 *
 *         ZMAP_MAKESTRING(6)  produces "6"
 *
 * n.b. the indirection of ZMAP_PUTSTRING() is required because of the
 * way the ANSI preprocessor handles strings */
#define ZMAP_PUTSTRING(x) #x
#define ZMAP_MAKESTRING(x) ZMAP_PUTSTRING(x)


/* Make a single version number out of the version, release and update numbers. */
#define ZMAP_MAKE_VERSION_NUMBER(VERSION, RELEASE, UPDATE) \
((VERSION * 10000) + (RELEASE * 100) + UPDATE)

/* Make a single version string out of the version, release and update numbers. */
#define ZMAP_MAKE_VERSION_STRING(VERSION, RELEASE, UPDATE) \
ZMAP_MAKESTRING(VERSION) "." ZMAP_MAKESTRING(RELEASE) "." ZMAP_MAKESTRING(UPDATE)

/* Make a title string containing the title of the application/library and
 *    and the version, release and update numbers. */
#define ZMAP_MAKE_TITLE_STRING(TITLE, VERSION, RELEASE, UPDATE) \
TITLE " - " ZMAP_MAKE_VERSION_STRING(VERSION, RELEASE, UPDATE)




/*!
 * Represents a ZMap logging object which will log messages, open/close the log etc. */
typedef struct  _ZMapLogStruct *ZMapLog ;



/*!
 * ZMagMagic: the type that all magic symbols are declared of.
 * They become magic (i.e. unique) by using the pointer
 * to that unique symbol. (Declaring the string as static guarantees that it is
 * unique as it can only be referenced in the file where it is declared.)
 *
 * Type-magics should be defined like this:
 *
 * ZMAP_DEFINE_NEW_MAGIC("MYTYPE") ;
 *
 * The address of the string is then used as the unique identifier and the string
 * can be used during debugging.
 *
 * If you use the magic in heap allocated structs then you _must_ reset the magic
 * on deallocating the struct to invalidate the memory and hence catch when callers
 * try to erroneously continue to reference it:
 *
 * ZMAP_MAGIC_RESET(struct->magic) ;
 *
 */
typedef const char * ZMapMagic ;

#define ZMAP_MAGIC_NEW(magic_var_name, type_name) static ZMapMagic magic_var_name = ZMAP_MAKESTRING((type_name))  " in file " __FILE__
#define ZMAP_MAGIC_IS_VALID(magic_var_name, magic_ptr) ((magic_var_name) == (magic_ptr))
#define ZMAP_MAGIC_ASSERT(magic_var_name, magic_ptr) zMapAssert(ZMAP_MAGIC_IS_VALID((magic_var_name), (magic_ptr)))
#define ZMAP_MAGIC_RESET(magic_ptr) (magic_ptr) = NULL


#define ZMAP_PTR_ZERO_FREE(ptr, struct_type) \
{                                            \
  memset((ptr), 0, sizeof(struct_type));     \
  g_free((ptr));                             \
  (ptr) = NULL;                              \
}


#define ZMAP_SWAP_TYPE(TYPE, A, B) { \
TYPE _c = (A);                       \
    (A) = (B);                       \
    (B) = (_c);                      \
}



/* Types of date formats that can be returned by zMapGetTimeString(). */
typedef enum
  {
    ZMAPTIME_STANDARD,					    /* "Thu Sep 30 10:05:27 2004" */
    ZMAPTIME_YMD,					    /* "1997-11-08" */
    ZMAPTIME_LOG,					    /* "2013/06/11 13:18:27.121129" */
    ZMAPTIME_SEC_MICROSEC,                                  /* "sssss.mmmmmm" (secs and micro secs)  */
    ZMAPTIME_USERFORMAT					    /* Users provides format string. */
  } ZMapTimeFormat ;

#define ZMAPTIME_SEC_MICROSEC_SEPARATOR ","



/* Gives process termination type. */
#define ZMAP_PROCTERM_LIST(_)                                           \
  _(ZMAP_PROCTERM_OK,        , "ok"      , "process exited normally."      , "") \
    _(ZMAP_PROCTERM_ERROR,   , "error"   , "process returned error code."  , "") \
    _(ZMAP_PROCTERM_SIGNAL,  , "signal"  , "proces terminated by signal.", "") \
    _(ZMAP_PROCTERM_STOPPED, , "stopped" , "process stopped by signal."    , "")

ZMAP_DEFINE_ENUM(ZMapProcessTerminationType, ZMAP_PROCTERM_LIST) ;



typedef struct
{
  guint    source_id;
  gpointer user_data;
}ZMapUtilsChildWatchDataStruct, *ZMapUtilsChildWatchData;



#define ZMAPLOG_STANZA  "logging"			    /* Does this need to be public ? */


gboolean zMapLogCreate(char *logname) ;
gboolean zMapLogConfigure(gboolean logging, gboolean log_to_file,
			  gboolean show_process, gboolean show_code, gboolean show_time,
			  gboolean catch_glib, gboolean echo_glib,
                          char *logfile_path, GError **error) ;
void zMapWriteStartMsg(void) ;
void zMapWriteStopMsg(void) ;
gboolean zMapLogStart(GError **error) ;
const char *zMapLogGetLogFilePath(void) ;
int zMapLogFileSize(void) ;
gboolean zMapLogStop(void) ;
void zMapLogStack(void);
void zMapLogDestroy(void) ;

gboolean zMapStack2fd(unsigned int remove, int fd) ;

void zMapSignalHandler(int sig_no) ;

char *zMapGetDir(const char *directory_in, gboolean home_relative, gboolean make_dir) ;
char *zMapGetFile(char *directory, const char *filename, gboolean make_file, const char *permissions, GError **error) ;
const char *zMapGetTmpDir() ;
char *zMapGetPath(char *path_in) ;
char *zMapExpandFilePath(const char *path_in) ;
gboolean zMapFileAccess(const char *filepath, const char *mode) ;
gboolean zMapFileEmpty(char *filepath) ;


/* routines to return basic version/information about zmap. */
const char *zMapGetAppName(void) ;
const char *zMapGetAppVersionString(void) ;
const char *zMapGetAppTitle(void) ;
const char *zMapGetCopyrightString(void) ;
const char *zMapGetWebSiteString(void) ;
const char *zMapGetCommentsString(void) ;
const char *zMapGetLicenseString(void) ;
gboolean zMapCompareVersionStings(const char *reference_version, const char *test_version, GError **error) ;
const char *zMapGetCompileString(void) ;

gboolean zMapUtilsConfigDebug(char *config_file) ;

char *zMapMakeUniqueID(char *prefix) ;

long int zMapUtilsGetRawTime(void) ;
char *zMapGetTimeString(ZMapTimeFormat format, char *format_str_in) ;
gboolean zMapTimeGetTime(char *time_str_in, ZMapTimeFormat format, char *format_str,
                         long int *secs_out, long int *microsecs_out) ;

const char *zMapUtilsPtr2Str(void *ptr) ;
void *zMapUtilsStr2Ptr(char *ptr_str) ;

char zMapInt2Char(int num) ;

gboolean zMapStr2Bool(char *str, gboolean *bool_out) ;
gboolean zMapStr2Int(const char *str_in, int *int_out) ;
gboolean zMapInt2Str(int int_in, char **str_out) ;
gboolean zMapStr2LongInt(const char *str, long int *long_int_out) ;
gboolean zMapStr2Float(char *str, float *float_out) ;
gboolean zMapStr2Double(char *str, double *double_out) ;

#define zMapUtilsSwop(TYPE, FIRST, SECOND)   \
  { TYPE tmp = (FIRST) ; (FIRST) = (SECOND) ; (SECOND) = tmp ; }


ZMAP_ENUM_TO_SHORT_TEXT_DEC(zmapProcTerm2ShortText, ZMapProcessTerminationType) ;
ZMAP_ENUM_TO_LONG_TEXT_DEC(zmapProcTerm2LongText, ZMapProcessTerminationType) ;
int zMapUtilsProcessTerminationStatus(int status, ZMapProcessTerminationType *termination_type_out) ;

gboolean zMapUtilsSysCall(char *cmd_str, char **err_msg_out) ;
gboolean zMapUtilsSpawnAsyncWithPipes(char *argv[], GIOFunc stdin_writer, GIOFunc stdout_reader, GIOFunc stderr_reader,
				      gpointer pipe_data, GDestroyNotify pipe_data_destroy, GError **error,
				      GChildWatchFunc child_func, ZMapUtilsChildWatchData child_func_data);

gboolean zMapLaunchWebBrowser(char *link, GError **error) ;

char *zMapUtilsSysGetSysName(void) ;

void zMapUtilsUserInit(void) ;
gboolean zMapUtilsUserIsDeveloper(void) ;
gboolean zMapUtilsUserSetDeveloper(char *passwd) ;

void zMapPrintQuark(GQuark quark) ;
void zMapLogQuark(GQuark quark) ;
gboolean zMapLogQuarkIsStr(GQuark quark, const char *str) ;
gboolean zMapLogQuarkIsExactStr(GQuark quark, char *str) ;
gboolean zMapLogQuarkHasStr(GQuark quark, char *sub_str) ;
gboolean zMapLogQuarkHasExactStr(GQuark quark, char *sub_str) ;

gboolean zMapCoordsClamp(int range_start, int range_end, int *start_inout, int *end_inout) ;
void zmapCoordsZeroBased(int *start_inout, int *end_inout) ;
void zMapCoordsToOffset(int base, int start_value, int *start_inout, int *end_inout) ;


/* zmapRadixSort.c */
#define RADIX_BITS      8           /* this will never change so don't*/
typedef guint ZMapRadixKeyFunc (gconstpointer thing,int digit);
GList * zMapRadixSort(GList *list_arg, ZMapRadixKeyFunc *key_func,int key_size);

gboolean zMapUtilsBioParseChromLoc(char *location_str, char **chromsome_out, int *start_out, int *end_out) ;
gboolean zMapUtilsBioParseChromNumber(char *chromosome_str, char **chromosome_out) ;



#endif /* ZMAP_UTILS_H */

