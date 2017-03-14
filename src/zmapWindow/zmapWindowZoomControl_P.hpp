/*  File: zmapWindowZoomControl_P.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description:
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAPWINDOWZOOMCONTROL_P_H
#define ZMAPWINDOWZOOMCONTROL_P_H

#include <ZMap/zmapUtils.hpp>
#include <zmapWindow_P.hpp>




/* This is the same as MAX_TEXT_COLUMN_WIDTH in zmapDraw_P.h This
 * isn't good, but there's must more to do to make it part of the
 * style for dna!!! */
#define ZMAP_ZOOM_MAX_TEXT_COLUMN_WIDTH (300.0)



enum
  {
    ZMAP_WINDOW_BROWSER_HEIGHT = 160,
    ZMAP_WINDOW_BROWSER_WIDTH  = 200
  };


typedef struct _ZMapWindowZoomControlStruct
{
  ZMapMagic magic;

  ZMapWindowZoomStatus status ;

  double zF ;
  double minZF ;
  double maxZF ;

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


} ZMapWindowZoomControlStruct;

#endif

