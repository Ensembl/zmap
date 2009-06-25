/*  File: zmapGFF2parser.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: parser for GFF version 2 format.
 *              
 * Exported functions: See ZMap/zmapGFF.h
 * HISTORY:
 * Last edited: Jun 25 15:51 2009 (edgrif)
 * Created: Fri May 28 14:25:12 2004 (edgrif)
 * CVS info:   $Id: zmapGFF2parser.c,v 1.92 2009-06-25 14:56:28 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFeature.h>
#include <zmapGFF_P.h>


/* THIS FILE NEEDS WORK TO COPE WITH ALIGN/BLOCK/COORD INFO..... */

#ifdef RDS_NO_TO_HACKED_CODE
/* HACKED CODE TO FIX UP LINKS IN BLOCK->SET->FEATURE */
static void setBlock(GQuark key_id, gpointer data, gpointer user_data) ;
static void setSet(GQuark key_id, gpointer data, gpointer user_data) ;
/* END OF HACKED CODE TO FIX UP LINKS IN BLOCK->SET->FEATURE */
#endif

typedef enum {NAME_FIND, NAME_USE_SOURCE, NAME_USE_SEQUENCE} NameFindType ;


static gboolean parseHeaderLine(ZMapGFFParser parser, char *line) ;
static gboolean parseBodyLine(ZMapGFFParser parser, char *line) ;
static gboolean makeNewFeature(ZMapGFFParser parser, NameFindType name_find,
			       char *sequence, char *source, char *ontology,
			       ZMapStyleMode feature_type,
			       int start, int end,
			       gboolean has_score, double score,
			       ZMapStrand strand, ZMapPhase phase, char *attributes,
			       char **err_text) ;
static gboolean getFeatureName(NameFindType name_find, char *sequence, char *attributes,
			       char *source,
			       ZMapStyleMode feature_type,
			       ZMapStrand strand, int start, int end, int query_start, int query_end,
			       char **feature_name, char **feature_name_id) ;
static char *getURL(char *attributes) ;
static GQuark getClone(char *attributes) ;
static GQuark getLocus(char *attributes) ;
static gboolean getKnownName(char *attributes, char **known_name_out) ;
static gboolean getHomolLength(char *attributes, int *length_out) ;
static gboolean getHomolAttrs(char *attributes, ZMapHomolType *homol_type_out,
			      int *start_out, int *end_out, ZMapStrand *strand_out) ;
static gboolean getAssemblyPathAttrs(char *attributes, char **assembly_name_unused,
				     ZMapStrand *strand_out, int *length_out, GArray **path_out) ;
static gboolean getCDSAttrs(char *attributes,
			    gboolean *start_not_found_out, int *start_phase_out,
			    gboolean *end_not_found_out) ;
static void getFeatureArray(GQuark key_id, gpointer data, gpointer user_data) ;
static void destroyFeatureArray(gpointer data) ;
static gboolean loadGaps(char *currentPos, GArray *gaps) ;
static void mungeFeatureType(char *source, ZMapStyleMode *type_inout);
static gboolean getNameFromNote(char *attributes, char **name) ;
static char *getNoteText(char *attributes) ;






/* types is the list of methods/types, call it what you will that we want to see
 * in the output, we may need to filter the incoming data stream to get this.
 * 
 * If parse_only is TRUE the parser will parse the GFF and report errors but will
 * _not_ create any features. This means the parser can be tested/used on huge datasets
 * without having to have huge amounts of memory to hold the feature structs.
 * You can only set parse_only when you create the parser, it cannot be set later. */
ZMapGFFParser zMapGFFCreateParser(GData *sources, gboolean parse_only)
{
  ZMapGFFParser parser ;
  GQuark locus_id ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  g_list_foreach(sources, stylePrintCB, NULL) ; /* debug */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  parser = g_new0(ZMapGFFParserStruct, 1) ;

  parser->state = ZMAPGFF_PARSE_HEADER ;
  parser->error = NULL ;
  parser->error_domain = g_quark_from_string(ZMAP_GFF_ERROR) ;
  parser->stop_on_error = FALSE ;
  parser->parse_only = parse_only ;

  parser->line_count = 0 ;
  parser->SO_compliant = FALSE ;
  parser->default_to_basic = FALSE ;

  parser->clip_mode = GFF_CLIP_NONE ;
  parser->clip_start = parser->clip_end = 0 ;

  parser->done_header = FALSE ;
  parser->done_version = FALSE ;
  parser->gff_version = -1 ;
  parser->done_source = FALSE ;
  parser->source_name = parser->source_version = NULL ;
  parser->done_sequence_region = FALSE ;
  parser->sequence_name = NULL ;
  parser->features_start = parser->features_end = 0 ;
  parser->sources = sources ;

  /* Check for Locus as one of the sources as this needs to be constructed as we go along. */
  locus_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_LOCUS_NAME) ;
  if (!(parser->locus_set_style = zMapFindStyle(parser->sources, locus_id)))
    {
      zMapLogWarning("Locus set will not be created, "
		     "could not find style \"%s\" for feature set \"%s\".",
		     ZMAP_FIXED_STYLE_LOCUS_NAME, ZMAP_FIXED_STYLE_LOCUS_NAME) ;
    }
  else
    {
      parser->locus_set_id = locus_id ;
    }



  if (!parser->parse_only)
    {
      g_datalist_init(&(parser->feature_sets)) ;
      parser->free_on_destroy  = TRUE ;
    }
  else
    {
      parser->feature_sets =  NULL ;
      parser->free_on_destroy  = FALSE ;
    }


  /* Allocated dynamically as these fields in GFF can be big. */
  parser->attributes_str = g_string_sized_new(GFF_MAX_FREETEXT_CHARS) ;
  parser->comments_str = g_string_sized_new(GFF_MAX_FREETEXT_CHARS) ;


  return parser ;
}



/* Parses a single line of GFF data, should be called repeatedly with successive lines
 * GFF data from a GFF source. This function expects to find first the GFF header and
 * then the GFF data. (See zMapGFFParseHeader() if you want to parse out the header
 * first.
 * 
 * This function expects a null-terminated C string that contains a complete GFF line
 * (comment or non-comment line), the function expects the caller to already have removed the
 * newline char from the end of the GFF line.
 * 
 * Returns FALSE if there is any error in the GFF header.
 * Returns FALSE if there is an error in the GFF body and stop_on_error == TRUE.
 *
 * Once an error has been returned the parser object cannot be used anymore and
 * zMapGFFDestroyParser() should be called to free it.
 *
 */

/* ISSUE: need to decide on rules for comments, can they be embedded within other gff lines, are
 * the header comments compulsory ? etc. etc. 
 * 
 * Current code assumes that the header block will be a contiguous set of header lines
 * at the top of the file and that the first non-header line marks the beginning
 * of the GFF data. If this is not true then its an error.
 */ 
gboolean zMapGFFParseLine(ZMapGFFParser parser, char *line)
{
  gboolean result = FALSE ;


  parser->line_count++ ;

  /* Look for the header information. */
  if (parser->state == ZMAPGFF_PARSE_HEADER)
    {
      if (!(result = parseHeaderLine(parser, line)))
	{
	  /* returns FALSE for two reasons: there was a parse error (note that we ignore
	   * stop_on_error, the header _must_ be correct), or the header section has
	   * finished - in this case we need to cancel the error and reparse the line. */
	  if (parser->error)
	    {
	      result = FALSE ;
	      parser->state = ZMAPGFF_PARSE_ERROR ;
	    }
	  else
	    {
	      result = TRUE ;

	      /* If we found all the header parts move on to the body. */
	      if (parser->done_header)
		parser->state = ZMAPGFF_PARSE_BODY ;
	    }
	}
    }

  /* Note can only be in parse body state if header correctly parsed. */
  if (parser->state == ZMAPGFF_PARSE_BODY)
    {
      /* Skip over comment lines, this is a CRUDE test, probably need something more subtle. */
      if (*(line) == '#')
	result = TRUE ;
      else
	{
	  /* THIS NEEDS WORK, ONCE I'VE SORTED OUT ALL THE PARSING STUFF...... */
	  if (!(result = parseBodyLine(parser, line)))
	    {
	      if (parser->error && parser->stop_on_error)
		{
		  result = FALSE ;
		  parser->state = ZMAPGFF_PARSE_ERROR ;
		}
	    }
	}
    }


  return result ;
}



