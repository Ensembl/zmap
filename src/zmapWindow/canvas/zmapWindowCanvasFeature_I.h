/*  File: zmapWindowCanvasFeature_I.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2014-2015: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Private header to operations on features within a column.
 *
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_FEATURE_P_H
#define ZMAP_WINDOW_FEATURE_P_H


#include <zmapWindowCanvasFeature.h>


/* Function pointer indices. */
typedef enum
  {
    CANVAS_FEATURE_FUNC_EXTENT,
    CANVAS_FEATURE_FUNC_SUBPART,
    CANVAS_FEATURE_FUNC_N_FUNC
  } ZMapWindowCanvasFeatureFunc ;


/* Foocanvas positions are doubles and foocanvas does not define one of these. */
typedef struct ZMapWindowCanvasCanvasSpanStructType
{
  double start, end ;
} ZMapWindowCanvasCanvasSpanStruct, *ZMapWindowCanvasCanvasSpan ;


/* Canvas feature Class struct. */
typedef struct ZMapWindowCanvasFeatureClassStructType
{
  size_t struct_size[FEATURE_N_TYPE];

  GList *feature_free_list[FEATURE_N_TYPE] ;

} ZMapWindowCanvasFeatureClassStruct ;



/* Data about displayed subcols. When bumped the features in a column may be
 * displayed as separate subcolumns. */
typedef struct ZMapWindowCanvasSubColStructType
{
  GQuark subcol_id ;                                        /* "name" of subcol. */

  double offset ;                                           /* subcol offset in column. */

  double width ;                                            /* subcol width. */

} ZMapWindowCanvasSubColStruct, *ZMapWindowCanvasSubCol ;



/* This is common to all of our display features. */
typedef struct ZMapWindowCanvasBaseStructType
{
  zmapWindowCanvasFeatureType type;
  double y1, y2;                                            /* top, bottom of item (box or line) */
} ZMapWindowCanvasBaseStruct ;



/* minimal data for a simple line or box,
 * to handle text or more complex things need to extend this */
typedef struct _zmapWindowCanvasGraphicsStruct
{
  /* Common to all canvas structs. */
  zmapWindowCanvasFeatureType type;
  double y1, y2;    	/* top, bottom of item (box or line) */

  /* include enough to handle lines boxes text, maybe arcs too */
  /* anything more complex need to be derived from this */
  double x1, x2;
  long fill_val, outline_val;
  char *text;

  int flags;                                                /* See FEATURE_XXXX above. */

} zmapWindowCanvasGraphicsStruct;



/* minimal data struct to define a feature handle boxes as y1,y2 + width */
typedef struct _zmapWindowCanvasFeatureStruct
{
  /* Common to all canvas structs. */
  zmapWindowCanvasFeatureType type ;
  double y1, y2;    	/* top, bottom of item (box or line) */

  ZMapFeature feature ;

  //  GList *from;		/* the list node that holds the feature */
  /* refer to comment above zmapWindowCanvasFeatureset.c/zMapWindowFeaturesetItemRemoveFeature() */

  double score ;                                            /* determines feature width */

  /* ideally these could be ints but the canvas works with doubles */
  double width;
  double bump_offset;	/* for X coord  (left hand side of sub column */

  int bump_col;		/* for calculating sub-col before working out width */

  long flags;				/* non standard display option eg selected */

  ZMapWindowCanvasFeature left,right;	/* for exons and alignments, NULL for simple features */

  /* NULL if splice highlighting is off, contains positions of all places highlights
   * need to be drawn within feature if highlighting is on. */
  GList *splice_positions ;

  /* NOT SURE IF WE NEED A SEPARATE LIST, THESE TWO THING MIGHT BE MUTUALLY EXCLUSIVE.... */
  GList *non_splice_positions ;



} zmapWindowCanvasFeatureStruct;



void zmapWindowCanvasFeatureFree(gpointer thing) ;



#endif /* ZMAP_WINDOW_FEATURE_P_H */
