/*  File: zmapCmdLineArgs.h
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
 * Description: Interface to retrieve command line args given to the
 *              program.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CMDLINEARGS_H
#define ZMAP_CMDLINEARGS_H

#include <glib.h>


/* The value (if any) for the command line option is returned in this union.
 * NOTE that since the code that uses the option _must_ know what sort of data it is, there
 * is no type field here to record which member of the union is valid. */
typedef union
{
  gboolean b ;
  int i ;
  char *s ;
  char **sa ;
  double f ;
} ZMapCmdLineArgsType ;



/* Common args that user may give. */

#define ZMAPARG_VERSION        "version"
#define ZMAPARG_RAW_VERSION    "raw_version"
#define ZMAPARG_SERIAL         "serial"
#define ZMAPARG_SLEEP          "sleep"
#define ZMAPARG_SEQUENCE_START "start"
#define ZMAPARG_SEQUENCE_END   "end"
#define ZMAPARG_CONFIG_FILE    "conf_file"
#define ZMAPARG_CONFIG_DIR     "conf_dir"
#define ZMAPARG_STYLES_FILE    "styles_file"
#define ZMAPARG_SINGLE_SCREEN  "single_screen"

/* Soon to be removed. */
#define ZMAPARG_WINDOW_ID      "win_id"


/* Developer or programmer args. */
#define ZMAPARG_REMOTE_DEBUG   "remote-debug"

/* These will go.... */
#define ZMAPARG_PEER_NAME      "peer-name"
#define ZMAPARG_PEER_CLIPBOARD "peer-clipboard"

#define ZMAPARG_PEER_SOCKET    "peer-socket"



#define ZMAPARG_SERIAL         "serial"
#define ZMAPARG_SLEEP          "sleep"
#define ZMAPARG_TIMING         "timing"
#define ZMAPARG_SHRINK         "shrink"	// to allow the window to shrink: gives too small size by default
#define ZMAPARG_SEQUENCE       "sequence"	// [dataset/]sequence
#define ZMAPARG_FILES          "<file(s)>"


void zMapCmdLineArgsCreate(int *argc, char *argv[]) ;
gboolean zMapCmdLineArgsValue(const char *arg_name, ZMapCmdLineArgsType *result) ;
char **zMapCmdLineFinalArg(void) ;
void zMapCmdLineArgsDestroy(void) ;


#endif /* !ZMAP_CMDLINEARGS_H */

