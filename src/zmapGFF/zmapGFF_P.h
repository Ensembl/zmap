/*  File: zmapGFF_P.h
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
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jun 15 15:29 2004 (edgrif)
 * Created: Sat May 29 13:18:32 2004 (edgrif)
 * CVS info:   $Id: zmapGFF_P.h,v 1.1 2004-06-15 14:37:35 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_GFF_P_H
#define ZMAP_GFF_P_H

#include <ZMap/zmapGFF.h>



/* Some defines for parsing stuff....my need v2 and v3 versions of these. */
enum {GFF_MANDATORY_FIELDS = 8, GFF_MAX_FIELD_CHARS = 50, GFF_MAX_FREETEXT_CHARS = 1000} ;



/* possible states for parsing GFF file, rather trivial in fact.... */
typedef enum {ZMAPGFF_PARSE_HEADER, ZMAPGFF_PARSE_BODY, ZMAPGFF_PARSE_ERROR} ZMapGFFParseState ;


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




/* For each set of features that come from a single source, we keep an array of those features
 * but also a list of features that need to be built up from several GFF lines. */
typedef struct ZMapGFFParserFeatureSetStruct_
{
  char *source ;					    /* Name of feature set source. */

  GArray *features ;					    /* All features in this feature set. */

  GData *multiline_features ;				    /* Features in this feature set that
							       must be built up from multiple
							       lines. */
  ZMapGFFParser parser ;				    /* Self reference forced on us because
							       GData destroy func. does not have a
							       user_data parameter. */

} ZMapGFFParserFeatureSetStruct, *ZMapGFFParserFeatureSet ;




/* this struct will need to hold the buffers for the parsing, the hash tables for referencing
 * features etc. etc....... */
typedef struct ZMapGFFParserStruct_
{
  ZMapGFFParseState state ;
  GError *error ;					    /* Holds recoverable error. */
  GQuark error_domain ;

  /* Header data, need to find all this for parsing to be valid. */
  gboolean done_version ;
  int gff_version ;

  gboolean done_source ;				    /* Not sure if we need this... */
  char *source_name ;
  char *source_version ;

  gboolean done_sequence_region ;
  char *sequence_name ;
  int sequence_start, sequence_end ;




  /* Some features need to be built up from multiple GFF lines so we keep associations
   * of these features in arrays. The arrays are indexed via sources. These arrays are only used
   * while building up the final arrays of features. */
  GData *feature_sets ;					    /* A list of arrays of features, the
							       arrays are indexed via their
							       "source". Hence each array contains
							       features from a single source. */

  gboolean free_on_destroy ;				    /* TRUE => free all feature arrays
							       when parser is destroyed. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Let's not worry about this for now.........we could even just return the array of arrays
   * dynamically by zooming through the sources and extracting all the sources.... */

  /* The final array of arrays of features. Each sub-array contains features that all have the
   * same source. */
  GArray *feature_sets ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


} ZMapGFFParserStruct ;








#endif /* ZMAP_GFF_P_H */
