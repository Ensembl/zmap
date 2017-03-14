/*  File: zmapGFFDirective.h
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
 * for representing and dealing with GFF file directives (headers); that
 * is, lines within the file starting with <##".
 *
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_GFFDIRECTIVE_H
#define ZMAP_GFFDIRECTIVE_H

#include <ZMap/zmapGFF.hpp>
#include <zmapGFF_P.hpp>
#include <zmapGFF3_P.hpp>



/*
 * Full declaration of the directive object. Was forward
 * declared in zmapGFF.h.
 */
typedef struct ZMapGFFDirectiveStruct_
  {
    unsigned int nInt ;
    int *piData ;
    unsigned int nString ;
    char **psData ;
  } ZMapGFFDirectiveStruct ;


/*
 * Full declaration of the directive info object. Data members have the
 * following meanings:
 *
 * eName              name, enum member defined above
 * pPrefixString      prefix string to expect in the file
 * iInts              number of integer data associated with this directive [0, ... )
 * iStrings           number of strings associated with this directive [0, ...)
 *
 */
typedef struct ZMapGFFDirectiveInfoStruct_
  {
    ZMapGFFDirectiveName eName ;
    const char *pPrefixString ;
    unsigned int iInts ;
    unsigned int iStrings ;
  } ZMapGFFDirectiveInfoStruct ;


/*
 * Directive creation and destruction.
 */
ZMapGFFDirective zMapGFFCreateDirective(ZMapGFFDirectiveName) ;
void zMapGFFDestroyDirective(ZMapGFFDirective) ;

/*
 * Directive utilities.
 */
ZMapGFFDirectiveName zMapGFFGetDirectiveName(const char * const) ;
const char* zMapGFFGetDirectivePrefix(ZMapGFFDirectiveName) ;
gboolean zMapGFFDirectiveNameValid(ZMapGFFDirectiveName) ;
ZMapGFFDirectiveInfoStruct zMapGFFGetDirectiveInfo(ZMapGFFDirectiveName) ;


#endif
