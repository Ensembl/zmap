/*  File: zmapDataSource.c
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
#include <ZMap/zmap.hpp>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapConfigIni.hpp>
#include <ZMap/zmapConfigStrings.hpp>
#include <ZMap/zmapGFF.hpp>
#include <ZMap/zmapServerProtocol.hpp>
#include <zmapDataSource_P.hpp>

static gboolean read_line_gio(GIOChannel * const gio_channel,  GString * const str ) ;
static gboolean read_line_hts(ZMapDataSourceHTSFile const hts_file, GString * const str ) ;

/*
 * The SO terms that contain the string "read" are as follows:
 *
 * read_pair
 * read
 * contig_read
 * five_prime_open_reading_frame
 * gene_with_stop_codon_read_through
 * reading_frame
 * blocked_reading_frame
 * stop_codon_read_through
 * dye_terminator_read
 * pyrosequenced_read
 * ligation_based_read
 * polymerase_synthesis_read
 * mRNA_read
 * genomic_DNA_read
 * BAC_read_contig
 * mitochondrial_DNA_read
 * chloroplast_DNA_read
 *
 * so I have chosen the more general parent term "read" (SO:0000150) to
 * be found at:
 *
 * http://www.sequenceontology.org/browser/current_svn/term/SO:0000150
 *
 */
static const char * const ZMAP_BAM_SO_TERM  = "read" ;
static const char * const ZMAP_BAM_SOURCE   = "zmap_bam2gff_conversion" ;
#define ZMAP_CIGARSTRING_MAXLENGTH 2048

/*
 * Create a ZMapDataSource object
 */
