/*  File: zmapGFF_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2013: Genome Research Ltd.
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
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description: Internal types, functions etc. for the GFF parser.
 *              This contains functionality common to v2 and v3 of GFF.
 *-------------------------------------------------------------------
 */


#ifndef ZMAP_GFF_P_H
#define ZMAP_GFF_P_H

#include <ZMap/zmapGFF.h>


/*
 * THE TWO MAX CHAR LIMITS NEED TO GO...NEEDS TO BE TOTALLY DYNAMIC.....
 * Some defines for parsing the feature records....may need v2 and v3 versions of these.
 */
#define ZMAPGFF_MANDATORY_FIELDS      8
#define ZMAPGFF_MAX_FIELD_CHARS     500
#define ZMAPGFF_MAX_FREETEXT_CHARS 5000

/*
 * Dynamically resizing buffers are good but they must have a limit to avoid malicious
 * exploitation.
 *
 * Note, our system here is that we set a limit to the length of line we will parse,
 * each buffer is then extended to be this size so that no one gff field value can cause
 * a buffer overrun.
 */
enum
{
  ZMAPGFF_MAX_LINE_LEN = 65536,
  ZMAPGFF_MAX_DYNAMIC_BUFFER_LEN = ZMAPGFF_MAX_LINE_LEN
} ;

/*
 * More stuff on buffer sizes (this used to be in private but is now common
 * to v2 and v3).
 */
enum
{
  ZMAPGFF_BUF_INIT_SIZE = 1500,
  ZMAPGFF_BUF_MULT = 2,
  ZMAPGFF_BUF_FORMAT_SIZE = 512
} ;


/*
 * Types of name find operation, depend on type of feature.
 */
typedef enum
{
  ZMAPGFF_NAME_FIND,
  ZMAPGFF_NAME_USE_SOURCE,
  ZMAPGFF_NAME_USE_SEQUENCE,
  ZMAPGFF_NAME_USE_GIVEN,
  ZMAPGFF_NAME_USE_GIVEN_OR_NAME
} NameFindType ;



/*
 * Parser FSM states. Do not change the values given here as these are
 * used elsewhere to index an array of state transition data.
 */
typedef enum
{
  ZMAPGFF_PARSER_NON = 0,        /* NONE                */
  ZMAPGFF_PARSER_DIR = 1,        /* HEADER/DIRECTIVE    */
  ZMAPGFF_PARSER_BOD = 2,        /* BODY                */
  ZMAPGFF_PARSER_SEQ = 3,        /* SEQUENCE/DNA        */
  ZMAPGFF_PARSER_FAS = 4,        /* FASTA               */
  ZMAPGFF_PARSER_CLO = 5,        /* closure             */
  ZMAPGFF_PARSER_ERR = 6         /* ERROR               */
} ZMapGFFParserState ;
#define ZMAPGFF_NUMBER_PARSER_STATES 7


/*
 * We follow glib convention in error domain naming:
 *          "The error domain is called <NAMESPACE>_<MODULE>_ERROR"
 */
#define ZMAP_GFF_ERROR "ZMAP_GFF_ERROR"


/*
 * buffer for reading directive strings...
 */
#define ZMAP_GFF_DIRECTIVE_BUFFER 1000

/*
 * Some error types for use during reading data stream.
 */
typedef enum
{
  ZMAPGFF_ERROR_HEADER,                                    /* Error in GFF header section.                        */
  ZMAPGFF_ERROR_BODY,                                      /* Error in GFF body section.                          */
  ZMAPGFF_ERROR_FASTA,                                     /* Error whilst reading FASTA section.                 */
  ZMAPGFF_ERROR_SO,                                        /* Error reading the SO collections                    */
  ZMAPGFF_ERROR_FAILED                                     /* Other fatal failure, error->message should explain. */
} ZMapGFFError ;


/*
 * Error level for the SO term checking during parsing.
 */
typedef enum
{
  ZMAPGFF_SOERRORLEVEL_NONE,     /* Do nothing                                */
  ZMAPGFF_SOERRORLEVEL_WARN,     /* Log a warning                             */
  ZMAPGFF_SOERRORLEVEL_ERR,      /* Throw an error                            */
  ZMAPGFF_SOERRORLEVEL_UNK       /* Unknown type                              */
} ZMapSOErrorLevel ;



/*
 * Buffers for GFF parsing.
 */
typedef enum
{
  ZMAPGFF_BUF_SEQ,
  ZMAPGFF_BUF_SOU,
  ZMAPGFF_BUF_TYP,
  ZMAPGFF_BUF_SCO,
  ZMAPGFF_BUF_STR,
  ZMAPGFF_BUF_PHA,
  ZMAPGFF_BUF_ATT,
  ZMAPGFF_BUF_COM,
  ZMAPGFF_BUF_TMP
} GFFStringFieldsType;
#define ZMAPGFF_NUMBER_PARSEBUF 9


/* Some features need to be built up from multiple GFF lines so we keep associations
 * of these features in arrays. The arrays are indexed via sources. These arrays are only used
 * while building up the final arrays of features. */
