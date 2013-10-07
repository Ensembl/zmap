#include <ZMap/zmapGFF.h>
#include "zmapGFF_P.h"

static gboolean resizeBuffers3(ZMapGFFParser parser, gsize line_length) ;
static gboolean resizeFormatStrs3(ZMapGFFParser parser) ;


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


/* Given a line length, will allocate buffers so they cannot overflow when parsing a line of this
 * length. The rationale here is that we might get handed a line that had just one field in it
 * that was the length of the line. By allocating buffers that are the line length we cannot
 * overrun our buffers even in this bizarre case !!
 *
 * Note that we attempt to avoid frequent reallocation by making buffer twice as large as required
 * (not including the final null char....).
 */
static gboolean resizeBuffers3(ZMapGFFParser parser, gsize line_length)
{
  gboolean resized = FALSE ;

  if (line_length > parser->buffer_length)
    {
      int i, new_line_length ;

      new_line_length = line_length * BUF_MULT ;

      for (i = 0 ; i < GFF_BUF_NUM ; i++)
	{
	  char **buf_ptr = parser->buffers[i] ;

	  g_free(buf_ptr) ;				    /* g_free() handles NULL pointers. */

	  buf_ptr = g_malloc0(new_line_length) ;

	  parser->buffers[i] = buf_ptr ;
	}

      parser->buffer_length = new_line_length ;
      resized = TRUE ;
    }


  return resized ;
}



/* Construct format strings to parse the main GFF fields and also sub-parts of a GFF line.
 * This needs to be done dynamically because we may need to change buffer size and hence
 * string format max length.
 *
 *
 * Notes on the format string for the main GFF fields:
 *
 * GFF version 2 format for a line is:
 *
 * <sequence> <source> <feature> <start> <end> <score> <strand> <phase> [attributes] [#comments]
 *
 * The format_str matches the above, splitting everything into its own strings, note that
 * some fields, although numerical, default to the char "." so cannot be simply scanned into
 * a number. The only tricky bit is to get at the attributes and comments which have
 * white space in them, this scanf format string seems to do it:
 *
 *  format_str = "%<length>s%<length>s%<length>s%d%d%<length>s%<length>s%<length>s %<length>[^#] %<length>c"
 *
 *  " %<length>[^#]" Jumps white space after the last mandatory field and then gets everything up to
 *              the next "#", so this will fail if people put a "#" in their attributes !
 *
 *  " %<length>c"    Reads everything from (and including) the "#" found by the previous pattern.
 *
 * If it turns out that people do have "#" chars in their attributes we will have do our own
 * parsing of this section of the line.
 *
 * mh17 NOTE BAM data has # in its attributes
 */
static gboolean resizeFormatStrs3(ZMapGFFParser parser)
{
  gboolean resized = TRUE ;				    /* Everything will work or abort(). */
  GString *format_str ;
  char *align_format_str ;
  gsize length ;


  /* Max length of string we can parse using scanf(), the "- 1" allows for the terminating null.  */
  length = parser->buffer_length - 1 ;


  /*
   * Redo main gff field parsing format string.
   */
  g_free(parser->format_str) ;

  format_str = g_string_sized_new(BUF_FORMAT_SIZE) ;

  /* Lot's of "%"s here because "%%" is the way to escape a "%" !! The G_GSSIZE_FORMAT is a
   * portable way to printf a gsize. */
#if QUOTED_HASH_KILLS_ATTRIBUTES
  g_string_append_printf(format_str,
                         "%%%" G_GSSIZE_FORMAT "s%%%" G_GSSIZE_FORMAT "s%%%" G_GSSIZE_FORMAT "s%%d%%d%%%"
			 G_GSSIZE_FORMAT "s%%%" G_GSSIZE_FORMAT "s%%%" G_GSSIZE_FORMAT "s %%%"
			 G_GSSIZE_FORMAT "[^#] %%%" G_GSSIZE_FORMAT "c",
			 length, length, length, length, length, length, length, length) ;
#else
	/* get attributes + comments together and split later */
  g_string_append_printf(format_str,
                         "%%%" G_GSSIZE_FORMAT "s%%%" G_GSSIZE_FORMAT "s%%%" G_GSSIZE_FORMAT "s%%d%%d%%%"
			 G_GSSIZE_FORMAT "s%%%" G_GSSIZE_FORMAT "s%%%" G_GSSIZE_FORMAT "s %%%"
			 G_GSSIZE_FORMAT "[^\n]s",
			 length, length, length, length, length, length, length) ;
#endif
  parser->format_str = g_string_free(format_str, FALSE) ;


  /*
   * Redo main gff field parsing format string.
   */
  g_free(parser->cigar_string_format_str) ;

  format_str = g_string_sized_new(BUF_FORMAT_SIZE) ;

  /* this is what I'm trying to get:  "XXXXX %*[\"]%50[^\"]%*[\"]%*s" which parses a string
   * like this:
   *
   *       XXXXXXX "M335ID55M"  where XXXXX will be one of the supported alignment formats.
   *  */
  align_format_str = "%%*[\"]" "%%%d" "[^\"]%%*[\"]%%*s" ;
  g_string_append_printf(format_str,
			 align_format_str,
			 length) ;

  parser->cigar_string_format_str = g_string_free(format_str, FALSE) ;

  return resized ;
}
