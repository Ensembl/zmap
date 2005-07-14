/*  File: zmapGFF2parser.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: parser for GFF version 2 format.
 *              
 * Exported functions: See ZMap/zmapGFF.h
 * HISTORY:
 * Last edited: Jul 14 09:44 2005 (rnc)
 * Created: Fri May 28 14:25:12 2004 (edgrif)
 * CVS info:   $Id: zmapGFF2parser.c,v 1.27 2005-07-14 09:55:58 rnc Exp $
 *-------------------------------------------------------------------
 */

#include <stdio.h>
#include <strings.h>
#include <errno.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFeature.h>
#include <zmapGFF_P.h>

/* HACKED CODE TO FIX UP LINKS IN BLOCK->SET->FEATURE */
void setBlock(GQuark key_id, gpointer data, gpointer user_data) ;
void setSet(GQuark key_id, gpointer data, gpointer user_data) ;



/* END OF HACKED CODE TO FIX UP LINKS IN BLOCK->SET->FEATURE */


static gboolean parseHeaderLine(ZMapGFFParser parser, char *line) ;
static gboolean parseBodyLine(ZMapGFFParser parser, char *line) ;

static gboolean makeNewFeature(ZMapGFFParser parser, char *sequence, char *source,
			       ZMapFeatureType feature_type,
			       int start, int end, double score, ZMapStrand strand,
			       ZMapPhase phase, char *attributes, GArray *gaps) ;
static gboolean getFeatureName(char *sequence, char *attributes, ZMapFeatureType feature_type,
			       ZMapStrand strand, int start, int end, int query_start, int query_end,
			       char **feature_name, char **feature_name_id) ;
static gboolean getHomolAttrs(char *attributes, ZMapHomolType *homol_type_out,
			      int *start_out, int *end_out) ;
static gboolean formatType(gboolean SO_compliant, gboolean default_to_basic,
			   char *feature_type, ZMapFeatureType *type_out) ;
static gboolean formatScore(char *score_str, gdouble *score_out) ;
static gboolean formatStrand(char *strand_str, ZMapStrand *strand_out) ;
static gboolean formatPhase(char *phase_str, ZMapPhase *phase_out) ;
static void getFeatureArray(GQuark key_id, gpointer data, gpointer user_data) ;
static void destroyFeatureArray(gpointer data) ;
static void printSource(GQuark key_id, gpointer data, gpointer user_data) ;

static gboolean loadGaps(char *currentPos, GArray *gaps);


/* types is the list of methods/types, call it what you will that we want to see
 * in the output, we may need to filter the incoming data stream to get this.
 * 
 * If parse_only is TRUE the parser will parse the GFF and report errors but will
 * _not_ create any features. This means the parser can be tested/used on huge datasets
 * without having to have huge amounts of memory to hold the feature structs.
 * You can only set parse_only when you create the parser, it cannot be set later. */
ZMapGFFParser zMapGFFCreateParser(GList *sources, gboolean parse_only)
{
  ZMapGFFParser parser ;

  parser = g_new0(ZMapGFFParserStruct, 1) ;

  parser->state = ZMAPGFF_PARSE_HEADER ;
  parser->error = NULL ;
  parser->error_domain = g_quark_from_string(ZMAP_GFF_ERROR) ;
  parser->stop_on_error = FALSE ;
  parser->parse_only = parse_only ;

  parser->line_count = 0 ;
  parser->SO_compliant = FALSE ;
  parser->default_to_basic = FALSE ;

  parser->done_header = FALSE ;
  parser->done_version = FALSE ;
  parser->gff_version = -1 ;
  parser->done_source = FALSE ;
  parser->source_name = parser->source_version = NULL ;
  parser->done_sequence_region = FALSE ;
  parser->sequence_name = NULL ;
  parser->features_start = parser->features_end = 0 ;

  parser->sources = sources ;

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

  return parser ;
}




