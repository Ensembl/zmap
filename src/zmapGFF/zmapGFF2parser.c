/*  Last edited: Jul 13 11:50 2011 (edgrif) */
/*  File: zmapGFF2parser.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: parser for GFF version 2 format.
 *
 * Exported functions: See ZMap/zmapGFF.h
 *-------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#include <ZMap/zmap.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapString.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapSO.h>
#include <zmapGFF_P.h>



typedef enum {NAME_FIND, NAME_USE_SOURCE, NAME_USE_SEQUENCE, NAME_USE_GIVEN, NAME_USE_GIVEN_OR_NAME } NameFindType ;



/* Some attributes look like this:    Tag "Value"
 *
 * use the following pattern in your sscanf() call to get Value:
 *
 *                  sscanf(ptr_to_tag, "Tag " VALUE_FORMAT_STR, rest of args) ;
 *  */
#define VALUE_FORMAT_STR "%*[\"]%50[^\"]%*[\"]%*s"


static gboolean parseHeaderLine(ZMapGFFParser parser, char *line) ;
static gboolean parseBodyLine(ZMapGFFParser parser, char *line, gsize line_length) ;
static gboolean parseSequenceLine(ZMapGFFParser parser, char *line) ;
static gboolean makeNewFeature(ZMapGFFParser parser, NameFindType name_find,
			       char *sequence, char *source, char *ontology,
			       ZMapStyleMode feature_type,
			       int start, int end,
			       gboolean has_score, double score,
			       ZMapStrand strand, ZMapPhase phase, char *attributes,
			       char **err_text) ;
static gboolean getFeatureName(NameFindType name_find, char *sequence, char *attributes,
			       char *given_name, char *source,
			       ZMapStyleMode feature_type,
			       ZMapStrand strand, int start, int end, int query_start, int query_end,
			       char **feature_name, char **feature_name_id) ;
static char *getURL(char *attributes) ;
static GQuark getClone(char *attributes) ;
static GQuark getLocus(char *attributes) ;
static gboolean getKnownName(char *attributes, char **known_name_out) ;
static gboolean getHomolLength(char *attributes, int *length_out) ;
static gboolean getHomolAttrs(char *attributes, ZMapHomolType *homol_type_out,
			      int *start_out, int *end_out, ZMapStrand *strand_out, double *percent_ID_out) ;
static gboolean getAssemblyPathAttrs(char *attributes, char **assembly_name_unused,
				     ZMapStrand *strand_out, int *length_out, GArray **path_out) ;
static gboolean getCDSStartAttr(char *attributes, gboolean *start_not_found_out, int *start_start_not_found_out) ;
static gboolean getCDSEndAttr(char *attributes, gboolean *end_not_found_out) ;
static gboolean getVariationString(char *attributes,
				   GQuark *SO_acc_out, char **name_str_out, char **variation_str_out) ;
static void getFeatureArray(GQuark key_id, gpointer data, gpointer user_data) ;
static void destroyFeatureArray(gpointer data) ;

static gboolean loadGaps(char *currentPos, GArray *gaps, ZMapStrand ref_strand, ZMapStrand match_strand) ;
static gboolean loadAlignString(ZMapGFFParser parser, char *attributes, GArray **gaps_out,
				ZMapStrand ref_strand, int ref_start, int ref_end,
				ZMapStrand match_strand, int match_start, int match_end) ;

static void mungeFeatureType(char *source, ZMapStyleMode *type_inout);
static gboolean getNameFromNote(char *attributes, char **name) ;
static char *getNoteText(char *attributes) ;
static gboolean resizeBuffers(ZMapGFFParser parser, gsize line_length) ;
static gboolean resizeFormatStrs(ZMapGFFParser parser) ;








/* Parser must be created with the reference sequence for which features are to be
 * parsed because files may contain features for several reference sequences and
 * we need to parse only those for our reference sequence.
 *
 * Optionally a feature start/end range may be specified, if start and end are 0 then
 * all features for the reference sequence are parsed from the gff.
 */
