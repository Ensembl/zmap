/*  File: zmapDataSlave.hpp
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *      Gemma Barson (Sanger Institute, UK) gb10@sanger.ac.uk
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
  bool GetServerInfo(DataSource &source,
                     char **config_file_out, ZMapURL *url_obj_out, char **version_str_out) ;
  DataSourceRequestType GetRequestType(DataSource &source) ;
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
