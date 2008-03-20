/*  File: zmapWindowItemTextFillColumn.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Mar 20 12:26 2008 (rds)
 * Created: Mon Apr  2 11:51:25 2007 (rds)
 * CVS info:   $Id: zmapWindowItemTextFillColumn.h,v 1.4 2008-03-20 13:24:55 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAPWINDOWITEMTEXTFILLCOLUMN_H
#define ZMAPWINDOWITEMTEXTFILLCOLUMN_H

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsFoo.h>

/* enum for the return item coords for 
 * zmapWindowItemTextIndexGetBounds()  
 * TL means Top Left, BR means Bottom Right
 */
enum
  {
    ITEMTEXT_CHAR_BOUND_TL_X  = 0,
    ITEMTEXT_CHAR_BOUND_TL_Y  = 1,
    ITEMTEXT_CHAR_BOUND_BR_X  = 2,
    ITEMTEXT_CHAR_BOUND_BR_Y  = 3,
    ITEMTEXT_CHAR_BOUND_COUNT = 4
  };

#ifdef LOAD_OF_OLD_CODE
enum
  {
    ITEMTEXT_ELIPSIS_SIZE     = 3,
  };

typedef struct
{
  gboolean initialised, truncated;
  double seq_start, seq_end;
  double offset_start;
  int truncate_at;
  double region_range;
  int rows, cols, last_populated_cell;
  double real_char_height;
  GString *row_text;
  int index_start, current_idx;
  GdkColor *text_colour;
  char elipsis[ITEMTEXT_ELIPSIS_SIZE];
}ZMapWindowItemTextIteratorStruct, *ZMapWindowItemTextIterator;

typedef struct _ZMapWindowItemTextContextStruct *ZMapWindowItemTextContext;

typedef struct
{
  double feature_start, feature_end;
  double visible_y1, visible_y2, s2c_offset;
  int bases_per_char;
  double width;
  FooCanvas *canvas;
  ZMapFeature feature;
}ZMapWindowItemTextEnvironmentStruct, *ZMapWindowItemTextEnvironment;      


ZMapWindowItemTextContext zmapWindowItemTextContextCreate(GdkColor *text_colour);
void zmapWindowItemTextContextInitialise(ZMapWindowItemTextContext     context,
                                         ZMapWindowItemTextEnvironment environment);
void zmapWindowItemTextGetWidthLimitBounds(ZMapWindowItemTextContext context,
                                           double *width_min, double *width_max);
gboolean zmapWindowItemTextIndexGetBounds(ZMapWindowItemTextContext context,
                                          int index, double *item_world_coords_out);
ZMapWindowItemTextIterator zmapWindowItemTextContextGetIterator(ZMapWindowItemTextContext context,
                                                                double column_width);
gboolean zmapWindowItemTextWorldToIndex(ZMapWindowItemTextContext context, 
                                        FooCanvasItem *text_item,
                                        double x, double y, int *index_out);
void zmapWindowItemTextCharBounds2OverlayPoints(ZMapWindowItemTextContext context,
                                                double *first, double *last,
                                                FooCanvasPoints *overlay_points);
void zmapWindowItemTextContextDestroy(ZMapWindowItemTextContext context);
#endif /* LOAD_OF_OLD_CODE */


void zmapWindowItemTextOverlayFromCellBounds(FooCanvasPoints *overlay_points,
					     double          *first, 
					     double          *last, 
					     double           minx, 
					     double           maxx);
void zmapWindowItemTextOverlayText2Overlay(FooCanvasItem   *item, 
					   FooCanvasPoints *points);
gboolean zmapWindowItemTextIndex2Item(FooCanvasItem *item,
				      int index,
				      double *bounds);


#endif /* ZMAPWINDOWITEMTEXTFILLCOLUMN_H  */
