/*  File: zmapWindowCanvasGlyph.c
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
 *-------------------------------------------------------------------
 * ZMap is free software; you can refeaturesetstribute it and/or
 * mofeaturesetfy it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is featuresetstributed in the hope that it will be useful,
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

#include <ZMap/zmap.h>

#include <math.h>
#include <string.h>

#include <ZMap/zmapFeature.h>
#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasGlyph_I.h>



/* glyph line widths. */
enum {DEFAULT_LINE_WIDTH = 1, SPLICE_LINE_WIDTH = 3} ;



/*
 * Drawing glyphs is dissapointingly complex
 * they are pixel based (same size always)
 * and defined by a random series of points
 * and can include line breaks and optional fill colours
 * But we do limit the number of points to 16
 */

/* we may want to call some of these functions from other features eg alignments */
/* this goes via direct function calls rather than the CanvasFeatureset virtual function arrays
   this module and CanvasAlignment are in parallel so it's kind of structured,
   (could yank the common code into another file if anyone gets precious about it)
 */

static ZMapStyleGlyphShape get_glyph_shape(ZMapFeatureTypeStyle style, int which, ZMapStrand strand) ;
static gboolean zmap_window_canvas_set_glyph(FooCanvasItem *foo,
					     ZMapWindowCanvasGlyph glyph, ZMapFeatureTypeStyle style,
					     ZMapFeature feature, double col_width, double score);
static void glyph_set_coords(ZMapWindowCanvasGlyph glyph);
static void glyph_to_canvas(GdkPoint *points, GdkPoint *coords, int count,int cx, int cy);
static void zmap_window_canvas_glyph_draw (ZMapWindowFeaturesetItem featureset,
					   ZMapWindowCanvasGlyph glyph, GdkDrawable *drawable, gboolean fill);
static void zmap_window_canvas_paint_feature_glyph(ZMapWindowFeaturesetItem featureset,
						   ZMapWindowCanvasFeature feature,
						   ZMapWindowCanvasGlyph glyph, double y1, GdkDrawable *drawable);

static void zMapWindowCanvasGlyphPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
				       GdkDrawable *drawable, GdkEventExpose *expose);

static ZMapWindowCanvasFeature zMapWindowCanvasGlyphAddFeature(ZMapWindowFeaturesetItem featureset,
							       ZMapFeature feature, double y1, double y2);

static void calcSplicePos(ZMapWindowFeaturesetItem featureset, ZMapFeature feature,
			  ZMapFeatureTypeStyle style, double score,
			  double *origin_out, double *width_out, double *height_out) ;

static void glyphColumnInit(ZMapWindowFeaturesetItem featureset) ;
static void glyphSetPaint(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable, GdkEventExpose *expose) ;
//static void glyphZoom(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable) ;
static double glyphPoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
			 double item_x, double item_y, int cx, int cy,
			 double x, double y, double x_off) ;
static void glyphColumnFree(ZMapWindowFeaturesetItem featureset) ;


/*
 * Caching glyphs
 *
 * alignments are the most populous features and may have a lot of glyphs attached when bumped
 * to avoid excessive memory use and pointless recalculation of glyph shapes
 * we cache thse if they have limited variability, determined by a signature.
 *
 * as glyphs are pixel based features and generally quite small there is the possibility to do this
 * even for fully variable configurations, imposing a maximum limit on the memory used and also display time
 * (not done in first implementation)
 *
 * Cached glyphs are held in a module global hash table and never freed
 */

GHashTable *glyph_cache_G = NULL;




/*
 *                 External interface
 */


void zMapWindowCanvasGlyphInit(void)
{
  gpointer funcs[FUNC_N_FUNC] = { NULL };

  funcs[FUNC_SET_INIT] = glyphColumnInit ;
  funcs[FUNC_SET_PAINT] = glyphSetPaint ;
//  funcs[FUNC_ZOOM] = glyphZoom ;
  funcs[FUNC_PAINT] = zMapWindowCanvasGlyphPaintFeature ;
  funcs[FUNC_POINT] = glyphPoint ;
  funcs[FUNC_FREE] = glyphColumnFree ;
  funcs[FUNC_ADD] = zMapWindowCanvasGlyphAddFeature ;

  zMapWindowCanvasFeatureSetSetFuncs(FEATURE_GLYPH, funcs, sizeof(zmapWindowCanvasGlyphStruct), 0);

  return ;
}



/* get an id for a simple glyph variant or NULL if it's too complex
 * we can get a the same shape in diff styles which may appear in diff columns
 * but glyphs are always the same size if this does not relate to feature score
 * so we can cache globally by glyph shape id
 */
