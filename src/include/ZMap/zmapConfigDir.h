/*  File: zmapConfigDir.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 * HISTORY:
 * Last edited: Feb 10 16:10 2005 (edgrif)
 * Created: Thu Feb 10 10:33:49 2005 (edgrif)
 * CVS info:   $Id: zmapConfigDir.h,v 1.1 2005-02-10 16:31:16 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONFIGDIR_H
#define ZMAP_CONFIGDIR_H


/* Default config directory name (in users home directory). */
#define ZMAP_USER_CONFIG_DIR    ".ZMap"

/* Default configuration file name within configuration directory. */
#define ZMAP_USER_CONFIG_FILE   "ZMap"


/* There is no context here because these commands create a global context for the whole
 * application so there is no point in returning it from the create. */
gboolean zMapConfigDirCreate(char *config_dir, char *config_file) ;
const char *zMapConfigDirDefaultName(void) ;
char *zMapConfigDirGetDir(void) ;
char *zMapConfigDirGetFile(void) ;
char *zMapConfigDirFindFile(char *filename) ;
void zMapConfigDirDestroy(void) ;

#endif /* !ZMAP_CONFIGDIR_H */

