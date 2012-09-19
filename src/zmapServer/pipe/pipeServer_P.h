/*  Last edited: Jul 23 14:58 2012 (edgrif) */
/*  File: pipeServer_P.h
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *      derived from fileServer_p.h by Ed Griffiths (edgrif@sanger.ac.uk)
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Defines/types etc. for the file accessing version
 *              of the server code.
 *
 *-------------------------------------------------------------------
 */
#ifndef PIPE_SERVER_P_H
#define PIPE_SERVER_P_H


#define PIPE_PROTOCOL_STR "GFF Pipe"			    /* For error messages. */
#define FILE_PROTOCOL_STR "GFF File"





/* Holds all the state we need to create and access the script output. */
typedef struct _PipeServerStruct
{
  gchar *config_file ;
  gchar *script_dir;		// default location for relative paths
  gchar *script_path ;	      // where our configured script is, including script-dir
  gchar *query;		    	// from query string
  GIOChannel *gff_pipe ;      // the pipe we read the script's output from
  GIOChannel *gff_stderr ;    // the pipe we read the script's error output from
  GPid child_pid;
  gint zmap_start,zmap_end;   // display coordinates of interesting region
  gint wait;                  // delay before getting data, mainly for testing

  ZMapURLScheme scheme;       // pipe:// or file://
  gchar *data_dir;            // default location for data files (when protocol is file://)

  char *styles_file ;

  gboolean error ;            /* TRUE if any error occurred. */
  char *last_err_msg ;
  gchar *stderr_output;        // all of it
  gint exit_code;

  char *protocol;             // GFF Pipe or File

  ZMapFeatureContext req_context ;
  ZMapGFFParser parser ;      // holds header features and sequence data till requested
  GString * gff_line ;
  ZMapServerResponseType result ;

  ZMapFeatureSequenceMap sequence_map;
  gboolean sequence_server;
  gboolean is_otter;

  GHashTable *source_2_sourcedata;  // mapping data as per config file
  GHashTable *featureset_2_column;

} PipeServerStruct, *PipeServer ;



#endif /* PIPE_SERVER_P_H */
