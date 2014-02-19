/*  File: zmapWindowCanvasGlyph_I.h
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2014: Genome Research Ltd.
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
 * Description: Private header for glyph "objects".
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CANVAS_GLYPH_I_H
#define ZMAP_CANVAS_GLYPH_I_H


#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasGlyph.h>

#define INCLUDE_TRUNCATION_GLYPHS 1

gboolean zmap_window_canvas_set_glyph(FooCanvasItem *foo,
					     ZMapWindowCanvasGlyph glyph, ZMapFeatureTypeStyle style,
					     ZMapFeature feature, double col_width, double score);

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

/*
 * Enum for the glyph "which" data member. Represents 3', 5' or
 * truncated at start or end. Note that the numerical values are
 * important here as they are tested for explicitly in much of
 * the drawing code.
 */
typedef enum
  {
    ZMAP_GLYPH_THREEPRIME      = 3,
    ZMAP_GLYPH_FIVEPRIME       = 5,
    ZMAP_GLYPH_TRUNCATED_START = 999,
    ZMAP_GLYPH_TRUNCATED_END   = 1000
  } ZMapGlyphWhichType ;


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
typedef struct _zmapWindowCanvasGlyphStruct
{
  zmapWindowCanvasFeatureStruct feature ;		    /* all the common stuff */
  GQuark sig ;						    /* signature: for debugging */

  gboolean coords_set ;
  gboolean use_glyph_colours ;				    /* only set if threshold ALT colour selected */
  gboolean sub_feature ;				    /* or free standing? */

  /* scale by this factor, -ve values imply flipping around the anchor point,
   * used for all glyphs except splices. */
  double width, height ;

  double origin ;                                           /* relative to the centre of the column */

  int which ;						    /* generic or 5' or 3' ? */

  ZMapStyleGlyphShape shape ;				    /* pointer to relevant style shape struct */

  GdkPoint coords[GLYPH_SHAPE_MAX_COORD] ;		    /* derived from style->shape struct
							       but adjusted for scale etc */
  GdkPoint points[GLYPH_SHAPE_MAX_COORD] ;		    /* offset to the canvas for gdk_draw */

  /* non selected colours */
  gboolean line_set ;
  gboolean area_set ;

  gulong line_pixel ;
  gulong area_pixel ;

} zmapWindowCanvasGlyphStruct ;




#endif /* !ZMAP_CANVAS_GLYPH_I_H */
