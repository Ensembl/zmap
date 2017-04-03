/*  File: zmapServer.h
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
 * Description: Generalised server interface, hides details of
 *              acedb/file etc. details from caller.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SERVER_H
#define ZMAP_SERVER_H

#include <glib.h>

#include <ZMap/zmapFeature.hpp>
#include <ZMap/zmapUrl.hpp>

#include <ZMap/zmapServerProtocol.hpp>



/* Opaque type, represents a connection to a data server. */
typedef struct _ZMapServerStruct *ZMapServer ;


gboolean zMapServerGlobalInit(ZMapURL url, void **server_global_data_out) ;
ZMapServerResponseType zMapServerCreateConnection(ZMapServer *server_out, void *server_global_data,
                                                  ZMapConfigSource config_source) ;
ZMapServerResponseType zMapServerOpenConnection(ZMapServer server,ZMapServerReqOpen req_open) ;
ZMapServerResponseType zMapServerGetServerInfo(ZMapServer server, ZMapServerReqGetServerInfo info) ;
ZMapServerResponseType zMapServerFeatureSetNames(ZMapServer server,
						 GList **feature_sets_inout,
						 GList **biotypes_inout,
						 GList *sources,
						 GList **required_styles,
						 GHashTable **featureset_2_stylelist_out,
						 GHashTable **source_2_featureset_out,
						 GHashTable **source_2_sourcedata_out) ;
ZMapServerResponseType zMapServerGetStyles(ZMapServer server, GHashTable **types_out) ;
ZMapServerResponseType zMapServerGetFeatures(ZMapServer server,
					     ZMapStyleTree &styles, ZMapFeatureContext feature_context) ;
ZMapServerResponseType zMapServerGetContextSequences(ZMapServer server,
						     ZMapStyleTree &styles, ZMapFeatureContext feature_context) ;
ZMapServerResponseType zMapServerStylesHaveMode(ZMapServer server, gboolean *have_mode) ;
ZMapServerResponseType zMapServerGetSequence(ZMapServer server, GList *sequences_inout) ;
ZMapServerResponseType zMapServerSetContext(ZMapServer server, ZMapFeatureContext feature_context) ;
ZMapFeatureContext zMapServerCopyContext(ZMapServer server) ;
const char *zMapServerLastErrorMsg(ZMapServer server) ;
ZMapServerResponseType zMapServerGetStatus(ZMapServer server, gint *exit_code);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
ZMapServerResponseType zMapServerGetConnectState(ZMapServer server, ZMapServerConnectStateType *connect_state) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

ZMapServerResponseType zMapServerCloseConnection(ZMapServer server) ;
ZMapServerResponseType zMapServerFreeConnection(ZMapServer server) ;


#endif /* !ZMAP_SERVER_H */
