/*  File: zmapSOParser.c
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 * Description: Functionality for dealing with SO terms and querying
 * the internally stored data.
 *-------------------------------------------------------------------
 */


#include "zmapSOParser_P.h"


/*
 * Create a single SO ID Data object with null data.
 */
ZMapSOIDData zMapSOIDDataCreate()
{
  ZMapSOIDData pID = NULL ;
  pID = g_malloc(sizeof(ZMapSOIDDataStruct)) ;
  if (!pID)
    return pID ;
  pID->iID = ZMAPSO_ID_UNK ;
  pID->sName = NULL ;
  pID->cStyleMode = ZMAPSTYLE_MODE_INVALID ;
  return pID ;
}

/*
 * Create a single SOID Data object with supplied data.
 */
ZMapSOIDData zMapSOIDDataCreateFromData(unsigned int iID, const char* const *sName, ZMapStyleMode cStyleMode )
{
  ZMapSOIDData pIDData = zMapSOIDDataCreate() ;
  if (!pIDData)
    return pIDData ;
  pIDData->iID = iID ;
  pIDData->sName = g_strdup((gchar*) sName) ;
  pIDData->cStyleMode = cStyleMode ;
  return pIDData ;
}

/*
 * Return the numerical ID of the SOIDData object
 */
unsigned int zMapSOIDDataGetID(const ZMapSOIDData const pData)
{
  if (!pData)
    return 0 ;
  return pData->iID ;
}


/*
 * Return the name of the SOIDData object as a string
 */
char * zMapSOIDDataGetName(const ZMapSOIDData const pData )
{
  if (!pData)
    return NULL ;
  return pData->sName ;
}


/*
 * Return the ZMapStyleMode of the SOID data object
 */
ZMapStyleMode zMapSOIDDataGetStyleMode(const ZMapSOIDData const pData )
{
  if (!pData)
    return ZMAPSTYLE_MODE_BASIC ;
  return pData->cStyleMode ;
}




/*
 * Destroy a single SO ID Data object.
 */
gboolean zMapSOIDDataDestroy(ZMapSOIDData const pID)
{
  gboolean bResult = FALSE ;
  if (!pID)
    return bResult ;
  bResult = TRUE ;
  if (pID->sName)
    g_free(pID->sName) ;
  g_free(pID) ;
  return bResult ;
}


/*
 * Lookup the style mode of the SO term from the numerical ID.
 */
ZMapStyleMode zMapSOSetGetStyleModeFromID(ZMapSOSetInUse cSOSetInUse, unsigned int iID)
{
  ZMapStyleMode cTheMode = ZMAPSTYLE_MODE_INVALID ;
  unsigned int i ;

  if (cSOSetInUse == ZMAPSO_USE_SOFA)
  {
    for (i=0; i<ZMAP_SO_DATA_TABLE01_NUM_ITEMS; ++i)
    {
      if (iID == ZMAP_SO_DATA_TABLE01[i].iID )
      {
        cTheMode =  ZMAP_SO_DATA_TABLE01[i].cStyleMode ;
        break ;
      }
    }
  }
  else if (cSOSetInUse == ZMAPSO_USE_SOXP)
  {
    for (i=0; i<ZMAP_SO_DATA_TABLE02_NUM_ITEMS; ++i)
    {
      if (iID == ZMAP_SO_DATA_TABLE02[i].iID )
      {
        cTheMode = ZMAP_SO_DATA_TABLE02[i].cStyleMode ;
        break ;
      }
    }
  }
  else if (cSOSetInUse == ZMAPSO_USE_SOXPSIMPLE)
  {
    for (i=0; i<ZMAP_SO_DATA_TABLE03_NUM_ITEMS; ++i)
    {
      if (iID == ZMAP_SO_DATA_TABLE03[i].iID )
      {
        cTheMode = ZMAP_SO_DATA_TABLE03[i].cStyleMode ;
        break ;
      }
    }
  }


  return cTheMode ;
}

/*
 * Lookup the style mode from the name.
 */
ZMapStyleMode zMapSOSetGetStyleModeFromName(ZMapSOSetInUse cSOSetInUse, const char * const sName )
{
  ZMapStyleMode cTheMode = ZMAPSTYLE_MODE_INVALID ;
  unsigned int i ;
  if (!sName || !*sName)
    return cTheMode;

  if (cSOSetInUse == ZMAPSO_USE_SOFA)
  {
    for (i=0; i<ZMAP_SO_DATA_TABLE01_NUM_ITEMS; ++i)
    {
      if (!strcmp(sName, ZMAP_SO_DATA_TABLE01[i].sName))
      {
        cTheMode = ZMAP_SO_DATA_TABLE01[i].cStyleMode ;
        break ;
      }
    }
  }
  else if (cSOSetInUse == ZMAPSO_USE_SOXP)
  {
    for (i=0; i<ZMAP_SO_DATA_TABLE02_NUM_ITEMS; ++i)
    {
      if (!strcmp(sName, ZMAP_SO_DATA_TABLE02[i].sName))
      {
        cTheMode = ZMAP_SO_DATA_TABLE02[i].cStyleMode ;
        break ;
      }
    }
  }
  else if (cSOSetInUse == ZMAPSO_USE_SOXPSIMPLE)
  {
    for (i=0; i<ZMAP_SO_DATA_TABLE03_NUM_ITEMS; ++i)
    {
      if (!strcmp(sName, ZMAP_SO_DATA_TABLE03[i].sName))
      {
        cTheMode = ZMAP_SO_DATA_TABLE03[i].cStyleMode ;
        break ;
      }
    }
  }

  return cTheMode ;
}

/*
 * Check whether or not the name passed in is present in the SO term arrays.
 * Return the associated numerical ID if it is present, ZMAPSO_ID_UNK otherwise.
 */
