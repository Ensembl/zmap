/*  File: zmapServerProtocolHandler.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description:
 * Exported functions: See ZMap/zmapServerProtocol.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>

#include <ZMap/zmapServerProtocol.hpp>
#include <ZMap/zmapThreadSlave.hpp>
#include <ZMap/zmapDataSlave.hpp>



using namespace ZMapDataSource ;




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
static ZMapThreadReturnCode terminateServer(ZMapServer *server, char **err_msg_out) ;
static ZMapThreadReturnCode destroyServer(ZMapServer *server) ;
static ZMapThreadReturnCode thread_RC(ZMapServerResponseType code) ;



static ZMapServerReqType getNextRequest(ZMapServerReqType curr_request, DataSourceRequestType type) ;



/*
 *                           External Interface
 */




/*
 *                     Package routines
 */


/* This routine sits in the slave thread and is called by the threading interface to
 * service a request from the master thread which includes decoding it, calling the appropriate server
 * routines and returning the answer to the master thread. */
ZMapThreadReturnCode zmapDataSourceThreadRequestHandler(void **slave_data, void *request_in, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  DataSource *source = static_cast<DataSource *>(request_in) ;
  DataSlave data_slave ;
  char *config_file = NULL ;
  ZMapURL url_obj = NULL ;
  char *version_str = NULL ;
  ZMapFeatureSequenceMap sequence_map = NULL ;
  int start = 0, end = 0 ;
  ZMapFeatureContext context = NULL ;
  ZMapStyleTree *styles = NULL ;
  DataSourceRequestType type = DataSourceRequestType::INVALID ;


  if ((*slave_data) || !request_in || !err_msg_out)
    {
      // Basic sanity check.
      char *err_msg ;

      err_msg = g_strdup_printf("Bad parameters to request handler: %s%s%s",
                                ((*slave_data) ? " slave_data not NULL" : ""),
                                (!request_in ? " no request_in param" : ""),
                                (!err_msg_out ? " no err_msg_out param" : "")) ;

      zMapLogWarning("%s", err_msg) ;

      *err_msg_out = err_msg ;

      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }
  else if (!data_slave.GetServerInfo(*source, &config_file, &url_obj, &version_str)
           || !data_slave.GetSequenceData(*source, &sequence_map, &start, &end))
    {
      // Get all the params needed for the server calls.
      char *err_msg ;

      // WE SHOULDN'T BE ABLE TO GET HERE.....AT ALL......

      err_msg = g_strdup("Missing source configuration parameters from the DataSource object.") ;

      zMapLogWarning("%s", err_msg) ;

      *err_msg_out = err_msg ;

      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }
  else
    {
      type = data_slave.GetRequestType(*source) ;

      if (type == DataSourceRequestType::GET_FEATURES)
        {
          DataSourceFeatures *feature_req = static_cast<DataSourceFeatures *>(source) ;

          if (!data_slave.GetRequestParams(*feature_req, &context, &styles))
            {
              char *err_msg ;

              err_msg = g_strdup("Missing \"feature\" request parameters from the DataSource object.") ;

              zMapLogWarning("%s", err_msg) ;

              *err_msg_out = err_msg ;

              thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
            }
        }
      else if (type == DataSourceRequestType::GET_SEQUENCE)
        {
          DataSourceSequence *sequence_req = static_cast<DataSourceSequence *>(source) ;

          if (!data_slave.GetRequestParams(*sequence_req, &context, &styles))
            {
              char *err_msg ;

              err_msg = g_strdup("Missing \"sequence\" request parameters from the DataSource object.") ;

              zMapLogWarning("%s", err_msg) ;

              *err_msg_out = err_msg ;

              thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
            }
        }
    }


  // If we got everything then make the server calls.
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
    {
      bool finished ;
      ZMapServerReqType server_request ;
      ZMapServerReqAny req_any ;
      ZMapServer server ;
      ZMapServerResponseType server_response ;

      finished = false ;
      server_request = ZMAP_SERVERREQ_CREATE ;
      req_any = NULL ;
      server = NULL ;
      while (!finished)
        {
          int exit_code = 0 ;
          char *std_err = NULL ;

          // Must be other stuff to fill in for some of these requests....
          switch (server_request)
            {
            case ZMAP_SERVERREQ_CREATE:
              {
                req_any = zMapServerRequestCreate(server_request, config_file, url_obj, version_str) ;

                break ;
              }
            
            case ZMAP_SERVERREQ_OPEN:
              {
                ZMapServerReqOpen req_open ;

                req_any = zMapServerRequestCreate(server_request) ;

                req_open = (ZMapServerReqOpen)req_any ;
                req_open->sequence_map = sequence_map ;
                req_open->zmap_start= start ;
                req_open->zmap_end = end ;

                break ;
              }


            case ZMAP_SERVERREQ_GETSERVERINFO:
              {
                req_any = zMapServerRequestCreate(server_request) ;

                break ;
              }

            case ZMAP_SERVERREQ_FEATURES:
            case ZMAP_SERVERREQ_SEQUENCE:
              {
                // Need to choose here whether to get features or sequence....according to type of
                // request object.

                if (type == DataSourceRequestType::GET_FEATURES)
                  {
                    ZMapServerReqGetFeatures req_features ;

                    req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_FEATURES) ;

                    req_features = (ZMapServerReqGetFeatures)req_any ;
                    req_features->context = context ;
                    req_features->styles = styles ;
                  }
                else // type == DataSourceRequestType::GET_SEQUENCE
                  {
                    ZMapServerReqGetFeatures req_sequence ;

                    req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_SEQUENCE) ;

                    req_sequence = (ZMapServerReqGetFeatures)req_any ;
                    req_sequence->context = context ;
                    req_sequence->styles = styles ;
                  }

                break ;
              }

            case ZMAP_SERVERREQ_GETSTATUS:
              {
                req_any = zMapServerRequestCreate(server_request) ;

                break ;
              }

            case ZMAP_SERVERREQ_TERMINATE:
              {
                req_any = zMapServerRequestCreate(server_request) ;

                finished = true ;

                break ;
              }

            default:
              {
                // should never get here....

                zMapWarnIfReached() ;

                // should break out here.....

                break ;
              }

            }

          // Send the command.
          server_response = zMapServerRequest(&server, req_any, err_msg_out) ;


          // If all is ok then continue, for some commands we need to set a reply.
          switch (server_request)
            {
            case ZMAP_SERVERREQ_FEATURES:
              {
                DataSourceFeatures *feature_source = static_cast<DataSourceFeatures *>(source) ;
                ZMapServerReqGetFeatures req_features = (ZMapServerReqGetFeatures)req_any ;

                if (server_response == ZMAP_SERVERRESPONSE_OK)
                  {
                    // nothing just now.....because pointers to structs are passed in, the structs
                    // get filled in.

                    data_slave.SetReply(*feature_source, req_features->context, req_features->styles) ;
                  }
                else
                  {
                    exit_code = req_features->exit_code ;
                    std_err = req_features->stderr_out ;

                    data_slave.SetError(*feature_source, exit_code, std_err) ;

                    finished = true ;
                  }

                break ;
              }

            case ZMAP_SERVERREQ_SEQUENCE:
              {
                DataSourceSequence *sequence_source = static_cast<DataSourceSequence *>(source) ;
                ZMapServerReqGetFeatures req_features = (ZMapServerReqGetFeatures)req_any ;

                if (server_response == ZMAP_SERVERRESPONSE_OK)
                  {
                    // nothing just now.....because pointers to structs are passed in, the structs
                    // get filled in.

                    data_slave.SetReply(*sequence_source, req_features->context, req_features->styles) ;
                  }
                else
                  {
                    exit_code = req_features->exit_code ;
                    std_err = req_features->stderr_out ;

                    data_slave.SetError(*sequence_source, exit_code, std_err) ;

                    finished = true ;
                  }

                break ;
              }

            default:
              {
                if (server_response != ZMAP_SERVERRESPONSE_OK)
                  {
                    const char *err_msg ;
                    exit_code = 1 ;

                    // Needs more detail.....
                    err_msg = g_strdup_printf("Server failed on request \"%s\" with error \"%s\".",
                                              zMapServerReqType2ExactStr(server_request),
                                              zMapServerResponseType2ExactStr(server_response)) ;

                    data_slave.SetError(*source, err_msg) ;

                    finished = true ;
                  }

                break ;
              }
            }

          // Clear up the request ready for the next one or to finish.
          zMapServerRequestDestroy(req_any) ;

          // Terminate or move to next request.
          if (server_response == ZMAP_SERVERRESPONSE_OK)
            {
              if (server_request == ZMAP_SERVERREQ_TERMINATE)
                finished = true ;
              else
                server_request = getNextRequest(server_request, type) ;
            }

        }


      // Here's the bit that's wrong.....the server_request should be returned separately from the
      // thread_rc.....

      // Slightly hacky here unfortunately, but there's no clean 1:1 relationship in the
      // reply:threadrc relationship.
      if (server_request == ZMAP_SERVERREQ_TERMINATE)
        {
          if (server_response == ZMAP_SERVERRESPONSE_OK)
            thread_rc = ZMAPTHREAD_RETURNCODE_QUIT ;
          else
            thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
        }
      else
        {
          thread_rc = thread_RC(server_response) ;
        }
    }


  return thread_rc ;
}



