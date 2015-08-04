/*  File: zmapGFF.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 * Used for formatting of output.
 */
typedef struct ZMapGFFFormatDataStruct_ *ZMapGFFFormatData ;


/*
 * Version of GFF in use; only these symbols should be used. These are used
 * as numerical values at some points, so must not be changed!
 */
typedef enum
{
  ZMAPGFF_VERSION_UNKNOWN = 0,
  ZMAPGFF_VERSION_2 = 2,
  ZMAPGFF_VERSION_3 = 3,
}  ZMapGFFVersion ;


/* Default gff version parsed. */
enum
{
  GFF_DEFAULT_VERSION = ZMAPGFF_VERSION_2
} ;

/*
 * These are flags to set to ouput various of the attributes
 * in GFFv3.
 */
typedef struct ZMapGFFAttributeFlagsStruct_
{
  unsigned int name        : 1 ;
  unsigned int id          : 1 ;
  unsigned int parent      : 1 ;
  unsigned int note        : 1 ;
  unsigned int locus       : 1 ;
  unsigned int percent_id  : 1 ;
  unsigned int url         : 1 ;
  unsigned int gap         : 1 ;
  unsigned int target      : 1 ;
  unsigned int variation   : 1 ;
  unsigned int sequence    : 1 ;
  unsigned int evidence    : 1 ;
} ZMapGFFAttributeFlagsStruct, *ZMapGFFAttributeFlags;

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



/*
 * Types/Struct for GFF file header info
 */

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
gboolean zMapGFFGetVersionFromGIO(GIOChannel * const pChannel, GString *pString,
                                  int * const piOut, GIOStatus *cStatusOut, GError **pError_out) ;


/*
 * Modified old interface.
 */
gboolean zMapGFFIsValidVersion(ZMapGFFParser) ;
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
void zMapGFFParserIncrementNumFeatures(ZMapGFFParser parser) ;
int zMapGFFGetVersion(ZMapGFFParser parser) ;
GError *zMapGFFGetError(ZMapGFFParser parser) ;
int zMapGFFGetLineNumber(ZMapGFFParser parser) ;
int zMapGFFGetLineBod(ZMapGFFParser parser) ;
int zMapGFFGetLineDir(ZMapGFFParser parser) ;
int zMapGFFGetLineSeq(ZMapGFFParser parser) ;
int zMapGFFGetLineFas(ZMapGFFParser parser) ;
void zMapGFFIncrementLineNumber(ZMapGFFParser parser) ;
void zMapGFFIncrementLineBod(ZMapGFFParser parser) ;
void zMapGFFIncrementLineDir(ZMapGFFParser parser) ;
void zMapGFFIncrementLineSeq(ZMapGFFParser parser) ;
void zMapGFFIncrementLineFas(ZMapGFFParser parser) ;
int zMapGFFGetFeaturesStart(ZMapGFFParser parser) ;
int zMapGFFGetFeaturesEnd(ZMapGFFParser parser) ;
char* zMapGFFGetSequenceName(ZMapGFFParser parser) ;
gboolean zMapGFFTerminated(ZMapGFFParser parser) ;
void zMapGFFSetFreeOnDestroy(ZMapGFFParser parser, gboolean free_on_destroy) ;
void zMapGFFDestroyParser(ZMapGFFParser parser) ;
gboolean zMapGFFParsingHeader(ZMapGFFParser parser) ;

/*
 * Unchanged old interface.
 */
gboolean zMapGFFParserSetSequenceFlag(ZMapGFFParser parser);
ZMapSequence zMapGFFGetSequence(ZMapGFFParser parser, GQuark sequence_name);
int zMapGFFGetSequenceNum(ZMapGFFParser parser) ;
gboolean zMapGFFSequenceDestroy(ZMapSequence sequence) ;
GHashTable *zMapGFFParserGetStyles(ZMapGFFParser parser);
void zMapGFFSetStopOnError(ZMapGFFParser parser, gboolean stop_on_error) ;
void zMapGFFSetParseOnly(ZMapGFFParser parser, gboolean parse_only) ;
gboolean zMapGFFGetFeatures(ZMapGFFParser parser, ZMapFeatureBlock feature_block) ;

