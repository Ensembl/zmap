/*  File: zmapDataStream_P.h
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
#ifndef DATA_STREAM_P_H
#define DATA_STREAM_P_H


#include <ZMap/zmapDataStream.hpp>


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
class ZMapDataStreamFeatureData
{
public:
  ZMapDataStreamFeatureData(const int start = 0,
                            const int end = 0,
                            const double score = 0.0,
                            const char strand_c = '.',
                            const char phase_c = '.',
                            const std::string &target_name = "",
                            const int target_start = 0,
                            const int target_end = 0,
                            const char target_strand_c = '.',
                            const std::string &cigar = "",
                            ZMapFeatureAlignFormat align_format = ZMAPALIGN_FORMAT_INVALID)
    : start_(start),
      end_(end),
      score_(score),
      strand_c_(strand_c),
      phase_c_(phase_c),
      target_name_(target_name),
      target_start_(target_start),
      target_end_(target_end),
      target_strand_c_(target_strand_c),
      cigar_(cigar),
      align_format_(align_format)
  {}

  int start_ ;
  int end_ ;
  double score_ ;
  char strand_c_ ;
  char phase_c_ ;
  std::string target_name_ ;
  int target_start_ ;
  int target_end_ ;
  int target_strand_c_ ;
  std::string cigar_ ;
  ZMapFeatureAlignFormat align_format_ ;
} ;


/*
 * General stream type
 */
class ZMapDataStreamStruct
{
public:
  ZMapDataStreamStruct(ZMapConfigSource source, const char *sequence, const int start, const int end) ;
  virtual ~ZMapDataStreamStruct() ;

  virtual gboolean init(const char *region_name, int start, int end) ;

  virtual bool isOpen() = 0 ;
  virtual bool checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server) = 0 ;
  virtual bool readLine() = 0 ;
  virtual bool gffVersion(int * const p_out_val) ;

  virtual bool parseSequence(gboolean &sequence_finished, std::string &err_msg) ;
  virtual void parserInit(GHashTable *featureset_2_column, GHashTable *source_2_sourcedata, ZMapStyleTree *styles) ;
  virtual bool parseBodyLine(GError **error) = 0 ;
  virtual bool addFeaturesToBlock(ZMapFeatureBlock feature_block) ;
  virtual bool checkFeatureCount(bool &empty, std::string &err_msg) ;
  virtual GList* getFeaturesets() ;
  virtual ZMapSequence getSequence(GQuark seq_id, GError **error) ;
  virtual bool terminated() ;

  ZMapFeatureSet makeFeatureSet(const char *feature_name_id, ZMapStyleMode feature_mode, const bool is_seq) ;
  ZMapFeature makeBEDFeature(struct bed *bed_feature, const int standard_fields, 
                             const char *so_term, GError **error) ;
  ZMapFeature makeFeature(const char *sequence, const char *so_type,
                          const int start, const int end, const double score,
                          const char strand_c, const char *feature_name_in,
                          const bool have_target = false, const int target_start = 0, const int target_end = 0, 
                          const char target_strand = '.', const char *cigar_string = NULL,
                          ZMapStyleMode feature_mode = ZMAPSTYLE_MODE_INVALID, 
                          const bool is_seq = false, GError **error = NULL) ;
  bool endOfFile() ;
  GError* error() ;

  ZMapDataStreamType type{ZMapDataStreamType::UNK} ;

protected:

  ZMapConfigSource source_{NULL} ;
  char *sequence_{nullptr} ;
  int start_{0} ;
  int end_{0} ;
  GError *error_{NULL} ;

  ZMapGFFParser parser_{NULL} ;
  ZMapFeatureSet feature_set_{NULL} ; // only used if all features go into same feature set
  int num_features_{0}    ;           // counts how many features we have created
  bool end_of_file_{false} ;          // set to true when there is no more to read

  GHashTable *featureset_2_column_{NULL} ;
  GHashTable *source_2_sourcedata_{NULL} ;
  ZMapStyleTree *styles_{NULL} ;

} ;


/*
 *  Full declarations of concrete types to represent the specific data sources types
 */

class ZMapDataStreamGIOStruct : public ZMapDataStreamStruct
{
public:
  ZMapDataStreamGIOStruct(ZMapConfigSource source, 
                          const char *file_name, const char *open_mode, 
                          const char *sequence, const int start, const int end) ;
  ~ZMapDataStreamGIOStruct() ;

  bool isOpen() ;
  bool checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server) ;
  bool readLine() ;
  bool gffVersion(int * const p_out_val) ;

  bool parseSequence(gboolean &sequence_finished, std::string &err_msg) ;
  void parserInit(GHashTable *featureset_2_column, GHashTable *source_2_sourcedata, ZMapStyleTree *styles) ;
  bool parseBodyLine(GError **error) ;
  bool addFeaturesToBlock(ZMapFeatureBlock feature_block) ;

