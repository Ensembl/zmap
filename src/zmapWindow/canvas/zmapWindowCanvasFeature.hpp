/*  File: zmapWindowCanvasFeature.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Public header to operations on features within a column.
 *              The features are not ZMapFeatures but display objects
 *              that represent and contain ZMapFeatures.
 *
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_FEATURE_H
#define ZMAP_WINDOW_FEATURE_H


#include <ZMap/zmapEnum.hpp>


/* enums for feature function lookup for each feature type.
 * NOTE these are set from the style mode but are defined separately as
 * CanvasFeaturesets do not initially handle all style modes.
 * See  zMapWindowFeaturesetAddItem()
 *
 * genomic features are FEATURE_BASIC to FEATURE_GLYPH
 *
 * FEATURE_GRAPHICS onwards are unadorned graphics primitives and FEATURE_GRAPHICS
 * is a catch-all for the featureset type. NOTE that FEATURE_GRAPHICS is used
 * in the code to test for feature vs. graphic items.
 */
#define ZMAP_CANVASFEATURE_TYPE_LIST(_)         \
_(FEATURE_INVALID,    , "invalid", "invalid type", "")           \
_(FEATURE_BASIC,    , "basic", "Basic feature", "")               \
_(FEATURE_ALIGN,    , "align", "Alignment feature", "")                   \
_(FEATURE_TRANSCRIPT,    , "transcript", "Transcript", "")              \
_(FEATURE_SEQUENCE,    , "sequence", "Sequence", "")                \
_(FEATURE_ASSEMBLY,    , "assembly", "Assembly", "")                \
_(FEATURE_LOCUS,    , "locus", "Locus", "")                   \
_(FEATURE_GRAPH,    , "graph", "Graph", "")                   \
_(FEATURE_GLYPH,    , "glyph", "Glyph", "")                                   \
_(FEATURE_GRAPHICS,    , "graphics", "Graphics", "") \
_(FEATURE_LINE,    , "line", "Line", "") \
_(FEATURE_BOX,    , "box", "Box", "") \
_(FEATURE_TEXT,    , "text", "Text", "") \
_(FEATURE_N_TYPE,    , "num_types", "Number of CanvasFeature types", "")

ZMAP_DEFINE_ENUM(zmapWindowCanvasFeatureType, ZMAP_CANVASFEATURE_TYPE_LIST) ;

typedef gint (FeatureCmpFunc)(gconstpointer a, gconstpointer b) ;

/* ZMapWindowCanvasFeature class struct. */
typedef struct ZMapWindowCanvasFeatureClassStructType *ZMapWindowCanvasFeatureClass ;


/* ZMapWindowCanvasFeature instance struct. */
typedef struct _zmapWindowCanvasFeatureStruct *ZMapWindowCanvasFeature ;



void zMapWindowCanvasFeatureInit(void) ;
void zMapWindowCanvasFeatureSetSize(int featuretype, gpointer *feature_funcs, size_t feature_struct_size) ;
ZMapWindowCanvasFeature zMapWindowCanvasFeatureAlloc(zmapWindowCanvasFeatureType type) ;
ZMapFeature zMapWindowCanvasFeatureGetFeature(ZMapWindowCanvasFeature feature) ;
gboolean zMapWindowCanvasFeatureGetFeatureExtent(ZMapWindowCanvasFeature feature, gboolean is_complex,
                                                 ZMapSpan span, double *width) ;
ZMapFeatureSubPart zMapWindowCanvasFeaturesetGetSubPart(FooCanvasItem *foo,
                                                        ZMapFeature feature, double x, double y) ;
void zMapWindowCanvasFeatureAddSplicePos(ZMapWindowCanvasFeature feature_item, int splice_pos,
                                         gboolean match, ZMapBoundaryType boundary_type) ;
void zMapWindowCanvasFeatureRemoveSplicePos(ZMapWindowCanvasFeature feature_item) ;
GString *zMapWindowCanvasFeature2Txt(ZMapWindowCanvasFeature canvas_feature) ;
double zMapWindowCanvasFeatureGetBumpOffset(ZMapWindowCanvasFeature canvas_feature) ;
double zMapWindowCanvasFeatureGetWidth(ZMapWindowCanvasFeature canvas_feature) ;

gint zMapWindowFeatureCmp(gconstpointer a, gconstpointer b) ;
gint zMapWindowFeatureFullCmp(gconstpointer a, gconstpointer b) ;
gint zMapFeatureNameCmp(gconstpointer a, gconstpointer b) ;
gint zMapFeatureSetNameCmp(gconstpointer a, gconstpointer b) ;


#endif /* ZMAP_WINDOW_FEATURE_H */
