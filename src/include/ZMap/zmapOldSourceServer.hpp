/*  File: zmapViewDataSourceServer.hpp
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2016: Genome Research Ltd.
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *      Gemma Barson (Sanger Institute, UK) gb10@sanger.ac.uk
 *
 * Description: Represents an "old" data source, this will go as the
 *              the new DataSource code is used.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_DATASOURCESERVER_H
#define ZMAP_DATASOURCESERVER_H


#include <ZMap/zmapEnum.hpp>
#include <ZMap/zmapUrl.hpp>
#include <ZMap/zmapThreadSource.hpp>
#include <ZMap/zmapServerProtocol.hpp>




/* Malcolm has introduced this and put it in the threads package where it is NOT used
 * so I have moved it here....but is this necessary....revisit his logic....
 * Is it necessary.....revisit this...... */
#define ZMAP_VIEW_THREAD_STATE_LIST(_)                                                           \
_(THREAD_STATUS_INVALID,    , "Invalid",       "In valid thread state.",            "") \
_(THREAD_STATUS_OK,         , "OK",            "Thread ok.",               "") \
_(THREAD_STATUS_PENDING,    , "Pending",       "Thread pending....means what ?",       "") \
_(THREAD_STATUS_FAILED,     , "Failed",        "Thread has failed (needs killing ?).", "")

ZMAP_DEFINE_ENUM(ZMapViewThreadStatus, ZMAP_VIEW_THREAD_STATE_LIST) ;



// opaque type for a view session...good, it's hidden !!
//
typedef struct ZMapViewSessionServerStructType  *ZMapViewSessionServer ;


/* Represents a single connection to a data source.
 * We maintain state for our connections otherwise we will try to issue requests that
 * they may not see. curr_request flip-flops between ZMAP_REQUEST_GETDATA and ZMAP_REQUEST_WAIT */
typedef struct ZMapNewDataSourceStructType
{
  ZMapViewSessionServer server ;			    /* Server data. */


  ZMapThreadRequest curr_request ;

  ZMapThreadSource::ThreadSource *thread ;

  gpointer request_data ;				    /* User data pointer, needed for step implementation. */


  /* UM...IS THIS THE RIGHT PLACE FOR THIS....DOES A VIEWCONNECTION REPRESENT A SINGLE THREAD....
   * CHECK THIS OUT.......
   *  */
  ZMapViewThreadStatus thread_status ;			    /* probably this is badly placed,
							       but not as badly as before
							       at least it has some persistance now */

  gboolean show_warning ;

} ZMapNewDataSourceStruct, *ZMapNewDataSource ;




void zMapServerFormatSession(ZMapViewSessionServer server_data, GString *session_text) ;

void zmapViewSessionAddServer(ZMapViewSessionServer session_data, const ZMapURL url, char *format) ;
void zmapViewSessionAddServerInfo(ZMapViewSessionServer session_data, ZMapServerReqGetServerInfo session) ;



// These type of calls should replace the above....
const char *zMapServerGetUrl(ZMapNewDataSource view_conn) ;



ZMapNewDataSource zMapServerCreateViewConnection(ZMapNewDataSource view_con,
                                                 gpointer connect_data,
                                                 const char *url) ;
void *zMapServerConnectionGetUserData(ZMapNewDataSource view_conn) ;
void zMapServerDestroyViewConnection(ZMapNewDataSource view_conn) ;



#endif /* !ZMAP_DATASOURCESERVER_H */
