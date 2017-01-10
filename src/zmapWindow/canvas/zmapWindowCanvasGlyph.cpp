/*  File: zmapWindowCanvasGlyph.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
 * Description: Implements create/draw interface for
 *              glyph features.
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <math.h>
#include <string.h>

#include <ZMap/zmapFeature.hpp>
#include <zmapWindowCanvasDraw.hpp>
#include <zmapWindowCanvasFeatureset_I.hpp>
#include <zmapWindowCanvasFeature_I.hpp>
#include <zmapWindowCanvasGlyph_I.hpp>



/* CURRENTLY WE CACHE WINDOW POSITIONS WHICH MEANS WE HAVE TO RECACULATE ON EVERY ZOOM, THIS
 * IS NOT EFFICIENT, IT WOULD BE BETTER TO CALCULATE ON THE FLY....BUT THAT'S QUITE A
 * A BIT OF REWRITE.... */


/* glyph line widths. */
enum {DEFAULT_LINE_WIDTH = 1, SPLICE_LINE_WIDTH = 1} ;



/* Drawing glyphs is dissapointingly complex they are pixel based (same size always) and defined
 * by a random series of points and can include line breaks and optional fill colours But we do
 * limit the number of points to 16
 *
 * We may want to call some of these functions from other features eg alignments,
 * this goes via direct function calls rather than the CanvasFeatureset virtual function arrays
 * this module and CanvasAlignment are in parallel so it's kind of structured,
 * (could yank the common code into another file if anyone gets precious about it)
 */



static void glyphColumnInit(ZMapWindowFeaturesetItem featureset) ;
static void glyphSetPaint(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable, GdkEventExpose *expose) ;
static void glyphPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                              GdkDrawable *drawable, GdkEventExpose *expose);
static void glyphPreZoom(ZMapWindowFeaturesetItem featureset) ;
static void glyphZoom(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable) ;
static ZMapWindowCanvasFeature glyphAddFeature(ZMapWindowFeaturesetItem featureset,
                                               ZMapFeature feature, double y1, double y2);
static double glyphPoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
                         double item_x, double item_y, int cx, int cy,
                         double x, double y, double x_off) ;
static void glyphColumnFree(ZMapWindowFeaturesetItem featureset) ;

static void setGlyphPerColData(ZMapWindowFeaturesetItem featureset) ;
static GQuark getGlyphSignature(ZMapFeatureTypeStyle style, ZMapStrand feature_strand,
                                ZMapGlyphWhichType which, double score) ;
static ZMapStyleGlyphShape getGlyphShape(ZMapFeatureTypeStyle style, ZMapGlyphWhichType which, ZMapStrand strand) ;
static gboolean setGlyph(FooCanvasItem *foo,
                         ZMapWindowCanvasGlyph glyph, ZMapFeatureTypeStyle style,
                         ZMapStrand feature_strand, double col_width, double score);
static void setGlyphCoords(ZMapWindowCanvasGlyph glyph, GlyphAnyColumnData any_col,
                           ZMapFeatureTypeStyle style, double score, double width) ;
static void glyph_to_canvas(GdkPoint *points, GdkPoint *coords, int count,int cx, int cy);
static void glyphDraw(ZMapWindowFeaturesetItem featureset,
                      ZMapWindowCanvasGlyph glyph, GdkDrawable *drawable, gboolean fill_flag);
static void paintGlyph(ZMapWindowFeaturesetItem featureset,
                       ZMapWindowCanvasFeature feature,
                       ZMapWindowCanvasGlyph glyph, double y1, GdkDrawable *drawable);
static void calcCanvasPos(gpointer data, gpointer user_data) ;
static void calcSplicePos(ZMapStyleGlyphShape shape, GFSpliceColumnData splice_col,

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                          ZMapFeature feature,
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

                          ZMapFeatureTypeStyle style, double score,
                          double *origin_out, double *width_out, double *height_out) ;
double getGlyphFeaturePos(ZMapFeature feat) ;
static void setGlyphCanvasCoords(ZMapWindowFeaturesetItem featureset,  ZMapWindowCanvasFeature feature,
                                 ZMapWindowCanvasGlyph glyph, double y1) ;




/*
 * Caching glyphs
 *
 * alignments are the most populous features and may have a lot of glyphs attached when bumped
 * to avoid excessive memory use and pointless recalculation of glyph shapes
 * we cache these if they have limited variability, determined by a signature.
 *
 * as glyphs are pixel based features and generally quite small there is the possibility to do this
 * even for fully variable configurations, imposing a maximum limit on the memory used and also display time
 * (not done in first implementation)
 *
 * Cached glyphs are held in a module global hash table and never freed.
 */

static GHashTable *glyph_cache_G = NULL ;



/* HACK......I want to move Steve's glyph's to use Malcolm's (kind of half implemented)
 * cache/style mechanism. I'm moving this here and providing a function to get it while I work
 * how Malcolm's stuff actually works.... */

/*
 * Glyphs to represent the trunction of a feature at the ZMap boundary. One for
 * truncation at the start and one for truncation at the end.
 */

static ZMapStyleGlyphShapeStruct truncation_shape_start_G =
{
  {0,0, 5,-5, 0,-10, -5,-5, 0,0},                           /* length 32 coordinate array */
  5,                                                        /* number of coordinates */
  10, 10,                                                   /* width and height */
  0,                                                        /* quark ID */
  GLYPH_DRAW_LINES                                          /* ZMapStyleGlyphDrawType:
                                                               LINES == OUTLINE, POLYGON == filled */
}  ;

static ZMapStyleGlyphShapeStruct truncation_shape_end_G =
{
  {0,0, -5,5, 0,10, 5,5, 0,0},
  5,
  10, 10,
  0,
  GLYPH_DRAW_LINES
}  ;



/* AND HERE'S SOME HACKERY FOR MY SPLICE GLYPHS....NEED TO CONVERT BOTH TO STYLES AT THE SAME TIME... */
static ZMapStyleGlyphShapeStruct junction_shape_start_G =
{
  {-6,1, -6,6, 6,6, 6,1, -6,1},                             /* length 32 coordinate array */
  5,                                                        /* number of coordinates */
  12, 6,                                                    /* width and height */
  0,                                                        /* quark ID */
  GLYPH_DRAW_POLYGON                                        /* ZMapStyleGlyphDrawType:
                                                               LINES == OUTLINE, POLYGON == filled */
}  ;

