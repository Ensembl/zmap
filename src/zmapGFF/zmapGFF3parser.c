#include <ZMap/zmapGFF.h>


ZMapGFFParser zMapGFFCreateParser3(int iGFFVersion, char *sequence, int features_start, int features_end)
{
  ZMapGFFParser parser = NULL ;

  if (iGFFVersion != ZMAPGFF_VERSION_3 )
    return parser;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if ((sequence && *sequence)
      && ((features_start == 1 && features_end == 0) || (features_start > 0 && features_end >= features_start)))
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    /* I WANT TO REMOVE THE 1,0 STUFF..... */
  if ((sequence && *sequence) && (features_start > 0 && features_end >= features_start))
    {
      parser = g_new0(ZMapGFFParserStruct, 1) ;

      parser->gff_version = iGFFVersion ;

      parser->state = ZMAPGFF_PARSE_HEADER ;
      parser->error = NULL ;
      parser->error_domain = g_quark_from_string(ZMAP_GFF_ERROR) ;
      parser->stop_on_error = FALSE ;

      parser->line_count = 0 ;
      parser->SO_compliant = FALSE ;
      parser->default_to_basic = FALSE ;

      parser->clip_mode = GFF_CLIP_NONE ;
      parser->clip_start = parser->clip_end = 0 ;

      parser->excluded_features = g_hash_table_new(NULL, NULL) ;

      /* Some of these may also be derived from the file meta-data. */
      parser->header_flags.done_header = FALSE ;
      parser->header_flags.got_gff_version = FALSE ;
      parser->header_flags.got_sequence_region = FALSE ;
      parser->header_state = GFF_HEADER_NONE ;


      parser->sequence_name = g_strdup(sequence) ;
      parser->features_start = features_start ;
      parser->features_end = features_end ;

      parser->raw_line_data = g_string_sized_new(2000) ;

      parser->sequence_flags.done_finished = TRUE ;	    /* default we don't parse the dna/protein */

      /* Set initial buffer & format string size to something that will probably be big enough. */
      resizeBuffers(parser, BUF_INIT_SIZE) ;

      resizeFormatStrs(parser) ;
    }

  return parser ;
}
