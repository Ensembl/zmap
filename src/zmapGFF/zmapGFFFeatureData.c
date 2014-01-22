/*  File: zmapGFFFeatureData.c
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
 * Description: FeatureData storage and management for GFFv3.
 *
 *-------------------------------------------------------------------
 */

#include "zmapGFFFeatureData.h"




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
ZMapGFFFeatureData zMapGFFFeatureDataCC(const ZMapGFFFeatureData const pData )
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
void zMapGFFFeatureDataDestroy(ZMapGFFFeatureData const pFeatureData)
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
gboolean zMapGFFFeatureDataSet(ZMapGFFFeatureData const pFeatureData,
                               const char* const sSequence,
                               const char* const sSource,
                               int iStart,
                               int iEnd,
                               gboolean bHasScore,
                               double dScore,
                               ZMapStrand cStrand,
                               ZMapPhase cPhase,
                               const char* const sAttributes,
                               const ZMapGFFAttribute * const pAttributes,
                               unsigned int nAttributes,
                               const ZMapSOIDData const pSOID
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
gboolean zMapGFFFeatureDataIsValid(const ZMapGFFFeatureData const pFeatureData  )
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
gboolean zMapGFFFeatureDataGetFlagSeq(const ZMapGFFFeatureData const pFeatureData)
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_seq ;
}

gboolean zMapGFFFeatureDataGetFlagSou(const ZMapGFFFeatureData const pFeatureData)
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_sou ;
}

gboolean zMapGFFFeatureDataGetFlagSta(const ZMapGFFFeatureData const pFeatureData)
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_sta ;
}

gboolean zMapGFFFeatureDataGetFlagEnd(const ZMapGFFFeatureData const pFeatureData)
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_end ;
}

gboolean zMapGFFFeatureDataGetFlagSco(const ZMapGFFFeatureData const pFeatureData)
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_sco ;
}

gboolean zMapGFFFeatureDataGetFlagStr(const ZMapGFFFeatureData const pFeatureData)
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_str ;
}

gboolean zMapGFFFeatureDataGetFlagPha(const ZMapGFFFeatureData const pFeatureData)
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_pha ;
}

gboolean zMapGFFFeatureDataGetFlagAtt(const ZMapGFFFeatureData const pFeatureData)
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_att ;
}


gboolean zMapGFFFeatureDataGetFlagPat(const ZMapGFFFeatureData const pFeatureData )
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_pat;
}

gboolean zMapGFFFeatureDataGetFlagSod(const ZMapGFFFeatureData const pFeatureData )
{
  if (!pFeatureData)
    return FALSE ;
  return pFeatureData->flags.got_sod ;
}

/*
 * Set some of the flags
 */
gboolean zMapGFFFeatureDataSetFlagSco(ZMapGFFFeatureData const pFeatureData, gboolean bValue)
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

gboolean zMapGFFFeatureDataSetSeq(ZMapGFFFeatureData const pFeatureData , const char * const sData)
{
  gboolean bResult = FALSE ;

  zMapReturnValIfFail(pFeatureData && sData, bResult ) ;

  if (pFeatureData->sSequence)
    g_free(pFeatureData->sSequence) ;

  pFeatureData->sSequence = g_strdup(sData) ;
  bResult = TRUE ;

  return bResult ;
}


gboolean zMapGFFFeatureDataSetSou(ZMapGFFFeatureData const pFeatureData, const char * const sData )
{
  gboolean bResult = FALSE ;

  zMapReturnValIfFail(pFeatureData && sData, bResult ) ;

  if (pFeatureData->sSource)
    g_free (pFeatureData->sSource) ;

  pFeatureData->sSource = g_strdup(sData) ;
  bResult = TRUE ;

  return bResult ;
}

gboolean zMapGFFFeatureDataSetSod(ZMapGFFFeatureData const pFeatureData , const ZMapSOIDData const pData )
{
  gboolean bResult = FALSE ;

  zMapReturnValIfFail(pFeatureData && pData, bResult ) ;

  if (pFeatureData->pSOIDData)
    zMapSOIDDataDestroy(pFeatureData->pSOIDData) ;

  pFeatureData->pSOIDData = zMapSOIDDataCC(pData) ;

  return bResult ;
}


