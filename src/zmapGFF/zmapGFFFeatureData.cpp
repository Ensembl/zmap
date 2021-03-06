/*  File: zmapGFFFeatureData.c
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
 * Description: FeatureData storage and management for GFFv3.
 *
 *-------------------------------------------------------------------
 */

#include <zmapGFFFeatureData.hpp>




/*
 * Create an empty ZMapGFFFeatureData object; contains information as parsed directly
 * from GFF lines and then passed on to create features. The raw data are as given below
 * along with flags to say whether or not each of them is present. Most should be, but
 * not all of the first eight data items are mandatory (despite their presence in the
 * GFF line being mandatory). The score, strand and phase are all optional.
 */
ZMapGFFFeatureData zMapGFFFeatureDataCreate()
{
  ZMapGFFFeatureData pFeatureData = NULL;
  pFeatureData = (ZMapGFFFeatureData) g_new0(ZMapGFFFeatureDataStruct, 1) ;
  if (!pFeatureData)
    return NULL;

  pFeatureData->sSequence      = NULL ;                    /* As parsed from GFF line                      */
  pFeatureData->sSource        = NULL ;                    /* As parsed from GFF line                      */
  pFeatureData->iStart         = 0 ;                       /* As parsed from GFF line                      */
  pFeatureData->iEnd           = 0 ;                       /* As parsed from GFF line                      */
  pFeatureData->dScore         = 0.0;                      /* As parsed from GFF line                      */
  pFeatureData->cStrand        = ZMAPSTRAND_NONE ;         /* As parsed from GFF line                      */
  pFeatureData->cPhase         = ZMAPPHASE_NONE ;          /* As parsed from GFF line                      */
  pFeatureData->sAttributes    = NULL ;                    /* As parsed from GFF line (may be NULL)        */
  pFeatureData->pAttributes    = NULL ;                    /* Array of attribute structs                   */ 
  pFeatureData->pSOIDData      = NULL ;                    /* From auto-generated header                   */

  pFeatureData->flags.got_seq        = FALSE ;
  pFeatureData->flags.got_sou        = FALSE ;
  pFeatureData->flags.got_sta        = FALSE ;
  pFeatureData->flags.got_end        = FALSE ;
  pFeatureData->flags.got_sco        = FALSE ;
  pFeatureData->flags.got_str        = FALSE ;
  pFeatureData->flags.got_pha        = FALSE ;
  pFeatureData->flags.got_att        = FALSE ;
  pFeatureData->flags.got_sod        = FALSE ;

  return pFeatureData ;
}

/*
 * A "copy constructor" that returns pointer to a new object by copying the argument.
 */
ZMapGFFFeatureData zMapGFFFeatureDataCC(ZMapGFFFeatureData pData )
{
  ZMapGFFFeatureData pFeatureData = NULL ;
  pFeatureData = (ZMapGFFFeatureData) g_new0(ZMapGFFFeatureDataStruct, 1) ;
  zMapReturnValIfFail(pData && pFeatureData, pFeatureData) ;

  pFeatureData->sSequence    = pData->sSequence ? g_strdup(pData->sSequence) : NULL ;
  pFeatureData->sSource      = pData->sSource ? g_strdup(pData->sSource) : NULL ;
  pFeatureData->iStart       = pData->iStart ;
  pFeatureData->iEnd         = pData->iEnd ;
  pFeatureData->dScore       = pData->dScore ;
  pFeatureData->cStrand      = pData->cStrand ;
  pFeatureData->cPhase       = pData->cPhase ;
  pFeatureData->sAttributes  = pData->sAttributes ? g_strdup(pData->sAttributes) : NULL ;
  pFeatureData->pSOIDData    = zMapSOIDDataCC(pData->pSOIDData) ;

  pFeatureData->flags.got_seq        = pData->flags.got_seq ;
  pFeatureData->flags.got_sou        = pData->flags.got_sou ;
  pFeatureData->flags.got_sta        = pData->flags.got_sta ;
  pFeatureData->flags.got_end        = pData->flags.got_end ;
  pFeatureData->flags.got_sco        = pData->flags.got_sco ;
  pFeatureData->flags.got_str        = pData->flags.got_str ;
  pFeatureData->flags.got_pha        = pData->flags.got_pha ;
  pFeatureData->flags.got_att        = pData->flags.got_att ;
  pFeatureData->flags.got_sod        = pData->flags.got_sod ;

  return pFeatureData ;
}



