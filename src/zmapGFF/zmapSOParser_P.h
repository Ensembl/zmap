/*  File: zmapGFFSOParser_P.h
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2013: Genome Research Ltd.
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
 *     	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
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


#include <ZMap/zmapSOParser.h>
#include <ZMap/zmapStyle.h>

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
    char * sName ;
    ZMapStyleMode cStyleMode ;
  } ZMapSOIDDataStruct ;

/*
 * Include data parsed out from SO files.
 */
#include "zmapSOData_P.h"

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

























