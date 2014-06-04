/*  File: zmapDataSource.c
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2014: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description: Code for concrete data sources. At the moment, two options
 * only, the GIO channel or HTS file.
 *
 * Exported functions:
 *-------------------------------------------------------------------
 */
#include <ZMap/zmap.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>
#include <ZMap/zmapGFF.h>
#include <ZMap/zmapServerProtocol.h>
#include <zmapDataSource_P.h>

static gboolean read_line_gio(GIOChannel * const gio_channel,  GString * const str ) ;
static gboolean read_line_hts(htsFile * const hts_file, GString * const str ) ;


/*
 * Create a ZMapDataSource object
 */
ZMapDataSource zMapDataSourceCreate(const char * const file_name )
{
  static const char * open_mode = "r" ;
  ZMapDataSource data_source = NULL ;
  ZMapDataSourceType source_type = ZMAPDATASOURCE_TYPE_UNK ;
  zMapReturnValIfFail(file_name && *file_name, data_source ) ;

  source_type = zMapDataSourceTypeFromFilename(file_name) ;
  if (source_type == ZMAPDATASOURCE_TYPE_GIO)
    {
      GError *error = NULL ;
      ZMapDataSourceGIO file = NULL ;
      file = (ZMapDataSourceGIO) g_new0(ZMapDataSourceGIOStruct, 1) ;
      if (file != NULL)
        {
          file->type = ZMAPDATASOURCE_TYPE_GIO ;
          file->io_channel = g_io_channel_new_file(file_name, open_mode, &error) ;
          if (file->io_channel != NULL && !error)
            {
              data_source = (ZMapDataSource) file ;
            }
          else
            {
              if (error)
                g_error_free(error) ;
              g_free(file) ;
            }
        }
    }
  else if (source_type == ZMAPDATASOURCE_TYPE_HTS)
    {
      ZMapDataSourceHTSFile file = NULL ;
      file = (ZMapDataSourceHTSFile) g_new0(ZMapDataSourceHTSFileStruct, 1) ;
      if (file != NULL)
        {
          file->type = ZMAPDATASOURCE_TYPE_HTS ;
          file->hts_file = hts_open(file_name, open_mode);
          if (file->hts_file != NULL)
            {
              data_source = (ZMapDataSource) file ;
            }
          else
            {
              g_free(file) ;
            }
        }
    }

  return data_source ;
}


ZMapDataSource zMapDataSourceCreateFromGIO(GIOChannel * const io_channel)
{
  ZMapDataSource data_source = NULL ;
  zMapReturnValIfFail(io_channel, data_source) ;

  ZMapDataSourceGIO file = NULL ;
  file = (ZMapDataSourceGIO) g_new0(ZMapDataSourceGIOStruct, 1) ;
  if (file != NULL)
    {
      file->type = ZMAPDATASOURCE_TYPE_GIO ;
      file->io_channel = io_channel ;
      if (file->io_channel != NULL)
        {
          data_source = (ZMapDataSource) file ;
        }
      else
        {
          g_free(file) ;
        }
    }

  return data_source ;
}

/*
 * Checks that the data source is open.
 */
gboolean zMapDataSourceIsOpen(ZMapDataSource const source)
{
  gboolean result = FALSE ;
  if (source->type == ZMAPDATASOURCE_TYPE_GIO)
    {
      GIOChannel *io_channel = ((ZMapDataSourceGIO) source)->io_channel ;
      if (io_channel)
        {
          result = TRUE ;
        }
    }
  else if (source->type == ZMAPDATASOURCE_TYPE_HTS)
    {
      htsFile *hts_file = ((ZMapDataSourceHTSFile) source)->hts_file ;
      if (hts_file)
        {
          result = TRUE ;
        }
    }

  return result ;
}


/*
 * Destroy the file object.
 */
gboolean zMapDataSourceDestroy( ZMapDataSource *p_data_source )
{
  gboolean result = FALSE ;
  zMapReturnValIfFail(p_data_source && *p_data_source , result ) ;
  ZMapDataSource data_source = *p_data_source ;

  if (data_source->type == ZMAPDATASOURCE_TYPE_GIO)
    {
      GIOStatus gio_status = G_IO_STATUS_NORMAL ;
      GError *err = NULL ;
      ZMapDataSourceGIO file = (ZMapDataSourceGIO) data_source ;
      gio_status = g_io_channel_shutdown( file->io_channel , FALSE, &err) ;
      if (gio_status != G_IO_STATUS_ERROR && gio_status != G_IO_STATUS_AGAIN)
        {
          g_io_channel_unref( file->io_channel ) ;
          file->io_channel = NULL ;
          result = TRUE ;
        }
      else
        {
          zMapLogCritical("Could not close GIOChannel in zMapDataSourceDestroy(), %s", "") ;
        }
    }
  else if (data_source->type == ZMAPDATASOURCE_TYPE_HTS)
    {
      ZMapDataSourceHTSFile file = (ZMapDataSourceHTSFile) data_source ;
      void * hts_file = file->hts_file ;
      /*
       * close hts_file
       * set result = TRUE (if it worked that is...)
       */
      result = TRUE ;
    }

  /*
   * If we suceeded in closing the stored
   * concrete file type then make sure that the
   * caller's pointer is set to NULL.
   */
  if (result)
    {
      g_free(data_source) ;
      *p_data_source = NULL ;
    }

  return result ;
}

/*
 * Return the type of the object
 */
ZMapDataSourceType zMapDataSourceGetType(ZMapDataSource data_source )
{
  zMapReturnValIfFail(data_source, ZMAPDATASOURCE_TYPE_UNK ) ;
  return data_source->type ;
}


/*
 * Read a line from the GIO channel and remove the terminating
 * newline if present. That is,
 * "<string_data>\n" -> "<string_data>"
 * (It is still a NULL terminated c-string though.)
 */
