/*  File: zmapWindowCanvasGlyph.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
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
#ifndef ZMAP_CANVAS_GLYPH_I_H
#define ZMAP_CANVAS_GLYPH_I_H

#include <ZMap/zmap.h>


#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasGlyph.h>



/* 
 * Data common to particular types of glyph, stored using the per_column_data pointer
 * in zmapWindowFeaturesetItemStruct.
 */

/* Splice marker column data. */
#define ZERO_LINE_COLOUR   "dark slate grey"
#define OTHER_LINE_COLOUR  "light grey"

typedef struct GlyphSpliceColumnDataStructName
{
  double glyph_len ;					    /* Standard glyph size, all scaled from this. */
  double col_width ;
  double min_size ;					    /* Min. size for glyph so it's easily clickable. */
  double scale_factor ;					    /* Scaling factor for glyphs. */

  double origin ;					    /* score == 0 position across column. */

  gboolean colours_set ;
  GdkColor zero_line_colour ;
  GdkColor other_line_colour ;

} GlyphSpliceColumnDataStruct, *GlyphSpliceColumnData ;



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

  /* Used for all glyphs except splices. */
  double width, height ; 	/* scale by this factor, -ve values imply flipping around the anchor point */
  double origin ;		/* relative to the centre of the column */

  int which ;						    /* generic or 5' or 3' ? */
  ZMapStyleGlyphShape shape ;				    /* pointer to relevant style shape struct */
  GdkPoint coords[GLYPH_SHAPE_MAX_COORD] ;		    /* derived from style->shape struct
							       but adjusted for scale etc */
  GdkPoint points[GLYPH_SHAPE_MAX_COORD] ;		    /* offset to the canvas for gdk_draw */

  /* non selected colours */
  gulong line_pixel ;
  gulong area_pixel ;
  gboolean line_set ;
  gboolean area_set ;

} zmapWindowCanvasGlyphStruct ;




#endif /* !ZMAP_CANVAS_GLYPH_I_H */
