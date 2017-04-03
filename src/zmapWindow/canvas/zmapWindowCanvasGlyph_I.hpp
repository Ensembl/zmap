/*  File: zmapWindowCanvasGlyph_I.h
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
 * Description: Private header for glyph "objects".
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CANVAS_GLYPH_I_H
#define ZMAP_CANVAS_GLYPH_I_H


#include <zmapWindowCanvasFeature_I.hpp>
#include <zmapWindowCanvasGlyph.hpp>

/* Per Column Glyph Data.
 *
 * Data common to particular types of glyph, stored using the per_column_data pointer
 * in zmapWindowFeaturesetItemStruct.
 */

typedef enum
  {
    ZMAP_GLYPH_SHAPE_INVALID,
    ZMAP_GLYPH_SHAPE_ANY,				    /* glyphs with no special data */
    ZMAP_GLYPH_SHAPE_GFSPLICE,				    /* acedb-style GF Splices */
  } ZMapGlyphShapeType ;


/* General struct for per column data, all other structs must have matching fields at the
 * start of their structs. */
typedef struct GlyphAnyColumnDataStructName
{
  ZMapGlyphShapeType glyph_type ;

  double min_score, max_score ;				    /* cached from style. */


} GlyphAnyColumnDataStruct, *GlyphAnyColumnData ;



/* Genefinder Splice marker column data. */
#define GF_ZERO_LINE_COLOUR   "dark slate grey"
#define GF_OTHER_LINE_COLOUR  "light grey"


typedef struct GFSpliceColumnDataStructName
{
  /* All glyph fields. */

  ZMapGlyphShapeType glyph_type ;

  double min_score, max_score ;				    /* cached from style. */


  /* GF Splice only fields below. */

  double col_width ;
  double min_size ;					    /* Min. size for glyph so it's easily clickable. */
  double scale_factor ;					    /* Scaling factor for glyphs. */

  double origin ;					    /* score == 0 position across column. */

  /* Graph line colours to show origin, 50% & 75%. */
  gboolean colours_set ;
  GdkColor zero_line_colour ;
  GdkColor other_line_colour ;

} GFSpliceColumnDataStruct, *GFSpliceColumnData ;





/* Per glyph data.
 *
 * NOTE
 * Glyphs are quite complex and we create an array of points for display
 * that are interpreted according to the glyph type.
 * As the CanvasFeatureset code is generic we start with a feature pointer
 * and evaluate the points on demand and cache these. (lazy evaluation)
 */
typedef struct ZMapWindowCanvasGlyphStructType
{
  zmapWindowCanvasFeatureStruct feature ;		    /* all the common stuff */
  GQuark sig ;						    /* signature: for debugging */

  gboolean coords_set ;
  gboolean use_glyph_colours ;				    /* only set if threshold ALT colour selected */
  gboolean sub_feature ;				    /* or free standing? */

  /* scale by this factor, -ve values imply flipping around the anchor point,
   * used for all glyphs except splices. If to_feature_width is TRUE then width of glyph
   * is scaled to feature width. */
  gboolean scale_to_feature_width ;
  double width, height ;

  double origin ;                                           /* relative to the centre of the column */

  ZMapGlyphWhichType which ;                                /* generic or 5' or 3' ? */

  ZMapStyleGlyphShape shape ;				    /* pointer to relevant style shape struct */

  GdkPoint coords[GLYPH_SHAPE_MAX_COORD] ;		    /* derived from style->shape struct
							       but adjusted for scale etc */
  GdkPoint points[GLYPH_SHAPE_MAX_COORD] ;		    /* offset to the canvas for gdk_draw */

  /* non selected colours */
  gboolean border_set ;
  gboolean fill_set ;

  gulong border_pixel ;
  gulong fill_pixel ;

} ZMapWindowCanvasGlyphStruct ;




#endif /* !ZMAP_CANVAS_GLYPH_I_H */