ZMapGFFParser zMapGFFCreateParser(char *sequence, int features_start, int features_end)
{
  ZMapGFFParser parser = NULL ;

  if ((sequence && *sequence)
      && ((features_start == 1 && features_end == 0) || (features_start > 0 && features_end >= features_start)))
    {
      parser = g_new0(ZMapGFFParserStruct, 1) ;

      parser->state = ZMAPGFF_PARSE_HEADER ;
      parser->error = NULL ;
      parser->error_domain = g_quark_from_string(ZMAP_GFF_ERROR) ;
      parser->stop_on_error = FALSE ;

      parser->line_count = 0 ;
      parser->SO_compliant = FALSE ;
      parser->default_to_basic = FALSE ;

      parser->clip_mode = GFF_CLIP_NONE ;
      parser->clip_start = parser->clip_end = 0 ;


      /* Some of these may also be derived from the file meta-data. */
      parser->header_flags.done_header = FALSE ;
      parser->header_flags.got_gff_version = FALSE ;
      parser->header_flags.got_sequence_region = FALSE ;

      parser->gff_version = GFF_DEFAULT_VERSION ;
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



/* We should do this internally with a var in the parser struct.... */
/* This function must be called prior to parsing feature lines, it is not required
 * for either the header lines or sequences. */
gboolean zMapGFFParserInitForFeatures(ZMapGFFParser parser, GHashTable *sources, gboolean parse_only)
{
  gboolean result = FALSE ;
  GQuark locus_id ;

  parser->sources = sources ;
  parser->parse_only = parse_only ;

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

  return result ;
}


/* Parses a single line of GFF data, should be called repeatedly with successive lines
 * of GFF data from a GFF source. This function expects the line to be a null-terminated
 * C string with any terminating newline char to already have been removed (this latter
 * because we don't know how the line is stored so don't want to write to it).
 *
 * This function expects to find the GFF header in the format below, once there are
 * no more header lines (i.e. a non-"##" lines is encountered) then the header is
 * considered to have be finished.
 *
 * The zMapGFFParseLine() function can then be used to parse the rest of the file.
 *
 *
 * GFFv2: no order of comment lines is given so the following lines are parsed
 * but may come in any order or may be completely absent:
 *
 * ##gff-version 2
 * ##sequence-region chr19-03 1000000 1010000
 *
 *
 * GFFv3: the version line _MUST_ be there and _MUST_ be the first file line,
 * after that lines can come in any order:
 *
 * ##gff-version 3
 * ##sequence-region seqid start end
 *
 *
 * Returns FALSE if there is any error in the GFF header.
 *
 * Once an error has been returned the parser object cannot be used anymore and
 * zMapGFFDestroyParser() should be called to free it.
 *
 */
gboolean zMapGFFParseHeader(ZMapGFFParser parser, char *line, gboolean *header_finished, gboolean *header_ok)
{
  gboolean result = FALSE ;

  zMapLogReturnValIfFail((parser && line && header_finished), FALSE) ;

  parser->line_count++ ;

  /* Look for the header information. */
  if (parser->state == ZMAPGFF_PARSE_HEADER)
    {
      if (zMapStringBlank(line))
	{
	  /* Remove blank lines...note: specs are uncertain about whether blank lines are legal. */
	  result = TRUE ;
	}
      else
	{
	  if ((result = parseHeaderLine(parser, line)))
	    {
	      /* Signal that last line was a header line so header not finished. */
	      if(parser->header_flags.got_sequence_region && parser->header_flags.got_gff_version)
	      	*header_ok = TRUE;
	      *header_finished = FALSE ;
	    }
	  else
	    {
	      /* Last line was not a header line so if there was no error then we assume
	       * the header is finished. */
	      if (parser->error)
		{
		  result = FALSE ;
		  parser->state = ZMAPGFF_PARSE_ERROR ;
		}
	      else
		{
		  /*  */
		  parser->header_flags.done_header = *header_finished = TRUE ;
	        if(parser->header_flags.got_sequence_region && parser->header_flags.got_gff_version)
	      	*header_ok = TRUE;

		  parser->state = ZMAPGFF_PARSE_BODY ;
		  result = TRUE ;
		}
	    }
	}
    }

  return result ;
}


/* Parses a single line of GFF data, should be called repeatedly with successive lines
 * of GFF data from a GFF source. This function expects the line to be a null-terminated
 * C string with any terminating newline char to already have been removed (this latter
 * because we don't know how the line is stored so don't want to write to it).
 *
 * This function expects to find sequence in the GFF format format below, once all the
 * sequence lines have been found or a non-comment line is found it will stop.
 * The zMapGFFParseLine() function can then be used to parse the rest of the file.
 *
 * Returns FALSE if there is any error in the GFF sequence.
 *
 * Once an error has been returned the parser object cannot be used anymore and
 * zMapGFFDestroyParser() should be called to free it.
 *
 * Returns TRUE in sequence_finished once all the sequence is parsed.
 */
gboolean zMapGFFParseSequence(ZMapGFFParser parser, char *line, gboolean *sequence_finished)
{
  gboolean result = FALSE ;

  zMapLogReturnValIfFail((parser && line && sequence_finished), FALSE) ;

  parser->line_count++ ;

  if (parser->state == ZMAPGFF_PARSE_BODY)
    parser->state = ZMAPGFF_PARSE_SEQUENCE ;

  if (parser->state == ZMAPGFF_PARSE_SEQUENCE)
    {
      if (zMapStringBlank(line))
	{
	  result = TRUE ;
	}
      if (!(result = parseSequenceLine(parser, line)))
	{
	  parser->state = ZMAPGFF_PARSE_BODY ;    // return FALSE, get ready for body section
	}
      else
	{
        result = TRUE;
	  if (parser->sequence_flags.done_finished)
	    {
	      parser->state = ZMAPGFF_PARSE_BODY ;
	      *sequence_finished = TRUE ;
	    }
	  else
	    {
	      *sequence_finished = FALSE ;
	    }
	}
    }

  return result ;
}



/* Convenience function, size 0 tells code to guess buffer size. */
gboolean zMapGFFParseLine(ZMapGFFParser parser, char *line)
{
  gboolean result = FALSE ;

  result = zMapGFFParseLineLength(parser, line, 0) ;

  return result ;
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
gboolean zMapGFFParseLineLength(ZMapGFFParser parser, char *line, gsize line_length)
{
  gboolean result = FALSE ;


  parser->line_count++ ;

  /* Look for the header information. */
  if (parser->state == ZMAPGFF_PARSE_HEADER)
    {
      if (!(result = parseHeaderLine(parser, line)))
	{
	  if (parser->error)
	    {
	      result = FALSE ;
	      parser->state = ZMAPGFF_PARSE_ERROR ;
	    }
	  else
	    {
	      /* If we found all the header parts move on to the body. */
	      parser->header_flags.done_header = TRUE ;
	      parser->state = ZMAPGFF_PARSE_BODY ;
	      result = TRUE ;
	    }
	}
    }

  /* Note can only be in parse body state if header correctly parsed. */
  if (parser->state == ZMAPGFF_PARSE_BODY)
    {
      if (g_str_has_prefix(line, "##DNA"))
	{
	  parser->state = ZMAPGFF_PARSE_SEQUENCE ;
	}
      else if (*(line) == '#')
	{
	  /* Skip over comment lines, this is a CRUDE test, probably need something more subtle. */
	  result = TRUE ;
	}
      else
	{
	  if (!(result = parseBodyLine(parser, line, line_length)))
	    {
	      if (parser->error && parser->stop_on_error)
		{
		  result = FALSE ;
		  parser->state = ZMAPGFF_PARSE_ERROR ;
		}
	    }
	}
    }


  if (parser->state == ZMAPGFF_PARSE_SEQUENCE)
    {
      if (!(result = parseSequenceLine(parser, line)))
	{
	  if (parser->error && parser->stop_on_error)
	    {
	      result = FALSE ;
	      parser->state = ZMAPGFF_PARSE_ERROR ;
	    }
	}
      else
	{
	  if (parser->sequence_flags.done_finished)
	    parser->state = ZMAPGFF_PARSE_BODY ;
	}
    }

  return result ;
}



/* WE NEED A NEW FUNC THAT PARSES A WHOLE STREAM.....AND HAS THESE RULES IN IT. */

/* Current code assumes that the header block will be a contiguous set of header lines
 * at the top of the file and that the first non-header line marks the beginning
 * of the GFF data. If this is not true then its an error. */





/* Returns as much information as possible from the header comments of the gff file.
 * Note that our current parsing code makes this an all or nothing piece of code:
 * either the whole header is there or nothing is.... */
ZMapGFFHeader zMapGFFGetHeader(ZMapGFFParser parser)
{
  ZMapGFFHeader header = NULL ;

	/* MH17: if we have a GFF file with header only we never get to set done header */
	/* on account of the structure of the code in use */
//if (parser->header_flags.done_header)
  if(parser->header_flags.got_gff_version && parser->header_flags.got_sequence_region)
    {
      header = g_new0(ZMapGFFHeaderStruct, 1) ;

      header->gff_version = parser->gff_version ;

      header->sequence_name = g_strdup(parser->sequence_name) ;
      header->features_start =  parser->features_start ;
      header->features_end = parser->features_end ;
    }

  return header ;
}

gboolean zMapGFFParserSetSequenceFlag(ZMapGFFParser parser)
{
  gboolean set = TRUE;

  parser->sequence_flags.done_start = FALSE;
  parser->sequence_flags.done_finished = FALSE;

  return set;
}

ZMapSequence zMapGFFGetSequence(ZMapGFFParser parser)
{
  ZMapSequence sequence = NULL;

  if (parser->header_flags.done_header)
    {
      if(parser->seq_data.type != ZMAPSEQUENCE_NONE
	 && (parser->seq_data.sequence != NULL && parser->raw_line_data == NULL))
	{
	  sequence = g_new0(ZMapSequenceStruct, 1);
	  *sequence = parser->seq_data;
	  sequence->name = g_quark_from_string(parser->sequence_name);

	  /* So we don't copy empty data */
	  parser->seq_data.type     = ZMAPSEQUENCE_NONE;
	  parser->seq_data.sequence = NULL; /* So it doesn't get free'd */
	}
    }

  return sequence;
}

void zMapGFFFreeHeader(ZMapGFFHeader header)
{
  zMapAssert(header) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  g_free(header->source_name) ;
  g_free(header->source_version) ;
  g_free(header->sequence_name) ;

  g_free(header) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}


/* Return the set of features read from a file for a block. */
gboolean zMapGFFGetFeatures(ZMapGFFParser parser, ZMapFeatureBlock feature_block)
{
  gboolean result = FALSE ;

  if (parser->state != ZMAPGFF_PARSE_ERROR)
    {
      int start,end;

      start = parser->features_start;
      end   = parser->features_end;

      if(parser->clip_mode)
      {
            if(start < parser->clip_start)
                  start = parser->clip_start;
            if(end > parser->clip_end)
                  end = parser->clip_end;
      }

#if MH17_NEED_ACTUAL_FEATURES_COORD_RANGE_FOR_EMPTY_FEATURESETS
/* MH17: this appears to rebase the features to the start of the sequence span :-( */
      if(!feature_block->block_to_sequence.block.x2)
#endif
      {
            /* as request coordinates are often given as 1,0 we need to put real coordinates in */
            /* ideally chromosome coordinates would be better */

            /* NOTE we need to know the actual data returned as we
             *  mark empty featuresets as loaded over this range
             */
            feature_block->block_to_sequence.block.x1 = start;
            feature_block->block_to_sequence.block.x2 = end;
      }
      if(!feature_block->block_to_sequence.parent.x2)
      {
            /* as request coordinates are often given as 1,0 we need to put real coordinates in */
            /* ideally chromosome coordinates would be better */
            feature_block->block_to_sequence.parent.x1 = start;
            feature_block->block_to_sequence.parent.x2 = end;
      }

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


/* Optionally set mappings that are keys from the GFF source to feature set and style names. */
void zMapGFFParseSetSourceHash(ZMapGFFParser parser,
			       GHashTable *source_2_feature_set, GHashTable *source_2_sourcedata)
{
  parser->source_2_feature_set = source_2_feature_set ;

  parser->source_2_sourcedata = source_2_sourcedata ;


  /* Locus is an odd one out just now, we need to handle this differently..... */
  if (parser->locus_set_id)
    {
      ZMapFeatureSetDesc set_data ;
      ZMapFeatureSource source_data ;


      set_data = g_new0(ZMapFeatureSetDescStruct, 1) ;
      set_data->column_id = parser->locus_set_id ;
      set_data->feature_set_text = g_strdup_printf("Locus IDs") ;

      g_hash_table_insert(parser->source_2_feature_set,
			  GINT_TO_POINTER(parser->locus_set_id),
			  set_data) ;


      source_data = g_new0(ZMapFeatureSourceStruct, 1) ;
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
  {
      parser->clip_mode = clip_mode ;
  }

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


int zMapGFFParserGetNumFeatures(ZMapGFFParser parser)
{
	return(parser->num_features);
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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (parser->error)
    g_error_free(parser->error) ;

  if (parser->source_name)
    g_free(parser->source_name) ;
  if (parser->source_version)
    g_free(parser->source_version) ;

  if (parser->date)
    g_free(parser->date) ;

  if (parser->sequence_name)
    g_free(parser->sequence_name) ;

  /* The datalist uses a destroy routine that only destroys the feature arrays if the caller wants
   * us to, see destroyFeatureArray() */
  if (!parser->parse_only && parser->feature_sets)
    g_datalist_clear(&(parser->feature_sets)) ;

  g_string_free(parser->attributes_str, TRUE) ;
  g_string_free(parser->comments_str, TRUE) ;

  g_free(parser) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  if (parser->sequence_name)
    g_free(parser->sequence_name) ;

  g_free(parser) ;



  return ;
}






/*
 *                         Internal functions
 */



/* This function expects a null-terminated C string that contains a GFF header line
 * which is a special form of comment line starting with a "##" in column 1.
 *
 * Currently we parse gff-version and sequence-region comments as they
 * are common to gff v2 and v3.
 *
 * Returns FALSE if passed a line that is not a header comment OR if there
 * was a parse error. In the latter case parser->error will have been set.
 */
static gboolean parseHeaderLine(ZMapGFFParser parser, char *line)
{
  gboolean result = FALSE ;
  enum {FIELD_BUFFER_LEN = 1001} ;			    /* If you change this, change the
							       scanf's below... */

  if (parser->state == ZMAPGFF_PARSE_HEADER)
    {
      if (!g_str_has_prefix(line, "##"))
	{
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
	  if (g_str_has_prefix(line, "##gff-version") && !parser->header_flags.got_gff_version)
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
		  if (version != 2 && version != 3)
		    {
		      parser->error = g_error_new(parser->error_domain, ZMAP_GFF_ERROR_HEADER,
						  "Only GFF versions 2 or 3 supported, line %d: \"%s\"",
						  parser->line_count, line) ;
		      result = FALSE ;
		    }
		  else if (version == 3 && parser->line_count != 1)
		    {
		      parser->error = g_error_new(parser->error_domain, ZMAP_GFF_ERROR_HEADER,
						  "GFFv3 \"##gff-version\" must be first line in file, line %d: \"%s\"",
						  parser->line_count, line) ;
		      result = FALSE ;
		    }
		  else
		    {
		      parser->gff_version = version ;
		      parser->header_flags.got_gff_version = TRUE ;
		    }
		}
	    }
	  else if (g_str_has_prefix(line, "##sequence-region") && !parser->header_flags.got_sequence_region)
	    {
	      char sequence_name[FIELD_BUFFER_LEN] = {'\0'} ;
	      int start = 0, end = 0 ;

	      fields = 4 ;
	      format_str = "%*s%1000s%d%d" ;

	      if ((fields = sscanf(line, format_str, &sequence_name[0], &start, &end)) != 3)
		{
		  parser->error = g_error_new(parser->error_domain, ZMAP_GFF_ERROR_HEADER,
					      "Bad \"##sequence-region\" line %d: \"%s\"",
					      parser->line_count, line) ;
		  result = FALSE ;
		}
	      else
		{
		  /* Parser is created with sequence name and start/end for the whole view, it is an
		   * error if in the gff header the sequence name is different or the coords
		   * lie outside start/end. */

		  /* They may have done this to get the whole sequence.... */
		  if (start == 1 && end == 0)
		    {
		      start = parser->features_start ;
		      end = parser->features_end ;
		    }
		  if(parser->features_start == 1 && parser->features_end == 0)
		    {
		    	/* mh17 else if we read a file://  with no seq sopecified it fails */
		      parser->features_start = start ;
		      parser->features_end = end ;
		    }

		  if (g_ascii_strcasecmp(&sequence_name[0], parser->sequence_name) != 0
		      || start > parser->features_end
		      || end < parser->features_start)
		    {
		      parser->error = g_error_new(parser->error_domain, ZMAP_GFF_ERROR_HEADER,
						  "Mismatch between original sequence/start/end:"
						  " \"%s\" %d %d"
						  " and header \"##sequence-region\" line %d: \"%s\"",
						  parser->sequence_name,
						  parser->features_start, parser->features_end,
						  parser->line_count, line) ;
		      result = FALSE ;
		    }
		  else
		    {
		      parser->sequence_name = g_strdup(&sequence_name[0]) ;
		      parser->features_start = start ;
		      parser->features_end = end ;
		      parser->header_flags.got_sequence_region = TRUE ;
		      //zMapLogWarning("get gff header: %d-%d",start,end);
		    }
		  if(end < start)		/* includes 1,0 */
		    {
		      parser->error = g_error_new(parser->error_domain, ZMAP_GFF_ERROR_HEADER,
						  "Invalid sequence/start/end:"
						  " \"%s\" %d %d"
						  " in header \"##sequence-region\" line %d: \"%s\"",
						  parser->sequence_name,
						  start, end,
						  parser->line_count, line) ;
		    }

		}

	    }
	}
    }

  return result ;
}


/* This function expects a null-terminated C string that contains a GFF sequence line
 * which is a special form of comment line starting with a "##".
 *
 * GFF version 2 format for sequence lines is:
 *
 * ##DNA
 * ##CGGGCTTTCACCATGTTGGCCAGGCT...CTGCCCGCCTCGGCCTCCCA
 * ##end-DNA
 *
 * We only support DNA sequences, anything else is an error.
 *
 * Returns FALSE (and sets parse->error) if there was a parse error,
 * TRUE if not. parser->sequence_flags.done_finished is set to TRUE
 * when sequence parse has finished.
 */
static gboolean parseSequenceLine(ZMapGFFParser parser, char *line)
{
  gboolean result = FALSE ;
  enum {FIELD_BUFFER_LEN = 1001} ;			    /* If you change this, change the
							       scanf's below... */

  if (parser->state == ZMAPGFF_PARSE_SEQUENCE)
    {
      if (!parser->sequence_flags.done_finished && !g_str_has_prefix(line, "##"))
	{
	  /* If we encounter a non-sequence line and we haven't yet finished the sequence
	   * then its an error. */
	  result = FALSE ;

        if(parser->sequence_flags.done_start)
	    {
            // treat no sequence as syntactically correct. getSequence() can report the not there error
            parser->error = g_error_new(parser->error_domain, ZMAP_GFF_ERROR_HEADER,
				      "Bad ## line %d: \"%s\"",
				      parser->line_count, line) ;
          }
	}
      else
	{
	  result = TRUE ;

	  if (g_str_has_prefix(line, "##DNA") && !parser->sequence_flags.done_start)
	    {
	      parser->sequence_flags.done_start = TRUE ;
	      parser->seq_data.type = ZMAPSEQUENCE_DNA ;
	    }
	  else if (g_str_has_prefix(line, "##end-DNA") && parser->sequence_flags.in_sequence_block)
	    {
	      if ((parser->features_end - parser->features_start + 1) != parser->raw_line_data->len)
		{
		  parser->error = g_error_new(parser->error_domain, ZMAP_GFF_ERROR_HEADER,
					      "##sequence-region length [%d] does not match DNA base count"
					      " [%" G_GSSIZE_FORMAT "].",
					      (parser->features_end - parser->features_start + 1),
					      parser->raw_line_data->len);

		  g_string_free(parser->raw_line_data, TRUE) ;
		  parser->raw_line_data = NULL ;
              parser->sequence_flags.done_finished = TRUE ;
		}
	      else
		{
		  parser->seq_data.length = parser->raw_line_data->len ;
		  parser->seq_data.sequence = g_string_free(parser->raw_line_data, FALSE) ;
		  parser->raw_line_data = NULL ;
		  parser->sequence_flags.done_finished = TRUE ;
		}
	    }
	  else if (g_str_has_prefix(line, "##")
		   && (parser->sequence_flags.done_start || parser->sequence_flags.in_sequence_block))
	    {
	      char *line_ptr = line ;

	      parser->sequence_flags.in_sequence_block = TRUE ;

	      /* must be sequence */
	      line_ptr+=2 ;					    /* move past ## */
	      /* save the string */
	      g_string_append(parser->raw_line_data, line_ptr) ;
	    }
	}
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
 *
 * For ZMap, we've modified the acedb gff dumper to output homology alignments after the
 * attributes, marked by a tag " Gaps ".  They're in groups of 4 space-separated coordinates,
 * successive groups being comma-separated.
 *
 * If there's a Gaps tag, we scanf using a different format string, then copy the attributes
 * manually, then call the loadGaps function to load the alignments.
 *
 * NOTE to handle BAM data that includes # inside attribute strings we take attributes and comments as one section
 * (comments are not processed anywhere)
 * Then we scan attributes and split this on an unquoted # and put the reamainder into commments
 *
 */

#define QUOTED_HASH_KILLS_ATTRIBUTES	0	/* set to 1 for previous code */

static gboolean parseBodyLine(ZMapGFFParser parser, char *line, gsize line_length)
{
  gboolean result = TRUE ;
  char *sequence, *source, *feature_type, *score_str, *strand_str, *phase_str, *attributes, *comments ;
  int start = 0, end = 0, fields = 0 ;
  double score = 0 ;
  ZMapStyleMode type ;
  ZMapStrand strand ;
  ZMapPhase phase ;
  gboolean has_score = FALSE ;
  char *err_text = NULL ;


  /* If line_length increases then increase the length of the buffers that receive text so that
   * they cannot overflow and redo the format string. */
  if (!line_length)
    line_length = strlen(line) ;

  if (line_length > parser->buffer_length)
    {
      resizeBuffers(parser, line_length) ;

      resizeFormatStrs(parser) ;

      zMapLogWarning("GFF parser buffers had to be resized to new line length: %d", parser->buffer_length) ;
    }

  /* These vars just for legibility. */
  sequence = (char *)(parser->buffers[GFF_BUF_SEQUENCE]) ;
  source = (char *)(parser->buffers[GFF_BUF_SOURCE]) ;
  feature_type = (char *)(parser->buffers[GFF_BUF_FEATURE_TYPE]) ;
  score_str = (char *)(parser->buffers[GFF_BUF_SCORE]) ;
  strand_str = (char *)(parser->buffers[GFF_BUF_STRAND]) ;
  phase_str = (char *)(parser->buffers[GFF_BUF_PHASE]) ;
  attributes = (char *)(parser->buffers[GFF_BUF_ATTRIBUTES]) ;
  comments = (char *)(parser->buffers[GFF_BUF_COMMENTS]) ;	/* this is not used */


  /* Parse a GFF line. */
  if ((fields = sscanf(line, parser->format_str,
		       sequence, source, feature_type,
		       &start, &end,
		       score_str, strand_str, phase_str,
		       attributes, comments)) < GFF_MANDATORY_FIELDS)

    {
      /* Not enough fields. */
      parser->error = g_error_new(parser->error_domain, ZMAP_GFF_ERROR_BODY,
				  "GFF line %d - Mandatory fields missing in: \"%s\"",
				  parser->line_count, line) ;
      result = FALSE ;
    }
  else
    {
#if !QUOTED_HASH_KILLS_ATTRIBUTES
      {
      	/* remove data after an unquoted hash
      	   not sure if this is necessary but lets play safe
      	   can just throw comments away, they are not used
      	 */
      	 char *p;
      	 int quoted = 0;

      	 for(p = attributes;*p;p++)
      	 {
      	 	if(*p == '"')
      	 		quoted = !quoted;
      	 	if(!quoted && *p == '#')
      	 	{
      	 		*p = 0;
      	 		comments = ++p;
      	 		break;
      	 	}
      	 }
      }
#endif
      /* Do some sanity checking... */
      if (g_ascii_strcasecmp(sequence, ".") == 0)
	err_text = g_strdup("sequence cannot be '.'") ;
      else if ((g_ascii_strcasecmp(source, ".") == 0)
	       || (g_ascii_strcasecmp(feature_type, ".") == 0))
	err_text = g_strdup("source and type cannot be '.'") ;
      else if (!zMapFeatureFormatType(parser->SO_compliant, parser->default_to_basic,
				      feature_type, &type))
	err_text = g_strdup_printf("feature_type not recognised: %s", feature_type) ;
      else if (start > end)
	err_text = g_strdup_printf("start > end, start = %d, end = %d", start, end) ;
      else if (!zMapFeatureFormatScore(score_str, &has_score, &score))
	err_text = g_strdup_printf("score format not recognised: %s", score_str) ;
      else if (!zMapFeatureFormatStrand(strand_str, &strand))
	err_text = g_strdup_printf("strand format not recognised: %s", strand_str) ;
      else if (!zMapFeatureFormatPhase(phase_str, &phase))
	err_text = g_strdup_printf("phase format not recognised: %s", phase_str) ;

      if (err_text)
	{
	  parser->error = g_error_new(parser->error_domain, ZMAP_GFF_ERROR_BODY,
				      "GFF line %d (a)- %s (\"%s\")",
				      parser->line_count, err_text, line) ;
	  g_free(err_text) ;
	  result = FALSE ;
 	}
    }

  if (result)
    {
      if (g_ascii_strcasecmp(source, "assembly_tag") == 0)
	{
	  /* I'm afraid I'm not doing assembly stuff at the moment, its not worth it....if I need
	   * to change this decision I can just this remove this section.....
	   * Code just silently drops these lines.
	   *  */
	  result = TRUE ;
	}
      else if (g_ascii_strcasecmp(sequence, parser->sequence_name) != 0)
	{
	  /* File may contain feature lines for other sequences, drop them. */
	  result = TRUE ;
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
//zMapLogWarning("clip: %d %d %d (%d %d)",parser->clip_mode,start,end,parser->clip_start,parser->clip_end);
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
					      "GFF line %d (b) - %s (\"%s\")",
					      parser->line_count, err_text, line) ;
		  g_free(err_text) ;
		}


	      /* If the Locus feature set has been requested then check to see if this feature
	       * has a locus and add it to the locus feature set. Note this is different in that
	       * locus entries are not exported as a separate features but as part of another
	       * feature.
	       *
	       * We may wish to revisit this and have locus items as separate features but its
	       * not straight forward to export them from acedb _and_ get their locations. */
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
						  "GFF line %d (c) - %s (\"%s\")",
						  parser->line_count, err_text, line) ;
		      g_free(err_text) ;
		    }
		}
	    }
	}
    }

  return result ;
}


#define BAM_ALIGNMENT_CIGAR 1

#if BAM_ALIGNMENT_CIGAR
/* temp bit of code to read in a gap string as provided for ENCODE data
 * of the form M23 N12234 M53
 *
 * NOTE that the two M's add up to 76 but this is not always the case
 * NOTE also that we tend to get one gap only but more are logically possible
 * and we need to cope
 * we only process M and N tags (or I or D instead of N if that's what anacode end up sending us)
 *
 * this is a deliberately dumb but simple function
 * if we get unexpected stuff like N88 M76 or M12 N12 N23 M29 then we handle it silently
 */

/*NOTE James did the string like this:
 * 23M1234N43M
 * ... which is different
 *
 * target is ref, query is match; see commant above loadGaps()
 */

static gboolean loadGapString(ZMapGFFParser parser,
				char *attributes, GArray **gaps_out,
				ZMapStrand ref_strand, int ref_start, int ref_end,
				ZMapStrand match_strand)
{
	char *p = attributes;
	int match_start = 1;

	/* skip over <cigar "> */
	while(*p && *p != '"' ) p++;
	if(!*p++)
		return FALSE;

	while (*p && *p != '"')
	{
		int bases = atoi(p);

		while(*p && *p <= ' ')
			p++;
		while(*p >= '0' && *p <= '9')
			p++;
		while(*p && *p <= ' ')
			p++;

		if(*p == 'M')
		{
			/* add a block */
			ZMapAlignBlockStruct ab = { 0 };

			/* Gap coords are positive, 1-based, start < end " So: q2 is off the end of the match */

			ab.q1 = match_start;
			ab.q2 = match_start + bases - 1;
			ab.q_strand = match_strand;
			ab.t1 = ref_start;
			ab.t2 = ref_start + bases - 1;
			ab.t_strand = ref_strand;

			g_array_append_val(*gaps_out,ab);

			/* skip to end of block */
			ref_start += bases;
			match_start += bases;
		}
		else if (*p == 'N')
		{
			/* skip to start of next block */
			ref_start += bases;
		}
		else
		{
			return FALSE;
		}
		p++;
	}

	return TRUE;
}


 #endif



/*
 * we can get tags in quoted strings, and maybe ';' too
 * i'm assuming that quotes cannot appear in quoted strings even with '\'
 */

 char *find_tag(char * str, char *tag)
 {
	char *p = str;
	int len = strlen(tag);
	int n_quote;

	while(*p)
	{
		if(!g_ascii_strncasecmp(p,tag,len))
		{
			p += len;
			while(*p == ' ' || *p == '\t')
				p++;
			return(p);
		}

		for(n_quote = 0;*p;p++)
		{
			if(*p == '"')
				n_quote++;
			if(*p == ';' && !(n_quote & 1))
				break;
		}
		while(*p == ';' || *p == ' ' || *p == '\t')
			p++;

	}
	return(NULL);
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
  ZMapFeatureTypeStyle  feature_style = NULL;
  ZMapFeature feature = NULL ;
  ZMapGFFParserFeatureSet parser_feature_set = NULL ;
  char *feature_set_name = NULL ;
/*  GQuark column_id = 0;*/
  gboolean feature_has_name ;
  ZMapFeature new_feature ;
  ZMapHomolType homol_type ;
  int query_start = 0, query_end = 0, query_length = 0 ;
  ZMapStrand query_strand ;
  double percent_id = 0 ;
  ZMapSpanStruct exon = {0}, *exon_ptr = NULL, intron = {0}, *intron_ptr = NULL ;
  char *url ;
  GQuark locus_id = 0 ;
  GArray *gaps = NULL;
  GArray *path = NULL ;
  char *gaps_onwards = NULL;
  char *note_text, *source_text = NULL ;
  GQuark clone_id = 0, source_id = 0 ;
  GQuark SO_acc = 0 ;
  char *name_string = NULL, *variation_string = NULL ;

  /* If the parser was given a source -> data mapping then
   * use that to get the style id and other
   * data otherwise use the source itself.
   */
  if (parser->source_2_sourcedata)
    {
      ZMapFeatureSource source_data ;


      if (!(source_data = g_hash_table_lookup(parser->source_2_sourcedata,
					      GINT_TO_POINTER(zMapFeatureSetCreateID(source)))))
	//                                    GINT_TO_POINTER(g_quark_from_string(source)))))
	{
	  *err_text = g_strdup_printf("feature ignored, could not find data for source \"%s\".", source) ;
	  result = FALSE ;

	  return result ;
	}
      else
	{
	  feature_style_id = zMapStyleCreateID((char *)g_quark_to_string(source_data->style_id)) ;

	  source_id = source_data->source_id ;
	  source_text = (char *)g_quark_to_string(source_data->source_text) ;
	}
    }
  else
    {
      source_id = feature_style_id = zMapStyleCreateID(source) ;
    }


  /* If the parser was given a source -> column group mapping then use that as the feature set
   * otherwise use the source itself. */
#if MH17_MAP_TO_COLUMN
  /*
   * to process featuresets (data sources) before displaying (or afterwards)
   * we need to have featuresets as separate data items
   * combining them here confused the model/ data with the view/ display layout
   * so it breaks MVC and also ISO 7-layer protocols
   * we do the mapping in zmapWindowDrawFeatures() instead.
   */
  if (parser->source_2_feature_set)
    {
      ZMapFeatureSetDesc set_data ;

      if (!(set_data = g_hash_table_lookup(parser->source_2_feature_set,
					   GINT_TO_POINTER(zMapFeatureSetCreateID(source)))))

	{
	  *err_text = g_strdup_printf("feature ignored, could not find column for source \"%s\".", source) ;
	  result = FALSE ;

	  return result ;
	}
      else
      {
            // feature_set_id being the column id... */
        feature_set_name = (char *)g_quark_to_string(set_data->feature_set_id) ;
/*        column_id = set_data->feature_set_id;*/
      }
    }
  else
    {
      feature_set_name = source ;
 /*     column_id = zMapStyleCreateID(source);*/
    }
#else
  /* don't map to column but instead make a note of the column id for later */
  /* this turns put to be needed for FToI hash functions */

    feature_set_name = source ;
/*
    column_id = zMapStyleCreateID(source);

  if (parser->source_2_feature_set)
    {
      ZMapFeatureSetDesc set_data ;

      if ((set_data = g_hash_table_lookup(parser->source_2_feature_set,
                                 GINT_TO_POINTER(zMapFeatureSetCreateID(source)))))
      {
        column_id = set_data->column_id;
      }
    }
*/
#endif

  // get the feature set so that we can find the style for the feature;
  parser_feature_set = (ZMapGFFParserFeatureSet)g_datalist_get_data(&(parser->feature_sets),
								    feature_set_name);
  /* If we don't have this feature_set yet, then make one. */
  if (!parser_feature_set)
    {
      ZMapSpan span;

      parser_feature_set = g_new0(ZMapGFFParserFeatureSetStruct, 1) ;

      g_datalist_set_data_full(&(parser->feature_sets),
			       feature_set_name, parser_feature_set, destroyFeatureArray) ;

      feature_set = parser_feature_set->feature_set = zMapFeatureSetCreate(feature_set_name , NULL) ;

      /* record the region we are getting */
      span = (ZMapSpan) g_new0(ZMapSpanStruct,1);
      span->x1 = parser->features_start;
      span->x2 = parser->features_end;
      feature_set->loaded = g_list_append(NULL,span);

//zMapLogWarning("gff span %s %d -> %d",feature_set_name,span->x1,span->x2);

      parser->src_feature_sets =
	g_list_prepend(parser->src_feature_sets,GUINT_TO_POINTER(feature_set->unique_id));

      // we need to copy as these may be re-used.
      // styles have already been inherited by this point by zmapView code and passed back to us
      parser_feature_set->feature_styles = g_hash_table_new(NULL,NULL);

/*      feature_set->column_id = column_id;*/

      parser_feature_set->multiline_features = NULL ;
      g_datalist_init(&(parser_feature_set->multiline_features)) ;

      parser_feature_set->parser = parser ;             /* We need parser flags in the destroy
							   function for the feature_set list. */
    }

  /*printf("GFF: src %s, style %s\n",source,g_quark_to_string(feature_style_id)); */


  if(!(feature_style = (ZMapFeatureTypeStyle)
       g_hash_table_lookup(parser_feature_set->feature_styles,GUINT_TO_POINTER(feature_style_id))))
    {
      if(!(feature_style = zMapFindStyle(parser->sources, feature_style_id)))
        {
          *err_text = g_strdup_printf("feature ignored, could not find style \"%s\" for feature set \"%s\".",
				      g_quark_to_string(feature_style_id), feature_set_name) ;
	  result = FALSE ;

          return result ;
        }

      g_hash_table_insert(parser_feature_set->feature_styles,GUINT_TO_POINTER(feature_style_id),(gpointer) feature_style);
      /* printf("using feature style %s @%p for %s\n",g_quark_to_string(feature_style->unique_id),feature_style, feature_set_name);*/

    }

  /* with one type of feature in a featureset this should be ok */
  parser_feature_set->feature_set->style = feature_style;

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

      if (!(result = getHomolAttrs(attributes,
				   &homol_type,
				   &query_start, &query_end, &query_strand,
				   &percent_id)))
	{
	  *err_text = g_strdup_printf("feature ignored, could not get Homol attrs") ;

	  return result ;
	}
      else
	{
	  result = getHomolLength(attributes, &query_length) ; /* Not fatal to not have length. */
	}
    }
  else if (feature_type == ZMAPSTYLE_MODE_ASSEMBLY_PATH)
    {
      if (!(result = getAssemblyPathAttrs(attributes, NULL, &query_strand, &query_length, &path)))
	{
	  *err_text = g_strdup_printf("feature ignored, could not get AssemblyPath attrs");

	  return result ;
	}
    }
  else if (feature_type == ZMAPSTYLE_MODE_BASIC)
    {
      if (g_ascii_strcasecmp(source, "ensembl_variation") == 0)
	{
	  /* For variation we need to parse the SO type and string out of the Name attribute. */
	  if (!(result = getVariationString(attributes, &SO_acc, &name_string, &variation_string)))
	    {
	      *err_text = g_strdup_printf("feature ignored, could not parse variation string");

	      return result ;
	    }
	}
	else
      {
	      name_find = NAME_USE_GIVEN_OR_NAME ;
	}
    }


  /* Was there a url for the feature ? */
  url = getURL(attributes) ;


  /* Was there a locus for the feature ? */
  locus_id = getLocus(attributes) ;


  /* Often text is attached to a feature as a "Note", retrieve this data if present. */
  note_text = getNoteText(attributes) ;


  /* Get the feature name which may not be unique and a feature "id" which _must_
   * be unique. */
  feature_has_name = getFeatureName(name_find, sequence, attributes,
				    name_string, source,
				    feature_type, strand,
				    start, end, query_start, query_end,
				    &feature_name, &feature_name_id) ;

  /* Check if the feature name for this feature is already known, if it is then check if there
   * is already a multiline feature with the same name as we will need to augment it with this data. */
  if (!parser->parse_only) // && parser_feature_set)
    {
      feature_set = parser_feature_set->feature_set ;

      if(feature_name)  /* have to check in case of no-name data errors */
            feature = (ZMapFeature)g_datalist_get_data(&(parser_feature_set->multiline_features),
						 feature_name) ;
    }


  if (parser->parse_only || !feature)
    {
      new_feature = zMapFeatureCreateEmpty() ;
      parser->num_features++;
    }


  /* FOR PARSE ONLY WE WOULD LIKE TO CONTINUE TO USE THE LOCAL VARIABLE new_feature....SORT THIS
   * OUT............. */

  if (parser->parse_only)
    {
      feature = new_feature ;
    }
  else if (!feature)
    {
      /* If we haven't got an existing feature then fill one in and add it to its feature set
       * if that exists, otherwise we have to create a list for the feature set and then
       * add that to the list of sources...ugh.  */

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

  if (!feature_name_id)
    {
      *err_text = g_strdup_printf("feature ignored as it has no name");
    }
  else if ((result = zMapFeatureAddStandardData(feature, feature_name_id, feature_name,
					   sequence, ontology,
					   feature_type, feature_style,
					   start, end,
					   has_score, score,
					   strand)))
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

	  /* Note that exons/introns are given one per line in GFF which is quite annoying.....it is
	   * out of sync with how homols with gaps are given.... */
	  if (g_ascii_strcasecmp(ontology, "coding_exon") == 0 || g_ascii_strcasecmp(ontology, "exon") == 0)
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


	  if (result)
	    {
	      gboolean start_not_found_flag = FALSE, end_not_found_flag = FALSE ;
	      int start_not_found = 0 ;

	      if (getCDSStartAttr(attributes, &start_not_found_flag, &start_not_found)
		  || getCDSEndAttr(attributes, &end_not_found_flag))
		{
		  result = zMapFeatureAddTranscriptStartEnd(feature,
							    start_not_found_flag, start_not_found,
							    end_not_found_flag) ;
		}
	    }

	  if (result && (exon_ptr || intron_ptr))
	    result = zMapFeatureAddTranscriptExonIntron(feature, exon_ptr, intron_ptr) ;
	}
      else if (feature_type == ZMAPSTYLE_MODE_ALIGNMENT)
	{


	  char *local_sequence_str ;
	  char *seq_str;
	  gboolean local_sequence = FALSE ;

	  /* I am not sure if we ever have target_phase from GFF output....check this out... "*/
	  if (zMapStyleIsParseGaps(feature_style))
	    {
	      static char *gaps_tag = ZMAPSTYLE_ALIGNMENT_GAPS " " ;
	      enum {GAP_STR_LEN = 5} ;

	      if ((gaps_onwards = strstr(attributes, gaps_tag)) != NULL)
		{
		  gaps = g_array_new(FALSE, FALSE, sizeof(ZMapAlignBlockStruct));
		  gaps_onwards += GAP_STR_LEN ;  /* skip over Gaps tag and parse "1 12 12 122, ..." incl "" not
						    terminated */

		  if (!loadGaps(gaps_onwards, gaps, strand, query_strand))
		    {
		      g_array_free(gaps, TRUE) ;
		      gaps = NULL ;
		    }
		}
	      else if ((gaps_onwards = strstr(attributes, ZMAPSTYLE_ALIGNMENT_CIGAR " ")))
		{
#if BAM_ALIGNMENT_CIGAR
#warning temporary code to get BAM alignments working, to be replaced by 'proper' cigar string handling
//		  if (!g_ascii_strcasecmp(ontology, "read"))
		  {
			gaps = g_array_new(FALSE, FALSE, sizeof(ZMapAlignBlockStruct));

			  if(!loadGapString(parser,
					 gaps_onwards, &gaps,
				       strand, start, end,
				       query_strand))
			  {
				zMapLogWarning("Could not parse cigar string: %s", gaps_onwards) ;
				g_array_free(gaps, TRUE) ;
				gaps = NULL ;
			  }
			  else if (gaps->len < 2)	/* ie is ungapped */
			  {
  				g_array_free(gaps, TRUE) ;
				gaps = NULL ;

			  }
		  }
//		  else
#else
		  if (!loadAlignString(parser,
				       gaps_onwards, &gaps,
				       strand, start, end,
				       query_strand, query_start, query_end))
		    {
		      zMapLogWarning("Could not parse align string: %s", gaps_onwards) ;
		    }
#endif
		}
	    }

	  if ((local_sequence_str = strstr(attributes, "Own_Sequence TRUE")))
	    {
	      local_sequence = TRUE ;
	    }

		/* own sequence means ACEDB has it; legacy data/code. sequence is given in GFF, so ZMap must store */
	  if((seq_str = find_tag(attributes,"sequence")))
	  {
		  char *p;

		  for(p = seq_str; *p > ';'; p++)
			  continue;

		  seq_str = g_strdup_printf("%.*s",p - seq_str, seq_str);
		  for(p = seq_str; *p > ';'; )
			  *p++ |= 0x20;	/* need to be lower case else rev comp gives zeroes */
	  }

	  result = zMapFeatureAddAlignmentData(feature, clone_id,
					       percent_id,
					       query_start, query_end,
					       homol_type, query_length, query_strand, ZMAPPHASE_0,
					       gaps,
					       zMapStyleGetWithinAlignError(feature_style),
					       local_sequence, seq_str) ;
	}
      else if (feature_type == ZMAPSTYLE_MODE_ASSEMBLY_PATH)
	{
	  result = zMapFeatureAddAssemblyPathData(feature, query_length, query_strand, path) ;
	}
      else
	{
	  if (variation_string)
	    {
	      if (!(result = zMapFeatureAddVariationString(feature, variation_string)))
		*err_text = g_strdup_printf("Bad format for variation_string \"%s\".", variation_string) ;
	    }

	  if (g_ascii_strcasecmp(source, "genomic_canonical") == 0)
	    {
	      char *known_name = NULL ;

	      result = TRUE;

	      if ((getKnownName(attributes, &known_name)))
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


      /* If the SO accession was not added by any of the above functions try to fudge it from
       * the source or type of the feature. */
      if (!SO_acc)
	{
	  SO_acc = zMapSOFeature2SO(feature) ;
	}

      if (SO_acc)
	{
	  if (!(result = zMapFeatureAddSOaccession(feature, SO_acc)))
	    *err_text = g_strdup_printf("Could not add SO accession \"%d\".", SO_acc) ;
	}

    }

  g_free(name_string) ;
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


/* servers have a list of columns in/out as provided by ACEDB and later used by pipes
 * here we privide a list of (single) featuresets as put into the context
 */
GList *zMapGFFGetFeaturesets(ZMapGFFParser parser)
{
  return (parser->src_feature_sets);
}




/* This reads any gaps which are present on the gff line. They are preceded by a Gaps tag, and are
 * presented as space-delimited groups of 4, consecutive groups being comma-delimited. gapsPos is
 * wherever we are in the gff and is set to NULL when strstr can't find another comma. fields must
 * be 4 for a gap so either way we drop out of the loop at the end. i.e. gaps string should be this
 * format:
 *
 *                             "34758 34799 531 544,34734 34751 545 550"
 *
 * Gap coords are positive, 1-based, start < end and in the order: ref_start ref_end match_start match_end
 */
static gboolean loadGaps(char *attributes, GArray *gaps, ZMapStrand ref_strand, ZMapStrand match_strand)
{
  gboolean valid = TRUE ;
  char *attr_str = attributes ;
  ZMapAlignBlockStruct gap = { 0 };
  char *gaps_format_str = "%d%d%d%d" ;
  int fields;

  /* Check rather rigidly that we are at start of number string. */
  if (((attr_str = strstr(attr_str, "\"")) != NULL) && g_ascii_isdigit(*(attr_str + 1)))
    {
      do
	{
	  attr_str++ ;					    /* Skip the leading '"' or ',' */

	  /* We should be looking at "number number number number , ....more stuff....." */
	  if ((fields = sscanf(attr_str, gaps_format_str, &gap.t1, &gap.t2, &gap.q1, &gap.q2)) == 4)
	    {
	      if (gap.q1 < 1 || gap.q2 < 1 || gap.t1 < 1 || gap.t2 < 1 || gap.q1 > gap.q2 || gap.t1 > gap.t2)
		{
		  valid = FALSE ;
		  break ;
		}
	      else
		{
		  gap.t_strand = ref_strand ;
		  gap.q_strand = match_strand ;

		  gaps = g_array_append_val(gaps, gap) ;
		}
	    }
	  else
	    {
	      /* anything other than 4 is not a gap */
	      valid = FALSE ;

	      break ;
	    }

	  /* Skip to ',' ending this set of numbers or to '"' marking end of Gap string. */
	  do
	    {
	      attr_str++ ;
	    }
	  while (*attr_str != ',' && *attr_str != '"') ;

	}
      while (*attr_str != '"') ;
    }

  return valid ;
}



/* This comment used to be identical to the one above loadGaps() directly above this function
 * which may be worth referring to
 * but this function fills in gaps data from a cigar or vulgar string
 */
static gboolean loadAlignString(ZMapGFFParser parser,
				char *attributes, GArray **gaps_out,
				ZMapStrand ref_strand, int ref_start, int ref_end,
				ZMapStrand match_strand, int match_start, int match_end)
{
  gboolean valid = FALSE ;
  int attr_fields ;

  if ((attr_fields = sscanf(attributes, parser->cigar_string_format_str, parser->buffers[GFF_BUF_TMP])) == 1)
    {
      valid = zMapFeatureAlignmentString2Gaps(ref_strand, ref_start, ref_end,
					      match_strand, match_start, match_end,
					      (char *)(parser->buffers[GFF_BUF_TMP]), gaps_out) ;
    }

  return valid ;
}



/* This routine attempts to find the feature's name from the GFF attributes field,
 * acedb has been modified so that with -zmap option it will output the name as:
 *
 *          Name "object name"    e.g.   Name "Sequence B0250.1"
 *
 * Some features do not have an obvious name though or the feature name is shared
 * amongst many features and so we must construct a name from the feature name
 * and the coords.
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
			       char *given_name, char *source,
			       ZMapStyleMode feature_type,
			       ZMapStrand strand, int start, int end, int query_start, int query_end,
			       char **feature_name, char **feature_name_id)
{
  gboolean has_name = FALSE ;
  char *tag_pos ;

  /* Probably we should do some checking to make sure start/end are in correct order....and
   * that other fields are correct....errr...shouldn't that already have been done ???? */

  /* REVISIT THESE TWO, NAME_USE_SOURCE ISN'T EVEN USED ANYWHERE..... */
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
  else if ((name_find == NAME_USE_GIVEN || name_find == NAME_USE_GIVEN_OR_NAME) && given_name && *given_name)
    {
      has_name = TRUE ;

      *feature_name = g_strdup(given_name) ;
      *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
					       start, end, query_start, query_end) ;
    }
  else if ((tag_pos = strstr(attributes, "Name")))
    {
      /* Parse out "Name <objname> ;" */
      int attr_fields ;
      char name[GFF_MAX_FIELD_CHARS + 1] = {'\0'} ;

      attr_fields = sscanf(tag_pos, "Name " VALUE_FORMAT_STR, &name[0]) ;

      if (attr_fields == 1)
	{
	  if(name[0])     /* das_WashU_PASA_human_ESTs have Name "" */
	    {
	      has_name         = TRUE ;
	      *feature_name    = g_strdup(name) ;
	      *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
						       start, end, query_start, query_end) ;
	    }
	}
    }
  else if(name_find != NAME_USE_GIVEN_OR_NAME)	/* chicken: for BAM we have a basic feature with name so let's dobge this in */
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
 *   Class "Protein" ; Name "BP:CBP01448" ; Align 157 197 + ; percentID 100 ; [Gaps "Qstart Qend Tstart Tend, ..."]
 *
 * Name has already been extracted, this routine looks at "Class" to get the
 * type of homol and at "Align" to get the match sequence coords/strand.
 *
 *
 * BAM data looks like this:
	chr14-04    encode    read    20781639    20781714    .    -    .    Target "JOHNLENNON_0006:1:61:1280:6420#0 1 76 -";sequence "CAAGAACACCAGACTGTGCAATCATGGATGGTTCAAGGGTGCCTTCATGGTTAGCAATAGTGATGTTTCGTAGCCT";Gap="M76";Name="JOHNLENNON_0006:1:61:1280:6420#0"
 * but is being recoded as an align with Gap called Cigar
 *
 * or via the bam_get_align script:
 *
 chr10-10        Tier2_HepG2_cytosol_longPolyA_rep2      similarity      50944554        50945894        .       -       .
       Name "MICHAELJACKSON_0008:2:20:8069:5465#0" ; cigar "13M1265N63M" ; Align "1 76 -" ; Length "76" ; percentID "100"
 *
 */
