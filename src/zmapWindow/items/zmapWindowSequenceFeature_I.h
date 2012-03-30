/*  File: zmapWindowSequenceFeature_I.h
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
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Internal header for object representing either a
 *              DNA or a Peptide sequence. Handles display, coords,
 *              highlighting.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_SEQUENCE_FEATURE_I_H
#define ZMAP_WINDOW_SEQUENCE_FEATURE_I_H

#include <zmapWindowSequenceFeature.h>
#include <zmapWindowCanvasItem_I.h>


/* OH GOSH....WHAT IS SO WRONG WITH ADDING A FEW COMMENTS...THIS IS SO TEDIOUS NOW....Ed */


typedef enum
  {
    SEQUENCE_SELECTED_SIGNAL,
    SEQUENCE_LAST_SIGNAL,
  } ZMapWindowSequenceFeatureSignalType ;


typedef struct _zmapWindowSequenceFeatureClassStruct
{
  zmapWindowCanvasItemClass __parent__;

  ZMapWindowSequenceFeatureSelectionCB selected_signal;

  guint signals[SEQUENCE_LAST_SIGNAL];
} zmapWindowSequenceFeatureClassStruct;



typedef struct _zmapWindowSequenceFeatureStruct
{
  zmapWindowCanvasItem __parent__;

  /* Frame is needed for 3 frame translation objects which display peptides in 3 frames. We need
   * to know which frame we are !! */
  ZMapFrame frame ;

  /* We display sequence where  start > 1  so need offset to correct coords for text display. */
  double offset ;


  struct
  {
    double zoom_x, zoom_y;

    /* world coords scroll region, also sets min/max x/y positions */
    double scr_x1, scr_y1, scr_x2, scr_y2;

  } float_settings;


  struct
  {
    unsigned int min_x_set : 1;
    unsigned int max_x_set : 1;
    unsigned int min_y_set : 1;
    unsigned int max_y_set : 1;
    
    unsigned int float_axis : 2;
  } float_flags;

} zmapWindowSequenceFeatureStruct ;


#endif /* ZMAP_WINDOW_SEQUENCE_FEATURE_I_H */
