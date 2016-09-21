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

#include <ZMap/zmap.hpp>

#include <iostream>
#include <sstream>
#include <stdexcept>

#include <ZMap/zmapThreadSource.hpp>
#include <ZMap/zmapServerProtocol.hpp>
#include <ZMap/zmapDataSlave.hpp>

#include <zmapDataSource_P.hpp>


using namespace std ;




namespace ZMapDataSource
{



  //
  //                 External routines
  //


  // Base DataSource class
  //

  // Note that for  url_ string it has to be () and not {}...I can't remember the exact reason but
  // there's some stupidity in C++ about this...it's in Scott Meyers book about C11/C14....
  DataSource::DataSource(DataSourceRequestType request, 
                         ZMapFeatureSequenceMap sequence_map, int start, int end,
                         const string &url, const string &config_file, const string &version)
    : reply_{DataSourceReplyType::INVALID}, state_{DataSourceState::INVALID},
    thread_(zmapDataSourceThreadRequestHandler,
            zmapDataSourceThreadTerminateHandler, zmapDataSourceThreadDestroyHandler),
    user_func_{NULL}, user_data_{NULL},
    request_{request}, 
    sequence_map_{sequence_map}, start_{start}, end_{end},
    url_(url), config_file_(config_file), version_(version),
    url_obj_{NULL}
{
  int url_parse_error ;

  if (request != DataSourceRequestType::GET_FEATURES && request != DataSourceRequestType::GET_SEQUENCE)
    {
      throw invalid_argument("Bad request type.") ;
    }
  else if (!sequence_map)
    {
      throw invalid_argument("Missing sequence map.") ;
    }
  else if (!start || !end || start > end)
    {
      throw invalid_argument("Invalid start/end.") ;
    }
  else if (!(url_.length()))
    {
      throw invalid_argument("Missing server url.") ;
    }
  else if (!(url_obj_ = url_parse(url_.c_str(), &url_parse_error)))
    {
      throw invalid_argument("Bad url.") ;
    }

  // Start polling, if this means we do too much polling we can have a function to start or do it
  // as part of the SendRequest....though that might induce some timing problems.
  //
  if (!(thread_.ThreadStart(ReplyCallbackFunc, this)))
    throw runtime_error("Could not start slave polling.") ;

  state_ = DataSourceState::INIT ;

  return ;
}


  bool DataSource::SendRequest(UserBaseCallBackFunc user_func, void *user_data)
  {
    bool result = false ;

    if (state_ == DataSourceState::INIT)
      {
        user_func_ = user_func ;
        user_data_ = user_data ;

        thread_.SendRequest(this) ;

        state_ = DataSourceState::WAITING ;

        result = true ;
      }

    return result ;
  }


  DataSourceRequestType DataSource::GetRequestType()
  {
    return request_ ;
  }


  bool DataSource::GetSequenceData(ZMapFeatureSequenceMap *sequence_map_out, int *start_out, int *end_out)
  {
    bool result = TRUE ;

    *sequence_map_out = sequence_map_ ;

    *start_out = start_ ;

    *end_out = end_ ;

    return result ;
  }

  bool DataSource::GetServerInfo(char **config_file_out, ZMapURL *url_obj_out, char **version_str_out)
  {
    bool result = TRUE ;

    *config_file_out = (char *)config_file_.c_str() ;

    *url_obj_out = url_obj_ ;

    *version_str_out = (char *)version_.c_str() ;

    return result ;
  }


  // This should never be called....does this need to be defined...is pure virtual in header...
  bool DataSource::SetError(const char *stderr)
  {
    bool result = false ;

    cout << "in the base class SetError()" << endl ;


    return result ;
  }


  // If we can't access our slave thread we unconditionally set error state with this.
  void DataSource::SetThreadError(const char *err_msg)
  {
    cout << "in the base class SetThreadError()" << endl ;

    thread_err_msg_ = err_msg ;

    DataSource::state_ = DataSourceState::GOT_ERROR ;

    return ;
  }


  // NOTES:
  //
  // This is a static class function so that it does not have a "this" pointer argument and hence
  // can be used directly as a zmapThread callback function.
  //
  // The user callback called from this function may well delete the source object
  // passed in to the function so this function should not use the object after calling
  // the user function.
  //
  // This function should ONLY be called when a reply has been received from the source thread
  // object. This is fundamental to how this all works.
  //
  // There is some subterfuge here: this function gets passed a pointer to an object that might be
  // any of the ZMapDataSource classes. This function then retrieves the users callback function from
  // the passed in object and this user callback function will be expecting to be passed an object
  // of that class, i.e. if the passed in object is of class DataSourceFeatures then the user
  // callback function will be expecting to receive an object of that class.
  //
  void DataSource::ReplyCallbackFunc(void *func_data)
  {
    DataSource *source = static_cast<DataSource *>(func_data) ;

    // Will log an error as we shouldn't be called unless we got a reply or an error.
    zMapReturnIfFail(source->state_ == DataSourceState::GOT_REPLY || source->state_ == DataSourceState::GOT_ERROR) ;

    // Check if the threadsource has failed...should get error from thread source....duh....
    if (source->thread_.HasFailed())
      {
        char *err_msg ;

        err_msg = g_strdup("thread has failed with a serious error, see log for details.") ;

        source->SetThreadError(err_msg) ;
      }

    // source may be deleted by user_callback so do not use it after this call.
    (source->user_func_)(source, source->user_data_) ;

    return ;
  }


