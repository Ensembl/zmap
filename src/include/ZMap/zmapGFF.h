/*  File: zmapGFF.h
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk,
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description: Interface to a GFF parser, the parser works in a
 *              "line at a time" way, the caller must pass complete
 *              GFF lines to the parser which then builds up arrays
 *              of ZMapFeatureStruct's, one for each GFF source.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_GFF_H
#define ZMAP_GFF_H

#include <glib.h>

#include <ZMap/zmapFeature.h>


/*
 * A forward declaration for all versions of the parser.
 */
typedef struct ZMapGFFParserStruct_ *ZMapGFFParser ;


/*
 * Version of GFF in use; only these symbols should be used. These are used
 * as numerical values at some points, so must not be changed!
 */
typedef enum
{
  ZMAPGFF_VERSION_2 = 2,
  ZMAPGFF_VERSION_3 = 3,
  ZMAPGFF_VERSION_UNKNOWN
}  ZMapGFFVersion ;


/* Default gff version parsed. */
enum
{
  GFF_DEFAULT_VERSION = ZMAPGFF_VERSION_2
} ;



/*
 * Feature clip mode, selects how individual feature coords should be clipped in relation
 * to the requested feature range (the gff file may contain features that are outside the
 * requested range.
 */
typedef enum
{
  GFF_CLIP_NONE,					       /* Don't clip feature coords at all. */
  GFF_CLIP_OVERLAP,					    /* Exclude features outside the range, clip anything that overlaps (default). */
  GFF_CLIP_ALL					         /* Exclude features outside or overlapping. */
}
ZMapGFFClipMode ;



/* Types/Struct for GFF file header info., currently this is all we are interested
 * in but this may expand as GFFv3 support is introduced. */

typedef enum
{
  GFF_HEADER_NONE,
  GFF_HEADER_ERROR,
  GFF_HEADER_INCOMPLETE,
  GFF_HEADER_COMPLETE,
  ZMAPGFF_HEADER_OK,               /* from v3 code */
  ZMAPGFF_HEADER_UNKNOWN           /* from v3 code */
} ZMapGFFHeaderState ;





/*
 * Some new GFF interface functions.
 */
gboolean zMapGFFGetVersionFromString(const char* const sString, int * const piOut) ;
gboolean zMapGFFGetVersionFromGIO(GIOChannel * const pChannel, int * const piOut ) ;


/*
 * Modified old interface.
 */
gboolean zMapGFFIsValidVersion(const ZMapGFFParser const ) ;
ZMapGFFParser zMapGFFCreateParser(int iGFFVersion, char *sequence, int features_start, int features_end) ;
gboolean zMapGFFParserInitForFeatures(ZMapGFFParser parser, GHashTable *sources, gboolean parse_only) ;
gboolean zMapGFFParseHeader(ZMapGFFParser parser, char *line, gboolean *header_finished, ZMapGFFHeaderState *header_state) ;
gboolean zMapGFFParseSequence(ZMapGFFParser parser, char *line, gboolean *sequence_finished) ;
gboolean zMapGFFParseLine(ZMapGFFParser parser, char *line) ;
gboolean zMapGFFParseLineLength(ZMapGFFParser parser, char *line, gsize line_length) ;
void zMapGFFParseSetSourceHash(ZMapGFFParser parser,
			       GHashTable *source_2_feature_set, GHashTable *source_2_sourcedata) ;
GList *zMapGFFGetFeaturesets(ZMapGFFParser parser);
void zMapGFFSetSOCompliance(ZMapGFFParser parser, gboolean SO_compliant) ;
void zMapGFFSetFeatureClip(ZMapGFFParser parser, ZMapGFFClipMode clip_mode) ;
void zMapGFFSetFeatureClipCoords(ZMapGFFParser parser, int start, int end) ;
void zMapGFFSetDefaultToBasic(ZMapGFFParser parser, gboolean default_to_basic);
int zMapGFFParserGetNumFeatures(ZMapGFFParser parser);
int zMapGFFGetVersion(ZMapGFFParser parser) ;
GError *zMapGFFGetError(ZMapGFFParser parser) ;
int zMapGFFGetLineNumber(ZMapGFFParser parser) ;
gboolean zMapGFFTerminated(ZMapGFFParser parser) ;
void zMapGFFSetFreeOnDestroy(ZMapGFFParser parser, gboolean free_on_destroy) ;
void zMapGFFDestroyParser(ZMapGFFParser parser) ;

/*
 * Unchanged old interface.
 */
gboolean zMapGFFParserSetSequenceFlag(ZMapGFFParser parser);
ZMapSequence zMapGFFGetSequence(ZMapGFFParser parser);
GHashTable *zMapGFFParserGetStyles(ZMapGFFParser parser);
void zMapGFFSetStopOnError(ZMapGFFParser parser, gboolean stop_on_error) ;
void zMapGFFSetParseOnly(ZMapGFFParser parser, gboolean parse_only) ;
gboolean zMapGFFGetFeatures(ZMapGFFParser parser, ZMapFeatureBlock feature_block) ;

gboolean zMapGFFDump(ZMapFeatureAny dump_set, GHashTable *styles, GIOChannel *file, GError **error_out);
gboolean zMapGFFDumpRegion(ZMapFeatureAny dump_set, GHashTable *styles,
			   ZMapSpan region_span, GIOChannel *file, GError **error_out) ;
gboolean zMapGFFDumpForeachList(ZMapFeatureAny first_feature, GHashTable *styles,
				GIOChannel *file, GError **error_out,
				char *sequence,
				GFunc *list_func_out, gpointer *list_data_out) ;

#endif /* ZMAP_GFF_H */