/* Parses a single line of GFF data, should be called repeatedly with successive lines
 * GFF data from a GFF source. This function expects to find the GFF header, once all
 * the required header lines have been found or a non-comment line is found it will stop.
 * The zMapGFFParseLine() function can then be used to parse the rest of the file.
 * 
 * This function expects a null-terminated C string that contains a complete GFF line
 * (comment or non-comment line), the function expects the caller to already have removed the
 * newline char from the end of the GFF line.
 * 
 * Returns FALSE if there is any error in the GFF header.
 * Returns FALSE if there is an error in the GFF body and stop_on_error == TRUE.
 *
 * Once an error has been returned the parser object cannot be used anymore and
 * zMapGFFDestroyParser() should be called to free it.
 *
 * Current code assumes that the header block will be a contiguous set of header lines
 * at the top of the file and that the first non-header line marks the beginning
 * of the GFF data. If this is not true then its an error.
 */ 
gboolean zMapGFFParseHeader(ZMapGFFParser parser, char *line, gboolean *header_finished)
{
  gboolean result = FALSE ;

  zMapAssert(parser && line && header_finished) ;

  parser->line_count++ ;

  /* Look for the header information. */
  if (parser->state == ZMAPGFF_PARSE_HEADER)
    {
      if (!(result = parseHeaderLine(parser, line)))
	{
	  /* returns FALSE for two reasons: there was a parse error (note that we ignore
	   * stop_on_error, the header _must_ be correct), or the header section has
	   * finished - in this case we need to cancel the error and reparse the line. */
	  if (parser->error)
	    {
	      result = FALSE ;
	      parser->state = ZMAPGFF_PARSE_ERROR ;
	    }
	  else
	    {
	      result = TRUE ;

	      /* If we found all the header parts move on to the body. */
	      if (parser->done_header)
		{
		  parser->state = ZMAPGFF_PARSE_BODY ;
		  *header_finished = TRUE ;
		}
	    }
	}
      else
	{
	  *header_finished = FALSE ;
	}
    }

  return result ;
}




/* Returns as much information as possible from the header comments of the gff file.
 * Note that our current parsing code makes this an all or nothing piece of code:
 * either the whole header is there or nothing is.... */
ZMapGFFHeader zMapGFFGetHeader(ZMapGFFParser parser)
{
  ZMapGFFHeader header = NULL ;

  if (parser->done_header)
    {
      header = g_new0(ZMapGFFHeaderStruct, 1) ;

      header->gff_version = parser->gff_version ;

      header->source_name  = g_strdup(parser->source_name) ;
      header->source_version = g_strdup(parser->source_version) ;

      header->sequence_name = g_strdup(parser->sequence_name) ;
      header->features_start =  parser->features_start ;
      header->features_end = parser->features_end ;
    }      

  return header ;
}


void zMapGFFFreeHeader(ZMapGFFHeader header)
{
  zMapAssert(header) ;

  g_free(header->source_name) ;
  g_free(header->source_version) ;
  g_free(header->sequence_name) ;

  g_free(header) ;

  return ;
}


/* Return the set of features read from a file for a block. */
gboolean zMapGFFGetFeatures(ZMapGFFParser parser, ZMapFeatureBlock feature_block)
{
  gboolean result = FALSE ;

  if (parser->state != ZMAPGFF_PARSE_ERROR)
    {
      /* Actually we should only need to test feature_sets here really as there shouldn't be any
       * for parse_only.... */
      if (!parser->parse_only && parser->feature_sets)
	{
          g_datalist_foreach(&(parser->feature_sets), getFeatureArray,
			     feature_block) ;
	}

      result = TRUE ;
    }

  return result ;
}


#ifdef RDS_NO_TO_HACKED_CODE
/* HACKED CODE TO FIX UP LINKS IN BLOCK->SET->FEATURE */
static void setBlock(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
#ifdef RDS_DONT_INCLUDE
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)user_data ;

  feature_set->parent = (ZMapFeatureAny)feature_block ;
  zMapFeatureBlockAddFeatureSet(feature_block, feature_set);
#endif

  g_datalist_foreach(&(feature_set->features), setSet, feature_set) ;

  return ;
}

static void setSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  ZMapFeatureSet feature_set = (ZMapFeatureSet)user_data ;

#ifdef RDS_DONT_INCLUDE
  feature->parent = (ZMapFeatureAny)feature_set ;
#endif
  zMapFeatureSetAddFeature(feature_set, feature);

  return ;
}
/* END OF HACKED CODE TO FIX UP LINKS IN BLOCK->SET->FEATURE */
#endif


/* Optionally set mappings that are keys from the GFF source to feature set and style names. */
void zMapGFFParseSetSourceHash(ZMapGFFParser parser,
			       GHashTable *source_2_feature_set, GHashTable *source_2_sourcedata)
{
  parser->source_2_feature_set = source_2_feature_set ;

  parser->source_2_sourcedata = source_2_sourcedata ;


  /* Locus is an odd one out just now, we need to handle this differently..... */
  if (parser->locus_set_id)
    {
      ZMapGFFSet set_data ;
      ZMapGFFSource source_data ;


      set_data = g_new0(ZMapGFFSetStruct, 1) ;
      set_data->feature_set_id = parser->locus_set_id ;
      set_data->description = g_strdup_printf("Locus IDs") ;

      g_hash_table_insert(parser->source_2_feature_set,
			  GINT_TO_POINTER(parser->locus_set_id),
			  set_data) ;


      source_data = g_new0(ZMapGFFSourceStruct, 1) ;
      source_data->source_id = parser->locus_set_id ;
      source_data->style_id = parser->locus_set_id ;
      source_data->source_text = g_quark_from_string("Locus IDs") ;

      
      g_hash_table_insert(parser->source_2_sourcedata,
			  GINT_TO_POINTER(parser->locus_set_id),
			  source_data) ;

    }


  return ;
}





/* If stop_on_error is TRUE the parser will not parse any further lines after it encounters
 * the first error in the GFF file. */
void zMapGFFSetStopOnError(ZMapGFFParser parser, gboolean stop_on_error)
{
  if (parser->state != ZMAPGFF_PARSE_ERROR)
    parser->stop_on_error = stop_on_error ;

  return ;
}


/* If SO_compliant is TRUE the parser will only accept SO terms for feature types. */
void zMapGFFSetSOCompliance(ZMapGFFParser parser, gboolean SO_compliant)
{
  if (parser->state != ZMAPGFF_PARSE_ERROR)
    parser->SO_compliant = SO_compliant ;

  return ;
}


/* Sets the clipping mode for handling features that are either partly or wholly outside
 * the requested start/end for the target sequence. */
void zMapGFFSetFeatureClip(ZMapGFFParser parser, ZMapGFFClipMode clip_mode)
{
  if (parser->state != ZMAPGFF_PARSE_ERROR)
    parser->clip_mode = clip_mode ;

  return ;
}


/* Sets the start/end coords for clipping features. */
void zMapGFFSetFeatureClipCoords(ZMapGFFParser parser, int start, int end)
{
  zMapAssert(start > 0 && end > 0 && start <= end) ;

  if (parser->state != ZMAPGFF_PARSE_ERROR)
    {
      parser->clip_start = start ;
      parser->clip_end = end ;
    }

  return ;
}




/* If default_to_basic is TRUE the parser will create basic features for any unrecognised
 * feature type. */
void zMapGFFSetDefaultToBasic(ZMapGFFParser parser, gboolean default_to_basic)
{
  if (parser->state != ZMAPGFF_PARSE_ERROR)
    parser->default_to_basic = default_to_basic ;

  return ;
}


/* Return the GFF version which the parser is using. This is determined from the GFF
 * input stream from the header comments. */
int zMapGFFGetVersion(ZMapGFFParser parser)
{
  int version = -1 ;

  if (parser->state != ZMAPGFF_PARSE_ERROR)
    version = parser->gff_version ;

  return version ;
}


/* Return line number of last line processed (this is the same as the number of lines processed.
 * N.B. we always return this as it may be required for error diagnostics. */
int zMapGFFGetLineNumber(ZMapGFFParser parser)
{
  return parser->line_count ;
}


/* If a zMapGFFNNN function has failed then this function returns a description of the error
 * in the glib GError format. If there has been no error then NULL is returned. */
GError *zMapGFFGetError(ZMapGFFParser parser)
{
  return parser->error ;
}

/* Returns TRUE if the parser has encountered an error from which it cannot recover and hence will
 * not process any more lines, FALSE otherwise. */
gboolean zMapGFFTerminated(ZMapGFFParser parser)
{
  gboolean result = TRUE ;

  if (parser->state != ZMAPGFF_PARSE_ERROR)
    result = FALSE ;

  return result ;
}

