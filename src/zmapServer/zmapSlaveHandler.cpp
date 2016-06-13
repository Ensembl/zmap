/*  File: zmapServerProtocolHandler.c
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


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* YOU SHOULD BE AWARE THAT ON SOME PLATFORMS (E.G. ALPHAS) THERE SEEMS TO BE SOME INTERACTION
 * BETWEEN pthread.h AND gtk.h SUCH THAT IF pthread.h COMES FIRST THEN gtk.h INCLUDES GET MESSED
 * UP AND THIS FILE WILL NOT COMPILE.... */
#include <pthread.h>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <ZMap/zmapThreads.hpp>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#include <ZMap/zmapDataSource.hpp>

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <ZMap/zmapDataSourceRequest.hpp>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#include <ZMap/zmapServerProtocol.hpp>
#include <ZMap/zmapSlaveHandler.hpp>


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <ZMap/zmapConfigIni.hpp>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <zmapServer_P.hpp>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#include <zmapServer.hpp>
#include <zmapServerRequestHandler.hpp>


using namespace ZMapDataSource ;



static ZMapThreadReturnCode threadRequestHandler(void **slave_data, void *request_in, char **err_msg_out) ;
static ZMapThreadReturnCode threadTerminateHandler(void **slave_data, char **err_msg_out) ;
static ZMapThreadReturnCode threadDestroyHandler(void **slave_data) ;

static ZMapThreadReturnCode terminateServer(ZMapServer *server, char **err_msg_out) ;
static ZMapThreadReturnCode destroyServer(ZMapServer *server) ;
static ZMapThreadReturnCode thread_RC(ZMapServerResponseType code) ;




/*
 *                           External Interface
 */

bool zMapSlaveHandlerInit(ZMapSlaveRequestHandlerFunc *handler_func_out,
                          ZMapSlaveTerminateHandlerFunc *terminate_func_out,
                          ZMapSlaveDestroyHandlerFunc *destroy_func_out)
{
  bool result = false ;

  
  *handler_func_out = threadRequestHandler ;
  *terminate_func_out = threadTerminateHandler ;
  *destroy_func_out = threadDestroyHandler ;


  return result ;
}






/*
 *                     Internal routines
 */



/* This routine sits in the slave thread and is called by the threading interface to
 * service a request from the master thread which includes decoding it, calling the appropriate server
 * routines and returning the answer to the master thread. */