GQuark zMapWindowCanvasGlyphSignature(ZMapFeatureTypeStyle style, ZMapFeature feature, int which, double score)
{
  ZMapStyleGlyphShape shape;
  char buf[64];	/* will never overflow */
  char strand = '+';
  char alt = 'N';

  if(!style)
    return 0;
  if(style->score_mode != ZMAPSCORE_INVALID && style->score_mode != ZMAPSTYLE_SCORE_ALT)
    return 0;

  shape = get_glyph_shape(style, which, feature->strand) ;

  if(!shape)
    return 0;

  if(feature->strand == ZMAPSTRAND_REVERSE)
    strand = '-';
  if(style->score_mode == ZMAPSTYLE_SCORE_ALT && score < zMapStyleGlyphThreshold(style))
    alt = 'A';

  sprintf(buf,"%s_%s%c%c",g_quark_to_string(style->unique_id),g_quark_to_string(shape->id),strand,alt);

  return g_quark_from_string(buf);
}


/* which is 5' or 3', will work if unspecified but not intentionally.... */
/* style is the type of glyph to draw driven by calling code, it's one of the feature's styles' sub-styles */
/* NOTE we return a canvas feature/ glyph even though we don't need the feature part */
/* NOTE this is not called for free standing glyphs */
ZMapWindowCanvasGlyph zMapWindowCanvasGetGlyph(ZMapWindowFeaturesetItem featureset,
					       ZMapFeatureTypeStyle style, ZMapFeature feature,
					       int which, double score)
{
  ZMapWindowCanvasGlyph glyph = NULL ;
  GQuark siggy ;
  double col_width ;

  siggy = zMapWindowCanvasGlyphSignature(style, feature, which, score) ;

  if (siggy)
    {
      if(glyph_cache_G)
	glyph = g_hash_table_lookup(glyph_cache_G,GUINT_TO_POINTER(siggy)) ;
      else
	glyph_cache_G = g_hash_table_new(NULL,NULL) ;
    }

  if (!glyph)
    {
      ZMapStyleGlyphShape shape ;
      FooCanvasItem *foo = (FooCanvasItem *)featureset ;

      shape = get_glyph_shape(style, which, feature->strand) ;

      if (!shape || shape->type == GLYPH_DRAW_INVALID || !shape->n_coords)
	{
	  zMapLogWarning("Could not find sub feature glyph shape for feature \"%s\" at %d, %d",
			 g_quark_to_string(feature->original_id), feature->x1, feature->x2) ;
	}
      else
	{
	  glyph = g_new0(zmapWindowCanvasGlyphStruct,1) ;		/* OK efficient if we really do cache these */

	  /* calulate shape when created and cached */

	  glyph->which = which ;
	  glyph->sub_feature = TRUE ;

	  glyph->shape = shape ;

	  col_width = zMapStyleGetWidth(featureset->style) ;
	  zmap_window_canvas_set_glyph(foo, glyph, style, feature, col_width, score) ;

	  if(siggy)
	    {
	      g_hash_table_insert(glyph_cache_G,GUINT_TO_POINTER(siggy),glyph) ;
	      glyph->sig = siggy ;
	      //zMapLogWarning("created glyph %s",g_quark_to_string(siggy)) ;
	    }
	}
    }

  return glyph ;
}



/* Draw handler for the glyph item */
/* the interface for free-standing glyphs, called via CanvasFeatureset virtual functions  */
/* must calculate the shape coords etc on first paint */
static void zMapWindowCanvasGlyphPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
				       GdkDrawable *drawable, GdkEventExpose *expose)
{
  ZMapWindowCanvasGlyph glyph = (ZMapWindowCanvasGlyph) feature;
  ZMapFeature feat = feature->feature;
  double y1;

#if 0
// now done in add function
  if (!glyph->coords_set)
    {
      /* for stand-alone glyphs which will be unset ie zero */
      /* NOTE this boundary code appears to be tied explcitly to GF splice features from ACEDB */
      if (zMapStyleIsSpliceStyle(feat->style))
	glyph->which = feat->boundary_type == ZMAPBOUNDARY_5_SPLICE ? 5 : 3;

      shape = get_glyph_shape(feat->style, glyph->which, feature->feature->strand) ;

      if(!shape || shape->type == GLYPH_DRAW_INVALID || !shape->n_coords)
	return;

      glyph->shape = shape;

      col_width = zMapStyleGetWidth(featureset->style);

      if(!zmap_window_canvas_set_glyph(foo, glyph, feat->style, feat, col_width, feat->score ))
	return;
    }
#endif

  /* Boundary features are usually given by the bases either side so the actual position
   * of the feature is given (counter-intuitively) by feat->x2 which is the point between
   * the two bases. (Remember that each base spans from coord -> coord+1) */
  if (feat->flags.has_boundary)
    {
      int diff ;

      diff = feat->x2 - feat->x1 ;

      if (diff == 1)
	y1 = feat->x2 ;
      else
	y1 = (feat->x1 + feat->x2) / 2 ;
    }
  else
    {
      y1 = feat->x1 ;
    }

  zmap_window_canvas_paint_feature_glyph(featureset, feature, glyph, y1, drawable);

  return ;
}



