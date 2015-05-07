/*  File: zmapWindowCanvasGlyph.h
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
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
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * implements callback functions for FeaturesetItem glyph features
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CANVAS_GLYPH_H
#define ZMAP_CANVAS_GLYPH_H


/* Opaque pointer to glyph struct. */
typedef struct ZMapWindowCanvasGlyphStructType *ZMapWindowCanvasGlyph ;


/* Enum for glyph 5' or 3' orientation, start or end truncation or junction. */
typedef enum
  {
    ZMAP_GLYPH_NONE,
    ZMAP_GLYPH_THREEPRIME,
    ZMAP_GLYPH_FIVEPRIME,
    ZMAP_GLYPH_TRUNCATED_START,
    ZMAP_GLYPH_TRUNCATED_END,
    ZMAP_GLYPH_JUNCTION_START,
    ZMAP_GLYPH_JUNCTION_END
  } ZMapGlyphWhichType ;


/* HACK TEMP. FUNCTION.... */
void zMapWindowCanvasGlyphGetTruncationShapes(ZMapStyleGlyphShape *start, ZMapStyleGlyphShape *end) ;


void zMapWindowCanvasGlyphInit(void) ;
ZMapWindowCanvasGlyph zMapWindowCanvasGlyphAlloc(ZMapStyleGlyphShape shape, ZMapGlyphWhichType which,
                                                 gboolean scale_to_feature_width, gboolean sub_feature) ;

gboolean zMapWindowCanvasGlyphSetColours(ZMapWindowCanvasGlyph glyph, gulong line_pixel, gulong area_pixel) ;

ZMapWindowCanvasGlyph zMapWindowCanvasGetGlyph(ZMapWindowFeaturesetItem featureset,
					       ZMapFeatureTypeStyle style,

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                                               ZMapFeature feature,
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
                                               ZMapStrand feature_strand,
					       ZMapGlyphWhichType which, double score) ;

void zMapWindowCanvasGlyphDrawGlyph(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                                    ZMapFeatureTypeStyle style, ZMapWindowCanvasGlyph glyph, GdkDrawable *drawable,
                                    double col_width, double optional_ypos) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
void zMapWindowCanvasGlyphDrawTruncationGlyph(FooCanvasItem *foo, ZMapWindowFeaturesetItem featureset,
                                              ZMapWindowCanvasFeature feature, ZMapFeatureTypeStyle style,
                                              ZMapWindowCanvasGlyph glyph,
                                              GdkDrawable *drawable, double col_width) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
void zMapWindowCanvasGlyphTruncationGlyph(FooCanvasItem *foo, ZMapWindowFeaturesetItem featureset,
                                          ZMapWindowCanvasFeature feature, ZMapFeatureTypeStyle style,
                                          ZMapWindowCanvasGlyph glyph,
                                          GdkDrawable *drawable, double col_width) ;

void zMapWindowCanvasGlyphPaintSubFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
					  ZMapWindowCanvasGlyph glyph, GdkDrawable *drawable) ;
void zMapWindowCanvasGlyphSetColData(ZMapWindowFeaturesetItem featureset) ;

void zMapWindowCanvasGlyphGetJunctionShapes(ZMapStyleGlyphShape *start_out, ZMapStyleGlyphShape *end_out) ;

#endif /* !ZMAP_CANVAS_GLYPH_H */