ZMapDataSource zMapDataSourceCreate(const char * const file_name, GError **error_out)
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
                g_propagate_error(error, error_out) ;
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

          if (file->hts_file)
            file->hts_hdr = sam_hdr_read(file->hts_file) ;

          file->hts_rec = bam_init1() ;
          if (file->hts_file && file->hts_hdr && file->hts_rec)
            {
              file->sequence = NULL ;
              file->source = g_strdup(ZMAP_BAM_SOURCE) ;
              file->so_type = g_strdup(ZMAP_BAM_SO_TERM) ;
              data_source = (ZMapDataSource) file ;
            }
          else
            {
              if (file->hts_file)
                hts_close(file->hts_file) ;
              if (file->hts_hdr)
                bam_hdr_destroy(file->hts_hdr) ;
              if (file->hts_rec)
                bam_destroy1(file->hts_rec) ;
              g_free(file) ;

              g_set_error(error, g_quark_from_string("ZMap"), 99, "Failed to open file '%s'", file_name) ;
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
  if (source && source->type == ZMAPDATASOURCE_TYPE_GIO)
    {
      GIOChannel *io_channel = ((ZMapDataSourceGIO) source)->io_channel ;
      if (io_channel)
        {
          result = TRUE ;
        }
    }
  else if (source && source->type == ZMAPDATASOURCE_TYPE_HTS)
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
          g_free(file) ;
        }
      else
        {
          zMapLogCritical("Could not close GIOChannel in zMapDataSourceDestroy(), %s", "") ;
        }
    }
  else if (data_source->type == ZMAPDATASOURCE_TYPE_HTS)
    {
      ZMapDataSourceHTSFile file = (ZMapDataSourceHTSFile) data_source ;
      if (file)
        {
          if (file->hts_file)
            hts_close(file->hts_file) ;
          if (file->hts_rec)
            bam_destroy1(file->hts_rec) ;
          if (file->hts_hdr)
            bam_hdr_destroy(file->hts_hdr) ;
          if (file->sequence)
            g_free(file->sequence) ;
          if (file->source)
            g_free(file->source) ;
          if (file->so_type)
            g_free(file->so_type) ;
          g_free(file) ;
          result = TRUE ;
        }
    }

  /*
   * If we suceeded in closing the stored
   * concrete file type then make sure that the
   * caller's pointer is set to NULL.
   */
  if (result)
    {
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
static gboolean read_line_hts(ZMapDataSourceHTSFile const hts_file, GString * const pStr )
{
  static const gssize string_start = 0 ;
  static const char *sFormatName = "Name=%s;",
    *sFormatCigar = "cigar_bam=%s;",
    *sFormatTarget = "Target=%s;";
  gboolean result = FALSE,
    bHasTargetStrand = FALSE,
    bHasNameAttribute = FALSE,
    bHasCigarAttribute = FALSE,
    bHasTargetAttribute = FALSE ;
  char *sGFFLine = NULL ;
  char cStrand = '\0',
    cPhase = '.',
    cTargetStrand = '.' ;
  int iStart = 0,
    iEnd = 0,
    iCigar = 0,
    nCigar = 0,
    iTargetStart = 0,
    iTargetEnd = 0;
  uint32_t *pCigar = NULL ;
  double dScore = 0.0 ;
  GString *pStringCigar = NULL,
    *pStringTarget = NULL,
    *pStringAttributes = NULL ;

  /*
   * Initial error check.
   */
  zMapReturnValIfFail(hts_file && hts_file->hts_file && hts_file->hts_hdr && hts_file->hts_rec, result ) ;

  /*
   * Erase current buffer contents
   */
  g_string_erase(pStr, string_start, -1) ;

  /*
   * Read line from HTS file and convert to GFF.
   */
  if ( sam_read1(hts_file->hts_file, hts_file->hts_hdr, hts_file->hts_rec) >= 0 )
    {
      /*
       * start, end, score and strand
       */
      iStart = hts_file->hts_rec->core.pos+1;
      iEnd   = iStart+hts_file->hts_rec->core.l_qseq-1;
      dScore = (double) hts_file->hts_rec->core.qual ;
      cStrand = '+' ;

      /*
       * "cigar_bam" attribute
       */
      nCigar = hts_file->hts_rec->core.n_cigar ;
      if (nCigar)
        {
          bHasCigarAttribute = TRUE ;
          pCigar = bam_get_cigar(hts_file->hts_rec) ;
          pStringCigar = g_string_new(NULL) ;
          for (iCigar=0; iCigar<nCigar; ++iCigar)
              g_string_append_printf(pStringCigar, "%i%c", bam_cigar_oplen(pCigar[iCigar]), bam_cigar_opchr(pCigar[iCigar])) ;
        }

      /*
       * "Target" (and "Name") attribute
       */
      pStringTarget = g_string_new(NULL) ;
      iTargetStart = 1 ;
      iTargetEnd = hts_file->hts_rec->core.l_qseq ;
      if (strlen(bam_get_qname(hts_file->hts_rec)))
        {
          bHasTargetAttribute = TRUE ;
          bHasNameAttribute = TRUE ;
          g_string_append_printf(pStringTarget, "%s %i %i", bam_get_qname(hts_file->hts_rec), iTargetStart, iTargetEnd ) ;
          bHasTargetStrand = TRUE ;
          cTargetStrand = '+' ;
          if (bHasTargetStrand)
            g_string_append_printf(pStringTarget, " %c", cTargetStrand) ;
        }

      /*
       * Construct attributes string.
       */
      pStringAttributes = g_string_new(NULL) ;
      if (bHasNameAttribute)
        {
          g_string_append_printf(pStringAttributes,
                                 sFormatName, bam_get_qname(hts_file->hts_rec) ) ;
        }
      if (bHasTargetAttribute)
        {
          g_string_append_printf(pStringAttributes,
                                 sFormatTarget, pStringTarget->str ) ;
        }
      if (bHasCigarAttribute)
        {
          g_string_append_printf(pStringAttributes,
                                 sFormatCigar, pStringCigar->str) ;
        }

      /*
       * Construct GFF line.
       */
      sGFFLine = g_strdup_printf("%s\t%s\t%s\t%i\t%i\t%f\t%c\t%c\t%s",
                                 hts_file->sequence,
                                 hts_file->source,
                                 hts_file->so_type,
                                 iStart, iEnd,
                                 dScore,
                                 cStrand,
                                 cPhase,
                                 pStringAttributes->str) ;
      g_string_insert(pStr, string_start, sGFFLine ) ;

      /*
       * Clean up on finish
       */
      if (sGFFLine)
        g_free(sGFFLine) ;
      if (pStringCigar)
        g_string_free(pStringCigar, TRUE) ;
      if (pStringTarget)
        g_string_free(pStringTarget, TRUE) ;
      if (pStringAttributes)
        g_string_free(pStringAttributes, TRUE) ;

      /*
       * return value
       */
      result = TRUE ;
    }

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
 *     that to GFF.
 *
 */
gboolean zMapDataSourceReadLine (ZMapDataSource const data_source , GString * const pStr )
{
  gboolean result = FALSE ;
  zMapReturnValIfFail(data_source && (data_source->type != ZMAPDATASOURCE_TYPE_UNK) && pStr, result ) ;
  if (data_source->type == ZMAPDATASOURCE_TYPE_GIO)
    {
      ZMapDataSourceGIO file = (ZMapDataSourceGIO) data_source ;
      result = read_line_gio(file->io_channel, pStr ) ;
    }
  else if (data_source->type == ZMAPDATASOURCE_TYPE_HTS)
    {
      ZMapDataSourceHTSFile file = (ZMapDataSourceHTSFile) data_source ;
      result = read_line_hts(file, pStr ) ;
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
   * We treat two cases. The HTS case must always return ZMAPGFF_VERSION_3
   * since we are converting HTS record data to GFF later on as it is read.
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


/* Read the HTS file header and look for the sequence name data.
 * This assumes that we have already opened the file and called
 * hts_hdr = sam_hdr_read() ;
 * Returns TRUE if reference sequence name found in BAM file header and
 * records that name in the HTS struct, returns FALSE otherwise.
 */
gboolean zMapDataSourceReadHTSHeader(ZMapDataSource source, const char *sequence)
{
  gboolean result = FALSE ;
  ZMapDataSourceHTSFile pHTS = (ZMapDataSourceHTSFile) source ;

  zMapReturnValIfFail(source && zMapDataSourceIsOpen(source), FALSE) ;
  zMapReturnValIfFail(pHTS && pHTS->hts_file, FALSE) ;

  // Read the HTS header and look a @SQ:<name> tag that matches the sequence
  // we are looking for.
  if (pHTS->hts_hdr && pHTS->hts_hdr->n_targets >= 1)
    {
      bool found ;
      int i ;

      for (i = 0, found = false ; i < pHTS->hts_hdr->n_targets ; i++)
        {
          if (g_ascii_strcasecmp(sequence, pHTS->hts_hdr->target_name[i]) == 0)
            {
              found = true ;
              break ;
            }
        }


      if (found)
        {
          pHTS->sequence = g_strdup(pHTS->hts_hdr->target_name[i]) ;
          result = TRUE ;
        }
    }

  return result ;
}
