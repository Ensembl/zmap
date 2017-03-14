/*  File: zmapGFFStringUtils.h
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
 * Description: Header file for some string utilities used by
 * GFF parser. Most important of which are the tokenizers.
 *
 *-------------------------------------------------------------------
 */


#ifndef HEADER_ZMAP_STRINGUTILS_H
#define HEADER_ZMAP_STRINGUTILS_H

/*
 * Standard library stuff required for these functions.
 */
#include <string.h>


/*
 * GLib also required.
 */
#include <glib.h>


/*
 * Some string utilities.
 */
char** zMapGFFStringUtilsTokenizer(char, const char * const, unsigned int*, gboolean, unsigned int, void*(*)(size_t), void(*)(void*), char*) ;
char** zMapGFFStringUtilsTokenizer02(char, char, const char * const , unsigned int *,  gboolean, void*(*)(size_t), void(*)(void*)) ;
void zMapGFFStringUtilsArrayDelete(char**, unsigned int, void(*)(void*)) ;
char * zMapGFFStringUtilsSubstring(const char* const, const char* const, void*(*local_malloc)(size_t)) ;
gboolean zMapGFFStringUtilsSubstringReplace(const char * const, const char * const, const char * const, char ** ) ;
char* zMapGFFEscape(const char * const sInput ) ;
char* zMapGFFUnescape(const char * const sInput ) ;



#endif
