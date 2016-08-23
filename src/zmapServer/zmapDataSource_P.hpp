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
 * Utility class to store info about a feature
 */
class ZMapDataSourceFeatureData
{
public:
  ZMapDataSourceFeatureData(const int start = 0,
                            const int end = 0,
                            const double score = 0.0,
                            const char strand_c = '.',
                            const char phase_c = '.',
                            const std::string &query_name = "",
                            const int query_start = 0,
                            const int query_end = 0,
                            const std::string &cigar = "")
    : start_(start),
      end_(end),
      score_(score),
      strand_c_(strand_c),
      phase_c_(phase_c),
      query_name_(query_name),
      query_start_(query_start),
      query_end_(query_end),
      cigar_(cigar)
  {}

  int start_ ;
  int end_ ;
  double score_ ;
  char strand_c_ ;
  char phase_c_ ;
  std::string query_name_ ;
  int query_start_ ;
  int query_end_ ;
  std::string cigar_ ;
} ;


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

  virtual bool parseSequence(gboolean &sequence_finished, std::string &err_msg) ;
  virtual void parserInit(GHashTable *featureset_2_column, GHashTable *source_2_sourcedata, ZMapStyleTree *styles) ;
  virtual bool parseBodyLine(bool &end_of_file, GError **error) = 0 ;
  virtual bool addFeaturesToBlock(ZMapFeatureBlock feature_block) ;
  virtual bool checkFeatureCount(bool &empty, std::string &err_msg) ;
  virtual GList* getFeaturesets() ;
  virtual ZMapSequence getSequence(GQuark seq_id, GError **error) ;
  virtual bool terminated() ;

  ZMapFeatureSet makeFeatureSet(const char *feature_name_id, GQuark feature_set_id, ZMapStyleMode feature_mode) ;
  ZMapFeature makeFeature(const char *sequence, const char *source, const char *so_type,
                          const int start, const int end, const double dScore,
                          const char strand_c, const char *feature_name,
                          const bool have_target = false, const int query_start = 0, const int query_end = 0,
                          ZMapStyleMode feature_mode = ZMAPSTYLE_MODE_INVALID, GError **error = NULL) ;
  
  bool endOfFile() ;
  GError* error() ;

  ZMapDataSourceType type ;

protected:

  GQuark source_name_ ;
  char *sequence_ ;
  int start_ ;
  int end_ ;
  GError *error_ ;

  ZMapGFFParser parser_ ;
  ZMapFeatureSet feature_set_ ; // only used if all features go into same feature set
  int num_features_ ;           // counts how many features we have created
  bool end_of_file_ ;           // set to true when there is no more to read

  GHashTable *featureset_2_column_ ;
  GHashTable *source_2_sourcedata_ ;
  ZMapStyleTree *styles_ ;
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

  bool parseSequence(gboolean &sequence_finished, std::string &err_msg) ;
  void parserInit(GHashTable *featureset_2_column, GHashTable *source_2_sourcedata, ZMapStyleTree *styles) ;
  bool parseBodyLine(bool &end_of_file, GError **error) ;
  bool addFeaturesToBlock(ZMapFeatureBlock feature_block) ;

private:
  const char *curLine() ;
  int curLineNumber() ;
  bool parseHeader(gboolean &done_header, ZMapGFFHeaderState &header_state, GError **error) ;

  GIOChannel *io_channel ;
  int gff_version_ ;
  bool gff_version_set_ ;
  GString *buffer_line_ ;
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
  bool parseBodyLine(bool &end_of_file, GError **error) ;

private:
  struct errCatch *err_catch_ ; // used by blatSrc library to catch errors
  struct bed* bed_features_ ;   // list of features read from the file
  struct bed* cur_feature_ ;    // current feature to process in the list of bed_features_
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
  bool parseBodyLine(bool &end_of_file, GError **error) ;

private:
  struct errCatch *err_catch_ ;
  struct bbiFile *bbi_file_ ;
  struct lm *lm_;                        // Memory pool to hold returned list from bbi file
  struct bigBedInterval *list_ ;         // list of intervals returned from the file
  struct bigBedInterval *cur_interval_ ; // current interval from list_
  struct bed* cur_feature_ ;             // current feature from interval
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
  bool parseBodyLine(bool &end_of_file, GError **error) ;

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
  bool parseBodyLine(bool &end_of_file, GError **error) ;

  htsFile *hts_file ;
  /* bam header and record object */
  bam_hdr_t *hts_hdr ;
  bam1_t *hts_rec ;

  /*
   * Data that we need to store
   */
  char *so_type ;

private:
  ZMapDataSourceFeatureData cur_feature_data_ ;
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
  bool parseBodyLine(bool &end_of_file, GError **error) ;

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
  ZMapDataSourceFeatureData cur_feature_data_ ;
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
