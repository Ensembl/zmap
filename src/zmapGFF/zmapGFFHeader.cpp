/*  File: zmapGFFHeader.c
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
 * Description: Header object storage and management for GFFv3. This
 * refers to the internal header of the parser (rather than what were
 * referred to as headers in v2; these are called "directives" in v3).
 *
 *-------------------------------------------------------------------
 */
#include <string.h>
#include <ZMap/zmapGFF.hpp>

#include <zmapGFFDirective.hpp>
#include <zmapGFFHeader.hpp>


 /*
  * Function to create a GFF header object, and initialize the data members.
  * Return as a base class pointer.
  */
ZMapGFFHeader zMapGFFCreateHeader()
{
  unsigned int iDir ;
  ZMapGFFHeader pHeader = NULL;

  pHeader = g_new0(ZMapGFFHeaderStruct, 1) ;

  pHeader->sequence_name = NULL ;
  pHeader->features_start = 0 ;
  pHeader->features_end = 0 ;
  pHeader->eState = ZMAPGFF_HEADER_UNKNOWN;

  pHeader->flags.got_min = FALSE ;
  pHeader->flags.got_ver = FALSE ;
  pHeader->flags.got_dna = FALSE ;
  pHeader->flags.got_sqr = FALSE ;
  pHeader->flags.got_feo = FALSE ;
  pHeader->flags.got_ato = FALSE ;
  pHeader->flags.got_soo = FALSE ;
  pHeader->flags.got_spe = FALSE ;
  pHeader->flags.got_gen = FALSE ;
  pHeader->flags.got_fas = FALSE ;
  pHeader->flags.got_clo = FALSE ;

  for (iDir=0; iDir<ZMAPGFF_NUMBER_DIR_TYPES; ++iDir)
    {
      pHeader->pDirective[iDir] = zMapGFFCreateDirective((ZMapGFFDirectiveName)iDir) ;
    }

  return pHeader;
}


/*
 * Data utility to get a int data member stored in a directive
*/
int zMapGFFGetDirectiveIntData(ZMapGFFHeader pHeader, ZMapGFFDirectiveName cTheDirName, unsigned int iIndex)
{
  ZMapGFFDirectiveInfoStruct cTheDirectiveInfo;
  int iResult = 0 ;
  if (!pHeader)
    return iResult ;
  if (!zMapGFFDirectiveNameValid(cTheDirName))
    return iResult ;

  cTheDirectiveInfo = zMapGFFGetDirectiveInfo(cTheDirName) ;
  if (iIndex < cTheDirectiveInfo.iInts)
    {
      iResult = pHeader->pDirective[cTheDirName]->piData[iIndex] ;
    }

  return iResult ;
}



/*
 * Data utility to get a string data member from a directive
*/
char *zMapGFFGetDirectiveStringData(ZMapGFFHeader pHeader, ZMapGFFDirectiveName cTheDirName, unsigned int iIndex)
{
  ZMapGFFDirectiveInfoStruct cTheDirectiveInfo ;
  char* sResult = NULL ;
  if (!pHeader)
    return sResult ;
  if (!zMapGFFDirectiveNameValid(cTheDirName))
    return sResult ;

  cTheDirectiveInfo = zMapGFFGetDirectiveInfo(cTheDirName) ;
  if (iIndex < cTheDirectiveInfo.iStrings)
    {
      sResult = pHeader->pDirective[cTheDirName]->psData[iIndex] ;
    }

  return sResult ;
}




/*
 * Function to destroy a GFF3 header object.
 */
void zMapGFFHeaderDestroy(ZMapGFFHeader pHeader)
{
  unsigned int iDir;
  if (!pHeader)
    return ;

  /*
   * Destroy individual directive objects
   */
  for (iDir=0; iDir<ZMAPGFF_NUMBER_DIR_TYPES; ++iDir)
    {
      zMapGFFDestroyDirective(pHeader->pDirective[iDir]) ;
    }

  /*
   * Delete the top level object
   */
  g_free(pHeader);
}


/*
 * Tests to see if the got_minimal header flag should be set. This is should only be
 * set if we have both the gff_version and got_sequence_region flags set.
 */
void zMapGFFHeaderMinimalTest(ZMapGFFHeader pHeader)
{
  if (!pHeader)
    return ;
  //pHeader->flags.got_min =
  //  pHeader->flags.got_ver & pHeader->flags.got_sqr ;
  pHeader->flags.got_min = pHeader->flags.got_ver ;
}










