/*  File: zmapFeatureLoadDisplay.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2013-2015: Genome Research Ltd.
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Interface for loading, manipulating, displaying
 *              sets of features.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_FEATURE_LOAD_DISPLAY_H
#define ZMAP_FEATURE_LOAD_DISPLAY_H

#include <map>

#include <ZMap/zmapStyle.hpp>
#include <ZMap/zmapStyleTree.hpp>
#include <ZMap/zmapConfigStanzaStructs.hpp>

#include <string>
#include <map>


/* Overview:
 * 
 * A "Featureset" is a set of one type of features (e.g. EST alignments from mouse),
 * this set is derived from a single data source. The meta data for the featureset
 * and the source is held in a  ZMapFeatureSourceStruct.
 * 
 * Featuresets are displayed in columns and there can be more than one featureset
 * in any one column. Data about which column a feature set is held in is held in
 * a ZMapFeatureSetDescStruct (bad name for it).
 * 
 * Columns hold one-to-many featuresets and information about columns is held in
 * a ZMapFeatureColumnStruct.
 * 
 * All these structs need to associated with each other and this information is
 * held in a ZMapFeatureContextMapStruct (again, not a good name).
 * 
 */


/* Struct holding information about sets of features. Can be used to look up the
 * style for a feature plus other stuff. */
typedef struct ZMapFeatureSourceStructType
{
  GQuark source_id ;    /* The source name. From ACE this is the key used to ref this struct */
                        /* but we can config an alternate name (requested by graham) */

  GQuark source_text ;  /* Description. */

  GQuark style_id ;     /* The style for processing the source. */

  GQuark related_column;	/* eg real data from coverage */

  GQuark maps_to;			/* composite featureset many->one
  					 * composite does not exist but all are displayed as one
  					 * requires ZMapWindowGraphDensityItem
  					 * only relevant to coverage data
  					 */

  gboolean is_seq;		/* true for coverage and real seq-data */

} ZMapFeatureSourceStruct, *ZMapFeatureSource ;


/* Struct for "feature set" information. Used to look up "meta" information for each feature set. */
typedef struct ZMapFeatureSetDescStructType
{
  GQuark column_id ;           /* The set name. (the display column) as a key value*/
  GQuark column_ID ;           /* The set name. (the display column) as display text*/

  GQuark feature_src_ID;            // the name of the source featureset (with upper case)
                                    // struct is keyed with normalised name
  const char *feature_set_text;           // renamed so we can search for this

} ZMapFeatureSetDescStruct, *ZMapFeatureSetDesc ;


/* All the info about a display column, note these are "logical" columns,
 * real display columns get munged with strand and frame */
typedef struct ZMapFeatureColumnStructType
{
  /* Column name, description, ordering. */
  GQuark unique_id ;					    /* For searching. */
  GQuark column_id ;					    /* For display. */
  char *column_desc ;
  int order ;

  /* column specific style data may be config'd explicitly or derived from contained featuresets */
  ZMapFeatureTypeStyle style ;
  GQuark style_id;					    /* can be set before we get the style itself */

  /* all the styles needed by the column */
  GList *style_table ;

  /* list of those configured these get filled in when servers request featureset-names
   * for pipe servers we could do this during server config but for ACE
   * (and possibly DAS) we have to wait till they provide data. */
  GList *featuresets_names ;

  /* we need both user style and unique id's both are filled in by lazy evaluation
   * if not configured explicitly (featuresets is set by the [columns] config)
   * NOTE we now have virtual featuresets for BAM coverage that do not exist
   * servers that provide a mapping must delete these lists */
  GList *featuresets_unique_ids ;

} ZMapFeatureColumnStruct, *ZMapFeatureColumn ;





