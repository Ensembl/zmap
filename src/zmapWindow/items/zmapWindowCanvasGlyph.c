/*  File: zmapWindowCanvasGlyph.c
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
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

static gboolean zmap_window_canvas_set_glyph(FooCanvasItem *foo, ZMapWindowCanvasGlyph glyph, ZMapFeatureTypeStyle style, ZMapFeature feature, double col_width, double score);
static void glyph_set_coords(ZMapWindowCanvasGlyph glyph);
static void glyph_to_canvas(GdkPoint *points, GdkPoint *coords, int count,int cx, int cy);
static void zmap_window_canvas_glyph_draw (ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasGlyph glyph, GdkDrawable *drawable, gboolean fill);
static void zmap_window_canvas_paint_feature_glyph(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, ZMapWindowCanvasGlyph glyph, double y1, GdkDrawable *drawable);




/*
 * Caching glyphs
 *
 * alignments are the most populous features and may have a lot of glpyhs attached when bumped
 * to avoid excessive memory use and pointless recalculation ofs glyph shapes
 * we cache thse if they have limited variability, determined by a signature.
 *
 * as glyphs are pixel base features and generally quite small there is the possibility to do this
 * even for fully variable configurations, imposing a maximum limit on the memory used and also display time
 * (not done in first implementation)
 *
 * Cached glyphs are held in a module global hash table and never freed
 */

GHashTable *glyph_cache_G = NULL;


ZMapStyleGlyphShape get_glyph_shape(ZMapFeatureTypeStyle style, int which)
{
	ZMapStyleGlyphShape shape;

	shape = (which == 5) ? zMapStyleGlyphShape5(style) :
              (which == 3) ? zMapStyleGlyphShape3(style) :
              (ZMapStyleGlyphShape) zMapStyleGlyphShape(style);

	return shape;	/* may be NULL */
}


/* get an id for a simple glyph variant or NULL if it's too complex
 * we can get a the same shape in diff styles which may appear in diff columns
 * but glyphs are always the same size if this does not relate to feature score
 * so we can cache globally by glyph shape id
 */
GQuark zMapWindowCanvasGlyphSignature(ZMapFeatureTypeStyle style, ZMapFeature feature, int which)
{
	ZMapStyleGlyphShape shape;
	char buf[64];	/* will never overflow */
	char strand = '+';
	char alt = 'N';

	if(!style)
		return 0;
	if(style->score_mode != ZMAPSCORE_INVALID && style->score_mode != ZMAPSTYLE_SCORE_ALT)
		return 0;

	shape = get_glyph_shape(style, which);

	if(!shape)
		return 0;

	if(feature->strand == ZMAPSTRAND_REVERSE)
		strand = '-';
	if(style->score_mode == ZMAPSTYLE_SCORE_ALT && feature->score < zMapStyleGlyphThreshold(style))
		alt = 'A';

      sprintf(buf,"%s_%s%c%c",g_quark_to_string(style->unique_id),g_quark_to_string(shape->id),strand,alt);

	return g_quark_from_string(buf);

}


/* which is 5' or 3', will work if unspecified but not intentionally.... */
/* style is the type of glyph to draw driven by calling code, it's one of the feature's styles' sub-styles */
/* NOTE we return a canvas feature/ glyph even though we don't need the feature part */
/* NOTE this is not called for free standing glyphs */
ZMapWindowCanvasGlyph zMapWindowCanvasGetGlyph(ZMapWindowFeaturesetItem featureset,ZMapFeatureTypeStyle style, ZMapFeature feature, int which, double score)
{
	GQuark siggy = zMapWindowCanvasGlyphSignature(style, feature, which);
	ZMapWindowCanvasGlyph glyph = NULL;
	double col_width;

	if(siggy)
	{
		if(glyph_cache_G)
			glyph = g_hash_table_lookup(glyph_cache_G,GUINT_TO_POINTER(siggy));
		else
			glyph_cache_G = g_hash_table_new(NULL,NULL);
	}

	if(!glyph)
	{
		glyph = g_new0(zmapWindowCanvasGlyphStruct,1);		/* OK efficient if we really do cache these */

		if(glyph)
		{
			FooCanvasItem *foo = (FooCanvasItem *) featureset;
			ZMapStyleGlyphShape shape;

			/* calulate shape when created and cached */

			glyph->which = which;
			glyph->sub_feature = TRUE;

			glyph->shape = shape = get_glyph_shape(style, glyph->which);
			if(!shape || shape->type == GLYPH_DRAW_INVALID || !shape->n_coords)
				return NULL;

			col_width = zMapStyleGetWidth(featureset->style);
			zmap_window_canvas_set_glyph(foo, glyph, style, feature, col_width, score);

			if(siggy)
			{
				g_hash_table_insert(glyph_cache_G,GUINT_TO_POINTER(siggy),glyph);
				glyph->sig = siggy;
//zMapLogWarning("created glyph %s\n",g_quark_to_string(siggy));
			}
		}
	}

	return glyph;
}





