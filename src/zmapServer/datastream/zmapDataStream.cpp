/*  File: zmapDataStream.c
 *  Author: Steve Miller (sm23@sanger.ac.uk)
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
 * Description: Code for concrete data sources.
 *
 * Exported functions:
 *-------------------------------------------------------------------
 */
#include <ZMap/zmap.hpp>

#include <map>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <cctype>
#include <string>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapConfigIni.hpp>
#include <ZMap/zmapConfigStrings.hpp>
#include <ZMap/zmapGFF.hpp>
#include <ZMap/zmapGFFStringUtils.hpp>
#include <ZMap/zmapServerProtocol.hpp>

#include <zmapDataStream_P.hpp>

#ifdef __cplusplus
extern "C" {
#endif

#include <blatSrc/basicBed.h>
#include <blatSrc/bigBed.h>
#include <blatSrc/bigWig.h>
#include <blatSrc/errCatch.h>

#ifdef __cplusplus
}
#endif


using namespace std ;


namespace // unnamed namespace
{

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
#define ZMAP_BAM_SO_TERM  "read"
#define ZMAP_BCF_SO_TERM  "snv"
#define ZMAP_BED_SO_TERM  "sequence_feature"
#define ZMAP_BIGBED_SO_TERM "sequence_feature"
#define ZMAP_BIGWIG_SO_TERM "score"
#define ZMAP_CIGARSTRING_MAXLENGTH 2048
#define READBUFFER_SIZE 2048
#define BED_DEFAULT_FIELDS 3       // min number of fields in a BED file
#define MAX_FILE_THREADS 10        // max number of file threads at any one time
#define SEQ_LIST_SEPARATOR "\n"    // used as separator in list of sequence names


/* 
 * Function declarations
 */
static string toLower(const string &s) ;
static ZMapDataStreamType dataSourceTypeFromExtension(const string &file_ext, GError **error_out) ;


/* 
 * Utility classes
 */

/* Need a semaphore to limit the number of threads.
 * From http://stackoverflow.com/questions/4792449/c0x-has-no-semaphores-how-to-synchronize-threads */

class semaphore {
public:
    using native_handle_type = std::condition_variable::native_handle_type;

    explicit semaphore(size_t n = 0);
    semaphore(const semaphore&) = delete;
    semaphore& operator=(const semaphore&) = delete;

    void notify();
    void wait();
    bool try_wait();
    template<class Rep, class Period>
    bool wait_for(const std::chrono::duration<Rep, Period>& d);
    template<class Clock, class Duration>
    bool wait_until(const std::chrono::time_point<Clock, Duration>& t);

    native_handle_type native_handle();

private:
    size_t                  count;
    std::mutex              mutex;
    std::condition_variable cv;
};


// Utility class to do error handling for blatSrc library.
// We must use this error handler for all calls to the library to make sure it doesn't abort.
class BlatLibErrHandler
{
public:
  BlatLibErrHandler() ;
  ~BlatLibErrHandler() ;

  bool errTry() ;
  bool errCatch() ;
  string errMsg() ;

private:
  struct errCatch *err_catch_{NULL} ;
} ;


/* Custom comparator for a map to make the key case-insensitive. This is a 'less' object,
 * i.e. operator() returns true if the first argument is less than the second. */
class caseInsensitiveCmp
{
public:
  bool operator()(const std::string& a_in, const std::string& b_in) const 
  {
    bool result = false ;

    string a = toLower(a_in) ;
    string b = toLower(b_in) ;
    
    result = (a < b) ;

    return result ;
  }
};


/* 
 * Globals
 */

/* Keep track of how many files we've opened in different threads */
static semaphore semaphore_G(MAX_FILE_THREADS) ;

/* Map of file types to a data-source types */
const map<string, ZMapDataStreamType> file_type_to_stream_type_G =
  {
    {"GFF",     ZMapDataStreamType::GIO}
    ,{"Bed",    ZMapDataStreamType::BED}
    ,{"bigBed", ZMapDataStreamType::BIGBED}
    ,{"bigWig", ZMapDataStreamType::BIGWIG}
    ,{"BAM",    ZMapDataStreamType::HTS}
    ,{"SAM",    ZMapDataStreamType::HTS}
    ,{"CRAM",   ZMapDataStreamType::HTS}
    ,{"BCF",    ZMapDataStreamType::BCF}
    ,{"VCF",    ZMapDataStreamType::BCF}
  } ;


} // unnamed namespace


/*
 * Constructors
 */

ZMapDataStreamStruct::ZMapDataStreamStruct(ZMapConfigSource source,
                                           const char *sequence,
                                           const int start,
                                           const int end)
  : type(ZMapDataStreamType::UNK), 
    source_(source),
    sequence_{nullptr},
    start_(start),
    end_(end),
    error_(NULL),
    parser_(NULL),
    feature_set_(NULL),
    num_features_(0),
    end_of_file_(false),
    featureset_2_column_(NULL),
    source_2_sourcedata_(NULL),
    styles_(NULL)
{
  semaphore_G.wait() ;

  if (sequence)
    sequence_ = g_strdup(sequence) ;
}

ZMapDataStreamGIOStruct::ZMapDataStreamGIOStruct(ZMapConfigSource source, 
                                                 const char *file_name,
                                                 const char *open_mode,
                                                 const char *sequence,
                                                 const int start,
                                                 const int end)
  : ZMapDataStreamStruct(source, sequence, start, end),
    gff_version_(ZMAPGFF_VERSION_UNKNOWN),
    gff_version_set_(false)
{
  type = ZMapDataStreamType::GIO ;
  io_channel = g_io_channel_new_file(file_name, open_mode, &error_) ;

  gffVersion(&gff_version_) ;
  gff_version_set_ = true ;

  parser_ = zMapGFFCreateParser(gff_version_,
                                sequence,
                                start,
                                end,
                                source_) ;

  /* The caller may only want a small part of the features in the stream so we set the
   * feature start/end from the block, not the gff stream start/end. */
  if (parser_ && end)
    {
      zMapGFFSetFeatureClipCoords(parser_, start, end) ;
      zMapGFFSetFeatureClip(parser_, GFF_CLIP_ALL);
    }

  buffer_line_ = g_string_sized_new(READBUFFER_SIZE) ;
}



ZMapDataStreamBEDStruct::ZMapDataStreamBEDStruct(ZMapConfigSource source, 
                                                 const char *file_name,
                                                 const char *open_mode,
                                                 const char *sequence,
                                                 const int start,
                                                 const int end)
  : ZMapDataStreamStruct(source, sequence, start, end),
    cur_feature_(NULL),
    standard_fields_(BED_DEFAULT_FIELDS)
{
  zMapReturnIfFail(source && file_name) ;
  type = ZMapDataStreamType::BED ;

  if (source->numFields() > 0)
    standard_fields_ = source->numFields() ;

  BlatLibErrHandler err_handler ;
  string err_msg ;
  
  if (err_handler.errTry())
    {
      // Open the file
      char *file_name_copy = g_strdup(file_name) ; // to avoid casting away const
      bed_features_ = bedLoadAll(file_name_copy) ;
      g_free(file_name_copy) ;
    }

  bool is_err = err_handler.errCatch() ;

  if (is_err)
    {
      // Remember the original error message
      err_msg = err_handler.errMsg() ;

      if (strncasecmp(file_name, "http://", 7) == 0 && strlen(file_name) > 7)
        {
          if (err_handler.errTry())
            {
              char *file_name_copy = g_strdup_printf("https://%s", file_name + 7) ;
              bed_features_ = bedLoadAll(file_name_copy) ;
              g_free(file_name_copy) ;
            }

          is_err = err_handler.errCatch() ;
        }
    }

  if (is_err)
    {
      zMapLogWarning("Failed to open file: %s", err_msg.c_str()) ;
      g_set_error(&error_, g_quark_from_string("ZMap"), 99, 
                  "Failed to open file: %s", err_msg.c_str()) ;
      bed_features_ = NULL ;
    }

  int gff_version = ZMAPGFF_VERSION_UNKNOWN ;
  gffVersion(&gff_version) ;

  parser_ = zMapGFFCreateParser(gff_version,
                                sequence,
                                start,
                                end,
                                source_) ;

  /* The caller may only want a small part of the features in the stream so we set the
   * feature start/end from the block, not the gff stream start/end. */
  if (parser_ && end)
    {
      zMapGFFSetFeatureClipCoords(parser_, start, end) ;
      zMapGFFSetFeatureClip(parser_, GFF_CLIP_ALL);
    }
}