/*
 * Delete a ZMapGFFFeatureData object
 */
void zMapGFFFeatureDataDestroy(ZMapGFFFeatureData pFeatureData)
{
  if (!pFeatureData)
    return ;

  /*
   * Destroy string data
   */
  if (pFeatureData->sSequence)
    g_free(pFeatureData->sSequence) ;
  if (pFeatureData->sSource)
    g_free(pFeatureData->sSource) ;
  if (pFeatureData->sAttributes)
    g_free(pFeatureData->sAttributes) ;

  /*
   * Destroy SOIDData object
   */
  if (pFeatureData->pSOIDData)
    zMapSOIDDataDestroy(pFeatureData->pSOIDData) ;

  /*
   * Delete top level object
   */
  g_free(pFeatureData) ;

}



/*
 * Set the internal data of the object. All data are required, apart from the score,
 * which we do not necessarily have.
 */
gboolean zMapGFFFeatureDataSet(ZMapGFFFeatureData pFeatureData,
                               const char* const sSequence,
                               const char* const sSource,
                               int iStart,
                               int iEnd,
                               gboolean bHasScore,
                               double dScore,
                               ZMapStrand cStrand,
                               ZMapPhase cPhase,
                               const char* const sAttributes,
                               ZMapGFFAttribute *pAttributes,
                               unsigned int nAttributes,
                               const ZMapSOIDData pSOID
                               )
{
  gboolean bResult = FALSE ;
  if (      !pFeatureData
        ||  !sSequence
        ||  !*sSequence
        ||  !sSource
        ||  !*sSource
        ||  !iStart
        ||  !iEnd
        ||  !pSOID
     )
    return bResult ;

  pFeatureData->sSequence         = g_strdup(sSequence) ;
  pFeatureData->flags.got_seq     = TRUE ;

  pFeatureData->sSource           = g_strdup(sSource) ;
  pFeatureData->flags.got_sou     = TRUE ;

  pFeatureData->iStart            = iStart ;
  pFeatureData->flags.got_sta     = TRUE ;

  pFeatureData->iEnd              = iEnd ;
  pFeatureData->flags.got_end     = TRUE ;

  if (bHasScore)
    {
      pFeatureData->dScore            = dScore ;
      pFeatureData->flags.got_sco     = TRUE ;
    }
  else
    {
      pFeatureData->dScore            = 0.0 ;
      pFeatureData->flags.got_sco     = FALSE ;
    }

  pFeatureData->cStrand           = cStrand ;
  pFeatureData->flags.got_str     = TRUE ;

  pFeatureData->cPhase            = cPhase ;
  pFeatureData->flags.got_pha     = TRUE ;

  pFeatureData->sAttributes       = g_strdup(sAttributes) ;
  pFeatureData->flags.got_att     = TRUE ;

  pFeatureData->pAttributes       = pAttributes ;
  pFeatureData->nAttributes       = nAttributes ;
  pFeatureData->flags.got_pat     = TRUE ;

  pFeatureData->pSOIDData         = pSOID ;
  pFeatureData->flags.got_sod     = TRUE ;

  return bResult ;
}



/*
 * Test whether or not the object is a valid one (i.e. contains
 * valid data fields). The data fields are set and tested elsewhere.
 * We require all fields to be set except for the score which is
 * not necessarily present in the original data.
 */
