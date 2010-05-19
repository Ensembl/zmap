/*  File: zmapGFF.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 * Description: Interface to a GFF parser, the parser works in a
 *              "line at a time" way, the caller must pass complete
 *              GFF lines to the parser which then builds up arrays
 *              of ZMapFeatureStruct's, one for each GFF source.
 *
 * HISTORY:
 * Last edited: Apr 21 18:23 2010 (edgrif)
 * Created: Sat May 29 13:18:32 2004 (edgrif)
 * CVS info:   $Id: zmapGFF.h,v 1.23 2010-05-19 13:15:31 mh17 Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_GFF_H
#define ZMAP_GFF_H

#include <glib.h>
#include <ZMap/zmapFeature.h>


/* An instance of a parser. */
typedef struct ZMapGFFParserStruct_ *ZMapGFFParser ;


/* Feature clip mode, selects how individual feature coords should be clipped in relation
 * to the requested feature range (the gff file may contain features that are outside the
 * requested range. */
typedef enum
  {
    GFF_CLIP_NONE,					    /* Don't clip feature coords at all. */
    GFF_CLIP_OVERLAP,					    /* Exclude features outside the range,
							       clip anything that overlaps (default). */
    GFF_CLIP_ALL					    /* Exclude features outside or
							       overlapping. */
  } ZMapGFFClipMode ;


/* Struct holding GFF file header info. */
typedef struct
{
  int gff_version ;

  char *source_name ;
  char *source_version ;

  char *sequence_name ;
  int features_start ;
  int features_end ;
} ZMapGFFHeaderStruct, *ZMapGFFHeader ;



/* Struct for "feature set" information. Used to look up "meta" information for each feature set. */
typedef struct
{
      // really need to change feature_set to column: it's confusing
  GQuark feature_set_id ;		/* The set name. (the display column) as a key value*/
  GQuark feature_set_ID ;           /* The set name. (the display column) as display text*/

  GQuark feature_src_ID;            // the name of the source featureset (with upper case)
                                    // struct is keyed with normalised name
//  char *description ;		      /* Description. */
  char *feature_set_text;           // renamed so we can search for this

} ZMapGFFSetStruct, *ZMapGFFSet ;



/* Struct holding "per source" information for GFF data. Can be used to look up the
 * style for a GFF feature plus other stuff. */
typedef struct
{
  GQuark source_id ;					    /* The source name. */

  GQuark source_text ;					    /* Description. */

  GQuark style_id ;					    /* The style for processing the source. */

} ZMapGFFSourceStruct, *ZMapGFFSource ;




ZMapGFFParser zMapGFFCreateParser(void) ;
gboolean zMapGFFParserInitForFeatures(ZMapGFFParser parser, GData *sources, gboolean parse_only) ;
gboolean zMapGFFParseHeader(ZMapGFFParser parser, char *line, gboolean *header_finished) ;
gboolean zMapGFFParseLine(ZMapGFFParser parser, char *line) ;
gboolean zMapGFFParseLineLength(ZMapGFFParser parser, char *line, gsize line_length) ;
gboolean zMapGFFParseSequence(ZMapGFFParser parser, char *line, gboolean *sequence_finished) ;
gboolean zMapGFFParserSetSequenceFlag(ZMapGFFParser parser);
ZMapSequence zMapGFFGetSequence(ZMapGFFParser parser);
void zMapGFFParseSetSourceHash(ZMapGFFParser parser,
			       GHashTable *source_2_feature_set, GHashTable *source_2_sourcedata) ;
void zMapGFFSetStopOnError(ZMapGFFParser parser, gboolean stop_on_error) ;
void zMapGFFSetParseOnly(ZMapGFFParser parser, gboolean parse_only) ;
void zMapGFFSetSOCompliance(ZMapGFFParser parser, gboolean SO_compliant) ;
void zMapGFFSetFeatureClip(ZMapGFFParser parser, ZMapGFFClipMode clip_mode) ;
void zMapGFFSetFeatureClipCoords(ZMapGFFParser parser, int start, int end) ;
ZMapGFFHeader zMapGFFGetHeader(ZMapGFFParser parser) ;
void zMapGFFFreeHeader(ZMapGFFHeader header) ;
gboolean zMapGFFGetFeatures(ZMapGFFParser parser, ZMapFeatureBlock feature_block) ;
int zMapGFFGetVersion(ZMapGFFParser parser) ;
int zMapGFFGetLineNumber(ZMapGFFParser parser) ;
GError *zMapGFFGetError(ZMapGFFParser parser) ;
gboolean zMapGFFTerminated(ZMapGFFParser parser) ;
void zMapGFFSetFreeOnDestroy(ZMapGFFParser parser, gboolean free_on_destroy) ;
void zMapGFFDestroyParser(ZMapGFFParser parser) ;

gboolean zMapGFFDump(ZMapFeatureAny dump_set, GData *styles, GIOChannel *file, GError **error_out);
gboolean zMapGFFDumpRegion(ZMapFeatureAny dump_set, GData *styles,
			   ZMapSpan region_span, GIOChannel *file, GError **error_out) ;
gboolean zMapGFFDumpForeachList(ZMapFeatureAny first_feature, GData *styles,
				GIOChannel *file, GError **error_out,
				char *sequence,
				GFunc *list_func_out, gpointer *list_data_out) ;

#endif /* ZMAP_GFF_H */
