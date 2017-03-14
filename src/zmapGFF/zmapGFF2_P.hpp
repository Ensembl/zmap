/*  File: zmapGFF2_P.h
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
 * Description: Private header file for GFF2-specific functions and
 * definitions (enumerations, etc).
 *
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_GFF2_P_H
#define ZMAP_GFF2_P_H


#include <ZMap/zmapGFF.hpp>

/*
 * The main parser struct, representing an instance of a parser.
 */
typedef struct ZMapGFF2ParserStruct_
{
  /*
   * Data common to both versions.
   */
  ZMAPGFF_PARSER_STRUCT_COMMON_DATA


  /*
   * Data for V2 only.
   */
  ZMapGFFHeaderState header_state ;

  struct
  {
    unsigned int done_header : 1 ;
    unsigned int got_gff_version : 1 ;
    unsigned int got_sequence_region : 1 ;
  } header_flags ;

  struct
  {
    unsigned int done_start : 1 ;
    unsigned int in_sequence_block : 1 ;
    unsigned int done_finished :1 ;
  } sequence_flags ;

} ZMapGFF2ParserStruct, *ZMapGFF2Parser ;



/*
 * Code specific to this version of the parser only.
 */
ZMapGFFParser zMapGFFCreateParser_V2(const char *sequence, int features_start, int features_end, ZMapConfigSource source) ;
gboolean zMapGFFParseHeader_V2(ZMapGFFParser parser_base, char *line, gboolean *header_finished, ZMapGFFHeaderState *header_state) ;
gboolean zMapGFFParseSequence_V2(ZMapGFFParser parser_base, char *line, gboolean *sequence_finished) ;
gboolean zMapGFFParseLineLength_V2(ZMapGFFParser parser_base, char *line, gsize line_length) ;
void zMapGFFDestroyParser_V2(ZMapGFFParser parser) ;



#endif