/* This function expects a null-terminated C string that contains a complete GFF line
 * (comment or non-comment line), the function expects the caller to already have removed the
 * newline char from the end of the GFF line.
 * 
 * Returns FALSE if there is any error in the GFF header.
 * Returns FALSE if there is an error in the GFF body and  stop_on_error == TRUE.
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


gboolean zMapGFFGetFeatures(ZMapGFFParser parser, ZMapFeatureBlock feature_block)
{
  gboolean result = FALSE ;

  if (parser->state != ZMAPGFF_PARSE_ERROR)
    {
      /* Actually we should only need to test feature_sets here really as there shouldn't be any
       * for parse_only.... */
      if (!parser->parse_only && parser->feature_sets)
	{
	  g_datalist_init(&(feature_block->feature_sets)) ;

	  g_datalist_foreach(&(parser->feature_sets), getFeatureArray,
			     &(feature_block->feature_sets)) ;


	  /* OK, THIS IS A HACK, REALLY THIS PARSER CODE SHOULD JUST USE THE FEATURE.H
	   * HEADER AND NOT DELVE INTO THE FEATURE BLOCK STUFF BY FOR NOW WE HAVE TO FIX
	   * UP ALL THE LINKS "BY HAND"..... */
	  g_datalist_foreach(&(feature_block->feature_sets), setBlock,
			     feature_block) ;


	}

      result = TRUE ;
    }

  return result ;
}



/* HACKED CODE TO FIX UP LINKS IN BLOCK->SET->FEATURE */
void setBlock(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)user_data ;

  feature_set->parent_block = feature_block ;

  g_datalist_foreach(&(feature_set->features), setSet, feature_set) ;

  return ;
}

void setSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  ZMapFeatureSet feature_set = (ZMapFeatureSet)user_data ;

  feature->parent_set = feature_set ;

  return ;
}

/* END OF HACKED CODE TO FIX UP LINKS IN BLOCK->SET->FEATURE */




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
    phase_str[GFF_MAX_FIELD_CHARS + 1] = {'\0'},
    attributes[GFF_MAX_FREETEXT_CHARS + 1] = {'\0'}, comments[GFF_MAX_FREETEXT_CHARS + 1] = {'\0'} ;
  int start = 0, end = 0 ;
  double score = 0 ;
  char *format_str = "%50s%50s%50s%d%d%50s%50s%50s %1000[^#] %1000c" ; 
  char *format_str_gaps = "%50s%50s%50s%d%d%50s%50s%50s %n" ; 
  int fields, charsRead, attsLen ;
  char *attsPos, *gapsPos;
  GArray *gaps = NULL;


  gapsPos = strstr(line, " Gaps ");
  if (gapsPos == NULL)
    {
      fields = sscanf(line, format_str,
		      &sequence[0], &source[0], &feature_type[0],
		      &start, &end, &score_str[0], &strand_str[0], &phase_str[0],
		      &attributes[0], &comments[0]);
    }
  else
    {
      fields = sscanf(line, format_str_gaps,
		      &sequence[0], &source[0], &feature_type[0],
		      &start, &end, &score_str[0], &strand_str[0], &phase_str[0],
		      &charsRead);

      /* The hard bit here is to distinguish the attributes field from any following
       * gaps pairs, so for now I'm just saying copy from where the sscanf ended
       * up to the Gaps tag, then go and do the gaps. */
      gaps = g_array_new(FALSE, FALSE, sizeof(ZMapAlignBlockStruct));
      attsPos = line + charsRead;
      attsLen = gapsPos - attsPos;
      strncpy(attributes, attsPos, attsLen);
      
      result = loadGaps(gapsPos, gaps);
    }

  if (result == TRUE 
      && (fields  < GFF_MANDATORY_FIELDS
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
      ZMapFeatureType type ;
      ZMapStrand strand ;
      ZMapPhase phase ;
      char *err_text = NULL ;

      /* I'm afraid I'm not doing assembly stuff at the moment, its not worth it....if I need
       * to change this decision I can just this section.....
       * Code just silently drops these lines.
       *  */
      if (g_ascii_strcasecmp(source, "assembly_tag") == 0)
	{
	  return TRUE ;
	}

      /* Check we could get the basic GFF fields.... */
      if (strlen(sequence) == GFF_MAX_FREETEXT_CHARS)
	err_text = g_strdup_printf("sequence name too long: %s", sequence) ;
      else if (strlen(source) == GFF_MAX_FREETEXT_CHARS)
	err_text = g_strdup_printf("source name too long: %s", source) ;
      else if (strlen(feature_type) == GFF_MAX_FREETEXT_CHARS)
	err_text = g_strdup_printf("feature_type name too long: %s", feature_type) ;
      else if (!formatType(parser->SO_compliant, parser->default_to_basic, feature_type, &type))
	err_text = g_strdup_printf("feature_type not recognised: %s", feature_type) ;
      else if (!formatScore(score_str, &score))
	err_text = g_strdup_printf("score format not recognised: %s", score_str) ;
      else if (!formatStrand(strand_str, &strand))
	err_text = g_strdup_printf("strand format not recognised: %s", strand_str) ;
      else if (!formatPhase(phase_str, &phase))
	err_text = g_strdup_printf("phase format not recognised: %s", phase_str) ;
      else
	{
	  GQuark source_id ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  /* debugging.... */
	  g_datalist_foreach(&(parser->sources), printSource, NULL) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  source_id = zMapStyleCreateID(source) ;

	  if (parser->sources && !(zMapFindStyle(parser->sources, source_id)))
	    err_text = g_strdup_printf("features with this source were not requested: %s",
				       source) ;
	}

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
	  result = makeNewFeature(parser, sequence, source, type,
				  start, end, score, strand, phase,
				  attributes, gaps) ;
	}
    }

  return result ;
}



