/*  File: zmapCmdLineArgs_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Internals for command line parsing.
 *
 * HISTORY:
 * Last edited: Oct 13 17:39 2010 (edgrif)
 * Created: Fri Feb  4 19:11:23 2005 (edgrif)
 * CVS info:   $Id: zmapCmdLineArgs_P.h,v 1.9 2010-10-19 15:50:14 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CMDLINEARGS_P_H
#define ZMAP_CMDLINEARGS_P_H

#include <gtk/gtk.h>
#include <ZMap/zmapCmdLineArgs.h>


#define ZMAPARG_VERSION_DESC        "Program version."
#define ZMAPARG_SLEEP_DESC          "Makes ZMap sleep for 15 secs at start-up to give time to attach a debugger."
#define ZMAPARG_SEQUENCE_START_DESC "Start coord in sequence, must be in range 1 -> seq_length."
#define ZMAPARG_SEQUENCE_END_DESC   "End coord in sequence, must be in range start -> seq_length, but end == 0 means show to end of sequence."
#define ZMAPARG_CONFIG_FILE_DESC    "Relative or full path to configuration file."
#define ZMAPARG_CONFIG_DIR_DESC     "Relative or full path to configuration directory."
#define ZMAPARG_WINDOW_ID_DESC      "Window ID of the controlling application."
#define ZMAPARG_SEQUENCE_DESC       "Sequence name."

#define ZMAPARG_NO_ARG         "<none>"
#define ZMAPARG_COORD_ARG      "coord"
#define ZMAPARG_FILE_ARG       "file path"
#define ZMAPARG_DIR_ARG        "directory"
#define ZMAPARG_WINID_ARG      "0x0000000"
#define ZMAPARG_SEQUENCE_ARG   "<sequence name>"

#define ZMAPARG_INVALID_INT -1
#define ZMAPARG_INVALID_BOOL FALSE
#define ZMAPARG_INVALID_STR NULL
#define ZMAPARG_INVALID_FLOAT 0.0

enum {ARG_SET = 1,					    /* Special value, do not alter. */
      ARG_VERSION,
      ARG_START, ARG_END,
      ARG_CONF_FILE, ARG_CONF_DIR,
      ARG_WINID
} ;



typedef struct _ZMapCmdLineArgsStruct
{
  /* The original argc/argv passed in. */
  int argc ;
  char **argv ;

  GOptionContext *opt_context ;

  /* This holds the final argument on the command line which is the sequence name. */
  char **sequence_arg ;

  /* All option values are stored here for later reference. */
  gboolean version ;

  int start, end ;

  char *config_dir ;
  char *config_file_path ;
  char *window ;
} ZMapCmdLineArgsStruct, *ZMapCmdLineArgs ;





#endif /* !ZMAP_CMDLINEARGS_P_H */

