/*  File: zmapGFF3_P.h
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
 * Description: Private header file for GFF3-specific functions and
 * definitions (enumerations, etc).
 *
 *-------------------------------------------------------------------
 */


#ifndef ZMAP_GFF3_P_H
#define ZMAP_GFF3_P_H

#include <ZMap/zmapGFF.hpp>
#include <ZMap/zmapMLF.hpp>
#include <zmapGFF_P.hpp>
#include <zmapSOParser_P.hpp>

/*
 * Directive types.
 */
typedef enum
  {
    ZMAPGFF_DIR_VER = 0,                         /* "##gff-version"         */
    ZMAPGFF_DIR_DNA = 1,                         /* "##DNA"                 */
    ZMAPGFF_DIR_EDN = 2,                         /* "##end-DNA"             */
    ZMAPGFF_DIR_SQR = 3,                         /* "##sequence-region"     */
    ZMAPGFF_DIR_FEO = 4,                         /* "##feature-ontology"    */
    ZMAPGFF_DIR_ATO = 5,                         /* "##attribute-ontology"  */
    ZMAPGFF_DIR_SOO = 6,                         /* "##source-ontology"     */
    ZMAPGFF_DIR_SPE = 7,                         /* "##species"             */
    ZMAPGFF_DIR_GEN = 8,                         /* "##genome-build"        */
    ZMAPGFF_DIR_FAS = 9,                         /* "##fasta"               */
    ZMAPGFF_DIR_CLO = 10,                        /* "###"                   */
    ZMAPGFF_DIR_UND = 11                         /* unknown/undefined       */
  } ZMapGFFDirectiveName ;
#define ZMAPGFF_NUMBER_DIR_TYPES 12


/*
 * List of data fields/types for GFF feature parsing.
 */
typedef enum
  {
    ZMAPGFF_FDA_SEQ,                           /* sequence            */
    ZMAPGFF_FDA_SOU,                           /* source              */
    ZMAPGFF_FDA_TYP,                           /* type                */
    ZMAPGFF_FDA_STA,                           /* start               */
    ZMAPGFF_FDA_END,                           /* end                 */
    ZMAPGFF_FDA_SCO,                           /* score               */
    ZMAPGFF_FDA_STR,                           /* strand              */
    ZMAPGFF_FDA_PHA                            /* phase               */
  } ZMapGFFMandatoryFieldName ;
/* we have
 *
 * #define ZMAPGFF_MANDATORY_FIELDS 8
 *
 * given above in the private header common to
 * both versions "zmapGFF_P.h".
 *
 */


/*
 * Line types that are relevant to parser FSM. Do not change the values
 * given here as these are used elsewhere to index an array. These symbols
 * are used only be v2.
 */
typedef enum
  {
    ZMAPGFF_LINE_EMP = 0,         /* empty line                 */
    ZMAPGFF_LINE_DNA = 1,         /* ##DNA line                 */
    ZMAPGFF_LINE_EDN = 2,         /* ##end-DNA line             */
    ZMAPGFF_LINE_DIR = 3,         /* general header/directive   */
    ZMAPGFF_LINE_COM = 4,         /* comment                    */
    ZMAPGFF_LINE_BOD = 5,         /* body line                  */
    ZMAPGFF_LINE_FAS = 6,         /* fasta directive            */
    ZMAPGFF_LINE_CLO = 7,         /* closure                    */
    ZMAPGFF_LINE_OTH = 8,         /* anything else              */
    ZMAPGFF_LINE_NUM = 9
  } ZMapGFFLineType ;
#define ZMAPGFF_NUMBER_LINE_TYPES ZMAPGFF_LINE_NUM

/*
 * Some forward declarations for types used by V3 only.
 */
typedef struct    ZMapGFFDirectiveInfoStruct_      *ZMapGFFDirectiveInfo ;
typedef struct        ZMapGFFDirectiveStruct_      *ZMapGFFDirective ;
typedef struct    ZMapGFFAttributeInfoStruct_      *ZMapGFFAttributeInfo ;
typedef struct        ZMapGFFAttributeStruct_      *ZMapGFFAttribute ;
typedef struct           ZMapGFFHeaderStruct_      *ZMapGFFHeader ;
typedef struct      ZMapGFFFeatureDataStruct_      *ZMapGFFFeatureData ;


/*
 * V3 parser struct.
 *
 * NB composite_features table is used in construction of transcript features.
 * It is a map of the (GQuark from) ID attribute to the (GQuark from) the
 * feature_name_id, that is, the feature->unique_id data member.
 */
typedef struct ZMapGFF3ParserStruct_
  {
    /*
     * Data common to both versions.
     */
    ZMAPGFF_PARSER_STRUCT_COMMON_DATA

    /*
     * Data for V3 only.
     */
    ZMapGFFHeader pHeader ;
    ZMapSOErrorLevel cSOErrorLevel ;
    /*ZMapMLF pMLF ; */
    ZMapSOSetInUse cSOSetInUse ;

    int iNumWrongSequence ;
    int nSequenceRecords ;
    ZMapSequenceStruct *pSeqData;

    gboolean bLogWarnings,
             bCheckSequenceLength;

    GHashTable *composite_features ;

} ZMapGFF3ParserStruct, *ZMapGFF3Parser ;



ZMapGFFParser zMapGFFCreateParser_V3(const char *sequence, int features_start, int features_end, ZMapConfigSource source) ;
void zMapGFFDestroyParser_V3(ZMapGFFParser parser) ;
gboolean zMapGFFParse_V3(ZMapGFFParser parser_base, char* const line ) ;

gboolean zMapGFFParseSequence_V3(ZMapGFFParser pParserBase, char * const sLine, gboolean *sequence_finished) ;


gboolean zMapGFFGetLogWarnings(ZMapGFFParser pParser );
gboolean zMapGFFSetSOSetInUse(ZMapGFFParser pParser, ZMapSOSetInUse );
ZMapSOSetInUse zMapGFFGetSOSetInUse(ZMapGFFParser pParser );
gboolean zMapGFFSetSOErrorLevel(ZMapGFFParser pParserBase, ZMapSOErrorLevel cErrorLevel) ;
ZMapSOErrorLevel zMapGFFGetSOErrorLevel(ZMapGFFParser pParserBase ) ;
gboolean zMapGFFGetHeaderGotMinimal_V3(ZMapGFFParser pParserBase) ;
gboolean zMapGFFGetHeaderGotSequenceRegion_V3(ZMapGFFParser pParserBase) ;







#endif