static ZMapStyleGlyphShapeStruct junction_shape_end_G =
{
  {-6,0, -6,-6, 6,-6, 6,0, -6,0},
  5,
  12, 6,
  0,
  GLYPH_DRAW_POLYGON
}  ;


/* END OF HACK...... */















/*
 *                 External interface
 */


/* HACK.....see globals above....temporary access to truncation glyphs...will move to style
 * in the end.... */
void zMapWindowCanvasGlyphGetTruncationShapes(ZMapStyleGlyphShape *start, ZMapStyleGlyphShape *end)
{

  if (start)
    *start = &truncation_shape_start_G ;

  if (end)
    *end = &truncation_shape_end_G ;

  return ;
}

void zMapWindowCanvasGlyphGetJunctionShapes(ZMapStyleGlyphShape *start_out, ZMapStyleGlyphShape *end_out)
{

  if (start_out)
    *start_out = &junction_shape_start_G ;

  if (end_out)
    *end_out = &junction_shape_end_G ;

  return ;
}







void zMapWindowCanvasGlyphInit(void)
{
  gpointer funcs[FUNC_N_FUNC] = { NULL } ;
  gpointer feature_funcs[CANVAS_FEATURE_FUNC_N_FUNC] = { NULL };

  funcs[FUNC_SET_INIT] = (void *)glyphColumnInit ;
  funcs[FUNC_SET_PAINT] = (void *)glyphSetPaint ;
  funcs[FUNC_PAINT] = (void *)glyphPaintFeature ;
  funcs[FUNC_PRE_ZOOM] = (void *)glyphPreZoom ;
  funcs[FUNC_ZOOM] = (void *)glyphZoom ;
  funcs[FUNC_POINT] = (void *)glyphPoint ;
  funcs[FUNC_FREE] = (void *)glyphColumnFree ;
  funcs[FUNC_ADD] = (void *)glyphAddFeature ;

  zMapWindowCanvasFeatureSetSetFuncs(FEATURE_GLYPH, funcs, (size_t) 0) ;

  zMapWindowCanvasFeatureSetSize(FEATURE_GLYPH, feature_funcs, sizeof(ZMapWindowCanvasGlyphStruct)) ;

  return ;
}






ZMapWindowCanvasGlyph zMapWindowCanvasGlyphAlloc(ZMapStyleGlyphShape shape, ZMapGlyphWhichType which,
                                                 gboolean scale_to_feature_width, gboolean sub_feature)
{
  ZMapWindowCanvasGlyph glyph = NULL ;

  glyph = g_new0(ZMapWindowCanvasGlyphStruct, 1) ;
  glyph->shape = shape ;
  glyph->which = which ;
  glyph->scale_to_feature_width = scale_to_feature_width ;
  glyph->sub_feature = sub_feature ;

  return glyph ;
}


gboolean zMapWindowCanvasGlyphSetColours(ZMapWindowCanvasGlyph glyph, gulong border_pixel, gulong fill_pixel)
{
  gboolean result = FALSE ;

  if (border_pixel)
    {
      glyph->border_pixel = border_pixel ;
      glyph->border_set = TRUE ;
    }

  if (fill_pixel)
    {
      glyph->fill_pixel = fill_pixel ;
      glyph->fill_set = TRUE ;
    }

  glyph->use_glyph_colours = TRUE ;

  return result ;
}





/* which is 5' or 3', will work if unspecified but not intentionally.... */
/* style is the type of glyph to draw driven by calling code, it's one of the feature's styles' sub-styles */
/* NOTE we return a canvas feature/ glyph even though we don't need the feature part */
/* NOTE this is not called for free standing glyphs */
ZMapWindowCanvasGlyph zMapWindowCanvasGetGlyph(ZMapWindowFeaturesetItem featureset,
                                               ZMapFeatureTypeStyle style,

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                                               ZMapFeature feature,
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
                                               ZMapStrand feature_strand,
                                               ZMapGlyphWhichType which, double score)
{
  ZMapWindowCanvasGlyph glyph = NULL ;
  GQuark siggy ;
  double col_width ;

  siggy = getGlyphSignature(style, feature_strand, which, score) ;

  if (siggy)
    {
      if(glyph_cache_G)
        glyph = (ZMapWindowCanvasGlyph)g_hash_table_lookup(glyph_cache_G,GUINT_TO_POINTER(siggy)) ;
      else
        glyph_cache_G = g_hash_table_new(NULL,NULL) ;
    }

  if (!glyph)
    {
      ZMapStyleGlyphShape shape ;
      FooCanvasItem *foo = (FooCanvasItem *)featureset ;

      shape = getGlyphShape(style, which, feature_strand) ;

      if (!shape || shape->type == GLYPH_DRAW_INVALID || !shape->n_coords)
        {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          /* put this at a higher level...... */

          zMapLogWarning("Could not find sub feature glyph shape for feature \"%s\" at %d, %d",
                         g_quark_to_string(feature->original_id), feature->x1, feature->x2) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

        }
      else
        {
          glyph = g_new0(ZMapWindowCanvasGlyphStruct, 1) ;  /* OK efficient if we really do cache these */

          /* calulate shape when created and cached */

          glyph->which = which ;
          glyph->sub_feature = TRUE ;

          glyph->shape = shape ;

          col_width = zMapStyleGetWidth(featureset->style) ;
          setGlyph(foo, glyph, style, feature_strand, col_width, score) ;

          if(siggy)
            {
              g_hash_table_insert(glyph_cache_G,GUINT_TO_POINTER(siggy),glyph) ;
              glyph->sig = siggy ;
            }
        }
    }

  return glyph ;
}


/* MUST BE MERGED WITH STEVE'S CODE.....OR RATHER, STEVE'S CODE MUST BE MERGED WITH THIS... */
/* the interface for sub-feature glyphs, called directly
 * glyphs are cached and we calculate the shape on first in zMapWindowCanvasGetGlyph() */
void zMapWindowCanvasGlyphPaintSubFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                                          ZMapWindowCanvasGlyph glyph, GdkDrawable *drawable)
{
  double y = 0.0 ;

  zMapReturnIfFail((featureset && feature && glyph && drawable)) ;

  y = (glyph->which == ZMAP_GLYPH_THREEPRIME ? feature->feature->x2 + 1 : feature->feature->x1) ;

  setGlyphCanvasCoords(featureset, feature, glyph, y) ;

  paintGlyph(featureset, feature, glyph, y, drawable) ;

  return ;
}


void zMapWindowCanvasGlyphSetColData(ZMapWindowFeaturesetItem featureset)
{
  setGlyphPerColData(featureset) ;

  return ;
}



/*
 * Draw the truncation glyph as a standalone shape. There are several steps:
 *
 * (1) Basic properties of glyph.
 * (2) World coordinates of glyph. The call to paintGlyph()
 *     makes a call to convert to canvas coordinates.
 * (4) Draw the thing.
 *
 * A more general function to just draw a standalone glyph could be based on
 * the approach used here which seems to be robust and not muckup anything
 * else. Not needed at the moment though.
 *
 */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
void zMapWindowCanvasGlyphDrawTruncationGlyph(FooCanvasItem *foo,
                                              ZMapWindowFeaturesetItem featureset,
                                              ZMapWindowCanvasFeature feature,
                                              ZMapFeatureTypeStyle style,
                                              ZMapWindowCanvasGlyph glyph,
                                              GdkDrawable *drawable,
                                              double col_width)
{
  double score = 0.0, y = 0.0 ;

  zMapReturnIfFail((foo && featureset && feature && style && glyph && drawable && (col_width > 0.0) )) ;
  zMapReturnIfFail( glyph->which == ZMAP_GLYPH_TRUNCATED_START || glyph->which == ZMAP_GLYPH_TRUNCATED_END ) ;

  /*
   * Basic properties of the glyph. The quantities 'width' and 'height'
   * are ratios, not numbers of pixels or units in world coordinates.
   */
  glyph->width = 1.0 ;
  glyph->height = 1.0 ;
  glyph->origin = col_width/2.0 ;

  setGlyphCoords(glyph, NULL, style, score, 0) ;
  glyph->coords_set = TRUE ;

  /*
   * Find the y coordinate of the glyph in world space.
   */
  if (glyph->which == ZMAP_GLYPH_TRUNCATED_START )
    {
      y = feature->y1 ;
    }
  else if (glyph->which == ZMAP_GLYPH_TRUNCATED_END )
    {
      y = feature->y2 + 1.0 ;
    }
  else
    {
      /* error; but should have been caught already */
      return ;
    }

  /*
   * At last, actually draw the thing.
   */
  paintGlyph(featureset, feature, glyph, y, drawable) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

/* WHY FOO AND FEATURESET...THEY ARE THE SAME THING...... */
void zMapWindowCanvasGlyphDrawGlyph(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                                    ZMapFeatureTypeStyle style, ZMapWindowCanvasGlyph glyph, GdkDrawable *drawable,
                                    double col_width, double optional_ypos)
{
  double score = 0.0, width = 0.0, y = 0.0 ;

  zMapReturnIfFail((featureset && feature && style && glyph && drawable && (col_width > 0.0))) ;


  if (optional_ypos != 0)
    {
      y = optional_ypos ;
    }
  else
    {
      /*
       * Find the y coordinate of the glyph in world space.
       */
      switch (glyph->which)
        {
        case ZMAP_GLYPH_TRUNCATED_START:
          {
            y = feature->y1 ;
            break ;
          }
        case ZMAP_GLYPH_TRUNCATED_END:
          {
            y = feature->y2 + 1.0 ;
            break ;
          }
        case ZMAP_GLYPH_JUNCTION_START:
          {
            y = feature->y1 ;
            break ;
          }
        case ZMAP_GLYPH_JUNCTION_END:
          {
            y = feature->y2 + 1.0 ;
            break ;
          }
        default:
          {
            /* error; but should have been caught already */
            zMapWarnIfReached() ;
            return ;
          }
        }
    }


  /*
   * Basic properties of the glyph. The quantities 'width' and 'height'
   * are ratios, not numbers of pixels or units in world coordinates.
   */
  glyph->width = 1.0 ;
  glyph->height = 1.0 ;


  glyph->origin = col_width/2.0 ;


  if (glyph->scale_to_feature_width)
    width = feature->width ;

  setGlyphCoords(glyph, NULL, style, score, width) ;
  glyph->coords_set = TRUE ;

  /*
   * At last, actually draw the thing.
   */
  paintGlyph(featureset, feature, glyph, y, drawable) ;

  return ;
}





/*
 *       External callbacks (static but called directly from other packages)
 */


/* Init the glyph column, depending on the feature type within the featureset there
 * may be 'per column' data that needs to be set up. */
static void glyphColumnInit(ZMapWindowFeaturesetItem featureset)
{
  setGlyphPerColData(featureset) ;

  return ;
}



/* Paint 'column' level features. */
static void glyphSetPaint(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable, GdkEventExpose *expose)
{
  GlyphAnyColumnData any_glyph_col = NULL ;

  zMapReturnIfFail(featureset) ;

  any_glyph_col = (GlyphAnyColumnData)(featureset->per_column_data) ;

  if (any_glyph_col->glyph_type == ZMAP_GLYPH_SHAPE_GFSPLICE)
    {
      GFSpliceColumnData splice_col = (GFSpliceColumnData)(featureset->per_column_data) ;
      FooCanvasItem *item = (FooCanvasItem *)&(featureset->__parent__) ;

      if (!(splice_col->colours_set))
        {
          if (!(splice_col->colours_set
                = zMapGUIGetColour(GTK_WIDGET(item->canvas),
                                   GF_ZERO_LINE_COLOUR, &(splice_col->zero_line_colour)))
              || !(splice_col->colours_set
                   = zMapGUIGetColour(GTK_WIDGET(item->canvas),
                                      GF_OTHER_LINE_COLOUR, &(splice_col->other_line_colour))))
            zMapLogCritical("%s", "Cannot create colours for GF splice display, no splices will be displayed.") ;
        }

      if (splice_col->colours_set)
        {
          double data_origin, pos_width, line_pos ;
          double x1, x2, y1, y2 ;

          gdk_gc_set_line_attributes(featureset->gc,
                                     DEFAULT_LINE_WIDTH,
                                     GDK_LINE_SOLID,
                                     GDK_CAP_BUTT,
                                     GDK_JOIN_ROUND) ;

          /* Work out where the 'zero' line is for features which can have -ve and +ve scores. */
          data_origin = item->x1 + splice_col->origin ;
          pos_width = splice_col->col_width - splice_col->origin ;

          /* Draw zero line. */
          gdk_gc_set_foreground(featureset->gc, &(splice_col->zero_line_colour)) ;

          x1 = data_origin ;
          x2 = data_origin ;
          y1 = item->y1 ;
          y2 = item->y2 ;
          zMap_draw_line(drawable, featureset, x1, y1, x2, y2) ;

          /* Draw 50% & 75% lines. */
          gdk_gc_set_foreground(featureset->gc, &(splice_col->other_line_colour)) ;

          line_pos = pos_width * 0.5 ;
          x1 = data_origin + line_pos ;
          x2 = data_origin + line_pos ;
          y1 = item->y1 ;
          y2 = item->y2 ;
          zMap_draw_line(drawable, featureset, x1, y1, x2, y2) ;

          line_pos = pos_width * 0.75 ;
          x1 = data_origin + line_pos ;
          x2 = data_origin + line_pos ;
          y1 = item->y1 ;
          y2 = item->y2 ;
          zMap_draw_line(drawable, featureset, x1, y1, x2, y2) ;
        }
    }

  return ;
}



/* Draw handler for the glyph item
 * The interface for free-standing glyphs, called via CanvasFeatureset virtual functions
 * must calculate the shape coords etc on first paint */
static void glyphPaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                              GdkDrawable *drawable, GdkEventExpose *expose)
{
  ZMapWindowCanvasGlyph glyph = (ZMapWindowCanvasGlyph)feature ;
  ZMapFeature feat = NULL ;
  double y1 ;
  int i, min_x, max_x, min_y, max_y, curr_x, curr_y ;

  zMapReturnIfFail(featureset && feature && feature->feature) ;

  feat = feature->feature ;

  y1 = getGlyphFeaturePos(feat) ;

  setGlyphCanvasCoords(featureset, feature, glyph, y1) ;

  paintGlyph(featureset, feature, glyph, y1, drawable) ;

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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zMapUtilsDebugPrintf(stdout, "glyphPaintFeature() x1, x2 y1,2 %d, %d %d, %d\n",
                       min_x, max_x, min_y, max_y) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}