static gboolean read_line_gio(GIOChannel * const pChannel,  GString * const str )
{
  gboolean result = FALSE ;
  GError *pErr = NULL ;
  gsize pos = 0 ;
  GIOStatus cIOStatus = G_IO_STATUS_NORMAL;

  cIOStatus = g_io_channel_read_line_string(pChannel, str, &pos, &pErr) ;
  if (cIOStatus == G_IO_STATUS_NORMAL && !pErr )
    {
      result = TRUE ;
      str->str[pos] = '\0';
    }

  return result ;
}

/*
 * Read a record from a HTS file and turn it into GFFv3.
 */
static gboolean read_line_hts(htsFile * const hts_file, GString * const str )
{
  gboolean result = FALSE ;

  return result ;
}


/*
 * Read one line and return as string.
 *
 * (a) GIO reads a line of GFF which is of the form
 *                   "<fields>...<fields>\n"
 *     so we also have to remove the terminating newline.
 *
 * (b) HTSFile has to read a HTS record and then convert
 *     that somehow to GFF. TBD.
 *
 */
gboolean zMapDataSourceReadLine (ZMapDataSource const data_source , GString * const str )
{
  gboolean result = FALSE ;
  zMapReturnValIfFail(data_source && (data_source->type != ZMAPDATASOURCE_TYPE_UNK) && str, result ) ;
  if (data_source->type == ZMAPDATASOURCE_TYPE_GIO)
    {
      ZMapDataSourceGIO file = (ZMapDataSourceGIO) data_source ;
      result = read_line_gio(file->io_channel, str ) ;
    }
  else if (data_source->type == ZMAPDATASOURCE_TYPE_HTS)
    {
      ZMapDataSourceHTSFile file = (ZMapDataSourceHTSFile) data_source ;
      result = read_line_hts(file->hts_file, str) ;
    }

  return result ;
}

/*
 * Get GFF version from the data source.
 */
gboolean zMapDataSourceGetGFFVersion(ZMapDataSource const data_source, int * const p_out_val)
{
  int out_val = ZMAPGFF_VERSION_UNKNOWN ;
  gboolean result = FALSE ;
  zMapReturnValIfFail(data_source && (data_source->type != ZMAPDATASOURCE_TYPE_UNK) && p_out_val, result ) ;

  /*
   * We treat two cases. The HTS case cannot fail since we are converting
   * HTS record data to GFF later on as it is read.
   */
  if (data_source->type == ZMAPDATASOURCE_TYPE_GIO)
    {
      ZMapDataSourceGIO file = (ZMapDataSourceGIO) data_source ;
      GString *pString = g_string_new(NULL) ;
      GIOStatus cIOStatus = G_IO_STATUS_NORMAL ;
      GError *pError = NULL ;
      result = zMapGFFGetVersionFromGIO(file->io_channel, pString,
                                        &out_val, &cIOStatus, &pError) ;
      *p_out_val = out_val ;
      if ( !result || (cIOStatus != G_IO_STATUS_NORMAL)  || pError ||
            ((out_val != ZMAPGFF_VERSION_2) && (out_val != ZMAPGFF_VERSION_3)) )
        {
          zMapLogCritical("Could not obtain GFF version from GIOChannel in zMapDataSourceGetGFFVersion(), %s", "") ;
          /* This is set to make sure that the calling program notices the error. */
          result = FALSE ;
        }
      g_string_free(pString, TRUE) ;
      if (pError)
        g_error_free(pError) ;
    }
  else if (data_source->type == ZMAPDATASOURCE_TYPE_HTS)
    {
      *p_out_val = ZMAPGFF_VERSION_3 ;
      result = TRUE ;
    }

  return result ;
}

/*
 * Inspect the filename string (might include the path on the front, but this is
 * (ignored) for the extension to determine type:
 *
 *       *.gff                            ZMAPDATASOURCE_TYPE_GIO
 *       *.[sam,bam,cram]                 ZMAPDATASOURCE_TYPE_HTS
 *       *.<everything_else>              ZMAPDATASOURCE_TYPE_UNK
 */
ZMapDataSourceType zMapDataSourceTypeFromFilename(const char * const file_name )
{
  static const char
    dot = '.',
    *ext01 = "gff",
    *ext02 = "sam",
    *ext03 = "bam",
    *ext04 = "cram" ;
  ZMapDataSourceType type = ZMAPDATASOURCE_TYPE_UNK ;
  char * pos = NULL, *tmp = (char*) file_name ;
  zMapReturnValIfFail( file_name && *file_name, type ) ;

  /*
   * Find last occurrance of 'dot' to get
   * file extension.
   */
  while ((tmp = strchr(tmp, dot)))
    {
      ++tmp ;
      pos = tmp ;
    }

  /*
   * Now inspect the file extension.
   */
  if (pos)
    {
      if (!g_ascii_strcasecmp(pos, ext01))
        {
          type = ZMAPDATASOURCE_TYPE_GIO ;
        }
      else if (!g_ascii_strcasecmp(pos, ext02))
        {
          type = ZMAPDATASOURCE_TYPE_HTS ;
        }
      else if (!g_ascii_strcasecmp(pos, ext03))
        {
          type = ZMAPDATASOURCE_TYPE_HTS ;
        }
      else if (!g_ascii_strcasecmp(pos, ext04))
        {
          type = ZMAPDATASOURCE_TYPE_HTS ;
        }
    }

  return type ;
}

/*
 * Convert a HTS record to GFF3 line.
 */
char * zMapHTSRecord2GFF3(const void * const hts_record)
{
  char * gff_line = NULL ;

  /*
   * Construct GFFv3 name from HTS record
   */

  return gff_line ;
}