/* This function is called if a thread terminates. */
ZMapThreadReturnCode zmapDataSourceThreadTerminateHandler(void **slave_data, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;

  //ok slave_data should be our object and we need to set the server out of that.....
  // and then call zMapServerTerminateHandler()

  ZMapServer server ;

  zMapReturnValIfFail((slave_data && err_msg_out), ZMAPTHREAD_RETURNCODE_BADREQ) ;

  server = (ZMapServer)*slave_data ;

  thread_rc = terminateServer(&server, err_msg_out) ;

  return thread_rc ;
}


/* This function is called if a thread terminates. */
ZMapThreadReturnCode zmapDataSourceThreadDestroyHandler(void **slave_data)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;

  //ok slave_data should be our object and we need to set the server out of that.....
  // and then call zMapServerDestroyHandler()

  ZMapServer server ;

  zMapReturnValIfFail((slave_data), ZMAPTHREAD_RETURNCODE_BADREQ) ;

  server = (ZMapServer)*slave_data ;

  thread_rc = destroyServer(&server) ;

  return thread_rc ;
}




/*
 *                     Internal routines
 */


static ZMapThreadReturnCode terminateServer(ZMapServer *server, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServerReqTerminateStruct term_req = {ZMAP_SERVERREQ_TERMINATE, ZMAP_SERVERRESPONSE_INVALID} ;
  ZMapServerResponseType reply ;

  if ((reply = zMapServerRequest(server, (ZMapServerReqAny)&term_req, err_msg_out)) == ZMAP_SERVERRESPONSE_OK)
    {
      *server = NULL ;
    }
  else
    {
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }

  return thread_rc ;
}