unsigned int zMapSOSetIsNamePresent(ZMapSOSetInUse cSOSetInUse, const char * const sType)
{
  unsigned int iResult = ZMAPSO_ID_UNK, i ;
  if (!sType || !*sType)
    return iResult ;

  if (cSOSetInUse == ZMAPSO_USE_SOFA)
  {
    for (i=0; i<ZMAP_SO_DATA_TABLE01_NUM_ITEMS; ++i)
    {
      if (!strcmp(sType, ZMAP_SO_DATA_TABLE01[i].sName))
      {
        iResult = ZMAP_SO_DATA_TABLE01[i].iID ;
        break ;
      }
    }
  }
  else if (cSOSetInUse == ZMAPSO_USE_SOXP)
  {
    for (i=0; i<ZMAP_SO_DATA_TABLE02_NUM_ITEMS; ++i)
    {
      if (!strcmp(sType, ZMAP_SO_DATA_TABLE02[i].sName))
      {
        iResult = ZMAP_SO_DATA_TABLE02[i].iID ;
        break ;
      }
    }
  }
  else if (cSOSetInUse == ZMAPSO_USE_SOXPSIMPLE)
  {
    for (i=0; i<ZMAP_SO_DATA_TABLE03_NUM_ITEMS; ++i)
    {
      if (!strcmp(sType, ZMAP_SO_DATA_TABLE03[i].sName))
      {
        iResult = ZMAP_SO_DATA_TABLE03[i].iID ;
        break ;
      }
    }
  }

  return iResult ;
}

/*
 * Check whether or not an ID (as number) passed in is present in the SO term arrays.
 * If it is present, then return the pointer to the name. Otherwise return NULL.
 */
char * zMapSOSetIsIDPresent(ZMapSOSetInUse cSOSetInUse, unsigned int iID )
{
  char* sResult = NULL ;
  unsigned int i ;

  if (cSOSetInUse == ZMAPSO_USE_SOFA)
  {
    for (i=0; i<ZMAP_SO_DATA_TABLE01_NUM_ITEMS; ++i)
    {
      if (iID == ZMAP_SO_DATA_TABLE01[i].iID )
      {
        sResult =  ZMAP_SO_DATA_TABLE01[i].sName ;
        break ;
      }
    }
  }
  else if (cSOSetInUse == ZMAPSO_USE_SOXP)
  {
    for (i=0; i<ZMAP_SO_DATA_TABLE02_NUM_ITEMS; ++i)
    {
      if (iID == ZMAP_SO_DATA_TABLE02[i].iID )
      {
        sResult = ZMAP_SO_DATA_TABLE02[i].sName ;
        break ;
      }
    }
  }
  else if (cSOSetInUse == ZMAPSO_USE_SOXPSIMPLE)
  {
    for (i=0; i<ZMAP_SO_DATA_TABLE03_NUM_ITEMS; ++i)
    {
      if (iID == ZMAP_SO_DATA_TABLE03[i].iID )
      {
        sResult = ZMAP_SO_DATA_TABLE03[i].sName;
        break ;
      }
    }
  }

  return sResult ;
}


/*
 * Need functions to lookup ZMapStyleMode from either the ID
 * or the name string.
 */













/*
 * Some static data for this translation unit. Use of these is now
 * deprecated.
 */
static const char * const sFilenameSOFA = "~/.ZMap/SOFA.obo" ;
static const char * const sFilenameSO   = "~/.ZMap/so-xp.obo" ;
static ZMapSOCollection pCollectionSO   = NULL ;
static ZMapSOCollection pCollectionSOFA = NULL ;




/*
 * Create a single SO ID object with null data.
 */
ZMapSOID zMapSOIDCreate()
{
  ZMapSOID pID = NULL ;
  pID = g_malloc(sizeof(ZMapSOIDStruct)) ;
  if (!pID)
    return pID ;
  pID->iID = ZMAPSO_ID_UNK ;
  pID->sID[0] = '\0' ; /* this array is allocated to a fixed size */
  return pID ;
}


/*
 * Copy an SOID object from pRHS to pLHS
 */
gboolean zMapSOIDCopy(ZMapSOID const pLHS, const ZMapSOID const pRHS)
{
  gboolean bResult = FALSE ;
  if (!pLHS || !pRHS)
    return bResult ;
  bResult = TRUE ;
  pLHS->iID = pRHS->iID;
  strncpy(pLHS->sID, pRHS->sID, ZMAPSO_ID_STRING_LENGTH) ;
  pLHS->sID[ZMAPSO_ID_STRING_LENGTH] = '\0' ;
  bResult = TRUE ;

  return bResult ;
}


/*
 * Test to see if two SOID objects are equal. Test numerical
 * and string IDs.
 */
gboolean zMapSOIDEquals(const ZMapSOID const pLHS, const ZMapSOID const pRHS)
{
  if (!pLHS || !pRHS)
    return FALSE ;
  if (pLHS->iID != pRHS->iID)
    return FALSE ;
  if (strcmp(pLHS->sID, pRHS->sID))
    return FALSE ;
  return TRUE ;
}


/*
 * Initialize the SOID to UNK numerical type with empty name string.
 */
gboolean zMapSOIDInitialize(ZMapSOID const pID)
{
  gboolean bResult = FALSE ;
  if (!pID)
    return bResult ;
  pID->iID = ZMAPSO_ID_UNK ;
  pID->sID[0] = '\0' ;
  return bResult ;
}



/*
 * Set the SOID object from the string given. Make some basic checks
 * and then parse the string for numerical value. Test that this is
 * in the correct range, then set numerical value and copy string.
 */
gboolean zMapSOIDSet(ZMapSOID const pID, const char* const sString)
{
  gboolean bResult = FALSE ;
  unsigned int iResult = ZMAPSO_ID_UNK ;
  if (    !pID
       || pID->iID!=ZMAPSO_ID_UNK
       || !sString
       || !*sString
       || strlen(sString)!=ZMAPSO_ID_STRING_LENGTH
     )
    return bResult ;
  if ((iResult = zMapSOIDParseString(sString)) == ZMAPSO_ID_UNK)
    return bResult ;
  pID->iID = iResult ;
  strncpy(pID->sID, sString, ZMAPSO_ID_STRING_LENGTH) ;
  pID->sID[ZMAPSO_ID_STRING_LENGTH] = '\0' ;
  return TRUE ;
}


