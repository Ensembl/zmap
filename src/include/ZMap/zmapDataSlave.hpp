/*  File: zmapDataSlave.hpp
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
 * Description: Interface that thread slave code uses to access the
 *              DataSource object to set features, errors etc.
 * 
 *              Currently this class is only aimed at the slave thread
 *              and so is restricted in what it can do. It is a kind
 *              a kind of facade to the DataSource object allowing
 *              getting/setting of that object without needing direct
 *              access to that object.
 *
 *              See this google doc for more details:
 * 
 * https://docs.google.com/document/d/14tt5oHQDhQMmB5AW2ghBy9SEp9hFnvedreLU-dcKbsc/edit#heading=h.efixygk0ozge
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAPDATASLAVE_H
#define ZMAPDATASLAVE_H


#include <ZMap/zmapDataSource.hpp>


namespace ZMapDataSource
{


class DataSlave
{
  public:


  // base object ops.
  bool GetSequenceData(DataSource &source,
                       ZMapFeatureSequenceMap *sequence_map_out, int *start_out, int *end_out) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  bool GetServerInfo(DataSource &source,
                     char **config_file_out, ZMapURL *url_obj_out, char **version_str_out) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  bool GetServerInfo(DataSource &source,
                     ZMapConfigSource *config_source_out) ;


  DataSourceRequest GetRequestType(DataSource &source) ;
  bool SetError(DataSource &source_object, const char *err_msg) ;


  // features object ops.
  bool GetRequestParams(DataSourceFeatures &features_object,
                        ZMapFeatureContext *context_out, ZMapStyleTree **styles_out) ;
  bool SetReply(DataSourceFeatures &features_object, ZMapFeatureContext context, ZMapStyleTree *styles) ;
  bool SetError(DataSourceFeatures &features_object, gint exit_code, const char *stderr) ;



  // sequence object ops.
  bool GetRequestParams(DataSourceSequence &sequence_object,
                        ZMapFeatureContext *context_out, ZMapStyleTree **styles_out) ;
  bool SetReply(DataSourceSequence &sequence_object, ZMapFeatureContext context, ZMapStyleTree *styles) ;
  bool SetError(DataSourceSequence &sequence_object, gint exit_code, const char *stderr) ;


  protected:

  private:

} ;


} /* ZMapDataSource namespace */


#endif /* ZMAPDATASLAVE_H */