static gboolean getHomolAttrs(char *attributes, ZMapHomolType *homol_type_out,
			      int *start_out, int *end_out, ZMapStrand *query_strand,
			      double *percent_ID_out)
{
  gboolean result = FALSE ;
  char *tag_pos ;
  ZMapHomolType homol_type = ZMAPHOMOL_NONE ;
  int attr_fields ;

  if ((tag_pos = strstr(attributes, "Class")))
    {
      char homol_type_str[GFF_MAX_FIELD_CHARS + 1] = {'\0'} ;

      attr_fields = sscanf(tag_pos, "Class " VALUE_FORMAT_STR, &homol_type_str[0]) ;

      if (attr_fields == 1)
	{
	  /* NOTE that we assume that Motifs are dna but some are not...need a fix from
	   * otterlace in the end.... */

	  if (g_ascii_strncasecmp(homol_type_str, "Sequence", 8) == 0)
	    homol_type = ZMAPHOMOL_N_HOMOL ;
	  else if (g_ascii_strncasecmp(homol_type_str, "Protein", 7) == 0)
	    homol_type = ZMAPHOMOL_X_HOMOL ;
	  else if (g_ascii_strncasecmp(homol_type_str, "Motif", 5) == 0)
	    homol_type = ZMAPHOMOL_N_HOMOL ;
	  else if (g_ascii_strncasecmp(homol_type_str, "Mass_spec_peptide", 5) == 0)
	    homol_type = ZMAPHOMOL_X_HOMOL ;
	  else if (g_ascii_strncasecmp(homol_type_str, "SAGE_tag", 5) == 0)
	    homol_type = ZMAPHOMOL_N_HOMOL ;
	}
      else
	{
	  zMapLogWarning("Bad homol type: %s", tag_pos) ;
	}
    }

  if (homol_type)
    {
      if ((tag_pos = strstr(attributes, "percentID")))
	{
	  /* Parse "percentID 89.3" */
	  char *attr_format_str = "%*s %lg" ;
	  double percent_ID = 0 ;

	  if ((attr_fields = sscanf(tag_pos, attr_format_str, &percent_ID)) == 1)
	    {
	      if (percent_ID > 0)
		{
		  *percent_ID_out = percent_ID ;
		  result = TRUE ;
		}
	      else
		{
		  zMapLogWarning("Bad homol percent ID: %s", tag_pos) ;
		}
	    }
	  else
	    {
	      zMapLogWarning("Could not parse Homol Data: %s", tag_pos) ;
	    }
	}

      if ((tag_pos = strstr(attributes, "Align")))
	{
	  /* Parse "Align 157 197 +" */
	  char *attr_format_str = "%*s %d%d %c" ;
	  int start = 0, end = 0 ;
	  char strand ;

	  if ((attr_fields = sscanf(tag_pos, attr_format_str, &start, &end, &strand)) == 3)
	    {
	      if (start > 0 && end > 0)
		{
		  *homol_type_out = homol_type ;

		  /* Always export as start < end */
		  if (start <= end)
		    {
		      *start_out = start ;
		      *end_out = end ;
		    }
		  else
		    {
		      *start_out = end ;
		      *end_out = start ;
		    }



		  if (strand == '+')
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
 * Format of Start_not_found attribute section is:
 *
 *      ............  [; start_not_found position]  .............
 *
 * Format string extracts position for start_not_found
 *
 *  */
static gboolean getCDSStartAttr(char *attributes, gboolean *start_not_found_flag_out, int *start_not_found_out)
{
  gboolean result = FALSE ;
  char *target ;

  if ((target = strstr(attributes, "start_not_found")))
    {
      int attr_fields ;
      char *attr_format_str = "%*s %d %*s" ;
      int start_not_found = 0 ;

      attr_fields = sscanf(target, attr_format_str, &start_not_found) ;

      if (attr_fields == 1)
	{
	  if (start_not_found >= 1 && start_not_found <= 3)
	    {
	      *start_not_found_flag_out = TRUE ;

	      *start_not_found_out = start_not_found ;

	      result = TRUE ;
	    }
	}
    }

  return result ;
}



static gboolean getCDSEndAttr(char *attributes, gboolean *end_not_found_flag_out)
{
  gboolean result = FALSE ;
  char *target ;

  if ((target = strstr(attributes, "end_not_found")))
    {
      *end_not_found_flag_out = TRUE ;
      result = TRUE ;
    }

  return result ;
}



/* Format of Length attribute section is:
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




/* Format of Variation string is like this:
 *
 *   SNP:
 *
 *   Name "rs28847272 - A/G"
 *
 *
 *   deletion:
 *
 *   Name "rs35621402 - -/G"
 *   Name "rs3047232 - -/AA"
 *   Name "rs57043843 - -/CTTA"
 *   Name "rs33945458 - -/(LARGEINSERTION)"
 *
 *
 *   insertion:
 *
 *   Name "rs35022483 - T/-"
 *   Name "rs61033774 - TAAA/-"
 *   Name "rs58131816 - AAAAAAGTTCCTTGCATGATTAAAAAAGTATT/-"
 *
 *
 *   CNV:
 *
 *   Name "cnvi0023136 - CNV_PROBE"    I'm not completely sure about this.
 *
 * Format string extracts the variation string.
 *
 *  */
static gboolean getVariationString(char *attributes,
				   GQuark *SO_acc_out, char **name_str_out, char **variation_str_out)
{
  gboolean result = FALSE ;
  char *target ;

  if ((target = strstr(attributes, "Name")))
    {
      int attr_fields ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      char *attr_format_str = "Name " "%*[\"]%*s - %50[^\"]%*[\"]%*s" ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      char *attr_format_str = "Name " "%*[\"]%s - %50[^\"]%*[\"]%*s" ;

      char name_str[GFF_MAX_FIELD_CHARS + 1] = {'\0'} ;
      char variation_str[GFF_MAX_FIELD_CHARS + 1] = {'\0'} ;

      if ((attr_fields = sscanf(target, attr_format_str, &name_str[0], &variation_str[0])) == 2)
	{
	  char *cp = &variation_str[0] ;
	  GQuark SO_acc ;

	  if ((SO_acc = zMapSOVariation2SO(cp)))
	    {
	      *SO_acc_out = SO_acc ;
	      *name_str_out = g_strdup(&name_str[0]) ;
	      *variation_str_out = g_strdup(&variation_str[0]) ;
	      result = TRUE ;
	    }
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
  if(parser_feature_set->feature_set && parser_feature_set->feature_set->style)
      zMapStyleDestroy(parser_feature_set->feature_set->style);

  g_hash_table_destroy(parser_feature_set->feature_styles);       // styles remain pointed at by features

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



/* Given a line length, will allocate buffers so they cannot overflow when parsing a line of this
 * length. The rationale here is that we might get handed a line that had just one field in it
 * that was the length of the line. By allocating buffers that are the line length we cannot
 * overrun our buffers even in this bizarre case !!
 *
 * Note that we attempt to avoid frequent reallocation by making buffer twice as large as required
 * (not including the final null char....).
 */
static gboolean resizeBuffers(ZMapGFFParser parser, gsize line_length)
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
static gboolean resizeFormatStrs(ZMapGFFParser parser)
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

  /* this is what I'm trying to get:  "cigar %*[\"]%50[^\"]%*[\"]%*s" which parses a string
   * like this:
   *
   *          "cigar "M335ID55M"
   *  */
  align_format_str = ZMAPSTYLE_ALIGNMENT_CIGAR " "  "%%*[\"]" "%%%d" "[^\"]%%*[\"]%%*s" ;
  g_string_append_printf(format_str,
			 align_format_str,
			 length) ;


  parser->cigar_string_format_str = g_string_free(format_str, FALSE) ;


  return resized ;
}

