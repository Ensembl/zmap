/*  File: zmapGFFGeneral.c
 *  Author: Steve Miller (sm23@sanger.ac.uk)
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk,
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description: This file contains material that is used by both
 * the v2 and v3 parsers. This consists of some (but not all) of the
 * functions publically exported as listed in src/include/ZMap/zmapGFF.h.
 *-------------------------------------------------------------------
 */


#include <ZMap/zmapGFF.h>
#include <ZMap/zmapFeatureLoadDisplay.h>

#include "zmapGFF_P.h"
#include "zmapGFF2_P.h"
#include "zmapGFF3_P.h"


/*
 * Check whether the parser passed in is a valid version (i.e. one that we handle)
 * or not. Returns TRUE if this is the case and FALSE otherwise.
 */
gboolean zMapGFFIsValidVersion(const ZMapGFFParser const pParser)
{
  gboolean bResult = FALSE ;

  if (!pParser)
  {

  }
  else if (pParser->gff_version == ZMAPGFF_VERSION_2)
  {
    bResult = TRUE ;
  }
  else if (pParser->gff_version == ZMAPGFF_VERSION_3)
  {
    bResult = TRUE ;
  }

  return bResult ;
}



/*
 * Internal static function declarations.
 */
static void getFeatureArray(GQuark key_id, gpointer data, gpointer user_data) ;
static void normaliseFeatures(GData **feature_sets) ;
static void checkFeatureSetCB(GQuark key_id, gpointer data, gpointer user_data_unused) ;
static void checkFeatureCB(GQuark key_id, gpointer data, gpointer user_data_unused) ;


/*
 * Public interface parser creation function. Calls version specific functions. Returns
 * NULL if we are passed unsupported version.
 */
ZMapGFFParser zMapGFFCreateParser(int iGFFVersion, char *sequence, int features_start, int features_end)
{
  ZMapGFFParser pParser = NULL ;

  if (iGFFVersion == ZMAPGFF_VERSION_2 )
  {
    pParser = zMapGFFCreateParser_V2(sequence, features_start, features_end) ;
  }
  else if (iGFFVersion == ZMAPGFF_VERSION_3 )
  {
    pParser = zMapGFFCreateParser_V3(sequence, features_start, features_end) ;
  }
  else
  {
    /* unknown version */
  }

  return pParser ;
}



/*
 * Public interface function to parse headers. Calls version specific functions.
 */
gboolean zMapGFFParseHeader(ZMapGFFParser parser, char *line, gboolean *header_finished, ZMapGFFHeaderState *header_state)
{
  gboolean bResult = FALSE ;

  if (!parser)
    return bResult ;

  if (!zMapGFFIsValidVersion(parser))
    return bResult ;

  if (parser->gff_version == ZMAPGFF_VERSION_2 )
  {
    bResult = zMapGFFParseHeader_V2(parser, line, header_finished, header_state) ;
  }
  else if (parser->gff_version == ZMAPGFF_VERSION_3 )
  {
    bResult = zMapGFFParse_V3(parser, line) ;
    *header_finished = zMapGFFGetHeaderGotMinimal_V3(parser) ;
  }
  else
  {
    /* unknown version  */
  }

  return bResult ;
}


/*
 * Public interface function to parse a section of sequence. Calls version specific functions.
 */
gboolean zMapGFFParseSequence(ZMapGFFParser parser, char *line, gboolean *sequence_finished)
{
  gboolean bResult = FALSE ;

  if (!parser)
    return bResult ;

  if (!zMapGFFIsValidVersion(parser))
    return bResult ;

  if (parser->gff_version == ZMAPGFF_VERSION_2 )
  {
    bResult = zMapGFFParseSequence_V2(parser, line, sequence_finished) ;
  }
  else if (parser->gff_version == ZMAPGFF_VERSION_3 )
  {
    /*
     * This should not be required since it's not part of the v3 spec.
     */
    bResult = FALSE ;
  }
  else
  {
    /* unknown version */
  }

  return bResult ;
}




/*
 * Public interface function for the general body line parser. Calls version specific stuff.
 */
