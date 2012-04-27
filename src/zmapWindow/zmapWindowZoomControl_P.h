/*  File: zmapWindowZoomControl_P.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#ifndef ZMAPWINDOWZOOMCONTROL_P_H
#define ZMAPWINDOWZOOMCONTROL_P_H

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>




/* This is the same as MAX_TEXT_COLUMN_WIDTH in zmapDraw_P.h This
 * isn't good, but there's must more to do to make it part of the
 * style for dna!!! */
#define ZMAP_ZOOM_MAX_TEXT_COLUMN_WIDTH (300.0)



enum {
  ZMAP_WINDOW_BROWSER_HEIGHT = 160,
  ZMAP_WINDOW_BROWSER_WIDTH  = 200
};

typedef struct _ZMapWindowZoomControlStruct
{
  ZMapMagic magic;

  double zF;
  double minZF;
  double maxZF;

  double textHeight;
  double textWidth;

  double pix2mmy ;					    /* Convert pixels -> screen mm. */
  double pix2mmx ;					    /* Convert pixels -> screen mm. */

  double max_window_size;
  int border;

  PangoFontDescription *font_desc;
  PangoFont *font; /* This needs to be a fixed
                    * width font. We use it to
                    * display the DNA which
                    * absolutely requires
                    * that. */

  ZMapWindowZoomStatus status;
} ZMapWindowZoomControlStruct;

#endif
