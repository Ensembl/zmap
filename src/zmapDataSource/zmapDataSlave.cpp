/*  File: zmapDataSlave.cpp
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


  DataSourceRequestType DataSlave::GetRequestType(DataSource &source)
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