ZMapDataStreamBIGBEDStruct::ZMapDataStreamBIGBEDStruct(ZMapConfigSource source, 
                                                       const char *file_name,
                                                       const char *open_mode,
                                                       const char *sequence,
                                                       const int start,
                                                       const int end)
  : ZMapDataStreamStruct(source, sequence, start, end),
    lm_(NULL),
    list_(NULL),
    cur_interval_(NULL),
    cur_feature_(NULL),
    standard_fields_(BED_DEFAULT_FIELDS)
{
  zMapReturnIfFail(file_name) ;
  type = ZMapDataStreamType::BIGBED ;

  if (source->numFields() > 0)
    standard_fields_ = source->numFields() ;

  BlatLibErrHandler err_handler ;
  string err_msg ;

  // Open the file
  if (err_handler.errTry())
    {
      char *file_name_copy = g_strdup(file_name) ; // to avoid casting away const
      bbi_file_ = bigBedFileOpen(file_name_copy);
      g_free(file_name_copy) ;

      if (bbi_file_)
        standard_fields_ = bbi_file_->definedFieldCount ;
    }

  bool is_err = err_handler.errCatch() ;

  if (is_err)
    {
      err_msg = err_handler.errMsg() ;

      /* If it's http try again as https */
      if (strncasecmp(file_name, "http://", 7) == 0 && strlen(file_name) > 7)
        {
          if (err_handler.errTry())
            {
              char *file_name_copy = g_strdup_printf("https://%s", file_name + 7) ;
              bbi_file_ = bigBedFileOpen(file_name_copy) ;
              g_free(file_name_copy) ;
            }

          is_err = err_handler.errCatch() ;
        }
    }

  if (is_err)
    {
      zMapLogWarning("Failed to open file: %s", err_msg.c_str()) ;
      g_set_error(&error_, g_quark_from_string("ZMap"), 99, 
                  "Failed to open file: %s", err_msg.c_str()) ;
      bbi_file_ = NULL ;
    }


  int gff_version = ZMAPGFF_VERSION_UNKNOWN ;
  gffVersion(&gff_version) ;

  parser_ = zMapGFFCreateParser(gff_version,
                                sequence,
                                start,
                                end,
                                source_) ;

  /* The caller may only want a small part of the features in the stream so we set the
   * feature start/end from the block, not the gff stream start/end. */
  if (parser_ && end)
    {
      zMapGFFSetFeatureClipCoords(parser_, start, end) ;
      zMapGFFSetFeatureClip(parser_, GFF_CLIP_ALL);
    }
}

ZMapDataStreamBIGWIGStruct::ZMapDataStreamBIGWIGStruct(ZMapConfigSource source, 
                                                       const char *file_name,
                                                       const char *open_mode,
                                                       const char *sequence,
                                                       const int start,
                                                       const int end)
  : ZMapDataStreamStruct(source, sequence, start, end),
    lm_(NULL),
    list_(NULL),
    cur_interval_(NULL)
{
  type = ZMapDataStreamType::BIGWIG ;

  BlatLibErrHandler err_handler ;
  string err_msg ;

  if (err_handler.errTry())
    {
      // Open the file
      char *file_name_copy = g_strdup(file_name) ; // to avoid casting away const
      bbi_file_ = bigWigFileOpen(file_name_copy);
      g_free(file_name_copy) ;
    }

  bool is_err = err_handler.errCatch() ;

  if (is_err)
    {
      err_msg = err_handler.errMsg() ;

      /* If it's http try again as https */
      if (strncasecmp(file_name, "http://", 7) == 0 && strlen(file_name) > 7)
        {
          if (err_handler.errTry())
            {
              char *file_name_copy = g_strdup_printf("https://%s", file_name + 7) ;
              bbi_file_ = bigWigFileOpen(file_name_copy) ;
              g_free(file_name_copy) ;
            }
        
          is_err = err_handler.errCatch() ;
        }
    }

  if (is_err)
    {
      zMapLogWarning("Failed to open file: %s", err_msg.c_str()) ;
      g_set_error(&error_, g_quark_from_string("ZMap"), 99, 
                  "Failed to open file: %s", err_msg.c_str()) ;
      bbi_file_ = NULL ;
    }


  int gff_version = ZMAPGFF_VERSION_UNKNOWN ;
  gffVersion(&gff_version) ;

  parser_ = zMapGFFCreateParser(gff_version,
                                sequence,
                                start,
                                end,
                                source_) ;

  /* The caller may only want a small part of the features in the stream so we set the
   * feature start/end from the block, not the gff stream start/end. */
  if (parser_ && end)
    {
      zMapGFFSetFeatureClipCoords(parser_, start, end) ;
      zMapGFFSetFeatureClip(parser_, GFF_CLIP_ALL);
    }
}

#ifdef USE_HTSLIB
ZMapDataStreamHTSStruct::ZMapDataStreamHTSStruct(ZMapConfigSource source, 
                                                 const char *file_name, 
                                                 const char *open_mode,
                                                 const char *sequence,
                                                 const int start,
                                                 const int end)
  : ZMapDataStreamStruct(source, sequence, start, end),
    hts_file(NULL),
    hts_hdr(NULL),
    hts_idx(NULL),
    hts_iter(NULL),
    hts_rec(NULL)
{
  type = ZMapDataStreamType::HTS ;

  hts_file = hts_open(file_name, open_mode);

  if (hts_file)
    hts_hdr = sam_hdr_read(hts_file) ;

  if (hts_hdr)
    hts_idx = sam_index_load(hts_file, file_name) ;

  hts_rec = bam_init1() ;
  if (hts_file && hts_hdr && hts_rec)
    {
    }
  else
    {
      zMapLogWarning("Failed to open file: %s", file_name) ;
      g_set_error(&error_, g_quark_from_string("ZMap"), 99, 
                  "Failed to open file: %s", file_name) ;
    }


  int gff_version = ZMAPGFF_VERSION_UNKNOWN ;
  gffVersion(&gff_version) ;

  parser_ = zMapGFFCreateParser(gff_version,
                                sequence,
                                start,
                                end,
                                source_) ;

  /* The caller may only want a small part of the features in the stream so we set the
   * feature start/end from the block, not the gff stream start/end. */
  if (parser_ && end)
    {
      zMapGFFSetFeatureClipCoords(parser_, start, end) ;
      zMapGFFSetFeatureClip(parser_, GFF_CLIP_ALL);
    }
}

ZMapDataStreamBCFStruct::ZMapDataStreamBCFStruct(ZMapConfigSource source, 
                                                 const char *file_name, 
                                                 const char *open_mode,
                                                 const char *sequence,
                                                 const int start,
                                                 const int end)
  : ZMapDataStreamStruct(source, sequence, start, end),
    hts_file(NULL),
    hts_hdr(NULL),
    hts_rec(NULL),
    rid_(0)
{
  type = ZMapDataStreamType::BCF ;

  hts_file = hts_open(file_name, open_mode);

  if (hts_file)
    hts_hdr = bcf_hdr_read(hts_file) ;

  hts_rec = bcf_init1() ;
  if (hts_file && hts_hdr && hts_rec)
    {
    }
  else
    {
      zMapLogWarning("Failed to open file: %s", file_name) ;
      g_set_error(&error_, g_quark_from_string("ZMap"), 99, 
                  "Failed to open file: %s", file_name) ;
    }


  int gff_version = ZMAPGFF_VERSION_UNKNOWN ;
  gffVersion(&gff_version) ;

  parser_ = zMapGFFCreateParser(gff_version,
                                sequence,
                                start,
                                end,
                                source) ;

  /* The caller may only want a small part of the features in the stream so we set the
   * feature start/end from the block, not the gff stream start/end. */
  if (parser_ && end)
    {
      zMapGFFSetFeatureClipCoords(parser_, start, end) ;
      zMapGFFSetFeatureClip(parser_, GFF_CLIP_ALL);
    }
}
#endif

/*
 * Destructors
 */

ZMapDataStreamStruct::~ZMapDataStreamStruct()
{
  semaphore_G.notify() ;

  if (sequence_)
    g_free(sequence_) ;

  if (parser_)
    zMapGFFDestroyParser(parser_) ;
}

ZMapDataStreamGIOStruct::~ZMapDataStreamGIOStruct()
{
  GIOStatus gio_status = G_IO_STATUS_NORMAL ;
  GError *err = NULL ;

  gio_status = g_io_channel_shutdown( io_channel , FALSE, &err) ;
  
  if (gio_status != G_IO_STATUS_ERROR && gio_status != G_IO_STATUS_AGAIN)
    {
      g_io_channel_unref( io_channel ) ;
      io_channel = NULL ;
    }
  else
    {
      zMapLogCritical("Could not close GIOChannel in zMapDataStreamDestroy(), %s", "") ;
    }

  if (buffer_line_)
    g_string_free(buffer_line_, TRUE) ;
}