// Why do we have this....not sure we need it any more....it should be same as terminate....
static ZMapThreadReturnCode destroyServer(ZMapServer *server)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServerReqTerminateStruct term_req = {ZMAP_SERVERREQ_TERMINATE, ZMAP_SERVERRESPONSE_INVALID} ;
  ZMapServerResponseType reply ;
  char *err_msg = NULL ;

  if ((reply = zMapServerRequest(server, (ZMapServerReqAny)&term_req, &err_msg)) == ZMAP_SERVERRESPONSE_OK)
    {
      *server = NULL ;
    }
  else
    {
      // Log err message
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }

  return thread_rc ;
}


/* THIS NEEDS CLEANING UP AND WILL BE BY THE THREADS REWRITE... */
/* Return a thread return code for each type of server response. */
static ZMapThreadReturnCode thread_RC(ZMapServerResponseType code)
{
  ZMapThreadReturnCode thread_rc ;

  switch(code)
    {
    case ZMAP_SERVERRESPONSE_OK:
      thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
      break ;

    case ZMAP_SERVERRESPONSE_BADREQ:
      thread_rc = ZMAPTHREAD_RETURNCODE_BADREQ ;
      break ;

    /*
     * Modified version; next step is to remove NODATA
     */
    case ZMAP_SERVERRESPONSE_REQFAIL:
    case ZMAP_SERVERRESPONSE_UNSUPPORTED:
    case ZMAP_SERVERRESPONSE_SOURCEERROR:
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
      break ;

    /*
     * Valid gff header, but no features or fasta data.
     */
    case ZMAP_SERVERRESPONSE_SOURCEEMPTY:
      thread_rc = ZMAPTHREAD_RETURNCODE_SOURCEEMPTY ;
      break ;


    case ZMAP_SERVERRESPONSE_TIMEDOUT:
    case ZMAP_SERVERRESPONSE_SERVERDIED:
      thread_rc = ZMAPTHREAD_RETURNCODE_SERVERDIED ;
      break ;

    default:
      thread_rc = ZMAPTHREAD_RETURNCODE_INVALID ;
      break ;
    }

  return thread_rc ;
}



static ZMapServerReqType getNextRequest(ZMapServerReqType curr_request, DataSourceRequestType type)
{
  ZMapServerReqType next_request ;

  switch (curr_request)
    {
    case ZMAP_SERVERREQ_CREATE:
      next_request = ZMAP_SERVERREQ_OPEN ;
      break ;
    case ZMAP_SERVERREQ_OPEN:
      next_request = ZMAP_SERVERREQ_GETSERVERINFO ;
      break ;
    case ZMAP_SERVERREQ_GETSERVERINFO:
      if (type == DataSourceRequestType::GET_FEATURES)
        next_request = ZMAP_SERVERREQ_FEATURES ;
      else
        next_request = ZMAP_SERVERREQ_SEQUENCE ;
      break ;
    case ZMAP_SERVERREQ_FEATURES:
    case ZMAP_SERVERREQ_SEQUENCE:
      next_request = ZMAP_SERVERREQ_GETSTATUS ;
      break ;
    case ZMAP_SERVERREQ_GETSTATUS:
      next_request = ZMAP_SERVERREQ_TERMINATE ;
      break ;
    case ZMAP_SERVERREQ_TERMINATE:
      next_request = ZMAP_SERVERREQ_INVALID ;
      break ;
    default:
      next_request = ZMAP_SERVERREQ_INVALID ;
      zMapWarnIfReached() ;
      break ;
    }


  return next_request ;
}