/* Called before any zooming, this column needs to be recalculated with every zoom, this call sets
 * a flag to make sure this happens. This is required because we cache canvas positions for
 * the glyph and they need to be cached. */
static void glyphPreZoom(ZMapWindowFeaturesetItem featureset)
{
  zMapWindowCanvasFeaturesetSetZoomRecalc(featureset, TRUE) ;

  return ;
}



/* Recalculate glyph canvas positions as zoom changes.... */
static void glyphZoom(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable)
{
  /* Go through all features and recalc glyph canvas coords.... */
  g_list_foreach(featureset->features, calcCanvasPos, featureset) ;

  return ;
}


/* we need to calculate the extent of the glyph which is somewhere around the single base coordinate */
static ZMapWindowCanvasFeature glyphAddFeature(ZMapWindowFeaturesetItem featureset,
                                               ZMapFeature feature, double y1, double y2)
{
  GlyphAnyColumnData any_glyph_col = NULL ;
  ZMapWindowCanvasFeature feat = NULL ;
  ZMapStyleGlyphShape shape ;
  ZMapGlyphWhichType boundary_type = (ZMapGlyphWhichType)0 ;

  zMapReturnValIfFail(featureset && featureset->per_column_data && feature, feat) ;

  any_glyph_col = (GlyphAnyColumnData)(featureset->per_column_data) ;


  if (any_glyph_col->glyph_type == ZMAP_GLYPH_SHAPE_GFSPLICE)
    boundary_type = (feature->boundary_type == ZMAPBOUNDARY_5_SPLICE ? ZMAP_GLYPH_FIVEPRIME : ZMAP_GLYPH_THREEPRIME) ;

  shape = getGlyphShape(*feature->style, boundary_type, feature->strand)  ;

  if (!shape)
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
      if (any_glyph_col->glyph_type == ZMAP_GLYPH_SHAPE_GFSPLICE)
        glyph->which = boundary_type ;

      glyph->shape = shape ;

      col_width = zMapStyleGetWidth(featureset->style) ;

      setGlyph(foo, glyph, *feature->style, feature->strand, col_width, feature->score ) ;

      /* as glyphs are fixed size we store the longest as pixels and expand on search */
      /* so here we overwrite the values set by zMapWindowFeaturesetAddFeature() ; */
      if (longest < shape->height)
        longest = shape->height ;
      featureset->longest = longest ;
    }

  return feat ;
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
 * be useless for glyph purposes.
 *
 *
 * THIS CODE HAS BEEN THROUGH SEVERAL (BROKEN) ITERATIONS, SO HERE'S HOW IT STANDS RIGHT NOW:
 *
 * GLYPHS ARE NOT SUPPOSED TO SCALE AS THE CANVAS IS ZOOMED IN/OUT, THEY REMAIN A
 * CONSTANT SIZE SO THEREFORE WE COMPARE THE FOOCANVAS WINDOW COORDS OF THE GLYPH
 * WITH THE FOOCANVAS WINDOW POSITION OF THE CLICK (cx & cy).
 *
 * IF AT A FUTURE DATE GLYPHS BECOME SCALABLE THEN THIS ROUTINE WILL NEED CHANGING
 * TO COPE WITH BOTH TYPES OF GLYPHS.
 *
 *  */
