/*  File: zmapDraw_P.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Nov 15 14:47 2005 (rds)
 * Created: Mon Jul 18 09:14:38 2005 (rds)
 * CVS info:   $Id: zmapDraw_P.h,v 1.3 2005-11-16 10:32:45 rds Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_DRAW_P_H
#define ZMAP_DRAW_P_H

#include <string.h>
#include <glib.h>
#include <math.h>
#include <ZMap/zmapDraw.h>
#include <ZMap/zmapUtils.h>

#define ZMAP_SCALE_MINORS_PER_MAJOR 10
#define ZMAP_FORCE_FIVES TRUE
#define MAX_TEXT_COLUMN_WIDTH (300.0)
#define ZMAP_DRAW_TEXT_ROW_DATA_KEY "text_row_data"

enum {
  REGION_ROW_LEFT,
  REGION_ROW_TOP,
  REGION_ROW_RIGHT,
  REGION_ROW_BOTTOM,
  REGION_ROW_EXTENSION_LEFT,
  REGION_ROW_EXTENSION_RIGHT,
  REGION_ROW_JOIN_TOP,
  REGION_ROW_JOIN_BOTTOM,
  REGION_LAST_LINE
};

/* Just a collection of ints, boring but makes it easier */
typedef struct _ZMapScaleBarStruct
{
  int base;                     /* One of 1 1e3 1e6 1e9 1e12 1e15 */
  int major;                    /* multiple of base */
  int minor;                    /* major / ZMAP_SCALE_MINORS_PER_MAJOR */
  char *unit;                   /* One of bp k M G T P */

  gboolean force_multiples_of_five;
  double zoom_factor;

  int start;
  int end;
  double top;
  double bottom;
} ZMapScaleBarStruct, *ZMapScaleBar;

static ZMapScaleBar createScaleBar_start_end_zoom_height(double start, double end, double zoom, double line);
static void drawScaleBar(ZMapScaleBar scaleBar, FooCanvasGroup *group, PangoFontDescription *font);
static void destroyScaleBar(ZMapScaleBar scaleBar);



#endif /* ZMAP_DRAW_P_H */
