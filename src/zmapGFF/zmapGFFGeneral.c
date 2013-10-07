#include <ZMap/zmapGFF.h>

#include "zmapGFF_P.h"
#include "zmapGFF2_P.h"
#include "zmapGFF3_P.h"


/*
 * Public interface parser creation function.
 */
ZMapGFFParser zMapGFFCreateParser(int iGFFVersion, char *sequence, int features_start, int features_end)
{
  ZMapGFFParser parser = NULL ;

  if (iGFFVersion == ZMAPGFF_VERSION_2 )
  {
    parser = zMapGFFCreateParser2(iGFFVersion, sequence, features_start, features_end) ;
  }
  else if (iGFFVersion == ZMAPGFF_VERSION_3 )
  {
    /* create v3 parser */
  }
  else
  {
    /* not a valid version */
  }

  return parser ;
}
