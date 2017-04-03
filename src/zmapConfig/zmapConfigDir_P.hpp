/*  File: zmapConfigDir_P.h
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
 * Description: zmap configuration file handling.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONFIGDIR_P_H
#define ZMAP_CONFIGDIR_P_H

#include <ZMap/zmapConfigDir.hpp>


typedef struct _ZMapConfigDirStruct
{
//  char *default_dir ;

  char *config_dir ;

  char *config_file ;

  char *zmap_conf_dir;		/* The directory of $ZMAP_HOME/etc */

  char *zmap_conf_file;		/* The full path of $ZMAP_HOME/etc/ZMAP_USER_CONFIG_FILE */

  char *sys_conf_dir;		/* The directory of /etc. N.B. This should be changed to reflect any ./configure --sysconfdir=/somewhere/etc */

  char *sys_conf_file;		/* The full path of $ZMAP_HOME/etc/ZMAP_USER_CONFIG_FILE */

  char *prefs_conf_dir;         /* The driectory of $ZMAP_HOME/etc/ZMAP_USER_CONFIG_FILE */

  char *prefs_conf_file;        /* The full path of $ZMAP_HOME/etc/ZMAP_USER_CONFIG_FILE */
} ZMapConfigDirStruct, *ZMapConfigDir ;



#endif /* !ZMAP_CONFIGDIR_P_H */
