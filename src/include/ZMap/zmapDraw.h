/*  File: zmapDraw.h
 *  Author: Rob Clack (rnc@sanger.ac.uk)
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Defines interface to drawing routines for features
 *              in the ZMap.
 *              
 * HISTORY:
 * Last edited: Sep  5 10:18 2007 (edgrif)
 * Created: Tue Jul 27 16:40:47 2004 (edgrif)
 * CVS info:   $Id: zmapDraw.h,v 1.39 2010-03-04 15:15:00 mh17 Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_DRAW_H
#define ZMAP_DRAW_H

#include <libfoocanvas/libfoocanvas.h>


/* Some basic drawing types. */
typedef enum
  {
    ZMAPDRAW_OBJECT_INVALID,
    ZMAPDRAW_OBJECT_LINE,
    ZMAPDRAW_OBJECT_GLYPH,
    ZMAPDRAW_OBJECT_BOX,
    ZMAPDRAW_OBJECT_POLYGON,
    ZMAPDRAW_OBJECT_TEXT
  } ZMapDrawObjectType ;


/* Different simple shapes that can be drawn on to the canvas. */
typedef enum
  {
    ZMAPDRAW_GLYPH_INVALID,

    ZMAPDRAW_GLYPH_LINE,				    /* A simple horizontal line. */

    ZMAPDRAW_GLYPH_ARROW,				    /* A crude arrow. */

    ZMAPDRAW_GLYPH_DOWN_BRACKET,			    /* Horizontal "L" shapes pointing up or down. */
    ZMAPDRAW_GLYPH_UP_BRACKET
  } ZMapDrawGlyphType ;


typedef enum
  {
    ZMAP_MIN_POINTS  = 3,

    ZMAP_FOUR_POINTS,
    ZMAP_FIVE_POINTS,
    ZMAP_SIX_POINTS,

    ZMAP_MAX_POINTS
  } ZMapExonPoints;

/* Simple diagrams missing horizontal lines to make polygon.
 * They will also be rotated to vertical (clockwise) in zmap.

 * Without checking too much I've used terms port and starboard to
 * identify the left and right side of the feature in a direction
 * dependent way. Internally this is true as well with the addition
 * of bow and stern to describe the "front" and "back" of a feature.
 * ZMAP_POLYGON_POINTING probably illustrates this best!

 */
typedef enum
  {
    ZMAP_POLYGON_NONE,            /* NOT A POLYGON Nothing will be drawn */
    ZMAP_POLYGON_SQUARE,          /* |  | */
    ZMAP_POLYGON_POINTING,        /* <  | */
    ZMAP_POLYGON_DOUBLE_POINTING, /* <  < */
    ZMAP_POLYGON_TRAPEZOID_PORT,  /* /  \ possibly /\ (It will point to port) */
    ZMAP_POLYGON_TRIANGLE_PORT,   /* /  \ possibly /\ (It will point to port) */
    ZMAP_POLYGON_TRAPEZOID_STAR,  /* \  / possibly \/ (It will point to starboard) */
    ZMAP_POLYGON_TRIANGLE_STAR,   /* \  / possibly \/ (It will point to starboard) */
    ZMAP_POLYGON_STAR_PORT,
    ZMAP_POLYGON_STAR_STAR,
    ZMAP_POLYGON_DIAMOND          /* <  > possibly <> i.e. diamond */
  } ZMapPolygonForm;

/* Warning most of the below aren't written yet */
typedef enum
  {
    ZMAP_ANNOTATE_NONE,         /* NOT AN ANNOTATION Nothing will be drawn */
    ZMAP_ANNOTATE_INTRON,       /* These always point right in vertical orientation */
    ZMAP_ANNOTATE_GAP,          /* draws a line between bow and stern centered between port and starboard */
    ZMAP_ANNOTATE_BOW_ARROW,    /* draws a centered arrow (line) towards bow */
    ZMAP_ANNOTATE_STERN_ARROW,    /* draws a centered arrow (line) towards bow */
    ZMAP_ANNOTATE_BOW_BOUNDARY,     /* draws a line on top of the bow */
    ZMAP_ANNOTATE_BOW_BOUNDARY_REGION, /* draws a polygon on top of original one from point dimension to bow */
    ZMAP_ANNOTATE_STERN_BOUNDARY,     /* draws a line on top of the stern */
    ZMAP_ANNOTATE_STERN_BOUNDARY_REGION, /* draws a polygon on top of original one from point dimension to stern */
    ZMAP_ANNOTATE_TRIANGLE_PORT, /* draws a centered triangle pointing 2 port */
    ZMAP_ANNOTATE_TRIANGLE_STAR, /* draws a centered triangle pointing 2 starboard */
    ZMAP_ANNOTATE_UTR_FIRST,     /* Resizes original polygon and butts up another at point dimension */
    ZMAP_ANNOTATE_UTR_LAST,      /* Resizes original polygon and butts up another at point dimension */
    ZMAP_ANNOTATE_EXTEND_ALIGN
  } ZMapAnnotateForm;

typedef enum
  {
    ZMAP_POINT_START_NORTHWEST = 0,
    ZMAP_POINT_START_NORTH,
    ZMAP_POINT_START_NORTHEAST,
    ZMAP_POINT_END_SOUTHEAST,
    ZMAP_POINT_END_SOUTH,
    ZMAP_POINT_END_SOUTHWEST
  } ZMapExonCompassPoints;

typedef enum
  {
    ZMAP_TEXT_HIGHLIGHT_NONE = 0,
    ZMAP_TEXT_HIGHLIGHT_MARQUEE,
    ZMAP_TEXT_HIGHLIGHT_BACKGROUND
  } ZMapDrawTextHighlightStyle;

