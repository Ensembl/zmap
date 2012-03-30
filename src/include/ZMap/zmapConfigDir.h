/*  File: zmapConfigDir.h
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
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

/* suffix for the window id file */
#define WINDOWID_SUFFIX "win_id" 

/* There is no context here because these commands create a global context for the whole
 * application so there is no point in returning it from the create. */
gboolean zMapConfigDirCreate(char *config_dir, char *config_file) ;
const char *zMapConfigDirDefaultName(void) ;
char *zMapConfigDirGetDir(void) ;
char *zMapConfigDirGetFile(void) ;
char *zMapConfigDirFindFile(char *filename) ;
char *zMapConfigDirGetZmapHomeFile(void);
char *zMapConfigDirGetSysFile(void);
void zMapConfigDirDestroy(void) ;
#endif /* !ZMAP_CONFIGDIR_H */