/*
 * Check whether or not the SOID is valid. This basically means that the
 * ID must be in the correct range, and must match what's stored in the
 * string data.
 */
gboolean zMapSOIDIsValid(const ZMapSOID const pID)
{
  gboolean bResult = FALSE ;
  if (    !pID
       || pID->iID != ZMAPSO_ID_UNK
       || !pID->sID
       || !*pID->sID
       || strlen(pID->sID) != ZMAPSO_ID_STRING_LENGTH
     )
    return bResult ;
  if (pID->iID == zMapSOIDParseString(pID->sID))
    bResult = TRUE ;
  return bResult ;
}



/*
 * Parse string argument for the numerical value of the
 * ID. String must have length ZMAPSO_ID_STRING_LENGTH,
 * be null terminated and be of the form
 *
 * "SO:jjjjjjj"
 *
 * The value ZMAPSO_ID_UNK is returned on failure.
 */
unsigned int zMapSOIDParseString(const char * const sString)
{
  static const char* const sFormatSO = "SO:%d" ;
  static const unsigned int iExpectedFields = 1 ;
  unsigned int iResult = ZMAPSO_ID_UNK ;

  if (     !sString
       ||  !*sString
       || strlen(sString)!=ZMAPSO_ID_STRING_LENGTH
     )
    return iResult ;

  /*
   * Look for appropriate data form in the input string.
   */
  if (sscanf(sString, sFormatSO, &iResult) != iExpectedFields)
  {
    iResult = ZMAPSO_ID_UNK ;
  }
  if (iResult > ZMAPSO_ID_MAX)
    iResult = ZMAPSO_ID_UNK ;

  return iResult ;
}

/*
 * Is the argument string a valid SO accession number (as defined
 * above)? Returns a boolean flag.
 */
gboolean zMapSOIDIsAccessionNumber(const char * const sString)
{
  if (zMapSOIDParseString(sString) == ZMAPSO_ID_UNK)
    return FALSE ;
  else
    return TRUE ;
}



/*
 * Return the stored ID numerical value.
 */
unsigned int zMapSOIDGetIDNumber(const ZMapSOID const pID)
{
  if (!pID)
    return ZMAPSO_ID_UNK ;
  return pID->iID;
}

/*
 * Return the stored ID as a string.
 */
char * zMapSOIDGetIDString(const ZMapSOID const pID )
{
  if (!pID)
    return NULL ;
  return pID->sID ;
}



/*
 * Destroy a single SO ID object. Do not need to seperately free
 * the string data.
 */
gboolean zMapSOIDDestroy(ZMapSOID const pID)
{
  gboolean bResult = FALSE ;
  if (!pID)
    return bResult ;
  bResult = TRUE ;
  g_free(pID) ;
  return bResult ;
}









/*
 * Create a single term with the ID and name given.
 */
ZMapSOTerm zMapSOTermCreate()
{
  ZMapSOTerm pTerm = NULL ;

  /*
   * Allocate for object.
   */
  pTerm = (ZMapSOTerm) g_malloc(sizeof(ZMapSOTermStruct)) ;
  if (!pTerm)
    return pTerm ;

  /*
   * Create ID object.
   */
  pTerm->pID = zMapSOIDCreate() ;
  if (!pTerm->pID)
  {
    zMapSOTermDestroy(pTerm) ;
    return NULL ;
  }

  /*
   * Initialize name string to NULL.
   */
  pTerm->sName = NULL ;

  /*
   * Initialize flags with some default values.
   */
  pTerm->flags.got_ter = FALSE ;
  pTerm->flags.got_idd = FALSE ;
  pTerm->flags.got_nam = FALSE ;
  pTerm->flags.got_obs = FALSE ;
  pTerm->flags.is_obs = FALSE ;

  /*
   * Return object.
   */
  return pTerm ;
}



/*
 * Add an ID to the term but only if it does not have one set already.
 */
gboolean zMapSOTermSetID(ZMapSOTerm const pTerm, const ZMapSOID const pID)
{
  gboolean bResult = FALSE ;
  if (     !pTerm
        || !pTerm->pID
        || (pTerm->pID->iID != ZMAPSO_ID_UNK)
        || !pID
        || (pID->iID == ZMAPSO_ID_UNK)
     )
    return bResult ;

  pTerm->flags.got_idd = TRUE;
  bResult = zMapSOIDCopy(pTerm->pID, pID) ;

  return bResult ;
}



/*
 * Add a name to the term. Ony add the name is one is not already
 * set.
 */
gboolean zMapSOTermSetName(ZMapSOTerm const pTerm, const char* const sName)
{
  gboolean bResult = FALSE ;
  if (     !pTerm
        || pTerm->sName
        || !sName
        || !*sName
     )
    return bResult ;
  pTerm->sName = (char*) g_malloc(strlen(sName)+1) ;
  if (!pTerm->sName)
    return bResult ;
  strcpy(pTerm->sName, sName) ;
  pTerm->flags.got_nam = TRUE ;
  return bResult ;
}



/*
 * Destroy a single term.
 */
gboolean zMapSOTermDestroy(ZMapSOTerm const pTerm)
{
  gboolean bResult = FALSE ;
  if (!pTerm)
    return bResult ;
  bResult = TRUE ;

  /*
   * Check and free SOID object.
   */
  if (pTerm->pID)
    zMapSOIDDestroy(pTerm->pID) ;

  /*
   * Check and free string.
   */
  if (pTerm->sName)
    g_free(pTerm->sName) ;

  /*
   * Free term object.
   */
  g_free(pTerm) ;

  /*
   * Return.
   */
  return bResult ;
}


/*
 * Is the term valid?
 */
gboolean zMapSOTermIsValid(ZMapSOTerm pTerm)
{
  gboolean bValid = FALSE ;
  if (!pTerm)
    return bValid ;
  bValid = pTerm->flags.got_nam && pTerm->flags.got_idd ;
  if (pTerm->flags.got_obs && pTerm->flags.is_obs)
    bValid = FALSE ;
  return bValid ;
}



