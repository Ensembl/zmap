/*  File: zmapDraw.h
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 * Last edited: Mar 29 17:37 2006 (rds)
 * Created: Tue Jul 27 16:40:47 2004 (edgrif)
 * CVS info:   $Id: zmapDraw.h,v 1.26 2006-03-30 14:20:33 rds Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_DRAW_H
#define ZMAP_DRAW_H

#include <libfoocanvas/libfoocanvas.h>

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

typedef struct _ZMapDrawTextIteratorStruct
{
  int iteration;

  double x, y;
  double seq_start, seq_end;
  int offset_start, offset_end;

  int shownSeqLength;
  int length2draw;
  int n_bases;

  int rows, cols;

  char *format;
  gboolean numbered;

  GdkColor *foreground;
  GdkColor *background;
  GdkColor *outline;
} ZMapDrawTextIteratorStruct, *ZMapDrawTextIterator;

typedef struct ZMapDrawTextRowDataStruct_
{
  int sequenceOffset;           /* This is the offset of the scroll region in characters */

  int rowOffset;                /* The offset of the character */

  int fullStrLength;            /* the string might get clipped. */
  int drawnStrLength;           /* this is the length of the string drawn */

  double columnWidth;           /* This is the width of the column */
  GdkColor *background;
  GdkColor *outline;
  /* text's char width is columnWidth / drawnStrLength */
} ZMapDrawTextRowDataStruct, *ZMapDrawTextRowData;


//#define MINVAL(x, y) ((x) < (y) ? (x) : (y))
//#define MAXVAL(x, y) ((x) > (y) ? (x) : (y))

FooCanvasItem *zMapDrawLine(FooCanvasGroup *group, double x1, double y1, double x2, double y2, 
			    GdkColor *colour, double thickness) ;
FooCanvasItem *zMapDrawPolyLine(FooCanvasGroup *group, FooCanvasPoints *points,
				GdkColor *colour, double thickness) ;
FooCanvasItem *zMapDrawBox(FooCanvasItem *group, 
			   double x1, double y1, double x2, double y2, 
			   GdkColor *line_colour, GdkColor *fill_colour) ;
FooCanvasItem *zMapDrawSolidBox(FooCanvasItem *group, 
				double x1, double y1, double x2, double y2, 
				GdkColor *fill_colour) ;
FooCanvasItem *zMapDisplayText(FooCanvasGroup *group, char *text, char *colour,
			       double x, double y) ;

FooCanvasItem *zMapDrawSSPolygon(FooCanvasItem *grp, ZMapPolygonForm form,
                                 double x1, double y1, double x2, double y2, 
                                 GdkColor *border, GdkColor *fill, int zmapStrand);

FooCanvasItem *zMapDrawAnnotatePolygon(FooCanvasItem *polygon, 
                                       ZMapAnnotateForm form,
                                       GdkColor *border,
                                       GdkColor *fill,
                                       double thickness,
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
                                 int y1Idx,
                                 int y2Idx,
                                 FooCanvasItem *textItem);
FooCanvasItem *zMapDrawRowOfText(FooCanvasGroup *group,
                                 PangoFontDescription *fixed_font,
                                 char *fullText, 
                                 ZMapDrawTextIterator iterator);
ZMapDrawTextRowData zMapDrawGetTextItemData(FooCanvasItem *item);

#endif /* ZMAP_DRAW_H */
