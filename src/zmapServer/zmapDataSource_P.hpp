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


#include <ZMap/zmapDataSource.hpp>


#ifdef USE_HTSLIB

/*
 * HTS header
 */
#include <htslib/hts.h>
#include <htslib/sam.h>
#include <htslib/vcf.h>

#endif


struct bed ;
struct errCatch ;


/*
 * General source type
 */
class ZMapDataSourceStruct
{
public:
  ZMapDataSourceStruct(const GQuark source_name, const char *sequence, const int start, const int end) ;
  virtual ~ZMapDataSourceStruct() ;

  virtual bool isOpen() = 0 ;
  virtual bool checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server) = 0 ;
  virtual bool readLine() = 0 ;
  virtual bool gffVersion(int * const p_out_val) ;

  virtual bool parseSequence(gboolean &sequence_finished, GError **error) ;
  virtual bool parseBodyLine(GError **error) ;
  virtual void parserInit(GHashTable *featureset_2_column, GHashTable *source_2_sourcedata, ZMapStyleTree &styles) ;
  virtual bool checkFeatures(bool &empty, std::string &err_msg) ;
  virtual bool getFeatures(ZMapFeatureBlock feature_block) ;
  virtual GList* getFeaturesets() ;
  virtual ZMapSequence getSequence(GQuark seq_id, GError **error) ;
  virtual int lineNumber() ;
  virtual const char *lineString() ;
  virtual bool endOfFile() ;
  virtual bool terminated() ;
  
  GError* error() ;

  ZMapDataSourceType type ;

protected:
  virtual bool parseHeader(gboolean &done_header, ZMapGFFHeaderState &header_state, GError **error) ;


  GQuark source_name_ ;
  char *sequence_ ;
  int start_ ;
  int end_ ;
  GError *error_ ;

  GString *buffer_line_ ;
  ZMapGFFParser parser_ ;

} ;


/*
 *  Full declarations of concrete types to represent the specific data sources types
 */

class ZMapDataSourceGIOStruct : public ZMapDataSourceStruct
{
public:
  ZMapDataSourceGIOStruct(const GQuark source_name, const char *file_name, const char *open_mode, 
                          const char *sequence, const int start, const int end) ;
  ~ZMapDataSourceGIOStruct() ;

  bool isOpen() ;
  bool checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server) ;
  bool readLine() ;
  bool gffVersion(int * const p_out_val) ;

  GIOChannel *io_channel ;

private:
  int gff_version_ ;
  bool gff_version_set_ ;
} ;


class ZMapDataSourceBEDStruct : public ZMapDataSourceStruct
{
public:
  ZMapDataSourceBEDStruct(const GQuark source_name, const char *file_name, const char *open_mode, 
                          const char *sequence, const int start, const int end) ;
  ~ZMapDataSourceBEDStruct() ;

  bool isOpen() ;
  bool checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server) ;
  bool readLine() ;

private:
  struct errCatch *err_catch_ ;
  struct bed* bed_features_ ;
  struct bed* cur_feature_ ;
} ;


class ZMapDataSourceBIGBEDStruct : public ZMapDataSourceStruct
{
public:
  ZMapDataSourceBIGBEDStruct(const GQuark source_name, const char *file_name, const char *open_mode, 
                             const char *sequence, const int start, const int end) ;
  ~ZMapDataSourceBIGBEDStruct() ;

  bool isOpen() ;
  bool checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server) ;
  bool readLine() ;

private:
  struct errCatch *err_catch_ ;
  struct bbiFile *bbi_file_ ;
  struct lm *lm_; // Memory pool to hold returned list from bbi file
  struct bigBedInterval *list_ ;
  struct bigBedInterval *cur_interval_ ; // current item from list_
} ;


class ZMapDataSourceBIGWIGStruct : public ZMapDataSourceStruct
{
public:
  ZMapDataSourceBIGWIGStruct(const GQuark source_name, const char *file_name, const char *open_mode, 
                             const char *sequence, const int start, const int end) ;
  ~ZMapDataSourceBIGWIGStruct() ;

  bool isOpen() ;
  bool checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server) ;
  bool readLine() ;

private:
  struct errCatch *err_catch_ ;
  struct bbiFile *bbi_file_ ;
  struct lm *lm_; // Memory pool to hold returned list from bbi file
  struct bbiInterval *list_ ;
  struct bbiInterval *cur_interval_ ; // current item from list_
} ;


#ifdef USE_HTSLIB

class ZMapDataSourceHTSStruct : public ZMapDataSourceStruct
{
public:
  ZMapDataSourceHTSStruct(const GQuark source_name, const char *file_name, const char *open_mode, 
                          const char *sequence, const int start, const int end) ;
  ~ZMapDataSourceHTSStruct() ;
  bool isOpen() ;
  bool checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server) ;
  bool readLine() ;

  htsFile *hts_file ;
  /* bam header and record object */
  bam_hdr_t *hts_hdr ;
  bam1_t *hts_rec ;

  /*
   * Data that we need to store
   */
  char *so_type ;
} ;


class ZMapDataSourceBCFStruct : public ZMapDataSourceStruct
{
public:
  ZMapDataSourceBCFStruct(const GQuark source_name, const char *file_name, const char *open_mode, 
                          const char *sequence, const int start, const int end) ;
  ~ZMapDataSourceBCFStruct() ;
  bool isOpen() ;
  bool checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server) ;
  bool readLine() ;

  htsFile *hts_file ;
  /* bam header and record object */
  bcf_hdr_t *hts_hdr ;
  bcf1_t *hts_rec ;

  /*
   * Data that we need to store
   */
  char *so_type ;

private:
  int rid_ ; // stores the ref sequence id value used by bcf parser for our required sequence_
} ;

#endif


typedef ZMapDataSourceGIOStruct *ZMapDataSourceGIO ;
typedef ZMapDataSourceBEDStruct *ZMapDataSourceBED ;
typedef ZMapDataSourceBIGBEDStruct *ZMapDataSourceBIGBED ;
typedef ZMapDataSourceBIGWIGStruct *ZMapDataSourceBIGWIG ;
#ifdef USE_HTSLIB
typedef ZMapDataSourceHTSStruct *ZMapDataSourceHTS ;
typedef ZMapDataSourceBCFStruct *ZMapDataSourceBCF ;
#endif




#endif
