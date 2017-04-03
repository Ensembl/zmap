/*  File: zmapDraw_P.h
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
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_DRAW_P_H
#define ZMAP_DRAW_P_H

#include <string.h>
#include <glib.h>
#include <math.h>
#include <ZMap/zmapDraw.hpp>
#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapFeature.hpp>

#define ZMAP_SCALE_MINORS_PER_MAJOR 10
#define ZMAP_FORCE_FIVES TRUE
#define MAX_TEXT_COLUMN_WIDTH (300.0)
#define ZMAP_DRAW_TEXT_ROW_DATA_KEY "text_row_data"
#define ZMAP_EXON_POINT_SIZE (0.5)

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

/* some nautical points to remove confusion */
typedef enum 
  {
    BOW_PORT = 1,
    BOW_BOW,
    BOW_STARBOARD,
    STERN_STARBOARD,
    STERN_STERN,
    STERN_PORT,

    POINT_MAX
  } basicPoints;

typedef enum
  {
    REVERSE_STRANDED = -1,
    FORWARD_STRANDED = 1
  } strand;

typedef enum
  {
    VERTICAL   = 16,
    HORIZONTAL = 32
  } orientation;

typedef enum
  {
    POINT_BOW_PORT_REVERSE        = (BOW_PORT        + VERTICAL) * REVERSE_STRANDED,
    POINT_BOW_BOW_REVERSE         = (BOW_BOW         + VERTICAL) * REVERSE_STRANDED,
    POINT_BOW_STARBOARD_REVERSE   = (BOW_STARBOARD   + VERTICAL) * REVERSE_STRANDED,
    POINT_STERN_STARBOARD_REVERSE = (STERN_STARBOARD + VERTICAL) * REVERSE_STRANDED,
    POINT_STERN_STERN_REVERSE     = (STERN_STERN     + VERTICAL) * REVERSE_STRANDED,
    POINT_STERN_PORT_REVERSE      = (STERN_PORT      + VERTICAL) * REVERSE_STRANDED,

    POINT_UNKNOWN = 0,          /* Error point. */

    POINT_BOW_PORT_FORWARD        = (BOW_PORT        + VERTICAL) * FORWARD_STRANDED,
    POINT_BOW_BOW_FORWARD         = (BOW_BOW         + VERTICAL) * FORWARD_STRANDED,
    POINT_BOW_STARBOARD_FORWARD   = (BOW_STARBOARD   + VERTICAL) * FORWARD_STRANDED,
    POINT_STERN_STARBOARD_FORWARD = (STERN_STARBOARD + VERTICAL) * FORWARD_STRANDED,
    POINT_STERN_STERN_FORWARD     = (STERN_STERN     + VERTICAL) * FORWARD_STRANDED,
    POINT_STERN_PORT_FORWARD      = (STERN_PORT      + VERTICAL) * FORWARD_STRANDED,

    POINT_UNKNOWN_FORWARD
  } ZMapBoxPointPosition;


#endif /* ZMAP_DRAW_P_H */