ZMapDataStreamBEDStruct::~ZMapDataStreamBEDStruct()
{
  if (bed_features_)
    {
      bedFreeList(&bed_features_) ;
      bed_features_ = NULL ;
    }
}

ZMapDataStreamBIGBEDStruct::~ZMapDataStreamBIGBEDStruct()
{
  if (bbi_file_)
    {
      bigBedFileClose(&bbi_file_) ;
      bbi_file_ = NULL ;
    }
}

ZMapDataStreamBIGWIGStruct::~ZMapDataStreamBIGWIGStruct()
{
  if (bbi_file_)
    {
      bigWigFileClose(&bbi_file_) ;
      bbi_file_ = NULL ;
    }
}


#ifdef USE_HTSLIB
ZMapDataStreamHTSStruct::~ZMapDataStreamHTSStruct()
{
  if (hts_file)
    hts_close(hts_file) ;
  if (hts_rec)
    bam_destroy1(hts_rec) ;
  if (hts_hdr)
    bam_hdr_destroy(hts_hdr) ;
  if (hts_idx)
    hts_idx_destroy(hts_idx) ;
  if (hts_iter)
    hts_itr_destroy(hts_iter) ;
}

ZMapDataStreamBCFStruct::~ZMapDataStreamBCFStruct()
{
  if (hts_file)
    hts_close(hts_file) ;
  if (hts_rec)
    bcf_destroy1(hts_rec) ;
  if (hts_hdr)
    bcf_hdr_destroy(hts_hdr) ;
}
#endif


/*
 * Class member access/query functions
 */

GError* ZMapDataStreamStruct::error()
{
  return error_ ;
}

bool ZMapDataStreamGIOStruct::isOpen()
{
  bool result = false ;

  // Check the io channel was opened ok and the parser was created
  if (io_channel && parser_)
    {
      result = true ;
    }

  return result ;
}

bool ZMapDataStreamBEDStruct::isOpen()
{
  bool result = false ;
  
  if (bed_features_)
    result = true ;

  return result ;
}

bool ZMapDataStreamBIGBEDStruct::isOpen()
{
  bool result = false ;

  if (bbi_file_)
    result = true ;

  return result ;
}

bool ZMapDataStreamBIGWIGStruct::isOpen()
{
  bool result = false ;

  if (bbi_file_)
    result = true ;

  return result ;
}

#ifdef USE_HTSLIB
bool ZMapDataStreamHTSStruct::isOpen()
{
  bool result = false ;

  if (hts_file)
    result = true ;
  
  return result ;
}

bool ZMapDataStreamBCFStruct::isOpen()
{
  bool result = false ;

  if (hts_file)
    result = true ;
  
  return result ;
}
#endif



/*
 * Get the GFF version. We must always return ZMAPGFF_VERSION_3 for any type 
 * where we are converting the input data to GFF later on.
 */
bool ZMapDataStreamStruct::gffVersion(int * const p_out_val)
{
  *p_out_val = ZMAPGFF_VERSION_3 ;
  return true ;
}

bool ZMapDataStreamGIOStruct::gffVersion(int * const p_out_val)
{
  if (gff_version_set_)
    {
      if (p_out_val)
        *p_out_val = gff_version_ ;

      return true ;
    }
  else
    {
      int out_val = ZMAPGFF_VERSION_UNKNOWN ;
      gboolean result = FALSE ;
      zMapReturnValIfFail(p_out_val, result) ;

      GString *pString = g_string_new(NULL) ;
      GIOStatus cIOStatus = G_IO_STATUS_NORMAL ;
      GError *pError = NULL ;

      result = zMapGFFGetVersionFromGIO(io_channel, pString,
                                        &out_val, &cIOStatus, &pError) ;

      *p_out_val = out_val ;

      if ( !result || (cIOStatus != G_IO_STATUS_NORMAL)  || pError ||
           ((out_val != ZMAPGFF_VERSION_2) && (out_val != ZMAPGFF_VERSION_3)) )
        {
          zMapLogCritical("Could not obtain GFF version from GIOChannel in gffVersion(), %s", "") ;
          /* This is set to make sure that the calling program notices the error. */
          result = FALSE ;
        }

      g_string_free(pString, TRUE) ;

      if (pError)
        g_error_free(pError) ;

      return result ;
    }
}


bool ZMapDataStreamGIOStruct::checkHeader(string &err_msg, bool &empty_or_eof, const bool sequence_server)
{
  bool result = false ;

  bool empty_file  = true ;  // set to true if file is completely empty
  bool more_data = true ;    // remains true while there is more unread data in the file
  boolean done_header = false ; // gets set to true if we successfully reach the end of the header

  ZMapGFFHeaderState header_state = GFF_HEADER_NONE ;
  GError *error = NULL ;

  empty_or_eof = false ;    // gets set to true if file is empty or we hit EOF while reading header

  /*
   * reset done flag for seq else skip the data
   */
  if (sequence_server)
    zMapGFFParserSetSequenceFlag(parser_);

  while ((more_data = readLine()))
    {
      empty_file = false ;
      result = true ;

      if (parseHeader(done_header, header_state, &error))
        {
          if (done_header)
            break ;
        }
      else
        {
          if (!done_header)
            {
              if (!error)
                {
                  stringstream err_ss ;
                  err_ss << "parseHeader() failed with no GError for line " << curLineNumber() << ": " << curLine() ;
                  err_msg = err_ss.str() ;
                }
              else
                {
                  /* If the error was serious we stop processing and return the error,
                   * otherwise we just log the error. */
                  if (terminated() && error->message)
                    {
                      err_msg = error->message ;
                    }
                  else
                    {
                      stringstream err_ss ;
                      err_ss << "parseHeader() failed for line " << curLineNumber() << ": " << curLine() ;
                      err_msg = err_ss.str() ;
                    }
                }
              
              result = false ;
            }

          break ;
        }
    } ;

  if (header_state == GFF_HEADER_ERROR || empty_file)
    {
      result = false ;
      
      if (!more_data)
        {
          empty_or_eof = true ;

          if (empty_file)
            {
              err_msg = "Empty file." ;
            }
          else
            {
              stringstream err_ss ;
              err_ss << "EOF reached while trying to read header, at line " << curLineNumber() ;
              err_msg = err_ss.str() ;
            }
        }
      else
        {
          stringstream err_ss ;
          err_ss << "Error in GFF header, at line " << curLineNumber() ;
          err_msg = err_ss.str() ;
        }
    }

  return result ;
}

bool ZMapDataStreamBEDStruct::checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server)
{
  // We can't easily check in advance what sequences are in the file so just allow it and filter
  // when we come to parse the individual lines.
  return true ;
}

/* Read the header info from the bigBed file and return true if it contains the required sequence
 * name */
bool ZMapDataStreamBIGBEDStruct::checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server)
{
  bool result = false ;
  zMapReturnValIfFail(isOpen(), result) ;

  BlatLibErrHandler err_handler ;
  
  if (err_handler.errTry())
    {
      // Get the list of sequences in the file and check whether our required sequence exists
      struct bbiChromInfo *chromList = bbiChromList(bbi_file_);
      stringstream available_seqs;

      for (bbiChromInfo *chrom = chromList; chrom != NULL; chrom = chrom->next)
        {
          if (strcmp(chrom->name, sequence_) == 0)
            {
              result = true ;
              break ;
            }

          available_seqs << chrom->name << SEQ_LIST_SEPARATOR ;
        }

      if (!result)
        {
          
          g_set_error(&error_, g_quark_from_string("ZMap"), 99,
                      "File header does not contain sequence '%s'. Available sequences in this file are:\n%s\n", 
                      sequence_, available_seqs.str().c_str()) ;
        }
    }

  if (err_handler.errCatch())
    {
      zMapLogWarning("Error checking header: %s", err_handler.errMsg().c_str());
    }

  if (error_ && error_->message)
    err_msg = error_->message ; 

  return result ;
}

/* Read the header info from the bigBed file and return true if it contains the required sequence
 * name */
bool ZMapDataStreamBIGWIGStruct::checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server)
{
  bool result = false ;
  zMapReturnValIfFail(isOpen(), result) ;

  BlatLibErrHandler err_handler ;

  if (err_handler.errTry())
    {
      // Get the list of sequences in the file and check whether our required sequence exists
      struct bbiChromInfo *chromList = bbiChromList(bbi_file_);
      stringstream available_seqs;

      for (bbiChromInfo *chrom = chromList; chrom != NULL; chrom = chrom->next)
        {
          if (strcmp(chrom->name, sequence_) == 0)
            {
              result = true ;
              break ;
            }

          available_seqs << chrom->name << SEQ_LIST_SEPARATOR ;
        }

      if (!result)
        {
          g_set_error(&error_, g_quark_from_string("ZMap"), 99,
                      "File header does not contain sequence '%s'. Available sequences in this file are:\n%s\n", 
                      sequence_, available_seqs.str().c_str()) ;
        }
    }

  if (err_handler.errCatch())
    {
      zMapLogWarning("Error checking file header: %s", err_handler.errMsg().c_str());
    }

  if (error_ && error_->message)
    err_msg = error_->message ; 

  return result ;
}



