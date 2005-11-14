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
 * Last edited: Nov 14 11:01 2005 (rds)
 * Created: Tue Jul 27 16:40:47 2004 (edgrif)
 * CVS info:   $Id: zmapDraw.h,v 1.22 2005-11-14 12:01:30 rds Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_DRAW_H
#define ZMAP_DRAW_H

#include <libfoocanvas/libfoocanvas.h>

typedef struct _ZMapDrawTextIteratorStruct
{
  int iteration;
  double x;
  double y;
  int offset;
  int line_height;
  int seq_start;
  int seq_length;
  int drawable_seq_length;
  int rows;
  int cols;
  char *format;
  double text_height;
} ZMapDrawTextIteratorStruct, *ZMapDrawTextIterator;

typedef struct ZMapDrawTextRowDataStruct_
{
  int sequenceOffset;
  int textWriteOffset;          /* should be the y coord of where the text is drawn */
  int fullStrLength;            /* the string might get clipped. */
  int drawnStrLength;           /* this is the length of the string drawn */
  double columnWidth;
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
                                 int full_str_length,
                                 int drawn_str_length,
                                 int offset,
                                 double column_width);
FooCanvasItem *zMapDrawRowOfText(FooCanvasGroup *group,
                                 PangoFontDescription *fixed_font,
                                 char *fullText, 
                                 ZMapDrawTextIterator iterator);
ZMapDrawTextRowData zMapDrawGetTextItemData(FooCanvasItem *item);

#endif /* ZMAP_DRAW_H */