/* the interface for sub-feature glyphs, called directly */
/* glyphs are cached and we calculate the shape on first in zMapWindowCanvasGetGlyph() */
void zMapWindowCanvasGlyphPaintSubFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
					  ZMapWindowCanvasGlyph glyph, GdkDrawable *drawable)
{
  /* !glyph  likely to happen frequently....why ?? */

  if (glyph)
    {
      double y1;

      y1 = glyph->which == 3 ? feature->feature->x2 + 1 : feature->feature->x1;
      //zMapLogWarning("paint glyph %f,%f %d, %s",feature->y1,feature->y2, glyph->which,g_quark_to_string(glyph->sig));

      zmap_window_canvas_paint_feature_glyph(featureset, feature, glyph, y1, drawable) ;
    }

  return ;
}






/*
 *                    Internal routines
 */


static ZMapStyleGlyphShape get_glyph_shape(ZMapFeatureTypeStyle style, int which,  ZMapStrand strand)
{
  ZMapStyleGlyphShape shape;

  shape = ((which == 5) ? zMapStyleGlyphShape5(style, strand == ZMAPSTRAND_REVERSE)
	   : ((which == 3) ? zMapStyleGlyphShape3(style, strand == ZMAPSTRAND_REVERSE)
	      : (ZMapStyleGlyphShape) zMapStyleGlyphShape(style))) ;

  return shape;	/* may be NULL */
}




/* set up the coord array for the paint function
 *
 * fill in coords copied from the shape
 * and adjust height width and origin
 * on paint these get shifted relative to the CanvasFeatureset
 * also cache the colour if score mode is ALT
 */
/* intitialise a glyph struct to have coord set for the given style + feature combo
 * called for both standalone features and sub-feature glyphs
 * NOTE sub-feature glyphs have an uninitialised feature part
 */
