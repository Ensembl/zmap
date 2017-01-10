/*  File: zmapGFFStringUtils.h
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