static double glyphPoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
                         double item_x, double item_y, int cx, int cy,
                         double local_x, double local_y, double x_off)
{
  double best = 1.0e36 ;
  ZMapWindowCanvasGlyph glyph = (ZMapWindowCanvasGlyph)gs ;

  zMapReturnValIfFail(glyph && glyph->shape, best) ;


  /* Glyphs only have shape when drawn (I think...) so it's possible when looking for nearby
   * glyphs for the point operation to be looking at glyphs that haven't been drawn and
   * do not have a shape. */
  /* mh17: i added a GlyphAdd function, so we can set the shape etc before paint, but this test is harmless */

  /* OK...LET'S ADD DEBUGGING TO SEE IF THEY ALWAYS HAVE SHAPE..... */
  if (!(glyph->shape))
    {
      zMapLogCritical("%s", "Trying to get position of glyph that has no shape yet !! This is a code error !!") ;
    }
  else
    {
      int i, min_x, max_x, min_y, max_y, curr_x, curr_y ;

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

      /* check y first, more likely to fail.... */
      if ((cy >= min_y && cy <= max_y) && (cx >= min_x && cx <= max_x))
        {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          zMapUtilsDebugPrintf(stdout,
                               "Found glyph\n"
                               "    Click coords - Item x,y: %.9g, %.9g\t canvas x,y: %d, %d\tlocal x.y: %.9g, %.9g\n"
                               "Foocanvas coords - x1, x2, y1, y2: %.9g, %.9g\t%.9g, %.9g\n"
                               "  FeaturesetItem - start,end:  %.9g, %.9g\tdx, dy: %.9g, %.9g\n"
                               "    Glyph coords - y1, y2 %.9g, %.9g\n"
                               "   Glyph Feature - x,y: %d, %d\n"
                               "           Glyph - pos  x1, x2 y1, y2:\t%d, %d  %d, %d\n\n",
                               item_x, item_y, cx, cy, local_x, local_y,
                               foo_item->x1, foo_item->x2, foo_item->y1, foo_item->y2,
                               fi->start, fi->end, fi->dx, fi->dy,
                               glyph->feature.y1, glyph->feature.y2,
                               glyph->feature.feature->x1, glyph->feature.feature->x2,
                               min_x, max_x, min_y, max_y) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

          best = 0.0 ;
        }
    }

  return best ;
}

/* Free the glyph column. */
static void glyphColumnFree(ZMapWindowFeaturesetItem featureset)
{
  zMapReturnIfFail(featureset) ;
  g_free(featureset->per_column_data) ;

  featureset->per_column_data = NULL ;

  return ;
}



/*
 *                    Internal routines
 */

/* get an id for a simple glyph variant or NULL if it's too complex
 * we can get a the same shape in diff styles which may appear in diff columns
 * but glyphs are always the same size if this does not relate to feature score
 * so we can cache globally by glyph shape id
 */
static GQuark getGlyphSignature(ZMapFeatureTypeStyle style, ZMapStrand feature_strand,
                                ZMapGlyphWhichType which, double score)
{
  GQuark id = 0 ;                                            /* zero is the null quark. */

  if (style && (style->score_mode == ZMAPSCORE_INVALID || style->score_mode == ZMAPSTYLE_SCORE_ALT))
    {
      ZMapStyleGlyphShape shape ;

      if ((shape = getGlyphShape(style, which, feature_strand)))
        {
          char *id_str ;
          char strand = '+' ;
          char alt = 'N' ;

          if (feature_strand == ZMAPSTRAND_REVERSE)
            strand = '-';

          if (style->score_mode == ZMAPSTYLE_SCORE_ALT && score < zMapStyleGlyphThreshold(style))
            alt = 'A';

          id_str = g_strdup_printf("%s_%s%c%c",
                                   g_quark_to_string(style->unique_id), g_quark_to_string(shape->id), strand, alt) ;

          id = g_quark_from_string(id_str) ;

          g_free(id_str) ;
        }
    }

  return id ;
}

/* Complicated call to get the right glyph shape for the strand/glyph type...
 *
 * Returns NULL if a shape cannot be found.
 *
 * It would be good to make this more obvious....
 *  */
static ZMapStyleGlyphShape getGlyphShape(ZMapFeatureTypeStyle style, ZMapGlyphWhichType which,  ZMapStrand strand)
{
  ZMapStyleGlyphShape shape ;

  shape = ((which == ZMAP_GLYPH_FIVEPRIME) ? zMapStyleGlyphShape5(style, strand == ZMAPSTRAND_REVERSE)
           : ((which == ZMAP_GLYPH_THREEPRIME) ? zMapStyleGlyphShape3(style, strand == ZMAPSTRAND_REVERSE)
              : (ZMapStyleGlyphShape) zMapStyleGlyphShape(style))) ;

  return shape ;        /* may be NULL */
}


