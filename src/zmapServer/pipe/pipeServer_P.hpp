/*  File: pipeServer_P.h
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk), Ed Griffiths (edgrif@sanger.ac.uk)
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

  char *analysis_summary ;                                  // e.g. Augustus, phastCons etc., human etc.

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