gboolean zMapGFFFeatureDataIsValid( ZMapGFFFeatureData pFeatureData  )
{
  gboolean bResult = FALSE ;
  if (!pFeatureData)
    return bResult ;
  if (!pFeatureData->flags.got_seq)
    return bResult ;
  if (!pFeatureData->flags.got_sou)
    return bResult ;
  if (!pFeatureData->flags.got_sta)
    return bResult ;
  if (!pFeatureData->flags.got_end)
    return bResult ;
  if (!pFeatureData->flags.got_str)
    return bResult ;
  if (!pFeatureData->flags.got_pha)
    return bResult ;
  if (!pFeatureData->flags.got_att)
    return bResult ;
  if (!pFeatureData->flags.got_pat)
    return bResult ;
  if (!pFeatureData->flags.got_sod)
    return bResult ;
  bResult = TRUE ;
  return bResult ;
}






/*
 * Functions to return the values of various flags.
 */
gboolean zMapGFFFeatureDataGetFlagSeq(ZMapGFFFeatureData pFeatureData)
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_seq ;
}

gboolean zMapGFFFeatureDataGetFlagSou(ZMapGFFFeatureData pFeatureData)
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_sou ;
}

gboolean zMapGFFFeatureDataGetFlagSta(ZMapGFFFeatureData pFeatureData)
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_sta ;
}

gboolean zMapGFFFeatureDataGetFlagEnd(ZMapGFFFeatureData pFeatureData)
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_end ;
}

gboolean zMapGFFFeatureDataGetFlagSco( ZMapGFFFeatureData pFeatureData)
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_sco ;
}

gboolean zMapGFFFeatureDataGetFlagStr( ZMapGFFFeatureData pFeatureData)
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_str ;
}

gboolean zMapGFFFeatureDataGetFlagPha(ZMapGFFFeatureData pFeatureData)
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_pha ;
}

gboolean zMapGFFFeatureDataGetFlagAtt(ZMapGFFFeatureData pFeatureData)
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_att ;
}


gboolean zMapGFFFeatureDataGetFlagPat(ZMapGFFFeatureData pFeatureData )
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_pat;
}

gboolean zMapGFFFeatureDataGetFlagSod(ZMapGFFFeatureData pFeatureData )
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_sod ;
}

/*
 * Set some of the flags
 */
gboolean zMapGFFFeatureDataSetFlagSco(ZMapGFFFeatureData pFeatureData, gboolean bValue)
{
  gboolean bResult = FALSE ;
  zMapReturnValIfFail(pFeatureData, bResult ) ;
  bResult = TRUE ;
  pFeatureData->flags.got_sco = bValue ;
  return bResult ;
}


/*
 * Functions to set some data members.
 */

gboolean zMapGFFFeatureDataSetSeq(ZMapGFFFeatureData pFeatureData , const char * const sData)
{
  gboolean bResult = FALSE ;

  zMapReturnValIfFail(pFeatureData && sData, bResult ) ;

  if (pFeatureData->sSequence)
    g_free(pFeatureData->sSequence) ;

  pFeatureData->sSequence = g_strdup(sData) ;
  bResult = TRUE ;

  return bResult ;
}


gboolean zMapGFFFeatureDataSetSou(ZMapGFFFeatureData pFeatureData, const char * const sData )
{
  gboolean bResult = FALSE ;

  zMapReturnValIfFail(pFeatureData && sData, bResult ) ;

  if (pFeatureData->sSource)
    g_free (pFeatureData->sSource) ;

  pFeatureData->sSource = g_strdup(sData) ;
  bResult = TRUE ;

  return bResult ;
}

gboolean zMapGFFFeatureDataSetSod(ZMapGFFFeatureData pFeatureData , ZMapSOIDData pData )
{
  gboolean bResult = FALSE ;

  zMapReturnValIfFail(pFeatureData && pData, bResult ) ;

  if (pFeatureData->pSOIDData)
    zMapSOIDDataDestroy(pFeatureData->pSOIDData) ;

  pFeatureData->pSOIDData = zMapSOIDDataCC(pData) ;

  return bResult ;
}


gboolean zMapGFFFeatureDataSetSta(ZMapGFFFeatureData pFeatureData, int iVal)
{
  gboolean bResult = FALSE ;

  zMapReturnValIfFail(pFeatureData, bResult) ;

  pFeatureData->iStart = iVal ;

  bResult = TRUE ;
  return bResult ;
}