  DataSource::~DataSource()
  {
    cout << "in base destructor" << endl ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    // ok...this is in the wrong place I think....the thread destructor will be called before the
    // thread is stopped properly.....oh bother....
    // Clear up the thread.
    thread_.ThreadStop() ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


    url_free(url_obj_) ;
    url_obj_ = NULL ;

    state_ = DataSourceState::INVALID ;

    return ;
  }




  // Features subclass
  //

  DataSourceFeatures::DataSourceFeatures(ZMapFeatureSequenceMap sequence_map, int start, int end,
                                         const string &config_file, const string &url, const string &server_version,
                                         ZMapFeatureContext context_inout, ZMapStyleTree *styles)
    : DataSource{DataSourceRequestType::GET_FEATURES, sequence_map, start, end, config_file, url, server_version},
    styles_{styles}, context_{context_inout}, err_msg_{}, exit_code_{0}
{
  if (!context_inout)
    {
      throw invalid_argument("Missing feature context argument to constructor.") ;
    }
  else if (!styles_)
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


  bool DataSourceFeatures::GetRequestParams(ZMapFeatureContext *context_out, ZMapStyleTree **styles_out)
  {
    bool result = false ;

    if (state_ != DataSourceState::GOT_ERROR && state_ != DataSourceState::FINISHED)
      {
        *context_out = context_ ;
        *styles_out = styles_ ;

        result = TRUE ;
      }

    return result ;
  }



  // Looks odd because we aren't setting anything but everything we are passed in is a pointer to
  // a struct that gets filled in, maybe we should null the struct and then accept it back....?
  bool DataSourceFeatures::SetReply(ZMapFeatureContext context, ZMapStyleTree *styles)
  {
    bool result = false ;

    if (DataSource::state_ == DataSourceState::WAITING)
      {
        context_ = context ;
        styles_ = styles ;

        DataSource::state_ = DataSourceState::GOT_REPLY ;

        result = true ;
      }

    return result ;
  }


  bool DataSourceFeatures::SetError(const char *stderr)
  {
    bool result = false ;

    // should throw if anything wrong with errmsg....

    if (DataSource::state_ == DataSourceState::WAITING)
      {
        if (stderr && *stderr)
          err_msg_ = stderr ;

        DataSource::state_ = DataSourceState::GOT_ERROR ;

        result = true ;
      }

    return result ;
  }

  bool DataSourceFeatures::SetError(gint exit_code, const char *stderr)
  {
    bool result = false ;

    cout << "in the features class SetError()" << endl ;

    if (DataSource::state_ == DataSourceState::WAITING)
      {
        exit_code_ = exit_code ;

        if (stderr && *stderr)
          err_msg_ = stderr ;

        DataSource::state_ = DataSourceState::GOT_ERROR ;

        cout << "in the features class SetError() and have set error state" << endl ;

        result = true ;
      }

    return result ;
  }

  DataSourceReplyType DataSourceFeatures::GetReply(ZMapFeatureContext *context_out, ZMapStyleTree **styles_out)
  {
    DataSourceReplyType result = DataSourceReplyType::FAILED ;

    if (DataSource::state_ == DataSourceState::GOT_REPLY)
      {
        *context_out = context_ ;
        context_ = NULL ;

        *styles_out = styles_ ;
        styles_ = NULL ;

        DataSource::state_ = DataSourceState::FINISHED ;

        result = DataSourceReplyType::GOT_DATA ;
      }
    else if (DataSource::state_ == DataSourceState::GOT_ERROR)
      {
        DataSource::state_ = DataSourceState::FINISHED ;

        result = DataSourceReplyType::GOT_ERROR ;
      }
    else if (DataSource::state_ == DataSourceState::WAITING)
      {
        result = DataSourceReplyType::WAITING ;
      }

    return result ;
  }


  bool DataSourceFeatures::GetError(const char **errmsg_out, gint *exit_code_out)
  {
    bool result = false ;

    if (DataSource::state_ == DataSourceState::GOT_ERROR)
      {
        *errmsg_out = err_msg_.c_str() ;

        *exit_code_out = exit_code_ ;

        result = true ;
      }

    return result ;
  }


  DataSourceFeatures::~DataSourceFeatures()
  {
    cout << "in feature destructor" << endl ;


    return ;
  }



  // Sequence subclass
  //

  DataSourceSequence::DataSourceSequence(ZMapFeatureSequenceMap sequence_map, int start, int end,
                                         const string &config_file, const string &url, const string &server_version,
                                         ZMapFeatureContext context_inout, ZMapStyleTree *styles)
    : DataSource{DataSourceRequestType::GET_SEQUENCE, sequence_map, start, end, config_file, url, server_version},
    styles_{styles}, context_{context_inout}, err_msg_{}, exit_code_{0}
{
  if (!context_inout)
    {
      throw invalid_argument("Missing feature context argument to constructor.") ;
    }
  else if (!styles_)
    {
      throw invalid_argument("Missing styles argument to constructor.") ;
    }

  return ;
}

  // Send seems to repeat a lot but I guess that's just coincidence.....
  bool DataSourceSequence::SendRequest(UserSequenceCallBackFunc user_func, void *user_data)
  {
    bool result = TRUE ;
    UserBaseCallBackFunc base_func = (UserBaseCallBackFunc)(user_func) ;

    result = DataSource::SendRequest(base_func, user_data) ;

    return result ;
  }


  bool DataSourceSequence::GetRequestParams(ZMapFeatureContext *context_out, ZMapStyleTree **styles_out)
  {
    bool result = false ;

    if (state_ != DataSourceState::GOT_ERROR && state_ != DataSourceState::FINISHED)
      {
        *context_out = context_ ;
        *styles_out = styles_ ;

        result = TRUE ;
      }

    return result ;
  }



  // Need to set sequence here.....check how this is done.....
  bool DataSourceSequence::SetReply(ZMapFeatureContext context, ZMapStyleTree *styles)
  {
    bool result = false ;

    if (DataSource::state_ == DataSourceState::WAITING)
      {
        context_ = context ;
        styles_ = styles ;

        DataSource::state_ = DataSourceState::GOT_REPLY ;

        result = true ;
      }

    return result ;
  }


  bool DataSourceSequence::SetError(const char *stderr)
  {
    bool result = false ;

    cout << "in the sequence class SetError()" << endl ;

    if (DataSource::state_ == DataSourceState::WAITING)
      {
        if (stderr && *stderr)
          err_msg_ = stderr ;

        DataSource::state_ = DataSourceState::GOT_ERROR ;

        cout << "in the sequence class SetError() and have set error state" << endl ;

        result = true ;
      }

    return result ;
  }


  bool DataSourceSequence::SetError(gint exit_code, const char *stderr)
  {
    bool result = false ;

    if (DataSource::state_ == DataSourceState::WAITING)
      {
        exit_code_ = exit_code ;

        if (stderr && *stderr)
          err_msg_ = stderr ;

        DataSource::state_ = DataSourceState::GOT_ERROR ;

        result = true ;
      }

    return result ;
  }


  // here we return the sequence I think ?? not the features etc.....check is stylestree is
  // altered, is it altered when we get features ??

  DataSourceReplyType DataSourceSequence::GetReply(ZMapFeatureContext *context_out, ZMapStyleTree **styles_out)
  {
    DataSourceReplyType result = DataSourceReplyType::FAILED ;

    if (DataSource::state_ == DataSourceState::GOT_REPLY)
      {
        *context_out = context_ ;
        context_ = NULL ;

        *styles_out = styles_ ;
        styles_ = NULL ;

        DataSource::state_ = DataSourceState::FINISHED ;

        result = DataSourceReplyType::GOT_DATA ;
      }
    else if (DataSource::state_ == DataSourceState::GOT_ERROR)
      {
        DataSource::state_ = DataSourceState::FINISHED ;

        result = DataSourceReplyType::GOT_ERROR ;
      }
    else if (DataSource::state_ == DataSourceState::WAITING)
      {
        result = DataSourceReplyType::WAITING ;
      }

    return result ;
  }


  bool DataSourceSequence::GetError(const char **errmsg_out, gint *exit_code_out)
  {
    bool result = false ;

    if (DataSource::state_ == DataSourceState::GOT_ERROR)
      {
        *errmsg_out = err_msg_.c_str() ;

        *exit_code_out = exit_code_ ;

        result = true ;
      }

    return result ;
  }


  DataSourceSequence::~DataSourceSequence()
  {
    cout << "in sequence destructor" << endl ;



    return ;
  }





  //
  //                 Package routines
  //





  //
  //                 Internal routines
  //




} // End of DataSource namespace
