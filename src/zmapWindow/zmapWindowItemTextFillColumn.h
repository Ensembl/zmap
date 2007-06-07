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
 * Last edited: Jun  7 10:37 2007 (rds)
 * Created: Mon Apr  2 11:51:25 2007 (rds)
 * CVS info:   $Id: zmapWindowItemTextFillColumn.h,v 1.1 2007-06-07 13:19:55 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAPWINDOWITEMTEXTFILLCOLUMN_H
#define ZMAPWINDOWITEMTEXTFILLCOLUMN_H

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsFoo.h>

#define ELIPSIS_SIZE 3

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
  char elipsis[ELIPSIS_SIZE];
}ZMapWindowItemTextIteratorStruct, *ZMapWindowItemTextIterator;

typedef struct _ZMapWindowItemTextContextStruct *ZMapWindowItemTextContext;

typedef struct
{
  double feature_start, feature_end;
  double visible_y1, visible_y2, s2c_offset;
  int bases_per_char;
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
ZMapWindowItemTextIterator zmapWindowItemTextContextGetIterator(ZMapWindowItemTextContext context);
void zmapWindowItemTextContextDestroy(ZMapWindowItemTextContext context);



#endif /* ZMAPWINDOWITEMTEXTFILLCOLUMN_H  */
