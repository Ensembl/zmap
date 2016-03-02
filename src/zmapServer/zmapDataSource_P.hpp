/*  File: zmapDataSource_P.h
 *  Author: Steve Miller (sm23@sanger.ac.uk)
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
 * and was written by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description:
 * Exported functions:
 *-------------------------------------------------------------------
 */

#ifndef DATA_SOURCE_P_H
#define DATA_SOURCE_P_H


#include <zmapDataSource.hpp>


#ifdef USE_HTSLIB

/*
 * HTS header. Temporary location.
 */
/* #include "/nfs/users/nfs_s/sm23/Work/htslib-develop/htslib/hts.h"
#include "/nfs/users/nfs_s/sm23/Work/htslib-develop/htslib/sam.h" */
#include <htslib/hts.h>
#include <htslib/sam.h>

#endif



/*
 *  Full declarations of concrete types to represent the GIO channel and
 *  HTS file data sources.
 */
typedef struct ZMapDataSourceGIOStruct_
  {
    ZMapDataSourceType type ;
    GIOChannel *io_channel ;
  } ZMapDataSourceGIOStruct , *ZMapDataSourceGIO  ;


#ifdef USE_HTSLIB

typedef struct ZMapDataSourceHTSFileStruct_
  {
    ZMapDataSourceType type ;
    htsFile *hts_file ;
    /* bam header and record object */
    bam_hdr_t *hts_hdr ;
    bam1_t *hts_rec ;

    /*
     * Data that we need to store
     */
    char * sequence,
         * source,
         * so_type ;
  } ZMapDataSourceHTSFileStruct , *ZMapDataSourceHTSFile;

#endif



/*
 * This is for temporary data that need to be stored while
 * records are being read from a file. At the moment this is used
 * only for HTS data sources, since we need a struct allocated to
 * read records into.
 */
typedef struct ZMapDataSourceTempStorageStruct_
  {
    ZMapDataSourceType type ;
  } ZMapDataSourceTempStorageStruct, *ZMapDataSourceTempStorage ;

typedef struct ZMapDataSourceTempStorageStructGIO_
  {
    ZMapDataSourceType type ;
  } ZMapDataSourceTempStorageStructGIO, *ZMapDataSourceTempStorageGIO ;

typedef struct ZMapDataSourceTempStorageStructHTS_
  {
    ZMapDataSourceType type ;
  } ZMapDataSourceTempStorageStructHTS, *ZMapDataSourceTempStorageHTS ;


#endif
