/*  File: check_zmapConfigLoader.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#ifndef CHECK_ZMAP_CONFIG_LOADER_H
#define CHECK_ZMAP_CONFIG_LOADER_H

#include <glib.h>
#include <check.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapConfigIni.h>

#define ZMAP_CHECK_CONFIG_SUITE_NAME "Config Loader Suite"

#ifndef CONFIG_DIR
#define CONFIG_DIR "share/ZMap"
#endif /* CONFIG_DIR */

#ifndef CONFIG_FILE
#define CONFIG_FILE "ZMap"
#endif /* CONFIG_FILE */

Suite *zMapCheckConfigLoader(void);

#endif /* CHECK_ZMAP_CONFIG_LOADER_H */