/* If free_on_destroy == TRUE then all the feature arrays will be freed when
 * the GFF parser is destroyed, otherwise they are left intact. Important
 * because caller may still want access to them after the destroy. */
void zMapGFFSetFreeOnDestroy(ZMapGFFParser parser, gboolean free_on_destroy)
{
  if (parser->state != ZMAPGFF_PARSE_ERROR)
    parser->free_on_destroy = free_on_destroy ;

  return ;
}


void zMapGFFDestroyParser(ZMapGFFParser parser)
{
  if (parser->error)
    g_error_free(parser->error) ;

  if (parser->source_name)
    g_free(parser->source_name) ;
  if (parser->source_version)
    g_free(parser->source_version) ;

  if (parser->sequence_name)
    g_free(parser->sequence_name) ;

  /* The datalist uses a destroy routine that only destroys the feature arrays if the caller wants
   * us to, see destroyFeatureArray() */
  if (!parser->parse_only && parser->feature_sets)
    g_datalist_clear(&(parser->feature_sets)) ;

  g_string_free(parser->attributes_str, TRUE) ;
  g_string_free(parser->comments_str, TRUE) ;

  g_free(parser) ;

  return ;
}






/* 
 * --------------------- Internal functions ---------------------------
 */



/* This function expects a null-terminated C string that contains a GFF header line
 * which is a special form of comment line starting with a "##".
 *
 * GFF version 2 format for header lines is:
 *
 * ##gff-version <version_number>
 * ##source-version <program_identifier> <program_version>
 * ##date YYYY-MM-DD
 * ##sequence-region <sequence_name> <sequence_start> <sequence_end>
 *
 * Currently we require the gff-version, source-version and sequence-region
 * to have been set and ignore any other header comments.
 * 
 * Returns FALSE if passed a line that is not a header comment OR if there
 * was a parse error. In the latter case parser->error will have been set.
 */
static gboolean parseHeaderLine(ZMapGFFParser parser, char *line)
{
  gboolean result = FALSE ;
  enum {FIELD_BUFFER_LEN = 1001} ;			    /* If you change this, change the
							       scanf's below... */


  if (!g_str_has_prefix(line, "##"))
    {
      /* If we encounter a non-header comment line and we haven't yet finished the header
       * then its an error, otherwise we just return FALSE as the line is probably the first.
       * line of the GFF body. */
      if (!parser->done_header)
	parser->error = g_error_new(parser->error_domain, ZMAP_GFF_ERROR_HEADER,
				    "Bad ## line %d: \"%s\"",
				    parser->line_count, line) ;

      result = FALSE ;
    }
  else
    {
      int fields = 0 ;
      char *format_str = NULL ;

      /* There may be other header comments that we are not interested in so we just return TRUE. */
      result = TRUE ;

      /* Note that number of fields returned by sscanf calls does not include the initial ##<word>
       * as this is not assigned to a variable. */
      /* this could be turned into a table driven piece of code but not worth the effort currently. */
      if (g_str_has_prefix(line, "##gff-version")
	  && !parser->done_version)
	{
	  int version ;

	  fields = 1 ;
	  format_str = "%*13s%d" ;
	  
	  if ((fields = sscanf(line, format_str, &version)) != 1)
	    {
	      parser->error = g_error_new(parser->error_domain, ZMAP_GFF_ERROR_HEADER,
					  "Bad ##gff-version line %d: \"%s\"",
					  parser->line_count, line) ;
	      result = FALSE ;
	    }
	  else
	    {
	      parser->gff_version = version ;
	      parser->done_version = TRUE ;
	    }
	}
      else if (g_str_has_prefix(line, "##source-version")
	       && !parser->done_source)
	{
	  char program[FIELD_BUFFER_LEN] = {'\0'}, version[FIELD_BUFFER_LEN] = {'\0'} ;

	  fields = 3 ;
	  format_str = "%*s%1000s%1000s" ;
	  
	  if ((fields = sscanf(line, format_str, &program[0], &version[0])) != 2)
	    {
	      parser->error = g_error_new(parser->error_domain, ZMAP_GFF_ERROR_HEADER,
					  "Bad ##source-version line %d: \"%s\"",
					  parser->line_count, line) ;
	      result = FALSE ;
	    }
	  else
	    {
	      parser->source_name = g_strdup(&program[0]) ;
	      parser->source_version = g_strdup(&version[0]) ;
	      parser->done_source = TRUE ;
	    }
     	}
      else if (g_str_has_prefix(line, "##sequence-region") && !parser->done_sequence_region)
	{
	  char sequence_name[FIELD_BUFFER_LEN] = {'\0'} ;
	  int start = 0, end = 0 ;

	  fields = 4 ;
	  format_str = "%*s%1000s%d%d" ;
	  
	  if ((fields = sscanf(line, format_str, &sequence_name[0], &start, &end)) != 3)
	    {
	      parser->error = g_error_new(parser->error_domain, ZMAP_GFF_ERROR_HEADER,
					  "Bad ##sequence-region line %d: \"%s\"",
					  parser->line_count, line) ;
	      result = FALSE ;
	    }
	  else
	    {
	      parser->sequence_name = g_strdup(&sequence_name[0]) ;
	      parser->features_start = start ;
	      parser->features_end = end ;
	      parser->done_sequence_region = TRUE ;

	      /* If Clip start/end not set, they default to features start/end. */
	      if (parser->clip_start == 0)
		{
		  parser->clip_start = parser->features_start ;
		  parser->clip_end = parser->features_end ;
		}
	    }
     
	}

      if (parser->done_version && parser->done_source && parser->done_sequence_region)
	parser->done_header = TRUE ;
    }


  return result ;
}



/* This function expects a null-terminated C string that contains a complete GFF record
 * (i.e. a non-comment line), the function expects the caller to already have removed the
 * newline char from the end of the GFF record.
 *
 * GFF version 2 format for a line is:
 *
 * <sequence> <source> <feature> <start> <end> <score> <strand> <phase> [attributes] [#comments]
 * 
 * The format_str matches the above splitting everything into its own strings, note that
 * some fields, although numerical, default to the char "." so cannot be simply scanned into
 * a number. The only tricky bit is to get at the attributes and comments which have
 * white space in them, this scanf format string seems to do it:
 * 
 *  format_str = "%50s%50s%50s%d%d%50s%50s%50s %999[^#] %999c"
 *  
 *  " %999[^#]" Jumps white space after the last mandatory field and then gets everything up to
 *              the next "#", so this will fail if people put a "#" in their attributes !
 * 
 *  " %999c"    Reads everything from (and including) the "#" found by the previous pattern.
 * 
 * If it turns out that people do have "#" chars in their attributes we will have do our own
 * parsing of this section of the line.
 * 
 * For ZMap, we've modified the acedb gff dumper to output homology alignments after the
 * attributes, marked by a tag " Gaps ".  They're in groups of 4 space-separated coordinates,
 * successive groups being comma-separated.  
 *
 * If there's a Gaps tag, we scanf using a different format string, then copy the attributes
 * manually, then call the loadGaps function to load the alignments.
 * 
 *  */
