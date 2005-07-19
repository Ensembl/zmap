/*  File: zmapCmdLineArgs.h
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
 * Description: Interface to retrieve command line args given to the
 *              program.
 *
 * HISTORY:
 * Last edited: Jul 19 12:00 2005 (edgrif)
 * Created: Mon Feb  7 08:19:50 2005 (edgrif)
 * CVS info:   $Id: zmapCmdLineArgs.h,v 1.3 2005-07-19 13:32:26 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CMDLINEARGS_H
#define ZMAP_CMDLINEARGS_H

#include <glib.h>


/* These are all the keywords that can be used as command line option flags (e.g. --start) */
#define ZMAPARG_VERSION        "version"
#define ZMAPARG_SEQUENCE_START "start"
#define ZMAPARG_SEQUENCE_END   "end"
#define ZMAPARG_CONFIG_FILE    "conf_file"
#define ZMAPARG_CONFIG_DIR     "conf_dir"
#define ZMAPARG_WINDOW_ID      "win_id"


/* The value (if any) for the command line option is returned in this union.
 * NOTE that since the code that uses the option _must_ know what sort of data it is, there
 * is no type field here to record which member of the union is valid. */
typedef union
{
  gboolean b ;
  int i ;
  char *s ;
  double f ;
} ZMapCmdLineArgsType ;


/* There is no context here because these commands create a global context for the whole
 * application so there is no point in returning it from the create. */
void zMapCmdLineArgsCreate(int argc, char *argv[]) ;
gboolean zMapCmdLineArgsValue(char *arg_name, ZMapCmdLineArgsType *result) ;
char *zMapCmdLineFinalArg(void) ;
void zMapCmdLineArgsDestroy(void) ;


#endif /* !ZMAP_CMDLINEARGS_H */

