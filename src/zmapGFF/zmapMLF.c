#include "zmapMLF_P.h"



/*
 * Create a MLF structure.
 */
ZMapMLF zMapMLFCreate()
{
  ZMapMLF pMLF = NULL ;

  pMLF = g_malloc(sizeof(ZMapMLFStruct)) ;
  if (pMLF)
  {
    pMLF->pIDTable = g_hash_table_new(NULL, NULL) ;
  }

  return pMLF ;
}

/*
 * This has to conform to the GFunc interface; not publically available.
 */
static void zMapMLFDestroyElement(gpointer pgqValue, gpointer pHashElement, gpointer pUserData)
{
  if (!pgqValue || !pHashElement || pUserData )
    return ;
  g_hash_table_destroy(pHashElement) ;
}


/*
 * Destroy a MLF structure.
 */
gboolean zMapMLFDestroy(ZMapMLF pMLF)
{
  gboolean bResult = FALSE ;
  if (!pMLF || !pMLF->pIDTable)
    return bResult ;

  g_hash_table_foreach(pMLF->pIDTable, zMapMLFDestroyElement, NULL );
  g_hash_table_destroy(pMLF->pIDTable) ;

  g_free(pMLF) ;
  bResult = TRUE ;

  return bResult ;
}



/*
 * Empty the container. In fact, destroy the old ID Table and
 * create a new one.
 */
gboolean zMapMLFEmpty(ZMapMLF const pMLF )
{
  gboolean bResult = FALSE ;
  if (!pMLF || !pMLF->pIDTable)
    return bResult ;

  g_hash_table_foreach(pMLF->pIDTable, zMapMLFDestroyElement, NULL );
  g_hash_table_destroy(pMLF->pIDTable) ;
  pMLF->pIDTable = g_hash_table_new(NULL, NULL) ;

  if (pMLF->pIDTable)
    bResult = TRUE ;

  return bResult ;
}


/*
 * Check whether or not a given ID (translated to GQuark) is present in
 * the top level structure. If the ID is present, it returns a pointer to
 * the associated value hash.
 */
gpointer zMapMLFIsIDPresent(const ZMapMLF const pMLF, GQuark gqValue)
{
  if (!pMLF || !gqValue)
    return FALSE ;
  return g_hash_table_lookup(pMLF->pIDTable, GINT_TO_POINTER(gqValue)) ;
}


/*
 * Add a new ID to the data structure. This means add a key-value pair that
 * consists of gqValue and a new (empty) hash table associated with it.
 */
gboolean zMapMLFAddID(const ZMapMLF const pMLF, GQuark gqValue)
{
  gboolean bResult = FALSE ;
  GHashTable *pTable = NULL ;
  if (!pMLF || !gqValue )
    return bResult ;

  /*
   * test whether or not element is already present
   */
  if (zMapMLFIsIDPresent(pMLF, gqValue) != NULL )
    return FALSE ;

  /*
   * add new ID to top level hash
   */
  pTable = g_hash_table_new(NULL, NULL) ;
  if (!pTable)
    return bResult ;
  g_hash_table_insert(pMLF->pIDTable, GINT_TO_POINTER(gqValue), pTable ) ;
  bResult = TRUE ;


  return bResult ;
}

/*
 * Remove an ID from the data structure.
 */
gboolean zMapMLFRemoveID(ZMapMLF const pMLF, GQuark gqValue)
{
  gboolean bResult = FALSE ;
  gpointer pValueTable = NULL ;
  if (!pMLF || !gqValue)
    return bResult ;
  if (zMapMLFIsIDPresent(pMLF, gqValue) == NULL )
    return bResult ;

  /*
   * Find pointer to value in table.
   */
  pValueTable = g_hash_table_lookup(pMLF->pIDTable, GINT_TO_POINTER(gqValue)) ;
  if (pValueTable)
    g_hash_table_destroy(pValueTable) ;

  /*
   * And remove that key from the table.
   */
  if (g_hash_table_remove(pMLF->pIDTable, GINT_TO_POINTER(gqValue)))
  {
    bResult = TRUE ;
  }

  return bResult ;
}



/*
 * Check whether or not a feature is present associated with a given ID in the
 * given MLF structure. Returns false if the ID is not present to start with.
 * Feature is identified through the pointer value it is accessed through.
 */
