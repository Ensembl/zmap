/*  File: zmapServer.h
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Generalised server interface, hides acedb/das/file
 *              details from caller.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SERVER_H
#define ZMAP_SERVER_H

#include <glib.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapUrl.h>
#include <ZMap/zmapServerProtocol.h>			    /*  Is this a good idea...see if there
							       is a more general sub header that
							       could be used.... */

#include <zmapServerPrototype.h>



/* Opaque type, represents a connection to a data server. */
typedef struct _ZMapServerStruct *ZMapServer ;



/* This routine must be called before any other server routines and must only be called once.
 * It is the callers responsibility to make sure this happens.
 * Provide matching Termination routine ???? */
gboolean zMapServerGlobalInit(ZMapURL url, void **server_global_data_out) ;


ZMapServerResponseType zMapServerCreateConnection(ZMapServer *server_out, void *server_global_data,
						  ZMapURL url,  char *format,
						  int timeout, char *version_str);
ZMapServerResponseType zMapServerOpenConnection(ZMapServer server,ZMapServerReqOpen req_open) ;
ZMapServerResponseType zMapServerGetServerInfo(ZMapServer server, ZMapServerInfo info) ;
ZMapServerResponseType zMapServerFeatureSetNames(ZMapServer server,
						 GList **feature_sets_inout,
						 GList *sources,
						 GList **required_styles,
						 GHashTable **featureset_2_stylelist_out,
						 GHashTable **source_2_featureset_out,
						 GHashTable **source_2_sourcedata_out) ;
ZMapServerResponseType zMapServerGetStyles(ZMapServer server, GHashTable **types_out) ;
ZMapServerResponseType zMapServerStylesHaveMode(ZMapServer server, gboolean *have_mode) ;
ZMapServerResponseType zMapServerGetSequence(ZMapServer server, GList *sequences_inout) ;
ZMapServerResponseType zMapServerSetContext(ZMapServer server, ZMapFeatureContext feature_context) ;
ZMapFeatureContext zMapServerCopyContext(ZMapServer server) ;
ZMapServerResponseType zMapServerGetFeatures(ZMapServer server, GHashTable *styles, ZMapFeatureContext feature_context) ;
ZMapServerResponseType zMapServerGetContextSequences(ZMapServer server,
						     GHashTable *styles, ZMapFeatureContext feature_context) ;
char *zMapServerLastErrorMsg(ZMapServer server) ;
ZMapServerResponseType zMapServerGetStatus(ZMapServer server, gint *exit_code, gchar **stderr_out);

ZMapServerResponseType zMapServerCloseConnection(ZMapServer server) ;
ZMapServerResponseType zMapServerFreeConnection(ZMapServer server) ;


#endif /* !ZMAP_SERVER_H */
