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
 * Description: Interface to data sources, allows the application to
 *              launch a thread and get data back from a data source.
 * 
 *              See this google doc for more details:
 * 
 * https://docs.google.com/document/d/14tt5oHQDhQMmB5AW2ghBy9SEp9hFnvedreLU-dcKbsc/edit#heading=h.efixygk0ozge
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAPDATASOURCE_H
#define ZMAPDATASOURCE_H

#include <string>
#include <vector>

#include <ZMap/zmapUrl.hpp>
#include <ZMap/zmapFeature.hpp>
#include <ZMap/zmapFeatureLoadDisplay.hpp>
#include <ZMap/zmapThreadSource.hpp>


namespace ZMapDataSource
{

  // convert to our enum system and add a macro to our enum system to support this c++ type of
  // enum, i.e. needs to add "class" and do away with typedef....
  //
  enum class DataSourceRequestType {INVALID, GET_FEATURES, GET_SEQUENCE} ;

  enum class DataSourceReplyType {INVALID, FAILED, WAITING, GOT_DATA, GOT_ERROR} ;



  // Abstract base class - currently no public operations.
  class DataSource
  {
    friend class DataSlave ;


  public:


  protected:

    typedef void (*UserBaseCallBackFunc)(DataSource *data_source, void *user_data) ;

    enum class DataSourceState {INVALID, INIT, WAITING, GOT_REPLY, GOT_ERROR, FINISHED} ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    DataSource(DataSourceRequestType request, 
               ZMapFeatureSequenceMap sequence_map, int start, int end,
               const std::string &url, const std::string &config_file, const std::string &version) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    DataSource(DataSourceRequestType request, 
               ZMapFeatureSequenceMap sequence_map, int start, int end,
               ZMapConfigSource config_source) ;


    bool SendRequest(UserBaseCallBackFunc user_func, void *user_data) ;

    virtual ~DataSource() ;

    DataSourceReplyType reply_ ;

    DataSourceState state_ ;


  private:

    // No default constructor.
    DataSource() = delete ;

    // No copy operations
    DataSource(const DataSource&) = delete ;
    DataSource& operator=(const DataSource&) = delete ;

    // no move operations
    DataSource(DataSource&&) = delete ;
    DataSource& operator=(DataSource&&) = delete ;

    bool GetSequenceData(ZMapFeatureSequenceMap *sequence_map_out, int *start_out, int *end_out) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    bool GetServerInfo(char **config_file_out, ZMapURL *url_obj_out, char **version_str_out) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    bool GetServerInfo(ZMapConfigSource *config_source_out) ;


    DataSourceRequestType GetRequestType() ;

    virtual bool SetError(const char *err_msg) = 0 ;

    void SetThreadError(const char *err_msg) ;

    static void ReplyCallbackFunc(void *func_data) ;



    ZMapThreadSource::ThreadSource thread_ ;

    UserBaseCallBackFunc user_func_ ;
    void *user_data_ ;

    DataSourceRequestType request_ ;


    ZMapFeatureSequenceMap sequence_map_ ;
    int start_, end_ ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    std::string url_ ;

    std::string config_file_ ;

    std::string version_ ;

    ZMapURL url_obj_ ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    // I'm making this a pointer because we don't have the code in place to copy these objects sensibly.
    ZMapConfigSource config_source_ ;



    const char *thread_err_msg_ ;

  } ;




  // Features request class - get features back from a data source.
  //
  class DataSourceFeatures: public DataSource
  {
    friend class DataSlave ;

  public:

  typedef void (*UserFeaturesCallBackFunc)(DataSourceFeatures *data_source, void *user_data) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    DataSourceFeatures(ZMapFeatureSequenceMap sequence_map, int start, int end,
                       const std::string &url, const std::string &config_file, const std::string &server_version,
                       ZMapFeatureContext context_inout, ZMapStyleTree *styles) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    DataSourceFeatures(ZMapFeatureSequenceMap sequence_map, int start, int end,
                       ZMapConfigSource config_source,
                       ZMapFeatureContext context_inout, ZMapStyleTree *styles) ;

    bool SendRequest(UserFeaturesCallBackFunc user_func, void *user_data) ;

    DataSourceReplyType GetReply(ZMapFeatureContext *context_out, ZMapStyleTree **styles_out) ;

    bool GetError(const char **errmsg_out, gint *exit_code_out) ;

    ~DataSourceFeatures() ;


  protected:



  private:

    bool GetRequestParams(ZMapFeatureContext *context_out, ZMapStyleTree **styles_out) ;

    bool SetReply(ZMapFeatureContext context_inout, ZMapStyleTree *styles) ;

    bool SetError(const char *err_msg) ;
    bool SetError(gint exit_code, const char *stderr) ;

    UserFeaturesCallBackFunc user_func_ ;
    void *user_data_ ;

    ZMapStyleTree *styles_ ;				    /* Needed for some features to control
							       how they are fetched. */

    ZMapFeatureContext context_ ;                           /* Returned feature sets. */

    std::string err_msg_ ;
    gint exit_code_ ;


  } ;




  // Sequence request class, currently very like the features class but this is likely to
  // change.
  //
  class DataSourceSequence: public DataSource
  {
    friend class DataSlave ;

  public:

  typedef void (*UserSequenceCallBackFunc)(DataSourceSequence *data_source, void *user_data) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    DataSourceSequence(ZMapFeatureSequenceMap sequence_map, int start, int end,
                       const std::string &config_file, const std::string &url, const std::string &server_version,
                       ZMapFeatureContext context_inout, ZMapStyleTree *styles) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    DataSourceSequence(ZMapFeatureSequenceMap sequence_map, int start, int end,
                       ZMapConfigSource config_source,
                       ZMapFeatureContext context_inout, ZMapStyleTree *styles) ;


    bool SendRequest(UserSequenceCallBackFunc user_func, void *user_data) ;

    DataSourceReplyType GetReply(ZMapFeatureContext *context_out, ZMapStyleTree **styles_out) ;

    bool GetError(const char **errmsg_out, gint *exit_code_out) ;

    ~DataSourceSequence() ;


  protected:



  private:

    bool GetRequestParams(ZMapFeatureContext *context_out, ZMapStyleTree **styles_out) ;

    bool SetReply(ZMapFeatureContext context_inout, ZMapStyleTree *styles) ;

    bool SetError(const char *err_msg) ;
    bool SetError(gint exit_code, const char *stderr) ;

    UserSequenceCallBackFunc user_func_ ;
    void *user_data_ ;

    ZMapStyleTree *styles_ ;				    /* Needed for some features to control
							       how they are fetched. */

    ZMapFeatureContext context_ ;                           /* Returned feature sets. */

    std::string err_msg_ ;
    gint exit_code_ ;

  } ;




} /* ZMapDataSource namespace */


#endif /* ZMAPDATASOURCE_H */
