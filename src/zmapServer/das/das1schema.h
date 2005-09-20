/*  File: das1schema.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Sep 15 18:15 2005 (rds)
 * Created: Wed Aug 31 15:59:12 2005 (rds)
 * CVS info:   $Id: das1schema.h,v 1.4 2005-09-20 17:16:11 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef DAS1SCHEMA_H
#define DAS1SCHEMA_H

#include <glib.h>

#ifdef DAS_USE_GLYPHS
typedef enum {
  DASONE_GA_HEIGHT    = 1 >> 1,
  DASONE_GA_FGCOLOR   = 1 >> 2,
  DASONE_GA_BGCOLOR   = 1 >> 3,
  DASONE_GA_LABEL     = 1 >> 4,
  DASONE_GA_BUMP      = 1 >> 5,
  DASONE_GA_PARALLEL  = 1 >> 6,
  DASONE_GA_LINEWIDTH = 1 >> 7,
  DASONE_GA_STYLE     = 1 >> 8,
  DASONE_GA_FONT      = 1 >> 9,
  DASONE_GA_FONTSIZE  = 1 >> 10,
  DASONE_GA_STRING    = 1 >> 11.
  DASONE_GA_DIRECTION = 1 >> 12,
}dasOneGlyphAttributes;
#endif /* DAS_USE_GLYPHS */

typedef enum {
  DASONE_GLYPH_NONE,
  DASONE_GLYPH_ARROW,
  DASONE_GLYPH_ANCHORED_ARROW,
  DASONE_GLYPH_BOX,
  DASONE_GLYPH_CROSS,
  DASONE_GLYPH_EX,
  DASONE_GLYPH_HIDDEN,
  DASONE_GLYPH_LINE,
  DASONE_GLYPH_SPAN,
  DASONE_GLYPH_TEXT,
  DASONE_GLYPH_TRIANGLE,
  DASONE_GLYPH_TOOMANY,
  DASONE_GLYPH_PRIMERS
} dasOneGlyphType;

typedef enum {
  DASONE_REF_UNSET       = 0,
  DASONE_REF_ISREFERENCE = 1 << 0,
  DASONE_REF_HASSUBPARTS = 1 << 1,
  DASONE_REF_HASSUPPARTS = 1 << 2
} dasOneRefProperties;

typedef struct _dasOneGlyphStruct
{
  dasOneGlyphType type;
  /* Further decide on this later as it's not important now */
}dasOneGlyphStruct, *dasOneGlyph;

typedef struct _dasOneStylesheetStruct
{
  GQuark type;
  dasOneGlyph glyph;
}dasOneStylesheetStruct, *dasOneStylesheet;

typedef struct _dasOneGroupStruct *dasOneGroup;

typedef struct _dasOneMethodStruct *dasOneMethod;

typedef struct _dasOneTargetStruct *dasOneTarget;

typedef struct _dasOneTypeStruct *dasOneType;

typedef struct _dasOneFeatureStruct *dasOneFeature;

typedef struct _dasOneSequenceStruct *dasOneSequence;

typedef struct _dasOneSegmentStruct *dasOneSegment;

typedef struct _dasOneEntryPointStruct *dasOneEntryPoint;

typedef struct _dasOneDSNStruct *dasOneDSN;

/* =========== */
/* DSN methods */

dasOneDSN dasOneDSN_create(char *id, char *version, char *name);
dasOneDSN dasOneDSN_create1(GQuark id, GQuark version, GQuark name);
void dasOneDSN_setAttributes(dasOneDSN dsn, char *mapmaster, char *description);
void dasOneDSN_setAttributes1(dasOneDSN dsn, GQuark mapmaster, GQuark description);
dasOneEntryPoint dasOneDSN_entryPoint(dasOneDSN dsn, dasOneEntryPoint ep);

GQuark dasOneDSN_id1(dasOneDSN dsn);

void dasOneDSN_free(dasOneDSN dsn);

/* ================== */
/* EntryPoint methods */
/* These maybe seem a little pointless ATM */
dasOneEntryPoint dasOneEntryPoint_create(char *href, char *version);
dasOneEntryPoint dasOneEntryPoint_create1(GQuark href, GQuark version);
void dasOneEntryPoint_addSegment(dasOneEntryPoint ep, dasOneSegment seg);
GList *dasOneEntryPoint_getSegmentsList(dasOneEntryPoint ep);
void dasOneEntryPoint_free(dasOneEntryPoint ep);

/* =============== */
/* Segment methods */

dasOneSegment dasOneSegment_create(char *id, int start, 
                                   int stop, char *type, 
                                   char *orientation);
dasOneSegment dasOneSegment_create1(GQuark id, int start, 
                                    int stop, GQuark type, 
                                    GQuark orientation);
GQuark dasOneSegment_id1(dasOneSegment seg);
gboolean dasOneSegment_getBounds(dasOneSegment seg, 
                                 int *start_out, int *end_out);

dasOneRefProperties dasOneSegment_refProperties(dasOneSegment seg, 
                                                char *isRef,
                                                char *subparts,
#ifdef SEGMENTS_HAVE_SUPERPARTS
                                                char *superparts,
#endif /* SEGMENTS_HAVE_SUPERPARTS */
                                                gboolean set);

char *dasOneSegment_description(dasOneSegment seg, char *desc);
void dasOneSegment_free(dasOneSegment seg);

/* ================ */
/* Sequence methods */
dasOneSequence dasOneSequence_create(char *id, 
                                     int start, int stop, 
                                     char *version);
dasOneSequence dasOneSequence_create1(GQuark id, 
                                      int start, int stop, 
                                      GQuark version);
void dasOneSequence_setMolType(dasOneSequence seq, char *molecule);
void dasOneSequence_setSequence(dasOneSequence seq, char *sequence);
void dasOneSequence_free(dasOneSequence seq);


/* =============== */
/* Feature Methods */
dasOneFeature dasOneFeature_create1(GQuark id, GQuark label);
GQuark dasOneFeature_id1(dasOneFeature feature);
gboolean dasOneFeature_getLocation(dasOneFeature feature,
                                  int *start_out, int *stop_out);
void dasOneFeature_setProperties(dasOneFeature feature, 
                                 int start, int stop,
                                 char *score, char *orientation,
                                 char *phase);
void dasOneFeature_setTarget(dasOneFeature feature, 
                             dasOneTarget target);
gboolean dasOneFeature_getTargetBounds(dasOneFeature feature, 
                                       int *start_out, int *stop_out);
void dasOneFeature_setType(dasOneFeature feature, 
                           dasOneType type);
void dasOneFeature_free(dasOneFeature feat);

/* ============ */
/* Type Methods */
dasOneType          dasOneType_create1(GQuark id, GQuark name, GQuark category);
dasOneRefProperties dasOneType_refProperties(dasOneType type, char *ref, 
                                             char *sub, char *super, 
                                             gboolean set);
void dasOneType_free(dasOneType type);

/* ============== */
/* Method Methods */
dasOneMethod dasOneMethod_create(char *id_or_name);
void dasOneMethod_free(dasOneMethod meth);

/* ============== */
/* Target Methods */
dasOneTarget dasOneTarget_create1(GQuark id, int start, int stop);
void dasOneTarget_free(dasOneTarget target);

/* ============= */
/* Group Methods */
dasOneGroup dasOneGroup_create1(GQuark id, GQuark type, GQuark label);
GQuark dasOneGroup_id1(dasOneGroup grp);

#endif /* DAS1SCHEMA_H */