static gboolean parseBodyLine(ZMapGFFParser parser, char *line)
{
  gboolean result = TRUE ;
  char sequence[GFF_MAX_FIELD_CHARS + 1] = {'\0'},
    source[GFF_MAX_FIELD_CHARS + 1] = {'\0'}, feature_type[GFF_MAX_FIELD_CHARS + 1] = {'\0'},
    score_str[GFF_MAX_FIELD_CHARS + 1] = {'\0'}, strand_str[GFF_MAX_FIELD_CHARS + 1] = {'\0'},
						   phase_str[GFF_MAX_FIELD_CHARS + 1] = {'\0'} ;
  char *attributes, *comments ;
  int line_length ;
  int start = 0, end = 0, fields = 0 ;
  double score = 0 ;
  char *format_str = "%50s%50s%50s%d%d%50s%50s%50s %5000[^#] %5000c" ; 
  

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* I'd like to do this to keep everything in step but haven't quite got the right incantation... */

  char *format_str =
    "%"ZMAP_MAKESTRING(GFF_MAX_FIELD_CHARS)"s"
    "%"ZMAP_MAKESTRING(GFF_MAX_FIELD_CHARS)"s"
    "%"ZMAP_MAKESTRING(GFF_MAX_FIELD_CHARS)"s"
    "%d%d"
    "%"ZMAP_MAKESTRING(GFF_MAX_FIELD_CHARS)"s"
    "%"ZMAP_MAKESTRING(GFF_MAX_FIELD_CHARS)"s"
    "%"ZMAP_MAKESTRING(GFF_MAX_FIELD_CHARS)"s"
    " %"ZMAP_MAKESTRING(GFF_MAX_FREETEXT_CHARS)"[^#]"
    " %"ZMAP_MAKESTRING(GFF_MAX_FREETEXT_CHARS)"c" ; 
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* Increase the length of the buffers for attributes and comments if needed, by making them
   * as long as the whole line then they must be long enough. */
  line_length = strlen(line) ;
  if (line_length > parser->attributes_str->allocated_len)
    {
      /* You can't just set a GString to a new length, you have to set it and then zero it. */
      parser->attributes_str = g_string_set_size(parser->attributes_str, line_length) ;
      parser->attributes_str = g_string_set_size(parser->attributes_str, 0) ;
      parser->comments_str = g_string_set_size(parser->comments_str, line_length) ;
      parser->comments_str = g_string_set_size(parser->comments_str, 0) ;
    }
  attributes = parser->attributes_str->str ;
  comments = parser->comments_str->str ;


  fields = sscanf(line, format_str,
                  &sequence[0], &source[0], &feature_type[0],
                  &start, &end, &score_str[0], &strand_str[0], &phase_str[0],
                  &attributes[0], &comments[0]);

  if (result == TRUE 
      && (fields < GFF_MANDATORY_FIELDS
	  || (g_ascii_strcasecmp(source, ".") == 0)
	  || (g_ascii_strcasecmp(feature_type, ".") == 0)))
    {
      parser->error = g_error_new(parser->error_domain, ZMAP_GFF_ERROR_BODY,
				  "GFF line %d - Mandatory fields missing in: \"%s\"",
				  parser->line_count, line) ;
      result = FALSE ;
    }
  else
    {
      ZMapStyleMode type ;
      ZMapStrand strand ;
      ZMapPhase phase ;
      gboolean has_score = FALSE ;
      char *err_text = NULL ;

      /* I'm afraid I'm not doing assembly stuff at the moment, its not worth it....if I need
       * to change this decision I can just this section.....
       * Code just silently drops these lines.
       *  */
      if (g_ascii_strcasecmp(source, "assembly_tag") == 0)
	{
	  result = TRUE ;

	  goto abort ;
	}


      /* Check we could get the basic GFF fields.... */
      if (strlen(sequence) == GFF_MAX_FREETEXT_CHARS)
	err_text = g_strdup_printf("sequence name too long: %s", sequence) ;
      else if (strlen(source) == GFF_MAX_FREETEXT_CHARS)
	err_text = g_strdup_printf("source name too long: %s", source) ;
      else if (strlen(feature_type) == GFF_MAX_FREETEXT_CHARS)
	err_text = g_strdup_printf("feature_type name too long: %s", feature_type) ;
      else if (strlen(attributes) == GFF_MAX_FREETEXT_CHARS)
	err_text = g_strdup_printf("attributes too long: %s", attributes) ;
      else if (!zMapFeatureFormatType(parser->SO_compliant, parser->default_to_basic,
				      feature_type, &type))
	err_text = g_strdup_printf("feature_type not recognised: %s", feature_type) ;
      else if (!zMapFeatureFormatScore(score_str, &has_score, &score))
	err_text = g_strdup_printf("score format not recognised: %s", score_str) ;
      else if (!zMapFeatureFormatStrand(strand_str, &strand))
	err_text = g_strdup_printf("strand format not recognised: %s", strand_str) ;
      else if (!zMapFeatureFormatPhase(phase_str, &phase))
	err_text = g_strdup_printf("phase format not recognised: %s", phase_str) ;

      if (err_text)
	{
	  parser->error = g_error_new(parser->error_domain, ZMAP_GFF_ERROR_BODY,
				      "GFF line %d - %s (\"%s\")",
				      parser->line_count, err_text, line) ;
	  g_free(err_text) ;
	  result = FALSE ;
 	}
      else
	{
	  gboolean include_feature = TRUE ;

          mungeFeatureType(source, &type);

	  /* Clip start/end as specified in clip_mode. */
	  if (parser->clip_mode != GFF_CLIP_NONE)
	    {
	      /* Anything outside always excluded. */
	      if (parser->clip_mode == GFF_CLIP_ALL || parser->clip_mode == GFF_CLIP_OVERLAP)
		{
		  if (start > parser->clip_end || end < parser->clip_start)
		    include_feature = FALSE ;
		}

	      /* Exclude overlaps for CLIP_ALL */
	      if (include_feature && parser->clip_mode == GFF_CLIP_ALL)
		{
		  if (start < parser->clip_start || end > parser->clip_end)
		    include_feature = FALSE ;
		}

	      /* Clip overlaps for CLIP_OVERLAP */
	      if (include_feature && parser->clip_mode == GFF_CLIP_OVERLAP)
		{
		  if (start < parser->clip_start)
		    start = parser->clip_start ;
		  if (end > parser->clip_end)
		    end = parser->clip_end ;
		}
	    }

	  if (include_feature)
	    {
	      GQuark locus_id = 0 ;

	      if (!(result = makeNewFeature(parser, NAME_FIND, sequence,
					    source, feature_type, type,
					    start, end, has_score, score, strand, phase,
					    attributes, &err_text)))
		{
		  parser->error = g_error_new(parser->error_domain, ZMAP_GFF_ERROR_BODY,
					      "GFF line %d - %s (\"%s\")",
					      parser->line_count, err_text, line) ;
		  g_free(err_text) ;
		}
	      

	      /* If the Locus feature set has been requested then check to see if this feature
	       * has a locus and add it to the locus feature set. Note this is different in that
	       * locus entries are not exported as a separate features but as part of another
	       * feature.
	       * 
	       * We may wish to revisit this and have locus items as separate features but its
	       * not straight forwared to export them from acedb _and_ get their locations. */
	      if (parser->locus_set_id && (locus_id = getLocus(attributes)))
		{

		  if (!zMapFeatureFormatType(parser->SO_compliant, parser->default_to_basic,
					     ZMAP_FIXED_STYLE_LOCUS_NAME, &type))
		    err_text = g_strdup_printf("feature_type not recognised: %s", ZMAP_FIXED_STYLE_LOCUS_NAME) ;


		  if (!(result = makeNewFeature(parser, NAME_USE_SEQUENCE, (char *)g_quark_to_string(locus_id),
						ZMAP_FIXED_STYLE_LOCUS_NAME, ZMAP_FIXED_STYLE_LOCUS_NAME, type,
						start, end, FALSE, 0.0, ZMAPSTRAND_NONE, ZMAPPHASE_NONE,
						attributes, &err_text)))
		    {
		      parser->error = g_error_new(parser->error_domain, ZMAP_GFF_ERROR_BODY,
						  "GFF line %d - %s (\"%s\")",
						  parser->line_count, err_text, line) ;
		      g_free(err_text) ;
		    }
		}
	    }
	}
    }


 abort:							    /* Used for non-processing of assembly_tags. */

  /* Reset contents to nothing. */
  parser->attributes_str = g_string_set_size(parser->attributes_str, 0) ;
  parser->comments_str = g_string_set_size(parser->comments_str, 0) ;

  return result ;
}


