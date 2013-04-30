/*  File: zmapCmdLineArgs.h
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Interface to retrieve command line args given to the
 *              program.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CMDLINEARGS_H
#define ZMAP_CMDLINEARGS_H

#include <glib.h>


/*! @addtogroup cmdline_args
 * @{
 */


/*!
 * The value (if any) for the command line option is returned in this union.
 * NOTE that since the code that uses the option _must_ know what sort of data it is, there
 * is no type field here to record which member of the union is valid. */
typedef union
{
  gboolean b ;
  int i ;
  char *s ;
  char **sa;
  double f ;
} ZMapCmdLineArgsType ;


/*!
 *
 * @section cmdline_flags   Command line flags
 *
 * <table>
 *  <tr>
 *  <th colspan=4 align=center>"ZMap Command Line Flags"</th>
 *  </tr>
 *  <tr>
 *  <th>Flag</th>
 *  <th>Datatype</th>
 *  <th>Default</th>
 *  <th>Description</th>
 *  </tr>
 *  <tr>
 *  <th>"version"</th>
 *  <td>none</td>
 *  <td>none</td>
 *  <td>--version, causes zmap to print its version information to stdout and then exit.</td>
 *  </tr>
 *  <tr>
 *  <th>"start"</th>
 *  <td>Int</td>
 *  <td>1</td>
 *  <td>--start=nnnn, sets start coordinate of section of sequence to be displayed.</td>
 *  </tr>
 *  <tr>
 *  <th>"end"</th>
 *  <td>Int</td>
 *  <td>end of sequence</td>
 *  <td>--end=nnnn, sets end coordinate of section of sequence to be displayed.</td>
 *  </tr>
 *  <tr>
 *  <th>"conf_dir"</th>
 *  <td>String</td>
 *  <td>$HOME/ZMap</td>
 *  <td>--conf_dir=directory, zmap will read all configuration information from "directory" instead of
 *      from $HOME/ZMap</td>
 *  </tr>
 *  <tr>
 *  <th>"conf_file"</th>
 *  <td>String</td>
 *  <td>$HOME/ZMap/.ZMap</td>
 *  <td>--conf_file=filename, zmap will read configuration information from "filename" instead of
 *      from $HOME/ZMap/.ZMap</td>
 *  </tr>
 *  <tr>
 *  <th>"win_id"</th>
 *  <td>Int</td>
 *  <td>none</td>
 *  <td>--win_id=nnnn, zmap will send xremote commands to the X window with window id "nnnn".</td>
 *  </tr>
 * </table>
 *
 *  */



/*! @} end of cmdline_args docs. */



#define ZMAPARG_VERSION        "version"
#define ZMAPARG_RAW_VERSION    "raw_version"
#define ZMAPARG_SERIAL         "serial"
#define ZMAPARG_SLEEP          "sleep"
#define ZMAPARG_SEQUENCE_START "start"
#define ZMAPARG_SEQUENCE_END   "end"
#define ZMAPARG_CONFIG_FILE    "conf_file"
#define ZMAPARG_CONFIG_DIR     "conf_dir"
#define ZMAPARG_WINDOW_ID      "win_id"
#define ZMAPARG_TIMING         "timing"
#define ZMAPARG_SHRINK         "shrink"	// to allow the window to shrink: gives too small size by default
#define ZMAPARG_SEQUENCE       "sequence"	// [dataset/]sequence
#define ZMAPARG_FILES          "<file(s)>"


void zMapCmdLineArgsCreate(int *argc, char *argv[]) ;
gboolean zMapCmdLineArgsValue(char *arg_name, ZMapCmdLineArgsType *result) ;
char **zMapCmdLineFinalArg(void) ;
void zMapCmdLineArgsDestroy(void) ;


#endif /* !ZMAP_CMDLINEARGS_H */

