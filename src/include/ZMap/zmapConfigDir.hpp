/*  File: zmapConfigDir.h
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
 * Description: Interface to get/set ZMap configuration directory
 *              and files.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONFIGDIR_H
#define ZMAP_CONFIGDIR_H


/* Default config directory name (in users home directory). */
#define ZMAP_USER_CONFIG_DIR    ".ZMap"

/* Default configuration file name within configuration directory. */
#define ZMAP_USER_CONFIG_FILE   "ZMap"

/* Default preferences file name within configuration directory. */
#define ZMAP_USER_PREFS_FILE   "zmap_prefs.ini"

/* suffix for the window id file */
#define WINDOWID_SUFFIX "win_id"

/* There is no context here because these commands create a global context for the whole
 * application so there is no point in returning it from the create. */
gboolean zMapConfigDirCreate(char *config_dir, char *config_file) ;
const char *zMapConfigDirDefaultName(void) ;
char *zMapConfigDirGetDir(void) ;
char *zMapConfigDirGetFile(void) ;
char *zMapConfigDirFindFile(const char *filename) ;
char *zMapConfigDirGetZmapHomeFile(void);
char *zMapConfigDirGetSysFile(void);
char *zMapConfigDirGetPrefsFile(void);
void zMapConfigDirDestroy(void) ;


#endif /* !ZMAP_CONFIGDIR_H */