gboolean zMapGFFParseLine(ZMapGFFParser parser, char *line)
{
  gboolean bResult = FALSE ;

  if (!parser)
    return bResult ;

  if (!zMapGFFIsValidVersion(parser))
    return bResult ;

  if (parser->gff_version == ZMAPGFF_VERSION_2)
  {
    bResult = zMapGFFParseLineLength_V2(parser, line, 0) ;
  }
  else if (parser->gff_version == ZMAPGFF_VERSION_3 )
  {
    bResult = zMapGFFParse_V3(parser, line) ;
  }
  else
  {
    /* unknown version */
  }

  return bResult ;
}

/*
 * Public interface for general body line parser. Calls version specific stuff.
 */
gboolean zMapGFFParseLineLength(ZMapGFFParser parser, char *line, gsize line_length)
{
  gboolean bResult = FALSE ;

  if (!parser)
    return bResult ;

  if (!zMapGFFIsValidVersion(parser))
    return bResult ;

  if (parser->gff_version == ZMAPGFF_VERSION_2)
  {
    bResult = zMapGFFParseLineLength_V2(parser, line, 0) ;
  }
  else if (parser->gff_version == ZMAPGFF_VERSION_3 )
  {
    bResult = zMapGFFParse_V3(parser, line) ;
  }
  else
  {
    /* unknown version */
  }

  return bResult ;

}


/*
 * General code to destroy parser.
 */
void zMapGFFDestroyParser(ZMapGFFParser parser)
{
  if (!parser)
    return ;

  if (!zMapGFFIsValidVersion(parser))
    return ;

  if (parser->gff_version == ZMAPGFF_VERSION_2)
  {
    zMapGFFDestroyParser_V2(parser) ;
  }
  else if (parser->gff_version == ZMAPGFF_VERSION_3)
  {
    zMapGFFDestroyParser_V3(parser) ;
  }
  else
  {
    /* unknown version */
  }

  return ;
}







/* the servers need styles to add DNA and 3FT
 * they used to create temp style and then destroy these but that's not very good
 * they don't have styles info directly but this is stored in the parser
 * during the protocol steps
 */
GHashTable *zMapGFFParserGetStyles(ZMapGFFParser parser)
{
  if(!parser)
    return NULL ;

  if (!zMapGFFIsValidVersion(parser))
    return NULL ;

  return parser->sources;
}




/*
 * Used for both versions.
 *
 * We should do this internally with a var in the parser struct....
 * This function must be called prior to parsing feature lines, it is not required
 * for either the header lines or sequences.
 */
gboolean zMapGFFParserInitForFeatures(ZMapGFFParser parser, GHashTable *sources, gboolean parse_only)
{
  gboolean result = FALSE ;
  GQuark locus_id ;

  if (!parser)
    return result ;

  if (!zMapGFFIsValidVersion(parser))
    return result ;

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


/*
 * Used by both versions.
 *
 * Return the set of features read from a file for a block.
 */
gboolean zMapGFFGetFeatures(ZMapGFFParser parser, ZMapFeatureBlock feature_block)
{
  int start,end;
  gboolean result = FALSE ;

  if (!parser)
    return result ;
  if (!zMapGFFIsValidVersion(parser))
    return result ;

  if (parser->state != ZMAPGFF_PARSER_ERR)
  {
    start = parser->features_start;
    end   = parser->features_end;

    if (parser->clip_mode)
    {
      if(start < parser->clip_start)
        start = parser->clip_start;
      if(end > parser->clip_end)
       end = parser->clip_end;
    }


	    /* as request coordinates are often given as 1,0 we need to put real coordinates in */
	    /* ideally chromosome coordinates would be better */

	    /* NOTE we need to know the actual data returned as we
	     *  mark empty featuresets as loaded over this range
	     */
    feature_block->block_to_sequence.block.x1 = start;
    feature_block->block_to_sequence.block.x2 = end;

    if (!feature_block->block_to_sequence.parent.x2)
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
      g_datalist_foreach(&(parser->feature_sets), getFeatureArray, feature_block) ;
    }

    /* It is possible for features to be incomplete from GFF (e.g. transcripts without
     * an exon), we attempt to normalise them where possible...currently only
     * transcripts/exons. */
    normaliseFeatures(&(parser->feature_sets)) ;

    result = TRUE ;

  }

  return result ;
}

