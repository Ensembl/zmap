/*  File: zmapGFF_P.h
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
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description: Internal types, functions etc. for the GFF parser.
 *              This contains functionality common to v2 and v3 of GFF.
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_GFF_P_H
#define ZMAP_GFF_P_H

#include <ZMap/zmapGFF.h>


/* Default gff version parsed. */
enum
{
  GFF_DEFAULT_VERSION = ZMAPGFF_VERSION_2
} ;


/* THE TWO MAX CHAR LIMITS NEED TO GO...NEEDS TO BE TOTALLY DYNAMIC..... */
/* Some defines for parsing the feature records....may need v2 and v3 versions of these. */
#define GFF_MANDATORY_FIELDS      8
#define GFF_MAX_FIELD_CHARS     500
#define GFF_MAX_FREETEXT_CHARS 5000


/* Dynamically resizing buffers are good but they must have a limit to avoid malicious
 * exploitation.
 *
 * Note, our system here is that we set a limit to the length of line we will parse,
 * each buffer is then extended to be this size so that no one gff field value can cause
 * a buffer overrun.
 */
enum {GFF_MAX_LINE_LEN = 65536, GFF_MAX_DYNAMIC_BUFFER_LEN = GFF_MAX_LINE_LEN} ;


/* possible states for parsing GFF file, rather trivial in fact.... */
typedef enum {ZMAPGFF_PARSE_HEADER, ZMAPGFF_PARSE_BODY,
	      ZMAPGFF_PARSE_SEQUENCE, ZMAPGFF_PARSE_ERROR} ZMapGFFParseState ;


/* We follow glib convention in error domain naming:
 *          "The error domain is called <NAMESPACE>_<MODULE>_ERROR" */
#define ZMAP_GFF_ERROR "ZMAP_GFF_ERROR"

typedef enum
{
  ZMAP_GFF_ERROR_HEADER,				    /* Error in GFF header section. */
  ZMAP_GFF_ERROR_BODY,					    /* Error in GFF body section. */
  ZMAP_GFF_ERROR_FAILED					    /* Other fatal failure, error->message
							       should explain. */
} ZMapGFFError ;



/* Used for maintaining dynamic buffers for parsing gff line elements. On
 * resizing the new buffers are size (line_length * BUF_MULT), so note
 * that the initial buffer size will be _twice_ whatever BUF_INIT is !
 *  */
typedef enum
  {
    GFF_BUF_SEQUENCE, GFF_BUF_SOURCE, GFF_BUF_FEATURE_TYPE,
    GFF_BUF_SCORE, GFF_BUF_STRAND, GFF_BUF_PHASE,
    GFF_BUF_ATTRIBUTES, GFF_BUF_COMMENTS,
    GFF_BUF_TMP,					    /* working buffer. */
    GFF_BUF_NUM
  } GFFStringFieldsType ;

enum {BUF_INIT_SIZE = 1500, BUF_MULT = 2, BUF_FORMAT_SIZE = 512} ;


/* Some features need to be built up from multiple GFF lines so we keep associations
 * of these features in arrays. The arrays are indexed via sources. These arrays are only used
 * while building up the final arrays of features. */
/* For each set of features that come from a single source, we keep an array of those features
 * but also a list of features that need to be built up from several GFF lines. */
typedef struct ZMapGFFParserFeatureSetStruct_
{
  ZMapFeatureSet feature_set ;				    /* The feature set, gets passed on to
							     * the feature context proper. */
                                               /* NB this is the display column not a set of similar features */

  GData *multiline_features ;				    /* Features in this feature set that
							       must be built up from multiple
							       lines. */
  ZMapGFFParser parser ;				    /* Self reference forced on us because
							       GData destroy func. does not have a
							       user_data parameter. */
  GHashTable *feature_styles;                   /* copies of styles needed by features
                                                 * that can get pointed at by them */

} ZMapGFFParserFeatureSetStruct, *ZMapGFFParserFeatureSet ;



/*
 * Definition for common data for all parser struct types. I have put this in as a macro
 * definition to make sure the same thing is used and the order of the data is preserved.
 * Original definitions with associated comments were as follows:


  ZMapGFFVersion gff_version ;
  int line_count ;					                          Contains number of lines processed.
  char *sequence_name ;
  int features_start, features_end ;			          In GFF these are based from 1

  int clip_start, clip_end ;				                 Coords used for clipping.
  GHashTable *sources ;		                        NOTE for sources read styles; If present, only make features from
                                                 GFF records with a source from this list.
  gboolean parse_only ;					                     TRUE => just parse the GFF for correctness, don't create feature arrays.
  ZMapFeatureTypeStyle locus_set_style ;	   	    cached locus style.
  GQuark locus_set_id ;					                     if not zero then make a locus set from locus tags in sequence objects.
  gboolean free_on_destroy ;				                 TRUE => free all feature arrays when parser is destroyed.
  GData *feature_sets ;					                     A list of ZMapGFFParserFeatureSetStruct. There is one of these structs per
							                                          "source". The struct contains among other things an array of all features for that source.
  GError *error ;					                           Holds last parser error.
  GHashTable *excluded_features ;			             Records all features that should be excluded (e.g. because they are outside coords of ref. sequence).
  GHashTable *source_2_feature_set ;			          Optionally maps source to a feature set.
  GHashTable *source_2_sourcedata ;			           Optionally maps source to extra source data.
  gboolean stop_on_error ;				                   Stop parsing if there is an error.
  gboolean default_to_basic ;			           	     TRUE => Unrecognised feature types will be created as basic features.

  GList *src_feature_sets;                       list of quarks of actual feature sets in the context

 */

#define ZMAPGFF_PARSER_STRUCT_COMMON_DATA                                                            \
                                              ZMapGFFVersion gff_version ;                           \
                                              ZMapGFFParseState state ;                              \
                                              int features_start,                                    \
                                                  features_end,                                      \
                                                  line_count ,                                       \
                                                  num_features,                                      \
                                                  clip_start,                                        \
                                                  clip_end ;                                         \
                                              char *sequence_name ;                                  \
                                              gboolean parse_only,                                   \
                                                free_on_destroy,                                     \
                                                stop_on_error,                                       \
                                                default_to_basic ;                                   \
                                              GString *raw_line_data ;                               \
                                              GHashTable *sources,                                   \
                                                         *excluded_features,                         \
                                                         *source_2_feature_set,                      \
                                                         *source_2_sourcedata;                       \
                                              ZMapFeatureTypeStyle locus_set_style ;	             	  \
                                              GQuark locus_set_id, error_domain ;                    \
                                              GData *feature_sets ;                                  \
                                              GError *error ;                                        \
                                              GList *src_feature_sets;




/*
 * Base "class" for the parser. The declarations of the derived ones are in
 * the files "zmapGFF2_P.h" and "zmapGFF3_P.h".
 *
 */
typedef struct ZMapGFFParserStruct_
{
  /*
   * Data common to both versions.
   */
  ZMAPGFF_PARSER_STRUCT_COMMON_DATA



} ZMapGFFParserStruct ;






#endif /* ZMAP_GFF_P_H */
