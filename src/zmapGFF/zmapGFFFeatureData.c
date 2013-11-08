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
                               unsigned int iStart,
                               unsigned int iEnd,
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

unsigned int  zMapGFFFeatureDataGetSta(const ZMapGFFFeatureData const pFeatureData )
{
  if (!pFeatureData)
    return 0 ;
  return pFeatureData->iStart ;
}

unsigned int  zMapGFFFeatureDataGetEnd(const ZMapGFFFeatureData const pFeatureData )
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


const ZMapGFFAttribute * zMapGFFFeatureDataGetAts(const ZMapGFFFeatureData const pFeatureData )
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