static gboolean zmap_window_canvas_set_glyph(FooCanvasItem *foo, ZMapWindowCanvasGlyph glyph,
					     ZMapFeatureTypeStyle style, ZMapFeature feature,
					     double col_width, double score)
{
  ZMapWindowFeaturesetItem featureset = (ZMapWindowFeaturesetItem)foo ;
  ZMapStyleScoreMode score_mode = ZMAPSCORE_INVALID;
  double width = 1.0, height = 1.0;        // these are ratios not pixels
  double min = 0.0,max = 0.0,range;
  ZMapStyleGlyphStrand strand = ZMAPSTYLE_GLYPH_STRAND_INVALID;
  ZMapStyleGlyphAlign align;
  double offset = 0.0, origin = 0.0;
  GdkColor *draw = NULL, *fill = NULL, *border = NULL;

  score_mode = zMapStyleGetScoreMode(style);

  /* a glyph can have different colour if score < threshold so save the actual colour here
   * it's easier to hard code this in the glyph module than have the featureset handle it
   * it is a module specific choice
   * NOTE we can still use selected colours via WINDOW_FOCUS_CACHE_SELECTED
   */



  if (glyph->sub_feature)		/* is sub-feature: use glyph colours not feature */
    {
      if(score_mode == ZMAPSTYLE_SCORE_ALT && score < zMapStyleGlyphThreshold(style))
	zMapStyleGetColours(style, STYLE_PROP_GLYPH_ALT_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill, &draw, &border);
      else
	zMapStyleGetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill, &draw, &border);

      glyph->line_set = FALSE;

      if(border)
	{
	  gulong pixel;

	  glyph->line_set = TRUE;
	  pixel = zMap_gdk_color_to_rgba(border);
	  glyph->line_pixel = foo_canvas_get_color_pixel(foo->canvas, pixel);
	}

      glyph->area_set = FALSE;

      if(fill)
	{
	  gulong pixel;

	  glyph->area_set = TRUE;
	  pixel = zMap_gdk_color_to_rgba(fill);
	  glyph->area_pixel = foo_canvas_get_color_pixel(foo->canvas, pixel);
	}

      glyph->use_glyph_colours = TRUE;
    }


  /* We need splice markers to behave in a particular way for wormbase, I've added this function
   * to get the positions so it doesn't interfere with the general workings of glyphs. */
  if (zMapStyleIsSpliceStyle(feature->style))
    {
      calcSplicePos(featureset, feature, style, score, &origin, &width, &height) ;
    }
  else
    {
      /* scale the glyph etc?? according to score */

      min = zMapStyleGetMinScore(style);
      max = zMapStyleGetMaxScore(style);
      range = max - min;

      // min and max may be signed... but max assumed to be more than min :-)
      // if score < min then don't display
      // if score > max display fill width
      // in between pro-rate

      if(min < 0.0)
	{
	  if(col_width)                       // origin corresponds to zero
	    {
	      origin = col_width * (-min / range);
	      range = score < 0.0 ? -min : max;
	    }
	}
      else
	{
	  max -= min;
	  score -= min;
	  if(score < min)
	    return FALSE;		/* not visible: don't draw */
	  if(col_width)                     // origin is mid point
	    origin = col_width / 2.0;
	}

      if (max)     // scale if configured
	{
	  if(score > max)
	    score = max;
	  if(score < min)
	    score = min;

	  if (score_mode == ZMAPSCORE_WIDTH || score_mode == ZMAPSCORE_SIZE)
	    width = (width * score) / range;
	  else if (score_mode == ZMAPSCORE_HEIGHT || score_mode == ZMAPSCORE_SIZE)
	    height = (height * score) / range;
	}

      // invert H or V??
      if(feature->strand == ZMAPSTRAND_REVERSE)
	strand = zMapStyleGlyphStrand(style);

      if(strand == ZMAPSTYLE_GLYPH_STRAND_FLIP_X)
	width = -width;

      if(strand == ZMAPSTYLE_GLYPH_STRAND_FLIP_Y)
	height = -height;

      if(col_width)
	{
	  // if niether of these are set we'll offset by zero and default to centred glyphs
	  // if config is poor they'll look silly so no need for an error message

	  align = zMapStyleGetAlign(style);
	  offset = col_width / 2;
	  if(align == ZMAPSTYLE_GLYPH_ALIGN_LEFT)
	    origin = 0.0;		// -= offset;
	  else if(align == ZMAPSTYLE_GLYPH_ALIGN_RIGHT)
	    origin = col_width; 	// += offset;
	}
    }

  /* so now we have origin = (x-offset) and width and height ratios */
  glyph->width = width;
  glyph->height = height;
  glyph->origin = origin;

  glyph_set_coords(glyph);

  glyph->coords_set = TRUE;

  return TRUE;
}


/* Calculate the position of splice markers so that the horizontal bit of the marker is where
 * the splice is and the vertical bit indicates a 5'(down) or 3'(up) marker. */
static void calcSplicePos(ZMapWindowFeaturesetItem featureset, ZMapFeature feature, ZMapFeatureTypeStyle style,
			  double score,
			  double *feature_origin_out, double *feature_width_out, double *feature_height_out)
{
  double min_score, max_score ;
  double feature_origin ;
  double width = 1.0, height = 1.0;        // these are ratios not pixels
  double x ;
  GlyphSpliceColumnData splice_data = (GlyphSpliceColumnData)(featureset->per_column_data) ;

  min_score = zMapStyleGetMinScore(style);
  max_score = zMapStyleGetMaxScore(style);

  if (score <= min_score)
    x = 0 ;
  else if (score >= max_score)
    x = splice_data->col_width ;
  else
    x = splice_data->scale_factor * (score - min_score) ;

  width = fabs(splice_data->origin - fabs(x)) ;

  if (width < splice_data->min_size)
    {
      if (score < 0)
	{
	  feature_origin = splice_data->origin + (splice_data->min_size - width) ;
	}
      else
	{
	  feature_origin = splice_data->origin - (splice_data->min_size - width) ;
	}

      width = splice_data->min_size ;
    }
  else
    {
      feature_origin = splice_data->origin  ;
    }


  /* width is in column units (pixels), glyph pixel height is ZMAPSTYLE_SPLICE_GLYPH_LEN pixels
   * so to convert to a width proportional to height divide by ZMAPSTYLE_SPLICE_GLYPH_LEN. */
  width = width / splice_data->glyph_len ;

  /* Return left, right and height. */
  if (score < 0)
    {
      /* Splice markers for negative scores are drawn the opposite way round. */
      *feature_width_out = -width ;
      *feature_height_out = height ;
    }
  else
    {
      *feature_width_out = width ;
      *feature_height_out = height ;
    }
  *feature_origin_out = feature_origin ;

  return ;
}