#ifdef USE_HTSLIB
/* Read the HTS file header and look for the sequence name data.
 * This assumes that we have already opened the file and called
 * hts_hdr = sam_hdr_read() ;
 * Returns TRUE if reference sequence name found in BAM file header.
 */
bool ZMapDataStreamHTSStruct::checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server)
{
  bool result = false ;
  zMapReturnValIfFail(isOpen(), result) ;

  // Read the HTS header and look a @SQ:<name> tag that matches the sequence
  // we are looking for.
  stringstream available_seqs;

  if (hts_hdr && hts_hdr->n_targets >= 1)
    {
      int i ;

      for (i = 0 ; i < hts_hdr->n_targets ; i++)
        {
          if (g_ascii_strcasecmp(sequence_, hts_hdr->target_name[i]) == 0)
            {
              result = true ;
              break ;
            }
          
          available_seqs << hts_hdr->target_name[i] << SEQ_LIST_SEPARATOR ;
        }
    }

  if (!result)
    {
      g_set_error(&error_, g_quark_from_string("ZMap"), 99,
                  "File header does not contain sequence '%s'. Available sequences in this file are:\n%s\n", 
                  sequence_, available_seqs.str().c_str()) ;
    }

  if (error_ && error_->message)
    err_msg = error_->message ; 

  return result ;
}

bool ZMapDataStreamBCFStruct::checkHeader(std::string &err_msg, bool &empty_or_eof, const bool sequence_server)
{
  bool result = false ;
  zMapReturnValIfFail(isOpen(), result) ;

  // Loop through all sequence names in the file and check for the one we want
  int nseqs = 0 ;
  const char **seqnames = bcf_hdr_seqnames(hts_hdr, &nseqs) ;
  stringstream available_seqs;

  for (int i = 0; i < nseqs; ++i)
    {
      if (strcmp(seqnames[i], sequence_) == 0)
        {
          // Found it. Remember its 0-based index. This is the rid that gets recorded
          // in the bcf parser.
          rid_ = i ;
          result = true ;
          break ;
        }

      available_seqs << seqnames[i] << SEQ_LIST_SEPARATOR ;
    }

  if (!result)
    {
      g_set_error(&error_, g_quark_from_string("ZMap"), 99,
                  "File header does not contain sequence '%s'. Available sequences in this file are:\n%s\n", 
                  sequence_, available_seqs.str().c_str()) ;
    }

  if (error_ && error_->message)
    err_msg = error_->message ; 

  return result ;
}
#endif


/*
 * Read a line from the GIO channel and remove the terminating
 * newline if present. That is,
 * "<string_data>\n" -> "<string_data>"
 * (It is still a NULL terminated c-string though.)
 */
bool ZMapDataStreamGIOStruct::readLine()
{
  bool result = false ;
  GError *pErr = NULL ;
  gsize pos = 0 ;
  GIOStatus cIOStatus = G_IO_STATUS_NORMAL;

  cIOStatus = g_io_channel_read_line_string(io_channel, buffer_line_, &pos, &pErr) ;
  if (cIOStatus == G_IO_STATUS_NORMAL && !pErr )
    {
      result = true ;
      buffer_line_->str[pos] = '\0';
    }

  if (!result)
    end_of_file_ = true ;

  return result ;
}


/*
 * Read a record from a BED file and turn it into GFFv3.
 */
bool ZMapDataStreamBEDStruct::readLine()
{
  bool result = false ;

  zMapReturnValIfFail(bed_features_, result) ;

  // Get the next feature in the list (or the first one if we haven't started yet)
  if (cur_feature_)
    cur_feature_ = cur_feature_->next ;
  else
    cur_feature_ = bed_features_ ;

  if (cur_feature_)
    {
      result = true ;
    }

  if (!result)
    end_of_file_ = true ;
  
  return result ;
}

/*
 * Read a record from a BIGBED file and turn it into GFFv3.
 */
bool ZMapDataStreamBIGBEDStruct::readLine()
{
  bool result = false ;

  BlatLibErrHandler err_handler ;

  // Get the next feature in the list (or the first one if we haven't started yet)
  if (cur_interval_)
    {
      cur_interval_ = cur_interval_->next ;
    }
  else
    {
      if (err_handler.errTry())
        {
          // First line. Initialise the query. 
          lm_ = lmInit(0); // Memory pool to hold returned list
          list_ = bigBedIntervalQuery(bbi_file_, sequence_, start_, end_, 0, lm_);

          cur_interval_ = list_ ;
        }

      if (err_handler.errCatch())
        {
          zMapLogWarning("Error initialising bigBed parser for '%s:%d-%d': %s",
                         sequence_, start_, end_, err_handler.errMsg().c_str());
        }
    }

  if (cur_interval_)
    {
      if (err_handler.errTry())
        {
          // Extract the bed contents out of the "rest" of the line
          char *line = g_strdup_printf("%s\t%d\t%d\t%s", 
                                       sequence_, cur_interval_->start, cur_interval_->end, cur_interval_->rest) ;
          char *row[bedKnownFields];
          const int fields_in_line = chopByWhite(line, row, ArraySize(row)) ;

          // standard_fields_ contains the number of standard fields in the bed file we should
          // read. There may be more fields in the line than this if it includes optional
          // user-defined fields. We ignore these for now.
          // gb10: I don't think there should be fewer than standard_fields_ in the line, but
          // check just in case.
          const int num_fields = min(fields_in_line, standard_fields_) ;

          // free previous feature, if there is one
          if (cur_feature_)
            bedFree(&cur_feature_) ;

          cur_feature_ = bedLoadN(row, num_fields) ;
        
          // Clean up
          g_free(line) ;

          if (cur_feature_)
            result = true ;
        }

      if (err_handler.errCatch())
        {
          zMapLogWarning("Error processing line '%s': %s", 
                         cur_interval_->rest, err_handler.errMsg().c_str());
        }
    }
  else
    {
      if (err_handler.errTry())
        {
          // Got to the end of the list, so clean up the lm (typically do this 
          // after each query)
          lmCleanup(&lm_);
        }

      if (err_handler.errCatch())
        {
          zMapLogWarning("Error cleaning up bigBed parser: %s", err_handler.errMsg().c_str());
        }
    }
          
  if (!result)
    end_of_file_ = true ;

  return result ;
}

/*
 * Read a record from a BIGWIG file and turn it into GFFv3.
 */
bool ZMapDataStreamBIGWIGStruct::readLine()
{
  bool result = false ;

  BlatLibErrHandler err_handler ;

  // Get the next feature in the list (or the first one if we haven't started yet)
  if (cur_interval_)
    {
      cur_interval_ = cur_interval_->next ;
    }
  else
    {
      if (err_handler.errTry())
        {
          // First line. Initialise the query. 
          lm_ = lmInit(0); // Memory pool to hold returned list
          list_ = bigWigIntervalQuery(bbi_file_, sequence_, start_, end_, lm_);

          cur_interval_ = list_ ;
        }

      if (err_handler.errCatch())
        {
          zMapLogWarning("Error initialising bigWig parser for '%s:%d-%d': %s", 
                         sequence_, start_, end_, err_handler.errMsg().c_str());
        }
    }

  if (cur_interval_)
    {
      result = true ;
    }
  else
    {
      if (err_handler.errTry())
        {
          // Got to the end of the list, so clean up the lm (typically do this 
          // after each query)
          lmCleanup(&lm_);
        }

      if (err_handler.errCatch())
        zMapLogWarning("Error cleaning up bigWig parser: %s", err_handler.errMsg().c_str());
    }

  if (!result)
    end_of_file_ = true ;

  return result ;
}

#ifdef USE_HTSLIB
bool ZMapDataStreamHTSStruct::readLine()
{
  bool result = false ;

  if (hts_iter)
    {
      if (bam_itr_next(hts_file, hts_iter, hts_rec) >= 0)
        {
          result = processRead();
        }
    }
  else if (sam_read1(hts_file, hts_hdr, hts_rec) >= 0)
    {
      result = processRead();
    }

  if (!result)
    end_of_file_ = true ;

  return result ;
}