/* This reads any gaps which are present on the gff line.
 * They are preceded by a Gaps tag, and are presented as
 * space-delimited groups of 4, consecutive groups being
 * comma-delimited. gapsPos is wherever we are in the gff
 * and is set to NULL when strstr can't find another comma.
 * fields must be 4 for a gap so either way we drop out
 * of the loop at the end. */
static gboolean loadGaps(char *gapsPos, GArray *gaps)
{
  ZMapAlignBlockStruct gap;
  char *gaps_format_str = "%d%d%d%d," ; 
  int fields, i;
  gboolean status = TRUE, valid = TRUE;

  gapsPos += 7;  /* skip over Gaps tag */

  while (status == TRUE && valid == TRUE)
    {
      fields = sscanf(gapsPos, gaps_format_str, &gap.q1, &gap.q2, &gap.t1, &gap.t2);
      if (fields == 4)
	{
	  gaps = g_array_append_val(gaps, gap);
	  if ((gapsPos = strstr(gapsPos, ",")) != NULL)
	    gapsPos++;
	  else
	    status = FALSE;    /* no more commas means we're at the end */
	}
      else
	valid = FALSE;  /* anything other than 4 is not a gap */
    }

  return valid;
}



static void printSource(GQuark key_id, gpointer data, gpointer user_data)
{


  printf("source id: %d, name: %s\n", key_id, g_quark_to_string(key_id)) ;

  return ;
}




