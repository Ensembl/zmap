/*  File: zmapGFFHeader.c
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
 * Description: Header object storage and management for GFFv3. This
 * refers to the internal header of the parser (rather than what were
 * referred to as headers in v2; these are called "directives" in v3).
 *
 *-------------------------------------------------------------------
 */
#include <string.h>
#include <ZMap/zmapGFF.h>

#include "zmapGFFDirective.h"
#include "zmapGFFHeader.h"


 /*
  * Function to create a GFF header object, and initialize the data members.
  * Return as a base class pointer.
  */
ZMapGFFHeader zMapGFFCreateHeader()
{
  unsigned int iDir ;
  ZMapGFFHeader pHeader = NULL;
  pHeader = g_new0(ZMapGFFHeaderStruct, 1) ;
  if (!pHeader)
    return NULL;

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
      pHeader->pDirective[iDir] = zMapGFFCreateDirective(iDir) ;
    }

  return pHeader;
}


/*
 * Data utility to get a int data member stored in a directive
*/
int zMapGFFGetDirectiveIntData(const ZMapGFFHeader const pHeader, ZMapGFFDirectiveName cTheDirName, unsigned int iIndex)
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
char *zMapGFFGetDirectiveStringData(const ZMapGFFHeader const pHeader, ZMapGFFDirectiveName cTheDirName, unsigned int iIndex)
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
void zMapGFFHeaderDestroy(ZMapGFFHeader const pHeader)
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
void zMapGFFHeaderMinimalTest(const ZMapGFFHeader const pHeader)
{
  if (!pHeader)
    return ;
  pHeader->flags.got_min =
    pHeader->flags.got_ver & pHeader->flags.got_sqr ;
}










