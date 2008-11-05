/*  File: zmapGFF.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * Last edited: Nov  4 09:49 2008 (rds)
 * Created: Sat May 29 13:18:32 2004 (edgrif)
 * CVS info:   $Id: zmapGFF.h,v 1.14 2008-11-05 12:12:44 rds Exp $
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


ZMapGFFParser zMapGFFCreateParser(GData *sources, gboolean parse_only) ;
gboolean zMapGFFParseHeader(ZMapGFFParser parser, char *line, gboolean *header_finished) ;
gboolean zMapGFFParseLine(ZMapGFFParser parser, char *line) ;
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

gboolean zMapGFFDump(ZMapFeatureAny dump_set, GIOChannel *file, GError **error_out);

#endif /* ZMAP_GFF_H */