static gboolean makeNewFeature(ZMapGFFParser parser, char *sequence, char *source,
			       ZMapFeatureType feature_type,
			       int start, int end, double score, ZMapStrand strand,
			       ZMapPhase phase, char *attributes, GArray *gaps)
{
  gboolean result = FALSE ;
  char *feature_name_id = NULL, *feature_name = NULL ;
  ZMapFeature feature = NULL ;
  ZMapGFFParserFeatureSet feature_set = NULL ; 
  gboolean feature_has_name ;
  GQuark style_id ;
  ZMapFeature new_feature ;
  ZMapHomolType homol_type ;
  int query_start = 0, query_end = 0 ;


  /* We require additional information from the attributes for some types. */
  if (feature_type == ZMAPFEATURE_HOMOL)
    {
      /* if this fails, what do we do...should just log the error I think..... */
      result = getHomolAttrs(attributes, &homol_type, &query_start, &query_end) ;
    }


  /* Get the feature name which may not be unique and a feature "id" which _must_
   * be unique. */
  feature_has_name = getFeatureName(sequence, attributes, feature_type, strand,
				    start, end, query_start, query_end,
				    &feature_name, &feature_name_id) ;


  /* Check if the "source" for this feature is already known, if it is then check if there
   * is already a multiline feature with the same name as we will need to augment it with this data. */
  if (!parser->parse_only &&
      (feature_set = (ZMapGFFParserFeatureSet)g_datalist_get_data(&(parser->feature_sets), source)))
    {
      feature = (ZMapFeature)g_datalist_get_data(&(feature_set->multiline_features), feature_name_id) ;
    }


  if (parser->parse_only || !feature)
    {
      new_feature = zmapFeatureCreateEmpty() ;
    }


  /* FOR PARSE ONLY WE WOULD LIKE TO CONTINUE TO USE THE LOCAL VARIABLE new_feature....SORT THIS
   * OUT............. */

  if (parser->parse_only)
    {
      feature = new_feature ;
    }
  else if (!feature)
    {
      /* If we haven't got a feature then create one, then add the feature to its source if that exists,
       * otherwise we have to create a list for the source and then add that to the list of sources
       *...ugh.  */

      /* If we don't have this feature_set yet, then make one. */
      if (!feature_set)
	{
	  feature_set = g_new0(ZMapGFFParserFeatureSetStruct, 1) ;

	  g_datalist_set_data_full(&(parser->feature_sets), source, feature_set, destroyFeatureArray) ;
	  
	  feature_set->original_id = g_quark_from_string(source) ;
	  feature_set->unique_id = zMapStyleCreateID(source) ;
	  feature_set->multiline_features = NULL ;
	  g_datalist_init(&(feature_set->multiline_features)) ;

	  g_datalist_init(&(feature_set->features)) ;

	  feature_set->parser = parser ;		    /* We need parser flags in the destroy
							       function for the feature_set list. */
	}


      /* Always add every new feature to the final set.... */
      g_datalist_set_data(&(feature_set->features), feature_name_id, new_feature) ;

      feature = new_feature ;

      /* THIS PIECE OF CODE WILL NEED TO BE CHANGED AS I DO MORE TYPES..... */
      /* If the feature is one that must be built up from several GFF lines then add it to
       * our set of such features. There are arcane/adhoc rules in action here, any features
       * that do not have their own feature_name  _cannot_  be multiline features as such features
       * can _only_ be identified if they do have their own name. */
      if (feature_has_name
	  && (feature_type == ZMAPFEATURE_SEQUENCE || feature_type == ZMAPFEATURE_TRANSCRIPT
	      || feature_type == ZMAPFEATURE_EXON || feature_type == ZMAPFEATURE_INTRON))
	{
	  g_datalist_set_data(&(feature_set->multiline_features), feature_name_id, feature) ;
	}
    }



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* style not kept in feature any more.... */

  /* we need to give it proper unique style name... */
  style_id = zMapStyleCreateID(source) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
 
  result = zmapFeatureAugmentData(feature, feature_name_id, feature_name, sequence,
				  feature_type, start, end, score, strand,
				  phase, homol_type, query_start, query_end, gaps) ;

  g_free(feature_name) ;
  g_free(feature_name_id) ;


  /* If we are only parsing then free any stuff allocated by addDataToFeature() */
  if (parser->parse_only)
    {
      zmapFeatureDestroy(feature) ;
    }

  return result ;
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
 * For GFF v2 we must exclude the following types of attributes that are _not_ object names:
 *
 *        Note "Left: B0250"
 *        Note "7 copies of 31mer"
 * 
 * and so on....
 * 
 *  */
static gboolean getFeatureName(char *sequence, char *attributes, ZMapFeatureType feature_type,
			       ZMapStrand strand, int start, int end, int query_start, int query_end,
			       char **feature_name, char **feature_name_id)
{
  gboolean has_name = FALSE ;


  /* Probably we should do some checking to make sure start/end are in correct order....and
   * that other fields are correct.... */

  if (feature_type == ZMAPFEATURE_SEQUENCE || feature_type == ZMAPFEATURE_TRANSCRIPT
      || feature_type == ZMAPFEATURE_EXON || feature_type == ZMAPFEATURE_INTRON)
    {
      /* Named feature such as a gene. */
      int attr_fields ;
      char *attr_format_str = "%50s %*[\"]%50[^\"]%*[\"]%*s" ;
      char class[GFF_MAX_FIELD_CHARS + 1] = {'\0'}, name[GFF_MAX_FIELD_CHARS + 1] = {'\0'} ;

      attr_fields = sscanf(attributes, attr_format_str, &class[0], &name[0]) ;

      if (attr_fields == 2)
	{
	  has_name = TRUE ;
	  *feature_name = g_strdup(name) ;

	  *feature_name_id = g_strdup(*feature_name) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  /* don't do this or it messes up completely for the above multi-part features. */

	  *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
						   start, end, query_start, query_end) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	}

    }
  else if (feature_type == ZMAPFEATURE_HOMOL)
    {
      /* In acedb output at least, homologies all have the same format. */
      int attr_fields ;
      char *attr_format_str = "Target %*[\"]%*[^:]%*[:]%50[^\"]%*[\"]%*s" ;
      char name[GFF_MAX_FIELD_CHARS + 1] = {'\0'} ;

      attr_fields = sscanf(attributes, attr_format_str, &name[0]) ;

      if (attr_fields == 1)
	{
	  has_name = FALSE ;
	  *feature_name = g_strdup(name) ;
	  *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
						   start, end, query_start, query_end) ;
	}
    }
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




  return has_name ;
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
			      int *start_out, int *end_out)
{
  gboolean result = FALSE ;
  int attr_fields ;
  char *attr_format_str = "%*s %*[\"]%50[^\"]%*s%d%d" ;
  char homol_type[GFF_MAX_FIELD_CHARS + 1] = {'\0'} ;
  int start = 0, end = 0 ;

  if ((attr_fields = sscanf(attributes, attr_format_str, &homol_type[0], &start, &end)) == 3)
    {
      if (g_ascii_strncasecmp(homol_type, "Sequence:", 9) == 0)
	*homol_type_out = ZMAPHOMOL_N_HOMOL ;
      else if (g_ascii_strncasecmp(homol_type, "Protein:", 8) == 0)
	*homol_type_out = ZMAPHOMOL_X_HOMOL ;
      /* or what.....these seem to the only possibilities for acedb gff output. */

      *start_out = start ;
      *end_out = end ;

      result = TRUE ;
    }

  return result ;
}





