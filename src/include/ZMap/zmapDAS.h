/*  File: zmapDAS.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: May 26 19:30 2006 (rds)
 * Created: Mon May 22 13:14:24 2006 (rds)
 * CVS info:   $Id: zmapDAS.h,v 1.3 2010-03-04 15:14:58 mh17 Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_DAS_H
#define ZMAP_DAS_H

#include <ZMap/zmapXML.h>

typedef enum
  {
    ZMAP_DAS_UNKNOWN,             /* DOESN'T even remotely look like DAS */

    ZMAP_DAS1_UNKNOWN,          /* DOESN'T look like DAS1 */
    ZMAP_DAS1_DSN,
    ZMAP_DAS1_ENTRY_POINTS,
    ZMAP_DAS1_DNA,
    ZMAP_DAS1_SEQUENCE,
    ZMAP_DAS1_TYPES,
    ZMAP_DAS1_FEATURES,         /* das not zmap like this one */
    ZMAP_DAS1_LINK,
    ZMAP_DAS1_STYLESHEET,

    ZMAP_DAS2_UNKNOWN          /* DOESN'T look like DAS2 */
  }ZMapDAS1QueryType;

typedef enum 
  {
    ZMAP_DAS1_NONE_GLYPH,
    ZMAP_DAS1_ARROW_GLYPH,
    ZMAP_DAS1_ANCHORED_ARROW_GLYPH,
    ZMAP_DAS1_BOX_GLYPH,
    ZMAP_DAS1_CROSS_GLYPH,
    ZMAP_DAS1_DOT_GLYPH,
    ZMAP_DAS1_EX_GLYPH,
    ZMAP_DAS1_HIDDEN_GLYPH,
    ZMAP_DAS1_LINE_GLYPH,
    ZMAP_DAS1_SPAN_GLYPH,
    ZMAP_DAS1_TEXT_GLYPH,
    ZMAP_DAS1_TRIANGLE_GLYPH,
    ZMAP_DAS1_TOOMANY_GLYPH,
    ZMAP_DAS1_PRIMERS_GLYPH
  } ZMapDAS1GlyphType;

typedef enum
  {
    ZMAP_DAS1_LINE_NO_STYLE,
    ZMAP_DAS1_LINE_HAT_STYLE,
    ZMAP_DAS1_LINE_DASHED_STYLE
  } ZMapDAS1GlyphLineStyle;

typedef struct _ZMapDAS1ParserStruct *ZMapDAS1Parser;
typedef struct _ZMapDAS2ParserStruct *ZMapDAS2Parser;

typedef struct
{
  GQuark href;
  GQuark version;
  GList *segments;
}ZMapDAS1EntryPointStruct, *ZMapDAS1EntryPoint;

typedef struct
{
  GQuark type_id, method, category, source;
}ZMapDAS1TypeStruct, *ZMapDAS1Type;


typedef struct
{
  GQuark source_id, version, name, map, desc;
  ZMapDAS1EntryPoint entry_point;
}ZMapDAS1DSNStruct, *ZMapDAS1DSN;

typedef struct
{
  char *sequence;
}ZMapDAS1DNAStruct, *ZMapDAS1DNA;

typedef struct
{
  GQuark group_id, type;
}ZMapDAS1GroupStruct, *ZMapDAS1Group;
typedef struct
{
  GQuark feature_id, label;

  GQuark type_id;
  ZMapDAS1Type type;

  GQuark method_id;

  int start, end, phase;
  double score;

  GQuark orientation;

  GList *groups;

#ifdef RDS_DONT_INCLUDE
  ZMapDAS1Group group;          /* fearing multiple.... */
#endif /* RDS_DONT_INCLUDE */

}ZMapDAS1FeatureStruct, *ZMapDAS1Feature;

