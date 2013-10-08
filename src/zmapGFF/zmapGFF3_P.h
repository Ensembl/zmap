

#include <ZMap/zmapGFF.h>

/*
 * This file contains code specific to this version of the parser only.
 */

typedef struct ZMapGFF3ParserStruct_
{
  /*
   * Data common to both versions.
   */
  ZMAPGFF_PARSER_STRUCT_COMMON_DATA


  /*
   * Data for this version only.
   */

} ZMapGFF3ParserStruct, *ZMapGFF3Parser ;



ZMapGFFParser zMapGFFCreateParser3(char *sequence, int features_start, int features_end) ;
