/*  File: pipeServer_P.h
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk), Ed Griffiths (edgrif@sanger.ac.uk)
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Defines/types etc. for the file accessing version
 *              of the server code.
 *
 *-------------------------------------------------------------------
 */
#ifndef PIPE_SERVER_P_H
#define PIPE_SERVER_P_H

#include <zmapServerPrototype.hpp>


#define PIPE_PROTOCOL_STR "GFF Pipe"			    /* For error messages. */


/* Holds all the state we need to create and access the script output. */
typedef struct _PipeServerStruct
{
  ZMapConfigSource source ;
  gchar *config_file ;

  char *url ;                                               /* Full url string. */
  const char *protocol ;                                    /* GFF Pipe or File */
  ZMapURLScheme scheme ;				    /* pipe:// or file:// */

  gchar *script_path ;					    /* Path to the script OR file. */
  gchar *script_args ;					    /* Args to the script derived from the url string */

  /* Pipe process. */
  GPid child_pid ;                                          /* pid of child process at other end of pipe. */
  guint child_watch_id ;                                    /* callback routine for child process exit. */
  gboolean child_exited ;                                   /* TRUE if child has exited. */
  gint exit_code ;                                          /* Can only be EXIT_SUCCESS or EXIT_FAILURE. */
  GIOChannel *gff_pipe ;				    /* the pipe we read the script's stdout from */

  /* Results of server requests. */
  ZMapServerResponseType result ;
  gboolean error ;					    /* TRUE if any error occurred. */
  char *last_err_msg ;

  gboolean sequence_server ;
  gboolean is_otter ;

  ZMapGFFParser parser ;				    /* holds header features and sequence data till requested */
  int gff_version ;                                         /* Cached because parser may be freed on error. */
  GString *gff_line ;                                       // Cached for efficiency.

  GQuark req_sequence ;
  gint zmap_start, zmap_end ;				    /* display coordinates of interesting region */
  ZMapFeatureContext req_context ;

  ZMapFeatureSequenceMap sequence_map ;
  char *styles_file ;
  GHashTable *source_2_sourcedata ;			    /* mapping data as per config file */
  GHashTable *featureset_2_column ;

} PipeServerStruct, *PipeServer ;



#endif /* PIPE_SERVER_P_H */