// set the points to draw centred on the glyph's sequence/column coordinates
// if we scale the glyph it's done here and NB we have to scale +ve and -ve coords for all shapes
// NOTE we convert from a list of single coords to GdkPoint coord pairs
//
static void glyph_set_coords(ZMapWindowCanvasGlyph glyph)
{
  int i,j,coord;

  switch(glyph->shape->type)
    {
    case GLYPH_DRAW_ARC:
      if(glyph->shape->n_coords > 2)       // bounding box, don't include the angles, these points get transformed
	glyph->shape->n_coords = 2;        // x-ref to _draw()
      // fall through

    case GLYPH_DRAW_LINES:
    case GLYPH_DRAW_BROKEN:
    case GLYPH_DRAW_POLYGON:

      for(i = j = 0; i < glyph->shape->n_coords * 2;i++)
	{
	  coord = glyph->shape->coords[i];         // X coord
	  if(coord == GLYPH_COORD_INVALID)
	    coord = (int) GLYPH_CANVAS_COORD_INVALID; // zero, will be ignored

	  if(i & 1)			/* Y coord */
	    {
	      coord *= glyph->height;
      	      glyph->coords[j].y = coord;		 // points are centred around the anchor of 0,0
      	      j++;
	    }
	  else
	    {
	      coord *= glyph->width;
      	      glyph->coords[j].x = (int) glyph->origin + coord;    // points are centred around the anchor of 0,0
	    }
	}
      break;

    default:
      // we get a warning message in _draw() so be silent here
      break;
    }

  return ;
}



/* oh gosh...what do 'points' end up measuring ?????? */
/* pixel coordinates, the glyph is based arounf cx,cy */
static void glyph_to_canvas(GdkPoint *points, GdkPoint *coords, int count, int cx, int cy)
{
  int i;

  for (i = 0; i < count; i++, points++, coords++)
    {
      points->x = coords->x + cx ;
      points->y = coords->y + cy ;
    }

  return ;
}



/* Init the glyph column. */
static void glyphColumnInit(ZMapWindowFeaturesetItem featureset)
{
  gboolean dummy_splice = TRUE ;

  /* Once again we need to know if this is splice markers...find out how to do this... */
  /* mh17 - how about from the style id == GF_Splice ?? */
  if (dummy_splice)
    {
      GlyphSpliceColumnData splice_data ;
      double min_score, max_score ;

      splice_data = g_new0(GlyphSpliceColumnDataStruct, 1) ;

      splice_data->glyph_len = ZMAPSTYLE_SPLICE_GLYPH_LEN ;

      splice_data->col_width = zMapStyleGetWidth(featureset->style) ;

      splice_data->min_size = splice_data->col_width / splice_data->glyph_len ;

      min_score = zMapStyleGetMinScore(featureset->style) ;
      max_score = zMapStyleGetMaxScore(featureset->style) ;

      splice_data->scale_factor = splice_data->col_width / (max_score - min_score) ;

      if (min_score < 0 && 0 < max_score)
	splice_data->origin = splice_data->col_width * (-(min_score) / (max_score - min_score)) ;
      else
	splice_data->origin = 0 ;


      featureset->per_column_data = splice_data ;
    }

  return ;
}



