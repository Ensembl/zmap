/*  File: fileServer_P.h
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
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Defines/types etc. for the file accessing version
 *              of the server code.
 *
 *-------------------------------------------------------------------
 */
#ifndef FILE_SERVER_P_H
#define FILE_SERVER_P_H

#include <zmapServerPrototype.hpp>

#include <ZMap/zmapDataSource.hpp>
#include <zmapDataSource_P.hpp>

/*
 * File server connection.
 */
typedef struct FileServerStruct_
{
  ZMapURLScheme scheme ;
  ZMapDataSource data_source ;
  ZMapServerResponseType result ;
  ZMapFeatureContext req_context ;
  ZMapFeatureSequenceMap sequence_map ;

  GQuark source_name ;
  char *config_file ;
  char *url ;                          /* Full url string. */
  char *path ;                         /* Filename out of the URL  */
  char *data_dir ;                     /* default location for data files (using file://)) */
  char *last_err_msg ;
  char *styles_file ;

  GQuark req_sequence;
  int gff_version, zmap_start, zmap_end, exit_code ;

  gboolean sequence_server, is_otter, error ;
  GHashTable *source_2_sourcedata ;
  GHashTable *featureset_2_column ;

} FileServerStruct, *FileServer ;



#endif /* !FILE_SERVER_P_H */