/*
 * Output functions.
 */
gboolean zMapGFFWriteFeatureGraph(ZMapFeature, ZMapGFFAttributeFlags, GString*, gboolean) ;
gboolean zMapGFFWriteFeatureText(ZMapFeature, ZMapGFFAttributeFlags, GString*, gboolean) ;
gboolean zMapGFFWriteFeatureTranscript(ZMapFeature , ZMapGFFAttributeFlags, GString *, gboolean) ;
gboolean zMapGFFWriteFeatureBasic(ZMapFeature , ZMapGFFAttributeFlags, GString *, gboolean) ;
gboolean zMapGFFWriteFeatureAlignment(ZMapFeature , ZMapGFFAttributeFlags, GString *, gboolean, const char *) ;
gboolean zMapGFFFormatAttributeSetGraph(ZMapGFFAttributeFlags ) ;
gboolean zMapGFFFormatAttributeSetBasic(ZMapGFFAttributeFlags ) ;
gboolean zMapGFFFormatAttributeSetTranscript(ZMapGFFAttributeFlags ) ;
gboolean zMapGFFFormatAttributeSetAlignment(ZMapGFFAttributeFlags ) ;
gboolean zMapGFFFormatAttributeSetText(ZMapGFFAttributeFlags ) ;
gboolean zMapGFFFormatAttributeUnsetAll(ZMapGFFAttributeFlags) ;
gboolean zMapGFFOutputWriteLineToGIO(GIOChannel *gio_channel, char **err_msg_out, GString *line, gboolean truncate_after_write) ;
gboolean zMapGFFFormatAppendAttribute(GString *, GString *, gboolean, gboolean) ;
char zMapGFFFormatStrand2Char(ZMapStrand strand) ;
char zMapGFFFormatPhase2Char(ZMapPhase phase) ;
gboolean zMapGFFFormatHeader(gboolean, GString *, const char *, int, int) ;
gboolean zMapGFFFormatMandatory(gboolean, GString *, const char *, const char *, const char *,
  int, int, float, ZMapStrand, ZMapPhase, gboolean, gboolean ) ;
gboolean zMapGFFDumpVersionSet(ZMapGFFVersion gff_version ) ;
ZMapGFFVersion zMapGFFDumpVersionGet() ;
gboolean zMapGFFDump(ZMapFeatureAny dump_set, GHashTable *styles, GIOChannel *file, GError **error_out);
gboolean zMapGFFDumpRegion(ZMapFeatureAny dump_set, GHashTable *styles,
  ZMapSpan region_span, GIOChannel *file, GError **error_out) ;
gboolean zMapGFFDumpList(GList *dump_list, GHashTable *styles, char *sequence,
  GIOChannel *file, GString *text_out, GError **error_out) ;
gboolean zMapGFFDumpFeatureSets(ZMapFeatureAny, GHashTable *, GList*, ZMapSpan,
  GIOChannel *, GError **) ;

gboolean zMapWriteAttributeURL(ZMapFeature, GString *) ;
gboolean zMapWriteAttributeName(ZMapFeature, GString *) ;
gboolean zMapWriteAttributeNote(ZMapFeature, GString *) ;
gboolean zMapWriteAttributeSequence(ZMapFeature, GString *, const char *) ;
gboolean zMapWriteAttributeLocus(ZMapFeature, GString * ) ;
gboolean zMapWriteAttributeID(ZMapFeature, GString *) ;
gboolean zMapWriteAttributeParent(ZMapFeature, GString *) ;
gboolean zMapWriteAttributeVariation(ZMapFeature, GString *) ;
gboolean zMapWriteAttributeTarget(ZMapFeature, GString *) ;
gboolean zMapWriteAttributePercentID(ZMapFeature, GString *) ;
gboolean zMapWriteAttributeGap(ZMapFeature, GString *) ;
gboolean zMapWriteAttributeEvidence(ZMapFeature, GString *) ;

#endif /* ZMAP_GFF_H */