static void setGlyphPerColData(ZMapWindowFeaturesetItem featureset)
{
  zMapReturnIfFail(featureset) ;

  if (zMapStyleIsSpliceStyle(featureset->style))
    {
      /* GF Splice glyphs....derived from acedb representation of genefinder data. */
      GFSpliceColumnData splice_col ;

      splice_col = g_new0(GFSpliceColumnDataStruct, 1) ;

      splice_col->glyph_type = ZMAP_GLYPH_SHAPE_GFSPLICE ;

      splice_col->min_score = zMapStyleGetMinScore(featureset->style) ;
      splice_col->max_score = zMapStyleGetMaxScore(featureset->style) ;

      splice_col->col_width = zMapStyleGetWidth(featureset->style) ;

      splice_col->min_size = ZMAPSTYLE_SPLICE_MIN_LEN ;


      splice_col->scale_factor = splice_col->col_width / (splice_col->max_score - splice_col->min_score) ;

      if (splice_col->min_score < 0 && 0 < splice_col->max_score)
        splice_col->origin = splice_col->col_width * (-(splice_col->min_score)
                                                      / (splice_col->max_score - splice_col->min_score)) ;
      else
        splice_col->origin = 0 ;

      featureset->per_column_data = splice_col ;
    }
  else
    {
      /* Glyphs with no special requirements. */
      GlyphAnyColumnData any_glyph_col ;

      any_glyph_col = g_new0(GlyphAnyColumnDataStruct, 1) ;

      any_glyph_col->glyph_type = ZMAP_GLYPH_SHAPE_ANY ;

      featureset->per_column_data = any_glyph_col ;
    }

  return ;
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
static gboolean setGlyph(FooCanvasItem *foo, ZMapWindowCanvasGlyph glyph,
                         ZMapFeatureTypeStyle style, ZMapStrand feature_strand,
                         double col_width, double score)
{
  ZMapWindowFeaturesetItem featureset = (ZMapWindowFeaturesetItem)foo ;
  GlyphAnyColumnData any_glyph_col = (GlyphAnyColumnData)(featureset->per_column_data) ;
  ZMapStyleScoreMode score_mode = ZMAPSCORE_INVALID;
  double width = 1.0, height = 1.0;                            /* these are ratios not pixels */
  double min_val = 0.0, max_val = 0.0,range;
  ZMapStyleGlyphStrand strand = ZMAPSTYLE_GLYPH_STRAND_INVALID;
  ZMapStyleGlyphAlign align;
  double offset = 0.0, origin = 0.0;
  GdkColor *draw_col = NULL, *fill_col = NULL, *border_col = NULL;


  score_mode = zMapStyleGetScoreMode(style);

  /* a glyph can have different colour if score < threshold so save the actual colour here
   * it's easier to hard code this in the glyph module than have the featureset handle it
   * it is a module specific choice
   * NOTE we can still use selected colours via WINDOW_FOCUS_CACHE_SELECTED
   */

  if (glyph->sub_feature)                /* is sub-feature: use glyph colours not feature */
    {
      if(score_mode == ZMAPSTYLE_SCORE_ALT && score < zMapStyleGlyphThreshold(style))
        zMapStyleGetColours(style, STYLE_PROP_GLYPH_ALT_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill_col, &draw_col, &border_col);
      else
        zMapStyleGetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill_col, &draw_col, &border_col);

      glyph->border_set = FALSE;

      if(border_col)
        {
          gulong pixel;

          glyph->border_set = TRUE;
          pixel = zMap_gdk_color_to_rgba(border_col);
          glyph->border_pixel = foo_canvas_get_color_pixel(foo->canvas, pixel);
        }

      glyph->fill_set = FALSE;

      if (fill_col)
        {
          gulong pixel;

          glyph->fill_set = TRUE;
          pixel = zMap_gdk_color_to_rgba(fill_col);
          glyph->fill_pixel = foo_canvas_get_color_pixel(foo->canvas, pixel);
        }

      glyph->use_glyph_colours = TRUE;
    }


  /* We need splice markers to behave in a particular way for wormbase, I've added this function
   * to get the positions so it doesn't interfere with the general workings of glyphs. */
  if (any_glyph_col && (any_glyph_col->glyph_type == ZMAP_GLYPH_SHAPE_GFSPLICE))
    {
      GFSpliceColumnData splice_col = (GFSpliceColumnData)any_glyph_col ;

      calcSplicePos(glyph->shape, splice_col, style, score, &origin, &width, &height) ;
    }
  else
    {
      /* scale the glyph etc?? according to score, note, we must use the score from the style
       * for anything that's not a gf splice. */
      min_val = zMapStyleGetMinScore(style);
      max_val = zMapStyleGetMaxScore(style);


      range = max_val - min_val;

      // min_val and max_val may be signed... but max_val assumed to be more than min_val :-)
      // if score < min_val then don't display
      // if score > max_val display fill width
      // in between pro-rate

      if(min_val < 0.0)
        {
          if(col_width)                       // origin corresponds to zero
            {
              origin = col_width * (-min_val / range);
              range = score < 0.0 ? -min_val : max_val;
            }
        }
      else
        {
          max_val -= min_val;
          score -= min_val;

          /*
           * not visible: don't draw
           */
          if(score < min_val)
            return FALSE;

          if(col_width)                     // origin is mid point
            origin = col_width / 2.0;
        }

      if (max_val)     // scale if configured
        {
          if(score > max_val)
            score = max_val;
          if(score < min_val)
            score = min_val;

          if (score_mode == ZMAPSCORE_WIDTH || score_mode == ZMAPSCORE_SIZE)
            width = (width * score) / range;
          else if (score_mode == ZMAPSCORE_HEIGHT || score_mode == ZMAPSCORE_SIZE)
            height = (height * score) / range;
        }

      // invert H or V??
      if(feature_strand == ZMAPSTRAND_REVERSE)
        strand = zMapStyleGlyphStrand(style);

      if(strand == ZMAPSTYLE_GLYPH_STRAND_FLIP_X)
        width = -width;

      if(strand == ZMAPSTYLE_GLYPH_STRAND_FLIP_Y)
        height = -height;

      if(col_width)
        {
          // if neither of these are set we'll offset by zero and default to centred glyphs
          // if config is poor they'll look silly so no need for an error message

          align = zMapStyleGetAlign(style);
          offset = col_width / 2;
          if(align == ZMAPSTYLE_GLYPH_ALIGN_LEFT)
            origin = 0.0;                // -= offset;
          else if(align == ZMAPSTYLE_GLYPH_ALIGN_RIGHT)
            origin = col_width;         // += offset;
        }
    }

  /* so now we have origin = (x-offset) and width and height ratios */
  glyph->width = width ;
  glyph->height = height ;
  glyph->origin = origin ;

  setGlyphCoords(glyph, any_glyph_col, style, score, 0) ;
  glyph->coords_set = TRUE ;

  return TRUE ;
}


/* Calculate the position of splice markers so that the horizontal bit of the marker is where
 * the splice is and the vertical bit indicates a 5'(down) or 3'(up) marker. */
