/*  File: zmapDataSlave.cpp
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
 * Description: Interface that slave code should use to access the
 *              data source object. Using this "helper" class means that
 *              many functions can be made private in the DataSource
 *              class and thus simplifies the interface presented to
 *              the application.
 *
 *              All the functions in this class are essentially
 *              "forwarding" functions to call private DataSource
 *              members.
 *
 * Exported functions: See zmapDataSlave.hpp
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapDataSlave.hpp>

using namespace ZMapDataSource ;




namespace ZMapDataSource
{



  //
  //                   External interface.
  //



  bool DataSlave::GetSequenceData(DataSource &source,
                                  ZMapFeatureSequenceMap *sequence_map_out, int *start_out, int *end_out)
  {
    bool result = false ;

    result = source.GetSequenceData(sequence_map_out, start_out, end_out) ;

    return result ;
  }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  bool DataSlave::GetServerInfo(DataSource &source,
                                char **config_file_out, ZMapURL *url_obj_out, char **version_str_out)
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    bool DataSlave::GetServerInfo(DataSource &source,
                                  ZMapConfigSource *config_source_out)
  {
    bool result = false ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    result = source.GetServerInfo(config_file_out, url_obj_out, version_str_out) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    result = source.GetServerInfo(config_source_out) ;

    return result ;
  }


  DataSourceRequest DataSlave::GetRequestType(DataSource &source)
  {
    return source.request_ ;
  }


  bool DataSlave::SetError(DataSource &source_object, const char *err_msg)
  {
    bool result = false ;

    result = source_object.SetError(err_msg) ;

    return result ;
  }


  bool DataSlave::GetRequestParams(DataSourceFeatures &features_class,
                                   ZMapFeatureContext *context_out, ZMapStyleTree **styles_out)
  {
    bool result = false ;

    result = features_class.GetRequestParams(context_out, styles_out) ;

    return result ;
  }


  bool DataSlave::SetReply(DataSourceFeatures &features_class,
                           ZMapFeatureContext context, ZMapStyleTree *styles)
  {
    bool result = false ;

    result = features_class.SetReply(context, styles) ;

    return result ;
  }

  bool DataSlave::SetError(DataSourceFeatures &features_class, gint exit_code, const char *stderr)
  {
    bool result = false ;

    result = features_class.SetError(exit_code, stderr) ;

    return result ;
  }



  bool DataSlave::GetRequestParams(DataSourceSequence &sequence_class,
                                   ZMapFeatureContext *context_out, ZMapStyleTree **styles_out)
  {
    bool result = false ;

    result = sequence_class.GetRequestParams(context_out, styles_out) ;

    return result ;
  }

  bool DataSlave::SetReply(DataSourceSequence &sequence_class,
                           ZMapFeatureContext context, ZMapStyleTree *styles)
  {
    bool result = false ;

    result = sequence_class.SetReply(context, styles) ;

    return result ;
  }

  bool DataSlave::SetError(DataSourceSequence &sequence_class, gint exit_code, const char *stderr)
  {
    bool result = false ;

    result = sequence_class.SetError(exit_code, stderr) ;

    return result ;
  }




} // End of DataSource namespace
