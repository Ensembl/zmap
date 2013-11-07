/*  File: zmapFeatureLoadDisplay.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2013: Genome Research Ltd.
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

#include <ZMap/zmapStyle.h>


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
  char *feature_set_text;           // renamed so we can search for this

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
  /* All the styles known to the view or window */
  GHashTable *styles ;

  /* Mapping of each column to all the styles it requires. using a GHashTable of
   * GLists of style quark id's. NB: this stores data from ZMap config
   * sections [featureset_styles] _and_ [column_styles] _and_ ACEDB
   * collisions are merged Columns treated as fake featuresets so as to have a style. */
  GHashTable *column_2_styles ;

  /* Mapping of a feature source to a column using ZMapFeatureSetDesc
   * NB: this contains data from ZMap config sections [columns] [featureset_description] _and_
   * ACEDB */
  GHashTable *featureset_2_column ;

  /* Mapping of a feature source to its data using ZMapFeatureSource
   * This consists of style id and description and source id
   * NB: the GFFSource.source  (quark) is the GFF_source name the hash table
   * is indexed by the featureset name quark. */
  GHashTable *source_2_sourcedata ;

  /* All the columns that ZMap will display stored as ZMapFeatureColumn
   * These may contain several featuresets each, They are in display order left to right. */
  GHashTable *columns ;

  /* sources of BAM data  as unique id's, use source_2_sourcedata  for display name */
  GList *seq_data_featuresets ;

  /* place holder for lists of featuresets */
  GHashTable *virtual_featuresets ;

} ZMapFeatureContextMapStruct, *ZMapFeatureContextMap ;



/* THIS IS THE WRONG PLACE FOR THIS I THINK...EG */
/* Holds data about a sequence to be fetched.
 * Used for the 'default-sequence' from the config file or one loaded later
 * via a peer program, e.g. otterlace. */
typedef struct ZMapFeatureSequenceMapStructType
{
  char *config_file ;

  char *dataset ;       /* eg human */
  char *sequence ;      /* eg chr6-18 */
  int start, end ;      /* chromosome coordinates */
} ZMapFeatureSequenceMapStruct, *ZMapFeatureSequenceMap ;



ZMapFeatureColumn zMapFeatureGetSetColumn(ZMapFeatureContextMap map, GQuark set_id) ;
gboolean zMapFeatureIsCoverageColumn(ZMapFeatureContextMap map, GQuark column_id) ;
gboolean zMapFeatureIsSeqColumn(ZMapFeatureContextMap map, GQuark column_id) ;
gboolean zMapFeatureIsSeqFeatureSet(ZMapFeatureContextMap map, GQuark fset_id) ;
GList *zMapFeatureGetColumnFeatureSets(ZMapFeatureContextMap map, GQuark column_id, gboolean unique_id) ;


#endif /* ZMAP_FEATURE_LOAD_DISPLAY_H */

