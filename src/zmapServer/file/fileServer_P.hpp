/*  File: fileServer_P.h
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
 * Description: Defines/types etc. for the file accessing version
 *              of the server code.
 *
 *-------------------------------------------------------------------
 */
#ifndef FILE_SERVER_P_H
#define FILE_SERVER_P_H

#include <ZMap/zmapDataStream.hpp>

#include <zmapServerPrototype.hpp>
#include <zmapDataStream_P.hpp>

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <zmapDataSource_P.hpp>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/*
 * File server connection.
 */
typedef struct FileServerStruct_
{
  ZMapURLScheme scheme ;
  ZMapDataStream data_stream ;
  ZMapServerResponseType result ;
  ZMapFeatureContext req_context ;

  ZMapConfigSource config_source ;    /* The source the server will process */
  char *config_file ;
  char *url ;                          /* Full url string. */
  char *path ;                         /* Filename out of the URL  */
  char *data_dir ;                     /* default location for data files (using file://)) */
  char *last_err_msg ;
  char *styles_file ;

  GQuark req_sequence;
  int gff_version, zmap_start, zmap_end, exit_code ;

  gboolean sequence_server, error ;
  GHashTable *source_2_sourcedata ;
  GHashTable *featureset_2_column ;

} FileServerStruct, *FileServer ;



#endif /* !FILE_SERVER_P_H */
