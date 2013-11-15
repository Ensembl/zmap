/*  File: zmapGFFDirective.c
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk,
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description: Directive storage and management for GFFv3.
 *
 *-------------------------------------------------------------------
 */

#include <string.h>
#include "zmapGFFDirective.h"

/*
 * This structure is to maintain a mapping between numerical indices of directives
 * and the corresponding strings (and possibly other data).
 */
const ZMapGFFDirectiveInfoStruct ZMAPGFF_DIR_INFO[ZMAPGFF_NUMBER_DIR_TYPES]  =
{
  {ZMAPGFF_DIR_VER,         "##gff-version",           1,       0       },
  {ZMAPGFF_DIR_DNA,                 "##DNA",           0,       0       },
  {ZMAPGFF_DIR_EDN,             "##end-DNA",           0,       0       },
  {ZMAPGFF_DIR_SQR,     "##sequence-region",           2,       1       },
  {ZMAPGFF_DIR_FEO,    "##feature-ontology",           0,       1       },
  {ZMAPGFF_DIR_ATO,  "##attribute-ontology",           0,       1       },
  {ZMAPGFF_DIR_SOO,     "##source-ontology",           0,       1       },
  {ZMAPGFF_DIR_SPE,             "##species",           0,       1       },
  {ZMAPGFF_DIR_GEN,        "##genome-build",           0,       2       },
  {ZMAPGFF_DIR_FAS,               "##FASTA",           0,       0       },
  {ZMAPGFF_DIR_CLO,                   "###",           0,       0       },
  {ZMAPGFF_DIR_UND,                      "",           0,       0       }
} ;




/*
 * Find the index of the directive given the string; argument is assumed to be of the form
 *      "##<name><otherdata>"
 * and will be queried from the above ZMapGFFDirectiveInfoStruct table.
 */
ZMapGFFDirectiveName zMapGFFGetDirectiveName(const char*const line)
{
  unsigned int iDir;
  for (iDir=0; iDir<ZMAPGFF_NUMBER_DIR_TYPES; ++iDir)
    {
      if (g_str_has_prefix(line, ZMAPGFF_DIR_INFO[iDir].pPrefixString))
        return ZMAPGFF_DIR_INFO[iDir].eName ;
    }
  return ZMAPGFF_DIR_UND;
}


/*
 * Find whether or not the name (enum entry) of a directive is valid.
 */
gboolean zMapGFFDirectiveNameValid(ZMapGFFDirectiveName eTheName)
{
  if (eTheName < ZMAPGFF_DIR_UND)
    return TRUE ;
  else
    return FALSE ;
}


/*
 * Find the prefix string of a directive given the name/number.
 */
const char* zMapGFFGetDirectivePrefix(ZMapGFFDirectiveName eTheName)
{
  if (!zMapGFFDirectiveNameValid(eTheName))
    return NULL;
  else
    return ZMAPGFF_DIR_INFO[eTheName].pPrefixString ;
}


/*
 * Lookup the directive info object from the name (uses table above).
 */
ZMapGFFDirectiveInfoStruct zMapGFFGetDirectiveInfo(ZMapGFFDirectiveName eTheName)
{
  unsigned int iDir ;
  ZMapGFFDirectiveInfoStruct cTheDirectiveInfo;
  for (iDir=0; iDir<ZMAPGFF_NUMBER_DIR_TYPES; ++iDir)
    {
      if (eTheName == ZMAPGFF_DIR_INFO[iDir].eName)
        cTheDirectiveInfo = ZMAPGFF_DIR_INFO[iDir] ;
    }
  return cTheDirectiveInfo ;
}




/*
 * This creates a directive of the type specified; allocates memory for the
 * int and string storage in the object.
 */
ZMapGFFDirective zMapGFFCreateDirective(ZMapGFFDirectiveName eTheName)
{
  unsigned int iLoop ;
  ZMapGFFDirective pDirective = NULL ;
  ZMapGFFDirectiveInfoStruct cTheDirectiveInfo ;
  if (!zMapGFFDirectiveNameValid(eTheName))
    return pDirective ;
  cTheDirectiveInfo = zMapGFFGetDirectiveInfo(eTheName) ;
  pDirective = (ZMapGFFDirective) g_new0(ZMapGFFDirectiveStruct, 1) ;

  pDirective->nInt = cTheDirectiveInfo.iInts;
  if (pDirective->nInt == 0)
    pDirective->piData = NULL ;
  else
    pDirective->piData = (int*) g_new0(int, pDirective->nInt ) ;

  pDirective->nString = cTheDirectiveInfo.iStrings ;
  if (pDirective->nString == 0)
    {
      pDirective->psData = NULL ;
    }
  else
    {
      pDirective->psData = (char**) g_new0(char*, pDirective->nString ) ;
      for (iLoop=0; iLoop<pDirective->nString; ++iLoop)
        pDirective->psData[iLoop] = NULL ;
    }

  return pDirective ;
}




/*
 * Function to destroy a directive object.
*/
void zMapGFFDestroyDirective(ZMapGFFDirective const pDirective)
{
  unsigned int iLoop ;
  if (!pDirective)
    return ;

  /* delete integer storage data if any allocated */
  if (pDirective->nInt && pDirective->piData)
    g_free(pDirective->piData) ;

  /* delete string data if any allocated */
  if (pDirective->nString && pDirective->psData)
    {
      for (iLoop=0; iLoop<pDirective->nString; ++iLoop)
        {
          if (pDirective->psData[iLoop])
            g_free(pDirective->psData[iLoop]) ;
        }
    }

  g_free(pDirective) ;
}