/*
 * Used by both versions.
 *
 * This is a GDataForeachFunc() and is called for each element of a GData list as a result
 * of a call to zmapGFFGetFeatures(). The function adds the feature array returned
 * in the GData element to the GArray in user_data.
 */
static void getFeatureArray(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapGFFParserFeatureSet parser_feature_set = (ZMapGFFParserFeatureSet)data ;
  ZMapFeatureSet feature_set = parser_feature_set->feature_set ;
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)user_data ;

  zMapFeatureBlockAddFeatureSet(feature_block, feature_set);

  return ;
}


/*
 * Used by both versions.
 *
 * Features may be created incomplete by 'faulty' GFF files, here we try
 * to make them valid.
 */
static void normaliseFeatures(GData **feature_sets)
{
  /* Look through all datasets.... */
  g_datalist_foreach(feature_sets, checkFeatureSetCB, NULL) ;

  return ;
}



/*
 * Used by both versions.
 *
 * A GDataForeachFunc() which checks to see if the dataset passed in has
 * mulitline features and if they are then checks each feature to make sure
 * it is complete/valid. Currently this is only transcripts...see checkFeatureCB().
 */
static void checkFeatureSetCB(GQuark key_id, gpointer data, gpointer user_data_unused)
{
  ZMapGFFParserFeatureSet parser_feature_set = (ZMapGFFParserFeatureSet)data ;

  if (parser_feature_set->multiline_features)
  {
    g_datalist_foreach(&(parser_feature_set->multiline_features), checkFeatureCB, NULL) ;
  }

  return ;
}


/*
 * Used by both versions.
 *
 * A GDataForeachFunc() which checks to see if the feature passed in is
 * complete/valid. Currently just checks transcripts to make sure they
 * have at least one exon.
 */
static void checkFeatureCB(GQuark key_id, gpointer data, gpointer user_data_unused)
{
  ZMapFeature feature = (ZMapFeature)data ;

  switch (feature->type)
  {
    case ZMAPSTYLE_MODE_TRANSCRIPT:
      zMapFeatureTranscriptNormalise(feature) ;
      break ;

    default:
      break ;
  }

  return ;
}



/*
 * Used by both versions.
 *
 * Optionally set mappings that are keys from the GFF source to feature set and style names.
 */