static gboolean makeNewFeature(ZMapGFFParser parser, NameFindType name_find,
			       char *sequence, char *source, char *ontology,
			       ZMapStyleMode feature_type,
			       int start, int end,
			       gboolean has_score, double score,
			       ZMapStrand strand, ZMapPhase phase,
			       char *attributes, char **err_text)
{
  gboolean result = FALSE ;
  char *feature_name_id = NULL, *feature_name = NULL ;
  GQuark feature_style_id ;
  ZMapFeatureSet feature_set = NULL ;
  ZMapFeatureTypeStyle feature_set_style, feature_style ;
  ZMapFeature feature = NULL ;
  ZMapGFFParserFeatureSet parser_feature_set = NULL ; 
  char *feature_set_name = NULL ;
  gboolean feature_has_name ;
  ZMapFeature new_feature ;
  ZMapHomolType homol_type ;
  int query_start = 0, query_end = 0, query_length = 0 ;
  ZMapStrand query_strand ;
  ZMapSpanStruct exon = {0}, *exon_ptr = NULL, intron = {0}, *intron_ptr = NULL ;
  char *url ;
  GQuark locus_id = 0 ;
  GArray *gaps = NULL;
  GArray *path = NULL ;
  char *gaps_onwards = NULL;
  char *note_text, *source_text = NULL ;
  GQuark clone_id = 0, source_id = 0 ;


  /* If the parser was given a source -> data mapping then use that to get the style id and other
   * data otherwise* use the source itself. */
  if (parser->source_2_sourcedata)
    {
      ZMapGFFSource source_data ;


      source_data = g_hash_table_lookup(parser->source_2_sourcedata,
					GINT_TO_POINTER(zMapStyleCreateID(source))) ;

      feature_style_id = zMapStyleCreateID((char *)g_quark_to_string(source_data->style_id)) ;

      source_id = source_data->source_id ;
      source_text = (char *)g_quark_to_string(source_data->source_text) ;
    }
  else  
    {
      source_id = feature_style_id = zMapStyleCreateID(source) ;
    }


  /* If the parser was given a source -> column group mapping then use that as the feature set
   * otherwise use the source itself. */
  if (parser->source_2_feature_set)
    {
      ZMapGFFSet set_data ;

      set_data = g_hash_table_lookup(parser->source_2_feature_set,
				     GINT_TO_POINTER(zMapStyleCreateID(source))) ;

      feature_set_name = (char *)g_quark_to_string(set_data->feature_set_id) ;
    }
  else
    {
      feature_set_name = source ;
    }



  /* If a feature set style or a feature style is missing then we can't carry on.
   * NOTE the feature sets style has the same name as the feature set. */
  if (!(feature_set_style = zMapFindStyle(parser->sources, feature_style_id)))
    {
      *err_text = g_strdup_printf("feature ignored, could not find style \"%s\" for feature set \"%s\".",
				  g_quark_to_string(feature_style_id), feature_set_name) ;
      result = FALSE ;

      return result ;
    }

  if (!(feature_style = zMapFindStyle(parser->sources, feature_style_id)))
    {
      *err_text = g_strdup_printf("feature ignored, could not find its style \"%s\".",
				  g_quark_to_string(feature_style_id)) ;
      result = FALSE ;

      return result ;
    }


  /* I'M NOT HAPPY WITH THIS, IT DOESN'T WORK AS A CONCEPT....NEED TYPES IN FEATURE STRUCT
   * AND IN STYLE...BUT THEY HAVE DIFFERENT PURPOSE.... */
  /* Big departure...get feature type from style..... */
  if (zMapStyleHasMode(feature_style))
    {
      ZMapStyleMode style_mode ;

      style_mode = zMapStyleGetMode(feature_style) ;

      feature_type = style_mode ;
    }


  /* We load some mode specific data which is needed in making a unique feature name. */
  if (feature_type == ZMAPSTYLE_MODE_ALIGNMENT)
    {
      clone_id = getClone(attributes) ;

      homol_type = ZMAPHOMOL_NONE ;

      if (!(result = getHomolAttrs(attributes, &homol_type, &query_start, &query_end, &query_strand)))
	return result ;
      else
	result = getHomolLength(attributes, &query_length) ; /* Not fatal to not have length. */
    }
  else if (feature_type == ZMAPSTYLE_MODE_ASSEMBLY_PATH)
    {
      if (!(result = getAssemblyPathAttrs(attributes, NULL, &query_strand, &query_length, &path)))
	return result ;
    }


  /* Was there a url for the feature ? */
  url = getURL(attributes) ;


  /* Was there a locus for the feature ? */
  locus_id = getLocus(attributes) ;


  /* Often text is attached to a feature as a "Note", retrieve this data if present. */
  note_text = getNoteText(attributes) ;


  /* Get the feature name which may not be unique and a feature "id" which _must_
   * be unique. */
  feature_has_name = getFeatureName(name_find, sequence, attributes, source, feature_type, strand,
				    start, end, query_start, query_end,
				    &feature_name, &feature_name_id) ;


  /* Check if the feature_set_name for this feature is already known, if it is then check if there
   * is already a multiline feature with the same name as we will need to augment it with this data. */
  if (!parser->parse_only &&
      (parser_feature_set = (ZMapGFFParserFeatureSet)g_datalist_get_data(&(parser->feature_sets),
									 feature_set_name)))
    {
      feature_set = parser_feature_set->feature_set ;

      feature = (ZMapFeature)g_datalist_get_data(&(parser_feature_set->multiline_features),
						 feature_name) ;
    }


  if (parser->parse_only || !feature)
    {
      new_feature = zMapFeatureCreateEmpty() ;
    }


  /* FOR PARSE ONLY WE WOULD LIKE TO CONTINUE TO USE THE LOCAL VARIABLE new_feature....SORT THIS
   * OUT............. */

  if (parser->parse_only)
    {
      feature = new_feature ;
    }
  else if (!feature)
    {
      /* If we haven't got an existing feature then fill one in and add it to its feature_set_name
       * if that exists, otherwise we have to create a list for the feature_set_name and then
       * add that to the list of sources...ugh.  */

      /* If we don't have this feature_set yet, then make one. */
      if (!parser_feature_set)
	{
	  parser_feature_set = g_new0(ZMapGFFParserFeatureSetStruct, 1) ;

	  g_datalist_set_data_full(&(parser->feature_sets),
				   feature_set_name, parser_feature_set, destroyFeatureArray) ;
	  
	  feature_set = parser_feature_set->feature_set = zMapFeatureSetCreate(feature_set_name, NULL) ;

	  zMapFeatureSetStyle(feature_set, feature_set_style) ;	/* Set the style for the set. */

	  parser_feature_set->multiline_features = NULL ;
	  g_datalist_init(&(parser_feature_set->multiline_features)) ;

	  parser_feature_set->parser = parser ;		    /* We need parser flags in the destroy
							       function for the feature_set list. */
	}
#ifdef RDS_USES_FEATURE_SET_ADD_FEATURE
      /* Always add every new feature to the final set.... */
      g_datalist_set_data(&(feature_set->features), feature_name_id, new_feature) ;
#endif

      feature = new_feature ;

      /* THIS PIECE OF CODE WILL NEED TO BE CHANGED AS I DO MORE TYPES..... */
      /* If the feature is one that must be built up from several GFF lines then add it to
       * our set of such features. There are arcane/adhoc rules in action here, any features
       * that do not have their own feature_name  _cannot_  be multiline features as such features
       * can _only_ be identified if they do have their own name. */
      if (feature_has_name && (feature_type == ZMAPSTYLE_MODE_TRANSCRIPT))
	{
	  g_datalist_set_data(&(parser_feature_set->multiline_features), feature_name, feature) ;
	}
    }


 if ((result = zMapFeatureAddStandardData(feature, feature_name_id, feature_name,
					  sequence, ontology,
					  feature_type, feature_style,
					  start, end,
					  has_score, score,
					  strand, phase)))
   {
     zMapFeatureSetAddFeature(feature_set, feature);

     if (url)
       zMapFeatureAddURL(feature, url) ;

     if (locus_id)
       zMapFeatureAddLocus(feature, locus_id) ;

     /* We always have a source_id and optionally text. */
     zMapFeatureAddText(feature, source_id, source_text, note_text) ;

     if (feature_type == ZMAPSTYLE_MODE_TRANSCRIPT)
       {
	 gboolean start_not_found = FALSE, end_not_found = FALSE ;
	 int start_phase = 0 ;

         /* Note that exons/introns are given one per line in GFF which is quite annoying.....it is
          * out of sync with how homols with gaps are given.... */
         if (g_ascii_strcasecmp(ontology, "coding_exon") == 0
             || g_ascii_strcasecmp(ontology, "exon") == 0)
           {
             exon.x1 = start ;
             exon.x2 = end ;
             exon_ptr = &exon ;
           }
         else if (g_ascii_strcasecmp(ontology, "intron") == 0)
           {
             intron.x1 = start ;
             intron.x2 = end ;
             intron_ptr = &intron ;
           }


	 if (g_ascii_strcasecmp(ontology, "CDS") == 0)
	   {
	     result = zMapFeatureAddTranscriptData(feature,
						   TRUE, start, end,
						   NULL, NULL) ;
	   }

	 if (result && (result = getCDSAttrs(attributes,
					     &start_not_found, &start_phase,
					     &end_not_found)))
	   {
	     result = zMapFeatureAddTranscriptStartEnd(feature,
						       start_not_found, start_phase,
						       end_not_found) ;
	   }

	 if (result && (exon_ptr || intron_ptr))
	   result = zMapFeatureAddTranscriptExonIntron(feature, exon_ptr, intron_ptr) ;
       }
     else if (feature_type == ZMAPSTYLE_MODE_ALIGNMENT)
       {
	 char *local_sequence_str ;
	 gboolean local_sequence = FALSE ;

	 /* I am not sure if we ever have target_phase from GFF output....check this out... */
         if (zMapStyleIsParseGaps(feature_style) && ((gaps_onwards = strstr(attributes, "\tGaps ")) != NULL)) 
           {
             gaps = g_array_new(FALSE, FALSE, sizeof(ZMapAlignBlockStruct));
             gaps_onwards += 6;  /* skip over Gaps tag and pass "1 12 12 122, ..." incl "" not
				    terminated */

	     if (!loadGaps(gaps_onwards, gaps))
	       {
		 g_array_free(gaps, TRUE) ;
		 gaps = NULL ;
	       }
           }

	 if ((local_sequence_str = strstr(attributes, "\tOwn_Sequence TRUE")))
	   local_sequence = TRUE ;

	 
	 result = zMapFeatureAddAlignmentData(feature, clone_id,
					      query_start, query_end,
					      homol_type, query_length, query_strand, ZMAPPHASE_0,
					      gaps, zmapStyleGetWithinAlignError(feature_style),
					      local_sequence) ;
       }
     else if (feature_type == ZMAPSTYLE_MODE_ASSEMBLY_PATH)
       {
	 result = zMapFeatureAddAssemblyPathData(feature, query_length, query_strand, path) ;
       }
     else
       {
	 if (g_ascii_strcasecmp(source, "genomic_canonical") == 0)
	   {
	     char *known_name = NULL ;

	     if ((result = getKnownName(attributes, &known_name)))
	       {
		 if (!(result = zMapFeatureAddKnownName(feature, known_name)))
		   *err_text = g_strdup_printf("Bad format for Known_name attribute \"%s\".", attributes) ;
	       }
	   }
	 else
	   {
	     if (g_ascii_strcasecmp(ontology, "splice5") == 0)
	       result = zMapFeatureAddSplice(feature, ZMAPBOUNDARY_5_SPLICE) ;
	     else if (g_ascii_strcasecmp(ontology, "splice3") == 0)
	       result = zMapFeatureAddSplice(feature, ZMAPBOUNDARY_3_SPLICE) ;

	     if (!result)
	       *err_text = g_strdup_printf("feature ignored, could not set \"%s\" splice data.", ontology) ;
	   }
       }
   }


  g_free(feature_name) ;
  g_free(feature_name_id) ;
  g_free(url) ;

  /* If we are only parsing then free any stuff allocated by addDataToFeature() */
  if (parser->parse_only)
    {
      zMapFeatureDestroy(feature) ;
    }


  return result ;
}


