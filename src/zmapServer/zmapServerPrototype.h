/*  File: zmapServerPrototype.h
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
 * and was written by
 *     Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk &,
 *       Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *  Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Describes the interface between zmapServer, the generalised
 *              server interface and the underlying server specific
 *              implementation. Only specific server implementations should
 *              include this header, its not really for general consumption.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SERVER_PROTOTYPEP_H
#define ZMAP_SERVER_PROTOTYPEP_H

#include <glib.h>

#include <ZMap/zmapFeature.h>
#include <ZMap/zmapServerProtocol.h>




/* Define function prototypes for generalised server calls, they all go in
 * the ZMapServerFuncsStruct struct that represents the calls for a server
 * of a particular protocol. */
typedef gboolean (*ZMapServerGlobalFunc)(void) ;

typedef gboolean (*ZMapServerCreateFunc)(void **server_conn,
					 char *config_file,
					 ZMapURL url, char *format,
                                         char *version_str, int timeout) ;

typedef ZMapServerResponseType (*ZMapServerOpenFunc)(void *server_conn, ZMapServerReqOpen req_open) ;

typedef ZMapServerResponseType (*ZMapServerGetServerInfo)(void *server_in, ZMapServerReqGetServerInfo info) ;

typedef ZMapServerResponseType (*ZMapServerGetFeatureSets)(void *server_in,
							   GList **feature_sets_inout,
							   GList *sources,
							   GList **required_styles_out,
							   GHashTable **featureset_2_stylelist_out,
							   GHashTable **featureset_2_column_out,
							   GHashTable **source_2_sourcedata_out) ;

typedef ZMapServerResponseType (*ZMapServerGetStyles)(void *server_in,
						      GHashTable **styles_out) ;

typedef ZMapServerResponseType (*ZMapServerStylesHaveModes)(void *server_in, gboolean *have_modes_out) ;

typedef ZMapServerResponseType (*ZMapServerGetSequence)(void *server_in, GList *sequences_inout) ;

typedef ZMapServerResponseType (*ZMapServerSetContextFunc)(void *server_conn, ZMapFeatureContext feature_context)  ;

typedef ZMapFeatureContext (*ZMapServerCopyContextFunc)(void *server_conn) ;

typedef ZMapServerResponseType (*ZMapServerGetFeatures)(void *server_conn,
							GHashTable *styles,
							ZMapFeatureContext feature_context, int *num_features_out) ;

typedef ZMapServerResponseType (*ZMapServerGetContextSequences)(void *server_conn,
								GHashTable *styles,
								ZMapFeatureContext feature_context) ;

typedef ZMapServerResponseType (*ZMapServerGetStatusFunc)(void *server_conn, gint *exit_code) ;

typedef ZMapServerResponseType (*ZMapServerGetConnectStateFunc)(void *server_conn,
								ZMapServerConnectStateType *connect_state) ;

typedef char *(*ZMapServerGetErrorMsgFunc)(void *server_conn) ;

typedef ZMapServerResponseType (*ZMapServerCloseFunc)(void *server_conn) ;

typedef ZMapServerResponseType (*ZMapServerDestroyFunc)(void *server_conn) ;


typedef struct _ZMapServerFuncsStruct
{
  ZMapServerGlobalFunc global_init ;
  ZMapServerCreateFunc create ;
  ZMapServerOpenFunc open ;
  ZMapServerGetServerInfo get_info ;
  ZMapServerGetFeatureSets feature_set_names ;
  ZMapServerGetStyles get_styles ;
  ZMapServerStylesHaveModes have_modes ;
  ZMapServerGetSequence get_sequence ;
  ZMapServerSetContextFunc set_context ;
  ZMapServerGetFeatures get_features ;
  ZMapServerGetContextSequences get_context_sequences ;
  ZMapServerGetErrorMsgFunc errmsg ;
  ZMapServerGetStatusFunc get_status ;
  ZMapServerGetConnectStateFunc get_connect_state ;
  ZMapServerCloseFunc close ;
  ZMapServerDestroyFunc destroy ;
} ZMapServerFuncsStruct, *ZMapServerFuncs ;



/* These are the hard coded function names for specific server implementations.
 * We could make this all more dynamic but this will do for now. */
/* commented out as only server implementations include this file, zmapServer_P.h has a copy
void acedbGetServerFuncs(ZMapServerFuncs acedb_funcs) ;
void dasGetServerFuncs(ZMapServerFuncs das_funcs) ;
void fileGetServerFuncs(ZMapServerFuncs file_funcs) ;
void pipeGetServerFuncs(ZMapServerFuncs pipe_funcs) ;
*/

/* Try to give consistent messages/logging.... */
#define ZMAP_SERVER_MSGPREFIX "Server %s:%s - "
#define ZMAP_PIPESERVER_MSGPREFIX "Server %s - "
#define ZMAP_PIPESERVER_MSG2FMT "Host URL was: %s %s"

/* LOGTYPE just be one of the zMapLogXXXX types, i.e. Message, Warning, Critical or Fatal */
#define ZMAPSERVER_LOG(LOGTYPE, PROTOCOL, HOST, FORMAT, ...) \
zMapLog##LOGTYPE(ZMAP_SERVER_MSGPREFIX FORMAT, PROTOCOL, HOST, __VA_ARGS__)

#define ZMAPPIPESERVER_LOG(LOGTYPE, PROTOCOL, HOST, ARG, FORMAT, ...) \
zMapLog##LOGTYPE(ZMAP_PIPESERVER_MSGPREFIX FORMAT, PROTOCOL, __VA_ARGS__);\
zMapLogMessage(ZMAP_PIPESERVER_MSG2FMT,  HOST, ARG)


#define ZMAPSERVER_MAKEMESSAGE(PROTOCOL, HOST, FORMAT, ...) \
g_strdup_printf(ZMAP_SERVER_MSGPREFIX FORMAT, PROTOCOL, HOST, __VA_ARGS__)



#endif /* !ZMAP_SERVER_PROTOTYPEP_H */
