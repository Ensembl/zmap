/*  File: zmapSO.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Functions, structs etc. for a Sequence Ontology (SO)
 *              package.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SO_H
#define ZMAP_SO_H

#include <ZMap/zmapFeature.hpp>


/* Calling functions will need to know SO accession strings so put them here in one place. */
#define SO_ACC_CNV           "SO:0001019"
#define SO_ACC_DELETION      "SO:0000159"
#define SO_ACC_INSERTION     "SO:0000667"
#define SO_ACC_POLYA_JUNC    "SO:0001430"
#define SO_ACC_POLYA_SEQ     "SO:0000610"
#define SO_ACC_POLYA_SIGNAL  "SO:0000551"
#define SO_ACC_POLYA_SITE    "SO:0000553"
#define SO_ACC_SEQ_ALT       "SO:0001059"
#define SO_ACC_SNP           "SO:0000694"
#define SO_ACC_SNV           "SO:0001483"
#define SO_ACC_MUTATION      "SO:0000109"
#define SO_ACC_SUBSTITUTION  "SO:1000002"
#define SO_ACC_TRANSCRIPT    "SO:0000673"




const char *zMapSOAcc2Term(GQuark SO_acc) ;
GQuark zMapSOAcc2TermID(GQuark SO_accession) ;
GQuark zMapSOVariation2SO(char *variation_str) ;
GQuark zMapSOFeature2SO(ZMapFeature feature) ;

#endif /* ZMAP_SO_H */
