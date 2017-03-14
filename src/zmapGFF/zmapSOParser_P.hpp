/*  File: zmapSOParser_P.h
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
 * Description: Private header file for data structure and functions
 * for dealing with parsing SO terms from the OBO file containing the
 * ontology itself.
 *
 *-------------------------------------------------------------------
 */



#ifndef ZMAP_SOPARSER_P_H
#define ZMAP_SOPARSER_P_H


#include <ZMap/zmapSOParser.hpp>
#include <ZMap/zmapStyle.hpp>

/*
 * Error domain for use with GError.
 */
#define ZMAPSO_PARSER_ERROR "ZMAPSO_PARSER_ERROR"


/*
 * Not sure if the UNK value is strictly necessary. The value of MAX
 * comes from the fact that 7 digits are used in the OBO file format
 * that's used to download the ontologies.
 */
#define ZMAPSO_ID_MAX 9999999
#define ZMAPSO_ID_UNK (ZMAPSO_ID_MAX+1)

/*
 * SO ID object.
 */
#define ZMAPSO_ID_STRING_LENGTH 10
typedef struct ZMapSOIDStruct_
  {
    unsigned int iID ;
    char sID[ZMAPSO_ID_STRING_LENGTH+1] ;
  } ZMapSOIDStruct ;

/*
 * Struct for storing SO ID data as a pair of integer and
 * string.
 */
typedef struct ZMapSOIDDataStruct_
  {
    unsigned int iID;
    const char * sName ;
    ZMapStyleMode cStyleMode ;
    ZMapHomolType cHomol ;
  } ZMapSOIDDataStruct ;

/*
 * Include data parsed out from SO files.
 */
#include "zmapSOData_P.hpp"

/*
 * Data structure for a single SO term. Contains:
 * ID                 as pointer to ZMapSOIDStruct
 * name               as pointer to char
 * flags              some flags
 */
typedef struct ZMapSOTermStruct_
  {
    ZMapSOID pID ;
    char * sName ;
    struct
      {
        unsigned int got_ter : 1 ;   /* refers to "[Term]" line               */
        unsigned int got_idd : 1 ;   /* refers to "id:" line                  */
        unsigned int got_nam : 1 ;   /* refers to "name:" line                */
        unsigned int got_obs : 1 ;   /* refers to "is_obsolete:" line         */
        unsigned int is_obs  : 1 ;   /* value found in "is_obsolete" line     */
      } flags ;
  } ZMapSOTermStruct ;


/*
 * Data structure to represent collection of
 * SO terms. Should be queried using external functions
 * only, to abstract internal representation.
 */
typedef struct ZMapSOCollectionStruct_
  {
    ZMapSOTerm *pTerms ;
    unsigned int iNumTerms ;
  } ZMapSOCollectionStruct ;



/*
 * Enum to define types of lines we are interested in.
 * There are several others but we do not need them
 * at present.
 */
typedef enum
  {
    ZMAPSO_LINE_TER,           /* [Term]                                            */
    ZMAPSO_LINE_TYP,           /* [Typedef]                                         */
    ZMAPSO_LINE_IDD,           /* id: <identifier>  (SO:XXXXXXX)                    */
    ZMAPSO_LINE_NAM,           /* name: <name_string>                               */
    ZMAPSO_LINE_OBS,           /* is_obsolete: [true,false]                         */
    ZMAPSO_LINE_UNK            /* unknown, i.e. everythig else                      */
  } ZMapSOParserLineType ;

/*
 * Parser for SO terms.
 */
typedef struct ZMapSOParserStruct_
  {
    unsigned int iLineCount ;
    GError *pError ;
    GQuark qErrorDomain ;
    gboolean bStopOnError ;
    ZMapSOParserLineType eLineType ;
    ZMapSOID pID ;
    char *sBufferLine ;
    char *sBuffer ;
    ZMapSOTerm pTerm ;
    ZMapSOCollection pCollection ;
  } ZMapSOParserStruct ;
#define ZMAPSO_PARSER_BUFFER_LINELENGTH 10000
#define ZMAPSO_PARSER_BUFFER_LENGTH 1000



#endif

