/* Paint 'column' level features. */
static void glyphSetPaint(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable, GdkEventExpose *expose)
{

  if (zMapStyleIsSpliceStyle(featureset->style))
    {
      FooCanvasItem *item = (FooCanvasItem *)&(featureset->__parent__) ;
      GlyphSpliceColumnData splice_data = (GlyphSpliceColumnData)(featureset->per_column_data) ;
      double x1, x2, y1, y2 ;


      if (!(splice_data->colours_set))
	{
	  if (!(splice_data->colours_set
		= zMapGUIGetColour(GTK_WIDGET(item->canvas), ZERO_LINE_COLOUR, &(splice_data->zero_line_colour)))
	      || !(splice_data->colours_set
		   = zMapGUIGetColour(GTK_WIDGET(item->canvas), OTHER_LINE_COLOUR, &(splice_data->other_line_colour))))
	    zMapLogCritical("%s", "Cannot create colours for splice display.") ;
	}

      if (splice_data->colours_set)
	{
	  gdk_gc_set_line_attributes(featureset->gc,
				     DEFAULT_LINE_WIDTH,
				     GDK_LINE_SOLID,
				     GDK_CAP_BUTT,
				     GDK_JOIN_ROUND) ;

	  /* Draw zero line. */
	  gdk_gc_set_foreground(featureset->gc, &(splice_data->zero_line_colour)) ;

	  x1 = (item->x1 + splice_data->origin) ;
	  x2 = (item->x1 + splice_data->origin) ;
	  y1 = item->y1 ;
	  y2 = item->y2 ;
	  zMap_draw_line(drawable, featureset, x1, y1, x2, y2) ;

	  /* Draw 50% & 75% lines. */
	  gdk_gc_set_foreground(featureset->gc, &(splice_data->other_line_colour)) ;

	  x1 = (item->x1 + (splice_data->col_width * 0.5)) ;
	  x2 = (item->x1 + (splice_data->col_width * 0.5)) ;
	  y1 = item->y1 ;
	  y2 = item->y2 ;
	  zMap_draw_line(drawable, featureset, x1, y1, x2, y2) ;

	  x1 = (item->x1 + (splice_data->col_width * 0.75)) ;
	  x2 = (item->x1 + (splice_data->col_width * 0.75)) ;
	  y1 = item->y1 ;
	  y2 = item->y2 ;
	  zMap_draw_line(drawable, featureset, x1, y1, x2, y2) ;
	}

    }

  return ;
}



#if 0
static void glyphZoom(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable)
{
  /* Not sure if I will need this later so left in for now. */

  return ;
}
#endif


/* Free the glyph column. */
static void glyphColumnFree(ZMapWindowFeaturesetItem featureset)
{
  g_free(featureset->per_column_data) ;
  featureset->per_column_data = NULL ;

  return ;
}





// x-ref with zmapWindowDump.c/dumpGlyph()
/* handle sub feature and free standing glyphs */
static void zmap_window_canvas_paint_feature_glyph(ZMapWindowFeaturesetItem featureset,
						   ZMapWindowCanvasFeature canvas_feature, ZMapWindowCanvasGlyph glyph,
						   double y1, GdkDrawable *drawable)
{
  gulong fill,outline;
  GdkColor c;
  FooCanvasItem *item = (FooCanvasItem *)featureset;
  ZMapFeature feature = canvas_feature->feature;
  int cx1, cy1;
  int colours_set, fill_set = 0, outline_set = 0;
  double x1;

  /* just draw the glyph */
  /* must recalculate gdk coords as columns can move around */

  x1 = featureset->dx;
  if(featureset->bumped)
    x1 += canvas_feature->bump_offset;

  y1 += featureset->dy - featureset->start;

  /* get item canvas coords, following example from FOO_CANVAS_RE (used by graph items) */
  foo_canvas_w2c(item->canvas, x1, y1, &cx1, &cy1);
  glyph_to_canvas(glyph->points, glyph->coords, glyph->shape->n_coords, cx1, cy1);


  /* we have pre-calculated pixel colours */
  colours_set = zMapWindowCanvasFeaturesetGetColours(featureset, canvas_feature, &fill, &outline);
  fill_set = colours_set & WINDOW_FOCUS_CACHE_FILL;
  outline_set = colours_set & WINDOW_FOCUS_CACHE_OUTLINE;

  if (glyph->use_glyph_colours)
    {
      fill_set = glyph->area_set;
      fill = glyph->area_pixel;

      outline_set = glyph->line_set;
      outline = glyph->line_pixel;
    }


  if (zMapStyleIsSpliceStyle(feature->style))
    gdk_gc_set_line_attributes(featureset->gc,
			       SPLICE_LINE_WIDTH,
			       GDK_LINE_SOLID,
			       GDK_CAP_BUTT,
			       GDK_JOIN_MITER) ;
  else
    gdk_gc_set_line_attributes(featureset->gc,
			       DEFAULT_LINE_WIDTH,
			       GDK_LINE_SOLID,
			       GDK_CAP_BUTT,
			       GDK_JOIN_ROUND) ;

  if (fill_set)
    {
      c.pixel = fill;
      gdk_gc_set_foreground (featureset->gc, &c);
      zmap_window_canvas_glyph_draw(featureset, glyph, drawable, TRUE);
    }

  if (outline_set)
    {
      c.pixel = outline;
      gdk_gc_set_foreground (featureset->gc, &c);
      zmap_window_canvas_glyph_draw(featureset, glyph, drawable, FALSE);
    }

  return ;
}


