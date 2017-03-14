/*  File: zmapGFFDirective.c
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
 * Description: Directive storage and management for GFFv3.
 *
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <zmapGFFDirective.hpp>

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
void zMapGFFDestroyDirective(ZMapGFFDirective pDirective)
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