/*
 * Return the SOID stored in the term.
 */
ZMapSOID zMapSOTermGetID(const ZMapSOTerm const pTerm )
{
  if (!pTerm)
    return NULL ;
  return pTerm->pID ;
}



/*
 * Return pointer to internally stored name string.
 * New memory is NOT allocated for the return value.
 */
char * zMapSOTermGetName(const ZMapSOTerm const pTerm)
{
  if (!pTerm)
    return NULL ;
  return pTerm->sName ;
}


/*
 * Copy the second argument to the first.
 */
gboolean zMapSOTermCopy(ZMapSOTerm const pLHS, const ZMapSOTerm const pRHS)
{
  gboolean bResult = FALSE ;
  unsigned int iLength = 0 ;
  if (   !pLHS
      || !pLHS->pID
      || !pLHS->sName
      || !pRHS
      || !pRHS->pID
      || !pRHS->sName
     )
    return bResult ;

  /*
   * Copy ID object.
   */
  if (!zMapSOIDCopy(pLHS->pID, pRHS->pID))
    return bResult ;

  /*
   * Copy name string.
   */
  if (pLHS->sName)
    g_free(pLHS->sName) ;
  iLength = strlen(pRHS->sName) + 1;
  pLHS->sName = (char *) g_malloc(iLength) ;
  strcpy(pLHS->sName, pRHS->sName) ;
  bResult = TRUE ;

  /*
   * Copy the flags
   */
  pLHS->flags = pRHS->flags ;

  return bResult ;
}

/*
 * Test to see if the two arguments are equal.
 */
gboolean zMapSOTermEquals(const ZMapSOTerm const pLHS, const ZMapSOTerm const pRHS)
{
  gboolean bResult = FALSE;
  if (   !pLHS
      || !pLHS->pID
      || !pLHS->sName
      || !pRHS
      || !pRHS->pID
      || !pRHS->sName
     )
    return bResult ;

  if (!zMapSOIDEquals(pLHS->pID, pRHS->pID))
    return bResult ;
  if (strcmp(pLHS->sName, pRHS->sName))
    return bResult ;

  if (pLHS->flags.got_ter != pRHS->flags.got_ter)
    return bResult ;
  if (pLHS->flags.got_idd != pRHS->flags.got_idd)
    return bResult ;
  if (pLHS->flags.got_nam != pRHS->flags.got_nam)
    return bResult ;
  if (pLHS->flags.got_obs != pRHS->flags.got_obs)
    return bResult ;
  if (pLHS->flags.is_obs  != pRHS->flags.is_obs )
    return bResult ;

  bResult = TRUE ;
  return bResult ;
}









/*
 * Create an empty SO Collection object.
 */
ZMapSOCollection zMapSOCollectionCreate()
{
  ZMapSOCollection pCollection  = NULL ;
  pCollection = (ZMapSOCollection) g_malloc(sizeof(ZMapSOCollectionStruct)) ;
  if (!pCollection)
    return pCollection ;
  pCollection->pTerms = NULL ;
  pCollection->iNumTerms = 0 ;
  return pCollection ;
}

/*
 * Return the number of terms in the collection.
 */
unsigned int zMapSOCollectionGetNumTerms(const ZMapSOCollection const pCollection)
{
  if (!pCollection)
    return 0 ;
  return pCollection->iNumTerms ;
}


/*
 * Add an SOTerm to the SOCollection object. Return false if it was present
 * already. THis is done by copying the pointer value, not creating a new object.
 */
gboolean zMapSOCollectionAddSOTerm(ZMapSOCollection const pCollection, const ZMapSOTerm const pTerm)
{
  gboolean bResult = FALSE ;
  ZMapSOTerm *pTerms = NULL ;
  unsigned int iTerm ;
  /*
   * Check to see whether we have a term with the same ID
   * in the collection already.
   */
  if (zMapSOCollectionIsTermPresent(pCollection, pTerm))
    return bResult ;

  /*
   * Create a new array of terms one larger that already stored.
   */
  pTerms = (ZMapSOTerm*) g_malloc(sizeof(ZMapSOTerm)*(pCollection->iNumTerms+1)) ;
  if (!pTerms)
    return bResult ;

  /*
   * Copy data to new array
   */
  for (iTerm=0; iTerm<pCollection->iNumTerms; ++iTerm)
    pTerms[iTerm] = pCollection->pTerms[iTerm] ;

  /*
   * Add the new one on the end.
   */
  pTerms[iTerm] = pTerm ;

  /*
   * Free old storage.
   */
  g_free(pCollection->pTerms) ;

  /*
   * Copy new pointer
   */
  pCollection->pTerms = pTerms ;
  ++pCollection->iNumTerms ;
  bResult = TRUE ;

  return bResult ;
}


/*
 * Destroy an SO Collection object
 */
gboolean zMapSOCollectionDestroy(ZMapSOCollection const pCollection)
{
  gboolean bResult = TRUE ;
  unsigned int iTerm ;
  if (!pCollection)
    return bResult ;
  for (iTerm=0; iTerm<pCollection->iNumTerms; ++iTerm)
    if (pCollection->pTerms[iTerm])
      zMapSOTermDestroy(pCollection->pTerms[iTerm]) ;
  g_free(pCollection->pTerms) ;
  g_free(pCollection) ;
  return bResult ;
}


/*
 * Query a SO Collection to see if a term is present.
 * Check is made on numerical and string IDs.
 */
gboolean zMapSOCollectionIsTermPresent(const ZMapSOCollection const pCollection, const ZMapSOTerm const pTerm)
{
  gboolean bResult = FALSE ;
  unsigned int iTerm ;
  if (!pCollection || !pTerm )
    return bResult ;
  for (iTerm=0; iTerm<pCollection->iNumTerms; ++iTerm)
    if (zMapSOIDEquals(pTerm->pID, pCollection->pTerms[iTerm]->pID))
      return TRUE ;
  return bResult ;
}

/*
 * Quesry an SO Collection to see if an SO name present. Check is made on name string only.
 */