static void calcSplicePos(ZMapStyleGlyphShape shape, GFSpliceColumnData splice_col,

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                          ZMapFeature feature,
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

                          ZMapFeatureTypeStyle style, double score,
                          double *feature_origin_out, double *feature_width_out, double *feature_height_out)
{
  double min_score, max_score ;
  double feature_origin ;
  double width = 1.0, height = 1.0;        // these are ratios not pixels
  double x ;

  min_score = splice_col->min_score ;
  max_score = splice_col->max_score ;

  if (score <= min_score)
    x = 0 ;
  else if (score >= max_score)
    x = splice_col->col_width ;
  else
    x = splice_col->scale_factor * (score - min_score) ;

  width = fabs(splice_col->origin - fabs(x)) ;


  if (width < splice_col->min_size)
    {
      if (score < 0)
        {
          feature_origin = splice_col->origin + (splice_col->min_size - width) ;
        }
      else
        {
          feature_origin = splice_col->origin - (splice_col->min_size - width) ;
        }

      width = splice_col->min_size ;
    }
  else
    {
      feature_origin = splice_col->origin  ;
    }

  width = width / shape->width ;

  /* Return left, right and height. */
  if (score < 0)
    {
      /* Splice markers for negative scores are drawn the opposite way round. */
      *feature_width_out = -width ;
    }
  else
    {
      *feature_width_out = width ;
    }
  *feature_height_out = height ;
  *feature_origin_out = feature_origin ;

  return ;
}


/* GFunc() called for each glyph feature item to calculate its canvas position. */
static void calcCanvasPos(gpointer data, gpointer user_data)
{
  ZMapWindowFeaturesetItem featureset = (ZMapWindowFeaturesetItem)user_data ;
  ZMapWindowCanvasFeature canvas_feature = (ZMapWindowCanvasFeature)data ;
  ZMapWindowCanvasGlyph glyph = (ZMapWindowCanvasGlyph)canvas_feature ;
  ZMapFeature feat = canvas_feature->feature ;
  double y1 ;

  y1 = getGlyphFeaturePos(feat) ;

  setGlyphCanvasCoords(featureset, canvas_feature, glyph, y1) ;

  return ;
}


/* Boundary features are usually given by the bases either side so the actual position
 * of the feature is given (counter-intuitively) by feat->x2 which is the point between
 * the two bases. (Remember that each base spans from coord -> coord+1) */
double getGlyphFeaturePos(ZMapFeature feat)
{
  double position = 0 ;

  if (feat->flags.has_boundary)
    {
      int diff ;

      diff = feat->x2 - feat->x1 ;

      if (diff == 1)
        position = feat->x2 ;
      else
        position = (feat->x1 + feat->x2) / 2 ;
    }
  else
    {
      position = feat->x1 ;
    }

  return position ;
}


static void setGlyphCanvasCoords(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                                 ZMapWindowCanvasGlyph glyph, double y1)
{
  /* just draw the glyph */
  /* must recalculate gdk coords as columns can move around */
  FooCanvasItem *item = (FooCanvasItem *)featureset ;
  ZMapWindowCanvasFeature canvas_feature = (ZMapWindowCanvasFeature)feature ;
  double x1 ;
  int cx1, cy1 ;

  x1 = featureset->dx + featureset->x_off ;

  if (featureset->bumped)
    x1 += canvas_feature->bump_offset ;

  y1 += featureset->dy - featureset->start ;

  /* get item canvas coords, following example from FOO_CANVAS_RE (used by graph items) */
  foo_canvas_w2c(item->canvas, x1, y1, &cx1, &cy1) ;
  glyph_to_canvas(glyph->points, glyph->coords, glyph->shape->n_coords, cx1, cy1) ;

  return ;
}



/* set the points to draw centred on the glyph's sequence/column coordinates
 * if we scale the glyph it's done here and NB we have to scale +ve and -ve coords for all shapes
 * NOTE we convert from a list of single coords to GdkPoint coord pairs
 *
 * score is unused at the moment !!!!!!
 *
 */
static void setGlyphCoords(ZMapWindowCanvasGlyph glyph, GlyphAnyColumnData any_col,
                           ZMapFeatureTypeStyle style, double score, double width)
{
  switch(glyph->shape->type)
    {
    case GLYPH_DRAW_ARC:
      {
        // bounding box, don't include the angles, these points get transformed x-ref to _draw()
        if(glyph->shape->n_coords > 2)
          glyph->shape->n_coords = 2 ;

        /* NOTE: fall through !! */
      }
    case GLYPH_DRAW_LINES:
    case GLYPH_DRAW_BROKEN:
    case GLYPH_DRAW_POLYGON:
      {
        gboolean scale_to_feature ;
        int left, right ;
        int i, j ;

        scale_to_feature = glyph->scale_to_feature_width ;

        if (scale_to_feature)
          {
            double half_width ;

            half_width = width / 2 ;

            left = (int)((glyph->origin - half_width) + 0.5) ;
            right = (int)((glyph->origin + half_width) + 0.5) ;
          }

        for (i = j = 0 ; i < glyph->shape->n_coords * 2 ; i++)
          {
            double coord_d ;                                    /* For accurate calculation. */

            if (glyph->shape->coords[i] == GLYPH_COORD_INVALID)
              coord_d = GLYPH_CANVAS_COORD_INVALID ;
            else
              coord_d = glyph->shape->coords[i] ;


            if (i & 1)
              {
                /* Y coords */
                coord_d *= glyph->height ;
                glyph->coords[j].y = (int)(coord_d + 0.5) ;   /* points are centred around the
                                                                 anchor of 0,0. */
                j++ ;
              }
            else
              {
                /* X coords */
                int x ;

                if (scale_to_feature)
                  {
                    /* hack.....what if shape as no -ve coords...duh..... */
                    if (coord_d < 0)
                      x = left ;
                    else
                      x = right ;
                  }
                else
                  {
                    coord_d *= glyph->width ;

                    /* hack because no any_col */
                    if (any_col && any_col->glyph_type == ZMAP_GLYPH_SHAPE_ANY)
                      {
                        if (glyph->width < 0)
                          coord_d = -coord_d ;
                      }

                    x = (int)(glyph->origin + coord_d + 0.5) ; /* points are centred around
                                                                  the anchor of 0,0 */
                  }

                glyph->coords[j].x = x ;
              }
          }

        break;
      }

    default:
      {
        zMapWarnIfReached() ;

        break;
      }
    }

  return ;
}



