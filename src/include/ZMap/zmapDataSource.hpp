/*  File: zmapDataSource.hpp
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk,
 *      Gemma Barson (Sanger Institute, UK) gb10@sanger.ac.uk
 *
 * Description: Interface to data sources.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAPDATASOURCE_H
#define ZMAPDATASOURCE_H

#include <string>
#include <vector>

#include <ZMap/zmapServerProtocol.hpp>
#include <ZMap/zmapThreads.hpp>




namespace ZMapDataSource
{


  enum class DataSourceReplyState {INVALID, WAITING, GOT_DATA, ERROR} ;


  // Abstract base class.
  class DataSource
  {
  public:

  typedef void (*UserBaseCallBackFunc)(DataSource *data_source, void *user_data) ;

    // No default constructor.
    DataSource() = delete ;

    // No copy operations
    DataSource(const DataSource&) = delete ;
    DataSource& operator=(const DataSource&) = delete ;

    // no move operations
    DataSource(DataSource&&) = delete ;
    DataSource& operator=(DataSource&&) = delete ;

    ZMapServerReqType GetRequestType() ;

    bool GetSequenceData(ZMapFeatureSequenceMap *sequence_map_out, int *start_out, int *end_out) ;

    bool GetServerInfo(char **config_file_out, ZMapURL *url_obj_out, char **version_str_out) ;

    void GetUserCallback(UserBaseCallBackFunc *user_func, void **user_data) ;

    void SetReply(ZMapServerResponseType response, gint exit_code, const char *stderr) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    DataSourceReplyState GetReply(void **reply_out, char **err_msg_out) ;



    const std::string &GetConfigFile() ;

    void Session2Str(std::string &session_str_out) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


    ~DataSource() ;


  protected:

    DataSource(ZMapServerReqType type, 
               ZMapFeatureSequenceMap sequence_map, int start, int end,
               const std::string &url, const std::string &config_file, const std::string &version) ;

    bool SendRequest(UserBaseCallBackFunc user_func, void *user_data) ;


  private:


    ZMapThread thread_ ;
    ZMapThreadPollSlaveCallbackData slave_poller_data_ ;

    UserBaseCallBackFunc user_func_ ;
    void *user_data_ ;

    ZMapServerReqType type_ ;
    ZMapServerResponseType response_ ;
    gint exit_code_ ;
    std::string stderr_out_ ;

    ZMapFeatureSequenceMap sequence_map_ ;
    int start_, end_ ;

    std::string url_ ;

    std::string config_file_ ;

    std::string version_ ;

    ZMapURL url_obj_ ;

    std::string protocol_ ;

    std::string format_ ;


    // not sure if this is all needed ??
    ZMapURLScheme scheme_ ;

    // Connection data, keyed by scheme field.
    union							    /* Keyed by scheme field above. */
    {
      struct
      {
        char *host ;
        int port ;
        char *database ;
      } acedb ;

      struct
      {
        char *path ;
      } file ;

      struct
      {
        char *path ;
        char *query ;
      } pipe ;

      struct
      {
        char *host ;
        int port ;
      } ensembl ;
    } scheme_data_ ;


  } ;


  // Features request class.
  //
  class DataSourceFeatures: public DataSource
  {
  public:

  typedef void (*UserFeaturesCallBackFunc)(DataSourceFeatures *data_source, void *user_data) ;

    DataSourceFeatures(ZMapFeatureSequenceMap sequence_map, int start, int end,
                       const std::string &url, const std::string &config_file, const std::string &server_version,
                       ZMapFeatureContext context_inout, ZMapStyleTree *styles) ;

    bool SendRequest(UserFeaturesCallBackFunc user_func, void *user_data) ;
    
    bool GetRequestData(ZMapFeatureContext *context_out, ZMapStyleTree **styles_out) ;

    ~DataSourceFeatures() ;


  protected:



  private:

    UserFeaturesCallBackFunc user_func_ ;
    void *user_data_ ;

    ZMapStyleTree *styles_ ;				    /* Needed for some features to control
							       how they are fetched. */

    ZMapFeatureContext context_ ;                           /* Returned feature sets. */

  } ;


  // Sequence request class.
  //
  class DataSourceSequence: public DataSource
  {
  public:

  typedef void (*UserSequenceCallBackFunc)(DataSourceSequence *data_source, void *user_data) ;

    DataSourceSequence(ZMapFeatureSequenceMap sequence_map, int start, int end,
                       const std::string &config_file, const std::string &url, const std::string &server_version) ;

    bool SendRequest(UserSequenceCallBackFunc user_func, void *user_data) ;

    ~DataSourceSequence() ;


  protected:



  private:

    UserSequenceCallBackFunc user_func_ ;
    void *user_data_ ;

    std::vector <ZMapSequenceStruct> *sequences_ ;

  } ;




} /* ZMapDataSource namespace */


#endif /* ZMAPDATASOURCE_H */