/* For each set of features that come from a single source, we keep an array of those features
 * but also a list of features that need to be built up from several GFF lines. */
typedef struct ZMapGFFParserFeatureSetStruct_
{
  ZMapFeatureSet feature_set ;                             /* The feature set, gets passed on to
                                                            * the feature context proper. */
                                                           /* NB this is the display column not a set of similar features */

  GData *multiline_features ;                              /* Features in this feature set that
                                                              must be built up from multiple lines. */
  ZMapGFFParser parser ;                                   /* Self reference forced on us because
                                                              GData destroy func. does not have a
                                                              user_data parameter. */
  GHashTable *feature_styles;                              /* copies of styles needed by features
                                                            * that can get pointed at by them */

} ZMapGFFParserFeatureSetStruct, *ZMapGFFParserFeatureSet ;



/*
 * Definition for common data for all parser struct types. I have put this in as a macro
 * definition to make sure the same thing is used and the order of the data is preserved.
 * Original definitions with associated comments were as follows:


  ZMapGFFVersion gff_version ;
  ZMapGFFClipMode clip_mode ;                              Decides how features that overla or are outside the start/end are handled.
  int line_count ;                                         Contains number of lines processed.
  char *sequence_name ;
  int features_start, features_end ;                       In GFF these are based from 1

  int clip_start, clip_end ;                               Coords used for clipping.
  GHashTable *sources ;                                    NOTE for sources read styles; If present, only make features from
                                                           GFF records with a source from this list.
  gboolean parse_only ;                                    TRUE => just parse the GFF for correctness, don't create feature arrays.
  ZMapFeatureTypeStyle locus_set_style ;                   cached locus style.
  GQuark locus_set_id ;                                    if not zero then make a locus set from locus tags in sequence objects.
  gboolean free_on_destroy ;                               TRUE => free all feature arrays when parser is destroyed.
  GData *feature_sets ;                                    A list of ZMapGFFParserFeatureSetStruct. There is one of these structs per
                                                           "source". The struct contains among other things an array of all features 
                                                           for that source.
  GError *error ;                                          Holds last parser error.
  GHashTable *excluded_features ;                          Records all features that should be excluded (e.g. because they are outside 
                                                           coords of ref. sequence).
  GHashTable *source_2_feature_set ;                       Optionally maps source to a feature set.
  GHashTable *source_2_sourcedata ;                        Optionally maps source to extra source data.
  gboolean stop_on_error ;                                 Stop parsing if there is an error.
  gboolean default_to_basic ;                              TRUE => Unrecognised feature types will be created as basic features.

  GList *src_feature_sets;                                 list of quarks of actual feature sets in the context

 */

#define ZMAPGFF_PARSER_STRUCT_COMMON_DATA                                                            \
                                              ZMapGFFVersion gff_version ;                           \
                                              ZMapGFFParserState state ;                             \
                                              ZMapGFFClipMode clip_mode ;                            \
                                              ZMapSequenceStruct seq_data ;                          \
                                                                                                     \
                                              int features_start,                                    \
                                                  features_end,                                      \
                                                  line_count ,                                       \
                                                  num_features,                                      \
                                                  clip_start,                                        \
                                                  clip_end ;                                         \
                                                                                                     \
                                              char cDelimBodyLine,                                   \
                                                   cDelimAttributes,                                 \
                                                   cDelimAttValue,                                   \
                                                   cDelimQuote ;                                     \
                                                                                                     \
                                              char *sequence_name,                                   \
                                                   *format_str,                                      \
                                                   *cigar_string_format_str ;                        \
                                                                                                     \
                                              char *buffers[ZMAPGFF_NUMBER_PARSEBUF] ;               \
                                                                                                     \
                                              gsize buffer_length ;                                  \
                                                                                                     \
                                              gboolean parse_only,                                   \
                                                free_on_destroy,                                     \
                                                stop_on_error,                                       \
                                                default_to_basic,                                    \
                                                SO_compliant ;                                       \
                                                                                                     \
                                              GString *raw_line_data ;                               \
                                                                                                     \
                                              GHashTable *sources,                                   \
                                                         *excluded_features,                         \
                                                         *source_2_feature_set,                      \
                                                         *source_2_sourcedata;                       \
                                                                                                     \
                                              ZMapFeatureTypeStyle locus_set_style;                  \ 
                                                                                                     \
                                              GQuark locus_set_id, error_domain ;                    \
                                                                                                     \
                                              GData *feature_sets ;                                  \
                                                                                                     \
                                              GError *error ;                                        \
                                                                                                     \
                                              GList *src_feature_sets;




/*
 * Base "class" for the parser. The declarations of the derived ones are in
 * the files "zmapGFF2_P.h" and "zmapGFF3_P.h".
 */
typedef struct ZMapGFFParserStruct_
{
  /*
   * Data common to both versions.
   */
  ZMAPGFF_PARSER_STRUCT_COMMON_DATA



} ZMapGFFParserStruct ;






#endif /* ZMAP_GFF_P_H */