/* Type must equal something that the code understands. In GFF v1 this is unregulated and
 * could be anything. By default unrecognised terms are not converted.
 * 
 * 
 * SO_compliant == TRUE   only recognised SO terms will be accepted for feature types.
 * SO_compliant == FALSE  both SO and the earlier more adhoc names will be accepted (source for
 *                        these terms is wormbase GFF dumps).
 * 
 * This option is only valid when SO_compliant == FALSE.
 * misc_default == TRUE   unrecognised terms will be returned as "misc_feature" features types.
 * misc_default == FALSE  unrecognised terms will not be converted.
 * 
 * 
 *  */
gboolean formatType(gboolean SO_compliant, gboolean default_to_basic,
		    char *feature_type, ZMapFeatureType *type_out)
{
  gboolean result = FALSE ;
  ZMapFeatureType type = ZMAPFEATURE_INVALID ;


  /* Is feature_type a SO term, note that I do case-independent compares, hope this is correct. */
  if (g_ascii_strcasecmp(feature_type, "trans_splice_acceptor_site") == 0)
    {
      type = ZMAPFEATURE_BOUNDARY ;
    }
  else if (g_ascii_strcasecmp(feature_type, "transposable_element_insertion_site") == 0
	   || g_ascii_strcasecmp(feature_type, "deletion") == 0)
    {
      type = ZMAPFEATURE_VARIATION ;
    }
  if (g_ascii_strcasecmp(feature_type, "region") == 0)
    {
      type = ZMAPFEATURE_BASIC ;
    }
  else if (g_ascii_strcasecmp(feature_type, "virtual_sequence") == 0)
    {
      type = ZMAPFEATURE_SEQUENCE ;
    }
  else if (g_ascii_strcasecmp(feature_type, "reagent") == 0
	   || g_ascii_strcasecmp(feature_type, "oligo") == 0
	   || g_ascii_strcasecmp(feature_type, "PCR_product") == 0
	   || g_ascii_strcasecmp(feature_type, "RNAi_reagent") == 0
	   || g_ascii_strcasecmp(feature_type, "clone") == 0
	   || g_ascii_strcasecmp(feature_type, "clone_end") == 0
	   || g_ascii_strcasecmp(feature_type, "clone_end") == 0)
    {
      type = ZMAPFEATURE_BASIC ;
    }
  else if (g_ascii_strcasecmp(feature_type, "UTR") == 0
	   || g_ascii_strcasecmp(feature_type, "polyA_signal_sequence") == 0
	   || g_ascii_strcasecmp(feature_type, "polyA_site") == 0)
    {
      /* these should in the end be part of a gene structure..... */
      type = ZMAPFEATURE_BASIC ;
    }
  else if (g_ascii_strcasecmp(feature_type, "operon") == 0)
    {
      /* this should in the end be part of a gene group, not sure how we display that..... */
      type = ZMAPFEATURE_BASIC ;
    }
  else if (g_ascii_strcasecmp(feature_type, "pseudogene") == 0)
    {
      /* In SO terms this is a region but we don't have a basic "region" type that includes
       * exons like structure...suggests we need to remodel our feature struct.... */
      type = ZMAPFEATURE_TRANSCRIPT ;
    }
  else if (g_ascii_strcasecmp(feature_type, "experimental_result_region") == 0
	   || g_ascii_strcasecmp(feature_type, "chromosomal_structural_element") == 0)
    {
      type = ZMAPFEATURE_BASIC ;
    }
  else if (g_ascii_strcasecmp(feature_type, "transcript") == 0
	   || g_ascii_strcasecmp(feature_type, "protein_coding_primary_transcript") == 0
	   || g_ascii_strcasecmp(feature_type, "CDS") == 0
	   || g_ascii_strcasecmp(feature_type, "mRNA") == 0
	   || g_ascii_strcasecmp(feature_type, "nc_primary_transcript") == 0)
    {
      type = ZMAPFEATURE_TRANSCRIPT ;
    }
  else if (g_ascii_strcasecmp(feature_type, "exon") == 0)
    {
      type = ZMAPFEATURE_EXON ;
    }
  else if (g_ascii_strcasecmp(feature_type, "intron") == 0)
    {
      type = ZMAPFEATURE_INTRON ;
    }
  else if (g_ascii_strcasecmp(feature_type, "nucleotide_match") == 0
	   || g_ascii_strcasecmp(feature_type, "expressed_sequence_match") == 0
	   || g_ascii_strcasecmp(feature_type, "EST_match") == 0
	   || g_ascii_strcasecmp(feature_type, "cDNA_match") == 0
	   || g_ascii_strcasecmp(feature_type, "translated_nucleotide_match") == 0
	   || g_ascii_strcasecmp(feature_type, "protein_match") == 0)
    {
      type = ZMAPFEATURE_HOMOL ;
    }
  else if (g_ascii_strcasecmp(feature_type, "repeat_region") == 0
	   || g_ascii_strcasecmp(feature_type, "inverted_repeat") == 0
	   || g_ascii_strcasecmp(feature_type, "tandem_repeat") == 0
	   || g_ascii_strcasecmp(feature_type, "transposable_element") == 0)
    {
      type = ZMAPFEATURE_BASIC ;
    }
  else if (g_ascii_strcasecmp(feature_type, "SNP") == 0
	   || g_ascii_strcasecmp(feature_type, "sequence_variant") == 0
	   || g_ascii_strcasecmp(feature_type, "substitution") == 0)
    {
      type = ZMAPFEATURE_VARIATION ;
    }


  if (!SO_compliant)
    {
      if (g_ascii_strcasecmp(feature_type, "SL1_acceptor_site") == 0
	  || g_ascii_strcasecmp(feature_type, "SL2_acceptor_site") == 0)
	{
	  type = ZMAPFEATURE_BOUNDARY ;
	}
      else if (g_ascii_strcasecmp(feature_type, "Clone_right_end") == 0)
	{
	  type = ZMAPFEATURE_BASIC ;
	}
      else if (g_ascii_strcasecmp(feature_type, "Clone") == 0
	       || g_ascii_strcasecmp(feature_type, "Clone_left_end") == 0
	       || g_ascii_strcasecmp(feature_type, "utr") == 0
	       || g_ascii_strcasecmp(feature_type, "experimental") == 0
	       || g_ascii_strcasecmp(feature_type, "reagent") == 0
	       || g_ascii_strcasecmp(feature_type, "repeat") == 0
	       || g_ascii_strcasecmp(feature_type, "structural") == 0
	       || g_ascii_strcasecmp(feature_type, "misc_feature") == 0)
	{
	  type = ZMAPFEATURE_BASIC ;
	}
      else if (g_ascii_strcasecmp(feature_type, "Pseudogene") == 0)
	{
	  /* REALLY NOT SURE ABOUT THIS CLASSIFICATION......SHOULD IT BE A TRANSCRIPT ? */
	  type = ZMAPFEATURE_TRANSCRIPT ;
	}
      else if (g_ascii_strcasecmp(feature_type, "SNP") == 0
	       || g_ascii_strcasecmp(feature_type, "complex_change_in_nucleotide_sequence") == 0)
	{
	  type = ZMAPFEATURE_VARIATION ;
	}
      else if (g_ascii_strcasecmp(feature_type, "Sequence") == 0)
	{
	  type = ZMAPFEATURE_SEQUENCE ;
	}
      else if (g_ascii_strcasecmp(feature_type, "transcript") == 0
	       || g_ascii_strcasecmp(feature_type, "protein-coding_primary_transcript") == 0
	       || g_ascii_strcasecmp(feature_type, "miRNA_primary_transcript") == 0
	       || g_ascii_strcasecmp(feature_type, "snRNA_primary_transcript") == 0
	       || g_ascii_strcasecmp(feature_type, "snoRNA_primary_transcript") == 0
	       || g_ascii_strcasecmp(feature_type, "tRNA_primary_transcript") == 0
	       || g_ascii_strcasecmp(feature_type, "rRNA_primary_transcript") == 0)
	{
	  /* N.B. "protein-coding_primary_transcript" is a typo in wormDB currently, should
	   * be all underscores. */
	  type = ZMAPFEATURE_TRANSCRIPT ;
	}
      else if (g_ascii_strcasecmp(feature_type, "similarity") == 0
	       || g_ascii_strcasecmp(feature_type, "transcription") == 0)
	{
	  type = ZMAPFEATURE_HOMOL ;
	}
      else if (g_ascii_strcasecmp(feature_type, "trans-splice_acceptor") == 0)
	{
	  type = ZMAPFEATURE_BOUNDARY ;
	}
      else if (g_ascii_strcasecmp(feature_type, "coding_exon") == 0
	       || g_ascii_strcasecmp(feature_type, "exon") == 0)
	{
	  type = ZMAPFEATURE_EXON ;
	}
      else if (g_ascii_strcasecmp(feature_type, "intron") == 0)
	{
	  type = ZMAPFEATURE_INTRON ;
	}
      else if (default_to_basic)
	{
	  /* If we allow defaulting of unrecognised features, the default is a "basic" feature. */
	  type = ZMAPFEATURE_BASIC ;
	}
    }


  if (type != ZMAPFEATURE_INVALID)
    {
      result = TRUE ;
      *type_out = type ;
    }

  return result ;
}


