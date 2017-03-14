/*  File: zmapGFFHeader.h
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
 * Description: Header file for data structure and associated functions
 * for dealing with GFF header. This essentially refers to a collection
 * of directive ("##" line) objects.
 *
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_GFFHEADER_P_H
#define ZMAP_GFFHEADER_P_H

#include <ZMap/zmapGFF.hpp>
#include <zmapGFF_P.hpp>
#include <zmapGFF3_P.hpp>

/*
 * Base `class'' struct for the header data structure.
 */
typedef struct ZMapGFFHeaderStruct_
  {
    char *sequence_name ;
    int features_start ;
    int features_end ;

    ZMapGFFHeaderState eState ;

    struct
      {
        unsigned int got_min : 1 ; /* minimal header (ver & sqr)  */
        unsigned int got_ver : 1 ; /* ##gff-version               */
        unsigned int got_dna : 1 ; /* ##DNA                       */
        unsigned int got_sqr : 1 ; /* ##sequence-region           */
        unsigned int got_feo : 1 ; /* ##feature-ontology          */
        unsigned int got_ato : 1 ; /* ##attribute-ontology        */
        unsigned int got_soo : 1 ; /* ##source-ontology           */
        unsigned int got_spe : 1 ; /* ##species                   */
        unsigned int got_gen : 1 ; /* ##genome-build              */
        unsigned int got_fas : 1 ; /* ##FASTA                     */
        unsigned int got_clo : 1 ; /* ###                         */
      } flags ;

    ZMapGFFDirective pDirective[ZMAPGFF_NUMBER_DIR_TYPES] ;

  } ZMapGFFHeaderStruct ;



/*
 * Header creation and destruction.
 */
ZMapGFFHeader zMapGFFCreateHeader() ;
void zMapGFFHeaderDestroy(ZMapGFFHeader ) ;
void zMapGFFHeaderMinimalTest(ZMapGFFHeader ) ;


/*
 * Some data utilities
*/
int zMapGFFGetDirectiveIntData(ZMapGFFHeader, ZMapGFFDirectiveName, unsigned int) ;
char *zMapGFFGetDirectiveStringData(ZMapGFFHeader, ZMapGFFDirectiveName, unsigned int) ;



#endif

