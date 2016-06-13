/*  File: zmapDataSource.cpp
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
 * Description: Interface to data source threads.
 *
 * Exported functions: See ZMap/zmapDataSource.hpp
 *
 *-------------------------------------------------------------------
 */

#include <sstream>
#include <stdexcept>

#include <ZMap/zmapServerProtocol.hpp>
#include <ZMap/zmapDataSource.hpp>

using namespace std ;




namespace ZMapDataSource
{


  static void replyFunc(void *func_data) ;



  //
  //                 External routines
  //


  // Note that for  url_ string it has to be () and not {}...I can't remember the exact reason but
  // there's some stupidity in C++ about this...it's in Scott Meyers book about C11/C14....
  DataSource::DataSource(ZMapServerReqType type, 
                         ZMapFeatureSequenceMap sequence_map, int start, int end,
                         const string &url, const string &config_file, const string &version)
    : thread_{NULL}, slave_poller_data_{NULL}, user_func_{NULL}, user_data_{NULL},
    type_{type}, response_{ZMAP_SERVERRESPONSE_INVALID}, exit_code_{0}, stderr_out_{},
    sequence_map_{sequence_map}, start_{start}, end_{end},
    url_(url), config_file_(config_file), version_(version),
    url_obj_{NULL}, protocol_{""}, format_{""}, scheme_{SCHEME_INVALID}
{
  // NEED SOME WORK ON THIS CHECKING NOW I'M MERGING THESE OBJS....

  if (!(url_.length()))
    {
      throw invalid_argument("Missing server url.") ;
    }
  else
    {
      int url_parse_error ;

      /* Parse the url and create connection. */
      if (!(url_obj_ = url_parse(url_.c_str(), &url_parse_error)))
        {
          zMapLogWarning("GUI: url %s did not parse. Parse error < %s >",
                         url_.c_str(), url_error(url_parse_error)) ;

          throw invalid_argument("Bad url.") ;
        }
    }

  if (!(thread_ = zMapThreadCreate(true)))
    {
      throw runtime_error("Cannot create thread.") ;
    }


  // Start polling, if this means we do too much polling we can have a function to start or do it
  // as part of the SendRequest....though that might induce some timing problems.
  //
  slave_poller_data_ = zMapThreadPollSlaveStart(thread_, replyFunc, this) ;


  return ;
}

  ZMapServerReqType DataSource::GetRequestType()
  {
    return type_ ;
  }


  bool DataSource::GetSequenceData(ZMapFeatureSequenceMap *sequence_map_out, int *start_out, int *end_out)
  {
    bool result = TRUE ;

    *sequence_map_out = sequence_map_ ;

    *start_out = start_ ;

    *end_out = end_ ;

    return result ;
  }


  bool DataSource::SendRequest(UserBaseCallBackFunc user_func, void *user_data)
  {
    bool result = false ;

    user_func_ = user_func ;
    user_data_ = user_data ;

    zMapThreadRequest(thread_, this) ;

    result = true ;

    return result ;
  }

