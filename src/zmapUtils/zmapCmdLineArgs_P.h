/*  File: zmapCmdLineArgs_P.h
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
 * Description: Internals for command line parsing.
 *
 * HISTORY:
 * Last edited: Feb 10 15:19 2005 (edgrif)
 * Created: Fri Feb  4 19:11:23 2005 (edgrif)
 * CVS info:   $Id: zmapCmdLineArgs_P.h,v 1.1 2005-02-10 16:31:50 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CMDLINEARGS_P_H
#define ZMAP_CMDLINEARGS_P_H

#include <popt.h>
#include <ZMap/zmapCmdLineArgs.h>



enum {ARG_SET = 1,					    /* Special value, do not alter. */
      ARG_START, ARG_END,
      ARG_CONF_FILE, ARG_CONF_DIR} ;



typedef struct _ZMapCmdLineArgsStruct
{
  /* The original argc/argv passed in. */
  int argc ;
  char **argv ;

  struct poptOption *options_table ;

  poptContext opt_context ;

  /* This holds the final argument on the command line which is the sequence name. */
  char *sequence_arg ;

  /* All option values are stored here for later reference. */
  int start, end ;

  char *config_dir ;
  char *config_file_path ;
} ZMapCmdLineArgsStruct, *ZMapCmdLineArgs ;





#endif /* !ZMAP_CMDLINEARGS_P_H */