// x-ref with zmapWindowDump.c/dumpGlyph()....why ????
/* handle sub feature and free standing glyphs */
static void paintGlyph(ZMapWindowFeaturesetItem featureset,
                       ZMapWindowCanvasFeature canvas_feature,
                       ZMapWindowCanvasGlyph glyph,
                       double y1, GdkDrawable *drawable)
{
  GlyphAnyColumnData any_glyph_col = (GlyphAnyColumnData)(featureset->per_column_data) ;
  gulong ufill,outline ;
  GdkColor c ;
  int colours_set, fill_set = 0, outline_set = 0 ;

  setGlyphCanvasCoords(featureset, canvas_feature, glyph, y1) ;

  /* we have pre-calculated pixel colours */
  colours_set = zMapWindowCanvasFeaturesetGetColours(featureset, canvas_feature, &ufill, &outline) ;
  fill_set = colours_set & WINDOW_FOCUS_CACHE_FILL ;
  outline_set = colours_set & WINDOW_FOCUS_CACHE_OUTLINE ;

  if (glyph->use_glyph_colours)
    {
      fill_set = glyph->fill_set;
      ufill = glyph->fill_pixel;

      outline_set = glyph->border_set;
      outline = glyph->border_pixel;
    }

  if (any_glyph_col && (any_glyph_col->glyph_type == ZMAP_GLYPH_SHAPE_GFSPLICE))
    gdk_gc_set_line_attributes(featureset->gc, SPLICE_LINE_WIDTH, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER) ;
  else
    gdk_gc_set_line_attributes(featureset->gc, DEFAULT_LINE_WIDTH, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND) ;

  if (fill_set)
    {
      c.pixel = ufill;
      gdk_gc_set_foreground (featureset->gc, &c);
      glyphDraw(featureset, glyph, drawable, TRUE);
    }

  if (outline_set)
    {
      c.pixel = outline;
      gdk_gc_set_foreground (featureset->gc, &c);
      glyphDraw(featureset, glyph, drawable, FALSE);
    }

  return ;
}


/* Calculate actual pixel coordinates on the foocanvas window, the glyph is centred around cx,cy.
 *
 * NOTE these coords are NOT the viewport coords but the coords on the zoomed foocanvas window. */
static void glyph_to_canvas(GdkPoint *points, GdkPoint *coords, int count, int cx, int cy)
{
  int i ;

  for (i = 0 ; i < count ; i++, points++, coords++)
    {
      points->x = coords->x + cx ;
      points->y = coords->y + cy ;
    }

  return ;
}


static void glyphDraw(ZMapWindowFeaturesetItem featureset,
                      ZMapWindowCanvasGlyph glyph, GdkDrawable *drawable, gboolean fill_flag)
{
  GlyphAnyColumnData any_glyph_col = (GlyphAnyColumnData)(featureset->per_column_data) ;


  /* Note that sub feature glyphs do not cache the feature. */

  if ((glyph->feature.feature) && any_glyph_col->glyph_type == ZMAP_GLYPH_SHAPE_GFSPLICE)
    {
      GdkColor c;
      gulong fill_pixel = 0, outline_pixel = 0 ;
      int i, min_x, max_x, min_y, max_y, curr_x, curr_y ;
      int status ;


      status = zMapWindowCanvasFeaturesetGetColours(featureset,
                                                    (ZMapWindowCanvasFeature)glyph,
                                                    &fill_pixel, &outline_pixel) ;
      if (fill_flag)
        {
          /* For GF splices only do this when focussed */
          if (glyph->feature.flags & WINDOW_FOCUS_GROUP_FOCUSSED)
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

              /* Somehow lines/rectangles are not matching up.....this is a hack until I can find
               * out why... */
              if (glyph->feature.feature->boundary_type == ZMAPBOUNDARY_5_SPLICE)
                {
                  max_y-- ;

                  if (glyph->feature.feature->score < 0)
                    max_x-- ;
                }

              /* Note we draw the rectangle across the whole glyph and then draw the outline on top,
               * it's too complicated to do anything else as the glyph does not have a complete outline. */
              c.pixel = fill_pixel ;
              gdk_gc_set_foreground(featureset->gc, &c) ;
              gdk_draw_rectangle(drawable,
                                 featureset->gc,
                                 TRUE,
                                 min_x,
                                 min_y,
                                 ((max_x - min_x) + 1),
                                 ((max_y - min_y) + 1)) ;

              c.pixel = outline_pixel ;
              gdk_gc_set_foreground(featureset->gc, &c);
              gdk_draw_lines(drawable, featureset->gc, glyph->points, glyph->shape->n_coords);
            }
        }
      else
        {
          c.pixel = outline_pixel;
          gdk_gc_set_foreground(featureset->gc, &c);
          gdk_draw_lines(drawable, featureset->gc, glyph->points, glyph->shape->n_coords);
        }

    }
  else
    {
      int start, end ;

      switch (glyph->shape->type)
        {
        case GLYPH_DRAW_LINES:
          {
            int debug = FALSE ;
            int i ;

            if (glyph->shape->n_coords)
              {
                for (i = 0 ; i < glyph->shape->n_coords ; i++)
                  {
                    zMapDebugPrint(debug, "Point %d:\t%d, %d", i, glyph->points[i].x, glyph->points[i].y) ;
                  }
              }

            gdk_draw_lines(drawable, featureset->gc, glyph->points, glyph->shape->n_coords) ;

            break;
          }
        case GLYPH_DRAW_BROKEN:
          {
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
          }
        case GLYPH_DRAW_POLYGON:
          {
            gdk_draw_polygon(drawable, featureset->gc, fill_flag,  glyph->points, glyph->shape->n_coords);

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

            gdk_draw_arc(drawable, featureset->gc, fill_flag, (int) x1, (int) y1, (int) (x2 - x1), (int) (y2 - y1), a1,a2);

            break;
          }
        default:
          {
            zMapWarnIfReached() ;

            break;
          }
        }
    }

  return ;
}