gboolean zMapSOCollectionIsNamePresent(const ZMapSOCollection const pCollection, const char* const sName)
{
  gboolean bResult = FALSE ;
  unsigned int iTerm ;
  if (!pCollection || !sName || !*sName )
    return bResult ;
  for (iTerm=0; iTerm<pCollection->iNumTerms; ++iTerm)
    if (!strcmp(sName, pCollection->pTerms[iTerm]->sName))
      return TRUE ;
  return bResult ;
}


/*
 * Find the SOID associated with a given name. Returns NULL if the
 * name is not present in the collection.
 */
ZMapSOID zMapSOCollectionFindIDFromName(const ZMapSOCollection const pCollection, const char* const sName)
{
  unsigned int iTerm = 0 ;
  for (iTerm=0; iTerm<pCollection->iNumTerms; ++iTerm)
    if (!strcmp(sName, pCollection->pTerms[iTerm]->sName))
      return pCollection->pTerms[iTerm]->pID ;
  return NULL ;
}


/*
 * Find the Name associated with a given SOID. Return NULL if the SOID is not in the collection.
 */
char * zMapSOCollectionFindNameFromID(const ZMapSOCollection const pCollection, const ZMapSOID const pID)
{
  unsigned int iTerm = 0 ;
  for (iTerm=0; iTerm<pCollection->iNumTerms; ++iTerm)
    if (zMapSOIDEquals(pID, pCollection->pTerms[iTerm]->pID))
      return pCollection->pTerms[iTerm]->sName ;
  return NULL ;
}



/*
 * Return a pointer to a term in the collection indexed with unsigned int.
 * Return NULL if it is not present.
 */
ZMapSOTerm zMapSOCollectionGetTerm(const ZMapSOCollection const pCollection, unsigned int iIndex)
{
  if (iIndex >= pCollection->iNumTerms)
    return NULL ;
  return pCollection->pTerms[iIndex] ;
}


/*
 * Checks the internal consistency of the collection. Currently
 * checks for duplicates only.
 */
gboolean zMapSOCollectionCheck(const ZMapSOCollection const pCollection)
{
  gboolean bResult = FALSE ;
  unsigned int iTerm = 0 ;
  ZMapSOTerm pTerm = NULL ;
  ZMapSOID pID01 = NULL, pID02 = NULL ;
  char *pName = NULL ;

  for (iTerm=0; iTerm<zMapSOCollectionGetNumTerms(pCollection); ++iTerm)
  {
    /*
     * Find current term and its SOID.
     */
    pTerm = NULL ;
    pTerm = zMapSOCollectionGetTerm(pCollection, iTerm) ;
    if (pTerm)
    {

      pID01 = zMapSOTermGetID(pTerm) ;

      /*
       * Name lookup with current term.
       */
      pID02 = zMapSOCollectionFindIDFromName(pCollection, pTerm->sName );
      if (!zMapSOIDEquals(pID01, pID02))
      {
        return bResult ;
      }

      /*
       * ID lookup with current term.
       */
      pName = zMapSOCollectionFindNameFromID(pCollection, pID01) ;
      if (strcmp(pName, pTerm->sName))
      {
        return bResult ;
      }

    }
    else
    {
      return bResult ;
    }

  }
  bResult = TRUE ;

  return bResult ;
}













/*
 * Find the type of the line we are currently on.
 */
static void zMapSOParserLineType(ZMapSOParser const pParser)
{
  ZMapSOParserLineType eType = ZMAPSO_LINE_UNK ;

  if (!pParser || !pParser->sBufferLine)
    return ;
  if (pParser->eLineType != ZMAPSO_LINE_UNK)
    return ;

  if (g_str_has_prefix(pParser->sBufferLine, "[Term]"))
  {
    eType = ZMAPSO_LINE_TER ;
  }
  else if (g_str_has_prefix(pParser->sBufferLine, "[Typedef]"))
  {
    eType = ZMAPSO_LINE_TYP ;
  }
  else if (g_str_has_prefix(pParser->sBufferLine, "id:"))
  {
    eType = ZMAPSO_LINE_IDD ;
  }
  else if (g_str_has_prefix(pParser->sBufferLine, "name:"))
  {
    eType = ZMAPSO_LINE_NAM ;
  }
  else if (g_str_has_prefix(pParser->sBufferLine, "is_obsolete:"))
  {
    eType = ZMAPSO_LINE_OBS ;
  }

  pParser->eLineType = eType ;

  return ;
}



/*
 * Create a SO parser object.
 */
ZMapSOParser zMapSOParserCreate()
{
  ZMapSOParser pParser  = NULL ;
  gboolean bClearCollection = FALSE ;

  pParser = (ZMapSOParser) g_malloc(sizeof(ZMapSOParserStruct)) ;
  if (!pParser)
    return pParser ;

  /*
   * Initialize line counter
   */
  pParser->iLineCount = 0 ;

  /*
   * Initialize the pointer to error object and the error domain
   */
  pParser->pError = NULL ;
  pParser->qErrorDomain = g_quark_from_string(ZMAPSO_PARSER_ERROR) ;

  /*
   * Stop on error flag. Default should be TRUE.
   */
  pParser->bStopOnError = TRUE ;

  /*
   * Initialize the line type
   */
  pParser->eLineType = ZMAPSO_LINE_UNK ;

  /*
   * Create SOID object
   */
  pParser->pID = zMapSOIDCreate() ;

  /*
   * Allocate for buffers; try to handdle memory allocation errors by
   * deleting everything created thus far and exiting.
   */
  pParser->sBufferLine = (char*) g_malloc(ZMAPSO_PARSER_BUFFER_LINELENGTH) ;
  if (!pParser->sBufferLine)
  {
    g_free(pParser) ;
    return NULL ;
  }
  pParser->sBuffer = (char*) g_malloc(ZMAPSO_PARSER_BUFFER_LENGTH) ;
  if (!pParser->sBuffer)
  {
    g_free(pParser->sBufferLine) ;
    g_free(pParser) ;
    return NULL ;
  }

  /*
   * Initialize pTerm and pCollection
   */
  pParser->pTerm = NULL ;
  pParser->pCollection = NULL ;

  /*
   * Initialize the parser and if this fails, destroy
   * it and return NULL.
   */
  if (!zMapSOParserInitialize(pParser, bClearCollection))
  {
    zMapSOParserDestroy(pParser) ;
    pParser = NULL ;
  }

  return pParser ;
}




