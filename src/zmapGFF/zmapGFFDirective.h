/*  File: zmapGFFDirective.h
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
 * Description: Header file for data structure and associated functions
 * for representing and dealing with GFF file directives (headers); that
 * is, lines within the file starting with "##".
 *
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_GFFDIRECTIVE_H
#define ZMAP_GFFDIRECTIVE_H

#include <ZMap/zmapGFF.h>
#include "zmapGFF_P.h"
#include "zmapGFF3_P.h"



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
  char *pPrefixString ;
  unsigned int iInts ;
  unsigned int iStrings ;
} ZMapGFFDirectiveInfoStruct ;


/*
 * Directive creation and destruction.
 */
ZMapGFFDirective zMapGFFCreateDirective(ZMapGFFDirectiveName) ;
void zMapGFFDestroyDirective(ZMapGFFDirective const) ;

/*
 * Directive utilities.
 */
ZMapGFFDirectiveName zMapGFFGetDirectiveName(const char * const) ;
const char* zMapGFFGetDirectivePrefix(ZMapGFFDirectiveName) ;
gboolean zMapGFFDirectiveNameValid(ZMapGFFDirectiveName) ;
ZMapGFFDirectiveInfoStruct zMapGFFGetDirectiveInfo(ZMapGFFDirectiveName) ;


#endif
