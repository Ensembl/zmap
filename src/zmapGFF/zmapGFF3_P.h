/*  File: zmapGFF3_P.h
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
 * Description: Private header file for GFF3-specific functions and
 * definitions (enumerations, etc).
 *
 *-------------------------------------------------------------------
 */


#ifndef ZMAP_GFF3_P_H
#define ZMAP_GFF3_P_H

#include <ZMap/zmapGFF.h>
#include <ZMap/zmapMLF.h>
#include "zmapGFF_P.h"
#include "zmapSOParser_P.h"

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
 * List of attributes that we may find. This is the union of
 * v3 definitions from sequenceontology.org together with ones
 * that I've found already in the v2 data.
 *
 * The strings associated with the attributes are shown in
 * comments on the right. The AttributeInfo class is where
 * the code finds these data.
 *
 * Note that there are two sets of attributes for v2 and v3,
 * and I have kept them seperate as shown below.
 *
 * This is because they may need to be treated in different
 * ways (or at least parsed differently) e.g. "Name" in v2
 * and v3, and also that I want to have one typename
 * (ZMapGFFAttributeName) rather than one for each version.
 * This enables one array to be used for lookups rather than
 * one for each version, etc.
 *
 * The "unknown" type does not necessarily mean invalid. It
 * is used to store anything that is not otherwise hard-coded
 * into the list.
 *
 * This could be simplified using hash tables, but will do
 * for the moment.
 *
 */
#define ZMAPGFF_ATT_V2START 0
#define ZMAPGFF_ATT_V3START 1000
typedef enum
  {
    ZMAPGFF_ATT_ALN2 = ZMAPGFF_ATT_V2START,              /* v2 "Align"                    */
    ZMAPGFF_ATT_NAM2,                                    /* v2 "Name"                     */
    ZMAPGFF_ATT_CLA2,                                    /* v2 "Class"                    */

    ZMAPGFF_ATT_IDD3 = ZMAPGFF_ATT_V3START,              /* v3 "ID"                       */
    ZMAPGFF_ATT_NAM3,                                    /* v3 "Name"                     */
    ZMAPGFF_ATT_ALI3,                                    /* v3 "Alias"                    */
    ZMAPGFF_ATT_PAR3,                                    /* v3 "Parent"                   */
    ZMAPGFF_ATT_TAR3,                                    /* v3 "Target"                   */
    ZMAPGFF_ATT_GAP3,                                    /* v3 "Gap"                      */
    ZMAPGFF_ATT_DER3,                                    /* v3 "Derives_from"             */
    ZMAPGFF_ATT_NOT3,                                    /* v3 "Note"                     */
    ZMAPGFF_ATT_DBX3,                                    /* v3 "Dbxref"                   */
    ZMAPGFF_ATT_ONT3,                                    /* v3 "Ontology_term"            */
    ZMAPGFF_ATT_ISC3,                                    /* v3 "Is_circular"              */

    ZMAPGFF_ATT_UNK,                                     /* unknown                       */
  } ZMapGFFAttributeName ;
#define ZMAPGFF_NUM_ATT_V2 3
#define ZMAPGFF_NUM_ATT_V3 11
#define ZMAPGFF_NUMBER_ATTRIBUTE_TYPES (ZMAPGFF_NUM_ATT_V2 + ZMAPGFF_NUM_ATT_V3  + 1)
#define ZMAPGFF_ATTRIBUTE_LIMIT 1000



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
    ZMAPGFF_LINE_OTH = 8          /* anything else              */
  } ZMapGFFLineType ;
#define ZMAPGFF_NUMBER_LINE_TYPES 9

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
    ZMapSequenceStruct *pSeqData;
    ZMapGFFHeader pHeader ;
    ZMapSOErrorLevel cSOErrorLevel ;
    ZMapMLF pMLF ;
    ZMapSOSetInUse cSOSetInUse ;

    unsigned int iNumWrongSequence,
                 nSequenceRecords;

    gboolean bLogWarnings,
             bCheckSequenceLength;

} ZMapGFF3ParserStruct, *ZMapGFF3Parser ;



ZMapGFFParser zMapGFFCreateParser_V3(char *sequence, int features_start, int features_end) ;
void zMapGFFDestroyParser_V3(ZMapGFFParser parser) ;
gboolean zMapGFFParse_V3(ZMapGFFParser parser_base, char* const line ) ;
gboolean zMapGFFGetLogWarnings(const ZMapGFFParser const pParser );
gboolean zMapGFFSetSOSetInUse(ZMapGFFParser const pParser, ZMapSOSetInUse );
ZMapSOSetInUse zMapGFFGetSOSetInUse(const ZMapGFFParser const pParser );
gboolean zMapGFFSetSOErrorLevel(ZMapGFFParser const pParserBase, ZMapSOErrorLevel cErrorLevel) ;
ZMapSOErrorLevel zMapGFFGetSOErrorLevel(const ZMapGFFParser const pParserBase ) ;
gboolean zMapGFFGetHeaderGotMinimal_V3(const ZMapGFFParser const pParserBase) ;







#endif