static void zmap_window_canvas_glyph_draw(ZMapWindowFeaturesetItem featureset,
					  ZMapWindowCanvasGlyph glyph, GdkDrawable *drawable, gboolean fill)
{
  int start,end ;

  /* Note that sub feature glyphs do not cache the feature. */
  if ((glyph->feature.feature) && zMapStyleIsSpliceStyle(glyph->feature.feature->style))
    {
      GdkColor c;
      gulong fill_pixel, outline_pixel ;
      int i, min_x, max_x, min_y, max_y, curr_x, curr_y ;
      int status ;

      status = zMapWindowCanvasFeaturesetGetColours(featureset,
						    (ZMapWindowCanvasFeature)glyph,
						    &fill_pixel, &outline_pixel) ;

      if (fill && glyph->feature.flags & WINDOW_FOCUS_GROUP_FOCUSSED)
	{
	  int line_adjust ;

	  /* Get feature extent on display. */
	  for (i = 0, min_x = G_MAXINT, max_x = 0, min_y = G_MAXINT, max_y = 0 ; i < glyph->shape->n_coords ; i++)
	    {
	      curr_x = glyph->points[i].x ;
	      curr_y = glyph->points[i].y ;

	      if (min_x > curr_x)
		min_x = curr_x ;
	      if (max_x < curr_x)
		max_x = curr_x ;

	      if (min_y > curr_y)
		min_y = curr_y ;
	      if (max_y < curr_y)
		max_y = curr_y ;
	    }

	  line_adjust = (SPLICE_LINE_WIDTH + 1) / 2 ;
	  c.pixel = fill_pixel;
	  gdk_gc_set_foreground(featureset->gc, &c) ;
	  gdk_draw_rectangle(drawable,
			     featureset->gc,
			     TRUE,
			     min_x,
			     min_y,
			     (max_x - min_x),
			     (max_y - min_y)) ;

	  c.pixel = outline_pixel;
	  gdk_gc_set_foreground(featureset->gc, &c);
	  gdk_draw_lines(drawable, featureset->gc, glyph->points, glyph->shape->n_coords);
	}

      if (!fill)
	{
	  c.pixel = outline_pixel;
	  gdk_gc_set_foreground(featureset->gc, &c);
	  gdk_draw_lines(drawable, featureset->gc, glyph->points, glyph->shape->n_coords);
	}

    }
  else
    {

  switch (glyph->shape->type)
    {
    case GLYPH_DRAW_LINES:
      {
	gdk_draw_lines (drawable, featureset->gc, glyph->points, glyph->shape->n_coords);

	break;
      }
    case GLYPH_DRAW_BROKEN:
      /*
       * in the shape structure the array of coords has invalid values at the break
       * and we draw lines between the points in between
       * NB: GDK uses points we have coordinate pairs
       */
      for(start = 0;start < glyph->shape->n_coords;start = end+1)
        {
          for(end = start;end < glyph->shape->n_coords && glyph->shape->coords[end+end] != GLYPH_COORD_INVALID; end++)
            continue;

          gdk_draw_lines(drawable, featureset->gc, glyph->points + start , end - start);
        }
      break;

    case GLYPH_DRAW_POLYGON:
      {
	gdk_draw_polygon(drawable, featureset->gc, fill,  glyph->points, glyph->shape->n_coords);

	break;
      }

    case GLYPH_DRAW_ARC:
      {
        double x1, y1, x2, y2;
        int a1,a2;

        x1 = glyph->points[0].x;
        y1 = glyph->points[0].y;
        x2 = glyph->points[1].x;
        y2 = glyph->points[1].y;
        a1 = (int) glyph->shape->coords[4] * 64;
        a2 = (int) glyph->shape->coords[5] * 64;

        gdk_draw_arc(drawable, featureset->gc, fill, (int) x1, (int) y1, (int) (x2 - x1), (int) (y2 - y1), a1,a2);
      }
      break;

    default:
      g_warning("Unknown Glyph Style");
      break;
    }
    }

  return ;
}




/* Checks to see if given x,y coord is within a glyph feature, has to work differently
 * for different glyph types:
 *
 * Splice markers: checks point is within extent of the angle bracket.
 *
 * Other glyphs: checks if point is within x,y and feature width.
 *
 *
 * reimplementation based on coords of glyph, not the feature coords which may
 * be useless for glyph purposes. */