/* This reads any gaps which are present on the gff line. They are preceded by a Gaps tag, and are presented as
 * space-delimited groups of 4, consecutive groups being comma-delimited. gapsPos is wherever we are in the gff
 * and is set to NULL when strstr can't find another comma. fields must be 4 for a gap so either way we drop out
 * of the loop at the end. i.e. gapPos should equal something like this (incl "")
 *
 *                             "531 544 34799 34758,545 550 34751 34734"
 * 
 * All gap coords should be positive and 1-based.
 */
static gboolean loadGaps(char *gapsPos, GArray *gaps)
{
  gboolean valid = TRUE ;
  gboolean avoidFirst_strstr = TRUE ;
  ZMapAlignBlockStruct gap = { 0 };
  char *gaps_format_str = "%d%d%d%d" ; 
  int fields;

  while (avoidFirst_strstr == TRUE || ((gapsPos = strstr(gapsPos, ",")) != NULL))
    {
      avoidFirst_strstr = FALSE; /* Only to get here to start with */

      /* ++gapsPos to skip the '"' or the ',' */
      if ((fields = sscanf(++gapsPos, gaps_format_str, &gap.q1, &gap.q2, &gap.t1, &gap.t2)) == 4)
        {
	  if (gap.q1 < 1 || gap.q2 < 1 || gap.t1 < 1 || gap.t2 < 1)
	    {
	      valid = FALSE ;
	      break ;
	    }
	  else
	    {
	      int tmp;

	      gap.q_strand = ZMAPSTRAND_FORWARD;
	      gap.t_strand = ZMAPSTRAND_FORWARD;

	      if (gap.q1 > gap.q2)
		{
		  tmp = gap.q2;
		  gap.q2 = gap.q1;
		  gap.q1 = tmp;
		  gap.q_strand = ZMAPSTRAND_REVERSE;
		}

	      if (gap.t1 > gap.t2)
		{
		  tmp = gap.t2;
		  gap.t2 = gap.t1;
		  gap.t1 = tmp;
		  gap.t_strand = ZMAPSTRAND_REVERSE;
		}

	      gaps = g_array_append_val(gaps, gap);
	    }
	}
      else
        {
          valid = FALSE;
          break;  /* anything other than 4 is not a gap */
        }
    }


  return valid ;
}


/* This routine attempts to find the features name from its attributes field, in output from
 * acedb the attributes start with:
 * 
 *          class "object_name"    e.g.   Sequence "B0250.1"
 * 
 * For some features this is not true or the feature name is shared amongst many
 * features and so we must construct a name from the feature name and the coords.
 * 
 * For homology features the name is in the attributes in the form:
 * 
 *        Target "Classname:objname" query_start query_end
 * 
 * For assembly path features the name is in the attributes in the form:
 * 
 *        Assembly_source "Classname:objname"
 * 
 * For genefinder features they all have a source field that starts "GF_" so we use that.
 * 
 *        B0250	GF_splice	splice3	106	107	0.233743	+	.
 *        B0250	GF_ATG	atg	38985	38987	1.8345	-	0
 *        etc.
 * 
 * Some features have their name given in the "Note" field appended to some GFF records.
 * This field is also used for general comments however so the code must attempt to
 * deal with this, here are some examples:
 * 
 * Here's one that is a feature name:
 * 
 *        Note "RP5-931H19"
 * 
 * and here's some that aren't:
 * 
 *        Note "Left: B0250"
 *        Note "7 copies of 31mer"
 * 
 * and so on....
 * 
 *  */
static gboolean getFeatureName(NameFindType name_find, char *sequence, char *attributes,
			       char *source,
			       ZMapStyleMode feature_type,
			       ZMapStrand strand, int start, int end, int query_start, int query_end,
			       char **feature_name, char **feature_name_id)
{
  gboolean has_name = FALSE ;


  /* Probably we should do some checking to make sure start/end are in correct order....and
   * that other fields are correct.... */
  if (name_find == NAME_USE_SEQUENCE)
    {
      has_name = TRUE ;

      *feature_name = g_strdup(sequence) ;
      *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
					       start, end, query_start, query_end) ;
    }
  else if (name_find == NAME_USE_SOURCE)
    {
      has_name = TRUE ;

      *feature_name = g_strdup(source) ;
      *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
					       start, end, query_start, query_end) ;
    }
  else
    {
      char *tag_pos ;

      if (feature_type == ZMAPSTYLE_MODE_ALIGNMENT)
	{
	  /* This needs amalgamating with the gethomols routine..... */

	  /* This is a horrible sort of catch all but we are forced into a bit by the lack of 
	   * clarity in the GFFv2 spec....needs some attention.... */
	  if (g_str_has_prefix(attributes, "Note"))
	    {
	      char *name = NULL ;

	      if (getNameFromNote(attributes, &name))
		{
		  *feature_name = g_strdup(name) ;
		  *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
							   start, end, query_start, query_end) ;

		  has_name = TRUE ;
		}
	      else
		{
		  *feature_name = g_strdup(sequence) ;
		  *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
							   start, end, query_start, query_end) ;
		}
	    }
	  else if ((tag_pos = strstr(attributes, "Target")))
	    {
	      /* In acedb output at least, homologies all have the same format. */
	      int attr_fields ;
	      char *attr_format_str = "Target %*[\"]%*[^:]%*[:]%50[^\"]%*[\"]%*s" ;
	      char name[GFF_MAX_FIELD_CHARS + 1] = {'\0'} ;

	      attr_fields = sscanf(tag_pos, attr_format_str, &name[0]) ;

	      if (attr_fields == 1)
		{
		  has_name = FALSE ;
		  *feature_name = g_strdup(name) ;
		  *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
							   start, end, query_start, query_end) ;
		}
	    }
	}
      else if (feature_type == ZMAPSTYLE_MODE_ASSEMBLY_PATH)
	{
	  if ((tag_pos = strstr(attributes, "Assembly_source")))
	    {
	      int attr_fields ;
	      char *attr_format_str = "Assembly_source %*[\"]%*[^:]%*[:]%50[^\"]%*[\"]%*s" ;
	      char name[GFF_MAX_FIELD_CHARS + 1] = {'\0'} ;

	      attr_fields = sscanf(tag_pos, attr_format_str, &name[0]) ;

	      if (attr_fields == 1)
		{
		  has_name = FALSE ;
		  *feature_name = g_strdup(name) ;
		  *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
							   start, end, query_start, query_end) ;
		}
	    }
	}
      else if (feature_type == ZMAPSTYLE_MODE_BASIC
	       && (g_str_has_prefix(source, "GF_")
		   || (g_ascii_strcasecmp(source, "hexexon") == 0)))
	{
	  /* Genefinder features, we use the source field as the name.... */

	  has_name = FALSE ;				    /* is this correct ??? */

	  *feature_name = g_strdup(source) ;
	  *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
						   start, end, query_start, query_end) ;
	}
      else /* if (feature_type == ZMAPSTYLE_MODE_TRANSCRIPT) */
	{
	  has_name = FALSE ;

	  /* This is a horrible sort of catch all but we are forced into a bit by the lack of 
	   * clarity in the GFFv2 spec....needs some attention.... */
	  if (g_str_has_prefix(attributes, "Note"))
	    {
	      char *name = NULL ;

	      if (getNameFromNote(attributes, &name))
		{
		  *feature_name = g_strdup(name) ;
		  *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
							   start, end, query_start, query_end) ;

		  has_name = TRUE ;
		}
	      else
		{
		  *feature_name = g_strdup(sequence) ;
		  *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
							   start, end, query_start, query_end) ;
		}
	    }
	  else
	    {
	      /* Named feature such as a gene. */
	      int attr_fields ;
	      char *attr_format_str = "%50s %*[\"]%50[^\"]%*[\"]%*s" ;
	      char class[GFF_MAX_FIELD_CHARS + 1] = {'\0'}, name[GFF_MAX_FIELD_CHARS + 1] = {'\0'} ;

	      attr_fields = sscanf(attributes, attr_format_str, &class[0], &name[0]) ;

	      if (attr_fields == 2)
		{
		  has_name         = TRUE ;
		  *feature_name    = g_strdup(name) ;
		  *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
							   start, end, query_start, query_end) ;
		}
	      else
		{
		  *feature_name = g_strdup(sequence) ;
		  *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
							   start, end, query_start, query_end) ;
		}
	    }
	}
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      else
	{
	  /* This is a horrible sort of catch all but we are forced into a bit by the lack of 
	   * clarity in the GFFv2 spec. */
	  has_name = FALSE ;

	  if (g_str_has_prefix(attributes, "Note"))
	    *feature_name = g_strdup(sequence) ;
	  else
	    *feature_name = g_strdup(sequence) ;

	  *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
						   start, end, query_start, query_end) ;
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }



  return has_name ;
}