gboolean zMapGFFFeatureDataSetSta(ZMapGFFFeatureData const pFeatureData, int iVal)
{
  gboolean bResult = FALSE ;

  zMapReturnValIfFail(pFeatureData, bResult) ;

  pFeatureData->iStart = iVal ;

  bResult = TRUE ;
  return bResult ;
}

gboolean zMapGFFFeatureDataSetEnd(ZMapGFFFeatureData const pFeatureData, int iVal)
{
  gboolean bResult = FALSE ;

  zMapReturnValIfFail(pFeatureData, bResult) ;

  pFeatureData->iEnd = iVal ;

  bResult = TRUE ;
  return bResult ;
}

gboolean zMapGFFFeatureDataSetSco(ZMapGFFFeatureData const pFeatureData, double dVal)
{
  gboolean bResult = FALSE ;

  zMapReturnValIfFail(pFeatureData, bResult) ;

  pFeatureData->dScore = dVal ;
  bResult = TRUE ;

  return bResult ;
}

gboolean zMapGFFFeatureDataSetStr(const ZMapGFFFeatureData const pFeatureData, ZMapStrand cStrand)
{
  gboolean bResult = FALSE ;

  zMapReturnValIfFail(pFeatureData, bResult) ;

  pFeatureData->cStrand = cStrand ;
  bResult = TRUE ;

  return bResult ;
}

gboolean zMapGFFFeatureDataSetPha(const ZMapGFFFeatureData const pFeatureData, ZMapPhase cPhase)
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
char*         zMapGFFFeatureDataGetSeq(const ZMapGFFFeatureData const pFeatureData)
{
  if (!pFeatureData)
    return NULL ;
  return pFeatureData->sSequence ;
}

char*         zMapGFFFeatureDataGetSou(const ZMapGFFFeatureData const pFeatureData)
{
  if (!pFeatureData)
    return NULL ;
  return pFeatureData->sSource ;
}

int           zMapGFFFeatureDataGetSta(const ZMapGFFFeatureData const pFeatureData )
{
  if (!pFeatureData)
    return 0 ;
  return pFeatureData->iStart ;
}

int           zMapGFFFeatureDataGetEnd(const ZMapGFFFeatureData const pFeatureData )
{
  if (!pFeatureData)
    return 0 ;
  return pFeatureData->iEnd ;
}

double        zMapGFFFeatureDataGetSco(const ZMapGFFFeatureData const pFeatureData)
{
  if (!pFeatureData)
    return 0.0 ;
  return pFeatureData->dScore ;
}

ZMapStrand    zMapGFFFeatureDataGetStr(const ZMapGFFFeatureData const pFeatureData)
{
  if (!pFeatureData)
    return ZMAPSTRAND_NONE ;
  return pFeatureData->cStrand ;
}

ZMapPhase     zMapGFFFeatureDataGetPha(const ZMapGFFFeatureData const pFeatureData )
{
  if (!pFeatureData)
    return ZMAPPHASE_NONE ;
  return pFeatureData->cPhase ;
}

char*         zMapGFFFeatureDataGetAtt(const ZMapGFFFeatureData const pFeatureData)
{
  if (!pFeatureData)
    return NULL ;
  return pFeatureData->sAttributes ;
}


unsigned int       zMapGFFFeatureDataGetNat(const ZMapGFFFeatureData const pFeatureData )
{
  if (!pFeatureData)
    return 0 ;
  return pFeatureData->nAttributes ;
}


ZMapGFFAttribute * zMapGFFFeatureDataGetAts(const ZMapGFFFeatureData const pFeatureData )
{
  if (!pFeatureData)
    return NULL ;
  return pFeatureData->pAttributes ;
}

const ZMapSOIDData zMapGFFFeatureDataGetSod(const ZMapGFFFeatureData const pFeatureData)
{
  if (!pFeatureData)
    return NULL ;
  return pFeatureData->pSOIDData ;
}