bool ZMapDataStreamBCFStruct::readLine()
{
  bool result = false ;

  int iStart = 0,
    iEnd = 0 ;

  /*
   * Initial error check.
   */
  zMapReturnValIfFail(hts_file && hts_hdr && hts_rec, result ) ;

  /*
   * Read line from HTS file and convert to GFF.
   */
  if ( bcf_read1(hts_file, hts_hdr, hts_rec) >= 0 )
    {
      bcf_unpack(hts_rec, BCF_UN_ALL) ;

      /* Get the ref sequence id and check it's the one we're looking for */
      if (hts_rec->rid == rid_)
        {
          iStart = hts_rec->pos + 1 ;
          iEnd   = iStart + hts_rec->rlen - 1 ;

          const char *name = hts_rec->d.id ;
          if (name && *name == '.')
            name = NULL ;

          /* Construct the ensembl_variation string for the attributes */
          string var_str ;
          bool first = true ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          bool deletion = false ;
          bool insertion = false ;
          bool snv = true ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


          for (int i = 0; i < hts_rec->d.m_allele; ++i)
            {
              // Use '/' separators between the alleles
              if (!first)
                var_str += "/" ;

              // If it's a deletion (a dot), replace it with a dash
              const char *val = hts_rec->d.allele[i] ;

              if (val && *val == '.')
                {
                  val = "-" ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                  if (first)
                    insertion = true ;
                  else 
                    deletion = true ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

                }

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
              else if (strlen(val) > 1)
                {
                  snv = false ;
                }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


              var_str += val ;
              
              first = false ;
            }



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          // Unused, not sure what intention was.
          const char *so_term = "SNV" ;
          if (insertion)
            so_term = "insertion" ;
          else if (deletion)
            so_term = "deletion" ;
          else if (!snv)
            so_term = "substitution" ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


          cur_feature_data_ = ZMapDataStreamFeatureData(iStart, iEnd, 0.0, '.', '.', name, 0, 0) ;

          result = true ;
        }
    }

  if (!result)
    end_of_file_ = true ;

  return result ;
}

#endif


/*
 * Parse a header line from the source. Returns true if successful. Sets done_header if there are
 * no more header lines to read.
 */
bool ZMapDataStreamGIOStruct::parseHeader(gboolean &done_header,
                                          ZMapGFFHeaderState &header_state,
                                          GError **error)
{
  bool result = zMapGFFParseHeader(parser_, buffer_line_->str, &done_header, &header_state) ;

  if (!result && error)
    {
      *error = zMapGFFGetError(parser_) ;
    }

  return result ;
}


/*
 * Parse sequence data from the source. Returns true if successful. Sets sequence_finished if
 * there is no more sequence to read.
 */
bool ZMapDataStreamStruct::parseSequence(gboolean &sequence_finished, string &err_msg)
{
  // Base class does nothing
  bool result = true ;
  return result ;
}

bool ZMapDataStreamGIOStruct::parseSequence(gboolean &sequence_finished, string &err_msg)
{
  bool result = true ;

  bool more_data = true ;
  GError *error = NULL ;

  do
    {
      if (!zMapGFFParseSequence(parser_, buffer_line_->str, &sequence_finished) || sequence_finished)
        break ;

      more_data = readLine() ;

    } while (more_data) ;

  error = zMapGFFGetError(parser_) ;

  if (error)
    {
      /* If the error was serious we stop processing and return the error,
       * otherwise we just log the error. */
      if (terminated() && error->message)
        {
          err_msg = error->message ;
        }
     else
       {
         stringstream err_ss ;
         err_ss << "zMapGFFParseSequence() failed for line " 
                << curLineNumber() << ": " << curLine() << " - " << error->message ;
         err_msg = err_ss.str() ;
        }

      result = false ;
    }
  else if (!sequence_finished)
    {
      result = false ;
    }

  return result ;
}

/*
 * This sets some things up for the GFF parser and is required before calling parserBodyLine
 */
void ZMapDataStreamStruct::parserInit(GHashTable *featureset_2_column,
                                      GHashTable *source_2_sourcedata,
                                      ZMapStyleTree *styles)
{
  featureset_2_column_ = featureset_2_column ;
  source_2_sourcedata_ = source_2_sourcedata ;
  styles_ = styles ;
}

void ZMapDataStreamGIOStruct::parserInit(GHashTable *featureset_2_column,
                                         GHashTable *source_2_sourcedata,
                                         ZMapStyleTree *styles)
{
  featureset_2_column_ = featureset_2_column ;
  source_2_sourcedata_ = source_2_sourcedata ;
  styles_ = styles ;

  zMapGFFParseSetSourceHash(parser_, featureset_2_column, source_2_sourcedata) ;
  zMapGFFParserInitForFeatures(parser_, styles, FALSE) ;
  zMapGFFSetDefaultToBasic(parser_, TRUE);
}

ZMapFeatureSet ZMapDataStreamStruct::makeFeatureSet(const char *feature_name_id,
                                                    ZMapStyleMode feature_mode,
                                                    const bool is_seq)
{
  ZMapFeatureSet feature_set = NULL ;

  /*
   * Now deal with the source -> data mapping referred to in the parser.
   */
  GQuark source_id = zMapStyleCreateIDFromID(source_->name_) ;
  GQuark feature_style_id = 0 ;
  ZMapFeatureSource source_data = NULL ;

  if (source_2_sourcedata_)
    {
      if (!(source_data = (ZMapFeatureSource)g_hash_table_lookup(source_2_sourcedata_, GINT_TO_POINTER(source_id))))
        {
          source_data = g_new0(ZMapFeatureSourceStruct,1);
          source_data->source_id = source_id;
          source_data->source_text = source_->name_ ;
          source_data->is_seq = is_seq ;

          g_hash_table_insert(source_2_sourcedata_,GINT_TO_POINTER(source_id), source_data);

          //zMapLogMessage("Created source_data: %s", g_quark_to_string(source_id)) ;
        }

      if (source_data->style_id)
        feature_style_id = zMapStyleCreateID((char *) g_quark_to_string(source_data->style_id)) ;
      else
        feature_style_id = zMapStyleCreateID((char *) g_quark_to_string(source_data->source_id)) ;

      source_id = source_data->source_id ;
      source_data->style_id = feature_style_id;
      //zMapLogMessage("Style id = %s", g_quark_to_string(source_data->style_id)) ;
    }
  else
    {
      source_id = feature_style_id = zMapStyleCreateID((char*)g_quark_to_string(source_->name_)) ;
    }

  ZMapFeatureTypeStyle feature_style = NULL ;

  feature_style = zMapFindFeatureStyle(styles_, feature_style_id, feature_mode) ;

  if (feature_style)
    {
      if (source_data)
        source_data->style_id = feature_style_id;

      styles_->add_style(feature_style) ;

      if (source_data && feature_style->unique_id != feature_style_id)
        source_data->style_id = feature_style->unique_id;

      feature_set = zMapFeatureSetCreate((char*)g_quark_to_string(source_->name_) , NULL, source_) ;

      zMapLogMessage("Created feature set: %s", g_quark_to_string(source_->name_)) ;

      feature_set->style = feature_style;
    }
  else
    {
      zMapLogWarning("Error creating feature %s (%s); no feature style found for %s",
                     feature_name_id, g_quark_to_string(source_->name_), g_quark_to_string(feature_style_id)) ;
    }

  return feature_set ;
}


// Utility to create a feature from a BED struct
ZMapFeature ZMapDataStreamStruct::makeBEDFeature(struct bed *bed_feature, 
                                                 const int standard_fields,
                                                 const char *so_term,
                                                 GError **error)
{
  ZMapStyleMode style = ZMAPSTYLE_MODE_BASIC ;

  // BED files with 12 fields represent transcripts
  if (standard_fields == 12)
    {
      so_term = "transcript" ;
      style = ZMAPSTYLE_MODE_TRANSCRIPT ;
    }

  ZMapFeature feature = makeFeature(bed_feature->chrom,
                                    so_term,
                                    bed_feature->chromStart + 1, //0-based
                                    bed_feature->chromEnd,       //chromEnd is one past the last coord
                                    bed_feature->score,
                                    bed_feature->strand[0],
                                    bed_feature->name,
                                    true,
                                    1,
                                    bed_feature->chromEnd - bed_feature->chromStart + 1,
                                    '.',
                                    NULL,
                                    style,
                                    false,
                                    error) ;

  if (feature && style == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      zMapFeatureTranscriptInit(feature);
      zMapFeatureAddTranscriptStartEnd(feature, FALSE, 0, FALSE);

      // If there is no CDS, the thickStart and thickEnd are usually set to chromStart
      if (bed_feature->thickEnd != bed_feature->thickStart)
        zMapFeatureAddTranscriptCDS(feature, TRUE, bed_feature->thickStart, bed_feature->thickEnd);

      for (unsigned int i = 0; i < bed_feature->blockCount; ++i)
        {
          const int exon_start = bed_feature->chromStarts[i] + bed_feature->chromStart ;
          const int exon_end = exon_start + bed_feature->blockSizes[i] ;
          ZMapSpanStruct span = {exon_start, exon_end} ;

          zMapFeatureAddTranscriptExonIntron(feature, &span, NULL) ;
        }

      zMapFeatureTranscriptRecreateIntrons(feature) ;
    }

  return feature ;
}