/*
 * Return the ZMapSOCollection object associated with the
 * parser.
 *
 * NOTE: this returns the pointer to the collection and
 * sets the data member of the parser to null.
 */
ZMapSOCollection zMapSOParserGetCollection(ZMapSOParser const pParser)
{
  ZMapSOCollection pCollection = NULL ;
  if (!pParser )
    return pCollection ;

  /*
   * We check to see if the parser still stores a valid term
   * and if so, add it to the collection.
   */
  if (zMapSOTermIsValid(pParser->pTerm))
  {
    if (!zMapSOCollectionAddSOTerm(pParser->pCollection, pParser->pTerm) )
       zMapSOTermDestroy(pParser->pTerm) ;
    pParser->pTerm = NULL ;
  }

  pCollection = pParser->pCollection ;
  pParser->pCollection = NULL ;
  return pCollection ;
}



/*
 * Initialize all data in the parser including pTerm and
 * pCollection. We destroy pTerm automatically. Only
 * destroy pCollection if the flag is set.
 *
 * If the parser currently stores a colleciton, it will be
 * deleted and replaced with a new empty one if the
 * bClearCollection variable is TRUE.
 */
gboolean zMapSOParserInitialize(ZMapSOParser const pParser, gboolean bClearCollection)
{
  if (!pParser)
    return FALSE ;
  pParser->iLineCount = 0 ;
  if (pParser->pError)
    g_error_free(pParser->pError) ;
  pParser->pError = NULL ;
  zMapSOParserInitializeForLine(pParser) ;
  if (pParser->pTerm)
    zMapSOTermDestroy(pParser->pTerm) ;
  pParser->pTerm = NULL ;
  if (pParser->pCollection && bClearCollection )
    zMapSOCollectionDestroy(pParser->pCollection) ;
  pParser->pCollection = zMapSOCollectionCreate() ;
  return TRUE ;
}

/*
 * Initialize the parser data and buffers for reading a new line.
 * Do not alter the pTerm and pCollection objects.
 *
 * This function sets the line type to unknown, initializes the ID
 * object to unknown, and then writes null characters into the
 * string buffers.
 */
gboolean zMapSOParserInitializeForLine(ZMapSOParser const pParser)
{
  gboolean bResult = FALSE ;
  if (!pParser || !pParser->sBufferLine || !pParser->sBuffer)
    return bResult ;
  bResult = TRUE ;

  /*
   * Initialize line type member.
   */
  pParser->eLineType = ZMAPSO_LINE_UNK ;

  /*
   * Initialize ID data member.
   */
  zMapSOIDInitialize(pParser->pID);

  /*
   * Initialize buffers with null characters.
   */
  memset((void*) pParser->sBufferLine, (char) 0, ZMAPSO_PARSER_BUFFER_LINELENGTH) ;
  memset((void*) pParser->sBuffer, (char) 0, ZMAPSO_PARSER_BUFFER_LENGTH) ;

  return bResult ;
}



/*
 * Destroy a parser object.
 */
gboolean zMapSOParserDestroy(ZMapSOParser const pParser)
{
  gboolean bResult = FALSE ;
  if (!pParser)
    return bResult ;
  bResult = TRUE ;
  if (pParser->pError)
    g_error_free(pParser->pError) ;
  if (pParser->pID)
    zMapSOIDDestroy(pParser->pID) ;
  if (pParser->sBufferLine)
    g_free(pParser->sBufferLine) ;
  if (pParser->sBuffer)
    g_free(pParser->sBuffer) ;
  if (pParser->pTerm)
    zMapSOTermDestroy(pParser->pTerm) ;
  if (pParser->pCollection)
    zMapSOCollectionDestroy(pParser->pCollection) ;
  g_free(pParser) ;
  return bResult ;
}


/*
 * Parse the line stored in the parser buffer for the
 * is_obsolete flag. Value must be "true" or "false"
 * strings.
 */
static gboolean zMapSOParseIsObs(ZMapSOParser const pParser)
{
  static const char * const sFormat = "is_obsolete:%s";
  static const char * const sTrue = "true" ;
  static const char * const sFalse = "false" ;
  static const unsigned int iExpectedFields = 1 ;
  gboolean bResult = FALSE ;

  /*
   * Some error checks.
   */
  if (    !pParser
      ||  !pParser->sBufferLine
      ||  !*pParser->sBufferLine
      ||  !pParser->sBuffer
      ||  !pParser->pTerm
     )
    return bResult ;

  /*
   * Look for appropriate data in the input string.
   */
  if (sscanf(pParser->sBufferLine, sFormat, pParser->sBuffer) == iExpectedFields)
  {
    if (!strcmp(sTrue, pParser->sBuffer))
    {
      pParser->pTerm->flags.is_obs = TRUE ;
      bResult = TRUE ;
    }
    else if (!strcmp(sFalse, pParser->sBuffer))
    {
      pParser->pTerm->flags.is_obs = FALSE ;
      bResult = TRUE ;
    }
    else
    {

      if (pParser->pError)
      {
        g_error_free(pParser->pError) ;
        pParser->pError = g_error_new(pParser->qErrorDomain, ZMAPSO_ERROR_PARSER_ISO,
                                    "ERROR in zMapSOParseIsOsolete(); HIT ERROR CONDITION WITH ERROR ALREADY SET at line %i, with l = '%s'",
                                    pParser->iLineCount, pParser->sBufferLine ) ;
      }
      else
      {
        pParser->pError = g_error_new(pParser->qErrorDomain, ZMAPSO_ERROR_PARSER_ISO,
                                    "ERROR in zMapSOParseID(); unable to set IsObsolete flag at line %i, with l = '%s'",
                                    pParser->iLineCount, pParser->sBufferLine ) ;
      }

    }

    pParser->pTerm->flags.got_obs = TRUE ;

  }

  return bResult ;
}