/* set up thecoord array for the paint function
 *
 * fill in coords copied from the shape
 * and adjust height width and origin
 * on paint these get shifted relative to the CanvasFeatureset
 * also cache the colour if score mode is ALT
 */
/* intitialise a glyph struct to have coord set fo the given style + feature combo
 * called for both standalone features and sub-feature glyphs
 * NOTE sub-feature glyphs have an uninitialised feature part
 */
static gboolean zmap_window_canvas_set_glyph(FooCanvasItem *foo, ZMapWindowCanvasGlyph glyph, ZMapFeatureTypeStyle style, ZMapFeature feature, double col_width, double score)
{
	ZMapStyleScoreMode score_mode = ZMAPSCORE_INVALID;
	double width = 1.0,height = 1.0;        // these are ratios not pixels
	double min = 0.0,max = 0.0,range;
	ZMapStyleGlyphStrand strand = ZMAPSTYLE_GLYPH_STRAND_INVALID;
	ZMapStyleGlyphAlign align;
	double offset = 0.0, origin = 0.0;
	GdkColor *draw = NULL, *fill = NULL, *border = NULL;

      score_mode = zMapStyleGetScoreMode(style);

	/* a glyph can hav different colour if score < threshold so save the actual colour here
	 * it's easier to hard code this in the glyph module than have the featureset handle it
	 * it is a module specific choice
	 * NOTE we can still use selected colours via WINDOW_FOCUS_CACHE_SELECTED
	 */

	if(glyph->sub_feature)		/* is sub-feature: use glyph colours not feature */
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

      if(max)     // scale if configured
      {
           if(score > max)
               score = max;
           if(score < min)
               score = min;

          if(score_mode == ZMAPSCORE_WIDTH || score_mode == ZMAPSCORE_SIZE)
            width = width * score / range;
          else if(score_mode == ZMAPSCORE_HEIGHT || score_mode == ZMAPSCORE_SIZE)
            height = height * score / range;
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

	/* so now we have origin = (x-offset) and width and height ratios */
	glyph->width = width;
	glyph->height = height;
	glyph->origin = origin;

	glyph_set_coords(glyph);

	glyph->coords_set = TRUE;

	return TRUE;
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




static void glyph_to_canvas(GdkPoint *points, GdkPoint *coords, int count,int cx, int cy)
{
  int i;

  for(i = 0; i < count; i++, points++, coords++)
    {
      points->x = coords->x + cx;
      points->y = coords->y + cy;
    }
}


/* Draw handler for the glyph item */
// x-ref with zmapWindowDump.c/dumpGlyph()
static void zmap_window_canvas_glyph_draw (ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasGlyph glyph, GdkDrawable *drawable, gboolean fill)
{
  int start,end;

  switch(glyph->shape->type)
    {
    case GLYPH_DRAW_LINES:
      gdk_draw_lines (drawable, featureset->gc, glyph->points, glyph->shape->n_coords);
      break;
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

          gdk_draw_lines (drawable, featureset->gc, glyph->points + start , end - start);
        }
      break;

    case GLYPH_DRAW_POLYGON:
        gdk_draw_polygon (drawable, featureset->gc, fill,  glyph->points, glyph->shape->n_coords);
      break;

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

  return ;
}



/* handle sub feature and free standing glyphs */
/* called from above fucntion and also ...PaintAlignment() */
static void zmap_window_canvas_paint_feature_glyph(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, ZMapWindowCanvasGlyph glyph, double y1, GdkDrawable *drawable)
{
	gulong fill,outline;
	GdkColor c;
	FooCanvasItem *item = (FooCanvasItem *) featureset;
      int cx1, cy1;
	int colours_set, fill_set = 0, outline_set = 0;
	double x1;

	/* just draw the glyph */
	/* must recalculate gdk coords as columns can move around */

	x1 = featureset->dx;
	if(featureset->bumped)
		x1 += feature->bump_offset;

	y1 += featureset->dy - featureset->start;

		/* get item canvas coords, following example from FOO_CANVAS_RE (used by graph items) */
	foo_canvas_w2c (item->canvas, x1, y1, &cx1, &cy1);
	glyph_to_canvas(glyph->points, glyph->coords, glyph->shape->n_coords, cx1, cy1);


		/* we have pre-calculated pixel colours */
	colours_set = zMapWindowCanvasFeaturesetGetColours(featureset, feature, &fill, &outline);
	fill_set = colours_set & WINDOW_FOCUS_CACHE_FILL;
	outline_set = colours_set & WINDOW_FOCUS_CACHE_OUTLINE;

	if(glyph->use_glyph_colours)
     	{
		fill_set = glyph->area_set;
		fill = glyph->area_pixel;
		outline_set = glyph->line_set;
		outline = glyph->line_pixel;
	}

	if(fill_set)
	{
		c.pixel = fill;
		gdk_gc_set_foreground (featureset->gc, &c);
		zmap_window_canvas_glyph_draw (featureset, glyph, drawable, TRUE);
	}

	if(outline_set)
	{
		c.pixel = outline;
		gdk_gc_set_foreground (featureset->gc, &c);
		zmap_window_canvas_glyph_draw (featureset, glyph, drawable, FALSE);
	}
}



/* the interface for free-standing glyphs, called via CanvasFeatureset virtual functions  */
/* must calculate the shape coords etc on first paint */
void zMapWindowCanvasGlyphPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable,GdkEventExpose *expose)
{
	ZMapWindowCanvasGlyph glyph = (ZMapWindowCanvasGlyph) feature;
	ZMapFeature feat = feature->feature;
	ZMapStyleGlyphShape shape;
	FooCanvasItem *foo = (FooCanvasItem *) featureset;
	double y1;
	double col_width;

	if(!glyph->coords_set)
	{
		/* for stand-alone glyphs which will be unset ie zero */
		/* NOTE this boundary code appears to be tied explcitly to GF splice features from ACEDB */
		if (feat->flags.has_boundary)
			glyph->which = feat->boundary_type == ZMAPBOUNDARY_5_SPLICE ? 5 : 3;

		shape = get_glyph_shape(feat->style, glyph->which);

		if(!shape || shape->type == GLYPH_DRAW_INVALID || !shape->n_coords)
			return;

		glyph->shape = shape;

		col_width = zMapStyleGetWidth(featureset->style);

		if(!zmap_window_canvas_set_glyph(foo, glyph, feat->style, feat, col_width, feat->score ))
			return;
	}

	y1 = feature->feature->x1;

	zmap_window_canvas_paint_feature_glyph(featureset, feature, glyph, y1, drawable);
}


/* the interface for sub-feature glyphs, called directly */
/* glyphs are cached and we calculate the shape on first in zMapWindowCanvasGetGlyph() */

void zMapWindowCanvasGlyphPaintSubFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, ZMapWindowCanvasGlyph glyph, GdkDrawable *drawable)
{
	double y1;

	if(!glyph)	/* likely to happen frequently */
		return;

	y1 = glyph->which == 3 ? feature->feature->x2 + 1 : feature->feature->x1;
//zMapLogWarning("paint glyph %f,%f %d, %s",feature->y1,feature->y2, glyph->which,g_quark_to_string(glyph->sig));

	zmap_window_canvas_paint_feature_glyph(featureset, feature, glyph, y1, drawable);

}



void zMapWindowCanvasGlyphInit(void)
{
	gpointer funcs[FUNC_N_FUNC] = { NULL };

	funcs[FUNC_PAINT] = zMapWindowCanvasGlyphPaintFeature;

	zMapWindowCanvasFeatureSetSetFuncs(FEATURE_GLYPH, funcs, sizeof(zmapWindowCanvasGlyphStruct));
}