static ZMapThreadReturnCode threadRequestHandler(void **slave_data, void *request_in, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  DataSource *source = static_cast<DataSource *>(request_in) ;
  ZMapServer server ;
  bool finished ;
  int stage ;
  ZMapServerReqAny req_any ;
  ZMapServerReqType req_type ;
  ZMapServerResponseType reply ;


  /* I think in this case slave_data will always be NULL !! CHECK THIS..... */
  if (*slave_data)
    server = (ZMapServer)*slave_data ;


  finished = false ;
  stage = 0 ;
  req_any = NULL ;
  server = NULL ;
  while (!finished)
    {

      // Must be other stuff to fill in for some of these requests....
      switch (stage)
        {
        case 0:
          {
            // get info from object to fill in these params....

            char *config_file = NULL ;
            ZMapURL url_obj = NULL ;
            char *version_str = NULL ;

            if (source->GetServerInfo(&config_file, &url_obj, &version_str))
              {
                req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_CREATE,
                                                  config_file, url_obj, version_str) ;
              }

            break ;
          }
            
        case 1:
          {
            ZMapServerReqOpen req_open ;

            req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_OPEN) ;

            req_open = (ZMapServerReqOpen)req_any ;

            source->GetSequenceData(&(req_open->sequence_map),
                                    &(req_open->zmap_start), &(req_open->zmap_end)) ;

            break ;
          }


        case 2:
          {
            req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_GETSERVERINFO) ;

            break ;
          }

        case 3:
          {
            // Need to choose here whether to get features or sequence....according to type of
            // request object.

            if (source->GetRequestType() == ZMAP_SERVERREQ_FEATURES)
              {
                DataSourceFeatures *feature_req = static_cast<DataSourceFeatures *>(source) ;
                ZMapServerReqGetFeatures req_features ;

                req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_FEATURES) ;

                req_features = (ZMapServerReqGetFeatures)req_any ;

                feature_req->GetRequestData(&(req_features->context), &(req_features->styles)) ;
              }
            else if (source->GetRequestType() == ZMAP_SERVERREQ_SEQUENCE)
              {
                req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_SEQUENCE) ;
              }
            else
              {
                finished = true ;

                // MUST SET ERROR CODE.....OR SOMETHING !!!
              }

            break ;
          }

        case 4:
          {
            req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_GETSTATUS) ;

            break ;
          }

        case 5:
          {
            req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_TERMINATE) ;

            finished = true ;

            break ;
          }

        default:
          {
            // should never get here....

            zMapWarnIfReached() ;

            break ;
          }

        }

      req_type = req_any->type ;

      // Send the command and get the return code...
      if ((reply = zMapServerRequest(&server, req_any, err_msg_out)) == ZMAP_SERVERRESPONSE_OK)
        {
          // If all is ok we need to retrieve the context or whatever from the request.....

          switch (req_type)
            {
            case ZMAP_SERVERREQ_FEATURES:
              {
                ZMapServerReqGetFeatures req_features = (ZMapServerReqGetFeatures)req_any ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                DataSourceFeatures *feature_req  = static_cast <DataSourceFeatures *> (source) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



                // context should have been filled in so we shouldn't need to set it back in the
                // request object but what if things failed ?? we need to return the result in the
                // request object....
                source->SetReply(req_features->response, req_features->exit_code, req_features->stderr_out) ;

                break ;
              }

            case ZMAP_SERVERREQ_SEQUENCE:
              {


                break ;
              }


            default:
              break ;
            }




          stage++ ;
        }
      else
        {
          finished = true ;


          // ERROR REPORTING.....
        }


      // Clear up the request.
      zMapServerRequestDestroy(req_any) ;

    }



  // Slightly hacky here unfortunately, but there's no clean 1:1 relationship in the
  // reply:threadrc relationship.
  if (req_type == ZMAP_SERVERREQ_TERMINATE)
    {
      if (reply == ZMAP_SERVERRESPONSE_OK)
        thread_rc = ZMAPTHREAD_RETURNCODE_QUIT ;
      else
        thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }
  else
    {
      thread_rc = thread_RC(reply) ;
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  // I DON'T THINK WE NEED TO DO THIS AS WE ARE NOT RETURNING A SERVER...
  /* Return server. */
  *slave_data = (void *)server ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return thread_rc ;
}



/* This function is called if a thread terminates. */
static ZMapThreadReturnCode threadTerminateHandler(void **slave_data, char **err_msg_out)
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
static ZMapThreadReturnCode threadDestroyHandler(void **slave_data)
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





static ZMapThreadReturnCode terminateServer(ZMapServer *server, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServerConnectStateType connect_state = ZMAP_SERVERCONNECT_STATE_INVALID ;

  if ((zMapServerGetConnectState(*server, &connect_state) == ZMAP_SERVERRESPONSE_OK)
      && connect_state == ZMAP_SERVERCONNECT_STATE_CONNECTED
      && (zMapServerCloseConnection(*server) == ZMAP_SERVERRESPONSE_OK))
    {
      *server = NULL ;
    }
  else if (connect_state == ZMAP_SERVERCONNECT_STATE_UNCONNECTED)
    {
      *server = NULL ;
    }
  else
    {
      *err_msg_out = g_strdup(zMapServerLastErrorMsg(*server)) ;
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }

  return thread_rc ;
}



static ZMapThreadReturnCode destroyServer(ZMapServer *server)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;

  if (zMapServerFreeConnection(*server) == ZMAP_SERVERRESPONSE_OK)
    {
      *server = NULL ;
    }
  else
    {
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