typedef struct ZMapDrawTextRowDataStruct_
{
  /* indices to the string we're displaying */
  /* i.e. slicing the string sequence[seq_index_start..seq_index_end] */
  /* we draw the string sequence[seq_index_start..seq_truncated_idx] */
  int seq_index_start, seq_truncated_idx, seq_index_end; 
  int curr_first_index;
  int chars_on_screen, chars_drawn;

  double char_width, char_height;
  double row_width, row_height;

  GdkColor *background;
  GdkColor *outline;

  ZMapDrawTextHighlightStyle highlight_style;
  /* text's char width is columnWidth / drawnStrLength */
} ZMapDrawTextRowDataStruct, *ZMapDrawTextRowData;

typedef struct _ZMapDrawTextIteratorStruct
{
  int iteration;                /* current iteration... */
  int rows, cols;               /* number of rows in iteration, columns per row */
  int lastPopulatedCell;        /* last cell to draw in in table */
  int truncate_at;              /* only draw this number of the columns */
  int index_start;              /* like offset_start, but the index in wrap_text */

  gboolean truncated;           /* flag to say we're not drawing all columns */

  double x, y;                  /* x & y coords of text NW anchored */
  double seq_start, seq_end;    /* start and end of sequence */
  double char_width, char_height; /* text dimensions */
  double offset_start;          /* item coord offset of first row (add to all!) */

  double x1, y1, x2, y2;        /* current row item bounds */

  char *wrap_text;              /* full text we're drawing */
  GString *row_text;            /* current row's text */

  ZMapDrawTextRowData row_data;

  /* colours */
  GdkColor *foreground;
  GdkColor *background;
  GdkColor *outline;
} ZMapDrawTextIteratorStruct, *ZMapDrawTextIterator;



FooCanvasItem *zMapDrawGlyph(FooCanvasGroup *group, double x, double y, ZMapDrawGlyphType glyph,
			     GdkColor *colour, double width, guint line_width) ;
FooCanvasItem *zMapDrawLine(FooCanvasGroup *group, double x1, double y1, double x2, double y2, 
			    GdkColor *colour, guint line_width) ;
FooCanvasItem *zMapDrawLineFull(FooCanvasGroup *group, FooCanvasGroupPosition position,
				double x1, double y1, double x2, double y2, 
				GdkColor *colour, guint line_width) ;
FooCanvasItem *zMapDrawPolyLine(FooCanvasGroup *group, FooCanvasPoints *points,
				GdkColor *colour, guint line_width) ;
FooCanvasItem *zMapDrawBox(FooCanvasGroup *group, 
			   double x1, double y1, double x2, double y2, 
			   GdkColor *line_colour, GdkColor *fill_colour, guint line_width) ;
FooCanvasItem *zMapDrawBoxFull(FooCanvasGroup *group,  FooCanvasGroupPosition position,
			       double x1, double y1, double x2, double y2, 
			       GdkColor *line_colour, GdkColor *fill_colour, guint line_width) ;

FooCanvasItem *zMapDrawBoxSolid(FooCanvasGroup *group, 
				double x1, double y1, double x2, double y2, 
				GdkColor *fill_colour) ;
FooCanvasItem *zMapDrawBoxOverlay(FooCanvasGroup *group, 
				  double x1, double y1, double x2, double y2, 
				  GdkColor *fill_colour) ;
void zMapDrawBoxChangeSize(FooCanvasItem *box, 
			   double x1, double y1, double x2, double y2) ;

FooCanvasItem *zMapDisplayText(FooCanvasGroup *group, char *text, char *colour,
			       double x, double y) ;

FooCanvasItem *zMapDrawSSPolygon(FooCanvasItem *grp, ZMapPolygonForm form,
                                 double x1, double y1, double x2, double y2, 
                                 GdkColor *border, GdkColor *fill, guint line_width, int zmapStrand);
FooCanvasItem *zMapDrawAnnotatePolygon(FooCanvasItem *polygon, 
                                       ZMapAnnotateForm form,
                                       GdkColor *border,
                                       GdkColor *fill,
                                       double thickness,
				       guint line_width,
                                       int zmapStrand);

FooCanvasItem *zMapDrawScale(FooCanvas *canvas, 
                             PangoFontDescription *font,
                             double zoom_factor, 
                             double start, double end,
                             double height);


/* This needs to be a bit cleverer, so you can't actually move the origin */
FooCanvasItem *zMapDrawRubberbandCreate(FooCanvas *canvas);

void zMapDrawRubberbandResize(FooCanvasItem *band, 
                              double origin_x, double origin_y, 
                              double current_x, double current_y
                              );

FooCanvasItem *zMapDrawHorizonCreate(FooCanvas *canvas);

void zMapDrawHorizonReposition(FooCanvasItem *line, double current_x);

void zMapDrawGetTextDimensions(FooCanvasGroup *group, double *width_out, double *height_out) ;

FooCanvasGroup *zMapDrawToolTipCreate(FooCanvas *canvas);
void zMapDrawToolTipSetPosition(FooCanvasGroup *tooltip, double x, double y, char *text);

/* For drawing/highlighting text on the canvas e.g. dna & 3 frame trans.*/
void zMapDrawHighlightTextRegion(FooCanvasGroup *grp,                                  
                                 double firstx, double firsty,
                                 double lastx,  double lasty,
                                 FooCanvasItem *textItem);
FooCanvasItem *zMapDrawTextWithFont(FooCanvasGroup *parent,
                                    char *some_text,
                                    PangoFontDescription *font_desc,
                                    double x, double y,
                                    GdkColor *text_color);

#endif /* ZMAP_DRAW_H */
