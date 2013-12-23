
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