void zMapGFFParseSetSourceHash(ZMapGFFParser parser,
			       GHashTable *source_2_feature_set, GHashTable *source_2_sourcedata)
{
  if (!parser)
    return ;
  if (!zMapGFFIsValidVersion(parser))
    return ;

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



/*
 * Used by both versions.
 *
 *servers have a list of columns in/out as provided by ACEDB and later used by pipes
 * here we privide a list of (single) featuresets as put into the context
 */
GList *zMapGFFGetFeaturesets(ZMapGFFParser parser)
{
  if (!parser)
    return NULL ;
  if (!zMapGFFIsValidVersion(parser))
    return NULL ;

  return (parser->src_feature_sets) ;
}



/*
 * Used by both versions.
 *
 *
 * If stop_on_error is TRUE the parser will not parse any further lines after it encounters
 * the first error in the GFF file.
 */
void zMapGFFSetStopOnError(ZMapGFFParser parser, gboolean stop_on_error)
{
  if (!parser)
    return ;
  if (!zMapGFFIsValidVersion(parser))
    return ;

  if (parser->state != ZMAPGFF_PARSER_ERR)
    parser->stop_on_error = stop_on_error ;

  return ;
}





/*
 * Used by both versions.
 *
 * If SO_compliant is TRUE the parser will only accept SO terms for feature types.
 */
void zMapGFFSetSOCompliance(ZMapGFFParser parser, gboolean SO_compliant)
{
  if (!parser)
    return ;
  if (!zMapGFFIsValidVersion(parser))
    return ;

  if (parser->state != ZMAPGFF_PARSER_ERR)
    parser->SO_compliant = SO_compliant ;

  return ;
}


/*
 * Used by both versions.
 * Sets the clipping mode for handling features that are either partly or wholly outside
 * the requested start/end for the target sequence.
 */
void zMapGFFSetFeatureClip(ZMapGFFParser parser, ZMapGFFClipMode clip_mode)
{
  if (!parser)
    return ;
  if (!zMapGFFIsValidVersion(parser))
    return ;

  if (parser->state != ZMAPGFF_PARSER_ERR)
  {
    parser->clip_mode = clip_mode ;
  }

  return ;
}


/*
 * Used by both versions.
 *
 * Sets the start/end coords for clipping features.
 */
void zMapGFFSetFeatureClipCoords(ZMapGFFParser parser, int start, int end)
{
  if (!parser)
    return ;
  if (!zMapGFFIsValidVersion(parser))
    return ;

  zMapAssert(start > 0 && end > 0 && start <= end) ;

  if (parser->state != ZMAPGFF_PARSER_ERR)
    {
      parser->clip_start = start ;
      parser->clip_end = end ;
    }

  return ;
}




/*
 * Used by both versions.
 *
 * If default_to_basic is TRUE the parser will create basic features for any unrecognised
 * feature type.
 */
void zMapGFFSetDefaultToBasic(ZMapGFFParser parser, gboolean default_to_basic)
{
  if (!parser)
    return ;
  if (!zMapGFFIsValidVersion(parser))
    return ;

  if (parser->state != ZMAPGFF_PARSER_ERR)
    parser->default_to_basic = default_to_basic ;

  return ;
}


/*
 * Used by both versions.
 *
 * Return the GFF version which the parser is using. This is determined from the GFF
 * input stream from the header comments.
 */
int zMapGFFGetVersion(ZMapGFFParser parser)
{
  int version = ZMAPGFF_VERSION_UNKNOWN ;
  if (!parser)
    return ZMAPGFF_VERSION_UNKNOWN ;
  if (!zMapGFFIsValidVersion(parser))
    return ZMAPGFF_VERSION_UNKNOWN ;

  if (parser->state != ZMAPGFF_PARSER_ERR)
    version = parser->gff_version ;

  return version ;
}



/*
 * Used by both versions.
 *
 * Return line number of last line processed (this is the same as the number of lines processed.
 * N.B. we always return this as it may be required for error diagnostics.
 */
int zMapGFFGetLineNumber(ZMapGFFParser parser)
{
  if (!parser)
    return 0 ;
  if (!zMapGFFIsValidVersion(parser))
    return 0 ;

  return parser->line_count ;
}


/*
 * Used by both versions.
 *
 * If a zMapGFFNNN function has failed then this function returns a description of the error
 * in the glib GError format. If there has been no error then NULL is returned.
 */
GError *zMapGFFGetError(ZMapGFFParser parser)
{
  if (!parser)
    return NULL ;
  if (!zMapGFFIsValidVersion(parser))
    return NULL ;

  return parser->error ;
}



int zMapGFFParserGetNumFeatures(ZMapGFFParser parser)
{
  if (!parser)
    return 0 ;
  if (!zMapGFFIsValidVersion(parser))
    return 0 ;
  return(parser->num_features) ;
}



/*
 * Used by both versions.
 *
 * Returns TRUE if the parser has encountered an error from which it cannot recover and hence will
 * not process any more lines, FALSE otherwise.
 */
gboolean zMapGFFTerminated(ZMapGFFParser parser)
{
  gboolean result = TRUE ;
  if (!parser)
    return result ;
  if (!zMapGFFIsValidVersion(parser))
    return result ;

  if (parser->state != ZMAPGFF_PARSER_ERR)
    result = FALSE ;

  return result ;
}



/*
 * Used by both versions.
 *
 *
 * If free_on_destroy == TRUE then all the feature arrays will be freed when
 * the GFF parser is destroyed, otherwise they are left intact. Important
 * because caller may still want access to them after the destroy.
 */
void zMapGFFSetFreeOnDestroy(ZMapGFFParser parser, gboolean free_on_destroy)
{
  if (!parser)
    return ;
  if (!zMapGFFIsValidVersion(parser))
    return ;

  if (parser->state != ZMAPGFF_PARSER_ERR)
    parser->free_on_destroy = free_on_destroy ;

  return ;
}






