/*  File: zmapServerPrototype.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Describes the interface between zmapServer, the generalised
 *              server interface and the underlying server specific
 *              implementation. Only specific server implementations should
 *              include this header, its not for general consumption.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SERVER_PROTOTYPEP_H
#define ZMAP_SERVER_PROTOTYPEP_H

#include <glib.h>

#include <ZMap/zmapServerProtocol.hpp>



/* Define function prototypes for generalised server calls, they all go in
 * the ZMapServerFuncsStruct struct that represents the calls for a server
 * of a particular protocol. */
typedef gboolean (*ZMapServerGlobalFunc)(void) ;

typedef gboolean (*ZMapServerCreateFunc)(void **server_conn,
                                         ZMapConfigSource config_source) ;

typedef ZMapServerResponseType (*ZMapServerOpenFunc)(void *server_conn, ZMapServerReqOpen req_open) ;

typedef ZMapServerResponseType (*ZMapServerGetServerInfo)(void *server_in, ZMapServerReqGetServerInfo info) ;

typedef ZMapServerResponseType (*ZMapServerGetFeatureSets)(void *server_in,
							   GList **feature_sets_inout,
							   GList **biotypes_inout,
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
							ZMapStyleTree &styles,
							ZMapFeatureContext feature_context) ;

// ok....need to remove feature context and styles from here and replace with raw dna stuff.....   
typedef ZMapServerResponseType (*ZMapServerGetContextSequences)(void *server_conn,
                                                                ZMapStyleTree &styles,
                                                                ZMapFeatureContext feature_context,
                                                                char *sequence_name,
                                                                int start, int end,
                                                                int *dna_length_out, char **dna_sequence_out) ;

typedef ZMapServerResponseType (*ZMapServerGetStatusFunc)(void *server_conn, gint *exit_code) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
typedef ZMapServerResponseType (*ZMapServerGetConnectStateFunc)(void *server_conn,
								ZMapServerConnectStateType *connect_state) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


typedef const char *(*ZMapServerGetErrorMsgFunc)(void *server_conn) ;

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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapServerGetConnectStateFunc get_connect_state ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  ZMapServerCloseFunc close ;
  ZMapServerDestroyFunc destroy ;
} ZMapServerFuncsStruct, *ZMapServerFuncs ;


/* Try to give consistent messages/logging.... */
#define ZMAP_SERVER_MSGPREFIX "Server %s:%s - "

/* LOGTYPE just be one of the zMapLogXXXX types, i.e. Message, Warning, Critical or Fatal */
#define ZMAPSERVER_LOG(LOGTYPE, PROTOCOL, HOST, FORMAT, ...) \
zMapLog##LOGTYPE(ZMAP_SERVER_MSGPREFIX FORMAT, PROTOCOL, HOST, __VA_ARGS__)

#define ZMAPSERVER_MAKEMESSAGE(PROTOCOL, HOST, FORMAT, ...) \
g_strdup_printf(ZMAP_SERVER_MSGPREFIX FORMAT, PROTOCOL, HOST, __VA_ARGS__)



#endif /* !ZMAP_SERVER_PROTOTYPEP_H */
