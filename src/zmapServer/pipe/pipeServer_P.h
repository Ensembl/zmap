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


#define PIPE_PROTOCOL_STR "GFF Pipe"			    /* For error messages. */
#define FILE_PROTOCOL_STR "GFF File"



/* Helpful to see script args in all error messages so add these to standard messages. */
#define ZMAPPIPESERVER_LOG(LOGTYPE, PROTOCOL, HOST, ARGS, FORMAT, ...) \
  ZMAPSERVER_LOG(LOGTYPE, PROTOCOL, HOST, ", script_args: \"%s\" ", FORMAT, __VA_ARGS__)




/* Holds all the state we need to create and access the script output. */
typedef struct _PipeServerStruct
{
  gchar *config_file ;

  char *protocol ;					    /* GFF Pipe or File */
  ZMapURLScheme scheme ;				    /* pipe:// or file:// */

  gchar *script_path ;					    /* Path to the script OR file. */
  gchar *script_args ;					    /* Args to the script derived from the url string */

  gchar *data_dir ;					    /* default location for data files
							       (when protocol is file://) */

  GIOChannel *gff_pipe ;				    /* the pipe we read the script's stdout from */

  GPid child_pid ;

  gint wait ;						    /* delay before getting data, mainly for testing */

  ZMapServerResponseType result ;
  gboolean error ;					    /* TRUE if any error occurred. */
  char *last_err_msg ;
  gint exit_code ;

  gboolean sequence_server ;
  gboolean is_otter ;

  ZMapGFFParser parser ;				    /* holds header features and sequence data till requested */
  GString *gff_line ;

  gint zmap_start, zmap_end ;				    /* display coordinates of interesting region */
  ZMapFeatureContext req_context ;
  ZMapFeatureSequenceMap sequence_map ;
  char *styles_file ;
  GHashTable *source_2_sourcedata ;			    /* mapping data as per config file */
  GHashTable *featureset_2_column ;

} PipeServerStruct, *PipeServer ;



#endif /* PIPE_SERVER_P_H */