ZMapFeature ZMapDataStreamStruct::makeFeature(const char *sequence,
                                              const char *so_type,
                                              const int start,
                                              const int end,
                                              const double score,
                                              const char strand_c,
                                              const char *feature_name_in,
                                              const bool have_target,
                                              const int target_start,
                                              const int target_end,
                                              const char target_strand_c,
                                              const char *cigar_string,
                                              ZMapStyleMode feature_mode,
                                              const bool is_seq,
                                              GError **error)
{
  ZMapFeature feature = NULL ;
  zMapReturnValIfFail(sequence && so_type, feature) ;

  bool ok = true ;
  ZMapStrand strand = ZMAPSTRAND_NONE ;
  ZMapStrand target_strand = ZMAPSTRAND_NONE ;
  const char *feature_name = feature_name_in ;

  if (!feature_name)
    feature_name = sequence ;

  if (strand_c == '+')
    strand = ZMAPSTRAND_FORWARD ;
  else if (strand_c == '-')
    strand = ZMAPSTRAND_REVERSE ;

  if (target_strand_c == '+')
    target_strand = ZMAPSTRAND_FORWARD ;
  else if (strand_c == '-')
    target_strand = ZMAPSTRAND_REVERSE ;

  GQuark unique_id = zMapFeatureCreateID(feature_mode,
                                         feature_name,
                                         strand,
                                         start,
                                         end,
                                         target_start,
                                         target_end) ;

  // Create the feature set, if not already created
  if (ok && !feature_set_)
    {
      feature_set_ = makeFeatureSet(feature_name,
                                    feature_mode,
                                    is_seq) ;

      if (!feature_set_)
        ok = false ;
    }

  // Ok, go ahead and create the feature
  if (ok)
    {
      feature = zMapFeatureCreateEmpty(error) ;

      if (!feature || (error && *error))
        ok = false ;
    }

  if (ok)
    {
      ok = zMapFeatureAddStandardData(feature, g_quark_to_string(unique_id), feature_name, 
                                      sequence, so_type, feature_mode,
                                      NULL, start, end, true, score, strand);
    }

  GArray *gaps = NULL ;

  if (ok && cigar_string && *cigar_string)
    {
      ok = zMapFeatureAlignmentString2Gaps(ZMAPALIGN_FORMAT_CIGAR_BAM,
                                           strand, start, end,
                                           target_strand, target_start, target_end,
                                           (char*)cigar_string, &gaps) ;
    }

  if (ok && feature && feature->mode == ZMAPSTYLE_MODE_ALIGNMENT)
    {
      int length = (target_end - target_start + 1) ;

      ok = zMapFeatureAddAlignmentData(feature, g_quark_from_string(feature_name), score,
                                       target_start, target_end, ZMAPHOMOL_N_HOMOL, length, 
                                       target_strand, ZMAPPHASE_0, gaps, 
                                       zMapStyleGetWithinAlignError(feature_set_->style), 
                                       FALSE, NULL) ;

    }

  if (ok)
    {
      zMapFeatureSetAddFeature(feature_set_, feature);
    }


  if (ok)
    {          
      ++num_features_ ;
    }
  else if (error && *error == NULL)
    {
      g_set_error(error, g_quark_from_string("ZMap"), 99, 
                  "Error creating feature: %s (%d %d) on sequence %s", 
                  feature_name, start, end, sequence) ;
    }

  return feature ;
}


/*
 * Parse a body line from the source. Returns true if successful.
 */
bool ZMapDataStreamGIOStruct::parseBodyLine(GError **error)
{
  // The buffer line has already been read by the functions that read the header etc. so parse it
  // first and then read the next line ready for next time.
  bool result = zMapGFFParseLine(parser_, buffer_line_->str) ;

  if (!result && error)
    {
      // the caller takes ownership of the error, so make a copy of the error in the gff parser
      *error = g_error_copy(zMapGFFGetError(parser_)) ;
    }

  readLine() ;

  return result ;
}


bool ZMapDataStreamBEDStruct::parseBodyLine(GError **error)
{
  bool result = true ;

  if (readLine())
    {
      ZMapFeature feature = makeBEDFeature(cur_feature_, standard_fields_, ZMAP_BED_SO_TERM, error) ;

      if (!feature)
        result = false ;
    }

  return result ;
}

bool ZMapDataStreamBIGBEDStruct::parseBodyLine(GError **error)
{
  bool result = true ;

  if (readLine())
    {
      ZMapFeature feature = makeBEDFeature(cur_feature_, standard_fields_, ZMAP_BIGBED_SO_TERM, error) ;

      if (!feature)
        result = false ;
    }

  return result ;
}

bool ZMapDataStreamBIGWIGStruct::parseBodyLine(GError **error)
{
  bool result = true ;

  if (readLine())
    {
      ZMapFeature feature = makeFeature(sequence_,
                                        ZMAP_BIGWIG_SO_TERM,
                                        cur_interval_->start,
                                        cur_interval_->end,
                                        cur_interval_->val,
                                        '.',
                                        NULL,
                                        false,
                                        0,
                                        0,
                                        '.',
                                        NULL,
                                        ZMAPSTYLE_MODE_GRAPH,
                                        true,
                                        error) ;

      if (!feature)
        result = false ;
    }

  return result ;
}

#ifdef USE_HTSLIB
bool ZMapDataStreamHTSStruct::parseBodyLine(GError **error)
{
  bool result = true ;

  if (readLine())
    {
      //bool ok = loadAlignString(parser_base, ZMAPALIGN_FORMAT_CIGAR_BAM,
      //                          gaps_onwards, &gaps,
      //                          strand, start, end,
      //                          query_strand, query_start, query_end) ;

      ZMapFeature feature = makeFeature(sequence_,
                                        ZMAP_BAM_SO_TERM,
                                        cur_feature_data_.start_,
                                        cur_feature_data_.end_,
                                        cur_feature_data_.score_,
                                        cur_feature_data_.strand_c_,
                                        cur_feature_data_.target_name_.c_str(),
                                        !cur_feature_data_.target_name_.empty(),
                                        cur_feature_data_.target_start_,
                                        cur_feature_data_.target_end_,
                                        cur_feature_data_.target_strand_c_,
                                        cur_feature_data_.cigar_.c_str(),
                                        ZMAPSTYLE_MODE_ALIGNMENT,
                                        true,
                                        error) ;

      if (!feature)
        result = false ;
    }

  return result ;
}

bool ZMapDataStreamBCFStruct::parseBodyLine(GError **error)
{
  bool result = true ;

  if (readLine())
    {
      ZMapFeature feature = makeFeature(sequence_,
                                        ZMAP_BCF_SO_TERM,
                                        cur_feature_data_.start_,
                                        cur_feature_data_.end_,
                                        cur_feature_data_.score_,
                                        cur_feature_data_.strand_c_,
                                        cur_feature_data_.target_name_.c_str(),
                                        !cur_feature_data_.target_name_.empty(),
                                        cur_feature_data_.target_start_,
                                        cur_feature_data_.target_end_,
                                        cur_feature_data_.target_strand_c_,
                                        cur_feature_data_.cigar_.c_str(), 
                                        ZMAPSTYLE_MODE_ALIGNMENT,
                                        false,
                                        error) ;

      if (!feature)
        result = false ;
    }

  return result ;
}
#endif // USE_HTSLIB



/*
 * This should be called after parsing is finished. It adds the features that were parsed
 * into the given block.
 */
bool ZMapDataStreamStruct::addFeaturesToBlock(ZMapFeatureBlock feature_block)
{
  if (feature_set_)
    zMapFeatureBlockAddFeatureSet(feature_block, feature_set_);

  return true ;
}

bool ZMapDataStreamGIOStruct::addFeaturesToBlock(ZMapFeatureBlock feature_block)
{
  bool result = zMapGFFGetFeatures(parser_, feature_block) ;

  /* Make sure parser does _not_ free our data ! */
  if (result)
    zMapGFFSetFreeOnDestroy(parser_, FALSE) ;

  return result ;
}


/*
 * This validates the number of features that were found and the length of sequence etc.
 */
