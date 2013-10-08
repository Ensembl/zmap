#include <ZMap/zmapGFF.h>


/* Types of name find operation, depend on type of feature. */
typedef enum {NAME_FIND, NAME_USE_SOURCE, NAME_USE_SEQUENCE, NAME_USE_GIVEN, NAME_USE_GIVEN_OR_NAME } NameFindType ;



/* The main parser struct, this represents an instance of a parser. */
typedef struct ZMapGFF2ParserStruct_
{
  /*
   * Data common to both versions.
   */
  ZMAPGFF_PARSER_STRUCT_COMMON_DATA

  /*
   * Data for this verison only.
   */

  /* File data: some derived from the file directly. */
  struct
  {
    unsigned int done_header : 1 ;			            /* Is the header processed ? */
    unsigned int got_gff_version : 1 ;
    unsigned int got_sequence_region : 1 ;
  } header_flags ;

  ZMapGFFHeaderState header_state ;

  gboolean SO_compliant ;				    /* TRUE => use only SO terms for
							       feature types. */
  ZMapGFFClipMode clip_mode ;				    /* Decides how features that overlap
							       or are outside the start/end are
							       handled. */

  /* Parsing buffers, since lines can be long we allocate these dynamically from the
   * known line length and construct a format string for the scanf using this length. */
  gsize buffer_length ;
  char **buffers[GFF_BUF_NUM] ;
  char *format_str ;
  char *cigar_string_format_str ;

  /* Parsing DNA sequence data, used when DNA sequence is embedded in the file. */
  struct
  {
    unsigned int done_start : 1 ;
    unsigned int in_sequence_block : 1 ;
    unsigned int done_finished :1 ;
  } sequence_flags ;

  ZMapSequenceStruct seq_data ;

} ZMapGFF2ParserStruct, *ZMapGFF2Parser ;



/*
 * Code specific to this version of the parser only.
 */
ZMapGFFParser zMapGFFCreateParser2(char *sequence, int features_start, int features_end) ;
gboolean zMapGFFParseHeader_V2(ZMapGFFParser parser_base, char *line, gboolean *header_finished, ZMapGFFHeaderState *header_state) ;
gboolean zMapGFFParseSequence_V2(ZMapGFFParser parser_base, char *line, gboolean *sequence_finished) ;
gboolean zMapGFFParseLineLength_V2(ZMapGFFParser parser_base, char *line, gsize line_length) ;