/* Format of URL attribute section is:
 * 
 *          URL "http://etc. etc." ;
 * 
 * Format string extracts url and returns it as a string that must be g_free'd when
 * no longer required.
 *
 * Currently the _maximum_ size for the URL is 256 chars. This has already 
 * been upped from the GFF_MAX_FIELD_CHARS of 50. attributes is 5000 
 * (GFF_MAX_FREETEXT_CHARS), but that's probably a bit silly.
 * 
 *  */
static char *getURL(char *attributes)
{
  char *url = NULL ;
  char *tag_pos ;

  if ((tag_pos = strstr(attributes, "URL")))
    {
      int attr_fields ;
      char *attr_format_str = "%*s %*[\"]%256[^\"]%*s[;]" ;
      char url_field[257] = {'\0'} ;

      if ((attr_fields = sscanf(tag_pos, attr_format_str, &url_field[0])) == 1)
	{
	  url = g_strdup(&url_field[0]) ;
	}
    }

  return url ;
}


/* Format of Locus attribute section is:
 * 
 *          Locus "<locus_name>" ;
 * 
 * Format string extracts locus name returns it as a quark.
 * 
 *  */
static GQuark getLocus(char *attributes)
{
  GQuark locus_id = 0 ;
  char *tag_pos ;

  if ((tag_pos = strstr(attributes, "Locus")))
    {
      int attr_fields ;
      char *attr_format_str = "%*s %*[\"]%50[^\"]%*s[;]" ;
      char locus_field[GFF_MAX_FIELD_CHARS + 1] = {'\0'} ;

      if ((attr_fields = sscanf(tag_pos, attr_format_str, &locus_field[0])) == 1)
	{
	  locus_id = g_quark_from_string(&locus_field[0]) ;
	}
    }

  return locus_id ;
}


/* Format of Clone attribute section is:
 * 
 *          Clone "<clone_name>" ;
 * 
 * Format string extracts clone name returns it as a quark.
 * 
 *  */
static GQuark getClone(char *attributes)
{
  GQuark clone_id = 0 ;
  char *tag_pos ;

  if ((tag_pos = strstr(attributes, "Clone")))
    {
      int attr_fields ;
      char *attr_format_str = "%*s %*[\"]%50[^\"]%*s[;]" ;
      char clone_field[GFF_MAX_FIELD_CHARS + 1] = {'\0'} ;

      if ((attr_fields = sscanf(tag_pos, attr_format_str, &clone_field[0])) == 1)
	{
	  clone_id = g_quark_from_string(&clone_field[0]) ;
	}
    }

  return clone_id ;
}


/* Format of Known name attribute section is:
 * 
 *          Known_name <"known_name"> ;
 * 
 * Format string extracts known name returns it as a string that must be g_free'd.
 * 
 *  */
static gboolean getKnownName(char *attributes, char **known_name_out)
{
  gboolean result = FALSE ;
  char *tag_pos ;

  if ((tag_pos = strstr(attributes, "Known_name")))
    {
      int attr_fields ;
      char *attr_format_str = "%*s %*[\"]%50[^\"]%*s[;]" ;
      char known_field[GFF_MAX_FIELD_CHARS + 1] = {'\0'} ;

      if ((attr_fields = sscanf(tag_pos, attr_format_str, &known_field[0])) == 1)
	{
	  char *known_name = NULL ;

	  known_name = g_strdup(&known_field[0]) ;
	  *known_name_out = known_name ;
	  result = TRUE ;
	}
    }

  return result ;
}


/* 
 * 
 * Format of similarity/homol attribute section is:
 * 
 *          Target "class:obj_name" start end [Gaps "Qstart Qend Tstart Tend, ..."]
 * 
 * Format string extracts  class:obj_name  and  start and end.
 * 
 *  */
static gboolean getHomolAttrs(char *attributes, ZMapHomolType *homol_type_out,
			      int *start_out, int *end_out, ZMapStrand *query_strand)
{
  gboolean result = FALSE ;
  char *tag_pos ;

  if ((tag_pos = strstr(attributes, "Target")))
    {
      int attr_fields ;
      char *attr_format_str = "%*s %*[\"]%50[^\"]%*s%d%d" ;
      char homol_type_str[GFF_MAX_FIELD_CHARS + 1] = {'\0'} ;
      int start = 0, end = 0 ;
      ZMapHomolType homol_type = ZMAPHOMOL_NONE ;

      if ((attr_fields = sscanf(tag_pos, attr_format_str, &homol_type_str[0], &start, &end)) == 3)
	{
	  if (g_ascii_strncasecmp(homol_type_str, "Sequence:", 9) == 0)
	    homol_type = ZMAPHOMOL_N_HOMOL ;
	  else if (g_ascii_strncasecmp(homol_type_str, "Protein:", 8) == 0)
	    homol_type = ZMAPHOMOL_X_HOMOL ;
	  else if (g_ascii_strncasecmp(homol_type_str, "Motif:", 6) == 0)
	    homol_type = ZMAPHOMOL_X_HOMOL ;

	  if (homol_type && start > 0 && end > 0)
	    {
	      *homol_type_out = homol_type ;
	      *start_out = start ;
	      *end_out = end ;

	      /* There is no recording of the strand of single length alignments in gff from acedb so we just
	       * assign to forward strand...probably there won't be in any single base alignments.... */
	      if (start <= end)
		*query_strand = ZMAPSTRAND_FORWARD ;
	      else
		*query_strand = ZMAPSTRAND_REVERSE ;

	      result = TRUE ;
	    }
	  else
	    {
	      zMapLogWarning("Bad homol type or start/end: %s", tag_pos) ;
	    }
	}
      else
	{
	  zMapLogWarning("Could not parse Homol Data: %s", tag_pos) ;
	}
    }
  else
    {
      /* Special for wormbase way of doing repeats...in this instance there are no match start/end coords. */
      if ((strstr(attributes, "Note"))
	  && ((strstr(attributes, "copies")) || (strstr(attributes, "loop"))))
	{
	  *homol_type_out = ZMAPHOMOL_N_HOMOL ;

	  result = TRUE ;
	}
      else
	{
	  zMapLogWarning("Could not parse wormbase style Homol Data: %s", tag_pos) ;
	}
    }

  return result ;
}