static double glyphPoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
			 double item_x, double item_y, int cx, int cy,
			 double local_x, double local_y, double x_off)
{
  double best = 1.0e36 ;
  ZMapWindowCanvasGlyph glyph = (ZMapWindowCanvasGlyph)gs ;
  double can_start_x, can_end_x, can_start_y, can_end_y ;
  double world_start_x, world_end_x, world_start_y, world_end_y ;
  int i, min_x, max_x, min_y, max_y, curr_x, curr_y ;
  FooCanvasItem *foo = (FooCanvasItem *)fi ;


  /* Glyphs only have shape when drawn (I think...) so it's possible when looking for nearby
   * glyphs for the point operation to be looking at glyphs that haven't been drawn and
   * do not have a shape. */
  /* mh17: i added a GlyphAdd fucntion, so we can set the shape etc before paint, but this test is harmless */
  if (glyph->shape)
    {
      /* Get feature extent on display. */
      for (i = 0, min_x = G_MAXINT, max_x = 0, min_y = G_MAXINT, max_y = 0 ; i < glyph->shape->n_coords ; i++)
	{
	  curr_x = glyph->points[i].x ;
	  curr_y = glyph->points[i].y ;

	  if (min_x > curr_x)
	    min_x = curr_x ;
	  if (max_x < curr_x)
	    max_x = curr_x ;

	  if (min_y > curr_y)
	    min_y = curr_y ;
	  if (max_y < curr_y)
	    max_y = curr_y ;
	}


      foo_canvas_c2w(foo->canvas, min_x, min_y, &can_start_x, &can_start_y) ;
      foo_canvas_c2w(foo->canvas, max_x, max_y, &can_end_x, &can_end_y) ;

      world_start_x = can_start_x ;
      world_end_x = can_end_x ;
      world_start_y = can_start_y ;
      world_end_y = can_end_y ;

      foo_canvas_item_i2w(foo, &world_start_x, &world_start_y) ;
      foo_canvas_item_i2w(foo, &world_end_x, &world_end_y) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      printf("Item x,y: %g, %g\t canvas x,y: %d, %d\tlocal x.y: %g, %g"
	     "\tCanvas x_start/x_end y_start/y_end: %g,%g %g,%g"
	     "\tWorld: x_start/x_end y_start/y_end: %g,%g %g,%g",
	     item_x, item_y, cx, cy, local_x, local_y,
	     can_start_x, can_end_x, can_start_y, can_end_y,
	     world_start_x, world_end_x, world_start_y, world_end_y) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      if (can_start_x < local_x && can_end_x > local_x
	  && can_start_y < local_y && can_end_y > local_y)			    /* overlaps cursor */
	{
	  best = 0.0;
	}
    }

  return best ;
}



/* we need to calculate the extent of the glyph which is somewhere around the single base coordinate */
static ZMapWindowCanvasFeature zMapWindowCanvasGlyphAddFeature(ZMapWindowFeaturesetItem featureset,
							       ZMapFeature feature, double y1, double y2)
{
  ZMapWindowCanvasFeature feat = NULL ;
  ZMapStyleGlyphShape shape ;
  int boundary_type = 0 ;


  if (zMapStyleIsSpliceStyle(feature->style))
    boundary_type = (feature->boundary_type == ZMAPBOUNDARY_5_SPLICE ? 5 : 3) ;

  shape = get_glyph_shape(feature->style, boundary_type, feature->strand)  ;

  if (!shape || shape->type == GLYPH_DRAW_INVALID || !shape->n_coords)
    {
      zMapLogWarning("Could not find glyph shape for feature \"%s\" at %d, %d",
		     g_quark_to_string(feature->original_id), feature->x1, feature->x2) ;
    }
 else
   {
     ZMapWindowCanvasGlyph glyph ;
     FooCanvasItem *foo = (FooCanvasItem *) featureset ;
     double col_width ;
     double longest = featureset->longest ;

     feat = zMapWindowFeaturesetAddFeature(featureset, feature, y1, y2) ;
     glyph = (ZMapWindowCanvasGlyph) feat ;

     /* work out the real coords of the glyph on display and asign the shape */
     /* for stand-alone glyphs which will be unset ie zero */
     /* NOTE this boundary code appears to be tied explcitly to GF splice features from ACEDB */
     if (zMapStyleIsSpliceStyle(feature->style))
       glyph->which = boundary_type ;

     glyph->shape = shape ;

     col_width = zMapStyleGetWidth(featureset->style) ;

     zmap_window_canvas_set_glyph(foo, glyph, feature->style, feature, col_width, feature->score ) ;

     /* as glyphs are fixed size we store the longest as pixels and expand on search */
     /* so here we overwrite the values set by zMapWindowFeaturesetAddFeature() ; */
     if(longest < shape->height)
       longest = shape->height ;
     featureset->longest = longest ;
   }

  return feat ;
}


