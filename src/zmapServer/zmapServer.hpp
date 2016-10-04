/*  File: zmapServer.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
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
                                                  GQuark source_name, char *config_file,
                                                  ZMapURL url,
                                                  char *version_str);
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
ZMapServerResponseType zMapServerGetConnectState(ZMapServer server, ZMapServerConnectStateType *connect_state) ;
ZMapServerResponseType zMapServerCloseConnection(ZMapServer server) ;
ZMapServerResponseType zMapServerFreeConnection(ZMapServer server) ;


#endif /* !ZMAP_SERVER_H */