/*
 * Parse the line stored in parser buffer for a so id.
 * This is a line of the form:
 * "id: SO:XXXXXXX"
 *
 * We want the number out of this to store as unsigned int.
 */
static gboolean zMapSOParseID(ZMapSOParser const pParser)
{
  static const char * const sFormatID = "id:%s" ;
  static const unsigned int iExpectedFields = 1 ;
  gboolean bResult = FALSE ;

  /*
   * Some error checks.
   */
  if (    !pParser
      ||  !pParser->sBufferLine
      ||  !*pParser->sBufferLine
      ||  !pParser->sBuffer
      ||  !pParser->pTerm
     )
    return bResult ;

  /*
   * Look for appropriate data form in the input string.
   */
  if (sscanf(pParser->sBufferLine, sFormatID, pParser->sBuffer) == iExpectedFields)
  {
    if (zMapSOIDSet(pParser->pID, pParser->sBuffer))
    {
      bResult = zMapSOTermSetID(pParser->pTerm, pParser->pID) ;
    }
    else
    {

      if (pParser->pError)
      {
        g_error_free(pParser->pError) ;
        pParser->pError = g_error_new(pParser->qErrorDomain, ZMAPSO_ERROR_PARSER_IDD,
                                    "ERROR in zMapSOParseID(); HIT ERROR CONDITION WITH ERROR ALREADY SET at line %i, with l = '%s'",
                                    pParser->iLineCount, pParser->sBufferLine ) ;
      }
      else
      {
        pParser->pError = g_error_new(pParser->qErrorDomain, ZMAPSO_ERROR_PARSER_IDD,
                                    "ERROR in zMapSOParseID(); unable to set SOID at line %i, with l = '%s'",
                                    pParser->iLineCount, pParser->sBufferLine ) ;
      }

    }
  }

  return bResult ;
}



/*
 * Parse the line stored in the parser buffer for a name.
 * This is a line of the form:
 * "name: <string>"
 *
 * We are just after the string here.
 */
static gboolean zMapSOParseName(ZMapSOParser const pParser)
{
  static const char * const sFormatName = "name:%s" ;
  static const unsigned int iExpectedFields = 1 ;
  gboolean bResult = FALSE ;

  /*
   * Some error checks.
   */
  if (     !pParser
      ||   !pParser->sBufferLine
      ||   !*pParser->sBufferLine
      ||   !pParser->sBuffer
     )
    return bResult ;

  /*
   * Look for data of the appropriate form.
   */
  if (sscanf(pParser->sBufferLine, sFormatName, pParser->sBuffer) == iExpectedFields)
  {
    bResult = zMapSOTermSetName(pParser->pTerm, pParser->sBuffer) ;
  }
  else
  {

    if (pParser->pError)
    {
      g_error_free(pParser->pError) ;
      pParser->pError = g_error_new(pParser->qErrorDomain, ZMAPSO_ERROR_PARSER_IDD,
                                  "ERROR in zMapSOParseName(); HIT ERROR CONDITION WITH ERROR ALREADY SET at line %i, with l = '%s'",
                                  pParser->iLineCount, pParser->sBufferLine ) ;
    }
    else
    {
      pParser->pError = g_error_new(pParser->qErrorDomain, ZMAPSO_ERROR_PARSER_IDD,
                                  "ERROR in zMapSOParseName(); unable to parse for name at line %i, with l = '%s'",
                                  pParser->iLineCount, pParser->sBufferLine ) ;
    }

  }

  return bResult ;
}



/*
 * Return the line type stored by the parser.
 */
static ZMapSOParserLineType zMapSOParserGetLineType(const ZMapSOParser const pParser)
{
  if (!pParser)
    return ZMAPSO_LINE_UNK ;
  return pParser->eLineType ;
}





/*
 * Parse one line at a time for SO terms.
 *
 * To be called repeatedly with lines from a SO-type file.
 *
 */
gboolean zMapSOParseLine(ZMapSOParser const pParser, const char * const sLine)
{
  gboolean bResult = FALSE ;
  ZMapSOParserLineType eLineType ;

  if (!pParser || !sLine )
    return bResult ;

  /*
   * Initialize parser.
   */
  ++pParser->iLineCount ;
  zMapSOParserInitializeForLine(pParser) ;
  strcpy(pParser->sBufferLine, sLine) ;
  zMapSOParserLineType(pParser) ;
  eLineType = zMapSOParserGetLineType(pParser) ;

  /*
   * Switch actions depending on line type.
   */
  if ((eLineType == ZMAPSO_LINE_TER) || (eLineType == ZMAPSO_LINE_TYP))
  {

    /*
     * If we have reached a [Term] or [Typedef] line then we should try to
     * add the current term before going on.
     *
     * If the term is not valid, we destroy and carry on. If the term is valid
     * and we cannot add to collection, destroy and carry on. Otherwise it is
     * added so we do not destroy term.
     */
    if  (     !zMapSOTermIsValid(pParser->pTerm)
          ||  !zMapSOCollectionAddSOTerm(pParser->pCollection, pParser->pTerm)
        )
      zMapSOTermDestroy(pParser->pTerm) ;
    pParser->pTerm = NULL ;

    /*
     * Only the [Term] line signals the beginning of a new SO term object.
     */
    if (eLineType == ZMAPSO_LINE_TER)
      pParser->pTerm = zMapSOTermCreate() ;

  }
  else if (eLineType == ZMAPSO_LINE_IDD)
  {

    if (pParser->pTerm && zMapSOParseID(pParser) )
    {
      bResult = TRUE ;
    }

  }
  else if (eLineType == ZMAPSO_LINE_NAM)
  {

    if (pParser->pTerm && zMapSOParseName(pParser) )
    {
      bResult = TRUE ;
    }

  }
  else if (eLineType == ZMAPSO_LINE_OBS)
  {
    if (pParser->pTerm && zMapSOParseIsObs(pParser) )
    {
      bResult = TRUE ;
    }
  }

  /*
   * Test to make sure that an error was not set during any of
   * the above, and if not we can return TRUE.
   */
  if (!pParser->pError)
    bResult = TRUE ;

  return bResult ;
}