bool ZMapDataStreamStruct::checkFeatureCount(bool &empty, 
                                             string &err_msg)
{
  bool result = true ;

  int num_features = zMapGFFParserGetNumFeatures(parser_) ;
  int n_lines_bod = zMapGFFGetLineBod(parser_) ;
  int n_lines_fas = zMapGFFGetLineFas(parser_) ;
  int n_lines_seq = zMapGFFGetLineSeq(parser_) ;

  if (!num_features)
    {
      err_msg = "No features found." ;
    }

  if (!num_features && !n_lines_bod && !n_lines_fas && !n_lines_seq)
    {
      // Return true to preserve old behaviour for calling function
      result = true ;
    }
  else if ((!num_features && n_lines_bod)
           || (n_lines_fas && !zMapGFFGetSequenceNum(parser_))
           || (n_lines_seq && !zMapGFFGetSequenceNum(parser_) && zMapGFFGetVersion(parser_) == ZMAPGFF_VERSION_2)  )
    {
      // Unexpected error
      result = false ;
    }

  return result ;
}


/*
 * This returns the list of featureset names that were found in the file.
 */
GList* ZMapDataStreamStruct::getFeaturesets()
{
  return zMapGFFGetFeaturesets(parser_) ;
}

/*
 * This returns the dna/peptide sequence that was parsed from the file, if it contained any
 */
ZMapSequence ZMapDataStreamStruct::getSequence(GQuark seq_id, 
                                               GError **error)
{
  ZMapSequence result = zMapGFFGetSequence(parser_, seq_id) ;

  if (!result && error)
    {
      *error = zMapGFFGetError(parser_);
    }

  return result ;
}

/*
 * Utility to return the current line number
 */
int ZMapDataStreamGIOStruct::curLineNumber()
{
  return zMapGFFGetLineNumber(parser_) ;
}


/*
 * Utility to return the current line string
 */
const char* ZMapDataStreamGIOStruct::curLine()
{
  return buffer_line_->str ;
}


/*
 * Returns true if the parser hit the end of the file
 */
bool ZMapDataStreamStruct::endOfFile()
{
  return end_of_file_ ;
}


/*
 * Returns true if the parser quit with a fatal error
 */
bool ZMapDataStreamStruct::terminated()
{
  return zMapGFFTerminated(parser_) ;
}


/* Functions to do any initialisation required at the start of each block of reads */
gboolean ZMapDataStreamStruct::init(const char *region_name, int start, int end)
{
  // base class does nothing
  return TRUE ;
}


#ifdef USE_HTSLIB
gboolean ZMapDataStreamHTSStruct::init(const char *region_name, int start, int end)
{
  gboolean result = FALSE;
  int begRange;
  int endRange;
  char region[128];
  int ref;

  ref = bam_name2id(hts_hdr, region_name);

  if (ref >= 0)
    {
      sprintf(region,"%s:%i-%i", region_name, start, end);
      if (hts_parse_reg(region, &begRange, &endRange) != NULL)
        {
          if (hts_iter)
            {
              if (ref != hts_iter->tid || begRange != hts_iter->beg || endRange != hts_iter->end)
                {
                  hts_itr_destroy(hts_iter);
                  hts_iter = NULL ;
                }
              else
                {
                  result = TRUE;
                }
            }

          if (!result && hts_idx)
            {
              hts_iter = sam_itr_queryi(hts_idx, ref, begRange, endRange);
              result = TRUE;
            }
        }
      else
        {
          fprintf(stderr, "Could not parse %s\n", region);
        }
    }
  else
    {
      fprintf(stderr, "Invalid region %s\n", region_name);
    }

  return result;
}


/*
 * Read a record from a HTS file and store it as the current_feature_data_
 */
bool ZMapDataStreamHTSStruct::processRead()
{
  gboolean result = FALSE ;
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
  stringstream ssCigar ;
  const char *query_name = NULL ;

  /*
   * Initial error check.
   */
  zMapReturnValIfFail(hts_file && hts_hdr && hts_rec, result ) ;


  /*
   * Read line from HTS file and convert to GFF.
   */
  /*
   * start, end, score and strand
   */
  iStart = hts_rec->core.pos+1;
  iEnd   = iStart;
  dScore = (double) hts_rec->core.qual ;
  cStrand = '+' ;

  /*
   * "cigar_bam" attribute
   */
  nCigar = hts_rec->core.n_cigar ;
  if (nCigar)
    {
      pCigar = bam_get_cigar(hts_rec) ;

      for (iCigar=0; iCigar<nCigar; ++iCigar)
        {
          ssCigar << bam_cigar_oplen(pCigar[iCigar]) << bam_cigar_opchr(pCigar[iCigar]) ;
          iEnd += bam_cigar_oplen(pCigar[iCigar]) ;
        }
    }

  if (iEnd == iStart)
    iEnd += hts_rec->core.l_qseq-1 ;

  /*
   * "Target" (and "Name") attribute
   */
  iTargetStart = 1 ;
  iTargetEnd = hts_rec->core.l_qseq ;
  query_name = bam_get_qname(hts_rec) ;

  if (query_name && strlen(query_name) > 0)
    {
      cTargetStrand = '+' ;
    }

  cur_feature_data_ = ZMapDataStreamFeatureData(iStart, iEnd, dScore, cStrand, cPhase, 
                                                query_name, iTargetStart, iTargetEnd, cTargetStrand,
                                                ssCigar.str().c_str(), ZMAPALIGN_FORMAT_CIGAR_BAM) ;

  /*
   * return value
   */
  result = TRUE ;

  return result ;
}

#endif //HTSLIB


/*
 * Create a ZMapDataStream object
 */
ZMapDataStream zMapDataStreamCreate(ZMapConfigSource source,
                                    const char * const file_name, 
                                    const char *sequence,
                                    const int start,
                                    const int end,
                                    GError **error_out)
{
  static const char * open_mode = "r" ;
  ZMapDataStream data_source = NULL ;
  ZMapDataStreamType stream_type = ZMapDataStreamType::UNK ;
  GError *error = NULL ;
  zMapReturnValIfFail(source && file_name && *file_name, data_source ) ;

  // If the file type is known for this source, use that for the stream_type. Otherwise, try to
  // determine the file type from the filename.
  if (!source->fileType().empty())
    stream_type = zMapDataStreamTypeFromFileType(source->fileType().c_str(), &error) ;
  else
    stream_type = zMapDataStreamTypeFromFilename(file_name, &error) ;

  if (!error && stream_type != ZMapDataStreamType::UNK)
    {
      switch (stream_type)
        {
        case ZMapDataStreamType::GIO:
          data_source = new ZMapDataStreamGIOStruct(source, file_name, open_mode, sequence, start, end) ;
          break ;
        case ZMapDataStreamType::BED:
          data_source = new ZMapDataStreamBEDStruct(source, file_name, open_mode, sequence, start, end) ;
          break ;
        case ZMapDataStreamType::BIGBED:
          data_source = new ZMapDataStreamBIGBEDStruct(source, file_name, open_mode, sequence, start, end) ;
          break ;
        case ZMapDataStreamType::BIGWIG:
          data_source = new ZMapDataStreamBIGWIGStruct(source, file_name, open_mode, sequence, start, end) ;
          break ;
#ifdef USE_HTSLIB
        case ZMapDataStreamType::HTS:
          data_source = new ZMapDataStreamHTSStruct(source, file_name, open_mode, sequence, start, end) ;
          break ;
        case ZMapDataStreamType::BCF:
          data_source = new ZMapDataStreamBCFStruct(source, file_name, open_mode, sequence, start, end) ;
          break ;
#endif
        default:
          zMapWarnIfReached() ;
          g_set_error(&error, g_quark_from_string("ZMap"), 99,
                      "Unexpected data source type for file '%s'", file_name) ;
          break ;
        }
    }
  else
    {
      g_set_error(&error, ZMAP_SERVER_ERROR, ZMAPSERVER_ERROR_UNKNOWN_TYPE,
                  "Invalid file type '%s' for file '%s'", source->fileType().c_str(), file_name) ;
    }

  if (data_source)
    {
      if (data_source->error() && !error)
        g_propagate_error(&error, data_source->error()) ;

      if (!data_source->isOpen())
        {
          delete data_source ;
          data_source = NULL ;
        }
    }

  if (error)
    g_propagate_error(error_out, error) ;

  return data_source ;
}


/*
 * Destroy the file object.
 */
bool zMapDataStreamDestroy( ZMapDataStream *p_data_source )
{
  bool result = false ;

  if (p_data_source && *p_data_source)
    {
      delete *p_data_source ;
      *p_data_source = NULL ;
      result = true ;
    }

  return result ;
}


/*
 * Checks that the data source is open.
 */
bool zMapDataStreamIsOpen(ZMapDataStream const source)
{
  gboolean result = false ;

  if (source)
    result = source->isOpen() ;

  return result ;
}


/*
 * Return the type of the object
 */
ZMapDataStreamType zMapDataStreamGetType(ZMapDataStream data_source )
{
  zMapReturnValIfFail(data_source, ZMapDataStreamType::UNK ) ;
  return data_source->type ;
}