/* 
 * 
 * Format of assembly path attributes section is:
 * 
 * Assembly_source "Sequence:B0250" ; Assembly_strand + ; Assembly_length 39216 ; Assembly_regions 1 39110 [, start end]+ ;
 * 
 * N.B. Assembly_source has already been extracted.
 * 
 *  */
static gboolean getAssemblyPathAttrs(char *attributes, char **assembly_name_unused,
				     ZMapStrand *strand_out, int *length_out, GArray **path_out)
{
  gboolean result = TRUE ;
  char *tag_pos ;
  int start = 0, end = 0 ;
  ZMapStrand strand ;
  int length = 0 ;
  GArray *path = NULL ;

  if (result && (result = GPOINTER_TO_INT(tag_pos = strstr(attributes, "Assembly_strand"))))
    {
      int attr_fields ;
      char *attr_format_str = "%*s%s" ;
      char strand_str[GFF_MAX_FIELD_CHARS + 1] = {'\0'} ;

      if ((attr_fields = sscanf(tag_pos, attr_format_str, &strand_str[0])) != 1
	  || !zMapFeatureFormatStrand(strand_str, &strand))
	{
	  result = FALSE ;

	  zMapLogWarning("Could not recover Assembly_region strand: %s", tag_pos) ;
	}
    }

  if (result && (result = GPOINTER_TO_INT(tag_pos = strstr(attributes, "Assembly_length"))))
    {
      int attr_fields ;
      char *attr_format_str = "%*s%d" ;

      if ((attr_fields = sscanf(tag_pos, attr_format_str, &length)) != 1)
	{
	  result = FALSE ;

	  zMapLogWarning("Could not recover Assembly_length length: %s", tag_pos) ;
	}
    }

  if (result && (result = GPOINTER_TO_INT(tag_pos = strstr(attributes, "Assembly_regions"))))
    {
      int attr_fields ;
      char *cp ;
      char *attr_format_str = "%d%d" ;

      path = g_array_new(FALSE, TRUE, sizeof(ZMapSpanStruct)) ;

      cp = tag_pos + strlen("Assembly_regions") ;
      do
	{
	  ZMapSpanStruct span = {0} ;

	  if ((attr_fields = sscanf(cp, attr_format_str, &span.x1, &span.x2)) == 2)
	    {
	      path = g_array_append_val(path, span);
	    }
	  else
	    {
	      result = FALSE ;

	      zMapLogWarning("Could not recover Assembly_region start/end coords from: %s", tag_pos) ;

	      break ;
	    }
	} while ((cp = strstr(cp, ",")) != NULL) ;

      if (!result)
	{
	  g_array_free(path, TRUE) ;
	  path = NULL ;
	}
    }

  if (result)
    {
      *strand_out = strand ;
      *length_out = length ;
      *path_out = path ;
    }


  return result ;
}




/* 
 * 
 * Format of CDS attribute section is:
 * 
 *          class "obj_name" [; start_not_found phase] [; end_not_found]
 * 
 * Format string extracts   phase for start_not_found
 * 
 *  */
static gboolean getCDSAttrs(char *attributes,
			    gboolean *start_not_found_out, int *start_phase_out,
			    gboolean *end_not_found_out)
{
  gboolean result = TRUE ;
  char *target ;
  gboolean start_not_found = FALSE, end_not_found = FALSE ;

  if ((target = strstr(attributes, "start_not_found")))
    {
      start_not_found = FALSE ;
      int attr_fields ;
      char *attr_format_str = "%*s %d %*s" ;
      int start_phase = 0 ;

      start_not_found =  TRUE ;

      attr_fields = sscanf(target, attr_format_str, &start_phase) ;

      if (attr_fields == 1)
	{
	  if (start_phase < 1 || start_phase > 3)
	    result = FALSE ;
	}

      if (result)
	{
	  *start_not_found_out = start_not_found ;

	  *start_phase_out = start_phase ;
	}
    }


  if (result && (target = strstr(attributes, "end_not_found")))
    *end_not_found_out = end_not_found = TRUE ;

  return result ;
}



/* Format of Lemgth attribute section is:
 * 
 *          Length <length> ;
 * 
 * Format string extracts the integer length.
 * 
 *  */
static gboolean getHomolLength(char *attributes, int *length_out)
{
  gboolean result = FALSE ;
  char *target ;

  if ((target = strstr(attributes, "Length")))
    {
      int attr_fields ;
      char *attr_format_str = "%*s %d %*s" ;
      int length = 0 ;

      if ((attr_fields = sscanf(target, attr_format_str, &length)) == 1)
	{
	  *length_out = length ;
	  result = TRUE ;
	}
    }

  return result ;
}



/* This is a GDataForeachFunc() and is called for each element of a GData list as a result
 * of a call to zmapGFFGetFeatures(). The function adds the feature array returned 
 * in the GData element to the GArray in user_data. */
static void getFeatureArray(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapGFFParserFeatureSet parser_feature_set = (ZMapGFFParserFeatureSet)data ;
  ZMapFeatureSet feature_set = parser_feature_set->feature_set ;
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)user_data ;

  zMapFeatureBlockAddFeatureSet(feature_block, feature_set);

  return ;
}


/* This is a GDestroyNotify() and is called for each element in a GData list when
 * the list is cleared with g_datalist_clear (), the function must free the GArray
 * and GData lists. */
static void destroyFeatureArray(gpointer data)
{
  ZMapGFFParserFeatureSet parser_feature_set = (ZMapGFFParserFeatureSet)data ;

  /* No data to free in this list, just clear it. */
  g_datalist_clear(&(parser_feature_set->multiline_features)) ;

  g_free(parser_feature_set) ;

  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void stylePrintCB(gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;

  printf("%s (%s)\n", g_quark_to_string(style->original_id), g_quark_to_string(style->unique_id)) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static void mungeFeatureType(char *source, ZMapStyleMode *type_inout)
{
  zMapAssert(type_inout);

  if(g_ascii_strcasecmp(source, "Genomic_canonical") == 0)
    *type_inout = ZMAPSTYLE_MODE_BASIC;

  return ;
}



/* We get passed a string that should be of the form:
 * 
 *    Note "some variable amount of text...."
 * 
 * Returns TRUE if the note is of the form:
 * 
 *    Note "valid_feature_name"
 * 
 * where a valid name starts with a letter and contains only alphanumberics and '_' or ':'.
 * 
 *  */
static gboolean getNameFromNote(char *attributes, char **name)
{
  gboolean result = FALSE ;
  int attr_fields ;
  char *note_format_str = "Note %*[\"]%5000[^\"]" ;
  char note[GFF_MAX_FREETEXT_CHARS + 1] = {'\0'} ;
  char *name_format_str = "%50s%50s" ;
  char feature_name[GFF_MAX_FIELD_CHARS + 1] = {'\0'}, rest[GFF_MAX_FIELD_CHARS + 1] = {'\0'} ;

  /* I couldn't find a way to do this in one sscanf() so I do it in two, getting the note text
   * first and then splitting out the name (hopefully). */
  if ((attr_fields = sscanf(attributes, note_format_str, &note[0])) == 1)
    {
      attr_fields = sscanf(&note[0], name_format_str, &feature_name[0], &rest[0]) ;

      if (attr_fields == 1)
	{
	  char *cptr ;

	  cptr = &feature_name[0] ;

	  if (g_ascii_isalpha(*cptr))
	    {
	      result = TRUE ;
	      while (*cptr)
		{
		  if (!g_ascii_isalnum(*cptr) && *cptr != '_' && *cptr != ':')
		    {
		      result = FALSE ;
		      break ;
		    }
		  cptr++ ;
		}

	      if (result)
		*name = &(feature_name[0]) ;
	    }
	}
    }

  return result ;
}



/* We get passed a string that should be of the form:
 * 
 *    Note "some variable amount of text...."
 * 
 * Returns a copy of the string or NULL if not in the above form. String
 * should be g_free'd by caller.
 * 
 *  */
static char *getNoteText(char *attributes)
{
  char *note_text = NULL ;
  int attr_fields ;
  char *note_format_str = "Note %*[\"]%5000[^\"]" ;
  char note[5000 + 1] = {'\0'} ;


  if (g_str_has_prefix(attributes, "Note")
      && (attr_fields = sscanf(attributes, note_format_str, &note[0])) == 1)
    {
      note_text = g_strdup(&note[0]) ;
    }

  return note_text ;
}