  void DataSource::GetUserCallback(UserBaseCallBackFunc *user_func, void **user_data)
  {
    *user_func = user_func_ ;
    *user_data = user_data_ ;

    return ;
  }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  // The threads stuff is quite messy so this routine trys to present a cleaner interface.
  DataSourceReplyState DataSource::GetReply(void **reply_out, char **err_msg_out)
  {
    DataSourceReplyState result = DataSourceReplyState::ERROR ;
    ZMapThreadReply reply_state = ZMAPTHREAD_REPLY_INVALID ;
    void *reply = NULL ;
    char *err_msg = NULL ;

    if (!zMapThreadGetReplyWithData(thread_, &reply_state, &reply, &err_msg))
      {
        result = DataSourceReplyState::WAITING ;
      }
    else
      {
        if (reply_state == ZMAPTHREAD_REPLY_GOTDATA)
          {
            *reply_out = reply ;

            result = DataSourceReplyState::GOT_DATA ;
          }
        else if (reply_state == ZMAPTHREAD_REPLY_WAIT)
          {
            result = DataSourceReplyState::WAITING ;
          }
        else
          {
            *err_msg_out = err_msg ;

            result = DataSourceReplyState::ERROR ;
          }
      }

    return result ;
  }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  const string &DataSource::GetURL()
  {
    const string &url_ptr = url_ ;

    return url_ptr ;
  }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */






bool DataSource::GetServerInfo(char **config_file_out, ZMapURL *url_obj_out, char **version_str_out)
{
  bool result = TRUE ;

  *config_file_out = (char *)config_file_.c_str() ;

  *url_obj_out = url_obj_ ;

  *version_str_out = (char *)version_.c_str() ;

  return result ;
}



void DataSource::SetReply(ZMapServerResponseType response, gint exit_code, const char *stderr)
{
  response_ = response ;
  exit_code_ = exit_code ;
  if (stderr && *stderr)
    stderr_out_ = stderr ;

  return ;
}

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
void DataSource::Session2Str(string &session_str_out)
{
  const string unavailable_txt("<< not available yet >>") ;
  ostringstream format_str ;

  format_str << "Server\n" ;

  format_str << "\tURL: " << url_ << "\n\n" ;

  format_str << "\tFormat: " << (format_.length() ? format_ : unavailable_txt) << "\n\n" ;
			 

  switch(scheme_)
    {
    case SCHEME_ACEDB:
      {
	format_str << "\tServer: " << scheme_data_.acedb.host << "\n\n" ;
	format_str << "\tPort: " << scheme_data_.acedb.port << "\n\n" ;
	format_str << "\tDatabase: "
                   << (scheme_data_.acedb.database
                       ? scheme_data_.acedb.database : unavailable_txt) << "\n\n" ;
	break ;
      }
    case SCHEME_FILE:
      {
	format_str << "\tFile: " << scheme_data_.file.path << "\n\n" ;
	break ;
      }
    case SCHEME_PIPE:
      {
	format_str << "\tScript: " << scheme_data_.pipe.path << "\n\n" ;
	format_str << "\tQuery: " << scheme_data_.pipe.query << "\n\n" ;
	break ;
      }
    case SCHEME_ENSEMBL:
      {
        format_str << "\tServer: " << scheme_data_.ensembl.host << "\n\n" ;
        format_str << "\tPort: " << scheme_data_.ensembl.port << "\n\n" ;
        break ;
      }
    default:
      {
	format_str << "\tUnsupported server type !" ;
	break ;
      }
    }


  session_str_out = format_str.str() ;


  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



DataSource::~DataSource()
{
  // Clear up the thread.
  zMapThreadPollSlaveStop(slave_poller_data_) ;

  zMapThreadDestroy(thread_) ;
  thread_ = NULL ;

  url_free(url_obj_) ;
  url_obj_ = NULL ;


  return ;
}




  // Features subclass

  DataSourceFeatures::DataSourceFeatures(ZMapFeatureSequenceMap sequence_map, int start, int end,
                                           const string &config_file, const string &url, const string &server_version,
                                           ZMapFeatureContext context_inout, ZMapStyleTree *styles)
    : DataSource{ZMAP_SERVERREQ_FEATURES, sequence_map, start, end, config_file, url, server_version},
    styles_{styles}, context_{context_inout}
{
  if (!styles_)
    {
      throw invalid_argument("Missing styles argument to constructor.") ;
    }


  return ;
}


  bool DataSourceFeatures::SendRequest(UserFeaturesCallBackFunc user_func, void *user_data)
  {
    bool result = TRUE ;
    UserBaseCallBackFunc base_func = (UserBaseCallBackFunc)(user_func) ;

    result = DataSource::SendRequest(base_func, user_data) ;

    return result ;
  }




bool DataSourceFeatures::GetRequestData(ZMapFeatureContext *context_out, ZMapStyleTree **styles_out)
{
  bool result = TRUE ;

  *context_out = context_ ;

  *styles_out = styles_ ;

  return result ;
}



DataSourceFeatures::~DataSourceFeatures()
{
  if (context_)
    {
      // need to destroy context...
      zMapFeatureContextDestroy(context_, TRUE) ;

      context_ = NULL ;
    }

  return ;
}



  // Sequence subclass.

  DataSourceSequence::DataSourceSequence(ZMapFeatureSequenceMap sequence_map, int start, int end,
                                           const string &config_file, const string &url, const string &server_version)
    : DataSource{ZMAP_SERVERREQ_SEQUENCE, sequence_map, start, end, config_file, url, server_version}, sequences_(NULL)
{


  return ;
}


DataSourceSequence::~DataSourceSequence()
{
  if (sequences_)
    delete sequences_ ;


  return ;
}





//
//                 Package routines
//











//
//                 Internal routines
//



// NOTE, this function is not part of the class and this is deliberate. The user_callback will
// probably delete the source object so this function must not use it after it has called the
// user_callback function.
//
static void replyFunc(void *func_data)
{
  DataSource *source = static_cast<DataSource *>(func_data) ;
  DataSource::UserBaseCallBackFunc user_callback = NULL ;
  void *user_data = NULL ;

  source->GetUserCallback(&user_callback, &user_data) ;

  // source may be deleted by user_callback so do not use it after this call.
  user_callback(source, user_data) ;


  return ;
}





} // End of DataSource namespace