/*
 * Return pointer to error.
 */
GError *zMapSOParserGetError(const ZMapSOParser const pParser)
{
  if (!pParser)
    return NULL ;
  return pParser->pError ;
}

/*
 * Return the parser line counter.
 */
unsigned int zMapSOParserGetLineCount(const ZMapSOParser const pParser )
{
  if (!pParser)
    return 0 ;
  return pParser->iLineCount ;
}


/*
 * Set the flag for the parser to stop when
 */
gboolean zMapSOParserSetStopOnError(ZMapSOParser const pParser, gboolean bValue)
{
  gboolean bResult = FALSE ;
  if (!pParser)
    return bResult ;
  bResult = TRUE ;
  pParser->bStopOnError = bValue ;
  return bResult ;
}


/*
 * Is the parser terminated?
 */
gboolean zMapSOParserTerminated(const ZMapSOParser const pParser)
{
  gboolean bResult = FALSE ;
  if (!pParser)
    return bResult ;
  if (pParser->bStopOnError && pParser->pError)
    bResult = TRUE ;
  return bResult ;
}

















/*
 * Read a line from a file and remove newline character.
 */
static gboolean zMapSOReadLineFromFile(char* sBuffer, unsigned int iLength, FILE* fInput)
{
  static const char cNewline = '\n' ;
  static const char cNull = '\0' ;
  gboolean bResult = FALSE ;
  char *sPos = NULL ;
  if (!sBuffer || !iLength)
    return bResult ;
  if (!fInput)
    return bResult ;

  /*
   * Read a single line from the file.
   */
  if (!fgets(sBuffer, iLength, fInput))
    return bResult ;
  bResult = TRUE ;

  /*
   * Find and remove terminating newline if present.
   */
  sPos = strchr(sBuffer, cNewline) ;
  if (sPos)
    *sPos = cNull ;

  return bResult ;
}





/*
 * Read a SO Collection from a file.
 */
gboolean zMapSOParseFile(ZMapSOParser const pParser, const char * const sFilename)
{
  static const char* const sFilemode = "r" ;
  char sBuffer[ZMAPSO_PARSER_BUFFER_LENGTH] ;
  gboolean bReturn = FALSE ;
  FILE *fInput = NULL ;

  /*
   * Error checks.
   */
  if (!pParser )
    return  bReturn ;
  if (!sFilename || !*sFilename)
    return bReturn ;

  /*
   * Open the file.
   */
  fInput = fopen(sFilename, sFilemode) ;
  if (!fInput)
    return bReturn ;

  /*
   * Read the file.
   */
  bReturn = TRUE ;
  while (!feof(fInput))
  {
    /*
     * Read line from file.
     */
    if (!zMapSOReadLineFromFile(sBuffer, ZMAPSO_PARSER_BUFFER_LENGTH, fInput))
      break ;

    /*
     * Parse that line.
     */
    zMapSOParseLine(pParser, sBuffer) ;

    /*
     * Test for parser error.
     */
    if (zMapSOParserTerminated(pParser))
    {
      printf("error reading file '%s' at i = %i, l = '%s'\n", sFilename, pParser->iLineCount, sBuffer) ;
      fflush(stdout) ;
      break ;
    }

  }

  fclose(fInput) ;

  return bReturn ;
}



/*
 * Read ZMapSOCollection from file.
 */
ZMapSOCollection zMapSOReadFile(const char* const sFilename)
{
  ZMapSOParser pParser = NULL ;
  ZMapSOCollection pCollection = NULL ;
  gboolean bClearCollection = TRUE ;

  /*
   * Create file parser.
   */
  pParser = zMapSOParserCreate() ;
  if (!pParser)
  {
    return pCollection ;
  }

  /*
   * Initialize for reading a file.
   */
  if (!zMapSOParserInitialize(pParser, bClearCollection))
  {
    zMapSOParserDestroy(pParser) ;
    return pCollection ;
  }

  /*
   * Read a collection of terms from a file.
   */
  if (!zMapSOParseFile(pParser, sFilename))
  {
    zMapSOParserDestroy(pParser) ;
    return pCollection ;
  }

  /*
   * Run internal check, and destroy collection if
   * it fails.
   */
  pCollection = zMapSOParserGetCollection(pParser) ;
  if (!zMapSOCollectionCheck(pCollection))
  {
    zMapSOCollectionDestroy(pCollection) ;
    pCollection = NULL ;
  }

  /*
   * Destroy parser.
   */
  zMapSOParserDestroy(pParser) ;

  return pCollection ;
}


/*
 * Read the SO Collections.
 */
gboolean zMapSOReadSOCollections()
{
  gboolean bResult = TRUE ;

  /*
   * Read files if they have not been read already.
   */
  if (!pCollectionSOFA)
    if (!(pCollectionSOFA = zMapSOReadFile(sFilenameSOFA) ))
  {
    zMapSOCollectionDestroy(pCollectionSOFA) ;
    bResult = FALSE ;
  }
  if (!pCollectionSO)
    if (!(pCollectionSO = zMapSOReadFile(sFilenameSO) ))
  {
    zMapSOCollectionDestroy(pCollectionSO) ;
    bResult = FALSE ;
  }

  return bResult ;
}

/*
 * Destroy SO collections.
 */
gboolean zMapSODestroySOCollections()
{
  gboolean bResult = TRUE ;

  if (!zMapSOCollectionDestroy(pCollectionSOFA))
    bResult = FALSE ;
  if (!zMapSOCollectionDestroy(pCollectionSO))
    bResult = FALSE ;

  return bResult ;
}


/*
 * Return the SO Collection.
 */
ZMapSOCollection zMapSOCollectionSO()
{
  return pCollectionSO ;
}

/*
 * Return the SOFA collection.
 */
ZMapSOCollection zMapSOCollectionSOFA()
{
  return pCollectionSOFA ;
}