gboolean zMapGFFFeatureDataSetEnd(ZMapGFFFeatureData pFeatureData, int iVal)
{
  gboolean bResult = FALSE ;

  zMapReturnValIfFail(pFeatureData, bResult) ;

  pFeatureData->iEnd = iVal ;

  bResult = TRUE ;
  return bResult ;
}

gboolean zMapGFFFeatureDataSetSco(ZMapGFFFeatureData pFeatureData, double dVal)
{
  gboolean bResult = FALSE ;

  zMapReturnValIfFail(pFeatureData, bResult) ;

  pFeatureData->dScore = dVal ;
  bResult = TRUE ;

  return bResult ;
}

gboolean zMapGFFFeatureDataSetStr(ZMapGFFFeatureData pFeatureData, ZMapStrand cStrand)
{
  gboolean bResult = FALSE ;

  zMapReturnValIfFail(pFeatureData, bResult) ;

  pFeatureData->cStrand = cStrand ;
  bResult = TRUE ;

  return bResult ;
}

gboolean zMapGFFFeatureDataSetPha(ZMapGFFFeatureData pFeatureData, ZMapPhase cPhase)
{
  gboolean bResult = FALSE ;

  zMapReturnValIfFail(pFeatureData, bResult) ;

  pFeatureData->cPhase = cPhase ;
  bResult = TRUE ;

  return bResult ;
}




/*
 * Functions to return various of the object data elements.
 */
char*         zMapGFFFeatureDataGetSeq(ZMapGFFFeatureData pFeatureData)
{
  zMapReturnValIfFail(pFeatureData, NULL) ;
  return pFeatureData->sSequence ;
}

char*         zMapGFFFeatureDataGetSou(ZMapGFFFeatureData pFeatureData)
{
  zMapReturnValIfFail(pFeatureData, NULL) ;
  return pFeatureData->sSource ;
}

int           zMapGFFFeatureDataGetSta(ZMapGFFFeatureData pFeatureData )
{
  zMapReturnValIfFail(pFeatureData, 0) ;
  return pFeatureData->iStart ;
}

int           zMapGFFFeatureDataGetEnd(ZMapGFFFeatureData pFeatureData )
{
  zMapReturnValIfFail(pFeatureData, 0) ;
  return pFeatureData->iEnd ;
}

double        zMapGFFFeatureDataGetSco(ZMapGFFFeatureData pFeatureData)
{
  zMapReturnValIfFail(pFeatureData, 0.0) ;
  return pFeatureData->dScore ;
}

ZMapStrand    zMapGFFFeatureDataGetStr(ZMapGFFFeatureData pFeatureData)
{
  zMapReturnValIfFail(pFeatureData, ZMAPSTRAND_NONE) ;
  return pFeatureData->cStrand ;
}

ZMapPhase     zMapGFFFeatureDataGetPha( ZMapGFFFeatureData  pFeatureData )
{
  zMapReturnValIfFail(pFeatureData, ZMAPPHASE_NONE ) ;
  return pFeatureData->cPhase ;
}

char*         zMapGFFFeatureDataGetAtt(ZMapGFFFeatureData pFeatureData)
{
  zMapReturnValIfFail(pFeatureData, NULL) ;
  return pFeatureData->sAttributes ;
}


unsigned int       zMapGFFFeatureDataGetNat(ZMapGFFFeatureData pFeatureData )
{
  zMapReturnValIfFail(pFeatureData, 0 ) ;
  return pFeatureData->nAttributes ;
}


ZMapGFFAttribute * zMapGFFFeatureDataGetAts(ZMapGFFFeatureData pFeatureData )
{
  zMapReturnValIfFail(pFeatureData, NULL );
  return pFeatureData->pAttributes ;
}

const ZMapSOIDData zMapGFFFeatureDataGetSod(ZMapGFFFeatureData pFeatureData)
{
  zMapReturnValIfFail(pFeatureData, NULL ) ;
  return pFeatureData->pSOIDData ;
}