/* Score must either be the char '.' or a valid floating point number, currently
 * if the score is '.' we return 0, this may not be correct. */
gboolean formatScore(char *score_str, gdouble *score_out)
{
  gboolean result = FALSE ;

  if (strlen(score_str) == 1 && strcmp(score_str, ".") == 0)
    {
      result = TRUE ;
      *score_out = 0 ;
    }
  else
    {
      gdouble score ;

      if ((result = zMapStr2Double(score_str, &score)))
	*score_out = score ;
    }

  return result ;
}
	

/* Strand must be one of the chars '+', '-' or '.'. */
gboolean formatStrand(char *strand_str, ZMapStrand *strand_out)
{
  gboolean result = FALSE ;

  if (strlen(strand_str) == 1
      && (*strand_str == '+' || *strand_str == '-' || *strand_str == '.'))
    {
      result = TRUE ;

      switch (*strand_str)
	{
	case '+':
	  *strand_out = ZMAPSTRAND_FORWARD ;
	  break ;
	case '-':
	  *strand_out = ZMAPSTRAND_REVERSE ;
	  break ;
	default:
	  *strand_out = ZMAPSTRAND_NONE ;
	  break ;
	}
    }

  return result ;
}
	

/* Phase must either be the char '.' or 0, 1 or 2. */
gboolean formatPhase(char *phase_str, ZMapPhase *phase_out)
{
  gboolean result = FALSE ;

  if (strlen(phase_str) == 1
      && (*phase_str == '.' == 0
	  || *phase_str == '0' == 0 || *phase_str == '1' == 0 || *phase_str == '2' == 0))
    {
      result = TRUE ;

      switch (*phase_str)
	{
	case '0':
	  *phase_out = ZMAPPHASE_0 ;
	  break ;
	case '1':
	  *phase_out = ZMAPPHASE_1 ;
	  break ;
	case '2':
	  *phase_out = ZMAPPHASE_2 ;
	  break ;
	default:
	  *phase_out = ZMAPPHASE_NONE ;
	  break ;
	}
    }

  return result ;
}



/* This is a GDataForeachFunc() and is called for each element of a GData list as a result
 * of a call to zmapGFFGetFeatures(). The function adds the feature array returned 
 * in the GData element to the GArray in user_data. */
static void getFeatureArray(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapGFFParserFeatureSet feature_set = (ZMapGFFParserFeatureSet)data ;
  GData **features = (GData **)user_data ;
  ZMapFeatureSet new_features ;

  new_features = zMapFeatureSetIDCreate(feature_set->original_id, feature_set->unique_id,
					feature_set->features) ;

  /* agh, poke in feature internals... */
  new_features->style_id = feature_set->unique_id ;

  g_datalist_id_set_data(features, new_features->unique_id, new_features) ;

  return ;
}


/* This is a GDestroyNotify() and is called for each element in a GData list when
 * the list is cleared with g_datalist_clear (), the function must free the GArray
 * and GData lists. */
static void destroyFeatureArray(gpointer data)
{
  ZMapGFFParserFeatureSet feature_set = (ZMapGFFParserFeatureSet)data ;

  /* No data to free in this list, just clear it. */
  g_datalist_clear(&(feature_set->multiline_features)) ;

  g_free(feature_set) ;

  return ;
}
