/*  File: zmapWindowCanvasGlyph.h
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
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