private:
  const char *curLine() ;
  int curLineNumber() ;
  bool parseHeader(gboolean &done_header, ZMapGFFHeaderState &header_state, GError **error) ;

  GIOChannel *io_channel{NULL} ;
  int gff_version_{0} ;
  bool gff_version_set_{false} ;
  GString *buffer_line_{NULL} ;
} ;


class ZMapDataStreamBEDStruct : public ZMapDataStreamStruct
{
public:
  ZMapDataStreamBEDStruct(ZMapConfigSource source, 
                          const char *file_name, const char *open_mode, 
                          const char *sequence, const int start, const int end) ;
  ~ZMapDataStreamBEDStruct() ;

  bool isOpen() ;
  bool checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server) ;
  bool readLine() ;
  bool parseBodyLine(GError **error) ;

private:
  struct bed* bed_features_{NULL} ;   // list of features read from the file
  struct bed* cur_feature_{NULL} ;    // current feature to process in the list of bed_features_
  int standard_fields_{0} ;
} ;


class ZMapDataStreamBIGBEDStruct : public ZMapDataStreamStruct
{
public:
  ZMapDataStreamBIGBEDStruct(ZMapConfigSource source, 
                             const char *file_name, const char *open_mode, 
                             const char *sequence, const int start, const int end) ;
  ~ZMapDataStreamBIGBEDStruct() ;

  bool isOpen() ;
  bool checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server) ;
  bool readLine() ;
  bool parseBodyLine(GError **error) ;

private:
  struct bbiFile *bbi_file_{NULL} ;
  struct lm *lm_{NULL};                        // Memory pool to hold returned list from bbi file
  struct bigBedInterval *list_{NULL} ;         // list of intervals returned from the file
  struct bigBedInterval *cur_interval_{NULL} ; // current interval from list_
  struct bed* cur_feature_{NULL} ;             // current feature from interval
  int standard_fields_{0} ;
} ;


class ZMapDataStreamBIGWIGStruct : public ZMapDataStreamStruct
{
public:
  ZMapDataStreamBIGWIGStruct(ZMapConfigSource source, 
                             const char *file_name, const char *open_mode, 
                             const char *sequence, const int start, const int end) ;
  ~ZMapDataStreamBIGWIGStruct() ;

  bool isOpen() ;
  bool checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server) ;
  bool readLine() ;
  bool parseBodyLine(GError **error) ;

private:
  struct bbiFile *bbi_file_{NULL} ;
  struct lm *lm_{NULL}; // Memory pool to hold returned list from bbi file
  struct bbiInterval *list_{NULL} ;
  struct bbiInterval *cur_interval_{NULL} ; // current item from list_
} ;


#ifdef USE_HTSLIB

class ZMapDataStreamHTSStruct : public ZMapDataStreamStruct
{
public:
  ZMapDataStreamHTSStruct(ZMapConfigSource source, 
                          const char *file_name, const char *open_mode, 
                          const char *sequence, const int start, const int end) ;
  ~ZMapDataStreamHTSStruct() ;

  gboolean init(const char *region_name, int start, int end) ;
  bool isOpen() ;
  bool checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server) ;
  bool readLine() ;
  bool parseBodyLine(GError **error) ;

  htsFile *hts_file{NULL} ;
  /* bam header and record object */
  bam_hdr_t *hts_hdr{NULL} ;
  hts_idx_t *hts_idx{NULL} ;
  hts_itr_t *hts_iter{NULL} ;
  bam1_t *hts_rec{NULL} ;

private:
  bool processRead() ;

  ZMapDataStreamFeatureData cur_feature_data_ ;  
} ;


class ZMapDataStreamBCFStruct : public ZMapDataStreamStruct
{
public:
  ZMapDataStreamBCFStruct(ZMapConfigSource source, 
                          const char *file_name, const char *open_mode, 
                          const char *sequence, const int start, const int end) ;
  ~ZMapDataStreamBCFStruct() ;
  bool isOpen() ;
  bool checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server) ;
  bool readLine() ;
  bool parseBodyLine(GError **error) ;

  htsFile *hts_file{NULL} ;
  /* bam header and record object */
  bcf_hdr_t *hts_hdr{NULL} ;
  bcf1_t *hts_rec{NULL} ;

private:
  int rid_{0} ; // stores the ref sequence id value used by bcf parser for our required sequence_
  ZMapDataStreamFeatureData cur_feature_data_ ;
} ;

#endif


typedef ZMapDataStreamGIOStruct *ZMapDataStreamGIO ;
typedef ZMapDataStreamBEDStruct *ZMapDataStreamBED ;
typedef ZMapDataStreamBIGBEDStruct *ZMapDataStreamBIGBED ;
typedef ZMapDataStreamBIGWIGStruct *ZMapDataStreamBIGWIG ;
#ifdef USE_HTSLIB
typedef ZMapDataStreamHTSStruct *ZMapDataStreamHTS ;
typedef ZMapDataStreamBCFStruct *ZMapDataStreamBCF ;
#endif




#endif /* DATA_STREAM_P_H */