/*
 * Inspect the filename string (might include the path on the front, but this is
 * (ignored) for the extension to determine type:
 *
 *       *.gff                            ZMapDataStreamType::GIO
 *       *.bed                            ZMapDataStreamType::BED
 *       *.[bb,bigBed]                    ZMapDataStreamType::BIGBED
 *       *.[bw,bigWig]                    ZMapDataStreamType::BIGWIG
 *       *.[sam,bam,cram]                 ZMapDataStreamType::HTS
 *       *.[bcf,vcf]                      ZMapDataStreamType::BCF
 *       *.<everything_else>              ZMapDataStreamType::UNK
 */
ZMapDataStreamType zMapDataStreamTypeFromFilename(const char * const file_name, GError **error_out)
{
  static const char dot = '.' ;
  ZMapDataStreamType type = ZMapDataStreamType::UNK ;
  GError *error = NULL ;
  char * pos = NULL ;
  char *tmp = (char*) file_name ;
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
      string file_ext(pos) ;
      type = dataSourceTypeFromExtension(file_ext, &error) ;
    }
  else
    {
      g_set_error(&error, ZMAP_SERVER_ERROR, ZMAPSERVER_ERROR_UNKNOWN_EXTENSION,
                  "File name does not have an extension so cannot determine type: '%s'",
                  file_name) ;
    }

  if (error)
    g_propagate_error(error_out, error) ;

  return type ;
}


/* Determine the file-type string from the given stream type. The file-type string is a list of
 * all file types for the data stream type, e.g. for HTS, the file-type is "BAM/SAM/CRAM" */
string zMapDataStreamTypeToFileType(ZMapDataStreamType &stream_type)
{
  string file_type ;

  // Find all file types for this source type
  for (auto &iter : file_type_to_stream_type_G)
    {
      string file_type = iter.first ;
      
      if (iter.second == stream_type)
        {
          // Append them as "/"-separated string
          if (!file_type.empty())
            file_type += "/" ;
          
          file_type += iter.first ;
        }
    }
  
  return file_type ;
}


/* Determine the source type from the given file-type string. This might be the inverse of
 * zMapDataStreamTypeToFileType (e.g. "BAM/SAM/CRAM"), or a file type string such as "SAM" or
 * "bigWig 9" that we need to find a match for. */
ZMapDataStreamType zMapDataStreamTypeFromFileType(const string &file_type, GError **error_out)
{
  ZMapDataStreamType result = ZMapDataStreamType::UNK ;

  int found_match_len = 0 ;

  // Loop through all file types
  for (auto &iter : file_type_to_stream_type_G)
    {
      string cur_file_type = iter.first ;
      ZMapDataStreamType cur_stream_type = iter.second ;

      // First, construct the composite file-type string for the current source, and see if it
      // matches the given file-type
      string cur_composite_file_type = zMapDataStreamTypeToFileType(cur_stream_type) ;

      if (cur_composite_file_type == file_type)
        {
          result = cur_stream_type ;
        }
      else
        {
          // Convert to lowercase for case-insensitive search
          string file_type_l = toLower(file_type) ;

          // See if the file-type string contains this file type e.g. a file-type string of "BAM/SAM/CRAM"
          // would contain the "SAM" file type
          string cur_file_type_l = toLower(cur_file_type) ;
          size_t found_pos = file_type_l.find(cur_file_type_l) ;

            if (found_pos != string::npos)
              {
                int cur_match_len = cur_file_type_l.length() ;

                // If there is more than one match, prefer the longest one, e.g. if we match "bed"
                // and "bigbed" then the result is "bigbed".
                if (result == ZMapDataStreamType::UNK || cur_match_len > found_match_len)
                  {
                    result = cur_stream_type ;
                    found_match_len = cur_match_len ;
                  }
              }
          }
      }

  if (result == ZMapDataStreamType::UNK)
    {
      g_set_error(error_out, ZMAP_SERVER_ERROR, ZMAPSERVER_ERROR_UNKNOWN_EXTENSION,
                  "Unknown file type '%s'", file_type.c_str()) ;
    }

  return result ;
}



/*
 *              Internal routines.
 */

namespace // unnamed namespace
{

inline semaphore::semaphore(size_t n) : count{n} {}

inline void semaphore::notify() {
    std::lock_guard<std::mutex> lock{mutex};
    ++count;
    cv.notify_one();
}

inline void semaphore::wait() {
    std::unique_lock<std::mutex> lock{mutex};
    cv.wait(lock, [&]{ return count > 0; });
    --count;
}

inline bool semaphore::try_wait() {
    std::lock_guard<std::mutex> lock{mutex};

    if (count > 0) {
        --count;
        return true;
    }

    return false;
}

template<class Rep, class Period>
bool semaphore::wait_for(const std::chrono::duration<Rep, Period>& d) {
    std::unique_lock<std::mutex> lock{mutex};
    auto finished = cv.wait_for(lock, d, [&]{ return count > 0; });

    if (finished)
        --count;

    return finished;
}

template<class Clock, class Duration>
bool semaphore::wait_until(const std::chrono::time_point<Clock, Duration>& t) {
    std::unique_lock<std::mutex> lock{mutex};
    auto finished = cv.wait_until(lock, t, [&]{ return count > 0; });

    if (finished)
        --count;

    return finished;
}

inline semaphore::native_handle_type semaphore::native_handle() {
    return cv.native_handle();
}


BlatLibErrHandler::BlatLibErrHandler()
{
}

BlatLibErrHandler::~BlatLibErrHandler()
{
  if (err_catch_)
    {
      errCatchFree(&err_catch_) ;
      err_catch_ = NULL ;
    }
}

// Initialises the handler. Returns true if ok. Must be called before errCatch. There must be one
// call to errCatch for each call to errTry.
bool BlatLibErrHandler::errTry()
{
  bool result = false ;

  if (err_catch_)
    errCatchFree(&err_catch_) ;

  err_catch_ = errCatchNew() ;

  if (err_catch_)
    result = errCatchStart(err_catch_) ;

  return result ;
}

// Ends the handler. Returns true if there was an error. Must be called after errTry. There must
// be one call to errCatch for each call to errTry.
bool BlatLibErrHandler::errCatch()
{
  bool result = false ;

  if (err_catch_)
    {
      errCatchEnd(err_catch_) ;
      result = err_catch_->gotError ;
    }

  return result ;
}

// Returns the error message (empty string if no error)
string BlatLibErrHandler::errMsg()
{
  string err_msg ;

  if (err_catch_ && err_catch_->gotError && 
      err_catch_->message && err_catch_->message->string)
    {
      err_msg = err_catch_->message->string ;
    }

  return err_msg ;
}



// Utility to convert a std::string to lowercase
static string toLower(const string &s)
{
  string result(s) ;
  
  for (unsigned int i = 0; i < s.length(); ++i)
    result[i] = std::tolower(result[i]) ;

  return result ;
}


/* Determine the source type from the given file extension (e.g. "bigWig") */
static ZMapDataStreamType dataSourceTypeFromExtension(const string &file_ext, GError **error_out)
{
  ZMapDataStreamType type = ZMapDataStreamType::UNK ;

  static const map<string, ZMapDataStreamType, caseInsensitiveCmp> file_extensions = 
    {
      {"gff",     ZMapDataStreamType::GIO}
      ,{"gff3",   ZMapDataStreamType::GIO}
      ,{"bed",    ZMapDataStreamType::BED}
      ,{"bb",     ZMapDataStreamType::BIGBED}
      ,{"bigBed", ZMapDataStreamType::BIGBED}
      ,{"bw",     ZMapDataStreamType::BIGWIG}
      ,{"bigWig", ZMapDataStreamType::BIGWIG}
#ifdef USE_HTSLIB
      ,{"sam",    ZMapDataStreamType::HTS}
      ,{"bam",    ZMapDataStreamType::HTS}
      ,{"cram",   ZMapDataStreamType::HTS}
      ,{"vcf",    ZMapDataStreamType::BCF}
      ,{"bcf",    ZMapDataStreamType::BCF}
#endif
    };

  auto found_iter = file_extensions.find(file_ext) ;
      
  if (found_iter != file_extensions.end())
    {
      type = found_iter->second ;
    }
  else
    {
      string expected("");
      for (auto iter = file_extensions.begin(); iter != file_extensions.end(); ++iter)
        expected += " ." + iter->first ;

      g_set_error(error_out, ZMAP_SERVER_ERROR, ZMAPSERVER_ERROR_UNKNOWN_EXTENSION,
                  "Unrecognised file extension .'%s'. Expected one of:%s", 
                  file_ext.c_str(), expected.c_str()) ;
    }

  return type ;
}


} // unnamed namespace