typedef struct
{
  ZMapDAS1GlyphType type;
  /* Common attributes */
  int height;
  GQuark fg, bg;                /* colours */
  gboolean label, bump;
  /* type specific */
  union{

    struct{
      gboolean parallel;
    }arrow;                     /* both arrow and anchored_arrow */

    struct{
      int linewidth;
    }box;                       /* a box */

    struct{
      ZMapDAS1GlyphLineStyle style;
    }line;                      /* a line */

    struct{
      char *font;
      int font_size;
      char *string;
      int style;
    }text;                      /* text */

    struct{
      int linewidth;
    }toomany;                   /* whatever toomany is */

    struct{
      int linewidth;
      char *direction;
    }triangle;                  /* a triangle pointing N,S,E or W */

  }shape;

}ZMapDAS1GlyphStruct, *ZMapDAS1Glyph;

typedef struct
{
  GQuark id;
  GList *types;
}ZMapDAS1StyleCategoryStruct, *ZMapDAS1StyleCategory; 

typedef struct
{
  GQuark id;
  GList *glyphs;
}ZMapDAS1StyleTypeStruct, *ZMapDAS1StyleType;

typedef struct
{
  GQuark stylesheet_version;
  GList *categories;
}ZMapDAS1StylesheetStruct, *ZMapDAS1Stylesheet;

typedef struct
{
  GQuark id, type, orientation, desc;
  gint start, stop;
  unsigned int is_reference : 1;
  unsigned int has_subparts : 1;
  unsigned int has_supparts : 1;
}ZMapDAS1SegmentStruct, *ZMapDAS1Segment;

typedef void (*ZMapDAS1DSNCB)        (ZMapDAS1DSN dsn,                gpointer user_data);
typedef void (*ZMapDAS1DNACB)        (ZMapDAS1DNA dna,                gpointer user_data);
typedef void (*ZMapDAS1EntryPointsCB)(ZMapDAS1EntryPoint entry_point, gpointer user_data);
typedef void (*ZMapDAS1TypesCB)      (ZMapDAS1Type type,              gpointer user_data);
typedef void (*ZMapDAS1FeaturesCB)   (ZMapDAS1Feature feature,        gpointer user_data);
typedef void (*ZMapDAS1StylesheetCB) (ZMapDAS1Stylesheet style,       gpointer user_data);


ZMapDAS1Parser zMapDAS1ParserCreate(ZMapXMLParser parser);

gboolean zMapDAS1ParserDSNPrepareXMLParser(ZMapDAS1Parser das, 
                                           ZMapDAS1DSNCB dsn_callback, 
                                           gpointer user_data);
gboolean zMapDAS1ParserDNAPrepareXMLParser(ZMapDAS1Parser das, 
                                           ZMapDAS1DNACB dna_callback, 
                                           gpointer user_data);
gboolean zMapDAS1ParserEntryPointsPrepareXMLParser(ZMapDAS1Parser das, 
                                                   ZMapDAS1EntryPointsCB entry_point_callback, 
                                                   gpointer user_data);
gboolean zMapDAS1ParserTypesPrepareXMLParser(ZMapDAS1Parser das, 
                                             ZMapDAS1TypesCB type_callback, 
                                             gpointer user_data);
gboolean zMapDAS1ParserFeaturesPrepareXMLParser(ZMapDAS1Parser das, 
                                                ZMapDAS1FeaturesCB feature_callback, 
                                                gpointer user_data);
gboolean zMapDAS1ParserStylesheetPrepareXMLParser(ZMapDAS1Parser das, 
                                                  ZMapDAS1StylesheetCB style_callback, 
                                                  gpointer user_data);
void zMapDAS1ParserDestroy(ZMapDAS1Parser das);
gboolean zMapDAS1DSNSegments(ZMapDAS1DSN dsn, GList **segments_out);
char *zMapDAS1DSNRefSegmentsQueryString(ZMapDAS1DSN dsn, char *separator);


#endif /* ZMAP_DAS_H */