gboolean zMapMLFIsFeaturePresent(const ZMapMLF const pMLF, GQuark gqID, const ZMapFeature const pFeature)
{
  gboolean bResult = FALSE ;
  GHashTable *pValueTable = NULL ;
  if (!pMLF || !gqID || !pFeature || !pFeature->unique_id)
    return bResult ;

  /*
   * First check whether or not the ID is present to start with.
   */
  if ((pValueTable = zMapMLFIsIDPresent(pMLF, gqID)) == NULL )
    return bResult ;

  /*
   * Now check whether or not the feature is present.
   */
  if (g_hash_table_lookup(pValueTable, GINT_TO_POINTER(pFeature->unique_id)) != NULL )
  {
    bResult = TRUE ;
  }

  return bResult ;
}


/*
 * Add a feature associated with a given ID. If the given ID is not present, then return false.
 * If the feature ID is already present in the ID value hash, then return false, otherwise
 * add it.
 */
gboolean zMapMLFAddFeatureToID(const ZMapMLF const pMLF, GQuark gqID, const ZMapFeature const pFeature )
{
  gboolean bResult = FALSE ;
  GHashTable *pValueTable = NULL ;
  if (!pMLF || !gqID || !pFeature || !pFeature->unique_id)
    return bResult ;

  /*
   * First check whether or not the ID is present to start with.
   */
  if ((pValueTable = zMapMLFIsIDPresent(pMLF, gqID)) == NULL )
    return bResult ;

  /*
   * If pFeature is already present, then return FALSE.
   * Otherwise add the feature to the given pValueTable.
   */
  if (g_hash_table_lookup(pValueTable, GINT_TO_POINTER(pFeature->unique_id) ) != NULL )
  {
    return bResult ;
  }
  else
  {
    g_hash_table_insert(pValueTable, GINT_TO_POINTER(pFeature->unique_id), GINT_TO_POINTER(pFeature->unique_id) ) ;
    bResult = TRUE ;
  }

  return bResult ;
}



/*
 * Remove a given feature from the specified ID. If the feature was not
 * present in the ID value hash, then return false.
 */
gboolean zMapMLFRemoveFeatureFromID(const ZMapMLF const pMLF, GQuark gqID, const ZMapFeature const pFeature )
{
  gboolean bResult = FALSE ;
  GHashTable * pValueTable = NULL ;
  if (!pMLF || !pFeature || !pFeature->unique_id )
    return bResult ;

  /*
   * First check whether or not the ID is present to start with.
   */
  if ((pValueTable = zMapMLFIsIDPresent(pMLF, gqID)) == NULL )
    return bResult ;

  /*
   * Remove the feature if it is present.
   */
  if (g_hash_table_lookup(pValueTable, GINT_TO_POINTER(pFeature->unique_id) ) != NULL )
  {
    g_hash_table_remove(pValueTable, GINT_TO_POINTER(pFeature->unique_id) ) ;
    bResult = TRUE ;
  }

  return bResult ;
}




/*
 * Return the number of IDs in the top level hash.
 */
unsigned int zMapMLFNumID(const ZMapMLF const pMLF )
{
  unsigned int nResult = 0 ;
  if (!pMLF || !pMLF->pIDTable)
    return nResult ;

  nResult = g_hash_table_size(pMLF->pIDTable) ;

  return nResult ;
}



/*
 * Return the number of features associated with a given ID. If the ID is
 * not present then return 0.
 */
unsigned int zMapMLFNumFeatures(const ZMapMLF const pMLF , GQuark gqID)
{
  unsigned int nResult = 0 ;
  gpointer pValueTable = NULL ;
  if (!pMLF || !pMLF->pIDTable || !gqID)
    return nResult ;

  if (!(pValueTable = zMapMLFIsIDPresent(pMLF, gqID)))
    return nResult ;

  nResult = g_hash_table_size(pValueTable) ;

  return nResult ;
}





/*
 * Iteration over IDs in the top level table.
 */
gboolean zMapMLFIDIteration(const ZMapMLF const pMLF,  gboolean (*iterationFunction)(GQuark, GHashTable *) )
{
  gboolean bResult = FALSE ;
  GHashTableIter ghIterator ;
  gpointer pKey = NULL,
    pValue = NULL ;
  GQuark gqID = 0 ;
  GHashTable *pValueTable = NULL ;

  if (!pMLF || !pMLF->pIDTable || !iterationFunction)
    return bResult ;

  g_hash_table_iter_init(&ghIterator, pMLF->pIDTable) ;
  while (g_hash_table_iter_next(&ghIterator, &pKey, &pValue))
  {
    /*
     * gqID is the current ID
     * pValueTable is the value table associated with this ID
     */
    gqID = GPOINTER_TO_INT(pKey) ;
    pValueTable = (GHashTable*) pValue ;
    if (!(bResult = iterationFunction(gqID, pValueTable) ))
      break ;
  }

  return bResult ;
}