/* All the featureset/featureset data/column/style data - used by view and window */
typedef struct ZMapFeatureContextMapStructType
{
  /* All the styles known to the view or window.
   * Maps the style's unique_id (GQuark) to ZMapFeatureTypeStyle. */
  //GHashTable *styles ;
  ZMapStyleTree styles ;

  /* All the columns that ZMap will display.
   * These may contain several featuresets each, They are in display order left to right.
   * Maps the column's unique_id (GQuark) to ZMapFeatureColumn */
  std::map<GQuark, ZMapFeatureColumn> *columns ;


  /* Mapping of a feature source to a column using ZMapFeatureSetDesc
   * NB: this contains data from ZMap config sections [columns] [featureset_description] _and_
   * ACEDB */
  /* gb10: Note that this relationship is cached in reverse in the ZMapFeatureColumn, but the data
   * originates from this hash table. It's something to do with acedb servers returning this
   * relationship after the column has been created so we don't know it up front. Perhaps that's
   * something we could revisit and maybe get rid of this hash table? */
  GHashTable *featureset_2_column ;

  /* Mapping of each column to all the styles it requires. 
   * NB: this stores data from ZMap config sections [featureset_styles] _and_ [column_styles] 
   * _and_ ACEDB collisions are merged Columns treated as fake featuresets so as to have a style.
   * Maps the column's unique_id (GQuark) to GList of style unique_ids (GQuark). */
  /* gb10: It looks like this is duplicate information because each ZMapFeatureColumn has a
   * style_table listing all of the styles that column requires. Also, the column has a list of
   * featuresets and each featureset has a pointer to the style. Perhaps we can get rid of this
   * hash table and use the column's style_table instead? Or get rid of both of those and just
   * access the styles directly from the featuresets. */
  GHashTable *column_2_styles ;

  /* The source data for a featureset. 
   * This consists of style id and description and source id
   * NB: the GFFSource.source  (quark) is the GFF_source name the hash table
   * Maps the featureset unique_id (GQuark) to ZMapFeatureSource. */
  /* gb10: Could we just get the ZMapFeatureSet to point directly to this data and get rid 
   * of this hash table? */
  GHashTable *source_2_sourcedata ;


  /* This is a list BAM featureset unique id's (GQuark). It is only used to create the Blixem and
   * ZMap right-click menus for short-read options. This info is also available via the is_seq
   * member of the ZMapFeatureSource struct. */
  /* gb10: I'm not sure how much those menus are used so maybe we should look into getting rid of
   * them? Or look at using is_seq to construct these menus instead (if that's not too slow)? */
  GList *seq_data_featuresets ;

  /* This maps virtual featuresets to their lists of real featuresets. The reverse mapping is
   * provided by the maps_to field in the ZMapFeatureSource. 
   * Maps the virtual featureset unique_id (GQuark) to a GList of real featureset unique_ids
   * (GQuarks) */
  /* gb10: This is currently only used in one place (zmapWindowFetchData) to get the list of
   * real featureset IDs from the virtual featureset ID. Perhaps we can use the info from maps_to
   * to do that instead (if that's not too slow)? */
  GHashTable *virtual_featuresets ;

  
  /* This allows columns to be grouped together on a user-defined/configured basis, e.g. for
   * running blixem on a related set of columns. A column can be in multiple groups. 
   * Maps the group unique_id (GQuark) to a GList of column unique ids (GQuark) */
  GHashTable *column_groups ;

  /* This lists all user-created sources */
  std::map<std::string, ZMapConfigSource> *sources ;

} ZMapFeatureContextMapStruct, *ZMapFeatureContextMap ;



/* This cached info about GFF parsing that is in progress. We need to cache this info if we need
 * to parse the input GFF file(s) on startup to populate the ZMapFeatureSequenceMap before we're
 * ready to read the features in themselves - then when we do come read the features, this cached
 * info lets us use the same parser to continue reading the file where we left off.
 * (Note that we can't rewind the input stream and start again because we want to support stdin.) */
typedef struct ZMapFeatureParserCacheStructType
{
  gpointer parser ;
  GString *line ;     /* the last line that was parsed */
  GIOChannel *pipe ;
  GIOStatus pipe_status ;
} ZMapFeatureParserCacheStruct, *ZMapFeatureParserCache ;



/* Holds data about a sequence to be fetched.
 * Used for the 'default-sequence' from the config file or one loaded later
 * via a peer program, e.g. otterlace. */
typedef struct ZMapFeatureSequenceMapStructType
{
  char *config_file ;
  GSList *file_list ;   /* list of filenames passed on command line */
  char *stylesfile ;    /* path to styles file given on command line or in config dir */

  GHashTable *cached_parsers ; /* filenames (as GQuarks) mapped to cached info about GFF parsing that
                                * is in progress (ZMapFeatureParserCache) if parsing has already been started */

  char *dataset ;                                           /* e.g. human */
  char *sequence ;                                          /* e.g. chr6-18 */
  int start, end ;                                          /* chromosome coordinates */
} ZMapFeatureSequenceMapStruct, *ZMapFeatureSequenceMap ;



ZMapFeatureColumn zMapFeatureGetSetColumn(ZMapFeatureContextMap map, GQuark set_id) ;
gboolean zMapFeatureIsCoverageColumn(ZMapFeatureContextMap map, GQuark column_id) ;
gboolean zMapFeatureIsSeqColumn(ZMapFeatureContextMap map, GQuark column_id) ;
gboolean zMapFeatureIsSeqFeatureSet(ZMapFeatureContextMap map, GQuark fset_id) ;
GList *zMapFeatureGetColumnFeatureSets(ZMapFeatureContextMap map, GQuark column_id, gboolean unique_id) ;

#endif /* ZMAP_FEATURE_LOAD_DISPLAY_H */

