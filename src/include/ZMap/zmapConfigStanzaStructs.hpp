/*  File: zmapConfigStanzaStructs.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * Description:
 *
 *-------------------------------------------------------------------
 */

#ifndef ZMAPCONFIGSTANZASTRUCTS_H
#define ZMAPCONFIGSTANZASTRUCTS_H

#include <list>
#include <string>

#include <ZMap/zmapUrl.hpp>


/* We should convert this to use the same calls/mechanism as the keyvalue stuff below. */
class ZMapConfigSourceStruct
{
public:
  ZMapConfigSourceStruct() ;
  ~ZMapConfigSourceStruct() ;

  void setUrl(const char *url) ;
  void setConfigFile(const char *config_file) ;
  void setFileType(const std::string &file_type) ;
  void setNumFields(const int num_fields) ;

  const char* url() const ;
  const ZMapURL urlObj() const ;
  const std::string urlError() const ;
  const char* configFile() const ;
  const std::string fileType() const ;
  int numFields() const ;
  std::string type() const ;
  std::string toplevelName() const ;
  void countSources(unsigned int &num_total, unsigned int &num_with_data, unsigned int &num_to_load, const bool recent = false) const ;


  GQuark name_{0} ;
  char *version{NULL} ;
  char *featuresets{NULL}; //, *navigatorsets ;
  char *biotypes{NULL};
//  char *styles_list{NULL};  not used,. pointless
  char *stylesfile{NULL} ;
  char *format{NULL} ;
  int timeout{0} ;
  gboolean delayed{FALSE} ; // if true, don't load this source on start up
  gboolean provide_mapping{FALSE};
  gboolean req_styles{FALSE};
  int group{0};
  bool recent{false};

  ZMapConfigSourceStruct* parent{NULL} ;

  std::list<ZMapConfigSourceStruct*> children ;

#define SOURCE_GROUP_NEVER    0     // these are bitfields, and correspond to the obvious strings
#define SOURCE_GROUP_START    1
#define SOURCE_GROUP_DELAYED  2
#define SOURCE_GROUP_ALWAYS   3

  char *url_{NULL} ; // should be private really but still used by source_set_property

private:
  mutable ZMapURL url_obj_{NULL} ;    // lazy-evaluated parsed version of the url_
  mutable int url_parse_error_{0} ;   // gets set to non-zero parsing url_obj_ failed
  GQuark config_file_{0} ;
  std::string file_type_ ;            // describes file type e.g. "bigBed". Empty if not applicable or unknown.
  unsigned int num_fields_{0} ;       // for file sources, number of fields. 0 if not applicable or unknown.
} ;

typedef ZMapConfigSourceStruct *ZMapConfigSource ;

typedef enum {ZMAPCONF_INVALID, ZMAPCONF_BOOLEAN, ZMAPCONF_INT, ZMAPCONF_DOUBLE,
	      ZMAPCONF_STR,  ZMAPCONF_STR_ARRAY} ZMapKeyValueType ;
typedef enum {ZMAPCONV_INVALID, ZMAPCONV_NONE, ZMAPCONV_STR2ENUM, ZMAPCONV_STR2COLOUR} ZMapKeyValueConv  ;

typedef int (*ZMapConfStr2EnumFunc)(const char *str) ;

typedef struct ZMapKeyValueStructID
{
  const char *name ;
  gboolean has_value ;

  ZMapKeyValueType type ;
  union
  {
    gboolean b ;
    int i ;
    double d ;
    char *str ;
    char **str_array ;
  } data ;

  ZMapKeyValueConv conv_type ;
  union
  {
    ZMapConfStr2EnumFunc str2enum ;
  } conv_func ;

} ZMapKeyValueStruct, *ZMapKeyValue ;


#endif /* ZMAPCONFIGSTANZASTRUCTS_H */
